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
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/

#ifdef __AVR__
  #include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
 #include <pgmspace.h>
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

#if !defined(__ARM_ARCH) && !defined(ENERGIA) && !defined(ESP8266) && !defined(ESP32) && !defined(__arc__)
 #include <util/delay.h>
#endif

#include <stdlib.h>

#include <Wire.h>
#include <SPI.h>
//#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

// stretch a 4 bit pattern: 0b1011 -> 0b11001111
#define EXTEND(b) (((b)&0b1000)<<4 | ((b)&0b1000)<<3 | ((b)&0b0100)<<3 | ((b)&0b0100)<<2 |((b)&0b0010)<<2 | ((b)&0b0010)<<1 | ((b)&0b0001)<<1 | ((b)&0b0001)<<0)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RSHIFT(a,b) ((b)>0 ? (a)>>(b) : (a)<<(-(b)))
#define LSHIFT(a,b) ((b)>0 ? (a)<<(b) : (a)>>(-(b)))

// the memory buffer for the LCD
static uint8_t buffer[CHIP8_SCREENHEIGHT * CHIP8_SCREENWIDTH / 8] = {
0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF9,
0x87, 0xC7, 0xF7, 0xFF, 0xFF, 0x1F, 0x1F, 0x3D,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFC, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xC0, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x0F, 0x07, 0x1F, 0x7F, 0xFF, 0xFF, 0xF8, 0xF8,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
0x00, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x0E,
0x06, 0x06, 0x06, 0x0C, 0xFF, 0xFF, 0xFF, 0x00,
0xFE, 0xFC, 0x00, 0x18, 0x3C, 0x7E, 0x66, 0xE6,
0x06, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x06,
0xFC, 0x4E, 0x46, 0x46, 0x46, 0x4E, 0x7C, 0x78,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0F, 0x1F,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00,
0x18, 0x18, 0x0C, 0x06, 0x0F, 0x0F, 0x0F, 0x00,
0x07, 0x01, 0x00, 0x04, 0x0E, 0x0C, 0x18, 0x0C,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00,
0x07, 0x0C, 0x0C, 0x18, 0x1C, 0x0C, 0x06, 0x06,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Custom 4x6 font, without bounding box (should be 5x7)
// Each 3 B group contains a character.
// Each nibble contains the 4 bits of a line of pixels of the character.
// This font was created by taking the 3x5 font from https://robey.lag.net/2010/01/23/tiny-monospace-font.html
// and making it one pixel wider and taller
static const uint8_t font4x6[] = {
    0x69, 0xbd, 0x96,   // 0
    0x4c, 0x44, 0x4e,   // 1
    0x69, 0x24, 0x8f,   // 2
    0xe1, 0x61, 0x1e,   // 3
    0x99, 0xf1, 0x11,   // 4
    0xf8, 0xe1, 0x1e,   // 5
    0x68, 0xe9, 0x96,   // 6
    0xf1, 0x24, 0x44,   // 7
    0x69, 0x69, 0x96,   // 8
    0x69, 0x97, 0x1e,   // 9
    0x69, 0xf9, 0x99,   // A
    0xe9, 0xe9, 0x9e,   // B
    0x78, 0x88, 0x87,   // C
    0xe9, 0x99, 0x9e,   // D
    0xf8, 0xe8, 0x8f,   // E
    0xf8, 0xe8, 0x88,   // F
    0x78, 0xb9, 0x97,   // G
    0x99, 0xf9, 0x99,   // H
    0xe4, 0x44, 0x4e,   // I
    0x11, 0x11, 0x96,   // J
    0x9a, 0xca, 0x99,   // K
    0x88, 0x88, 0x8f,   // L
    0x9f, 0xf9, 0x99,   // M
    0x9d, 0xfb, 0x99,   // N
    0x69, 0x99, 0x96,   // O
    0xe9, 0xe8, 0x88,   // P
    0x69, 0x99, 0xb6,   // Q
    0xe9, 0xea, 0x99,   // R
    0x78, 0x61, 0x1e,   // S
    0xe4, 0x44, 0x44,   // T
    0x99, 0x99, 0x97,   // U
    0x99, 0x99, 0x66,   // V
    0x99, 0x9f, 0xf9,   // W
    0x99, 0x69, 0x99,   // X
    0x99, 0x62, 0x22,   // Y
    0xf1, 0x24, 0x8f,   // Z
};
#define FONT_0 0
#define FONT_A 10

#define ssd1306_swap(a, b) { int16_t t = a; a = b; b = t; }

// initializer for I2C - we only indicate the reset pin!
Adafruit_SSD1306::Adafruit_SSD1306(int8_t reset) { //:Adafruit_GFX(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT) {
  sclk = dc = cs = sid = -1;
  rst = reset;
}

void Adafruit_SSD1306::begin(uint8_t vccstate, uint8_t i2caddr, bool reset) {
  _vccstate = vccstate;
  _i2caddr = i2caddr;

  // I2C Init
  Wire.begin();
  if ((reset) && (rst >= 0)) {
    // Setup reset pin direction (used by both SPI and I2C)
    pinMode(rst, OUTPUT);
    digitalWrite(rst, HIGH);
    // VDD (3.3V) goes high at start, lets just chill for a ms
    delay(1);
    // bring reset low
    digitalWrite(rst, LOW);
    // wait 10ms
    delay(10);
    // bring out of reset
    digitalWrite(rst, HIGH);
    // turn on VCC (9V?)
  }

  // Init sequence
  ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
  ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  ssd1306_command(0x80);                                  // the suggested ratio 0x80

  ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
  ssd1306_command(SSD1306_LCDHEIGHT - 1);

  ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  ssd1306_command(0x0);                                   // no offset
  ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
  ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
  if (vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x10); }
  else
    { ssd1306_command(0x14); }
  ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
  ssd1306_command(0x00);                                  // 0x0 act like ks0108
  ssd1306_command(SSD1306_SEGREMAP | 0x1);
  ssd1306_command(SSD1306_COMSCANDEC);

 #if defined SSD1306_128_32
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x02);
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  ssd1306_command(0x8F);

