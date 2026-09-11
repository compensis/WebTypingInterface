
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <ESPmDNS.h>

#include "wifi_credentials.h"

bool typingInProgress = false;

class DelayedKeyboard: public USBHIDKeyboard {
public:
  template <typename T>
  size_t tap(T k) {
    // Press and release key (if press was successfull)
    auto ret = press(k);
    // Wait for the key to be sent
    delay(10);
    if(ret){
      release(k);
    }
    return ret;
  }
} Keyboard;

WebServer server(80);

String textBuffer;
bool startTyping = false;

// ================= HTML ================= 

const char* htmlPage =
#include "index.html"

void type(const String& text) {
  for (int i = 0; i < text.length(); i++) {
    char c = text[i];

    if (c == '\n') {
      Keyboard.press(KEY_RETURN);
      delay(10);
      Keyboard.release(KEY_RETURN);
    } else {
      Keyboard.tap(c);
    }
  }
}

// ================= WEB HANDLERS ================= 

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSend() {
  textBuffer = server.arg("plain");

  Serial.printf("Received text (%d chars):\n", textBuffer.length());
  Serial.println(textBuffer);

  startTyping = true;
  server.send(200, "text/plain", "OK");
}

// ================= SETUP ================= 

void setup() {
  delay(3000);

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
    type(textToType);
    typingInProgress = false;
  }
}
