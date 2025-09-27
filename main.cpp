#include "bno085.h"
#include "loadcell.h"
#include <iostream>

int main() {
    BNO085 imu("/dev/i2c-1", 0x4B);
    if (!imu.initialize()) {
        std::cerr << "IMU init failed\n";
        return 1;
    }
    auto imu_data = imu.read(0x00, 8); // Example: read 8 bytes from reg 0x00
    if (imu_data.empty()) {
        std::cerr << "IMU read failed\n";
        return 1;
    }
    std::cout << "IMU raw data:";
    for (auto b : imu_data) std::cout << " " << std::hex << (int)b;
    std::cout << std::dec << std::endl;

    // Update: Use GPIO pins for LoadCell, not I2C
    int PIN_DT = 5;   // GPIO5
    int PIN_SCK = 6;  // GPIO6
    LoadCell lc(PIN_DT, PIN_SCK);
    if (!lc.initialize()) {
        std::cerr << "LoadCell init failed\n";
        return 1;
    }
    long raw = lc.read_raw();
    std::cout << "LoadCell raw value: " << raw << std::endl;

    return 0;
}