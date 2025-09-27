#include "loadcell.h"
#include <wiringPi.h>
#include <iostream>

LoadCell::LoadCell(int pin_dt, int pin_sck)
    : dt_pin(pin_dt), sck_pin(pin_sck) {}

LoadCell::~LoadCell() {}

bool LoadCell::initialize() {
    if (wiringPiSetupGpio() == -1) { // Use BCM GPIO numbering
        std::cerr << "Failed to initialize wiringPi\n";
        return false;
    }
    pinMode(dt_pin, INPUT);
    pinMode(sck_pin, OUTPUT);
    digitalWrite(sck_pin, LOW);
    return true;
}

long LoadCell::read_raw() {
    // Wait for HX711 to become ready (DT pin goes LOW)
    while (digitalRead(dt_pin) == HIGH) {
        delayMicroseconds(10);
    }

    long value = 0;
    for (int i = 0; i < 24; ++i) {
        digitalWrite(sck_pin, HIGH);
        delayMicroseconds(1);
        value = (value << 1) | digitalRead(dt_pin);
        digitalWrite(sck_pin, LOW);
        delayMicroseconds(1);
    }

    // Set gain (1 more clock pulse)
    digitalWrite(sck_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(sck_pin, LOW);
    delayMicroseconds(1);

    // Convert to signed 24-bit value
    if (value & 0x800000) {
        value |= ~0xFFFFFF;
    }

    return value;
}