#include <SDL.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "Game/LunarConfig.h"
#include "Game/LunarPhysics.h"
#include "Game/LunarTerrain.h"
#include "Game/LunarState.h"
#include "Game/LunarRenderer.h"

// Stubs for hardware audio/LED functions
void audio_thrust_start() {}
void audio_thrust_stop() {}
void audio_landed() {}
void audio_crashed() {}
void audio_low_fuel() {}
void leds_thrust() {}
void leds_landed() {}
void leds_crashed() {}
void leds_idle() {}

#define ZOOM 2
#define SIM_W (LN_SCREEN_W * ZOOM)
#define SIM_H (LN_SCREEN_H * ZOOM)
#define LANDER_SIZE 8
#define NUM_STARS 40

static GameState gs;
static Camera cam;
static int16_t stars[NUM_STARS][2];

static void generate_stars() {
    uint32_t seed = 12345;
    for (int i = 0; i < NUM_STARS; i++) {
        seed = seed * 1103515245 + 12345;
        stars[i][0] = (int16_t)(seed % LN_WORLD_W);
        seed = seed * 1103515245 + 12345;
        stars[i][1] = (int16_t)(seed % (LN_WORLD_H / 2));
    }
}

// Scale screen coords (320x240) to window coords (640x480)
static inline int sx(int16_t v) { return v * ZOOM; }

