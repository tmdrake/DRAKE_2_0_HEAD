/*
 * Runs on ESP-12E (Aka NodeMCU 1.0)
 * Board version 3.1.2
 * Select NO-OTA/FS
 */
#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
#include <stdlib.h>  ///For Standard C operation

/*Which Pin are FAN*/
#define FAN_PIN D4
#define LIGHT_SENSOR A0
int sensorValue = 0;
bool dim_eyes = false;  //FLAG for Dimming eyes

//Timers for multitasking
#include "Timer.h"
Timer t;

//EEPROM FOR SETTINGS
#include <EEPROM.h>

//Onewire
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/*******************************/
#include <Adafruit_NeoPixel.h>
#define LED_PIN D2
#define LED_PIN1 D1
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 50
Adafruit_NeoPixel spikes(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
//LED[0-3 are eyes 4-50 are spikes disks]
//Adafruit_NeoPixel test(LED_COUNT, LED_PIN1, NEO_GRB + NEO_KHZ800);
/*******************************/

// setup() function -- runs once at startup --------------------------------
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/* Set these to your desired credentials. */
const char *ssid = "TMDRAKE";
//const char *password = APPSK;
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 10);
IPAddress subnet(255, 255, 255, 0);
unsigned int localPort = 1234;  // local port to listen on
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];  // buffer to hold incoming packet,
//char ReplyBuffer[] = "acknowledged\r\n";        // a string to send back
WiFiUDP Udp;
WiFiUDP Udp_sound;


//INTERNAL VARABLES
bool flashed = false;
int mode = 0;
long micLevel = 0;
bool soundmode = false;
bool enableSound = true;
unsigned long lastime = 0;
unsigned long lastmiclevel = -1;
////////////////////////////////////////

//Configure IO and STARTUPS
void setup() {
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);  //Turn Fan on

  //test.begin();               // INITIALIZE NeoPixel spikes object (REQUIRED, Almost forgot)
  spikes.begin();             // INITIALIZE NeoPixel spikes object (REQUIRED)
  spikes.show();              // Turn OFF all pixels ASAP
  spikes.setBrightness(100);  // Set BRIGHTNESS to about 1/5 (max = 255)

/*
  eye.begin();           // INITIALIZE NeoPixel spikes object (REQUIRED)
  eye.show();            // Turn OFF all pixels ASAP
  eye.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)
*/
  //setup serial console
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  Serial.println("Drake's HEAD...GO!");

  //Enable CDS Cell detection to lower brightness of the eye section
  pinMode(LIGHT_SENSOR, INPUT);

  t.every(1000, checkLight);  //check every 1 second
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  t.every(5000, checkSensor);  //check every 5 seconds


  // Setup AP, target
  WiFi.softAPConfig(local_IP, gateway, subnet);
  //WiFi.softAP(ssid);
  WiFi.softAP(ssid, NULL, 2, true, 8);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Udp.begin(localPort);
  Udp_sound.begin(1237);  //Sound stream port

  //wdt_disable();  //Disable the 1.5 second watchdog timer, hardware still applies.
}


/*Main Loop that handles all functions*/
void loop() {
  t.update();  //multiple functions


  if (!flashed) {
    // Flash has higher priority
    //fading();  //Although rainbow is cooler, Fading is async
    sound_detect();  //M0 emulation until we get the mode system up
  }

  checkSerial();     //check for interface
  checkUDP();        //remote commands
  checkUDP_sound();  //sound fed into mic varable
  wdt_reset();       //kickin the dog.
}

void checkUDP() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {

    // read the packet into packetBufffer, main control channel
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;               //End of array
    if ((char)packetBuffer[0] == 'R')  //We recieved a sync command
      resetfading();
    else if ((char)packetBuffer[0] == 'L')
      flash_lamp();
    else if ((char)packetBuffer[0] == 'M') {
      Serial.print("Mode:");
      //Serial.println(packetBuffer[1]);
      mode = atoi(packetBuffer + 1);  //ignore the first BYTE, and second is the value
      //mode = (int)packetBuffer[1]-48; //convert to decimal
      Serial.println(packetBuffer[1]);
    }
  }
}


