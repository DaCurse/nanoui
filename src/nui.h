#ifndef NUI_H
#define NUI_H

#include <stdbool.h>
#include <stdint.h>

#define NUI_MAX_COMMANDS (1024)
#define NUI_LAYOUT_STACK_SIZE (32)
#define NUI_SCISSORS_STACK_SIZE (32)
#define NUI_CONTAINER_LIST_SIZE (32)

typedef uint32_t NUI_Id;

typedef struct {
    int x, y, w, h;
} NUI_AABB;

typedef struct {
    unsigned char r, g, b, a;
} NUI_Color;

// A container has a retained area and z-index
typedef struct {
    NUI_Id id;
    NUI_AABB area;
    int z_index;

    // Command slice within the global command buffer
    int command_start_index;
    int command_count;
} NUI_Container;

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
    NUI_AABB area;
} NUI_CommandScissors;

typedef struct {
    NUI_CommandType type;
    union {
        NUI_CommandRect rect;
        NUI_CommandText text;
        NUI_CommandScissors scissors;
    };
} NUI_Command;

typedef struct {
    int mouse_x, mouse_y;
    int drag_offset_x, drag_offset_y;
    bool mouse_down;

    bool mouse_pressed;
    bool mouse_released;

    bool mouse_pressed_queued;
    bool mouse_released_queued;
} NUI_InputState;

typedef enum {
    NUI_LAYOUT_VERTICAL,
    NUI_LAYOUT_HORIZONTAL,
} NUI_LayoutMode;

typedef struct {
    int cursor_x, cursor_y;
    int start_x, start_y;
    int size_x, size_y;
    int row_height;
    int width;
    int margin;
    NUI_LayoutMode mode;
} NUI_Layout;

typedef struct {
    NUI_Color text;

    NUI_Color window_bg;
    NUI_Color window_title_bar;

    NUI_Color button_idle;
    NUI_Color button_hot;
    NUI_Color button_active;

    NUI_Color border;
    int border_radius;

    int padding_x, padding_y;
    int margin;

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
    NUI_Style style;

    // Layout
    NUI_Layout layout;
    NUI_Layout layout_stack[NUI_LAYOUT_STACK_SIZE];
    int layout_stack_top;

    // Scissors
    NUI_AABB scissors_stack[NUI_SCISSORS_STACK_SIZE];
    NUI_AABB current_scissors;
    int scissors_stack_top;

    // Container list
    NUI_Container container_list[NUI_CONTAINER_LIST_SIZE];
    int container_count;
    // Topmost hovered container
    NUI_Id hover_container_id;
    int last_z_index;

    // The active container currently being drawn to
    NUI_Container *current_container;
    // Command iterator State
    NUI_Container *sorted_containers[NUI_CONTAINER_LIST_SIZE];
    int sorted_count;
    // The index of the current container being iterated when draining commands
    int iter_container_index;
    // The offset of the current command within the current container being
    // iterated
    int iter_cmd_offset;

    // Interaction state
    NUI_Id hot;
    NUI_Id active;
    NUI_Id last_active;

    // Command Buffer
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
void nui_scissors_push(NUI_Context *ctx, NUI_AABB area);
void nui_scissors_pop(NUI_Context *ctx);
void nui_layout_push(NUI_Context *ctx, NUI_AABB area, NUI_LayoutMode mode);
void nui_layout_pop(NUI_Context *ctx);
bool nui_window_begin(NUI_Context *ctx, const char *title, NUI_AABB area);
void nui_window_end(NUI_Context *ctx);
bool nui_button(NUI_Context *ctx, const char *label);

// Commands
bool nui_next_command(NUI_Context *ctx, NUI_Command *out_cmd);

#endif // NUI_H
