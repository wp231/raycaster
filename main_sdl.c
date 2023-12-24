#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "raycaster.h"
#include "raycaster_fixed.h"
#include "raycaster_float.h"
#include "renderer.h"

// 函數：draw_buffer
// 參數：sdlRenderer - SDL 渲染器
//       sdlTexture - SDL 紋理
//       fb - 彩色緩衝區
//       dx - x方向偏移
// 說明：將彩色緩衝區的內容渲染到 SDL 窗口上
static void draw_buffer(SDL_Renderer *sdlRenderer,
                        SDL_Texture *sdlTexture,
                        uint32_t *fb,
                        int dx)
{
    int pitch = 0;
    void *pixelsPtr;
    if (SDL_LockTexture(sdlTexture, NULL, &pixelsPtr, &pitch)) {
        fprintf(stderr, "Unable to lock texture");
        exit(1);
    }
    memcpy(pixelsPtr, fb, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(sdlTexture);
    SDL_Rect r;
    r.x = dx * SCREEN_SCALE;
    r.y = 0;
    r.w = SCREEN_WIDTH * SCREEN_SCALE;
    r.h = SCREEN_HEIGHT * SCREEN_SCALE;
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &r);
}

// 函數：process_event
// 參數：event - SDL 事件
//       moveDirection - 移動方向
//       rotateDirection - 旋轉方向
// 返回：如果事件為退出事件，返回 true；否則返回 false
// 說明：處理 SDL 事件，並更新移動和旋轉方向
static bool process_event(const SDL_Event *event,
                          int *moveDirection,
                          int *rotateDirection)
{
    if (event->type == SDL_QUIT) {
        return true;
    } else if ((event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) &&
               event->key.repeat == 0) {
        SDL_KeyboardEvent k = event->key;
        bool p = event->type == SDL_KEYDOWN;
        switch (k.keysym.sym) {
        case SDLK_ESCAPE:
            return true;
            break;
        case SDLK_UP:
            *moveDirection = p ? 1 : 0;
            break;
        case SDLK_DOWN:
            *moveDirection = p ? -1 : 0;
            break;
        case SDLK_LEFT:
            *rotateDirection = p ? -1 : 0;
            break;
        case SDLK_RIGHT:
            *rotateDirection = p ? 1 : 0;
            break;
        default:
            break;
        }
    }
    return false;
}

// 主函數：main
// 參數：argc - 命令行參數數量
//       args - 命令行參數
// 返回：程式退出碼
// 說明：程式的主入口點
int main(int argc, char *args[])
{
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    } else {
        // 創建 SDL 窗口
        SDL_Window *sdlWindow =
            SDL_CreateWindow("RayCaster [fixed-point vs. floating-point]",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_SCALE * (SCREEN_WIDTH * 2 + 1),
                             SCREEN_SCALE * SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

        if (sdlWindow == NULL) {
            printf("Window could not be created! SDL_Error: %s\n",
                   SDL_GetError());
        } else {
            // 初始化遊戲和光線追踪器
            Game game;
            RayCaster *floatCaster = RayCasterFloatConstruct();
            Renderer floatRenderer = RendererConstruct(floatCaster);
            uint32_t floatBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
            RayCaster *fixedCaster = RayCasterFixedConstruct();
            Renderer fixedRenderer = RendererConstruct(fixedCaster);
            uint32_t fixedBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
            int moveDirection = 0;
            int rotateDirection = 0;
            bool isExiting = false;
            const Uint64 tickFrequency = SDL_GetPerformanceFrequency();
            Uint64 tickCounter = SDL_GetPerformanceCounter();
            SDL_Event event;

            // 創建 SDL 渲染器和紋理
            SDL_Renderer *sdlRenderer = SDL_CreateRenderer(
                sdlWindow, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            SDL_Texture *fixedTexture = SDL_CreateTexture(
                sdlRenderer, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
            SDL_Texture *floatTexture = SDL_CreateTexture(
                sdlRenderer, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

            // 主循環
            while (!isExiting) {
                // 更新遊戲和光線追踪器，獲取渲染的彩色緩衝區
                RendererTraceFrame(&floatRenderer, &game, floatBuffer);
                RendererTraceFrame(&fixedRenderer, &game, fixedBuffer);

                // 渲染彩色緩衝區到窗口上
                draw_buffer(sdlRenderer, fixedTexture, fixedBuffer, 0);
                draw_buffer(sdlRenderer, floatTexture, floatBuffer,
                            SCREEN_WIDTH + 1);

                // 更新 SDL 窗口
                SDL_RenderPresent(sdlRenderer);

                // 處理事件並更新遊戲狀態
                if (SDL_PollEvent(&event)) {
                    isExiting =
                        process_event(&event, &moveDirection, &rotateDirection);
                }
                const Uint64 nextCounter = SDL_GetPerformanceCounter();
                const Uint64 ticks = nextCounter - tickCounter;
                tickCounter = nextCounter;
                GameMove(&game, moveDirection, rotateDirection,
                         ticks / (SDL_GetPerformanceFrequency() >> 8));
            }
            // 釋放資源
            SDL_DestroyTexture(floatTexture);
            SDL_DestroyTexture(fixedTexture);
            SDL_DestroyRenderer(sdlRenderer);
            SDL_DestroyWindow(sdlWindow);
        }
    }

    SDL_Quit();
    return 0;
}
