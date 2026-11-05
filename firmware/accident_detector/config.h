#pragma once
#include <Arduino.h>
#include "secrets.h"

// =====================================================================
//  Pin map (Arduino Uno / Nano). See docs/wiring.md
// =====================================================================
// ADXL345 accelerometer on hardware SPI: MOSI=11, MISO=12, SCK=13
constexpr uint8_t PIN_ADXL_CS = 10;

// NEO-6M GPS (UART, software serial, receive only)
constexpr uint8_t PIN_GPS_RX = 4;      // Arduino RX <- NEO-6M TX
constexpr uint8_t PIN_GPS_TX = 5;      // unused by the NEO-6M

// SIM900A GSM (UART, software serial)
constexpr uint8_t PIN_GSM_RX = 7;      // Arduino RX <- SIM900A TXD
constexpr uint8_t PIN_GSM_TX = 8;      // Arduino TX -> SIM900A RXD

constexpr uint8_t PIN_CANCEL_BTN = 2;  // push button to GND (internal pull-up)
constexpr uint8_t PIN_LED        = 3;
constexpr uint8_t PIN_BUZZER     = 6;

// =====================================================================
//  Detection tuning. Mount the sensor FLAT with the Z axis pointing UP.
// =====================================================================
constexpr float    IMPACT_G          = 4.0f;   // total acceleration that counts as a crash
constexpr float    ROLLOVER_Z_G      = 0.5f;   // Z below this = vehicle no longer upright
constexpr uint32_t ROLLOVER_HOLD_MS  = 1500;   // how long it must stay tilted
constexpr uint32_t SAMPLE_INTERVAL_MS = 10;    // 100 Hz

// =====================================================================
//  Alert behaviour
// =====================================================================
constexpr uint32_t COUNTDOWN_MS       = 10000; // time to press CANCEL before SMS is sent
constexpr uint32_t CANCEL_COOLDOWN_MS = 5000;  // ignore triggers after a cancel
constexpr uint32_t ALERT_COOLDOWN_MS  = 60000; // ignore triggers after an alert
constexpr uint8_t  SMS_RETRIES        = 3;
constexpr uint32_t GPS_LIVE_MAX_AGE_MS = 10000; // older fix is reported as "last known"

constexpr bool DEBUG_SERIAL = true;            // print readings every 500 ms
