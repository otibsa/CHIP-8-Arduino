#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <I2C_EEPROM.h>
#include <CHIP8.h>

#define MOD(a,b) ((a)>=0 ? (a)%(b) : (((a)%(b)) + (b)) % (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

typedef enum {c_UP, c_RIGHT, c_DOWN, c_LEFT} cursor_move;
typedef struct {
    uint16_t size;
    uint16_t address;
} CHIP8_rom;

class Menu {
    public:
        Menu(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin);
        void begin(uint16_t eeprom_offset, uint16_t item_count);

    protected:
        virtual void on_key(char key) = 0;
        virtual void on_cursor_move() = 0;
        virtual void show_page() = 0;
        virtual void show_info() = 0;
        virtual void empty_menu() = 0;
        void init_page();
        void beep(uint8_t n);
        void move_cursor(cursor_move m);
        void update_cursor();

        Adafruit_SSD1306 *oled;
        Keypad *keypad;
        I2C_EEPROM *eeprom;
        CPU *cpu;
        uint8_t buzzer_pin;

        uint8_t buffer[64]; // enough for 8x8 Bytes or 16xCHIP8_rom (4 B each)
        uint16_t item_count;
        uint8_t x_step;
        uint8_t cols;
        uint8_t rows;
        uint16_t page;
        uint8_t cursor;
        uint16_t old_page;
        uint8_t old_cursor;
        uint16_t last_page;
        uint8_t last_cursor;
        uint16_t eeprom_offset;
        bool modified_page;
        bool stop;
};

class Hex_Editor : public Menu {
    public:
        Hex_Editor(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin);
        void set_offset(uint16_t offset);

    protected:
        void on_key(char key);
        void on_cursor_move();
        void show_page();
        void show_info();
        void empty_menu();

    private:
        uint16_t hex_editor_offset;
};

class ROM_Menu : public Menu {
    public:
        ROM_Menu(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin, Hex_Editor *hex_editor);

    private:
        void on_key(char key);
        void on_cursor_move();
        void show_page();
        void show_info();
        void empty_menu();

        void new_rom();

        CHIP8_rom *roms;
        Hex_Editor *hex_editor;
};

#endif
