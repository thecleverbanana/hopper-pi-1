#include "ism330dhcx-pid/ism330dhcx_reg.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

// Structure to hold IMU context
typedef struct {
    uint8_t address;
    stmdev_ctx_t dev_ctx;
    int16_t accel_raw[3];
    float accel_mg[3];
    int initialized;
    uint64_t read_count;
    struct timespec last_read_time;
    struct timespec start_time;
    double current_freq;
    double avg_freq;
} imu_context_t;

// I2C device handle
static int i2c_fd = -1;
static uint8_t current_address = 0;

// Switch I2C address
static int switch_i2c_address(uint8_t address)
{
    if (current_address != address) {
        if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) {
            fprintf(stderr, "Failed to set I2C address 0x%02X: %s\n", address, strerror(errno));
            return -1;
        }
        current_address = address;
    }
    return 0;
}

// I2C read function for STM driver
static int32_t i2c_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len)
{
    uint8_t addr = *(uint8_t *)handle;
    
    if (switch_i2c_address(addr) != 0) {
        return -1;
    }
    
    if (write(i2c_fd, &reg, 1) != 1) {
        fprintf(stderr, "I2C write error (read setup) at 0x%02X: %s\n", addr, strerror(errno));
        return -1;
    }
    
    if (read(i2c_fd, data, len) != len) {
        fprintf(stderr, "I2C read error at 0x%02X: %s\n", addr, strerror(errno));
        return -1;
    }
    
    return 0;
}

// I2C write function for STM driver
static int32_t i2c_write(void *handle, uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t addr = *(uint8_t *)handle;
    
    if (switch_i2c_address(addr) != 0) {
        return -1;
    }
    
    uint8_t *buffer = malloc(len + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return -1;
    }
    
    buffer[0] = reg;
    memcpy(buffer + 1, data, len);
    
    if (write(i2c_fd, buffer, len + 1) != (len + 1)) {
        fprintf(stderr, "I2C write error at 0x%02X: %s\n", addr, strerror(errno));
        free(buffer);
        return -1;
    }
    
    free(buffer);
    return 0;
}

// Delay function
static void delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

