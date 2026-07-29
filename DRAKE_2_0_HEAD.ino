/*
 * Runs on ESP-12E (Aka NodeMCU 1.0)
 * Board version 3.1.2
 * Select NO-OTA/FS
 *
 * Updated July 2026:
 *   - Encrypted ESP-NOW for mic + commands + sensor feedback
 *   - SoftAP channel 2 (shared with ESP-NOW)
 *   - UDP kept as transitional fallback
 */
#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
#include <stdlib.h>

#define FAN_PIN D4
#define LIGHT_SENSOR A0
int sensorValue = 0;
bool dim_eyes = false;

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
  Serial.println("Drake's HEAD...GO! (ESP-NOW encrypted)");

  pinMode(LIGHT_SENSOR, INPUT);

  t.every(1000, checkLight);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  t.every(5000, checkSensor);

  // SoftAP on channel 2 (ESP-NOW must share this channel)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, NULL, 2, true, 8);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  setupEspNow();

  // UDP fallback still available during bench bring-up
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
  checkUDP_sound();  // fallback only
  wdt_reset();
}

void checkUDP() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    if ((char)packetBuffer[0] == 'R')
      resetfading();
    else if ((char)packetBuffer[0] == 'L')
      flash_lamp();
    else if ((char)packetBuffer[0] == 'M') {
      Serial.print("UDP Mode:");
      mode = atoi(packetBuffer + 1);
      Serial.println(packetBuffer[1]);
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
  // Transitional fallback if ESP-NOW peer not configured yet
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
  Serial.print("sensor=");
  Serial.println(sensorValue);

  if (sensorValue < 500)
    dim_eyes = false;
  else
    dim_eyes = true;

  // Preferred: ESP-NOW
  espnowSendLight((uint16_t)constrain(sensorValue, 0, 65535));

  // Fallback UDP
  Udp.beginPacket(IPAddress(192, 168, 4, 10), 1235);
  Udp.print(sensorValue);
  Udp.endPacket();
}

void checkSensor() {
  Serial.print("Head temp=");
  float temperature = sensors.getTempFByIndex(0) - 3;
  Serial.println(temperature);
  sensors.requestTemperatures();

  if (temperature > 85)
    digitalWrite(FAN_PIN, HIGH);
  else
    digitalWrite(FAN_PIN, LOW);

  espnowSendTemp(temperature);

  Udp.beginPacket(IPAddress(192, 168, 4, 10), 1236);
  Udp.print(temperature);
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
