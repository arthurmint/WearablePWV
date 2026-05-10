#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ─────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────
const uint8_t WIFI_CHANNEL = 6;

// ─────────────────────────────────────────
// PulsePacket — must be identical to Module A's definition
// ─────────────────────────────────────────
struct PulsePacket {
    uint32_t t1_ms;
    uint32_t seq;
};

// ─────────────────────────────────────────
// ESP-NOW receive callback
// ─────────────────────────────────────────
void onEspNowDataRecv(const esp_now_recv_info *info,
                      const uint8_t           *incomingData,
                      int                      len) {
    if (len != sizeof(PulsePacket)) {
        Serial.printf("⚠ Unexpected payload size: got %d, expected %d\n",
                      len, (int)sizeof(PulsePacket));
        return;
    }

    PulsePacket pkt;
    memcpy(&pkt, incomingData, sizeof(PulsePacket));

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    Serial.println("╔════════════════════════════════════════╗");
    Serial.printf ("║ Pulse packet from %s\n", macStr);
    Serial.printf ("║  t1  = %lu ms\n", pkt.t1_ms);
    Serial.printf ("║  seq = %lu\n",    pkt.seq);
    Serial.println("╚════════════════════════════════════════╝");
}

// ─────────────────────────────────────────
// Setup
// ─────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Module B — pulse timestamp receiver");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.print("Module B MAC: ");
    Serial.println(WiFi.macAddress());

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed — halting");
        while (true) delay(1000);
    }

    if (esp_now_register_recv_cb(onEspNowDataRecv) != ESP_OK) {
        Serial.println("ESP-NOW callback registration failed — halting");
        while (true) delay(1000);
    }

    uint8_t ch; wifi_second_chan_t sc;
    esp_wifi_get_channel(&ch, &sc);
    Serial.printf("✓ ESP-NOW receiver ready on channel %u\n", ch);
}

// ─────────────────────────────────────────
// Loop
// ─────────────────────────────────────────
void loop() {
    // Nothing to do — all data arrives via the callback
    delay(10);
}