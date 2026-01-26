CC = gcc
EMCC = emcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags sdl2 SDL2_ttf` 
LDFLAGS = `pkg-config --libs sdl2 SDL2_ttf`  -lm

WASM_BUILD_DIR = build_wasm
WASM_FLAGS = -s EXPORTED_FUNCTIONS='["_nui_init", "_nui_frame_begin", "_nui_frame_end", "_nui_window_begin", "_nui_window_end", "_nui_button", "_nui_input_mouse_move", "_nui_input_mouse_button", "_nui_next_command", "_malloc", "_free"]' \
             -s EXPORTED_RUNTIME_METHODS='["addFunction", "setValue", "ccall", "cwrap", "getValue", "UTF8ToString"]' \
             -s ALLOW_MEMORY_GROWTH=1 \
			 -s ALLOW_TABLE_GROWTH \
             -O3

ifdef PRINT_CMDS
CFLAGS += -DPRINT_CMDS_ONCE
endif

SRC_DIR = src
EXAMPLE_DIR = examples/sdl2
WASM_DIR = examples/wasm
BUILD_DIR = build

EXAMPLE_NAME = $(notdir $(EXAMPLE_DIR))
TARGET = $(BUILD_DIR)/nui_$(EXAMPLE_NAME)
WASM_TARGET = $(WASM_BUILD_DIR)/nui_wasm.js

LIB_SRC = $(SRC_DIR)/nui.c
EXAMPLE_SRC = $(EXAMPLE_DIR)/main.c
WASM_SRC = $(WASM_DIR)/main.c

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

wasm: $(LIB_SRC) $(WASM_SRC)
	@mkdir -p $(WASM_BUILD_DIR)
	$(EMCC) $(LIB_SRC) $(WASM_SRC) -I$(SRC_DIR) -o $(WASM_TARGET) $(WASM_FLAGS)
	@cp $(WASM_DIR)/index.html $(WASM_BUILD_DIR)/index.html
	@echo "WASM build complete. Run 'emrun $(WASM_BUILD_DIR)/index.html' to test."

format:
	clang-format -i $(FORMAT_SOURCES)

clean:
	rm -rf $(BUILD_DIR) $(WASM_BUILD_DIR)

.PHONY: all clean wasm format