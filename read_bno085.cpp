#include "bno085.h"
#include "util.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdint>

const float RAD2DEG = 180.0f / M_PI;

int main() {
    BNO085 bno(1, 0x4B); // I2C bus 1, address 0x4B
    if (!bno.begin()) {
        std::cerr << "BNO085 init failed\n";
        return 1;
    }

    if (!bno.configureAccelerometer()) { 
        std::cerr << "Failed to configure Accelerometer\n";
        return 1;
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // bno.calibrateAccelerometerOffset(100, 10);

    while (true) {
    float ax, ay, az;
        if (bno.getAccelerometer(ax, ay, az)) {
            bno.applyAccelerometerOffset(ax, ay, az);
            std::cout << "Accel (m/s²): "
                    << ax << ", " << ay << ", " << az << "\n";
        }

        else {
            // std::cerr << "Failed to read accelerometer\n";
        }

        std::this_thread::sleep_for(std::chrono::microseconds(2500)); // 400 Hz
    }

    // while (true) {
    //     float ax, ay, az;
    //     if (bno.getAccelerometer(ax, ay, az)) {
    //         std::cout << "Accel (m/s²): " 
    //                   << ax << ", " << ay << ", " << az << "\n";
    //     } else {
    //         // std::cerr << "Failed to read accelerometer\n";
    //     }

    //     // std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 Hz print
    //     std::this_thread::sleep_for(std::chrono::microseconds(2500)); // 400 Hz print
    // }

    // if (!bno.configureRotationVector()) { // 50 Hz
    //     std::cerr << "Failed to configure Rotation Vector\n";
    //     return 1;
    // }

    // std::cout << "Configured accelerometer and rotation vector, reading data...\n";

    // while (true) {
    //     float qi, qj, qk, qr;

    //     if (bno.getRotationVector(qi, qj, qk, qr)) {
    //         // std::cout << "Rotation Vector (qi, qj, qk, qr): "<< qi << ", " << qj << ", " << qk << ", " << qr << "\n";
            
    //         float roll, pitch, yaw;
    //         quat_to_euler(qi, qj, qk, qr, roll, pitch, yaw);
    //         // std::cout << "Euler (rpy): " << roll << ", " << pitch << ", " << yaw << "\n";
    //         std::cout << "Euler (deg): "<< roll * RAD2DEG << ", " << pitch * RAD2DEG << ", " << yaw * RAD2DEG << "\n";

    //     } else {
    //         // std::cerr << "Failed to read Rotation Vector\n";
    //     }

    //     std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50Hz
    // }

    return 0;
}