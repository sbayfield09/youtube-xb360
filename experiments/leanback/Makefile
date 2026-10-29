XENONFIN ?= /workspace/XenonFin
CC ?= cc
CFLAGS := -std=c99 -O2 -Wall -Wextra -Werror
.PHONY: all test clean
all: build/native-gate build/input-test
build:
	mkdir -p $@
build/native-gate: native_gate.c $(XENONFIN)/platform/xenon/protocol.c $(XENONFIN)/platform/xenon/json.c | build
	$(CC) $(CFLAGS) -I$(XENONFIN)/platform/xenon $^ -o $@
build/input-test: input_test.c input.h | build
	$(CC) $(CFLAGS) input_test.c -o $@
test: all
	./build/input-test
	python3 -c 'import subprocess; r=subprocess.run(["./build/native-gate"]); assert r.returncode == 2, r.returncode'
clean:
	rm -rf build
