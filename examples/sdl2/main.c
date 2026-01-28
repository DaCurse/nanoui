#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

static void sdl_render_rect(SDL_Renderer *renderer,
                            const NUI_CommandRect *rect_cmd) {
    SDL_Rect sdl_rect = {rect_cmd->rect.x, rect_cmd->rect.y, rect_cmd->rect.w,
                         rect_cmd->rect.h};
    SDL_SetRenderDrawColor(renderer, rect_cmd->color.r, rect_cmd->color.g,
                           rect_cmd->color.b, rect_cmd->color.a);
    SDL_RenderFillRect(renderer, &sdl_rect);
}

void sdl_measure_text(NUI_UserFont font, const char *text, int *out_width,
                      int *out_height) {

    TTF_Font *sdl_font = (TTF_Font *)font;
    if (!text || !font ||
        TTF_SizeText(sdl_font, text, out_width, out_height) != 0) {
        *out_width = 0;
        *out_height = 0;
    }
}

void sdl_render_text(SDL_Renderer *renderer, TTF_Font *font,
                     const NUI_CommandText *text_cmd) {
    SDL_Color color = *(SDL_Color *)(&text_cmd->color);
    SDL_Surface *surf = TTF_RenderText_Blended(font, text_cmd->text, color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect dst = {text_cmd->x, text_cmd->y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &dst);

        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}

void sdl_set_scissors(SDL_Renderer *renderer,
                      const NUI_CommandScissors *scissor_cmd) {

    SDL_Rect rect = {scissor_cmd->area.x, scissor_cmd->area.y,
                     scissor_cmd->area.w, scissor_cmd->area.h};
    SDL_RenderSetClipRect(renderer, &rect);
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n",
                SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        printf("TTF_Init error: %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font *default_font =
        TTF_OpenFont("examples/fonts/SpaceMono-Regular.ttf", 14);
    if (!default_font) {
        printf("Failed to load font: %s\n", TTF_GetError());
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

    // Initialize context
    NUI_Context ctx = {0};
    nui_init(&ctx, sdl_measure_text, default_font);

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
        nui_frame_begin(&ctx);

        if (nui_window_begin(&ctx, "Test Window",
                             (NUI_AABB){10, 20, 300, 130})) {

            static char labels[3][32];
            for (int i = 0; i < 3; i++) {
                snprintf(labels[i], sizeof(labels[i]), "Click Me %d", i + 1);
                if (nui_button(&ctx, labels[i])) {
                    printf("Button %d Clicked in Window 1!\n", i + 1);
                }
            }

            nui_window_end(&ctx);
        }

        if (nui_window_begin(&ctx, "Test Window2",
                             (NUI_AABB){120, 80, 300, 130})) {
            static char labels[3][32];
            for (int i = 0; i < 3; i++) {
                snprintf(labels[i], sizeof(labels[i]), "Click Me %d", i + 1);
                if (nui_button(&ctx, labels[i])) {
                    printf("Button %d Clicked in Window 2!\n", i + 1);
                }
            }

            nui_window_end(&ctx);
        }

        nui_frame_end(&ctx);

        // Drain Command Buffer and render
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
        SDL_RenderClear(renderer);

        NUI_Command cmd;
        while (nui_next_command(&ctx, &cmd)) {
            switch (cmd.type) {
            case NUI_CMD_RECT:
                sdl_render_rect(renderer, &cmd.rect);
#ifdef PRINT_CMDS_ONCE
                printf(
                    "  NUI_CMD_RECT: x=%d y=%d w=%d h=%d color=(%d,%d,%d,%d)\n",
                    cmd.rect.rect.x, cmd.rect.rect.y, cmd.rect.rect.w,
                    cmd.rect.rect.h, cmd.rect.color.r, cmd.rect.color.g,
                    cmd.rect.color.b, cmd.rect.color.a);
#endif
                break;
            case NUI_CMD_TEXT:
                sdl_render_text(renderer, default_font, &cmd.text);
#ifdef PRINT_CMDS_ONCE
                printf("  NUI_CMD_TEXT: x=%d y=%d text=\"%s\" "
                       "color=(%d,%d,%d,%d)\n",
                       cmd.text.x, cmd.text.y, cmd.text.text, cmd.text.color.r,
                       cmd.text.color.g, cmd.text.color.b, cmd.text.color.a);
#endif
                break;
            case NUI_CMD_SCISSORS:
                sdl_set_scissors(renderer, &cmd.scissors);
#ifdef PRINT_CMDS_ONCE
                printf("  NUI_CMD_SCISSORS: area=(x=%d y=%d w=%d h=%d)\n",
                       cmd.scissors.area.x, cmd.scissors.area.y,
                       cmd.scissors.area.w, cmd.scissors.area.h);
#endif
                break;
            default:
                UNREACHABLE("NUI_CommandType");
            }
        }

#ifdef PRINT_CMDS_ONCE
        abort();
#endif

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

