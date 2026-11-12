/* Tests for the canvas and bar components: the grouped and floating toolbar, the format bar,
 * live cursors, canvas controls, the result card and the selection bar. Runs headless with
 * the software renderer. Behaviour is checked through what the components return and edit,
 * and through the keyboard; cursor motion is checked from the pixels. */
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

static cui_ctx *new_ctx(float w, float h, bool reduced) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = CUI_THEME_DARK });
    cui_set_viewport(ui, w, h, 1);
    cui_set_reduced_motion(ui, reduced);
    cui_input_mouse_leave(ui);
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static float g_dt = 1.0f / 60.0f;
static bool g_rtl;
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, g_dt);
        cui_column(ui, .pad = 20, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND,
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
static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods); cui_input_key(ui, k, false, mods);
    run(ui, fn, u, 1);
}
static void tabs(cui_ctx *ui, build_fn fn, void *u, int n) { for (int i = 0; i < n; i++) key(ui, fn, u, CUI_KEY_TAB, 0); }
static cui_id nested(cui_ctx *ui, const char *scope, const char *name) {
    cui_push_id(ui, scope);
    cui_id id = cui_make_id(ui, name);
    cui_pop_id(ui);
    return id;
}
static cui_vec2 mid(cui_rect r) { return (cui_vec2){ r.x + r.w * 0.5f, r.y + r.h * 0.5f }; }
#define TIP 3   /* the first fully red pixel sits this far inside the arrow's tip, past its white edge */

/* ------------------------------------------------------------------------ */
/* format bar                                                               */

typedef struct { cui_text_format f; int r, all; cui_format_bar_opts o; } fmt_case;
static void build_fmt(cui_ctx *ui, void *u) {
    fmt_case *c = u;
    c->r = cui_format_bar_ex(ui, "fmt", &c->f, &c->o);
    c->all |= c->r;
}
/* Tab order: color, bold, underline, link, style, alignment, list, delete, more */
enum { FT_COLOR = 1, FT_BOLD, FT_UNDER, FT_LINK, FT_STYLE, FT_ALIGN, FT_LIST, FT_DELETE, FT_MORE };

static void test_format_bar(void) {
    printf("format bar\n");
    cui_ctx *ui = new_ctx(800, 500, true);
    fmt_case c;
    memset(&c, 0, sizeof c);
    c.f.size = 44;
    run(ui, build_fmt, &c, 3);
    CHECK_EQ(c.all, 0);

    /* bold and underline toggle from the keyboard and report a change */
    tabs(ui, build_fmt, &c, FT_BOLD);
    key(ui, build_fmt, &c, CUI_KEY_SPACE, 0);
    CHECK(c.f.bold);
    CHECK(c.all & CUI_FORMAT_CHANGED);
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK(c.f.underline);
    CHECK(c.all & CUI_FORMAT_CHANGED);

    /* link reports a click and leaves the format to the app */
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK(c.all & CUI_FORMAT_LINK);
    CHECK(!(c.all & CUI_FORMAT_CHANGED));
    CHECK(!c.f.link);

    /* the style menu: Enter opens it on the current style, Down and Enter pick Title */
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_DOWN, 0);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    CHECK_EQ(c.f.style, 1);
    CHECK_NEAR(c.f.size, 56, 1e-4);
    CHECK(c.all & CUI_FORMAT_CHANGED);

    /* a click on the style trigger opens the menu too, and its own style list wins */
    static const cui_text_style mine[] = { { "Body", 15, 13, CUI_FONT_BODY }, { "Big", 30, 20, CUI_FONT_MEDIUM } };
    c.o.styles = mine; c.o.style_count = 2;
    c.f.style = 5;   /* out of range: the bar clamps it */
    run(ui, build_fmt, &c, 2);
    CHECK_EQ(c.f.style, 0);
    cui_rect st = cui_get_rect(ui, nested(ui, "fmt", "#style"));
    CHECK(st.w >= 118 - 0.5f);
    click(ui, build_fmt, &c, mid(st).x, mid(st).y);
    run(ui, build_fmt, &c, 2);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_DOWN, 0);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.f.style, 1);
    CHECK_NEAR(c.f.size, 30, 1e-4);
    c.o.styles = NULL; c.o.style_count = 0;
    c.f.style = 0;
    run(ui, build_fmt, &c, 3);

    /* the color grid opens under the swatch; a click on a swatch picks its tone */
    cui_rect sw = cui_get_rect(ui, nested(ui, "fmt", "#color"));
    CHECK(sw.w > 0);
    click(ui, build_fmt, &c, mid(sw).x, mid(sw).y);
    run(ui, build_fmt, &c, 2);
    c.all = 0;
    float gx = sw.x + 10 + 13 + 32 * 1, gy = sw.y + sw.h + 6 + 10 + 13 + 32;   /* row 2, column 2 */
    click(ui, build_fmt, &c, gx, gy);
    CHECK_EQ(c.f.color, CUI_TONE_TEAL);   /* the default grid: neutral, rose, danger, warning / success, teal, info, violet */
    CHECK(c.all & CUI_FORMAT_CHANGED);
    run(ui, build_fmt, &c, 3);
    /* from the keyboard: Down opens it with focus on the current tone, the arrows move, Enter picks */
    click(ui, build_fmt, &c, 790, 490);   /* a click on the page drops the focus */
    tabs(ui, build_fmt, &c, FT_COLOR);
    key(ui, build_fmt, &c, CUI_KEY_DOWN, 0);
    run(ui, build_fmt, &c, 2);
    key(ui, build_fmt, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_fmt, &c, CUI_KEY_UP, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.f.color, CUI_TONE_DANGER);   /* teal, right to info, up to danger */
    CHECK(c.all & CUI_FORMAT_CHANGED);
    cui_destroy(ui);
}

