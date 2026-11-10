/* Tests for the instrument components: the dot-matrix chart, chart annotations, the event
 * track, the readout and the dot-matrix display. Runs headless with the software renderer.
 * Behaviour is checked through the results the components return; drawing is checked by
 * rendering into memory and looking at the pixels where dots, lines and glyphs must be.
 *
 *   cui_test_instrument [image-dir]   with a directory, also writes docs images there */
#include <cui/cui.h>
#include <cui/cui_render.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failed, g_checks;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { g_checks++; long long a_ = (long long)(a), b_ = (long long)(b); if (a_ != b_) { g_failed++; \
    printf("  FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)
#define CHECK_NEAR(a, b, eps) do { g_checks++; double a_ = (double)(a), b_ = (double)(b); if (!(fabs(a_ - b_) <= (eps))) { g_failed++; \
    printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)

static cui_sw_renderer *g_sw;

static cui_ctx *new_ctx(float w, float h, bool dark) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT });
    cui_set_viewport(ui, w, h, 1);
    cui_set_reduced_motion(ui, true);
    cui_input_mouse_leave(ui);
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 20, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND) fn(ui, user);
        cui_end_frame(ui);
    }
}
static void move(cui_ctx *ui, build_fn fn, void *u, float x, float y) { cui_input_mouse_move(ui, x, y); run(ui, fn, u, 1); }
static void press(cui_ctx *ui, build_fn fn, void *u) { cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1); }
static void release(cui_ctx *ui, build_fn fn, void *u) { cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1); }
static void click(cui_ctx *ui, build_fn fn, void *u, float x, float y) { move(ui, fn, u, x, y); press(ui, fn, u); release(ui, fn, u); }
static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods); cui_input_key(ui, k, false, mods);
    run(ui, fn, u, 1);
}

/* the frame rendered into memory, at scale 1, so a point is a pixel */
typedef struct { unsigned char *px; int w, h; } image;
static image render(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    image im = { malloc((size_t)dd->width_px * dd->height_px * 4), dd->width_px, dd->height_px };
    cui_sw_render(g_sw, dd, im.px, im.w, im.h, im.w * 4, CUI_SW_RGBA);
    return im;
}
static int lum(image im, int x, int y) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return -1;
    const unsigned char *p = im.px + ((size_t)y * im.w + x) * 4;
    return (p[0] * 3 + p[1] * 6 + p[2]) / 10;
}
static int CUI__MAXI(int a, int b) { return a > b ? a : b; }
/* the brightest pixel in a small square: dots are antialiased, so a center may not be exact */
static int peak(image im, int cx, int cy, int r) {
    int best = 0;
    for (int y = cy - r; y <= cy + r; y++) for (int x = cx - r; x <= cx + r; x++) best = CUI__MAXI(best, lum(im, x, y));
    return best;
}

/* ------------------------------------------------------------------------ */
/* dot-matrix chart                                                         */

typedef struct {
    float a[49], b[49];
    int ns, n;
    cui_dot_chart_opts o;
    cui_dot_chart_result r;
    int clicks;
    double last_click;
} dot_case;

static void build_dots(cui_ctx *ui, void *u) {
    dot_case *c = u;
    cui_dot_series s[2] = { { c->a, "A", false }, { c->b, "B", true } };
    c->r = cui_dot_chart_ex(ui, "dots", s, c->ns, c->n, &c->o);
    if (c->r.clicked) { c->clicks++; c->last_click = c->r.x; }
}

