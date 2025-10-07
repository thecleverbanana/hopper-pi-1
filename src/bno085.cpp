#include "bno085.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>


BNO085::BNO085(int i2c_bus, int address) : i2c_bus(i2c_bus), dev_addr(address), i2c_fd(-1) {}

BNO085::~BNO085() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool BNO085::begin() {
    std::string device = "/dev/i2c-" + std::to_string(i2c_bus);
    i2c_fd = open(device.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "Failed to open I2C device " << device << "\n";
        return false;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, dev_addr) < 0) {
        std::cerr << "Failed to set I2C address\n";
        return false;
    }

    // Send calibration command
    uint8_t cmd[4] = { 0x07, 0x00, 0x05, 0x00 }; // SH2_CMD_ME_CALIBRATION
    writeBytes(cmd, 4);
    usleep(10000); // 10 ms delay

    // Optionally save calibration to flash
    uint8_t saveCmd[4] = { 0x07, 0x00, 0x06, 0x00 }; // SH2_CMD_ME_SAVE_DCD
    writeBytes(saveCmd, 4);

    std::cout << "BNO085 calibration initialized.\n";
    return true;
}

bool BNO085::writeBytes(const uint8_t *data, int len) {
    if (write(i2c_fd, data, len) != len) {
        std::cerr << "I2C write error\n";
        return false;
    }
    return true;
}

bool BNO085::readBytes(uint8_t *buffer, int len) {
    if (read(i2c_fd, buffer, len) != len) {
        std::cerr << "I2C read error\n";
        return false;
    }
    return true;
}


std::vector<uint8_t> BNO085::readData(int length) {
    std::vector<uint8_t> buffer(length, 0);
    if (!readBytes(buffer.data(), length)) {
        buffer.clear();
    }
    return buffer;
}

bool BNO085::configureAccelerometer() {
    static uint8_t cargo_no = 1;
    uint8_t cmd[21] = {
        0x15, 0x00, 0x02, cargo_no++, 0xFD, 0x04, // Sensor ID = 0x04 (Accel)
        // 0x00,0x00,0x00, 0x10,0x27, // 100 Hz = 10 ms = 0x2710
        0x00,0x00,0x00, 0xC4,0x09, // 400 Hz = 2.5 ms = 0x09C4
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    return writeBytes(cmd, 21);
}

bool BNO085::getAccelerometer(float &ax, float &ay, float &az) {
    auto data = readData(32);
    if (data.size() < 10) return false;

    for (size_t i = 0; i < data.size() - 10; i++) {
        if (data[i] == 0x04) { // Accelerometer report ID
            int16_t raw_x = (int16_t)(data[i+4] | (data[i+5] << 8));
            int16_t raw_y = (int16_t)(data[i+6] | (data[i+7] << 8));
            int16_t raw_z = (int16_t)(data[i+8] | (data[i+9] << 8));

            // Convert from Q8 to m/s²
            ax = raw_x / 256.0f;
            ay = raw_y / 256.0f;
            az = raw_z / 256.0f;

            return true;
        }
    }
    return false;
}

bool BNO085::configureRotationVector() {
    static uint8_t cargo_no = 1;
    uint8_t cmd[21] = {
        0x15, 0x00, 0x02, cargo_no++, 0xFD, 0x28, // header + ARVR RotVec
        0x00,0x00,0x00, 0x10,0x27,  // 100Hz = 20us = 0x2710
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    return writeBytes(cmd, 21);
}

bool BNO085::getRotationVector(float &qi, float &qj, float &qk, float &qr) {
    auto data = readData(32);
    if (data.size() < 14) return false;

    for (size_t i = 0; i < data.size() - 14; i++) {
        if (data[i] == 0x28) { // Rotation Vector report ID
            int16_t raw_i = (int16_t)(data[i+4] | (data[i+5] << 8));
            int16_t raw_j = (int16_t)(data[i+6] | (data[i+7] << 8));
            int16_t raw_k = (int16_t)(data[i+8] | (data[i+9] << 8));
            int16_t raw_r = (int16_t)(data[i+10] | (data[i+11] << 8));

            // Q14 format (1 unit = 1/16384)
            qi = raw_i / 16384.0f;
            qj = raw_j / 16384.0f;
            qk = raw_k / 16384.0f;
            qr = raw_r / 16384.0f;

            return true;
        }
    }
    return false;
}