static void test_format_keys(void) {
    printf("format bar: flyouts and delete from the keyboard\n");
    cui_ctx *ui = new_ctx(800, 500, true);
    fmt_case c;
    memset(&c, 0, sizeof c);
    c.f.size = 44;
    run(ui, build_fmt, &c, 3);

    /* alignment: Enter opens the flyout with focus on the current choice, Right and Enter pick center */
    tabs(ui, build_fmt, &c, FT_ALIGN);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    key(ui, build_fmt, &c, CUI_KEY_RIGHT, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    CHECK_EQ(c.f.align, CUI_FORMAT_ALIGN_CENTER);
    CHECK(c.all & CUI_FORMAT_CHANGED);

    /* list: the menu, Down twice to the checklist; picking it again turns the list off */
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    key(ui, build_fmt, &c, CUI_KEY_DOWN, 0);   /* from Bulleted list, highlighted as it opens */
    key(ui, build_fmt, &c, CUI_KEY_DOWN, 0);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    CHECK_EQ(c.f.list, CUI_FORMAT_LIST_CHECKS);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);   /* focus came back to the list button: open again */
    run(ui, build_fmt, &c, 2);
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);   /* the current kind starts highlighted */
    run(ui, build_fmt, &c, 2);
    CHECK_EQ(c.f.list, CUI_FORMAT_LIST_NONE);

    /* delete asks first: the press that opens the question does not answer it */
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    CHECK(!(c.all & CUI_FORMAT_DELETE));
    /* Esc closes it without deleting */
    key(ui, build_fmt, &c, CUI_KEY_ESCAPE, 0);
    run(ui, build_fmt, &c, 20);
    CHECK(!(c.all & CUI_FORMAT_DELETE));
    /* a fresh Enter on the trigger opens it again, and a second Enter confirms */
    tabs(ui, build_fmt, &c, 0);
    key(ui, build_fmt, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);   /* back on delete */
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    run(ui, build_fmt, &c, 2);
    CHECK(!(c.all & CUI_FORMAT_DELETE));
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK(c.all & CUI_FORMAT_DELETE);

    /* the more button reports a click */
    run(ui, build_fmt, &c, 20);
    key(ui, build_fmt, &c, CUI_KEY_TAB, 0);
    c.all = 0;
    key(ui, build_fmt, &c, CUI_KEY_ENTER, 0);
    CHECK(c.all & CUI_FORMAT_MORE);
    cui_destroy(ui);
}

/* at a phone's width the end of the bar goes into the toolbar's "more" menu, and picking
 * alignment there steps it instead of opening a flyout with nowhere to hang */
