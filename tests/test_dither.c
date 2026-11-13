/* Tests for the ordered dither primitive and the components drawn with it. Runs headless with
 * the software renderer. The primitive is checked cell by cell against the 8x8 Bayer matrix;
 * the components through what they return and edit, the keyboard, and their pixels. */
#include "../src/cui_internal.h"
#include <cui/cui_render.h>

static int g_failed, g_checks;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { g_checks++; long long a_ = (long long)(a), b_ = (long long)(b); if (a_ != b_) { g_failed++; \
    printf("  FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)

static cui_sw_renderer *g_sw;
static float g_dt = 1.0f / 60.0f;
static bool g_rtl;

static cui_ctx *new_ctx(float w, float h, bool reduced) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = CUI_THEME_DARK, .accent = CUI_ACCENT_NEUTRAL });
    cui_set_viewport(ui, w, h, 1);
    cui_set_reduced_motion(ui, reduced);
    cui_input_mouse_leave(ui);
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, g_dt);
        cui_column(ui, .pad = 20, .gap = 12, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND,
                   .direction = g_rtl ? CUI_DIRECTION_RTL : CUI_DIRECTION_LTR) fn(ui, user);
        cui_end_frame(ui);
    }
}
static void move(cui_ctx *ui, build_fn fn, void *u, float x, float y) { cui_input_mouse_move(ui, x, y); run(ui, fn, u, 1); }
static void click(cui_ctx *ui, build_fn fn, void *u, float x, float y) {
    move(ui, fn, u, x, y);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1);
}
static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k) {
    cui_input_key(ui, k, true, 0); cui_input_key(ui, k, false, 0);
    run(ui, fn, u, 1);
}

/* the last frame, rendered at scale 1 */
typedef struct { unsigned char *px; int w, h; } image;
static image render(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    image im = { malloc((size_t)dd->width_px * dd->height_px * 4), dd->width_px, dd->height_px };
    cui_sw_render(g_sw, dd, im.px, im.w, im.h, im.w * 4, CUI_SW_RGBA);
    return im;
}
static const unsigned char *at(image im, float x, float y) { return im.px + ((size_t)(int)y * im.w + (size_t)(int)x) * 4; }
static bool same(image a, image b) { return a.w == b.w && a.h == b.h && memcmp(a.px, b.px, (size_t)a.w * a.h * 4) == 0; }

/* ------------------------------------------------------------------------ */
/* the primitive                                                            */

static int bayer(int x, int y) {
    static const int m[64] = { 0, 32, 8, 40, 2, 34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4, 36, 14, 46, 6, 38,
                               60, 28, 52, 20, 62, 30, 54, 22, 3, 35, 11, 43, 1, 33, 9, 41, 51, 19, 59, 27, 49, 17, 57, 25,
                               15, 47, 7, 39, 13, 45, 5, 37, 63, 31, 55, 23, 61, 29, 53, 21 };
    return m[((y & 7) << 3) | (x & 7)];
}

/* five 64 point squares at 2 point cells: densities, a floor, a phase and a rounded cut */
typedef struct { float density, floor, phase; float radius; } prim_case;
static const prim_case g_prim[5] = { { 0.5f, 0, 0, 0 }, { 0.25f, 0, 0, 0 }, { 1, 0.5f, 0, 0 }, { 0.5f, 0, 3, 0 }, { 1, 0, 0, 16 } };
static cui_rect g_canvas;
static void paint_prim(cui_ctx *ui, cui_rect r, void *u) {
    (void)u;
    g_canvas = r;
    cui_draw_rect(ui, r, CUI_CORNERS(0), cui__rgba(0, 0, 0, 255));
    for (int k = 0; k < 5; k++) {
        cui_rect sq = { r.x + 80 * (float)k, r.y, 64, 64 };
        cui__dither d;
        memset(&d, 0, sizeof d);
        d.color = cui__rgba(255, 255, 255, 255);
        d.origin = (cui_vec2){ sq.x, sq.y };
        d.floor = g_prim[k].floor;
        d.phase.x = g_prim[k].phase;
        d.shape = sq;
        d.radius = CUI_CORNERS(g_prim[k].radius);
        cui__draw_dither_rect(ui, &d, sq, g_prim[k].density, g_prim[k].density, false);
    }
    /* a gradient, 1 at the left to 0 at the right */
    cui__dither d;
    memset(&d, 0, sizeof d);
    d.color = cui__rgba(255, 255, 255, 255);
    d.origin = (cui_vec2){ r.x, r.y + 80 };
    cui__draw_dither_rect(ui, &d, (cui_rect){ r.x, r.y + 80, 256, 16 }, 1, 0, false);
}
static void build_prim(cui_ctx *ui, void *u) { (void)u; cui_canvas(ui, "prim", CUI_PX(420), CUI_PX(100), paint_prim, NULL, 0); }

