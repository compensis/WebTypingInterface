
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "tusb.h"
#include <ESPmDNS.h>

#include "wifi_credentials.h"

USBHIDKeyboard Keyboard;
bool announcedUsbConnected = false;

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
  size_t length
) {
  switch (type) {
  case WStype_CONNECTED:
    Serial.printf("WebSocket Client %u verbunden\n", client);
    break;

  case WStype_DISCONNECTED:
    Serial.printf("WebSocket Client %u getrennt\n", client);
    announcedUsbConnected = false;
    break;

  case WStype_BIN:
    Serial.printf("WebSocket Client receives %d bytes\n", length);
    if (length == 2) {
      const uint8_t modifiers = payload[0];
      const uint8_t keyCode = payload[1];
      Serial.printf("modifiers: %d\n", modifiers);
      Serial.printf("keyCode: %d\n", keyCode);
      
      KeyReport report = {};
      report.modifiers = modifiers;
      report.keys[0] = keyCode;

      Keyboard.sendReport(&report);
      delay(10);
      Keyboard.releaseAll();
      break;
    }
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

  const char* hostName = "web-typing-interface";
  WiFi.setHostname(hostName);

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

  // setup http://<hostname>.local
  MDNS.begin(hostName);
  Serial.printf("http://%s.local\n", hostName);
}

void loop() {
  bool usbConnected = tud_mounted();
  if (usbConnected != announcedUsbConnected) {
    if (webSocket.connectedClients() > 0) {
      announcedUsbConnected = usbConnected;
      const char* msg = usbConnected ? "connected" : "disconnected";
      Serial.printf("USB %s\n", msg);
      webSocket.broadcastTXT(msg);
    }
  }

  server.handleClient();
  webSocket.loop();
}
