/*
 * New_Modes.ino – Head (50 NeoPixels)
 * Non-blocking modes 2-10 aligned with Tail/PAWB.
 * Eyes (pixels 0-3) re-applied with CDS dim after each frame.
 */

static unsigned long headModePrev = 0;
static uint16_t headStep = 0;
static uint8_t solidR = 150, solidG = 0, solidB = 255;
static int lastHeadMode = -1;

void resetHeadModeState() {
  headModePrev = 0;
  headStep = 0;
}

// Apply CDS eye dim on pixels 0-3 without clobbering the rest
void applyEyeDim() {
  float b = dim_eyes ? (eyeDimPercent / 100.0f) : 1.0f;
  eyesbrightness(b, true);
}

// Mode 2 – VU from remote mic
void mode_vu() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();

  int n = map(constrain(micLevel, 0, 1200), 0, 1200, 0, spikes.numPixels());
  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i < 4) continue; // eyes handled below
    spikes.setPixelColor(i, i < n ? spikes.Color(150, 0, 255) : 0);
  }
  applyEyeDim();
  spikes.show();
}

// Mode 3 – Rainbow
void mode_rainbow_chase() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    uint16_t h = headStep + (i * 65536L / spikes.numPixels());
    spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(h)));
  }
  applyEyeDim();
  spikes.show();
  headStep += 256;
}

// Mode 4 – Comet
void mode_comet() {
  if (millis() - headModePrev < 45) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i,
      ((c >> 16) & 0xFF) * 0.7,
      ((c >> 8) & 0xFF) * 0.7,
      (c & 0xFF) * 0.7);
  }

  // Comet on spike region (4+)
  int span = spikes.numPixels() - 4;
  if (span < 1) span = 1;
  int head = 4 + (headStep % span);
  spikes.setPixelColor(head, 255, 255, 255);
  if (head > 4) spikes.setPixelColor(head - 1, 180, 100, 255);
  if (head > 5) spikes.setPixelColor(head - 2, 80, 0, 150);

  applyEyeDim();
  spikes.show();
  headStep++;
}

// Mode 5 – Breathing
void mode_breathing() {
  if (millis() - headModePrev < 30) return;
  headModePrev = millis();

  static int breath = 40;
  static int dir = 1;
  breath += dir * 3;
  if (breath >= 255) { breath = 255; dir = -1; }
  if (breath <= 20)  { breath = 20;  dir = 1; }

  uint32_t color = spikes.gamma32(spikes.ColorHSV(headStep, 255, breath));
  for (int i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, color);

  applyEyeDim();
  spikes.show();
  headStep += 20;
}

// Mode 6 – Fire
void mode_fire() {
  if (millis() - headModePrev < 40) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    if (i < 4) continue;
    int heat = random(0, 255);
    heat = constrain(heat - ((i - 4) * 8), 0, 255);
    uint8_t r, g, b;
    if (heat < 85) {
      r = heat * 3; g = 0; b = 0;
    } else if (heat < 170) {
      r = 255; g = (heat - 85) * 3; b = 0;
    } else {
      r = 255; g = 255; b = (heat - 170) * 2;
    }
    spikes.setPixelColor(i, r, g, b);
  }
  applyEyeDim();
  spikes.show();
}

// Mode 7 – Sparkle
void mode_sparkle() {
  if (millis() - headModePrev < 50) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    uint32_t c = spikes.getPixelColor(i);
    spikes.setPixelColor(i,
      ((c >> 16) & 0xFF) * 0.85,
      ((c >> 8) & 0xFF) * 0.85,
      (c & 0xFF) * 0.85);
  }
  if (random(0, 100) < 35) {
    int p = random(4, spikes.numPixels()); // prefer spikes
    if (random(0, 2))
      spikes.setPixelColor(p, 255, 255, 255);
    else
      spikes.setPixelColor(p, 180, 80, 255);
  }
  applyEyeDim();
  spikes.show();
}

// Mode 8 – Wave
void mode_wave() {
  if (millis() - headModePrev < 35) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++) {
    // triangle wave (no sin dependency issues)
    int phase = (headStep + i * 12) % 100;
    int w = phase < 50 ? phase * 2 : (100 - phase) * 2;
    uint8_t bri = map(w, 0, 100, 20, 220);
    spikes.setPixelColor(i, (bri * 150) / 255, 0, bri);
  }
  applyEyeDim();
  spikes.show();
  headStep++;
}

// Mode 9 – Solid
void mode_solid() {
  if (millis() - headModePrev < 200) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, solidR, solidG, solidB);
  applyEyeDim();
  spikes.show();
}

// Mode 10 – Off
void mode_off() {
  if (millis() - headModePrev < 300) return;
  headModePrev = millis();

  for (int i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, 0, 0, 0);
  spikes.show();
}

void mode_selector(int m) {
  if (m != lastHeadMode) {
    resetHeadModeState();
    lastHeadMode = m;
  }

  switch (m) {
    case 0:
      soundloop(millis(), 50, false, micLevel);
      break;
    case 1:
      soundloop(millis(), 50, true, micLevel);
      break;
    case 2:  mode_vu();            break;
    case 3:  mode_rainbow_chase(); break;
    case 4:  mode_comet();         break;
    case 5:  mode_breathing();     break;
    case 6:  mode_fire();          break;
    case 7:  mode_sparkle();       break;
    case 8:  mode_wave();          break;
    case 9:  mode_solid();         break;
    case 10: mode_off();           break;
    default:
      mode = 0;
      break;
  }
}
