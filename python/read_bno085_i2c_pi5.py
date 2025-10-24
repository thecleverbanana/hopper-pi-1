import time
import board
import busio
from adafruit_bno08x.i2c import BNO08X_I2C
from adafruit_bno08x import BNO_REPORT_ACCELEROMETER

REPORT_INTERVAL = 1000  # 1ms = 1000µs → 1000Hz
i2c = busio.I2C(board.SCL, board.SDA)
bno = BNO08X_I2C(i2c, address=0x4A)
bno.enable_feature(BNO_REPORT_ACCELEROMETER, REPORT_INTERVAL)

print("Measuring TRUE sensor refresh frequency for 5 seconds...\n")
time.sleep(1)

t_start = time.time()
t_last_change = t_start
t_prev = t_start
prev_accel = None
change_times = []

while (time.time() - t_start) < 5.0:
    ax, ay, az = bno.acceleration
    now = time.time()

    if prev_accel != (ax, ay, az):
        change_times.append(now - t_last_change)
        t_last_change = now
    prev_accel = (ax, ay, az)
    t_prev = now

if len(change_times) > 1:
    avg_dt = sum(change_times) / len(change_times)
    freq = 1.0 / avg_dt
    print(f"Unique samples: {len(change_times)} in 5s")
    print(f"Average Δt = {avg_dt*1000:.3f} ms  →  True refresh = {freq:.1f} Hz")
    print(f"Min Δt = {min(change_times)*1000:.3f} ms, Max Δt = {max(change_times)*1000:.3f} ms")
else:
    print("Not enough unique frames detected.")