static void test_dot_chart(void) {
    printf("dot chart: hover readout, x mapping, click, keyboard cursor, stacking, NaN, empty, sizes\n");
    cui_ctx *ui = new_ctx(560, 320, false);
    static dot_case c;
    memset(&c, 0, sizeof c);
    for (int i = 0; i < 49; i++) { c.a[i] = (float)i; c.b[i] = 10; }
    c.ns = 2; c.n = 49;
    c.o = (cui_dot_chart_opts){ ._ = 0, .height = 240, .x_min = 6, .x_max = 30, .x_units = CUI_AXIS_HOURS, .min = 0, .max = 60, .unit = "MG" };
    run(ui, build_dots, &c, 3);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "dots"));
    CHECK_NEAR(R.w, 520, 0.5);
    CHECK_NEAR(R.h, 240, 0.5);
    CHECK_EQ(c.r.hovered, -1);
    /* the plot: the right gutter for labels and the row of x labels are not part of it */
    float pw = R.w - 34, ph = R.h - 18;
    move(ui, build_dots, &c, R.x + pw * 0.5f, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, 24);
    CHECK_NEAR(c.r.x, 18.0, 1e-9);                /* the middle of 06:00 to 30:00 */
    CHECK_NEAR(c.r.value, 24 + 10, 1e-4);         /* the stack: both series */
    move(ui, build_dots, &c, R.x + 1, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, 0);
    move(ui, build_dots, &c, R.x + pw - 1, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, 48);
    CHECK_NEAR(c.r.value, 58, 1e-4);
    /* the gutter and the label row are outside the plot: no readout there */
    move(ui, build_dots, &c, R.x + pw + 10, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, -1);
    move(ui, build_dots, &c, R.x + pw * 0.5f, R.y + ph + 8);
    CHECK_EQ(c.r.hovered, -1);
    /* a click returns the pointer's own x, between samples */
    click(ui, build_dots, &c, R.x + pw * 0.25f, R.y + ph * 0.5f);
    CHECK_EQ(c.clicks, 1);
    CHECK_NEAR(c.last_click, 6 + 24 * 0.25, 0.05);
    /* the keys: the chart takes focus, the arrows move a cursor along the samples */
    move(ui, build_dots, &c, 2, 2);
    CHECK_EQ(c.r.hovered, -1);
    key(ui, build_dots, &c, CUI_KEY_TAB, 0);
    key(ui, build_dots, &c, CUI_KEY_RIGHT, 0);
    CHECK_EQ(c.r.hovered, 25);                    /* from the middle, one to the right */
    key(ui, build_dots, &c, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_EQ(c.r.hovered, 35);
    key(ui, build_dots, &c, CUI_KEY_END, 0);
    CHECK_EQ(c.r.hovered, 48);
    key(ui, build_dots, &c, CUI_KEY_RIGHT, 0);
    CHECK_EQ(c.r.hovered, 48);                    /* stays at the end */
    key(ui, build_dots, &c, CUI_KEY_HOME, 0);
    CHECK_EQ(c.r.hovered, 0);
    CHECK_NEAR(c.r.x, 6, 1e-9);
    int before = c.clicks;
    key(ui, build_dots, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.clicks, before + 1);
    CHECK_NEAR(c.last_click, 6, 1e-9);            /* Enter clicks at the cursor */
    key(ui, build_dots, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r.hovered, -1);
    /* bad values count as zero; NaN and infinity do not spoil the scale */
    c.a[10] = NAN; c.a[11] = -50; c.a[12] = INFINITY;
    c.o.min = c.o.max = 0;
    run(ui, build_dots, &c, 1);
    move(ui, build_dots, &c, R.x + pw * 10 / 48.0f, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, 10);
    CHECK_NEAR(c.r.value, 10, 1e-4);
    move(ui, build_dots, &c, R.x + pw * 12 / 48.0f, R.y + ph * 0.5f);
    CHECK_NEAR(c.r.value, 10, 1e-4);
    /* nothing to draw: no series, no samples, NULL values */
    c.ns = 0; c.n = 0;
    run(ui, build_dots, &c, 2);
    move(ui, build_dots, &c, R.x + pw * 0.5f, R.y + ph * 0.5f);
    CHECK_EQ(c.r.hovered, -1);
    cui_begin_frame(ui, 1 / 60.0f);
    cui_dot_chart_result e = cui_dot_chart(ui, "null", NULL, 3, 10);
    cui_dot_chart(ui, "neg", NULL, -1, -5);
    cui_dot_series nulls[1] = { { NULL, NULL, false } };
    cui_dot_chart(ui, "nullvals", nulls, 1, 10, .height = 4, .pitch = 1000);
    cui_dot_series one[1] = { { c.b, "B", false } };
    cui_dot_chart(ui, "one", one, 1, 1, .pitch = -3);
    cui_end_frame(ui);
    CHECK_EQ(e.hovered, -1);
    CHECK(!e.clicked);
    cui_destroy(ui);
}

/* pixels: a full column is lit to the top, an empty one only shows its dim grid, and the
 * accent series is drawn in the accent color */
typedef struct { float v[2][2]; cui_dot_chart_opts o; } dot_pix;
static void build_dot_pix(cui_ctx *ui, void *u) {
    dot_pix *p = u;
    cui_dot_series s[2] = { { p->v[0], "lo", false }, { p->v[1], "hi", true } };
    cui_dot_chart_ex(ui, "pix", s, 2, 2, &p->o);
}

static void test_dot_pixels(void) {
    printf("dot chart pixels: lit up to the value, dim above it, the accent band, no grid\n");
    cui_ctx *ui = new_ctx(400, 260, true);
    cui_set_accent(ui, CUI_ACCENT_BLUE);
    static dot_pix p;
    /* the left half is half full of the first series, the right half has both, to the top */
    p.v[0][0] = 50; p.v[0][1] = 50;
    p.v[1][0] = 0;  p.v[1][1] = 50;
    p.o = (cui_dot_chart_opts){ ._ = 0, .height = 220, .min = 0, .max = 100, .pitch = 10, .no_axis = true, .no_readout = true };
    run(ui, build_dot_pix, &p, 3);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "pix"));
    image im = render(ui);
    /* the grid of 10 point cells: columns from the left, rows from the bottom */
    int cols = (int)((R.w - 34) / 10), rows = (int)(R.h / 10);
    float gx = R.x + ((R.w - 34) - cols * 10) * 0.5f + 5, gy = R.y + R.h - (R.h - rows * 10) * 0.5f - 5;
    int bg = lum(im, (int)R.x + 2, (int)R.y + 2);
    int low_left = peak(im, (int)gx, (int)(gy - 2 * 10), 2);           /* a lit dot of the first series */
    int top_left = peak(im, (int)gx, (int)(gy - (rows - 1) * 10), 2);   /* above the value: dim */
    int top_right = peak(im, (int)(gx + (cols - 1) * 10), (int)(gy - (rows - 2) * 10), 2);
    CHECK(low_left > bg + 120);
    CHECK(top_left < low_left - 80);
    CHECK(top_left > bg);                                                /* the dim grid shows */
    /* the top right belongs to the accent series: blue, not grey */
    {
        int x = (int)(gx + (cols - 1) * 10), y = (int)(gy - (rows - 2) * 10);
        const unsigned char *px = im.px + ((size_t)y * im.w + x) * 4;
        CHECK(top_right > bg + 60);
        CHECK(px[2] > px[0] + 40);
    }
    free(im.px);
    p.o.no_grid = true;
    run(ui, build_dot_pix, &p, 1);
    im = render(ui);
    CHECK(peak(im, (int)gx, (int)(gy - (rows - 1) * 10), 2) <= bg + 2);   /* no grid: nothing above the value */
    free(im.px);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* annotations                                                              */

