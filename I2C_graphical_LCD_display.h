/*
 I2C_graphical_LCD_display.h
 
 
 Written by Nick Gammon.
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
 
 Forked by Bruce Ratoff
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
																 -- These defines must appear in calling app PRIOR to including this library

 version 4.1 : 08 June 2020			 -- Added bit operation optimization for ESP8266																 

  * These changes required hardware changes to pin configurations

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
//#define DEBUG

#ifndef I2C_graphical_LCD_display_H
#define I2C_graphical_LCD_display_H

// Define this to cache display content instead of reading back from display
//#define WRITETHROUGH_CACHE

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include <WProgram.h>
#endif

#if defined(MCP23017)
#include <Wire.h>
#elif defined(MCP23S17)
#include <SPI.h>
#endif

#if defined(MCP23017) || defined(MCP23S17)
#define MCP23x17
#endif

// WRITETHROUGH_CACHE must be defined if 2-wire interface is used
#if !defined(MCP23x17) && !defined(WRITETHROUGH_CACHE)
#define WRITETHROUGH_CACHE
#endif

#if defined(__AVR__)
#include <avr/pgmspace.h>
#elif !defined(pgm_read_byte)
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

// MCP23017/MCP23S17 registers (everything except direction defaults to 0)

#define IODIRA   0x00   // IO direction  (0 = output, 1 = input (Default))
#define IODIRB   0x01
#define IOPOLA   0x02   // IO polarity   (0 = normal, 1 = inverse)
#define IOPOLB   0x03
#define GPINTENA 0x04   // Interrupt on change (0 = disable, 1 = enable)
#define GPINTENB 0x05
#define DEFVALA  0x06   // Default comparison for interrupt on change (interrupts on opposite)
#define DEFVALB  0x07
#define INTCONA  0x08   // Interrupt control (0 = interrupt on change from previous, 1 = interrupt on change from DEFVAL)
#define INTCONB  0x09
#define IOCON    0x0A   // IO Configuration: bank/mirror/seqop/disslw/haen/odr/intpol/notimp
//#define IOCON 0x0B  // same as 0x0A
#define GPPUA    0x0C   // Pull-up resistor (0 = disabled, 1 = enabled)
#define GPPUB    0x0D
#define INFTFA   0x0E   // Interrupt flag (read only) : (0 = no interrupt, 1 = pin caused interrupt)
#define INFTFB   0x0F
#define INTCAPA  0x10   // Interrupt capture (read only) : value of GPIO at time of last interrupt
#define INTCAPB  0x11
#define GPIOA    0x12   // Port value. Write to change, read to obtain value
#define GPIOB    0x13
#define OLLATA   0x14   // Output latch. Write to latch output.
#define OLLATB   0x15


/*

 My mappings of the KS0108 registers:
 

 LCD PIN  MCP23017 PIN  Name   Purpose
 
 ---- Wire these pins together as shown ------
 
 --- Port "A" - control lines
 
  6      28 (GPA7)     E      Enable data transfer on 1 -> 0 transition  (see LCD_ENABLE)
  5      27 (GPA6)     R/~W   1 = read, 0 = write (to LCD) (see LCD_READ)
  4      26 (GPA5)     D/~I   1 = data, 0 = instruction    (see LCD_DATA)
 17      25 (GPA4)     ~RST   1 = not reset, 0 = reset
 16      24 (GPA3)     CS2    Chip select for IC2 (1 = active)  (see LCD_CS2)
 15      23 (GPA2)     CS1    Chip select for IC1 (1 = active)  (see LCD_CS1)
 
 --- Port "B" - data lines
 
  7      1  (GPB0)     DB0    Data bit 0
  8      2  (GPB1)     DB1    Data bit 1
  9      3  (GPB2)     DB2    Data bit 2
 10      4  (GPB3)     DB3    Data bit 3
 11      5  (GPB4)     DB4    Data bit 4
 12      6  (GPB5)     DB5    Data bit 5
 13      7  (GPB6)     DB6    Data bit 6
 14      8  (GPB7)     DB7    Data bit 7
 
 ---- Pins on LCD display which are not connected to the I/O expander ----
 
  1                    GND    LCD logic ground
  2                    +5V    LCD logic power
  3                    V0     Contrast - connect to contrast pot, middle (wiper)
 17                   ~RST    Tie to +5V via 10K resistor (reset signal)
 18                    Vee    Negative voltage - connect to contrast pot, one side *
 19                    A      Power supply for LED light (+5V)  A=anode
 20                    K      GND for LED light                 K=cathode
 
 * Third leg of contrast pot is wired to ground.
 
 ---- Pins on MCP23017 which are not connected to the LCD display ----
 
  9   (VDD)            +5V    Power for MCP23017
 10   (VSS)            GND    Ground for MCP23017
 11   (CS)             SS     (Slave Select) - connect to Arduino pin D10 if using SPI (D53 on the Mega)
 12   (SCL/SCK)        SCL    (Clock) - connect to Arduino pin A5 for I2C (D21 on the Mega) (for SPI SCK: D13, or D52 on the Mega)
 13   (SDA/SI)         SDA    (Data)  - connect to Arduino pin A4 for I2C (D20 on the Mega) (for SPI MOSI: D11, or D51 on the Mega)
 14   (SO)             MISO   (SPI slave out) - connect to Arduino pin D12 if using SPI (D50 on the Mega)
 15   (A0)             Address jumper 0 - connect to ground (unless you want a different address)
 16   (A1)             Address jumper 1 - connect to ground
 17   (A2)             Address jumper 2 - connect to ground
 18   (~RST)           Tie to +5V via 10K resistor (reset signal)
 19   (INTA)           Interrupt for port A (not used)
 20   (INTB)           Interrupt for port B (not used)
 21   (GPA0)           Not used
 22   (GPA1)           Not used
 23   (GPA2)           Not used
 
 IMPORTANT: For reliable operation:
			1.	Place 0.1 ufd bypass capacitors across pins 1&2 of the LCD and across pins 9&10 of the MCP23017.
			2.	If using I2C, add 4.7K pull-ups from pin 12 to +5v and pin 13 to +5v on the MCP23017.

======== CONNECTIONS FOR 74HC595 2-wire INTERFACE ========

LCD pins 1,2,3,18,19,20 are connected same as above
LCD pin 5 (R/~W) is connected to GND
LCD pin 17 (~RST) is tied to +5v via a 10K resistor

---- Pins on 74HC595 (uses 2) ----
IC1
  1	  (QB)			   CS2	   LCD pin 16
  2   (QC)			   CS1	   LCD pin 15
  3   (QD)			   D7		   LCD pin 14
  4   (QE)         D6      LCD pin 13
  5   (QF)         D5      LCD pin 12
  6   (QG)         D4      LCD pin 11
  7   (QH)         D3      LCD pin 10
  8   (VSS)        GND     Ground for IC1
  9   (QH*)        Carry   IC2 pin 14 (SER) IC1 shift register out to IC2 shift register in
 10   (SCL)        +5V     IC1 clear input pulled high (always disabled)
 11   (SCK)        CLK     Arduino output pin for CLK (NOTE: Same for IC1 and IC2)
 12   (RCK)        LATCH   IC2 pin 12 and anode of D1
 13   (G)          GND     IC1 output enable pulled low (always enabled)
 14   (SER)        DATA    Free side of R1 (see below) and Arduino output pin for DATA
 15   (QA)                 No connection
 16   (VDD)        +5V     Power for IC1
 
IC2
  1	  (QB)			   D1	   	 LCD pin 8
  2   (QC)			   D0	   	 LCD pin 7
  3   (QD)			   DI	   	 LCD pin 4
  4   (QE)         Enable	 LCD pin 6
  5   (QF)               	 No connection
  6   (QG)                 No connection
  7   (QH)                 No connection
  8   (VSS)        GND     Ground for IC2
  9   (QH*)        Control Cathode of D1
 10   (SCL)        +5V     IC2 clear input pulled high (always disabled)
 11   (SCK)        CLK     Arduino output pin for CLK (NOTE: Same for IC1 and IC2)
 12   (RCK)        LATCH   IC1 pin 12 and anode of D1 (NOTE: Same for IC1 and IC2)
 13   (G)          GND     IC2 output enable pulled low (always enabled - Same for IC1 and IC2)
 14   (SER)        Carry   IC1 pin 9 (QH*) IC2 shift register in from IC1 shift register out
 15   (QA)         D2      LCD pin 9
 16   (VDD)        +5V     Power for IC2
 
2-wire Control circuit:
																	o Control (from IC2 pin 9)
																	|
																	D1 (cathode)
																	D1 (anode)
																	|
	DATA (from Arduino) o----R1-----o----o LATCH (to IC1 and IC2 pin 12)
													(1K)

LCD Power On Reset circuit:
								   +5V
									|
									R3 (1K)
									|
									o----o LCD_RST (LCD pin 17)
									|
									C3 (1uf)
									|
									V
								   GND

For reliable operation, place a 0.1 ufd bypass capacitor across power pins (8 and 16) of each 74HC595

*/


