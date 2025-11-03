int testled = 0;

void checkSerial()
{
  //routine to handle serial commands
if (Serial.available() > 0)
{

    byte inByte = Serial.read();

    switch (inByte)
    {
      case 'L':
      {
        Serial.println("Flashing LAMP!");
        flash_lamp();        
        break;
      }
      case 'U':
      {
        Serial.print("Test LED:");  
        //digitalWrite(UV_LED, HIGH);
        testled++;
        Serial.println(testled);
        turn_all_off();
        spikes.setPixelColor(testled, spikes.Color(255, 0, 0));        
        spikes.show();
        delay(1000); //debugging
        break;
      }
      case 'D':
      {
        Serial.print("Test LED:");  
        //digitalWrite(UV_LED, HIGH);
        testled--;
        Serial.println(testled);
        turn_all_off();
        spikes.setPixelColor(testled, spikes.Color(255, 0, 0));        
        spikes.show();
        delay(1000); //debugging
        break;     
      }
//      case 'u':
//      {
//        Serial.println(F("Turning OFF UV Emiiters!"));  
//        //digitalWrite(UV_LED, LOW);     
//        uv_brightness = 0;
//        break;
//      }
//      case 'v':
//      {
//        Serial.print(F("Direct UV control:"));
//        int temp = Serial.parseInt();
//        if (temp > 0 && temp < 256)
//           uv_brightness = (byte)temp;
//        Serial.println(uv_brightness);    
//        break;        
//      }
//      
//      case 's':
//      {
//        Serial.print(F("Sensitivity:"));
//        int temp = Serial.parseInt();
//        if (temp > 0 && temp < 256)
//           sensitivity = (byte)temp;
//        Serial.println(sensitivity);    
//        break;
//      }
//
     case 'M':
     {
       Serial.print(F("Mode?"));
       int temp = Serial.parseInt();
       if (temp >= 0 && temp <= 1)
          mode = (byte)temp;
       Serial.println(mode);
       if (EEPROM.read(1) != mode)
          EEPROM.write(1, mode);
       break;
     }
            
      
      default:
      {
        Serial.println("*******************");
        Serial.println("Commands:");
        Serial.println("L-Flash Lamps");
        Serial.println("U/D-LED TEST.");
        Serial.print("M:");Serial.println(mode);
        Serial.println("*******************");
        Serial.println("*******************");       
//        Serial.println(F("U/D/u-UV emitters!"));
//        Serial.println(F("M - Mode control [0-5]"));
//        Serial.println(F("s<sensitivty>-Change sensitivty."));
//        Serial.print(F("S:"));
//        Serial.println(sensitivity);
//        Serial.print(F("U:"));
//        Serial.println(uv_brightness);s
        break;
      }
        
      


      
    }
    
  
}  
  
}
