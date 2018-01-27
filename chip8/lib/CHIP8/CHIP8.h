#ifndef CHIP8
#define CHIP8

#include <Keypad.h>
#include <SPI.h>
#include <SpiRAM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <I2C_EEPROM.h>

#define swap_u16(w) ( ((w&0xFF00) >> 8) | ((w&0xFF) << 8) )


const uint8_t hexsprites[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
    0x20, 0x60, 0x20, 0x20, 0x70,   // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
    0xF0, 0x80, 0xF0, 0x80, 0x80,   // F
};

class CPU {
    public:
        CPU(Adafruit_SSD1306 *display, Keypad *keypad, SpiRAM *memory, I2C_EEPROM *eeprom, void (*tick_callback)(), uint8_t buzzer_pin, uint16_t clock_speed = 600);
        void begin();
        void load(uint16_t program_offset, uint16_t len);
        void run();
        bool keep_going;  // for debugging

    private:
        void execute(uint16_t opcode);
        void clock_tick();

        // peripherals
        Adafruit_SSD1306 *display;
        Keypad *keypad;
        SpiRAM *memory;
        I2C_EEPROM *eeprom;
        void (*tick_callback)();
        uint8_t buzzer_pin;

        // CHIP8 registers
        uint16_t pc;
        uint8_t V[16];
        uint16_t I;
        uint16_t stack[16];
        uint8_t sp;
        uint8_t delay_ctr;
        uint8_t sound_ctr;

        uint16_t clock_speed;       // speed of the CPU in Hz (must be multiple of 60)
        uint32_t ticks;             // number of cycles the CPU went through
        uint32_t next_tick;         // timestamp of the next CPU cycle measured in us
        uint16_t memory_offset;     // offset in the external RAM (used for resets without having to erase the RAM)
};

static uint8_t from_hex(char c) {
    if ('0'<=c && c<='9') return c-'0';
    if ('A'<=c && c<='F') return c-'A'+10;
    if ('a'<=c && c<='f') return c-'a'+10;
    return 0xFF;
}

static char to_hex(uint8_t x) {
    if (0<=x && x<=9) return '0'+x;
    if (0xA<=x && x<=0xF) return 'A'+x-10;
    return 'X';
}

static void hex_string(uint8_t x, char* dst) {
    dst[0] = to_hex(x>>4);
    dst[1] = to_hex(x&0xF);
}

static void hex_string(uint16_t x, char* dst) {
    dst[0] = to_hex(x>>12);
    dst[1] = to_hex((x>>8) & 0xF);
    dst[2] = to_hex((x>>4) & 0xF);
    dst[3] = to_hex(x & 0xF);
}

static void print_hex(uint8_t x) {
    if (x<=0xF) {
        Serial.print("0");
    }
    Serial.print(x, HEX);
}

static void print_hex(uint16_t x) {
    if (x<=0xFFF) {
        Serial.print("0");
    }
    if (x<=0xFF) {
        Serial.print("0");
    }
    if (x<=0xF) {
        Serial.print("0");
    }
    Serial.print(x, HEX);
}

#endif
