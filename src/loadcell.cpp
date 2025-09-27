#include "loadcell.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>

LoadCell::LoadCell(const std::string& i2c_device, int address)
    : i2c_fd(-1), i2c_addr(address), device(i2c_device) {}

LoadCell::~LoadCell() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool LoadCell::initialize() {
    i2c_fd = open(device.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "Failed to open I2C device\n";
        return false;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, i2c_addr) < 0) {
        std::cerr << "Failed to set I2C address\n";
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    return true;
}

std::vector<uint8_t> LoadCell::read(size_t length) {
    if (i2c_fd < 0) return {};
    std::vector<uint8_t> buf(length);
    if (::read(i2c_fd, buf.data(), length) != (ssize_t)length) return {};
    return buf;
}