// Initialize a single IMU
static int init_imu(imu_context_t *imu, int i2c_bus, uint8_t address)
{
    int ret;
    uint8_t whoami;
    
    imu->address = address;
    imu->initialized = 0;
    
    // Initialize device context
    imu->dev_ctx.write_reg = i2c_write;
    imu->dev_ctx.read_reg = i2c_read;
    imu->dev_ctx.mdelay = delay_ms;
    imu->dev_ctx.handle = &imu->address; // Store address in handle
    
    printf("\n=== Initializing IMU at address 0x%02X ===\n", address);
    
    // Check device ID (Who Am I)
    ret = ism330dhcx_device_id_get(&imu->dev_ctx, &whoami);
    if (ret != 0) {
        fprintf(stderr, "Failed to read device ID at 0x%02X: %d\n", address, ret);
        return -1;
    }
    
    if (whoami != ISM330DHCX_ID) {
        fprintf(stderr, "Invalid device ID at 0x%02X: 0x%02X (expected 0x%02X)\n", 
                address, whoami, ISM330DHCX_ID);
        return -1;
    }
    
    printf("Device ID: 0x%02X (OK)\n", whoami);
    
    // Reset device
    printf("Resetting device...\n");
    ret = ism330dhcx_reset_set(&imu->dev_ctx, PROPERTY_ENABLE);
    if (ret != 0) {
        fprintf(stderr, "Failed to reset device at 0x%02X: %d\n", address, ret);
        return -1;
    }
    
    // Wait for reset to complete
    uint8_t reset_status = PROPERTY_ENABLE;
    int timeout = 100; // 100 * 10ms = 1 second timeout
    while (reset_status == PROPERTY_ENABLE && timeout > 0) {
        delay_ms(10);
        ret = ism330dhcx_reset_get(&imu->dev_ctx, &reset_status);
        if (ret != 0) {
            fprintf(stderr, "Failed to read reset status at 0x%02X: %d\n", address, ret);
            return -1;
        }
        timeout--;
    }
    
    if (reset_status == PROPERTY_ENABLE) {
        fprintf(stderr, "Reset timeout at 0x%02X\n", address);
        return -1;
    }
    
    printf("Reset complete.\n");
    
    // Configure device
    printf("Configuring accelerometer...\n");
    
    // Enable auto-increment for multi-byte reads
    ret = ism330dhcx_auto_increment_set(&imu->dev_ctx, PROPERTY_ENABLE);
    if (ret != 0) {
        fprintf(stderr, "Failed to enable auto-increment at 0x%02X: %d\n", address, ret);
        return -1;
    }
    
    // Enable block data update
    ret = ism330dhcx_block_data_update_set(&imu->dev_ctx, PROPERTY_ENABLE);
    if (ret != 0) {
        fprintf(stderr, "Failed to enable block data update at 0x%02X: %d\n", address, ret);
        return -1;
    }
    
    // Set accelerometer full scale (4g)
    ret = ism330dhcx_xl_full_scale_set(&imu->dev_ctx, ISM330DHCX_4g);
    if (ret != 0) {
        fprintf(stderr, "Failed to set accelerometer full scale at 0x%02X: %d\n", address, ret);
        return -1;
    }
    printf("  Full scale: 4g\n");
    
    // Set accelerometer output data rate (1666 Hz - closest to 1000 Hz)
    ret = ism330dhcx_xl_data_rate_set(&imu->dev_ctx, ISM330DHCX_XL_ODR_1666Hz);
    if (ret != 0) {
        fprintf(stderr, "Failed to set accelerometer data rate at 0x%02X: %d\n", address, ret);
        return -1;
    }
    printf("  Data rate: 1666 Hz\n");
    
    // Initialize timing variables
    imu->read_count = 0;
    imu->current_freq = 0.0;
    imu->avg_freq = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &imu->last_read_time);
    clock_gettime(CLOCK_MONOTONIC, &imu->start_time);
    
    imu->initialized = 1;
    printf("IMU at 0x%02X initialized successfully!\n", address);
    
    return 0;
}

// Read acceleration from a single IMU (optimized for speed)
static int read_imu_accel(imu_context_t *imu)
{
    int ret;
    struct timespec current_time;
    
    if (!imu->initialized) {
        return -1;
    }
    
    // Get current time before read
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    // Read raw acceleration data directly (skip data ready check for speed)
    // The sensor is configured at 1666 Hz, so data should be ready
    ret = ism330dhcx_acceleration_raw_get(&imu->dev_ctx, imu->accel_raw);
    if (ret != 0) {
        return -1;
    }
    
    // Convert to mg (4g full scale)
    imu->accel_mg[0] = ism330dhcx_from_fs4g_to_mg(imu->accel_raw[0]);
    imu->accel_mg[1] = ism330dhcx_from_fs4g_to_mg(imu->accel_raw[1]);
    imu->accel_mg[2] = ism330dhcx_from_fs4g_to_mg(imu->accel_raw[2]);
    
    // Calculate frequency
    imu->read_count++;
    if (imu->read_count > 1) {
        // Calculate time difference in seconds (instantaneous frequency)
        double time_diff = (current_time.tv_sec - imu->last_read_time.tv_sec) +
                          (current_time.tv_nsec - imu->last_read_time.tv_nsec) / 1e9;
        
        if (time_diff > 0) {
            // Calculate instantaneous frequency (samples per second)
            imu->current_freq = 1.0 / time_diff;
        }
        
        // Calculate average frequency from start
        double total_time = (current_time.tv_sec - imu->start_time.tv_sec) +
                           (current_time.tv_nsec - imu->start_time.tv_nsec) / 1e9;
        
        if (total_time > 0) {
            imu->avg_freq = imu->read_count / total_time;
        }
    }
    
    imu->last_read_time = current_time;
    
    return 0;
}

