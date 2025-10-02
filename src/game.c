#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define WINDOW_W 800
#define WINDOW_H 600

typedef struct {
    float x, y, w, h;
    float vy;
} Paddle;

typedef struct {
    float x, y, r;
    float vx, vy;
} Ball;

void draw_rect(SDL_Renderer *ren, float x, float y, float w, float h, SDL_Color c) {
    SDL_Rect r = {(int)x, (int)y, (int)w, (int)h};
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ren, &r);
}

void draw_circle(SDL_Renderer *ren, int cx, int cy, int radius, SDL_Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) SDL_RenderDrawPoint(ren, cx + dx, cy + dy);
        }
    }
}

// 5x7 bitmap font for uppercase letters used in launch screen
// each char is 7 rows of 5 bits, LSB = leftmost column bit, rows stored top->bottom.
typedef struct { char ch; uint8_t rows[7]; } Glyph;

static const Glyph tiny_font[] = {
    {' ', {0,0,0,0,0,0,0}},
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
};

static const Glyph* find_glyph(char c) {
    for (size_t i = 0; i < sizeof(tiny_font)/sizeof(tiny_font[0]); ++i) {
        if (tiny_font[i].ch == c) return &tiny_font[i];
    }
    return NULL;
}

void draw_text(SDL_Renderer *ren, int x, int y, int scale, SDL_Color col, const char *text) {
    int spacing = scale;
    int cx = x;
    for (size_t i = 0; i < strlen(text); ++i) {
        char c = text[i];
        const Glyph *g = find_glyph(c);
        if (!g) { cx += (5*scale) + spacing; continue; }
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = g->rows[row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (1 << (4-col))) {
                    SDL_Rect p = { cx + col*scale, y + row*scale, scale, scale };
                    SDL_SetRenderDrawColor(ren, col, col, col, 255); // placeholder, will set color below
                    SDL_SetRenderDrawColor(ren, col, col, col, 255); // redundant but safe
                    SDL_SetRenderDrawColor(ren, col, col, col, 255);
                }
            }
        }
        // redraw the glyph properly with color
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = g->rows[row];
            for (int colb = 0; colb < 5; ++colb) {
                if (bits & (1 << (4-colb))) {
                    SDL_Rect p = { cx + colb*scale, y + row*scale, scale, scale };
                    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
                    SDL_RenderFillRect(ren, &p);
                }
            }
        }
        cx += (5*scale) + spacing;
    }
}

