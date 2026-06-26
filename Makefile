CC ?= gcc
AR ?= ar

BUILD_DIR ?= build
SRC_DIR := src
TEST_DIR := tests

CPPFLAGS ?= -I. -I$(SRC_DIR)
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=
LDLIBS ?=

LIB_SRCS := $(SRC_DIR)/operand.c $(SRC_DIR)/decode.c $(SRC_DIR)/disasm.c $(SRC_DIR)/dogarm.c
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))

.PHONY: all lib tools example test check ci clean

all: lib tools example

lib: $(BUILD_DIR)/libdogarm.a $(BUILD_DIR)/libdogarm.so

tools: $(BUILD_DIR)/dogarm-disasm

example: $(BUILD_DIR)/example

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libdogarm.a: $(LIB_OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/libdogarm.so: $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $^ -o $@

$(BUILD_DIR)/example: example/main.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/dogarm-disasm: tools/dogarm-disasm.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_decode: $(TEST_DIR)/test_decode.c $(BUILD_DIR)/libdogarm.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD_DIR)/libdogarm.a $(LDFLAGS) $(LDLIBS) -o $@

test: $(BUILD_DIR)/test_decode
	./$(BUILD_DIR)/test_decode

check: all test

ci:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -Wconversion -Werror -O2" check

clean:
	rm -rf $(BUILD_DIR)
