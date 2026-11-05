#pragma once
#include <string.h>
#include "config.h"

// SIM900A text-mode SMS over UART (AT commands). Uses flash strings to save RAM.
namespace gsm {

static bool ready = false;

// Finds a token in a character stream without buffering it.
struct Matcher {
  const char* tok; uint8_t len; uint8_t pos;
  explicit Matcher(const char* t) : tok(t), len(strlen(t)), pos(0) {}
  bool push(char c) {
    if (c == tok[pos]) pos++;
    else pos = (c == tok[0]) ? 1 : 0;
    if (pos == len) { pos = 0; return true; }
    return false;
  }
};

inline void flush(Stream& s) { while (s.available()) s.read(); }

// true when `token` arrives; false on "ERROR" or timeout.
inline bool waitFor(Stream& s, const char* token, uint32_t timeoutMs) {
  Matcher ok(token), err("ERROR");
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (s.available()) {
      char c = s.read();
      if (ok.push(c)) return true;
      if (err.push(c)) return false;
    }
  }
  return false;
}

inline bool command(Stream& s, const __FlashStringHelper* cmd, uint32_t timeoutMs = 2000) {
  flush(s);
  s.println(cmd);
  return waitFor(s, "OK", timeoutMs);
}

inline bool registered(Stream& s) {
  flush(s);
  s.println(F("AT+CREG?"));
  char buf[40]; uint8_t n = 0;
  uint32_t start = millis();
  while (millis() - start < 1500 && n < sizeof(buf) - 1) {
    if (s.available()) buf[n++] = s.read();
  }
  buf[n] = 0;
  return strstr(buf, ",1") != nullptr || strstr(buf, ",5") != nullptr;  // home / roaming
}

// Starts the module and waits up to 30 s for the network. Returns true if the module answers.
inline bool begin(Stream& s) {
  delay(500);
  ready = false;
  for (uint8_t i = 0; i < 5 && !ready; i++) ready = command(s, F("AT"), 1000);
  if (!ready) return false;

  command(s, F("ATE0"));                 // echo off
  command(s, F("AT+CMGF=1"));            // text mode
  command(s, F("AT+CSCS=\"GSM\""));

  uint32_t start = millis();
  while (millis() - start < 30000) {
    if (registered(s)) break;
    delay(1000);
  }
  return true;
}

inline bool sendSms(Stream& s, const char* number, const char* text) {
  if (!ready) return false;
  flush(s);
  s.print(F("AT+CMGS=\""));
  s.print(number);
  s.println('"');
  if (!waitFor(s, ">", 5000)) return false;
  s.print(text);
  s.write((uint8_t)26);                  // Ctrl+Z ends the message
  return waitFor(s, "+CMGS:", 30000);
}

}  // namespace gsm
