#!/usr/bin/env python3
"""Linear Actuator control application with integrated terminal GUI.

Supports:
  - Curses-based terminal GUI (default)
  - Command-line modes (--home, --run, --interactive)
"""

import curses
import threading
import time
from datetime import datetime
from pathlib import Path
from typing import Optional

import argparse
import sys
import serial

from src.arduino_controller import HopperController
from src.mocap_receiver import MocapReceiver

DEFAULT_PORT = "/dev/ttyACM0"
DEFAULT_BAUD = 115200
DEFAULT_MOCAP_IP = "0.0.0.0"
DEFAULT_MOCAP_PORT = 9999


# ===== GUI CLASS =====

class LinearActuatorGUI:
    """Lightweight curses GUI for Linear Actuator control and mocap recording."""

    def __init__(self, arduino_port: str, arduino_baud: int, mocap_ip: str = "0.0.0.0", mocap_port: int = 9999, speed_low: int = 50, speed_high: int = 100):
        self.arduino_port = arduino_port
        self.arduino_baud = arduino_baud
        self.mocap_ip = mocap_ip
        self.mocap_port = mocap_port
        self.speed_low = speed_low
        self.speed_high = speed_high
        self.current_speed = speed_low  # Default to low speed
        self.speed_input_mode = False  # For custom speed input
        self.speed_input_buffer = ""   # For direct number entry

        self.arduino: Optional[HopperController] = None
        self.mocap: Optional[MocapReceiver] = None

        self.recording = False
        self.record_file: Optional[Path] = None
        self.record_thread: Optional[threading.Thread] = None
        self.stop_recording = False

        self.status_msg = ""
        self.status_time = 0.0

        # Velocity tracking for Y-axis
        self.prev_y_pos = None
        self.prev_y_time = None
        self.velocity_y = 0.0
        
        # Menu items (will be generated dynamically)
        self.menu_items = []
        self._update_menu_items()

    def run(self, stdscr):
        """Main GUI loop."""
        # Initialize colors for better terminal compatibility
        curses.start_color()
        curses.use_default_colors()

        # Define color pairs (foreground, background)
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)    # Selected item
        curses.init_pair(2, curses.COLOR_GREEN, -1)                   # Status OK
        curses.init_pair(3, curses.COLOR_RED, -1)                     # Status error
        curses.init_pair(4, curses.COLOR_YELLOW, -1)                  # Recording

        curses.curs_set(0)
        stdscr.nodelay(True)      # Non-blocking input
        stdscr.timeout(50)        # 50ms timeout (faster response, less CPU)
        
        try:
            self._connect_hardware()
            self._gui_loop(stdscr)
        finally:
            self._cleanup()
            curses.curs_set(1)

    def _connect_hardware(self):
        """Connect to Arduino and start mocap receiver."""
        try:
            self.arduino = HopperController(self.arduino_port, self.arduino_baud)
            self.status_msg = "Arduino: OK"
        except Exception as e:
            self.status_msg = f"Arduino: FAIL - {str(e)[:30]}"

        try:
            self.mocap = MocapReceiver(ip=self.mocap_ip, port=self.mocap_port)
            self.mocap.start()
            self.status_msg = "Ready"
        except Exception as e:
            self.status_msg = f"Mocap: FAIL - {str(e)[:30]}"

    def _cleanup(self):
        """Shutdown all hardware and stop recording."""
        if self.recording:
            self._stop_recording()
        # Stop motor before closing
        if self.arduino:
            try:
                self.arduino.send("s")  # Stop command
            except Exception:
                pass  # Ignore errors during cleanup
        if self.mocap:
            self.mocap.stop()
        if self.arduino:
            self.arduino.close()

    def _gui_loop(self, stdscr):
        """Main event loop - optimized for performance."""
        selected = 0
        last_render = 0
        render_interval = 0.1  # Render every 100ms
        frame_count = 0
        
        while True:
            now = time.time()
            
            # Non-blocking input read
            key = stdscr.getch()
            
            # Handle input immediately (don't wait for render interval)
            if key == ord('q') or key == ord('Q'):
                break
            elif self.speed_input_mode:
                # Speed input mode: handle direct number entry
                if key >= ord('0') and key <= ord('9'):
                    # Add digit to buffer (max 5 digits for 0-15000 range)
                    if len(self.speed_input_buffer) < 5:
                        self.speed_input_buffer += chr(key)
                elif key == curses.KEY_BACKSPACE or key == 127 or key == 8:  # Backspace/Delete
                    # Remove last character
                    self.speed_input_buffer = self.speed_input_buffer[:-1]
                elif key == ord('\n'):  # Enter - confirm speed
                    try:
                        new_speed = int(self.speed_input_buffer) if self.speed_input_buffer else 0
                        self.current_speed = max(0, min(15000, new_speed))  # Clamp to 0-15000
                        self.speed_input_mode = False
                        self.speed_input_buffer = ""
                        self._update_menu_items()
                        self._set_status(f"Speed set to {self.current_speed}")
                    except ValueError:
                        self._set_status("Invalid speed value")
                        self.speed_input_buffer = ""
                elif key == 27:  # Escape - cancel
                    self.speed_input_mode = False
                    self.speed_input_buffer = ""
                    self._set_status("Speed input cancelled")
            else:
                # Normal menu navigation
                if key == curses.KEY_UP:
                    selected = (selected - 1) % len(self.menu_items)
                elif key == curses.KEY_DOWN:
                    selected = (selected + 1) % len(self.menu_items)
                elif key == ord('\n'):
                    action = self.menu_items[selected][1]
                    if action is None:
                        break
                    if action:
                        action()
            
            # Render at controlled interval to avoid CPU thrashing
            if (now - last_render) > render_interval:
                self._render(stdscr, selected)
                last_render = now
                frame_count += 1

    def _render(self, stdscr, selected: int):
        """Render the GUI - minimal and fast."""
        try:
            stdscr.clear()
            max_y, max_x = stdscr.getmaxyx()
            
            # Title (single line, no exception handling overhead)
            title = "= LINEAR ACTUATOR CONTROL ="
            title_x = max(0, (max_x - len(title)) // 2)
            if title_x < max_x:
                stdscr.addstr(0, title_x, title)

            # Speed input mode display
            if self.speed_input_mode:
                display_value = self.speed_input_buffer if self.speed_input_buffer else "0"
                speed_msg = f"ENTER SPEED: {display_value}_  (0-15000, Enter to confirm, Esc to cancel)"
                speed_y = 1
                if speed_y < max_y:
                    speed_msg = speed_msg[:max_x-2].ljust(max_x-2)
                    stdscr.addstr(speed_y, 2, speed_msg, curses.A_BOLD)

            # Menu items
            menu_start_y = 3 if self.speed_input_mode else 2
            for idx, (label, _) in enumerate(self.menu_items):
                y = menu_start_y + idx
                if y >= max_y - 3:
                    break
                
                prefix = ">> " if idx == selected else "   "
                line = f"{prefix}{label}"
                
                # Truncate to screen width
                line = line[:max_x-2]
                
                if idx == selected:
                    stdscr.addstr(y, 2, line, curses.color_pair(1))
                else:
                    stdscr.addstr(y, 2, line)

            # Status area
            menu_offset = 1 if self.speed_input_mode else 0
            status_y = menu_start_y + len(self.menu_items) + 2 + menu_offset
            if status_y < max_y:
                elapsed = time.time() - self.status_time
                msg = self.status_msg if elapsed < 3 else ""
                if msg:
                    msg = msg[:max_x-2].ljust(max_x-2)
                    # Color code status messages
                    if "ERROR" in msg or "FAIL" in msg:
                        stdscr.addstr(status_y, 2, msg, curses.color_pair(3))  # Red for errors
                    elif "OK" in msg or "Ready" in msg or "Saved" in msg:
                        stdscr.addstr(status_y, 2, msg, curses.color_pair(2))  # Green for success
                    else:
                        stdscr.addstr(status_y, 2, msg)  # Default color for other messages

            # Mocap status (velocity display)
            if status_y + 1 < max_y and self.mocap and self.mocap.has_data():
                data = self.mocap.get_latest_data()
                if data:
                    # Calculate Y-axis velocity
                    current_time = time.time()
                    if self.prev_y_pos is not None and self.prev_y_time is not None:
                        dt = current_time - self.prev_y_time
                        if dt > 0.001:  # Avoid division by very small numbers
                            dy = data.body_y - self.prev_y_pos
                            self.velocity_y = dy / dt

                    # Update previous values
                    self.prev_y_pos = data.body_y
                    self.prev_y_time = current_time

                    mocap_msg = f"Leg Vel Y: {self.velocity_y:+.4f} m/s"
                    mocap_msg = mocap_msg[:max_x-2].ljust(max_x-2)
                    stdscr.addstr(status_y + 1, 2, mocap_msg)

            # Recording status
            if status_y + 2 < max_y and self.recording and self.record_file:
                rec_msg = f"REC: {self.record_file.name}"
                rec_msg = rec_msg[:max_x-2].ljust(max_x-2)
                stdscr.addstr(status_y + 2, 2, rec_msg, curses.color_pair(4))  # Yellow for recording

            stdscr.refresh()
        except Exception as e:
            pass  # Silently ignore render errors (window resize, etc)

    # ===== Command handlers =====

    def _cmd_home(self):
        if self.arduino:
            self.arduino.send("h")
            self._set_status("Home sent")
        else:
            self._set_status("ERROR: Arduino not connected")

    def _cmd_run(self, speed: int):
        if self.arduino:
            self.arduino.send(f"r {speed}")
            self._set_status(f"Run {speed}")
        else:
            self._set_status("ERROR: Arduino not connected")

    def _cmd_stop(self):
        if self.arduino:
            self.arduino.send("s")
            self._set_status("Stop sent")
        else:
            self._set_status("ERROR: Arduino not connected")

    def _cmd_record_start(self):
        if self.recording:
            self._set_status("Already recording")
            return

        if not self.mocap or not self.mocap.has_data():
            self._set_status("ERROR: Mocap not ready")
            return

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.record_file = Path("mocap_data") / f"linear_actuator_{timestamp}.csv"
        self.record_file.parent.mkdir(parents=True, exist_ok=True)

        self.recording = True
        self.stop_recording = False
        self.record_thread = threading.Thread(target=self._record_loop, daemon=True)
        self.record_thread.start()
        self._set_status(f"Recording...")

    def _cmd_record_stop(self):
        if not self.recording:
            self._set_status("Not recording")
            return

        self.stop_recording = True
        if self.record_thread:
            self.record_thread.join(timeout=1)

        self.recording = False
        self._set_status(f"Saved: {self.record_file.name if self.record_file else 'unknown'}")

    def _cmd_run_current(self):
        """Run with current configured speed."""
        if self.arduino:
            self.arduino.send(f"r {self.current_speed}")
            self._set_status(f"Run {self.current_speed}")
        else:
            self._set_status("ERROR: Arduino not connected")

    def _cmd_set_speed(self):
        """Enter speed input mode for custom speed setting."""
        self.speed_input_mode = True
        self.speed_input_buffer = str(self.current_speed)  # Pre-fill with current speed
        self._set_status("Enter speed (0-15000), Enter to confirm, Esc to cancel")

    def _cmd_trial(self):
        """Run full trial sequence: home -> record start -> run -> stop record."""
        import time

        def sequence_step(step_name, delay=0.5):
            self._set_status(f"Trial: {step_name}")
            time.sleep(delay)

        try:
            # Step 1: Home
            sequence_step("Homing...")
            self._cmd_home()
            time.sleep(2)  # Wait for homing to complete

            # Step 2: Start recording
            sequence_step("Starting recording...")
            self._cmd_record_start()
            time.sleep(0.5)

            # Step 3: Run with current speed
            sequence_step(f"Running at {self.current_speed}...")
            self._cmd_run_current()
            time.sleep(3)  # Run for 3 seconds

            # Step 4: Stop
            sequence_step("Stopping motor...")
            self._cmd_stop()
            time.sleep(0.5)

            # Step 5: Stop recording
            sequence_step("Stopping recording...")
            self._cmd_record_stop()
            time.sleep(0.5)

            self._set_status("Trial completed!")

        except Exception as e:
            self._set_status(f"Trial failed: {str(e)[:30]}")

    def _update_menu_items(self):
        """Update menu items with current speed."""
        self.menu_items = [
            ("Home", self._cmd_home),
            (f"Run {self.current_speed}", self._cmd_run_current),
            ("Set Speed", self._cmd_set_speed),
            ("Trial", self._cmd_trial),
            ("Stop", self._cmd_stop),
            ("Record Start", self._cmd_record_start),
            ("Record Stop", self._cmd_record_stop),
            ("Exit", None),
        ]

    def _stop_recording(self):
        self.stop_recording = True
        if self.record_thread and self.record_thread.is_alive():
            self.record_thread.join(timeout=1)
        self.recording = False

    def _record_loop(self):
        """Background thread: save mocap data to CSV."""
        try:
            with open(self.record_file, 'w') as f:
                f.write("timestamp,body_x,body_y,body_z,foot_x,foot_y,foot_z\n")

                while not self.stop_recording:
                    data = self.mocap.get_latest_data()
                    if data:
                        ts = datetime.now().isoformat()
                        f.write(f"{ts},{data.body_x},{data.body_y},{data.body_z},"
                                f"{data.foot_x},{data.foot_y},{data.foot_z}\n")
                        f.flush()
                    time.sleep(0.01)
        except Exception as e:
            self._set_status(f"Record error: {str(e)[:30]}")
            self.recording = False

    def _set_status(self, msg: str):
        self.status_msg = msg
        self.status_time = time.time()


# ===== CLI HELPER FUNCTIONS =====

def run_once(ctrl, speed):
    """Home and run sequence."""
    ctrl.send("h")
    time.sleep(3)      
    ctrl.send(f"r {speed}")
    time.sleep(5) 


def interactive(ctrl):
    """Interactive command-line mode."""
    print("Interactive mode")
    print("Commands: h | r <speed> | s | ? | exit")

    while True:
        try:
            cmd = input("cmd> ").strip()
            if cmd in ("exit", "quit"):
                break
            if cmd == "":
                continue
            ctrl.send(cmd)
        except KeyboardInterrupt:
            print("\n[CTRL-C]")
            break


# ===== MAIN =====

def main():
    parser = argparse.ArgumentParser(
        description="Linear Actuator Control",
        epilog="Default (no args): Launch curses GUI. Use --help for other modes."
    )
    parser.add_argument("--port", default=DEFAULT_PORT, help="Serial port (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate (default: 115200)")
    parser.add_argument("--mocap-ip", default=DEFAULT_MOCAP_IP, help=f"Mocap IP (default: {DEFAULT_MOCAP_IP})")
    parser.add_argument("--mocap-port", type=int, default=DEFAULT_MOCAP_PORT,
                       help=f"Mocap UDP port (default: {DEFAULT_MOCAP_PORT})")
    parser.add_argument("--speed-low", type=int, default=50, help="Low speed for Run Low command (default: 50)")
    parser.add_argument("--speed-high", type=int, default=100, help="High speed for Run High command (default: 100)")

    parser.add_argument("--home", action="store_true", help="Send home command and exit")
    parser.add_argument("--run", type=int, metavar="SPEED", help="Home, run at SPEED, then exit")
    parser.add_argument("--interactive", action="store_true", help="Interactive CLI mode")
    parser.add_argument("--gui", action="store_true", help="Launch GUI (default if no other mode)")

    args = parser.parse_args()

    # Determine mode
    mode_count = sum([args.home, args.run is not None, args.interactive, args.gui])
    use_gui = (mode_count == 0) or args.gui
    use_cli_mode = args.home or args.run is not None or args.interactive

    # GUI mode (default)
    if use_gui:
        try:
            gui = LinearActuatorGUI(arduino_port=args.port, arduino_baud=args.baud,
                           mocap_ip=args.mocap_ip, mocap_port=args.mocap_port,
                           speed_low=args.speed_low, speed_high=args.speed_high)
            curses.wrapper(gui.run)
        except KeyboardInterrupt:
            print("\n[CTRL-C] Stopping motor and exiting...")
            # Try to stop the motor before exiting
            try:
                if gui.arduino:
                    gui.arduino.send("s")  # Stop command
                    print("Motor stopped")
            except Exception as e:
                print(f"Warning: Could not stop motor: {e}")
        except Exception as e:
            print(f"[ERROR] GUI failed: {e}", file=sys.stderr)
            sys.exit(1)
        return

    # CLI modes
    try:
        ctrl = HopperController(args.port, args.baud)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open serial port: {e}", file=sys.stderr)
        sys.exit(1)

    try:
        if args.home:
            ctrl.send("h")
        elif args.run is not None:
            run_once(ctrl, args.run)
        elif args.interactive:
            interactive(ctrl)
    finally:
        ctrl.close()


if __name__ == "__main__":
    main()
