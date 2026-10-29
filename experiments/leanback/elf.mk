DEVKITXENON ?= /workspace/setup/devkit
.DEFAULT_GOAL := all
XENONFIN ?= /workspace/XenonFin
include $(DEVKITXENON)/rules
CC := $(DEVKITXENON)/bin/xenon-gcc
OBJCOPY := $(DEVKITXENON)/bin/xenon-objcopy
OUT := build/elf
TARGET := $(OUT)/youtube-leanback-diagnostic
NATIVE := $(XENONFIN)/platform/xenon
MBEDTLS ?= /workspace/setup/mbedtls-3.6.7
CFLAGS := -std=gnu99 -O2 -g -Wall -Wextra -Werror $(MACHDEP) -I$(LIBXENON_INC) -I$(NATIVE) -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/main.o $(addprefix $(OUT)/,console_batch.o console_frame.o framebuffer.o protocol.o json.o)
OBJECTS += $(addprefix $(OUT)/,native_net.o driver_guard.o init_guard.o http.o)
OBJECTS += $(addprefix $(OUT)/net-,probe.o platform.o dns_wire.o)
TLS_OBJECTS := $(patsubst $(MBEDTLS)/library/%.c,$(OUT)/mbed/%.o,$(wildcard $(MBEDTLS)/library/*.c))
CFLAGS += -I$(MBEDTLS)/include -I$(CURDIR)/net -DMBEDTLS_CONFIG_FILE='"tls_config.h"'
$(OBJECTS) $(TLS_OBJECTS): elf.mk
.PHONY: all clean
all: $(TARGET).elf
$(OUT):
	mkdir -p $@
$(OUT)/main.o: elf/main.c input.h | $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/net-%.o: net/%.c | $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/mbed/%.o: $(MBEDTLS)/library/%.c net/tls_config.h | $(OUT)
	mkdir -p $(OUT)/mbed
	$(CC) $(filter-out -Werror,$(CFLAGS)) -MMD -MP -c $< -o $@
$(OUT)/%.o: $(NATIVE)/%.c | $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(TARGET).linked: $(OBJECTS) $(TLS_OBJECTS) elf.mk
	$(CC) $(OBJECTS) $(TLS_OBJECTS) $(MACHDEP) -Wl,--gc-sections -Wl,--wrap=mdelay -T $(LDSCRIPT) -L$(LIBXENON_LIB) -Wl,-Map,$@.map -lfat -lxenon -lm -o $@
$(TARGET).elf: $(TARGET).linked
	$(OBJCOPY) -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	$(DEVKITXENON)/bin/xenon-strip $@
clean:
	rm -rf $(OUT)
-include $(OBJECTS:.o=.d) $(TLS_OBJECTS:.o=.d)