typedef struct { cui_chart_annotation a[30]; int n; bool line; } ann_case;
static void build_ann(cui_ctx *ui, void *u) {
    ann_case *c = u;
    static float zero[11];
    if (c->line) {
        static float vals[5] = { 0, 25, 50, 75, 100 };
        cui_series s[1] = { { "x", vals, 5, { 0 } } };
        cui_line_chart(ui, "lc", s, 1, .height = 200, .min = 0, .max = 100, .annotations = c->a, .annotation_count = c->n);
        cui_bar_chart(ui, "bc", s, 1, .height = 120, .annotations = c->a, .annotation_count = c->n);
    } else {
        cui_dot_series s[1] = { { zero, NULL, false } };
        cui_dot_chart(ui, "an", s, 1, 11, .height = 220, .x_min = 0, .x_max = 10, .min = 0, .max = 100,
                      .no_grid = true, .no_axis = true, .no_readout = true, .annotations = c->a, .annotation_count = c->n);
    }
}

static void test_annotations(void) {
    printf("annotations: vertical and dashed lines where their values are, labels in the headroom, out of range, many\n");
    cui_ctx *ui = new_ctx(480, 300, true);
    static ann_case c;
    memset(&c, 0, sizeof c);
    c.a[0] = (cui_chart_annotation){ CUI_ANNOTATION_X, 5, 0, "MIDDLE", "05", false };
    c.a[1] = (cui_chart_annotation){ CUI_ANNOTATION_Y, 0, 25, "25", NULL, false };
    c.n = 2;
    run(ui, build_ann, &c, 3);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "an"));
    image im = render(ui);
    int bg = lum(im, (int)R.x + 2, (int)(R.y + R.h - 3));
    /* the label's two lines take the top of the box: the plot starts below them */
    float head = R.h - 0;
    {
        /* find the vertical line: the brightest column across the lower plot */
        int bestx = -1, best = 0;
        for (int x = (int)R.x; x < (int)(R.x + R.w - 34); x++) {
            int v = lum(im, x, (int)(R.y + R.h - 20));
            if (v > best) { best = v; bestx = x; }
        }
        float want = R.x + (R.w - 34) * 0.5f;
        CHECK(best > bg + 30);
        CHECK(fabsf(bestx - want) <= 1.5f);
        (void)head;
    }
    {
        /* the dashed line at 25 of 0..100: a row with dashes, a quarter up the plot */
        int found = 0;
        for (int y = (int)(R.y + R.h * 0.6f); y < (int)(R.y + R.h - 2); y++) {
            int on = 0;
            for (int x = (int)R.x + 2; x < (int)(R.x + R.w - 40); x++) on += lum(im, x, y) > bg + 25;
            if (on > 60 && on < (int)(R.w - 40) * 0.8f) { found = y; break; }
        }
        CHECK(found > 0);
        /* the plot is the box less the headroom; 25% up from its bottom */
        float fs_lines = 0; (void)fs_lines;
        CHECK(found > R.y + R.h * 0.7f);
    }
    {
        /* the label: text pixels above the plot, right of the line */
        int text = 0;
        for (int y = (int)R.y; y < (int)R.y + 14; y++)
            for (int x = (int)(R.x + (R.w - 34) * 0.5f + 4); x < (int)(R.x + (R.w - 34) * 0.5f + 60); x++) text += lum(im, x, y) > bg + 60;
        CHECK(text > 20);
        /* the "25" sits in the gutter */
        int glabel = 0;
        for (int y = (int)R.y; y < (int)(R.y + R.h); y++)
            for (int x = (int)(R.x + R.w - 30); x < (int)(R.x + R.w); x++) glabel += lum(im, x, y) > bg + 40;
        CHECK(glabel > 8);
    }
    free(im.px);
    /* outside the range, NULL labels, more than fit: drawn or skipped, never a crash */
    c.n = 30;
    for (int i = 2; i < 30; i++) c.a[i] = (cui_chart_annotation){ (cui_annotation_kind)(i % 3), (double)(i % 13) - 1, (double)(i * 9 % 130) - 10, i % 4 ? "LABEL" : NULL, i % 2 ? "DETAIL" : NULL, i % 5 == 0 };
    run(ui, build_ann, &c, 2);
    im = render(ui);
    free(im.px);
    /* the same annotations on line and bar charts */
    c.line = true;
    c.n = 2;
    c.a[0].x = 2;   /* the third point of five */
    run(ui, build_ann, &c, 3);
    cui_rect L = cui_get_rect(ui, cui_make_id(ui, "lc"));
    CHECK(L.w > 100);
    im = render(ui);
    free(im.px);
    c.n = 30;
    run(ui, build_ann, &c, 2);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* event track                                                              */

typedef struct {
    double t[8]; int n, cap, sel;
    cui_event_track_opts o;
    cui_event_track_result r;
    int changes, adds, removes;
} ev_case;

static void build_ev(cui_ctx *ui, void *u) {
    ev_case *c = u;
    c->o.selected = &c->sel;
    c->r = cui_event_track_ex(ui, "ev", c->t, &c->n, c->cap, 0, 24, &c->o);
    if (c->r.changed) c->changes++;
    if (c->r.added >= 0) c->adds++;
    if (c->r.removed >= 0) c->removes++;
}

