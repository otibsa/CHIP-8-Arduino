#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H

#include <Arduino.h>
#include <Wire.h>

class I2C_EEPROM {
    public:
        I2C_EEPROM(uint8_t i2c_address);

        void begin();
        uint8_t read(uint16_t address);
        uint8_t read(uint16_t address, uint8_t *dst, uint8_t len);
        uint8_t write(uint16_t address, uint8_t b);
        uint8_t write(uint16_t address, uint8_t *src, uint8_t len);
        uint8_t set(uint16_t address, uint8_t b, uint8_t len);

    private:
        uint8_t i2c_address;

        void _write_address(uint16_t address);
};

#endif