static void draw(SDL_Renderer *r) {
    camera_update(cam, gs.lander, gs.phase);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    // Stars
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int i = 0; i < NUM_STARS; i++) {
        int16_t x = world_to_screen_x(stars[i][0], cam);
        int16_t y = world_to_screen_y(stars[i][1], cam);
        if (x >= 0 && x < LN_SCREEN_W && y >= 0 && y < LN_SCREEN_H) {
            SDL_Rect dot = {sx(x), sx(y), ZOOM, ZOOM};
            SDL_RenderFillRect(r, &dot);
        }
    }

    // Terrain
    for (uint16_t i = 0; i + 1 < gs.terrain.num_points; i++) {
        int16_t x1 = world_to_screen_x(gs.terrain.points[i][0], cam);
        int16_t y1 = world_to_screen_y(gs.terrain.points[i][1], cam);
        int16_t x2 = world_to_screen_x(gs.terrain.points[i + 1][0], cam);
        int16_t y2 = world_to_screen_y(gs.terrain.points[i + 1][1], cam);
        if ((x1 < 0 && x2 < 0) || (x1 >= LN_SCREEN_W && x2 >= LN_SCREEN_W)) continue;

        bool is_zone = (gs.terrain.points[i][0] >= gs.terrain.zone_x1 &&
                        gs.terrain.points[i + 1][0] <= gs.terrain.zone_x2 &&
                        gs.terrain.points[i][1] == gs.terrain.zone_y &&
                        gs.terrain.points[i + 1][1] == gs.terrain.zone_y);
        if (is_zone) SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
        else SDL_SetRenderDrawColor(r, 0, 200, 0, 255);
        SDL_RenderDrawLine(r, sx(x1), sx(y1), sx(x2), sx(y2));
    }

    // Zone indicator (off-screen arrow)
    {
        int16_t zx = world_to_screen_x((gs.terrain.zone_x1 + gs.terrain.zone_x2) / 2, cam);
        int16_t zy = world_to_screen_y(gs.terrain.zone_y, cam);
        if (zx < 0 || zx >= LN_SCREEN_W || zy < 0 || zy >= LN_SCREEN_H) {
            int16_t ix = zx < 0 ? 8 : (zx >= LN_SCREEN_W ? LN_SCREEN_W - 8 : zx);
            int16_t iy = zy < 0 ? 8 : (zy >= LN_SCREEN_H ? LN_SCREEN_H - 20 : zy);
            SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
            SDL_RenderDrawLine(r, sx(ix), sx(iy - 4), sx(ix + 4), sx(iy));
            SDL_RenderDrawLine(r, sx(ix + 4), sx(iy), sx(ix), sx(iy + 4));
            SDL_RenderDrawLine(r, sx(ix), sx(iy + 4), sx(ix - 4), sx(iy));
            SDL_RenderDrawLine(r, sx(ix - 4), sx(iy), sx(ix), sx(iy - 4));
        }
    }

    // Lander — same triangle math as LunarRenderer.cpp
    {
        const Lander &l = gs.lander;
        float s = sinf(l.rotation), c = cosf(l.rotation);
        float pts[3][2] = {
            { 0, -LANDER_SIZE},
            {-LANDER_SIZE / 2.0f,  LANDER_SIZE / 2.0f},
            { LANDER_SIZE / 2.0f,  LANDER_SIZE / 2.0f}
        };
        int lx[3], ly[3];
        for (int i = 0; i < 3; i++) {
            float rx = pts[i][0] * c - pts[i][1] * s;
            float ry = pts[i][0] * s + pts[i][1] * c;
            lx[i] = sx(world_to_screen_x(l.x + rx, cam));
            ly[i] = sx(world_to_screen_y(l.y + ry, cam));
        }
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        for (int i = 0; i < 3; i++) {
            int ni = (i + 1) % 3;
            SDL_RenderDrawLine(r, lx[i], ly[i], lx[ni], ly[ni]);
        }

        // Thrust flame
        if (l.thrusting && l.fuel > 0) {
            float fx = 0, fy = LANDER_SIZE;
            float rfx = fx * c - fy * s;
            float rfy = fx * s + fy * c;
            int flx = sx(world_to_screen_x(l.x + rfx, cam));
            int fly = sx(world_to_screen_y(l.y + rfy, cam));
            SDL_SetRenderDrawColor(r, 255, 140, 0, 255);
            SDL_RenderDrawLine(r, lx[1], ly[1], flx, fly);
            SDL_RenderDrawLine(r, lx[2], ly[2], flx, fly);
        }
    }

    // HUD — fuel bar
    {
        float fuel_pct = gs.lander.fuel / LN_INITIAL_FUEL;
        if (fuel_pct < 0) fuel_pct = 0;
        SDL_Rect bg = {sx(LN_SCREEN_W - 84), sx(4), sx(80), sx(8)};
        SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
        SDL_RenderFillRect(r, &bg);
        SDL_Rect fg = {sx(LN_SCREEN_W - 83), sx(5), (int)(fuel_pct * sx(78)), sx(6)};
        if (fuel_pct > 0.25f) SDL_SetRenderDrawColor(r, 0, 200, 0, 255);
        else SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
        SDL_RenderFillRect(r, &fg);
    }

    // Phase overlay
    if (gs.phase == PHASE_LANDED) {
        SDL_SetRenderDrawColor(r, 0, 255, 0, 128);
        SDL_Rect banner = {SIM_W / 4, SIM_H / 2 - 20, SIM_W / 2, 40};
        SDL_RenderFillRect(r, &banner);
    } else if (gs.phase == PHASE_CRASHED) {
        SDL_SetRenderDrawColor(r, 255, 0, 0, 128);
        SDL_Rect banner = {SIM_W / 4, SIM_H / 2 - 20, SIM_W / 2, 40};
        SDL_RenderFillRect(r, &banner);
    }

    SDL_RenderPresent(r);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow("Lunar Lander Badge Sim",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SIM_W, SIM_H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    generate_stars();

    uint8_t diff = 0;
    if (argc > 1) diff = atoi(argv[1]);
    game_init(gs, diff, (uint32_t)time(NULL));
    gs.phase = PHASE_PLAYING;
    gs.start_ms = SDL_GetTicks();
    cam.x = LN_START_X;
    cam.y = LN_START_Y;
    cam.target_x = LN_START_X;
    cam.target_y = LN_START_Y;
    cam.lerp_speed = 0.05f;

    bool thrust = false;
    int rotate = 0;
    bool running = true;
    uint32_t last = SDL_GetTicks();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP: case SDLK_SPACE: thrust = true; break;
                    case SDLK_LEFT: rotate = -1; break;
                    case SDLK_RIGHT: rotate = 1; break;
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_r:
                        game_init(gs, diff, (uint32_t)SDL_GetTicks());
                        gs.phase = PHASE_PLAYING;
                        gs.start_ms = SDL_GetTicks();
                        break;
                }
            }
            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP: case SDLK_SPACE: thrust = false; break;
                    case SDLK_LEFT: if (rotate == -1) rotate = 0; break;
                    case SDLK_RIGHT: if (rotate == 1) rotate = 0; break;
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        if (now - last >= 16) {
            if (gs.phase == PHASE_PLAYING)
                game_tick(gs, thrust, rotate, now);
            draw(ren);
            last = now;
        }
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
