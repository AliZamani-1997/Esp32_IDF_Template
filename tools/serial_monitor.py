#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Minimal serial monitor / flasher helper for Windows.
# Handy when you don't want to open the full ESP-IDF environment.
#
# Usage:
#   python tools/serial_monitor.py COM5 115200
#
# Requires: pyserial  (pip install pyserial)

import sys
import serial


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print("Opening %s at %d baud (Ctrl+C to quit)..." % (port, baud))
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            while True:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    sys.stdout.write(data.decode("utf-8", errors="replace"))
                    sys.stdout.flush()
    except serial.SerialException as exc:
        print("Serial error: %s" % exc)
        return 1
    except KeyboardInterrupt:
        print("\nBye.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
