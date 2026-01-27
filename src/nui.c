#include "nui.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define NUI_NEXT_COMMAND_SAFE(ctx)                                             \
    ((ctx->command_count < NUI_MAX_COMMANDS)                                   \
         ? (&ctx->commands[ctx->command_count++])                              \
         : (assert(0 && "NUI Command buffer overflow"), (NUI_Command *)0))

// Scissors covering the entire possible area
static const NUI_AABB NUI_ROOT_SCISSORS =
    (NUI_AABB){0, 0, 0x10000000, 0x10000000};

const NUI_Style nui_default_style = {
    .text = {0xFF, 0xFF, 0xFF, 0xFF},

    .window_bg = {0x33, 0x33, 0x33, 0xFF},
    .window_title_bar = {0x11, 0x11, 0x11, 0xFF},

    .button_idle = {0x44, 0x44, 0x44, 0xFF},
    .button_hot = {0x66, 0x66, 0x66, 0xFF},
    .button_active = {0x22, 0x22, 0x22, 0xFF},

    .border = {0x00, 0x00, 0x00, 0xFF},
    .border_radius = 1,

    .padding_x = 12,
    .padding_y = 8,
    .margin = 10,
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

static inline bool nui_aabb_contains(NUI_AABB aabb, int x, int y) {
    return (x >= aabb.x) && (x < aabb.x + aabb.w) && (y >= aabb.y) &&
           (y < aabb.y + aabb.h);
}

static inline NUI_AABB nui_aabb_intersects(NUI_AABB a, NUI_AABB b) {
    int x1 = MAX(a.x, b.x);
    int y1 = MAX(a.y, b.y);
    int x2 = MIN(a.x + a.w, b.x + b.w);
    int y2 = MIN(a.y + a.h, b.y + b.h);

    if (x2 <= x1 || y2 <= y1) {
        return (NUI_AABB){0, 0, 0, 0};
    }

    return (NUI_AABB){
        .x = x1,
        .y = y1,
        .w = x2 - x1,
        .h = y2 - y1,
    };
}

static inline bool nui_aabb_overlaps(NUI_AABB a, NUI_AABB b) {
    return (a.x < b.x + b.w) && (a.x + a.w > b.x) && (a.y < b.y + b.h) &&
           (a.y + a.h > b.y);
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

static inline void nui_push_command_scissors(NUI_Context *ctx, NUI_AABB rect) {
    NUI_Command *cmd = NUI_NEXT_COMMAND_SAFE(ctx);
    if (!cmd)
        return;

    cmd->type = NUI_CMD_SCISSORS;
    cmd->scissors.area = rect;
}

static int nui_compare_containers(const void *a, const void *b) {
    const NUI_Container *ca = *(const NUI_Container **)a;
    const NUI_Container *cb = *(const NUI_Container **)b;

    // Lower Z-index first, higher last
    if (ca->z_index < cb->z_index)
        return -1;
    if (ca->z_index > cb->z_index)
        return 1;
    return 0;
}

static inline void nui_bring_to_front(NUI_Context *ctx,
                                      NUI_Container *container) {
    container->z_index = ++ctx->last_z_index;
}

static NUI_Container *nui_get_container(NUI_Context *ctx, NUI_Id id) {
    // Search for existing container
    for (int i = 0; i < ctx->container_count; i++) {
        if (ctx->container_list[i].id == id) {
            return &ctx->container_list[i];
        }
    }

    // Create new container
    assert(ctx->container_count < NUI_CONTAINER_LIST_SIZE &&
           "container list overflow");

    NUI_Container *container = &ctx->container_list[ctx->container_count++];
    memset(container, 0, sizeof(*container));
    container->id = id;
    nui_bring_to_front(ctx, container);

    return container;
}

void nui_init(NUI_Context *ctx, NUI_MeasureTextCallback measure_text,
              NUI_UserFont font) {
    assert(measure_text && "measure_text must not be NULL");
    ctx->measure_text = measure_text;
    ctx->font = font;
    ctx->style = nui_default_style;
}

void nui_set_style(NUI_Context *ctx, NUI_Style style) { ctx->style = style; }

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

void nui_frame_begin(NUI_Context *ctx) {
    ctx->input.mouse_pressed = ctx->input.mouse_pressed_queued;
    ctx->input.mouse_released = ctx->input.mouse_released_queued;
    ctx->input.mouse_pressed_queued = false;
    ctx->input.mouse_released_queued = false;

    // Reset render state
    ctx->command_count = 0;

    ctx->hot = 0;

    ctx->scissors_stack_top = 0;
    ctx->current_scissors = NUI_ROOT_SCISSORS;

    ctx->layout_stack_top = 0;
    memset(&ctx->layout, 0, sizeof(ctx->layout));

    // Find hovered container
    ctx->hover_container_id = 0;
    int best_z = -1;
    for (int i = 0; i < ctx->container_count; i++) {
        NUI_Container *c = &ctx->container_list[i];
        if (c->command_count > 0 &&
            nui_aabb_contains(c->area, ctx->input.mouse_x,
                              ctx->input.mouse_y)) {
            if (c->z_index > best_z) {
                best_z = c->z_index;
                ctx->hover_container_id = c->id;
            }
        }

        // Reset command count from last frame
        c->command_count = 0;
    }
}

void nui_frame_end(NUI_Context *ctx) {
    if (ctx->input.mouse_released) {
        ctx->active = 0;
    }

    // Sort containers by Z-index for rendering
    ctx->sorted_count = 0;
    for (int i = 0; i < ctx->container_count; i++) {
        if (ctx->container_list[i].command_count > 0) {
            ctx->sorted_containers[ctx->sorted_count++] =
                &ctx->container_list[i];
        }
    }
    qsort(ctx->sorted_containers, ctx->sorted_count,
          sizeof(*ctx->sorted_containers), nui_compare_containers);

    // Reset command draining iteration state
    ctx->iter_container_index = 0;
    ctx->iter_cmd_offset = 0;
}

void nui_scissors_push(NUI_Context *ctx, NUI_AABB area) {
    assert(ctx->scissors_stack_top < NUI_SCISSORS_STACK_SIZE &&
           "scissors stack overflow");

    // Push current scissors onto stack
    ctx->scissors_stack[ctx->scissors_stack_top++] = ctx->current_scissors;
    // Intersect with new area and set as current scissors
    ctx->current_scissors = nui_aabb_intersects(ctx->current_scissors, area);
    nui_push_command_scissors(ctx, ctx->current_scissors);
}

void nui_scissors_pop(NUI_Context *ctx) {
    assert(ctx->scissors_stack_top > 0 && "scissors stack underflow");

    ctx->current_scissors = ctx->scissors_stack[--ctx->scissors_stack_top];
    nui_push_command_scissors(ctx, ctx->current_scissors);
}

static NUI_AABB nui_layout_allocate(NUI_Context *ctx, int w, int h) {
    NUI_Layout *layout = &ctx->layout;

    // Handle wrapping for horizontal layout if exceeding available width
    if (layout->mode == NUI_LAYOUT_HORIZONTAL) {
        int right_edge = layout->cursor_x + w;
        int max_edge = layout->start_x + layout->width;

        if (right_edge > max_edge) {
            layout->cursor_x = layout->start_x;
            layout->cursor_y += layout->row_height + layout->margin;
            layout->row_height = 0;
        }
    }

    // Compute the rectangle to allocate
    NUI_AABB rect = {layout->cursor_x, layout->cursor_y, w, h};

    // Track the tallest item in the current row for horizontal layout
    if (h > layout->row_height)
        layout->row_height = h;

    // Update the total occupied size of the layout
    int occupied_x = (rect.x + rect.w) - layout->start_x;
    int occupied_y = (rect.y + rect.h) - layout->start_y;
    if (occupied_x > layout->size_x)
        layout->size_x = occupied_x;
    if (occupied_y > layout->size_y)
        layout->size_y = occupied_y;

    // Advance the cursor for the next allocation
    if (layout->mode == NUI_LAYOUT_VERTICAL) {
        layout->cursor_y += h + layout->margin;
    } else {
        layout->cursor_x += w + layout->margin;
    }

    return rect;
}

void nui_layout_push(NUI_Context *ctx, NUI_AABB area, NUI_LayoutMode mode) {
    assert(ctx->layout_stack_top < NUI_LAYOUT_STACK_SIZE &&
           "layout stack overflow");

    ctx->layout_stack[ctx->layout_stack_top++] = ctx->layout;

    ctx->layout.cursor_x = area.x;
    ctx->layout.cursor_y = area.y;
    ctx->layout.start_x = area.x;
    ctx->layout.start_y = area.y;
    ctx->layout.size_x = 0;
    ctx->layout.size_y = 0;
    ctx->layout.width = area.w;
    ctx->layout.margin = ctx->style.margin;
    ctx->layout.mode = mode;
}

void nui_layout_pop(NUI_Context *ctx) {
    assert(ctx->layout_stack_top > 0 && "layout stack underflow");

    NUI_Layout child = ctx->layout;
    ctx->layout = ctx->layout_stack[--ctx->layout_stack_top];

    nui_layout_allocate(ctx, child.size_x, child.size_y);
}

bool nui_window_begin(NUI_Context *ctx, const char *title, NUI_AABB area) {
    NUI_Id id = nui_hash(title, 0);
    NUI_Container *container = nui_get_container(ctx, id);

    int title_h = ctx->style.padding_y * 2 + 16;

    // Initialize container area on first use
    if (container->area.w == 0) {
        container->area = area;
        container->area.h += title_h;
    }

    // Calculate title bar area
    NUI_AABB title_area = {
        container->area.x,
        container->area.y,
        container->area.w,
        title_h,
    };

    // Dragging and focus handling
    bool hovered =
        nui_aabb_contains(title_area, ctx->input.mouse_x, ctx->input.mouse_y);
    if (hovered) {
        ctx->hot = id;
    }

    if (ctx->active == id) {
        // Window is being dragged
        if (ctx->input.mouse_down) {
            container->area.x = ctx->input.mouse_x - ctx->input.drag_offset_x;
            container->area.y = ctx->input.mouse_y - ctx->input.drag_offset_y;
            title_area.x = container->area.x;
            title_area.y = container->area.y;
        } else {
            ctx->active = 0;
        }
    } else if (ctx->hot == id && ctx->input.mouse_pressed) {
        // Start dragging
        ctx->active = id;
        ctx->input.drag_offset_x = ctx->input.mouse_x - container->area.x;
        ctx->input.drag_offset_y = ctx->input.mouse_y - container->area.y;
        nui_bring_to_front(ctx, container);
    }

    // Calculate area below title bar
    NUI_AABB body_area = {
        container->area.x,
        container->area.y + title_h,
        container->area.w,
        container->area.h - title_h,
    };
    // Calculate area inside margins
    NUI_AABB content_area = {
        body_area.x + ctx->style.margin,
        body_area.y + ctx->style.margin,
        body_area.w - (ctx->style.margin * 2),
        body_area.h - (ctx->style.margin * 2),
    };

    // Skip rendering if window is outside current scissors
    if (!nui_aabb_overlaps(content_area, ctx->current_scissors)) {
        return false;
    }

    // Initialize the command slice for this container
    container->command_start_index = ctx->command_count;
    container->command_count = 0;
    // Set as active container to track command count
    ctx->current_container = container;

    // Render title bar and window background
    nui_push_command_rect(ctx, title_area, ctx->style.window_title_bar);
    nui_scissors_push(ctx, title_area);
    nui_push_command_text(ctx, title, title_area.x + ctx->style.padding_x,
                          title_area.y + ctx->style.padding_y, ctx->style.text);
    nui_scissors_pop(ctx);

    // Prepare content area and layout
    nui_push_command_rect(ctx, body_area, ctx->style.window_bg);
    nui_scissors_push(ctx, content_area);
    nui_layout_push(ctx, content_area, NUI_LAYOUT_VERTICAL);

    return true;
}

void nui_window_end(NUI_Context *ctx) {
    nui_scissors_pop(ctx);
    nui_layout_pop(ctx);

    // Finalize command count for the active container
    if (ctx->current_container) {
        ctx->current_container->command_count =
            ctx->command_count - ctx->current_container->command_start_index;
        ctx->current_container = NULL;
    }
}

bool nui_button(NUI_Context *ctx, const char *label) {
    if (!ctx->current_container) {
        assert("nui_button called without a parent container" && 0);
        return false;
    }
    NUI_Id id = nui_hash(label, ctx->current_container->id);

    // Derive position size based on current layout
    int text_w, text_h;
    ctx->measure_text(ctx->font, label, &text_w, &text_h);
    int button_w = text_w + (ctx->style.padding_x * 2);
    int button_h = text_h + (ctx->style.padding_y * 2);
    NUI_AABB area = nui_layout_allocate(ctx, button_w, button_h);

    // Hover and click
    bool is_top_window =
        (ctx->current_container->id == ctx->hover_container_id);
    bool mouse_in_rect =
        nui_aabb_contains(area, ctx->input.mouse_x, ctx->input.mouse_y);
    bool mouse_in_scissor = nui_aabb_contains(
        ctx->current_scissors, ctx->input.mouse_x, ctx->input.mouse_y);

    if (is_top_window && mouse_in_rect && mouse_in_scissor) {
        ctx->hot = id;
    }

    if (ctx->hot == id && ctx->input.mouse_pressed) {
        ctx->active = id;
    }

    // Render button
    NUI_Color color = ctx->style.button_idle;
    if (ctx->active == id) {
        color = ctx->style.button_active;
    } else if (ctx->hot == id) {
        color = ctx->style.button_hot;
    }

    // Draw border (outer rect)
    nui_push_command_rect(ctx, area, ctx->style.border);
    // Draw button (inner rect, inset by border_radius)
    NUI_AABB inner_rect = {area.x + ctx->style.border_radius,
                           area.y + ctx->style.border_radius,
                           area.w - 2 * ctx->style.border_radius,
                           area.h - 2 * ctx->style.border_radius};
    nui_push_command_rect(ctx, inner_rect, color);
    nui_scissors_push(ctx, inner_rect);
    nui_push_command_text(
        ctx, label, inner_rect.x + (inner_rect.w - text_w) / 2,
        inner_rect.y + (inner_rect.h - text_h) / 2, ctx->style.text);
    nui_scissors_pop(ctx);

    return (ctx->active == id && ctx->input.mouse_released);
}

bool nui_next_command(NUI_Context *ctx, NUI_Command *out_cmd) {
    // Iterate through sorted containers
    while (ctx->iter_container_index < ctx->sorted_count) {
        NUI_Container *c = ctx->sorted_containers[ctx->iter_container_index];

        // Retrieve next command from this container
        if (ctx->iter_cmd_offset < c->command_count) {
            // Get actual command index in global buffer
            int cmd_index = c->command_start_index + ctx->iter_cmd_offset;
            *out_cmd = ctx->commands[cmd_index];

            ctx->iter_cmd_offset++;
            return true;
        }

        // Finished this container, move to next
        ctx->iter_container_index++;
        ctx->iter_cmd_offset = 0;
    }

    return false;
}
