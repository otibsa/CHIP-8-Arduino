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
// void write_stream(uint16_t address, uint8_t *buffer, uint16_t len);
// void write_byte(uint16_t address, uint8_t b);
// void read_stream(uint16_t address, uint8_t *buffer, uint8_t len);
// uint8_t read_byte(uint16_t address);

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

    oled.clear();
    oled.show();
    oled.set_mode(ASCII_MODE);
    oled.drawString(0,0,"The quick brown fox jumps over the lazy dog");
    oled.show();
    oled.show(4,0, 8,0, true);
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

