#pragma once
#include <string>
#include <vector>

class BNO085 {
public:
    BNO085(const std::string& i2c_device, int address);
    ~BNO085();

    bool initialize();
    std::vector<uint8_t> read(uint8_t reg, size_t length);

private:
    int i2c_fd;
    int i2c_addr;
    std::string device;
};