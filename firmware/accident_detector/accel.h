#pragma once
#include <SPI.h>
#include <math.h>
#include "config.h"

// Minimal ADXL345 driver over 4-wire SPI (mode 3), +/-16 g full resolution.
namespace accel {

constexpr uint8_t REG_DEVID       = 0x00;
constexpr uint8_t REG_BW_RATE     = 0x2C;
constexpr uint8_t REG_POWER_CTL   = 0x2D;
constexpr uint8_t REG_DATA_FORMAT = 0x31;
constexpr uint8_t REG_DATAX0      = 0x32;
constexpr float   G_PER_LSB       = 0.0039f;   // full-resolution scale

struct Sample { float x, y, z; };

inline void writeReg(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(PIN_ADXL_CS, LOW);
  SPI.transfer(reg);
  SPI.transfer(value);
  digitalWrite(PIN_ADXL_CS, HIGH);
  SPI.endTransaction();
}

inline uint8_t readReg(uint8_t reg) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(PIN_ADXL_CS, LOW);
  SPI.transfer(0x80 | reg);               // read bit
  uint8_t v = SPI.transfer(0x00);
  digitalWrite(PIN_ADXL_CS, HIGH);
  SPI.endTransaction();
  return v;
}

// Returns false if the sensor does not answer with its device ID (0xE5).
inline bool begin() {
  pinMode(PIN_ADXL_CS, OUTPUT);
  digitalWrite(PIN_ADXL_CS, HIGH);
  SPI.begin();
  delay(10);
  if (readReg(REG_DEVID) != 0xE5) return false;
  writeReg(REG_BW_RATE, 0x0A);            // 100 Hz output rate
  writeReg(REG_DATA_FORMAT, 0x0B);        // full resolution, +/-16 g
  writeReg(REG_POWER_CTL, 0x08);          // measurement mode
  return true;
}

inline Sample read() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(PIN_ADXL_CS, LOW);
  SPI.transfer(0xC0 | REG_DATAX0);        // read + multi-byte
  uint8_t b[6];
  for (uint8_t i = 0; i < 6; i++) b[i] = SPI.transfer(0x00);
  digitalWrite(PIN_ADXL_CS, HIGH);
  SPI.endTransaction();

  int16_t x = (int16_t)((b[1] << 8) | b[0]);
  int16_t y = (int16_t)((b[3] << 8) | b[2]);
  int16_t z = (int16_t)((b[5] << 8) | b[4]);
  return { x * G_PER_LSB, y * G_PER_LSB, z * G_PER_LSB };
}

inline float magnitude(const Sample& s) {
  return sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
}

}  // namespace accel
