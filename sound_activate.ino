/* Sound loop + CDS-aware eye brightness
 *
 * Eyes (pixels 0-3): when dim_eyes is set by checkLight()/CDS threshold,
 * brightness uses eyeDimPercent/100 (app-tunable via D command).
 */
float cR = 0, cG = 0, cB = 0;

void soundloop(unsigned long millis, long refresh_ms, bool color, long micinput) {
  static unsigned long soundloop_previousMillis = 0;
  static int k = 0;
  static int col = 0;

  if (millis - soundloop_previousMillis >= refresh_ms) {
    soundloop_previousMillis = millis;

    long squareLevel = micinput;

    for (uint16_t i = spikes.numPixels(); i > 0; i--) {
      spikes.setPixelColor(i, spikes.getPixelColor(i - 1));
    }

    if (squareLevel / 1.0f > 0.5) {
      spikes.setPixelColor(4, spikes.Color(0, 0, 0));
    }

    setRgb(squareLevel / 1.0f);

    if (!color) {
      fadeRgb();
    }

    /* Eyes 0-3: full brightness or dimmed by CDS */
    if (dim_eyes)
      eyesbrightness(eyeDimPercent / 100.0f, true);
    else
      eyesbrightness(1.0f, true);

    spikes.show();
    k++;
  }

  if (k >= spikes.numPixels()) {
    col = ++col % 6;
    if (color) {
      cycleRgb(col);
    }
    k = 0;
  }
}

void cycleRgb(int col) {
  switch (col) {
    case 0: cR = 1;   cG = 0;   cB = 0;   break;
    case 1: cR = 0.5; cG = 0.5; cB = 0;   break;
    case 2: cR = 0;   cG = 1;   cB = 0;   break;
    case 3: cR = 0;   cG = 0.5; cB = 0.5; break;
    case 4: cR = 0;   cG = 0;   cB = 1;   break;
    case 5: cR = 0.5; cG = 0;   cB = 0.5; break;
  }
}

void setRgb(float val) {
  if (val > 1.0f) val = 1.0f;
  spikes.setPixelColor(4, (int)(val * cR * 255), (int)(val * cG * 255), (int)(val * cB * 255));
}

void fadeRgb() {
  const int phaseLength = 100;
  const int period = phaseLength * 5;
  static int iteration = 0;
  int phase = iteration / phaseLength;
  int step = iteration % phaseLength;

  switch (phase) {
    case 0: cR = 1; cG = step / (float)phaseLength; cB = 0; break;
    case 1: cR = (phaseLength - step) / (float)phaseLength; cG = 1; cB = 0; break;
    case 2: cR = 0; cG = 1; cB = step / (float)phaseLength; break;
    case 3: cR = 0; cG = (phaseLength - step) / (float)phaseLength; cB = 1; break;
    case 4: cR = step / (float)phaseLength; cG = 0; cB = 1; break;
    case 5: cR = 1; cG = 0; cB = (phaseLength - step) / (float)phaseLength; break;
  }
  iteration = ++iteration % period;
}
