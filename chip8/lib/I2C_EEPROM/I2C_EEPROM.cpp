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
    _write_address(address);
    Wire.endTransmission();

    Wire.requestFrom(i2c_address, len);
    while (Wire.available() && i<len) {
        dst[i++] = Wire.read();
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
    _write_address(address);
    len = Wire.write(src, len);
    Wire.endTransmission();

    delay(4);
}

uint8_t I2C_EEPROM::set(uint16_t address, uint8_t b, uint8_t len) {
    uint8_t written = 0;
    uint8_t i;
    uint8_t buffer[64];
    memset(buffer, b, 64);

    for (i=0; i<len; i+=64) {
        written += write(address+i, buffer, MIN(len-i, 64));
    }
    return written;
}
