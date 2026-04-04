// espnow_comm.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// -----------------------------
// State for received messages
// -----------------------------

// Last message received from the other ESP32 via ESP-NOW
static String g_lastEspNowMessage;

// Flag to indicate that a new message has arrived
static bool g_hasNewEspNowMessage = false;

// Optional: store last sender MAC for debugging
static uint8_t g_lastSenderMac[6] = {0};


// ------------------------------------
// ESP-NOW receive callback (NEW API)
// ------------------------------------
// New signature: const esp_now_recv_info *info, const uint8_t *incomingData, int len
inline void onEspNowDataRecv(const esp_now_recv_info *info,
                             const uint8_t *incomingData,
                             int len) {
  // Get sender MAC from info struct
  const uint8_t *mac = info->src_addr;
  memcpy(g_lastSenderMac, mac, 6);

  // Copy incoming bytes into a String (assume sender sends text)
  String msg;
  msg.reserve(len + 1);
  for (int i = 0; i < len; i++) {
    msg += static_cast<char>(incomingData[i]);
  }

  g_lastEspNowMessage = msg;
  g_hasNewEspNowMessage = true;

  // Debug output on serial
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  Serial.println("╔════════════════════════════════════════╗");
  Serial.print("║ ESP-NOW MESSAGE RECEIVED from ");
  Serial.print(macStr);
  Serial.println(" ║");
  Serial.print("║ Content: ");
  Serial.print(msg);
  Serial.println(" ║");
  Serial.println("╚════════════════════════════════════════╝");
}


// ------------------------------------
// Initialisation of ESP-NOW (gateway)
// ------------------------------------
inline bool initEspNowReceiver() {
  // WiFi.mode() and AP are already configured in the web_server code.
  // Verify we're in the right mode
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  
  Serial.print("WiFi mode: ");
  if (mode == WIFI_MODE_AP) Serial.println("AP only");
  else if (mode == WIFI_MODE_STA) Serial.println("STA only");
  else if (mode == WIFI_MODE_APSTA) Serial.println("AP+STA (correct for ESP-NOW + web server)");
  else Serial.println("Unknown");

  // Get and display current channel
  uint8_t currentChannel;
  wifi_second_chan_t secondChan;
  esp_wifi_get_channel(&currentChannel, &secondChan);
  Serial.print("Current WiFi channel: ");
  Serial.println(currentChannel);

  // Initialize ESP-NOW
  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.print("Error initializing ESP-NOW, error code: ");
    Serial.println(initResult);
    return false;
  }

  // Register the receive callback using the new function signature
  esp_err_t cbResult = esp_now_register_recv_cb(onEspNowDataRecv);
  if (cbResult != ESP_OK) {
    Serial.print("Error registering ESP-NOW callback, error code: ");
    Serial.println(cbResult);
    return false;
  }

  Serial.println("✓ ESP-NOW initialized successfully as receiver");
  Serial.println("✓ Listening for messages on channel " + String(currentChannel));
  
  return true;
}


// -------------------------------------------------
// Interface functions for other modules / main loop
// -------------------------------------------------

// Check if a new ESP-NOW message has arrived since last read
inline bool espNowHasNewMessage() {
  return g_hasNewEspNowMessage;
}

// Get the last ESP-NOW message.
// If clearFlag is true, the "new message" flag is cleared.
inline String espNowGetLastMessage(bool clearFlag = true) {
  String msg = g_lastEspNowMessage;
  if (clearFlag) {
    g_hasNewEspNowMessage = false;
  }
  return msg;
}

// Optional: get last sender MAC as a String (for display/logging)
inline String espNowGetLastSenderMac() {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_lastSenderMac[0], g_lastSenderMac[1], g_lastSenderMac[2],
           g_lastSenderMac[3], g_lastSenderMac[4], g_lastSenderMac[5]);
  return String(macStr);
}