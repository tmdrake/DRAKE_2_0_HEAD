# DRAKE_2_0_HEAD

ESP8266 (NodeMCU 1.0) firmware for the **dragonsuit head**.

## Role
- SoftAP `TMDRAKE` @ `192.168.4.1`, **channel 2**, hidden
- **Encrypted ESP-NOW** ↔ Tail (mic in, commands in, light/temp out)
- UDP fallback still present for bench bring-up
- 50× NeoPixels, fan, CDS, temperature sensor

## ESP-NOW bench setup
1. Flash Head + Tail, open Serial 115200 on both.
2. Copy each board’s MAC from the log.
3. Set `TAIL_PEER_MAC` in `EspNowCom.ino` (this repo).
4. Set `HEAD_PEER_MAC` in Tail `EspNowCom.ino`.
5. Keys must match: `TMDrakePMK_2026!` / `TMDrakeLMK_2026!`
6. Reflash both.

Full protocol docs:  
**https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/ESPNOW.md**  
**https://github.com/tmdrake/DRAKE_2_0_TAIL/blob/main/SYSTEM.md**

http://tmdrake.com