static void test_primitive(void) {
    printf("primitive\n");
    cui_ctx *ui = new_ctx(480, 160, true);
    run(ui, build_prim, NULL, 2);
    image im = render(ui);
    int lit[5] = { 0 }, wrong[5] = { 0 }, subset_bad = 0;
    for (int k = 0; k < 4; k++)
        for (int cy = 0; cy < 32; cy++)
            for (int cx = 0; cx < 32; cx++) {
                /* the center pixel of the cell */
                bool on = at(im, g_canvas.x + 80 * k + cx * 2 + 1, g_canvas.y + cy * 2 + 1)[0] > 128;
                int sx = cx + (int)g_prim[k].phase;
                float t = (bayer(sx, cy) + 0.5f) / 64.0f;
                bool want = t >= g_prim[k].floor && t < g_prim[k].density;
                lit[k] += on;
                wrong[k] += on != want;
                /* lower densities light a subset of the cells of higher ones */
                if (k == 1 && on && !(at(im, g_canvas.x + cx * 2 + 1, g_canvas.y + cy * 2 + 1)[0] > 128)) subset_bad++;
            }
    for (int k = 0; k < 4; k++) CHECK_EQ(wrong[k], 0);
    CHECK_EQ(lit[0], 512);   /* half of 1024 cells */
    CHECK_EQ(lit[1], 256);
    CHECK_EQ(lit[2], 512);   /* a floor of 0.5 turns off the lower half */
    CHECK_EQ(subset_bad, 0);
    /* every pixel of a cell agrees, so a dot is a 2 by 2 square */
    int split = 0;
    for (int cy = 0; cy < 32; cy++)
        for (int cx = 0; cx < 32; cx++) {
            unsigned char v = at(im, g_canvas.x + cx * 2, g_canvas.y + cy * 2)[0];
            if (at(im, g_canvas.x + cx * 2 + 1, g_canvas.y + cy * 2 + 1)[0] != v || at(im, g_canvas.x + cx * 2 + 1, g_canvas.y + cy * 2)[0] != v) split++;
        }
    CHECK_EQ(split, 0);
    /* the rounded cut: the corner is empty, the middle full */
    CHECK(at(im, g_canvas.x + 320 + 1, g_canvas.y + 1)[0] < 30);
    CHECK(at(im, g_canvas.x + 320 + 32, g_canvas.y + 32)[0] > 200);
    /* the gradient: dense at the left, empty at the right, thinning in between */
    int left = 0, mid = 0, right = 0;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 32; x++) {
            left += at(im, g_canvas.x + x, g_canvas.y + 80 + y)[0] > 128;
            mid += at(im, g_canvas.x + 112 + x, g_canvas.y + 80 + y)[0] > 128;
            right += at(im, g_canvas.x + 224 + x, g_canvas.y + 80 + y)[0] > 128;
        }
    CHECK(left > mid && mid > right);
    CHECK(left > 420);
    CHECK(right < 60);
    free(im.px);
    cui_destroy(ui);
}

/* cells with a density each: 20 x 10 cells of 2 points from a point off the pixel grid */
static float g_cell_density[200];
static cui_vec2 g_cell_origin;
static void paint_cells(cui_ctx *ui, cui_rect r, void *u) {
    (void)u;
    cui_draw_rect(ui, r, CUI_CORNERS(0), cui__rgba(0, 0, 0, 255));
    cui__dither d;
    memset(&d, 0, sizeof d);
    d.color = cui__rgba(255, 255, 255, 255);
    g_cell_origin = d.origin = (cui_vec2){ r.x + 3.3f, r.y + 2.6f };
    for (int k = 0; k < 200; k++) g_cell_density[k] = (float)((k * 7 + (k / 20) * 13) % 17) / 16.0f;
    for (int k = 40; k < 60; k++) g_cell_density[k] = 0.5f;   /* a row of one density: one quad */
    cui__draw_dither_cells(ui, &d, 1, 2, 20, 10, g_cell_density);
}
static void build_cells(cui_ctx *ui, void *u) { (void)u; cui_canvas(ui, "cells", CUI_PX(60), CUI_PX(40), paint_cells, NULL, 0); }

