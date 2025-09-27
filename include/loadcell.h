#pragma once
#include <string>
#include <vector>

class LoadCell {
public:
    LoadCell(const std::string& i2c_device, int address);
    ~LoadCell();

    bool initialize();
    std::vector<uint8_t> read(size_t length);

private:
    int i2c_fd;
    int i2c_addr;
    std::string device;
};