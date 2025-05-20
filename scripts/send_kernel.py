import serial
import struct
import os
import time
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--window", "-w", action="store_true")
args = parser.parse_args()

kernel_img = "../build/kernel8.img"
kernel_size = os.path.getsize(kernel_img)

tag = "KIMG"
hex_tag = int.from_bytes(tag.encode(), "big")
header = struct.pack('>II', hex_tag, kernel_size)
interval = 8e-5
print(f"size: {kernel_size}")
print(header)

target_port = 'COM7' if args.window else '/dev/ttyUSB0'
ser = serial.Serial(target_port, baudrate=115200, timeout = 1)
# open('/dev/ttyUSB0', 'wb', buffering= 0) as target
with open(kernel_img, 'rb', buffering=0) as image:
    ser.write(header)
    print("header sent!")
    time.sleep(1)

    print("sending kernel...")
    while kernel_size > 0:
        ser.write(image.read(1))
        kernel_size -= 1
        time.sleep(interval)
    ser.flush()
    print("All Done!")
    time.sleep(1)

ser.close()