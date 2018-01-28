#include <Keypad.h>
#include <SPI.h>
#include <SpiRAM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <I2C_EEPROM.h>
#include <Menu.h>

#include <CHIP8.h>

#define SS_PIN_RAM A0
#define BUZZER_PIN 10
#define KROWS 4
#define KCOLS 4
#define OLED_RESET A2   // not connected
#define OLED_ADDRESS 0x3C
#define EEPROM_ADDRESS 0x50  // external EEPROM 24AA256
#define BUTTON_PIN A3

#define MOD(a,b) ((a)>=0 ? (a)%(b) : (((a)%(b)) + (b)) % (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define swap_u16(w) ( ((w&0xFF00) >> 8) | ((w&0xFF) << 8) )

char keys[KROWS][KCOLS] {
    {'1', '2', '3', 'C'},
    {'4', '5', '6', 'D'},
    {'7', '8', '9', 'E'},
    {'A', '0', 'B', 'F'}
};
byte rowPins[KROWS] = {5, 4, 3, 2};
byte colPins[KCOLS] = {9, 8, 7, 6};

byte clock = 0;
SpiRAM SpiRam(0, SS_PIN_RAM);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KROWS, KCOLS);
Adafruit_SSD1306 oled(OLED_RESET);
I2C_EEPROM eeprom(EEPROM_ADDRESS);

void tick_callback();
CPU cpu(&oled, &keypad, &SpiRam, &eeprom, 0/*tick_callback*/, BUZZER_PIN, 1200);

// typedef struct {
//     uint16_t size;
//     char name[8];
//     uint16_t address;
// } CHIP8_rom;

bool button_before = false;
bool button_now = false;
uint8_t cursor;

void beep_n(int n);
uint16_t input_program();
void hex_editor(uint16_t address, uint16_t len, uint16_t address_offset);
void show_cursor(uint8_t cursor, bool highlight, uint8_t x_step, uint8_t x_count);
void show_memory_page(uint16_t address, uint8_t *buffer, uint8_t len, uint8_t x_step, uint8_t x_count);
void show_address(uint16_t address);
void rom_menu();
void show_rom_names(uint16_t address, CHIP8_rom *roms, uint8_t len, uint8_t x_step, uint8_t x_count);

void setup() {
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400000);
    pinMode(BUZZER_PIN, OUTPUT);
    analogWrite(BUZZER_PIN, 0);
    pinMode(SS_PIN_RAM, OUTPUT);  // necessary for analog pin A0
    pinMode(BUTTON_PIN, INPUT);
    randomSeed(analogRead(OLED_RESET)); // not connected
    oled.begin();
    //oled.rotate180();

    Serial.println("restarted");
    ROM_Menu rM(2,8);
    rM.begin();
    return;

    oled.clear();
    oled.show();
    oled.set_mode(ASCII_MODE);
    //oled.drawChar(10,0, 'A');
    //oled.drawChar(10,1, 'B');
    //oled.show(10,1, 10,1, false);
    //delay(500);
    //oled.show(10,0, 10,0, true);
    //oled.show(10,0, 10,1, true);
    while (1) {
        //hex_editor(0, 1);
        rom_menu();
        oled.clear();
        oled.drawString(0,0,String("RESTART"));
        oled.show();
    }
}

void tick_callback() {
    beep_n(1);
    while (1) {
        button_before = button_now;
        button_now = digitalRead(BUTTON_PIN) == HIGH;
        if (button_before && button_now) {
            // continue CPU on rising edge
            break;
        }
        delay(50);
    }
}

void loop() {
}

void beep_n(int n) {
    for (; n>0; n--) {
        analogWrite(BUZZER_PIN, 127);
        delay(20);
        analogWrite(BUZZER_PIN, 0);
        delay(20);
    }
}

// hex character to uint8
uint8_t hc2u8(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0xFF;
}

