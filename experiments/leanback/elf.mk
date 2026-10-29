DEVKITXENON ?= /workspace/setup/devkit
XENONFIN ?= /workspace/XenonFin
include $(DEVKITXENON)/rules
CC := $(DEVKITXENON)/bin/xenon-gcc
OBJCOPY := $(DEVKITXENON)/bin/xenon-objcopy
OUT := build/elf
TARGET := $(OUT)/youtube-leanback-diagnostic
NATIVE := $(XENONFIN)/platform/xenon
CFLAGS := -std=gnu99 -O2 -g -Wall -Wextra -Werror $(MACHDEP) -I$(LIBXENON_INC) -I$(NATIVE) -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/main.o $(addprefix $(OUT)/,console_batch.o console_frame.o framebuffer.o protocol.o json.o)
.PHONY: all clean
all: $(TARGET).elf
$(OUT):
	mkdir -p $@
$(OUT)/main.o: elf/main.c input.h | $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/%.o: $(NATIVE)/%.c | $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(TARGET).linked: $(OBJECTS) elf.mk
	$(CC) $(OBJECTS) $(MACHDEP) -Wl,--gc-sections -T $(LDSCRIPT) -L$(LIBXENON_LIB) -Wl,-Map,$@.map -lxenon -lm -o $@
$(TARGET).elf: $(TARGET).linked
	$(OBJCOPY) -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	$(DEVKITXENON)/bin/xenon-strip $@
clean:
	rm -rf $(OUT)
-include $(OBJECTS:.o=.d)
