
// Fill along the length of the spikes in various colors...
//  colorWipe(spikes.Color(255,   0,   0), 50); // Red
//  colorWipe(spikes.Color(  0, 255,   0), 50); // Green
//  colorWipe(spikes.Color(  0,   0, 255), 50); // Blue
//
//  // Do a theater marquee effect in various colors...
//  theaterChase(spikes.Color(150, 0, 255), 50); // White, half brightness
//  theaterChase(spikes.Color(127,   0,   0), 50); // Red, half brightness
//  theaterChase(spikes.Color(  0,   0, 127), 50); // Blue, half brightness

//  rainbow(10);             // Flowing rainbow cycle along the whole spikes
//  theaterChaseRainbow(10); // Rainbow-enhanced theaterChase variant


// Some functions of our own for creating animated effects -----------------

// Fill spikes pixels one after another with a color. spikes is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// spikes.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < spikes.numPixels(); i++) {  // For each pixel in spikes...
    spikes.setPixelColor(i, color);               //  Set pixel's color (in RAM)
    spikes.show();                                //  Update spikes to match
    delay(wait);                                  //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la spikes.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; a++) {   // Repeat 10 times...
    for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
      spikes.clear();              //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of spikes in steps of 3...
      for (int c = b; c < spikes.numPixels(); c += 3) {
        spikes.setPixelColor(c, color);  // Set pixel 'c' to value 'color'
      }
      spikes.show();  // Update spikes with new contents
      delay(wait);    // Pause for a moment
    }
  }
}


// Rainbow cycle along whole spikes. Pass delay time (in ms) between frames.
void A_rainbow(unsigned long millis, long refresh_ms) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  //for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
  static unsigned long A_rainbow_previousMillis = 0;  //persistant varable
  static long firstPixelHue = 0;

  while (firstPixelHue < 5 * 65536 && millis - A_rainbow_previousMillis >= refresh_ms /*delay*/) {
    A_rainbow_previousMillis = millis;

    for (int i = 0; i < spikes.numPixels(); i++) {  // For each pixel in spikes...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the spikes
      // (spikes.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / spikes.numPixels());
      // spikes.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through spikes.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(pixelHue)));
      //eye.setPixelColor(i, eye.gamma32(eye.ColorHSV(pixelHue))); //Copy the main code for now
    }
    spikes.show();  // Update spikes with new contents
    //eye.show();
    //delay(wait);  // Pause for a moment
    firstPixelHue += 256;
  }
  //}
}

// Rainbow cycle along whole spikes. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < spikes.numPixels(); i++) {  // For each pixel in spikes...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the spikes
      // (spikes.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / spikes.numPixels());
      // spikes.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through spikes.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      spikes.setPixelColor(i, spikes.gamma32(spikes.ColorHSV(pixelHue)));
      //eye.setPixelColor(i, eye.gamma32(eye.ColorHSV(pixelHue))); //Copy the main code for now
    }
    spikes.show();  // Update spikes with new contents
    //eye.show();
    delay(wait);  // Pause for a moment
  }
}


// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;           // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++) {   // Repeat 30 times...
    for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
      spikes.clear();              //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of spikes in increments of 3...
      for (int c = b; c < spikes.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the spikes (spikes.numPixels() steps):
        int hue = firstPixelHue + c * 65536L / spikes.numPixels();
        uint32_t color = spikes.gamma32(spikes.ColorHSV(hue));  // hue -> RGB
        spikes.setPixelColor(c, color);                         // Set pixel 'c' to value 'color'
      }
      spikes.show();                // Update spikes with new contents
      delay(wait);                  // Pause for a moment
      firstPixelHue += 65536 / 90;  // One cycle of color wheel over 90 frames
    }
  }
}