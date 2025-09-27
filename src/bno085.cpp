#include "bno085.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>

// SHTP channels (Sensor Hub Transport Protocol) 
// https://docs.sparkfun.com/SparkFun_VR_IMU_Breakout_BNO086_QWIIC/assets/component_documentation/Sensor-Hub-Transport-Protocol.pdf
static constexpr uint8_t CHANNEL_COMMAND  = 0; // seldom used by host
static constexpr uint8_t CHANNEL_CONTROL  = 2; // configuration commands
static constexpr uint8_t CHANNEL_INPUT    = 3; // normal sensor data

// SH-2 Report IDs
static constexpr uint8_t SH2_SET_FEATURE  = 0xFD;
static constexpr uint8_t SH2_LINEAR_ACCEL = 0x04; // Linear Acceleration，m/s^2，Q=8


BNO085::BNO085(const std::string& i2c_device, int address)
    : i2c_fd(-1), i2c_addr(address), device(i2c_device) {}

BNO085::~BNO085() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool BNO085::initialize() {
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

std::vector<uint8_t> BNO085::read(uint8_t reg, size_t length) {
    if (i2c_fd < 0) return {};
    if (write(i2c_fd, &reg, 1) != 1) return {};
    std::vector<uint8_t> buf(length);
    if (::read(i2c_fd, buf.data(), length) != (ssize_t)length) return {};
    return buf;
}

// send a SHTP packet (blocking)
bool BNO085::shtpWrite(uint8_t channel, const uint8_t* payload, size_t len) {
    if (i2c_fd < 0) return false;
    // SHTP Header: 4 bytes, e.g. [lenL, lenH, channel, seq]
    uint16_t total_len = static_cast<uint16_t>(len + 4);
    std::vector<uint8_t> frame(total_len);
    le16(&frame[0], total_len);
    frame[2] = channel;
    // Sequence number for control channel
    if (channel == CHANNEL_CONTROL) frame[3] = seq_control++;
    else frame[3] = 0;

    if (len) std::memcpy(&frame[4], payload, len);

    ssize_t w = ::write(i2c_fd, frame.data(), frame.size());
    return (w == (ssize_t)frame.size());
}

// read on frame from SHTP (non-blocking)
bool BNO085::shtpRead(uint8_t& out_channel, std::vector<uint8_t>& out_payload) {
    if (i2c_fd < 0) return false;

    uint8_t hdr[4];
    ssize_t r = ::read(i2c_fd, hdr, 4);
    if (r < 0) {
        return false;
    }
    if (r != 4) {
        // if not complete header, ignore
        return false;
    }

    uint16_t total_len = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
    if (total_len < 4) {
        // empty/invalid header
        return false;
    }
    out_channel = hdr[2];
    uint16_t payload_len = total_len - 4;
    out_payload.resize(payload_len);

    // read payload
    size_t got = 0;
    while (got < payload_len) {
        ssize_t n = ::read(i2c_fd, out_payload.data() + got, payload_len - got);
        if (n < 0) {
            // if no data available, return false
            usleep(200);
            continue;
        }
        if (n == 0) {
            usleep(200);
            continue;
        }
        got += (size_t)n;
    }
    return true;
}

bool BNO085::enableLinearAcceleration(uint32_t period_us) {
    // SH-2 Set Feature (0xFD) format：
    // [0] ReportID=0xFD
    // [1] FeatureID (= 0x04 Linear Accel)
    // [2] Feature flags (u8) = 0
    // [3..4] Change sensitivity (u16) = 0 （no on-change）
    // [5..8] Report interval (u32, us)
    // [9..12] Batch interval (u32, us) = 0
    // [13] Sensor-specific config length optional bytes = 0
    uint8_t payload[13];
    payload[0] = SH2_SET_FEATURE;
    payload[1] = SH2_LINEAR_ACCEL;
    payload[2] = 0;                  // flags
    le16(&payload[3], 0);            // change sensitivity
    le32(&payload[5], period_us);    // report interval (us)
    le32(&payload[9], 0);            // batch interval

    if (!shtpWrite(CHANNEL_CONTROL, payload, sizeof(payload))) {
        std::cerr << "SHTP write(SetFeature) failed\n";
        return false;
    }

    // usleep(2000);
    return true;
}

bool BNO085::readLinearAcceleration(LinearAccel& out) {
    uint8_t ch;
    std::vector<uint8_t> pl;

    // read one SHTP packet
    if (!shtpRead(ch, pl)) return false;

    // Only process input channel and non-empty payload
    if (ch != CHANNEL_INPUT || pl.empty()) return false;

    // sensor data report ID
    uint8_t report_id = pl[0];
    if (report_id != SH2_LINEAR_ACCEL) return false;

    // linear acceleration data format:
    // Byte 0: Report ID (0x04)
    // 1: sequence
    // 2: status
    // 3: delay
    // 4..5: X (int16, Q=8)
    // 6..7: Y
    // 8..9: Z
    if (pl.size() < 10) return false;

    auto s16 = [&](int idx)->int16_t {
        return (int16_t)((int)pl[idx] | ((int)pl[idx+1] << 8));
    };

    const float scale = 1.0f / 256.0f; // Q=8
    out.x = (float)s16(4) * scale;
    out.y = (float)s16(6) * scale;
    out.z = (float)s16(8) * scale;
    return true;
}
