#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

typedef enum {c_UP, c_RIGHT, c_DOWN, c_LEFT} cursor_move;
typedef struct {
    uint16_t size;
    char name[8];
    uint16_t address;
} CHIP8_rom;

class Menu {
    public:
        Menu(uint8_t cols, uint8_t rows);
        void begin();

    protected:
        virtual void key_5() = 0;
        virtual void key_0() = 0;
        virtual void key_F() = 0;

        uint8_t buffer[256];
        uint8_t cols;
        uint8_t rows;

    private:
        void move_cursor(cursor_move m);
};

class ROM_Menu : public Menu {
    public:
        ROM_Menu(uint8_t cols, uint8_t rows);

    private:
        void key_5();
        void key_0();
        void key_F();

        CHIP8_rom *roms;
};

class Hex_Editor : public Menu {
    public:
        Hex_Editor(uint8_t cols, uint8_t rows);

    protected:
        void key_5();
        void key_0();
        void key_F();
};

#endif