// GPA port - these show which wires from the LCD are connected to which pins on the I/O expander

#define LCD_CS1    0b00000100   // chip select 1  (pin 23)                            0x04
#define LCD_CS2    0b00001000   // chip select 2  (pin 24)                            0x08
#define LCD_RESET  0b00010000   // reset (pin 25)                                     0x10
#define LCD_DATA   0b00100000   // 1xxxxxxx = data; 0xxxxxxx = instruction  (pin 26)  0x20
#define LCD_READ   0b01000000   // x1xxxxxx = read; x0xxxxxx = write  (pin 27)        0x40
#define LCD_ENABLE 0b10000000   // enable by toggling high/low  (pin 28)              0x80


// Commands sent when LCD in "instruction" mode (LCD_DATA bit set to 0)

#define LCD_ON          0x3F
#define LCD_OFF         0x3E
#define LCD_SET_ADD     0x40   // plus X address (0 to 63) 
#define LCD_SET_PAGE    0xB8   // plus Y address (0 to 7)
#define LCD_DISP_START  0xC0   // plus X address (0 to 63) - for scrolling

class I2C_graphical_LCD_display : public Print
{
private:
  
  byte _chipSelect;  // currently-selected chip (LCD_CS1 or LCD_CS2)
  byte _lcdx;        // current x position (0 - 127)
  byte _lcdy;        // current y position (0 - 63)
  
