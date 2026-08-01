# DRAKE_2_0_HEAD

ESP8266 (NodeMCU) dragonsuit **head** firmware.

## Features
- SoftAP `TMDRAKE` **visible** channel 2 + **ESP-NOW** ↔ Tail
- **Modes 0–10 non-blocking** (parity with Tail / PAWB)
- **Last mode saved to EEPROM** on `M` / `C` / phase mode snap; restored at boot
- CDS photocell dims **eye pixels 0–3** (`I` / `D` from app)
- Fan auto / on / off + temperature threshold (`F*` / `FT*`)
  - Boot: fan **off** until a valid temp sample (auto only runs above threshold, default 85 °F)
  - `F0` off · `F1` on · `F2` auto · `FT<n>` threshold °F
- **DS18B20 re-probe**: bus search at boot, after 3 bad reads, and every 5 min (hot-plug without reboot)
- **Phase sync** from Tail (ESP-NOW `0x05`) for rainbow / comet / breathe / wave

## ESP-NOW (read this)

Full blame/gotchas: **[ESPNOW.md in Tail repo](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/ESPNOW.md)** (SoftAP MAC ≠ STA MAC, encrypt OFF for ESP32↔ESP8266).

Boot serial prints **STA MAC** and **SoftAP MAC**. Tail must peer the **SoftAP** address (or uses `WiFi.BSSID()` automatically).

## Modes

| ID | Name |
|----|------|
| 0–1 | Sound Phase / Distinct (remote mic) |
| 2 | VU |
| 3–8 | Rainbow, Comet, Breath, Fire, Sparkle, Wave |
| 9–10 | Solid, Off |

Implementation: `New_Modes.ino` + `sound_activate.ino` — **no `delay()` on active path**.  
Legacy blocking demos in `Other_modes.ino` are unused.

## Repo contents

Mostly `.ino` + text. Libraries or assets needed for the build/docs are fine.  
See [REPO.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/REPO.md) (agent tools may not upload PNG/PDF — use local git).

## Firmware team docs (Tail repo)
- [FIRMWARE_NOTES.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/FIRMWARE_NOTES.md)
- [SYSTEM.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/SYSTEM.md)
- [ESPNOW.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/ESPNOW.md)

http://tmdrake.com
