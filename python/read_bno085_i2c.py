# probe_bno085_basic.py
import time, board, busio
from adafruit_bno08x.i2c import BNO08X_I2C
from adafruit_bno08x import BNO_REPORT_ACCELEROMETER

i2c = busio.I2C(board.SCL, board.SDA)
addr = 0x4B   # change to 0x4A if i2cdetect shows that instead
bno = BNO08X_I2C(i2c, address=addr)  # <-- NO reset_pin / int_pin

# enable just one simple feature first
bno.enable_feature(BNO_REPORT_ACCELEROMETER)

print("BNO085 accel OK @ 0x%02X" % addr)
while True:
    ax, ay, az = bno.acceleration
    print(f"Acc {ax:.2f},{ay:.2f},{az:.2f}")
    time.sleep(0.025)