static void test_format_overflow(void) {
    printf("format bar: overflow\n");
    cui_ctx *ui = new_ctx(330, 400, true);
    fmt_case c;
    memset(&c, 0, sizeof c);
    c.f.size = 44;
    run(ui, build_fmt, &c, 4);
    cui_rect bar = cui_get_rect(ui, cui_make_id(ui, "fmt"));
    CHECK(bar.w > 0 && bar.x + bar.w <= 330 - 20 + 0.5f);
    cui_rect st = cui_get_rect(ui, nested(ui, "fmt", "#style"));
    CHECK(st.w > 0);   /* the start of the bar stays */
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* canvas controls                                                          */

typedef struct { cui_node_view view; int r, all; cui_canvas_controls_opts o; bool grid; float w, h; } rail_case;
static void build_rail(cui_ctx *ui, void *u) {
    rail_case *c = u;
    cui_box(ui, .id = "canvas", .width = CUI_PX(c->w), .height = CUI_PX(c->h), .bg = CUI_COLOR_SURFACE) {}
    c->r = cui_canvas_controls_ex(ui, "zoom", &c->view, &c->o);
    c->all |= c->r;
}
/* the canvas point at the middle of the canvas */
static cui_vec2 middle_point(const rail_case *c) {
    float z = c->view.zoom > 0 ? c->view.zoom : 1;
    return (cui_vec2){ (c->w * 0.5f - c->view.x) / z, (c->h * 0.5f - c->view.y) / z };
}

static void test_canvas_controls(void) {
    printf("canvas controls\n");
    cui_ctx *ui = new_ctx(800, 600, true);
    rail_case c;
    memset(&c, 0, sizeof c);
    c.w = 600; c.h = 400;
    cui_canvas_toggle t[] = { { "\xee\x86\x8e", "Grid", &c.grid } };
    c.o.toggles = t; c.o.toggle_count = 1;
    run(ui, build_rail, &c, 3);
    CHECK_EQ(c.all, 0);
    /* the rail sits inside the canvas, at its right edge, centered */
    cui_rect canvas = cui_get_rect(ui, cui_make_id(ui, "canvas")), rail = cui_get_rect(ui, cui_make_id(ui, "zoom"));
    CHECK_NEAR(canvas.x + canvas.w - (rail.x + rail.w), 12, 0.5);
    CHECK_NEAR(rail.y + rail.h * 0.5f, canvas.y + canvas.h * 0.5f, 1);
    cui_rect pct = cui_get_rect(ui, nested(ui, "zoom", "#zoom"));
    CHECK(pct.w > 0 && pct.x > rail.x && pct.x + pct.w < rail.x + rail.w);

    /* zoom in from the keyboard: through the presets, keeping the middle where it was */
    cui_vec2 m0 = middle_point(&c);
    tabs(ui, build_rail, &c, 1);
    c.all = 0;
    key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 1.25, 1e-4);
    CHECK(c.all & CUI_CANVAS_ZOOMED);
    cui_vec2 m1 = middle_point(&c);
    CHECK_NEAR(m1.x, m0.x, 0.01);
    CHECK_NEAR(m1.y, m0.y, 0.01);
    key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 1.5, 1e-4);
    for (int i = 0; i < 6; i++) key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 4, 1e-4);   /* the last preset, and no further */

    /* the readout resets to 100% */
    key(ui, build_rail, &c, CUI_KEY_TAB, 0);
    c.all = 0;
    key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 1, 1e-4);
    CHECK(c.all & CUI_CANVAS_ZOOMED);
    m1 = middle_point(&c);
    CHECK_NEAR(m1.x, m0.x, 0.01);

    /* zoom out steps down to the first preset */
    key(ui, build_rail, &c, CUI_KEY_TAB, 0);
    key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 0.8, 1e-4);
    key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 0.67, 1e-4);
    for (int i = 0; i < 8; i++) key(ui, build_rail, &c, CUI_KEY_ENTER, 0);
    CHECK_NEAR(c.view.zoom, 0.25, 1e-4);

    /* someone else changed the zoom (the wheel): the next step follows from there */
    c.view.zoom = 0.9f;
    run(ui, build_rail, &c, 1);
    click(ui, build_rail, &c, rail.x + rail.w * 0.5f, rail.y + 4 + 18);   /* the + button */
    CHECK_NEAR(c.view.zoom, 1, 1e-4);

    /* fit frames the content: as large as fits in 90% of the canvas, centered */
    c.o.content = (cui_rect){ 100, 50, 400, 100 };
    tabs(ui, build_rail, &c, 0);
    move(ui, build_rail, &c, 1, 1);
    key(ui, build_rail, &c, CUI_KEY_TAB, 0);   /* from +: readout, -, fit */
    cui_rect fit_r = { rail.x, rail.y + 4 + 36 + 2 + 22 + 2 + 36 + 2 + 7 + 2, rail.w, 36 };
    c.all = 0;
    click(ui, build_rail, &c, mid(fit_r).x, mid(fit_r).y);
    CHECK(c.all & CUI_CANVAS_FIT);
    CHECK(c.all & CUI_CANVAS_ZOOMED);
    CHECK_NEAR(c.view.zoom, 600 * 0.9 / 400, 1e-3);   /* width limits: 1.35 */
    cui_vec2 m2 = middle_point(&c);
    CHECK_NEAR(m2.x, 300, 0.05);
    CHECK_NEAR(m2.y, 100, 0.05);
    /* without content, fit is only reported */
    c.o.content = (cui_rect){ 0, 0, 0, 0 };
    float before = c.view.zoom;
    c.all = 0;
    click(ui, build_rail, &c, mid(fit_r).x, mid(fit_r).y);
    CHECK(c.all & CUI_CANVAS_FIT);
    CHECK(!(c.all & CUI_CANVAS_ZOOMED));
    CHECK_NEAR(c.view.zoom, before, 1e-6);

    /* a toggle flips its flag */
    cui_rect tg = { rail.x, fit_r.y + 36 + 2, rail.w, 36 };
    c.all = 0;
    click(ui, build_rail, &c, mid(tg).x, mid(tg).y);
    CHECK(c.grid);
    CHECK(c.all & CUI_CANVAS_TOGGLED);

    /* presets of your own, and the view's zoom 0 meaning 1 */
    static const float steps[] = { 0.5f, 1, 2 };
    c.o.steps = steps; c.o.step_count = 3;
    c.view = (cui_node_view){ 0, 0, 0 };
    run(ui, build_rail, &c, 2);
    click(ui, build_rail, &c, rail.x + rail.w * 0.5f, rail.y + 4 + 18);
    CHECK_NEAR(c.view.zoom, 2, 1e-4);
    click(ui, build_rail, &c, rail.x + rail.w * 0.5f, rail.y + 4 + 18);
    CHECK_NEAR(c.view.zoom, 2, 1e-4);
    cui_destroy(ui);

    /* right to left: the rail moves to the left edge */
    g_rtl = true;
    ui = new_ctx(800, 600, true);
    memset(&c, 0, sizeof c);
    c.w = 600; c.h = 400;
    run(ui, build_rail, &c, 4);
    canvas = cui_get_rect(ui, cui_make_id(ui, "canvas"));
    rail = cui_get_rect(ui, cui_make_id(ui, "zoom"));
    CHECK_NEAR(rail.x - canvas.x, 12, 0.5);
    cui_destroy(ui);
    g_rtl = false;
}