static void test_cells(void) {
    printf("cells\n");
    static const float scales[2] = { 1, 1.5f };
    for (int si = 0; si < 2; si++) {
        float sc = scales[si];
        cui_ctx *ui = new_ctx(100, 80, true);
        cui_set_viewport(ui, 100, 80, sc);
        run(ui, build_cells, NULL, 2);
        const cui_draw_data *dd = cui_get_draw_data(ui);
        image im = { malloc((size_t)dd->width_px * dd->height_px * 4), dd->width_px, dd->height_px };
        cui_sw_render(g_sw, dd, im.px, im.w, im.h, im.w * 4, CUI_SW_RGBA);
        /* the grid the primitive uses: whole pixel cells from a whole pixel origin */
        int cell = (int)roundf(2 * sc), gx = (int)roundf(g_cell_origin.x * sc), gy = (int)roundf(g_cell_origin.y * sc);
        int wrong = 0, split = 0, lit = 0;
        for (int j = 0; j < 10; j++)
            for (int i = 0; i < 20; i++) {
                int px = gx + (i + 1) * cell, py = gy + (j + 2) * cell;
                float t = (bayer(i + 1, j + 2) + 0.5f) / 64.0f;
                bool want = t < g_cell_density[j * 20 + i];
                bool on = at(im, (float)px, (float)py)[0] > 128;
                lit += on;
                wrong += on != want;
                /* every pixel of the cell agrees: a cell is an exact square at any scale */
                for (int y = 0; y < cell; y++)
                    for (int x = 0; x < cell; x++) split += (at(im, (float)(px + x), (float)(py + y))[0] > 128) != on;
            }
        CHECK_EQ(wrong, 0);
        CHECK_EQ(split, 0);
        CHECK(lit > 50 && lit < 150);
        /* the cells before the first one stay dark */
        CHECK(at(im, (float)(gx + 1), (float)(gy + 1))[0] < 30);
        free(im.px);
        cui_destroy(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* dither progress                                                          */

typedef struct { float value; bool indeterminate; } prog_case;
static void build_prog(cui_ctx *ui, void *u) {
    prog_case *c = u;
    cui_progress(ui, c->value, .dither = true, .indeterminate = c->indeterminate, .width = CUI_PX(400));
}
/* bright pixels along the middle of the bar, in [x0, x1) of its width */
static int bright(image im, cui_rect bar, float f0, float f1) {
    int n = 0;
    for (float x = bar.x + bar.w * f0; x < bar.x + bar.w * f1; x += 1) n += at(im, x, bar.y + bar.h * 0.5f)[0] > 200;
    return n;
}

static void test_progress(void) {
    printf("dither progress\n");
    cui_ctx *ui = new_ctx(480, 80, true);
    prog_case c = { 0, false };
    run(ui, build_prog, &c, 3);
    image im = render(ui);
    cui_rect bar = { 20, 20, 400, 8 };
    CHECK_EQ(bright(im, bar, 0, 1), 0);   /* 0% shows no dots */
    free(im.px);
    c.value = 1;
    run(ui, build_prog, &c, 3);
    im = render(ui);
    CHECK(bright(im, bar, 0.02f, 0.98f) >= 384);   /* full */
    free(im.px);
    c.value = 0.5f;
    run(ui, build_prog, &c, 3);
    im = render(ui);
    CHECK(bright(im, bar, 0.02f, 0.48f) >= 182);   /* solid up to the value */
    CHECK(bright(im, bar, 0.5f, 0.65f) > 0);       /* dots past it */
    CHECK_EQ(bright(im, bar, 0.67f, 1), 0);        /* nothing 60 points past the edge */
    image still = im;
    /* reduced motion: the pattern holds still */
    run(ui, build_prog, &c, 30);
    im = render(ui);
    CHECK(same(im, still));
    free(im.px);
    /* right to left: the bar sits at the right of the column, its fill starting at the right */
    g_rtl = true;
    run(ui, build_prog, &c, 3);
    im = render(ui);
    cui_rect rbar = { 480 - 20 - 400, 20, 400, 8 };
    CHECK(bright(im, rbar, 0.52f, 0.98f) >= 182);
    CHECK_EQ(bright(im, rbar, 0, 0.33f), 0);
    free(im.px);
    g_rtl = false;
    free(still.px);
    cui_destroy(ui);

    /* with motion the dots drift forward a cell every tenth of a second */
    ui = new_ctx(480, 80, false);
    run(ui, build_prog, &c, 90);
    im = render(ui);
    run(ui, build_prog, &c, 6);
    image later = render(ui);
    CHECK(!same(im, later));
    CHECK(bright(later, bar, 0.02f, 0.48f) >= 182);   /* the solid part does not move */
    free(im.px); free(later.px);
    /* indeterminate: a band of dots, somewhere on the track, that moves */
    prog_case ind = { 0, true };
    run(ui, build_prog, &ind, 30);
    im = render(ui);
    CHECK(bright(im, bar, 0, 1) > 10);
    run(ui, build_prog, &ind, 10);
    later = render(ui);
    CHECK(!same(im, later));
    free(im.px); free(later.px);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* pattern meter                                                            */

typedef struct { int shown; bool gray; cui_id bar; } meter_case;
static void build_meter(cui_ctx *ui, void *u) {
    meter_case *c = u;
    static const cui_meter_part parts[] = { { "Photos", 50 }, { "Video", 30 }, { "Apps", 20 } };
    cui_box(ui, .width = CUI_PX(400)) {
        c->shown = cui_pattern_meter(ui, "m", parts, 3, .total = 200, .label = "Storage", .unit = "GB", .grayscale = c->gray);
        c->bar = cui_last_id(ui);
    }
}

static void test_meter(void) {
    printf("pattern meter\n");
    cui_ctx *ui = new_ctx(480, 200, true);
    meter_case c;
    memset(&c, 0, sizeof c);
    run(ui, build_meter, &c, 3);
    CHECK_EQ(c.shown, -1);
    cui_rect r = cui_get_rect(ui, c.bar);
    CHECK(r.w > 390 && r.h == 18);
    float y = r.y + r.h * 0.5f;
    /* parts take 25%, 15% and 10% of the bar; the rest is free */
    move(ui, build_meter, &c, r.x + r.w * 0.10f, y); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 0);
    move(ui, build_meter, &c, r.x + r.w * 0.30f, y); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 1);
    move(ui, build_meter, &c, r.x + r.w * 0.45f, y); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 2);
    move(ui, build_meter, &c, r.x + r.w * 0.75f, y); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, -1);
    /* the legend: the first entry's swatch starts the row 10 points under the bar */
    move(ui, build_meter, &c, r.x + 7, r.y + r.h + 10 + 8); run(ui, build_meter, &c, 1);
    CHECK_EQ(c.shown, 0);
    cui_input_mouse_leave(ui);
    run(ui, build_meter, &c, 2);
    CHECK_EQ(c.shown, -1);
    /* the keyboard: Tab to the bar, then step through the parts */
    key(ui, build_meter, &c, CUI_KEY_TAB);
    key(ui, build_meter, &c, CUI_KEY_RIGHT); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 0);
    key(ui, build_meter, &c, CUI_KEY_RIGHT); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 1);
    key(ui, build_meter, &c, CUI_KEY_END); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 2);
    key(ui, build_meter, &c, CUI_KEY_RIGHT); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 2);
    key(ui, build_meter, &c, CUI_KEY_ESCAPE); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, -1);
    key(ui, build_meter, &c, CUI_KEY_LEFT); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 2);
    key(ui, build_meter, &c, CUI_KEY_HOME); run(ui, build_meter, &c, 1); CHECK_EQ(c.shown, 0);
    cui_destroy(ui);

    /* grayscale: every lit dot in the bar is the text color, no hue anywhere */
    ui = new_ctx(480, 200, true);
    memset(&c, 0, sizeof c);
    c.gray = true;
    run(ui, build_meter, &c, 3);
    r = cui_get_rect(ui, c.bar);
    image im = render(ui);
    int hue = 0, lit = 0;
    for (float yy = r.y + 2; yy < r.y + r.h - 2; yy += 1)
        for (float x = r.x + 8; x < r.x + r.w - 8; x += 1) {
            const unsigned char *p = at(im, x, yy);
            int mx = CUI_MAX(p[0], CUI_MAX(p[1], p[2])), mn = CUI_MIN(p[0], CUI_MIN(p[1], p[2]));
            hue += mx - mn > 6;
            lit += p[0] > 200;
        }
    CHECK_EQ(hue, 0);
    CHECK(lit > 0);
    free(im.px);
    cui_destroy(ui);

    /* right to left: the first part is at the right end */
    g_rtl = true;
    ui = new_ctx(480, 200, true);
    memset(&c, 0, sizeof c);
    run(ui, build_meter, &c, 3);
    r = cui_get_rect(ui, c.bar);
    move(ui, build_meter, &c, r.x + r.w * 0.92f, r.y + r.h * 0.5f); run(ui, build_meter, &c, 1);
    CHECK_EQ(c.shown, 0);
    move(ui, build_meter, &c, r.x + r.w * 0.08f, r.y + r.h * 0.5f); run(ui, build_meter, &c, 1);
    CHECK_EQ(c.shown, -1);
    g_rtl = false;
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* dither area fills                                                        */

