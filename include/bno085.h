#pragma once
#include <string>
#include <vector>

struct LinearAccel {
    float x, y, z; // m/s^2
};

class BNO085 {
public:
    BNO085(const std::string& i2c_device, int address);
    ~BNO085();

    bool initialize();
    std::vector<uint8_t> read(uint8_t reg, size_t length);
    bool enableLinearAcceleration(uint32_t period_us);
    bool readLinearAcceleration(LinearAccel& out);

private:
    int i2c_fd;
    int i2c_addr;
    std::string device;

    // SHTP protocol state helpers
    uint8_t seq_control = 0;
    bool shtpWrite(uint8_t channel, const uint8_t* payload, size_t len);
    bool shtpRead(uint8_t& out_channel, std::vector<uint8_t>& out_payload);
    static void le16(uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
    static void le32(uint8_t* p, uint32_t v) {
        p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
    }
};