/* with motion on, zoom eases to the preset in about a quarter of a second at any frame rate */
static void test_zoom_easing(void) {
    printf("canvas controls: easing\n");
    float at_quarter[2];
    float dts[2] = { 1.0f / 30.0f, 1.0f / 144.0f };
    for (int k = 0; k < 2; k++) {
        g_dt = dts[k];
        cui_ctx *ui = new_ctx(800, 600, false);
        rail_case c;
        memset(&c, 0, sizeof c);
        c.w = 600; c.h = 400;
        run(ui, build_rail, &c, 4);
        cui_rect rail = cui_get_rect(ui, cui_make_id(ui, "zoom"));
        click(ui, build_rail, &c, rail.x + rail.w * 0.5f, rail.y + 4 + 18);
        float prev = c.view.zoom;
        bool monotonic = true;
        int n = (int)lroundf(0.08f / g_dt);
        for (int i = 0; i < n; i++) { run(ui, build_rail, &c, 1); if (c.view.zoom < prev - 1e-5f) monotonic = false; prev = c.view.zoom; }
        at_quarter[k] = c.view.zoom;
        CHECK(c.view.zoom > 1.0f && c.view.zoom < 1.25f);
        run(ui, build_rail, &c, (int)(1.0f / g_dt));
        CHECK(monotonic);
        CHECK_NEAR(c.view.zoom, 1.25, 1e-4);
        cui_destroy(ui);
    }
    CHECK_NEAR(at_quarter[0], at_quarter[1], 0.03);
    g_dt = 1.0f / 60.0f;
}

