# DRAKE_2_0_HEAD

ESP8266 (NodeMCU) dragonsuit **head** firmware.

## Features
- SoftAP `TMDRAKE` channel 2 + encrypted **ESP-NOW** ↔ Tail
- **Modes 0–10 non-blocking** (parity with Tail / PAWB)
- CDS photocell dims **eye pixels 0–3** (`I` / `D` from app)
- Fan auto / on / off + temperature threshold (`F*` / `FT*`)

## Modes

| ID | Name |
|----|------|
| 0–1 | Sound Phase / Distinct (remote mic) |
| 2 | VU |
| 3–8 | Rainbow, Comet, Breath, Fire, Sparkle, Wave |
| 9–10 | Solid, Off |

Implementation: `New_Modes.ino` + `sound_activate.ino` — **no `delay()` on active path**.  
Legacy blocking demos in `Other_modes.ino` are unused.

## Firmware team docs (Tail repo)
- [FIRMWARE_NOTES.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/FIRMWARE_NOTES.md)
- [SYSTEM.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/SYSTEM.md)
- [ESPNOW.md](https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/ESPNOW.md)

http://tmdrake.com
