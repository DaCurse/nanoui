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
    NUI_CMD_SCISSORS,
} NUI_CommandType;

typedef struct {
    NUI_AABB rect;
    NUI_Color color;
} NUI_CommandRect;

typedef struct {
    const char *text;
    int x, y;
    NUI_Color color;
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
    int cursor_x, cursor_y; // TODO
} NUI_LayoutState;

typedef struct {
    NUI_Color text;

    NUI_Color button_idle;
    NUI_Color button_hot;
    NUI_Color button_active;

} NUI_Style;

extern const NUI_Style nui_default_style;

// User provided font type
typedef void *NUI_UserFont;
typedef void (*NUI_MeasureTextCallback)(NUI_UserFont font, const char *text,
                                        int *out_width, int *out_height);

typedef struct {
    // User provided
    NUI_MeasureTextCallback measure_text;
    NUI_UserFont font;

    // State
    NUI_InputState input;
    NUI_LayoutState layout;
    NUI_Style style;

    NUI_Id hot;
    NUI_Id active;
    NUI_Id last_active;

    NUI_Id id_stack[NUI_ID_STACK_SIZE];
    int id_stack_count;

    NUI_Command commands[NUI_MAX_COMMANDS];
    int command_count;
} NUI_Context;

// Context
void nui_init(NUI_Context *ctx, NUI_MeasureTextCallback measure_text,
              NUI_UserFont font);
void nui_set_style(NUI_Context *ctx, NUI_Style style);

// Input
void nui_input_mouse_move(NUI_Context *ctx, int x, int y);
void nui_input_mouse_button(NUI_Context *ctx, bool down);

// Widgets
void nui_frame_begin(NUI_Context *ctx);
void nui_frame_end(NUI_Context *ctx);
bool nui_button(NUI_Context *ctx, const char *label, NUI_AABB rect);

// Commands
bool nui_next_command(NUI_Context *ctx, NUI_Command *out_cmd);

#endif // NUI_H