/* ------------------------------------------------------------------------ */
/* live cursors                                                             */

typedef struct { cui_live_user u[2]; int n; cui_live_cursors_opts o; cui_node_view view; bool use_view; } cursor_case;
static void build_cursors(cui_ctx *ui, void *u) {
    cursor_case *c = u;
    cui_box(ui, .id = "area", .width = CUI_PX(500), .height = CUI_PX(300), .bg = CUI_COLOR_BACKGROUND) {}
    if (c->use_view) c->o.view = &c->view;
    cui_live_cursors_ex(ui, "people", c->u, c->n, &c->o);
}
/* the top-left of everything drawn in pure red: the arrow's tip, give or take a point */
static cui_vec2 red_tip(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    int minx = 1 << 30, miny = 1 << 30;
    for (int y = 0; y < dd->height_px; y++)
        for (int x = 0; x < dd->width_px; x++) {
            const unsigned char *p = px + ((size_t)y * dd->width_px + x) * 4;
            if (p[0] > 200 && p[1] < 60 && p[2] < 60) { if (x < minx) minx = x; if (y < miny) miny = y; }
        }
    free(px);
    return (cui_vec2){ (float)minx, (float)miny };
}

static void test_live_cursors(void) {
    printf("live cursors\n");
    float at[2][2];
    float dts[2] = { 1.0f / 30.0f, 1.0f / 120.0f };
    for (int k = 0; k < 2; k++) {
        g_dt = dts[k];
        cui_ctx *ui = new_ctx(560, 360, false);
        cursor_case c;
        memset(&c, 0, sizeof c);
        c.n = 1;
        c.u[0] = (cui_live_user){ .name = "Kate", .color = { 255, 0, 0, 255 }, .x = 40, .y = 40 };
        run(ui, build_cursors, &c, 3);
        cui_rect area = cui_get_rect(ui, cui_make_id(ui, "area"));
        cui_vec2 p = red_tip(ui);
        CHECK_NEAR(p.x, area.x + 40 + TIP, 1.5);   /* the first position is taken as it is */
        CHECK_NEAR(p.y, area.y + 40 + TIP, 1.5);
        /* a new position: the cursor glides there, part of the way after a tenth of a second */
        c.u[0].x = 240; c.u[0].y = 140;
        run(ui, build_cursors, &c, (int)lroundf(0.1f / g_dt));
        p = red_tip(ui);
        at[k][0] = p.x - area.x; at[k][1] = p.y - area.y;
        CHECK(p.x > area.x + 45 && p.x < area.x + 235);
        CHECK(p.y > area.y + 42 && p.y < area.y + 138);
        /* and it settles on the target */
        run(ui, build_cursors, &c, (int)(1.5f / g_dt));
        p = red_tip(ui);
        CHECK_NEAR(p.x, area.x + 240 + TIP, 1.5);
        CHECK_NEAR(p.y, area.y + 140 + TIP, 1.5);
        cui_destroy(ui);
    }
    /* the same place at the same time, whatever the frame rate */
    CHECK_NEAR(at[0][0], at[1][0], 6);
    CHECK_NEAR(at[0][1], at[1][1], 4);
    g_dt = 1.0f / 60.0f;

    /* no_glide and reduced motion jump; a view maps canvas points */
    for (int mode = 0; mode < 3; mode++) {
        cui_ctx *ui = new_ctx(560, 360, mode == 1);
        cursor_case c;
        memset(&c, 0, sizeof c);
        c.n = 1;
        c.u[0] = (cui_live_user){ .name = "Kate", .color = { 255, 0, 0, 255 }, .x = 40, .y = 40 };
        c.o.no_glide = mode == 0;
        if (mode == 2) { c.use_view = true; c.view = (cui_node_view){ 100, 20, 2 }; c.o.no_glide = true; }
        run(ui, build_cursors, &c, 3);
        c.u[0].x = 100; c.u[0].y = 60;
        run(ui, build_cursors, &c, 1);
        cui_rect area = cui_get_rect(ui, cui_make_id(ui, "area"));
        cui_vec2 p = red_tip(ui);
        float ex = mode == 2 ? 100 + 100 * 2 : 100, ey = mode == 2 ? 20 + 60 * 2 : 60;
        CHECK_NEAR(p.x, area.x + ex + TIP, 1.5);
        CHECK_NEAR(p.y, area.y + ey + TIP, 1.5);
        cui_destroy(ui);
    }

    /* away hides the pointer; cursors outside the area are clipped */
    cui_ctx *ui = new_ctx(560, 360, true);
    cursor_case c;
    memset(&c, 0, sizeof c);
    c.n = 2;
    c.u[0] = (cui_live_user){ .name = "Kate", .color = { 255, 0, 0, 255 }, .x = 40, .y = 40, .away = true };
    c.u[1] = (cui_live_user){ .name = "Paul", .color = { 255, 0, 0, 255 }, .x = 900, .y = 40 };
    run(ui, build_cursors, &c, 3);
    cui_vec2 p = red_tip(ui);
    CHECK(p.x > 10000);   /* nothing red drawn */
    cui_destroy(ui);
}