static cui_id g_chart;
static void build_area(cui_ctx *ui, void *u) {
    (void)u;
    static const float a[] = { 50, 50, 50, 50, 50, 50 }, b[] = { 50, 50, 50, 50, 50, 50 };
    cui_series s[2] = { { "Low", a, 6, { 230, 40, 40, 255 } }, { "High", b, 6, { 40, 40, 230, 255 } } };
    cui_line_chart(ui, "c", s, 2, .dither = true, .stacked = true, .min = 0, .max = 100, .height = 220, .no_legend = true);
    g_chart = cui_last_id(ui);
}

static void test_area(void) {
    printf("dither area fills\n");
    cui_ctx *ui = new_ctx(520, 280, true);
    run(ui, build_area, NULL, 3);
    image im = render(ui);
    cui_rect r = cui_get_rect(ui, g_chart);
    /* the plot: 6 points under the top, 12 shorter than the chart without x labels */
    float py = r.y + 6, ph = r.h - 12;
    int red_low = 0, blue_low = 0, red_high = 0, blue_high = 0, near_line = 0, near_base = 0;
    for (float x = r.x + r.w * 0.3f; x < r.x + r.w * 0.7f; x += 1) {
        for (float y = py + ph * 0.60f; y < py + ph * 0.95f; y += 1) {
            const unsigned char *p = at(im, x, y);
            red_low += p[0] > 200 && p[2] < 80;
            blue_low += p[2] > 200 && p[0] < 80;
        }
        for (float y = py + ph * 0.10f; y < py + ph * 0.40f; y += 1) {
            const unsigned char *p = at(im, x, y);
            red_high += p[0] > 200 && p[2] < 80;
            blue_high += p[2] > 200 && p[0] < 80;
        }
        for (float y = py + ph * 0.53f; y < py + ph * 0.60f; y += 1) near_line += at(im, x, y)[0] > 200;
        for (float y = py + ph * 0.90f; y < py + ph * 0.97f; y += 1) near_base += at(im, x, y)[0] > 200;
    }
    /* each band holds only its own color: the bands never mix */
    CHECK(red_low > 100); CHECK_EQ(blue_low, 0);
    CHECK(blue_high > 100); CHECK_EQ(red_high, 0);
    /* and thins out from its line down to its base */
    CHECK(near_line > near_base * 2);
    free(im.px);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* veil                                                                     */

typedef struct { bool shown, toggle, changed, pressed; int presses; cui_id veil; } veil_case;
static void build_veil(cui_ctx *ui, void *u) {
    veil_case *c = u;
    cui_veil_opts o;
    memset(&o, 0, sizeof o);
    o.toggle = c->toggle;
    o.label = "Hidden";
    c->changed = cui_veil_begin_ex(ui, "v", &c->shown, &o);
    cui_box(ui, .width = CUI_PX(200), .height = CUI_PX(100), .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) {
        if (cui_button(ui, "Inside")) c->presses++;
    }
    cui_veil_end(ui);
    c->veil = cui_last_id(ui);
}

static void test_veil(void) {
    printf("veil\n");
    cui_ctx *ui = new_ctx(400, 300, true);
    veil_case c;
    memset(&c, 0, sizeof c);
    run(ui, build_veil, &c, 3);
    cui_rect r = cui_get_rect(ui, c.veil);
    CHECK(r.w == 200 && r.h == 100);   /* the content keeps its space */
    float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f;
    /* covered: the click reveals, and the button under the veil does not get it */
    move(ui, build_veil, &c, cx, cy);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_veil, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_veil, &c, 1);
    CHECK(c.shown);
    CHECK(c.changed);
    CHECK_EQ(c.presses, 0);
    run(ui, build_veil, &c, 1);
    CHECK(!c.changed);
    /* revealed: the content takes clicks, and without toggle the veil stays off */
    click(ui, build_veil, &c, cx, cy);
    CHECK_EQ(c.presses, 1);
    click(ui, build_veil, &c, r.x + 4, r.y + 4);
    CHECK(c.shown);
    /* toggle: a click beside the content covers it again */
    c.toggle = true;
    click(ui, build_veil, &c, r.x + 4, r.y + 4);
    CHECK(!c.shown);
    /* the app hides and shows it too */
    c.shown = true; run(ui, build_veil, &c, 2);
    image open = render(ui);
    c.shown = false; run(ui, build_veil, &c, 2);
    image shut = render(ui);
    CHECK(!same(open, shut));
    free(open.px);
    /* reduced motion: the noise holds still */
    run(ui, build_veil, &c, 30);
    image later = render(ui);
    CHECK(same(shut, later));
    free(shut.px); free(later.px);
    cui_destroy(ui);

    /* the keyboard: Tab to the veil, Enter reveals and Enter covers */
    ui = new_ctx(400, 300, true);
    memset(&c, 0, sizeof c);
    run(ui, build_veil, &c, 3);
    key(ui, build_veil, &c, CUI_KEY_TAB);
    key(ui, build_veil, &c, CUI_KEY_ENTER);
    CHECK(c.shown);
    key(ui, build_veil, &c, CUI_KEY_ENTER);
    CHECK(!c.shown);
    /* covered, the button inside is out of reach: Tab goes past it, and Enter reaches nothing */
    key(ui, build_veil, &c, CUI_KEY_TAB);
    key(ui, build_veil, &c, CUI_KEY_ENTER);
    CHECK_EQ(c.presses, 0);
    /* revealed, Tab reaches it */
    c.shown = true;
    run(ui, build_veil, &c, 2);
    key(ui, build_veil, &c, CUI_KEY_TAB);
    key(ui, build_veil, &c, CUI_KEY_ENTER);
    CHECK_EQ(c.presses, 1);
    cui_destroy(ui);

    /* with motion: the veil moves while covered, dissolves over about 0.28 s, then holds still */
    ui = new_ctx(400, 300, false);
    memset(&c, 0, sizeof c);
    run(ui, build_veil, &c, 3);
    image a = render(ui);
    run(ui, build_veil, &c, 30);
    image b = render(ui);
    CHECK(!same(a, b));
    free(a.px); free(b.px);
    c.shown = true;
    run(ui, build_veil, &c, 6);
    a = render(ui);
    run(ui, build_veil, &c, 20);
    b = render(ui);
    run(ui, build_veil, &c, 30);
    image cdone = render(ui);
    CHECK(!same(a, b));
    CHECK(same(b, cdone));
    free(a.px); free(b.px); free(cdone.px);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* spinners                                                                 */

typedef struct { cui_spinner_style style; float size; cui_color color; bool legacy; float top; } spin_case;
static void build_spin(cui_ctx *ui, void *u) {
    spin_case *c = u;
    if (c->top > 0) cui_box(ui, .height = CUI_PX(c->top)) {}
    cui_row(ui) {
        cui_box(ui, .id = "spin") {
            if (c->legacy) (cui_spinner)(ui, c->size);
            else cui_spinner(ui, c->size, .style = c->style, .color = c->color);
        }
    }
}
/* pixels in r that differ from the background, and those outside r that do */
static void lit_count(image im, cui_rect r, int *in, int *out) {
    *in = *out = 0;
    const unsigned char *bg = at(im, 2, 2);
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++) {
            const unsigned char *p = at(im, (float)x, (float)y);
            if (abs(p[0] - bg[0]) + abs(p[1] - bg[1]) + abs(p[2] - bg[2]) < 24) continue;
            if (cui__rect_contains(r, x + 0.5f, y + 0.5f)) (*in)++;
            else (*out)++;
        }
}

