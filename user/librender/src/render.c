#include "../include/lava2d/lava2d.h"
#include "../include/lava2d/lava2d_font.h"
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <minos/sysstd.h>
#include <math.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX((x), (min)), (max)))
#define ABS(x) ((x) < 0 ? -(x) : (x))

int lava2d_create(
    Lava2DContext* ctx,
    PlutoInstance* instance,
    size_t width,
    size_t height,
    const char* title
) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->instance = instance;
    ctx->width = width;
    ctx->height = height;
    ctx->pitch = width * sizeof(uint32_t);

    ctx->window = pluto_create_window(instance, &(WmCreateWindowInfo){
        .width = width,
        .height = height,
        .title = (char*)title
    }, 16);

    ctx->shm = pluto_create_shm_region(instance, &(WmCreateSHMRegion){
        .size = ctx->pitch * height
    });

    if (_shmmap(ctx->shm, (void**)&ctx->pixels) < 0)
        return -1;

    // Initialize backbuffer
    lava2d_matrix_init(&ctx->backbuffer, width, height);
    ctx->dirty = false;
    ctx->dirty_rect = (LavaRect){0, 0, (int)width, (int)height};

    return 0;
}

void lava2d_destroy(Lava2DContext* ctx) {
    lava2d_matrix_free(&ctx->backbuffer);
}

static inline void lava2d_put_pixel_internal(
    Lava2DContext* ctx,
    int x, int y,
    uint32_t color
) {
    if (x < 0 || y < 0 || x >= (int)ctx->width || y >= (int)ctx->height)
        return;
    
    // Write to backbuffer
    ctx->backbuffer.data[y * ctx->width + x] = color;
    
    // Update dirty rectangle
    if (!ctx->dirty) {
        ctx->dirty_rect = (LavaRect){x, y, 1, 1};
        ctx->dirty = true;
    } else {
        ctx->dirty_rect.x = MIN(ctx->dirty_rect.x, x);
        ctx->dirty_rect.y = MIN(ctx->dirty_rect.y, y);
        ctx->dirty_rect.w = MAX(ctx->dirty_rect.x + ctx->dirty_rect.w, x) - ctx->dirty_rect.x + 1;
        ctx->dirty_rect.h = MAX(ctx->dirty_rect.y + ctx->dirty_rect.h, y) - ctx->dirty_rect.y + 1;
    }
}

// Public pixel functions
void lava2d_put_pixel(Lava2DContext* ctx, int x, int y, uint32_t color) {
    lava2d_put_pixel_internal(ctx, x, y, color);
}

static inline uint32_t lava2d_get_pixel(
    Lava2DContext* ctx,
    int x, int y
) {
    if (x < 0 || y < 0 || x >= (int)ctx->width || y >= (int)ctx->height)
        return 0;
    return ctx->backbuffer.data[y * ctx->width + x];
}