/* the presence pill names whoever speaks, and stays a fixed size while the bars move */
typedef struct { cui_live_user u[7]; int n; } pill_case;
static void build_pill(cui_ctx *ui, void *u) {
    pill_case *c = u;
    cui_presence_pill(ui, "here", c->u, c->n, .max_visible = 4);
}
static void test_presence_pill(void) {
    printf("presence pill\n");
    cui_ctx *ui = new_ctx(500, 200, false);
    pill_case c;
    memset(&c, 0, sizeof c);
    static const char *names[] = { "Paul", "Kate", "Mario", "Ana", "Lee", "Sam" };
    c.n = 6;
    for (int k = 0; k < 6; k++) c.u[k].name = names[k];
    run(ui, build_pill, &c, 3);
    cui_rect quiet = cui_get_rect(ui, cui_make_id(ui, "here"));
    c.u[1].speaking = true;
    run(ui, build_pill, &c, 3);
    cui_rect talk = cui_get_rect(ui, cui_make_id(ui, "here"));
    CHECK(talk.w > quiet.w + 20);   /* bars and the name */
    run(ui, build_pill, &c, 7);
    cui_rect later = cui_get_rect(ui, cui_make_id(ui, "here"));
    CHECK_NEAR(later.w, talk.w, 0.01);
    c.u[1].away = true;   /* someone away does not speak */
    run(ui, build_pill, &c, 2);
    CHECK_NEAR(cui_get_rect(ui, cui_make_id(ui, "here")).w, quiet.w, 0.01);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* selection bar                                                            */

typedef struct { bool sel[5]; int r, count; bool counted; cui_selection_bar_opts o; bool popup; } sbar_case;
static const cui_selection_action g_acts[] = { { NULL, "Move" }, { NULL, "Copy" }, { NULL, "Delete", CUI_TONE_DANGER } };
static void build_sbar(cui_ctx *ui, void *u) {
    sbar_case *c = u;
    cui_box(ui, .id = "list", .width = CUI_PX(400), .height = CUI_PX(200), .bg = CUI_COLOR_SURFACE) {
        cui_button(ui, "Options");
        if (c->popup) cui_popover_open(ui, "pop");
        if (cui_popover_begin(ui, "pop", NULL)) { cui_text(ui, "A popover"); cui_popover_end(ui); }
    }
    if (c->counted) c->r = cui_selection_bar_ex(ui, "bulk", c->count, g_acts, 3, &c->o);
    else {
        c->o.selected = c->sel; c->o.item_count = 5;
        c->o.anchor = cui_make_id(ui, "list");
        c->r = cui_selection_bar_ex(ui, "bulk", 0, g_acts, 3, &c->o);
    }
}

static void test_selection_bar(void) {
    printf("selection bar\n");
    cui_ctx *ui = new_ctx(600, 400, true);
    sbar_case c;
    memset(&c, 0, sizeof c);
    run(ui, build_sbar, &c, 3);
    CHECK_EQ(c.r, CUI_SELECTION_NONE);
    CHECK(cui_get_rect(ui, nested(ui, "bulk", "#clear")).w == 0);   /* no bar without a selection */
    /* Esc with nothing selected is left for others */
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_NONE);

    c.sel[1] = c.sel[3] = true;
    run(ui, build_sbar, &c, 3);
    cui_rect list = cui_get_rect(ui, cui_make_id(ui, "list"));
    cui_rect clear = cui_get_rect(ui, nested(ui, "bulk", "#clear"));
    CHECK(clear.w > 0);
    CHECK(clear.y > list.y + list.h);   /* under the list */

    /* Esc clears your flags and reports it */
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_CLEARED);
    CHECK(!c.sel[1] && !c.sel[3]);
    run(ui, build_sbar, &c, 30);
    CHECK_EQ(c.r, CUI_SELECTION_NONE);

    /* the clear button does the same */
    c.sel[0] = true;
    run(ui, build_sbar, &c, 3);
    clear = cui_get_rect(ui, nested(ui, "bulk", "#clear"));
    move(ui, build_sbar, &c, mid(clear).x, mid(clear).y);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_sbar, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_sbar, &c, 1);
    CHECK_EQ(c.r, CUI_SELECTION_CLEARED);
    CHECK(!c.sel[0]);
    move(ui, build_sbar, &c, 1, 1);
    run(ui, build_sbar, &c, 30);

    /* actions from the keyboard: Tab goes Options, clear, Move, Copy, Delete */
    c.sel[2] = true;
    click(ui, build_sbar, &c, 580, 380);   /* a click on the page drops the focus the clear button took */
    run(ui, build_sbar, &c, 3);
    tabs(ui, build_sbar, &c, 5);
    key(ui, build_sbar, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.r, 2);
    CHECK(c.sel[2]);   /* an action leaves the selection to the app */
    tabs(ui, build_sbar, &c, 0);
    key(ui, build_sbar, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_sbar, &c, CUI_KEY_SPACE, 0);
    CHECK_EQ(c.r, 1);

    /* an open popover takes Esc first */
    c.popup = true;
    run(ui, build_sbar, &c, 3);
    c.popup = false;
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_NONE);
    CHECK(c.sel[2]);
    run(ui, build_sbar, &c, 2);
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_CLEARED);
    cui_destroy(ui);

    /* a count of your own: cleared is reported, the count stays yours; no_escape keeps Esc */
    ui = new_ctx(600, 400, true);
    memset(&c, 0, sizeof c);
    c.counted = true;
    c.count = 4;
    run(ui, build_sbar, &c, 3);
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_CLEARED);
    c.o.no_escape = true;
    run(ui, build_sbar, &c, 2);
    key(ui, build_sbar, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.r, CUI_SELECTION_NONE);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* grouped and floating toolbar                                             */

