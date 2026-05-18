CC      ?= cc
SRC     := janky.c
TARGET  := janky

WARN    := -Wall -Wextra
CFLAGS  := -std=c11 -ffreestanding -g -O0 $(WARN)

SAN_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all debug asan ubsan release run clean

all: debug

# Default working build: freestanding, no sanitizers.
#
# Sanitizers (asan/ubsan below) are intentionally NOT the default: janky.c
# defines a *global* malloc, so the sanitizer runtimes route their own
# internal allocations through the 2 KB bump allocator and exit(-1) before
# main() produces output. These targets are kept for reference and will
# only work once the allocator is renamed (e.g. j_malloc) or the heap is
# made large enough to satisfy the runtime's allocations.
debug: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

asan: $(SRC)
	$(CC) $(CFLAGS) $(SAN_FLAGS) -o $(TARGET) $(SRC)

ubsan: $(SRC)
	$(CC) $(CFLAGS) -fsanitize=undefined -fno-omit-frame-pointer -o $(TARGET) $(SRC)

# Optimized, no sanitizers
release: $(SRC)
	$(CC) -std=c11 -ffreestanding -O2 -DNDEBUG $(WARN) -o $(TARGET) $(SRC)

run: debug
	./$(TARGET)

clean:
	rm -f $(TARGET) a.out
