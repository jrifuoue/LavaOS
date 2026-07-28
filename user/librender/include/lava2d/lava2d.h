#pragma once
#include <stddef.h>
#include <stdint.h>
#include <pluto.h>

typedef struct {
    int x, y;
} LavaPoint;

typedef union {
    uint32_t value;
    struct {
        uint8_t b, g, r, a;
    };
} LavaColor;

typedef struct {
    int x, y;
    int w, h;
} LavaRect;

typedef struct {
    uint32_t* data;
    size_t width;
    size_t height;
} LavaPixelMatrix;

typedef struct {
    PlutoInstance* instance;
    int window;
    int shm;
    uint32_t* pixels;
    size_t width;
    size_t height;
    size_t pitch;
    
    LavaPixelMatrix backbuffer;
    
    LavaRect dirty_rect;
    int dirty;
} Lava2DContext;

int lava2d_create(
    Lava2DContext* ctx,
    PlutoInstance* instance,
    size_t width,
    size_t height,
    const char* title
);

void lava2d_destroy(Lava2DContext* ctx);

void lava2d_put_pixel(
    Lava2DContext* ctx,
    int x, int y,
    uint32_t color
);

static inline uint32_t lava2d_get_pixel(
    Lava2DContext* ctx,
    int x, int y
);

void lava2d_clear(Lava2DContext* ctx, uint32_t color);

void lava2d_fill_rect(
    Lava2DContext* ctx,
    int x, int y,
    int w, int h,
    uint32_t color
);

void lava2d_draw_rect(
    Lava2DContext* ctx,
    int x, int y,
    int w, int h,
    uint32_t color,
    int thickness
);

void lava2d_draw_line(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    uint32_t color
);

void lava2d_draw_circle(
    Lava2DContext* ctx,
    int cx, int cy,
    int radius,
    uint32_t color
);

void lava2d_fill_circle(
    Lava2DContext* ctx,
    int cx, int cy,
    int radius,
    uint32_t color
);

void lava2d_draw_triangle(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint32_t color
);

void lava2d_fill_triangle(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint32_t color
);

uint32_t lava2d_alpha_blend(uint32_t src, uint32_t dst);
uint32_t lava2d_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void lava2d_matrix_init(LavaPixelMatrix* mat, size_t width, size_t height);
void lava2d_matrix_free(LavaPixelMatrix* mat);
void lava2d_matrix_blit(
    Lava2DContext* ctx,
    LavaPixelMatrix* mat,
    int dst_x, int dst_y
);

void lava2d_draw_char(
    Lava2DContext* ctx,
    int x, int y,
    char c,
    uint32_t color
);

int lava2d_draw_string(
    Lava2DContext* ctx,
    int x, int y,
    const char* str,
    uint32_t color
);

int lava2d_text_width(const char* str);

void lava2d_update_dirty(Lava2DContext* ctx, int x, int y, int w, int h);
void lava2d_present(Lava2DContext* ctx);