#elif defined SSD1306_128_64
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x12);
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  if (vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x9F); }
  else
    { ssd1306_command(0xCF); }

#elif defined SSD1306_96_16
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(0x2);   //ada x12
  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  if (vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x10); }
  else
    { ssd1306_command(0xAF); }

#endif

  ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
  if (vccstate == SSD1306_EXTERNALVCC)
    { ssd1306_command(0x22); }
  else
    { ssd1306_command(0xF1); }
  ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
  ssd1306_command(0x40);
  ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6

  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(SSD1306_DISPLAYON);//--turn on oled panel
}

void Adafruit_SSD1306::ssd1306_command(uint8_t c) {
  // I2C
  uint8_t control = 0x00;   // Co = 0, D/C = 0
  Wire.beginTransmission(_i2caddr);
  Wire.write(control);
  Wire.write(c);
  Wire.endTransmission();
}

// clear everything
void Adafruit_SSD1306::clear(void) {
  memset(buffer, 0, (CHIP8_SCREENWIDTH*CHIP8_SCREENHEIGHT/8));
}

void Adafruit_SSD1306::set_mode(bool ascii_mode) {
    this->ascii_mode = ascii_mode;
}

void Adafruit_SSD1306::rotate180() {
    rotated = !rotated;
}

void Adafruit_SSD1306::show(void) {
    show(0, 0, CHIP8_SCREENWIDTH-1, CHIP8_SCREENHEIGHT-1);
}

