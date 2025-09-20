import time
import statistics
import RPi.GPIO as GPIO
from hx711 import HX711
import numpy as np 

PIN_DT  = 5   # GPIO5
PIN_SCK = 6   # GPIO6

SAMPLES = 15          # Mean value
CHANNEL = 'A'         # 'A' or 'B'
GAIN    = 128         # 128：A Channel；32：AChannel；64：Channel

# Calibration 
SCALE   = 1.0         # g
OFFSET  = 0           

def main():
    GPIO.setwarnings(False)
    hx = HX711(dout_pin=PIN_DT, pd_sck_pin=PIN_SCK, channel=CHANNEL, gain=GAIN)
    hx.reset()

    try:
        while True:
            raw = np.mean(hx.get_raw_data())
            print(raw)
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        GPIO.cleanup()

if __name__ == "__main__":
    main()
