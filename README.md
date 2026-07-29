# DRAKE_2_0_HEAD

ESP8266 (NodeMCU 1.0) firmware for the **dragonsuit head**.

## Role
- SoftAP `TMDRAKE` @ `192.168.4.1` (hidden, channel 2)
- Receives **commands** on UDP **1234** (`M`, `L`, `R`)
- Receives **mic stream** on UDP **1237** (binary 2-byte int16 preferred; ASCII fallback)
- Sends **light sensor** to Tail on UDP **1235**
- Sends **temperature** to Tail on UDP **1236**
- 50× NeoPixels (spikes + eyes), fan control, CDS dimming

## Latency / WiFi
- `WiFi.setSleepMode(WIFI_NONE_SLEEP)` — power-save off
- Prefer binary mic packets from Tail (see Tail `sound_activate.ino`)

## Full system docs
Architecture, mode list, and protocol details live in the Tail repo:

**https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/SYSTEM.md**

## Board
- ESP-12E / NodeMCU 1.0
- Core 3.1.2, NO-OTA / no FS as historically used

http://tmdrake.com