static void test_spinners(void) {
    printf("spinners\n");
    /* the ring is the spinner it was, through the macro and through the function */
    cui_ctx *ui = new_ctx(200, 120, true);
    spin_case c = { CUI_SPINNER_ARC, 20, { 0 }, false, 0 };
    run(ui, build_spin, &c, 2);
    image a = render(ui);
    cui_destroy(ui);
    ui = new_ctx(200, 120, true);
    c.legacy = true;
    run(ui, build_spin, &c, 2);
    image b = render(ui);
    CHECK(same(a, b));
    free(a.px); free(b.px);
    cui_destroy(ui);

    /* every dither style draws dots inside its box and nowhere else, at a full and an inline size,
     * and holds still under reduced motion without asking for frames */
    static const float sizes[2] = { 56, 14 };
    static const int widths[2][15] = { { 56, 56, 56, 56, 56, 56, 44, 44, 88, 56, 80, 49, 56, 56, 56 },
                                       { 14, 14, 14, 14, 14, 14, 11, 11, 22, 14, 20, 12, 14, 14, 14 } };
    for (int si = 0; si < 2; si++)
        for (int st = CUI_SPINNER_COMET; st <= CUI_SPINNER_PLASMA; st++) {
            ui = new_ctx(200, 120, true);
            spin_case sc = { (cui_spinner_style)st, sizes[si], { 0 }, false, 0 };
            run(ui, build_spin, &sc, 2);
            cui_rect r = cui__prev_rect(ui, cui_make_id(ui, "spin"));
            CHECK_EQ((int)r.w, widths[si][st]);
            CHECK_EQ((int)r.h, (int)sizes[si]);
            image im = render(ui);
            int in, out;
            lit_count(im, r, &in, &out);
            if (in == 0 || out != 0) printf("  style %d at %g: %d in, %d out\n", st, sizes[si], in, out);
            CHECK(in > 0);
            CHECK_EQ(out, 0);
            CHECK(cui_next_wakeup(ui) < 0);
            run(ui, build_spin, &sc, 40);
            image later = render(ui);
            CHECK(same(im, later));
            free(im.px); free(later.px);
            cui_destroy(ui);
        }

    /* inline sizes draw one point dots: some 2x2 pixel blocks are not one color */
    ui = new_ctx(200, 120, true);
    c = (spin_case){ CUI_SPINNER_COMET, 14, { 0 }, false, 0 };
    cui_set_viewport(ui, 200, 120, 2);
    run(ui, build_spin, &c, 2);
    {
        const cui_draw_data *dd = cui_get_draw_data(ui);
        image im = { malloc((size_t)dd->width_px * dd->height_px * 4), dd->width_px, dd->height_px };
        cui_sw_render(g_sw, dd, im.px, im.w, im.h, im.w * 4, CUI_SW_RGBA);
        /* at scale 2 a one point dot is two pixels wide: colors change only between even and odd
         * pixels, and on both kinds of even boundary (two point dots would change on one only) */
        int odd = 0, even[2] = { 0, 0 };
        for (int y = 0; y < im.h; y++)
            for (int x = 1; x < im.w; x++) {
                bool diff = at(im, (float)x - 1, (float)y)[0] != at(im, (float)x, (float)y)[0];
                if (x & 1) odd += diff;
                else even[(x >> 1) & 1] += diff;
            }
        CHECK_EQ(odd, 0);
        CHECK(even[0] > 0 && even[1] > 0);
        free(im.px);
    }
    cui_destroy(ui);

    /* the color option: dots in the danger color */
    ui = new_ctx(200, 120, true);
    c = (spin_case){ CUI_SPINNER_PULSE, 56, CUI_COLOR_DANGER, false, 0 };
    run(ui, build_spin, &c, 2);
    {
        image im = render(ui);
        cui_rect r = cui__prev_rect(ui, cui_make_id(ui, "spin"));
        int red = 0;
        for (float y = r.y; y < r.y + r.h; y++)
            for (float x = r.x; x < r.x + r.w; x++) red += at(im, x, y)[0] > at(im, x, y)[1] + 60;
        CHECK(red > 20);
        free(im.px);
    }
    cui_destroy(ui);

    /* with motion: a wakeup at the next step, not every frame, and dots that move */
    static const float fps[15] = { 0, 30, 15, 20, 24, 12, 15, 15, 15, 30, 30, 15, 15, 20, 12 };
    for (int st = CUI_SPINNER_COMET; st <= CUI_SPINNER_PLASMA; st++) {
        ui = new_ctx(200, 120, false);
        spin_case sc = { (cui_spinner_style)st, 56, { 0 }, false, 0 };
        run(ui, build_spin, &sc, 30);
        float w = cui_next_wakeup(ui);
        CHECK(w >= 0 && w <= 1.0f / fps[st] + 1e-4f);
        image im = render(ui);
        run(ui, build_spin, &sc, 15);
        image later = render(ui);
        if (same(im, later)) printf("  style %d does not move\n", st);
        CHECK(!same(im, later));
        free(im.px); free(later.px);
        cui_destroy(ui);
    }

    /* a spinner below the window paints nothing and asks for no frames */
    ui = new_ctx(200, 120, false);
    c = (spin_case){ CUI_SPINNER_COMET, 56, { 0 }, false, 400 };
    run(ui, build_spin, &c, 10);
    CHECK(cui_next_wakeup(ui) < 0);
    cui_destroy(ui);

    /* no state: trails come from the time alone, so a context that jumped to the same time agrees */
    image imgs[2];
    for (int k = 0; k < 2; k++) {
        ui = new_ctx(200, 120, false);
        c = (spin_case){ CUI_SPINNER_ORBIT, 56, { 0 }, false, 0 };
        g_dt = k ? 37.0f / 60.0f : 1.0f / 60.0f;
        run(ui, build_spin, &c, k ? 1 : 37);
        g_dt = 1.0f / 60.0f;
        imgs[k] = render(ui);
        cui_destroy(ui);
    }
    CHECK(same(imgs[0], imgs[1]));
    free(imgs[0].px); free(imgs[1].px);
}

