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

    LoadCell lc("/dev/i2c-1", 0x2A); // Replace 0x2A with your load cell's I2C address
    if (!lc.initialize()) {
        std::cerr << "LoadCell init failed\n";
        return 1;
    }
    auto lc_data = lc.read(3); // Example: read 3 bytes (typical for 24-bit ADC)
    if (lc_data.empty()) {
        std::cerr << "LoadCell read failed\n";
        return 1;
    }
    std::cout << "LoadCell raw data:";
    for (auto b : lc_data) std::cout << " " << std::hex << (int)b;
    std::cout << std::dec << std::endl;

    return 0;
}