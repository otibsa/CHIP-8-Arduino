#include <Keypad.h>
#include <SPI.h>
#include <SpiRAM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <I2C_EEPROM.h>

#include <CHIP8.h>

#define SS_PIN_RAM A0
#define BUZZER_PIN 10
#define KROWS 4
#define KCOLS 4
#define OLED_RESET A2   // not connected
#define OLED_ADDRESS 0x3C
#define EEPROM_ADDRESS 0x50  // external EEPROM 24AA256
#define BUTTON_PIN A3

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

bool button_before = false;
bool button_now = false;
uint8_t cursor;

void beep_n(int n);
uint8_t hc2u8(char c);
uint16_t input_program();
void print_byte(uint8_t b);
void hex_editor(uint16_t address);
void show_cursor(uint8_t cursor, bool highlight);
void show_memory_page(uint16_t address, uint8_t *buffer, uint8_t len);
void show_address(uint16_t address);

void setup() {
    uint8_t b;
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

    //oled.clear();
    //oled.show();
    //oled.set_mode(ASCII_MODE);
    //oled.drawChar(10,0, 'A');
    //oled.drawChar(10,1, 'B');
    //oled.show(10,1, 10,1, false);
    //delay(500);
    //oled.show(10,0, 10,0, true);
    //oled.show(10,0, 10,1, true);
    hex_editor(0);
    return;

    button_now = digitalRead(BUTTON_PIN) == HIGH;
    // if button is pressed on startup, read a program into the eeprom
    if (button_now) {
        beep_n(3);
        oled.clear();

        Serial.println("First 256 bytes of EEPROM:");
        for (uint16_t i=0; i<256; i++) {
            b = eeprom.read(i);
            print_byte(b);
            print_hex(b);
            Serial.print(" ");
        }
        oled.show();
        Serial.println();
        uint16_t len = input_program();
        Serial.println("Written into EEPROM:");
        for (uint16_t i=0; i<len; i++) {
            b = eeprom.read(i);
            print_hex(b);
            Serial.print(" ");
        }
        Serial.println("\n");
        beep_n(3);
    } else {
        // load MAZE
        cpu.begin();
        cpu.load(0, 34);
        beep_n(2);
        cpu.run();
    }
    //oled.show();
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

void print_byte(uint8_t b) {
    for (uint8_t i=0; i<8; i++) {
        oled.drawPixel((i+8*cursor)%64, cursor/8, (b&(0x80>>i)) ? WHITE : BLACK);
    }
    cursor++;
}

uint16_t input_program() {
    uint8_t b;
    uint16_t i = 0;
    uint8_t buffer[8];
    bool save = false;
    cursor = 0;

    Serial.println("waiting for user input");
    while (1) {
        // read from keypad
        while (1) {
            button_before = button_now;
            button_now = digitalRead(BUTTON_PIN) == HIGH;
            b = (uint8_t)keypad.getKey();
            if (b) {
                b = hc2u8(b);
                break;
            }
            if (!button_before && button_now) {
                // rising edge
                save = true;
                break;
            }
            delay(10);
        }
        if (save) {
            break;
        }
        beep_n(1);
        Serial.print(b, HEX);
        b <<= 4;
        b |= hc2u8(keypad.waitForKey());
        Serial.print(b&0xF, HEX);
        Serial.print(" ");
        beep_n(1);

        buffer[i%sizeof(buffer)] = b;
        print_byte(b);
        oled.show();
        i++;

        if (i%sizeof(buffer) == 0) {
            Serial.println("\n[Saving]");
            eeprom.write(i-sizeof(buffer), buffer, sizeof(buffer));
        }
    }
    eeprom.write(i-(i%sizeof(buffer)), buffer, i%sizeof(buffer));
    Serial.print("Saved ");
    Serial.print(i);
    Serial.println(" Bytes to ROM");
    return i;
}

void loop() {

    return;

    int i;
    char c;
    uint8_t buffer[16];
    uint8_t x;
    analogWrite(BUZZER_PIN, 127);
    delay(1000);
    analogWrite(BUZZER_PIN, 0);

    for (i=0; i<16; i++) {
        analogWrite(BUZZER_PIN, 127);
        delay(20);
        analogWrite(BUZZER_PIN, 0);
        c = keypad.waitForKey();
        x = hc2u8(c);
        if (x == 0xFF) {
            x = 0;
        }
        buffer[i] = x<<4;

        c = keypad.waitForKey();
        x = hc2u8(c);
        if (x == 0xFF) {
            x = 0;
        }
        buffer[i] |= x;
        Serial.print(i);
        Serial.print(": ");
        Serial.print(buffer[i], HEX);
        Serial.println();
    }

    Serial.print("buffer: ");
    for (i=0; i<16; i++) {
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
    SpiRam.write_stream(0, (char*)buffer, 16);
    delay(100);

    buffer[0] = 0;
    SpiRam.read_stream(0, (char*)buffer, 16);
    x = 0;
    for (i=0; i<16; i++) {
        x ^= buffer[i];
    }
    Serial.print("Hash: ");
    Serial.println(x);

    beep_n(x);

    delay(5000);
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


void hex_editor(uint16_t address) {
    // show and modify contents of the external EEPROM
    uint8_t buffer[64];
    uint8_t cursor = 0;
    uint16_t page = 0;
    uint8_t old_cursor = 1;
    uint16_t old_page = 1;
    bool modified_page = false;
    char c;
    uint8_t new_value;

    oled.clear();
    oled.show();
    oled.set_mode(ASCII_MODE);

    while (1) {
        if (page != old_page) {
            if (modified_page) {
                eeprom.write(address+old_page*64, buffer, sizeof(buffer));
            }
            modified_page = false;
            show_memory_page(address+page*64, buffer, sizeof(buffer));
        }
        if (cursor != old_cursor) {
            // unhighlight current cursor
            show_cursor(old_cursor, false);
            show_address(address+page*64+cursor);
            // highlight cursor
            show_cursor(cursor, true);
        }

        c = keypad.waitForKey();
        old_cursor = cursor;
        old_page = page;
        // move cursor
        switch (c) {
        case '5':
            beep_n(1);
            new_value = from_hex(keypad.waitForKey());
            oled.drawChar(3*(cursor%8), cursor/8, to_hex(new_value&0xF));
            oled.show(3*(cursor%8), cursor/8, 3*(cursor%8), cursor/8);
            new_value <<= 4;
            new_value |= from_hex(keypad.waitForKey());
            oled.drawChar(1+3*(cursor%8), cursor/8, to_hex(new_value&0xF));
            oled.show(1+3*(cursor%8), cursor/8, 1+3*(cursor%8), cursor/8);
            buffer[cursor] = new_value;
            modified_page = true;
            show_cursor(cursor, true);
            beep_n(1);
            break;
        case '2':
            // up
            if (cursor < 8) {
                page--;
                cursor += 56;
            } else {
                cursor -= 8;
            }
            break;
        case '4':
            // left
            if (cursor == 0) {
                page--;
                cursor = 63;
            } else {
                cursor--;
            }
            break;
        case '6':
            // right
            if (cursor == 63) {
                page++;
                cursor = 0;
            } else {
                cursor++;
            }
            break;
        case '8':
            // down
            if (cursor/8 == 7) {
                page++;
                cursor = cursor % 8;
            } else {
                cursor += 8;
            }
            break;
        }
    }
}

void show_cursor(uint8_t cursor, bool highlight) {
    oled.show(3*(cursor%8), cursor/8, 1+3*(cursor%8), cursor/8, highlight);
}

void show_address(uint16_t address) {
    //oled.drawString(20,8, "    ");
    char s[4];
    hex_string(address, s);
    oled.drawString(21,8, s, 4);
    oled.show(21,8, 24,8, false);
}

void show_memory_page(uint16_t address, uint8_t *buffer, uint8_t len) {
    uint8_t i;
    uint8_t s;
    // show current page
    oled.clear();
    // address = address/64;
    // address = address % (0x8000-64);
    memset(buffer, 0, len);
    eeprom.read(address, buffer, len);
    for (i=0; i<len; i++) {
        oled.drawChar(3*(i%8), i/8, to_hex(buffer[i]>>4));
        oled.drawChar(1+3*(i%8), i/8, to_hex(buffer[i]&0xF));

        oled.drawChar(2+3*(i%8), i/8, ' ');
    }
    oled.show();
}
