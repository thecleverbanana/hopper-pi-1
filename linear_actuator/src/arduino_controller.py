#!/usr/bin/env python3
"""Arduino/Serial controller for Hopper linear actuator."""

import serial
import time


class HopperController:
    """Controller for Hopper robot via serial communication."""

    def __init__(self, port, baud):
        """Initialize serial connection to Arduino.
        
        Args:
            port: Serial port (e.g., "/dev/ttyACM0")
            baud: Baud rate (e.g., 115200)
        """
        self.ser = serial.Serial(
            port=port,
            baudrate=baud,
            timeout=1
        )
        time.sleep(2)
        print(f"[OK] Connected to {port} @ {baud}")

    def send(self, cmd, delay=0.1):
        """Send a command to the Arduino.
        
        Args:
            cmd: Command string
            delay: Delay (seconds) after sending
        """
        print(f">> {cmd}")
        self.ser.write((cmd + "\n").encode())
        time.sleep(delay)

    def close(self):
        """Close the serial connection."""
        self.ser.close()
        print("[OK] Serial closed")
