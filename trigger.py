#!/usr/bin/env python3
"""
Minimal host script to trigger TinyUSB RP2040 MSC endpoint double-arming bug
"""
import os
import sys
import subprocess
import time

def find_msc_device():
    """Find the first external USB MSC device"""
    if sys.platform == "darwin":
        # macOS: use diskutil to find external disks
        try:
            result = subprocess.run(['diskutil', 'list'],
                                  capture_output=True, text=True, check=True)
            lines = result.stdout.split('\n')

            for line in lines:
                if '/dev/disk' in line and 'external' in line.lower():
                    # Extract disk number
                    parts = line.split()
                    for part in parts:
                        if part.startswith('/dev/disk'):
                            return part.replace('disk', 'rdisk')

            print("No external USB disk found")
            return None

        except subprocess.CalledProcessError as e:
            print(f"Error running diskutil: {e}")
            return None
    else:
        print("This script currently supports macOS only")
        return None

def trigger_bug(device_path):
    """Repeatedly read from device to trigger the bug"""
    print(f"Reading from {device_path} to trigger the bug...")
    print("The device should panic/crash after a few reads.")

    try:
        with open(device_path, 'rb') as f:
            for i in range(100):  # Read 100 sectors
                f.seek(0)  # Always read from LBA 0
                data = f.read(512)
                print(f"Read {len(data)} bytes (iteration {i+1})")

                # Small delay to allow device to process
                time.sleep(0.01)

    except IOError as e:
        print(f"Device access failed: {e}")
        print("This might indicate the bug was triggered (device disconnected)")
        return True
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False

    return False

if __name__ == "__main__":
    device = find_msc_device()
    if device:
        print(f"Found device: {device}")
        trigger_bug(device)
    else:
        print("No suitable device found")
        sys.exit(1)
