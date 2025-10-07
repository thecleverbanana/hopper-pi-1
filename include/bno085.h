#pragma once
#include <string>
#include <vector>
#include <cstdint>

class BNO085 {
public:
    BNO085(int i2c_bus = 1, int address = 0x4A);  // default I2C bus 1, address 0x4A
    ~BNO085();

    bool begin();
    std::vector<uint8_t> readData(int length = 32); // read one data packet
    bool configureRotationVector();  // output config ARVR-Stabilized Rotation Vector @50Hz
    bool getRotationVector(float &qi, float &qj, float &qk, float &qr);
    bool configureLinearAcceleration();
    bool getLinearAcceleration(float &ax, float &ay, float &az);
    bool configureAccelerometer();
    bool getAccelerometer(float &ax, float &ay, float &az);

private:
    int i2c_fd;
    int i2c_bus;
    int dev_addr;

    bool writeBytes(const uint8_t *data, int len);
    bool readBytes(uint8_t *buffer, int len);
};