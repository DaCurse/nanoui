#include "nui.h"

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

void nui_begin_frame(NUI_Context *ctx) {
    ctx->input.mouse_pressed = ctx->input.mouse_pressed_queued;
    ctx->input.mouse_released = ctx->input.mouse_released_queued;
    ctx->input.mouse_pressed_queued = false;
    ctx->input.mouse_released_queued = false;

    ctx->command_count = 0;
    ctx->hot = 0;

    ctx->layout.cursor_x = 0;
    ctx->layout.cursor_y = 0;
}

void nui_end_frame(NUI_Context *ctx) {
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

    bool hovered =
        nui_aabb_contains(rect, ctx->input.mouse_x, ctx->input.mouse_y);
    if (hovered && (ctx->active == 0 || ctx->active == id)) {
        ctx->hot = id;
        if (ctx->input.mouse_pressed) {
            ctx->active = id;
        }
    }

    bool triggered = ctx->input.mouse_released && ctx->active == id && hovered;

    NUI_Color color = {0x44, 0x44, 0x44, 0xFF};
    if (ctx->active == id) {
        color = (NUI_Color){0x22, 0x22, 0x22, 0xFF};
    } else if (ctx->hot == id) {
        color = (NUI_Color){0x66, 0x66, 0x66, 0xFF};
    }

    if (ctx->command_count < NUI_MAX_COMMANDS) {
        NUI_Command *cmd = &ctx->commands[ctx->command_count++];
        cmd->type = NUI_CMD_RECT;
        cmd->rect.rect = rect;
        cmd->rect.color = color;
    }

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
