/*
 * Accident Detection & Alert System
 * Arduino Uno/Nano | ADXL345 (SPI) | NEO-6M GPS (UART) | SIM900A GSM (UART)
 *
 * A crash (high-g impact) or rollover (vehicle no longer upright) starts a
 * countdown with buzzer + LED. If the rider does not press CANCEL in time, an
 * SMS with a Google Maps link to the last GPS position is sent.
 *
 * Only one SoftwareSerial port can listen at a time, so the GPS is listened to
 * during normal operation and the GSM port is switched on only to send the alert.
 */
#include <SoftwareSerial.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "accel.h"
#include "gps.h"
#include "gsm.h"

static SoftwareSerial gpsPort(PIN_GPS_RX, PIN_GPS_TX);
static SoftwareSerial gsmPort(PIN_GSM_RX, PIN_GSM_TX);

static const char* const ALERT_NUMBERS[] = {
  ALERT_NUMBER_1,
#ifdef ALERT_NUMBER_2
  ALERT_NUMBER_2,
#endif
};
static const uint8_t NUM_ALERT_NUMBERS = sizeof(ALERT_NUMBERS) / sizeof(ALERT_NUMBERS[0]);

enum class State : uint8_t { IDLE, COUNTDOWN, SENDING, COOLDOWN };
enum class Cause : uint8_t { IMPACT, ROLLOVER };

static State    state = State::IDLE;
static Cause    cause = Cause::IMPACT;
static uint32_t stateSince = 0;
static uint32_t cooldownMs = 0;
static uint32_t lastSampleMs = 0, lastDebugMs = 0, tiltSince = 0;
static bool     tilting = false;
static float    peakG = 0;

static void go(State s) { state = s; stateSince = millis(); }

static void alarm(bool on) {
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

static void beep(uint8_t times, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < times; i++) { alarm(true); delay(onMs); alarm(false); delay(offMs); }
}

static bool cancelPressed() {
  if (digitalRead(PIN_CANCEL_BTN) != LOW) return false;
  delay(30);                                   // debounce
  return digitalRead(PIN_CANCEL_BTN) == LOW;
}

static void trigger(Cause c, float g) {
  cause = c;
  peakG = g;
  tilting = false;
  Serial.println(c == Cause::IMPACT ? F("Impact detected, countdown started")
                                    : F("Rollover detected, countdown started"));
  go(State::COUNTDOWN);
}

static void buildMessage(char* msg, size_t size) {
  char gbuf[8], lat[16], lng[16];
  int n;
  if (cause == Cause::IMPACT) {
    dtostrf(peakG, 1, 1, gbuf);
    n = snprintf(msg, size, "ACCIDENT ALERT! Impact detected (%sg).", gbuf);
  } else {
    n = snprintf(msg, size, "ACCIDENT ALERT! Vehicle rollover detected.");
  }
  if (gps::hasFix()) {
    dtostrf(gps::lat(), 1, 6, lat);
    dtostrf(gps::lng(), 1, 6, lng);
    snprintf(msg + n, size - n, " %s https://maps.google.com/?q=%s,%s",
             gps::isLive() ? "Location:" : "Last known location:", lat, lng);
  } else {
    snprintf(msg + n, size - n, " Location unavailable (no GPS fix).");
  }
}

static bool sendAlert() {
  char msg[160];
  buildMessage(msg, sizeof(msg));
  Serial.println(msg);

  gsmPort.listen();                            // switch listener GPS -> GSM
  if (!gsm::ready) gsm::begin(gsmPort);

  bool any = false;
  for (uint8_t i = 0; i < NUM_ALERT_NUMBERS; i++) {
    for (uint8_t attempt = 0; attempt < SMS_RETRIES; attempt++) {
      if (gsm::sendSms(gsmPort, ALERT_NUMBERS[i], msg)) { any = true; break; }
      delay(2000);
    }
  }
  gpsPort.listen();                            // back to GPS
  return any;
}

// ---------------------------------------------------------------- states
static void idle(uint32_t now) {
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  accel::Sample s = accel::read();
  float g = accel::magnitude(s);

  if (DEBUG_SERIAL && now - lastDebugMs >= 500) {
    lastDebugMs = now;
    Serial.print(F("g=")); Serial.print(g, 2);
    Serial.print(F(" z=")); Serial.print(s.z, 2);
    Serial.print(F(" gpsFix=")); Serial.println(gps::hasFix());
  }

  if (g >= IMPACT_G) { trigger(Cause::IMPACT, g); return; }

  if (s.z < ROLLOVER_Z_G) {
    if (!tilting) { tilting = true; tiltSince = now; }
    else if (now - tiltSince >= ROLLOVER_HOLD_MS) trigger(Cause::ROLLOVER, g);
  } else {
    tilting = false;
  }
}

static void countdown(uint32_t now) {
  uint32_t t = now - stateSince;
  alarm((t / 250) % 2 == 0);                   // 2 Hz beep + blink

  if (cancelPressed()) {
    alarm(false);
    Serial.println(F("Cancelled by user"));
    cooldownMs = CANCEL_COOLDOWN_MS;
    go(State::COOLDOWN);
  } else if (t >= COUNTDOWN_MS) {
    go(State::SENDING);
  }
}

static void sending() {
  alarm(true);
  bool ok = sendAlert();
  alarm(false);
  Serial.println(ok ? F("Alert sent") : F("Alert FAILED"));
  if (ok) beep(3, 100, 100); else beep(1, 800, 0);
  cooldownMs = ALERT_COOLDOWN_MS;
  go(State::COOLDOWN);
}

static void cooldown(uint32_t now) {
  if (now - stateSince >= cooldownMs) { tilting = false; go(State::IDLE); }
}

// ---------------------------------------------------------------- Arduino
void setup() {
  Serial.begin(115200);
  pinMode(PIN_CANCEL_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  alarm(false);

  if (!accel::begin()) {
    Serial.println(F("ADXL345 not found. Check SPI wiring."));
    while (true) { beep(1, 100, 100); }        // fast blink = hardware fault
  }

  gsmPort.begin(9600);
  Serial.println(gsm::begin(gsmPort) ? F("GSM module OK") : F("GSM module NOT responding"));

  gpsPort.begin(9600);
  gpsPort.listen();

  beep(1, 150, 0);                             // boot beep
  Serial.println(F("Accident detector ready"));
}

void loop() {
  gps::feed(gpsPort);
  uint32_t now = millis();

  switch (state) {
    case State::IDLE:      idle(now);      break;
    case State::COUNTDOWN: countdown(now); break;
    case State::SENDING:   sending();      break;
    case State::COOLDOWN:  cooldown(now);  break;
  }
}