int main(int argc, char **argv) {
    // init sdl
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "error initialising sdl: %s\n", SDL_GetError());
        return 1;
    }

    // init window
    SDL_Window *win = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 0);
    if (!win) {
        fprintf(stderr, "error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // init renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // create paddles and ball
    Paddle left = {40, WINDOW_H/2 - 50, 16, 100, 0};
    Paddle right = {WINDOW_W - 56, WINDOW_H/2 - 50, 16, 100, 0};
    Ball ball = {WINDOW_W/2.0f, WINDOW_H/2.0f, 8, 300.0f, 200.0f};

    int score_left = 0, score_right = 0;
    bool launched = false; // wait for space to start

    bool running = true;
    Uint64 prev = SDL_GetPerformanceCounter();

    // game loop
    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - prev) / (float)SDL_GetPerformanceFrequency();
        prev = now;
        if (dt > 0.05f) dt = 0.05f;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (!launched && e.key.keysym.sym == SDLK_SPACE) {
                    launched = true;
                    // randomize initial ball direction
                    ball.vx = 300.0f * (rand() % 2 ? 1 : -1);
                    ball.vy = 200.0f * (rand() % 2 ? 1 : -1);
                }
                if (launched && e.key.keysym.sym == SDLK_r) {
                    // reset ball
                    ball.x = WINDOW_W/2.0f; ball.y = WINDOW_H/2.0f;
                    ball.vx = 300.0f * (rand() % 2 ? 1 : -1);
                    ball.vy = 200.0f * (rand() % 2 ? 1 : -1);
                }
            }
        }

        const Uint8 *key = SDL_GetKeyboardState(NULL);
        // If not launched yet, skip updating game objects; only check for input events
        if (!launched) {
            // render launch screen
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            SDL_Color white = {255,255,255,255};
            const char *msg = "PRESS SPACE TO PLAY";
            // draw text centered
            int scale = 6;
            int text_w = (5*scale + scale) * strlen(msg);
            int x = (WINDOW_W - text_w) / 2;
            int y = WINDOW_H/2 - (7*scale)/2;
            draw_text(ren, x, y, scale, white, msg);
            SDL_RenderPresent(ren);
            SDL_Delay(16);
            continue;
        }
        // left paddle, w/s
        left.vy = 0;
        if (key[SDL_SCANCODE_W]) left.vy = -400.0f;
        if (key[SDL_SCANCODE_S]) left.vy = 400.0f;
        // right paddle, up/down arrow
        right.vy = 0;
        if (key[SDL_SCANCODE_UP]) right.vy = -400.0f;
        if (key[SDL_SCANCODE_DOWN]) right.vy = 400.0f;

        // integrate paddles
        left.y += left.vy * dt;
        right.y += right.vy * dt;
        if (left.y < 0) left.y = 0;
        if (left.y + left.h > WINDOW_H) left.y = WINDOW_H - left.h;
        if (right.y < 0) right.y = 0;
        if (right.y + right.h > WINDOW_H) right.y = WINDOW_H - right.h;

        // ball movement
        ball.x += ball.vx * dt;
        ball.y += ball.vy * dt;

        // ball collission with top/bottom
        if (ball.y - ball.r < 0) ball.y = ball.r; ball.vy = -ball.vy;
        if (ball.y + ball.r > WINDOW_H) ball.y = WINDOW_H - ball.r; ball.vy = -ball.vy;

        // ball collission with paddles (aabb approx)
        SDL_Rect brect = {(int)(ball.x - ball.r), (int)(ball.y - ball.r), (int)(ball.r*2), (int)(ball.r*2)};
        SDL_Rect lrect = {(int)left.x, (int)left.y, (int)left.w, (int)left.h};
        SDL_Rect rrect = {(int)right.x, (int)right.y, (int)right.w, (int)right.h};

        if (SDL_HasIntersection(&brect, &lrect)) {
            ball.x = left.x + left.w + ball.r;
            ball.vx = fabsf(ball.vx) + 10.0f; // increase speed slightly
            // tweak velocity y based on where it hit the paddle to simulate physics
            float rel = (ball.y - (left.y + left.h/2)) / (left.h/2);
            ball.vy = rel * 300.0f;
        }
        if (SDL_HasIntersection(&brect, &rrect)) {
            ball.x = right.x - ball.r;
            ball.vx = -fabsf(ball.vx) - 10.0f;
            float rel = (ball.y - (right.y + right.h/2)) / (right.h/2);
            ball.vy = rel * 300.0f;
        }

        // score logic, ball goes off screen
        if (ball.x < 0) {
            score_right++;
            ball.x = WINDOW_W/2.0f; ball.y = WINDOW_H/2.0f;
            ball.vx = 300.0f; ball.vy = 200.0f * (rand()%2?1:-1);
        }
        if (ball.x > WINDOW_W) {
            score_left++;
            ball.x = WINDOW_W/2.0f; ball.y = WINDOW_H/2.0f;
            ball.vx = -300.0f; ball.vy = 200.0f * (rand()%2?1:-1);
        }

        // render window
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        // center line
        SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
        for (int y = 0; y < WINDOW_H; y += 20) {
            SDL_Rect seg = {WINDOW_W/2 - 2, y, 4, 12};
            SDL_RenderFillRect(ren, &seg);
        }

        // paddle rendering
        SDL_Color white = {255,255,255,255};
        draw_rect(ren, left.x, left.y, left.w, left.h, white);
        draw_rect(ren, right.x, right.y, right.w, right.h, white);

        // ball rendering
        draw_circle(ren, (int)ball.x, (int)ball.y, (int)ball.r, white);

        // score rendering, basic display with rects
        for (int i = 0; i < score_left && i < 10; ++i) {
            SDL_Rect s = {50 + i*12, 20, 8, 12};
            SDL_SetRenderDrawColor(ren, 200,200,200,255); SDL_RenderFillRect(ren, &s);
        }
        for (int i = 0; i < score_right && i < 10; ++i) {
            SDL_Rect s = {WINDOW_W - 50 - i*12 - 8, 20, 8, 12};
            SDL_SetRenderDrawColor(ren, 200,200,200,255); SDL_RenderFillRect(ren, &s);
        }

        SDL_RenderPresent(ren);

        SDL_Delay(1);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
