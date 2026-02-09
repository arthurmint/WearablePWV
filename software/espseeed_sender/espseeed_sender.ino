  #include <Arduino.h>
  #include <WiFi.h>
  #include <esp_now.h>
  #include <esp_wifi.h>

  // -------------------------
  // Configuration
  // -------------------------

  // MAC address of the GATEWAY ESP32 (the one hosting the website)
  uint8_t gatewayMac[] = { 0x58, 0x8C, 0x81, 0xA8, 0x40, 0x7C };

  // Must match the AP/ESP-NOW channel used by the gateway
  const uint8_t WIFI_CHANNEL = 6;

  // Max length of a message we send over ESP-NOW
  const size_t MAX_MESSAGE_LEN = 200;


  // -------------------------
  // Serial input buffer
  // -------------------------

  String inputBuffer;


  // -------------------------
  // ESP-NOW setup
  // -------------------------

  bool initEspNowSender() {
    // We operate as a station only (no AP, no router connection)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // just to be sure

    // Set Wi-Fi channel to match the gateway's channel
    // (Required so ESP-NOW frames meet on the same channel)
    esp_wifi_set_promiscuous(true);   // needed for channel change on some chips
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    Serial.print("Using WiFi channel: ");
    Serial.println(WIFI_CHANNEL);

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return false;
    }

    // Configure the peer (gateway)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayMac, 6);
    peerInfo.channel = WIFI_CHANNEL;    // same channel
    peerInfo.ifidx   = WIFI_IF_STA;     // use station interface (matches STA MAC)
    peerInfo.encrypt = false;           // no encryption for now


    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add ESP-NOW peer (gateway)");
      return false;
    }

    Serial.println("ESP-NOW sender initialized, peer added.");
    return true;
  }


  // -------------------------
  // Helper: send a text message
  // -------------------------

  void sendEspNowMessage(const String& msg) {
    if (msg.length() == 0) return;

    size_t len = msg.length();
    if (len > MAX_MESSAGE_LEN) {
      Serial.println("Message too long, truncating.");
      len = MAX_MESSAGE_LEN;
    }

    esp_err_t result = esp_now_send(gatewayMac,
                                    (const uint8_t*)msg.c_str(),
                                    len);

    if (result == ESP_OK) {
      Serial.print("Sent: ");
      Serial.println(msg);
    } else {
      Serial.print("Error sending message, err=");
      Serial.println((int)result);
    }
  }


  // -------------------------
  // Setup & loop
  // -------------------------

  void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("ESP-NOW Sender (XIAO ESP32-C3)");
    Serial.println("Type a message and press ENTER to send it to the gateway.");
    Serial.println();

    if (!initEspNowSender()) {
      Serial.println("ESP-NOW init failed. Check wiring / board / core.");
    }
  }

  void loop() {
    // Read characters from Serial until newline, then send as one message
    while (Serial.available() > 0) {
      char c = Serial.read();

      // Handle CRLF (\r\n) gracefully
      if (c == '\r') {
        // ignore, wait for '\n'
        continue;
      }

      if (c == '\n') {
        // End of line -> send the message
        String msg = inputBuffer;
        inputBuffer = "";  // clear for next input

        msg.trim();  // remove any extra whitespace
        if (msg.length() > 0) {
          sendEspNowMessage(msg);
        } else {
          Serial.println("Empty line, nothing sent.");
        }
      } else {
        // Add character to buffer if we have space
        if (inputBuffer.length() < MAX_MESSAGE_LEN) {
          inputBuffer += c;
        } else {
          // If buffer full, ignore extra chars
        }
      }
    }

    // Small delay to avoid spinning unnecessarily
    delay(5);
  }
