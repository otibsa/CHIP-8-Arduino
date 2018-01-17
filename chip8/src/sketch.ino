#include <Keypad.h>
#include <SPI.h>
#include <SpiRAM.h>

#define SS_PIN A0
#define BUZZER_PIN 10
#define KROWS 4
#define KCOLS 4

char keys[KROWS][KCOLS] {
    {'1', '2', '3', 'C'},
    {'4', '5', '6', 'D'},
    {'7', '8', '9', 'E'},
    {'A', '0', 'B', 'F'}
};
byte rowPins[KROWS] = {5, 4, 3, 2};
byte colPins[KCOLS] = {9, 8, 7, 6};

byte clock = 0;
SpiRAM SpiRam(0, SS_PIN);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KROWS, KCOLS);

void setup() {
    Serial.begin(9600);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(SS_PIN, OUTPUT);  // necessary for analog pin A0
}

void loop() {
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
    SpiRam.write_stream(0, buffer, 16);
    delay(100);

    buffer[0] = 0;
    SpiRam.read_stream(0, buffer, 16);
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

