// espnow_comm.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ─────────────────────────────────────────
// Shared data struct (used by both devices)
// ─────────────────────────────────────────
struct PPGPacket {
  uint16_t pd1;
  uint16_t pd2;
  uint16_t pd3;
  uint16_t pd4;
  uint32_t timestamp_ms;  // millis() on the sender for latency tracking
};

// ─────────────────────────────────────────
// Receiver state
// ─────────────────────────────────────────
static PPGPacket  g_lastPPGPacket        = {0};
static bool       g_hasNewEspNowMessage  = false;
static uint8_t    g_lastSenderMac[6]     = {0};

// Keep the original String-based accessor for web_server compatibility
static String g_lastEspNowMessage;


// ─────────────────────────────────────────
// Receive callback
// ─────────────────────────────────────────
inline void onEspNowDataRecv(const esp_now_recv_info *info,
                             const uint8_t *incomingData,
                             int len) {
  memcpy(g_lastSenderMac, info->src_addr, 6);

  if (len == sizeof(PPGPacket)) {
    memcpy(&g_lastPPGPacket, incomingData, sizeof(PPGPacket));
    g_hasNewEspNowMessage = true;

    // Also populate the legacy String for the web viewer
    char buf[64];
    snprintf(buf, sizeof(buf), "PD1:%u PD2:%u PD3:%u PD4:%u",
             g_lastPPGPacket.pd1, g_lastPPGPacket.pd2,
             g_lastPPGPacket.pd3, g_lastPPGPacket.pd4);
    g_lastEspNowMessage = String(buf);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    Serial.println("╔════════════════════════════════════════╗");
    Serial.print  ("║ PPG PACKET from "); Serial.println(macStr);
    Serial.printf ("║  PD1=%-5u  PD2=%-5u  PD3=%-5u  PD4=%-5u\n",
                   g_lastPPGPacket.pd1, g_lastPPGPacket.pd2,
                   g_lastPPGPacket.pd3, g_lastPPGPacket.pd4);
    Serial.printf ("║  Sender timestamp: %lu ms\n", g_lastPPGPacket.timestamp_ms);
    Serial.println("╚════════════════════════════════════════╝");
  } else {
    Serial.printf("ESP-NOW: unexpected payload size %d (expected %d)\n",
                  len, sizeof(PPGPacket));
  }
}


// ─────────────────────────────────────────
// Receiver init (gateway / display device)
// ─────────────────────────────────────────
inline bool initEspNowReceiver() {
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  Serial.print("WiFi mode: ");
  if      (mode == WIFI_MODE_AP)    Serial.println("AP only");
  else if (mode == WIFI_MODE_STA)   Serial.println("STA only");
  else if (mode == WIFI_MODE_APSTA) Serial.println("AP+STA");
  else                              Serial.println("Unknown");

  uint8_t ch; wifi_second_chan_t sc;
  esp_wifi_get_channel(&ch, &sc);
  Serial.printf("Current WiFi channel: %u\n", ch);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
  if (esp_now_register_recv_cb(onEspNowDataRecv) != ESP_OK) {
    Serial.println("ESP-NOW callback registration failed");
    return false;
  }

  Serial.println("✓ ESP-NOW receiver ready on channel " + String(ch));
  return true;
}


// ─────────────────────────────────────────
// Sender init (sensor device)
// Call this INSTEAD of initEspNowReceiver()
// peerMac = MAC address of the receiver ESP32
// ─────────────────────────────────────────
static esp_now_peer_info_t g_peerInfo = {};

inline bool initEspNowSender(const uint8_t peerMac[6], uint8_t channel = 1) {
  // Sensor device uses plain STA mode (no AP needed)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Lock onto the same channel as the receiver
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW sender init failed");
    return false;
  }

  memcpy(g_peerInfo.peer_addr, peerMac, 6);
  g_peerInfo.channel = channel;
  g_peerInfo.encrypt = false;

  if (esp_now_add_peer(&g_peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    return false;
  }

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           peerMac[0], peerMac[1], peerMac[2],
           peerMac[3], peerMac[4], peerMac[5]);
  Serial.printf("✓ ESP-NOW sender ready → peer %s (ch %u)\n", macStr, channel);
  return true;
}


// ─────────────────────────────────────────
// Send a PPG reading
// ─────────────────────────────────────────
inline bool espNowSendPPG(uint16_t pd1, uint16_t pd2,
                           uint16_t pd3, uint16_t pd4) {
  PPGPacket pkt;
  pkt.pd1          = pd1;
  pkt.pd2          = pd2;
  pkt.pd3          = pd3;
  pkt.pd4          = pd4;
  pkt.timestamp_ms = millis();

  esp_err_t result = esp_now_send(g_peerInfo.peer_addr,
                                  (const uint8_t*)&pkt,
                                  sizeof(PPGPacket));
  if (result != ESP_OK) {
    Serial.printf("ESP-NOW send failed: %d\n", result);
    return false;
  }
  return true;
}


// ─────────────────────────────────────────
// Receiver accessors (unchanged)
// ─────────────────────────────────────────
inline bool    espNowHasNewMessage()               { return g_hasNewEspNowMessage; }
inline PPGPacket espNowGetLastPacket(bool clear = true) {
  if (clear) g_hasNewEspNowMessage = false;
  return g_lastPPGPacket;
}
inline String  espNowGetLastMessage(bool clearFlag = true) {
  if (clearFlag) g_hasNewEspNowMessage = false;
  return g_lastEspNowMessage;
}
inline String espNowGetLastSenderMac() {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_lastSenderMac[0], g_lastSenderMac[1], g_lastSenderMac[2],
           g_lastSenderMac[3], g_lastSenderMac[4], g_lastSenderMac[5]);
  return String(macStr);
}