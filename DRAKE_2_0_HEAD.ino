/*
 * Head ESP8266 – modes 0-10, C color, CDS, fan, ESP-NOW
 */
#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards!
#endif
#include <stdlib.h>

#define FAN_PIN D4
#define LIGHT_SENSOR A0
int sensorValue = 0;
bool dim_eyes = false;

int fanMode = 2;
float fanThresholdF = 85.0f;
int cdsThreshold = 500;
int eyeDimPercent = 10;

#include "Timer.h"
Timer t;

#include <EEPROM.h>

#include <DallasTemperature.h>
#define ONE_WIRE_BUS D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#include <Adafruit_NeoPixel.h>
#define LED_PIN D2
#define LED_PIN1 D1
#define LED_COUNT 50
Adafruit_NeoPixel spikes(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "TMDRAKE";
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 10);
IPAddress subnet(255, 255, 255, 0);
unsigned int localPort = 1234;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];
WiFiUDP Udp;
WiFiUDP Udp_sound;

bool flashed = false;
int mode = 0;
long micLevel = 0;
bool soundmode = false;
bool enableSound = true;
unsigned long lastime = 0;
unsigned long lastmiclevel = -1;
float lastTempF = 0;

// DS18B20 re-probe (begin() only searches once unless we call it again)
static uint8_t tempFailStreak = 0;
static unsigned long lastTempReprobeMs = 0;
#define TEMP_FAILS_BEFORE_REPROBE  3          // ~15 s of bad reads @ 5 s interval
#define TEMP_PERIODIC_REPROBE_MS   (5UL * 60UL * 1000UL)  // hot-plug recovery

// Library “no device” / bus error (DallasTemperature DEVICE_DISCONNECTED_F ≈ -196.6)
bool tempBusError(float rawF) {
  return (rawF < -100.0f || rawF > 180.0f);  // allow real suit temps; drop open-bus codes
}

// Plausible head/suit air temp for fan auto only
bool tempOkForFan(float tF) {
  return (tF > 40.0f && tF < 140.0f);
}

/** Full 1-Wire search again — picks up reconnects without rebooting Head. */
void reprobeTempBus(const char *reason) {
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setResolution(12);
  lastTempReprobeMs = millis();
  tempFailStreak = 0;
  Serial.print("DS18B20 re-probe (");
  Serial.print(reason);
  Serial.print("): devices=");
  Serial.println(sensors.getDeviceCount());
  sensors.requestTemperatures();
}

void applyFanOutput() {
  if (fanMode == 0) {
    digitalWrite(FAN_PIN, LOW);   // force off
  } else if (fanMode == 1) {
    digitalWrite(FAN_PIN, HIGH);  // force on
  } else {
    // Auto: only with a sane reading (not bus error, not room-cold bench noise as “hot”)
    bool on = tempOkForFan(lastTempF) && (lastTempF > fanThresholdF);
    digitalWrite(FAN_PIN, on ? HIGH : LOW);
  }
}

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);  // start OFF; auto applies after first good temp sample
  applyFanOutput();

  spikes.begin();
  spikes.show();
  spikes.setBrightness(100);

  Serial.begin(115200);
  Serial.println("Drake's HEAD...GO! (modes 0-10 + C)");

  pinMode(LIGHT_SENSOR, INPUT);

  t.every(1000, checkLight);
  // First bus search at boot (also see reprobeTempBus on errors / every 5 min)
  reprobeTempBus("boot");
  t.every(5000, checkSensor);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  // channel 2, SSID visible (hidden SoftAP made Tail pairing flaky), max 8 STAs
  WiFi.softAP(ssid, NULL, 2, false, 8);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // Max SoftAP TX for suit range (dBm scale 0–20.5 on ESP8266)
  WiFi.setOutputPower(20.5f);

  setupEspNow();
  Udp.begin(localPort);
  Udp_sound.begin(1237);
}

void loop() {
  t.update();
  if (!flashed) sound_detect();
  checkSerial();
  checkUDP();
  checkUDP_sound();
  wdt_reset();
}

void checkUDP() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if (n < 0) n = 0;
    if (n > UDP_TX_PACKET_MAX_SIZE) n = UDP_TX_PACKET_MAX_SIZE;
    packetBuffer[n] = 0;
    Serial.print("UDP CMD: ");
    Serial.println(packetBuffer);
    handleHeadCommand(packetBuffer);
  }
}

