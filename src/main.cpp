
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <ESPmDNS.h>

#include "wifi_credentials.h"

USBHIDKeyboard Keyboard;

WebServer server(80);
WebSocketsServer webSocket(81);

extern const char htmlPage[] asm("_binary_src_index_html_start");

void handleRoot() {
  server.send(200, "text/html", htmlPage);
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
      if (length == 2) {
        const uint8_t modifiers = payload[0];
        const uint8_t keyCode = payload[1];
        Serial.printf("modifiers: %d\n", modifiers);
        Serial.printf("keyCode: %d\n, ", keyCode);
        
        KeyReport report = {};
        report.modifiers = modifiers;
        report.keys[0] = keyCode;

        Keyboard.sendReport(&report);
        delay(10);
        Keyboard.releaseAll();
        break;
      }
    }
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