typedef struct { bool bold; int mode, scale; bool primary, cycled, radio; float width; cui_toolbar_style style; } tool_case;
static void build_tools(cui_ctx *ui, void *u) {
    tool_case *c = u;
    static const char *const icons[] = { "\xee\x84\x94", "\xee\x87\xb9", "\xee\x84\xbd" };
    static const char *const names[] = { "Move", "Draw", "Text" };
    static const char *const scales[] = { "1x", "2x", "4x" };
    cui_box(ui, .width = CUI_PX(c->width)) {
        cui_toolbar(ui, "tb", .style = c->style) {
            cui_toolbar_group(ui) {
                cui_toolbar_button(ui, "\xee\x81\xac", .tooltip = "Bold", .toggle = &c->bold);
                cui_toolbar_button(ui, "\xee\x86\xb2", .tooltip = "Italic");
            }
            cui_toolbar_group(ui) {
                if (cui_toolbar_radio(ui, "mode", icons, 3, &c->mode, .labels = names)) c->radio = true;
                if (cui_toolbar_cycle(ui, "scale", scales, 3, &c->scale, .tooltip = "Scale")) c->cycled = true;
            }
            if (cui_toolbar_primary(ui, "\xee\x84\xbd", .tooltip = "Go")) c->primary = true;
        }
    }
}

