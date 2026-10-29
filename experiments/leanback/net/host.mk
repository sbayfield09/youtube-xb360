MBEDTLS ?= /workspace/setup/mbedtls-3.6.7
XENONFIN ?= /workspace/XenonFin
OUT := build/host-tls
CC := cc
CONFIG := -DMBEDTLS_CONFIG_FILE='"tls_config.h"'
FLAGS := -std=c99 -O1 -g -Wall -Wextra -I$(CURDIR)/net -I$(MBEDTLS)/include $(CONFIG) -ffunction-sections -fdata-sections
TLS := $(patsubst $(MBEDTLS)/library/%.c,$(OUT)/%.o,$(wildcard $(MBEDTLS)/library/*.c))
LIB := $(OUT)/tls.a
.PHONY: all
all: $(LIB)
$(OUT):
	mkdir -p $@
$(OUT)/%.o: $(MBEDTLS)/library/%.c net/tls_config.h | $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(LIB): $(TLS)
	ar rcs $@ $^
