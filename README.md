# DRAKE_2_0_HEAD

ESP8266 (NodeMCU) dragonsuit **head** firmware.

## Features
- SoftAP `TMDRAKE` channel 2 + encrypted **ESP-NOW** ↔ Tail
- **Modes 0–10 non-blocking** (parity with Tail / PAWB)
- CDS photocell dims **eye pixels 0–3** (`I` / `D` from app)
- Fan auto / on / off + temperature threshold (`F*` / `FT*`)
  - Boot: fan **off** until a valid temp sample (auto only runs above threshold, default 85 °F)
  - `F0` off · `F1` on · `F2` auto · `FT<n>` threshold °F

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
