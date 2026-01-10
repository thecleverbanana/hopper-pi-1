#!/usr/bin/env python3
"""Simple motion capture receiver for Linear Actuator (single marker)."""

import socket
import threading
import struct
from dataclasses import dataclass


@dataclass
class MotionDataBodyFoot:
    """Container for body and foot position data."""
    body_x: float
    body_y: float
    body_z: float
    foot_x: float
    foot_y: float
    foot_z: float

    def to_bytes(self) -> bytes:
        """Pack to 6 floats (24 bytes) - for compatibility, though receiver only accepts 12 bytes."""
        return struct.pack('ffffff',
                          self.body_x, self.body_y, self.body_z,
                          self.foot_x, self.foot_y, self.foot_z)

    @classmethod
    def from_bytes(cls, data: bytes) -> 'MotionDataBodyFoot':
        """Unpack from 12 bytes (3 floats) - single marker position for both body and foot."""
        if len(data) != 12:
            raise ValueError(f"Expected 12 bytes, got {len(data)}")
        # Single marker position for both body and foot
        body_x, body_y, body_z = struct.unpack('fff', data[:12])
        return cls(body_x, body_y, body_z, body_x, body_y, body_z)


class MocapReceiver:
    """Simple UDP receiver for motion capture data."""

    def __init__(self, ip: str = "0.0.0.0", port: int = 9999):
        """Initialize the mocap receiver."""
        self.local_ip = ip
        self.local_port = port
        self.stop_flag = False
        self.data_received = False
        self.latest_data = None
        self.data_mutex = threading.Lock()
        self.sockfd = None
        self.recv_thread = None
        self.stop_flag = False
        self.data_received = False
        self.latest_data: Optional[MotionDataBodyFoot] = None
        self.data_mutex = threading.Lock()
        self.sockfd: Optional[socket.socket] = None
        self.recv_thread: Optional[threading.Thread] = None

    def start(self):
        """Start listening for UDP packets."""
        try:
            self.sockfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sockfd.bind((self.local_ip, self.local_port))
            print(f"Listening on {self.local_ip}:{self.local_port} ...")

            self.stop_flag = False
            self.recv_thread = threading.Thread(target=self._receive_loop)
            self.recv_thread.start()
        except Exception as e:
            print(f"Socket creation/bind failed: {e}")
            if self.sockfd:
                self.sockfd.close()
                self.sockfd = None

    def stop(self):
        """Stop the receiver."""
        self.stop_flag = True

        # Interrupt recvfrom() if blocked
        if self.sockfd:
            try:
                self.sockfd.shutdown(socket.SHUT_RDWR)
            except:
                pass

        if self.recv_thread and self.recv_thread.is_alive():
            self.recv_thread.join()

        if self.sockfd:
            self.sockfd.close()
            self.sockfd = None

    def has_data(self) -> bool:
        """Check if motion data has been received."""
        return self.data_received

    def get_latest_data(self) -> MotionDataBodyFoot:
        """Get the most recently received motion data."""
        with self.data_mutex:
            return self.latest_data

    def _receive_loop(self):
        """Background thread: receive UDP packets."""
        buffer = bytearray(1024)

        while not self.stop_flag:
            try:
                bytes_received, sender = self.sockfd.recvfrom_into(buffer)

                if bytes_received < 0:
                    if self.stop_flag:
                        break
                    print("recvfrom failed")
                    continue

                if bytes_received == 12:  # Exactly 12 bytes for single marker (3 floats)
                    motion_data = MotionDataBodyFoot.from_bytes(buffer[:12])

                    with self.data_mutex:
                        self.latest_data = motion_data
                    self.data_received = True

                    # Optional debug print
                    # print(f"Received data: body=({motion_data.body_x:.2f}, {motion_data.body_y:.2f}, {motion_data.body_z:.2f})")

            except OSError:
                if not self.stop_flag:
                    print("Socket error in receive loop")
                break


if __name__ == '__main__':
    # Simple test
    receiver = MocapReceiver("0.0.0.0", 9999)
    receiver.start()

    try:
        print("Waiting for mocap data...")
        import time
        while not receiver.has_data():
            time.sleep(0.1)

        data = receiver.get_latest_data()
        print(f"Received: body=({data.body_x:.2f}, {data.body_y:.2f}, {data.body_z:.2f}), "
              f"foot=({data.foot_x:.2f}, {data.foot_y:.2f}, {data.foot_z:.2f})")
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        receiver.stop()
