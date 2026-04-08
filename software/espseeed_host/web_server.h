// web_server.h (FIXED)
#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_wifi.h>

#include "config.h"
#include "web_pages.h"

// Single instances for server and websocket
inline AsyncWebServer& getServer() {
  static AsyncWebServer server(80);
  return server;
}

inline AsyncWebSocket& getWebSocket() {
  static AsyncWebSocket ws("/ws");
  return ws;
}

inline void onWebSocketEvent(AsyncWebSocket *server,
                             AsyncWebSocketClient *client,
                             AwsEventType type,
                             void *arg,
                             uint8_t *data,
                             size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected\n", client->id());
    client->text("ESP32 → WebSocket connected 👋");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg;
      msg.reserve(len + 1);
      for (size_t i = 0; i < len; i++) {
        msg += (char)data[i];
      }
      Serial.printf("Browser → %s\n", msg.c_str());
    }
  }
}


// Start the ESP32 as a Wi-Fi Access Point
inline void initWiFiAP() {
  // IMPORTANT: Set mode to AP+STA before starting AP
  WiFi.mode(WIFI_AP_STA);
  delay(100);  // Small delay for mode to take effect
  
  //esp_wifi_set_max_tx_power(84);
  // Start the AP with specific channel
  bool apStarted = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_CHANNEL);

  if (!apStarted) {
    Serial.println("Failed to start Access Point!");
    return;
  }

  // CRITICAL FIX: Explicitly set the WiFi channel for both AP and STA
  // This ensures ESP-NOW works on the same channel
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  IPAddress ip = WiFi.softAPIP();
  
  // Verify the channel
  uint8_t currentChannel;
  wifi_second_chan_t secondChan;
  esp_wifi_get_channel(&currentChannel, &secondChan);
  
  Serial.println("====================================");
  Serial.println("Access Point started");
  Serial.print("SSID: ");
  Serial.println(WIFI_AP_SSID);
  Serial.print("Password: ");
  Serial.println(WIFI_AP_PASSWORD);
  Serial.print("Channel: ");
  Serial.println(currentChannel);
  Serial.print("AP IP address: ");
  Serial.println(ip);
  Serial.println("Connect to this Wi-Fi and open: http://192.168.4.1/");
  Serial.println("====================================");
}

// Initialize AP + HTTP server + WebSocket
inline void initWebServer() {
  initWiFiAP();

  AsyncWebServer& server = getServer();
  AsyncWebSocket& ws = getWebSocket();

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Root path: serve the page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  // 404 handler
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Not Found");
  });

  server.begin();
  Serial.println("HTTP + WebSocket server started");
}

// To be called regularly from loop()
inline void handleWebServer() {
  getWebSocket().cleanupClients();
}