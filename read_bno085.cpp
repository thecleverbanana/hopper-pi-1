#include "bno085.h"
#include <iostream>

int main() {
    BNO085 imu("/dev/i2c-1", 0x4B);
    if (!imu.initialize()) {
        std::cerr << "IMU init failed\n";
        return 1;
    }
    // enable linear acceleration with 10ms interval
    if (!imu.enableLinearAcceleration(10000)) {
        std::cerr << "Enable Linear Accel failed\n";
        return 1;
    }

    while (true) {
        LinearAccel la;
        if (imu.readLinearAcceleration(la)) {
            std::cout << "lin acc [m/s^2] "
                      << la.x << ", " << la.y << ", " << la.z << "\n";
        }

        usleep(2000);
    }
}