static void test_toolbar(void) {
    printf("toolbar: grouped and floating\n");
    for (int s = 1; s <= 2; s++) {
        cui_ctx *ui = new_ctx(700, 400, true);
        tool_case c;
        memset(&c, 0, sizeof c);
        c.width = 600;
        c.style = (cui_toolbar_style)s;
        run(ui, build_tools, &c, 4);
        /* as wide as its items, all of them shown */
        cui_rect tb = cui_get_rect(ui, cui_make_id(ui, "tb"));
        CHECK(tb.w > 200 && tb.w < 600);
        cui_rect seg = cui_get_rect(ui, nested(ui, "tb", "mode"));
        CHECK(seg.w > 0 && seg.x > tb.x && seg.x + seg.w < tb.x + tb.w);
        /* Tab order: bold, italic, the three icons, the picker, the primary */
        tabs(ui, build_tools, &c, 1);
        key(ui, build_tools, &c, CUI_KEY_SPACE, 0);
        CHECK(c.bold);
        tabs(ui, build_tools, &c, 2);
        key(ui, build_tools, &c, CUI_KEY_RIGHT, 0);   /* the radio set moves with the arrows */
        CHECK_EQ(c.mode, 1);
        CHECK(c.radio);
        tabs(ui, build_tools, &c, 2);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        CHECK(c.cycled);
        CHECK_EQ(c.scale, 1);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        CHECK_EQ(c.scale, 0);   /* wraps around */
        key(ui, build_tools, &c, CUI_KEY_TAB, 0);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        CHECK(c.primary);
        cui_destroy(ui);

        /* narrow: the end goes into the "more" menu, from which the radio still chooses */
        ui = new_ctx(700, 400, true);
        memset(&c, 0, sizeof c);
        c.width = 160;
        c.style = (cui_toolbar_style)s;
        run(ui, build_tools, &c, 5);
        tb = cui_get_rect(ui, cui_make_id(ui, "tb"));
        CHECK(tb.w <= 160 + 0.5f);
        /* bold, italic, then "more": open it and pick the third icon (items: Move, Draw, Text, Scale, Go) */
        tabs(ui, build_tools, &c, 3);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        run(ui, build_tools, &c, 2);
        for (int i = 0; i < 3; i++) key(ui, build_tools, &c, CUI_KEY_DOWN, 0);
        key(ui, build_tools, &c, CUI_KEY_ENTER, 0);
        run(ui, build_tools, &c, 2);
        CHECK_EQ(c.mode, 2);
        CHECK(c.radio);
        cui_destroy(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* result card                                                              */

typedef struct { bool clicked; int n; } card_case;
static void build_card(cui_ctx *ui, void *u) {
    card_case *c = u;
    if (cui_result_card(ui, "card", 0, "Final result", .caption = "A caption", .badge = "Done", .width = CUI_PX(200))) { c->clicked = true; c->n++; }
}
static void test_result_card(void) {
    printf("result card\n");
    cui_ctx *ui = new_ctx(400, 400, true);
    card_case c = { false, 0 };
    run(ui, build_card, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "card"));
    CHECK_NEAR(r.w, 200, 0.5);
    CHECK_NEAR(r.h, 250, 0.5);   /* five by four */
    click(ui, build_card, &c, mid(r).x, mid(r).y);
    CHECK_EQ(c.n, 1);
    move(ui, build_card, &c, 1, 1);
    tabs(ui, build_card, &c, 1);
    key(ui, build_card, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.n, 2);
    cui_destroy(ui);
}

int main(void) {
    g_sw = cui_sw_create();
    test_format_bar();
    test_format_keys();
    test_format_overflow();
    test_canvas_controls();
    test_zoom_easing();
    test_live_cursors();
    test_presence_pill();
    test_selection_bar();
    test_toolbar();
    test_result_card();
    cui_sw_destroy(g_sw);
    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
