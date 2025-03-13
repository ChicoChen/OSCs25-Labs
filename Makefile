BUILD_DIR = ./build
INCLUDE_DIR = ./include
SRC_DIR = ./src
LIB_DIR = ./lib

CFILES = $(wildcard $(LIB_DIR)/*.c) $(wildcard $(SRC_DIR)/*.c)
ASSEMBLYS = $(wildcard $(SRC_DIR)/*.S)
OBJECTS = $(CFILES:.c=.o) $(ASSEMBLYS:.S=.o)

COMPILE_FLAG = -nostdlib -g
LOADER = loader
KERNEL = kernel8
LLDB_FLAG = -s -S
MINI_UART_FLAG = -serial null -serial stdio

.PHONY: all
all: $(BUILD_DIR) run

test:
	echo $(notdir $(OBJECTS))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	
%.o: %.S | $(BUILD_DIR)
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c $< -o $(BUILD_DIR)/$(notdir $@)

%.o: %.c | $(BUILD_DIR)
	clang --target=aarch64-raspi3-elf -mcpu=cortex-a53 $(COMPILE_FLAG) -c -I $(INCLUDE_DIR) $< -o $(BUILD_DIR)/$(notdir $@)

loader: $(LOADER).o | $(BUILD_DIR)
	ld.lld -m aarch64elf -T loader.ld -o $(BUILD_DIR)/$(LOADER).elf $(BUILD_DIR)/$<
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)/$(LOADER).elf $(BUILD_DIR)/$(LOADER).img

kernel: $(OBJECTS) | $(BUILD_DIR)
	ld.lld -m aarch64elf -T kernel.ld -o $(BUILD_DIR)/$(KERNEL).elf $(addprefix $(BUILD_DIR)/, $(notdir $(OBJECTS)))
	llvm-objcopy --output-target=aarch64-rpi3-elf -g -O binary $(BUILD_DIR)/$(KERNEL).elf $(BUILD_DIR)/$(KERNEL).img

run: kernel loader
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$(KERNEL).img -display none $(MINI_UART_FLAG)

debug: kernel loader
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$(KERNEL).img -display none $(MINI_UART_FLAG) $(LLDB_FLAG) 

asm: kernel loader
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$(KERNEL).img -display none -d in_asm $(MINI_UART_FLAG) $(LLDB_FLAG) 

clean: 
	rm -rf $(BUILD_DIR)
