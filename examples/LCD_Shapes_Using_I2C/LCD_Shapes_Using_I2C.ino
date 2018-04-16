
// Demo of KS0108B graphics LCD screen connected to MCP23017 16-port I/O expander
// This demo shows the shape drawing functions
// uses Nick Gammon's KS0108 library as modified by Bruce Ratoff

// Author: Bruce Ratoff
// Date: 10 August 2017
//

#include <I2C_graphical_LCD_display.h>

I2C_graphical_LCD_display lcd;

// example bitmap
const byte picture [] PROGMEM = {
 0x1C, 0x22, 0x49, 0xA1, 0xA1, 0x49, 0x22, 0x1C,  // face  
 0x10, 0x08, 0x04, 0x62, 0x62, 0x04, 0x08, 0x10,  // star destroyer
 0x4C, 0x52, 0x4C, 0x40, 0x5F, 0x44, 0x4A, 0x51,  // OK logo
};

void setup () 
{
//  lcd.begin ();     // Use this for I2C mode
//  lcd.begin(0x20, 0, 10);   // Use this for SPI mode
  lcd.begin(11, 12);  // Use this for 595H mode (data pin, clock pin)

  unsigned long startTime = millis();

  for (int r = 4; r < 17; r+=4) {
    lcd.circle(20, 20, r, 1);
  }

  lcd.fillCircle(60, 20, 16, 1);

  // black box  
  lcd.clear (6, 40, 30, 63, 0xFF);

  // draw text in inverse
  lcd.gotoxy (40, 40);
  lcd.setInv(true);
  lcd.print ("Bruce KO4XL");
  lcd.setInv(false);

  // bit blit in a picture
  lcd.gotoxy (40, 56);
  lcd.blit (picture, sizeof picture);
  
  // draw a framed rectangle
  lcd.frameRect (40, 49, 60, 53, 1, 1);

  // draw a white diagonal line
  lcd.line (6, 40, 30, 63, 0);

  lcd.gotoxy(72, 56);
  lcd.print(millis() - startTime);
  lcd.print(" ms");
  
}  // end of setup

void loop () 
{}  // nothing to see here, move along