void Adafruit_SSD1306::show(int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, bool inverted) {
    // push the buffer onto the display, but only the section that contains (x,y)
    uint8_t start_page;
    uint8_t end_page;
    uint8_t start_col;
    uint8_t end_col;
    uint8_t seg;

    calc_coordinates(start_x,    start_y,     end_x,    end_y,
                     &start_col, &start_page, &end_col, &end_page);

    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(start_col);
    ssd1306_command(end_col);

    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(start_page); // Page start address (0 = reset)
    ssd1306_command(end_page); // Page end address

    // Serial.print("start_page: ");
    // Serial.println(start_page);
    // Serial.print("end_page: ");
    // Serial.println(end_page);
    // Serial.print("start_col: ");
    // Serial.println(start_col);
    // Serial.print("end_col: ");
    // Serial.println(end_col);

    for (uint8_t page=start_page; page<=end_page; page++) {
        for (uint8_t col=start_col; col<=end_col; col+=16) {
            Wire.beginTransmission(_i2caddr);
            WIRE_WRITE(0x40);
            for (uint8_t i=0; i<16 && (col+i)<=end_col; i++) {
                // map the page and column to an offset in the buffer
                seg = calc_segment(page, col+i);
                // if (ascii_mode && page == 0 && col+i<=10) {
                //     Serial.print("page: ");
                //     Serial.println(page);
                //     Serial.print("col: ");
                //     Serial.println(col+i);
                //     Serial.print("showing seg: ");
                //     Serial.println(seg);
                // }
                if (inverted) {
                    //Serial.print("page: ");
                    //Serial.println(page);
                    //Serial.print("end_page: ");
                    //Serial.println(end_page);
                    //Serial.print("seg: ");
                    //Serial.println(seg);
                    //Serial.print("col: ");
                    //Serial.println(col+i);
                    //Serial.println();
                    //if (ascii_mode && page == end_page && page < (SSD1306_LCDHEIGHT/8)-1) {
                    //    // only invert the upper character, but not the lower one
                    //    seg = seg ^ (0xFF - ((1<<end_page)-1));
                    if (ascii_mode && page == end_y) {
                        seg ^= 0xFF >> page;
                    } else if (ascii_mode && page == start_y-1) {
                        seg ^= 0xFF << (8-start_y);
                    } else {
                        seg ^= 0xFF;
                    }
                }
                WIRE_WRITE(seg);
            }
            Wire.endTransmission();
        }
    }
}

void Adafruit_SSD1306::calc_coordinates(int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y,
                                        uint8_t *start_col, uint8_t *start_page, uint8_t *end_col, uint8_t *end_page) {
    int16_t limit_x = CHIP8_SCREENWIDTH-1;
    int16_t limit_y = CHIP8_SCREENHEIGHT-1;

    if (ascii_mode) {
        limit_x = ASCII_WIDTH-1;
        limit_y = ASCII_HEIGHT-1;
    }
    start_x = MAX(start_x, 0);
    start_x = MIN(start_x, limit_x);
    end_x = MIN(end_x, limit_x);
    end_x = MAX(start_x, end_x);

    start_y = MAX(start_y, 0);
    start_y = MIN(start_y, limit_y);
    end_y = MIN(end_y, limit_y);
    end_y = MAX(start_y, end_y);

    if (rotated) {
        int16_t tmp = start_x;
        start_x = limit_x - end_x;
        end_x = limit_x - tmp;
        tmp = start_y;
        start_y = limit_y - end_y;
        end_y = limit_y - tmp;
    }

    if (!ascii_mode) {
        *start_page = start_y/4;
        *end_page = end_y/4;
#if PIXEL_MODE == 21 || PIXEL_MODE == 11
        *start_page /= 2;
        *end_page /= 2;
#endif

        *start_col = start_x;
        *end_col = end_x;
#if PIXEL_MODE == 22 || PIXEL_MODE == 21
        // wide pixels
        *start_col = start_x*2;
        *end_col = end_x*2+1;
#endif
    } else {
        *start_page = (start_y*7)/8;
        *end_page = ((end_y+1)*7)/8;

        *start_col = start_x*5;
        *end_col = (end_x+1)*5;
    }
}