/* ------------------------------------------------------------------------ */
/* status spinner                                                           */

typedef struct { cui_status status; bool keep, shown; } status_case;
static void build_status(cui_ctx *ui, void *u) {
    status_case *c = u;
    cui_row(ui) { cui_box(ui, .id = "st") { c->shown = cui_status_spinner(ui, "s", c->status, .size = 56, .keep = c->keep); } }
}
/* pixels in the spinner's box near the success (k 1) or danger (k 2) color, or near the background (k 0) */
static int status_px(cui_ctx *ui, image im, int k) {
    cui_rect r = cui__prev_rect(ui, cui_make_id(ui, "st"));
    cui_color want = k == 1 ? cui_color_of(ui, CUI_COLOR_SUCCESS) : k == 2 ? cui_color_of(ui, CUI_COLOR_DANGER) : cui_color_of(ui, CUI_COLOR_BACKGROUND);
    int n = 0;
    for (float y = r.y; y < r.y + r.h; y++)
        for (float x = r.x; x < r.x + r.w; x++) {
            const unsigned char *p = at(im, x, y);
            n += abs(p[0] - want.r) + abs(p[1] - want.g) + abs(p[2] - want.b) < 12;
        }
    return n;
}

static void test_status_spinner(void) {
    printf("status spinner\n");
    cui_ctx *ui = new_ctx(200, 120, false);
    status_case c = { CUI_STATUS_BUSY, false, false };
    run(ui, build_status, &c, 10);
    CHECK(c.shown);
    float w = cui_next_wakeup(ui);
    CHECK(w >= 0 && w <= 1.0f / 30 + 1e-4f);
    image im = render(ui);
    CHECK_EQ(status_px(ui, im, 1), 0);
    free(im.px);
    /* success: a green disc with a check cut into it, held, then gone */
    c.status = CUI_STATUS_SUCCESS;
    run(ui, build_status, &c, 6);
    im = render(ui);
    int early = status_px(ui, im, 1);
    free(im.px);
    run(ui, build_status, &c, 30);   /* half a second in: held */
    im = render(ui);
    int held = status_px(ui, im, 1), r = (int)(28 * 28 * 3.14159f);
    CHECK(early > 0 && early < held);
    CHECK(held > r / 2 && held < r);
    /* the check is cut out of the disc in the background color, inside the disc */
    cui_rect box = cui__prev_rect(ui, cui_make_id(ui, "st"));
    const unsigned char *mid = at(im, box.x + 26, box.y + 32);   /* on the check's short stroke */
    cui_color bg = cui_color_of(ui, CUI_COLOR_BACKGROUND);
    CHECK(abs(mid[1] - bg.g) < 12);
    free(im.px);
    /* while held it wakes once, at the end of the hold, not every step */
    w = cui_next_wakeup(ui);
    CHECK(w > 0.5f);
    run(ui, build_status, &c, 150);   /* past the fade */
    CHECK(!c.shown);
    CHECK(cui_next_wakeup(ui) < 0);
    im = render(ui);
    CHECK_EQ(status_px(ui, im, 1), 0);
    free(im.px);
    /* failure: the danger color */
    c.status = CUI_STATUS_ERROR;
    run(ui, build_status, &c, 40);
    im = render(ui);
    CHECK(status_px(ui, im, 2) > r / 2);
    free(im.px);
    /* keep: the mark stays and nothing wakes */
    c.keep = true;
    run(ui, build_status, &c, 200);
    CHECK(c.shown);
    CHECK(cui_next_wakeup(ui) < 0);
    /* idle shows nothing */
    c.status = CUI_STATUS_IDLE;
    run(ui, build_status, &c, 2);
    CHECK(!c.shown);
    im = render(ui);
    CHECK_EQ(status_px(ui, im, 2), 0);
    free(im.px);
    cui_destroy(ui);

    /* reduced motion: the result at once, then gone at once after the hold */
    ui = new_ctx(200, 120, true);
    c = (status_case){ CUI_STATUS_BUSY, false, false };
    run(ui, build_status, &c, 3);
    CHECK(cui_next_wakeup(ui) < 0);
    c.status = CUI_STATUS_SUCCESS;
    run(ui, build_status, &c, 1);
    im = render(ui);
    CHECK(status_px(ui, im, 1) > r / 2);
    free(im.px);
    w = cui_next_wakeup(ui);
    CHECK(w > 2.0f && w <= 2.4f);
    run(ui, build_status, &c, 150);
    CHECK(!c.shown);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* skeleton group                                                           */

static void build_skel(cui_ctx *ui, void *u) {
    (void)u;
    cui_box(ui, .width = CUI_PX(400)) {
        cui_skeleton_group(ui, .dither = true) {
            cui_row(ui, .gap = 0, .width = CUI_FILL) {
                cui_box(ui, .id = "sa", .width = CUI_PCT(50)) { cui_skeleton(ui, CUI_FILL, 24); }
                cui_box(ui, .id = "sb", .width = CUI_PCT(50)) { cui_skeleton(ui, CUI_FILL, 24); }
            }
        }
    }
}
/* pixels in the box of `id` brighter than the placeholder's base */
static int sheen_px(cui_ctx *ui, image im, const char *id) {
    cui_rect r = cui__prev_rect(ui, cui_make_id(ui, id));
    int base = at(im, r.x + r.w - 6, r.y + 12)[0], n = 0;
    for (float y = r.y + 2; y < r.y + r.h - 2; y++)
        for (float x = r.x + 8; x < r.x + r.w - 8; x++) n += at(im, x, y)[0] > base + 8;
    return n;
}

static void test_skeleton_group(void) {
    printf("skeleton group\n");
    /* held still: the band rests a third of the way across the group, so it lights the first
     * placeholder and not the second, which a sweep per placeholder would */
    cui_ctx *ui = new_ctx(480, 120, true);
    run(ui, build_skel, NULL, 2);
    image im = render(ui);
    CHECK(sheen_px(ui, im, "sa") > 20);
    CHECK_EQ(sheen_px(ui, im, "sb"), 0);
    CHECK(cui_next_wakeup(ui) < 0);
    run(ui, build_skel, NULL, 30);
    image later = render(ui);
    CHECK(same(im, later));
    free(im.px); free(later.px);
    cui_destroy(ui);
    /* moving: twenty steps a second, and the band travels */
    ui = new_ctx(480, 120, false);
    run(ui, build_skel, NULL, 20);
    float w = cui_next_wakeup(ui);
    CHECK(w >= 0 && w <= 1.0f / 20 + 1e-4f);
    im = render(ui);
    run(ui, build_skel, NULL, 12);
    later = render(ui);
    CHECK(!same(im, later));
    free(im.px); free(later.px);
    cui_destroy(ui);
}

int main(void) {
    g_sw = cui_sw_create();
    test_primitive();
    test_cells();
    test_progress();
    test_meter();
    test_area();
    test_veil();
    test_spinners();
    test_status_spinner();
    test_skeleton_group();
    cui_sw_destroy(g_sw);
    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
