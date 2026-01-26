CC = gcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags sdl2`
LDFLAGS = `pkg-config --libs sdl2` -lm

SRC_DIR = src
EXAMPLE_DIR = examples/sdl2
BUILD_DIR = build

EXAMPLE_NAME = $(notdir $(EXAMPLE_DIR))
TARGET = $(BUILD_DIR)/nui_$(EXAMPLE_NAME)

LIB_SRC = $(SRC_DIR)/nui.c
EXAMPLE_SRC = $(EXAMPLE_DIR)/main.c
OBJS = $(BUILD_DIR)/nui.o $(BUILD_DIR)/main.o

FORMAT_SOURCES = $(shell find . -name "*.c" -o -name "*.h")

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/nui.o: $(LIB_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(EXAMPLE_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

format:
	clang-format -i $(FORMAT_SOURCES)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean