#pragma once

#include "../ExtrasConfig.h"
#include <Arduino.h>

#if EXTRAS_ENABLED

/*
 * Small driver for the Bosch BME280 (temperature, humidity, pressure) and
 * BMP280 (temperature, pressure) on the I2C bus that the clock already uses.
 */
class Bme280 {
public:
    bool begin(); // probes 0x76 and 0x77
    bool read();  // refreshes the values below

    bool isPresent() const { return present; }
    bool hasHumidity() const { return chipId == CHIP_ID_BME280; }
    const char *chipName() const;

    float temperature = NAN; // °C
    float humidity = NAN;    // %RH (BME280 only)
    float pressure = NAN;    // hPa, absolute (station pressure)

private:
    static constexpr uint8_t CHIP_ID_BMP280 = 0x58;
    static constexpr uint8_t CHIP_ID_BME280 = 0x60;

    bool readRegisters(uint8_t reg, uint8_t *buf, uint8_t len);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readCalibration();

    bool present = false;
    uint8_t address = 0;
    uint8_t chipId = 0;

    uint16_t digT1 = 0;
    int16_t digT2 = 0, digT3 = 0;
    uint16_t digP1 = 0;
    int16_t digP2 = 0, digP3 = 0, digP4 = 0, digP5 = 0, digP6 = 0, digP7 = 0,
            digP8 = 0, digP9 = 0;
    uint8_t digH1 = 0, digH3 = 0;
    int16_t digH2 = 0, digH4 = 0, digH5 = 0;
    int8_t digH6 = 0;
};

#endif
