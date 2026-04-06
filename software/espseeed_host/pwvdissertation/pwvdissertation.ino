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
  // Housekeeping for WebSockets
  handleWebServer();
  // If a new ESP-NOW message has arrived, broadcast it to all connected browsers
  if (espNowHasNewMessage()) {
    String msg = espNowGetLastMessage(true);   // clears the "new" flag
    String sender = espNowGetLastSenderMac();

    
    String line = "ESP-NOW → [" + sender + "] " + msg;

    // Send to all connected WebSocket clients
    getWebSocket().textAll(line);

    // Also print to Serial for debugging
    Serial.println("✓ Broadcast to WebSocket: " + line);
  }

  // Small delay to avoid spinning too fast
  delay(5);
}
