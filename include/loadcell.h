#pragma once

class LoadCell {
public:
    LoadCell(int pin_dt, int pin_sck);
    ~LoadCell();

    bool initialize();
    long read_raw();
    

private:
    int dt_pin;
    int sck_pin;
};