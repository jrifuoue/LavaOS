#include <lava2d/lava2d.h>
#include <libwm/events.h>
#include <math.h>
#include <minos/sysstd.h>

#define WIN_W 640
#define WIN_H 480

static Lava2DContext ctx;

static void draw_background(void) {
    for(int y = 0; y < WIN_H; y++) {
        uint8_t r = (uint8_t)(y * 255 / WIN_H);
        uint8_t b = (uint8_t)(255 - y * 255 / WIN_H);
        lava2d_draw_line(&ctx, 0, y, WIN_W, y, lava2d_make_color(r, 30, b, 255));
    }
}

static void draw_title(void) {
    lava2d_fill_rect(&ctx, WIN_W / 2 - 100, 10, 200, 30, lava2d_make_color(0, 0, 0, 200));
    lava2d_draw_rect(&ctx, WIN_W / 2 - 100, 10, 200, 30, lava2d_make_color(255, 255, 255, 255), 2);
    lava2d_draw_string(&ctx, WIN_W / 2 - 80, 16, "lava2d Demo", lava2d_make_color(255, 255, 255, 255));
}

static void draw_shape_gallery(void) {
    int cx = WIN_W / 2;
    int base_y = 80;

    lava2d_fill_rect(&ctx, cx - 280, base_y, 120, 120, lava2d_make_color(220, 50, 50, 255));
    lava2d_draw_rect(&ctx, cx - 280, base_y, 120, 120, lava2d_make_color(255, 200, 50, 255), 3);

    lava2d_fill_circle(&ctx, cx - 100, base_y + 60, 55, lava2d_make_color(50, 180, 50, 255));
    lava2d_draw_circle(&ctx, cx - 100, base_y + 60, 55, lava2d_make_color(255, 255, 100, 255));

    lava2d_fill_triangle(&ctx, cx + 80, base_y + 110, cx + 20, base_y, cx + 140, base_y, lava2d_make_color(50, 100, 220, 255));
    lava2d_draw_triangle(&ctx, cx + 80, base_y + 110, cx + 20, base_y, cx + 140, base_y, lava2d_make_color(200, 200, 255, 255));
}

static void draw_pattern_grid(void) {
    int start_y = 230;
    int cols = 12, rows = 6;
    int cell = 18;
    int ox = WIN_W / 2 - (cols * cell) / 2;

    for(int r = 0; r < rows; r++) {
        for(int c = 0; c < cols; c++) {
            int x = ox + c * cell;
            int y = start_y + r * cell;
            uint8_t red = (uint8_t)((c * 255) / cols);
            uint8_t green = (uint8_t)((r * 255) / rows);
            uint8_t blue = (uint8_t)(255 - (c * 255) / cols);
            lava2d_fill_rect(&ctx, x + 1, y + 1, cell - 2, cell - 2, lava2d_make_color(red, green, blue, 255));
        }
    }
}

static void draw_lines_starburst(void) {
    int cx = WIN_W / 2;
    int cy = 360;
    int count = 24;

    for(int i = 0; i < count; i++) {
        float angle = (float)i * 3.14159f * 2.0f / (float)count;
        int x1 = cx + (int)(cosf(angle) * 80);
        int y1 = cy + (int)(sinf(angle) * 80);
        uint8_t r = (uint8_t)(128 + 127 * cosf(angle));
        uint8_t g = (uint8_t)(128 + 127 * sinf(angle));
        uint8_t b = (uint8_t)(128 + 127 * cosf(angle + 2.0f));
        lava2d_draw_line(&ctx, cx, cy, x1, y1, lava2d_make_color(r, g, b, 255));
    }

    lava2d_fill_circle(&ctx, cx, cy, 8, lava2d_make_color(255, 255, 255, 255));
}

static void draw_alpha_demos(void) {
    int x = 120;
    int y = 340;

    lava2d_fill_circle(&ctx, x, y, 30, lava2d_make_color(255, 0, 0, 255));
    lava2d_fill_circle(&ctx, x + 25, y, 30, lava2d_make_color(0, 255, 0, 128));
    lava2d_fill_circle(&ctx, x + 50, y, 30, lava2d_make_color(0, 0, 255, 128));

    x = WIN_W - 120;
    lava2d_fill_rect(&ctx, x - 35, y - 25, 70, 50, lava2d_make_color(80, 80, 80, 255));
    lava2d_fill_rect(&ctx, x - 25, y - 15, 50, 30, lava2d_make_color(255, 100, 50, 160));
    lava2d_draw_rect(&ctx, x - 35, y - 25, 70, 50, lava2d_make_color(200, 200, 200, 255), 1);
}

static void draw_font_demo(void) {
    int y = 60;

    lava2d_draw_string(&ctx, 10, y, "Font Rendering:", lava2d_make_color(255, 255, 0, 255));
    y += 20;

    lava2d_draw_string(&ctx, 10, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", lava2d_make_color(255, 255, 255, 255));
    y += 18;
    lava2d_draw_string(&ctx, 10, y, "abcdefghijklmnopqrstuvwxyz", lava2d_make_color(200, 200, 255, 255));
    y += 18;
    lava2d_draw_string(&ctx, 10, y, "0123456789 !@#$%^&*()", lava2d_make_color(100, 255, 100, 255));
    y += 18;
    lava2d_draw_string(&ctx, 10, y, "Hello, LavaOS! Rendering text", lava2d_make_color(255, 180, 100, 255));
    y += 18;
    lava2d_draw_string(&ctx, 10, y, "with the lava2d graphics lib.", lava2d_make_color(255, 180, 100, 255));

    lava2d_draw_rect(&ctx, 5, 58, 260, 96, lava2d_make_color(255, 255, 255, 80), 1);
}

static void draw_bottom_bar(void) {
    lava2d_fill_rect(&ctx, 0, WIN_H - 30, WIN_W, 30, lava2d_make_color(20, 20, 30, 230));
    lava2d_draw_line(&ctx, 0, WIN_H - 30, WIN_W, WIN_H - 30, lava2d_make_color(100, 150, 255, 255));
    lava2d_draw_string(&ctx, 10, WIN_H - 22, "lava2d | Press any key to exit", lava2d_make_color(180, 180, 200, 255));
}

int main(void) {
    PlutoInstance instance;
    pluto_create_instance(&instance);

    if(lava2d_create(&ctx, &instance, WIN_W, WIN_H, "lava2d Demo") < 0)
        return 1;

    draw_background();
    draw_title();
    draw_font_demo();
    lava2d_present(&ctx);

    pluto_draw_shm_region(&instance, &(WmDrawSHMRegion){
        .window = ctx.window,
        .shm_key = ctx.shm,
        .width = WIN_W,
        .height = WIN_H,
        .pitch_bytes = WIN_W * sizeof(uint32_t),
    });

    int resp;
    for(;;) {
        pluto_read_next_packet(&instance, &resp);
        PlutoEventQueue* evq = &instance.windows.items[ctx.window]->event_queue;
        while(evq->head != evq->tail) {
            evq->tail = evq->tail % evq->cap;
            size_t at = evq->tail++;
            PlutoEvent event = evq->buffer[at];
            if(event.event == WM_EVENT_KEY_DOWN) {
                lava2d_destroy(&ctx);
                return 0;
            }
        }
    }
}
