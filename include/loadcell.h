#pragma once
#include <gpiod.h>

class LoadCell {
public:
    LoadCell(const char* chipname, int pin_dt, int pin_sck);
    ~LoadCell();

    bool initialize();
    long read_raw();

private:
    const char* chipname;
    int dt_pin;
    int sck_pin;

    gpiod_chip* chip;
    gpiod_line* dt_line;
    gpiod_line* sck_line;
};
