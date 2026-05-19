CC      ?= cc
AR      ?= ar

SRC      := janky.c          # the allocator, on its own
DEMO_SRC := demo.c           # throwaway driver (main)
LIB      := libjanky.a       # allocator as a linkable static library
TARGET   := janky            # the demo executable

WARN    := -Wall -Wextra
CFLAGS  := -std=c11 -ffreestanding -g -O0 $(WARN)

SAN_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

UNITY_DIR := vendor/unity
TEST_SRC  := tests/test_janky.c $(UNITY_DIR)/unity.c
TEST_BIN  := test_janky
# Rename the global allocator so Unity/libc keep using the real malloc while
# the tests drive j_malloc/j_free/j_realloc. -ffreestanding is dropped here
# because the test harness links against the hosted C library.
TEST_DEFS := -DJANKY_TEST \
             -Dmalloc=j_malloc -Dfree=j_free -Drealloc=j_realloc

.PHONY: all debug asan ubsan release lib run test clean

all: debug

# --- the allocator, as a standalone static library ----------------------
# Link this into any program to use janky as its malloc/free/realloc:
#   cc myprog.c libjanky.a -o myprog
lib: $(LIB)

$(LIB): $(SRC) janky.h
	$(CC) $(CFLAGS) -c -o janky.o $(SRC)
	$(AR) rcs $(LIB) janky.o

# --- the demo (allocator + driver) --------------------------------------
#
# Sanitizers (asan/ubsan below) are intentionally NOT the default: janky.c
# defines a *global* malloc, so the sanitizer runtimes route their own
# internal allocations through the 4 KB allocator and exit(-1) before
# main() produces output. These targets are kept for reference and will
# only work once the allocator is renamed (e.g. j_malloc) or the heap is
# made large enough to satisfy the runtime's allocations.
debug: $(SRC) $(DEMO_SRC) janky.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(DEMO_SRC)

asan: $(SRC) $(DEMO_SRC) janky.h
	$(CC) $(CFLAGS) $(SAN_FLAGS) -o $(TARGET) $(SRC) $(DEMO_SRC)

ubsan: $(SRC) $(DEMO_SRC) janky.h
	$(CC) $(CFLAGS) -fsanitize=undefined -fno-omit-frame-pointer -o $(TARGET) $(SRC) $(DEMO_SRC)

# Optimized, no sanitizers
release: $(SRC) $(DEMO_SRC) janky.h
	$(CC) -std=c11 -ffreestanding -O2 -DNDEBUG $(WARN) -o $(TARGET) $(SRC) $(DEMO_SRC)

run: debug
	./$(TARGET)

test: $(SRC) $(TEST_SRC) janky.h
	$(CC) -std=c11 -g -O0 $(WARN) $(TEST_DEFS) -I$(UNITY_DIR) \
		-o $(TEST_BIN) $(SRC) $(TEST_SRC)
	./$(TEST_BIN)

clean:
	rm -f $(TARGET) $(TEST_BIN) $(LIB) janky.o a.out
