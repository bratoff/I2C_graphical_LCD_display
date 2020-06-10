/*
 I2C_graphical_LCD_display.cpp
 
 
 Written by Nick Gammon
 Date: 14 February 2011.
 
 HISTORY
 
 Version 1.0 : 15 February 2011
 Version 1.1 : 15 February 2011  -- added write-through cache
 Version 1.2 : 19 February 2011  -- allowed for more than 256 bytes in lcd.blit
 Version 1.3 : 21 February 2011  -- swapped some pins around to make it easier to make circuit boards *
 Version 1.4 : 24 February 2011  -- added code to raise reset line properly, also scrolling code *
 Version 1.5 : 28 February 2011  -- added support for SPI interface
 Version 1.6 : 13 March 2011     -- fixed bug in reading data from SPI interface
 Version 1.7 : 13 April 2011     -- made the bitmap for letter "Q" look a bit better
 Version 1.8 : 10 March 2012     -- adapted to work on Arduino IDE 1.0 onwards
 Version 1.9 : 26 May 2012       -- default to I2C rather than SPI on the begin() function
                                 -- also increased initial LCD_BUSY_DELAY from 20 to 50 uS
 Version 1.10:  8 July 2012      -- fixed issue with dropping enable before reading from display
 Version 1.11: 15 August 2014    -- added support for Print class, and an inverse mode
 
 Modifications by Bruce Ratoff
 Date: 03 December 2016
 
 Version 2.0 : 03 December 2016  -- added open and filled circle drawing capability
						 :									 -- optimized several drawing functions
 Version 2.1 : 10 December 2016  -- added setFont() to support additional fonts in letter()
 Version 2.2 : 11 December 2016  -- add edge clipping for all shapes

 Version 3.0 : 02 February 2017  -- add support for 74HC595 2-wire interface
 Version 3.1 : 08 February 2017  -- add optimized bit operations for AVR architecture
 Version 3.2 : 12 February 2017  -- optimize 2-wire by combining shift and latch operations
 Version 3.3 : 19 April 2017     -- optimize bit operations for popular ARM boards
                                 -- fixed compilation errors for ARM boards
																 -- add SPI and I2C includes so they're no longer needed in calling program

 Version 4.0 : 20 April 2017     -- Add #defines for each interface type so that extra libraries don't get included
                                 -- The library will assume the 74HC595 (2-wire) interface by default
																 -- To use MCP23017 interface, #define MCP23017
																 -- To use MCP23S17 interface, #define MCP23S17

 version 4.1 : 08 June 2020			 -- Added bit operation optimization for ESP8266																 
 
 * These changes required hardware changes to pin configurations
 
 SEE I2C_graphical_LCD_display.h FOR HARDWARE CONNECTIONS
   
 PERMISSION TO DISTRIBUTE
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 and associated documentation files (the "Software"), to deal in the Software without restriction, 
 including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in 
 all copies or substantial portions of the Software.
 
 
 LIMITATION OF LIABILITY
 
 The software is provided "as is", without warranty of any kind, express or implied, 
 including but not limited to the warranties of merchantability, fitness for a particular 
 purpose and noninfringement. In no event shall the authors or copyright holders be liable 
 for any claim, damages or other liability, whether in an action of contract, 
 tort or otherwise, arising from, out of or in connection with the software 
 or the use or other dealings in the software. 
 
 */


#include "I2C_graphical_LCD_display.h"

// SPI is so fast we need to give the LCD time to catch up.
// This is the number of microseconds we wait. Something like 20 to 50 is probably reasonable.
//  Increase this value if the display is either not working, or losing data.

#define LCD_BUSY_DELAY 50   // microseconds

// font data - each character is 8 pixels deep and 5 pixels wide

