/*
 * EspNowCom.ino – Encrypted ESP-NOW (Head ESP8266 ↔ Tail ESP32)
 * CMD handler forwards extended settings: F*, I*, D*, M*, L*, R*
 */

#include <espnow.h>

// Tail ESP32 MAC (from flash log): 24:0A:C4:81:4A:B0
uint8_t TAIL_PEER_MAC[6] = {0x24, 0x0A, 0xC4, 0x81, 0x4A, 0xB0};

static const u8 ESPNOW_PMK[16] = {'T','M','D','r','a','k','e','P','M','K','_','2','0','2','6','!'};
static const u8 ESPNOW_LMK[16] = {'T','M','D','r','a','k','e','L','M','K','_','2','0','2','6','!'};

#define ESPNOW_CH 2
// Must match Tail — ESP32↔ESP8266 encrypted ESP-NOW often fails silently
#define ESPNOW_ENCRYPT 0

enum EspNowType : uint8_t {
  EN_MIC   = 0x01,
  EN_CMD   = 0x02,
  EN_LIGHT = 0x03,
  EN_TEMP  = 0x04,
  EN_PHASE = 0x05
};

bool espnowReady = false;

// Animation phase master from Tail (EN_PHASE)
uint16_t syncPhase = 0;
unsigned long lastPhaseMs = 0;
bool phaseSyncActive() { return (millis() - lastPhaseMs) < 250; }

void printMacBytes(const uint8_t *m) {
  char buf[24];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  Serial.println(buf);
}

void onEspNowRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  (void)mac;
  if (len < 1 || !data) return;
  uint8_t type = data[0];

  if (type == EN_MIC && len >= 3) {
    int16_t level = ((int16_t)data[1] << 8) | data[2];
    if (level < 0) level = 0;
    micNoteLevel((long)level);
  } else if (type == EN_CMD && len >= 2) {
    char cmd[32];
    uint8_t n = len - 1;
    if (n > 30) n = 30;
    memcpy(cmd, data + 1, n);
    cmd[n] = 0;
    Serial.print("ESP-NOW CMD: ");
    Serial.println(cmd);
    handleHeadCommand(cmd);
  } else if (type == EN_PHASE && len >= 4) {
    syncPhase = ((uint16_t)data[1] << 8) | data[2];
    lastPhaseMs = millis();
    uint8_t m = data[3];
    if (m <= 10 && m != (uint8_t)mode) {
      mode = m;
      saveMode(mode);
      resetHeadModeState();
      Serial.print("ESP-NOW phase mode→");
      Serial.println(mode);
    }
  }
}

void onEspNowSent(uint8_t *mac, uint8_t status) {
}

bool setupEspNow() {
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init FAILED");
    return false;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);
#if ESPNOW_ENCRYPT
  esp_now_set_kok((u8 *)ESPNOW_PMK, 16);
#endif

  bool placeholder =
      (TAIL_PEER_MAC[0] == 0xFF && TAIL_PEER_MAC[1] == 0xFF &&
       TAIL_PEER_MAC[2] == 0xFF && TAIL_PEER_MAC[3] == 0xFF &&
       TAIL_PEER_MAC[4] == 0xFF && TAIL_PEER_MAC[5] == 0xFF);

  // Print both — SoftAP MAC is what Tail should peer to when Head is AP
  Serial.print("STA MAC:     ");
  Serial.println(WiFi.macAddress());
  Serial.print("SoftAP MAC:  ");
  Serial.println(WiFi.softAPmacAddress());

  if (placeholder) {
    Serial.println("ESP-NOW: TAIL_PEER_MAC not set — set it and reflash");
    espnowReady = false;
    return false;
  }

#if ESPNOW_ENCRYPT
  if (esp_now_add_peer(TAIL_PEER_MAC, ESP_NOW_ROLE_COMBO, ESPNOW_CH, (u8 *)ESPNOW_LMK, 16) != 0) {
#else
  // key=NULL / key_len=0 → unencrypted peer (works ESP32↔ESP8266)
  if (esp_now_add_peer(TAIL_PEER_MAC, ESP_NOW_ROLE_COMBO, ESPNOW_CH, NULL, 0) != 0) {
#endif
    Serial.println("ESP-NOW add peer FAILED");
    return false;
  }

  espnowReady = true;
#if ESPNOW_ENCRYPT
  Serial.println("ESP-NOW ready (encrypted) ↔ Tail");
#else
  Serial.println("ESP-NOW ready (no encrypt) ↔ Tail");
#endif
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
