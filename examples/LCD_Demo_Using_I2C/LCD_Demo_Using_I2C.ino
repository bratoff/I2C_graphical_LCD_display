
// Demo of KS0108B graphics LCD screen connected to MCP23017 16-port I/O expander

// Author: Nick Gammon
// Date: 14 February 2011
// Modified 10 August 2017 by Bruce Ratoff
//  to work with my modifications to Nick's library
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
  lcd.begin(11,12);   // data pin, clock pin for 595H mode

  unsigned long startTime = millis();           // Time these functions

  // draw all available letters
  for (int i = ' '; i <= 0x7f; i++)
    lcd.letter (i);

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

  lcd.gotoxy (80, 56);
  lcd.print(millis() - startTime);    // Display execution time
	lcd.print(" ms");
}  // end of setup

void loop () 
{}  // nothing to see here, move along



