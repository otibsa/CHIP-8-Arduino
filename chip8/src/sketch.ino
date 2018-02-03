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
CPU cpu(&oled, &keypad, &SpiRam, &eeprom, 0/*tick_callback*/, BUZZER_PIN, 300);

bool button_before = false;
bool button_now = false;
uint8_t cursor;

void beep_n(int n);
void show_roms();

void setup() {
    uint8_t rom_count;
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
    Hex_Editor hex_editor(8, 8, &oled, &keypad, &eeprom, &cpu, BUZZER_PIN);
    ROM_Menu rom_menu(2, 8, &oled, &keypad, &eeprom, &cpu, BUZZER_PIN, &hex_editor);

    show_roms();
    while (1) {
        rom_count = eeprom.read(0);
        Serial.print(F("ROM count is "));
        Serial.println(rom_count);
        rom_menu.begin(1, rom_count);
    }
}

void show_roms() {
    uint8_t i;
    uint16_t address = 1;
    uint8_t rom_count = eeprom.read(0);
    uint8_t buffer[64];
    uint16_t size;
    uint16_t j;
    char name[9] = {0};

    for (i=0; i<rom_count; i++) {
        Serial.println();
        Serial.print(F("ROM "));
        Serial.print(i);
        Serial.println(F(":"));
        Serial.print(F("Offset: 0x"));
        Serial.println(address, HEX);

        address += eeprom.read(address, (uint8_t*)&size, 2);
        address += eeprom.read(address, (uint8_t*)name, 8);
        Serial.print(F("Name: "));
        Serial.println(name);
        Serial.print(F("Size: "));
        Serial.println(size);
        // print ROM content?
        address += size;
    }
}

void tick_callback() {
    beep_n(1);
    while (1) {
        button_before = button_now;
        button_now = digitalRead(BUTTON_PIN) == HIGH;
        if (!button_before && button_now) {
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

