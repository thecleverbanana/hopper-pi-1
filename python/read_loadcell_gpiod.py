import time
import numpy as np
import gpiod

PIN_DT  = 5
PIN_SCK = 6
CHIP    = "gpiochip4"   # Adjust based on `sudo gpiodetect`

def read_raw(chip, dt_line, sck_line):
    # Wait until data ready
    while dt_line.get_value() == 1:
        time.sleep(0.00001)

    value = 0
    for _ in range(24):
        sck_line.set_value(1)
        time.sleep(0.000002)
        value = (value << 1) | dt_line.get_value()
        sck_line.set_value(0)
        time.sleep(0.000002)

    # extra pulse for gain
    sck_line.set_value(1)
    time.sleep(0.000002)
    sck_line.set_value(0)
    time.sleep(0.000002)

    if value & 0x800000:
        value |= ~0xFFFFFF

    return value

def main():
    chip = gpiod.Chip(CHIP)
    dt_line = chip.get_line(PIN_DT)
    sck_line = chip.get_line(PIN_SCK)

    dt_line.request(consumer="loadcell", type=gpiod.LINE_REQ_DIR_IN)
    sck_line.request(consumer="loadcell", type=gpiod.LINE_REQ_DIR_OUT)

    print("Starting HX711 reading loop...")
    while True:
        vals = [read_raw(chip, dt_line, sck_line) for _ in range(10)]
        avg = np.mean(vals)
        print(f"Raw mean: {avg:.1f}")
        time.sleep(0.1)

if __name__ == "__main__":
    main()
