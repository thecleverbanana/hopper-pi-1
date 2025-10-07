#include "loadcell.h"
#include <pigpio.h>
#include <iostream>

LoadCell::LoadCell(int pin_dt, int pin_sck)
    : dt_pin(pin_dt), sck_pin(pin_sck) {}

LoadCell::~LoadCell() {
    // Optionally stop pigpio here if you started it in initialize()
}

bool LoadCell::initialize() {
    if (gpioInitialise() < 0) {
        std::cerr << "Failed to initialize pigpio\n";
        return false;
    }
    gpioSetMode(dt_pin, PI_INPUT);
    gpioSetMode(sck_pin, PI_OUTPUT);
    gpioWrite(sck_pin, PI_LOW);
    return true;
}

long LoadCell::read_raw() {
    // Wait for HX711 to become ready (DT pin goes LOW)
    while (gpioRead(dt_pin) == 1) {
        gpioDelay(10); // microseconds
    }

    long value = 0;
    for (int i = 0; i < 24; ++i) {
        gpioWrite(sck_pin, PI_HIGH);
        gpioDelay(1);
        value = (value << 1) | gpioRead(dt_pin);
        gpioWrite(sck_pin, PI_LOW);
        gpioDelay(1);
    }

    // Set gain (1 more clock pulse)
    gpioWrite(sck_pin, PI_HIGH);
    gpioDelay(1);
    gpioWrite(sck_pin, PI_LOW);
    gpioDelay(1);

    // Convert to signed 24-bit value
    if (value & 0x800000) {
        value |= ~0xFFFFFF;
    }

    return value;
}