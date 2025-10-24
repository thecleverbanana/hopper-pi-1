#include "loadcell.h"
#include <iostream>
#include <unistd.h>

LoadCell::LoadCell(const char* chipname, int pin_dt, int pin_sck)
    : chipname(chipname), dt_pin(pin_dt), sck_pin(pin_sck),
      chip(nullptr), dt_line(nullptr), sck_line(nullptr) {}

LoadCell::~LoadCell() {
    if (chip) gpiod_chip_close(chip);
}

bool LoadCell::initialize() {
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip\n";
        return false;
    }

    dt_line = gpiod_chip_get_line(chip, dt_pin);
    sck_line = gpiod_chip_get_line(chip, sck_pin);
    if (!dt_line || !sck_line) {
        std::cerr << "Failed to get GPIO lines\n";
        return false;
    }

    if (gpiod_line_request_input(dt_line, "loadcell") < 0 ||
        gpiod_line_request_output(sck_line, "loadcell", 0) < 0) {
        std::cerr << "Failed to request GPIO lines\n";
        return false;
    }

    return true;
}

long LoadCell::read_raw() {
    while (gpiod_line_get_value(dt_line) == 1) {
        usleep(10); // microseconds
    }

    long value = 0;
    for (int i = 0; i < 24; ++i) {
        gpiod_line_set_value(sck_line, 1);
        usleep(1);
        value = (value << 1) | gpiod_line_get_value(dt_line);
        gpiod_line_set_value(sck_line, 0);
        usleep(1);
    }

    gpiod_line_set_value(sck_line, 1);
    usleep(1);
    gpiod_line_set_value(sck_line, 0);
    usleep(1);

    if (value & 0x800000) value |= ~0xFFFFFF;
    return value;
}
