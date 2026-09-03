
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <ESPmDNS.h>

#include "wifi_credentials.h"

bool typingInProgress = false;

int typingSpeedMs = 80;     // base delay between keystrokes
int errorPercent = 5;      // percent chance of typo

bool keepAliveEnabled = false;
unsigned long lastKeepAliveTime = 0;
const unsigned long KEEP_ALIVE_INTERVAL = 5UL * 60UL * 1000UL; // 5 minutes

USBHIDKeyboard Keyboard;

WebServer server(80);

String textBuffer;
bool startTyping = false;

// ================= HTML ================= 

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 USB HID Keyboard</title>
  <style>
    body { font-family: Arial; }
    textarea { width: 100%; font-size: 16px; }
    .slider-container { margin-top: 10px; }
    label { display: block; margin-top: 10px; }
  </style>

  <script>
    document.addEventListener("DOMContentLoaded", function() {
      const textarea = document.getElementById("inputText");
      const speedSlider = document.getElementById("speed");
      const errorSlider = document.getElementById("error");
      const speedValue = document.getElementById("speedValue");
      const errorValue = document.getElementById("errorValue");

      speedValue.textContent = speedSlider.value;
      errorValue.textContent = errorSlider.value;

      speedSlider.oninput = () => speedValue.textContent = speedSlider.value;
      errorSlider.oninput = () => errorValue.textContent = errorSlider.value;

      textarea.addEventListener("keydown", function(e) {
        if (e.key === "Enter" && !e.shiftKey) {
          e.preventDefault();
          sendText();
        }
      });

      function sendText() {
        const text = textarea.value;
        if (!text || text.trim().length === 0) return;

        const params =
          "text=" + encodeURIComponent(text) +
          "&speed=" + speedSlider.value +
          "&error=" + errorSlider.value;

        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/send", true);
        xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        xhr.send(params);

        textarea.value = "";
        textarea.focus();
      }
    });
  </script>
</head>

<body>
  <h2>ESP32 USB HID Keyboard</h2>

  <textarea id="inputText" rows="10"
    placeholder="Enter = send | Shift+Enter = newline"></textarea>

  <div class="slider-container">
    <label>
      Typing Speed (ms): <span id="speedValue"></span>
      <input type="range" id="speed" min="20" max="200" value="80">
    </label>

    <label>
      Error %: <span id="errorValue"></span>%
      <input type="range" id="error" min="0" max="20" value="5">
    </label>
  </div>
</body>
</html>
)rawliteral";


// ================= HUMAN TYPING ================= 

void humanDelay() {
  int jitter = random(-typingSpeedMs / 3, typingSpeedMs / 3);
  delay(max(10, typingSpeedMs + jitter));
}

void typeChar(char c) {
  Keyboard.print(c);
}

void backspace() {
  Keyboard.press(KEY_BACKSPACE);
  delay(10);
  Keyboard.release(KEY_BACKSPACE);
}

void humanType(const String& text) {
  for (int i = 0; i < text.length(); i++) {
    humanDelay();

    char c = text[i];

    // 5% typo chance only for letters
    if (random(0, 100) < errorPercent && isalpha(c)) {
      Keyboard.print('x');
      delay(random(40, 120));
      Keyboard.press(KEY_BACKSPACE);
      Keyboard.release(KEY_BACKSPACE);
    }

    if (c == '\n') {
      Keyboard.press(KEY_RETURN);
      delay(10);
      Keyboard.release(KEY_RETURN);
    } else {
      typeChar(c);
    }

    // Natural pauses
    if (c == '.' || c == ',')
      delay(random(300, 700));
    if (c == '\n')
      delay(random(500, 1000));
  }
}

// ================= WEB HANDLERS ================= 

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSend() {
  textBuffer = server.arg("text");

  if (server.hasArg("speed")) {
    typingSpeedMs = server.arg("speed").toInt();
  }

  if (server.hasArg("error")) {
    errorPercent = server.arg("error").toInt();
  }

  Serial.printf(
    "Received text (%d chars), speed=%dms, error=%d%%\n",
    textBuffer.length(),
    typingSpeedMs,
    errorPercent
  );

  startTyping = true;
  server.send(200, "text/plain", "OK");
}

// ================= SETUP ================= 

void setup() {
  delay(1000);

  Serial.begin(115200);       // Start the Serial Monitor
  while (!Serial) { }         // Wait for Serial to be ready (optional)
  
  Serial.println("Booting ESP32-S3...");
  
  // USB HID
  Keyboard.begin();
  USB.begin();

  WiFi.setHostname("human-typing-keyboard");

  //start wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi ");

  //keep trying wifi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      retries++;
      if (retries > 20) {  // timeout after 10 seconds
          Serial.println("\nFailed to connect to Wi-Fi!");
          break;
      }
  }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi Connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
  }

  //turn on the web server
  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.begin();

  // setup http://human-typing-keyboard.local
  MDNS.begin("human-typing-keyboard");
}

// ================= LOOP ================= 

void loop() {
  server.handleClient();

  if (startTyping && !typingInProgress) {
    typingInProgress = true;
    String textToType = textBuffer;  // copy
    startTyping = false;              // reset immediately
    delay(500);                       // optional focus delay
    humanType(textToType);
    typingInProgress = false;
  }
}
