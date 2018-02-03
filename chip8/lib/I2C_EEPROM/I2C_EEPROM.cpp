#include <I2C_EEPROM.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

I2C_EEPROM::I2C_EEPROM(uint8_t i2c_address) : i2c_address(i2c_address) {
}

void I2C_EEPROM::begin()
{
    Wire.begin();
}

void I2C_EEPROM::_write_address(uint16_t address) {
    Wire.beginTransmission(i2c_address);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
}

uint8_t I2C_EEPROM::read(uint16_t address) {
    _write_address(address);
    Wire.endTransmission();

    Wire.requestFrom(i2c_address, 1);
    return Wire.read();
}

uint8_t I2C_EEPROM::read(uint16_t address, uint8_t *dst, uint8_t len) {
    uint8_t i=0;
    while (i<len) {
        _write_address(address+i);
        Wire.endTransmission();

        Wire.requestFrom(i2c_address, len-i);
        while (Wire.available() && i<len) {
            dst[i++] = Wire.read();
        }
    }
    return i;
}

uint8_t I2C_EEPROM::write(uint16_t address, uint8_t b) {
    uint8_t len;
    _write_address(address);
    len = Wire.write(b);
    Wire.endTransmission();

    delay(4);
    return len;
}

uint8_t I2C_EEPROM::write(uint16_t address, uint8_t *src, uint8_t len) {
    uint8_t written = 0;
    uint8_t i;
    uint8_t test_buffer[16] = {0};
    uint8_t j;
    uint8_t tries=0;
    while (written < len) {
        _write_address(address+written);
        if (tries < 3) {
            i = Wire.write(src+written, MIN(len-written, 16));
        } else {
            i = Wire.write(*(src+written));
        }
        Wire.endTransmission();

        delay(4);
        read(address+written, test_buffer, i);
        if (memcmp(test_buffer, (uint8_t*)(src+written), i)) {
            tries++;
            Serial.println(F("BAD! Read different bytes than just written"));
            Serial.print(F("At EEPROM address "));
            Serial.println(address+written);
            Serial.println(F("Wrote:"));
            for(j=0;j<i; j++) {
                if (src[written+j] < 0xF) {
                    Serial.print("0");
                }
                Serial.print(src[written+j], HEX);
                Serial.print(" ");
            }
            Serial.println(F("\r\nRead:"));
            for(j=0;j<i; j++) {
                if (test_buffer[j] < 0xF) {
                    Serial.print("0");
                }
                Serial.print(test_buffer[j], HEX);
                Serial.print(" ");
            }
            Serial.println();
            Serial.println(F("Writing again"));
        } else {
            written += i;
            tries=0;
        }
        if (i==0 || tries >= 5) {
            break;
        }
    }
    return written;
}

uint8_t I2C_EEPROM::set(uint16_t address, uint8_t b, uint8_t len) {
    uint8_t written = 0;
    uint8_t i;
    uint8_t buffer[30];
    memset(buffer, b, 30);

    while (written < len) {
        _write_address(address+written);
        written += Wire.write(buffer, MIN(len-written, 30));
        Wire.endTransmission();

        delay(4);
        if (i==0) {
            break;
        }
    }
    return written;
}
