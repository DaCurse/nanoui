#include <SDL2/SDL.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "nui.h"

#define WINDOW_WIDTH (800)
#define WINDOW_HEIGHT (600)

#define TODO(x)                                                                \
    do {                                                                       \
        assert(x && 0);                                                        \
        abort();                                                               \
    } while (0)
#define UNREACHABLE(x) TODO(x)

static void render_rect(SDL_Renderer *renderer,
                        const NUI_CommandRect *rect_cmd) {
    SDL_Rect sdl_rect = {rect_cmd->rect.x, rect_cmd->rect.y, rect_cmd->rect.w,
                         rect_cmd->rect.h};
    SDL_SetRenderDrawColor(renderer, rect_cmd->color.r, rect_cmd->color.g,
                           rect_cmd->color.b, rect_cmd->color.a);
    SDL_RenderFillRect(renderer, &sdl_rect);
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n",
                SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "NUI SDL2 Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n",
                SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n",
                SDL_GetError());
        return 1;
    }

    NUI_Context ctx = {0};

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        // User Input
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_MOUSEMOTION:
                nui_input_mouse_move(&ctx, e.motion.x, e.motion.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                nui_input_mouse_button(&ctx, e.type == SDL_MOUSEBUTTONDOWN);
                break;
            default:
                break;
            }
        }

        // UI / Logic
        nui_begin_frame(&ctx);

        if (nui_button(&ctx, "Click Me", (NUI_AABB){100, 100, 100, 30})) {
            printf("Button 1 Clicked!\n");
        }

        if (nui_button(&ctx, "Click Me 2", (NUI_AABB){100, 140, 100, 30})) {
            printf("Button 2 Clicked!\n");
        }

        nui_end_frame(&ctx);

        // Command Buffer / Rendering
        SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
        SDL_RenderClear(renderer);

        NUI_Command cmd;
        while (nui_next_command(&ctx, &cmd)) {
            switch (cmd.type) {
            case NUI_CMD_RECT: {
                render_rect(renderer, &cmd.rect);
            } break;
            case NUI_CMD_TEXT:
                TODO("NUI_CMD_TEXT");
            case NUI_CMD_SCISSOR:
                TODO("NUI_CMD_SCISSOR");
            default:
                UNREACHABLE("NUI_CommandType");
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
