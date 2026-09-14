
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <ESPmDNS.h>

#include "wifi_credentials.h"

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
WebSocketsServer webSocket(81);

extern const char htmlPage[] asm("_binary_src_index_html_start");

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

uint8_t jsKeyCodeToHid(uint8_t keyCode) {
    if (keyCode >= 65 && keyCode <= 90)
        return keyCode - 65 + 0x04;
    if (keyCode >= 49 && keyCode <= 57)
        return keyCode - 49 + 0x1E;
    if (keyCode == 48)
        return 0x27;

    switch (keyCode) {
        case 13: return 0x28; // Enter
        case 27: return 0x29; // Escape
        case 8:  return 0x2A; // Backspace
        case 9:  return 0x2B; // Tab
        case 32: return 0x2C; // Space
    }
    return 0;
}

void webSocketEvent(
    uint8_t client,
    WStype_t type,
    uint8_t *payload,
    size_t length)
{
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("WebSocket Client %u verbunden\n", client);
            //Serial.printf("length: %d\n", length);
            //Serial.printf("Byte 0: %d\n", payload[0]);
            break;

        case WStype_DISCONNECTED:
            Serial.printf("WebSocket Client %u getrennt\n", client);
            break;

        case WStype_BIN: {
            Serial.printf("length: %d\n", length);
            Serial.print("KeyCode: ");
            KeyReport report = {};
            for (size_t i = 0; i < length; i++) {
              int keyCode = payload[i];
              Serial.printf("%d, ", keyCode);
              switch (keyCode) {
                  case 16: // Shift
                      report.modifiers |= 0x02;
                      break;
                  case 17: // Ctrl
                      report.modifiers |= 0x01;
                      break;
                  case 18: // Alt
                      report.modifiers |= 0x04;
                      break;
                  case 91: // Meta
                      report.modifiers |= 0x08;
                      break;
                  default:
                    report.keys[0] = jsKeyCodeToHid(keyCode);
                    break;
              }
            }
            Serial.print("\b\b\n");
            Keyboard.sendReport(&report);
            delay(10);
            Keyboard.releaseAll();
            break;
        }

        default:
            break;
    }
}

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

  // turn on the web server
  server.on("/", handleRoot);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // setup http://human-typing-keyboard.local
  MDNS.begin("human-typing-keyboard");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
