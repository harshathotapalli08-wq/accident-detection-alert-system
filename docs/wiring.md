# Wiring

All grounds (battery, regulator, Arduino, SIM900A, sensors) **must be common**.

## Pin map (Arduino Uno / Nano)

| Module | Module pin | Arduino pin | Bus / notes |
|---|---|---|---|
| ADXL345 | CS | D10 | SPI |
| ADXL345 | SDI / SDA | D11 (MOSI) | SPI |
| ADXL345 | SDO | D12 (MISO) | SPI |
| ADXL345 | SCL / SCLK | D13 (SCK) | SPI |
| NEO-6M | TX | D4 | UART (software serial, 9600) |
| SIM900A | TXD | D7 | UART (software serial, 9600) |
| SIM900A | RXD | D8 | UART, use a voltage divider (see below) |
| Cancel button | one leg | D2 | other leg to GND (internal pull-up) |
| LED | anode (via 220 ohm) | D3 | |
| Buzzer (active) | + | D6 | |

## Power

| Part | Supply | Note |
|---|---|---|
| Arduino | 7-12 V into VIN, or USB | |
| ADXL345 | **3.3 V** | |
| NEO-6M | 3.3 V or 5 V | Needs sky view; first fix can take minutes |
| SIM900A | **Dedicated 5 V, 2 A supply** | Current peaks ~2 A while transmitting; the Arduino 5 V pin cannot supply this |

## Logic levels

- The ADXL345 is a 3.3 V part. Use a breakout with level shifting, or a 3.3 V level shifter on CS, MOSI and SCK.
- Arduino D8 (5 V) to SIM900A RXD: use a divider (for example 1 kohm in series, 2 kohm to GND) to bring it to about 3.3 V.

## Mounting

Fix the ADXL345 **flat with the Z axis pointing up** in the vehicle (or helmet). Rollover detection relies on Z dropping away from +1 g.

## Why only one listener at a time

An Uno has one hardware UART (used for USB). GPS and GSM both use SoftwareSerial, which can listen to only one port at a time. The firmware listens to the GPS normally and switches to the GSM only while sending the alert.
