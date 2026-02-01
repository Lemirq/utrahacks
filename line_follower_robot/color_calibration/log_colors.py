#!/usr/bin/env python3
"""
Color Sensor Data Logger

Captures color sensor readings from Arduino and saves to CSV file.
Press Ctrl+C to stop logging.

Usage:
    python3 log_colors.py

The script will auto-detect your Arduino port on macOS.
"""

import serial
import serial.tools.list_ports
import time
import sys
from datetime import datetime

def find_arduino_port():
    """Auto-detect Arduino port on macOS"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Arduino UNO R4 WiFi typically shows up as usbmodem
        if 'usbmodem' in port.device or 'usbserial' in port.device:
            return port.device
    return None

def main():
    # Find Arduino port
    port = find_arduino_port()

    if not port:
        print("ERROR: Arduino not found!")
        print("Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"  - {p.device}: {p.description}")
        sys.exit(1)

    print(f"Found Arduino at: {port}")

    # Create output filename with timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"color_data_{timestamp}.csv"

    print(f"Logging to: {output_file}")
    print()
    print("=" * 50)
    print("INSTRUCTIONS:")
    print("1. Place sensor over each color for 5-10 seconds")
    print("2. Move slowly between colors")
    print("3. Press Ctrl+C when done")
    print("=" * 50)
    print()
    print("Starting in 3 seconds...")
    time.sleep(3)

    try:
        # Open serial connection
        ser = serial.Serial(port, 9600, timeout=1)
        time.sleep(2)  # Wait for Arduino to reset

        # Open output file
        with open(output_file, 'w') as f:
            reading_count = 0

            print("\nLogging started! Move sensor over colors...")
            print("-" * 50)
            print("Reading | Red   | Green | Blue")
            print("-" * 50)

            while True:
                line = ser.readline().decode('utf-8').strip()

                if line and not line.startswith('reading'):
                    # Write to file
                    f.write(line + '\n')
                    f.flush()

                    # Parse and display
                    parts = line.split(',')
                    if len(parts) >= 4:
                        reading_count += 1
                        r, g, b = parts[1], parts[2], parts[3]
                        print(f"{reading_count:7} | {r:5} | {g:5} | {b:5}")

                elif line.startswith('reading'):
                    # Write header
                    f.write(line + '\n')

    except KeyboardInterrupt:
        print("\n")
        print("=" * 50)
        print(f"Logging stopped!")
        print(f"Data saved to: {output_file}")
        print(f"Total readings: {reading_count}")
        print("=" * 50)

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