  byte _port;        // port that the MCP23017 is on (should be 0x20 to 0x27)
  byte _ssPin;       // if non-zero use SPI rather than I2C (and this is the SS pin)

  byte readData ();

#if defined(MCP23x17)
  void expanderWrite (const byte reg, const byte data);
  void startSend ();    // prepare for sending to MCP23017  (eg. set SS low)
  void doSend (const byte what);  // send a byte to the MCP23017
  void endSend ();      // finished sending  (eg. set SS high)
#else
	void do2wireSend (const byte rs, const byte data, const byte enable);		// Send command or data on 2-wire interface
#endif

  boolean _invmode;
  
  const byte * _fMap;		// pointer to current font table
  int _fWidth;		// width of current font
  bool _fSpace;		// should we add a space after each character
  byte _fStart;		// starting character in font
  int _fLength;		// number of chars in current font

  byte _clkPin;		// pin for 2-wire CLK
  byte _dataPin;	// pin for 2-wire DATA
#if defined(__AVR__) && !defined(MCP23x17)
  volatile byte * _clkPort;	// CLK port
  byte _clkMask;	// CLK bitmask
  volatile byte * _dataPort;	// DATA port
  byte _dataMask;	// DATA bitmask
#elif defined(ARDUINO_ARCH_SAMD) && !defined(MCP23x17)
	volatile uint32_t * _clkPort;
	uint32_t _clkMask;
	volatile uint32_t * _dataPort;
	uint32_t _dataMask;
#elif defined(ARDUINO_ARCH_ESP8266)
	uint16_t _clkMask;
	uint16_t _dataMask;
#endif

#ifdef WRITETHROUGH_CACHE
  byte _cache [64 * 128 / 8];
  int  _cacheOffset;
#endif
  
public:
  
  // constructor
#if defined(MCP23x17)
  I2C_graphical_LCD_display () : _port (0x20), _ssPin (10), _invmode(false), _clkPin(0), _dataPin(0) {};
#else
  I2C_graphical_LCD_display (const byte clkPin, const byte dataPin) :
								_port (0x20), _ssPin(0), _invmode(false)
								{_clkPin = clkPin; _dataPin = dataPin;};
#endif

  void begin (const byte port = 0x20, const byte i2cAddress = 0, const byte ssPin = 0);
  void cmd (const byte data);
  void gotoxy (byte x, byte y);
  void writeData (byte data, const boolean inv);
  void writeData (byte data) { writeData(data, _invmode);}
  void letter (byte c, const boolean inv);
  void letter (byte c) {letter(c, _invmode);}
  void string (const char * s, const boolean inv);
  void string (const char * s) {string(s, _invmode);}
  void blit (const byte * pic, const unsigned int size);
  void clear (const byte x1 = 0,    // start pixel
              const byte y1 = 0,     
              const byte x2 = 127,  // end pixel
              const byte y2 = 63,   
              const byte val = 0);   // what to fill with 
  void setPixel (const byte x, const byte y, const byte val = 1);
  void fillRect (const byte x1 = 0,   // start pixel
                const byte y1 = 0,     
                const byte x2 = 127, // end pixel
                const byte y2 = 63,    
                const byte val = 1);  // what to draw (0 = white, 1 = black) 
  void frameRect (const byte x1 = 0,    // start pixel
                 const byte y1 = 0,     
                 const byte x2 = 127, // end pixel
                 const byte y2 = 63,    
                 const byte val = 1,    // what to draw (0 = white, 1 = black) 
                 const byte width = 1);
  void line  (const byte x1 = 0,    // start pixel
              const byte y1 = 0,     
              const byte x2 = 127,  // end pixel
              const byte y2 = 63,   
              const byte val = 1);  // what to draw (0 = white, 1 = black) 
  void scroll (const byte y = 0);   // set scroll position
  void circle (const byte x = 0,		// center point x
			   const byte y = 0,		// center point y
			   const byte r = 1,		// radius
			   const byte val = 1);		// color (0 = white, 1 = black)
  void fillCircle (const byte x = 0,		// center point x
					 const byte y = 0,		// center point y
					 const byte r = 1,		// radius
					 const byte val = 1);	// color (0 = white, 1 = black)

#if defined(ARDUINO) && ARDUINO >= 100
	virtual size_t write(uint8_t c) {letter(c, _invmode); return 1; }
#else
	void write(uint8_t c) { letter(c, _invmode); }
#endif

	void setInv(boolean inv) {_invmode = inv;} // set inverse mode state true == inverse
	void setFont(const void * fontMap = NULL,			// Set font table (assumed in PROGMEM)
				 const int width = 5,			// Width of a character
				 const bool space = true,		// Add space after each character?
				 const byte start = 0x20,		// First character in font
				 const int length = 96);		// Number of characters in font
};

#endif  // I2C_graphical_LCD_display_H