void rom_menu() {
    // parse EEPROM for CHIP8 roms
    CHIP8_rom roms[16] = {0};
    uint8_t rom_count;
    uint16_t rom_address;
    uint8_t cursor = 0;
    uint8_t page = 0;
    uint8_t old_cursor = 1;
    uint8_t old_page = 1;
    uint8_t last_page;
    uint8_t last_cursor;
    uint8_t menu_rows = 8;
    uint8_t menu_cols = 2;
    uint8_t menu_sep = 12;
    char c;

    rom_count = eeprom.read(0);
    Serial.print("There are ");
    Serial.print(rom_count);
    Serial.println(" CHIP8 roms installed.");
    last_page = (rom_count-1)/16;
    last_cursor = (rom_count-1)%16;
    rom_address = 1;  // start searching after rom_count
    oled.clear();
    oled.show();
    oled.set_mode(ASCII_MODE);
    while (1) {
        Serial.print("page: ");
        Serial.println(page);
        Serial.print("cursor: ");
        Serial.println(cursor);
        if (page != old_page) {
            // show names of roms and wait for user input
            show_rom_names(rom_address, roms, page==last_page ? last_cursor+1 : menu_rows*menu_cols , menu_sep, menu_cols);
        }
        if (cursor != old_cursor) {
            show_cursor(old_cursor, false, menu_sep, menu_cols);
            show_cursor(cursor, true, menu_sep, menu_cols);
        }
        c = keypad.waitForKey();
        old_page = page;
        old_cursor = cursor;
        switch (c) {
        case '0':
            // edit
            oled.clear();
            oled.show();
            beep_n(2);
            Serial.print("editing ROM with size ");
            Serial.println(roms[cursor].size);
            hex_editor(roms[cursor].address, roms[cursor].size, 0x200);
            break;
        case 'F':
            // exit rom_menu
            return;
        case '5':
            // start rom
            oled.set_mode(RAW_MODE);
            oled.clear();
            oled.show();
            cpu.begin();
            cpu.load(roms[cursor].address, roms[cursor].size);
            beep_n(3);
            cpu.run();
            oled.clear();
            oled.set_mode(ASCII_MODE);
            break;
        case '2':
            // up
            if (cursor < menu_cols) {
                page = MOD(page-1, last_page+1);
                cursor += menu_cols*(menu_rows-1);
            } else {
                cursor -= menu_cols;
            }
            break;
        case '4':
            // left
            if (cursor == 0) {
                page = MOD(page-1, last_page+1);
                cursor = menu_cols*menu_rows - 1;
            } else {
                cursor--;
            }
            break;
        case '6':
            // right
            if (cursor == menu_cols*menu_rows -1 || (page==last_page && cursor == last_cursor)) {
                page = MOD(page+1, last_page+1);
                cursor = 0;
            } else {
                cursor++;
            }
            break;
        case '8':
            // down
            if (cursor/menu_cols == menu_rows-1 || (page==last_page && cursor == last_cursor)) {
                page = MOD(page+1, last_page+1);
                cursor = cursor % menu_cols;
            } else {
                if (page == last_page && cursor+menu_cols > last_cursor) {
                    cursor = cursor % menu_cols;
                } else {
                    cursor += menu_cols;
                }
            }
            break;
        }
        if (page == last_page) {
            cursor = MIN(cursor, last_cursor);
        }
    }

    // 5 -> start rom
    // 0 -> edit rom, start hex_editor at relevant address and len
    // F -> restart arduino?
}

void show_rom_names(uint16_t address, CHIP8_rom *roms, uint8_t len, uint8_t x_step, uint8_t x_count) {
    uint8_t cursor;
    char name[8];
    for (cursor=0; cursor<len; cursor++) {
        address += eeprom.read(address, (uint8_t*)&(roms[cursor].size), 2);
        address += eeprom.read(address, (uint8_t*)name, 8);
        roms[cursor].address = address;
        address += roms[cursor].size;
        Serial.print("Found ROM \"");
        Serial.print(String(name));
        Serial.print("\" (");
        Serial.print(roms[cursor].size);
        Serial.print(" B), begins at 0x");
        print_hex(roms[cursor].address);
        Serial.println();

        oled.drawString(x_step*(cursor%x_count), cursor/x_count, String(name));
    }
    oled.show();
}

