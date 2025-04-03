#!/bin/bash

for file in *.S; do
    if [ -f "$file" ]; then #TODO: what is []? and what is [[]]?
        base_name="${file%.S}"
        clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 -nostdlib "$file" -o "$base_name.elf"
        llvm-objcopy --output-target=aarch64-rpi3-elf -O binary "$base_name.elf" "$base_name.img"
        mv "$base_name.img" ../rootfs/
    fi
done
