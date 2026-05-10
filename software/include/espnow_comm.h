// espnow_comm.h  —  Module A (sender only)
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ─────────────────────────────────────────
// Pulse packet
// Transmitted to Module B the instant a PPG peak is detected.
// ─────────────────────────────────────────
struct PulsePacket {
    uint32_t t1_ms;  // millis() on Module A at the detected peak
    uint32_t seq;    // monotonically increasing — lets Module B detect dropped packets
};

// ─────────────────────────────────────────
// Peer (Module B) info
// ─────────────────────────────────────────
static esp_now_peer_info_t g_peerInfo = {};

// ─────────────────────────────────────────
// Sender init
// Call once in setup().
// peerMac = MAC address of Module B.
// channel must match the channel Module B is listening on.
// ─────────────────────────────────────────
inline bool initEspNowSender(const uint8_t peerMac[6], uint8_t channel = 1) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return false;
    }

    memcpy(g_peerInfo.peer_addr, peerMac, 6);
    g_peerInfo.channel = channel;
    g_peerInfo.encrypt = false;

    if (esp_now_add_peer(&g_peerInfo) != ESP_OK) {
        Serial.println("ESP-NOW: failed to add peer");
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
// Send a pulse detection event
// Call this once per detected peak.
// t1_ms : millis() timestamp of the peak (from PPGPeakDetector)
// seq   : caller-managed counter, increment after each successful send
// ─────────────────────────────────────────
inline bool espNowSendPulse(uint32_t t1_ms, uint32_t seq) {
    PulsePacket pkt;
    pkt.t1_ms = t1_ms;
    pkt.seq   = seq;

    esp_err_t result = esp_now_send(g_peerInfo.peer_addr,
                                    (const uint8_t*)&pkt,
                                    sizeof(PulsePacket));
    if (result != ESP_OK) {
        Serial.printf("espNowSendPulse failed (err %d)\n", result);
        return false;
    }
    Serial.printf("→ Pulse sent  t1=%lu ms  seq=%lu\n", t1_ms, seq);
    return true;
}