uint32_t lava2d_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t lava2d_alpha_blend(uint32_t src, uint32_t dst) {
    uint8_t sa = (src >> 24) & 0xFF;
    if (sa == 0) return dst;
    if (sa == 255) return src;
    
    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = src & 0xFF;
    
    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = dst & 0xFF;
    
    uint8_t r = dr + ((sr - dr) * sa) / 255;
    uint8_t g = dg + ((sg - dg) * sa) / 255;
    uint8_t b = db + ((sb - db) * sa) / 255;
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Optimized clear
void lava2d_clear(Lava2DContext* ctx, uint32_t color) {
    uint32_t* buf = ctx->backbuffer.data;
    size_t count = ctx->width * ctx->height;
    
    // Use memset-like optimization for solid colors
    for (size_t i = 0; i < count; i += 4) {
        buf[i] = color;
        if (i + 1 < count) buf[i + 1] = color;
        if (i + 2 < count) buf[i + 2] = color;
        if (i + 3 < count) buf[i + 3] = color;
    }
    
    ctx->dirty = true;
    ctx->dirty_rect = (LavaRect){0, 0, (int)ctx->width, (int)ctx->height};
}

// Optimized rectangle fill
void lava2d_fill_rect(
    Lava2DContext* ctx,
    int x, int y,
    int w, int h,
    uint32_t color
) {
    // Clipping
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)ctx->width) w = ctx->width - x;
    if (y + h > (int)ctx->height) h = ctx->height - y;
    if (w <= 0 || h <= 0) return;
    
    // Direct memory write for speed
    for (int yy = 0; yy < h; yy++) {
        uint32_t* row = ctx->backbuffer.data + (y + yy) * ctx->width + x;
        for (int xx = 0; xx < w; xx++) {
            row[xx] = color;
        }
    }
    
    // Update dirty rect
    if (!ctx->dirty) {
        ctx->dirty_rect = (LavaRect){x, y, w, h};
        ctx->dirty = true;
    } else {
        ctx->dirty_rect.x = MIN(ctx->dirty_rect.x, x);
        ctx->dirty_rect.y = MIN(ctx->dirty_rect.y, y);
        ctx->dirty_rect.w = MAX(ctx->dirty_rect.x + ctx->dirty_rect.w, x + w) - ctx->dirty_rect.x;
        ctx->dirty_rect.h = MAX(ctx->dirty_rect.y + ctx->dirty_rect.h, y + h) - ctx->dirty_rect.y;
    }
}

// Rectangle outline
void lava2d_draw_rect(
    Lava2DContext* ctx,
    int x, int y,
    int w, int h,
    uint32_t color,
    int thickness
) {
    if (thickness <= 0) return;
    
    // Top and bottom
    lava2d_fill_rect(ctx, x, y, w, thickness, color);
    lava2d_fill_rect(ctx, x, y + h - thickness, w, thickness, color);
    
    // Left and right (avoid corners)
    lava2d_fill_rect(ctx, x, y + thickness, thickness, h - 2 * thickness, color);
    lava2d_fill_rect(ctx, x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);
}