void handleHeadCommand(const char *s) {
  if (!s || !s[0]) return;
  char c0 = s[0];

  if (c0 == 'R') {
    resetfading();
    resetHeadModeState();
  } else if (c0 == 'L') {
    flash_lamp();
  } else if (c0 == 'M') {
    if (s[1] >= '0' && s[1] <= '9') mode = s[1] - '0';
    else if (s[1] == 'A' || s[1] == 'a') mode = 10;
    else mode = atoi(s + 1);
    if (mode < 0) mode = 0;
    if (mode > 10) mode = 10;
    resetHeadModeState();
    Serial.print("Mode:"); Serial.println(mode);
  } else if (c0 == 'C') {
    int r = 0, g = 0, b = 0;
    if (sscanf(s + 1, "%d,%d,%d", &r, &g, &b) == 3) {
      setSolidColor(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255));
      mode = 9;
      resetHeadModeState();
      Serial.println("Solid color set");
    }
  } else if (c0 == 'F') {
    if (s[1] == 'T' || s[1] == 't') {
      float tf = atof(s + 2);
      if (tf >= 50 && tf <= 120) {
        fanThresholdF = tf;
        applyFanOutput();
      }
    } else {
      int m = atoi(s + 1);
      if (m >= 0 && m <= 2) {
        fanMode = m;
        applyFanOutput();
      }
    }
  } else if (c0 == 'I') {
    int v = atoi(s + 1);
    if (v >= 0 && v <= 1023) cdsThreshold = v;
  } else if (c0 == 'D') {
    int v = atoi(s + 1);
    if (v >= 1 && v <= 100) eyeDimPercent = v;
  }
}

void sound_detect() {
  if (mode >= 2 && mode <= 10) {
    mode_selector(mode);
    return;
  }
  if (soundmode && enableSound) {
    mode_selector(mode);
    if (millis() - lastime > 10000) {
      soundmode = false;
      resetBrightnessandDirection();
    }
  } else {
    fading();
  }
  if (micLevel > 100) {
    soundmode = true;
    lastime = millis();
  }
}

void checkUDP_sound() {
  int packetSize = Udp_sound.parsePacket();
  if (!packetSize) return;
  int n = Udp_sound.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
  if (n == 2) {
    int16_t level = ((uint8_t)packetBuffer[0] << 8) | (uint8_t)packetBuffer[1];
    if (level < 0) level = 0;
    micLevel = level;
  } else if (n > 0) {
    packetBuffer[n] = 0;
    micLevel = atol(packetBuffer);
    if (micLevel < 0) micLevel = 0;
  }
}

void checkLight() {
  sensorValue = analogRead(LIGHT_SENSOR);
  dim_eyes = (sensorValue >= cdsThreshold);
  espnowSendLight((uint16_t)constrain(sensorValue, 0, 65535));
  Udp.beginPacket(IPAddress(192, 168, 4, 10), 1235);
  Udp.print(sensorValue);
  Udp.endPacket();
}

void checkSensor() {
  // Periodic re-search so a probe plugged in after boot is found without reboot
  if (millis() - lastTempReprobeMs >= TEMP_PERIODIC_REPROBE_MS) {
    reprobeTempBus("periodic");
    applyFanOutput();
    return;  // next cycle reads the conversion we just requested
  }

  // Read result of the *previous* requestTemperatures() (async)
  float rawF = sensors.getTempFByIndex(0);
  float tF = rawF - 3.0f;  // calibration offset (was always applied)

  if (!tempBusError(rawF)) {
    tempFailStreak = 0;
    lastTempF = tF;
    espnowSendTemp(lastTempF);
    Udp.beginPacket(IPAddress(192, 168, 4, 10), 1236);
    Udp.print(lastTempF);
    Udp.endPacket();
    sensors.requestTemperatures();
  } else {
    tempFailStreak++;
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 15000) {
      lastLog = millis();
      Serial.print("Temp bus error rawF=");
      Serial.print(rawF);
      Serial.print(" devices=");
      Serial.print(sensors.getDeviceCount());
      Serial.print(" fails=");
      Serial.println(tempFailStreak);
    }
    // After several bad reads, full bus search again (hot-unplug / reconnect)
    if (tempFailStreak >= TEMP_FAILS_BEFORE_REPROBE) {
      reprobeTempBus("after failures");
    } else {
      sensors.requestTemperatures();
    }
  }

  applyFanOutput();
}

void flash_lamp() {
  turn_all_on();
  flashed = true;
  t.after(100, turn_all_off);
}

void turn_all_off() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, 0);
  spikes.show();
  flashed = false;
}

void turn_all_on() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150, 150, 150));
  spikes.show();
}
