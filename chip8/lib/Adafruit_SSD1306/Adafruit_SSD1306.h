/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/
#ifndef _Adafruit_SSD1306_H_
#define _Adafruit_SSD1306_H_

#if ARDUINO >= 100
 #include "Arduino.h"
 #define WIRE_WRITE Wire.write
#else
 #include "WProgram.h"
  #define WIRE_WRITE Wire.send
#endif

#if defined(__SAM3X8E__)
 typedef volatile RwReg PortReg;
 typedef uint32_t PortMask;
 #define HAVE_PORTREG
#elif defined(ARDUINO_ARCH_SAMD)
// not supported
#elif defined(ESP8266) || defined(ESP32) || defined(ARDUINO_STM32_FEATHER) || defined(__arc__)
  typedef volatile uint32_t PortReg;
  typedef uint32_t PortMask;
#elif defined(__AVR__)
  typedef volatile uint8_t PortReg;
  typedef uint8_t PortMask;
  #define HAVE_PORTREG
#else
  // chances are its 32 bit so assume that
  typedef volatile uint32_t PortReg;
  typedef uint32_t PortMask;
#endif

#include <SPI.h>
// #include <Adafruit_GFX.h>

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define SSD1306_I2C_ADDRESS   0x3C  // 011110+SA0+RW - 0x3C or 0x3D
// Address for 128x32 is 0x3C
// Address for 128x64 is 0x3D (default) or 0x3C (if SA0 is grounded)

/*=========================================================================
    SSD1306 Displays
    -----------------------------------------------------------------------
    The driver is used in multiple displays (128x64, 128x32, etc.).
    Select the appropriate display below to create an appropriately
    sized framebuffer, etc.

    SSD1306_128_64  128x64 pixel display

    SSD1306_128_32  128x32 pixel display

    SSD1306_96_16

    -----------------------------------------------------------------------*/
#define SSD1306_128_64
//   #define SSD1306_128_32
//   #define SSD1306_96_16

   // Choose whether or not to scale the CHIP8's 64x32 display onto the OLED screen
   // by using 2x2 pixels, 2x1 pixels or no scaling with 1x1 pixels
#define PIXEL_MODE 22
//    #define PIXEL_MODE 21
//    #define PIXEL_MODE 11
/*=========================================================================*/

#if defined SSD1306_128_64 && defined SSD1306_128_32
  #error "Only one SSD1306 display can be specified at once in SSD1306.h"
#endif
#if !defined SSD1306_128_64 && !defined SSD1306_128_32 && !defined SSD1306_96_16
  #error "At least one SSD1306 display must be specified in SSD1306.h"
#endif

#if defined SSD1306_128_64
  #define SSD1306_LCDWIDTH                  128
  #define SSD1306_LCDHEIGHT                 64
#endif
#if defined SSD1306_128_32
  #define SSD1306_LCDWIDTH                  128
  #define SSD1306_LCDHEIGHT                 32
  #if PIXEL_MODE == 22
    #define PIXEL_MODE 21
  #endif
#endif
#if defined SSD1306_96_16
  #define SSD1306_LCDWIDTH                  96
  #define SSD1306_LCDHEIGHT                 16
#endif

#define CHIP8_SCREENWIDTH 64
#define CHIP8_SCREENHEIGHT 32
#define GLYPH_WIDTH 4
#define GLYPH_HEIGHT 6
#define ASCII_WIDTH (SSD1306_LCDWIDTH/(GLYPH_WIDTH+1))
#define ASCII_HEIGHT (SSD1306_LCDHEIGHT/(GLYPH_HEIGHT+1))
#define ASCII_MODE true
#define RAW_MODE false

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

class Adafruit_SSD1306 { // : public Adafruit_GFX {
 public:
  Adafruit_SSD1306(int8_t RST = -1);

  void begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = SSD1306_I2C_ADDRESS, bool reset=true);
  void ssd1306_command(uint8_t c);

  void clear(void);
  void set_mode(bool ascii_mode);
  void rotate180();
  void show();
  void show(int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, bool inverted=false);

  bool getPixel(int16_t x, int16_t y);
  void drawPixel(int16_t x, int16_t y);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  bool drawSprite(uint8_t x, uint8_t y, uint8_t *sprite, uint8_t n, bool _show=false);
  void drawChar(uint8_t x, uint8_t y, char c);
  void drawString(uint8_t x, uint8_t y, char *s, uint8_t len);
  void drawString(uint8_t x, uint8_t y, char *s);
  void drawString(uint8_t x, uint8_t y, String s);
  void drawStringLine(uint8_t y, char *s);

 private:
  int8_t _i2caddr, _vccstate, sid, sclk, dc, rst, cs;
  bool rotated = false;
  bool ascii_mode = false;

  uint8_t calc_segment(uint8_t page, uint8_t col);
  void calc_coordinates(int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, uint8_t *start_page, uint8_t *start_col, uint8_t *end_page, uint8_t *end_col);
  void get_glyph(char c, uint8_t *b0, uint8_t *b1, uint8_t *b2);

};

#endif /* _Adafruit_SSD1306_H_ */