// Bresenham's line algorithm
void lava2d_draw_line(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    uint32_t color
) {
    int dx = ABS(x1 - x0);
    int dy = ABS(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        lava2d_put_pixel_internal(ctx, x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Circle drawing using midpoint algorithm
void lava2d_draw_circle(
    Lava2DContext* ctx,
    int cx, int cy,
    int radius,
    uint32_t color
) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        lava2d_put_pixel_internal(ctx, cx + x, cy + y, color);
        lava2d_put_pixel_internal(ctx, cx + y, cy + x, color);
        lava2d_put_pixel_internal(ctx, cx - y, cy + x, color);
        lava2d_put_pixel_internal(ctx, cx - x, cy + y, color);
        lava2d_put_pixel_internal(ctx, cx - x, cy - y, color);
        lava2d_put_pixel_internal(ctx, cx - y, cy - x, color);
        lava2d_put_pixel_internal(ctx, cx + y, cy - x, color);
        lava2d_put_pixel_internal(ctx, cx + x, cy - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

// Filled circle
void lava2d_fill_circle(
    Lava2DContext* ctx,
    int cx, int cy,
    int radius,
    uint32_t color
) {
    for (int y = -radius; y <= radius; y++) {
        int x_max = (int)sqrt(radius * radius - y * y);
        for (int x = -x_max; x <= x_max; x++) {
            lava2d_put_pixel_internal(ctx, cx + x, cy + y, color);
        }
    }
}

// Triangle outline
void lava2d_draw_triangle(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint32_t color
) {
    lava2d_draw_line(ctx, x0, y0, x1, y1, color);
    lava2d_draw_line(ctx, x1, y1, x2, y2, color);
    lava2d_draw_line(ctx, x2, y2, x0, y0, color);
}

// Filled triangle using scanline algorithm
void lava2d_fill_triangle(
    Lava2DContext* ctx,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint32_t color
) {
    // Sort by y
    if (y0 > y1) { int t; t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    if (y1 > y2) { int t; t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    if (y0 > y1) { int t; t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    
    if (y0 == y2) return; // Degenerate triangle
    
    int total_height = y2 - y0;
    
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;
        
        int x_a = x0 + (x2 - x0) * alpha;
        int x_b = second_half ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;
        
        if (x_a > x_b) { int t = x_a; x_a = x_b; x_b = t; }
        
        for (int x = x_a; x <= x_b; x++) {
            lava2d_put_pixel_internal(ctx, x, y0 + i, color);
        }
    }
}

// Matrix operations
void lava2d_matrix_init(LavaPixelMatrix* mat, size_t width, size_t height) {
    mat->width = width;
    mat->height = height;
    mat->data = (uint32_t*)calloc(width * height, sizeof(uint32_t));
}

void lava2d_matrix_free(LavaPixelMatrix* mat) {
    if (mat->data) {
        free(mat->data);
        mat->data = NULL;
    }
}

void lava2d_matrix_blit(
    Lava2DContext* ctx,
    LavaPixelMatrix* mat,
    int dst_x, int dst_y
) {
    for (size_t y = 0; y < mat->height; y++) {
        for (size_t x = 0; x < mat->width; x++) {
            uint32_t pixel = mat->data[y * mat->width + x];
            if (pixel & 0xFF000000) { // Only draw non-transparent pixels
                lava2d_put_pixel_internal(ctx, dst_x + x, dst_y + y, pixel);
            }
        }
    }
}

// Optimized present with dirty rectangle support
void lava2d_present(Lava2DContext* ctx) {
    if (!ctx->dirty) return;
    
    // Clip dirty rect to bounds
    LavaRect dr = ctx->dirty_rect;
    if (dr.x < 0) { dr.w += dr.x; dr.x = 0; }
    if (dr.y < 0) { dr.h += dr.y; dr.y = 0; }
    if (dr.x + dr.w > (int)ctx->width) dr.w = ctx->width - dr.x;
    if (dr.y + dr.h > (int)ctx->height) dr.h = ctx->height - dr.y;
    
    if (dr.w <= 0 || dr.h <= 0) {
        ctx->dirty = false;
        return;
    }
    
    // Copy only dirty region to display buffer
    for (int y = dr.y; y < dr.y + dr.h; y++) {
        uint32_t* src = ctx->backbuffer.data + y * ctx->width + dr.x;
        uint32_t* dst = ctx->pixels + y * ctx->width + dr.x;
        memcpy(dst, src, dr.w * sizeof(uint32_t));
    }
    
    // Pass dirty rectangle to WM for partial updates
    pluto_draw_shm_region(ctx->instance, &(WmDrawSHMRegion){
        .window = ctx->window,
        .shm_key = ctx->shm,
        .width = ctx->width,
        .height = ctx->height,
        .pitch_bytes = ctx->pitch,
    });
    
    ctx->dirty = false;
}

void lava2d_draw_char(
    Lava2DContext* ctx,
    int x, int y,
    char c,
    uint32_t color
) {
    unsigned char ch = (unsigned char)c;
    const uint8_t* glyph = &lava2d_builtin_font[ch * LAVA2D_FONT_HEIGHT];

    for(int row = 0; row < LAVA2D_FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for(int col = 0; col < LAVA2D_FONT_WIDTH; col++) {
            if(bits & (0x80 >> col))
                lava2d_put_pixel_internal(ctx, x + col, y + row, color);
        }
    }
}

int lava2d_draw_string(
    Lava2DContext* ctx,
    int x, int y,
    const char* str,
    uint32_t color
) {
    int ox = x;
    for(; *str; str++) {
        if(*str == '\n') {
            x = ox;
            y += LAVA2D_FONT_HEIGHT;
            continue;
        }
        lava2d_draw_char(ctx, x, y, *str, color);
        x += LAVA2D_FONT_WIDTH;
    }
    return x - ox;
}

int lava2d_text_width(const char* str) {
    int w = 0;
    for(; *str; str++) {
        if(*str == '\n') break;
        w += LAVA2D_FONT_WIDTH;
    }
    return w;
}
