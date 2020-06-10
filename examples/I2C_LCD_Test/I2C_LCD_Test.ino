
// Demo of KS0108B graphics LCD screen connected to MCP23017 16-port I/O expander

// Author: Nick Gammon
// Date: 14 February 2011

// Modified by Bruce Ratoff
// Date: 03 December 2016
// - Use CP437 for character test
// - Separate demo into multiple pages with pauses
// - Add circle and filled circle demos for updated library
// Date: 10 December 2016
// - Use setInv() and print(F()) instead of string()
// - Use new setFont() function from updated library to set CP437 font
// - Add animations in loop()
// Date: 20 April 2017
// - Move I2C and SPI header includes to I2C_graphical_LCD_display.h
//   (so they are no longer needed in calling program)

#include <I2C_graphical_LCD_display.h>

I2C_graphical_LCD_display lcd(0,2);

#include <cp437_font.h>

// example bitmap
const byte picture [] PROGMEM = {
 0x1C, 0x22, 0x49, 0xA1, 0xA1, 0x49, 0x22, 0x1C,  // face  
 0x10, 0x08, 0x04, 0x62, 0x62, 0x04, 0x08, 0x10,  // star destroyer
 0x4C, 0x52, 0x4C, 0x40, 0x5F, 0x44, 0x4A, 0x51,  // OK logo
};

void setup () 
{
  Serial.begin(115200);
  lcd.begin ();

  lcd.setFont();  // set to default font
  for (int i = 32; i<96; ++i)     // draw all characters
    lcd.letter(i);
  delay(3000);
  
  lcd.clear();
  lcd.setFont(cp437_font, 8, false, 0, 256);  // switch to fancy font
  for (int i = 0; i <= 0x7f; i++)   // draw first 128 characters
    lcd.letter(i);
  delay(3000);

  lcd.clear();
  for (int i = 0x80; i <= 0xff; i++)
    lcd.letter(i);    // draw upper 128 characters
  delay(3000);

  lcd.setFont();    // switch back to default font

  lcd.clear();
  // filled box  
  lcd.clear (7, 40, 31, 63, 0xFF);

  // draw text in inverse
  lcd.gotoxy (48, 0);
  lcd.setInv(true);
  lcd.print(F("Hello"));
  lcd.setInv(false);
  lcd.gotoxy (48, 24);
  lcd.print(F("Bruce"));

  // bit blit in a picture
  lcd.gotoxy (76, 56);
  lcd.blit (picture, sizeof picture);
  
  // draw a framed rectangle
  lcd.frameRect (76, 40, 96, 53, 1, 1);

  // draw a white diagonal line
  lcd.line (7, 40, 31, 63, 0);

  // draw a white circle
  lcd.circle(107, 16, 13, 1);

  // draw a white filled circle
  lcd.fillCircle(20, 16, 13, 1);

}  // end of setup

void loop () 
{
  int xdir = 2;
  int xrd = 1;
  int ydir = 2;
  int yrd = 1;
  int r = 10;
  int s = 2*r;
  int x = r+1;
  int rx = 126-s;
  int y = r+1;
  int ry = 1;
  long tstart;
  while (true)
  {
    delay(0);   // ESP8266 needs this to keep watchdog from resetting
    tstart = millis();
    lcd.circle(x,y,r,1);
    lcd.circle(x,y,r,0);
    lcd.frameRect(rx, ry, rx+s, ry+s, 1, 1);
    lcd.frameRect(rx, ry, rx+s, ry+s, 0, 1);
    if((x+r) > 126 || (x-r) < 1)
      xdir = -xdir;
    if((y+r) > 62 || (y-r) < 1)
      ydir = -ydir;
    if((rx+s) > 126 || rx < 1)
      xrd = -xrd;
    if((ry+s) > 62 || ry < 1)
      yrd = -yrd;
    x += xdir;
    y += ydir;
    rx += xrd;
    ry += yrd;
    Serial.println(millis()-tstart);
  }
}
