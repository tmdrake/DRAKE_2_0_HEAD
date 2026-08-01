/* Sound Phase (M0) / Sound Pulse (M1) + CDS-aware eyes
 *
 * Eyes 0–3: CDS dim. Body flood starts at pixel 4 (never shift into eyes).
 */
float cR = 1.0f, cG = 0.0f, cB = 0.0f;

static unsigned long soundloop_previousMillis = 0;
static int soundloop_k = 0;
static int soundloop_col = 0;
static uint16_t soundloop_hue = 0;
static bool soundloop_inited = false;

void resetSoundloopState() {
  soundloop_previousMillis = 0;
  soundloop_k = 0;
  soundloop_col = 0;
  soundloop_hue = 0;
  soundloop_inited = false;
  cR = 1.0f;
  cG = 0.0f;
  cB = 0.0f;
}

static void hueToRgb(uint16_t h, float *r, float *g, float *b) {
  uint8_t sextant = h / 10923;
  uint16_t rem = h % 10923;
  float t = rem / 10923.0f;
  switch (sextant % 6) {
    case 0: *r = 1;   *g = t;   *b = 0;   break;
    case 1: *r = 1-t; *g = 1;   *b = 0;   break;
    case 2: *r = 0;   *g = 1;   *b = t;   break;
    case 3: *r = 0;   *g = 1-t; *b = 1;   break;
    case 4: *r = t;   *g = 0;   *b = 1;   break;
    default:*r = 1;   *g = 0;   *b = 1-t; break;
  }
}

/*
 * M0 Sound Phase / M1 Sound Pulse — matches Tail flood (micNorm01).
 * micinput kept for call-site compat; intensity from shared micNorm01().
 */
void soundloop(unsigned long nowMs, long refresh_ms, bool color, long micinput) {
  (void)micinput;
  if (refresh_ms < 8) refresh_ms = 8;
  if (refresh_ms > 80) refresh_ms = 80;

  if (nowMs - soundloop_previousMillis < (unsigned long)refresh_ms) return;
  soundloop_previousMillis = nowMs;

  if (!soundloop_inited) {
    if (color)
      cycleRgb(0);
    else
      hueToRgb(soundloop_hue, &cR, &cG, &cB);
    soundloop_inited = true;
  }

  // Same scale as VU / Tail: excess above floor → 0..1
  float bright = micNorm01();

  const uint16_t bodyStart = 4;
  const uint16_t n = spikes.numPixels();

  // Fast flood: shift body only
  for (uint16_t i = n - 1; i > bodyStart; i--) {
    spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
  }

  if (bright < 0.04f)
    spikes.setPixelColor(bodyStart, 0);
  else
    setRgb(bright);

  if (!color) {
    uint16_t step = 400 + (uint16_t)(bright * 1400.0f);
    soundloop_hue = (uint16_t)(soundloop_hue + step);
    hueToRgb(soundloop_hue, &cR, &cG, &cB);
  }

  if (dim_eyes)
    eyesbrightness(eyeDimPercent / 100.0f, true);
  else
    eyesbrightness(1.0f, true);

  spikes.show();
  soundloop_k++;

  if (soundloop_k >= (int)(n - bodyStart)) {
    soundloop_col = ++soundloop_col % 6;
    if (color) {
      cycleRgb(soundloop_col);
    }
    soundloop_k = 0;
  }
}

void cycleRgb(int col) {
  switch (col) {
    case 0: cR = 1;   cG = 0;   cB = 0;   break;
    case 1: cR = 1;   cG = 0.55f; cB = 0; break;
    case 2: cR = 0;   cG = 1;   cB = 0;   break;
    case 3: cR = 0;   cG = 0.7f; cB = 1;  break;
    case 4: cR = 0.15f; cG = 0; cB = 1;   break;
    case 5: cR = 0.85f; cG = 0; cB = 0.7f; break;
    default: cR = 1;  cG = 0;   cB = 0;   break;
  }
}

void setRgb(float val) {
  if (val < 0.0f) val = 0.0f;
  if (val > 1.0f) val = 1.0f;
  spikes.setPixelColor(4,
    (int)(val * cR * 255.0f + 0.5f),
    (int)(val * cG * 255.0f + 0.5f),
    (int)(val * cB * 255.0f + 0.5f));
}

void fadeRgb() {
  soundloop_hue = (uint16_t)(soundloop_hue + 800);
  hueToRgb(soundloop_hue, &cR, &cG, &cB);
}
