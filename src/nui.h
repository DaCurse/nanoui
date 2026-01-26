#ifndef NUI_H
#define NUI_H

#include <stdbool.h>
#include <stdint.h>

#define NUI_MAX_COMMANDS (1024)
#define NUI_ID_STACK_SIZE (16)

typedef uint32_t NUI_Id;

typedef struct {
    int x, y, w, h;
} NUI_AABB;

typedef struct {
    unsigned char r, g, b, a;
} NUI_Color;

typedef enum {
    NUI_CMD_RECT,
    NUI_CMD_TEXT,
    NUI_CMD_SCISSOR,
} NUI_CommandType;

typedef struct {
    NUI_AABB rect;
    NUI_Color color;
} NUI_CommandRect;

typedef struct {
    const char *text;
    int x, y;
} NUI_CommandText;

typedef struct {
    bool clear;
    NUI_AABB area; // set if clear == false
} NUI_CommandScissor;

typedef struct {
    NUI_CommandType type;
    union {
        NUI_CommandRect rect;
        NUI_CommandText text;
        NUI_CommandScissor scissor;
    };
} NUI_Command;

typedef struct {
    int mouse_x, mouse_y;
    bool mouse_down;

    bool mouse_pressed;
    bool mouse_released;

    bool mouse_pressed_queued;
    bool mouse_released_queued;
} NUI_InputState;

typedef struct {
    int cursor_x, cursor_y;
    int item_spacing;
} NUI_LayoutState;

typedef struct {
    NUI_InputState input;
    NUI_LayoutState layout;

    NUI_Id hot;
    NUI_Id active;
    NUI_Id last_active;

    NUI_Id id_stack[NUI_ID_STACK_SIZE];
    int id_stack_count;

    NUI_Command commands[NUI_MAX_COMMANDS];
    int command_count;
} NUI_Context;

// Input
void nui_input_mouse_move(NUI_Context *ctx, int x, int y);
void nui_input_mouse_button(NUI_Context *ctx, bool down);

// Widgets
void nui_begin_frame(NUI_Context *ctx);
void nui_end_frame(NUI_Context *ctx);
bool nui_button(NUI_Context *ctx, const char *label, NUI_AABB rect);

// Commands
bool nui_next_command(NUI_Context *ctx, NUI_Command *out_cmd);

#endif // NUI_H
