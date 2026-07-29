/*
 * Runs on ESP-12E (Aka NodeMCU 1.0)
 * Board version 3.1.2
 *
 * Head controls:
 *  - Fan: auto (by temp threshold), force ON, force OFF
 *  - CDS light sensor (A0): dims eye pixels (0-3) when reading >= threshold
 *
 * See Tail SYSTEM.md / APP_INTERFACE.md for full docs.
 */
#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards!
#endif
#include <stdlib.h>

#define FAN_PIN D4
#define LIGHT_SENSOR A0
int sensorValue = 0;
bool dim_eyes = false;

// ---- App-tunable Head settings ----
int fanMode = 2;              // 0=force OFF, 1=force ON, 2=AUTO by temperature
float fanThresholdF = 85.0f;  // °F — fan ON when temp > this (AUTO mode)
int cdsThreshold = 500;       // CDS reading >= this → dim eyes
int eyeDimPercent = 10;       // eye brightness % when dimmed (1-100)

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

void applyFanOutput() {
  if (fanMode == 0) {
    digitalWrite(FAN_PIN, LOW);   // force OFF
  } else if (fanMode == 1) {
    digitalWrite(FAN_PIN, HIGH);  // force ON
  } else {
    // AUTO
    digitalWrite(FAN_PIN, (lastTempF > fanThresholdF) ? HIGH : LOW);
  }
}

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);

  spikes.begin();
  spikes.show();
  spikes.setBrightness(100);

  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  Serial.println("Drake's HEAD...GO! (fan/CDS settings)");

  pinMode(LIGHT_SENSOR, INPUT);

  t.every(1000, checkLight);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  t.every(5000, checkSensor);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, NULL, 2, true, 8);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  setupEspNow();

  Udp.begin(localPort);
  Udp_sound.begin(1237);
}

void loop() {
  t.update();

  if (!flashed) {
    sound_detect();
  }

  checkSerial();
  checkUDP();
  checkUDP_sound();
  wdt_reset();
}

void checkUDP() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    handleHeadCommand(packetBuffer);
  }
}

/* Shared command parser (UDP ASCII or ESP-NOW CMD payload) */
void handleHeadCommand(const char *s) {
  if (!s || !s[0]) return;
  char c0 = s[0];

  if (c0 == 'R') {
    resetfading();
  } else if (c0 == 'L') {
    flash_lamp();
  } else if (c0 == 'M') {
    if (s[1] >= '0' && s[1] <= '9') mode = s[1] - '0';
    else if (s[1] == 'A' || s[1] == 'a') mode = 10;
    else mode = atoi(s + 1);
    Serial.print("Mode:"); Serial.println(mode);
  } else if (c0 == 'F') {
    // F0=off F1=on F2=auto  |  FT85 = threshold °F
    if (s[1] == 'T' || s[1] == 't') {
      float t = atof(s + 2);
      if (t >= 50 && t <= 120) {
        fanThresholdF = t;
        Serial.print("Fan threshold F="); Serial.println(fanThresholdF);
        applyFanOutput();
      }
    } else {
      int m = atoi(s + 1);
      if (m >= 0 && m <= 2) {
        fanMode = m;
        Serial.print("Fan mode="); Serial.println(fanMode);
        applyFanOutput();
      }
    }
  } else if (c0 == 'I') {
    // I<n> CDS / illumination threshold for eye dim
    int v = atoi(s + 1);
    if (v >= 0 && v <= 1023) {
      cdsThreshold = v;
      Serial.print("CDS threshold="); Serial.println(cdsThreshold);
    }
  } else if (c0 == 'D') {
    // D<n> eye dim percent 1-100 when CDS says dim
    int v = atoi(s + 1);
    if (v >= 1 && v <= 100) {
      eyeDimPercent = v;
      Serial.print("Eye dim %="); Serial.println(eyeDimPercent);
    }
  }
}

void sound_detect() {
  if (soundmode && enableSound) {
    if (mode == 1) {
      soundloop(millis(), 50, true, micLevel);
    } else {
      soundloop(millis(), 50, false, micLevel);
    }

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

/*
 * CDS (photocell) on A0
 * --------------------
 * Hardware: light-dependent resistor voltage divider into Head A0.
 * Reading range roughly 0–1023 (ESP8266 ADC).
 *
 * Behaviour (eyes = NeoPixel indices 0–3):
 *   if sensorValue >= cdsThreshold  → dim_eyes = true
 *       eyes drawn at eyeDimPercent/100 brightness (default 10%)
 *   if sensorValue <  cdsThreshold  → dim_eyes = false
 *       eyes at full relative brightness (1.0)
 *
 * Default threshold 500 matches original firmware. Tunable via app command I<n>.
 * Applied inside soundloop via eyesbrightness() in eyes_led.ino / sound_activate.ino.
 */
void checkLight() {
  sensorValue = analogRead(LIGHT_SENSOR);
  Serial.print("CDS=");
  Serial.println(sensorValue);

  dim_eyes = (sensorValue >= cdsThreshold);

  espnowSendLight((uint16_t)constrain(sensorValue, 0, 65535));

  Udp.beginPacket(IPAddress(192, 168, 4, 10), 1235);
  Udp.print(sensorValue);
  Udp.endPacket();
}

void checkSensor() {
  Serial.print("Head temp=");
  lastTempF = sensors.getTempFByIndex(0) - 3;
  Serial.println(lastTempF);
  sensors.requestTemperatures();

  applyFanOutput();

  espnowSendTemp(lastTempF);

  Udp.beginPacket(IPAddress(192, 168, 4, 10), 1236);
  Udp.print(lastTempF);
  Udp.endPacket();
}

void flash_lamp() {
  turn_all_on();
  flashed = true;
  t.after(100, turn_all_off);
}

void turn_all_off() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  spikes.show();
  flashed = false;
}

void turn_all_on() {
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150, 150, 150));
  spikes.show();
}
