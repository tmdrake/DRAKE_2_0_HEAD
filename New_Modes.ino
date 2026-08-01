/*
 * New_Modes.ino – Head non-blocking modes 2-10 + setSolidColor
 * Uses Tail EN_PHASE (syncPhase) when fresh so animations lock to the suit master clock.
 */
static unsigned long headModePrev = 0;
static uint16_t headStep = 0;
static uint8_t solidR = 150, solidG = 0, solidB = 255;
static int lastHeadMode = -1;

void resetHeadModeState() {
  headModePrev = 0;
  headStep = 0;
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidR = r; solidG = g; solidB = b;
}

void applyEyeDim() {
  float b = dim_eyes ? (eyeDimPercent / 100.0f) : 1.0f;
  eyesbrightness(b, true);
}

/** Current phase: Tail master if packets are fresh, else free-run headStep. */
static uint16_t animPhase(bool advanceLocal, uint16_t localInc) {
  if (phaseSyncActive()) {
    return syncPhase;
  }
  if (advanceLocal) headStep += localInc;
  return headStep;
}

/** Triangle 20..255..20 — must match Tail breathFromPhase(). */
static uint8_t breathFromPhase(uint16_t phase) {
  uint16_t cycle = phase % 512;
  if (cycle < 256)
    return (uint8_t)map(cycle, 0, 255, 20, 255);
  return (uint8_t)map(cycle, 256, 511, 255, 20);
}

void mode_vu() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();
  int n = map(constrain(micLevel, 0, 1200), 0, 1200, 0, spikes.numPixels());
  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i < 4) continue;
    spikes.setPixelColor(i, i < n ? spikes.Color(150, 0, 255) : 0);
  }
  applyEyeDim();
  spikes.show();
}

void mode_rainbow_chase() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();
  uint16_t step = animPhase(true, 256);
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint16_t h = step + (i * 65536L / spikes.numPixels());
    spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(h)));
  }
  applyEyeDim();
  spikes.show();
}

void mode_comet() {
  if (millis() - headModePrev < 45) return;
  headModePrev = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i, ((c >> 16) & 0xFF) * 0.7, ((c >> 8) & 0xFF) * 0.7, (c & 0xFF) * 0.7);
  }
  int span = spikes.numPixels() - 4;
  if (span < 1) span = 1;
  uint16_t step = animPhase(true, 1);
  int head = 4 + (step % span);
  spikes.setPixelColor(head, 255, 255, 255);
  if (head > 4) spikes.setPixelColor(head - 1, 180, 100, 255);
  if (head > 5) spikes.setPixelColor(head - 2, 80, 0, 150);
  applyEyeDim();
  spikes.show();
}

void mode_breathing() {
  if (millis() - headModePrev < 30) return;
  headModePrev = millis();
  uint16_t step = animPhase(true, 4);
  uint8_t breath = breathFromPhase(step);
  uint16_t hue = (uint16_t)(step * 64);
  uint32_t color = spikes.gamma32(spikes.ColorHSV(hue, 255, breath));
  for (int i = 0; i < spikes.numPixels(); i++) spikes.setPixelColor(i, color);
  applyEyeDim();
  spikes.show();
}

void mode_fire() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i < 4) continue;
    int heat = constrain(random(0, 255) - ((i - 4) * 8), 0, 255);
    uint8_t r, g, b;
    if (heat < 85) { r = heat * 3; g = 0; b = 0; }
    else if (heat < 170) { r = 255; g = (heat - 85) * 3; b = 0; }
    else { r = 255; g = 255; b = (heat - 170) * 2; }
    spikes.setPixelColor(i, r, g, b);
  }
  applyEyeDim();
  spikes.show();
}

void mode_sparkle() {
  if (millis() - headModePrev < 50) return;
  headModePrev = millis();
  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i, ((c >> 16) & 0xFF) * 0.85, ((c >> 8) & 0xFF) * 0.85, (c & 0xFF) * 0.85);
  }
  if (random(0, 100) < 35) {
    int p = random(4, spikes.numPixels());
    spikes.setPixelColor(p, random(0, 2) ? 0xFFFFFF : spikes.Color(180, 80, 255));
  }
  applyEyeDim();
  spikes.show();
}

void mode_wave() {
  if (millis() - headModePrev < 35) return;
  headModePrev = millis();
  uint16_t step = animPhase(true, 1);
  for (int i = 0; i < spikes.numPixels(); i++) {
    // Match Tail wave shape closely (sin-based); ESP8266 has sinf
    float phase = (float)(step + i * 30) / 40.0f;
    float wave = (sin(phase) + 1.0f) * 0.5f;
    uint8_t bri = (uint8_t)(wave * 220.0f);
    spikes.setPixelColor(i, (bri * 150) / 255, 0, bri);
  }
  applyEyeDim();
  spikes.show();
}

void mode_solid() {
  if (millis() - headModePrev < 200) return;
  headModePrev = millis();
  for (int i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, solidR, solidG, solidB);
  applyEyeDim();
  spikes.show();
}

void mode_off() {
  if (millis() - headModePrev < 300) return;
  headModePrev = millis();
  for (int i = 0; i < spikes.numPixels(); i++) spikes.setPixelColor(i, 0);
  spikes.show();
}

void mode_selector(int m) {
  if (m != lastHeadMode) {
    resetHeadModeState();
    lastHeadMode = m;
  }
  switch (m) {
    case 0: soundloop(millis(), 50, false, micLevel); break;
    case 1: soundloop(millis(), 50, true, micLevel); break;
    case 2: mode_vu(); break;
    case 3: mode_rainbow_chase(); break;
    case 4: mode_comet(); break;
    case 5: mode_breathing(); break;
    case 6: mode_fire(); break;
    case 7: mode_sparkle(); break;
    case 8: mode_wave(); break;
    case 9: mode_solid(); break;
    case 10: mode_off(); break;
    default: mode = 0; break;
  }
}
