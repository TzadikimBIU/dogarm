CC ?= gcc
AR ?= ar

BUILD_DIR ?= build
SRC_DIR := src
TEST_DIR := tests
PIC_DIR := $(BUILD_DIR)/pic

CPPFLAGS ?= -I. -I$(SRC_DIR)
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=
LDLIBS ?=
DEPFLAGS ?= -MMD -MP

LIB_SRCS := $(SRC_DIR)/operand.c $(SRC_DIR)/decode.c $(SRC_DIR)/disasm.c $(SRC_DIR)/dogarm.c
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
LIB_PIC_OBJS := $(patsubst $(SRC_DIR)/%.c,$(PIC_DIR)/%.o,$(LIB_SRCS))
DEPS := $(LIB_OBJS:.o=.d) $(LIB_PIC_OBJS:.o=.d)

.PHONY: all lib tools example test test-diff check sanitize ci clean

all: lib tools example

lib: $(BUILD_DIR)/libdogarm.a $(BUILD_DIR)/libdogarm.so

tools: $(BUILD_DIR)/dogarm-disasm

example: $(BUILD_DIR)/example

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PIC_DIR):
	mkdir -p $(PIC_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(PIC_DIR)/%.o: $(SRC_DIR)/%.c | $(PIC_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/libdogarm.a: $(LIB_OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/libdogarm.so: $(LIB_PIC_OBJS)
	$(CC) -shared $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/example: example/main.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/dogarm-disasm: tools/dogarm-disasm.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_decode: $(TEST_DIR)/test_decode.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

test: $(BUILD_DIR)/test_decode test-diff
	./$(BUILD_DIR)/test_decode

test-diff: $(BUILD_DIR)/dogarm-disasm
	sh $(TEST_DIR)/diff_objdump.sh ./$(BUILD_DIR)/dogarm-disasm

check: all test

sanitize:
	$(MAKE) clean
	ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 $(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined" check

ci:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -Wconversion -Werror -O2" check
	$(MAKE) sanitize

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
