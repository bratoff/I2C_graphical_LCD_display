I2C_graphical_LCD_display
=========================

Arduino library for KS0108 LCD displays using I2C, SPI or 2-wire 74HC595 protocol.

Forked from https://github.com/nickgammon/I2C_graphical_LCD_display
Enhanced by Bruce Ratoff, KO4XL@yahoo.com, https://github.com/bratoff

For details about the theory, wiring, schematic, etc. see:
http://www.gammon.com.au/forum/?id=10940

LCD display: KS0108
IO expander: MCP23017 (also supported is the SPI version of the chip: MCP23S17)

Library documentation also on the above web page.

The original library has been expanded to include open and filled circles, and
an additional interface has been added using two 74HC595 latched shift registers.

Other enhancements include:
- Significant speed improvement in WRITETHRU_CACHE mode
- Optimization of shape drawing algorithms
- Conditional compilations based on presence of I2C and/or SPI libraries to reduce code size
- Platform-specific optimizations for AVR and some ARM architectures