static float ev_x(cui_rect R, double t) { return R.x + (float)(t / 24.0) * (R.w - 34); }

static void test_event_track(void) {
    printf("event track: add, drag with snapping and reordering, remove, keys, capacity, unsorted input\n");
    cui_ctx *ui = new_ctx(560, 200, false);
    static ev_case c;
    memset(&c, 0, sizeof c);
    c.t[0] = 6; c.t[1] = 12; c.n = 2; c.cap = 4; c.sel = -1;
    c.o = (cui_event_track_opts){ ._ = 0, .step = 0.5, .units = CUI_AXIS_HOURS };
    run(ui, build_ev, &c, 3);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "ev"));
    CHECK_NEAR(R.h, 54, 0.5);
    float dy = R.y + 12;
    /* a click on an empty spot adds an event there, snapped, and keeps the array sorted */
    click(ui, build_ev, &c, ev_x(R, 9.1), dy);
    CHECK_EQ(c.n, 3);
    CHECK_NEAR(c.t[1], 9.0, 1e-9);
    CHECK_EQ(c.sel, 1);
    CHECK_EQ(c.adds, 1);
    CHECK(c.t[0] < c.t[1] && c.t[1] < c.t[2]);
    /* a press on an event selects it; dragging moves it past its neighbour, and the
     * selection follows it to its new place */
    move(ui, build_ev, &c, ev_x(R, 6), dy);
    press(ui, build_ev, &c);
    CHECK_EQ(c.sel, 0);
    move(ui, build_ev, &c, ev_x(R, 10.2), dy);
    CHECK(c.r.dragging);
    CHECK_NEAR(c.t[1], 10.0, 1e-9);
    CHECK_EQ(c.sel, 1);
    CHECK_NEAR(c.t[0], 9.0, 1e-9);
    move(ui, build_ev, &c, ev_x(R, 30), dy);         /* past the end: clamped */
    CHECK_NEAR(c.t[c.n - 1], 24.0, 1e-9);
    CHECK_EQ(c.sel, c.n - 1);
    release(ui, build_ev, &c);
    CHECK(!c.r.dragging);
    CHECK_EQ(c.n, 3);
    /* a press that does not move does not change anything */
    int changes = c.changes;
    click(ui, build_ev, &c, ev_x(R, 12), dy);
    CHECK_EQ(c.changes, changes);
    CHECK_EQ(c.sel, 1);
    /* a double-click on an event removes it (after a pause, so the click before does not count) */
    run(ui, build_ev, &c, 30);
    move(ui, build_ev, &c, ev_x(R, 12), dy);
    press(ui, build_ev, &c); release(ui, build_ev, &c); press(ui, build_ev, &c); release(ui, build_ev, &c);
    CHECK_EQ(c.n, 2);
    CHECK_EQ(c.removes, 1);
    CHECK_NEAR(c.t[0], 9.0, 1e-9);
    CHECK_NEAR(c.t[1], 24.0, 1e-9);
    /* capacity: clicks add until it is full and no further */
    click(ui, build_ev, &c, ev_x(R, 3), dy);
    click(ui, build_ev, &c, ev_x(R, 15), dy);
    CHECK_EQ(c.n, 4);
    click(ui, build_ev, &c, ev_x(R, 19), dy);
    CHECK_EQ(c.n, 4);
    /* no_add and no_remove */
    c.n = 1; c.t[0] = 12; c.sel = -1;
    c.o.no_add = true; c.o.no_remove = true;
    run(ui, build_ev, &c, 1);
    click(ui, build_ev, &c, ev_x(R, 4), dy);
    CHECK_EQ(c.n, 1);
    move(ui, build_ev, &c, ev_x(R, 12), dy);
    press(ui, build_ev, &c); release(ui, build_ev, &c); press(ui, build_ev, &c); release(ui, build_ev, &c);
    CHECK_EQ(c.n, 1);
    c.o.no_add = false; c.o.no_remove = false;

    /* the keys */
    c.t[0] = 6; c.t[1] = 12; c.t[2] = 18; c.n = 3; c.sel = -1;
    run(ui, build_ev, &c, 1);
    move(ui, build_ev, &c, 2, 190);
    key(ui, build_ev, &c, CUI_KEY_TAB, 0);
    key(ui, build_ev, &c, CUI_KEY_DOWN, 0);
    CHECK_EQ(c.sel, 0);
    key(ui, build_ev, &c, CUI_KEY_DOWN, 0);
    CHECK_EQ(c.sel, 1);
    key(ui, build_ev, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.t[1], 12.5, 1e-9);
    key(ui, build_ev, &c, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_NEAR(c.t[1], 17.5, 1e-9);
    key(ui, build_ev, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_ev, &c, CUI_KEY_RIGHT, 0);   /* to 18.5: past the third, which keeps its place in the order */
    CHECK_NEAR(c.t[2], 18.5, 1e-9);
    CHECK_EQ(c.sel, 2);
    CHECK_NEAR(c.t[1], 18.0, 1e-9);
    key(ui, build_ev, &c, CUI_KEY_UP, 0);
    CHECK_EQ(c.sel, 1);
    key(ui, build_ev, &c, CUI_KEY_HOME, 0);
    CHECK_EQ(c.sel, 0);
    key(ui, build_ev, &c, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK_NEAR(c.t[0], 1.0, 1e-9);
    key(ui, build_ev, &c, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK_NEAR(c.t[0], 0.0, 1e-9);              /* clamped at the start */
    int n0 = c.n;
    key(ui, build_ev, &c, CUI_KEY_ENTER, 0);      /* a new event one step after the selected one */
    CHECK_EQ(c.n, n0 + 1);
    CHECK_NEAR(c.t[1], 0.5, 1e-9);
    CHECK_EQ(c.sel, 1);
    key(ui, build_ev, &c, CUI_KEY_DELETE, 0);
    CHECK_EQ(c.n, n0);
    CHECK_EQ(c.sel, 1);                           /* the next one is selected */
    key(ui, build_ev, &c, CUI_KEY_END, 0);
    CHECK_EQ(c.sel, c.n - 1);
    key(ui, build_ev, &c, CUI_KEY_BACKSPACE, 0);
    CHECK_EQ(c.n, n0 - 1);
    key(ui, build_ev, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.sel, -1);
    CHECK_EQ(c.r.selected, -1);

    /* the caller's array arrives unsorted and out of range: sorted, clamped, and the
     * selection stays on its event */
    c.t[0] = 20; c.t[1] = -5; c.t[2] = 8; c.n = 3; c.sel = 0;
    run(ui, build_ev, &c, 1);
    CHECK_NEAR(c.t[0], 0, 1e-9);
    CHECK_NEAR(c.t[1], 8, 1e-9);
    CHECK_NEAR(c.t[2], 20, 1e-9);
    CHECK_EQ(c.sel, 2);
    /* a count past the capacity, a selection past the count, no array at all */
    c.n = 99; c.sel = 50;
    run(ui, build_ev, &c, 1);
    CHECK_EQ(c.n, c.cap);
    CHECK_EQ(c.sel, -1);
    cui_begin_frame(ui, 1 / 60.0f);
    int n = 5;
    cui_event_track_result r = cui_event_track(ui, "none", NULL, &n, 5, 0, 1);
    cui_event_track(ui, "nocount", c.t, NULL, 5, 0, 1);
    cui_event_track(ui, "badrange", c.t, &c.n, c.cap, 5, 5);
    cui_end_frame(ui);
    CHECK_EQ(n, 0);
    CHECK_EQ(r.selected, -1);
    cui_destroy(ui);
}

/* the track and a dot chart stacked at one width put the same x in the same place */
typedef struct { double t[2]; int n; float v[25]; cui_dot_chart_result r; } align_case;
static void build_align(cui_ctx *ui, void *u) {
    align_case *c = u;
    cui_dot_series s[1] = { { c->v, NULL, false } };
    cui_column(ui, .width = CUI_FILL, .gap = 0) {
        c->r = cui_dot_chart(ui, "chart", s, 1, 25, .x_min = 0, .x_max = 24, .no_axis = true);
        cui_event_track(ui, "track", c->t, &c->n, 2, 0, 24);
    }
}
static void test_alignment(void) {
    printf("dot chart and event track: one x scale\n");
    cui_ctx *ui = new_ctx(640, 360, false);
    static align_case c = { { 6, 18 }, 2, { 0 } };
    for (int i = 0; i < 25; i++) c.v[i] = 5;
    run(ui, build_align, &c, 3);
    cui_rect ch = cui_get_rect(ui, cui_make_id(ui, "chart")), tr = cui_get_rect(ui, cui_make_id(ui, "track"));
    CHECK_NEAR(ch.x, tr.x, 0.01);
    CHECK_NEAR(ch.w, tr.w, 0.01);
    /* hovering the chart straight above each event reads the sample at the event's time */
    for (int k = 0; k < 2; k++) {
        move(ui, build_align, &c, ev_x(tr, c.t[k]), ch.y + ch.h * 0.5f);
        CHECK_EQ(c.r.hovered, (int)c.t[k]);
        CHECK_NEAR(c.r.x, c.t[k], 1e-9);
    }
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* readout                                                                  */

typedef struct { cui_control_size size; const char *unit, *detail; cui_text_align align; } ro_case;
static void build_ro(cui_ctx *ui, void *u) {
    ro_case *c = u;
    cui_row(ui) cui_box(ui, .id = "box") cui_readout(ui, "AT BED", "54", .unit = c->unit, .detail = c->detail, .size = c->size, .align = c->align);
    cui_box(ui, .id = "wide", .width = CUI_PX(300)) cui_readout(ui, "PEAK", "203", .unit = "MG", .align = c->align, .width = CUI_FILL);
}
static void test_readout(void) {
    printf("readout: sizes, unit, detail, empty values\n");
    cui_ctx *ui = new_ctx(500, 400, false);
    static ro_case c;
    float h[3];
    cui_control_size sizes[3] = { CUI_SIZE_SM, CUI_SIZE_MD, CUI_SIZE_LG };
    for (int k = 0; k < 3; k++) {
        c = (ro_case){ sizes[k], "MG", NULL, CUI_TEXT_START };
        run(ui, build_ro, &c, 2);
        h[k] = cui_get_rect(ui, cui_make_id(ui, "box")).h;
    }
    CHECK(h[0] < h[1] && h[1] < h[2]);
    c = (ro_case){ CUI_SIZE_MD, "MG", NULL, CUI_TEXT_START };
    run(ui, build_ro, &c, 2);
    cui_rect a = cui_get_rect(ui, cui_make_id(ui, "box"));
    c.unit = NULL;
    run(ui, build_ro, &c, 2);
    cui_rect b = cui_get_rect(ui, cui_make_id(ui, "box"));
    CHECK(a.w > b.w);                              /* the unit adds width */
    c.detail = "UNDER THE LINE BY 00:10";
    run(ui, build_ro, &c, 2);
    cui_rect d = cui_get_rect(ui, cui_make_id(ui, "box"));
    CHECK(d.h > b.h + 10);                         /* the detail adds a line */
    c.align = CUI_TEXT_END;
    run(ui, build_ro, &c, 2);
    cui_begin_frame(ui, 1 / 60.0f);
    cui_readout(ui, NULL, NULL);
    cui_readout(ui, "", "", .unit = "", .detail = "");
    cui_readout(ui, "TONE", "1", .tone = CUI_TONE_DANGER, .accent = false);
    cui_readout(ui, "ACCENT", "2", .accent = true, .align = CUI_TEXT_CENTER);
    cui_end_frame(ui);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* dot-matrix display                                                       */

typedef struct { const char *text; cui_dot_display_opts o; cui_size box; } dd_case;
static void build_dd(cui_ctx *ui, void *u) {
    dd_case *c = u;
    cui_row(ui) cui_box(ui, .id = "dd", .width = c->box) cui_dot_display_ex(ui, c->text, &c->o);
}

static void test_dot_display(void) {
    printf("dot display: size, glyph dots, shrink to fit, other characters, marquee\n");
    cui_ctx *ui = new_ctx(600, 200, true);
    static dd_case c;
    c = (dd_case){ "A", { ._ = 0, .pitch = 10, .no_dim = true }, { 0 } };
    run(ui, build_dd, &c, 2);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "dd"));
    CHECK_NEAR(r.w, 50, 0.5);                      /* five columns of one glyph */
    CHECK_NEAR(r.h, 70, 0.5);                      /* seven rows */
    image im = render(ui);
    int bg = lum(im, (int)r.x - 4, (int)r.y - 4);
    /* "A": the top row is .###. and the fourth row is ##### */
    CHECK(peak(im, (int)r.x + 5, (int)r.y + 5, 1) <= bg + 10);
    for (int k = 1; k <= 3; k++) CHECK(peak(im, (int)r.x + 5 + k * 10, (int)r.y + 5, 1) > bg + 100);
    CHECK(peak(im, (int)r.x + 45, (int)r.y + 5, 1) <= bg + 10);
    for (int k = 0; k < 5; k++) CHECK(peak(im, (int)r.x + 5 + k * 10, (int)r.y + 35, 1) > bg + 100);
    /* the middle of the fifth row of "A" is empty: #...# */
    CHECK(peak(im, (int)r.x + 25, (int)r.y + 45, 1) <= bg + 10);
    free(im.px);
    /* width: six columns a glyph, less the last gap */
    c.text = "12:30";
    run(ui, build_dd, &c, 2);
    CHECK_NEAR(cui_get_rect(ui, cui_make_id(ui, "dd")).w, (5 * 6 - 1) * 10, 0.5);
    /* UTF-8: the degree sign and the middle dot are one glyph each, unknown characters a box */
    c.text = "20\xc2\xb0" "C\xc2\xb7\xe2\x9c\x93";
    run(ui, build_dd, &c, 2);
    CHECK_NEAR(cui_get_rect(ui, cui_make_id(ui, "dd")).w, (6 * 6 - 1) * 10, 0.5);
    /* in a narrower box the dots shrink: everything drawn stays inside it */
    c.text = "OVER THE LINE";
    c.o.width = CUI_FILL;
    c.box = CUI_PX(200);
    run(ui, build_dd, &c, 2);
    r = cui_get_rect(ui, cui_make_id(ui, "dd"));
    CHECK_NEAR(r.w, 200, 0.5);
    im = render(ui);
    int outside = 0, inside = 0;
    for (int y = (int)r.y; y < (int)(r.y + r.h); y++) {
        for (int x = (int)(r.x + r.w) + 2; x < (int)(r.x + r.w) + 60 && x < im.w; x++) outside += lum(im, x, y) > bg + 40;
        for (int x = (int)r.x; x < (int)(r.x + r.w); x++) inside += lum(im, x, y) > bg + 40;
    }
    CHECK_EQ(outside, 0);
    CHECK(inside > 100);
    free(im.px);
    /* marquee: asks for frames only while the text is wider than its box, and never with reduced motion */
    c.o.marquee = true;
    cui_set_reduced_motion(ui, false);
    run(ui, build_dd, &c, 3);
    CHECK(cui_next_wakeup(ui) > 0 && cui_next_wakeup(ui) < 0.1f);
    c.box = CUI_PX(590);
    c.text = "OK";
    run(ui, build_dd, &c, 3);
    CHECK(cui_next_wakeup(ui) < 0 || cui_next_wakeup(ui) > 0.1f);
    cui_set_reduced_motion(ui, true);
    c.box = CUI_PX(100); c.text = "A LONG LINE OF TEXT";
    run(ui, build_dd, &c, 3);
    CHECK(cui_next_wakeup(ui) < 0 || cui_next_wakeup(ui) > 0.1f);
    cui_begin_frame(ui, 1 / 60.0f);
    cui_dot_display(ui, NULL);
    cui_dot_display(ui, "", .pitch = -1);
    cui_dot_display(ui, "\xff\xfe broken \xe2", .pitch = 1000, .color = CUI_RGB(0xff0000), .align = CUI_TEXT_END);
    cui_end_frame(ui);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* right to left, phone width                                               */

typedef struct { double t[3]; int n; float v[13]; } rtl_case;
static void build_rtl(cui_ctx *ui, void *u) {
    rtl_case *c = u;
    cui_dot_series s[1] = { { c->v, NULL, false } };
    cui_dot_chart(ui, "chart", s, 1, 13, .x_min = 6, .x_max = 30, .x_units = CUI_AXIS_HOURS);
    cui_event_track(ui, "track", c->t, &c->n, 3, 6, 30, .units = CUI_AXIS_HOURS);
    cui_readout(ui, "PEAK", "203", .unit = "MG");
    cui_dot_display(ui, "LAST CUP BY 10:45", .width = CUI_FILL);
}
static void test_rtl_and_phone(void) {
    printf("right to left keeps the time axis left to right; a phone-width box keeps everything inside\n");
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .direction = CUI_DIRECTION_RTL });
    cui_set_viewport(ui, 360, 700, 1);
    cui_set_reduced_motion(ui, true);
    static rtl_case c = { { 8, 12, 16 }, 3, { 0 } };
    for (int i = 0; i < 13; i++) c.v[i] = (float)(i * (12 - i));
    run(ui, build_rtl, &c, 3);
    cui_rect ch = cui_get_rect(ui, cui_make_id(ui, "chart")), tr = cui_get_rect(ui, cui_make_id(ui, "track"));
    CHECK(ch.x >= 0 && ch.x + ch.w <= 360.5f);
    CHECK(tr.x >= 0 && tr.x + tr.w <= 360.5f);
    /* the earliest time is still on the left: a click near the left edge adds an early event */
    cui_input_mouse_move(ui, tr.x + 4, tr.y + 12);
    run(ui, build_rtl, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_rtl, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_rtl, &c, 1);
    CHECK_EQ(c.n, 3);                                /* full: capacity 3 */
    c.n = 2;
    run(ui, build_rtl, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_rtl, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_rtl, &c, 1);
    CHECK_EQ(c.n, 3);
    CHECK(c.t[0] < 7);
    cui_destroy(ui);
}

/* the side of a box where something is drawn: mean x of the bright pixels, as a share of the box */
static float lit_side(image im, cui_rect r, int bg) {
    double sx = 0; long n = 0;
    for (int y = (int)r.y; y < (int)(r.y + r.h); y++)
        for (int x = (int)r.x; x < (int)(r.x + r.w); x++) if (lum(im, x, y) > bg + 50) { sx += x; n++; }
    return n ? (float)((sx / n - r.x) / r.w) : -1;
}

typedef struct { int sel; double t[3]; int n; float v[25]; cui_dot_chart_result r; } dir_case;
static void build_dir(cui_ctx *ui, void *u) {
    dir_case *c = u;
    cui_box(ui, .id = "ro", .width = CUI_FILL) cui_readout(ui, "LABEL", "54", .unit = "MG", .width = CUI_FILL);
    cui_box(ui, .id = "dd", .width = CUI_FILL) cui_dot_display(ui, "12:30", .pitch = 4, .width = CUI_FILL);
    cui_box(ui, .id = "dde", .width = CUI_FILL) cui_dot_display(ui, "12:30", .pitch = 4, .width = CUI_FILL, .align = CUI_TEXT_END);
    cui_dot_series s[1] = { { c->v, NULL, false } };
    c->r = cui_dot_chart(ui, "chart", s, 1, 25, .x_min = 0, .x_max = 24, .height = 140);
    cui_event_track(ui, "track", c->t, &c->n, 3, 0, 24, .step = 1, .selected = &c->sel);
}

/* the same screen left to right and right to left: text-like parts follow the reading
 * direction, time axes do not, and the keys move along time either way */
static void test_directions(void) {
    printf("directions: readout and display placement follow the reading direction; chart and track keep time left to right\n");
    for (int rtl = 0; rtl < 2; rtl++) {
        cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = CUI_THEME_DARK,
                                                 .direction = rtl ? CUI_DIRECTION_RTL : CUI_DIRECTION_LTR });
        cui_set_viewport(ui, 600, 520, 1);
        cui_set_reduced_motion(ui, true);
        cui_input_mouse_leave(ui);
        static dir_case c;
        memset(&c, 0, sizeof c);
        c.t[0] = 6; c.t[1] = 18; c.n = 2; c.sel = 0;
        for (int i = 0; i < 25; i++) c.v[i] = 10;
        run(ui, build_dir, &c, 3);
        image im = render(ui);
        cui_rect ro = cui_get_rect(ui, cui_make_id(ui, "ro")), dd = cui_get_rect(ui, cui_make_id(ui, "dd")), dde = cui_get_rect(ui, cui_make_id(ui, "dde"));
        int bg = lum(im, 4, 4);
        float side_ro = lit_side(im, ro, bg), side_dd = lit_side(im, dd, bg), side_dde = lit_side(im, dde, bg);
        if (rtl) { CHECK(side_ro > 0.6f); CHECK(side_dd > 0.6f); CHECK(side_dde >= 0 && side_dde < 0.4f); }
        else     { CHECK(side_ro >= 0 && side_ro < 0.4f); CHECK(side_dd >= 0 && side_dd < 0.4f); CHECK(side_dde > 0.6f); }
        free(im.px);
        /* time runs left to right in both: the left of the chart is the first sample */
        cui_rect ch = cui_get_rect(ui, cui_make_id(ui, "chart")), tr = cui_get_rect(ui, cui_make_id(ui, "track"));
        move(ui, build_dir, &c, ch.x + 3, ch.y + 40);
        CHECK_EQ(c.r.hovered, 0);
        move(ui, build_dir, &c, ch.x + ch.w - 34 - 3, ch.y + 40);
        CHECK_EQ(c.r.hovered, 24);
        /* Right on the chart moves the cursor later, and on the track moves the event later */
        move(ui, build_dir, &c, 2, 515);
        click(ui, build_dir, &c, ch.x + (ch.w - 34) * 0.5f, ch.y + 40);   /* focus the chart */
        move(ui, build_dir, &c, 2, 515);
        key(ui, build_dir, &c, CUI_KEY_RIGHT, 0);
        CHECK_EQ(c.r.hovered, 13);
        move(ui, build_dir, &c, tr.x + (tr.w - 34) * 6 / 24.0f, tr.y + 12);
        press(ui, build_dir, &c); release(ui, build_dir, &c);              /* focus the track, select the event at 06 */
        key(ui, build_dir, &c, CUI_KEY_RIGHT, 0);
        CHECK_NEAR(c.t[0], 7, 1e-9);
        CHECK_EQ(c.sel, 0);
        cui_destroy(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* images for the docs                                                      */

typedef struct { double cups[4]; int n, sel; float curve[4][73]; } shot_state;
static double shot_cup(double t) {
    if (t <= 0) return 0;
    double ke = log(2.0) / 5, ka = log(2.0) / 0.25;
    return 95 * ka / (ka - ke) * (exp(-ke * t) - exp(-ka * t));
}
static void build_shot(cui_ctx *ui, void *u) {
    shot_state *s = u;
    double peak = 0, peak_at = 6;
    for (int k = 0; k < 73; k++) {
        double at = 6 + 24.0 * k / 72, tot = 0;
        for (int c = 0; c < s->n; c++) { s->curve[c][k] = (float)shot_cup(at - s->cups[c]); tot += s->curve[c][k]; }
        if (tot > peak) { peak = tot; peak_at = at; }
    }
    double bed = 0;
    for (int c = 0; c < s->n; c++) bed += shot_cup(23 - s->cups[c]);
    char l1[32], l2[32], d1[16];
    snprintf(l1, sizeof l1, "PEAK %.0f MG", peak);
    snprintf(l2, sizeof l2, "%.0f MG STILL IN YOU", bed);
    snprintf(d1, sizeof d1, "%02d:%02d", (int)peak_at, (int)lround((peak_at - (int)peak_at) * 60) % 60);
    cui_dot_series ser[4];
    for (int c = 0; c < s->n; c++) ser[c] = (cui_dot_series){ s->curve[c], NULL, c == s->sel };
    cui_chart_annotation notes[3] = { { CUI_ANNOTATION_Y, 0, 50, "50", NULL, false }, { CUI_ANNOTATION_X, 23, 0, "BED 23:00", l2, false },
                                      { CUI_ANNOTATION_X, peak_at, 0, l1, d1, false } };
    cui_row(ui, .width = CUI_FILL, .gap = 32, .align = CUI_ALIGN_END) {
        cui_readout(ui, "CAFFEINE", "285", .unit = "MG", .detail = "3 CUPS \xc2\xb7 HALF-LIFE 5H");
        cui_spacer(ui);
        cui_readout(ui, "AT BED", "54", .unit = "MG", .size = CUI_SIZE_SM, .accent = true, .detail = "OVER THE LINE");
    }
    cui_column(ui, .width = CUI_FILL, .gap = 0) {
        cui_dot_chart(ui, "level", ser, s->n, 73, .height = 230, .x_min = 6, .x_max = 30, .x_units = CUI_AXIS_HOURS, .min = 0, .max = 240,
                      .unit = "MG", .no_axis = true, .annotations = notes, .annotation_count = 3);
        cui_event_track(ui, "cups", s->cups, &s->n, 4, 6, 30, .step = 0.25, .units = CUI_AXIS_HOURS, .label_step = 6, .selected = &s->sel);
    }
    cui_dot_display(ui, "LAST CUP BY 10:45", .pitch = 3);
}

static void write_shot(const char *dir, bool dark) {
    static shot_state s;
    s = (shot_state){ { 8, 10.75, 12.25 }, 3, 2, { { 0 } } };
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT, .accent = CUI_ACCENT_BLUE });
    cui_set_viewport(ui, 720, 470, 2);
    cui_set_reduced_motion(ui, true);
    cui_input_mouse_leave(ui);
    for (int i = 0; i < 20; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 28, .gap = 22, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND) build_shot(ui, &s);
        cui_end_frame(ui);
    }
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    char path[600];
    snprintf(path, sizeof path, "%s/instruments%s.png", dir, dark ? "-dark" : "");
    cui_write_png(path, px, dd->width_px, dd->height_px, dd->width_px * 4);
    printf("wrote %s\n", path);
    free(px);
    cui_destroy(ui);
    cui_sw_destroy(sw);
}

int main(int argc, char **argv) {
    g_sw = cui_sw_create();
    test_dot_chart();
    test_dot_pixels();
    test_annotations();
    test_event_track();
    test_alignment();
    test_readout();
    test_dot_display();
    test_rtl_and_phone();
    test_directions();
    cui_sw_destroy(g_sw);
    if (argc > 1) { write_shot(argv[1], false); write_shot(argv[1], true); }
    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
