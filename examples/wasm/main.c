#include <emscripten.h>

#include "nui.h"

EMSCRIPTEN_KEEPALIVE
void run_ui_frame(NUI_Context *ctx) {
    nui_frame_begin(ctx);

    if (nui_window_begin(ctx, "WASM Window", (NUI_AABB){50, 50, 300, 200})) {
        nui_button(ctx, "Hello from WASM");
        nui_window_end(ctx);
    }

    if (nui_window_begin(ctx, "WASM Window 2", (NUI_AABB){80, 80, 300, 200})) {
        nui_button(ctx, "Hello from WASM Again");
        nui_window_end(ctx);
    }

    nui_frame_end(ctx);
}