void sound_detect() {
  if (soundmode && enableSound) {

    if (mode == 1) {  //MO to M1 selection
      //soundloop(millis(), 50, true, 100);  //false = color phase, true = distinct colors
        soundloop(millis(), 50, true, micLevel);  //false = color phase, true = distinct colors
    } else {
        soundloop(millis(), 50, false, micLevel);  //false = color phase, true = distinct colors
    }

    //TODO: Need to reimplment the mode selection
    //mode_selector(mode);

    //digitalWrite(LED_BUILTIN, HIGH);
    if (millis() - lastime > 10000) {
      soundmode = false;  //put the system back into fading mode, after 10 seconds.
      resetBrightnessandDirection();
    }
  } else {


    fading();
    //digitalWrite(LED_BUILTIN, LOW);
    //lastmiclevel = 0;
  }
  ///Check for Sound!
  //long micLevel = analogRead(MIC) - OFFSET;  //adafruit offset
  //Serial.println(micLevel);
  //delay(10);
  if (micLevel > 100 /*TRIGGER SENSITIVITY*/) {
    soundmode = true;
    lastime = millis();  //reset our timeout
  }
}

void checkUDP_sound() {
  /*Check for virtual mic (streamed packets depending on the mode)*/
  int packetSize = Udp_sound.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
    int n = Udp_sound.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;  //End of array

    //Serial.print("Contents:");
    micLevel = atol(packetBuffer);
    Serial.print("MIC:");
    Serial.println(micLevel);  //Data is fed into a varable (may circular buffer in the future)
    /*micLevel: containing the value of the MIC of capture from sensor data, will drop to 0 when nothing is heard or disabled*/
  }
}

void checkLight() {

  // read the analog in value
  sensorValue = analogRead(LIGHT_SENSOR);

  // print the readings in the Serial Monitor
  Serial.print("sensor=");
  Serial.println(sensorValue);
  //Not needed with Background loop

  if (sensorValue < 500) {  //Brightness on eye control. TODO: should be a varabile
    //spikes.setBrightness(200);
    dim_eyes = false;
  } else {
    //spikes.setBrightness(10);
    dim_eyes = true;
  }

  /*Send Light value back on port 1235*/
  Udp.beginPacket(IPAddress(192, 168, 4, 255), 1235);
  Udp.print(sensorValue);
  Udp.endPacket();
}


void checkSensor() {
  /*Automatic Fan control*/
  Serial.print("Head temp=");
  float temperature = sensors.getTempFByIndex(0) - 3;
  Serial.println(temperature);
  sensors.requestTemperatures();

  /*Check for temperature then switch fan on*/
  if (temperature > 85)
    digitalWrite(FAN_PIN, HIGH);
  else
    digitalWrite(FAN_PIN, LOW);

  /*Send temperature value back on port 1236*/
  Udp.beginPacket(IPAddress(192, 168, 4, 255), 1236);
  Udp.print(temperature);
  Udp.endPacket();
}


void flash_lamp() {
  turn_all_on();
  flashed = true;
  t.after(100, turn_all_off);

  //delay(100);
  //turn_all_off();
}

//Below are routines to save code
void turn_all_off() {
  /*Handles turning off both spikess and testing coms*/
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(0, 0, 0));
  spikes.show();
  // for (uint16_t i = 0; i < test.numPixels(); i++)
  //   test.setPixelColor(i, test.Color(0, 0, 0));
  // test.show();


  flashed = false;
}

void turn_all_on() {
  /*Handles turning on both spikess and testing coms*/
  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150, 150, 150));
  spikes.show();

  // for (uint16_t i = 0; i < test.numPixels(); i++)
  //   test.setPixelColor(i, test.Color(150, 150, 150));
  // test.show();
}
