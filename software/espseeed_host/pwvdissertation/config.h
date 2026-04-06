// config.h
#pragma once
#include <Arduino.h>

// ---------- Wi-Fi Access Point configuration ----------

// The Wi-Fi network that the ESP32 will create.
// Connect your phone/laptop to this SSID + password.
static const char* WIFI_AP_SSID     = "ESP32_Gateway";
static const char* WIFI_AP_PASSWORD = "esp32password";

// (Optional) Wi-Fi channel for the AP (1–13).
// later want ESP-NOW on the same channel.
static const uint8_t WIFI_CHANNEL = 1;
