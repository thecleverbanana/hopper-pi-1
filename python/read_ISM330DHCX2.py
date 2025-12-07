#!/usr/bin/env python3
import qwiic_ism330dhcx
import sys
import time

def init_imu(addr):
    imu = qwiic_ism330dhcx.QwiicISM330DHCX(address=addr)




    imu.begin()
    imu.device_reset()
    while not imu.get_device_reset():
        time.sleep(0.1)

    imu.set_device_config()
    imu.set_block_data_update()
    imu.set_accel_data_rate(imu.kXlOdr104Hz)
    imu.set_accel_full_scale(imu.kXlFs4g)
    imu.set_accel_filter_lp2()
    imu.set_accel_slope_filter(imu.kLpOdrDiv100)

    imu.set_gyro_data_rate(imu.kGyroOdr104Hz)
    imu.set_gyro_full_scale(imu.kGyroFs500dps)
    imu.set_gyro_filter_lp1()
    imu.set_gyro_lp1_bandwidth(imu.kBwMedium)

    print(f"[OK] IMU initialized at address 0x{addr:02X}")
    return imu


def run_dual_imu():
    print("\nQwiic ISM330DHCX Dual IMU Example\n")

    imu_a = init_imu(0x6A)
    imu_b = init_imu(0x6B)

    if imu_a is None and imu_b is None:
        print("[FATAL] No IMUs found.")
        return
    elif imu_a is None:
        print("[WARN] Only IMU 0x6B active.")
    elif imu_b is None:
        print("[WARN] Only IMU 0x6A active.")

    time.sleep(0.1)

    while True:
        line = ""
        if imu_a and imu_a.check_status():
            a_accel = imu_a.get_accel()
            a_gyro = imu_a.get_gyro()
            line += f"IMU 0x6A | Accel: ({a_accel.xData:7.2f}, {a_accel.yData:7.2f}, {a_accel.zData:7.2f}) mg | "
            line += f"Gyro: ({a_gyro.xData:7.2f}, {a_gyro.yData:7.2f}, {a_gyro.zData:7.2f}) dps  ||  "

        if imu_b and imu_b.check_status():
            b_accel = imu_b.get_accel()
            b_gyro = imu_b.get_gyro()
            line += f"IMU 0x6B | Accel: ({b_accel.xData:7.2f}, {b_accel.yData:7.2f}, {b_accel.zData:7.2f}) mg | "
            line += f"Gyro: ({b_gyro.xData:7.2f}, {b_gyro.yData:7.2f}, {b_gyro.zData:7.2f}) dps"

        if line:
            print(line)
        time.sleep(0.1)


if __name__ == '__main__':
    try:
        run_dual_imu()
    except (KeyboardInterrupt, SystemExit):
        print("\n[EXIT] Ending dual IMU example.")
        sys.exit(0)
