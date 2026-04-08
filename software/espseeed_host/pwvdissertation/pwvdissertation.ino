// pwvdissertation.ino
#include <WiFi.h>
#include <Arduino.h>
#include "config.h"
#include "web_pages.h"
#include "web_server.h"
#include "espnow_comm.h"


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("Booting ESP32 XIAO C3 Gateway (WebSocket + ESP-NOW receiver)...");

  // 1) Start AP + HTTP + WebSocket
  initWebServer();

  // Print MAC addresses so you know which one to use on the sender
  Serial.println("====================================");
  Serial.print("STA MAC (for ESP-NOW): ");
  Serial.println(WiFi.macAddress());        // THIS is what sender should target
  
  Serial.print("AP MAC (WiFi hotspot): ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.println("====================================");
  Serial.println("IMPORTANT: Update sender code with the STA MAC address above!");
  Serial.println("====================================");

  // 2) Start ESP-NOW in receive mode
  if (!initEspNowReceiver()) {
    Serial.println("ESP-NOW init failed. Gateway will still serve the web page, but no radio data.");
  } else {
    Serial.println("ESP-NOW receiver ready and listening...");
  }


}

void loop() {
  handleWebServer();

  // Normal ESP-NOW messages → broadcast to browsers
  if (espNowHasNewMessage()) {
    String msg    = espNowGetLastMessage(true);
    String sender = espNowGetLastSenderMac();
    String line   = "MSG:[" + sender + "] " + msg;
    getWebSocket().textAll(line);
    Serial.println("✓ WS broadcast: " + line);
  }

  // Latency reports → send as a distinct message type
  if (espNowHasNewLatency()) {
    float rtt = espNowGetLastLatency(true);
    // Format: "LAT:<rtt_ms>" — the browser parses this separately
    String line = "LAT:" + String(rtt, 1);
    getWebSocket().textAll(line);
    Serial.printf("✓ WS latency broadcast: %s ms RTT\n", String(rtt, 1).c_str());
  }

  delay(5);
}
