/*
 * EspNowCom.ino – Encrypted ESP-NOW (Head ESP8266 ↔ Tail ESP32)
 *
 * BENCH SETUP:
 *  1. Flash both boards, open Serial 115200.
 *  2. Note each board's printed MAC.
 *  3. Put Tail MAC into TAIL_PEER_MAC below; put Head MAC into Tail's HEAD_PEER_MAC.
 *  4. Keys PMK/LMK must match Tail exactly.
 *  5. SoftAP channel is 2 — ESP-NOW uses channel 2.
 *
 * Packet types (first byte):
 *  0x01 MIC   + int16 BE
 *  0x02 CMD   + ASCII ("M6", "L0", "R0")
 *  0x03 LIGHT + uint16 BE
 *  0x04 TEMP  + int16 BE (F × 10)
 */

#include <espnow.h>

// ---- CHANGE AFTER FIRST BOOT ----
uint8_t TAIL_PEER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // REPLACE with Tail MAC

static const u8 ESPNOW_PMK[16] = {'T','M','D','r','a','k','e','P','M','K','_','2','0','2','6','!'};
static const u8 ESPNOW_LMK[16] = {'T','M','D','r','a','k','e','L','M','K','_','2','0','2','6','!'};

#define ESPNOW_CH 2

enum EspNowType : uint8_t {
  EN_MIC   = 0x01,
  EN_CMD   = 0x02,
  EN_LIGHT = 0x03,
  EN_TEMP  = 0x04
};

bool espnowReady = false;

void printMacBytes(const uint8_t *m) {
  char buf[24];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  Serial.println(buf);
}

void onEspNowRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len < 1) return;
  uint8_t type = data[0];

  if (type == EN_MIC && len >= 3) {
    int16_t level = ((int16_t)data[1] << 8) | data[2];
    if (level < 0) level = 0;
    micLevel = level;
  } else if (type == EN_CMD && len >= 2) {
    // ASCII command after type byte
    char cmd = (char)data[1];
    if (cmd == 'R') {
      resetfading();
    } else if (cmd == 'L') {
      flash_lamp();
    } else if (cmd == 'M' && len >= 3) {
      char d = (char)data[2];
      if (d >= '0' && d <= '9') mode = d - '0';
      else if (d == 'A' || d == 'a') mode = 10;
      Serial.print("ESP-NOW Mode:");
      Serial.println(mode);
    }
  }
}

void onEspNowSent(uint8_t *mac, uint8_t status) {
  // status 0 = success on ESP8266
}

bool setupEspNow() {
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init FAILED");
    return false;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);

  // Encryption keys (ESP8266 API)
  esp_now_set_kok((u8 *)ESPNOW_PMK, 16);

  bool placeholder =
      (TAIL_PEER_MAC[0] == 0xFF && TAIL_PEER_MAC[1] == 0xFF &&
       TAIL_PEER_MAC[2] == 0xFF && TAIL_PEER_MAC[3] == 0xFF &&
       TAIL_PEER_MAC[4] == 0xFF && TAIL_PEER_MAC[5] == 0xFF);

  Serial.print("This Head MAC: ");
  printMacBytes(WiFi.softAPmacAddress());
  // softAPmacAddress returns String on some cores — also print STA mac
  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

  if (placeholder) {
    Serial.println("ESP-NOW: TAIL_PEER_MAC not set — set it and reflash");
    espnowReady = false;
    return false;
  }

  // Role combo, channel, encrypt with LMK
  if (esp_now_add_peer(TAIL_PEER_MAC, ESP_NOW_ROLE_COMBO, ESPNOW_CH, (u8 *)ESPNOW_LMK, 16) != 0) {
    Serial.println("ESP-NOW add peer FAILED");
    return false;
  }

  espnowReady = true;
  Serial.println("ESP-NOW ready (encrypted) ↔ Tail");
  return true;
}

void espnowSendLight(uint16_t value) {
  if (!espnowReady) return;
  uint8_t pkt[3];
  pkt[0] = EN_LIGHT;
  pkt[1] = (value >> 8) & 0xFF;
  pkt[2] = value & 0xFF;
  esp_now_send(TAIL_PEER_MAC, pkt, 3);
}

void espnowSendTemp(float tempF) {
  if (!espnowReady) return;
  int16_t t10 = (int16_t)(tempF * 10.0f);
  uint8_t pkt[3];
  pkt[0] = EN_TEMP;
  pkt[1] = (t10 >> 8) & 0xFF;
  pkt[2] = t10 & 0xFF;
  esp_now_send(TAIL_PEER_MAC, pkt, 3);
}
