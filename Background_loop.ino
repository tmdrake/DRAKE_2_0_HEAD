/* Idle purple fade — non-blocking (millis), CDS-aware eyes */
int brightness;
bool _direction;

void resetfading() {
  resetBrightnessandDirection();
  Serial.println("Sync Requested...");
}

void fading() {
  static unsigned long previousMillis;

  if (millis() - previousMillis < 50) return;
  previousMillis = millis();

  if (_direction)
    brightness--;
  else
    brightness++;

  if (brightness >= 75)
    _direction = true;
  else if (brightness <= 0)
    _direction = false;

  set_brightness(brightness);
}

void set_brightness(byte __brightness) {
  float _brightness = (float)__brightness / 100.0;

  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150 * _brightness, 0, 255 * _brightness));

  if (dim_eyes)
    eyesbrightness(eyeDimPercent / 100.0f, true);
  else
    eyesbrightness(1.0f, true);

  spikes.show();
}

void resetBrightnessandDirection() {
  _direction = false;
  brightness = 0;
}