void hex_editor(uint16_t address, uint16_t len, uint16_t address_offset) {
    // show and modify contents of the external EEPROM
    if (address + len > 0x7FFF) {
        return;
    }
    uint16_t start_address = address;
    uint8_t buffer[64];
    uint8_t cursor = 0;
    uint16_t page = 0;
    uint8_t old_cursor = 1;
    uint16_t old_page = 1;
    uint16_t last_page = (len-1)/64;
    uint8_t last_cursor = (len-1)%64;
    char c;
    bool modified_page = false;
    uint8_t new_value;
    uint8_t menu_rows = 8;
    uint8_t menu_cols = 8;
    uint8_t menu_sep = 3;

    oled.clear();
    oled.show();
    oled.set_mode(ASCII_MODE);

    while (1) {
        if (page != old_page) {
            if (modified_page) {
                eeprom.write(address+old_page*64, buffer, old_page==last_page ? last_cursor+1 : sizeof(buffer));
            }
            modified_page = false;
            show_memory_page(address+page*64, buffer, page==last_page ? last_cursor+1 : sizeof(buffer), menu_sep, menu_cols);
        }
        if (cursor != old_cursor) {
            // unhighlight current cursor
            show_cursor(old_cursor, false, menu_sep, menu_cols);
            show_address(address_offset-start_address+address+page*64+cursor);
            // highlight cursor
            show_cursor(cursor, true, menu_sep, menu_cols);
        }

        c = keypad.waitForKey();
        old_cursor = cursor;
        old_page = page;
        // move cursor
        switch (c) {
        case '0':
            // save
            eeprom.write(address+page*64, buffer, page==last_page ? last_cursor+1 : sizeof(buffer));
            beep_n(3);
            modified_page = false;
            break;
        case 'F':
            // exit hex_editor without saving
            return;
        case '2':
            // up
            if (cursor < menu_cols) {
                page = MOD(page-1, last_page+1);
                cursor += menu_cols*(menu_rows-1);
            } else {
                cursor -= menu_cols;
            }
            break;
        case '4':
            // left
            if (cursor == 0) {
                page = MOD(page-1, last_page+1);
                cursor = menu_cols*menu_rows - 1;
            } else {
                cursor--;
            }
            break;
        case '5':
            // edit
            beep_n(1);
            new_value = from_hex(keypad.waitForKey());
            oled.drawChar(menu_sep*(cursor%menu_cols), cursor/menu_cols, to_hex(new_value&0xF));
            oled.show(menu_sep*(cursor%menu_cols), cursor/menu_cols, menu_sep*(cursor%menu_cols), cursor/menu_cols);
            new_value <<= 4;
            new_value |= from_hex(keypad.waitForKey());
            oled.drawChar(1+menu_sep*(cursor%menu_cols), cursor/menu_cols, to_hex(new_value&0xF));
            oled.show(1+menu_sep*(cursor%menu_cols), cursor/menu_cols, 1+menu_sep*(cursor%menu_cols), cursor/menu_cols);
            buffer[cursor] = new_value;
            modified_page = true;
            show_cursor(cursor, true, menu_sep, menu_cols);
            beep_n(1);
            break;
        case '6':
            // right
            if (cursor == menu_cols*menu_rows -1 || (page==last_page && cursor == last_cursor)) {
                page = MOD(page+1, last_page+1);
                cursor = 0;
            } else {
                cursor++;
            }
            break;
        case '8':
            // down
            if (cursor/menu_cols == menu_rows-1 || (page==last_page && cursor == last_cursor)) {
                page = MOD(page+1, last_page+1);
                cursor = cursor % menu_cols;
            } else {
                if (page == last_page && cursor+menu_cols > last_cursor) {
                    cursor = cursor % menu_cols;
                } else {
                    cursor += menu_cols;
                }
            }
            break;
        }

        if (page == last_page) {
            cursor = MIN(cursor, last_cursor);
        }
    }
}

void show_cursor(uint8_t cursor, bool highlight, uint8_t x_step, uint8_t x_count) {
    oled.show(x_step*(cursor%x_count), cursor/x_count, (x_step-1-1)+x_step*(cursor%x_count), cursor/x_count, highlight);
}

void show_address(uint16_t address) {
    //oled.drawString(20,8, "    ");
    char s[4];
    hex_string(address, s);
    oled.drawString(21,8, s, 4);
    oled.show(21,8, 24,8, false);
}

void show_memory_page(uint16_t address, uint8_t *buffer, uint8_t len, uint8_t x_step, uint8_t x_count) {
    uint8_t cursor;
    // show current page
    oled.clear();
    // address = address/64;
    // address = address % (0x8000-64);
    memset(buffer, 0, len);
    eeprom.read(address, buffer, len);
    for (cursor=0; cursor<len; cursor++) {
        oled.drawChar(x_step*(cursor%x_count), cursor/x_count, to_hex(buffer[cursor]>>4));
        oled.drawChar(1+x_step*(cursor%x_count), cursor/x_count, to_hex(buffer[cursor]&0xF));

        oled.drawChar(2+x_step*(cursor%x_count), cursor/x_count, ' ');
    }
    oled.show();
}
