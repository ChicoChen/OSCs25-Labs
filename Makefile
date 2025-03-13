INCLUDE_DIR = ./include/
LIB_DIR = ./lib/
SRC_DIR = ./src/
BUILD_DIR = ./build/

LOADER = loader
KERNEL = kernel8

KERNEL_DIR = $(KERNEL)/
LOADER_DIR = $(LOADER)/

COMPILE_FLAG = -nostdlib -g
LLDB_FLAG = -s -S
MINI_UART_FLAG = -serial null -serial stdio

ASSEMBLIES = $(SRC_DIR)$(KERNEL_DIR)$(KERNEL).S $(SRC_DIR)$(LOADER_DIR)$(LOADER).S
LOADER_ENTRY = $(SRC_DIR)$(LOADER_DIR)$(LOADER)_entry.c
KERNEL_ENTRY = $(SRC_DIR)$(KERNEL_DIR)$(KERNEL)_entry.c
CFILES = $(wildcard $(LIB_DIR)*.c)

# KERNEL_O = $(patsubst %, $(BUILD_DIR)/%, $(notdir $(ASSEMBLIES:.S=.o)))
OBJECTS = $(CFILES:.c=.o)


.PHONY: all
all: $(BUILD_DIR) run

test:
	echo $(wildcard $(BUILD_DIR)*.o)
	# echo $(CFILES)
	# echo $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(ASSEMBLIES:.S=.o))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)$(LOADER_DIR)
	mkdir -p $(BUILD_DIR)$(KERNEL_DIR)

# kernel_o: $(ASSEMBLIES) | $(BUILD_DIR)
# 	$(foreach source_s, $(ASSEMBLIES), \
# 		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c $(source_s) -o $(BUILD_DIR)/$(notdir $(source_s:.s=.o));\
# 	)

asm_obj: $(ASSEMBLIES) | $(BUILD_DIR)
	$(foreach source_s, $(ASSEMBLIES), \
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c $(source_s) \
				-o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(source_s:.S=.o));\
	)

c_obj: $(CFILES)| $(BUILD_DIR)
	$(foreach source_c, $(CFILES),\
		clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(source_c) -o $(BUILD_DIR)$(notdir $(source_c:.c=.o));\
	)
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(KERNEL_ENTRY) -o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(KERNEL_ENTRY:.c=.o))
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $(LOADER_ENTRY) -o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(LOADER_ENTRY:.c=.o))

loader: asm_obj c_obj | $(BUILD_DIR)
	ld.lld -m aarch64elf -T $(SRC_DIR)$(LOADER_DIR)$(LOADER).ld -o $(BUILD_DIR)$(LOADER_DIR)/$(LOADER).elf \
			$(BUILD_DIR)$(LOADER_DIR)$(LOADER).o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(KERNEL_ENTRY:.c=.o))\
			$(wildcard $(BUILD_DIR)*.o)
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(LOADER_DIR)$(LOADER).elf $(BUILD_DIR)$(LOADER_DIR)$(LOADER).img

kernel: asm_obj c_obj | $(BUILD_DIR)
	ld.lld -m aarch64elf -T $(SRC_DIR)$(KERNEL_DIR)$(KERNEL).ld -o $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).elf \
							$(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).o $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(KERNEL_ENTRY:.c=.o))\
							$(wildcard $(BUILD_DIR)*.o)
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).elf $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).img

load: loader
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(LOADER_DIR)$(LOADER).img -display none $(MINI_UART_FLAG) $(LLDB_FLAG)

run: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG)

debug: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).img -display none $(MINI_UART_FLAG) $(LLDB_FLAG) 

asm: kernel
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)$(KERNEL_DIR)$(KERNEL).img -display none -d in_asm $(MINI_UART_FLAG) $(LLDB_FLAG)

clean: 
	rm -rf $(BUILD_DIR)
