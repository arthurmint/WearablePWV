// espnow_comm.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static String g_lastEspNowMessage;
static bool g_hasNewEspNowMessage = false;
static uint8_t g_lastSenderMac[6] = {0};

// Separate storage for latency reports
static float g_lastLatencyMs = -1.0f;
static bool g_hasNewLatency = false;

inline bool ensurePeerRegistered(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return true;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;         // 0 = current channel
  peer.ifidx   = WIFI_IF_AP;
  peer.encrypt = false;

  esp_err_t r = esp_now_add_peer(&peer);
  if (r != ESP_OK) {
    Serial.printf("Failed to add peer, err=%d\n", (int)r);
    return false;
  }
  Serial.println("✓ Sender registered as ESP-NOW peer");
  return true;
}

inline void onEspNowDataRecv(const esp_now_recv_info *info,
                             const uint8_t *incomingData,
                             int len) {
  const uint8_t *mac = info->src_addr;
  memcpy(g_lastSenderMac, mac, 6);

  String msg;
  msg.reserve(len + 1);
  for (int i = 0; i < len; i++) msg += static_cast<char>(incomingData[i]);

  // --- PING handling ---
  if (msg.startsWith("PING:")) {
    // Echo timestamp straight back as PONG
    String pong = "PONG:" + msg.substring(5);
    if (ensurePeerRegistered(mac)) {
      esp_now_send(mac, (const uint8_t*)pong.c_str(), pong.length());
    }
    Serial.println("↩ PONG sent for " + msg);
    return;   // don't expose PING to main loop
  }

  // --- Latency report from sender ---
  if (msg.startsWith("LAT:")) {
    g_lastLatencyMs = msg.substring(4).toFloat();
    g_hasNewLatency = true;
    Serial.printf("📶 Latency report: %.1f ms RTT\n", g_lastLatencyMs);
    return;   // handled separately
  }

  // --- Normal message ---
  g_lastEspNowMessage = msg;
  g_hasNewEspNowMessage = true;

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("ESP-NOW from %s: %s\n", macStr, msg.c_str());
}

inline bool initEspNowReceiver() {
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  if (mode != WIFI_MODE_APSTA) {
    Serial.println("Warning: expected APSTA mode for ESP-NOW + web server");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }
  if (esp_now_register_recv_cb(onEspNowDataRecv) != ESP_OK) {
    Serial.println("Error registering ESP-NOW callback");
    return false;
  }

  Serial.println("✓ ESP-NOW receiver ready");
  return true;
}

// --- Public interface ---

inline bool espNowHasNewMessage() { return g_hasNewEspNowMessage; }

inline String espNowGetLastMessage(bool clearFlag = true) {
  String msg = g_lastEspNowMessage;
  if (clearFlag) g_hasNewEspNowMessage = false;
  return msg;
}

inline bool espNowHasNewLatency() { return g_hasNewLatency; }

inline float espNowGetLastLatency(bool clearFlag = true) {
  float v = g_lastLatencyMs;
  if (clearFlag) g_hasNewLatency = false;
  return v;
}

inline String espNowGetLastSenderMac() {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_lastSenderMac[0], g_lastSenderMac[1], g_lastSenderMac[2],
           g_lastSenderMac[3], g_lastSenderMac[4], g_lastSenderMac[5]);
  return String(macStr);
}