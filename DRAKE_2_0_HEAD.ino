/*
 * Runs on ESP-12E (NodeMCU 1.0)
 * Modes 0-10 non-blocking (match Tail/PAWB)
 * CDS dims eyes 0-3; fan + ESP-NOW settings
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

void applyFanOutput() {
  if (fanMode == 0)
    digitalWrite(FAN_PIN, LOW);
  else if (fanMode == 1)
    digitalWrite(FAN_PIN, HIGH);
  else
    digitalWrite(FAN_PIN, (lastTempF > fanThresholdF) ? HIGH : LOW);
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
  Serial.println("Drake's HEAD...GO! (modes 0-10)");

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
  // Visual modes 2-10 run continuously (match Tail)
  if (mode >= 2 && mode <= 10) {
    mode_selector(mode);
    return;
  }

  // Modes 0-1: sound-reactive when mic hot, else idle fade
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
  lastTempF = sensors.getTempFByIndex(0) - 3;
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
