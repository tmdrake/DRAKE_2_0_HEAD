
void eyesbrightness(float brightness, bool red) {
  /*
    EYE LED's are on: 0-3
    brightness values 0.0-1.1
    interrupt code
    */
  //uint32_t lastcolors[5];
  //spikes.setPixelColor(1, spikes.Color(150 * _brightness , 0, 255 * _brightness ));
  for (int i = 0; i < 4; i++) {
    //backup last color
    //lastcolors[i] = spikes.getPixelColor(i);
    
    if (red)
      spikes.setPixelColor(i, spikes.Color(150 * brightness, 0, 0)); //Red eyes 
    else
      spikes.setPixelColor(i, spikes.Color(Red(spikes.getPixelColor(i) )* brightness, Green(spikes.getPixelColor(i) )* brightness, Blue(spikes.getPixelColor(i))* brightness));
    
  }  

  

  //restore last color information
  /*
  for (int i = 0; i < 4; i++) {
    spikes.setPixelColor(i, lastcolors[i]);
  }*/
  //but do not show, use it for the loop
  
  //spikes.setBrightness(uint8_t)
}


// Return color, dimmed by 75% (used by scanner)
// uint32_t DimColor(uint32_t color) {
//   uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
//   return dimColor;
// }

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color) {
  return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color) {
  return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color) {
  return color & 0xFF;
}