uint8_t Adafruit_SSD1306::calc_segment(uint8_t page, uint8_t col) {
    uint8_t offset;
    
    if (!ascii_mode) {
#if PIXEL_MODE == 22
        if (page % 2 == 0){
            offset = page*SSD1306_LCDWIDTH/4+col/2;
            return EXTEND(buffer[offset] & 0xF);
        } else {
            offset = (page-1)*SSD1306_LCDWIDTH/4+col/2;
            return EXTEND(buffer[offset] >> 4);
        }
#elif PIXEL_MODE == 21
        if (page*8 < CHIP8_SCREENHEIGHT) {
            offset = page*CHIP8_SCREENWIDTH+col/2;
            return buffer[offset];
        } else {
            return 0;
        }
#elif PIXEL_MODE == 12
        if ((col-(col%16)) < CHIP8_SCREENWIDTH) {
            if (page%2 == 0) {
                offset = page*SSD1306_LCDWIDTH/4+col;
                return EXTEND(buffer[offset] & 0xF);
            } else {
                offset = (page-1)*SSD1306_LCDWIDTH/4+col;
                return EXTEND(buffer[offset] >> 4);
            }
        } else {
            return 0;
        }
#elif PIXEL_MODE == 11
        if (page*8 < CHIP8_SCREENHEIGHT && (col-(col%16)) < CHIP8_SCREENWIDTH) {
            // we are in the upper left half of the OLED screen
            offset = page*CHIP8_SCREENWIDTH + col;
            return buffer[offset];
        } else {
            return 0;
        }
#endif
    } else {
        // calc buffer offset
        if (col % 5 == 0) {
            return 0;
        }
        // Glyphs are 6 pixels high, with "padding" 7 pixels. A page for the SSD1306 is 8 pixels high.
        // We need to map the page and column to the pixels of two characters in the buffer
        // Saving space in memory (3 B per glyph) and on the screen (4x6 pixels) comes with a tradeoff in complexity when calculating the correct pixels to load
        char upper_char;
        char lower_char;
        uint8_t glyph_buffer[6];
        int8_t col_shift;
        upper_char = buffer[page*ASCII_WIDTH + col/5];
        lower_char = buffer[(page+1)*ASCII_WIDTH + col/5];
        get_glyph(upper_char, &glyph_buffer[0], &glyph_buffer[1], &glyph_buffer[2]);
        get_glyph(lower_char, &glyph_buffer[3], &glyph_buffer[4], &glyph_buffer[5]);

        // gather all twelve pixels from the glyphs of the two characters
        uint16_t bits = 0;
        col_shift = (col%5)-1;
        for (int i=0; i<sizeof(glyph_buffer); i++) {
            // first 6 pixels from the first glyph, then a 0, then 6 pixels from the second glyph
            bits |= LSHIFT(glyph_buffer[i]&(0x80>>col_shift), (i/3)+col_shift-6+2*i);
            bits |= LSHIFT(glyph_buffer[i]&(0x08>>col_shift), (i/3)+col_shift-1+2*i);
        }

        // cut out the relevant pixels
        return (bits >> page) & 0xFF;
    }
    return 0;
}

// read a value from the screen buffer
bool Adafruit_SSD1306::getPixel(int16_t x, int16_t y) {
  if (rotated) {
      x = CHIP8_SCREENWIDTH - x - 1;
      y = CHIP8_SCREENHEIGHT - y - 1;
  }
  if ((x < 0) || (x >= CHIP8_SCREENWIDTH) || (y < 0) || (y >= CHIP8_SCREENHEIGHT))
    return 0xFF;
  return buffer[x +(y/8)*CHIP8_SCREENWIDTH] & (1 << (y&7)) > 0; 
}

// the most basic function, set a single pixel
void Adafruit_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= CHIP8_SCREENWIDTH) || (y < 0) || (y >= CHIP8_SCREENHEIGHT))
        return;
    if (rotated) {
        x = CHIP8_SCREENWIDTH - x - 1;
        y = CHIP8_SCREENHEIGHT - y - 1;
    }

    // x is which column
    switch (color)
    {
      case WHITE:   buffer[x+ (y/8)*CHIP8_SCREENWIDTH] |=  (1 << (y&7)); break;
      case BLACK:   buffer[x+ (y/8)*CHIP8_SCREENWIDTH] &= ~(1 << (y&7)); break;
      case INVERSE: buffer[x+ (y/8)*CHIP8_SCREENWIDTH] ^=  (1 << (y&7)); break;
    }

}

