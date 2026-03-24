#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
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

// Window = 2x the game screen for visibility
#define ZOOM 2
#define SIM_W (LN_SCREEN_W * ZOOM)
#define SIM_H (LN_SCREEN_H * ZOOM)
#define NUM_STARS 40

static GameState gs;
static Camera cam;
static int16_t stars[NUM_STARS][2];
static TTF_Font *font = NULL;
static TTF_Font *font_sm = NULL;

static void generate_stars() {
    uint32_t seed = 12345;
    for (int i = 0; i < NUM_STARS; i++) {
        seed = seed * 1103515245 + 12345;
        stars[i][0] = (int16_t)(seed % LN_WORLD_W);
        seed = seed * 1103515245 + 12345;
        stars[i][1] = (int16_t)(seed % (LN_WORLD_H / 2));
    }
}

// Scale game-screen coords (320x240) to window coords (640x480)
static inline int zx(int16_t v) { return v * ZOOM; }

static void draw_text(SDL_Renderer *r, TTF_Font *f, int x, int y, const char *text, SDL_Color col) {
    SDL_Surface *surf = TTF_RenderText_Blended(f, text, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

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
            SDL_Rect dot = {zx(x), zx(y), ZOOM, ZOOM};
            SDL_RenderFillRect(r, &dot);
        }
    }

    // Terrain fill
    SDL_SetRenderDrawColor(r, 20, 40, 20, 255);
    for (uint16_t i = 0; i + 1 < gs.terrain.num_points; i++) {
        int tx1 = zx(world_to_screen_x(gs.terrain.points[i][0], cam));
        int ty1 = zx(world_to_screen_y(gs.terrain.points[i][1], cam));
        int tx2 = zx(world_to_screen_x(gs.terrain.points[i + 1][0], cam));
        int ty2 = zx(world_to_screen_y(gs.terrain.points[i + 1][1], cam));
        if (tx1 > tx2) { std::swap(tx1, tx2); std::swap(ty1, ty2); }
        for (int x = std::max(0, tx1); x <= std::min(SIM_W - 1, tx2); x++) {
            int y = (tx2 == tx1) ? ty1 : ty1 + (ty2 - ty1) * (x - tx1) / (tx2 - tx1);
            if (y < 0) y = 0;
            if (y < SIM_H)
                SDL_RenderDrawLine(r, x, y, x, SIM_H - 1);
        }
    }

    // Terrain
    for (uint16_t i = 0; i + 1 < gs.terrain.num_points; i++) {
        int16_t x1 = world_to_screen_x(gs.terrain.points[i][0], cam);
        int16_t y1 = world_to_screen_y(gs.terrain.points[i][1], cam);
        int16_t x2 = world_to_screen_x(gs.terrain.points[i + 1][0], cam);
        int16_t y2 = world_to_screen_y(gs.terrain.points[i + 1][1], cam);

        bool is_zone = (gs.terrain.points[i][0] >= gs.terrain.zone_x1 &&
                        gs.terrain.points[i + 1][0] <= gs.terrain.zone_x2 &&
                        gs.terrain.points[i][1] == gs.terrain.zone_y &&
                        gs.terrain.points[i + 1][1] == gs.terrain.zone_y);
        if (is_zone) {
            // Thick bright yellow landing zone just below terrain surface
            SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
            SDL_RenderDrawLine(r, zx(x1), zx(y1), zx(x2), zx(y2));
            SDL_RenderDrawLine(r, zx(x1), zx(y1)+1, zx(x2), zx(y2)+1);
            SDL_RenderDrawLine(r, zx(x1), zx(y1)+2, zx(x2), zx(y2)+2);
            SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
            SDL_RenderDrawLine(r, zx(x1), zx(y1)+3, zx(x2), zx(y2)+3);
        } else {
            SDL_SetRenderDrawColor(r, 0, 200, 0, 255);
            SDL_RenderDrawLine(r, zx(x1), zx(y1), zx(x2), zx(y2));
        }
    }

    // Lander — lunar module matching web version
    {
        const Lander &l = gs.lander;
        float s = sinf(l.rotation), c = cosf(l.rotation);
        auto pt = [&](float lx, float ly, int &ox, int &oy) {
            float wx = l.x + lx * c - ly * s;
            float wy = l.y + lx * s + ly * c;
            ox = zx(world_to_screen_x(wx, cam));
            oy = zx(world_to_screen_y(wy, cam));
        };

        SDL_SetRenderDrawColor(r, l.crashed?255:255, l.crashed?0:255, l.crashed?0:255, 255);
        // Body (-12,-25) to (12,-5)
        int bx[4], by[4];
        pt(-12,-25,bx[0],by[0]); pt(12,-25,bx[1],by[1]);
        pt(12,-5,bx[2],by[2]); pt(-12,-5,bx[3],by[3]);
        for(int i=0;i<4;i++){int ni=(i+1)%4; SDL_RenderDrawLine(r,bx[i],by[i],bx[ni],by[ni]);}
        // Command module (-8,-30) to (8,-25)
        int cx[4], cy[4];
        pt(-8,-30,cx[0],cy[0]); pt(8,-30,cx[1],cy[1]);
        pt(8,-25,cx[2],cy[2]); pt(-8,-25,cx[3],cy[3]);
        for(int i=0;i<4;i++){int ni=(i+1)%4; SDL_RenderDrawLine(r,cx[i],cy[i],cx[ni],cy[ni]);}
        // Landing legs
        SDL_SetRenderDrawColor(r, l.crashed?255:180, l.crashed?0:180, l.crashed?0:180, 255);
        int lx1,ly1,lx2,ly2,lx3,ly3;
        pt(-10,-5,lx1,ly1); pt(-16,0,lx2,ly2); pt(-18,0,lx3,ly3);
        SDL_RenderDrawLine(r,lx1,ly1,lx2,ly2); SDL_RenderDrawLine(r,lx2,ly2,lx3,ly3);
        pt(10,-5,lx1,ly1); pt(16,0,lx2,ly2); pt(18,0,lx3,ly3);
        SDL_RenderDrawLine(r,lx1,ly1,lx2,ly2); SDL_RenderDrawLine(r,lx2,ly2,lx3,ly3);
        // Thrust flame
        if (l.thrusting && l.fuel > 0) {
            int fx1,fy1,fx2,fy2,ftx,fty;
            pt(-5,-5,fx1,fy1); pt(5,-5,fx2,fy2); pt(0,6,ftx,fty);
            SDL_SetRenderDrawColor(r, 255, 140, 0, 255);
            SDL_RenderDrawLine(r,fx1,fy1,ftx,fty); SDL_RenderDrawLine(r,fx2,fy2,ftx,fty);
        }

        // Lander center dot for visibility
        int cdx = zx(world_to_screen_x(l.x, cam));
        int cdy = zx(world_to_screen_y(l.y - 15, cam));
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDrawPoint(r, cdx, cdy);
        SDL_RenderDrawPoint(r, cdx-1, cdy);
        SDL_RenderDrawPoint(r, cdx+1, cdy);
        SDL_RenderDrawPoint(r, cdx, cdy-1);
        SDL_RenderDrawPoint(r, cdx, cdy+1);
    }

    // HUD — fuel bar
    {
        float fuel_pct = gs.lander.fuel / LN_INITIAL_FUEL;
        if (fuel_pct < 0) fuel_pct = 0;
        SDL_Rect bg = {zx(LN_SCREEN_W - 84), zx(4), zx(80), zx(8)};
        SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
        SDL_RenderFillRect(r, &bg);
        SDL_Rect fg = {zx(LN_SCREEN_W - 83), zx(5), (int)(fuel_pct * zx(78)), zx(6)};
        if (fuel_pct > 0.25f) SDL_SetRenderDrawColor(r, 0, 200, 0, 255);
        else SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
        SDL_RenderFillRect(r, &fg);
    }

    // HUD — text telemetry
    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {255, 60, 60, 255};
        char buf[64];

        int fuel_pct = (int)(gs.lander.fuel * 100.0f / LN_INITIAL_FUEL);
        if (fuel_pct < 0) fuel_pct = 0;
        snprintf(buf, sizeof(buf), "Fuel: %d%%", fuel_pct);
        draw_text(r, font, 8, 8, buf, white);

        float vy = gs.lander.vy;
        float spd = lander_speed(gs.lander);
        SDL_Color spd_col = (vy > 1.0f) ? red : (vy < -1.0f) ? SDL_Color{60,255,60,255} : white;
        const char *arrow = (vy > 1.0f) ? " v" : (vy < -1.0f) ? " ^" : "";
        snprintf(buf, sizeof(buf), "Spd: %.1f%s", spd, arrow);
        draw_text(r, font, 8, 28, buf, spd_col);

        float alt = terrain_height_at(gs.terrain, gs.lander.x) - gs.lander.y;
        if (alt < 0) alt = 0;
        snprintf(buf, sizeof(buf), "Alt: %d", (int)alt);
        draw_text(r, font, 8, 48, buf, white);

        float angle_deg = gs.lander.rotation * 180.0f / M_PI;
        snprintf(buf, sizeof(buf), "Ang: %.0f", angle_deg);
        draw_text(r, font, 8, 68, buf,
                  fabsf(gs.lander.rotation) > LN_MAX_LANDING_ANGLE ? red : white);

        if (alt < 80 && spd > LN_MAX_LANDING_SPEED)
            draw_text(r, font, SIM_W / 2 - 60, SIM_H - 40, "! TOO FAST !", red);
        else if (alt < 80 && fabsf(gs.lander.rotation) > LN_MAX_LANDING_ANGLE)
            draw_text(r, font, SIM_W / 2 - 50, SIM_H - 40, "! ANGLE !", red);
    }

    // Phase overlays
    if (gs.phase == PHASE_MENU && font) {
        SDL_Color cyan = {0, 255, 255, 255};
        draw_text(r, font, SIM_W / 2 - 100, 8, "Press any key to start", cyan);
    } else if (gs.phase == PHASE_LANDED && font) {
        SDL_Color green = {0, 255, 0, 255};
        char buf[64];
        snprintf(buf, sizeof(buf), "LANDED! Score: %d  [R]estart", game_calc_score(gs));
        draw_text(r, font, SIM_W / 2 - 140, SIM_H / 2 - 10, buf, green);
    } else if (gs.phase == PHASE_CRASHED && font) {
        SDL_Color red = {255, 0, 0, 255};
        draw_text(r, font, SIM_W / 2 - 100, SIM_H / 2 - 10, "CRASHED!  [R]estart", red);
    }

    if (font_sm && gs.phase == PHASE_PLAYING) {
        SDL_Color dim = {120, 120, 120, 255};
        draw_text(r, font_sm, SIM_W - 180, SIM_H - 36, "Arrows/Space:fly  R:restart", dim);
    }

    SDL_RenderPresent(r);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    const char *font_paths[] = {
        "/System/Library/Fonts/SFNSMono.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        NULL
    };
    for (int i = 0; font_paths[i] && !font; i++) {
        font = TTF_OpenFont(font_paths[i], 16);
        if (font) font_sm = TTF_OpenFont(font_paths[i], 12);
    }

    SDL_Window *win = SDL_CreateWindow("Lunar Lander Badge Sim",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SIM_W, SIM_H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    generate_stars();

    uint8_t diff = 0;
    if (argc > 1) diff = atoi(argv[1]);
    game_init(gs, diff, (uint32_t)time(NULL));
    gs.phase = PHASE_MENU;
    gs.mode = MODE_OFFLINE;
    gs.start_ms = SDL_GetTicks();
    cam = {0, 0, 0, 0, 0};

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
                    case SDLK_UP: case SDLK_SPACE:
                        thrust = true;
                        if (gs.phase == PHASE_MENU) { gs.phase = PHASE_PLAYING; gs.start_ms = SDL_GetTicks(); }
                        break;
                    case SDLK_LEFT:
                        rotate = -1;
                        if (gs.phase == PHASE_MENU) { gs.phase = PHASE_PLAYING; gs.start_ms = SDL_GetTicks(); }
                        break;
                    case SDLK_RIGHT:
                        rotate = 1;
                        if (gs.phase == PHASE_MENU) { gs.phase = PHASE_PLAYING; gs.start_ms = SDL_GetTicks(); }
                        break;
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_r:
                        game_init(gs, diff, (uint32_t)SDL_GetTicks());
                        gs.phase = PHASE_MENU;
                        gs.mode = MODE_OFFLINE;
                        gs.start_ms = SDL_GetTicks();
                        thrust = false; rotate = 0;
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

    if (font_sm) TTF_CloseFont(font_sm);
    if (font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