const byte font [96] [5] PROGMEM = {
  { 0x00, 0x00, 0x00, 0x00, 0x00 }, // space  (0x20)
  { 0x00, 0x00, 0x2F, 0x00, 0x00 }, // !
  { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "
  { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // #
  { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // $
  { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %
  { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &
  { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '
  { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // (
  { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // (
  { 0x14, 0x08, 0x3E, 0x08, 0x14 }, // *
  { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // +
  { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,
  { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -
  { 0x00, 0x30, 0x30, 0x00, 0x00 }, // .
  { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /
   
  { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 0  (0x30)
  { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 1
  { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2
  { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 3
  { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 4
  { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5
  { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 6
  { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7
  { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8
  { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 9
  { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :
  { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;
  { 0x08, 0x14, 0x22, 0x41, 0x00 }, // <
  { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =
  { 0x00, 0x41, 0x22, 0x14, 0x08 }, // >
  { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?
  
  { 0x32, 0x49, 0x79, 0x41, 0x3E }, // @  (0x40)
  { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // A
  { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // B
  { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // C
  { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // D
  { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // E
  { 0x7F, 0x09, 0x09, 0x09, 0x01 }, // F
  { 0x3E, 0x41, 0x49, 0x49, 0x7A }, // G
  { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // H
  { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // I
  { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // J
  { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // K
  { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // L
  { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, // M
  { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // N
  { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // O
  
  { 0x3F, 0x09, 0x09, 0x09, 0x06 }, // P  (0x50)
  { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // Q
  { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // R
  { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S
  { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // T
  { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // U
  { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // V
  { 0x3F, 0x40, 0x30, 0x40, 0x3F }, // W
  { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X
  { 0x07, 0x08, 0x70, 0x08, 0x07 }, // Y
  { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z
  { 0x00, 0x7F, 0x41, 0x41, 0x00 }, // [
  { 0x02, 0x04, 0x08, 0x10, 0x20 }, // backslash
  { 0x00, 0x41, 0x41, 0x7F, 0x00 }, // ]
  { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^
  { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _
  
  { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `  (0x60)
  { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a
  { 0x7F, 0x50, 0x48, 0x48, 0x30 }, // b
  { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c
  { 0x38, 0x44, 0x44, 0x48, 0x7F }, // d
  { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e
  { 0x08, 0x7E, 0x09, 0x01, 0x02 }, // f
  { 0x0C, 0x52, 0x52, 0x52, 0x3E }, // g
  { 0x7F, 0x08, 0x04, 0x04, 0x78 }, // h
  { 0x00, 0x44, 0x7D, 0x40, 0x00 }, // i
  { 0x20, 0x40, 0x44, 0x3D, 0x00 }, // j
  { 0x7F, 0x10, 0x28, 0x44, 0x00 }, // k
  { 0x00, 0x41, 0x7F, 0x40, 0x00 }, // l
  { 0x7C, 0x04, 0x18, 0x04, 0x78 }, // m
  { 0x7C, 0x08, 0x04, 0x04, 0x78 }, // n
  { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o
  
  { 0x7C, 0x14, 0x14, 0x14, 0x08 }, // p  (0x70)
  { 0x08, 0x14, 0x14, 0x08, 0x7C }, // q
  { 0x7C, 0x08, 0x04, 0x04, 0x08 }, // r
  { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s
  { 0x04, 0x3F, 0x44, 0x40, 0x20 }, // t
  { 0x3C, 0x40, 0x40, 0x20, 0x7C }, // u
  { 0x1C, 0x20, 0x40, 0x20, 0x1C }, // v
  { 0x3C, 0x40, 0x30, 0x40, 0x3C }, // w
  { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x
  { 0x0C, 0x50, 0x50, 0x50, 0x3C }, // y
  { 0x44, 0x64, 0x54, 0x4C, 0x44 }, // z
  { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {
  { 0x00, 0x00, 0x7F, 0x00, 0x00 }, // |
  { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }
  { 0x30, 0x08, 0x10, 0x20, 0x18 }, // ~
  { 0x7F, 0x55, 0x49, 0x55, 0x7F }  // unknown char (0x7F)
  
};

// Port manipulation macros for 2-wire interface
#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
#define clkpulse() {*_clkPort |= _clkMask; *_clkPort ^= _clkMask;}
#define setdata(v) {if(v) *_dataPort |= _dataMask; else *_dataPort &= ~_dataMask;}
#elif defined(digitalWriteFast)
#define clkpulse() {digitalWriteFast(_clkPin, HIGH); digitalWriteFast(_clkPin, LOW);}
#define setdata(v) {digitalWriteFast(_dataPin, v);}
#elif defined(ARDUINO_ARCH_ESP8266)
#define clkpulse() {GPOS = _clkMask; GPOC = _clkMask;}
#define setdata(v) {if(v) GPOS = _dataMask; else GPOC = _dataMask;}
#else
#define clkpulse() {digitalWrite(_clkPin, HIGH); digitalWrite(_clkPin, LOW);}
#define setdata(v) {digitalWrite(_dataPin, v);}
#endif

#define sendbit(v) {setdata(v); clkpulse();}

#if !defined(MCP23x17)
void I2C_graphical_LCD_display::do2wireSend(const byte rs, const byte data, const byte enable)
{
	sendbit(1);		// Leading 1 to eventually enable latch
	sendbit(0);		// fillers
	sendbit(0);
	sendbit(enable);	// LCD enable
	sendbit(rs);	// rs is 0 for command, 1 for data
	byte t = data;
	for(char i = 0; i < 8; ++i)	 // send data byte one bit at a time, LSB first
	{
		sendbit(t & 0x01);
		t >>= 1;
	}
	sendbit((_chipSelect & LCD_CS1) != 0);
	sendbit((_chipSelect & LCD_CS2) != 0);
	sendbit(1);			// Latch new data
	sendbit(0);
//	for(int i = 0; i < 15; ++i)
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
	clkpulse();		// Clear the shifter
}
#endif

#if defined(MCP23017)
// glue routines for version 1.0+ of the IDE
uint8_t i2c_read ()
{
#if defined(ARDUINO) && ARDUINO >= 100
  return Wire.read ();
#else
  return Wire.receive ();
#endif
} // end of Nunchuk::i2c_read

void i2c_write (int data)
{
#if defined(ARDUINO) && ARDUINO >= 100
  Wire.write (data);
#else
  Wire.send (data);
#endif
} // end of Nunchuk::i2c_write
#endif  //defined(MCP23017)

#if defined(MCP23x17)
// prepare for sending to MCP23017 
void I2C_graphical_LCD_display::startSend ()   
{
  
  if (_ssPin)
    {
    delayMicroseconds(LCD_BUSY_DELAY);
    digitalWrite (_ssPin, LOW); 
    SPI.transfer (_port << 1);
    }
  else
    Wire.beginTransmission (_port);
  
}  // end of I2C_graphical_LCD_display::startSend

// send a byte via SPI or I2C
void I2C_graphical_LCD_display::doSend (const byte what)   
{
  if (_ssPin)
    SPI.transfer (what);
  else
    i2c_write (what);
}  // end of I2C_graphical_LCD_display::doSend

// finish sending to MCP23017 
void I2C_graphical_LCD_display::endSend ()   
{
  if (_ssPin)
    digitalWrite (_ssPin, HIGH); 
  else
    Wire.endTransmission ();
 
}  // end of I2C_graphical_LCD_display::endSend
#endif


// set up - call before using
// specify 
//  * the port that the MCP23017 is on (default 0x20)
//  * the i2c port (default 0)
//  * the SPI SS (slave select) pin - leave as default of zero for I2C operation

// turns LCD on, clears memory, sets the cursor to 0,0

// Approx time to run: 600 ms on Arduino Uno
void I2C_graphical_LCD_display::begin (const byte port, 
                                       const byte i2cAddress,
                                       const byte ssPin)
{
#if !defined(MCP23x17)
//  if (_clkPin > 0 && _dataPin > 0)		//If 2-argument constructor was used, it's the 2-wire interface
//  {
		_port = 0;
		_ssPin = 0;
#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
		_clkPort = portOutputRegister(digitalPinToPort(_clkPin));
		_dataPort = portOutputRegister(digitalPinToPort(_dataPin));
		_clkMask = digitalPinToBitMask(_clkPin);
		_dataMask = digitalPinToBitMask(_dataPin);
#endif
#if defined(ARDUINO_ARCH_ESP8266)
		_clkMask = 1<<_clkPin;
		_dataMask = 1<<_dataPin;
#endif
		pinMode(_clkPin, OUTPUT);
		pinMode(_dataPin, OUTPUT);
		digitalWrite(_clkPin, LOW);
		_chipSelect = 0;
		do2wireSend(0, 0, 0);			// clear the shifter and latch
//  }
//  else																// it's the MCP23x17 port expander interface
#else
//  {
		_port = port;   // remember port
		_ssPin = ssPin; // and SPI slave select pin
			
		if (_ssPin)
			SPI.begin ();
		else
			Wire.begin (i2cAddress);   

		// un-comment next line for faster I2C communications:
		//   TWBR = 12;

		// byte mode (not sequential)
		expanderWrite (IOCON, 0b00100000);
			
		// all pins as outputs
		expanderWrite (IODIRA, 0);
		expanderWrite (IODIRB, 0);
			
		// take reset line low
		startSend ();
			doSend (GPIOA);
			doSend (0); // all lines low
		endSend ();
			
		// now raise reset (and enable) line and wait briefly
		expanderWrite (GPIOA, LCD_ENABLE | LCD_RESET);
//  }
#endif

	delay(2);	// time for LCD to finish resetting

  // Select default built-in font
	setFont();

  // turn LCD chip 1 on
  _chipSelect = LCD_CS1;
  cmd (LCD_ON);

  // turn LCD chip 2 on
  _chipSelect = LCD_CS2;
  cmd (LCD_ON);
  
  // clear entire LCD display
  clear ();
  
  // and put the cursor in the top-left corner
  gotoxy (0, 0);
  
  // ensure scroll is set to zero
  scroll (0);   
  
}  // end of I2C_graphical_LCD_display::begin (initializer)


// send command to LCD display (chip 1 or 2 as in chipSelect variable)
// for example, setting page (Y) or address (X)
void I2C_graphical_LCD_display::cmd (const byte data)
{
//	if (_clkPin > 0)
//	{
#if !defined(MCP23x17)
		do2wireSend(0, data, 1);		// rs is 0 meaning instruction, raise enable
		do2wireSend(0, data, 0);		// now drop enable
//	}
//	else
//	{
#else
  startSend ();
    doSend (GPIOA);                      // control port
    doSend (LCD_RESET | LCD_ENABLE | _chipSelect);   // set enable high (D/I is low meaning instruction) 
    doSend (data);                       // (command written to GPIOB)
    doSend (LCD_RESET | _chipSelect);    // (GPIOA again) pull enable low to toggle data 
  endSend ();
//	}
#endif
} // end of I2C_graphical_LCD_display::cmd 

// set our "cursor" to the x/y position
// works out whether this refers to chip 1 or chip 2 and sets chipSelect appropriately

// Approx time to run: 33 ms on Arduino Uno
void I2C_graphical_LCD_display::gotoxy (byte x, 
                                        byte y)
{

  if (x > 127) 
    x = 0;                
  if (y > 63)  
    y = 0;
  
#ifdef WRITETHROUGH_CACHE
  _cacheOffset = 0;
#endif
  
  // work out which chip
  if (x >= 64)
    {
    x -= 64;  
    _chipSelect = LCD_CS2;
#ifdef WRITETHROUGH_CACHE
    _cacheOffset = 64 * 64 / 8;  // half-way through cache
#endif
      
    }
  else
    _chipSelect = LCD_CS1;
  
  // remember for incrementing later
  _lcdx = x;
  _lcdy = y;
  
  // command LCD to the correct page and address
  cmd (LCD_SET_PAGE | (y >> 3) );  // 8 pixels to a page
  cmd (LCD_SET_ADD  | x );          
  
#ifdef WRITETHROUGH_CACHE
  _cacheOffset += (x << 3) | y >> 3;
#endif  
}  // end of I2C_graphical_LCD_display::gotoxy 

#if defined(MCP23x17)
// set register "reg" on expander to "data"
// for example, IO direction
void I2C_graphical_LCD_display::expanderWrite (const byte reg, 
                                               const byte data ) 
{
  startSend ();
    doSend (reg);
    doSend (data);
  endSend ();
} // end of I2C_graphical_LCD_display::expanderWrite
#endif

// read the byte corresponding to the selected x,y position
byte I2C_graphical_LCD_display::I2C_graphical_LCD_display::readData ()
{
  
#if defined(WRITETHROUGH_CACHE)
  return _cache [_cacheOffset];

#else
  // data port (on the MCP23017) is now input
  expanderWrite (IODIRB, 0xFF);
  
  // lol, see the KS0108 spec sheet - you need to read twice to get the data
  startSend ();
    doSend (GPIOA);                  // control port
    doSend (LCD_RESET | LCD_READ | LCD_DATA | LCD_ENABLE | _chipSelect);  // set enable high 
  endSend ();

  startSend ();
    doSend (GPIOA);                  // control port
    doSend (LCD_RESET | LCD_READ | LCD_DATA | _chipSelect);  // pull enable low to toggle data 
  endSend ();

  startSend ();
  doSend (GPIOA);                  // control port
  doSend (LCD_RESET | LCD_READ | LCD_DATA | LCD_ENABLE | _chipSelect);  // set enable high 
  endSend ();

  byte data;

  if (_ssPin)
    {
    digitalWrite (_ssPin, LOW); 
    SPI.transfer ((_port << 1) | 1);  // read operation has low-bit set
    SPI.transfer (GPIOB);             // which register to read from
    data = SPI.transfer (0);          // get byte back
    digitalWrite (_ssPin, HIGH); 
    }
  else
    {
    // initiate blocking read into internal buffer
    Wire.requestFrom (_port, (byte) 1);
    
    // don't bother checking if available, Wire.receive does that anyway
    //  also it returns 0x00 if nothing there, so we don't need to bother doing that
    data = i2c_read ();
    }  

  // drop enable AFTER we have read it
  startSend ();
  doSend (GPIOA);                  // control port
  doSend (LCD_RESET | LCD_READ | LCD_DATA | _chipSelect);  // pull enable low to toggle data 
  endSend ();

  // data port (on the MCP23017) is now output again
  expanderWrite (IODIRB, 0);
  
  return data;
#endif
}  // end of I2C_graphical_LCD_display::readData

// write a byte to the LCD display at the selected x,y position
// if inv true, invert the data
// writing advances the cursor 1 pixel to the right
// it wraps to the next "line" if necessary (a line is 8 pixels deep)
void I2C_graphical_LCD_display::writeData (byte data, 
                                           const boolean inv)
{
  // invert data to be written if wanted
  if (inv)
    data ^= 0xFF;
  
//  if (_clkPin > 0)		// Are we using the 2-wire interface
//	{
#if !defined(MCP23x17)
		do2wireSend(1, data, 1);		// rs is 1 to indicate data, raise enable
		do2wireSend(1, data, 0);		// now drop enable
//	}
//	else
//	{
#else
	// note that the MCP23x17 automatically toggles between port A and port B
  // so the four sends do this:
  //   1. Choose initial port as GPIOA (general IO port A)
  //   2. Port A: set E high
  //   3. Port B: send the data byte
  //   4. Port A: set E low to toggle the transfer of data

  startSend ();
    doSend (GPIOA);                  // control port
    doSend (LCD_RESET | LCD_DATA | LCD_ENABLE | _chipSelect);  // set enable high 
    doSend (data);                   // (screen data written to GPIOB)
    doSend (LCD_RESET | LCD_DATA | _chipSelect);  // (GPIOA again) pull enable low to toggle data 
  endSend ();
//	}
#endif

#ifdef WRITETHROUGH_CACHE
  _cache [_cacheOffset] = data;
#endif 
  
  // we have now moved right one pixel (in the LCD hardware)
  _lcdx++;

  
  // see if we moved from chip 1 to chip 2, or wrapped at end of line
  if (_lcdx >= 64)
    {
    if (_chipSelect == LCD_CS1)  // on chip 1, move to chip 2
      gotoxy (64, _lcdy);
    else
      gotoxy (0, _lcdy + 8);  // go back to chip 1, down one line
    }  // if >= 64
  else
    {
#ifdef WRITETHROUGH_CACHE
    _cacheOffset += 8;
#endif
    }
  
}  // end of I2C_graphical_LCD_display::writeData


// write one letter (space to 0x7F), inverted or normal

// Approx time to run: 4 ms on Arduino Uno
void I2C_graphical_LCD_display::letter (byte c, 
                                        const boolean inv)
{
  if (c < _fStart || c > (_fStart + _fLength - 1))
    c = _fStart + _fLength - 1;  // unknown glyph
  
  c -= _fStart; // force into range of our font table
  
  // no room for a whole character? drop down a line
  // letters are 5 wide, so once we are past 59, there isn't room before we hit 63
  if (_lcdx >= (65 - (_fWidth + (_fSpace ? 1 : 0))) && _chipSelect == LCD_CS2)
    gotoxy (0, _lcdy + 8);
  
  // font data is in PROGMEM memory (firmware)
  for (int x = 0; x < _fWidth; x++)
    writeData (pgm_read_byte (_fMap + (c * _fWidth) + x), inv);
  if( _fSpace )
    writeData (0, inv);  // one-pixel gap between letters

}  // end of I2C_graphical_LCD_display::letter

// write an entire null-terminated string to the LCD: inverted or normal
void I2C_graphical_LCD_display::string (const char * s, 
                                        const boolean inv)
{
  char c;
  while ((c = *(s++)))
    letter (c, inv); 
}  // end of I2C_graphical_LCD_display::string

// blits (copies) a series of bytes to the LCD display from an array in PROGMEM

// Approx time to run: 2 ms/byte on Arduino Uno
void I2C_graphical_LCD_display::blit (const byte * pic, 
                                      const unsigned int size)
{
  for (unsigned int x = 0; x < size; x++, pic++)
    writeData (pgm_read_byte (pic));
}  // end of I2C_graphical_LCD_display::blit

// clear rectangle x1,y1,x2,y2 (inclusive) to val (eg. 0x00 for black, 0xFF for white)
// default is entire screen to black
// rectangle is forced to nearest (lower) 8 pixels vertically
// this if faster than lcd_fill_rect because it doesn't read from the display

// Approx time to run: 120 ms on Arduino Uno for 20 x 50 pixel rectangle
void I2C_graphical_LCD_display::clear (const byte x1,    // start pixel
                                       const byte y1,     
                                       const byte x2,  // end pixel
                                       const byte y2,   
                                       const byte val)   // what to fill with 
{
  for (byte y = y1; y <= y2; y += 8)
    {
    gotoxy (x1, y);
    for (byte x = x1; x <= x2; x++)
      writeData (val);
    } // end of for y
  
  gotoxy (x1, y1);
} // end of I2C_graphical_LCD_display::clear

// set or clear a pixel at x,y
// warning: this is slow because we have to read the existing pixel in from the LCD display
// so we can change a single bit in it
void I2C_graphical_LCD_display::setPixel (const byte x, 
                                          const byte y, 
                                          const byte val)
{
	if (x < 128 && y < 64)
	{
	  // select appropriate page and byte
	  gotoxy (x, y);

	  // get existing pixel values
	  byte c = readData ();
	  
	  // toggle or clear this particular one as required
	  if (val)
		c |=   1 << (y & 7);    // set pixel
	  else
		c &= ~(1 << (y & 7));   // clear pixel
	  
	#ifndef WRITETHROUGH_CACHE
	  // go back to that place (because readData() moved it)
	  gotoxy (x, y);
	#endif

	  // write changed data back
	  writeData (c);
	} 
}  // end of I2C_graphical_LCD_display::setPixel

// fill the rectangle x1,y1,x2,y2 (inclusive) with black (1) or white (0)
// if possible use lcd_clear instead because it is much faster
// however lcd_clear clears batches of 8 vertical pixels

// Approx time to run: 5230 ms on Arduino Uno for 20 x 50 pixel rectangle
//    (Yep, that's over 5 seconds!)
void I2C_graphical_LCD_display::fillRect (const byte x1, // start pixel
                                          const byte y1,     
                                          const byte x2, // end pixel
                                          const byte y2,    
                                          const byte val)  // what to draw (0 = white, 1 = black) 
{
  for (byte y = y1; y <= y2; y++)
	for (byte x = x1; x <= x2; x++)
      setPixel (x, y, val);
}  // end of I2C_graphical_LCD_display::fillRect

// frame the rectangle x1,y1,x2,y2 (inclusive) with black (1) or white (0)
// width is width of frame, frames grow inwards

// Approx time to run:  730 ms on Arduino Uno for 20 x 50 pixel rectangle with 1-pixel wide border
//             1430 ms on Arduino Uno for 20 x 50 pixel rectangle with 2-pixel wide border
void I2C_graphical_LCD_display::frameRect (const byte x1, // start pixel
                                           const byte y1,     
                                           const byte x2, // end pixel
                                           const byte y2,    
                                           const byte val,    // what to draw (0 = white, 1 = black) 
                                           const byte width)
{
  byte x, y, i;
  
  // top and bottom lines
  for (x = x1; x <= x2; x++)
    for (i = 0; i < width; i++)
	{
      setPixel (x, y1 + i, val);
      setPixel (x, y2 - i, val);
    }
  
  // left and right lines
  for (y = y1; y <= y2; y++)
    for (i = 0; i < width; i++)
	{
      setPixel (x1 + i, y, val);
      setPixel (x2 - i, y, val);
    }
  
}  // end of I2C_graphical_LCD_display::frameRect

// draw a line from x1,y1 to x2,y2 (inclusive) with black (1) or white (0)
// Warning: fairly slow, as is anything that draws individual pixels
void I2C_graphical_LCD_display::line  (const byte x1,  // start pixel
                                       const byte y1,     
                                       const byte x2,  // end pixel
                                       const byte y2,   
                                       const byte val)  // what to draw (0 = white, 1 = black) 
{
  byte x, y;
  
  // vertical line? do quick way
  if (x1 == x2)
    {
    for (y = y1; y <= y2; y++)
      setPixel (x1, y, val);
    return;
  }
  
  // horizontal line? do quick way
  if (y1 == y2)
    { 
    for (x = x1; x <= x2; x++)
      setPixel (x, y1, val);
    return;
    }
  
  int x_diff = x2 - x1,
      y_diff = y2 - y1;
  
  // if x difference > y difference, draw every x pixels
  if (abs(x_diff) > abs (y_diff))
  {
    int x_inc = 1;
    int y_inc = (y_diff << 8) / x_diff;
    int y_temp = y1 << 8;
    if (x_diff < 0)
      x_inc = -1;
    
    for (x = x1; x != x2; x += x_inc)
      {
      setPixel (x, y_temp >> 8, val);
      y_temp += y_inc;
      }
    
    return;
  }

  // otherwise draw every y pixels
  int x_inc = (x_diff << 8) / y_diff;
  int y_inc = 1;
  if (y_diff < 0)
    y_inc = -1;
  
  int x_temp = x1 << 8;
  for (y = y1; y != y2; y += y_inc)
  {
    setPixel (x_temp >> 8, y, val);
    x_temp += x_inc;
  }
	
	// Last pixel must be at specified endpoint
	setPixel(x2, y2, val);
  
  
} // end of I2C_graphical_LCD_display::line

// set scroll position to y
void I2C_graphical_LCD_display::scroll (const byte y)   // set scroll position
{
  if (y < 64)
  {
	  byte old_cs = _chipSelect;
	  _chipSelect = LCD_CS1;
	  cmd (LCD_DISP_START | (y & 0x3F) );  // set scroll position
	  _chipSelect = LCD_CS2;
	  cmd (LCD_DISP_START | (y & 0x3F) );  // set scroll position
	  _chipSelect = old_cs;
  }
} // end of I2C_graphical_LCD_display::scroll

// BRR 03-Dec-2016

//	Draw an open circle with center (x0,y0) and radius r in color val
//	Uses midpoint algorithm - compute one octant and reflect it 7 times
void I2C_graphical_LCD_display::circle (const byte x0,		// center point x
		   const byte y0,		// center point y
		   const byte r,		// radius
		   const byte val)		// color (0 = white, 1 = black)
{
	byte x = r;
	byte y = 0;
	int err = 0;

	while(x >= y)
	{
		setPixel(x0 + x, y0 + y, val);
		setPixel(x0 + y, y0 + x, val);
		if (y <= x0)
		{
			setPixel(x0 - y, y0 + x, val);
			if (x <= y0)
				setPixel(x0 - y, y0 - x, val);
		}
		if (x <= x0)
		{
			setPixel(x0 - x, y0 + y, val);
			if (y <= y0)
				setPixel(x0 - x, y0 - y, val);
		}
		if (x <= y0)
			setPixel(x0 + y, y0 - x, val);
		if (y <= y0)
			setPixel(x0 + x, y0 - y, val);
		
		++y;
		err += 1 + 2*y;
		if(2*(err-x) + 1 > 0)
		{
			--x;
			err += 1 - 2*x;
		}
	}
}  // circle

//	Draw a filled circle with center (x0,y0) and radius r in color val
//	Adaptation of midpoint algorithm - fill a circle by
//	drawing lines between horizontally opposing octants
void I2C_graphical_LCD_display::fillCircle (const byte x0,		// center point x
											  const byte y0,		// center point y
											  const byte r,			// radius
											  const byte val)		// color (0 = white, 1 = black)
{
	byte x = r;
	byte y = 0;
	int err = 0;
	byte xx;
	
	while(x >= y)
	{
		for (xx=x0-x; xx<=x0+x; ++xx)
		{
			setPixel(xx, y0+y, val);
			if (y <= y0)
				setPixel(xx, y0-y, val);
		}
		for (xx=x0-y; xx<=x0+y; ++xx)
		{
			setPixel(xx, y0+x, val);
			if (x <= y0)
				setPixel(xx, y0-x, val);
		}

		++y;
		err += 1 + 2*y;
		if(2*(err-x) + 1 > 0)
		{
			--x;
			err += 1 - 2*x;
		}
	}
}
//  filledCircle

void I2C_graphical_LCD_display::setFont (const void * fontMap,
										 const int width,
										 const bool space,
										 const byte start,
										 const int length)
{
	if (fontMap == NULL)
	{
		_fMap = (byte *)font;
		_fWidth = 5;
		_fSpace = true;
		_fStart = 0x20;
		_fLength = 96;
	}
	else
	{
		_fMap = (const byte *)fontMap;
		_fWidth = width;
		_fSpace = space;
		_fStart = start;
		_fLength = length;
	}
}