int main(int argc, char *argv[])
{
    int i2c_bus = 1; // Default I2C bus for Raspberry Pi
    int ret;
    imu_context_t imu_a, imu_b;
    uint8_t addr_a = 0x6A;
    uint8_t addr_b = 0x6B;
    
    // Parse command line arguments
    if (argc > 1) {
        i2c_bus = atoi(argv[1]);
    }
    
    printf("ISM330DHCX Dual IMU Linear Acceleration Test\n");
    printf("I2C Bus: %d\n", i2c_bus);
    printf("IMU A Address: 0x%02X\n", addr_a);
    printf("IMU B Address: 0x%02X\n", addr_b);
    
    // Open I2C device
    char device[20];
    snprintf(device, sizeof(device), "/dev/i2c-%d", i2c_bus);
    i2c_fd = open(device, O_RDWR);
    if (i2c_fd < 0) {
        fprintf(stderr, "Failed to open I2C device %s: %s\n", device, strerror(errno));
        return 1;
    }
    
    // Initialize both IMUs
    if (init_imu(&imu_a, i2c_bus, addr_a) != 0) {
        fprintf(stderr, "Failed to initialize IMU A at 0x%02X\n", addr_a);
        close(i2c_fd);
        return 1;
    }
    
    delay_ms(50); // Small delay between initializations
    
    if (init_imu(&imu_b, i2c_bus, addr_b) != 0) {
        fprintf(stderr, "Failed to initialize IMU B at 0x%02X\n", addr_b);
        close(i2c_fd);
        return 1;
    }
    
    // Small delay to allow sensors to stabilize
    delay_ms(100);
    
    printf("\n=== Reading acceleration data (1000 loops) ===\n");
    printf("Format: Loop | IMU 0x6A | Accel: (X, Y, Z) mg @ Inst Hz / Avg Hz  ||  IMU 0x6B | Accel: (X, Y, Z) mg @ Inst Hz / Avg Hz\n\n");
    
    const int MAX_LOOPS = 1000;
    const int PRINT_INTERVAL = 50; // Print every 50 reads to reduce overhead
    int loop_count = 0;
    int print_counter = 0;
    
    // Read acceleration data from both IMUs in a loop
    while (loop_count < MAX_LOOPS) {
        // Try to read from IMU A (skip data ready check for speed)
        read_imu_accel(&imu_a);
        
        // Try to read from IMU B (skip data ready check for speed)
        read_imu_accel(&imu_b);
        
        // Print data periodically to reduce I/O overhead
        if (print_counter % PRINT_INTERVAL == 0 || loop_count == MAX_LOOPS - 1) {
            printf("Loop %4d | IMU 0x%02X | Accel: (%8.2f, %8.2f, %8.2f) mg @ %6.1f / %6.1f Hz  ||  ", 
                   loop_count, addr_a, imu_a.accel_mg[0], imu_a.accel_mg[1], imu_a.accel_mg[2], 
                   imu_a.current_freq, imu_a.avg_freq);
            printf("IMU 0x%02X | Accel: (%8.2f, %8.2f, %8.2f) mg @ %6.1f / %6.1f Hz\r", 
                   addr_b, imu_b.accel_mg[0], imu_b.accel_mg[1], imu_b.accel_mg[2], 
                   imu_b.current_freq, imu_b.avg_freq);
            fflush(stdout);
        }
        
        loop_count++;
        print_counter++;
        
        // No delay - read as fast as possible to reach 1000 Hz
    }
    
    // Print final statistics
    printf("\n\n=== Final Statistics ===\n");
    printf("IMU 0x%02X:\n", addr_a);
    printf("  Total reads: %lu\n", imu_a.read_count);
    printf("  Average frequency: %.2f Hz\n", imu_a.avg_freq);
    printf("  Last instantaneous frequency: %.2f Hz\n", imu_a.current_freq);
    
    printf("\nIMU 0x%02X:\n", addr_b);
    printf("  Total reads: %lu\n", imu_b.read_count);
    printf("  Average frequency: %.2f Hz\n", imu_b.avg_freq);
    printf("  Last instantaneous frequency: %.2f Hz\n", imu_b.current_freq);
    
    printf("\nTotal loops executed: %d\n", loop_count);
    
    close(i2c_fd);
    return 0;
}
