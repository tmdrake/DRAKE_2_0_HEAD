int brightness;
bool _direction;

void resetfading() {
  resetBrightnessandDirection();
  Serial.println("Sync Requested...");
}

void fading() {

  static unsigned long previousMillis;

  if (millis() - previousMillis >= 50)  //time expired
  {



    if (_direction)
      brightness--;
    else
      brightness++;

    if (brightness >= 75)
      _direction = true;
    else if (brightness <= 0)
      _direction = false;

    set_brightness(brightness);
    //Serial.println(brightness);


    previousMillis = millis();
  }
}

void set_brightness(byte __brightness) {
  float _brightness = (float)__brightness / 100.0;

  for (uint16_t i = 0; i < spikes.numPixels(); i++)
    spikes.setPixelColor(i, spikes.Color(150 * _brightness, 0, 255 * _brightness));

  /*Patching for the eyes*/
  if (dim_eyes) 
    eyesbrightness(0.1);  //10%
  else
    eyesbrightness(1);    //100%
  
  
  /*********************/
  spikes.show();
}

void resetBrightnessandDirection() {
  _direction = false;
  brightness = 0;
}
