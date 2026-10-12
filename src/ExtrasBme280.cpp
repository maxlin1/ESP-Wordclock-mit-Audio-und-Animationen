#include "Extras/Bme280.h"

#if EXTRAS_ENABLED

#include <Wire.h>

namespace {
constexpr uint8_t REG_CALIB_T_P = 0x88; // 0x88 ... 0x9F
constexpr uint8_t REG_CALIB_H1 = 0xA1;
constexpr uint8_t REG_CHIP_ID = 0xD0;
constexpr uint8_t REG_CALIB_H2 = 0xE1; // 0xE1 ... 0xE7
constexpr uint8_t REG_CTRL_HUM = 0xF2;
constexpr uint8_t REG_CTRL_MEAS = 0xF4;
constexpr uint8_t REG_CONFIG = 0xF5;
constexpr uint8_t REG_DATA = 0xF7; // 0xF7 ... 0xFE

uint16_t u16le(const uint8_t *p) { return p[0] | (p[1] << 8); }
int16_t s16le(const uint8_t *p) { return static_cast<int16_t>(u16le(p)); }
} // namespace

//------------------------------------------------------------------------------

bool Bme280::readRegisters(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(address, len) != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

//------------------------------------------------------------------------------

bool Bme280::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

//------------------------------------------------------------------------------

const char *Bme280::chipName() const {
    if (!present) {
        return "-";
    }
    return chipId == CHIP_ID_BME280 ? "BME280" : "BMP280";
}

//------------------------------------------------------------------------------

bool Bme280::readCalibration() {
    uint8_t c[24];
    if (!readRegisters(REG_CALIB_T_P, c, sizeof(c))) {
        return false;
    }
    digT1 = u16le(&c[0]);
    digT2 = s16le(&c[2]);
    digT3 = s16le(&c[4]);
    digP1 = u16le(&c[6]);
    digP2 = s16le(&c[8]);
    digP3 = s16le(&c[10]);
    digP4 = s16le(&c[12]);
    digP5 = s16le(&c[14]);
    digP6 = s16le(&c[16]);
    digP7 = s16le(&c[18]);
    digP8 = s16le(&c[20]);
    digP9 = s16le(&c[22]);

    if (chipId == CHIP_ID_BME280) {
        uint8_t h[7];
        if (!readRegisters(REG_CALIB_H1, &digH1, 1) ||
            !readRegisters(REG_CALIB_H2, h, sizeof(h))) {
            return false;
        }
        digH2 = s16le(&h[0]);
        digH3 = h[2];
        digH4 = static_cast<int16_t>(static_cast<int8_t>(h[3]) * 16 |
                                     (h[4] & 0x0F));
        digH5 =
            static_cast<int16_t>(static_cast<int8_t>(h[5]) * 16 | (h[4] >> 4));
        digH6 = static_cast<int8_t>(h[6]);
    }
    return true;
}

//------------------------------------------------------------------------------

bool Bme280::begin() {
    present = false;
    const uint8_t addresses[] = {0x76, 0x77};
    for (uint8_t candidate : addresses) {
        address = candidate;
        uint8_t id = 0;
        if (!readRegisters(REG_CHIP_ID, &id, 1)) {
            continue;
        }
        if (id != CHIP_ID_BME280 && id != CHIP_ID_BMP280) {
            continue;
        }
        chipId = id;
        if (!readCalibration()) {
            continue;
        }
        if (chipId == CHIP_ID_BME280) {
            // humidity oversampling x1 (must be written before ctrl_meas)
            writeRegister(REG_CTRL_HUM, 0x01);
        }
        // standby 1000 ms, filter off
        writeRegister(REG_CONFIG, 0xA0);
        // temperature x1, pressure x1, normal mode
        writeRegister(REG_CTRL_MEAS, 0x27);
        present = true;
        Serial.printf("%s found at 0x%02X\n", chipName(), address);
        delay(50);
        read();
        return true;
    }
    Serial.println("No BME280/BMP280 found");
    return false;
}

//------------------------------------------------------------------------------

bool Bme280::read() {
    if (!present) {
        return false;
    }
    uint8_t d[8];
    const uint8_t len = (chipId == CHIP_ID_BME280) ? 8 : 6;
    if (!readRegisters(REG_DATA, d, len)) {
        return false;
    }

    const int32_t adcP = (static_cast<int32_t>(d[0]) << 12) |
                         (static_cast<int32_t>(d[1]) << 4) | (d[2] >> 4);
    const int32_t adcT = (static_cast<int32_t>(d[3]) << 12) |
                         (static_cast<int32_t>(d[4]) << 4) | (d[5] >> 4);

    if (adcT == 0x80000) {
        return false; // no measurement yet
    }

    // Temperature (data sheet, 32 bit integer formula)
    int32_t var1 = ((((adcT >> 3) - (static_cast<int32_t>(digT1) << 1))) *
                    static_cast<int32_t>(digT2)) >>
                   11;
    int32_t var2 = (((((adcT >> 4) - static_cast<int32_t>(digT1)) *
                      ((adcT >> 4) - static_cast<int32_t>(digT1))) >>
                     12) *
                    static_cast<int32_t>(digT3)) >>
                   14;
    const int32_t tFine = var1 + var2;
    temperature = ((tFine * 5 + 128) >> 8) / 100.0f;

    // Pressure (64 bit integer formula)
    if (adcP != 0x80000) {
        int64_t p1 = static_cast<int64_t>(tFine) - 128000;
        int64_t p2 = p1 * p1 * static_cast<int64_t>(digP6);
        p2 = p2 + ((p1 * static_cast<int64_t>(digP5)) << 17);
        p2 = p2 + (static_cast<int64_t>(digP4) << 35);
        p1 = ((p1 * p1 * static_cast<int64_t>(digP3)) >> 8) +
             ((p1 * static_cast<int64_t>(digP2)) << 12);
        p1 = ((static_cast<int64_t>(1) << 47) + p1) *
                 static_cast<int64_t>(digP1) >>
             33;
        if (p1 != 0) {
            int64_t p = 1048576 - adcP;
            p = (((p << 31) - p2) * 3125) / p1;
            p1 = (static_cast<int64_t>(digP9) * (p >> 13) * (p >> 13)) >> 25;
            p2 = (static_cast<int64_t>(digP8) * p) >> 19;
            p = ((p + p1 + p2) >> 8) + (static_cast<int64_t>(digP7) << 4);
            pressure = (p / 256.0f) / 100.0f; // Pa -> hPa
        }
    }

    // Humidity (32 bit integer formula)
    if (chipId == CHIP_ID_BME280) {
        const int32_t adcH = (static_cast<int32_t>(d[6]) << 8) | d[7];
        if (adcH != 0x8000) {
            int32_t v = tFine - 76800;
            v = (((((adcH << 14) - (static_cast<int32_t>(digH4) << 20) -
                    (static_cast<int32_t>(digH5) * v)) +
                   16384) >>
                  15) *
                 (((((((v * static_cast<int32_t>(digH6)) >> 10) *
                      (((v * static_cast<int32_t>(digH3)) >> 11) + 32768)) >>
                     10) +
                    2097152) *
                       static_cast<int32_t>(digH2) +
                   8192) >>
                  14));
            v = v - (((((v >> 15) * (v >> 15)) >> 7) *
                      static_cast<int32_t>(digH1)) >>
                     4);
            v = v < 0 ? 0 : v;
            v = v > 419430400 ? 419430400 : v;
            humidity = (v >> 12) / 1024.0f;
        }
    }
    return true;
}

#endif
