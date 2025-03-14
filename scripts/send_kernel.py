import struct
import os
import time

kernel_img = "../build/kernel8.img"
kernel_size = os.path.getsize(kernel_img)

tag = "KIMG"
hex_tag = int.from_bytes(tag.encode(), "big")
header = struct.pack('>II', hex_tag, kernel_size)

interval = 8e-5

with open('/dev/pts/7', 'wb', buffering= 0) as target,\
    open(kernel_img, 'rb', buffering=0) as image:
    target.write(header)
    time.sleep(3)

    while kernel_size > 0:
        target.write(image.read(1))
        kernel_size -= 1
        time.sleep(interval)
    time.sleep(1)
