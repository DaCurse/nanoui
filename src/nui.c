#include "nui.h"

#include <assert.h>

#define NUI_NEXT_COMMAND_SAFE(ctx)                                             \
    ((ctx->command_count < NUI_MAX_COMMANDS)                                   \
         ? (&ctx->commands[ctx->command_count++])                              \
         : (assert(0 && "NUI Command buffer overflow"), (NUI_Command *)0))

const NUI_Style nui_default_style = {
    .text = {0xFF, 0xFF, 0xFF, 0xFF},

    .button_idle = {0x44, 0x44, 0x44, 0xFF},
    .button_hot = {0x66, 0x66, 0x66, 0xFF},
    .button_active = {0x22, 0x22, 0x22, 0xFF},
};
// FNV-1a hash
static NUI_Id nui_hash(const char *str, NUI_Id seed) {
    NUI_Id hash = seed ? seed : 2166136261u; // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

static bool nui_aabb_contains(NUI_AABB aabb, int x, int y) {
    return (x >= aabb.x) && (x < (aabb.x + aabb.w)) && (y >= aabb.y) &&
           (y < (aabb.y + aabb.h));
}

static inline void nui_push_command_rect(NUI_Context *ctx, NUI_AABB rect,
                                         NUI_Color color) {
    NUI_Command *cmd = NUI_NEXT_COMMAND_SAFE(ctx);
    if (!cmd)
        return;

    cmd->type = NUI_CMD_RECT;
    cmd->rect.rect = rect;
    cmd->rect.color = color;
}

static inline void nui_push_command_text(NUI_Context *ctx, const char *text,
                                         int x, int y, NUI_Color color) {
    NUI_Command *cmd = NUI_NEXT_COMMAND_SAFE(ctx);
    if (!cmd)
        return;

    cmd->type = NUI_CMD_TEXT;
    cmd->text.text = text;
    cmd->text.x = x;
    cmd->text.y = y;
    cmd->text.color = color;
}

static inline void nui_push_command_scissors(NUI_Context *ctx, bool clear,
                                             NUI_AABB rect) {
    NUI_Command *cmd = NUI_NEXT_COMMAND_SAFE(ctx);
    if (!cmd)
        return;

    cmd->type = NUI_CMD_SCISSORS;
    cmd->scissor.clear = clear;
    cmd->scissor.area = rect;
}

void nui_init(NUI_Context *ctx, NUI_MeasureTextCallback measure_text,
              NUI_UserFont font) {
    ctx->measure_text = measure_text;
    ctx->font = font;
    ctx->style = nui_default_style;
}

void nui_set_style(NUI_Context *ctx, NUI_Style style) { ctx->style = style; }

void nui_frame_begin(NUI_Context *ctx) {
    ctx->input.mouse_pressed = ctx->input.mouse_pressed_queued;
    ctx->input.mouse_released = ctx->input.mouse_released_queued;
    ctx->input.mouse_pressed_queued = false;
    ctx->input.mouse_released_queued = false;

    ctx->command_count = 0;
    ctx->hot = 0;

    ctx->layout.cursor_x = 0;
    ctx->layout.cursor_y = 0;
}

void nui_frame_end(NUI_Context *ctx) {
    if (ctx->input.mouse_released) {
        ctx->active = 0;
    }
}

void nui_input_mouse_move(NUI_Context *ctx, int x, int y) {
    ctx->input.mouse_x = x;
    ctx->input.mouse_y = y;
}

void nui_input_mouse_button(NUI_Context *ctx, bool down) {
    ctx->input.mouse_down = down;
    if (down) {
        ctx->input.mouse_pressed_queued = true;
    } else {
        ctx->input.mouse_released_queued = true;
    }
}

bool nui_button(NUI_Context *ctx, const char *label, NUI_AABB rect) {
    NUI_Id id = nui_hash(label, 0);

    // Interaction
    bool hovered =
        nui_aabb_contains(rect, ctx->input.mouse_x, ctx->input.mouse_y);
    if (hovered && (ctx->active == 0 || ctx->active == id)) {
        ctx->hot = id;
        if (ctx->input.mouse_pressed) {
            ctx->active = id;
        }
    }
    bool triggered = ctx->input.mouse_released && ctx->active == id && hovered;

    // Rendering
    NUI_Color color = ctx->style.button_idle;
    if (ctx->active == id) {
        color = ctx->style.button_active;
    } else if (ctx->hot == id) {
        color = ctx->style.button_hot;
    }
    nui_push_command_rect(ctx, rect, color);
    nui_push_command_scissors(ctx, false, rect);

    int text_w, text_h;
    ctx->measure_text(ctx->font, label, &text_w, &text_h);
    nui_push_command_text(ctx, label, rect.x + (rect.w - text_w) / 2,
                          rect.y + (rect.h - text_h) / 2, ctx->style.text);
    nui_push_command_scissors(ctx, true, (NUI_AABB){0});

    return triggered;
}

bool nui_next_command(NUI_Context *ctx, NUI_Command *out_cmd) {
    if (ctx->command_count == 0) {
        return false;
    }

    *out_cmd = ctx->commands[0];
    for (int i = 1; i < ctx->command_count; i++) {
        ctx->commands[i - 1] = ctx->commands[i];
    }

    ctx->command_count--;
    return true;
}
