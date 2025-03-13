# ---------- Directories ----------
INCLUDE_DIR = include/
LIB_DIR = lib/
SRC_DIR = src/
BUILD_DIR = build/
# ---------- Program Name ----------
LOADER = loader
KERNEL = kernel8
# ---------- Compile flag ----------
COMPILE_FLAG = -nostdlib -g
LLDB_FLAG = -s -S
MINI_UART_FLAG = -serial null -serial stdio

# ---------- Files ----------
ASSEMBLIES = $(wildcard $(SRC_DIR)*.S)
LOADER_ENTRY = $(LOADER)_entry.c
KERNEL_ENTRY = $(KERNEL)_entry.c
CFILES = $(wildcard $(LIB_DIR)*.c)
# OBJECTS = $(CFILES:.c=.o)


.PHONY: all
all: $(BUILD_DIR) run

test:
	# echo $(wildcard $(BUILD_DIR)*.o)
	echo $(ASSEMBLIES)
	# echo $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(ASSEMBLIES:.S=.o))
# ---------- Building section ----------
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)$(LIB_DIR)

asm_obj: $(ASSEMBLIES) build
	$(foreach source_s, $(ASSEMBLIES), \
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c $(source_s) \
				-o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(source_s:.S=.o));\
	)

c_obj: $(CFILES) build
	$(foreach source_c, $(CFILES),\
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(source_c) -o $(BUILD_DIR)$(LIB_DIR)$(notdir $(source_c:.c=.o));\
	)
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(SRC_DIR)$(LOADER_ENTRY) -o $(BUILD_DIR)$(LOADER_ENTRY:.c=.o)
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(SRC_DIR)$(KERNEL_ENTRY) -o $(BUILD_DIR)$(KERNEL_ENTRY:.c=.o)

loader: asm_obj c_obj build
	ld.lld -m aarch64elf -T $(SRC_DIR)$(LOADER).ld -o $(BUILD_DIR)$(LOADER).elf \
			$(BUILD_DIR)$(LOADER).o $(BUILD_DIR)$(LOADER_ENTRY:.c=.o)
			# TODO: Other needed package
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(LOADER).elf $(BUILD_DIR)$(LOADER).img

kernel: asm_obj c_obj build
	ld.lld -m aarch64elf -T $(SRC_DIR)$(KERNEL).ld -o $(BUILD_DIR)$(KERNEL).elf \
							$(BUILD_DIR)$(KERNEL).o $(BUILD_DIR)$(KERNEL_ENTRY:.c=.o)\
							$(wildcard $(BUILD_DIR)$(LIB_DIR)*.o)
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(KERNEL).elf $(BUILD_DIR)$(KERNEL).img

# ---------- Debug Section ----------
load: loader
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(LOADER).img -display none -d in_asm $(MINI_UART_FLAG) $(LLDB_FLAG)

run: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG)

debug: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG) $(LLDB_FLAG) 

asm: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL).img -display none -d in_asm $(MINI_UART_FLAG) $(LLDB_FLAG)

# ---------- Debug Section ----------
clean: 
	rm -rf $(BUILD_DIR)
