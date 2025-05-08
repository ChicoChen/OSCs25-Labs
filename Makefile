# ---------- Directories ----------
INCLUDE_DIR = include/
LIB_DIR = lib/
SRC_DIR = src/
BUILD_DIR = build/
ROOTFS = rootfs/
# ---------- Program Name ----------
LOADER = loader
KERNEL = kernel8
INITRAMFS = m_initramfs.cpio
EXCEPTION = exception
# ---------- Compile flag ----------
NO_WARN = -Wno-incompatible-library-redeclaration -Wno-asm-operand-widths -Wno-pointer-to-int-cast -Wno-int-to-void-pointer-cast -Wno-c23-extensions
COMPILE_FLAG = -nostdlib -ffreestanding -g $(NO_WARN)
LLDB_FLAG = -s -S
MINI_UART_FLAG = -serial null -serial stdio
INITRAMFS_FLAG = -initrd initramfs.cpio
DTB_FLAG = -dtb bcm2710-rpi-3-b-plus.dtb

# ---------- Dependencies ----------
LOADER_DEPS = mini_uart str_utils utils

# ---------- Files ----------
ASSEMBLIES = $(wildcard $(SRC_DIR)*.S)
LOADER_ENTRY = $(LOADER)_entry.c
KERNEL_ENTRY = $(KERNEL)_entry.c
LIBFILES = $(shell find lib -type f -iname '*.S') $(shell find lib -type f -iname '*.c')
# OBJECTS = $(LIBFILES:.c=.o)

.PHONY: all
all: build loader kernel initramfs

test:
	echo $(LIBFILES)
# ---------- Building section ----------
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)$(LIB_DIR)

initramfs:
	cd $(ROOTFS) && find . | cpio -o -H newc > ../$(INITRAMFS)

src_obj: $(ASSEMBLIES) build
	@$(foreach source_s, $(ASSEMBLIES), \
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c $(source_s) \
				-o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(source_s:.S=.o));\
	)

lib_obj: $(LIBFILES) build
	@$(foreach lib_source, $(LIBFILES),\
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(lib_source) \
				-o $(BUILD_DIR)$(LIB_DIR)$(notdir $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(lib_source))));\
	)
	
	@clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(SRC_DIR)$(LOADER_ENTRY) -o $(BUILD_DIR)$(LOADER_ENTRY:.c=.o)
	@clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(SRC_DIR)$(KERNEL_ENTRY) -o $(BUILD_DIR)$(KERNEL_ENTRY:.c=.o)

loader: src_obj lib_obj
	ld.lld -m aarch64elf -T $(SRC_DIR)$(LOADER).ld -o $(BUILD_DIR)$(LOADER).elf \
			$(BUILD_DIR)$(LOADER).o $(BUILD_DIR)$(LOADER_ENTRY:.c=.o) \
			$(patsubst %, $(BUILD_DIR)$(LIB_DIR)%, $(LOADER_DEPS:=.o))
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(LOADER).elf $(BUILD_DIR)$(LOADER).img

kernel: src_obj lib_obj
	ld.lld -m aarch64elf -T $(SRC_DIR)$(KERNEL).ld -o $(BUILD_DIR)$(KERNEL).elf \
							$(BUILD_DIR)$(KERNEL).o $(BUILD_DIR)$(KERNEL_ENTRY:.c=.o)\
							$(wildcard $(BUILD_DIR)$(LIB_DIR)*.o)
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(KERNEL).elf $(BUILD_DIR)$(KERNEL).img

# ---------- Debug Section ----------
load: loader kernel initramfs
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(LOADER).img -display none -serial null -serial pty $(MINI_UART_FLAG)  $(INITRAMFS_FLAG) -d in_asm $(LLDB_FLAG) 

load_debug: loader kernel initramfs
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(LOADER).img -display none -serial null -serial pty $(INITRAMFS_FLAG) $(LLDB_FLAG) 

run: kernel initramfs
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG) $(INITRAMFS_FLAG) $(DTB_FLAG)

debug: kernel initramfs
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG) $(DTB_FLAG) $(INITRAMFS_FLAG) $(LLDB_FLAG)

asm: kernel initramfs
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none -d in_asm $(MINI_UART_FLAG) $(DTB_FLAG) $(INITRAMFS_FLAG) $(LLDB_FLAG)

# ---------- Debug Section ----------
clean: 
	rm -rf $(BUILD_DIR)
