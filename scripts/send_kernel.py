import struct
import os
import time

kernel_img = "../build/kernel8.img"
kernel_size = os.path.getsize(kernel_img)

tag = "KIMG"
hex_tag = int.from_bytes(tag.encode(), "big")
header = struct.pack('>II', hex_tag, kernel_size)
interval = 8e-5
print(f"size: {kernel_size}")
print(header)

with open('/dev/ttyUSB0', 'wb', buffering= 0) as target,\
    open(kernel_img, 'rb', buffering=0) as image:
    target.write(header)
    print("header sent!")
    time.sleep(1)

    print("sending kernel...")
    while kernel_size > 0:
        target.write(image.read(1))
        kernel_size -= 1
        time.sleep(interval)
    target.flush()
    print("All Done!")
    time.sleep(1)