void Adafruit_SSD1306::drawPixel(int16_t x, int16_t y) {
    drawPixel(x,y, WHITE);
}

bool Adafruit_SSD1306::drawSprite(uint8_t start_x, uint8_t start_y, uint8_t *sprite, uint8_t n, bool _show) {
    bool flag = false;
    int16_t _x, _y;
    uint8_t sprite_x = start_x;
    uint8_t sprite_y = start_y;
    uint8_t width = 0;
    uint8_t height = 0;

    for (uint8_t y=0; y<n; y++) {
        for (uint8_t x=0; x<8; x++) {
            if (sprite[y] & (0x80>>x)) {
                _x = (int16_t)((start_x+x) % CHIP8_SCREENWIDTH);
                _y = (int16_t)((start_y+y) % CHIP8_SCREENHEIGHT);
                sprite_x = MIN(sprite_x, (uint8_t)_x);
                sprite_y = MIN(sprite_y, (uint8_t)_y);
                width = MAX(width, x);
                height = MAX(height, y);

                flag |= getPixel(_x, _y);
                drawPixel(_x, _y, INVERSE);
            }
        }
    }
    if (_show) {
        show(sprite_x, sprite_y, sprite_x+width, sprite_y+height);
    }
    return flag;
}

void Adafruit_SSD1306::drawChar(uint8_t x, uint8_t y, char c) {
    x = x % ASCII_WIDTH;
    y = y % ASCII_HEIGHT;

    buffer[y*ASCII_WIDTH + x] = (uint8_t)c;
}

void Adafruit_SSD1306::drawString(uint8_t x, uint8_t y, char *s, uint8_t len) {
    for (uint8_t i=0; i<len; i++) {
        if (x >= ASCII_WIDTH) {
            x=0;
            y++;
        }
        y = y % ASCII_HEIGHT;

        drawChar(x, y, s[i]);
        x++;
    }
}

void Adafruit_SSD1306::drawString(uint8_t x, uint8_t y, char *s) {
    drawString(x, y, s, strlen(s));
}

void Adafruit_SSD1306::drawString(uint8_t x, uint8_t y, String s) {
    drawString(x, y, s.c_str(), s.length());
}

void Adafruit_SSD1306::drawStringLine(uint8_t y, char *s) {
    bool found_end = false;
    for (uint8_t x=0; x<ASCII_WIDTH; x++) {
        if (s[x] == 0) {
            found_end = true;
        }
        if (found_end) {
            drawChar(x, y, ' ');
        } else {
            drawChar(x, y, s[x]);
        }
    }
}

void Adafruit_SSD1306::get_glyph(char c, uint8_t *b0, uint8_t *b1, uint8_t *b2) {
    if ('0' <= c && c <= '9') {
        if (b0) *b0 = font4x6[3*(c-'0')+FONT_0+0];
        if (b1) *b1 = font4x6[3*(c-'0')+FONT_0+1];
        if (b2) *b2 = font4x6[3*(c-'0')+FONT_0+2];
    } else if ('A' <= c && c <= 'Z') {;
        if (b0) *b0 = font4x6[3*(c-'A'+FONT_A)+0];
        if (b1) *b1 = font4x6[3*(c-'A'+FONT_A)+1];
        if (b2) *b2 = font4x6[3*(c-'A'+FONT_A)+2];
    } else if ('a' <= c && c <= 'z') {;
        // replace with uppercase
        if (b0) *b0 = font4x6[3*(c-'a'+FONT_A)+0];
        if (b1) *b1 = font4x6[3*(c-'a'+FONT_A)+1];
        if (b2) *b2 = font4x6[3*(c-'a'+FONT_A)+2];
    } else {
        // non-printable or space
        if (b0) *b0 = 0;
        if (b1) *b1 = 0;
        if (b2) *b2 = 0;
    }
}
