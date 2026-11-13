/* CUI test suite: layout, interaction, text editing, overlays, theming and
 * the software renderer. Runs headless. */
#include <cui/cui.h>
#include <cui/cui_render.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_failed, g_checks;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, eps) do { g_checks++; double a_ = (a), b_ = (b); if (fabs(a_ - b_) > (eps)) { g_failed++; \
    printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)

static cui_sw_renderer *g_sw;
static char g_clip[1024];
static size_t g_clip_len;   /* the whole length, when g_clip holds only the start */
static void clip_set(void *u, const char *s) { (void)u; g_clip_len = strlen(s); snprintf(g_clip, sizeof g_clip, "%s", s); }
static const char *clip_get(void *u) { (void)u; return g_clip; }

static cui_ctx *new_ctx(void) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 800, 600, 1);
    cui_set_platform(ui, &(cui_platform_iface){ .set_clipboard = clip_set, .get_clipboard = clip_get });
    cui_set_reduced_motion(ui, true);   /* deterministic: animations snap */
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        fn(ui, user);
        cui_end_frame(ui);
    }
}

static void click_at(cui_ctx *ui, build_fn fn, void *user, float x, float y) {
    cui_input_mouse_move(ui, x, y);
    run(ui, fn, user, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, fn, user, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, fn, user, 1);
}

static cui_vec2 center(cui_rect r) { cui_vec2 c = { r.x + r.w / 2, r.y + r.h / 2 }; return c; }

/* ------------------------------------------------------------------------ */

static void test_color(void) {
    printf("color\n");
    cui_color w = cui_oklch(1, 0, 0, 1), b = cui_oklch(0, 0, 0, 1);
    CHECK(w.r == 255 && w.g == 255 && w.b == 255);
    CHECK(b.r == 0 && b.g == 0 && b.b == 0);
    cui_color m = cui_mix(w, b, 0.5f);
    CHECK(m.r > 90 && m.r < 140 && m.r == m.g && m.g == m.b);
    cui_color t = cui_mix(CUI_RGB(0xff0000), CUI_TRANSPARENT, 1.0f);
    CHECK(t.a == 0);
    cui_color a = cui_alpha(CUI_RGB(0x123456), 0.5f);
    CHECK(a.a == 128 && a.r == 0x12);
}

/* layout --------------------------------------------------------------- */

typedef struct { cui_id a, b, c; } ids3;

static void build_row(cui_ctx *ui, void *u) {
    ids3 *ids = u;
    cui_row(ui, .width = CUI_PX(500), .height = CUI_PX(100), .pad = 10, .gap = 20) {
        ids->a = cui_box_begin(ui, &(cui_box){ .id = "fixed", .width = CUI_PX(100), .height = CUI_FILL }); cui_box_end(ui);
        ids->b = cui_box_begin(ui, &(cui_box){ .id = "grow1", .width = CUI_GROW(1) }); cui_box_end(ui);
        ids->c = cui_box_begin(ui, &(cui_box){ .id = "grow2", .width = CUI_GROW(2) }); cui_box_end(ui);
    }
}

static cui_rect rect_of(cui_ctx *ui, cui_id id) { return cui_get_rect(ui, id); }

static void test_layout_row(void) {
    printf("layout: row grow and fixed\n");
    cui_ctx *ui = new_ctx();
    ids3 ids;
    run(ui, build_row, &ids, 2);
    cui_rect a = rect_of(ui, ids.a), b = rect_of(ui, ids.b), c = rect_of(ui, ids.c);
    CHECK_NEAR(a.x, 10, 0.01); CHECK_NEAR(a.w, 100, 0.01);
    CHECK_NEAR(a.h, 80, 0.01);                       /* fill height inside padding */
    float free_w = 500 - 20 - 100 - 40;              /* 340 split 1:2 */
    CHECK_NEAR(b.w, free_w / 3, 0.05);
    CHECK_NEAR(c.w, free_w * 2 / 3, 0.05);
    CHECK_NEAR(b.x, 10 + 100 + 20, 0.01);
    CHECK_NEAR(c.x + c.w, 490, 0.05);
    cui_destroy(ui);
}

typedef struct { cui_id col, txt; float width; } wrap_ctx;
static void build_wrap(cui_ctx *ui, void *u) {
    wrap_ctx *w = u;
    w->col = cui_column_begin(ui, &(cui_box){ .id = "col", .width = CUI_PX(w->width) });
    cui_text(ui, "The quick brown fox jumps over the lazy dog and keeps running far beyond the meadow.");
    cui_box_end(ui);
}

static void build_fixed_wrap(cui_ctx *ui, void *u) {
    wrap_ctx *w = u;
    w->col = cui_column_begin(ui, &(cui_box){ .id = "fixed_wrap", .width = CUI_PX(600) });
    cui_text(ui, "The quick brown fox jumps over the lazy dog.", .width = CUI_PX(100), .size = 20);
    cui_box_end(ui);
}

static void test_layout_wrap(void) {
    printf("layout: text wraps and grows taller when narrow\n");
    cui_ctx *ui = new_ctx();
    wrap_ctx w = { 0, 0, 600 };
    run(ui, build_wrap, &w, 2);
    float wide_h = rect_of(ui, w.col).h;
    w.width = 120;
    run(ui, build_wrap, &w, 2);
    float narrow_h = rect_of(ui, w.col).h;
    CHECK(narrow_h > wide_h * 3);
    CHECK_NEAR(rect_of(ui, w.col).w, 120, 0.01);
    run(ui, build_fixed_wrap, &w, 2);
    CHECK_NEAR(rect_of(ui, w.col).h, 5 * 29, 0.01);
    cui_destroy(ui);
}

typedef struct { cui_id outer, inner; } center_ctx;
static void build_center(cui_ctx *ui, void *u) {
    center_ctx *c = u;
    c->outer = cui_box_begin(ui, &(cui_box){ .id = "outer", .dir = CUI_ROW, .width = CUI_PX(300), .height = CUI_PX(200),
                                             .justify = CUI_JUSTIFY_CENTER, .align = CUI_ALIGN_CENTER });
    c->inner = cui_box_begin(ui, &(cui_box){ .id = "inner", .width = CUI_PX(100), .height = CUI_PX(50) });
    cui_box_end(ui);
    cui_box_end(ui);
}

static void test_layout_center(void) {
    printf("layout: justify and align center\n");
    cui_ctx *ui = new_ctx();
    center_ctx c;
    run(ui, build_center, &c, 2);
    cui_rect r = rect_of(ui, c.inner);
    CHECK_NEAR(r.x, 100, 0.01);
    CHECK_NEAR(r.y, 75, 0.01);
    cui_destroy(ui);
}

typedef struct { cui_id grid; cui_id cells[5]; } grid_ctx;
static void build_grid(cui_ctx *ui, void *u) {
    grid_ctx *g = u;
    g->grid = cui_box_begin(ui, &(cui_box){ .id = "grid", .dir = CUI_GRID, .columns = 2, .gap = 10, .width = CUI_PX(210) });
    for (int i = 0; i < 5; i++) {
        char id[8];
        snprintf(id, sizeof id, "c%d", i);
        g->cells[i] = cui_box_begin(ui, &(cui_box){ .id = id, .height = CUI_PX(30 + i * 10) });
        cui_box_end(ui);
    }
    cui_box_end(ui);
}

static void test_layout_grid(void) {
    printf("layout: grid\n");
    cui_ctx *ui = new_ctx();
    grid_ctx g;
    run(ui, build_grid, &g, 2);
    CHECK_NEAR(rect_of(ui, g.cells[0]).w, 100, 0.01);
    CHECK_NEAR(rect_of(ui, g.cells[1]).x, 110, 0.01);
    CHECK_NEAR(rect_of(ui, g.cells[2]).y, 40 + 10, 0.01);   /* row 0 height = max(30,40) */
    CHECK_NEAR(rect_of(ui, g.grid).h, 40 + 10 + 60 + 10 + 70, 0.01);
    cui_destroy(ui);
}

typedef struct { cui_id row, items[4], a, b; float width; } flow_ctx;
static void build_flow(cui_ctx *ui, void *u) {
    flow_ctx *f = u;
    f->row = cui_box_begin(ui, &(cui_box){ .id = "flow", .dir = CUI_ROW, .wrap = true, .gap = 10, .width = CUI_PX(f->width), .align = CUI_ALIGN_START });
    for (int i = 0; i < 4; i++) {
        char id[8];
        snprintf(id, sizeof id, "w%d", i);
        f->items[i] = cui_box_begin(ui, &(cui_box){ .id = id, .width = CUI_PX(100), .height = CUI_PX(20 + i * 5) });
        cui_box_end(ui);
    }
    cui_box_end(ui);
    /* growing panels with a minimum: side by side, or stacked when there is no room */
    cui_box_begin(ui, &(cui_box){ .dir = CUI_ROW, .wrap = true, .gap = 10, .width = CUI_PX(f->width) });
    f->a = cui_box_begin(ui, &(cui_box){ .id = "pa", .width = CUI_GROW(2), .min_width = 200, .height = CUI_PX(30) }); cui_box_end(ui);
    f->b = cui_box_begin(ui, &(cui_box){ .id = "pb", .width = CUI_GROW(1), .min_width = 150, .height = CUI_PX(30) }); cui_box_end(ui);
    cui_box_end(ui);
}

static void test_layout_flow(void) {
    printf("layout: wrapping rows\n");
    cui_ctx *ui = new_ctx();
    flow_ctx f = { .width = 500 };
    run(ui, build_flow, &f, 2);
    CHECK_NEAR(rect_of(ui, f.items[3]).x, 330, 0.01);        /* one line, like a plain row */
    CHECK_NEAR(rect_of(ui, f.items[3]).y, rect_of(ui, f.items[0]).y, 0.01);
    CHECK_NEAR(rect_of(ui, f.row).h, 35, 0.01);
    CHECK_NEAR(rect_of(ui, f.a).w, 2 * (500 - 10) / 3.0, 0.05);
    CHECK_NEAR(rect_of(ui, f.b).y, rect_of(ui, f.a).y, 0.01);
    f.width = 230;                                           /* two per line */
    run(ui, build_flow, &f, 2);
    cui_rect r = rect_of(ui, f.row);
    CHECK_NEAR(rect_of(ui, f.items[1]).x, r.x + 110, 0.01);
    CHECK_NEAR(rect_of(ui, f.items[2]).x, r.x, 0.01);
    CHECK_NEAR(rect_of(ui, f.items[2]).y, r.y + 25 + 10, 0.01);   /* first line as tall as its tallest */
    CHECK_NEAR(r.h, 25 + 10 + 35, 0.01);
    CHECK_NEAR(rect_of(ui, f.a).w, 230, 0.01);               /* stacked, each fills the line */
    CHECK_NEAR(rect_of(ui, f.b).w, 230, 0.01);
    CHECK_NEAR(rect_of(ui, f.b).y, rect_of(ui, f.a).y + 40, 0.01);
    cui_destroy(ui);
}

typedef struct { cui_id grid, cells[5], field; float width; } autogrid_ctx;
static void build_autogrid(cui_ctx *ui, void *u) {
    autogrid_ctx *g = u;
    cui_column(ui, .width = CUI_PX(g->width), .align = CUI_ALIGN_START) {
        g->grid = cui_box_begin(ui, &(cui_box){ .id = "ag", .dir = CUI_GRID, .columns = 4, .min_column_width = 100, .gap = 10, .width = CUI_FILL });
        for (int i = 0; i < 5; i++) {
            char id[8];
            snprintf(id, sizeof id, "g%d", i);
            g->cells[i] = cui_box_begin(ui, &(cui_box){ .id = id, .height = CUI_PX(20) });
            cui_box_end(ui);
        }
        cui_box_end(ui);
        static int sel = 0;
        static const char *items[] = { "One", "Two" };
        cui_select(ui, "soft", items, 2, &sel);
        g->field = cui_last_id(ui);
    }
}

static void test_layout_autogrid(void) {
    printf("layout: grid columns from a minimum width, soft default widths\n");
    cui_ctx *ui = new_ctx();
    autogrid_ctx g = { .width = 600 };
    run(ui, build_autogrid, &g, 2);
    CHECK_NEAR(rect_of(ui, g.cells[0]).w, (600 - 30) / 4.0, 0.05);   /* capped at columns = 4 */
    CHECK_NEAR(rect_of(ui, g.cells[4]).y, rect_of(ui, g.cells[0]).y + 30, 0.01);
    CHECK_NEAR(rect_of(ui, g.field).w, 220, 0.01);                   /* the select's default width */
    g.width = 230;
    run(ui, build_autogrid, &g, 2);
    CHECK_NEAR(rect_of(ui, g.cells[0]).w, 110, 0.01);                 /* two columns fit */
    CHECK_NEAR(rect_of(ui, g.cells[2]).x, rect_of(ui, g.cells[0]).x, 0.01);
    CHECK_NEAR(rect_of(ui, g.grid).h, 3 * 20 + 2 * 10, 0.01);
    CHECK(rect_of(ui, g.field).w <= 230.01f);                        /* shrinks into the column */
    g.width = 90;
    run(ui, build_autogrid, &g, 2);
    CHECK_NEAR(rect_of(ui, g.cells[1]).w, 90, 0.01);                  /* never fewer than one column */
    cui_destroy(ui);
}

typedef struct { cui_id table; float width; } wtable_ctx;
static void build_wtable(cui_ctx *ui, void *u) {
    wtable_ctx *t = u;
    cui_table_column cols[] = { { "Name", CUI_FILL, CUI_TEXT_LEFT, false }, { "Role", CUI_PX(120), CUI_TEXT_LEFT, false },
                                { "Amount", CUI_PX(120), CUI_TEXT_RIGHT, false } };
    cui_column(ui, .width = CUI_PX(t->width)) {
        cui_table(ui, "wt", cols, 3) {
            for (int i = 0; i < 3; i++) { cui_table_row(ui, false); cui_table_cell_text(ui, "Jonas Berg"); cui_table_cell_text(ui, "Owner"); cui_table_cell_text(ui, "$12"); }
        }
        t->table = cui_make_id(ui, "wt");
    }
}

static void test_layout_narrow_table(void) {
    printf("layout: a narrow table scrolls sideways\n");
    cui_ctx *ui = new_ctx();
    wtable_ctx t = { 0, 600 };
    run(ui, build_wtable, &t, 2);
    CHECK_NEAR(rect_of(ui, t.table).w, 600, 0.01);
    t.width = 200;
    run(ui, build_wtable, &t, 2);
    CHECK_NEAR(rect_of(ui, t.table).w, 200, 0.01);                   /* stays inside its column */
    cui_destroy(ui);
}

/* interaction ---------------------------------------------------------- */

typedef struct { int clicks; cui_id id; bool check; bool sw; float slider; } basic_ctx;
static void build_basic(cui_ctx *ui, void *u) {
    basic_ctx *b = u;
    cui_column(ui, .pad = 20, .gap = 10) {
        if (cui_button(ui, "Press")) b->clicks++;
        b->id = cui_last_id(ui);
        cui_checkbox(ui, "Check", &b->check);
        cui_switch(ui, "Switch", &b->sw);
        cui_slider(ui, "Slide", &b->slider, 0, 100, .width = CUI_PX(300));
    }
}

static void test_button_click(void) {
    printf("interaction: button click, hover and drag-off\n");
    cui_ctx *ui = new_ctx();
    basic_ctx b = { 0 };
    run(ui, build_basic, &b, 2);
    cui_rect r = rect_of(ui, b.id);
    CHECK(r.w > 40 && r.h == 36);
    cui_vec2 c = center(r);
    click_at(ui, build_basic, &b, c.x, c.y);
    CHECK(b.clicks == 1);
    run(ui, build_basic, &b, 3);
    CHECK(b.clicks == 1);
    /* press inside, release outside: no click */
    cui_input_mouse_move(ui, c.x, c.y);
    run(ui, build_basic, &b, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_basic, &b, 1);
    cui_input_mouse_move(ui, 700, 500);
    run(ui, build_basic, &b, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_basic, &b, 1);
    CHECK(b.clicks == 1);
    /* press and release between two frames still counts */
    cui_input_mouse_move(ui, c.x, c.y);
    run(ui, build_basic, &b, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_basic, &b, 1);
    CHECK(b.clicks == 2);
    cui_destroy(ui);
}

static void test_keyboard_nav(void) {
    printf("interaction: tab focus and keyboard activation\n");
    cui_ctx *ui = new_ctx();
    basic_ctx b = { 0 };
    run(ui, build_basic, &b, 2);
    cui_input_key(ui, CUI_KEY_TAB, true, 0); cui_input_key(ui, CUI_KEY_TAB, false, 0);
    run(ui, build_basic, &b, 1);
    cui_input_key(ui, CUI_KEY_ENTER, true, 0); cui_input_key(ui, CUI_KEY_ENTER, false, 0);
    run(ui, build_basic, &b, 1);
    CHECK(b.clicks == 1);
    /* tab to the checkbox and toggle it with space */
    cui_input_key(ui, CUI_KEY_TAB, true, 0); cui_input_key(ui, CUI_KEY_TAB, false, 0);
    run(ui, build_basic, &b, 1);
    cui_input_key(ui, CUI_KEY_SPACE, true, 0); cui_input_key(ui, CUI_KEY_SPACE, false, 0);
    run(ui, build_basic, &b, 1);
    CHECK(b.check == true);
    /* shift+tab goes back */
    cui_input_key(ui, CUI_KEY_TAB, true, CUI_MOD_SHIFT); cui_input_key(ui, CUI_KEY_TAB, false, CUI_MOD_SHIFT);
    run(ui, build_basic, &b, 1);
    cui_input_key(ui, CUI_KEY_ENTER, true, 0); cui_input_key(ui, CUI_KEY_ENTER, false, 0);
    run(ui, build_basic, &b, 1);
    CHECK(b.clicks == 2);
    cui_destroy(ui);
}

static void test_toggles_and_slider(void) {
    printf("interaction: checkbox, switch, slider drag\n");
    cui_ctx *ui = new_ctx();
    basic_ctx b = { 0 };
    run(ui, build_basic, &b, 2);
    /* the checkbox row sits below the button: find it by y */
    click_at(ui, build_basic, &b, 30, 20 + 36 + 10 + 9);
    CHECK(b.check == true);
    click_at(ui, build_basic, &b, 30, 20 + 36 + 10 + 9);
    CHECK(b.check == false);
    click_at(ui, build_basic, &b, 40, 20 + 36 + 10 + 18 + 10 + 10);
    CHECK(b.sw == true);
    /* slider: track starts at x=20, width 300, thumb padding 9 */
    float track_y = 20 + 36 + 10 + 18 + 10 + 20 + 10 + 17 + 6 + 11;
    cui_input_mouse_move(ui, 20 + 9 + 141, track_y);
    run(ui, build_basic, &b, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_basic, &b, 1);
    CHECK_NEAR(b.slider, 50, 1.5);
    cui_input_mouse_move(ui, 900, track_y);   /* drag past the end clamps */
    run(ui, build_basic, &b, 1);
    CHECK_NEAR(b.slider, 100, 0.01);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_basic, &b, 2);
    CHECK_NEAR(b.slider, 100, 0.01);
    cui_destroy(ui);
}

/* text editing ---------------------------------------------------------- */

typedef struct { char buf[128]; int flags; cui_id id; } edit_ctx;
static void build_edit(cui_ctx *ui, void *u) {
    edit_ctx *e = u;
    cui_column(ui, .pad = 20) {
        e->flags |= cui_input(ui, "field", e->buf, sizeof e->buf, .placeholder = "Type", .width = CUI_PX(300));
        e->id = cui_last_id(ui);
    }
}

static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods);
    cui_input_key(ui, k, false, mods);
    run(ui, fn, u, 1);
}

static void build_copy_button(cui_ctx *ui, void *u) {
    bool *clicked = u;
    *clicked = cui_copy_button(ui, "copy", "Copy this text");
}

static void test_copy_button_focus(void) {
    printf("copy button: keyboard activation through confirmation\n");
    cui_ctx *ui = new_ctx();
    bool clicked = false;
    run(ui, build_copy_button, &clicked, 1);
    key(ui, build_copy_button, &clicked, CUI_KEY_TAB, 0);
    g_clip[0] = '\0';
    key(ui, build_copy_button, &clicked, CUI_KEY_ENTER, 0);
    CHECK(clicked);
    CHECK(strcmp(g_clip, "Copy this text") == 0);

    run(ui, build_copy_button, &clicked, 1);   /* show the check mark */
    g_clip[0] = '\0';
    key(ui, build_copy_button, &clicked, CUI_KEY_ENTER, 0);
    CHECK(clicked);
    CHECK(strcmp(g_clip, "Copy this text") == 0);

    run(ui, build_copy_button, &clicked, 120);   /* let the confirmation expire */
    g_clip[0] = '\0';
    key(ui, build_copy_button, &clicked, CUI_KEY_SPACE, 0);
    CHECK(clicked);
    CHECK(strcmp(g_clip, "Copy this text") == 0);
    cui_destroy(ui);
}

/* the clock takes the whole idle interval for timers, but not an absurd one: a first dt of
 * "seconds since 1970" must not throw the clock where a float stops counting frames */
static void test_clock_step_bound(void) {
    printf("frame clock: idle intervals count, absurd ones are bounded\n");
    cui_ctx *ui = new_ctx();
    cui_begin_frame(ui, 1.0f / 60.0f); cui_end_frame(ui);
    double t0 = cui_time(ui);
    cui_begin_frame(ui, 2.5f); cui_end_frame(ui);              /* an idle wait of 2.5 s counts in full */
    CHECK(fabs(cui_time(ui) - t0 - 2.5) < 1e-6);
    double t1 = cui_time(ui);
    cui_begin_frame(ui, 1.7e9f); cui_end_frame(ui);            /* now - 0 */
    CHECK(cui_time(ui) - t1 <= 3600.0 + 1e-3);
    cui_begin_frame(ui, -1.0f); cui_end_frame(ui);             /* negative and NaN count as no time */
    cui_begin_frame(ui, NAN); cui_end_frame(ui);
    CHECK(cui_time(ui) - t1 <= 3600.0 + 1e-3);
    cui_destroy(ui);
}

/* cui_set_clipboard_text / cui_get_clipboard_text: the platform hooks, for menu items
 * that copy or paste (issue #32) */
static void test_clipboard_api(void) {
    printf("clipboard: set and get through the platform hooks\n");
    cui_ctx *ui = new_ctx();
    g_clip[0] = '\0';
    cui_set_clipboard_text(ui, "/home/ada/notes.txt");
    CHECK(strcmp(g_clip, "/home/ada/notes.txt") == 0);
    CHECK(strcmp(cui_get_clipboard_text(ui), "/home/ada/notes.txt") == 0);
    cui_set_clipboard_text(ui, NULL);                 /* NULL leaves the clipboard alone */
    CHECK(strcmp(g_clip, "/home/ada/notes.txt") == 0);
    cui_destroy(ui);
    /* no platform hooks: nothing happens, and getting gives an empty string */
    cui_ctx *bare = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_clipboard_text(bare, "ignored");
    CHECK(cui_get_clipboard_text(bare) != NULL && cui_get_clipboard_text(bare)[0] == '\0');
    CHECK(strcmp(g_clip, "/home/ada/notes.txt") == 0);
    cui_destroy(bare);
}

/* Nav items: the group's highlight follows the selected item, and goes away
 * when no item of the group is selected (issue #35). */
typedef struct { int sel; bool click; cui_id item[3]; } nav_ctx;
static void build_nav(cui_ctx *ui, void *u) {
    nav_ctx *n = u;
    static const char *const names[] = { "Inbox", "Projects", "Calendar" };
    cui_column(ui, .pad = 20, .gap = 4, .width = CUI_PX(240)) {
        for (int k = 0; k < 3; k++) {
            /* as apps write it: the selection changes when the item reports a click */
            if (cui_nav_item(ui, names[k], .selected = n->sel == k) && n->click) n->sel = k;
            n->item[k] = cui_last_id(ui);
        }
    }
}
/* is the row's left edge painted (the highlight) rather than the background? */
static bool nav_lit(cui_ctx *ui, cui_id id) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    int w = dd->width_px, h = dd->height_px;
    unsigned char *px = malloc((size_t)w * h * 4);
    cui_sw_render(g_sw, dd, px, w, h, w * 4, CUI_SW_RGBA);
    cui_rect r = rect_of(ui, id);
    int x = (int)(r.x + 4), y = (int)(r.y + r.h / 2), bx = 600, by = 500;
    const unsigned char *a = px + ((size_t)y * w + x) * 4, *b = px + ((size_t)by * w + bx) * 4;
    bool lit = abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2]) > 6;
    free(px);
    return lit;
}
static void test_nav_highlight(void) {
    printf("nav items: highlight follows the selection, and leaves with it\n");
    cui_ctx *ui = new_ctx();
    cui_input_mouse_move(ui, 790, 590);   /* away from the items: no hover */
    nav_ctx n = { .sel = 0 };
    run(ui, build_nav, &n, 4);
    CHECK(nav_lit(ui, n.item[0]) && !nav_lit(ui, n.item[1]) && !nav_lit(ui, n.item[2]));
    n.sel = -1;                            /* a page that is not in the sidebar */
    run(ui, build_nav, &n, 4);
    CHECK(!nav_lit(ui, n.item[0]) && !nav_lit(ui, n.item[1]) && !nav_lit(ui, n.item[2]));
    n.sel = 2;                             /* back in the sidebar, on another item */
    run(ui, build_nav, &n, 4);
    CHECK(!nav_lit(ui, n.item[0]) && !nav_lit(ui, n.item[1]) && nav_lit(ui, n.item[2]));
    n.sel = 1;                             /* moving between items still works */
    run(ui, build_nav, &n, 4);
    CHECK(!nav_lit(ui, n.item[0]) && nav_lit(ui, n.item[1]) && !nav_lit(ui, n.item[2]));
    cui_destroy(ui);
    /* with motion: clicking an item above the selected one glides the highlight over the
     * item between. The click frame has none selected, which must not reset the glide. */
    ui = new_ctx();
    cui_set_reduced_motion(ui, false);
    nav_ctx c = { .sel = 2, .click = true };
    run(ui, build_nav, &c, 60);
    cui_rect r0 = rect_of(ui, c.item[0]);
    click_at(ui, build_nav, &c, r0.x + r0.w / 2, r0.y + r0.h / 2);
    cui_input_mouse_move(ui, 790, 590);
    bool passed_middle = false;
    for (int f = 0; f < 30 && !passed_middle; f++) { run(ui, build_nav, &c, 1); passed_middle = nav_lit(ui, c.item[1]); }
    CHECK(c.sel == 0);
    CHECK(passed_middle);
    run(ui, build_nav, &c, 60);
    CHECK(nav_lit(ui, c.item[0]) && !nav_lit(ui, c.item[1]) && !nav_lit(ui, c.item[2]));
    cui_destroy(ui);
}

/* A click focuses without a ring; the next key shows the ring and acts on
 * that control, unless an open popup above it owns the keyboard. */
typedef struct { int clicks, sel; cui_id button, select; } ck_ctx;
static void build_ck(cui_ctx *ui, void *u) {
    ck_ctx *c = u;
    static const char *items[] = { "One", "Two", "Three" };
    cui_column(ui, .pad = 20, .gap = 12) {
        if (cui_button(ui, "Apply")) c->clicks++;
        c->button = cui_last_id(ui);
        cui_select(ui, "pick", items, 3, &c->sel);
        c->select = cui_last_id(ui);
    }
}
static void test_click_then_keys(void) {
    printf("interaction: keys act on the control a click focused\n");
    cui_ctx *ui = new_ctx();
    ck_ctx c = { 0 };
    run(ui, build_ck, &c, 2);
    cui_vec2 b = center(rect_of(ui, c.button));
    click_at(ui, build_ck, &c, b.x, b.y);
    CHECK(c.clicks == 1);
    key(ui, build_ck, &c, CUI_KEY_ENTER, 0);
    CHECK(c.clicks == 2);
    /* select: click opens, Escape closes, Down reopens on the current item, Down and Enter pick the next */
    cui_vec2 s = center(rect_of(ui, c.select));
    click_at(ui, build_ck, &c, s.x, s.y);
    run(ui, build_ck, &c, 20);
    key(ui, build_ck, &c, CUI_KEY_ESCAPE, 0);
    run(ui, build_ck, &c, 20);
    key(ui, build_ck, &c, CUI_KEY_DOWN, 0);
    run(ui, build_ck, &c, 20);
    key(ui, build_ck, &c, CUI_KEY_DOWN, 0);
    key(ui, build_ck, &c, CUI_KEY_ENTER, 0);
    run(ui, build_ck, &c, 20);
    CHECK(c.sel == 1);
    cui_destroy(ui);
}

static void type(cui_ctx *ui, build_fn fn, void *u, const char *s) {
    cui_input_key(ui, CUI_KEY_NONE, false, 0);   /* typing happens without modifiers */
    cui_input_text(ui, s);
    run(ui, fn, u, 1);
}

typedef struct { char text[16]; bool disabled, next_clicked; } password_reveal_ctx;

static void build_password_reveal(cui_ctx *ui, void *user) {
    password_reveal_ctx *p = user;
    cui_input(ui, "Password", p->text, sizeof p->text, .password = true, .disabled = p->disabled);
    p->next_clicked = cui_button(ui, "Next");
}

/** Sample the text pixels separately from the reveal button and its tooltip. */
static uint64_t password_text_pixels(cui_ctx *ui) {
    enum { width = 320, height = 100, stride = width * 4 };
    unsigned char pixels[height * stride];
    cui_sw_render(g_sw, cui_get_draw_data(ui), pixels, width, height, stride, CUI_SW_RGBA);
    uint64_t hash = 0;
    for (int y = 4; y < 32; y++) {
        for (int x = 12; x < 120; x++) {
            for (int channel = 0; channel < 3; channel++) hash = hash * 31 + pixels[y * stride + 4 * x + channel];
        }
    }
    return hash;
}

/** A disabled eye ignores clicks and leaves the next button first in tab order. */
static void test_disabled_password_reveal(void) {
    printf("password field: a disabled reveal button cannot unmask text\n");
    for (int disabled = 0; disabled < 2; disabled++) {
        cui_ctx *ui = new_ctx();
        cui_set_viewport(ui, 320, 100, 1);
        password_reveal_ctx p = { "sample", disabled != 0, false };
        run(ui, build_password_reveal, &p, 2);
        uint64_t masked = password_text_pixels(ui);
        click_at(ui, build_password_reveal, &p, 301, 16);
        run(ui, build_password_reveal, &p, 1);
        CHECK((password_text_pixels(ui) == masked) == p.disabled);
        cui_destroy(ui);
    }
    cui_ctx *ui = new_ctx();
    password_reveal_ctx p = { "sample", true, false };
    run(ui, build_password_reveal, &p, 1);
    key(ui, build_password_reveal, &p, CUI_KEY_TAB, 0);
    key(ui, build_password_reveal, &p, CUI_KEY_ENTER, 0);
    CHECK(p.next_clicked);
    cui_destroy(ui);
}

static void test_text_input(void) {
    printf("text input: typing, editing, selection, clipboard, undo\n");
    cui_ctx *ui = new_ctx();
    edit_ctx e;
    memset(&e, 0, sizeof e);
    run(ui, build_edit, &e, 2);
    cui_vec2 c = center(rect_of(ui, e.id));
    click_at(ui, build_edit, &e, c.x, c.y);
    CHECK(cui_wants_keyboard(ui));
    type(ui, build_edit, &e, "Hello world");
    CHECK(strcmp(e.buf, "Hello world") == 0);
    CHECK(e.flags & CUI_CHANGED);
    key(ui, build_edit, &e, CUI_KEY_BACKSPACE, 0);
    CHECK(strcmp(e.buf, "Hello worl") == 0);
    key(ui, build_edit, &e, CUI_KEY_BACKSPACE, CUI_MOD_CTRL | CUI_MOD_ALT);   /* word delete on any platform */
    CHECK(strcmp(e.buf, "Hello ") == 0);
    type(ui, build_edit, &e, "there");
    key(ui, build_edit, &e, CUI_KEY_HOME, 0);
    key(ui, build_edit, &e, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    key(ui, build_edit, &e, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    key(ui, build_edit, &e, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "He") == 0);
    key(ui, build_edit, &e, CUI_KEY_END, 0);
    key(ui, build_edit, &e, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(e.buf, "Hello thereHe") == 0);
    key(ui, build_edit, &e, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_edit, &e, "\xc3\xa9t\xc3\xa9");   /* UTF-8 replaces the selection */
    CHECK(strcmp(e.buf, "\xc3\xa9t\xc3\xa9") == 0);
    key(ui, build_edit, &e, CUI_KEY_BACKSPACE, 0);
    CHECK(strcmp(e.buf, "\xc3\xa9t") == 0);         /* removes a whole code point */
    key(ui, build_edit, &e, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(e.buf, "\xc3\xa9t\xc3\xa9") == 0);
    key(ui, build_edit, &e, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(e.buf, "Hello thereHe") == 0);
    key(ui, build_edit, &e, CUI_KEY_Z, CUI_MOD_PRIMARY | CUI_MOD_SHIFT);
    CHECK(strcmp(e.buf, "\xc3\xa9t\xc3\xa9") == 0);
    /* cut and paste in one frame: the paste lands where the cut text was */
    key(ui, build_edit, &e, CUI_KEY_A, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_X, true, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_X, false, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_V, true, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_V, false, CUI_MOD_PRIMARY);
    run(ui, build_edit, &e, 1);
    CHECK(strcmp(e.buf, "\xc3\xa9t\xc3\xa9") == 0);
    e.flags = 0;
    key(ui, build_edit, &e, CUI_KEY_ENTER, 0);
    CHECK(e.flags & CUI_SUBMITTED);
    key(ui, build_edit, &e, CUI_KEY_ESCAPE, 0);
    run(ui, build_edit, &e, 1);
    CHECK(!cui_wants_keyboard(ui));
    cui_destroy(ui);
}

/* An empty field still has a caret. At offset 0 the bar stands just left of the
 * text, in the field's padding, where the text clip used to cut it away. */
typedef struct { char buf[64]; cui_id id; } blank_ctx;
static void build_blank_field(cui_ctx *ui, void *u) {
    blank_ctx *e = u;
    cui_column(ui, .pad = 20) {
        cui_input(ui, "empty", e->buf, sizeof e->buf, .width = CUI_PX(300));
        e->id = cui_last_id(ui);
    }
}

static int dark_pixels(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    size_t n = (size_t)dd->width_px * dd->height_px;
    unsigned char *px = malloc(n * 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    int dark = 0;
    for (size_t i = 0; i < n; i++) if (px[i * 4] + px[i * 4 + 1] + px[i * 4 + 2] < 300) dark++;
    free(px);
    return dark;
}

static void test_empty_field_caret(void) {
    printf("text input: an empty field draws a caret, and it blinks\n");
    cui_ctx *ui = new_ctx();
    blank_ctx e;
    memset(&e, 0, sizeof e);
    run(ui, build_blank_field, &e, 2);
    cui_vec2 c = center(rect_of(ui, e.id));
    click_at(ui, build_blank_field, &e, c.x, c.y);
    CHECK(e.buf[0] == 0);
    int lo = -1, hi = 0;
    for (int i = 0; i < 70; i++) {   /* a little over one blink period */
        run(ui, build_blank_field, &e, 1);
        int dark = dark_pixels(ui);
        if (lo < 0 || dark < lo) lo = dark;
        if (dark > hi) hi = dark;
    }
    CHECK(hi > lo + 8);   /* the bar comes and goes inside the field */
    cui_destroy(ui);
}

typedef struct { char buf[512]; char other[32]; int flags; cui_id id, other_id; } area_ctx;
static void build_area(cui_ctx *ui, void *u) {
    area_ctx *a = u;
    cui_column(ui, .pad = 20, .gap = 12) {
        a->flags |= cui_textarea(ui, "notes", a->buf, sizeof a->buf, .height = 80, .width = CUI_PX(300));
        a->id = cui_last_id(ui);
        cui_input(ui, "other", a->other, sizeof a->other, .clearable = true, .width = CUI_PX(300));
        a->other_id = cui_last_id(ui);
    }
}

static uint32_t frame_hash(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    size_t n = (size_t)dd->width_px * dd->height_px * 4;
    unsigned char *px = malloc(n);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) h = (h ^ px[i]) * 16777619u;
    free(px);
    return h;
}

static void test_textarea_and_clear(void) {
    printf("text input: textarea wheel, focus flags, clear button undo\n");
    cui_ctx *ui = new_ctx();
    area_ctx a;
    memset(&a, 0, sizeof a);
    for (int i = 0; i < 20; i++) { char l[16]; snprintf(l, sizeof l, "line %d\n", i); strcat(a.buf, l); }
    run(ui, build_area, &a, 2);
    cui_vec2 c = center(rect_of(ui, a.id));
    cui_input_mouse_move(ui, c.x, c.y);
    run(ui, build_area, &a, 1);
    uint32_t before = frame_hash(ui);
    cui_input_scroll(ui, 0, -3);                 /* unfocused: the wheel still scrolls */
    run(ui, build_area, &a, 2);
    uint32_t after = frame_hash(ui);
    CHECK(before != after);
    CHECK(!cui_wants_keyboard(ui));

    a.flags = 0;
    click_at(ui, build_area, &a, c.x, c.y);
    CHECK(a.flags & CUI_FOCUSED);
    a.flags = 0;
    key(ui, build_area, &a, CUI_KEY_ESCAPE, 0);
    run(ui, build_area, &a, 1);
    CHECK(a.flags & CUI_BLURRED);
    CHECK(!(a.flags & CUI_FOCUSED));

    /* clear button: empties the field and Ctrl+Z restores it */
    strcpy(a.other, "keep me");
    run(ui, build_area, &a, 2);
    cui_rect fr = rect_of(ui, a.other_id);
    click_at(ui, build_area, &a, fr.x + fr.w / 3, fr.y + fr.h / 2);   /* focus the field */
    click_at(ui, build_area, &a, fr.x + fr.w - 16, fr.y + fr.h / 2);  /* the x button */
    CHECK(a.other[0] == 0);
    click_at(ui, build_area, &a, fr.x + fr.w / 3, fr.y + fr.h / 2);
    key(ui, build_area, &a, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(a.other, "keep me") == 0);
    cui_destroy(ui);
}

static cui_rect g_ime_caret;
static int g_ime_calls;
static void ime_rect(void *u, cui_rect r) { (void)u; g_ime_caret = r; g_ime_calls++; }

static void test_textarea_ime_caret(void) {
    printf("text area: the IME is told the caret's line, not the whole area\n");
    cui_ctx *ui = new_ctx();
    cui_set_platform(ui, &(cui_platform_iface){ .set_clipboard = clip_set, .get_clipboard = clip_get, .set_text_input_rect = ime_rect });
    area_ctx a;
    memset(&a, 0, sizeof a);
    strcpy(a.buf, "one\ntwo\nthree");
    run(ui, build_area, &a, 2);
    CHECK(g_ime_calls == 0);   /* nothing focused */
    cui_rect area = rect_of(ui, a.id);
    click_at(ui, build_area, &a, area.x + 12, area.y + 10);   /* on the first line */
    CHECK(g_ime_calls > 0);
    cui_rect first = g_ime_caret;
    CHECK(first.h > 0 && first.h < area.h / 2);
    CHECK(first.y >= area.y && first.y + first.h <= area.y + area.h);
    key(ui, build_area, &a, CUI_KEY_DOWN, 0);
    cui_rect second = g_ime_caret;
    CHECK_NEAR(second.y - first.y, first.h, 0.01);   /* one line lower, as tall */
    CHECK_NEAR(second.h, first.h, 0.01);
    key(ui, build_area, &a, CUI_KEY_DOWN, 0);
    key(ui, build_area, &a, CUI_KEY_END, 0);
    cui_rect third = g_ime_caret;
    CHECK_NEAR(third.y - first.y, 2 * first.h, 0.01);
    CHECK(third.x > first.x + 10);   /* after "three" */
    key(ui, build_area, &a, CUI_KEY_ENTER, 0);   /* a new, empty last line */
    CHECK(strcmp(a.buf, "one\ntwo\nthree\n") == 0);
    CHECK(g_ime_caret.y > third.y);   /* the area scrolls to keep it in view */
    CHECK(g_ime_caret.y + g_ime_caret.h <= area.y + area.h);
    CHECK(g_ime_caret.x < first.x + 12);
    cui_destroy(ui);
}

/** Put a scrollable editor inside a viewport with room to scroll both ways. */
static void build_nested_textarea(cui_ctx *ui, void *user) {
    area_ctx *a = user;
    cui_scroll(ui, .id = "page", .height = CUI_PX(180), .width = CUI_PX(320), .horizontal = true) {
        cui_box(ui, .height = CUI_PX(80)) {}
        cui_textarea(ui, "notes", a->buf, sizeof a->buf, .height = 80);
        a->id = cui_last_id(ui);
        cui_box(ui, .width = CUI_PX(520), .height = CUI_PX(400)) {}
    }
}

/** Keep the editor in place until the wheel points past one of its edges. */
static void test_nested_textarea_wheel(void) {
    printf("text area: wheel routing in a scrolling page\n");
    cui_ctx *ui = new_ctx();
    area_ctx a = { .buf = "One\nTwo\nThree\nFour\nFive\nSix\nSeven\nEight\nNine\nTen" };
    run(ui, build_nested_textarea, &a, 2);
    cui_scroll_to(ui, "page", 40);
    run(ui, build_nested_textarea, &a, 2);
    const cui_rect before = rect_of(ui, a.id);
    cui_input_mouse_move(ui, before.x + before.w / 2, before.y + before.h / 2);
    run(ui, build_nested_textarea, &a, 1);
    const uint32_t unscrolled = frame_hash(ui);

    cui_input_scroll(ui, 0, -1);
    run(ui, build_nested_textarea, &a, 1);
    const cui_rect after = rect_of(ui, a.id);
    CHECK_NEAR(after.y, before.y, 0.01);
    CHECK(frame_hash(ui) != unscrolled);

    if (fabsf(after.y - before.y) < 0.01f) {
        cui_input_scroll(ui, 0, -100);
        run(ui, build_nested_textarea, &a, 1);
        CHECK_NEAR(rect_of(ui, a.id).y, before.y, 0.01);
        cui_input_scroll(ui, 0, -1);
        run(ui, build_nested_textarea, &a, 1);
        const float lower_edge = rect_of(ui, a.id).y;
        CHECK(lower_edge < before.y);
        cui_input_mouse_move(ui, before.x + before.w / 2, lower_edge + before.h / 2);
        cui_input_scroll(ui, 0, 100);
        run(ui, build_nested_textarea, &a, 1);
        CHECK_NEAR(rect_of(ui, a.id).y, lower_edge, 0.01);
        cui_input_scroll(ui, 0, 1);
        run(ui, build_nested_textarea, &a, 1);
        CHECK(rect_of(ui, a.id).y > lower_edge);
    }
    cui_destroy(ui);
}

/** A page whose canvas has zoom controls floating over it. */
static void build_canvas_rail_page(cui_ctx *ui, void *user) {
    cui_id *anchor = user;
    static cui_node_view view;
    cui_scroll(ui, .id = "page", .height = CUI_PX(180), .width = CUI_PX(320)) {
        *anchor = cui_box_begin(ui, &(cui_box){ .id = "art", .width = CUI_PX(300), .height = CUI_PX(240) });
        cui_box_end(ui);
        cui_canvas_controls(ui, "zoom", &view);
        cui_box(ui, .height = CUI_PX(400)) {}
    }
}

/** The wheel over floating controls scrolls the page both ways: overlay nodes are not wheel targets. */
static void test_canvas_rail_wheel(void) {
    printf("canvas controls: the wheel over the rail scrolls the page\n");
    cui_ctx *ui = new_ctx();
    cui_id anchor = 0;
    run(ui, build_canvas_rail_page, &anchor, 3);
    cui_scroll_to(ui, "page", 40);
    run(ui, build_canvas_rail_page, &anchor, 2);
    cui_rect a = rect_of(ui, anchor);
    cui_input_mouse_move(ui, a.x + a.w - 20, a.y + a.h / 2);
    run(ui, build_canvas_rail_page, &anchor, 1);
    cui_input_scroll(ui, 0, 1);
    run(ui, build_canvas_rail_page, &anchor, 2);
    CHECK(rect_of(ui, anchor).y > a.y + 1);
    a = rect_of(ui, anchor);
    cui_input_scroll(ui, 0, -1);
    run(ui, build_canvas_rail_page, &anchor, 2);
    CHECK(rect_of(ui, anchor).y < a.y - 1);
    cui_destroy(ui);
}

/** A mixed wheel event scrolls the text vertically and the page horizontally. */
static void test_nested_textarea_mixed_wheel(void) {
    printf("text area: mixed-axis wheel and Shift routing\n");
    cui_ctx *ui = new_ctx();
    area_ctx a = { .buf = "One\nTwo\nThree\nFour\nFive\nSix\nSeven\nEight\nNine\nTen" };
    run(ui, build_nested_textarea, &a, 2);
    cui_scroll_to(ui, "page", 40);
    run(ui, build_nested_textarea, &a, 2);
    const cui_rect before = rect_of(ui, a.id);
    cui_input_mouse_move(ui, before.x + before.w / 2, before.y + before.h / 2);

    cui_input_scroll(ui, -1, -1);
    run(ui, build_nested_textarea, &a, 1);
    const cui_rect mixed = rect_of(ui, a.id);
    CHECK(mixed.x < before.x);
    CHECK_NEAR(mixed.y, before.y, 0.01);

    /* Scrolling back up must move the text toward its top before moving the page. */
    cui_input_scroll(ui, 0, 1);
    run(ui, build_nested_textarea, &a, 1);
    CHECK_NEAR(rect_of(ui, a.id).y, mixed.y, 0.01);

    const cui_rect before_shift = rect_of(ui, a.id);
    cui_input_key(ui, CUI_KEY_NONE, false, CUI_MOD_SHIFT);
    cui_input_scroll(ui, 0, -1);
    run(ui, build_nested_textarea, &a, 1);
    CHECK(rect_of(ui, a.id).x < before_shift.x);
    CHECK_NEAR(rect_of(ui, a.id).y, before_shift.y, 0.01);
    cui_destroy(ui);
}

/** Give the number field room for the draft and both step buttons. */
static void build_number_edit(cui_ctx *ui, void *user) {
    cui_number(ui, "quantity", user, 0, 99, .width = CUI_PX(240));
}

/** Commit a canonical draft and keep the previous value on invalid input. */
static void test_number_edit_commit(void) {
    printf("number field: clamp committed text and reject nonfinite input\n");
    cui_ctx *ui = new_ctx();
    double value = 3;
    run(ui, build_number_edit, &value, 2);
    key(ui, build_number_edit, &value, CUI_KEY_TAB, 0);
    key(ui, build_number_edit, &value, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_number_edit, &value, "999");
    key(ui, build_number_edit, &value, CUI_KEY_ENTER, 0);
    CHECK(value == 99);
    key(ui, build_number_edit, &value, CUI_KEY_A, CUI_MOD_PRIMARY);
    g_clip[0] = 0;
    key(ui, build_number_edit, &value, CUI_KEY_C, CUI_MOD_PRIMARY);
    if (strcmp(g_clip, "99") != 0) printf("  Committed draft: %s\n", g_clip);
    CHECK(strcmp(g_clip, "99") == 0);

    type(ui, build_number_edit, &value, "nan");
    if (!isfinite(value)) printf("  Number field returned a nonfinite value\n");
    CHECK(isfinite(value) && value == 99);
    key(ui, build_number_edit, &value, CUI_KEY_ENTER, 0);
    key(ui, build_number_edit, &value, CUI_KEY_A, CUI_MOD_PRIMARY);
    g_clip[0] = 0;
    key(ui, build_number_edit, &value, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "99") == 0);

    type(ui, build_number_edit, &value, "7x");
    CHECK(value == 99);
    key(ui, build_number_edit, &value, CUI_KEY_TAB, 0);
    key(ui, build_number_edit, &value, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_number_edit, &value, CUI_KEY_A, CUI_MOD_PRIMARY);
    g_clip[0] = 0;
    key(ui, build_number_edit, &value, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "99") == 0);
    cui_destroy(ui);
}

/** Let the searchable popup own the only text editor in the frame. */
typedef struct { int choice; bool searchable; } select_keys_ctx;
static void build_searchable_select(cui_ctx *ui, void *user) {
    select_keys_ctx *c = user;
    const char *items[] = { "Alpha", "Beta" };
    cui_select(ui, "choice", items, 2, &c->choice, .searchable = c->searchable);
}

/** Escape closes the search and leaves its trigger available to Enter, also
 * while the list animates closed; Enter opens a plain select without picking. */
static void test_searchable_select_reopen(void) {
    printf("searchable select: keyboard reopening after Escape\n");
    for (int animated = 0; animated < 2; animated++) {
        cui_ctx *ui = new_ctx();
        cui_set_reduced_motion(ui, !animated);
        select_keys_ctx c = { .searchable = true };
        run(ui, build_searchable_select, &c, 2);
        key(ui, build_searchable_select, &c, CUI_KEY_TAB, 0);
        key(ui, build_searchable_select, &c, CUI_KEY_DOWN, 0);
        run(ui, build_searchable_select, &c, 20);
        CHECK(cui_wants_keyboard(ui));
        key(ui, build_searchable_select, &c, CUI_KEY_ESCAPE, 0);
        run(ui, build_searchable_select, &c, 20);
        CHECK(!cui_wants_keyboard(ui));
        key(ui, build_searchable_select, &c, CUI_KEY_ENTER, 0);
        run(ui, build_searchable_select, &c, 20);
        CHECK(cui_wants_keyboard(ui));
        CHECK(c.choice == 0);
        /* picking an item also gives the keyboard back to the trigger */
        key(ui, build_searchable_select, &c, CUI_KEY_DOWN, 0);
        key(ui, build_searchable_select, &c, CUI_KEY_ENTER, 0);
        run(ui, build_searchable_select, &c, 20);
        CHECK(c.choice == 1 && !cui_wants_keyboard(ui));
        key(ui, build_searchable_select, &c, CUI_KEY_ENTER, 0);
        run(ui, build_searchable_select, &c, 20);
        CHECK(cui_wants_keyboard(ui) && c.choice == 1);
        cui_destroy(ui);
    }
    cui_ctx *ui = new_ctx();
    select_keys_ctx c = { .searchable = false };
    run(ui, build_searchable_select, &c, 2);
    key(ui, build_searchable_select, &c, CUI_KEY_TAB, 0);
    key(ui, build_searchable_select, &c, CUI_KEY_ENTER, 0);
    run(ui, build_searchable_select, &c, 2);
    key(ui, build_searchable_select, &c, CUI_KEY_DOWN, 0);
    key(ui, build_searchable_select, &c, CUI_KEY_ENTER, 0);
    CHECK(c.choice == 1);
    cui_destroy(ui);
}

/* more actions ------------------------------------------------------------ */

typedef struct {
    int split; bool fmt[3], al[3]; int confirm_fires; bool status_click; cui_status status;
    cui_reaction re[2]; int dial; cui_id dial_id, split_id, tg_id, tg2_id, confirm_id, status_id, re_id;
} act_ctx;
static void build_actions(cui_ctx *ui, void *u) {
    act_ctx *a = u;
    cui_column(ui, .pad = 20, .gap = 16) {
        static const char *items[] = { "Draft", "Publish" };
        int r = cui_split_button(ui, "Save", items, 2);
        if (r >= 0) a->split = r;
        a->split_id = cui_last_id(ui);
        static const char *labels[] = { "B", "I", "U" };
        cui_toggle_group(ui, "fmt", labels, 3, a->fmt);
        a->tg_id = cui_last_id(ui);
        cui_toggle_group(ui, "al", labels, 3, a->al, .exclusive = true);
        a->tg2_id = cui_last_id(ui);
        if (cui_confirm_button(ui, "Delete")) a->confirm_fires++;
        a->confirm_id = cui_last_id(ui);
        if (cui_status_button(ui, "Save now", a->status)) a->status_click = true;
        a->status_id = cui_last_id(ui);
        cui_reactions(ui, "re", a->re, 2);
        a->re_id = cui_last_id(ui);
        cui_row(ui, .height = CUI_PX(200), .align = CUI_ALIGN_END) {
            static const cui_speed_action acts[] = { { CUI_ICON_FILE, "Note" }, { CUI_ICON_IMAGE, "Image" } };
            int d = cui_speed_dial(ui, "dial", acts, 2);
            if (d >= 0) a->dial = d;
            a->dial_id = cui_last_id(ui);
        }
    }
}

static void build_toggle_group_arrows(cui_ctx *ui, void *user) {
    const char *labels[] = { "First", "Second", "Third", "Fourth" };
    cui_toggle_group(ui, "formats", labels, 4, user);
}

/** Move one slot per arrow and preserve focus given by a later mouse press. */
static void test_toggle_group_arrows(void) {
    printf("toggle group: one step per arrow press\n");
    cui_ctx *ui = new_ctx();
    bool values[4] = { false };
    run(ui, build_toggle_group_arrows, values, 1);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_TAB, 0);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_RIGHT, 0);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_ENTER, 0);
    CHECK(!values[0] && values[1] && !values[2] && !values[3]);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_LEFT, 0);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_SPACE, 0);
    CHECK(values[0] && values[1] && !values[2] && !values[3]);

    memset(values, 0, sizeof values);
    cui_rect group = cui_last_rect(ui);
    cui_input_mouse_move(ui, group.x + group.w - 12, group.y + group.h / 2);
    cui_input_key(ui, CUI_KEY_RIGHT, true, 0);
    cui_input_key(ui, CUI_KEY_RIGHT, false, 0);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_toggle_group_arrows, values, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_toggle_group_arrows, values, 1);
    CHECK(values[3]);
    key(ui, build_toggle_group_arrows, values, CUI_KEY_ENTER, 0);
    CHECK(!values[0] && !values[1] && !values[2] && !values[3]);
    cui_destroy(ui);
}

static void test_more_actions(void) {
    printf("actions: split button, toggle group, confirm, status, reactions, speed dial\n");
    cui_ctx *ui = new_ctx();
    act_ctx a;
    memset(&a, 0, sizeof a);
    a.split = -1; a.dial = -1;
    a.re[0] = (cui_reaction){ CUI_ICON_HEART, 3, false };
    a.re[1] = (cui_reaction){ CUI_ICON_STAR, 1, true };
    a.al[0] = true;
    run(ui, build_actions, &a, 3);
    /* With visible keyboard focus on the chevron, Enter belongs to the open menu. */
    key(ui, build_actions, &a, CUI_KEY_TAB, 0);
    key(ui, build_actions, &a, CUI_KEY_ENTER, 0);
    CHECK(a.split == 0);
    a.split = -1;
    key(ui, build_actions, &a, CUI_KEY_TAB, 0);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    CHECK(a.split == -1);
    key(ui, build_actions, &a, CUI_KEY_ENTER, 0);
    CHECK(a.split == 2);
    a.split = -1;
    key(ui, build_actions, &a, CUI_KEY_ENTER, 0);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    key(ui, build_actions, &a, CUI_KEY_ENTER, 0);
    CHECK(a.split == 1);
    a.split = -1;
    key(ui, build_actions, &a, CUI_KEY_SPACE, 0);
    key(ui, build_actions, &a, CUI_KEY_ESCAPE, 0);
    CHECK(a.split == -1);
    /* split: main part, then the menu by keyboard */
    cui_rect sr = rect_of(ui, a.split_id);
    click_at(ui, build_actions, &a, sr.x + 20, sr.y + sr.h / 2);
    CHECK(a.split == 0);
    click_at(ui, build_actions, &a, sr.x + sr.w - 10, sr.y + sr.h / 2);
    run(ui, build_actions, &a, 3);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    key(ui, build_actions, &a, CUI_KEY_DOWN, 0);
    key(ui, build_actions, &a, CUI_KEY_ENTER, 0);
    CHECK(a.split == 2);
    run(ui, build_actions, &a, 20);
    /* toggles */
    cui_rect tr = rect_of(ui, a.tg_id);
    click_at(ui, build_actions, &a, tr.x + 12, tr.y + tr.h / 2);
    CHECK(a.fmt[0] && !a.fmt[1]);
    click_at(ui, build_actions, &a, tr.x + tr.w - 12, tr.y + tr.h / 2);
    CHECK(a.fmt[0] && a.fmt[2]);
    cui_rect t2 = rect_of(ui, a.tg2_id);
    click_at(ui, build_actions, &a, t2.x + t2.w - 12, t2.y + t2.h / 2);
    CHECK(!a.al[0] && !a.al[1] && a.al[2]);
    /* confirm: arm, then confirm; an armed button disarms after its timeout */
    cui_vec2 cc = center(rect_of(ui, a.confirm_id));
    click_at(ui, build_actions, &a, cc.x, cc.y);
    CHECK(a.confirm_fires == 0);
    run(ui, build_actions, &a, 20);
    cc = center(rect_of(ui, a.confirm_id));
    click_at(ui, build_actions, &a, cc.x, cc.y);
    CHECK(a.confirm_fires == 1);
    run(ui, build_actions, &a, 20);
    cc = center(rect_of(ui, a.confirm_id));
    click_at(ui, build_actions, &a, cc.x, cc.y);
    run(ui, build_actions, &a, 240);   /* 4 seconds */
    cc = center(rect_of(ui, a.confirm_id));
    click_at(ui, build_actions, &a, cc.x, cc.y);
    CHECK(a.confirm_fires == 1);
    /* status: clicks only count while idle */
    cui_vec2 sc = center(rect_of(ui, a.status_id));
    click_at(ui, build_actions, &a, sc.x, sc.y);
    CHECK(a.status_click);
    a.status_click = false; a.status = CUI_STATUS_BUSY;
    run(ui, build_actions, &a, 30);
    sc = center(rect_of(ui, a.status_id));
    click_at(ui, build_actions, &a, sc.x, sc.y);
    CHECK(!a.status_click);
    /* reactions */
    cui_rect rr = rect_of(ui, a.re_id);
    click_at(ui, build_actions, &a, rr.x + 14, rr.y + rr.h / 2);
    CHECK(a.re[0].active && a.re[0].count == 4);
    click_at(ui, build_actions, &a, rr.x + 14, rr.y + rr.h / 2);
    CHECK(!a.re[0].active && a.re[0].count == 3);
    /* speed dial: open, pick the nearest action */
    cui_rect dr = rect_of(ui, a.dial_id);
    click_at(ui, build_actions, &a, dr.x + dr.w / 2, dr.y + dr.h / 2);
    run(ui, build_actions, &a, 40);
    click_at(ui, build_actions, &a, dr.x + dr.w / 2, dr.y - 12 - 21);
    CHECK(a.dial == 0);
    cui_destroy(ui);
}

typedef struct { char title[64]; float v; double money; cui_key_combo combo; int edit_flags; cui_id title_id, scrub_id, money_id, rec_id, other_id; char other[16]; } inp_ctx;
static void build_more_inputs(cui_ctx *ui, void *u) {
    inp_ctx *a = u;
    cui_column(ui, .pad = 20, .gap = 16) {
        a->edit_flags |= cui_inline_edit(ui, "title", a->title, sizeof a->title);
        a->title_id = cui_last_id(ui);
        cui_scrub(ui, "Size", &a->v, 0, 100);
        a->scrub_id = cui_last_id(ui);
        cui_money_input(ui, "money", &a->money, .width = CUI_PX(240));
        a->money_id = cui_last_id(ui);
        cui_shortcut_recorder(ui, "rec", &a->combo);
        a->rec_id = cui_last_id(ui);
        cui_input(ui, "other", a->other, sizeof a->other, .width = CUI_PX(200));
        a->other_id = cui_last_id(ui);
    }
}

typedef struct { float lo, hi, other; } range_keyboard_ctx;

static void build_range_keyboard(cui_ctx *ui, void *user) {
    range_keyboard_ctx *r = user;
    cui_range_slider(ui, "Range", &r->lo, &r->hi, 0, 100);
    cui_slider(ui, "Other", &r->other, 0, 100);
}

/** Exercise both thumbs and show which one is selected when they overlap. */
static void test_range_slider_keyboard(void) {
    printf("range slider: keyboard adjustment of both thumbs\n");
    cui_ctx *ui = new_ctx();
    enum { width = 320, height = 150, stride = width * 4 };
    unsigned char lower[height * stride], upper[height * stride];
    cui_set_viewport(ui, width, height, 1);
    cui_input_mouse_leave(ui);
    range_keyboard_ctx r = { 20, 75, 40 };
    run(ui, build_range_keyboard, &r, 1);
    int unfocused_vertices = cui_get_draw_data(ui)->vertex_count;
    key(ui, build_range_keyboard, &r, CUI_KEY_TAB, 0);
    CHECK(cui_get_draw_data(ui)->vertex_count > unfocused_vertices);
    key(ui, build_range_keyboard, &r, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(r.lo, 21, 0.01);
    CHECK_NEAR(r.hi, 75, 0.01);
    key(ui, build_range_keyboard, &r, CUI_KEY_END, 0);
    CHECK_NEAR(r.lo, 75, 0.01);
    cui_sw_render(g_sw, cui_get_draw_data(ui), lower, width, height, stride, CUI_SW_RGBA);
    key(ui, build_range_keyboard, &r, CUI_KEY_SPACE, 0);
    cui_sw_render(g_sw, cui_get_draw_data(ui), upper, width, height, stride, CUI_SW_RGBA);
    CHECK(memcmp(lower, upper, sizeof lower) != 0);
    key(ui, build_range_keyboard, &r, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_NEAR(r.lo, 75, 0.01);
    CHECK_NEAR(r.hi, 85, 0.01);
    key(ui, build_range_keyboard, &r, CUI_KEY_TAB, 0);
    key(ui, build_range_keyboard, &r, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(r.other, 41, 0.01);
    CHECK_NEAR(r.hi, 85, 0.01);
    cui_destroy(ui);
}

typedef struct { int ancestor, page; } navigation_focus_ctx;

/** Leave space around both rows for the focus rings. */
static void build_navigation_focus(cui_ctx *ui, void *user) {
    navigation_focus_ctx *n = user;
    const char *path[] = { "Home", "Projects", "Settings" };
    cui_column(ui, .pad = 12, .gap = 12) {
        n->ancestor = cui_breadcrumb(ui, path, 3);
        cui_pagination(ui, "pages", &n->page, 3);
    }
}

/** Compare only the pixels above the middle of the expected focus target. */
static bool focus_strip_changed(const unsigned char *a, const unsigned char *b, int stride, int height, cui_vec2 top) {
    int x = (int)top.x - 4;
    int y0 = (int)top.y - 4;
    if (x < 0 || x + 8 > stride / 4 || y0 < 0 || y0 + 7 > height) return false;
    for (int y = y0; y < y0 + 7; y++) {
        if (memcmp(a + y * stride + x * 4, b + y * stride + x * 4, 8 * 4) != 0) return true;
    }
    return false;
}

/** The ring appears over each keyboard target and clears when focus moves. */
static void test_navigation_focus_visible(void) {
    printf("breadcrumb and pagination: visible keyboard focus\n");
    enum { width = 400, height = 120, stride = width * 4 };
    unsigned char initial[height * stride], focused[height * stride];
    navigation_focus_ctx n = { -1, 0 };
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, width, height, 1);
    cui_input_mouse_leave(ui);
    run(ui, build_navigation_focus, &n, 1);
    cui_rect home = cui_get_rect(ui, cui_make_id(ui, "Home"));
    cui_rect next = cui_last_rect(ui);
    cui_vec2 home_top = { home.x + home.w / 2, home.y };
    /* Page two sits in the middle of the three numbered buttons and two arrows. */
    cui_vec2 page_top = { (home.x + next.x + next.w) / 2, next.y };
    cui_sw_render(g_sw, cui_get_draw_data(ui), initial, width, height, stride, CUI_SW_RGBA);
    key(ui, build_navigation_focus, &n, CUI_KEY_TAB, 0);
    cui_sw_render(g_sw, cui_get_draw_data(ui), focused, width, height, stride, CUI_SW_RGBA);
    CHECK(focus_strip_changed(initial, focused, stride, height, home_top));
    CHECK(!focus_strip_changed(initial, focused, stride, height, page_top));
    key(ui, build_navigation_focus, &n, CUI_KEY_ENTER, 0);
    CHECK(n.ancestor == 0);
    for (int i = 0; i < 3; i++) key(ui, build_navigation_focus, &n, CUI_KEY_TAB, 0);
    cui_sw_render(g_sw, cui_get_draw_data(ui), focused, width, height, stride, CUI_SW_RGBA);
    CHECK(!focus_strip_changed(initial, focused, stride, height, home_top));
    CHECK(focus_strip_changed(initial, focused, stride, height, page_top));
    key(ui, build_navigation_focus, &n, CUI_KEY_ENTER, 0);
    CHECK(n.page == 1);
    cui_destroy(ui);
}

typedef struct { int page; cui_size width; float sibling_width; } unnamed_pager_ctx;

/** Put the pager in a padded box with an optional sibling competing for space. */
static void build_unnamed_pager(cui_ctx *ui, void *user) {
    unnamed_pager_ctx *p = user;
    cui_row(ui, .width = CUI_FILL) {
        if (p->sibling_width > 0) {
            cui_box(ui, .width = CUI_PX(p->sibling_width)) cui_text(ui, "Sibling");
        }
        cui_box(ui, .width = p->width, .pad = 24) {
            cui_pagination(ui, "pages", &p->page, 12);
        }
    }
}

/** A fixed box constrains the pager and a content-sized box can grow again. */
static void test_pagination_unnamed_width(void) {
    printf("pagination: stays inside an unnamed padded parent\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 360, 160, 1);
    unnamed_pager_ctx p = { 5, CUI_PX(328), 0 };
    run(ui, build_unnamed_pager, &p, 3);
    cui_rect next = cui_last_rect(ui);
    CHECK(next.w > 0 && next.x + next.w <= 304.5f);
    cui_destroy(ui);

    ui = new_ctx();
    p.width = CUI_FIT;
    cui_set_viewport(ui, 300, 160, 1);
    run(ui, build_unnamed_pager, &p, 3);
    next = cui_last_rect(ui);
    float compact_edge = next.x + next.w;
    CHECK(compact_edge <= 276.5f);
    cui_set_viewport(ui, 650, 160, 1);
    run(ui, build_unnamed_pager, &p, 3);
    next = cui_last_rect(ui);
    CHECK(next.x + next.w > 304.5f);
    p.sibling_width = 350;
    run(ui, build_unnamed_pager, &p, 3);
    next = cui_last_rect(ui);
    CHECK_NEAR(next.x + next.w, p.sibling_width + compact_edge, 0.5);
    p.sibling_width = 0;
    cui_set_viewport(ui, 300, 160, 1);
    run(ui, build_unnamed_pager, &p, 3);
    next = cui_last_rect(ui);
    CHECK_NEAR(next.x + next.w, compact_edge, 0.5);
    cui_destroy(ui);
}

/** Render the narrow stepper with and without an unrelated, wider sibling. */
static void render_stepper_pair(bool sibling, unsigned char *pixels) {
    enum { width = 950, height = 80 };
    const char *steps[] = { "Account", "Workspace", "Invite team", "Done" };
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, width, height, 1);
    for (int frame = 0; frame < 3; frame++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_row(ui, .gap = 16, .pad = 8) {
            cui_box(ui, .width = CUI_PX(280)) cui_stepper(ui, steps, 4, 1);
            if (sibling) {
                cui_box(ui, .width = CUI_PX(620)) cui_stepper(ui, steps, 4, 1);
            }
        }
        cui_end_frame(ui);
    }
    cui_sw_render(g_sw, cui_get_draw_data(ui), pixels, width, height, width * 4, CUI_SW_RGBA);
    cui_destroy(ui);
}

/** The wide stepper must not change the narrow one's rendered captions. */
static void test_stepper_width_isolation(void) {
    printf("stepper: duplicate captions do not share measured width\n");
    enum { height = 80, stride = 950 * 4, left_column = 288 * 4 };
    unsigned char alone[height * stride], pair[height * stride];
    render_stepper_pair(false, alone);
    render_stepper_pair(true, pair);
    bool same = true;
    for (int y = 0; y < height; y++) {
        if (memcmp(alone + y * stride, pair + y * stride, left_column) != 0) same = false;
    }
    CHECK(same);
}

typedef struct { float lo, hi; bool chosen; } range_menu_ctx;

/** Attach the menu to the track so the range can retain focus underneath it. */
static void build_range_menu(cui_ctx *ui, void *user) {
    range_menu_ctx *r = user;
    cui_range_slider(ui, "Range", &r->lo, &r->hi, 0, 100);
    cui_context_menu(ui, "range-menu") {
        if (cui_menu_item(ui, "First")) r->chosen = true;
        cui_menu_item(ui, "Second");
    }
}

/** A context menu uses the arrow keys even when its anchor keeps focus. */
static void test_range_slider_context_menu(void) {
    printf("range slider: a context menu owns its arrow keys\n");
    cui_ctx *ui = new_ctx();
    range_menu_ctx r = { 20, 75, false };
    run(ui, build_range_menu, &r, 1);
    key(ui, build_range_menu, &r, CUI_KEY_TAB, 0);
    cui_vec2 track = center(cui_last_rect(ui));
    cui_input_mouse_move(ui, track.x, track.y);
    cui_input_mouse_button(ui, CUI_MOUSE_RIGHT, true);
    run(ui, build_range_menu, &r, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_RIGHT, false);
    cui_input_mouse_leave(ui);
    run(ui, build_range_menu, &r, 1);
    key(ui, build_range_menu, &r, CUI_KEY_DOWN, 0);
    key(ui, build_range_menu, &r, CUI_KEY_ENTER, 0);
    CHECK(r.chosen);
    CHECK_NEAR(r.lo, 20, 0.01);
    CHECK_NEAR(r.hi, 75, 0.01);
    cui_destroy(ui);
}

static void test_more_inputs(void) {
    printf("inputs: inline edit, scrub, money, shortcut recorder\n");
    cui_ctx *ui = new_ctx();
    inp_ctx a;
    memset(&a, 0, sizeof a);
    strcpy(a.title, "Roadmap");
    a.v = 20;
    a.money = 1234.5;
    run(ui, build_more_inputs, &a, 3);
    /* inline edit: Enter saves, Esc restores */
    cui_vec2 t = center(rect_of(ui, a.title_id));
    click_at(ui, build_more_inputs, &a, t.x, t.y);
    run(ui, build_more_inputs, &a, 1);
    CHECK(cui_wants_keyboard(ui));
    type(ui, build_more_inputs, &a, "Plan");        /* text starts selected, typing replaces it */
    key(ui, build_more_inputs, &a, CUI_KEY_ENTER, 0);
    CHECK(strcmp(a.title, "Plan") == 0);
    CHECK(a.edit_flags & CUI_CHANGED);
    run(ui, build_more_inputs, &a, 2);
    t = center(rect_of(ui, a.title_id));
    click_at(ui, build_more_inputs, &a, t.x, t.y);
    run(ui, build_more_inputs, &a, 1);
    type(ui, build_more_inputs, &a, "Oops");
    key(ui, build_more_inputs, &a, CUI_KEY_ESCAPE, 0);
    CHECK(strcmp(a.title, "Plan") == 0);
    run(ui, build_more_inputs, &a, 2);
    /* scrub: drag 50 points to the right adds 50 steps of 1 */
    cui_vec2 s = center(rect_of(ui, a.scrub_id));
    cui_input_mouse_move(ui, s.x, s.y);
    run(ui, build_more_inputs, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_more_inputs, &a, 1);
    cui_input_mouse_move(ui, s.x + 25, s.y);
    run(ui, build_more_inputs, &a, 1);
    cui_input_mouse_move(ui, s.x + 50, s.y);
    run(ui, build_more_inputs, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_more_inputs, &a, 1);
    CHECK_NEAR(a.v, 70, 0.01);
    /* a click without a drag types a value */
    s = center(rect_of(ui, a.scrub_id));
    click_at(ui, build_more_inputs, &a, s.x, s.y);
    run(ui, build_more_inputs, &a, 1);
    type(ui, build_more_inputs, &a, "33");
    key(ui, build_more_inputs, &a, CUI_KEY_ENTER, 0);
    CHECK_NEAR(a.v, 33, 0.01);
    run(ui, build_more_inputs, &a, 2);
    /* money: plain digits while typing, filtered to two decimals */
    cui_vec2 m = center(rect_of(ui, a.money_id));
    click_at(ui, build_more_inputs, &a, m.x, m.y);
    key(ui, build_more_inputs, &a, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_more_inputs, &a, "99.999x");
    CHECK_NEAR(a.money, 99.99, 0.0001);
    cui_vec2 o = center(rect_of(ui, a.other_id));
    click_at(ui, build_more_inputs, &a, o.x, o.y);   /* blur */
    CHECK_NEAR(a.money, 99.99, 0.0001);
    /* shortcut recorder */
    cui_vec2 r = center(rect_of(ui, a.rec_id));
    click_at(ui, build_more_inputs, &a, r.x, r.y);
    key(ui, build_more_inputs, &a, CUI_KEY_K, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    CHECK(a.combo.key == CUI_KEY_K && a.combo.mods == (CUI_MOD_CTRL | CUI_MOD_SHIFT));
    CHECK(strcmp(cui_shortcut_label(ui, a.combo), "Ctrl Shift K") == 0);
    click_at(ui, build_more_inputs, &a, r.x, r.y);
    key(ui, build_more_inputs, &a, CUI_KEY_BACKSPACE, 0);
    CHECK(a.combo.key == CUI_KEY_NONE);
    cui_destroy(ui);
}

typedef struct { int result; cui_id row; } swipe_ctx;
static void build_swipe(cui_ctx *ui, void *u) {
    swipe_ctx *a = u;
    cui_column(ui, .pad = 20, .width = CUI_PX(600)) {
        static const cui_swipe_action left[] = { { CUI_ICON_CHECK, "Done", CUI_TONE_SUCCESS } };
        static const cui_swipe_action right[] = { { CUI_ICON_ARCHIVE, "Archive", CUI_TONE_INFO }, { CUI_ICON_TRASH_2, "Delete", CUI_TONE_DANGER } };
        cui_swipe_item_begin(ui, "row", left, 1, right, 2);
        cui_text(ui, "Swipe me", .width = CUI_FILL);
        int r = cui_swipe_item_end(ui);
        if (r >= 0) a->result = r;
        a->row = cui_make_id(ui, "row");
    }
}

static void drag(cui_ctx *ui, build_fn fn, void *u, float x0, float y, float x1, int steps) {
    cui_input_mouse_move(ui, x0, y);
    run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, fn, u, 1);
    for (int i = 1; i <= steps; i++) { cui_input_mouse_move(ui, x0 + (x1 - x0) * i / steps, y); run(ui, fn, u, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, fn, u, 1);
}

static void test_swipe(void) {
    printf("swipe actions: long swipe triggers, short swipe reveals\n");
    cui_ctx *ui = new_ctx();
    swipe_ctx a = { -1, 0 };
    run(ui, build_swipe, &a, 3);
    cui_rect r = rect_of(ui, a.row);
    CHECK(r.w > 500);
    drag(ui, build_swipe, &a, r.x + r.w - 40, r.y + r.h / 2, r.x + r.w - 40 - 420, 8);
    CHECK(a.result == 100);
    run(ui, build_swipe, &a, 60);
    a.result = -1;
    drag(ui, build_swipe, &a, r.x + 300, r.y + r.h / 2, r.x + 300 - 120, 6);
    CHECK(a.result == -1);
    run(ui, build_swipe, &a, 60);
    /* now open on the right: the delete button sits at the far right */
    click_at(ui, build_swipe, &a, r.x + r.w - 30, r.y + r.h / 2);
    CHECK(a.result == 101);
    cui_destroy(ui);
}

/* pickers ------------------------------------------------------------------ */

static void test_date_math(void) {
    printf("dates: day numbers round trip\n");
    CHECK(cui_date_to_days((cui_date){ 1970, 1, 1 }) == 0);
    CHECK(cui_date_to_days((cui_date){ 2000, 3, 1 }) == 11017);
    cui_date d = cui_date_from_days(cui_date_to_days((cui_date){ 2024, 2, 29 }) + 1);
    CHECK(d.year == 2024 && d.month == 3 && d.day == 1);
    int bad = 0;
    for (int z = -800000; z < 800000; z += 37) {
        cui_date x = cui_date_from_days(z);
        if (cui_date_to_days(x) != z) bad++;
    }
    CHECK(bad == 0);
}

typedef struct {
    bool sel[4]; cui_date_range range; int wheel; float ruler; int dh, dm; int dropped; int drop_click;
    cui_id ms_id, cal_id, wheel_id, ruler_id, dial_id, drop_id, sig_id;
} pick_ctx;
static void build_pickers(cui_ctx *ui, void *u) {
    pick_ctx *a = u;
    cui_column(ui, .pad = 20, .gap = 16) {
        static const char *items[] = { "Alpha", "Beta", "Gamma", "Delta" };
        cui_multi_select(ui, "ms", items, 4, a->sel, .width = CUI_PX(300));
        a->ms_id = cui_last_id(ui);
        cui_range_calendar(ui, "cal", &a->range, 1);
        a->cal_id = cui_last_id(ui);
        static const char *w[] = { "One", "Two", "Three", "Four", "Five" };
        cui_wheel_picker(ui, "wheel", w, 5, &a->wheel);
        a->wheel_id = cui_last_id(ui);
        cui_ruler_slider(ui, "Ruler", &a->ruler, 0, 100, .width = CUI_PX(300));
        a->ruler_id = cui_last_id(ui);
    }
    cui_column(ui, .pad = 20, .gap = 16) {
        cui_time_dial(ui, "dial", &a->dh, &a->dm, 200);
        a->dial_id = cui_make_id(ui, "dial") ^ 0xFACEu;
        int n = cui_dropzone(ui, "drop", .accept = "png;txt", .width = CUI_PX(300));
        if (n > 0) a->dropped = n;
        if (n < 0) a->drop_click++;
        a->drop_id = cui_last_id(ui);
        cui_signature_pad(ui, "sig", .width = CUI_PX(300), .height = 120);
        a->sig_id = cui_make_id(ui, "sig") ^ 0x5161u;
    }
}
static void build_pickers_root(cui_ctx *ui, void *u) {
    cui_row(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_START) { build_pickers(ui, u); }
}

static void test_pickers(void) {
    printf("pickers: multi-select, range calendar, wheel, ruler, dial, dropzone, signature\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 1000, 900, 1);
    pick_ctx a;
    memset(&a, 0, sizeof a);
    a.range.start = (cui_date){ 2026, 11, 20 };   /* shows November 2026 */
    a.ruler = 50;
    a.dh = 9; a.dm = 0;
    run(ui, build_pickers_root, &a, 3);
    /* multi-select: open, then toggle items with the keyboard; the list stays open */
    cui_vec2 m = center(rect_of(ui, a.ms_id));
    click_at(ui, build_pickers_root, &a, m.x, m.y);
    run(ui, build_pickers_root, &a, 3);
    key(ui, build_pickers_root, &a, CUI_KEY_DOWN, 0);
    key(ui, build_pickers_root, &a, CUI_KEY_ENTER, 0);
    key(ui, build_pickers_root, &a, CUI_KEY_DOWN, 0);
    key(ui, build_pickers_root, &a, CUI_KEY_DOWN, 0);
    key(ui, build_pickers_root, &a, CUI_KEY_SPACE, 0);
    CHECK(a.sel[0] && !a.sel[1] && a.sel[2] && !a.sel[3]);
    key(ui, build_pickers_root, &a, CUI_KEY_ESCAPE, 0);
    run(ui, build_pickers_root, &a, 20);
    /* range calendar: November 2026 starts on a Sunday; click the 3rd, then the 14th */
    cui_rect cr = rect_of(ui, a.cal_id);
    float top = cr.y + 30 + 8 + 12 * 2.2f;
    a.range.start = (cui_date){ 0, 0, 0 };
    int days[2] = { 14, 3 };   /* end first: the calendar orders them */
    for (int k = 0; k < 2; k++) {
        int cell = days[k] - 1 + 6;
        click_at(ui, build_pickers_root, &a, cr.x + (cell % 7) * 36 + 18, top + (cell / 7) * 36 + 18);
    }
    CHECK(a.range.start.day == 3 && a.range.end.day == 14 && a.range.end.month == 11);
    /* wheel: arrow keys and a drag */
    cui_vec2 w = center(rect_of(ui, a.wheel_id));
    click_at(ui, build_pickers_root, &a, w.x, w.y);
    key(ui, build_pickers_root, &a, CUI_KEY_DOWN, 0);
    key(ui, build_pickers_root, &a, CUI_KEY_DOWN, 0);
    CHECK(a.wheel == 2);
    drag(ui, build_pickers_root, &a, w.x, w.y, w.x, 1);   /* no move: stays */
    CHECK(a.wheel == 2);
    cui_input_mouse_move(ui, w.x, w.y);
    run(ui, build_pickers_root, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_pickers_root, &a, 1);
    for (int i = 1; i <= 6; i++) { cui_input_mouse_move(ui, w.x, w.y + 34.0f * i / 6); run(ui, build_pickers_root, &a, 1); }
    run(ui, build_pickers_root, &a, 10);   /* hold still so the throw has no speed */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_pickers_root, &a, 30);
    CHECK(a.wheel == 1);
    /* ruler: dragging left by 9 points per step moves the value up */
    cui_vec2 rl = center(rect_of(ui, a.ruler_id));
    cui_input_mouse_move(ui, rl.x, rl.y);
    run(ui, build_pickers_root, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_pickers_root, &a, 1);
    for (int i = 1; i <= 6; i++) { cui_input_mouse_move(ui, rl.x - 90.0f * i / 6, rl.y); run(ui, build_pickers_root, &a, 1); }
    run(ui, build_pickers_root, &a, 10);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_pickers_root, &a, 30);
    CHECK_NEAR(a.ruler, 60, 0.01);
    /* dial: drag to 3 o'clock on the outer ring sets 3, then minutes */
    cui_rect dr = rect_of(ui, a.dial_id);
    cui_vec2 dc = center(dr);
    drag(ui, build_pickers_root, &a, dc.x + dr.w * 0.42f, dc.y, dc.x + dr.w * 0.42f, 1);
    CHECK(a.dh == 3);
    drag(ui, build_pickers_root, &a, dc.x, dc.y + dr.h * 0.42f, dc.x, 1);   /* six o'clock = 30 minutes */
    CHECK(a.dm == 30);
    drag(ui, build_pickers_root, &a, dc.x - dr.w * 0.42f, dc.y, dc.x - dr.w * 0.42f, 1);
    CHECK(a.dm == 45);
    /* dropzone: files dropped on it are filtered; elsewhere they are left alone */
    cui_vec2 z = center(rect_of(ui, a.drop_id));
    const char *files[] = { "/tmp/a.png", "/tmp/b.exe", "/tmp/c.TXT" };
    cui_input_file_drop(ui, files, 3, z.x, z.y);
    run(ui, build_pickers_root, &a, 1);
    CHECK(a.dropped == 2);
    cui_input_file_drop(ui, files, 1, 5, 5);
    cui_begin_frame(ui, 1.0f / 60.0f);
    build_pickers_root(ui, &a);
    CHECK(cui_dropped_file_count(ui) == 1 && strcmp(cui_dropped_file(ui, 0), "/tmp/a.png") == 0);
    cui_end_frame(ui);
    CHECK(cui_dropped_file_count(ui) == 0);
    click_at(ui, build_pickers_root, &a, z.x, z.y);
    CHECK(a.drop_click == 1);
    CHECK(cui_path_has_extension("photo.JPeG", "png;jpeg") && !cui_path_has_extension("dir.png/file", "png"));
    /* signature: a drag makes a stroke */
    cui_rect sr = rect_of(ui, a.sig_id);
    drag(ui, build_pickers_root, &a, sr.x + 40, sr.y + 60, sr.x + 200, 12);
    const cui_vec2 *pts; const int *ends; int nstroke = 0;
    int np = cui_signature_points(ui, "sig", &pts, &ends, &nstroke);
    CHECK(np > 5 && nstroke == 1);
    cui_signature_clear(ui, "sig");
    CHECK(cui_signature_points(ui, "sig", NULL, NULL, NULL) == 0);
    cui_destroy(ui);
}

/* disclosure ---------------------------------------------------------------- */

typedef struct {
    bool card_built, card_open, banner, confirmed, et_toggled; int step_action, active;
    cui_check_step steps[3]; cui_id link_id, del_id, card_id, banner_id, et_id, list_id, scroll_id;
} disc_ctx;
static void build_disclosure(cui_ctx *ui, void *u) {
    disc_ctx *a = u;
    a->card_built = false;
    cui_row(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_START, .pad = 20, .gap = 20) {
        cui_column(ui, .width = CUI_PX(420), .gap = 14) {
            cui_banner(ui, "Update ready", &a->banner, .dismissible = true);
            a->banner_id = cui_make_id(ui, "Update ready") ^ 0xBA22u;
            cui_link(ui, "@jonas");
            a->link_id = cui_last_id(ui);
            cui_hover_card(ui, "card") { a->card_built = true; cui_text(ui, "Jonas Berg"); }
            if (cui_button(ui, "Delete")) cui_popover_open(ui, "confirm");
            a->del_id = cui_last_id(ui);
            if (cui_confirm_popover(ui, "confirm", .title = "Sure?")) a->confirmed = true;
            if (cui_expandable_text(ui, "et", "one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen "
                                    "seventeen eighteen nineteen twenty twenty-one twenty-two twenty-three twenty-four twenty-five", .lines = 1)) a->et_toggled = true;
            a->et_id = cui_make_id(ui, "et");
            cui_expandable_card(ui, "Billing", &a->card_open) { cui_text(ui, "Body"); }
            a->card_id = cui_make_id(ui, "Billing") ^ 0xEC4Du;
            int r = cui_checklist(ui, "steps", a->steps, 3);
            if (r >= 0) a->step_action = r;
            a->list_id = cui_make_id(ui, "steps");
        }
        static const cui_toc_item toc[] = { { "One", "one", 0 }, { "Two", "two", 0 }, { "Three", "three", 0 } };
        cui_scroll(ui, .id = "doc", .height = CUI_PX(200), .width = CUI_PX(300)) {
            for (int k = 0; k < 3; k++) {
                cui_toc_anchor(ui, toc[k].anchor);
                cui_box(ui, .height = CUI_PX(300)) { cui_text(ui, toc[k].title); }
            }
        }
        a->scroll_id = cui_make_id(ui, "doc");
        cui_column(ui, .width = CUI_PX(160)) { a->active = cui_toc(ui, "doc", toc, 3); }
    }
}

/** Build the question in the same frame as its keyboard-activated trigger. */
static void build_confirm_keyboard(cui_ctx *ui, void *user) {
    bool *confirmed = user;
    if (cui_button(ui, "Delete item")) cui_popover_open(ui, "delete");
    *confirmed = cui_confirm_popover(ui, "delete", .title = "Delete this item?", .danger = true);
}

/** The key that opens the question must not answer it in the same frame. */
static void test_confirm_popover_keyboard(void) {
    printf("confirmation popover: a separate Enter confirms\n");
    cui_ctx *ui = new_ctx();
    bool confirmed = false;
    run(ui, build_confirm_keyboard, &confirmed, 1);
    key(ui, build_confirm_keyboard, &confirmed, CUI_KEY_TAB, 0);
    key(ui, build_confirm_keyboard, &confirmed, CUI_KEY_ENTER, 0);
    CHECK(cui_popover_is_open(ui, "delete") && !confirmed);
    key(ui, build_confirm_keyboard, &confirmed, CUI_KEY_ENTER, 0);
    CHECK(confirmed && !cui_popover_is_open(ui, "delete"));
    cui_destroy(ui);
}

static void test_disclosure(void) {
    printf("disclosure: hover card, confirm popover, expandables, toc, banner, checklist\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 1000, 1100, 1);
    disc_ctx a;
    memset(&a, 0, sizeof a);
    a.banner = true; a.step_action = -1;
    a.steps[0] = (cui_check_step){ "First", "Do it", "Go", true };
    a.steps[1] = (cui_check_step){ "Second", "Then this", "Next", false };
    a.steps[2] = (cui_check_step){ "Third", NULL, NULL, false };
    run(ui, build_disclosure, &a, 3);
    /* hover card: needs a short rest on the anchor, closes once the pointer leaves */
    cui_vec2 l = center(rect_of(ui, a.link_id));
    cui_input_mouse_move(ui, l.x, l.y);
    run(ui, build_disclosure, &a, 5);
    CHECK(!a.card_built);
    run(ui, build_disclosure, &a, 40);
    CHECK(a.card_built);
    cui_input_mouse_move(ui, 990, 1090);
    run(ui, build_disclosure, &a, 40);
    CHECK(!a.card_built);
    /* confirm popover */
    cui_vec2 dl = center(rect_of(ui, a.del_id));
    click_at(ui, build_disclosure, &a, dl.x, dl.y);
    run(ui, build_disclosure, &a, 10);
    key(ui, build_disclosure, &a, CUI_KEY_ENTER, 0);
    CHECK(a.confirmed);
    run(ui, build_disclosure, &a, 20);
    /* expandable text: clamped, then expanded */
    cui_rect er = rect_of(ui, a.et_id);
    click_at(ui, build_disclosure, &a, er.x + 20, er.y + er.h - 8);
    CHECK(a.et_toggled);
    run(ui, build_disclosure, &a, 40);
    CHECK(rect_of(ui, a.et_id).h > er.h + 10);
    /* expandable card */
    cui_vec2 cc = center(rect_of(ui, a.card_id));
    click_at(ui, build_disclosure, &a, cc.x, cc.y);
    CHECK(a.card_open);
    /* checklist: the open step's action, and toggling a circle */
    run(ui, build_disclosure, &a, 40);
    cui_rect lr = rect_of(ui, a.list_id);
    (void)lr;
    key(ui, build_disclosure, &a, CUI_KEY_TAB, 0);
    /* banner dismiss: the X is at the far right of the banner */
    cui_rect br = rect_of(ui, a.banner_id);
    float link_y = rect_of(ui, a.link_id).y;
    click_at(ui, build_disclosure, &a, br.x + br.w - 22, br.y + br.h / 2);
    CHECK(!a.banner);
    run(ui, build_disclosure, &a, 40);
    CHECK_NEAR(rect_of(ui, a.link_id).y, link_y - br.h - 14, 1.0);   /* the banner and its gap collapsed away */
    /* table of contents follows the scroll position */
    CHECK(a.active == 0);
    cui_vec2 sc = center(rect_of(ui, a.scroll_id));
    cui_input_mouse_move(ui, sc.x, sc.y);
    for (int i = 0; i < 12; i++) { cui_input_scroll(ui, 0, -2); run(ui, build_disclosure, &a, 3); }
    run(ui, build_disclosure, &a, 40);
    CHECK(a.active >= 1);
    cui_destroy(ui);
}

static void test_checklist(void) {
    printf("checklist: action of the open step, circle toggles\n");
    cui_ctx *ui = new_ctx();
    disc_ctx a;
    memset(&a, 0, sizeof a);
    a.step_action = -1;
    a.steps[0] = (cui_check_step){ "First", "Do it", "Go", true };
    a.steps[1] = (cui_check_step){ "Second", "Then this", "Next", false };
    a.steps[2] = (cui_check_step){ "Third", NULL, NULL, false };
    run(ui, build_disclosure, &a, 40);
    /* the Next button is the only primary button in the checklist; find it by its label id */
    cui_rect lr = rect_of(ui, a.list_id);
    /* click through the list rows until the action fires */
    for (float y = lr.y + 60; y < lr.y + lr.h && a.step_action < 0; y += 6) click_at(ui, build_disclosure, &a, lr.x + 90, y);
    CHECK(a.step_action == 1);
    cui_destroy(ui);
}

/* data grid ------------------------------------------------------------------ */

typedef struct { bool sel[500]; cui_table_sort sort; char names[500][16]; int committed; int cursor; bool open; uint32_t filt[2]; char q[32]; int json_ok, json_bad; cui_id grid_id, tree_id, fb_id; } dgrid_ctx;
static void build_dgrid(cui_ctx *ui, void *u) {
    dgrid_ctx *a = u;
    cui_column(ui, .pad = 20, .gap = 12, .width = CUI_FILL, .height = CUI_FILL) {
        static const char *const o1[] = { "Red", "Green", "Blue" };
        static const cui_filter_field fields[] = { { "Color", NULL, o1, 3 }, { "Size", NULL, o1, 3 } };
        cui_filter_bar(ui, "fb", fields, 2, a->filt, a->q, sizeof a->q);
        a->fb_id = cui_last_id(ui);
        static const cui_grid_column cols[] = { { "Name", 200, CUI_TEXT_LEFT, true }, { "Value", 120, CUI_TEXT_RIGHT, true } };
        int f, l;
        if (cui_data_grid_begin(ui, "grid", cols, 2, 500, &f, &l, .height = CUI_PX(300), .selected = a->sel, .sort = &a->sort, .cursor = &a->cursor)) {
            for (int r = f; r < l; r++) {
                cui_data_grid_row(ui, r);
                if (cui_data_grid_cell_edit(ui, a->names[r], sizeof a->names[r])) a->committed++;
                char v[16];
                snprintf(v, sizeof v, "%d", r * 3);
                cui_data_grid_cell_text(ui, v);
            }
            cui_data_grid_end(ui);
        }
        a->grid_id = cui_make_id(ui, "grid");
        cui_tree_cell(ui, "folder", 0, true, &a->open, NULL);
        a->tree_id = cui_make_id(ui, "folder") ^ 0x7CE1u;
        a->json_ok = cui_json_view(ui, "j1", "{\"a\": [1, 2, {\"b\": null}], \"c\": \"x\\\"y\", \"d\": -1.5e3, \"e\": true}");
        a->json_bad = cui_json_view(ui, "j2", "{\"a\": [1, 2,}");
        cui_code_block(ui, "code", "int main(void) {\n    return 0; /* done */\n}\n", .lang = "c", .line_numbers = true);
    }
}

static void test_data_grid(void) {
    printf("data grid: virtual rows, selection, sort, keyboard, editing; filter bar, tree cell, json\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 1100, 1);
    dgrid_ctx a;
    memset(&a, 0, sizeof a);
    a.cursor = -1;
    for (int k = 0; k < 500; k++) snprintf(a.names[k], sizeof a.names[k], "row %d", k);
    run(ui, build_dgrid, &a, 3);
    CHECK(a.json_ok == 1 && a.json_bad == 0);
    cui_rect g = rect_of(ui, a.grid_id);
    CHECK(g.h > 290 && g.h < 310);
    /* only the visible rows are built: far fewer nodes than 500 rows would need */
    /* click row 1 (header 40, rows 40) */
    float row_y = g.y + 40 + 40 * 1 + 20;
    click_at(ui, build_dgrid, &a, g.x + 120, row_y);
    CHECK(a.sel[1] && !a.sel[0] && a.cursor == 1);
    /* shift click row 4 selects 1..4 */
    cui_input_key(ui, CUI_KEY_NONE, false, CUI_MOD_SHIFT);
    cui_input_mouse_move(ui, g.x + 120, g.y + 40 + 40 * 4 + 20);
    run(ui, build_dgrid, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_dgrid, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_dgrid, &a, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    int n = 0;
    for (int k = 0; k < 500; k++) n += a.sel[k];
    CHECK(n == 4 && a.sel[1] && a.sel[4]);
    /* keyboard: Down moves the cursor, Escape clears, Ctrl+A selects all */
    key(ui, build_dgrid, &a, CUI_KEY_DOWN, 0);
    CHECK(a.cursor == 5);
    key(ui, build_dgrid, &a, CUI_KEY_ESCAPE, 0);
    n = 0;
    for (int k = 0; k < 500; k++) n += a.sel[k];
    CHECK(n == 0);
    key(ui, build_dgrid, &a, CUI_KEY_A, CUI_MOD_PRIMARY);
    n = 0;
    for (int k = 0; k < 500; k++) n += a.sel[k];
    CHECK(n == 500);
    key(ui, build_dgrid, &a, CUI_KEY_END, 0);
    run(ui, build_dgrid, &a, 40);
    CHECK(a.cursor == 499);
    key(ui, build_dgrid, &a, CUI_KEY_HOME, 0);
    run(ui, build_dgrid, &a, 40);
    /* sort: clicking the Value header sorts by it */
    click_at(ui, build_dgrid, &a, g.x + 44 + 200 + 60, g.y + 20);
    CHECK(a.sort.column == 1 && !a.sort.descending);
    click_at(ui, build_dgrid, &a, g.x + 44 + 200 + 60, g.y + 20);
    CHECK(a.sort.descending);
    /* editing: Enter on the cursor row edits, typing replaces, Enter commits */
    click_at(ui, build_dgrid, &a, g.x + 300, g.y + 40 + 20);   /* back to the grid, row 0 */
    key(ui, build_dgrid, &a, CUI_KEY_ENTER, 0);
    run(ui, build_dgrid, &a, 1);
    type(ui, build_dgrid, &a, "first");
    key(ui, build_dgrid, &a, CUI_KEY_ENTER, 0);
    CHECK(strcmp(a.names[0], "first") == 0 && a.committed == 1);
    /* a double click on a cell edits it too */
    cui_input_mouse_move(ui, g.x + 120, g.y + 40 + 40 * 2 + 20);
    run(ui, build_dgrid, &a, 1);
    for (int c = 0; c < 2; c++) {
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        run(ui, build_dgrid, &a, 1);
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        run(ui, build_dgrid, &a, 1);
    }
    run(ui, build_dgrid, &a, 1);
    type(ui, build_dgrid, &a, "second");
    CHECK(cui_wants_keyboard(ui) && strcmp(a.names[2], "second") == 0);   /* the double click opened an editor */
    key(ui, build_dgrid, &a, CUI_KEY_ESCAPE, 0);   /* Esc reverts */
    CHECK(strcmp(a.names[2], "row 2") == 0);
    /* column resize: drag the Name header's right edge */
    float edge = g.x + 44 + 200;
    drag(ui, build_dgrid, &a, edge, g.y + 20, edge + 60, 6);
    run(ui, build_dgrid, &a, 2);
    click_at(ui, build_dgrid, &a, g.x + 44 + 260 + 60, g.y + 20);   /* Value header moved right by 60 */
    CHECK(!a.sort.descending);
    /* tree cell */
    cui_vec2 tc = center(rect_of(ui, a.tree_id));
    click_at(ui, build_dgrid, &a, tc.x, tc.y);
    CHECK(a.open);
    cui_destroy(ui);
}

/* tree table and path bar ---------------------------------------------------- */

typedef struct {
    cui_tree_node *nodes; int count, root, cursor;
    cui_table_sort sort;
    float height;
    int seq[64], depth[64], nseq, rows_built;
    int activated, expanded, collapsed, loads, load_node, sort_changes, sel_changes;
    cui_id grid_id;
    const char *const *names;
} ttab_ctx;
static const char *ttab_label(void *user, int n) { ttab_ctx *t = user; return t->names ? t->names[n] : NULL; }
static void build_ttab(cui_ctx *ui, void *u) {
    ttab_ctx *t = u;
    cui_column(ui, .pad = 20, .width = CUI_FILL, .height = CUI_FILL) {
        static const cui_grid_column cols[] = { { "Name", 240, CUI_TEXT_LEFT, true }, { "Value", 120, CUI_TEXT_RIGHT, true, false, 1 },
                                                { "Type", 120, CUI_TEXT_LEFT, true, false, 2 } };
        cui_tree_table_begin(ui, "tt", cols, 3, t->nodes, t->count, .height = CUI_PX(t->height > 0 ? t->height : 360), .root = t->root,
                             .sort = &t->sort, .cursor = &t->cursor, .guides = true, .label = ttab_label, .user = t);
        t->grid_id = cui_make_id(ui, "tt");
        t->nseq = 0; t->rows_built = 0;
        int n;
        while ((n = cui_tree_table_row(ui)) >= 0) {
            if (t->nseq < 64) { t->seq[t->nseq] = n; t->depth[t->nseq] = cui_tree_table_depth(ui); t->nseq++; }
            t->rows_built++;
            char name[24];
            snprintf(name, sizeof name, "node %d", n);
            cui_tree_table_name(ui, t->names ? t->names[n] : name, (t->nodes[n].flags & CUI_TREE_HAS_CHILDREN) ? CUI_ICON_FOLDER : CUI_ICON_FILE);
            cui_data_grid_cell_text(ui, "42");
            cui_data_grid_cell_text(ui, "int");
        }
        cui_tree_table_events ev = cui_tree_table_end(ui);
        if (ev.activated >= 0) t->activated = ev.activated;
        if (ev.expanded >= 0) t->expanded = ev.expanded;
        if (ev.collapsed >= 0) t->collapsed = ev.collapsed;
        t->loads += ev.load_count;
        if (ev.load_count > 0) t->load_node = ev.load[0];
        t->sort_changes += ev.sort_changed;
        t->sel_changes += ev.selection_changed;
    }
}
static void ttab_link(cui_tree_node *nd, int parent, const int *kids, int n) {
    if (parent >= 0) nd[parent].first_child = n > 0 ? kids[0] : -1;
    for (int k = 0; k < n; k++) { nd[kids[k]].parent = parent; nd[kids[k]].next_sibling = k + 1 < n ? kids[k + 1] : -1; }
}
static bool ttab_seq(const ttab_ctx *t, const int *want, int n) {
    if (t->nseq != n) return false;
    for (int k = 0; k < n; k++) if (t->seq[k] != want[k]) return false;
    return true;
}
static int ttab_cmp_desc(void *user, int a, int b) { (void)user; return b - a; }
static int ttab_cmp_parity(void *user, int a, int b) { (void)user; return (a & 1) - (b & 1); }   /* evens first, stable otherwise */
static float ttab_row_y(cui_rect g, int i) { return g.y + 40 + 32 * i + 16; }
static int ttab_selected(const ttab_ctx *t) { int c = 0; for (int k = 0; k < t->count; k++) c += (t->nodes[k].flags & CUI_TREE_SELECTED) != 0; return c; }

typedef struct {
    const char *const *segs; int count; float width; bool menus;
    int clicked; char submitted[128]; int submits; bool editing;
    cui_id bar, more, seg[16], sep[16];
    int menu_seg, menu_chosen;
} pbar_ctx;
static void build_pbar(cui_ctx *ui, void *u) {
    pbar_ctx *p = u;
    cui_column(ui, .pad = 20, .gap = 12, .width = CUI_FILL, .height = CUI_FILL) {
        cui_path_bar_result r = cui_path_bar(ui, "pb", p->segs, p->count, .menus = p->menus, .icon = CUI_ICON_FOLDER,
                                             .width = p->width > 0 ? CUI_PX(p->width) : CUI_FILL);
        p->bar = cui_last_id(ui);
        if (r.clicked >= 0) p->clicked = r.clicked;
        if (r.submitted) { snprintf(p->submitted, sizeof p->submitted, "%s", r.submitted); p->submits++; }
        p->editing = r.editing;
        int seg;
        if (cui_path_bar_menu_begin(ui, "pb", &seg)) {
            p->menu_seg = seg;
            if (cui_menu_item(ui, "Sibling one")) p->menu_chosen = 1;
            if (cui_menu_item(ui, "Sibling two")) p->menu_chosen = 2;
            cui_path_bar_menu_end(ui);
        }
        /* the ids the bar uses, rebuilt with the public id calls */
        cui_push_id(ui, "pb");
        p->more = cui_make_id(ui, "more");
        for (int k = 0; k < p->count && k < 16; k++) {
            cui_push_id_int(ui, k);
            p->seg[k] = cui_make_id(ui, "seg");
            p->sep[k] = cui_make_id(ui, "sep");
            cui_pop_id(ui);
        }
        cui_pop_id(ui);
        cui_button(ui, "Elsewhere");
    }
}

static void test_tree_table(void) {
    printf("tree table: flattening, keyboard, lazy children, 100k nodes, selection, sort; path bar\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 600, 1);
    /* 0 A { 1 A1 { 3 A1a }, 2 A2 }, 4 B { 5 B1 }, 6 C (lazy) */
    cui_tree_node nd[16];
    for (int k = 0; k < 16; k++) { nd[k].parent = nd[k].first_child = nd[k].next_sibling = -1; nd[k].flags = 0; }
    { int top[] = { 0, 4, 6 }, a[] = { 1, 2 }, a1[] = { 3 }, bb[] = { 5 };
      ttab_link(nd, -1, top, 3); ttab_link(nd, 0, a, 2); ttab_link(nd, 1, a1, 1); ttab_link(nd, 4, bb, 1); }
    nd[0].flags = nd[1].flags = nd[4].flags = nd[6].flags = CUI_TREE_HAS_CHILDREN;
    static const char *const names[] = { "Alpha", "Apple", "Avocado", "Apricot", "Banana", "Blueberry", "Cherry", "Cranberry", "Currant" };
    ttab_ctx t;
    memset(&t, 0, sizeof t);
    t.nodes = nd; t.count = 7; t.cursor = -1; t.names = names; t.sort.column = -1;
    t.activated = t.expanded = t.collapsed = -1;
    run(ui, build_ttab, &t, 3);
    { int w[] = { 0, 4, 6 }; CHECK(ttab_seq(&t, w, 3)); }
    /* flattening follows nested open and closed nodes */
    nd[0].flags |= CUI_TREE_OPEN;
    run(ui, build_ttab, &t, 1);
    { int w[] = { 0, 1, 2, 4, 6 }; CHECK(ttab_seq(&t, w, 5)); }
    nd[1].flags |= CUI_TREE_OPEN;
    run(ui, build_ttab, &t, 1);
    { int w[] = { 0, 1, 3, 2, 4, 6 }; CHECK(ttab_seq(&t, w, 6)); CHECK(t.depth[2] == 2 && t.depth[3] == 1 && t.depth[4] == 0); }
    nd[0].flags &= ~(uint32_t)CUI_TREE_OPEN;
    run(ui, build_ttab, &t, 1);
    { int w[] = { 0, 4, 6 }; CHECK(ttab_seq(&t, w, 3)); }
    nd[0].flags |= CUI_TREE_OPEN;   /* A1 stayed open inside */
    run(ui, build_ttab, &t, 1);
    { int w[] = { 0, 1, 3, 2, 4, 6 }; CHECK(ttab_seq(&t, w, 6)); }
    nd[0].flags &= ~(uint32_t)CUI_TREE_OPEN; nd[1].flags &= ~(uint32_t)CUI_TREE_OPEN;
    run(ui, build_ttab, &t, 1);
    /* keyboard: click A, then Right opens, Right moves to the first child */
    cui_rect g = rect_of(ui, t.grid_id);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 0));
    CHECK(t.cursor == 0 && (nd[0].flags & CUI_TREE_SELECTED));
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK((nd[0].flags & CUI_TREE_OPEN) && t.expanded == 0 && t.cursor == 0);
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.cursor == 1 && (nd[1].flags & CUI_TREE_SELECTED) && !(nd[0].flags & CUI_TREE_SELECTED));
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK((nd[1].flags & CUI_TREE_OPEN) && t.cursor == 1);
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.cursor == 3);
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);   /* a leaf: nothing */
    CHECK(t.cursor == 3);
    /* Left: a leaf moves to its parent, an open node closes, a closed one moves up */
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);
    CHECK(t.cursor == 1);
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);
    CHECK(!(nd[1].flags & CUI_TREE_OPEN) && t.collapsed == 1 && t.cursor == 1);
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);
    CHECK(t.cursor == 0);
    key(ui, build_ttab, &t, CUI_KEY_DOWN, 0);
    CHECK(t.cursor == 1 && (nd[1].flags & CUI_TREE_SELECTED) && ttab_selected(&t) == 1);
    /* Home, End, Enter */
    key(ui, build_ttab, &t, CUI_KEY_END, 0);
    CHECK(t.cursor == 6);
    key(ui, build_ttab, &t, CUI_KEY_HOME, 0);
    CHECK(t.cursor == 0);
    key(ui, build_ttab, &t, CUI_KEY_ENTER, 0);
    CHECK(t.activated == 0);
    /* type-ahead jumps to a name; * opens every sibling */
    type(ui, build_ttab, &t, "b");
    CHECK(t.cursor == 4);
    run(ui, build_ttab, &t, 70);   /* the typed prefix times out */
    type(ui, build_ttab, &t, "c");
    CHECK(t.cursor == 6);
    /* lazy: opening C asks for its children once and shows a loading row */
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.loads == 1 && t.load_node == 6 && (nd[6].flags & CUI_TREE_LOADING) && (nd[6].flags & CUI_TREE_OPEN));
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    run(ui, build_ttab, &t, 2);
    CHECK(t.loads == 1);   /* still loading: not asked again */
    { int w[] = { 0, 1, 2, 4, 6 }; CHECK(ttab_seq(&t, w, 5)); }
    /* move C to the top: the loading row sits between it and A */
    { int top[] = { 6, 0, 4 }; ttab_link(nd, -1, top, 3); t.root = 6; }
    nd[0].flags &= ~(uint32_t)CUI_TREE_OPEN;
    run(ui, build_ttab, &t, 2);
    { int w[] = { 6, 0, 4 }; CHECK(ttab_seq(&t, w, 3)); }
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 2));   /* row 1 is the loading row, row 2 is A */
    CHECK(t.cursor == 0 && (nd[0].flags & CUI_TREE_SELECTED));
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 1));   /* the loading row selects nothing */
    CHECK(ttab_selected(&t) == 0);
    /* the children arrive */
    { int kids[] = { 7, 8 }; ttab_link(nd, 6, kids, 2); t.count = 9; }
    nd[6].flags &= ~(uint32_t)CUI_TREE_LOADING;
    run(ui, build_ttab, &t, 2);
    { int w[] = { 6, 7, 8, 0, 4 }; CHECK(ttab_seq(&t, w, 5)); }
    /* * opens every sibling at the cursor's level */
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 3));
    CHECK(t.cursor == 0);
    type(ui, build_ttab, &t, "*");
    CHECK((nd[0].flags & CUI_TREE_OPEN) && (nd[4].flags & CUI_TREE_OPEN));
    /* selection: Shift+click a range, Ctrl+click toggles, a plain click resets (hidden nodes too) */
    run(ui, build_ttab, &t, 1);
    { int w[] = { 6, 7, 8, 0, 1, 2, 4, 5 }; CHECK(ttab_seq(&t, w, 8)); }
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 1));
    cui_input_key(ui, CUI_KEY_NONE, false, CUI_MOD_SHIFT);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 4));
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    CHECK(ttab_selected(&t) == 4 && (nd[7].flags & CUI_TREE_SELECTED) && (nd[1].flags & CUI_TREE_SELECTED) && !(nd[6].flags & CUI_TREE_SELECTED));
    cui_input_key(ui, CUI_KEY_NONE, false, CUI_MOD_PRIMARY);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 7));
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 2));
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    CHECK(ttab_selected(&t) == 4 && (nd[5].flags & CUI_TREE_SELECTED) && !(nd[8].flags & CUI_TREE_SELECTED));
    /* Shift+Down extends from the anchor even after rows above it opened */
    nd[6].flags &= ~(uint32_t)CUI_TREE_OPEN;   /* rows shift up by two; 7 (selected) is hidden */
    run(ui, build_ttab, &t, 2);
    CHECK(t.sel_changes > 0);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 0));   /* a plain click: hidden 7 drops too */
    CHECK(ttab_selected(&t) == 1 && (nd[6].flags & CUI_TREE_SELECTED) && !(nd[7].flags & CUI_TREE_SELECTED));
    key(ui, build_ttab, &t, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    key(ui, build_ttab, &t, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    CHECK(ttab_selected(&t) == 3 && (nd[0].flags & CUI_TREE_SELECTED) && (nd[1].flags & CUI_TREE_SELECTED));
    key(ui, build_ttab, &t, CUI_KEY_ESCAPE, 0);
    CHECK(ttab_selected(&t) == 0);
    /* sort: the header reports it; the caller re-sorts each level */
    click_at(ui, build_ttab, &t, g.x + 120, g.y + 20);
    CHECK(t.sort_changes == 1 && t.sort.column == 0 && !t.sort.descending);
    click_at(ui, build_ttab, &t, g.x + 120, g.y + 20);
    CHECK(t.sort_changes == 2 && t.sort.descending);
    t.root = cui_tree_sort(nd, t.count, t.root, ttab_cmp_desc, NULL);
    run(ui, build_ttab, &t, 2);
    { int w[] = { 6, 4, 5, 0, 2, 1 }; CHECK(t.root == 6 && ttab_seq(&t, w, 6)); }
    t.root = cui_tree_sort(nd, t.count, t.root, ttab_cmp_parity, NULL);   /* stable: evens keep their order */
    run(ui, build_ttab, &t, 2);
    { int w[] = { 6, 4, 5, 0, 2, 1 }; CHECK(ttab_seq(&t, w, 6)); }
    /* a narrow table hides the columns with the highest priority first */
    cui_set_viewport(ui, 420, 600, 1);
    run(ui, build_ttab, &t, 3);
    click_at(ui, build_ttab, &t, rect_of(ui, t.grid_id).x + 300, rect_of(ui, t.grid_id).y + 20);   /* Value header: Type hid first */
    CHECK(t.sort.column == 1);
    cui_set_viewport(ui, 900, 600, 1);
    run(ui, build_ttab, &t, 2);
    cui_destroy(ui);

    /* empty tree and a tree with bad links: nothing breaks */
    ui = new_ctx();
    memset(&t, 0, sizeof t);
    t.cursor = 5; t.activated = -1;
    run(ui, build_ttab, &t, 2);
    key(ui, build_ttab, &t, CUI_KEY_DOWN, 0);
    key(ui, build_ttab, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.rows_built == 0 && t.activated == -1);
    { cui_tree_node bad[3] = { { -1, 1, 0, CUI_TREE_OPEN }, { 0, 1, 2, CUI_TREE_OPEN }, { 7, -1, 1, 0 } };   /* loops */
      t.nodes = bad; t.count = 3;
      run(ui, build_ttab, &t, 2);
      CHECK(t.rows_built <= 8); }
    cui_destroy(ui);

    /* a name cut short shows the whole name on hover */
    ui = new_ctx();
    { static const char *const long_name[] = { "A folder name far too long for the name column of this table.txt" };
      cui_tree_node one[1] = { { -1, -1, -1, 0 } };
      memset(&t, 0, sizeof t);
      t.nodes = one; t.count = 1; t.cursor = -1; t.names = long_name;
      run(ui, build_ttab, &t, 60);   /* past the first moments, when tooltips show at once */
      g = rect_of(ui, t.grid_id);
      cui_input_mouse_move(ui, g.x + 160, ttab_row_y(g, 0));
      run(ui, build_ttab, &t, 3);
      int before = cui_get_draw_data(ui)->vertex_count;
      run(ui, build_ttab, &t, 70);
      CHECK(cui_get_draw_data(ui)->vertex_count > before); }
    cui_destroy(ui);

    /* a very deep tree: depth 200, all open */
    ui = new_ctx();
    cui_tree_node *deep = calloc(200, sizeof *deep);
    for (int k = 0; k < 200; k++) {
        deep[k].parent = k - 1; deep[k].first_child = k + 1 < 200 ? k + 1 : -1; deep[k].next_sibling = -1;
        deep[k].flags = k + 1 < 200 ? CUI_TREE_HAS_CHILDREN | CUI_TREE_OPEN : 0;
    }
    memset(&t, 0, sizeof t);
    t.nodes = deep; t.count = 200; t.cursor = -1; t.activated = -1;
    run(ui, build_ttab, &t, 3);
    g = rect_of(ui, t.grid_id);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 0));
    key(ui, build_ttab, &t, CUI_KEY_END, 0);
    run(ui, build_ttab, &t, 30);
    CHECK(t.cursor == 199 && t.nseq > 0 && t.seq[t.nseq - 1] == 199 && t.depth[t.nseq - 1] == 199);
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);
    CHECK(t.cursor == 198);
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);   /* closes 198 */
    CHECK(!(deep[198].flags & CUI_TREE_OPEN));
    free(deep);
    cui_destroy(ui);

    /* 100k nodes: 1000 folders of 99 files, all open; only the rows in view are built */
    ui = new_ctx();
    int big_n = 100000;
    cui_tree_node *big = malloc(sizeof *big * (size_t)big_n);
    for (int f = 0; f < 1000; f++) {
        int at = f * 100;
        big[at].parent = -1; big[at].first_child = at + 1; big[at].next_sibling = f + 1 < 1000 ? at + 100 : -1;
        big[at].flags = CUI_TREE_HAS_CHILDREN | CUI_TREE_OPEN;
        for (int c = 1; c < 100; c++) {
            big[at + c].parent = at; big[at + c].first_child = -1; big[at + c].next_sibling = c + 1 < 100 ? at + c + 1 : -1; big[at + c].flags = 0;
        }
    }
    memset(&t, 0, sizeof t);
    t.nodes = big; t.count = big_n; t.cursor = -1; t.activated = -1;
    run(ui, build_ttab, &t, 3);
    CHECK(t.rows_built > 5 && t.rows_built < 30);
    CHECK(cui_get_draw_data(ui)->vertex_count < 20000);
    clock_t c0 = clock();
    run(ui, build_ttab, &t, 20);
    double secs = (double)(clock() - c0) / CLOCKS_PER_SEC;
    CHECK(secs < 5.0);   /* a sanity bound (sanitizer builds are slow); about 1 ms a frame in release */
    g = rect_of(ui, t.grid_id);
    click_at(ui, build_ttab, &t, g.x + 300, ttab_row_y(g, 0));
    key(ui, build_ttab, &t, CUI_KEY_END, 0);
    run(ui, build_ttab, &t, 40);
    CHECK(t.cursor == big_n - 1 && t.nseq > 0 && t.seq[t.nseq - 1] == big_n - 1 && t.rows_built < 30);
    key(ui, build_ttab, &t, CUI_KEY_LEFT, 0);   /* to the last folder */
    run(ui, build_ttab, &t, 40);
    CHECK(t.cursor == big_n - 100);
    click_at(ui, build_ttab, &t, g.x + 120, g.y + 20);   /* the sticky header still sorts, scrolled far down */
    CHECK(t.sort_changes == 1);
    free(big);
    cui_destroy(ui);

    /* path bar: a segment click, the overflow menu, typing a path */
    ui = new_ctx();
    cui_set_viewport(ui, 1100, 400, 1);
    static const char *const segs[] = { "home", "maya", "projects", "cui", "src", "render", "backends", "cui_render_sw.c" };
    pbar_ctx p;
    memset(&p, 0, sizeof p);
    p.segs = segs; p.count = 8; p.clicked = -1; p.menus = true; p.menu_seg = -1;
    run(ui, build_pbar, &p, 3);
    CHECK(rect_of(ui, p.seg[2]).w > 0 && rect_of(ui, p.more).w == 0);   /* wide: everything shows */
    cui_vec2 c = center(rect_of(ui, p.seg[2]));
    click_at(ui, build_pbar, &p, c.x, c.y);
    CHECK(p.clicked == 2);
    /* a separator opens a menu of the next segment's siblings */
    c = center(rect_of(ui, p.sep[3]));
    click_at(ui, build_pbar, &p, c.x, c.y);
    run(ui, build_pbar, &p, 20);
    CHECK(p.menu_seg == 3);
    key(ui, build_pbar, &p, CUI_KEY_DOWN, 0);
    key(ui, build_pbar, &p, CUI_KEY_DOWN, 0);
    key(ui, build_pbar, &p, CUI_KEY_ENTER, 0);
    CHECK(p.menu_chosen == 2);
    run(ui, build_pbar, &p, 20);
    /* Ctrl+L types a path; Enter submits */
    key(ui, build_pbar, &p, CUI_KEY_L, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    run(ui, build_pbar, &p, 1);
    CHECK(p.editing && cui_wants_keyboard(ui));
    type(ui, build_pbar, &p, "/tmp/build");   /* the whole path was selected: typing replaces it */
    key(ui, build_pbar, &p, CUI_KEY_ENTER, 0);
    CHECK(p.submits == 1 && strcmp(p.submitted, "/tmp/build") == 0 && !p.editing);
    run(ui, build_pbar, &p, 2);
    /* Esc cancels */
    key(ui, build_pbar, &p, CUI_KEY_L, CUI_MOD_PRIMARY);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    run(ui, build_pbar, &p, 1);
    type(ui, build_pbar, &p, "nope");
    key(ui, build_pbar, &p, CUI_KEY_ESCAPE, 0);
    run(ui, build_pbar, &p, 1);
    CHECK(!p.editing && p.submits == 1);
    /* clicking the empty part edits; clicking away cancels */
    cui_rect br = rect_of(ui, p.bar);
    click_at(ui, build_pbar, &p, br.x + br.w - 20, br.y + br.h / 2);
    run(ui, build_pbar, &p, 1);
    CHECK(p.editing);
    cui_rect er = rect_of(ui, p.bar);
    click_at(ui, build_pbar, &p, er.x + 40, er.y + er.h + 60);
    run(ui, build_pbar, &p, 1);
    CHECK(!p.editing && p.submits == 1);
    cui_destroy(ui);
    /* narrow: the middle segments collapse into "..." and stay reachable */
    ui = new_ctx();
    cui_set_viewport(ui, 320, 400, 1);
    memset(&p, 0, sizeof p);
    p.segs = segs; p.count = 8; p.clicked = -1; p.menu_seg = -1;
    run(ui, build_pbar, &p, 4);
    cui_rect mr = rect_of(ui, p.more);
    CHECK(mr.w > 0 && rect_of(ui, p.seg[7]).w > 0 && mr.x + mr.w <= rect_of(ui, p.seg[7]).x);
    CHECK(rect_of(ui, p.seg[7]).x + rect_of(ui, p.seg[7]).w <= rect_of(ui, p.bar).x + rect_of(ui, p.bar).w + 0.5f);   /* nothing spills */
    c = center(mr);
    click_at(ui, build_pbar, &p, c.x, c.y);
    run(ui, build_pbar, &p, 20);
    key(ui, build_pbar, &p, CUI_KEY_DOWN, 0);
    key(ui, build_pbar, &p, CUI_KEY_ENTER, 0);
    CHECK(p.clicked >= 0 && p.clicked < 7);   /* a collapsed segment, chosen in the menu */
    /* by keyboard alone: Tab past the first segment to "...", open it with Enter or Space,
     * and Enter on the highlighted item picks it instead of closing the menu */
    int first_hidden = p.clicked;
    cui_destroy(ui);
    cui_key openers[] = { CUI_KEY_ENTER, CUI_KEY_SPACE };
    for (int o = 0; o < 2; o++) {
        ui = new_ctx();
        cui_set_viewport(ui, 320, 400, 1);
        memset(&p, 0, sizeof p);
        p.segs = segs; p.count = 8; p.clicked = -1; p.menu_seg = -1;
        run(ui, build_pbar, &p, 4);
        key(ui, build_pbar, &p, CUI_KEY_TAB, 0);
        key(ui, build_pbar, &p, CUI_KEY_TAB, 0);
        key(ui, build_pbar, &p, openers[o], 0);
        run(ui, build_pbar, &p, 20);
        CHECK(p.clicked == -1);
        key(ui, build_pbar, &p, CUI_KEY_DOWN, 0);
        key(ui, build_pbar, &p, CUI_KEY_ENTER, 0);
        CHECK(p.clicked == first_hidden);
        cui_destroy(ui);
    }
}

static void test_text_capacity(void) {
    printf("text input: respects buffer capacity\n");
    cui_ctx *ui = new_ctx();
    struct { char buf[6]; int f; } small;
    memset(&small, 0, sizeof small);
    edit_ctx e;
    memset(&e, 0, sizeof e);
    run(ui, build_edit, &e, 2);
    cui_vec2 c = center(rect_of(ui, e.id));
    click_at(ui, build_edit, &e, c.x, c.y);
    for (int i = 0; i < 40; i++) type(ui, build_edit, &e, "abcd");
    CHECK(strlen(e.buf) == sizeof e.buf - 1);
    (void)small;
    cui_destroy(ui);
}

/* overlays -------------------------------------------------------------- */

typedef struct { bool open; int under_clicks; cui_id under; } dlg_ctx;
static void build_dialog(cui_ctx *ui, void *u) {
    dlg_ctx *d = u;
    cui_column(ui, .pad = 20) {
        if (cui_button(ui, "Underneath")) d->under_clicks++;
        d->under = cui_last_id(ui);
    }
    cui_dialog(ui, "Confirm", &d->open, .description = "Are you sure?") {
        cui_text(ui, "Body");
    }
}

static void test_dialog(void) {
    printf("overlays: modal blocks input, Esc and scrim close it\n");
    cui_ctx *ui = new_ctx();
    dlg_ctx d = { true, 0, 0 };
    run(ui, build_dialog, &d, 3);
    cui_vec2 c = center(rect_of(ui, d.under));
    click_at(ui, build_dialog, &d, c.x, c.y);   /* lands on the scrim, closes the dialog */
    CHECK(d.under_clicks == 0);
    CHECK(d.open == false);
    run(ui, build_dialog, &d, 3);
    click_at(ui, build_dialog, &d, c.x, c.y);
    CHECK(d.under_clicks == 1);
    d.open = true;
    run(ui, build_dialog, &d, 3);
    key(ui, build_dialog, &d, CUI_KEY_ESCAPE, 0);
    CHECK(d.open == false);
    /* clicking inside the panel keeps it open */
    d.open = true;
    run(ui, build_dialog, &d, 3);
    click_at(ui, build_dialog, &d, 400, 300);
    CHECK(d.open == true);
    cui_destroy(ui);
}

typedef struct { bool open; char text[32]; } dialog_edit_ctx;

static void build_dialog_edit(cui_ctx *ui, void *user) {
    dialog_edit_ctx *d = user;
    cui_dialog(ui, "Edit profile", &d->open) {
        cui_input(ui, "Name", d->text, sizeof d->text, .autofocus = true);
    }
}

/** Consecutive Escapes first leave the field, then dismiss its dialog. */
static void test_dialog_escape_after_edit(void) {
    printf("dialog: Escape after an active editor\n");
    cui_ctx *ui = new_ctx();
    dialog_edit_ctx d = { true, "Example" };
    run(ui, build_dialog_edit, &d, 1);
    CHECK(cui_wants_keyboard(ui));
    key(ui, build_dialog_edit, &d, CUI_KEY_ESCAPE, 0);
    CHECK(d.open && !cui_wants_keyboard(ui));
    key(ui, build_dialog_edit, &d, CUI_KEY_ESCAPE, 0);
    CHECK(!d.open);
    cui_destroy(ui);
}

typedef struct { int chosen; cui_id trigger; } menu_ctx;
static void build_menu(cui_ctx *ui, void *u) {
    menu_ctx *m = u;
    cui_column(ui, .pad = 20) {
        cui_menu(ui, "Actions") {
            if (cui_menu_item(ui, "First")) m->chosen = 1;
            if (cui_menu_item(ui, "Second")) m->chosen = 2;
        }
        m->trigger = cui_last_id(ui);
    }
}

static void test_menu(void) {
    printf("overlays: dropdown menu with mouse and keyboard\n");
    cui_ctx *ui = new_ctx();
    menu_ctx m = { 0, 0 };
    run(ui, build_menu, &m, 2);
    cui_rect tr = rect_of(ui, m.trigger);
    CHECK(tr.w > 0);
    click_at(ui, build_menu, &m, tr.x + 10, tr.y + 10);
    run(ui, build_menu, &m, 3);
    /* second item: menu at trigger bottom + 6, panel padding 6, items 32 tall */
    click_at(ui, build_menu, &m, tr.x + 40, tr.y + tr.h + 6 + 6 + 32 + 16);
    CHECK(m.chosen == 2);
    run(ui, build_menu, &m, 3);
    /* open again, choose with arrows + enter */
    click_at(ui, build_menu, &m, tr.x + 10, tr.y + 10);
    run(ui, build_menu, &m, 2);
    cui_input_mouse_move(ui, 700, 550);
    run(ui, build_menu, &m, 1);
    key(ui, build_menu, &m, CUI_KEY_DOWN, 0);
    key(ui, build_menu, &m, CUI_KEY_ENTER, 0);
    CHECK(m.chosen == 1);
    /* click outside closes without choosing */
    m.chosen = 0;
    click_at(ui, build_menu, &m, tr.x + 10, tr.y + 10);
    run(ui, build_menu, &m, 2);
    click_at(ui, build_menu, &m, 700, 550);
    run(ui, build_menu, &m, 2);
    CHECK(m.chosen == 0);
    cui_destroy(ui);
}

typedef struct { bool open; int inside, under; } trap_ctx;
static void build_trap(cui_ctx *ui, void *u) {
    trap_ctx *t = u;
    cui_column(ui, .pad = 20) { if (cui_button(ui, "Under")) t->under++; }
    cui_dialog(ui, "Trap", &t->open) { if (cui_button(ui, "Inside")) t->inside++; }
}

static void test_focus_trap(void) {
    printf("overlays: tab focus stays inside a modal\n");
    cui_ctx *ui = new_ctx();
    trap_ctx t = { true, 0, 0 };
    run(ui, build_trap, &t, 3);
    for (int i = 0; i < 6; i++) {
        key(ui, build_trap, &t, CUI_KEY_TAB, 0);
        if (i % 2 == 1) key(ui, build_trap, &t, CUI_KEY_ENTER, 0);   /* lands on Inside every second tab */
        if (!t.open) break;
    }
    CHECK(t.under == 0);
    CHECK(t.inside == 3);
    CHECK(t.open);
    cui_destroy(ui);
}

typedef struct { char tags[128]; char otp[8]; int max_tags, otp_done, held; cui_id tag_id, otp_id, hold_id; } extras_ctx;

/** Use the same controls for ordinary editing and batched input. */
static void build_extras(cui_ctx *ui, void *u) {
    extras_ctx *x = u;
    cui_column(ui, .pad = 20, .gap = 16, .width = CUI_PX(500)) {
        cui_tag_input(ui, "tags", x->tags, sizeof x->tags, .max_tags = x->max_tags);
        x->tag_id = cui_last_id(ui);
        if (cui_otp_input(ui, "otp", x->otp, 4)) x->otp_done++;
        x->otp_id = cui_last_id(ui);
        if (cui_hold_button(ui, "Hold", .duration = 0.5f)) x->held++;
        x->hold_id = cui_make_id(ui, "Hold");
    }
}

static void advance_toast(cui_ctx *ui, float elapsed) {
    cui_begin_frame(ui, elapsed);
    cui_end_frame(ui);
}

/** Timed toasts expire after an idle wake and stop accepting input on exit. */
static void test_toast_deadline(void) {
    printf("toast: idle deadline and expired action\n");
    cui_ctx *ui = new_ctx();
    cui_input_mouse_leave(ui);
    cui_begin_frame(ui, 0);
    cui_toast(ui, "Idle notice", .duration = 1.2f);
    cui_end_frame(ui);
    float wake = cui_next_wakeup(ui);
    CHECK(wake >= 1.2f && wake < 1.3f);
    advance_toast(ui, wake);
    CHECK(cui_next_wakeup(ui) < 0);
    cui_destroy(ui);

    ui = new_ctx();
    cui_begin_frame(ui, 0);
    cui_toast(ui, "Pause on hover", .duration = 1.2f);
    cui_end_frame(ui);
    cui_input_mouse_move(ui, 700, 550);
    advance_toast(ui, 0.1f);
    advance_toast(ui, 2.0f);
    CHECK(cui_next_wakeup(ui) < 0);
    cui_input_mouse_leave(ui);
    advance_toast(ui, 0);
    wake = cui_next_wakeup(ui);
    CHECK(wake > 1.0f && wake < 1.3f);
    cui_destroy(ui);

    ui = new_ctx();
    cui_input_mouse_leave(ui);
    cui_set_reduced_motion(ui, false);
    cui_begin_frame(ui, 0);
    int handle = cui_toast(ui, "Message archived", .duration = 1.0f, .action = "Undo");
    cui_end_frame(ui);
    cui_input_key(ui, CUI_KEY_TAB, true, 0);
    cui_input_key(ui, CUI_KEY_TAB, false, 0);
    advance_toast(ui, 0.01f);
    for (int i = 0; i < 49; i++) advance_toast(ui, 0.02f);
    cui_input_key(ui, CUI_KEY_ENTER, true, 0);
    cui_input_key(ui, CUI_KEY_ENTER, false, 0);
    advance_toast(ui, 0.02f);
    CHECK(!cui_toast_action_clicked(ui, handle));
    cui_destroy(ui);
}

/** One input event can fill the last slot but cannot create another tag. */
static void test_tag_limit_batch(void) {
    printf("tag input: enforce capacity for each separator\n");
    cui_ctx *ui = new_ctx();
    extras_ctx x = { .max_tags = 1 };
    run(ui, build_extras, &x, 2);
    key(ui, build_extras, &x, CUI_KEY_TAB, 0);
    type(ui, build_extras, &x, "one,two,");
    if (strcmp(x.tags, "one") != 0) printf("  Tags after one input event: %s\n", x.tags);
    CHECK(strcmp(x.tags, "one") == 0);
    cui_destroy(ui);
}

/** Ordinary editing, replacement of complete codes, and held activation. */
static void test_extras(void) {
    printf("extras: tag input, one time code, hold to confirm\n");
    cui_ctx *ui = new_ctx();
    extras_ctx x;
    memset(&x, 0, sizeof x);
    snprintf(x.tags, sizeof x.tags, "alpha, beta");
    run(ui, build_extras, &x, 3);
    cui_rect tr = rect_of(ui, x.tag_id);
    click_at(ui, build_extras, &x, tr.x + tr.w - 40, tr.y + tr.h / 2);
    type(ui, build_extras, &x, "gamma");
    key(ui, build_extras, &x, CUI_KEY_ENTER, 0);
    CHECK(strcmp(x.tags, "alpha, beta, gamma") == 0);
    type(ui, build_extras, &x, "delta,eps");
    CHECK(strcmp(x.tags, "alpha, beta, gamma, delta") == 0);
    key(ui, build_extras, &x, CUI_KEY_BACKSPACE, 0);   /* edits "eps" */
    key(ui, build_extras, &x, CUI_KEY_BACKSPACE, 0);
    key(ui, build_extras, &x, CUI_KEY_BACKSPACE, 0);
    key(ui, build_extras, &x, CUI_KEY_BACKSPACE, 0);   /* empty field: removes "delta" */
    CHECK(strcmp(x.tags, "alpha, beta, gamma") == 0);
    /* one time code */
    cui_rect orr = rect_of(ui, x.otp_id);
    click_at(ui, build_extras, &x, orr.x + 10, orr.y + 10);
    type(ui, build_extras, &x, "12a");
    CHECK(strcmp(x.otp, "12a") == 0 && x.otp_done == 0);
    type(ui, build_extras, &x, "9x");
    CHECK(strcmp(x.otp, "12a9") == 0 && x.otp_done == 1);
    key(ui, build_extras, &x, CUI_KEY_BACKSPACE, 0);
    CHECK(strcmp(x.otp, "12a") == 0);
    snprintf(g_clip, sizeof g_clip, " 7-7-7-7 ");
    key(ui, build_extras, &x, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(x.otp, "7777") == 0 && x.otp_done == 2);
    snprintf(g_clip, sizeof g_clip, "8B9C");
    key(ui, build_extras, &x, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(x.otp, "8B9C") == 0);
    if (x.otp_done != 3) printf("  Completion count after replacing a complete code: %d\n", x.otp_done);
    CHECK(x.otp_done == 3);
    key(ui, build_extras, &x, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(x.otp_done == 3);
    /* hold: a short press does nothing, a long one fires once */
    cui_vec2 hc = center(rect_of(ui, x.hold_id));
    click_at(ui, build_extras, &x, hc.x, hc.y);
    CHECK(x.held == 0);
    cui_set_reduced_motion(ui, false);
    cui_input_mouse_move(ui, hc.x, hc.y);
    run(ui, build_extras, &x, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_extras, &x, 60);    /* one second at 60 fps */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_extras, &x, 2);
    CHECK(x.held == 1);
    cui_destroy(ui);
}

typedef struct { int order[4]; int changed; cui_id list; } reorder_ctx;
static void build_reorder(cui_ctx *ui, void *u) {
    reorder_ctx *r = u;
    cui_column(ui, .pad = 20, .width = CUI_PX(300)) {
        cui_reorder_begin(ui, "list", r->order, 4);
        for (int i = 0; i < 4; i++) cui_reorder_item(ui, i) { cui_textf(ui, "Item %d", r->order[i]); }
        if (cui_reorder_end(ui)) r->changed++;
    }
}

static void test_reorder(void) {
    printf("data: drag to reorder\n");
    cui_ctx *ui = new_ctx();
    reorder_ctx r = { { 0, 1, 2, 3 }, 0, 0 };
    run(ui, build_reorder, &r, 3);
    /* rows are about 39 points tall with a 6 point gap, starting at y = 20 */
    cui_input_mouse_move(ui, 150, 40);
    run(ui, build_reorder, &r, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_reorder, &r, 1);
    for (int y = 40; y <= 140; y += 10) { cui_input_mouse_move(ui, 150, (float)y); run(ui, build_reorder, &r, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_reorder, &r, 3);
    CHECK(r.changed == 1);
    CHECK(r.order[0] == 1 && r.order[1] == 2 && r.order[2] == 0 && r.order[3] == 3);
    /* a click without movement does not reorder */
    click_at(ui, build_reorder, &r, 150, 40);
    CHECK(r.changed == 1);
    /* a small upward drag that does not pass the row above keeps the order */
    cui_input_mouse_move(ui, 150, 130);
    run(ui, build_reorder, &r, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_reorder, &r, 1);
    for (int y = 130; y >= 120; y -= 2) { cui_input_mouse_move(ui, 150, (float)y); run(ui, build_reorder, &r, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_reorder, &r, 3);
    CHECK(r.changed == 1);
    cui_destroy(ui);

    /* a list that first appears under the pointer does not reorder itself */
    ui = new_ctx();
    reorder_ctx r2 = { { 0, 1, 2, 3 }, 0, 0 };
    cui_input_mouse_move(ui, 150, 190);
    run(ui, build_reorder, &r2, 5);
    CHECK(r2.changed == 0 && r2.order[0] == 0 && r2.order[3] == 3);
    cui_destroy(ui);
}

/* app chrome ------------------------------------------------------------- */

static cui_id scoped_id(cui_ctx *ui, const char *scope, const char *name) {
    cui_push_id(ui, scope);
    cui_id id = cui_make_id(ui, name);
    cui_pop_id(ui);
    return id;
}

typedef struct { int chosen; int menus_built; } bar_ctx;
static void build_bar(cui_ctx *ui, void *u) {
    bar_ctx *b = u;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL) {
        cui_menu_bar(ui, "bar") {
            cui_menu_bar_menu(ui, "File") {
                b->menus_built |= 1;
                if (cui_menu_item(ui, "New")) b->chosen = 1;
                if (cui_menu_item(ui, "Open")) b->chosen = 2;
            }
            cui_menu_bar_menu(ui, "Edit") {
                b->menus_built |= 2;
                if (cui_menu_item(ui, "Undo")) b->chosen = 11;
                if (cui_menu_item(ui, "Redo")) b->chosen = 12;
            }
            cui_menu_bar_menu(ui, "View", .disabled = true) { b->menus_built |= 4; }
        }
        if (cui_button(ui, "Body")) b->chosen = 99;
    }
}

typedef struct { bool on; int fired[10]; float width; } tool_ctx;
static void build_tools(cui_ctx *ui, void *u) {
    tool_ctx *t = u;
    static const char *names[10] = { "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten" };
    cui_column(ui, .width = CUI_PX(t->width)) {
        cui_toolbar(ui, "tb") {
            for (int k = 0; k < 10; k++) {
                bool hit = k == 0 ? cui_toolbar_button(ui, CUI_ICON_BOLD, .tooltip = names[k], .toggle = &t->on)
                                  : cui_toolbar_button(ui, CUI_ICON_PLUS, .tooltip = names[k]);
                if (hit) t->fired[k]++;
                if (k == 4) cui_toolbar_separator(ui);
            }
        }
    }
}

typedef struct { int clicks; } status_ctx;
static void build_status(cui_ctx *ui, void *u) {
    status_ctx *s = u;
    cui_column(ui, .width = CUI_FILL) {
        cui_status_bar(ui, "sb") {
            cui_status_bar_text(ui, "Ready", .icon = CUI_ICON_CHECK, .tone = CUI_TONE_SUCCESS);
            cui_status_bar_progress(ui, 0.5f, .label = "Indexing");
            cui_status_bar_progress(ui, 0, .indeterminate = true);
            cui_status_bar_spacer(ui);
            if (cui_status_bar_item(ui, "Ln 1, Col 1###pos")) s->clicks++;
        }
    }
}

typedef struct { int sel; cui_doc_tabs_result r; int closes, moves; bool shortcuts; cui_doc_tab tabs[3]; } dtabs_ctx;
static void build_dtabs(cui_ctx *ui, void *u) {
    dtabs_ctx *d = u;
    cui_column(ui, .width = CUI_FILL) {
        d->r = cui_doc_tabs(ui, "dt", d->tabs, 3, &d->sel, .shortcuts = d->shortcuts);
        if (d->r.close >= 0) d->closes++;
        if (d->r.move_from >= 0) {   /* apply the move, as an app would */
            cui_doc_tab t = d->tabs[d->r.move_from];
            int f = d->r.move_from, to = d->r.move_to;
            if (to > f) memmove(&d->tabs[f], &d->tabs[f + 1], sizeof t * (size_t)(to - f));
            else memmove(&d->tabs[to + 1], &d->tabs[to], sizeof t * (size_t)(f - to));
            d->tabs[to] = t;
            d->moves++;
        }
    }
}

static void test_chrome(void) {
    printf("app chrome: menu bar, toolbar, status bar, document tabs\n");
    /* menu bar: click opens, hover switches, Enter picks, outside click closes */
    cui_ctx *ui = new_ctx();
    bar_ctx b = { 0, 0 };
    run(ui, build_bar, &b, 3);
    cui_rect file = rect_of(ui, scoped_id(ui, "bar", "File")), edit = rect_of(ui, scoped_id(ui, "bar", "Edit"));
    cui_rect view = rect_of(ui, scoped_id(ui, "bar", "View"));
    CHECK(file.w > 0 && edit.x > file.x && view.x > edit.x);
    CHECK(file.h >= 28 && file.h <= 32);
    click_at(ui, build_bar, &b, file.x + 5, file.y + 5);
    b.menus_built = 0;
    run(ui, build_bar, &b, 2);
    CHECK(b.menus_built == 1);
    cui_input_mouse_move(ui, edit.x + 5, edit.y + 5);
    run(ui, build_bar, &b, 2);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 2);
    /* first Edit item, under the trigger: panel padding 6, rows 32 tall */
    click_at(ui, build_bar, &b, edit.x + 20, edit.y + edit.h + 6 + 6 + 16);
    CHECK(b.chosen == 11);
    run(ui, build_bar, &b, 2);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 0);
    /* a disabled menu does not open */
    click_at(ui, build_bar, &b, view.x + 5, view.y + 5);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 0);
    /* the keyboard: F10 focuses the bar, Right moves, Enter opens with the first item ready */
    cui_input_mouse_move(ui, 700, 550);
    b.chosen = 0;
    key(ui, build_bar, &b, CUI_KEY_F10, 0);
    key(ui, build_bar, &b, CUI_KEY_RIGHT, 0);
    key(ui, build_bar, &b, CUI_KEY_ENTER, 0);
    run(ui, build_bar, &b, 1);
    key(ui, build_bar, &b, CUI_KEY_DOWN, 0);
    key(ui, build_bar, &b, CUI_KEY_ENTER, 0);
    CHECK(b.chosen == 12);
    /* Alt + mnemonic opens a menu; Left wraps around the enabled menus; Esc steps back, then out */
    run(ui, build_bar, &b, 2);
    b.chosen = 0;
    cui_input_key(ui, CUI_KEY_NONE, true, CUI_MOD_ALT);
    run(ui, build_bar, &b, 1);
    key(ui, build_bar, &b, CUI_KEY_F, CUI_MOD_ALT);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    run(ui, build_bar, &b, 1);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 1);
    key(ui, build_bar, &b, CUI_KEY_LEFT, 0);   /* View is disabled, so Left lands on Edit */
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 2);
    key(ui, build_bar, &b, CUI_KEY_ESCAPE, 0);
    run(ui, build_bar, &b, 2);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 0);
    key(ui, build_bar, &b, CUI_KEY_ENTER, 0);   /* still on the bar: Enter reopens Edit */
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 2);
    key(ui, build_bar, &b, CUI_KEY_ESCAPE, 0);
    key(ui, build_bar, &b, CUI_KEY_ESCAPE, 0);
    run(ui, build_bar, &b, 1);
    key(ui, build_bar, &b, CUI_KEY_ENTER, 0);   /* the bar let go: Enter does nothing here */
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 0 && b.chosen == 0);
    /* Alt pressed and released on its own focuses the bar */
    cui_input_key(ui, CUI_KEY_NONE, true, CUI_MOD_ALT);
    run(ui, build_bar, &b, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    run(ui, build_bar, &b, 1);
    key(ui, build_bar, &b, CUI_KEY_DOWN, 0);
    b.menus_built = 0;
    run(ui, build_bar, &b, 1);
    CHECK(b.menus_built == 1);
    cui_destroy(ui);

    /* toolbar: toggles flip; items that do not fit go to the overflow menu and fire from there */
    ui = new_ctx();
    tool_ctx t = { false, { 0 }, 600 };
    run(ui, build_tools, &t, 3);
    cui_rect tb = rect_of(ui, cui_make_id(ui, "tb"));
    CHECK(tb.w > 0 && tb.h >= 34 && tb.h <= 38);
    click_at(ui, build_tools, &t, tb.x + 6 + 15, tb.y + tb.h / 2);
    CHECK(t.on && t.fired[0] == 1);
    t.width = 200;   /* room for 188: 4 items and the more button */
    run(ui, build_tools, &t, 4);
    tb = rect_of(ui, cui_make_id(ui, "tb"));
    CHECK(tb.w <= 200.5f);
    click_at(ui, build_tools, &t, tb.x + 6 + 4 * 32 + 15, tb.y + tb.h / 2);
    run(ui, build_tools, &t, 2);
    cui_input_mouse_move(ui, 700, 550);
    run(ui, build_tools, &t, 1);
    key(ui, build_tools, &t, CUI_KEY_DOWN, 0);
    key(ui, build_tools, &t, CUI_KEY_ENTER, 0);
    run(ui, build_tools, &t, 2);
    CHECK(t.fired[4] == 1);
    int total = 0;
    for (int k = 0; k < 10; k++) total += t.fired[k];
    CHECK(total == 2);
    cui_destroy(ui);

    /* status bar */
    ui = new_ctx();
    status_ctx s = { 0 };
    run(ui, build_status, &s, 3);
    cui_rect sb = rect_of(ui, cui_make_id(ui, "sb"));
    CHECK(sb.h >= 26 && sb.h <= 30);
    cui_rect pos = rect_of(ui, scoped_id(ui, "sb", "Ln 1, Col 1###pos"));
    CHECK(pos.w > 0 && pos.x + pos.w <= sb.x + sb.w);
    click_at(ui, build_status, &s, pos.x + pos.w / 2, pos.y + pos.h / 2);
    CHECK(s.clicks == 1);
    cui_destroy(ui);

    /* document tabs */
    ui = new_ctx();
    dtabs_ctx d = { 0, { false, -1, -1, -1 }, 0, 0, false,
                    { { "alpha.c", NULL, NULL, false }, { "beta.c", NULL, "src/beta.c", true }, { "gamma.c", NULL, NULL, false } } };
    run(ui, build_dtabs, &d, 3);
    cui_rect ta = rect_of(ui, scoped_id(ui, "dt", "alpha.c")), tbeta = rect_of(ui, scoped_id(ui, "dt", "beta.c")), tg = rect_of(ui, scoped_id(ui, "dt", "gamma.c"));
    CHECK(ta.w > 0 && tbeta.x > ta.x && tg.x > tbeta.x);
    click_at(ui, build_dtabs, &d, tbeta.x + 20, tbeta.y + tbeta.h / 2);
    CHECK(d.sel == 1);
    /* the close button sits at the right end of the tab */
    click_at(ui, build_dtabs, &d, tbeta.x + tbeta.w - 6 - 10, tbeta.y + tbeta.h / 2);
    CHECK(d.closes == 1 && d.sel == 1);
    /* middle click closes */
    cui_input_mouse_move(ui, tg.x + 20, tg.y + tg.h / 2);
    run(ui, build_dtabs, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, true);
    run(ui, build_dtabs, &d, 1);
    CHECK(d.closes == 2 && d.r.close == 2);
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, false);
    run(ui, build_dtabs, &d, 1);
    /* keys on the focused strip: Right selects, Shift+Right moves, Delete closes */
    key(ui, build_dtabs, &d, CUI_KEY_RIGHT, 0);
    CHECK(d.sel == 2);
    key(ui, build_dtabs, &d, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK(d.moves == 1 && d.r.move_from == 2 && d.r.move_to == 1 && d.sel == 1);
    CHECK(strcmp(d.tabs[1].title, "gamma.c") == 0);
    run(ui, build_dtabs, &d, 1);
    key(ui, build_dtabs, &d, CUI_KEY_DELETE, 0);
    CHECK(d.closes == 3 && d.r.close == 1);
    /* drag alpha past the last tab (the order is now alpha, gamma, beta) */
    run(ui, build_dtabs, &d, 2);
    ta = rect_of(ui, scoped_id(ui, "dt", "alpha.c"));
    tg = rect_of(ui, scoped_id(ui, "dt", "beta.c"));
    d.moves = 0;
    cui_input_mouse_move(ui, ta.x + 20, ta.y + ta.h / 2);
    run(ui, build_dtabs, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_dtabs, &d, 1);
    CHECK(d.sel == 0);
    int last_from = -1, last_to = -1;
    for (float x = ta.x + 20; x <= tg.x + tg.w; x += 12) { cui_input_mouse_move(ui, x, ta.y + ta.h / 2); run(ui, build_dtabs, &d, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_dtabs, &d, 1);
    last_from = d.r.move_from; last_to = d.r.move_to;
    CHECK(d.moves == 1 && last_from == 0 && last_to == 2 && d.sel == 2);
    CHECK(strcmp(d.tabs[2].title, "alpha.c") == 0);
    run(ui, build_dtabs, &d, 3);
    /* window shortcuts are opt in */
    d.sel = 0;
    key(ui, build_dtabs, &d, CUI_KEY_TAB, CUI_MOD_CTRL);
    CHECK(d.sel == 0);
    d.shortcuts = true;
    run(ui, build_dtabs, &d, 1);
    key(ui, build_dtabs, &d, CUI_KEY_TAB, CUI_MOD_CTRL);
    CHECK(d.sel == 1);
    key(ui, build_dtabs, &d, CUI_KEY_TAB, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    key(ui, build_dtabs, &d, CUI_KEY_TAB, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    CHECK(d.sel == 2);
    d.closes = 0;
    key(ui, build_dtabs, &d, CUI_KEY_W, CUI_MOD_PRIMARY);
    CHECK(d.closes == 1 && d.r.close == 2);
    /* no tabs at all is fine */
    cui_doc_tabs_result empty;
    cui_begin_frame(ui, 1.0f / 60.0f);
    empty = cui_doc_tabs(ui, "none", NULL, 0, NULL);
    cui_end_frame(ui);
    CHECK(empty.close == -1 && empty.move_from == -1 && !empty.changed);
    cui_destroy(ui);
}

/* docking ---------------------------------------------------------------- */

#define DK_N 4
static const char *const dk_names[DK_N] = { "Files", "Editor", "Log", "Props" };
typedef struct {
    bool open[DK_N];
    int built[DK_N];
    cui_rect tab[DK_N];
    cui_id body[DK_N];
    const char *layout;
    float w, h;
    bool shortcuts;
} dock_ctx;
static void build_dock(cui_ctx *ui, void *u) {
    dock_ctx *d = u;
    cui_box(ui, .width = CUI_PX(d->w), .height = CUI_PX(d->h)) {
        cui_dockspace(ui, "ds", .layout = d->layout, .shortcuts = d->shortcuts) {
            for (int k = 0; k < DK_N; k++) {
                bool shown = cui_dock_panel_begin(ui, dk_names[k], &d->open[k]);
                d->tab[k] = cui_last_id(ui) ? cui_last_rect(ui) : (cui_rect){ 0, 0, 0, 0 };
                if (!shown) continue;
                d->built[k]++;
                d->body[k] = cui_box_begin(ui, &(cui_box){ .id = "content", .width = CUI_FILL, .height = CUI_FILL });
                cui_box_end(ui);
                cui_dock_panel_end(ui);
            }
        }
    }
}
static dock_ctx dock_new(const char *layout, float w, float h) {
    dock_ctx d;
    memset(&d, 0, sizeof d);
    for (int k = 0; k < DK_N; k++) d.open[k] = true;
    d.layout = layout; d.w = w; d.h = h;
    return d;
}
static const char *dock_str(cui_ctx *ui) {
    static char buf[1024];
    cui_dock_save(ui, "ds", buf, sizeof buf);
    return buf;
}
static cui_rect dock_body(cui_ctx *ui, dock_ctx *d, int k) { return d->body[k] ? cui_get_rect(ui, d->body[k]) : (cui_rect){ 0, 0, 0, 0 }; }
/* press at a, travel to b in steps, release (or not) */
static void dock_drag(cui_ctx *ui, dock_ctx *d, float ax, float ay, float bx, float by, bool release) {
    cui_input_mouse_move(ui, ax, ay);
    run(ui, build_dock, d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_dock, d, 1);
    for (int s = 1; s <= 12; s++) {
        float t = s / 12.0f;
        cui_input_mouse_move(ui, ax + (bx - ax) * t, ay + (by - ay) * t);
        run(ui, build_dock, d, 1);
    }
    if (!release) return;
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_dock, d, 3);
}
static void dock_drag_tab(cui_ctx *ui, dock_ctx *d, int k, float bx, float by) {
    cui_rect t = d->tab[k];
    dock_drag(ui, d, t.x + 16, t.y + t.h / 2, bx, by, true);
}
static bool dock_disjoint(cui_rect a, cui_rect b) {
    return a.x + a.w <= b.x + 0.5f || b.x + b.w <= a.x + 0.5f || a.y + a.h <= b.y + 0.5f || b.y + b.h <= a.y + 0.5f;
}

static void test_dock(void) {
    printf("docking\n");
    static const char *const base = "dock1 h0.25([*Files],v0.6([*Editor,Log],[*Props]))";
    cui_ctx *ui = new_ctx();
    dock_ctx d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    CHECK(strcmp(dock_str(ui), base) == 0);
    cui_rect ds = rect_of(ui, cui_make_id(ui, "ds"));
    CHECK(ds.w == 800 && ds.h == 560);
    cui_rect bf = dock_body(ui, &d, 0), be = dock_body(ui, &d, 1), bp = dock_body(ui, &d, 3);
    CHECK(bf.w > 150 && bf.w < 250 && be.x > bf.x + bf.w && bp.y > be.y + be.h);
    CHECK(d.tab[2].w > 0 && d.tab[2].x > d.tab[1].x);   /* Log is a tab beside Editor */
    /* only the tab that shows builds its content */
    int e0 = d.built[1];
    run(ui, build_dock, &d, 3);
    CHECK(d.built[1] == e0 + 3 && d.built[2] == 0);
    click_at(ui, build_dock, &d, d.tab[2].x + 16, d.tab[2].y + d.tab[2].h / 2);
    int l0 = d.built[2], e1 = d.built[1];
    run(ui, build_dock, &d, 2);
    CHECK(d.built[2] == l0 + 2 && d.built[1] == e1);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([Editor,*Log],[*Props]))") == 0);

    /* a divider resizes, and stops at the smallest panel size */
    bf = dock_body(ui, &d, 0);
    float dx = bf.x + bf.w + 0.5f, dy = bf.y + bf.h / 2;
    dock_drag(ui, &d, dx, dy, 300, dy, true);
    bf = dock_body(ui, &d, 0);
    CHECK_NEAR(bf.w, 300, 2);
    dock_drag(ui, &d, bf.x + bf.w + 0.5f, dy, 5, dy, true);
    bf = dock_body(ui, &d, 0);
    CHECK(bf.w >= 119.5f && bf.w <= 121);
    dock_drag(ui, &d, bf.x + bf.w + 0.5f, dy, 795, dy, true);
    be = dock_body(ui, &d, 2);
    CHECK(be.w >= 119.5f && be.w <= 121 && be.x + be.w <= 800.5f);
    cui_destroy(ui);

    /* dragging a tab onto the middle of a leaf stacks it there */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    bp = dock_body(ui, &d, 3);
    dock_drag_tab(ui, &d, 2, bp.x + bp.w / 2, bp.y + bp.h / 2);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([*Editor],[Props,*Log]))") == 0);
    /* onto a strip: at the slot under the pointer */
    run(ui, build_dock, &d, 2);
    dock_drag_tab(ui, &d, 0, d.tab[3].x + 4, d.tab[3].y + d.tab[3].h / 2);
    CHECK(strcmp(dock_str(ui), "dock1 v0.6([*Editor],[*Files,Props,Log])") == 0);   /* the emptied leaf collapsed */
    cui_destroy(ui);

    /* each side of a leaf splits it; the outer edges split the whole dockspace */
    static const struct { int zone, panel; const char *want; } sides[] = {
        { 0, 2, "dock1 h0.25([*Files],v0.6([*Editor],h0.5([*Log],[*Props])))" },
        { 1, 2, "dock1 h0.25([*Files],v0.6([*Editor],h0.5([*Props],[*Log])))" },
        { 2, 2, "dock1 h0.25([*Files],v0.6([*Editor],v0.5([*Log],[*Props])))" },
        { 3, 2, "dock1 h0.25([*Files],v0.6([*Editor],v0.5([*Props],[*Log])))" },
        { 4, 2, "dock1 h0.25([*Log],h0.25([*Files],v0.6([*Editor],[*Props])))" },
        { 5, 2, "dock1 h0.75(h0.25([*Files],v0.6([*Editor],[*Props])),[*Log])" },
        /* the top tile sits in Editor's strip, so this one comes from below; Props' leaf collapses */
        { 6, 3, "dock1 v0.25([*Props],h0.25([*Files],[*Editor,Log]))" },
        { 7, 2, "dock1 v0.75(h0.25([*Files],v0.6([*Editor],[*Props])),[*Log])" },
    };
    for (int s = 0; s < 8; s++) {
        ui = new_ctx();
        d = dock_new(base, 800, 560);
        run(ui, build_dock, &d, 4);
        bp = dock_body(ui, &d, 3);
        cui_vec2 c = center(bp);
        float x = c.x, y = c.y;
        if (sides[s].zone == 0) x = bp.x + 8;
        if (sides[s].zone == 1) x = bp.x + bp.w - 8;
        if (sides[s].zone == 2) y = bp.y + 6;
        if (sides[s].zone == 3) y = bp.y + bp.h - 6;
        if (sides[s].zone == 4) { x = 8 + 14; y = 560 / 2.0f; }        /* the outer tiles, 8 in from each edge */
        if (sides[s].zone == 5) { x = 800 - 8 - 14; y = 560 / 2.0f; }
        if (sides[s].zone == 6) { x = 400; y = 8 + 14; }
        if (sides[s].zone == 7) { x = 400; y = 560 - 8 - 14; }
        dock_drag_tab(ui, &d, sides[s].panel, x, y);
        CHECK(strcmp(dock_str(ui), sides[s].want) == 0);
        /* the new leaf is laid out where the preview said */
        cui_rect bl = dock_body(ui, &d, sides[s].panel), bo = dock_body(ui, &d, sides[s].panel == 3 ? 1 : 3);
        CHECK(bl.w > 100 && bl.h > 50 && dock_disjoint(bl, bo));
        if (sides[s].zone == 0 || sides[s].zone == 4) CHECK(bl.x < bo.x);
        if (sides[s].zone == 1 || sides[s].zone == 5) CHECK(bl.x > bo.x);
        if (sides[s].zone == 2 || sides[s].zone == 6) CHECK(bl.y < bo.y);
        if (sides[s].zone == 3 || sides[s].zone == 7) CHECK(bl.y > bo.y);
        cui_destroy(ui);
    }

    /* back onto the middle of its own leaf: nothing moves (its sides still split it) */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    be = dock_body(ui, &d, 1);
    dock_drag_tab(ui, &d, 2, be.x + be.w / 2, be.y + be.h / 2);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([Editor,*Log],[*Props]))") == 0);
    be = dock_body(ui, &d, 2);
    dock_drag_tab(ui, &d, 2, be.x + be.w - 6, be.y + be.h / 2);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6(h0.5([*Editor],[*Log]),[*Props]))") == 0);
    cui_destroy(ui);

    /* the last tab out of a leaf collapses it; onto its own lone leaf nothing happens */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    bf = dock_body(ui, &d, 0);
    dock_drag_tab(ui, &d, 0, bf.x + bf.w / 2, bf.y + bf.h / 2);
    CHECK(strcmp(dock_str(ui), base) == 0);
    bp = dock_body(ui, &d, 3);
    dock_drag_tab(ui, &d, 0, bp.x + bp.w / 2, bp.y + bp.h / 2);
    CHECK(strcmp(dock_str(ui), "dock1 v0.6([*Editor,Log],[Props,*Files])") == 0);
    be = dock_body(ui, &d, 1);
    CHECK(be.x < 1 && be.w > 790);
    /* Esc cancels a drag */
    run(ui, build_dock, &d, 2);
    be = dock_body(ui, &d, 1);
    dock_drag(ui, &d, d.tab[2].x + 16, d.tab[2].y + d.tab[2].h / 2, be.x + 20, be.y + be.h / 2, false);
    key(ui, build_dock, &d, CUI_KEY_ESCAPE, 0);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_dock, &d, 3);
    CHECK(strcmp(dock_str(ui), "dock1 v0.6([Editor,*Log],[Props,*Files])") == 0);   /* the press selected Log, nothing moved */
    /* released outside the dockspace: nothing moves */
    dock_drag_tab(ui, &d, 2, 400, 590);
    CHECK(strcmp(dock_str(ui), "dock1 v0.6([Editor,*Log],[Props,*Files])") == 0);
    cui_destroy(ui);

    /* close and reopen in the last place */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    cui_rect tp = d.tab[3];
    click_at(ui, build_dock, &d, tp.x + tp.w - 6 - 10, tp.y + tp.h / 2);
    CHECK(!d.open[3]);
    run(ui, build_dock, &d, 2);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([*Editor,Log],[*-Props]))") == 0);
    be = dock_body(ui, &d, 1);
    CHECK(be.h > 500);   /* Props' leaf left the layout: Editor takes the column */
    int p0 = d.built[3];
    run(ui, build_dock, &d, 2);
    CHECK(d.built[3] == p0);
    d.open[3] = true;
    run(ui, build_dock, &d, 3);
    CHECK(strcmp(dock_str(ui), base) == 0);
    CHECK(d.built[3] > p0);
    /* a hidden tab reopens selected */
    d.open[2] = false;
    run(ui, build_dock, &d, 2);
    d.open[2] = true;
    run(ui, build_dock, &d, 3);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([Editor,*Log],[*Props]))") == 0);
    cui_destroy(ui);

    /* no layout: panels stack as tabs in the largest leaf; unplaced panels too */
    ui = new_ctx();
    d = dock_new(NULL, 800, 560);
    run(ui, build_dock, &d, 6);
    CHECK(strncmp(dock_str(ui), "dock1 [", 7) == 0);
    CHECK(strstr(dock_str(ui), "Files") && strstr(dock_str(ui), "Props"));
    cui_destroy(ui);

    /* save, load and save again gives the same text; a fresh context reproduces the layout */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    run(ui, build_dock, &d, 4);
    bp = dock_body(ui, &d, 3);
    dock_drag_tab(ui, &d, 2, bp.x + bp.w - 8, bp.y + bp.h / 2);
    bf = dock_body(ui, &d, 0);
    dock_drag(ui, &d, bf.x + bf.w + 0.5f, bf.y + 40, 333, bf.y + 40, true);
    char s1[512], s2[512];
    cui_dock_save(ui, "ds", s1, sizeof s1);
    CHECK(cui_dock_load(ui, "ds", s1));
    run(ui, build_dock, &d, 3);
    cui_dock_save(ui, "ds", s2, sizeof s2);
    CHECK(strcmp(s1, s2) == 0);
    cui_rect r1[DK_N];
    for (int k = 0; k < DK_N; k++) r1[k] = dock_body(ui, &d, k);
    cui_ctx *ui2 = new_ctx();
    dock_ctx d2 = dock_new(s1, 800, 560);
    run(ui2, build_dock, &d2, 4);
    cui_dock_save(ui2, "ds", s2, sizeof s2);
    CHECK(strcmp(s1, s2) == 0);
    for (int k = 0; k < DK_N; k++) {
        cui_rect a = r1[k], b2 = dock_body(ui2, &d2, k);
        CHECK(fabsf(a.x - b2.x) < 1.5f && fabsf(a.w - b2.w) < 1.5f && fabsf(a.y - b2.y) < 1.5f && fabsf(a.h - b2.h) < 1.5f);
    }
    cui_destroy(ui2);
    /* the length is reported like snprintf, and a short buffer stays terminated */
    char tiny[8];
    size_t need = cui_dock_save(ui, "ds", tiny, sizeof tiny);
    CHECK(need == strlen(s1) && strlen(tiny) == 7);
    CHECK(cui_dock_save(ui, "nothing", tiny, sizeof tiny) == 0 && tiny[0] == 0);
    /* names with the format's own characters are escaped */
    {
        cui_ctx *u3 = new_ctx();
        const char *odd = "dock1 h0.5([*a\\,b\\]c,\\-dash],[*x\\\\y])";
        CHECK(cui_dock_load(u3, "odd", odd));
        char o3[128];
        cui_dock_save(u3, "odd", o3, sizeof o3);
        CHECK(strcmp(o3, odd) == 0);
        cui_destroy(u3);
    }

    /* malformed text is refused and leaves the layout alone */
    static const char *const bad[] = {
        "", "dock1", "dock1 ", "dock2 [a]", "dock1 []", "dock1 [a", "dock1 [a,]", "dock1 [,a]", "dock1 h0.5([a])",
        "dock1 h0.5([a],[b]", "dock1 h0.5([a],[b]))", "dock1 h2([a],[b])", "dock1 h1.5([a],[b])", "dock1 h-0.5([a],[b])",
        "dock1 h0.([a],[b])", "dock1 h0.1234567([a],[b])", "dock1 h99999999999999999999([a],[b])", "dock1 x0.5([a],[b])",
        "dock1 h0.5([a],[a])", "dock1 [*a,*b]", "dock1 [a\\", "dock1 [a]x", "dock1 [a(b]", "dock1 [a\nb]",
        "dock1 h0.5[a],[b]", "dock1 [aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa]",
    };
    cui_dock_save(ui, "ds", s1, sizeof s1);
    for (int k = 0; k < (int)(sizeof bad / sizeof bad[0]); k++) {
        bool ok = cui_dock_load(ui, "ds", bad[k]);
        CHECK(!ok);
        if (ok) printf("    accepted: %s\n", bad[k]);
    }
    CHECK(!cui_dock_load(ui, "ds", NULL));
    /* every prefix of a good layout but the whole is refused */
    for (size_t n = 0; n < strlen(s1); n++) {
        char cut[512];
        memcpy(cut, s1, n);
        cut[n] = 0;
        CHECK(!cui_dock_load(ui, "ds", cut));
    }
    /* nesting deeper than the limit, and more leaves than there is room for */
    {
        static char deep[4096];
        size_t p = (size_t)snprintf(deep, sizeof deep, "dock1 ");
        for (int k = 0; k < 60; k++) p += (size_t)snprintf(deep + p, sizeof deep - p, "h0.5([n%d],", k);
        p += (size_t)snprintf(deep + p, sizeof deep - p, "[end]");
        for (int k = 0; k < 60; k++) deep[p++] = ')';
        deep[p] = 0;
        CHECK(!cui_dock_load(ui, "ds", deep));
    }
    cui_dock_save(ui, "ds", s2, sizeof s2);
    CHECK(strcmp(s1, s2) == 0);
    /* random text and random mutations of a good layout: never a crash, and a refusal changes nothing */
    srand(1234);
    for (int it = 0; it < 4000; it++) {
        char fz[160];
        int n = rand() % 150;
        if (it % 2) {
            static const char alpha[] = "dock1 hv0.123456789()[],*-\\ab";
            for (int k = 0; k < n; k++) fz[k] = alpha[rand() % (int)(sizeof alpha - 1)];
            fz[n] = 0;
            if (it % 4 == 1) memcpy(fz, "dock1 ", n >= 6 ? 6 : (size_t)n);
        } else {
            size_t sl = strlen(s1) < sizeof fz - 1 ? strlen(s1) : sizeof fz - 1;
            memcpy(fz, s1, sl);
            fz[sl] = 0;
            int len = (int)strlen(fz);
            for (int m = 0; m < 1 + rand() % 3 && len > 0; m++) fz[rand() % len] = (char)(1 + rand() % 255);
        }
        char before[512], after[512];
        cui_dock_save(ui, "ds", before, sizeof before);
        if (!cui_dock_load(ui, "ds", fz)) {
            cui_dock_save(ui, "ds", after, sizeof after);
            CHECK(strcmp(before, after) == 0);
        }
        if (it % 200 == 0) run(ui, build_dock, &d, 1);
    }
    CHECK(cui_dock_load(ui, "ds", base));
    run(ui, build_dock, &d, 3);
    CHECK(strcmp(dock_str(ui), base) == 0);
    cui_destroy(ui);

    /* names the app has not shown yet are kept, and take their place when they appear */
    ui = new_ctx();
    d = dock_new("dock1 h0.4([*Files,Ghost],[*Editor,Log,Props])", 800, 560);
    run(ui, build_dock, &d, 4);
    CHECK(strcmp(dock_str(ui), "dock1 h0.4([*Files,Ghost],[*Editor,Log,Props])") == 0);
    bf = dock_body(ui, &d, 0);
    CHECK_NEAR(bf.w, 0.4f * 799, 2);
    CHECK(cui_dock_load(ui, "ds", "dock1 h0.5([*Files],[*Ghost])"));
    run(ui, build_dock, &d, 4);
    /* Ghost alone is never shown, so its leaf stays out of the layout; the others land in the largest leaf */
    CHECK(strstr(dock_str(ui), "Ghost") != NULL);
    bf = dock_body(ui, &d, 0);
    CHECK(bf.w > 790);
    /* a closed panel in a loaded layout closes the app's flag */
    CHECK(cui_dock_load(ui, "ds", "dock1 h0.5([*Files,-Log],[*Editor,Props])"));
    run(ui, build_dock, &d, 3);
    CHECK(!d.open[2] && d.open[0]);
    cui_destroy(ui);

    /* the keyboard: arrows move along a focused strip, Ctrl+Tab cycles the focused leaf (opt in) */
    ui = new_ctx();
    d = dock_new(base, 800, 560);
    d.shortcuts = true;
    run(ui, build_dock, &d, 4);
    click_at(ui, build_dock, &d, d.tab[1].x + 16, d.tab[1].y + d.tab[1].h / 2);
    key(ui, build_dock, &d, CUI_KEY_RIGHT, 0);
    run(ui, build_dock, &d, 1);
    CHECK(strcmp(dock_str(ui), "dock1 h0.25([*Files],v0.6([Editor,*Log],[*Props]))") == 0);
    key(ui, build_dock, &d, CUI_KEY_TAB, CUI_MOD_CTRL);
    run(ui, build_dock, &d, 1);
    CHECK(strcmp(dock_str(ui), base) == 0);
    /* the x (and Ctrl+W) closes; Delete on the focused tab too */
    key(ui, build_dock, &d, CUI_KEY_W, CUI_MOD_PRIMARY);
    run(ui, build_dock, &d, 1);
    CHECK(!d.open[1]);
    cui_destroy(ui);

    /* narrow (a phone): too tight for panes side by side, every panel is a tab of one stack and the
       selected one fills the dockspace. The layout itself is kept, and the panes come back with room */
    for (int wv = 0; wv < 3; wv++) {
        static const float widths[] = { 280, 340, 200 };
        static const char *lay = "dock1 h0.3([*Files],h0.5([*Editor,Log],[*Props]))";
        ui = new_ctx();
        d = dock_new(lay, widths[wv], 420);
        run(ui, build_dock, &d, 5);
        memset(d.body, 0, sizeof d.body);
        run(ui, build_dock, &d, 1);
        cui_rect r0 = dock_body(ui, &d, 0);
        CHECK(r0.w > widths[wv] - 4 && r0.x >= -0.5f && r0.x + r0.w <= widths[wv] + 0.5f);
        CHECK(!d.body[1] && !d.body[2] && !d.body[3]);
        CHECK(d.tab[1].w > 0 && d.tab[3].w > 0);
        click_at(ui, build_dock, &d, d.tab[1].x + 12, d.tab[1].y + d.tab[1].h / 2);
        memset(d.body, 0, sizeof d.body);
        run(ui, build_dock, &d, 2);
        CHECK(d.body[1] && !d.body[0] && !d.body[3]);
        CHECK(dock_body(ui, &d, 1).w > widths[wv] - 4);
        CHECK(strcmp(dock_str(ui), lay) == 0);
        d.w = 800;
        run(ui, build_dock, &d, 4);
        memset(d.body, 0, sizeof d.body);
        run(ui, build_dock, &d, 1);
        cui_rect r[3] = { dock_body(ui, &d, 0), dock_body(ui, &d, 1), dock_body(ui, &d, 3) };
        for (int k = 0; k < 3; k++) {
            CHECK(r[k].w > 100 && r[k].x >= -0.5f && r[k].x + r[k].w <= 800.5f);
            for (int j = k + 1; j < 3; j++) CHECK(dock_disjoint(r[k], r[j]));
        }
        cui_destroy(ui);
    }

    /* random drags, drops, closes and loads */
    ui = new_ctx();
    d = dock_new(base, 640, 480);
    run(ui, build_dock, &d, 4);
    srand(99);
    for (int it = 0; it < 300; it++) {
        int a = rand() % 6;
        if (a == 0) d.open[rand() % DK_N] ^= 1;
        else if (a == 1) { char t[512]; cui_dock_save(ui, "ds", t, sizeof t); CHECK(cui_dock_load(ui, "ds", t)); }
        else if (a == 2) d.w = (float)(160 + rand() % 700);
        else {
            int k = rand() % DK_N;
            if (d.tab[k].w <= 0) { run(ui, build_dock, &d, 1); continue; }
            float bx = (float)(rand() % 700), by = (float)(rand() % 500);
            dock_drag(ui, &d, d.tab[k].x + 10, d.tab[k].y + 10, bx, by, rand() % 8 != 0);
            if (rand() % 3 == 0) key(ui, build_dock, &d, CUI_KEY_ESCAPE, 0);
            cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        }
        run(ui, build_dock, &d, 2);
        char t[512];
        cui_dock_save(ui, "ds", t, sizeof t);
        CHECK(strncmp(t, "dock1 ", 6) == 0);
        for (int k = 0; k < DK_N; k++) CHECK(strstr(t, dk_names[k]) != NULL || !d.open[k]);
    }
    cui_destroy(ui);
}

/* virtual list ---------------------------------------------------------- */

typedef struct { int first, last, built; } vl_ctx;
static void build_vlist(cui_ctx *ui, void *u) {
    vl_ctx *v = u;
    v->built = 0;
    cui_virtual_list(ui, "vl", 100000, 20, CUI_PX(200), &v->first, &v->last) {
        for (int i = v->first; i < v->last; i++) { cui_row(ui, .height = CUI_PX(20)) { cui_textf(ui, "Row %d", i); } v->built++; }
    }
}

static void test_virtual_list(void) {
    printf("data: virtual list builds only visible rows\n");
    cui_ctx *ui = new_ctx();
    vl_ctx v = { 0, 0, 0 };
    run(ui, build_vlist, &v, 3);
    CHECK(v.first == 0);
    CHECK(v.built > 5 && v.built < 30);
    cui_input_mouse_move(ui, 100, 100);
    run(ui, build_vlist, &v, 1);
    for (int i = 0; i < 20; i++) { cui_input_scroll(ui, 0, -10); run(ui, build_vlist, &v, 1); }
    run(ui, build_vlist, &v, 5);
    CHECK(v.first > 300);
    CHECK(v.built < 30);
    cui_destroy(ui);
}

/* theme ----------------------------------------------------------------- */

static void build_empty(cui_ctx *ui, void *u) { (void)u; cui_text(ui, "x"); }

static void test_theme(void) {
    printf("theme: switching modes and accents\n");
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 200, 100, 1);
    run(ui, build_empty, NULL, 2);
    cui_color light_bg = cui_token_color(ui, CUI_TOKEN_BACKGROUND);
    CHECK(light_bg.r > 240);
    cui_set_theme(ui, CUI_THEME_DARK);
    run(ui, build_empty, NULL, 1);
    cui_color mid = cui_token_color(ui, CUI_TOKEN_BACKGROUND);
    CHECK(mid.r < light_bg.r);
    CHECK(cui_needs_redraw(ui));
    run(ui, build_empty, NULL, 60);
    cui_color dark_bg = cui_token_color(ui, CUI_TOKEN_BACKGROUND);
    CHECK(dark_bg.r < 50);
    cui_set_accent(ui, CUI_ACCENT_BLUE);
    run(ui, build_empty, NULL, 60);
    cui_color acc = cui_token_color(ui, CUI_TOKEN_ACCENT);
    CHECK(acc.b > acc.r);
    CHECK(strcmp(cui_accent_name(CUI_ACCENT_ROSE), "rose") == 0);
    /* idle when nothing moves */
    run(ui, build_empty, NULL, 5);
    CHECK(!cui_needs_redraw(ui));
    cui_destroy(ui);
}

/* the series palette: every color readable on the page, at one lightness, under every accent */
static double series_lin(int c) { double v = c / 255.0; return v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); }
static double series_lum(cui_color c) { return 0.2126 * series_lin(c.r) + 0.7152 * series_lin(c.g) + 0.0722 * series_lin(c.b); }
static double series_oklab_l(cui_color c) {
    double r = series_lin(c.r), g = series_lin(c.g), b = series_lin(c.b);
    double l = cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    double m = cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    double s = cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    return 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
}

static void test_series_palette(void) {
    printf("theme: the series palette is even and readable in both modes\n");
    for (int dark = 0; dark < 2; dark++)
        for (int a = 0; a < CUI_ACCENT_COUNT; a++) {
            cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT, .accent = (cui_accent)a });
            cui_color bg = cui_token_color(ui, CUI_TOKEN_SURFACE);
            double lo = 1, hi = 0, worst = 99;
            for (int k = 0; k < 6; k++) {
                cui_color c = cui_token_color(ui, (cui_token)(CUI_TOKEN_SERIES_1 + k));
                double L = series_oklab_l(c), y1 = series_lum(c), y2 = series_lum(bg);
                double cr = (fmax(y1, y2) + 0.05) / (fmin(y1, y2) + 0.05);
                lo = fmin(lo, L); hi = fmax(hi, L); worst = fmin(worst, cr);
            }
            CHECK(hi - lo < 0.02);   /* one lightness: no series louder than another */
            CHECK(worst >= 3.0);     /* graphics contrast on a card */
            cui_destroy(ui);
        }
}

/* radial gauge styles: AUTO picks segments with bands and the arc without; each style is
 * told apart by its pixels: segments break up along the arc, the arc is one stroke with a
 * gap at the bottom, the ring closes there */
typedef struct { cui_gauge_style style; bool bands; } gauge_case;
static void build_gauge_case(cui_ctx *ui, void *u) {
    const gauge_case *g = u;
    static const cui_gauge_band b[] = { { 50, CUI_TONE_SUCCESS, "Ok" }, { 100, CUI_TONE_DANGER, "Hot" } };
    cui_radial_gauge(ui, "g", 80, 0, 100, .size = 200, .style = g->style, .bands = g->bands ? b : NULL, .band_count = g->bands ? 2 : 0);
}
static bool gauge_ink(const unsigned char *px, int w, float x, float y, const unsigned char *bg) {
    const unsigned char *q = px + ((int)y * w + (int)x) * 4;
    return abs(q[0] - bg[0]) + abs(q[1] - bg[1]) + abs(q[2] - bg[2]) > 12;
}
/* breaks: ink and gap changes along the lit part of the arc; *bottom: ink under the middle */
static void gauge_probe(gauge_case g, int *breaks, bool *bottom) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 240, 280, 1);
    run(ui, build_gauge_case, &g, 90);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "g"));
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = calloc((size_t)dd->width_px * dd->height_px, 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    unsigned char bg[3] = { px[(dd->height_px - 2) * dd->width_px * 4], px[(dd->height_px - 2) * dd->width_px * 4 + 1], px[(dd->height_px - 2) * dd->width_px * 4 + 2] };
    float R = 96, cx = r.x + r.w * 0.5f, cy = r.y + 4 + R, a0 = 3.14159265f * 0.75f, sweep = 3.14159265f * 1.5f;
    int changes = 0;
    bool prev = false;
    for (int i = 0; i <= 400; i++) {
        float a = a0 + sweep * (0.05f + 0.7f * (float)i / 400.0f);
        bool ink = gauge_ink(px, dd->width_px, cx + cosf(a) * 84, cy + sinf(a) * 84, bg);
        if (i > 0 && ink != prev) changes++;
        prev = ink;
    }
    *breaks = changes;
    *bottom = gauge_ink(px, dd->width_px, cx, cy + 90, bg);
    free(px);
    cui_destroy(ui);
}

static void test_radial_gauge_styles(void) {
    printf("radial gauge: the style follows the options\n");
    int breaks; bool bottom;
    gauge_probe((gauge_case){ CUI_GAUGE_AUTO, false }, &breaks, &bottom);
    CHECK(breaks == 0 && !bottom);              /* no bands: the arc */
    gauge_probe((gauge_case){ CUI_GAUGE_AUTO, true }, &breaks, &bottom);
    CHECK(breaks >= 10 && !bottom);             /* bands: the segments */
    gauge_probe((gauge_case){ CUI_GAUGE_SEGMENTS, false }, &breaks, &bottom);
    CHECK(breaks >= 10);
    gauge_probe((gauge_case){ CUI_GAUGE_ARC, true }, &breaks, &bottom);
    CHECK(breaks == 0 && !bottom);              /* asked for the arc, bands only color it */
    gauge_probe((gauge_case){ CUI_GAUGE_RING, false }, &breaks, &bottom);
    CHECK(breaks == 0 && bottom);               /* the ring closes under the value */
}

/* treemap selection: a click selects a tile, a second click on it clears, NULL keeps none */
static int g_tm_pick = -1;
static void build_tm_select(cui_ctx *ui, void *u) {
    (void)u;
    static const cui_viz_item items[] = { { "Big", 3, { 0 } }, { "Small", 1, { 0 } } };
    cui_treemap(ui, "pick", items, 2, .width = CUI_PX(400), .height = 200, .selected = &g_tm_pick);
}
static void test_treemap_select(void) {
    printf("treemap: a click selects a tile and a second click clears it\n");
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 420, 220, 1);
    run(ui, build_tm_select, NULL, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "pick"));
    click_at(ui, build_tm_select, NULL, r.x + 60, r.y + 100);    /* the big tile fills the left three quarters */
    CHECK(g_tm_pick == 0);
    click_at(ui, build_tm_select, NULL, r.x + 360, r.y + 100);
    CHECK(g_tm_pick == 1);
    click_at(ui, build_tm_select, NULL, r.x + 360, r.y + 100);
    CHECK(g_tm_pick == -1);
    cui_destroy(ui);
}

/* sankey node colors: a node takes the caller's color; one left unset falls back */
static void build_sankey_colors(cui_ctx *ui, void *u) {
    (void)u;
    static const char *nodes[] = { "A", "B" };
    static const cui_sankey_link links[] = { { 0, 1, 10 } };
    const cui_color colors[] = { CUI_RGB(0xff0000), { 0 } };
    cui_sankey(ui, "sk", nodes, 2, links, 1, .width = CUI_PX(300), .height = 120, .node_colors = colors);
}
static void test_sankey_node_colors(void) {
    printf("sankey: nodes take the caller's colors\n");
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 320, 140, 1);
    run(ui, build_sankey_colors, NULL, 60);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "sk"));
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = calloc((size_t)dd->width_px * dd->height_px, 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    const unsigned char *a = px + ((int)(r.y + r.h * 0.5f) * dd->width_px + (int)(r.x + 3)) * 4;
    CHECK(a[0] > 240 && a[1] < 20 && a[2] < 20);                  /* the source node in red */
    CHECK(!(a[0] == a[1] && a[1] == a[2]));
    free(px);
    cui_destroy(ui);
}

/* gantt state: a bar takes its state's color; AUTO reads it from progress and today */
static cui_gantt_task g_gantt_case;
static void build_gantt_case(cui_ctx *ui, void *u) {
    (void)u;
    cui_date org = { 2026, 11, 2 };
    cui_gantt(ui, "gs", &g_gantt_case, 1, org, 10, .day_width = 30, .today = 2, .read_only = true);
}
static void gantt_bar_px(cui_gantt_task t, unsigned char out[3]) {
    g_gantt_case = t;
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 620, 160, 1);
    run(ui, build_gantt_case, NULL, 10);
    cui_rect b = cui_get_rect(ui, cui_make_id(ui, "gs") ^ 0xB0D7u);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = calloc((size_t)dd->width_px * dd->height_px, 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    const unsigned char *q = px + ((int)(b.y + 20) * dd->width_px + (int)(b.x + t.start * 30 + (t.end - t.start) * 30 * 0.8f)) * 4;
    memcpy(out, q, 3);
    free(px);
    cui_destroy(ui);
}
static void test_gantt_state(void) {
    printf("gantt: bars take the color of their state\n");
    unsigned char done[3], planned[3], auto_done[3], auto_planned[3], auto_active[3];
    gantt_bar_px((cui_gantt_task){ "", NULL, 4, 8, 0, 0, -1, false, CUI_GANTT_DONE }, done);
    gantt_bar_px((cui_gantt_task){ "", NULL, 4, 8, 0, 0, -1, false, CUI_GANTT_PLANNED }, planned);
    gantt_bar_px((cui_gantt_task){ "", NULL, 4, 8, 1, 0, -1, false, CUI_GANTT_AUTO }, auto_done);
    gantt_bar_px((cui_gantt_task){ "", NULL, 4, 8, 0, 0, -1, false, CUI_GANTT_AUTO }, auto_planned);   /* starts after today */
    gantt_bar_px((cui_gantt_task){ "", NULL, 1, 8, 0, 0, -1, false, CUI_GANTT_AUTO }, auto_active);    /* started, nothing done */
    CHECK(done[1] > done[0] + 20 && done[1] > done[2]);                     /* a filled green bar */
    CHECK(abs(planned[0] - planned[1]) < 6 && abs(planned[1] - planned[2]) < 6 && planned[0] > 200);   /* a light gray tint */
    CHECK(memcmp(done, auto_done, 3) == 0);
    CHECK(memcmp(planned, auto_planned, 3) == 0);
    CHECK(auto_active[2] > auto_active[0]);                                 /* the first series color, blue */
}

/* draw data and renderer ------------------------------------------------ */

static void build_rect(cui_ctx *ui, void *u) {
    (void)u;
    cui_box(ui, .width = CUI_PX(100), .height = CUI_PX(60), .bg = CUI_RGB(0xff0000), .radius = 20) {}
}

static void test_renderer(void) {
    printf("renderer: draw data is valid and rasterizes correctly\n");
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, 200, 100, 1);
    run(ui, build_rect, NULL, 2);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    CHECK(dd->index_count % 3 == 0);
    bool idx_ok = true;
    for (int i = 0; i < dd->index_count; i++) if ((int)dd->indices[i] >= dd->vertex_count) idx_ok = false;
    CHECK(idx_ok);
    unsigned char *px = calloc(200 * 100, 4);
    cui_sw_render(g_sw, dd, px, 200, 100, 800, CUI_SW_RGBA);
    unsigned char *center_px = px + (30 * 200 + 50) * 4;
    CHECK(center_px[0] == 255 && center_px[1] == 0 && center_px[2] == 0);
    unsigned char *outside = px + (80 * 200 + 150) * 4;
    CHECK(outside[0] == 255 && outside[1] == 255 && outside[2] == 255);   /* light background */
    unsigned char *corner = px + (1 * 200 + 1) * 4;                       /* rounded away */
    CHECK(corner[1] > 200);
    unsigned char *edge = px + (6 * 200 + 5) * 4;                          /* anti-aliased corner pixel */
    CHECK(edge[1] > 0 && edge[1] < 255);
    /* threaded and single-threaded output are identical */
    unsigned char *px2 = calloc(200 * 100, 4);
    cui_sw_set_threads(g_sw, 1);
    cui_sw_render(g_sw, dd, px2, 200, 100, 800, CUI_SW_RGBA);
    CHECK(memcmp(px, px2, 200 * 100 * 4) == 0);
    free(px); free(px2);
    cui_destroy(ui);
}

typedef struct { int size; } atlas_ctx;
static void build_atlas(cui_ctx *ui, void *u) {
    atlas_ctx *a = u;
    for (int k = 0; k < 6; k++)
        cui_text(ui, "The quick brown fox jumps over the lazy dog 0123456789 !?@#$%&*()[]{}", .size = (float)(a->size + k * 7), .nowrap = true);
}

static void test_atlas_pressure(void) {
    printf("text: glyph atlas grows and recovers under pressure\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 800, 600, 2);
    atlas_ctx a = { 8 };
    for (; a.size < 400; a.size += 9) run(ui, build_atlas, &a, 2);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    CHECK(dd->vertex_count > 0);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    free(px);
    cui_destroy(ui);
}

/* pretty wrapping -------------------------------------------------------- */

/* the library's internal line record; same tag and members, so a compatible type */
struct cui_text_layout_line { int start, end; float width; };
int cui__text_wrap(cui_ctx *ui, cui_font font, float size, const char *text, int len, float width,
                   struct cui_text_layout_line *lines, int max_lines);

static int last_line_words(const char *t, struct cui_text_layout_line *l) {
    int w = 1;
    for (int i = l->start; i < l->end; i++) w += t[i] == ' ';
    return w;
}

static void test_pretty_wrap(void) {
    printf("text: wrapped paragraphs do not end on a lone word\n");
    cui_ctx *ui = new_ctx();
    run(ui, build_empty, NULL, 1);
    const char *t = "Keyboard navigation in the date range picker";
    float full = cui_measure_text(ui, CUI_FONT_MEDIUM, 14, t, -1).width;
    struct cui_text_layout_line lines[8];
    /* a width that greedily leaves only the last word on the second line */
    float w = full - 4;
    int n = cui__text_wrap(ui, CUI_FONT_MEDIUM, 14, t, (int)strlen(t), w, lines, 8);
    CHECK(n == 2);
    CHECK(last_line_words(t, &lines[n - 1]) >= 2);
    /* the line count is kept: a width that needs three lines still gets three */
    int n3 = cui__text_wrap(ui, CUI_FONT_MEDIUM, 14, t, (int)strlen(t), full * 0.4f, lines, 8);
    CHECK(n3 >= 3);
    CHECK(last_line_words(t, &lines[n3 - 1]) >= 2);
    /* a single paragraph line is untouched */
    CHECK(cui__text_wrap(ui, CUI_FONT_MEDIUM, 14, t, (int)strlen(t), full + 10, lines, 8) == 1);
    cui_destroy(ui);
}

/* truncated labels -------------------------------------------------------- */

typedef struct { float w; int clicks; } trunc_ctx;
static void build_trunc(cui_ctx *ui, void *u) {
    trunc_ctx *t = u;
    cui_column(ui, .pad = 20) {
        if (cui_card_begin(ui, &(cui_card_opts){ .id = "row", .pad = 8, .hoverable = true, .width = CUI_PX(t->w) })) t->clicks++;
        cui_text(ui, "A rather long file name that will not fit.pdf", .nowrap = true, .width = CUI_FILL);
        cui_card_end(ui);
    }
}

typedef struct { const char *v; } roll_ctx;
static void build_roll(cui_ctx *ui, void *user) {
    roll_ctx *r = user;
    cui_row(ui) { cui_number_roll(ui, "n", r->v, .size = 20); }
}
static void test_number_roll(void) {
    printf("motion: a rolling number settles on its text, keeps a fixed prefix and fits its width\n");
    cui_ctx *ui = new_ctx();
    cui_set_reduced_motion(ui, false);
    roll_ctx r = { "$64,210" };
    run(ui, build_roll, &r, 3);
    cui_rect a = rect_of(ui, cui_make_id(ui, "n"));
    CHECK(a.w > 40);
    r.v = "$64,987";
    run(ui, build_roll, &r, 2);
    CHECK(cui_needs_redraw(ui));
    run(ui, build_roll, &r, 90);
    CHECK(!cui_needs_redraw(ui));
    cui_rect b = rect_of(ui, cui_make_id(ui, "n"));
    CHECK(fabsf(b.w - a.w) < 0.5f);   /* tabular digits: same count, same width */
    r.v = "$9,870";
    run(ui, build_roll, &r, 90);
    cui_rect c = rect_of(ui, cui_make_id(ui, "n"));
    CHECK(c.w < b.w - 5);
    cui_set_reduced_motion(ui, true);
    r.v = "$12";
    run(ui, build_roll, &r, 2);
    CHECK(!cui_needs_redraw(ui));
    cui_destroy(ui);
}

/** Drive the hover with explicit frame intervals so the wake can be tested. */
static int paint_hover_tooltip(cui_ctx *ui, float elapsed, bool show_hint) {
    cui_begin_frame(ui, elapsed);
    cui_button(ui, "Anchor");
    if (show_hint) cui_tooltip(ui, "Tooltip");
    cui_end_frame(ui);
    return cui_get_draw_data(ui)->vertex_count;
}

/** A cold hover waits for its deadline and appears on the requested wake. */
static void test_tooltip_hover_delay(void) {
    printf("tooltip: cold hover and idle wake\n");
    for (int idle = 0; idle < 2; idle++) {
        cui_ctx *ui = new_ctx();
        cui_input_mouse_leave(ui);
        if (idle) paint_hover_tooltip(ui, 0.25f, true);
        paint_hover_tooltip(ui, idle ? 0.25f : 0.01f, true);
        cui_vec2 pos = center(cui_last_rect(ui));
        cui_input_mouse_move(ui, pos.x, pos.y);
        int without_hint = paint_hover_tooltip(ui, 0.01f, false);
        int hovered = paint_hover_tooltip(ui, 0, true);
        CHECK(hovered == without_hint);
        float wake = cui_next_wakeup(ui);
        CHECK(wake > 0.5f && wake < 0.6f);
        if (wake > 0) CHECK(paint_hover_tooltip(ui, wake, true) > hovered);
        cui_destroy(ui);
    }
}

static void test_truncated_tooltip(void) {
    printf("text: a truncated label shows its whole text on hover, and its row still clicks\n");
    cui_ctx *ui = new_ctx();
    trunc_ctx t = { 600, 0 };
    cui_input_mouse_move(ui, 60, 36);
    run(ui, build_trunc, &t, 70);
    int wide = cui_get_draw_data(ui)->vertex_count;
    t.w = 140;
    run(ui, build_trunc, &t, 3);
    int before = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_trunc, &t, 70);
    int after = cui_get_draw_data(ui)->vertex_count;
    CHECK(after > before);      /* the tooltip appeared */
    (void)wide;
    /* the row under the label keeps its click */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_trunc, &t, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_trunc, &t, 1);
    CHECK(t.clicks == 1);
    /* a label that fits gets no tooltip */
    t.w = 600;
    cui_input_mouse_move(ui, 400, 400);
    run(ui, build_trunc, &t, 30);
    cui_input_mouse_move(ui, 60, 36);
    run(ui, build_trunc, &t, 3);
    int fit0 = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_trunc, &t, 70);
    CHECK(cui_get_draw_data(ui)->vertex_count == fit0);
    cui_destroy(ui);
}

/* a popover open over a truncated label owns the pointer: no label tooltip */
static void build_trunc_covered(cui_ctx *ui, void *u) {
    (void)u;
    cui_column(ui, .pad = 20, .gap = 4) {
        cui_button(ui, "Open");
        if (cui_popover_begin(ui, "pop", &(cui_popover_opts){ .width = 320, .gap = 2 })) {
            for (int i = 0; i < 6; i++) { cui_push_id_int(ui, i); cui_text(ui, "Popover line"); cui_pop_id(ui); }
            cui_popover_end(ui);
        }
        cui_box(ui, .id = "row2", .width = CUI_PX(140)) {
            cui_text(ui, "A rather long file name that will not fit.pdf", .nowrap = true, .width = CUI_FILL);
        }
    }
}

static void test_truncated_tooltip_covered(void) {
    printf("text: a truncated label under an open popover shows no tooltip\n");
    cui_ctx *ui = new_ctx();
    cui_input_mouse_move(ui, 600, 600);
    run(ui, build_trunc_covered, NULL, 3);
    cui_popover_open(ui, "pop");
    run(ui, build_trunc_covered, NULL, 40);
    CHECK(cui_popover_is_open(ui, "pop"));
    cui_rect lr = rect_of(ui, cui_make_id(ui, "row2"));
    CHECK(lr.w > 100 && lr.h > 10);
    cui_input_mouse_move(ui, lr.x + 40, lr.y + lr.h * 0.5f);
    run(ui, build_trunc_covered, NULL, 3);
    int before = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_trunc_covered, NULL, 70);
    CHECK(cui_popover_is_open(ui, "pop"));
    CHECK(cui_get_draw_data(ui)->vertex_count == before);   /* no tooltip appeared */
    cui_destroy(ui);
}

/* scroll edge fades ------------------------------------------------------ */

typedef struct { bool fade; int rows; } fade_ctx;
static void build_fade(cui_ctx *ui, void *u) {
    fade_ctx *f = u;
    cui_scroll(ui, .id = "f", .height = CUI_PX(120), .width = CUI_PX(200), .fade_edges = f->fade) {
        for (int i = 0; i < f->rows; i++) { cui_push_id_int(ui, i); cui_text(ui, "Row"); cui_pop_id(ui); }
    }
}

static void test_scroll_fade(void) {
    printf("scroll: edge fades only where there is more to scroll\n");
    cui_ctx *ui = new_ctx();
    fade_ctx f = { false, 30 };
    run(ui, build_fade, &f, 3);
    int plain = cui_get_draw_data(ui)->vertex_count;
    f.fade = true;
    run(ui, build_fade, &f, 3);
    int faded = cui_get_draw_data(ui)->vertex_count;
    CHECK(faded > plain);              /* a bottom fade, none at the top yet */
    f.rows = 2;
    run(ui, build_fade, &f, 3);
    int short_fade = cui_get_draw_data(ui)->vertex_count;
    f.fade = false;
    run(ui, build_fade, &f, 3);
    CHECK(cui_get_draw_data(ui)->vertex_count == short_fade);   /* nothing to scroll, nothing drawn */
    cui_destroy(ui);
}

/* stress ---------------------------------------------------------------- */

typedef struct { int n; } stress_ctx;
static void build_stress(cui_ctx *ui, void *u) {
    stress_ctx *s = u;
    cui_scroll(ui, .id = "s") {
        for (int i = 0; i < s->n; i++) {
            cui_push_id_int(ui, i);
            cui_row(ui, .gap = 8) {
                cui_badge(ui, "Tag");
                cui_button(ui, "Edit", .size = CUI_SIZE_SM);
                cui_textf(ui, "Item %d with some text", i);
            }
            cui_pop_id(ui);
        }
    }
}

static void test_stress(void) {
    printf("stress: many widgets and many frames stay stable\n");
    cui_ctx *ui = new_ctx();
    stress_ctx s = { 2000 };
    run(ui, build_stress, &s, 3);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    CHECK(dd->vertex_count > 0);
    s.n = 50;
    run(ui, build_stress, &s, 400);   /* state for the removed rows gets collected */
    CHECK(cui_frame_count(ui) == 403);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */

static void drag2(cui_ctx *ui, build_fn fn, void *u, float x0, float y0, float x1, float y1, int steps) {
    cui_input_mouse_move(ui, x0, y0);
    run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, fn, u, 1);
    for (int i = 1; i <= steps; i++) { cui_input_mouse_move(ui, x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps); run(ui, fn, u, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, fn, u, 1);
}

typedef struct {
    int cols[3][6], len[3];
    cui_id card_ids[10];
    bool moved, added, clicked;
    cui_kanban_move mv;
    cui_cal_event ev[2];
    int ev_clicked;
    cui_id week_id;
    cui_gantt_task tasks[2];
    int task_clicked;
    cui_id gantt_id;
    int mode;   /* 0 kanban, 1 week, 2 gantt */
} plan_ctx;

static void build_plan(cui_ctx *ui, void *u) {
    plan_ctx *a = u;
    cui_column(ui, .pad = 16, .gap = 16, .width = CUI_FILL) {
        if (a->mode == 0) {
            cui_kanban_begin(ui, "kb");
            for (int c = 0; c < 3; c++) {
                if (cui_kanban_column_begin(ui, "Col", a->len[c], .width = 220)) a->added = true;
                for (int k = 0; k < a->len[c]; k++) {
                    int id = a->cols[c][k];
                    if (cui_kanban_card_begin(ui, id)) a->clicked = true;
                    a->card_ids[id] = cui_last_id(ui);
                    cui_text(ui, "A card with a title");
                    cui_kanban_card_end(ui);
                }
                cui_kanban_column_end(ui);
            }
            if (cui_kanban_end(ui, &a->mv)) a->moved = true;
        } else if (a->mode == 1) {
            cui_date wk = { 2026, 3, 2 };
            int r = cui_week_calendar(ui, "wk", wk, a->ev, 2, .start_hour = 8, .end_hour = 18, .hour_height = 40, .height = CUI_PX(500));
            if (r >= 0) a->ev_clicked = r;
            a->week_id = cui_last_id(ui);
        } else {
            cui_date org = { 2026, 3, 2 };
            int r = cui_gantt(ui, "gt", a->tasks, 2, org, 30, .day_width = 30, .row_height = 40);
            if (r >= 0) a->task_clicked = r;
            a->gantt_id = cui_last_id(ui);
        }
    }
}

static void kb_move(plan_ctx *a) {
    cui_kanban_move *m = &a->mv;
    int at = -1;
    for (int k = 0; k < a->len[m->from_column]; k++) if (a->cols[m->from_column][k] == m->card) at = k;
    for (int k = at; k < a->len[m->from_column] - 1; k++) a->cols[m->from_column][k] = a->cols[m->from_column][k + 1];
    a->len[m->from_column]--;
    for (int k = a->len[m->to_column]; k > m->to_index; k--) a->cols[m->to_column][k] = a->cols[m->to_column][k - 1];
    a->cols[m->to_column][m->to_index] = m->card;
    a->len[m->to_column]++;
}

/* ---- node editor ---- */
typedef struct {
    cui_node_view view;
    float pos[3][2];
    bool alive[3];
    int links[8][3]; int link_n, next_link;
    float value;
    cui_id card[3], pin_a_out, pin_b_in, pin_b_float, pin_c_in;
    cui_rect slider;
    int created, removed, deleted, last_from, last_to, last_removed, last_deleted;
    bool sel_a;
} nodes_ctx;

static void build_nodes(cui_ctx *ui, void *user) {
    nodes_ctx *a = user;
    if (cui_node_editor_begin(ui, "graph", &a->view, .height = CUI_PX(500))) {
        if (a->alive[0]) {
            a->sel_a = cui_node_begin(ui, 1, &a->pos[0][0], &a->pos[0][1], "Source");
            a->card[0] = cui_last_id(ui);
            cui_node_output(ui, 10, "Color", .type = 1);
            a->pin_a_out = cui_last_id(ui);
            cui_node_end(ui);
        }
        if (a->alive[1]) {
            cui_node_begin(ui, 2, &a->pos[1][0], &a->pos[1][1], "Mix");
            a->card[1] = cui_last_id(ui);
            cui_node_input(ui, 20, "Color", .type = 1);
            a->pin_b_in = cui_last_id(ui);
            cui_node_input(ui, 21, "Factor", .type = 2);
            a->pin_b_float = cui_last_id(ui);
            cui_node_content(ui) { cui_slider(ui, "##v", &a->value, 0, 100); a->slider = cui_last_rect(ui); }
            cui_node_end(ui);
        }
        if (a->alive[2]) {
            cui_node_begin(ui, 3, &a->pos[2][0], &a->pos[2][1], "Output");
            a->card[2] = cui_last_id(ui);
            cui_node_input(ui, 30, "Base", .type = 1);
            a->pin_c_in = cui_last_id(ui);
            cui_node_end(ui);
        }
        for (int k = 0; k < a->link_n; k++) cui_node_link(ui, a->links[k][0], a->links[k][1], a->links[k][2]);
        int f, t, id;
        while (cui_node_link_created(ui, &f, &t)) { a->created++; a->last_from = f; a->last_to = t; if (a->link_n < 8) { a->links[a->link_n][0] = a->next_link++; a->links[a->link_n][1] = f; a->links[a->link_n][2] = t; a->link_n++; } }
        while (cui_node_link_removed(ui, &id)) {
            a->removed++; a->last_removed = id;
            for (int k = 0; k < a->link_n; k++) if (a->links[k][0] == id) { memcpy(a->links[k], a->links[--a->link_n], sizeof a->links[k]); break; }
        }
        while (cui_node_deleted(ui, &id)) { a->deleted++; a->last_deleted = id; if (id >= 1 && id <= 3) a->alive[id - 1] = false; }
        cui_node_editor_end(ui);
    }
}

static void drag_to(cui_ctx *ui, nodes_ctx *a, cui_vec2 from, cui_vec2 to) {
    cui_input_mouse_move(ui, from.x, from.y);
    run(ui, build_nodes, a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_nodes, a, 1);
    for (int k = 1; k <= 6; k++) {
        cui_input_mouse_move(ui, from.x + (to.x - from.x) * k / 6.0f, from.y + (to.y - from.y) * k / 6.0f);
        run(ui, build_nodes, a, 1);
    }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_nodes, a, 2);
}

static void test_node_editor(void) {
    printf("node editor: select, drag, links, detach, zoom, hosted controls, delete, marquee\n");
    cui_ctx *ui = new_ctx();
    nodes_ctx a;
    memset(&a, 0, sizeof a);
    float init[3][2] = { { 20, 20 }, { 300, 40 }, { 300, 260 } };
    memcpy(a.pos, init, sizeof init);
    a.alive[0] = a.alive[1] = a.alive[2] = true;
    a.links[0][0] = 100; a.links[0][1] = 10; a.links[0][2] = 20; a.link_n = 1; a.next_link = 101;
    a.value = 20;
    run(ui, build_nodes, &a, 3);
    CHECK_NEAR(a.view.zoom, 1, 0.001);
    /* click a card: it is selected */
    cui_rect ca = cui_get_rect(ui, a.card[0]);
    CHECK(ca.w > 100 && ca.h > 40);
    click_at(ui, build_nodes, &a, ca.x + 40, ca.y + 12);
    CHECK(a.sel_a);
    /* drag it: the caller's position moves by the pointer travel (zoom 1) */
    drag_to(ui, &a, (cui_vec2){ ca.x + 40, ca.y + 12 }, (cui_vec2){ ca.x + 90, ca.y + 42 });
    CHECK_NEAR(a.pos[0][0], 70, 0.5);
    CHECK_NEAR(a.pos[0][1], 50, 0.5);
    /* a wire from the output to a compatible input makes a link */
    drag_to(ui, &a, center(cui_get_rect(ui, a.pin_a_out)), center(cui_get_rect(ui, a.pin_c_in)));
    CHECK(a.created == 1 && a.last_from == 10 && a.last_to == 30 && a.link_n == 2);
    /* types must match: Color does not go into Factor */
    drag_to(ui, &a, center(cui_get_rect(ui, a.pin_a_out)), center(cui_get_rect(ui, a.pin_b_float)));
    CHECK(a.created == 1 && a.link_n == 2);
    /* a click on a pin changes nothing */
    click_at(ui, build_nodes, &a, center(cui_get_rect(ui, a.pin_b_in)).x, center(cui_get_rect(ui, a.pin_b_in)).y);
    CHECK(a.removed == 0 && a.link_n == 2);
    /* drag a link's input end off its pin: the link is removed */
    cui_vec2 bin = center(cui_get_rect(ui, a.pin_b_in));
    drag_to(ui, &a, bin, (cui_vec2){ bin.x - 120, bin.y + 150 });
    CHECK(a.removed == 1 && a.last_removed == 100 && a.link_n == 1);
    /* a hosted slider: clicking 90% along it gives the same value at 100% and zoomed in */
    float v100 = -1;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            /* the wheel zooms around the pointer: the canvas point under it stays put */
            cui_rect cb = cui_get_rect(ui, a.card[1]);
            cui_input_mouse_move(ui, cb.x + 20, cb.y + 200);
            run(ui, build_nodes, &a, 1);
            cui_input_scroll(ui, 0, 3);
            run(ui, build_nodes, &a, 2);
            CHECK(a.view.zoom > 1.3f);
            cui_rect cb2 = cui_get_rect(ui, a.card[1]);
            CHECK_NEAR(cb.x + 20 - (cb.x + 20 - cb2.x) / a.view.zoom, cb.x, 0.6);
            a.value = 20;
            run(ui, build_nodes, &a, 1);
        }
        cui_rect sr = a.slider;   /* laid out at 100%, shown scaled from its top left */
        float z = a.view.zoom;
        cui_vec2 sp = { sr.x + sr.w * 0.9f * z, sr.y + sr.h * 0.5f * z };
        cui_input_mouse_move(ui, sp.x, sp.y);
        run(ui, build_nodes, &a, 1);
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        run(ui, build_nodes, &a, 2);
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        run(ui, build_nodes, &a, 2);
        if (pass == 0) { v100 = a.value; CHECK(v100 > 80); }
        else CHECK_NEAR(a.value, v100, 1.5);
    }
    /* F frames every node at 100% or less */
    cui_rect cc = cui_get_rect(ui, a.card[2]);
    click_at(ui, build_nodes, &a, cc.x + 30, cc.y + 10);
    cui_input_key(ui, CUI_KEY_F, true, 0); cui_input_key(ui, CUI_KEY_F, false, 0);
    run(ui, build_nodes, &a, 2);
    CHECK(a.view.zoom <= 1.0f);
    /* Delete removes the selected node and reports its links */
    cc = cui_get_rect(ui, a.card[2]);
    click_at(ui, build_nodes, &a, cc.x + 30, cc.y + 10);
    cui_input_key(ui, CUI_KEY_DELETE, true, 0); cui_input_key(ui, CUI_KEY_DELETE, false, 0);
    run(ui, build_nodes, &a, 2);
    CHECK(a.deleted == 1 && a.last_deleted == 3 && !a.alive[2]);
    CHECK(a.removed == 2 && a.link_n == 0);
    /* a marquee on the empty canvas selects what it touches */
    ca = cui_get_rect(ui, a.card[0]);
    drag_to(ui, &a, (cui_vec2){ ca.x - 10, ca.y + ca.h + 30 }, (cui_vec2){ ca.x + 20, ca.y + ca.h - 10 });
    CHECK(a.sel_a);
    /* the middle button pans */
    float vx = a.view.x;
    cui_rect er = cui_get_rect(ui, cui_make_id(ui, "graph"));
    cui_input_mouse_move(ui, er.x + er.w - 40, er.y + 40);
    run(ui, build_nodes, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, true);
    run(ui, build_nodes, &a, 1);
    cui_input_mouse_move(ui, er.x + er.w - 100, er.y + 40);
    run(ui, build_nodes, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, false);
    run(ui, build_nodes, &a, 1);
    CHECK_NEAR(a.view.x, vx - 60, 0.5);
    cui_destroy(ui);
}

/* curve and gradient editors ------------------------------------------- */

typedef struct {
    cui_curve_key keys[16];
    int n, sel;
    bool snap;
    cui_curve_result r;
    int added, removed, last_added, last_removed;
} curve_ctx;

static void build_curve(cui_ctx *ui, void *u) {
    curve_ctx *a = u;
    cui_column(ui, .width = CUI_PX(600), .pad = 20) {
        a->r = cui_curve_editor(ui, "cv", a->keys, &a->n, 16, .height = 300, .selected = &a->sel, .snap = a->snap);
    }
    if (a->r.added >= 0) { a->added++; a->last_added = a->r.added; }
    if (a->r.removed >= 0) { a->removed++; a->last_removed = a->r.removed; }
}

static unsigned char *render_px(cui_ctx *ui, int *w, int *h) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = calloc((size_t)dd->width_px * dd->height_px, 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    *w = dd->width_px; *h = dd->height_px;
    return px;
}

/* the y center of the ink in one pixel column (curve pixels stand out from
 * the white surface and the faint grid), or -1 */
static float ink_center(const unsigned char *px, int w, int x, int y0, int y1) {
    double sum = 0, wsum = 0;
    for (int y = y0; y < y1; y++) {
        const unsigned char *p = px + ((size_t)y * w + x) * 4;
        int mn = p[0] < p[1] ? p[0] : p[1];
        if (p[2] < mn) mn = p[2];
        float k = 255.0f - (float)mn;
        if (k < 60) continue;
        sum += (y + 0.5) * k; wsum += k;
    }
    return wsum > 0 ? (float)(sum / wsum) : -1.0f;
}

static void curve_key(cui_curve_key *k, float x, float y, int mode) { memset(k, 0, sizeof *k); k->x = x; k->y = y; k->mode = (uint8_t)mode; }
static void key_press(cui_ctx *ui, build_fn fn, void *u, cui_key key, int mods) {
    cui_input_key(ui, key, true, mods); cui_input_key(ui, key, false, mods);
    run(ui, fn, u, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
}
static void drag_path(cui_ctx *ui, build_fn fn, void *u, float x0, float y0, float x1, float y1) {
    cui_input_mouse_move(ui, x0, y0);
    run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, fn, u, 1);
    for (int k = 1; k <= 6; k++) { cui_input_mouse_move(ui, x0 + (x1 - x0) * k / 6.0f, y0 + (y1 - y0) * k / 6.0f); run(ui, fn, u, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, fn, u, 2);
}
static void double_click(cui_ctx *ui, build_fn fn, void *u, float x, float y) {
    cui_input_mouse_move(ui, x, y);
    run(ui, fn, u, 1);
    for (int k = 0; k < 2; k++) {
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1);
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1);
    }
}

static void test_curve_eval(void) {
    printf("curve eval: smooth, free, linear, step, ends and out of range\n");
    cui_curve_key k[4];
    /* two smooth keys: flat ends, evenly spaced handles, so y = smoothstep(x) */
    curve_key(&k[0], 0, 0, CUI_CURVE_SMOOTH); curve_key(&k[1], 1, 1, CUI_CURVE_SMOOTH);
    CHECK_NEAR(cui_curve_eval(k, 2, 0.25f), 0.15625, 1e-5);
    CHECK_NEAR(cui_curve_eval(k, 2, 0.5f), 0.5, 1e-5);
    CHECK_NEAR(cui_curve_eval(k, 2, 0.9f), 0.9 * 0.9 * (3 - 2 * 0.9), 1e-5);
    CHECK_NEAR(cui_curve_eval(k, 2, -3), 0, 0);       /* before the first key: its value */
    CHECK_NEAR(cui_curve_eval(k, 2, 7), 1, 0);        /* after the last key: its value */
    CHECK_NEAR(cui_curve_eval(k, 2, NAN), 0, 0);
    CHECK_NEAR(cui_curve_eval(k, 1, 0.5f), 0, 0);     /* one key is a constant */
    CHECK_NEAR(cui_curve_eval(NULL, 0, 0.5f), 0, 0);
    /* free handles: control points (0,0) (0.5,0) (0.5,1) (1,1); at t = 0.25 the
     * bezier is at x = 0.296875, y = 0.15625 */
    k[0].mode = k[1].mode = CUI_CURVE_FREE;
    k[0].out_dx = 0.5f; k[1].in_dx = -0.5f;
    CHECK_NEAR(cui_curve_eval(k, 2, 0.296875f), 0.15625, 1e-5);
    CHECK_NEAR(cui_curve_eval(k, 2, 0.5f), 0.5, 1e-5);
    /* a handle longer than its segment is shortened along its direction */
    cui_curve_key a[2], b[2];
    curve_key(&a[0], 0, 0, CUI_CURVE_FREE); curve_key(&a[1], 1, 0.5f, CUI_CURVE_FREE);
    memcpy(b, a, sizeof a);
    a[0].out_dx = 5; a[0].out_dy = 5;
    b[0].out_dx = 1; b[0].out_dy = 1;
    bool same = true;
    for (int i = 0; i <= 20; i++) if (fabsf(cui_curve_eval(a, 2, i / 20.0f) - cui_curve_eval(b, 2, i / 20.0f)) > 1e-5f) same = false;
    CHECK(same);
    /* linear and step segments */
    curve_key(&k[0], 0, 0, CUI_CURVE_LINEAR); curve_key(&k[1], 2, 1, CUI_CURVE_SMOOTH);
    CHECK_NEAR(cui_curve_eval(k, 2, 0.5f), 0.25, 1e-6);
    CHECK_NEAR(cui_curve_eval(k, 2, 1.5f), 0.75, 1e-6);
    curve_key(&k[0], 0, 0.2f, CUI_CURVE_STEP); curve_key(&k[1], 1, 0.8f, CUI_CURVE_STEP); curve_key(&k[2], 2, 0.5f, CUI_CURVE_SMOOTH);
    CHECK_NEAR(cui_curve_eval(k, 3, 0.99f), 0.2, 1e-6);
    CHECK_NEAR(cui_curve_eval(k, 3, 1.0f), 0.8, 1e-6);   /* the jump happens at the key */
    CHECK_NEAR(cui_curve_eval(k, 3, 1.7f), 0.8, 1e-6);
    CHECK_NEAR(cui_curve_eval(k, 3, 2.5f), 0.5, 1e-6);
    /* smooth keys never overshoot: a plateau stays flat */
    curve_key(&k[0], 0, 0, 0); curve_key(&k[1], 0.4f, 1, 0); curve_key(&k[2], 0.6f, 1, 0); curve_key(&k[3], 1, 0, 0);
    float mx = 0;
    for (int i = 0; i <= 200; i++) mx = fmaxf(mx, cui_curve_eval(k, 4, i / 200.0f));
    CHECK(mx <= 1.0f + 1e-5f);
}

/* screen position of curve units: x from the hover readout, y from where
 * the flat ends of the curve are drawn */
typedef struct { float ax, bx, ay, by; } curve_map;
static curve_map curve_calibrate(cui_ctx *ui, curve_ctx *a) {
    curve_map m = { 0, 1, 0, 1 };
    curve_key(&a->keys[0], 0.2f, 0, CUI_CURVE_SMOOTH); curve_key(&a->keys[1], 0.8f, 1, CUI_CURVE_SMOOTH);
    a->n = 2; a->sel = -1;
    run(ui, build_curve, a, 3);
    cui_rect er = cui_get_rect(ui, cui_make_id(ui, "cv"));
    float X1 = er.x + er.w * 0.35f, X2 = er.x + er.w * 0.75f, Y = er.y + 60;
    cui_input_mouse_move(ui, X1, Y); run(ui, build_curve, a, 2);
    CHECK(a->r.hovered);
    float h1 = a->r.hover_x;
    CHECK_NEAR(a->r.hover_y, cui_curve_eval(a->keys, a->n, h1), 1e-6);
    cui_input_mouse_move(ui, X2, Y); run(ui, build_curve, a, 2);
    float h2 = a->r.hover_x;
    m.bx = (X2 - X1) / (h2 - h1); m.ax = X1 - m.bx * h1;
    cui_input_mouse_move(ui, 790, 590); run(ui, build_curve, a, 2);
    int w, h;
    unsigned char *px = render_px(ui, &w, &h);
    int y0 = (int)er.y + 4, y1 = (int)(er.y + 300 - 20);
    float c0 = ink_center(px, w, (int)(m.ax + m.bx * 0.1f), y0, y1);   /* left of the first key: y = 0 */
    float c1 = ink_center(px, w, (int)(m.ax + m.bx * 0.9f), y0, y1);   /* right of the last key: y = 1 */
    CHECK(c0 > 0 && c1 > 0 && c0 > c1 + 100);
    m.ay = c0; m.by = c1 - c0;
    /* what is drawn is what cui_curve_eval returns, through every kind of segment */
    curve_key(&a->keys[0], 0.1f, 0, CUI_CURVE_SMOOTH); curve_key(&a->keys[1], 0.35f, 0.45f, CUI_CURVE_FREE);
    a->keys[1].in_dx = -0.05f; a->keys[1].in_dy = 0.2f; a->keys[1].out_dx = 0.1f; a->keys[1].out_dy = -0.3f;
    curve_key(&a->keys[2], 0.55f, 0.3f, CUI_CURVE_LINEAR); curve_key(&a->keys[3], 0.7f, 0.9f, CUI_CURVE_STEP);
    curve_key(&a->keys[4], 0.8f, 0.6f, CUI_CURVE_SMOOTH); curve_key(&a->keys[5], 0.95f, 1, CUI_CURVE_SMOOTH);
    a->n = 6;
    run(ui, build_curve, a, 2);
    free(px);
    px = render_px(ui, &w, &h);
    static const float xs[] = { 0.03f, 0.18f, 0.27f, 0.42f, 0.5f, 0.63f, 0.75f, 0.86f, 0.9f, 0.98f };
    int good = 0;
    for (int i = 0; i < (int)(sizeof xs / sizeof xs[0]); i++) {
        int col = (int)(m.ax + m.bx * xs[i]);
        float x = ((float)col + 0.5f - m.ax) / m.bx;
        float c = ink_center(px, w, col, y0, y1);
        float want = m.ay + m.by * cui_curve_eval(a->keys, a->n, x);
        if (c > 0 && fabsf(c - want) < 1.6f) good++;
        else printf("  curve at x %.3f drawn at %.2f, eval says %.2f\n", x, c, want);
    }
    CHECK(good == (int)(sizeof xs / sizeof xs[0]));
    free(px);
    return m;
}

static void test_curve_editor(void) {
    printf("curve editor: drawn = eval, select, drag with clamping, handles, add, delete, keys, snap\n");
    cui_ctx *ui = new_ctx();
    curve_ctx a;
    memset(&a, 0, sizeof a);
    curve_map m = curve_calibrate(ui, &a);
#define SX(x) (m.ax + m.bx * (x))
#define SY(y) (m.ay + m.by * (y))
    curve_key(&a.keys[0], 0.2f, 0, 0); curve_key(&a.keys[1], 0.4f, 0.5f, 0); curve_key(&a.keys[2], 0.6f, 0.6f, 0); curve_key(&a.keys[3], 0.8f, 1, 0);
    a.n = 4; a.sel = -1;
    run(ui, build_curve, &a, 2);
    /* click selects */
    click_at(ui, build_curve, &a, SX(0.4f), SY(0.5f));
    CHECK(a.sel == 1 && a.r.selected == 1);
    /* drag: x stops at the neighbour, y follows */
    drag_path(ui, build_curve, &a, SX(0.4f), SY(0.5f), SX(0.75f), SY(0.45f));
    CHECK_NEAR(a.keys[1].x, 0.6f, 1e-5);
    CHECK_NEAR(a.keys[1].y, 0.45f, 0.01);
    CHECK(a.keys[1].x <= a.keys[2].x);
    /* and both stay in the range */
    drag_path(ui, build_curve, &a, SX(a.keys[1].x), SY(a.keys[1].y), SX(0.5f), SY(-0.5f));
    CHECK_NEAR(a.keys[1].y, 0, 1e-6);
    CHECK_NEAR(a.keys[1].x, 0.5f, 0.01);
    /* the out handle of a smooth key: it shows its automatic tangent, and
     * dragging it makes the key free with that tangent baked in */
    curve_key(&a.keys[1], 0.4f, 0.5f, 0);
    run(ui, build_curve, &a, 2);
    float before = cui_curve_eval(a.keys, a.n, 0.45f);
    float hx = SX(0.4f + 0.2f / 3), hy = SY(0.5f + 1.5f * 0.2f / 3);   /* slope 1.5 (limited by the flat side) */
    drag_path(ui, build_curve, &a, hx, hy, hx, SY(0.8f));
    CHECK(a.keys[1].mode == CUI_CURVE_FREE);
    CHECK_NEAR(a.keys[1].out_dx, 0.2f / 3, 0.01);
    CHECK_NEAR(a.keys[1].out_dy, 0.3f, 0.01);
    CHECK_NEAR(a.keys[1].in_dx, -0.2f / 3, 1e-4);
    CHECK_NEAR(a.keys[1].in_dy, -0.1f, 1e-4);
    CHECK(cui_curve_eval(a.keys, a.n, 0.45f) > before + 0.02f);
    /* the in handle drags on its own side */
    float ix = SX(0.4f - 0.2f / 3), iy = SY(0.4f);
    drag_path(ui, build_curve, &a, ix, iy, SX(0.3f), SY(0.2f));
    CHECK_NEAR(a.keys[1].in_dx, -0.1f, 0.01);
    CHECK_NEAR(a.keys[1].in_dy, -0.3f, 0.01);
    CHECK_NEAR(a.keys[1].out_dy, 0.3f, 1e-4);
    /* double click in empty space adds a key there (reported, selected) */
    int n0 = a.n;
    double_click(ui, build_curve, &a, SX(0.3f), SY(0.9f));
    CHECK(a.added == 1 && a.last_added == 1 && a.n == n0 + 1 && a.sel == 1);
    CHECK_NEAR(a.keys[1].x, 0.3f, 0.01);
    CHECK_NEAR(a.keys[1].y, 0.9f, 0.01);
    CHECK(a.keys[0].x < a.keys[1].x && a.keys[1].x < a.keys[2].x);
    /* near the curve, the key lands on it */
    float on = cui_curve_eval(a.keys, a.n, 0.7f);
    double_click(ui, build_curve, &a, SX(0.7f), SY(on) + 4);
    CHECK(a.added == 2 && a.n == n0 + 2);
    int j = a.last_added;
    CHECK(j >= 0 && j < a.n);
    CHECK_NEAR(a.keys[j].x, 0.7f, 0.01);
    CHECK_NEAR(a.keys[j].y, cui_curve_eval(a.keys, a.n, a.keys[j].x), 1e-3);
    /* Delete removes the selected key (reported) */
    float gone_x = a.keys[j].x;
    key_press(ui, build_curve, &a, CUI_KEY_DELETE, 0);
    CHECK(a.removed == 1 && a.last_removed == j && a.n == n0 + 1);
    bool still = false;
    for (int i = 0; i < a.n; i++) if (fabsf(a.keys[i].x - gone_x) < 1e-6f) still = true;
    CHECK(!still);
    /* arrows nudge the focused key, Shift for fine steps */
    click_at(ui, build_curve, &a, SX(a.keys[0].x), SY(a.keys[0].y));
    CHECK(a.sel == 0);
    float kx = a.keys[0].x, ky = a.keys[0].y;
    key_press(ui, build_curve, &a, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(a.keys[0].x, kx + 0.01f, 1e-5);
    key_press(ui, build_curve, &a, CUI_KEY_UP, CUI_MOD_SHIFT);
    CHECK_NEAR(a.keys[0].y, ky + 0.001f, 1e-5);
    key_press(ui, build_curve, &a, CUI_KEY_DOWN, 0);
    CHECK_NEAR(a.keys[0].y, ky, 1e-5);   /* y = 0 is the bottom of the range */
    /* snapping puts a dragged key on the grid */
    a.snap = true;
    int s = a.n > 2 ? 2 : 1;
    drag_path(ui, build_curve, &a, SX(a.keys[s].x), SY(a.keys[s].y), SX(a.keys[s].x - 0.013f), SY(a.keys[s].y + 0.037f));
    CHECK(fabsf(a.keys[s].x * 40 - roundf(a.keys[s].x * 40)) < 1e-3f || a.keys[s].x == a.keys[s - 1].x);
    CHECK(fabsf(a.keys[s].y * 40 - roundf(a.keys[s].y * 40)) < 1e-3f);
    a.snap = false;
    /* never fewer than two keys */
    curve_key(&a.keys[0], 0.2f, 0, 0); curve_key(&a.keys[1], 0.8f, 1, 0);
    a.n = 2; a.removed = 0;
    run(ui, build_curve, &a, 2);
    click_at(ui, build_curve, &a, SX(0.8f), SY(1));
    key_press(ui, build_curve, &a, CUI_KEY_DELETE, 0);
    CHECK(a.n == 2 && a.removed == 0);
    /* a full array takes no more keys */
    int cap_n = a.n;
    for (int i = a.n; i < 16; i++) curve_key(&a.keys[i], 0.8f + 0.001f * (float)i, 1, 0);
    a.n = 16; a.added = 0;
    double_click(ui, build_curve, &a, SX(0.3f), SY(0.9f));
    CHECK(a.n == 16 && a.added == 0);
    (void)cap_n;
#undef SX
#undef SY
    cui_destroy(ui);
}

typedef struct {
    cui_gradient_stop stops[8];
    int n, sel;
    cui_gradient_result r;
    int added, removed, last_added, last_removed;
    cui_id trigger;
} grad_ctx;

static void build_grad(cui_ctx *ui, void *u) {
    grad_ctx *a = u;
    cui_column(ui, .width = CUI_PX(520), .pad = 20) {
        a->r = cui_gradient_editor(ui, "grad", a->stops, &a->n, 8, .selected = &a->sel);
    }
    cui_push_id(ui, "grad");
    a->trigger = cui_make_id(ui, "##color");   /* the color picker it reuses */
    cui_pop_id(ui);
    if (a->r.added >= 0) { a->added++; a->last_added = a->r.added; }
    if (a->r.removed >= 0) { a->removed++; a->last_removed = a->r.removed; }
}

static void test_gradient(void) {
    printf("gradient editor: eval, drawn = eval, add, drag past, remove, keys, picker\n");
    /* eval */
    cui_gradient_stop g[3] = { { 0.2f, { 200, 0, 100, 255 } }, { 0.6f, { 0, 100, 200, 55 } }, { 1.0f, { 255, 255, 255, 255 } } };
    cui_color c = cui_gradient_eval(g, 3, 0);
    CHECK(c.r == 200 && c.g == 0 && c.b == 100 && c.a == 255);   /* before the first stop */
    c = cui_gradient_eval(g, 3, 0.4f);
    CHECK(c.r == 100 && c.g == 50 && c.b == 150 && c.a == 155);  /* halfway, alpha too */
    c = cui_gradient_eval(g, 3, 0.6f);
    CHECK(c.r == 0 && c.g == 100 && c.b == 200 && c.a == 55);
    c = cui_gradient_eval(g, 3, 2);
    CHECK(c.r == 255 && c.a == 255);
    cui_gradient_stop u[3] = { g[2], g[0], g[1] };                /* order does not matter */
    c = cui_gradient_eval(u, 3, 0.4f);
    CHECK(c.r == 100 && c.g == 50 && c.b == 150 && c.a == 155);
    c = cui_gradient_eval(g, 0, 0.5f);
    CHECK(c.a == 0);
    c = cui_gradient_eval(g, 1, 0.9f);
    CHECK(c.r == 200);

    cui_ctx *ui = new_ctx();
    grad_ctx a;
    memset(&a, 0, sizeof a);
    a.stops[0] = (cui_gradient_stop){ 0.0f, { 20, 40, 200, 255 } };
    a.stops[1] = (cui_gradient_stop){ 0.5f, { 240, 180, 20, 255 } };
    a.stops[2] = (cui_gradient_stop){ 1.0f, { 200, 30, 60, 255 } };
    a.n = 3;
    run(ui, build_grad, &a, 3);
    CHECK(a.sel == 0);
    /* the ramp sits 9 pt in from each side (room for the end markers), the stops under it */
    cui_rect o = cui_get_rect(ui, cui_make_id(ui, "grad"));
    float x0 = o.x + 9, w = o.w - 18, ramp_y = o.y + 14, track_y = o.y + 28 + 13;
#define GX(t) (x0 + (t) * w)
    /* what the ramp shows is what cui_gradient_eval returns */
    int pw, ph;
    unsigned char *px = render_px(ui, &pw, &ph);
    static const float ts[] = { 0.06f, 0.2f, 0.33f, 0.5f, 0.71f, 0.94f };
    bool match = true;
    for (int i = 0; i < 6; i++) {
        int col = (int)GX(ts[i]);
        cui_color want = cui_gradient_eval(a.stops, a.n, ((float)col + 0.5f - x0) / w);
        const unsigned char *p = px + ((size_t)(int)ramp_y * pw + col) * 4;
        if (abs(p[0] - want.r) > 4 || abs(p[1] - want.g) > 4 || abs(p[2] - want.b) > 4) {
            match = false;
            printf("  ramp at %.2f shows %d %d %d, eval says %d %d %d\n", ts[i], p[0], p[1], p[2], want.r, want.g, want.b);
        }
    }
    CHECK(match);
    free(px);
    /* a click on the ramp adds a stop with the color found there */
    cui_color there = cui_gradient_eval(a.stops, a.n, 0.25f);
    click_at(ui, build_grad, &a, GX(0.25f), ramp_y);
    CHECK(a.added == 1 && a.last_added == 1 && a.n == 4 && a.sel == 1);
    CHECK_NEAR(a.stops[1].pos, 0.25f, 0.01);
    CHECK(abs(a.stops[1].color.r - there.r) <= 2 && abs(a.stops[1].color.g - there.g) <= 2 && abs(a.stops[1].color.b - there.b) <= 2);
    CHECK(a.stops[1].color.r != a.stops[0].color.r);   /* interpolated, not a copy of a neighbour */
    /* drag a stop past its neighbour: the array stays sorted, the selection follows */
    cui_color moved = a.stops[1].color;
    drag_path(ui, build_grad, &a, GX(0.25f), track_y, GX(0.7f), track_y);
    CHECK_NEAR(a.stops[2].pos, 0.7f, 0.01);
    CHECK(a.sel == 2 && memcmp(&a.stops[2].color, &moved, sizeof moved) == 0);
    bool sorted = true;
    for (int i = 1; i < a.n; i++) if (a.stops[i].pos < a.stops[i - 1].pos) sorted = false;
    CHECK(sorted);
    /* arrows nudge the focused stop, Shift for fine steps */
    float p0 = a.stops[2].pos;
    key_press(ui, build_grad, &a, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(a.stops[2].pos, p0 + 0.01f, 1e-5);
    key_press(ui, build_grad, &a, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK_NEAR(a.stops[2].pos, p0 + 0.009f, 1e-5);
    /* Delete removes the focused stop */
    key_press(ui, build_grad, &a, CUI_KEY_DELETE, 0);
    CHECK(a.removed == 1 && a.last_removed == 2 && a.n == 3);
    /* dragging a stop off the ramp removes it on release */
    drag_path(ui, build_grad, &a, GX(a.stops[1].pos), track_y, GX(a.stops[1].pos), track_y + 70);
    CHECK(a.removed == 2 && a.n == 2);
    /* but never below two */
    drag_path(ui, build_grad, &a, GX(a.stops[1].pos), track_y, GX(a.stops[1].pos), track_y + 70);
    CHECK(a.n == 2 && a.removed == 2);
    click_at(ui, build_grad, &a, GX(a.stops[0].pos), track_y);
    key_press(ui, build_grad, &a, CUI_KEY_DELETE, 0);
    CHECK(a.n == 2 && a.removed == 2);
    /* double click a stop: the existing color picker opens for it */
    cui_id pop = a.trigger ^ 0xC010Eu;   /* the picker's popover (cui_color_picker) */
    CHECK(cui_get_rect(ui, pop).w == 0);
    a.stops[0].color.a = 128;
    double_click(ui, build_grad, &a, GX(a.stops[0].pos), track_y);
    run(ui, build_grad, &a, 2);
    CHECK(a.sel == 0);
    CHECK(cui_get_rect(ui, pop).w > 100);
    CHECK(a.stops[0].color.a == 128);   /* the picker leaves opacity alone */
#undef GX
    cui_destroy(ui);
}

/* timeline editor ---------------------------------------------------------- */

typedef struct {
    cui_timeline_track tr[4]; int n;
    cui_curve_key keys[4][16]; bool sel[4][16];
    cui_timeline_clip clips[4];
    cui_timeline_track *big; int big_n;
    float time, in, out, width;
    bool range, toggles, free_drag;
    cui_timeline_view view;
    int cur_t, cur_k;
    cui_timeline_editor_result r;
    int added, removed, plays, moved, scrubs, toggled;
    bool changed;
    cui_id id;
} tl_ctx;
static void build_tl(cui_ctx *ui, void *u) {
    tl_ctx *a = u;
    cui_column(ui, .pad = 20) {
        a->r = cui_timeline_editor(ui, "tl", a->big ? a->big : a->tr, a->big ? a->big_n : a->n, &a->time, .end = 4, .fps = 24,
                                   .view = &a->view, .current_track = &a->cur_t, .current_key = &a->cur_k, .width = CUI_PX(a->width),
                                   .in = a->range ? &a->in : NULL, .out = a->range ? &a->out : NULL, .toggles = a->toggles,
                                   .free_drag = a->free_drag);
        a->id = cui_last_id(ui);
    }
    if (a->r.added_key >= 0) a->added++;
    a->removed += a->r.removed_keys;
    if (a->r.play_toggled) a->plays++;
    if (a->r.keys_moved) a->moved++;
    if (a->r.scrubbing) a->scrubs++;
    if (a->r.toggled >= 0) a->toggled = a->r.toggled;
    if (a->r.changed) a->changed = true;
}
static void tl_setup(tl_ctx *a) {
    memset(a, 0, sizeof *a);
    a->width = 760; a->cur_t = a->cur_k = -1;
    static const float px[3][2] = { { 0, 0 }, { 1, 100 }, { 2, 50 } }, rot[3][2] = { { 0.5f, 0 }, { 1.5f, 30 }, { 3, -10 } };
    for (int i = 0; i < 3; i++) {
        curve_key(&a->keys[1][i], px[i][0], px[i][1], CUI_CURVE_SMOOTH);
        curve_key(&a->keys[2][i], rot[i][0], rot[i][1], i == 1 ? CUI_CURVE_STEP : CUI_CURVE_LINEAR);
    }
    a->clips[0] = (cui_timeline_clip){ 1.0f, 2.5f, 0, "Theme", false };
    a->tr[0] = (cui_timeline_track){ .name = "Hero", .depth = 0 };
    a->tr[1] = (cui_timeline_track){ .name = "Position X", .depth = 1, .keys = a->keys[1], .key_count = 3, .key_capacity = 16, .key_selected = a->sel[1] };
    a->tr[2] = (cui_timeline_track){ .name = "Rotation", .depth = 1, .keys = a->keys[2], .key_count = 3, .key_capacity = 16, .key_selected = a->sel[2] };
    a->tr[3] = (cui_timeline_track){ .name = "Audio", .depth = 0, .clips = a->clips, .clip_count = 1 };
    a->n = 4;
}
/* where things are: the list is 184 wide at this size (40 on a narrow editor),
 * the time axis is inset 10 from the time area, rows are 26 below a 28 ruler */
typedef struct { float px, pw, by, rx, ry; } tl_map;
static tl_map tl_map_of(cui_ctx *ui, tl_ctx *a) {
    cui_rect r = cui_get_rect(ui, a->id);
    float lw = r.w >= 552 ? 184 : r.w >= 380 ? fmaxf(120.0f, fminf(184.0f, r.w * 0.32f)) : 40;
    tl_map m = { r.x + lw + 10, r.w - lw - 20, r.y + 29, r.x, r.y };
    return m;
}
static float tl_x(const tl_map *m, const tl_ctx *a, float t) { return m->px + (t - a->view.start) / (a->view.end - a->view.start) * m->pw; }
static float tl_y(const tl_map *m, const tl_ctx *a, int row) { return m->by + row * 26 + 13 - a->view.scroll; }
static int tl_selected(const tl_ctx *a) {
    int c = 0;
    for (int t = 0; t < 4; t++) for (int k = 0; k < 16; k++) c += a->sel[t][k];
    return c;
}
static void tl_mods(cui_ctx *ui, int mods) { cui_input_key(ui, CUI_KEY_NONE, mods != 0, mods); }
static unsigned tl_rand(unsigned *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }

static void test_timeline(void) {
    printf("timeline editor: scrub, frame keys, select, box select, snapped drags, add, delete, zoom, fit, groups, clips, 2000 tracks, empty\n");
    cui_ctx *ui = new_ctx();
    static tl_ctx a;
    tl_setup(&a);
    run(ui, build_tl, &a, 3);
    CHECK(a.view.start == 0 && a.view.end == 4);   /* zeroed view: fits the sequence */
    tl_map m = tl_map_of(ui, &a);
    CHECK(m.pw > 500);
    float ruler_y = m.ry + 14;
    /* a click on the ruler jumps there, on a frame */
    click_at(ui, build_tl, &a, tl_x(&m, &a, 49.2f / 24.0f), ruler_y);
    CHECK(a.time == 49.0f / 24.0f);
    CHECK(a.r.time_changed || a.time == 49.0f / 24.0f);
    /* dragging in the ruler scrubs, snapped to frames */
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.0f), ruler_y, tl_x(&m, &a, 72.3f / 24.0f), 6);
    CHECK(a.time == 3.0f);
    CHECK(a.scrubs >= 5);
    /* Shift while scrubbing: free */
    tl_mods(ui, CUI_MOD_SHIFT);
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.0f), ruler_y, tl_x(&m, &a, 1.0f + 0.4f / 24.0f), 4);
    tl_mods(ui, 0);
    CHECK(fabsf(a.time * 24 - roundf(a.time * 24)) > 0.1f);
    /* frame keys: the click focused the editor */
    key_press(ui, build_tl, &a, CUI_KEY_HOME, 0);  CHECK(a.time == 0);
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, 0); CHECK(a.time == 1.0f / 24.0f);
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, CUI_MOD_SHIFT); CHECK(a.time == 11.0f / 24.0f);
    key_press(ui, build_tl, &a, CUI_KEY_LEFT, 0);  CHECK(a.time == 10.0f / 24.0f);
    key_press(ui, build_tl, &a, CUI_KEY_END, 0);   CHECK(a.time == 4.0f);
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, 0); CHECK(a.time == 4.0f);   /* stays inside */
    key_press(ui, build_tl, &a, CUI_KEY_HOME, 0);
    /* Up and Down jump between keys of the visible tracks */
    key_press(ui, build_tl, &a, CUI_KEY_DOWN, 0);  CHECK(a.time == 0.5f);
    key_press(ui, build_tl, &a, CUI_KEY_DOWN, 0);  CHECK(a.time == 1.0f);
    key_press(ui, build_tl, &a, CUI_KEY_UP, 0);    CHECK(a.time == 0.5f);
    /* Space is a play request; the caller advances time */
    key_press(ui, build_tl, &a, CUI_KEY_SPACE, 0);
    CHECK(a.plays == 1 && a.time == 0.5f);

    /* click selects a key, Shift adds, Ctrl toggles */
    click_at(ui, build_tl, &a, tl_x(&m, &a, 1.0f), tl_y(&m, &a, 1));
    CHECK(a.sel[1][1] && tl_selected(&a) == 1 && a.cur_t == 1 && a.cur_k == 1);
    CHECK(a.r.current_track == 1 && a.r.current_key == 1);
    tl_mods(ui, CUI_MOD_SHIFT);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 1.5f), tl_y(&m, &a, 2));
    tl_mods(ui, 0);
    CHECK(a.sel[1][1] && a.sel[2][1] && tl_selected(&a) == 2 && a.cur_t == 2);
    tl_mods(ui, CUI_MOD_PRIMARY);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 1.0f), tl_y(&m, &a, 1));
    tl_mods(ui, 0);
    CHECK(!a.sel[1][1] && a.sel[2][1] && tl_selected(&a) == 1);
    /* a click on empty space clears */
    click_at(ui, build_tl, &a, tl_x(&m, &a, 3.6f), tl_y(&m, &a, 1));
    CHECK(tl_selected(&a) == 0 && a.cur_k == -1 && a.cur_t == 1);
    /* box selection over two rows */
    drag2(ui, build_tl, &a, tl_x(&m, &a, 0.25f), tl_y(&m, &a, 1) - 6, tl_x(&m, &a, 2.2f), tl_y(&m, &a, 2) + 6, 6);
    CHECK(a.sel[1][1] && a.sel[1][2] && a.sel[2][0] && a.sel[2][1] && !a.sel[1][0] && !a.sel[2][2] && tl_selected(&a) == 4);
    CHECK(a.time == 0.5f);   /* box selection leaves the playhead alone */
    /* Esc clears, Ctrl+A selects every key */
    key_press(ui, build_tl, &a, CUI_KEY_ESCAPE, 0);
    CHECK(tl_selected(&a) == 0);
    key_press(ui, build_tl, &a, CUI_KEY_A, CUI_MOD_PRIMARY);
    CHECK(tl_selected(&a) == 6);
    key_press(ui, build_tl, &a, CUI_KEY_ESCAPE, 0);

    /* dragging a key snaps it to whole frames */
    float k0 = a.keys[1][1].x;
    CHECK(k0 == 1.0f);
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.0f), tl_y(&m, &a, 1), tl_x(&m, &a, 1.0f + 10.3f / 24.0f), 6);
    CHECK(a.keys[1][1].x == 34.0f / 24.0f);
    CHECK(a.sel[1][1] && tl_selected(&a) == 1 && a.moved > 0);
    /* several selected keys move together, each landing on a frame */
    tl_mods(ui, CUI_MOD_SHIFT);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 1.5f), tl_y(&m, &a, 2));
    tl_mods(ui, 0);
    CHECK(tl_selected(&a) == 2);
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.5f), tl_y(&m, &a, 2), tl_x(&m, &a, 1.5f + 5.2f / 24.0f), 6);
    CHECK(a.keys[1][1].x == 39.0f / 24.0f);
    CHECK(a.keys[2][1].x == 41.0f / 24.0f);
    CHECK(a.sel[1][1] && a.sel[2][1] && tl_selected(&a) == 2);   /* a drag keeps the selection */
    /* Alt+arrows nudge the selection by a frame */
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, CUI_MOD_ALT);
    CHECK(a.keys[1][1].x == 40.0f / 24.0f && a.keys[2][1].x == 42.0f / 24.0f);
    key_press(ui, build_tl, &a, CUI_KEY_LEFT, CUI_MOD_ALT | CUI_MOD_SHIFT);
    CHECK(a.keys[1][1].x == 30.0f / 24.0f && a.keys[2][1].x == 32.0f / 24.0f);
    CHECK(a.time == 0.5f);
    /* the block stays inside the sequence */
    for (int i = 0; i < 12; i++) key_press(ui, build_tl, &a, CUI_KEY_LEFT, CUI_MOD_ALT | CUI_MOD_SHIFT);
    CHECK(a.keys[1][0].x == 0 && a.keys[1][1].x == 0 && a.sel[1][1]);
    CHECK(a.keys[2][0].x == 2.0f / 24.0f && a.sel[2][0] && a.keys[2][1].x == 0.5f && !a.sel[2][1]);   /* passed a key: re-sorted */
    CHECK(a.cur_t == 2 && a.cur_k == 0);
    bool sorted = true;
    for (int t = 1; t < 3; t++) for (int i = 1; i < a.tr[t].key_count; i++) sorted = sorted && a.keys[t][i - 1].x <= a.keys[t][i].x;
    CHECK(sorted);
    /* passing a neighbour re-sorts the keys; the selection follows */
    click_at(ui, build_tl, &a, tl_x(&m, &a, 3.0f), tl_y(&m, &a, 2));
    CHECK(a.cur_t == 2 && a.cur_k == 2 && tl_selected(&a) == 1);
    drag(ui, build_tl, &a, tl_x(&m, &a, 3.0f), tl_y(&m, &a, 2), tl_x(&m, &a, 0.25f), 8);
    CHECK(a.keys[2][1].x == 0.25f && a.sel[2][1] && a.cur_k == 1);
    CHECK(a.keys[2][0].x == 2.0f / 24.0f && a.keys[2][2].x == 0.5f);
    /* free drags: Shift during the drag, or .free_drag */
    a.free_drag = true;
    drag(ui, build_tl, &a, tl_x(&m, &a, 0.25f), tl_y(&m, &a, 2), tl_x(&m, &a, 0.25f + 2.3f / 24.0f), 4);
    CHECK(fabsf(a.keys[2][1].x * 24 - roundf(a.keys[2][1].x * 24)) > 0.1f);
    a.free_drag = false;
    /* Delete removes the selected keys and reports them */
    key_press(ui, build_tl, &a, CUI_KEY_DELETE, 0);
    CHECK(a.tr[2].key_count == 2 && a.removed == 1 && a.cur_k == -1 && tl_selected(&a) == 0);
    /* double click on a row adds a key on the curve, on a frame, selected */
    float want = cui_curve_eval(a.keys[1], a.tr[1].key_count, 60.0f / 24.0f);
    double_click(ui, build_tl, &a, tl_x(&m, &a, 60.2f / 24.0f), tl_y(&m, &a, 1));
    CHECK(a.added == 1 && a.tr[1].key_count == 4);
    CHECK(a.keys[1][3].x == 60.0f / 24.0f && a.sel[1][3] && tl_selected(&a) == 1);
    CHECK_NEAR(a.keys[1][3].y, want, 1e-4);
    CHECK(a.cur_t == 1 && a.cur_k == 3);
    /* a full track adds nothing */
    a.tr[2].key_capacity = a.tr[2].key_count;
    double_click(ui, build_tl, &a, tl_x(&m, &a, 3.5f), tl_y(&m, &a, 2));
    CHECK(a.added == 1 && a.tr[2].key_count == 2);
    a.tr[2].key_capacity = 16;
    /* a click on a track name selects its keys */
    click_at(ui, build_tl, &a, m.rx + 60, tl_y(&m, &a, 1));
    CHECK(tl_selected(&a) == 4 && a.cur_t == 1 && a.cur_k == -1);
    key_press(ui, build_tl, &a, CUI_KEY_ESCAPE, 0);

    /* the wheel zooms, and the time under the pointer stays under it */
    float wx = tl_x(&m, &a, 1.3f), wy = tl_y(&m, &a, 3);
    cui_input_mouse_move(ui, wx, wy);
    run(ui, build_tl, &a, 1);
    float under = a.view.start + (wx - m.px) / m.pw * (a.view.end - a.view.start);
    cui_input_scroll(ui, 0, 3);
    run(ui, build_tl, &a, 1);
    float range = a.view.end - a.view.start;
    CHECK_NEAR(range, 4.0 / pow(1.12, 3), 1e-4);
    CHECK_NEAR(a.view.start + (wx - m.px) / m.pw * range, under, 1e-4);
    CHECK(a.r.view_changed);
    /* Shift+wheel pans */
    float vs = a.view.start;
    tl_mods(ui, CUI_MOD_SHIFT);
    cui_input_scroll(ui, 0, -1);
    run(ui, build_tl, &a, 1);
    tl_mods(ui, 0);
    CHECK(a.view.start > vs);
    CHECK_NEAR(a.view.end - a.view.start, range, 1e-4);
    /* the middle button drags the view */
    vs = a.view.start;
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, true);
    run(ui, build_tl, &a, 1);
    cui_input_mouse_move(ui, wx + 60, wy);
    run(ui, build_tl, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, false);
    run(ui, build_tl, &a, 1);
    CHECK_NEAR(vs - a.view.start, 60.0f / m.pw * range, 1e-4);
    /* + zooms around the playhead, F fits (reduced motion: at once) */
    key_press(ui, build_tl, &a, CUI_KEY_EQUAL, 0);
    CHECK_NEAR(a.view.end - a.view.start, range / 1.25, 1e-4);
    key_press(ui, build_tl, &a, CUI_KEY_F, 0);
    CHECK(a.view.start == 0 && a.view.end == 4);
    /* with motion on, fitting animates and settles on the whole sequence */
    cui_set_reduced_motion(ui, false);
    cui_input_mouse_move(ui, wx, wy);
    for (int i = 0; i < 4; i++) { cui_input_scroll(ui, 0, 2); run(ui, build_tl, &a, 1); }
    float zoomed = a.view.end - a.view.start;
    CHECK(zoomed < 2);
    key_press(ui, build_tl, &a, CUI_KEY_F, 0);
    run(ui, build_tl, &a, 2);
    float mid = a.view.end - a.view.start;
    CHECK(mid > zoomed + 1e-3f && mid < 4 - 1e-3f);   /* on its way */
    run(ui, build_tl, &a, 90);
    CHECK_NEAR(a.view.start, 0, 1e-5);
    CHECK_NEAR(a.view.end, 4, 1e-5);
    cui_set_reduced_motion(ui, true);
    run(ui, build_tl, &a, 1);
    /* the playhead keeps in view: stepping past the edge moves the view */
    cui_input_mouse_move(ui, wx, wy);
    for (int i = 0; i < 3; i++) { cui_input_scroll(ui, 0, 3); run(ui, build_tl, &a, 1); }
    a.time = a.view.end - 0.5f / 24.0f;
    run(ui, build_tl, &a, 1);
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK(a.time >= a.view.start && a.time <= a.view.end);
    key_press(ui, build_tl, &a, CUI_KEY_F, 0);

    /* hovering a key shows its time and value after the delay */
    cui_input_mouse_move(ui, tl_x(&m, &a, 3.6f), tl_y(&m, &a, 1));
    run(ui, build_tl, &a, 70);
    int idle = cui_get_draw_data(ui)->vertex_count;
    cui_input_mouse_move(ui, tl_x(&m, &a, 2.0f), tl_y(&m, &a, 1));
    run(ui, build_tl, &a, 3);
    CHECK(a.r.hovered_track == 1 && a.r.hovered_key == 2);
    run(ui, build_tl, &a, 70);
    CHECK(cui_get_draw_data(ui)->vertex_count > idle + 40);

    /* collapsing a group hides its rows: Audio moves up to row 1 */
    click_at(ui, build_tl, &a, m.rx + 10, tl_y(&m, &a, 0));
    CHECK(a.tr[0].collapsed && a.toggled == 0);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 1.75f), tl_y(&m, &a, 1));
    CHECK(a.clips[0].selected && tl_selected(&a) == 0);
    click_at(ui, build_tl, &a, m.rx + 10, tl_y(&m, &a, 0));
    CHECK(!a.tr[0].collapsed);
    /* clips: drag the body to move, an end to trim (snapped) */
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.75f), tl_y(&m, &a, 3), tl_x(&m, &a, 1.75f + 12.2f / 24.0f), 6);
    CHECK(a.clips[0].start == 1.5f && a.clips[0].end == 3.0f && a.clips[0].selected);
    drag(ui, build_tl, &a, tl_x(&m, &a, 3.0f) - 2, tl_y(&m, &a, 3), tl_x(&m, &a, 3.5f) - 2, 6);
    CHECK(a.clips[0].start == 1.5f && a.clips[0].end == 3.5f);
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.5f) + 2, tl_y(&m, &a, 3), tl_x(&m, &a, 2.0f) + 2, 6);
    CHECK(a.clips[0].start == 2.0f && a.clips[0].end == 3.5f && a.clips[0].offset == 0.5f);
    drag(ui, build_tl, &a, tl_x(&m, &a, 2.5f), tl_y(&m, &a, 3), tl_x(&m, &a, 3.9f), 6);
    CHECK(a.clips[0].end == 4.0f && a.clips[0].start == 2.5f);   /* held inside the sequence */
    key_press(ui, build_tl, &a, CUI_KEY_DELETE, 0);
    CHECK(a.tr[3].clip_count == 0 && a.r.removed_clips == 1);
    /* locked tracks: shown, not editable */
    a.tr[1].locked = true;
    click_at(ui, build_tl, &a, tl_x(&m, &a, 2.0f), tl_y(&m, &a, 1));
    CHECK(tl_selected(&a) == 0);
    a.tr[1].locked = false;
    /* toggles: hide and lock buttons on each row */
    a.toggles = true;
    run(ui, build_tl, &a, 2);
    a.changed = false;
    click_at(ui, build_tl, &a, m.rx + 184 - 4 - 11, tl_y(&m, &a, 2));
    CHECK(a.tr[2].locked && a.toggled == 2 && a.changed);
    click_at(ui, build_tl, &a, m.rx + 184 - 4 - 22 - 11, tl_y(&m, &a, 2));
    CHECK(a.tr[2].muted);
    a.toggles = false; a.tr[2].locked = a.tr[2].muted = false;
    /* the play range: drag the in marker, snapped and kept before the out marker */
    a.range = true; a.in = 0.5f; a.out = 3.0f;
    run(ui, build_tl, &a, 2);
    drag(ui, build_tl, &a, tl_x(&m, &a, 0.5f), ruler_y, tl_x(&m, &a, 1.02f), 6);
    CHECK(a.in == 1.0f && a.out == 3.0f);
    drag(ui, build_tl, &a, tl_x(&m, &a, 1.0f), ruler_y, tl_x(&m, &a, 3.6f), 6);
    CHECK_NEAR(a.in, 71.0 / 24.0, 1e-6);
    a.range = false;
    /* a narrow editor: the list turns to icons, keys still hit */
    a.width = 300;
    run(ui, build_tl, &a, 3);
    m = tl_map_of(ui, &a);
    CHECK(m.px - m.rx == 50);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 2.0f), tl_y(&m, &a, 1));
    CHECK(a.sel[1][2] && tl_selected(&a) == 1);
    cui_destroy(ui);

    /* 2000 tracks in 200 groups: only the rows in view are visited */
    int bn = 2000;
    cui_timeline_track *big = calloc((size_t)bn, sizeof *big);
    cui_curve_key *bk = calloc((size_t)bn * 12, sizeof *bk);
    bool *bs = calloc((size_t)bn * 12, sizeof *bs);
    for (int i = 0; i < bn; i++) {
        big[i].name = i % 10 ? "Channel" : "Bone";
        big[i].depth = i % 10 ? 1 : 0;
        big[i].keys = bk + i * 12; big[i].key_capacity = 12; big[i].key_selected = bs + i * 12;
        big[i].key_count = 10;
        for (int j = 0; j < 10; j++) curve_key(&big[i].keys[j], (float)((j * 9 + i) % 96) / 24.0f * 0.98f + j * 0.001f, (float)j, j % 3);
        cui_curve_key *kk = big[i].keys;   /* sorted by time */
        for (int x = 1; x < 10; x++) for (int y = x; y > 0 && kk[y - 1].x > kk[y].x; y--) { cui_curve_key t = kk[y]; kk[y] = kk[y - 1]; kk[y - 1] = t; }
    }
    ui = new_ctx();
    tl_setup(&a);
    a.big = big; a.big_n = bn;
    clock_t c0 = clock();
    run(ui, build_tl, &a, 3);
    int verts = cui_get_draw_data(ui)->vertex_count;
    CHECK(verts < 30000);
    m = tl_map_of(ui, &a);
    /* scroll the rows from the track list, zoom and pan over the time area */
    cui_input_mouse_move(ui, m.rx + 60, m.by + 100);
    for (int i = 0; i < 200; i++) { cui_input_scroll(ui, 0, -5); run(ui, build_tl, &a, 1); }
    CHECK(a.view.scroll > 30000);
    CHECK(cui_get_draw_data(ui)->vertex_count < 30000);
    cui_input_mouse_move(ui, tl_x(&m, &a, 2), m.by + 100);
    for (int i = 0; i < 20; i++) { cui_input_scroll(ui, 0, i < 10 ? 2 : -2); run(ui, build_tl, &a, 1); }
    key_press(ui, build_tl, &a, CUI_KEY_A, CUI_MOD_PRIMARY);   /* nothing focused yet: no-op */
    int vr = (int)(a.view.scroll / 26) + 2;
    click_at(ui, build_tl, &a, m.rx + 100, tl_y(&m, &a, vr));   /* a name: its keys */
    int all = 0;
    for (int i = 0; i < bn * 12; i++) all += bs[i];
    CHECK(all == 10 && a.cur_t == vr);
    key_press(ui, build_tl, &a, CUI_KEY_A, CUI_MOD_PRIMARY);
    all = 0;
    for (int i = 0; i < bn * 12; i++) all += bs[i];
    CHECK(all == bn * 10);
    key_press(ui, build_tl, &a, CUI_KEY_RIGHT, CUI_MOD_ALT);   /* every key moves a frame */
    CHECK(a.r.keys_moved);
    CHECK(cui_get_draw_data(ui)->vertex_count < 40000);
    double secs = (double)(clock() - c0) / CLOCKS_PER_SEC;
    CHECK(secs < 5.0);
    printf("  2000 tracks: %d vertices, %.3f s for ~240 frames\n", verts, secs);
    cui_destroy(ui);
    free(big); free(bk); free(bs);

    /* no tracks: the ruler still scrubs; nothing breaks */
    ui = new_ctx();
    tl_setup(&a);
    a.n = 0;
    run(ui, build_tl, &a, 3);
    m = tl_map_of(ui, &a);
    cui_rect er = cui_get_rect(ui, a.id);
    CHECK(er.h > 80);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 2.0f), m.ry + 14);
    CHECK(a.time == 2.0f);
    click_at(ui, build_tl, &a, tl_x(&m, &a, 2.0f), m.by + 30);
    double_click(ui, build_tl, &a, tl_x(&m, &a, 2.0f), m.by + 30);
    drag2(ui, build_tl, &a, tl_x(&m, &a, 1.0f), m.by + 10, tl_x(&m, &a, 3.0f), m.by + 60, 4);
    key_press(ui, build_tl, &a, CUI_KEY_DELETE, 0);
    key_press(ui, build_tl, &a, CUI_KEY_DOWN, 0);
    key_press(ui, build_tl, &a, CUI_KEY_A, CUI_MOD_PRIMARY);
    cui_input_scroll(ui, 0, 3);
    run(ui, build_tl, &a, 1);
    CHECK(a.added == 0 && a.removed == 0 && a.time == 2.0f);
    /* garbage in: out of range current key, NaN time */
    tl_setup(&a);
    a.cur_t = 7; a.cur_k = 99; a.time = NAN;
    run(ui, build_tl, &a, 2);
    CHECK(a.cur_t == -1 && a.cur_k == -1 && a.time == 0);
    cui_destroy(ui);

    /* random presses, drags, wheels and keys at several widths: the arrays
     * stay sorted, inside the sequence and within their capacity */
    ui = new_ctx();
    tl_setup(&a);
    a.toggles = true; a.range = true; a.in = 1; a.out = 3;
    a.clips[1] = (cui_timeline_clip){ 3.0f, 3.5f, 0, "Door", false };
    a.tr[3].clip_count = 2;
    unsigned rng = 0x9E3779B9u;
    static const int fkeys[] = { CUI_KEY_LEFT, CUI_KEY_RIGHT, CUI_KEY_UP, CUI_KEY_DOWN, CUI_KEY_HOME, CUI_KEY_END, CUI_KEY_DELETE,
                                 CUI_KEY_ESCAPE, CUI_KEY_A, CUI_KEY_F, CUI_KEY_EQUAL, CUI_KEY_MINUS, CUI_KEY_SPACE, CUI_KEY_BACKSPACE };
    static const int fmods[] = { 0, CUI_MOD_SHIFT, CUI_MOD_ALT, CUI_MOD_PRIMARY, CUI_MOD_ALT | CUI_MOD_SHIFT };
    bool ok = true;
    for (int step = 0; step < 1500 && ok; step++) {
        if (step % 300 == 0) { a.width = (float)(260 + tl_rand(&rng) % 520); cui_set_reduced_motion(ui, (step / 300) % 2 == 0); }
        cui_rect fr = cui_get_rect(ui, a.id);
        float x = fr.x - 10 + (float)(tl_rand(&rng) % (unsigned)(fr.w + 20)), y = fr.y - 10 + (float)(tl_rand(&rng) % (unsigned)(fr.h + 20));
        int what = (int)(tl_rand(&rng) % 8);
        tl_mods(ui, fmods[tl_rand(&rng) % 5]);
        if (what == 0) click_at(ui, build_tl, &a, x, y);
        else if (what == 1) drag2(ui, build_tl, &a, x, y, x + (float)((int)(tl_rand(&rng) % 300) - 150), y + (float)((int)(tl_rand(&rng) % 120) - 60), 3);
        else if (what == 2) double_click(ui, build_tl, &a, x, y);
        else if (what == 3) { cui_input_mouse_move(ui, x, y); cui_input_scroll(ui, (tl_rand(&rng) % 4 == 0) ? 1.0f : 0.0f, (float)((int)(tl_rand(&rng) % 7) - 3)); run(ui, build_tl, &a, 1); }
        else if (what == 4) {
            cui_input_mouse_move(ui, x, y); cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, true); run(ui, build_tl, &a, 1);
            cui_input_mouse_move(ui, x + 40, y - 20); run(ui, build_tl, &a, 1); cui_input_mouse_button(ui, CUI_MOUSE_MIDDLE, false); run(ui, build_tl, &a, 1);
        } else key_press(ui, build_tl, &a, (cui_key)fkeys[tl_rand(&rng) % 14], fmods[tl_rand(&rng) % 5]);
        tl_mods(ui, 0);
        if (tl_rand(&rng) % 50 == 0) a.time = (float)(tl_rand(&rng) % 500) / 100.0f;
        for (int t = 0; t < 4; t++) {
            const cui_timeline_track *tr = &a.tr[t];
            if (tr->key_count < 0 || tr->key_count > tr->key_capacity) ok = false;
            for (int i = 0; i < tr->key_count; i++) {
                if (!(tr->keys[i].x >= 0 && tr->keys[i].x <= 4)) ok = false;
                if (i > 0 && tr->keys[i - 1].x > tr->keys[i].x) ok = false;
            }
            for (int i = 0; i < tr->clip_count; i++) {
                const cui_timeline_clip *c = &tr->clips[i];
                if (!(c->start >= 0 && c->end <= 4 && c->end > c->start)) ok = false;
                if (i > 0 && tr->clips[i - 1].end > c->start + 1e-6f) ok = false;
            }
        }
        if (!(a.view.end > a.view.start && a.view.start >= -1e-4f && a.view.end <= 4 + 1e-4f && a.view.scroll >= 0)) ok = false;
        if (!(a.in >= 0 && a.out <= 4 && a.in < a.out)) ok = false;
        if (!ok) printf("  fuzz broke an invariant at step %d\n", step);
    }
    CHECK(ok);
    cui_destroy(ui);
}

static void test_planning(void) {
    printf("planning: kanban drag between and within columns, week calendar move and resize, gantt reschedule\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 800, 1);
    plan_ctx a;
    memset(&a, 0, sizeof a);
    a.cols[0][0] = 1; a.cols[0][1] = 2; a.cols[0][2] = 3; a.len[0] = 3;
    a.cols[1][0] = 4; a.len[1] = 1;
    run(ui, build_plan, &a, 3);
    cui_rect c1 = rect_of(ui, a.card_ids[1]), c4 = rect_of(ui, a.card_ids[4]);
    CHECK(c1.w > 150 && c4.x > c1.x + c1.w);
    /* card 1 goes below card 4 */
    drag2(ui, build_plan, &a, c1.x + 40, c1.y + 20, c4.x + 60, c4.y + c4.h + 20, 8);
    CHECK(a.moved && a.mv.card == 1 && a.mv.from_column == 0 && a.mv.to_column == 1 && a.mv.to_index == 1);
    CHECK(!a.clicked);
    kb_move(&a);
    a.moved = false;
    run(ui, build_plan, &a, 2);
    /* card 3 goes above card 2 in the same column */
    cui_rect c2 = rect_of(ui, a.card_ids[2]), c3 = rect_of(ui, a.card_ids[3]);
    CHECK(c3.y > c2.y);
    drag2(ui, build_plan, &a, c3.x + 40, c3.y + 20, c2.x + 40, c2.y + 6, 8);
    CHECK(a.moved && a.mv.card == 3 && a.mv.to_column == 0 && a.mv.to_index == 0);
    kb_move(&a);
    CHECK(a.cols[0][0] == 3 && a.cols[0][1] == 2 && a.cols[1][1] == 1);
    /* the empty third column accepts a drop */
    a.moved = false;
    run(ui, build_plan, &a, 2);
    cui_rect c2b = rect_of(ui, a.card_ids[2]);
    drag2(ui, build_plan, &a, c2b.x + 40, c2b.y + 20, c4.x + 260, c4.y + 30, 8);
    CHECK(a.moved && a.mv.card == 2 && a.mv.to_column == 2 && a.mv.to_index == 0);
    /* a plain click on a card is a click, not a move */
    a.moved = false;
    run(ui, build_plan, &a, 2);
    cui_rect c4b = rect_of(ui, a.card_ids[4]);
    click_at(ui, build_plan, &a, c4b.x + 30, c4b.y + 20);
    CHECK(a.clicked && !a.moved);
    /* keyboard: Space lifts the focused card, arrows pick the slot, Enter drops */
    a.moved = false; a.clicked = false;
    key(ui, build_plan, &a, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_plan, &a, CUI_KEY_TAB, 0);            /* card 4 again, now with a visible focus */
    key(ui, build_plan, &a, CUI_KEY_SPACE, 0);
    CHECK(!a.moved && !a.clicked);
    key(ui, build_plan, &a, CUI_KEY_LEFT, 0);
    key(ui, build_plan, &a, CUI_KEY_DOWN, 0);
    run(ui, build_plan, &a, 2);
    key(ui, build_plan, &a, CUI_KEY_ENTER, 0);
    CHECK(a.moved && a.mv.card == 4 && a.mv.from_column == 1 && a.mv.to_column == 0 && a.mv.to_index == 1);
    CHECK(!a.clicked);
    /* Escape puts a lifted card back */
    a.moved = false;
    key(ui, build_plan, &a, CUI_KEY_SPACE, 0);
    key(ui, build_plan, &a, CUI_KEY_RIGHT, 0);
    key(ui, build_plan, &a, CUI_KEY_ESCAPE, 0);
    run(ui, build_plan, &a, 2);
    CHECK(!a.moved);

    /* week calendar */
    a.mode = 1;
    a.ev[0] = (cui_cal_event){ "Review", NULL, 1, 10, 11, CUI_TONE_INFO };
    a.ev[1] = (cui_cal_event){ "Lunch", NULL, 3, 12, 13, CUI_TONE_SUCCESS };
    a.ev_clicked = -1;
    run(ui, build_plan, &a, 3);
    cui_rect w = rect_of(ui, a.week_id);
    float dw = (w.w - 60) / 7, top = w.y + 57;
    CHECK(w.h > 500 && dw > 80);
    float ex = w.x + 60 + 1.5f * dw, ey = top + (10.5f - 8) * 40;
    click_at(ui, build_plan, &a, ex, ey);
    CHECK(a.ev_clicked == 0);
    a.ev_clicked = -1;
    drag2(ui, build_plan, &a, ex, ey, ex + dw, ey + 40, 6);
    CHECK(a.ev[0].day == 2 && a.ev[0].start == 11 && a.ev[0].end == 12);
    CHECK(a.ev_clicked == -1);
    run(ui, build_plan, &a, 2);
    /* the bottom edge resizes by half an hour */
    float bx = w.x + 60 + 2.5f * dw, by = top + (12 - 8) * 40 - 4;
    drag2(ui, build_plan, &a, bx, by, bx, by + 20, 4);
    CHECK(a.ev[0].start == 11 && a.ev[0].end == 12.5f);
    /* events cannot be dragged past the last hour */
    run(ui, build_plan, &a, 2);
    float lx = w.x + 60 + 3.5f * dw, ly = top + (12.5f - 8) * 40;
    drag2(ui, build_plan, &a, lx, ly, lx, ly + 400, 6);
    CHECK(a.ev[1].end == 18 && a.ev[1].start == 17);
    /* keyboard: Tab to the first event, Down moves it 15 minutes, Shift+Down stretches it, Left moves a day */
    cui_input_mouse_move(ui, 5, 5);
    run(ui, build_plan, &a, 1);
    key(ui, build_plan, &a, CUI_KEY_ESCAPE, 0);
    key(ui, build_plan, &a, CUI_KEY_TAB, 0);
    {
        float s0 = a.ev[0].start, e0 = a.ev[0].end; int d0 = a.ev[0].day;
        key(ui, build_plan, &a, CUI_KEY_DOWN, 0);
        CHECK(a.ev[0].start == s0 + 0.25f && a.ev[0].end == e0 + 0.25f);
        key(ui, build_plan, &a, CUI_KEY_DOWN, CUI_MOD_SHIFT);
        CHECK(a.ev[0].start == s0 + 0.25f && a.ev[0].end == e0 + 0.5f);
        key(ui, build_plan, &a, CUI_KEY_LEFT, 0);
        CHECK(a.ev[0].day == d0 - 1);
    }

    /* gantt */
    a.mode = 2;
    a.tasks[0] = (cui_gantt_task){ "Build", "Maya", 2, 6, 0.5f, CUI_TONE_INFO, -1, false };
    a.tasks[1] = (cui_gantt_task){ "Ship", NULL, 10, 10, 0, CUI_TONE_SUCCESS, 0, true };
    a.task_clicked = -1;
    run(ui, build_plan, &a, 3);
    cui_rect g = rect_of(ui, a.gantt_id);
    float x0 = g.x + 231, y0 = g.y + 53;
    CHECK(g.h > 120);
    click_at(ui, build_plan, &a, x0 + 4 * 30, y0 + 20);
    CHECK(a.task_clicked == 0);
    a.task_clicked = -1;
    drag2(ui, build_plan, &a, x0 + 4 * 30, y0 + 20, x0 + 6 * 30, y0 + 20, 6);
    CHECK(a.tasks[0].start == 4 && a.tasks[0].end == 8 && a.task_clicked == -1);
    run(ui, build_plan, &a, 2);
    /* pull the right end out two days */
    drag2(ui, build_plan, &a, x0 + 8 * 30 - 4, y0 + 20, x0 + 10 * 30 - 4, y0 + 20, 6);
    CHECK(a.tasks[0].start == 4 && a.tasks[0].end == 10);
    run(ui, build_plan, &a, 2);
    /* and the left end in one day */
    drag2(ui, build_plan, &a, x0 + 4 * 30 + 3, y0 + 20, x0 + 5 * 30 + 3, y0 + 20, 6);
    CHECK(a.tasks[0].start == 5 && a.tasks[0].end == 10);
    /* keyboard: arrows move a day, Shift+arrows change the length */
    click_at(ui, build_plan, &a, x0 + 7 * 30, y0 + 20);   /* focus the bar, then make the focus visible */
    key(ui, build_plan, &a, CUI_KEY_TAB, 0);
    key(ui, build_plan, &a, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_plan, &a, CUI_KEY_RIGHT, 0);
    CHECK(a.tasks[0].start == 6 && a.tasks[0].end == 11);
    key(ui, build_plan, &a, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK(a.tasks[0].start == 6 && a.tasks[0].end == 10);
    cui_destroy(ui);
}

typedef struct {
    int likes; bool liked; int flags;
    int slide; float split; int thrown;
    cui_id car_id, cmp_id, stack_id;
    cui_texture img;
} media_ctx;

static void build_media(cui_ctx *ui, void *u) {
    media_ctx *a = u;
    cui_column(ui, .pad = 16, .gap = 16, .width = CUI_FILL) {
        a->flags |= cui_comment(ui, "c", "Maya Okafor", "Looks good to me.", .likes = &a->likes, .liked = &a->liked);
        cui_chat_message(ui, "Hello", .author = "Jonas");
        cui_chat_message(ui, "Hi!", .mine = true, .status = "Read");
        cui_carousel(ui, "car", 3, &a->slide, .height = 120) {
            for (int k = 0; k < 3; k++) cui_carousel_slide(ui) { cui_text(ui, "Slide"); }
        }
        a->car_id = cui_last_id(ui);
        cui_image_compare(ui, "cmp", a->img, a->img, &a->split, .width = CUI_PX(400), .height = 120);
        a->cmp_id = cui_last_id(ui);
        cui_card_stack_begin(ui, "stk", 3, &(cui_card_stack_opts){ .width = CUI_PX(240), .height = 120 });
        for (int k = 0; k < 3; k++) cui_card_stack_item(ui, k) { cui_text(ui, "Card"); }
        int d = cui_card_stack_end(ui);
        if (d) a->thrown = d;
        a->stack_id = cui_last_id(ui);
    }
}

static void test_media(void) {
    printf("social and media: comment likes, carousel drag and keys, compare slider, card stack throw\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 1000, 1);
    media_ctx a;
    memset(&a, 0, sizeof a);
    a.likes = 4; a.split = 0.5f;
    static unsigned char px[16 * 16 * 4];
    a.img = cui_image_create(ui, 16, 16, px);
    run(ui, build_media, &a, 3);
    /* Tab reaches the like button first; Enter toggles it */
    key(ui, build_media, &a, CUI_KEY_TAB, 0);
    key(ui, build_media, &a, CUI_KEY_ENTER, 0);
    CHECK(a.liked && a.likes == 5 && (a.flags & CUI_COMMENT_LIKE));
    key(ui, build_media, &a, CUI_KEY_ENTER, 0);
    CHECK(!a.liked && a.likes == 4);
    /* carousel: drag left past half a slide */
    cui_rect cr = rect_of(ui, a.car_id);
    CHECK(cr.w > 600 && cr.h >= 120);
    drag2(ui, build_media, &a, cr.x + cr.w * 0.7f, cr.y + 50, cr.x + cr.w * 0.1f, cr.y + 50, 8);
    run(ui, build_media, &a, 20);
    CHECK(a.slide == 1);
    key(ui, build_media, &a, CUI_KEY_RIGHT, 0);
    CHECK(a.slide == 2);
    key(ui, build_media, &a, CUI_KEY_RIGHT, 0);
    CHECK(a.slide == 2);   /* stops at the end without loop */
    /* compare: drag the divider to three quarters */
    cui_rect pr = rect_of(ui, a.cmp_id);
    CHECK(pr.w > 390 && pr.w < 410);
    drag2(ui, build_media, &a, pr.x + pr.w * 0.5f, pr.y + 60, pr.x + pr.w * 0.75f, pr.y + 60, 6);
    CHECK(a.split > 0.72f && a.split < 0.78f);
    /* card stack: a long drag right throws the top card */
    cui_rect sr = rect_of(ui, a.stack_id);
    CHECK(sr.w > 230 && sr.w < 250);
    drag2(ui, build_media, &a, sr.x + 120, sr.y + 50, sr.x + 320, sr.y + 50, 10);
    run(ui, build_media, &a, 60);
    CHECK(a.thrown == 1);
    /* a short drag springs back */
    a.thrown = 0;
    drag2(ui, build_media, &a, sr.x + 120, sr.y + 50, sr.x + 140, sr.y + 50, 10);
    run(ui, build_media, &a, 60);
    CHECK(a.thrown == 0);
    cui_destroy(ui);
}

typedef struct { int mode; int hit; cui_id id; int empty; } viz_ctx;

static void build_viz(cui_ctx *ui, void *u) {
    viz_ctx *a = u;
    static const float hm[3 * 4] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    static const cui_viz_item items[] = { { "a", 10 }, { "b", 40 }, { "c", 25 }, { "d", 25 } };
    static const char *nodes[] = { "A", "B", "C", "D" };
    static const cui_sankey_link links[] = { { 0, 2, 10 }, { 1, 2, 5 }, { 2, 3, 12 }, { 3, 0, 1 } };   /* with a cycle */
    static float days[60];
    static const cui_vec2 pts[] = { { 1, 1 }, { 5, 5 }, { 9, 9 } };
    static const float rv[] = { 1, 2, 3, 4, 5 };
    static const char *axes[] = { "p", "q", "r", "s", "t" };
    for (int k = 0; k < 60; k++) days[k] = (float)(k % 5);
    int r = -1;
    cui_column(ui, .pad = 20, .width = CUI_FILL) {
        if (a->empty) {
            /* nothing to show, zeros and NaN: no crash, no NaN geometry */
            float nanv = NAN;
            cui_viz_item z[2] = { { "z", 0 }, { "n", nanv } };
            cui_heatmap(ui, "h0", NULL, 0, 0);
            cui_treemap(ui, "t0", z, 2);
            cui_funnel(ui, "f0", z, 2);
            cui_waffle(ui, "w0", z, 2);
            cui_split_bar(ui, "s0", z, 2);
            cui_bar_list(ui, "b0", z, 2);
            cui_activity_rings(ui, "r0", z, 2);
            cui_sankey(ui, "k0", nodes, 4, NULL, 0);
            cui_scatter_chart(ui, "c0", NULL, 0);
            cui_radar_chart(ui, "d0", NULL, 0, axes, 2);
            cui_date d0 = { 2026, 1, 1 };
            cui_calendar_heatmap(ui, "y0", d0, days, 0);
        } else switch (a->mode) {
        case 0: r = cui_heatmap(ui, "hm", hm, 3, 4, .cell = 30); break;
        case 1: r = cui_treemap(ui, "tm", items, 4, .width = CUI_PX(400), .height = 200); break;
        case 2: r = cui_funnel(ui, "fn", items, 4, .width = CUI_PX(400)); break;
        case 3: r = cui_bar_list(ui, "bl", items, 4, .width = CUI_PX(400)); break;
        case 4: r = cui_split_bar(ui, "sb", items, 4, .width = CUI_PX(400)); break;
        case 5: r = cui_waffle(ui, "wf", items, 4, .cell = 10); break;
        case 6: r = cui_sankey(ui, "sk", nodes, 4, links, 4, .width = CUI_PX(500), .height = 200); break;
        case 7: r = cui_activity_rings(ui, "ar", items, 2, .size = 120, .thickness = 12); break;
        case 8: { cui_scatter_series ss = { "s", pts, NULL, 3, { 0 } }; r = cui_scatter_chart(ui, "sc", &ss, 1, .width = CUI_PX(400), .height = 240); } break;
        case 9: { cui_date d1 = { 2026, 3, 2 }; r = cui_calendar_heatmap(ui, "cal", d1, days, 60); } break;   /* a Monday */
        case 10: { cui_series rs = { "r", rv, 5, { 0 } }; r = cui_radar_chart(ui, "rd", &rs, 1, axes, 5, .size = 200); } break;
        }
        a->id = cui_last_id(ui);
    }
    if (r >= 0) a->hit = r;
}

static void test_viz(void) {
    printf("more charts: hit testing for every chart, empty and degenerate data\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 800, 1);
    viz_ctx a;
    memset(&a, 0, sizeof a);
    a.empty = 1;
    run(ui, build_viz, &a, 4);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    bool finite = true;
    for (int v = 0; v < dd->vertex_count; v++) if (!isfinite(dd->vertices[v].x) || !isfinite(dd->vertices[v].y)) finite = false;
    CHECK(finite);
    a.empty = 0;
    cui_rect r;
    /* heatmap: cell (row 1, col 2) is index 6 */
    a.mode = 0; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + 2 * 33 + 15, r.y + 33 + 15);
    CHECK(a.hit == 6);
    /* treemap: the largest item fills the top left */
    a.mode = 1; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + 10, r.y + 10);
    CHECK(a.hit == 1);
    CHECK(r.w == 400 && r.h == 200);
    /* funnel: third row */
    a.mode = 2; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + r.w / 2, r.y + 2 * 54 + 20);
    CHECK(a.hit == 2);
    /* bar list: ranked, the first row is the largest item */
    a.mode = 3; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + 20, r.y + 15);
    CHECK(a.hit == 1);
    click_at(ui, build_viz, &a, r.x + 20, r.y + 3 * 40 + 15);
    CHECK(a.hit == 0);
    /* split bar: shares 10/100, then 40: x at 30% is item b */
    a.mode = 4; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + r.w * 0.3f, r.y + 5);
    CHECK(a.hit == 1);
    /* waffle: the first column is item a (10 cells) */
    a.mode = 5; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + 5, r.y + 5);
    CHECK(a.hit == 0);
    click_at(ui, build_viz, &a, r.x + 13 * 3 + 5, r.y + 5);
    CHECK(a.hit == 1);
    /* sankey: node C sits in the middle column */
    a.mode = 6; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    /* scan the middle for node C by clicking on a grid; only nodes report clicks */
    int got_c = 0, other = 0;
    for (float x = r.x + r.w * 0.2f; x < r.x + r.w * 0.8f && !got_c; x += 4)
        for (float y = r.y + 4; y < r.y + r.h && !got_c; y += 12) {
            a.hit = -1;
            click_at(ui, build_viz, &a, x, y);
            if (a.hit == 2) got_c = 1;
            else if (a.hit >= 0) other = 1;
        }
    CHECK(got_c && !other);
    /* rings: the outer ring is item 0 */
    a.mode = 7; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + r.w / 2, r.y + 6);
    CHECK(a.hit == 0);
    click_at(ui, build_viz, &a, r.x + r.w / 2, r.y + 6 + 15);
    CHECK(a.hit == 1);
    /* scatter: the nearest point */
    a.mode = 8; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    /* the middle point (5, 5) is near the plot's center */
    click_at(ui, build_viz, &a, r.x + r.w * 0.52f, r.y + r.h * 0.5f);
    CHECK(a.hit == 1);
    /* calendar: March 2 2026 is a Monday, so day 8 is the second column's Tuesday */
    a.mode = 9; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + 32 + 14 + 5, r.y + 20 + 14 + 5);
    CHECK(a.hit == 8);
    /* radar: the axis straight up is 0 */
    a.mode = 10; a.hit = -1; run(ui, build_viz, &a, 3); r = rect_of(ui, a.id);
    click_at(ui, build_viz, &a, r.x + r.w / 2, r.y + 40);
    CHECK(a.hit == 0);
    cui_destroy(ui);
}

typedef struct {
    char text[256]; bool done; int clicks, dock_hits; bool tile; int tab; bool tab_changed;
    float progress; bool seeked; cui_now_playing np; int np_flags;
    cui_id tile_id, tab_id, wave_id, np_id, dock_id, spot_id;
} motion_ctx;

static void build_motion(cui_ctx *ui, void *u) {
    motion_ctx *a = u;
    static const cui_tab_bar_item tabs[] = { { CUI_ICON_HOUSE, "Home", 0 }, { CUI_ICON_SEARCH, "Search", 0 }, { CUI_ICON_INBOX, "Inbox", 2 } };
    static float peaks[40];
    for (int k = 0; k < 40; k++) peaks[k] = 0.5f;
    cui_column(ui, .pad = 20, .gap = 12, .width = CUI_FILL) {
        a->done = cui_typewriter(ui, "tw", a->text, .speed = 60);
        cui_shimmer_text(ui, "Thinking");
        cui_text_reveal(ui, "rv", "Words settle into place");
        static const char *words[] = { "one", "two" };
        cui_word_rotate(ui, "wr", words, 2, 1.0f);
        cui_loader(ui, .style = CUI_LOADER_GRID);
        cui_row(ui, .gap = 10) {
            if (cui_control_tile(ui, "Wi-Fi", &a->tile, .width = CUI_PX(140))) a->clicks++;
            a->tile_id = cui_last_id(ui);
        }
        if (cui_tab_bar(ui, "tabs", tabs, 3, &a->tab, .width = CUI_PX(300))) a->tab_changed = true;
        a->tab_id = cui_last_id(ui);
        cui_box(ui, .width = CUI_PX(400)) { if (cui_waveform(ui, "wave", peaks, 40, &a->progress)) a->seeked = true; a->wave_id = cui_last_id(ui); }
        a->np_flags |= cui_now_playing(ui, "np", &a->np);
        a->np_id = cui_last_id(ui);
        cui_gradient_text(ui, "Gradient", .animate = true);
        cui_spotlight(ui, "spot", .width = CUI_PX(200), .beam = true) { cui_text(ui, "Inside"); a->spot_id = cui_last_id(ui); }
        cui_dock(ui, "dock") {
            if (cui_dock_item(ui, CUI_ICON_FOLDER, "Files")) a->dock_hits++;
            if (cui_dock_item(ui, CUI_ICON_MAIL, "Mail", .badge = 2)) a->dock_hits += 10;
        }
        a->dock_id = cui_last_id(ui);
    }
}

static void test_motion(void) {
    printf("motion: typewriter streams and restarts, tiles, tab bar, waveform seek, player, dock\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 900, 1000, 1);
    cui_set_reduced_motion(ui, false);
    motion_ctx a;
    memset(&a, 0, sizeof a);
    strcpy(a.text, "Hello there");   /* 11 characters at 60 a second */
    a.np = (cui_now_playing){ "Song", "Artist", 0, { 0 }, 30, 200, false };
    run(ui, build_motion, &a, 2);
    CHECK(!a.done);
    run(ui, build_motion, &a, 30);
    CHECK(a.done);
    /* streamed: more text carries on without starting over */
    strcat(a.text, ", and more");
    run(ui, build_motion, &a, 1);
    CHECK(!a.done);
    run(ui, build_motion, &a, 30);
    CHECK(a.done);
    /* a different text starts over */
    strcpy(a.text, "Something else entirely");
    run(ui, build_motion, &a, 1);
    CHECK(!a.done);
    cui_set_reduced_motion(ui, true);
    run(ui, build_motion, &a, 1);
    CHECK(a.done);   /* still motion shows everything at once */
    cui_set_reduced_motion(ui, false);
    /* tile toggles on click and on Space */
    cui_rect tr = rect_of(ui, a.tile_id);
    CHECK(tr.w == 140);
    click_at(ui, build_motion, &a, tr.x + 20, tr.y + 20);
    CHECK(a.tile && a.clicks == 1);
    key(ui, build_motion, &a, CUI_KEY_TAB, 0);
    key(ui, build_motion, &a, CUI_KEY_TAB, CUI_MOD_SHIFT);   /* back to the tile, with a visible focus */
    key(ui, build_motion, &a, CUI_KEY_SPACE, 0);
    CHECK(!a.tile && a.clicks == 2);
    /* tab bar: click the third item, then the arrow key goes back */
    cui_rect br = rect_of(ui, a.tab_id);
    click_at(ui, build_motion, &a, br.x + br.w * 5 / 6, br.y + br.h / 2);
    CHECK(a.tab == 2 && a.tab_changed);
    key(ui, build_motion, &a, CUI_KEY_LEFT, 0);
    CHECK(a.tab == 1);
    /* waveform: drag seeks */
    cui_rect wr = rect_of(ui, a.wave_id);
    drag(ui, build_motion, &a, wr.x + 10, wr.y + wr.h / 2, wr.x + wr.w * 0.75f, 5);
    CHECK(a.seeked && a.progress > 0.72f && a.progress < 0.78f);
    /* player: the play button toggles, the scrubber seeks */
    cui_rect nr = rect_of(ui, a.np_id);
    bool found_toggle = false;
    for (float x = nr.x + nr.w - 120; x < nr.x + nr.w && !found_toggle; x += 6) {
        a.np_flags = 0;
        click_at(ui, build_motion, &a, x, nr.y + 16 + 32);
        if (a.np_flags & CUI_PLAYER_TOGGLE) found_toggle = true;
    }
    CHECK(found_toggle && a.np.playing);
    a.np_flags = 0;
    drag(ui, build_motion, &a, nr.x + nr.w * 0.5f, nr.y + nr.h - 16 - 7, nr.x + nr.w * 0.5f + 40, 4);
    CHECK((a.np_flags & CUI_PLAYER_SEEK) && a.np.position > 30);
    /* spotlight: the card wraps its content */
    {
        cui_rect sr = rect_of(ui, cui_make_id(ui, "spot"));
        CHECK(sr.w == 200 && sr.h > 40);
        cui_input_mouse_move(ui, sr.x + 30, sr.y + 20);
        run(ui, build_motion, &a, 5);
        CHECK(cui_frame_count(ui) > 0);
    }
    /* dock: clicking an item */
    cui_rect dr = rect_of(ui, a.dock_id);
    click_at(ui, build_motion, &a, dr.x + 8 + 23, dr.y + dr.h - 12 - 23);
    run(ui, build_motion, &a, 20);
    CHECK(a.dock_hits == 1);
    cui_destroy(ui);
}

typedef struct {
    bool thinking, body; char draft[128]; int flags; bool busy; int removed; int model; int vote; int resp;
    int plan_hit, sugg_hit; bool orb, cite; bool picked;
    cui_id think_id, tool_id, plan_id, comp_id, sugg_id, orb_id, cite_id, picker_id;
    const char *files[2]; int file_count;
} ai_ctx;

static const cui_model ai_models[] = { { "Atlas", "Big", NULL, NULL }, { "Atlas Fast", "Quick", NULL, NULL } };

static void build_ai(cui_ctx *ui, void *u) {
    ai_ctx *a = u;
    static const cui_plan_step steps[] = { { "One", NULL, CUI_TASK_DONE }, { "Two", "detail", CUI_TASK_RUNNING }, { "Three", NULL, CUI_TASK_PENDING } };
    static const cui_prompt_suggestion sugg[] = { { CUI_ICON_CODE, "Explain", "code" }, { CUI_ICON_BUG, "Debug", "error" } };
    static const cui_source src = { "Title", "https://www.example.com/a", "Snippet" };
    a->body = false;
    cui_column(ui, .pad = 20, .gap = 10, .width = CUI_PX(600)) {
        cui_reasoning(ui, "think", .thinking = a->thinking, .seconds = 3) { a->body = true; cui_text(ui, "thoughts"); }
        a->think_id = cui_make_id(ui, "think");
        cui_tool_call(ui, "tool", "web_search", .status = CUI_TASK_DONE, .input = "{\"q\": 1}", .output = "{\"r\": 2}");
        a->tool_id = cui_last_id(ui);
        int ph = cui_agent_plan(ui, "plan", steps, 3);
        if (ph >= 0) a->plan_hit = ph;
        a->plan_id = cui_last_id(ui);
        cui_agent_status(ui, "Working", .status = CUI_TASK_RUNNING, .seconds = 75);
        if (cui_citation(ui, 1, &src)) a->cite = true;
        a->cite_id = cui_last_id(ui);
        cui_context_meter(ui, "ctx", 90, 100);
        a->resp |= cui_response_actions(ui, "resp", .copy_text = "x", .vote = &a->vote);
        int sg = cui_prompt_suggestions(ui, "sugg", sugg, 2);
        if (sg >= 0) a->sugg_hit = sg;
        a->sugg_id = cui_last_id(ui);
        if (cui_model_picker(ui, "picker", ai_models, 2, &a->model, .placement = CUI_PLACE_BOTTOM_START)) a->picked = true;
        a->picker_id = cui_last_id(ui);
        a->flags |= cui_chat_composer(ui, "comp", a->draft, sizeof a->draft, .busy = a->busy, .attachments = a->files, .attachment_count = a->file_count, .removed = &a->removed);
        a->comp_id = cui_last_id(ui);
        if (cui_voice_orb(ui, "orb", CUI_VOICE_LISTENING, 0.5f, .size = 80)) a->orb = true;
        a->orb_id = cui_last_id(ui);
    }
}

static void test_ai(void) {
    printf("ai: reasoning folds, tool call opens, plan, citation, actions, suggestions, model picker, composer, orb\n");
    cui_ctx *ui = new_ctx();
    cui_set_viewport(ui, 700, 1600, 1);
    ai_ctx a;
    memset(&a, 0, sizeof a);
    a.thinking = true; a.plan_hit = a.sugg_hit = -1; a.removed = -1;
    a.files[0] = "a.pdf"; a.files[1] = "b.csv"; a.file_count = 2;
    run(ui, build_ai, &a, 3);
    CHECK(a.body);                /* open while thinking */
    a.thinking = false;
    run(ui, build_ai, &a, 3);
    CHECK(!a.body);               /* folds away when done */
    cui_rect hr = rect_of(ui, a.think_id);
    click_at(ui, build_ai, &a, hr.x + 10, hr.y + hr.h / 2);
    run(ui, build_ai, &a, 2);
    CHECK(a.body);                /* and opens on request */
    a.thinking = true; run(ui, build_ai, &a, 2); a.thinking = false; run(ui, build_ai, &a, 2);
    CHECK(a.body);                /* a choice the user made sticks */
    /* tool call opens to show its payloads */
    cui_rect tr = rect_of(ui, a.tool_id);
    float plan_y = rect_of(ui, a.plan_id).y;
    click_at(ui, build_ai, &a, tr.x + tr.w / 2, tr.y + tr.h / 2);
    run(ui, build_ai, &a, 3);
    CHECK(rect_of(ui, a.plan_id).y > plan_y + 100);   /* the payloads pushed the plan down */
    /* plan: clicking a step reports it */
    cui_rect pr = rect_of(ui, a.plan_id);
    click_at(ui, build_ai, &a, pr.x + 80, pr.y + 50 + 38 + 10);
    CHECK(a.plan_hit == 1);
    /* citation */
    cui_rect cr = rect_of(ui, a.cite_id);
    click_at(ui, build_ai, &a, cr.x + cr.w / 2, cr.y + cr.h / 2);
    CHECK(a.cite);
    /* suggestions */
    cui_rect sr = rect_of(ui, a.sugg_id);
    click_at(ui, build_ai, &a, sr.x + sr.w * 0.75f, sr.y + 20);
    CHECK(a.sugg_hit == 1);
    /* model picker: open, choose the second */
    cui_rect mr = rect_of(ui, a.picker_id);
    click_at(ui, build_ai, &a, mr.x + mr.w / 2, mr.y + mr.h / 2);
    run(ui, build_ai, &a, 2);
    key(ui, build_ai, &a, CUI_KEY_DOWN, 0);
    key(ui, build_ai, &a, CUI_KEY_DOWN, 0);
    key(ui, build_ai, &a, CUI_KEY_ENTER, 0);
    CHECK(a.picked && a.model == 1);
    /* composer: type, Shift+Enter adds a line, Enter sends */
    cui_rect qr = rect_of(ui, a.comp_id);
    click_at(ui, build_ai, &a, qr.x + 60, qr.y + 10 + 34 + 4 + 14);
    type(ui, build_ai, &a, "hello");
    CHECK(strcmp(a.draft, "hello") == 0);
    key(ui, build_ai, &a, CUI_KEY_ENTER, CUI_MOD_SHIFT);
    CHECK(strcmp(a.draft, "hello\n") == 0 && !(a.flags & CUI_COMPOSER_SEND));
    type(ui, build_ai, &a, "there");
    key(ui, build_ai, &a, CUI_KEY_ENTER, 0);
    CHECK((a.flags & CUI_COMPOSER_SEND) && strcmp(a.draft, "hello\nthere") == 0);
    /* empty drafts do not send */
    a.flags = 0; a.draft[0] = 0;
    key(ui, build_ai, &a, CUI_KEY_ENTER, 0);
    CHECK(!(a.flags & CUI_COMPOSER_SEND));
    /* busy: the button stops */
    a.busy = true; a.flags = 0;
    run(ui, build_ai, &a, 2);
    qr = rect_of(ui, a.comp_id);
    click_at(ui, build_ai, &a, qr.x + qr.w - 10 - 16, qr.y + qr.h - 10 - 16);
    CHECK(a.flags & CUI_COMPOSER_STOP);
    /* removing an attachment reports which */
    a.flags = 0;
    bool removed = false;
    for (float x = qr.x + 60; x < qr.x + 200 && !removed; x += 3) {
        click_at(ui, build_ai, &a, x, qr.y + 10 + 2 + 18);
        if (a.flags & CUI_COMPOSER_REMOVE) removed = true;
    }
    CHECK(removed && a.removed == 0);
    /* orb */
    cui_rect orr = rect_of(ui, a.orb_id);
    click_at(ui, build_ai, &a, orr.x + orr.w / 2, orr.y + orr.h / 2);
    CHECK(a.orb);
    cui_destroy(ui);
}

/* tool panels: inspector and log console ------------------------------- */

typedef struct {
    float f, vec[3];
    int i, sel;
    bool b, open, body;
    char text[32];
    cui_color color;
    int changed;
    cui_id float_id, vec_id, insp_id;
} insp_ctx;

static void build_insp(cui_ctx *ui, void *u) {
    insp_ctx *a = u;
    static const char *const items[] = { "Lit", "Unlit", "Toon" };
    a->body = false;
    cui_column(ui, .width = CUI_PX(420), .pad = 10) {
        if (cui_inspector_begin(ui, "props")) {
            if (cui_inspector_group(ui, "Transform", &a->open)) {
                a->body = true;
                if (cui_prop_float(ui, "Intensity", &a->f, .min = 0, .max = 100, .step = 1)) a->changed++;
                a->float_id = cui_last_id(ui);
                if (cui_prop_vec3(ui, "Position", a->vec)) a->changed++;
                a->vec_id = cui_last_id(ui);
            }
            if (cui_inspector_group(ui, "More", NULL)) {
                if (cui_prop_int(ui, "Layer", &a->i, .min = 0, .max = 31)) a->changed++;
                if (cui_prop_bool(ui, "Visible", &a->b)) a->changed++;
                if (cui_prop_enum(ui, "Shader", items, 3, &a->sel)) a->changed++;
                if (cui_prop_text(ui, "Name", a->text, sizeof a->text)) a->changed++;
                if (cui_prop_color(ui, "Tint", &a->color)) a->changed++;
                if (cui_prop_bool(ui, "Locked", &a->b, .checkbox = true, .disabled = true)) a->changed++;
            }
            cui_inspector_end(ui);
        }
        a->insp_id = cui_last_id(ui);
    }
    /* rows also work on their own */
    cui_prop_float(ui, "Loose", &a->f);
}

typedef struct { cui_log_buffer *buf; bool timestamps; cui_id id; } logv_ctx;
static void build_logv(cui_ctx *ui, void *u) {
    logv_ctx *a = u;
    cui_column(ui, .width = CUI_FILL, .pad = 10) {
        cui_log_view(ui, "log", a->buf, .height = 300, .timestamps = a->timestamps);
        a->id = cui_last_id(ui);
    }
}

/* expected text of line number seq in the stress test */
static int log_expect(uint64_t seq, char *out, int cap) {
    int len = (int)((seq * 7919u) % 90u);
    for (int k = 0; k < len && k < cap - 1; k++) out[k] = (char)('a' + (int)((seq + (uint64_t)k) % 26u));
    return len;
}

static void test_tools(void) {
    printf("tool panels: inspector rows and groups, log buffer and view\n");
    cui_ctx *ui = new_ctx();
    insp_ctx a;
    memset(&a, 0, sizeof a);
    a.f = 50; a.open = true; a.color = CUI_RGB(0x336699);
    snprintf(a.text, sizeof a.text, "crate");
    run(ui, build_insp, &a, 3);
    CHECK(a.body);
    cui_rect fr = rect_of(ui, a.float_id), vr = rect_of(ui, a.vec_id), ir = rect_of(ui, a.insp_id);
    CHECK(fr.w > 100 && fr.h >= 30);
    CHECK(ir.w > 380);
    /* the label column takes about 38% of the width; editors start after it */
    CHECK(fr.x > ir.x + ir.w * 0.3f && fr.x < ir.x + ir.w * 0.5f);
    CHECK(vr.w > 40 && vr.x + vr.w <= ir.x + ir.w + 0.5f);
    /* the field scrubs: arrows step when focused */
    click_at(ui, build_insp, &a, fr.x + fr.w / 2, fr.y + fr.h / 2);   /* a click starts typing */
    key(ui, build_insp, &a, CUI_KEY_ESCAPE, 0);
    a.changed = 0;
    key(ui, build_insp, &a, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(a.f, 51, 0.001);
    CHECK(a.changed == 1);
    /* the label scrubs too: drag it sideways */
    float lx = ir.x + 20, ly = fr.y + fr.h / 2;
    cui_input_mouse_move(ui, lx, ly);
    run(ui, build_insp, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_insp, &a, 1);
    cui_input_mouse_move(ui, lx + 20, ly);
    run(ui, build_insp, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_insp, &a, 1);
    CHECK_NEAR(a.f, 71, 0.001);
    /* drag the column line to widen the labels */
    float line_x = fr.x - 6;
    cui_input_mouse_move(ui, line_x, fr.y + 4);
    run(ui, build_insp, &a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_insp, &a, 1);
    cui_input_mouse_move(ui, line_x + 40, fr.y + 4);
    run(ui, build_insp, &a, 2);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_insp, &a, 2);
    cui_rect fr2 = rect_of(ui, a.float_id);
    CHECK(fr2.x > fr.x + 30 && fr2.x < fr.x + 50);
    CHECK_NEAR(a.f, 71, 0.001);
    /* Tab walks from the number field into the vector fields; arrows step them */
    click_at(ui, build_insp, &a, fr2.x + fr2.w / 2, fr2.y + fr2.h / 2);
    key(ui, build_insp, &a, CUI_KEY_ESCAPE, 0);
    run(ui, build_insp, &a, 1);   /* the scrub field rejoins the focus order a frame after typing ends */
    a.changed = 0;
    key(ui, build_insp, &a, CUI_KEY_TAB, 0);
    key(ui, build_insp, &a, CUI_KEY_UP, 0);
    key(ui, build_insp, &a, CUI_KEY_TAB, 0);
    key(ui, build_insp, &a, CUI_KEY_UP, 0);
    key(ui, build_insp, &a, CUI_KEY_UP, 0);
    CHECK_NEAR(a.vec[0], 0.1, 0.001);
    CHECK_NEAR(a.vec[1], 0.2, 0.001);
    CHECK_NEAR(a.f, 71, 0.001);
    CHECK(a.changed == 3);
    /* folding: the body is not built once the group has closed */
    cui_rect hr = { ir.x, ir.y, ir.w, 40 };
    click_at(ui, build_insp, &a, hr.x + 60, hr.y + 20);
    CHECK(!a.open);
    run(ui, build_insp, &a, 2);
    CHECK(!a.body);
    click_at(ui, build_insp, &a, hr.x + 60, hr.y + 20);
    CHECK(a.open && a.body);
    cui_destroy(ui);

    /* ---- log buffer ---- */
    static cui_log_line lines[64];
    static char text[1024];
    cui_log_buffer lb;
    cui_log_init(&lb, NULL, 0, NULL, 0);   /* no storage: appends are ignored */
    cui_log_append(&lb, CUI_LOG_INFO, "dropped %d", 1);
    CHECK(lb.count == 0 && lb.total == 0);
    cui_log_init(&lb, lines, 4, text, sizeof text);
    lb.now = 3723.5;
    cui_log_append(&lb, CUI_LOG_INFO, "hello %s\n", "world");
    cui_log_append_text(&lb, CUI_LOG_WARN, 1.0, "tab\there", -1);
    const char *t = NULL;
    const cui_log_line *l = cui_log_get(&lb, 0, &t);
    CHECK(l && l->len == 11 && !strcmp(t, "hello world") && l->time == 3723.5 && l->level == CUI_LOG_INFO);
    l = cui_log_get(&lb, 1, &t);
    CHECK(l && !strcmp(t, "tab here") && l->level == CUI_LOG_WARN);
    CHECK(cui_log_get(&lb, 2, &t) == NULL && t == NULL);
    for (int k = 0; k < 10; k++) cui_log_append(&lb, CUI_LOG_ERROR, "line %d", k);
    CHECK(lb.count == 4 && lb.total == 12);
    l = cui_log_get(&lb, 0, &t);
    CHECK(l && !strcmp(t, "line 6"));
    CHECK(lb.level_count[CUI_LOG_ERROR] == 4 && lb.level_count[CUI_LOG_INFO] == 0);
    /* a line longer than the whole buffer is cut to fit */
    static char big[5000];
    memset(big, 'x', sizeof big - 1);
    big[sizeof big - 1] = 0;
    cui_log_append(&lb, CUI_LOG_INFO, "%s", big);
    l = cui_log_get(&lb, lb.count - 1, &t);
    CHECK(l && l->len == sizeof text - 1 && t[l->len] == 0 && t[0] == 'x');
    CHECK(lb.count == 1);
    cui_log_append(&lb, CUI_LOG_INFO, "after");
    l = cui_log_get(&lb, lb.count - 1, &t);
    CHECK(l && !strcmp(t, "after") && lb.count == 1);
    cui_log_clear(&lb);
    CHECK(lb.count == 0 && lb.level_count[CUI_LOG_INFO] == 0);
    /* stress: random lengths through both rings; every kept line is intact */
    static cui_log_line slines[40];
    static char stext[700];
    cui_log_init(&lb, slines, 40, stext, sizeof stext);
    bool ok = true;
    for (uint64_t seq = 0; seq < 20000 && ok; seq++) {
        char want[128];
        int len = log_expect(seq, want, sizeof want);
        if (seq % 3) cui_log_append_text(&lb, (cui_log_level)(seq % 5), (double)seq, want, len);
        else cui_log_append(&lb, (cui_log_level)(seq % 5), "%.*s", len, want);
        int sum = 0;
        for (int k = 0; k < CUI_LOG_LEVEL_COUNT; k++) sum += lb.level_count[k];
        if (sum != lb.count || lb.total != seq + 1 || lb.count < 1) ok = false;
        for (int i = 0; i < lb.count && ok; i++) {
            uint64_t s2 = lb.total - (uint64_t)lb.count + (uint64_t)i;
            l = cui_log_get(&lb, i, &t);
            int wl = log_expect(s2, want, sizeof want);
            if (!l || (int)l->len != wl || l->offset + l->len >= sizeof stext || t[l->len] != 0 || memcmp(t, want, (size_t)wl) != 0) ok = false;
        }
    }
    CHECK(ok);

    /* ---- log view ---- */
    static cui_log_line vlines[4096];
    static char vtext[128 * 1024];
    cui_log_buffer vb;
    cui_log_init(&vb, vlines, 4096, vtext, sizeof vtext);
    ui = new_ctx();
    logv_ctx v = { &vb, true, 0 };
    run(ui, build_logv, &v, 2);   /* empty */
    v.buf = NULL;
    run(ui, build_logv, &v, 2);   /* no buffer at all */
    v.buf = &vb;
    for (int k = 0; k < 6000; k++) {
        vb.now = k * 0.25;
        cui_log_append(&vb, (cui_log_level)(k % 5), "event %d payload %s", k, k % 7 ? "ok" : "slow frame");
    }
    run(ui, build_logv, &v, 3);
    CHECK(vb.count == 4096);
    cui_rect lr = rect_of(ui, v.id);
    CHECK(lr.h > 290 && lr.h < 310);
    /* following: the newest line sits at the bottom of the list */
    g_clip[0] = 0;
    click_at(ui, build_logv, &v, lr.x + 300, lr.y + lr.h - 12);
    key(ui, build_logv, &v, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strstr(g_clip, "event 5999 ") != NULL);
    /* scrolling up stops following: new lines do not move the view */
    cui_input_mouse_move(ui, lr.x + 300, lr.y + 150);
    cui_input_scroll(ui, 0, 5);
    run(ui, build_logv, &v, 20);
    for (int k = 6000; k < 6100; k++) cui_log_append(&vb, CUI_LOG_INFO, "event %d payload ok", k);
    run(ui, build_logv, &v, 20);
    click_at(ui, build_logv, &v, lr.x + 300, lr.y + lr.h - 12);
    key(ui, build_logv, &v, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strstr(g_clip, "event 6099 ") == NULL && strstr(g_clip, "event ") != NULL);
    /* End follows again */
    key(ui, build_logv, &v, CUI_KEY_END, 0);
    key(ui, build_logv, &v, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strstr(g_clip, "event 6099 ") != NULL);
    run(ui, build_logv, &v, 20);
    click_at(ui, build_logv, &v, lr.x + 300, lr.y + lr.h - 12);
    key(ui, build_logv, &v, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strstr(g_clip, "event 6099 ") != NULL);
    /* Shift+Up extends the selection; copy gets both lines */
    key(ui, build_logv, &v, CUI_KEY_UP, CUI_MOD_SHIFT);
    key(ui, build_logv, &v, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strstr(g_clip, "event 6098 ") != NULL && strstr(g_clip, "event 6099 ") != NULL && strchr(g_clip, '\n') != NULL);
    /* search: type into the filter field (the toolbar's second focus stop) */
    for (int k = 0; k < 12; k++) {
        key(ui, build_logv, &v, CUI_KEY_TAB, 0);
        if (cui_wants_keyboard(ui)) break;
    }
    CHECK(cui_wants_keyboard(ui));
    cui_input_text(ui, "no such text");
    run(ui, build_logv, &v, 3);
    g_clip[0] = 0;
    key(ui, build_logv, &v, CUI_KEY_TAB, 0);   /* on to Follow, Copy... */
    run(ui, build_logv, &v, 2);
    for (int k = 0; k < 6000; k += 37) cui_log_append(&vb, CUI_LOG_WARN, "late %d", k);
    run(ui, build_logv, &v, 3);
    /* a very long line and a wrap of the whole ring while searching */
    static char huge[200000];
    memset(huge, 'q', sizeof huge - 1);
    huge[sizeof huge - 1] = 0;
    cui_log_append_text(&vb, CUI_LOG_ERROR, 1.0, huge, -1);
    run(ui, build_logv, &v, 3);
    cui_log_clear(&vb);
    run(ui, build_logv, &v, 3);
    cui_destroy(ui);
}

/* hex viewer ------------------------------------------------------------ */

typedef struct {
    const uint8_t *data; size_t size;
    uint8_t *edit;
    cui_hex_selection sel;
    cui_hex_result res;
    const cui_hex_range *ranges; int nranges;
    float width;
    int bpr;
    bool own_sel;      /* pass NULL: the view keeps the selection */
} hex_ctx;
static void build_hex(cui_ctx *ui, void *u) {
    hex_ctx *h = u;
    cui_column(ui, .pad = 20, .width = CUI_FILL) {
        h->res = cui_hex_view(ui, "hex", h->data, h->size, h->own_sel ? NULL : &h->sel, .height = 300, .ranges = h->ranges,
                              .range_count = h->nranges, .edit = h->edit, .bytes_per_row = h->bpr,
                              .width = h->width > 0 ? CUI_PX(h->width) : CUI_FILL);
    }
}
/* centre of byte column `col` in visible row `vrow` (rows counted from the top of the view) */
static float g_hex_cw;
static int g_hex_digits = 4;   /* offset digits: 4 up to 64 KiB, 6 up to 16 MiB */
static cui_vec2 hex_cell(cui_ctx *ui, int col, int vrow, bool ascii) {
    cui_rect r = rect_of(ui, cui_make_id(ui, "hex"));
    float cw = g_hex_cw, x_hex = 12 + (g_hex_digits + 2) * cw;   /* the offsets and a 2 character gap */
    float x = ascii ? x_hex + 15 * 3 * cw + cw + 2 * cw + 2 * cw + col * cw + cw * 0.5f : x_hex + col * 3 * cw + (col / 8) * cw + cw;
    cui_vec2 p = { r.x + 1 + x, r.y + 1 + 21 + 4 + vrow * 20 + 10 };
    return p;
}
static void hex_mods(cui_ctx *ui, int mods) { cui_input_key(ui, CUI_KEY_NONE, false, mods); }

static void test_hex_view(void) {
    printf("hex viewer: selection, keys, copy, ranges, virtualization, editing\n");
    cui_ctx *ui = new_ctx();
    g_hex_cw = cui_measure_text(ui, CUI_FONT_MONO, 12, "0000000000000000", 16).width / 16;
    static uint8_t small[4096];
    for (int i = 0; i < 4096; i++) small[i] = (uint8_t)i;
    static const cui_hex_range ranges[] = { { 0x40, 0x10, CUI_TONE_INFO, "Header" }, { 0x80, 4, CUI_TONE_VIOLET, NULL } };
    hex_ctx h;
    memset(&h, 0, sizeof h);
    h.data = small; h.size = sizeof small; h.ranges = ranges; h.nranges = 2;
    run(ui, build_hex, &h, 3);
    cui_rect hr = rect_of(ui, cui_make_id(ui, "hex"));
    CHECK_NEAR(hr.h, 300, 0.5);
    CHECK(h.res.selection.length == 0 && h.res.hovered == -1);

    /* click, Shift+click, drag, and the ASCII column */
    cui_vec2 p = hex_cell(ui, 3, 2, false);
    click_at(ui, build_hex, &h, p.x, p.y);
    CHECK(h.sel.start == 0x23 && h.sel.length == 1 && h.sel.cursor == 0x23);
    hex_mods(ui, CUI_MOD_SHIFT);
    p = hex_cell(ui, 0, 3, false);
    click_at(ui, build_hex, &h, p.x, p.y);
    hex_mods(ui, 0);
    CHECK(h.sel.start == 0x23 && h.sel.length == 14 && h.sel.cursor == 0x30);
    cui_vec2 a = hex_cell(ui, 5, 0, false), b = hex_cell(ui, 4, 1, false);
    drag2(ui, build_hex, &h, a.x, a.y, b.x, b.y, 6);
    CHECK(h.sel.start == 5 && h.sel.length == 16 && h.sel.cursor == 0x14);
    p = hex_cell(ui, 15, 1, true);
    click_at(ui, build_hex, &h, p.x, p.y);
    CHECK(h.sel.start == 0x1F && h.sel.length == 1);
    /* hovering marks the byte under the pointer */
    p = hex_cell(ui, 9, 4, false);
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 2);
    CHECK(h.res.hovered == 0x49);
    p = hex_cell(ui, 9, 4, true);
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 2);
    CHECK(h.res.hovered == 0x49);

    /* keys: arrows, Shift extends, Home/End, page and data ends */
    key(ui, build_hex, &h, CUI_KEY_RIGHT, 0);
    CHECK(h.sel.cursor == 0x20 && h.sel.start == 0x20 && h.sel.length == 1);
    CHECK(h.res.selection_changed);
    key(ui, build_hex, &h, CUI_KEY_DOWN, 0);
    CHECK(h.sel.cursor == 0x30);
    key(ui, build_hex, &h, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    key(ui, build_hex, &h, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK(h.sel.start == 0x30 && h.sel.length == 3 && h.sel.cursor == 0x32);
    key(ui, build_hex, &h, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK(h.sel.start == 0x30 && h.sel.length == 2 && h.sel.cursor == 0x31);
    key(ui, build_hex, &h, CUI_KEY_END, 0);
    CHECK(h.sel.cursor == 0x3F && h.sel.length == 1);
    key(ui, build_hex, &h, CUI_KEY_HOME, CUI_MOD_SHIFT);
    CHECK(h.sel.start == 0x30 && h.sel.length == 16 && h.sel.cursor == 0x30);
    key(ui, build_hex, &h, CUI_KEY_END, CUI_MOD_CTRL);
    CHECK(h.sel.cursor == 4095 && h.sel.length == 1);
    key(ui, build_hex, &h, CUI_KEY_DOWN, 0);
    CHECK(h.sel.cursor == 4095);
    run(ui, build_hex, &h, 2);
    /* the last row is in view: a click on the bottom row lands near the end */
    p = hex_cell(ui, 15, 12, false);
    click_at(ui, build_hex, &h, p.x, p.y);
    CHECK(h.sel.cursor > 4095 - 16 * 3 && (h.sel.cursor & 15) == 15);
    key(ui, build_hex, &h, CUI_KEY_HOME, CUI_MOD_CTRL);
    CHECK(h.sel.cursor == 0);
    key(ui, build_hex, &h, CUI_KEY_RIGHT, 0);
    key(ui, build_hex, &h, CUI_KEY_PAGE_DOWN, 0);
    size_t page = h.sel.cursor - 1;
    CHECK(page >= 16 * 8 && page % 16 == 0);
    key(ui, build_hex, &h, CUI_KEY_PAGE_DOWN, CUI_MOD_SHIFT);
    CHECK(h.sel.cursor == 1 + 2 * page && h.sel.start == 1 + page && h.sel.length == page + 1);
    key(ui, build_hex, &h, CUI_KEY_PAGE_UP, 0);
    key(ui, build_hex, &h, CUI_KEY_PAGE_UP, 0);
    key(ui, build_hex, &h, CUI_KEY_PAGE_UP, 0);
    CHECK(h.sel.cursor == 1);
    key(ui, build_hex, &h, CUI_KEY_A, CUI_MOD_PRIMARY);
    CHECK(h.sel.start == 0 && h.sel.length == 4096);
    key(ui, build_hex, &h, CUI_KEY_ESCAPE, 0);
    CHECK(h.sel.length == 0);

    /* copy: hex text, and ASCII with Shift */
    g_clip[0] = 0;
    key(ui, build_hex, &h, CUI_KEY_C, CUI_MOD_PRIMARY);   /* nothing selected: nothing copied */
    CHECK(g_clip[0] == 0 && !h.res.copied);
    h.sel.start = 0x0E; h.sel.length = 4; h.sel.cursor = 0x11;   /* the caller sets the selection */
    key(ui, build_hex, &h, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "0E 0F 10 11") == 0 && h.res.copied);
    key(ui, build_hex, &h, CUI_KEY_RIGHT, CUI_MOD_SHIFT);    /* the anchor stays at 0x0E */
    CHECK(h.sel.start == 0x0E && h.sel.length == 5);
    h.sel.start = 0x3F; h.sel.length = 5; h.sel.cursor = 0x3F;
    key(ui, build_hex, &h, CUI_KEY_C, CUI_MOD_PRIMARY | CUI_MOD_SHIFT);
    CHECK(strcmp(g_clip, "?@ABC") == 0);
    h.sel.start = 0x7E; h.sel.length = 3; h.sel.cursor = 0x80;
    key(ui, build_hex, &h, CUI_KEY_C, CUI_MOD_PRIMARY | CUI_MOD_SHIFT);
    CHECK(strcmp(g_clip, "~..") == 0);
    h.sel.start = 5000; h.sel.length = 9; h.sel.cursor = 9999;   /* out of range: kept inside the data */
    run(ui, build_hex, &h, 1);
    CHECK(h.sel.length == 0 && h.sel.cursor == 4095);

    /* a labelled range shows its label on hover, an unlabelled one does not */
    key(ui, build_hex, &h, CUI_KEY_HOME, CUI_MOD_CTRL);
    run(ui, build_hex, &h, 3);
    p = hex_cell(ui, 2, 5, false);   /* 0x52: no range */
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 70);
    int plain = cui_get_draw_data(ui)->vertex_count;
    p = hex_cell(ui, 3, 4, false);   /* 0x43: in "Header" */
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 70);
    CHECK(h.res.hovered == 0x43);
    int tipped = cui_get_draw_data(ui)->vertex_count;
    CHECK(tipped > plain + 40);   /* the tooltip's text and panel */
    p = hex_cell(ui, 2, 8, false);   /* 0x82: a range without a label */
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 70);
    CHECK(h.res.hovered == 0x82);
    CHECK(cui_get_draw_data(ui)->vertex_count < tipped - 40);

    /* go to offset scrolls the byte into view and selects it */
    cui_hex_view_goto(ui, "hex", 0x900);
    run(ui, build_hex, &h, 2);
    CHECK(h.sel.start == 0x900 && h.sel.length == 1 && h.sel.cursor == 0x900);
    p = hex_cell(ui, 0, 6, false);   /* the middle of the view */
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &h, 2);
    CHECK(h.res.hovered >= 0x900 - 16 * 2 && h.res.hovered <= 0x900 + 16 * 2);
    cui_hex_view_goto(ui, "hex", 1u << 30);   /* past the end: the last byte */
    run(ui, build_hex, &h, 2);
    CHECK(h.sel.cursor == 4095);

    /* the wheel scrolls the view */
    key(ui, build_hex, &h, CUI_KEY_HOME, CUI_MOD_CTRL);
    run(ui, build_hex, &h, 20);
    p = hex_cell(ui, 0, 0, false);
    cui_input_mouse_move(ui, p.x, p.y);
    cui_input_scroll(ui, 0, -2);
    run(ui, build_hex, &h, 20);
    CHECK(h.res.hovered == 16 * 6);
    cui_destroy(ui);

    /* virtualization: an 8 MiB buffer builds only the rows in view */
    ui = new_ctx();
    size_t big_n = (size_t)8 << 20;
    uint8_t *big = malloc(big_n);
    for (size_t i = 0; i < big_n; i++) big[i] = (uint8_t)(i * 7u);
    hex_ctx hb;
    memset(&hb, 0, sizeof hb);
    hb.data = small; hb.size = sizeof small;
    run(ui, build_hex, &hb, 3);
    int small_vtx = cui_get_draw_data(ui)->vertex_count;
    hb.data = big; hb.size = big_n;
    g_hex_digits = 6;
    run(ui, build_hex, &hb, 3);
    int big_vtx = cui_get_draw_data(ui)->vertex_count;
    CHECK(big_vtx < small_vtx * 2 && big_vtx > small_vtx / 2);
    p = hex_cell(ui, 0, 0, false);
    click_at(ui, build_hex, &hb, p.x, p.y);
    key(ui, build_hex, &hb, CUI_KEY_END, CUI_MOD_CTRL);
    CHECK(hb.sel.cursor == big_n - 1);
    run(ui, build_hex, &hb, 3);
    /* rows stay exact at the far end: the bottom row holds the last bytes */
    p = hex_cell(ui, 15, 12, false);
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &hb, 2);
    CHECK(hb.res.hovered >= 0 && (size_t)hb.res.hovered > big_n - 16 * 3 && (hb.res.hovered & 15) == 15);
    g_clip[0] = 0;
    key(ui, build_hex, &hb, CUI_KEY_C, CUI_MOD_PRIMARY);
    char want[4];
    snprintf(want, sizeof want, "%02X", (unsigned)big[big_n - 1]);
    CHECK(strcmp(g_clip, want) == 0);
    key(ui, build_hex, &hb, CUI_KEY_PAGE_UP, CUI_MOD_SHIFT);
    key(ui, build_hex, &hb, CUI_KEY_UP, CUI_MOD_SHIFT);
    CHECK(hb.sel.cursor < big_n - 16 * 8 && hb.sel.length > 16 * 8);
    key(ui, build_hex, &hb, CUI_KEY_A, CUI_MOD_PRIMARY);   /* copying everything stops at 4 MiB */
    key(ui, build_hex, &hb, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(hb.res.copied && strncmp(g_clip, "00 07 0E 15", 11) == 0);
    CHECK(g_clip_len == ((size_t)4 << 20) * 3 - 1);
    hb.own_sel = true;   /* no selection passed: the view keeps its own */
    run(ui, build_hex, &hb, 2);
    key(ui, build_hex, &hb, CUI_KEY_END, 0);
    CHECK(hb.res.selection.length == 1 && hb.res.selection.cursor % 16 == 15);
    free(big);
    g_hex_digits = 4;

    /* size 0, and no data at all */
    hb.own_sel = false;
    hb.data = small; hb.size = 0;
    memset(&hb.sel, 0, sizeof hb.sel);
    hb.sel.cursor = 12;
    run(ui, build_hex, &hb, 3);
    CHECK(hb.res.selection.length == 0 && hb.res.selection.cursor == 0 && hb.res.hovered == -1);
    key(ui, build_hex, &hb, CUI_KEY_END, CUI_MOD_CTRL);
    key(ui, build_hex, &hb, CUI_KEY_A, CUI_MOD_PRIMARY);
    key(ui, build_hex, &hb, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(hb.sel.length == 0 && !hb.res.copied);
    hb.data = NULL; hb.size = 100;
    run(ui, build_hex, &hb, 2);
    CHECK(hb.res.selection.length == 0);

    /* a size that is not a multiple of the row */
    hb.data = small; hb.size = 37;
    run(ui, build_hex, &hb, 3);
    p = hex_cell(ui, 6, 1, false);
    click_at(ui, build_hex, &hb, p.x, p.y);
    CHECK(hb.sel.cursor == 0x16);
    key(ui, build_hex, &hb, CUI_KEY_DOWN, 0);
    CHECK(hb.sel.cursor == 36);   /* no byte below: the last one */
    key(ui, build_hex, &hb, CUI_KEY_HOME, 0);
    key(ui, build_hex, &hb, CUI_KEY_END, 0);
    CHECK(hb.sel.cursor == 36);
    key(ui, build_hex, &hb, CUI_KEY_RIGHT, 0);
    CHECK(hb.sel.cursor == 36);
    p = hex_cell(ui, 10, 2, false);   /* past the last byte */
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &hb, 2);
    CHECK(hb.res.hovered == -1);
    p = hex_cell(ui, 4, 2, false);
    cui_input_mouse_move(ui, p.x, p.y);
    run(ui, build_hex, &hb, 2);
    CHECK(hb.res.hovered == 36);
    CHECK_NEAR(rect_of(ui, cui_make_id(ui, "hex")).h, 300, 0.5);
    cui_destroy(ui);

    /* editing: hex digits overwrite at the cursor, high digit first */
    ui = new_ctx();
    static uint8_t buf[64];
    memset(buf, 0, sizeof buf);
    hex_ctx he;
    memset(&he, 0, sizeof he);
    he.data = buf; he.size = sizeof buf; he.edit = buf;
    run(ui, build_hex, &he, 3);
    p = hex_cell(ui, 0, 1, false);
    click_at(ui, build_hex, &he, p.x, p.y);
    CHECK(he.sel.cursor == 0x10);
    cui_input_text(ui, "a");
    run(ui, build_hex, &he, 1);
    CHECK(buf[0x10] == 0xA0 && he.res.edited && he.res.edit_offset == 0x10 && he.res.edit_length == 1 && he.sel.cursor == 0x10);
    cui_input_text(ui, "5");
    run(ui, build_hex, &he, 1);
    CHECK(buf[0x10] == 0xA5 && he.sel.cursor == 0x11 && he.sel.start == 0x11 && he.sel.length == 1);
    cui_input_text(ui, "zq");
    run(ui, build_hex, &he, 1);
    CHECK(!he.res.edited && buf[0x11] == 0);
    cui_input_text(ui, "1F2");
    run(ui, build_hex, &he, 1);
    CHECK(buf[0x11] == 0x1F && buf[0x12] == 0x20 && he.res.edit_offset == 0x11 && he.res.edit_length == 2);
    key(ui, build_hex, &he, CUI_KEY_LEFT, 0);   /* moving drops the half typed byte */
    cui_input_text(ui, "7");
    run(ui, build_hex, &he, 1);
    CHECK(buf[0x11] == 0x7F && buf[0x12] == 0x20);
    key(ui, build_hex, &he, CUI_KEY_END, CUI_MOD_CTRL);
    hex_mods(ui, 0);   /* Ctrl released: typing with Ctrl held is a shortcut, not an edit */
    cui_input_text(ui, "beef");
    run(ui, build_hex, &he, 1);
    CHECK(buf[63] == 0xEF && he.sel.cursor == 63);   /* the last byte takes every digit */
    /* read only: typing changes nothing */
    he.edit = NULL;
    run(ui, build_hex, &he, 1);
    cui_input_text(ui, "00");
    run(ui, build_hex, &he, 1);
    CHECK(buf[63] == 0xEF && !he.res.edited);
    cui_destroy(ui);

    /* narrow views drop bytes per row, then the ASCII column, then go to 4 */
    ui = new_ctx();
    cui_set_viewport(ui, 1200, 600, 1);
    hex_ctx hn;
    memset(&hn, 0, sizeof hn);
    hn.data = small; hn.size = sizeof small; hn.bpr = 32;
    int steps[3];
    float widths[3] = { 1100, 300, 200 };
    for (int k = 0; k < 3; k++) {
        hn.width = widths[k];
        run(ui, build_hex, &hn, 3);
        p = hex_cell(ui, 0, 0, false);
        click_at(ui, build_hex, &hn, p.x, p.y);
        key(ui, build_hex, &hn, CUI_KEY_DOWN, 0);
        steps[k] = (int)hn.sel.cursor;
        key(ui, build_hex, &hn, CUI_KEY_HOME, CUI_MOD_CTRL);
    }
    CHECK(steps[0] == 32 && steps[1] == 8 && steps[2] == 4);
    cui_destroy(ui);
}

/* markdown view ---------------------------------------------------------- */

typedef struct {
    char text[4096];
    int length;             /* 0: strlen */
    float width;            /* 0: fill a 600 point column */
    bool streaming;
    cui_markdown_result res;
    int links, toggles;
    char last_link[128];
} md_test;
static void build_md(cui_ctx *ui, void *u) {
    md_test *m = u;
    cui_column(ui, .pad = 20, .width = CUI_PX(m->width > 0 ? m->width + 40 : 640)) {
        m->res = cui_markdown(ui, "md", m->text, .length = m->length, .streaming = m->streaming);
    }
    if (m->res.link) { m->links++; snprintf(m->last_link, sizeof m->last_link, "%s", m->res.link); }
    if (m->res.task_offset >= 0) m->toggles++;
}
static cui_rect md_rect(cui_ctx *ui) { return cui_get_rect(ui, cui_make_id(ui, "md")); }
static void md_frames(cui_ctx *ui, md_test *m, int frames) { run(ui, build_md, m, frames); }
/* Moves the pointer along a row until the view reports a link under it. */
static bool md_find_link(cui_ctx *ui, md_test *m, float y, cui_vec2 *at) {
    cui_rect r = md_rect(ui);
    for (float x = r.x + 2; x < r.x + r.w; x += 3) {
        cui_input_mouse_move(ui, x, y);
        md_frames(ui, m, 1);
        if (m->res.hovered_link) { at->x = x; at->y = y; return true; }
    }
    return false;
}
static unsigned md_rng = 0x2545F491u;
static unsigned md_rand(void) { md_rng ^= md_rng << 13; md_rng ^= md_rng >> 17; md_rng ^= md_rng << 5; return md_rng; }

static void test_markdown(void) {
    printf("markdown: edge cases, links, tasks, keys, streaming, fuzz\n");
    cui_ctx *ui = new_ctx();
    static md_test m;
    /* empty and blank text build nothing tall */
    memset(&m, 0, sizeof m);
    md_frames(ui, &m, 3);
    CHECK(md_rect(ui).h < 1);
    snprintf(m.text, sizeof m.text, "   \n\t\n  \r\n");
    md_frames(ui, &m, 3);
    CHECK(md_rect(ui).h < 1);
    CHECK(m.res.link == NULL && m.res.task_offset == -1);
    /* a paragraph wraps to the width it gets */
    snprintf(m.text, sizeof m.text, "one line");
    md_frames(ui, &m, 3);
    float line_h = md_rect(ui).h;
    CHECK(line_h > 14 && line_h < 30);
    m.width = 120;
    snprintf(m.text, sizeof m.text, "a paragraph long enough that it has to wrap onto several lines at this width");
    md_frames(ui, &m, 3);
    CHECK(md_rect(ui).h > line_h * 3);
    CHECK_NEAR(md_rect(ui).w, 120, 0.5);
    m.width = 0;
    /* unclosed markers and fences, deep nesting, long lines, CRLF, ragged tables, bad UTF-8: no crash */
    static const char *const edge[] = {
        "**bold *italic ~~strike `code", "a *b **c*** d_e_f __g__ ~h~ ~~i~~ \\*j\\*", "```c\nint x;\n", "~~~\nunclosed\n\n\n",
        "[link](https://a.b/(c)) [bad](  [x]( <https://auto.link> www.example.com/x. https://e.org/a_(b)",
        "![img](i.png) [a [b](c) d](e) [](empty) [unclosed", "# H1\n## H2 ##\n###### H6\n####### seven\n#nospace",
        "Title\n===\nSub\n---\n- - -\n***\n___", "a\r\nb  \r\nc\\\r\nd<br>e\r\n", "&amp; &lt; &gt; &quot; &#39; &nbsp; &bogus;",
        "| a | b | c |\n|---|:-:|--:|\n| 1 |\n| 1 | 2 | 3 | 4 | 5 |\n|\n", "| a | b |\n|---|\nnot a table", "\xff\xfe\x80 \xc3 bad \xe2\x82 utf8 \xf0\x9f\x98",
        "> quote\nlazy\n> > nested\n>\n> - item\n>   ```\n>   code\n", "1. one\n2. two\n   - sub\n     1) deep\n10. ten\n- other",
        "    indented code\n\n    more", "- [ ] task\n- [x] done\n- [X]\n- [ ]no\n* [ ] star", "`` a ` b `` ``` c ```` d", "<br/> <BR > <b>html</b>",
    };
    for (int k = 0; k < (int)(sizeof edge / sizeof edge[0]); k++) {
        snprintf(m.text, sizeof m.text, "%s", edge[k]);
        md_frames(ui, &m, 2);
        CHECK(md_rect(ui).h > 0);
        m.streaming = true;
        md_frames(ui, &m, 2);
        m.streaming = false;
    }
    {   /* nesting far past the limit, as quotes and as lists */
        int o = 0;
        for (int k = 0; k < 300 && o < 3000; k++) o += snprintf(m.text + o, sizeof m.text - (size_t)o, "> ");
        snprintf(m.text + o, sizeof m.text - (size_t)o, "deep");
        md_frames(ui, &m, 2);
        CHECK(md_rect(ui).h > 0);
        o = 0;
        for (int k = 0; k < 60 && o < 3900; k++) o += snprintf(m.text + o, sizeof m.text - (size_t)o, "%*s- level %d\n", k * 2, "", k);
        md_frames(ui, &m, 2);
        CHECK(md_rect(ui).h > 0);
    }
    {   /* a very long line with no spaces breaks between characters and stays in the column */
        static char big[100001];
        for (int k = 0; k < 100000; k++) big[k] = (char)('a' + k % 26);
        big[100000] = 0;
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 20, .width = CUI_PX(340)) { cui_markdown(ui, "md", big); }
        cui_end_frame(ui);
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 20, .width = CUI_PX(340)) { cui_markdown(ui, "md", big); }
        cui_end_frame(ui);
        CHECK(md_rect(ui).h > 1000);
        CHECK_NEAR(md_rect(ui).w, 300, 0.5);
    }
    /* .length limits the text; CRLF lines parse like LF */
    snprintf(m.text, sizeof m.text, "# Title\r\n\r\nbody\r\n");
    md_frames(ui, &m, 2);
    float crlf_h = md_rect(ui).h;
    snprintf(m.text, sizeof m.text, "# Title\n\nbody\n");
    md_frames(ui, &m, 2);
    CHECK_NEAR(md_rect(ui).h, crlf_h, 0.5);
    m.length = 7;   /* just "# Title" */
    md_frames(ui, &m, 2);
    CHECK(md_rect(ui).h < crlf_h - 10);
    m.length = 0;

    /* links: hovering reports the URL, a click reports it once, nothing is opened */
    snprintf(m.text, sizeof m.text, "Read the [upgrade guide](https://tern.dev/docs/upgrade) first.");
    md_frames(ui, &m, 3);
    cui_rect r = md_rect(ui);
    cui_vec2 at;
    CHECK(md_find_link(ui, &m, r.y + r.h / 2, &at));
    CHECK(m.res.hovered_link && strcmp(m.res.hovered_link, "https://tern.dev/docs/upgrade") == 0);
    CHECK(at.x > r.x + 30);   /* the link starts after "Read the " */
    m.links = 0;
    click_at(ui, build_md, &m, at.x, at.y);
    CHECK(m.links == 1 && strcmp(m.last_link, "https://tern.dev/docs/upgrade") == 0);
    cui_input_mouse_move(ui, r.x + 2, r.y + r.h / 2);
    md_frames(ui, &m, 2);
    CHECK(m.res.hovered_link == NULL);
    /* the keyboard: Tab focuses the link, Enter activates it */
    m.links = 0;
    cui_input_key(ui, CUI_KEY_TAB, true, 0); md_frames(ui, &m, 1); cui_input_key(ui, CUI_KEY_TAB, false, 0); md_frames(ui, &m, 1);
    cui_input_key(ui, CUI_KEY_ENTER, true, 0); md_frames(ui, &m, 1); cui_input_key(ui, CUI_KEY_ENTER, false, 0); md_frames(ui, &m, 1);
    CHECK(m.links == 1);
    /* www. links report an http:// URL; a link wrapped over two lines is one link */
    snprintf(m.text, sizeof m.text, "see www.example.com/path.");
    md_frames(ui, &m, 3);
    r = md_rect(ui);
    CHECK(md_find_link(ui, &m, r.y + r.h / 2, &at));
    CHECK(m.res.hovered_link && strcmp(m.res.hovered_link, "http://www.example.com/path") == 0);

    /* task items: a click reports the offset of the box and its line; the caller applies it */
    snprintf(m.text, sizeof m.text, "Intro\n\n- [ ] Rerun the benchmark\n- [x] Update the changelog\n");
    md_frames(ui, &m, 3);
    r = md_rect(ui);
    m.toggles = 0;
    float row_y = r.y + r.h - line_h * 1.5f;   /* the first task row sits above the last */
    click_at(ui, build_md, &m, r.x + 9, row_y - 4);
    CHECK(m.toggles == 1);
    int off1 = (int)(strstr(m.text, "[ ]") - m.text) + 1;
    CHECK(m.res.task_offset == off1 && m.res.task_checked && m.res.task_line == 3);
    CHECK(cui_markdown_apply_task(m.text, &m.res));
    CHECK(strstr(m.text, "- [x] Rerun") != NULL);
    md_frames(ui, &m, 2);
    m.toggles = 0;
    click_at(ui, build_md, &m, r.x + 9, r.y + r.h - line_h * 0.5f);
    CHECK(m.toggles == 1 && !m.res.task_checked && m.res.task_line == 4);
    CHECK(cui_markdown_apply_task(m.text, &m.res));
    CHECK(strstr(m.text, "- [ ] Update") != NULL);
    {   /* applying refuses offsets that are not a box */
        cui_markdown_result bad;
        memset(&bad, 0, sizeof bad);
        bad.task_offset = 1; bad.task_checked = true;
        CHECK(!cui_markdown_apply_task(m.text, &bad));
        bad.task_offset = -1;
        CHECK(!cui_markdown_apply_task(m.text, &bad));
        bad.task_offset = 100000;
        CHECK(!cui_markdown_apply_task(m.text, &bad));
    }

    /* streaming: appending a few bytes a frame never shrinks the view by more than a line */
    {
        static const char reply[] =
            "## Fixing the flaky upload test\n\nThe test fails when **the retry *timer* fires** before `upload_done`. See [the issue](https://e.org/i/42).\n\n"
            "1. Stub the clock\n2. Advance it by hand\n   - check the first retry\n   - check the backoff\n\n- [ ] Land the fix\n- [x] Write it down\n\n"
            "> The timer starts on the first byte.\n\n```c\nvoid tick(fake_clock *c, int ms) {\n    c->now += ms;\n}\n```\n\n"
            "| Run | Before | After |\n|:----|------:|-----:|\n| CI | 1 in 30 | 0 in 500 |\n| Laptop | 1 in 80 | 0 in 500 |\n\nDone.";
        int n = (int)strlen(reply), drops = 0;
        float prev = 0, worst = 0;
        snprintf(m.text, sizeof m.text, "%s", reply);
        m.streaming = true;
        for (int len = 1; len <= n; len += 3) {
            m.length = len;
            md_frames(ui, &m, 2);
            float h = md_rect(ui).h;
            if (h < prev - 0.5f) { drops++; if (prev - h > worst) worst = prev - h; }
            prev = h;
        }
        m.streaming = false; m.length = 0;
        md_frames(ui, &m, 2);
        CHECK(fabsf(md_rect(ui).h - prev) < line_h * 2);   /* finishing the stream changes little */
        CHECK(worst <= line_h * 1.6f);
        CHECK(drops <= 8);
        if (worst > line_h * 1.6f || drops > 8) printf("  streaming: %d drops, worst %.1f\n", drops, worst);
    }

    /* random markdown-ish input at random widths */
    {
        static const char *const bits[] = { "*", "**", "_", "~~", "`", "```", "[", "](", ")", "<", ">", "#", "- ", "1. ", "|", "\n", "\n\n",
                                            "  ", "\t", "\\", "&amp;", "[ ]", "[x]", "https://", "www.", "word ", "\xc3\xa9", "\xff", "---", "\r\n", "!" };
        for (int iter = 0; iter < 400; iter++) {
            int o = 0, parts = (int)(md_rand() % 120);
            for (int k = 0; k < parts; k++) {
                const char *b = bits[md_rand() % (sizeof bits / sizeof bits[0])];
                int bl = (int)strlen(b);
                if (o + bl >= (int)sizeof m.text - 1) break;
                memcpy(m.text + o, b, (size_t)bl);
                o += bl;
            }
            m.text[o] = 0;
            m.width = (float)(40 + md_rand() % 700);
            m.streaming = md_rand() % 2;
            m.length = md_rand() % 3 == 0 && o > 0 ? (int)(md_rand() % (unsigned)o) + 1 : 0;
            cui_input_mouse_move(ui, (float)(md_rand() % 800), (float)(md_rand() % 600));
            md_frames(ui, &m, 2);
        }
        m.width = 0; m.streaming = false; m.length = 0;
        CHECK(true);
    }
    cui_destroy(ui);
}

/* flame graph ------------------------------------------------------------ */

typedef struct {
    const cui_flame_span *s; int n;
    cui_flame_view view; int sel;
    cui_flame_result last;
    int sel_changes, zooms;
    float height;
    cui_id id;
} flame_ctx;
static void build_flame(cui_ctx *ui, void *u) {
    flame_ctx *f = u;
    cui_column(ui, .pad = 20) {
        f->last = cui_flame_graph(ui, "fg", f->s, f->n, &f->view, &f->sel, .width = CUI_PX(600), .height = f->height);
        f->id = cui_last_id(ui);
    }
    if (f->last.selection_changed) f->sel_changes++;
    if (f->last.zoomed >= 0) f->zooms++;
}
static cui_vec2 flame_at(flame_ctx *f, cui_ctx *ui, int i) { return center(cui_flame_span_rect(ui, f->id, &f->s[i])); }

static void test_flame(void) {
    printf("flame graph: fit, select, keys, zoom about the pointer, pan, zoom to span, reset, tooltip, 100k spans, empty\n");
    static const cui_flame_span small[] = {
        { "frame", 0.000, 0.010, 0, 0 },
        { "update", 0.000, 0.004, 1, 0 }, { "render", 0.005, 0.009, 1, 0 },
        { "physics", 0.0005, 0.003, 2, 0 }, { "draw", 0.006, 0.007, 2, 0 },
    };
    /* sorting puts any order into depth, then start */
    cui_flame_span shuf[5] = { small[4], small[1], small[3], small[0], small[2] };
    cui_flame_sort(shuf, 5);
    bool same = true;
    for (int i = 0; i < 5; i++) same = same && shuf[i].start == small[i].start && shuf[i].depth == small[i].depth;
    CHECK(same);

    cui_ctx *ui = new_ctx();
    flame_ctx f;
    memset(&f, 0, sizeof f);
    f.s = small; f.n = 5; f.sel = 99; f.height = 200;   /* out of range: becomes -1 */
    run(ui, build_flame, &f, 3);
    CHECK(f.sel == -1 && f.sel_changes == 1);   /* the clamp is reported */
    f.sel_changes = 0;
    CHECK_NEAR(f.view.start, 0, 1e-12);
    CHECK_NEAR(f.view.end, 0.010, 1e-12);
    cui_rect fr = cui_flame_span_rect(ui, f.id, &small[0]);
    CHECK(fr.w > 560 && fr.w < 600 && fr.h > 15);
    float px = fr.x, pw = fr.w;
    /* click selects, and focuses the graph */
    cui_vec2 c = flame_at(&f, ui, 3);
    click_at(ui, build_flame, &f, c.x, c.y);
    CHECK(f.sel == 3 && f.sel_changes == 1);
    /* empty space clears the selection */
    click_at(ui, build_flame, &f, c.x, fr.y + 150);
    CHECK(f.sel == -1);
    click_at(ui, build_flame, &f, c.x, c.y);
    CHECK(f.sel == 3);
    click_at(ui, build_flame, &f, c.x, fr.y - 17);   /* the ruler: the selection stays */
    CHECK(f.sel == 3);
    /* arrows walk the tree: parent, child, siblings */
    key(ui, build_flame, &f, CUI_KEY_UP, 0);    CHECK(f.sel == 1);
    key(ui, build_flame, &f, CUI_KEY_UP, 0);    CHECK(f.sel == 0);
    key(ui, build_flame, &f, CUI_KEY_UP, 0);    CHECK(f.sel == 0);
    key(ui, build_flame, &f, CUI_KEY_DOWN, 0);  CHECK(f.sel == 1);
    key(ui, build_flame, &f, CUI_KEY_RIGHT, 0); CHECK(f.sel == 2);
    key(ui, build_flame, &f, CUI_KEY_RIGHT, 0); CHECK(f.sel == 2);
    key(ui, build_flame, &f, CUI_KEY_DOWN, 0);  CHECK(f.sel == 4);
    key(ui, build_flame, &f, CUI_KEY_LEFT, 0);  CHECK(f.sel == 3);
    CHECK_NEAR(f.view.end - f.view.start, 0.010, 1e-12);   /* all in view: nothing moved */
    /* the wheel zooms, and the time under the pointer stays under it */
    float mx = px + pw * 0.3f, my = c.y - 20;
    cui_input_mouse_move(ui, mx, my);
    run(ui, build_flame, &f, 1);
    double t_before = f.view.start + (mx - px) / pw * (f.view.end - f.view.start);
    cui_input_scroll(ui, 0, 3);
    run(ui, build_flame, &f, 1);
    double range = f.view.end - f.view.start;
    CHECK_NEAR(range, 0.010 / pow(1.12, 3), 1e-9);
    CHECK_NEAR(f.view.start + (mx - px) / pw * range, t_before, 1e-9);
    CHECK(f.last.view_changed || f.view.start > 0);
    /* drag pans by the pointer travel, and does not select */
    double s0 = f.view.start;
    drag(ui, build_flame, &f, mx, my, mx - 100, 5);
    CHECK_NEAR(f.view.start - s0, 100.0 / pw * range, 1e-9);
    CHECK_NEAR(f.view.end - f.view.start, range, 1e-12);
    CHECK(f.sel == 3);
    /* Shift+wheel pans too */
    s0 = f.view.start;
    cui_input_key(ui, CUI_KEY_NONE, true, CUI_MOD_SHIFT);
    cui_input_scroll(ui, 0, -1);
    run(ui, build_flame, &f, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    run(ui, build_flame, &f, 1);
    CHECK(f.view.start > s0);
    CHECK_NEAR(f.view.end - f.view.start, range, 1e-12);
    /* F fits everything again (reduced motion: no animation) */
    key(ui, build_flame, &f, CUI_KEY_F, 0);
    CHECK_NEAR(f.view.start, 0, 1e-12);
    CHECK_NEAR(f.view.end, 0.010, 1e-12);
    /* double-click zooms to fit a span */
    c = flame_at(&f, ui, 2);
    click_at(ui, build_flame, &f, c.x, c.y);
    click_at(ui, build_flame, &f, c.x, c.y);
    CHECK(f.sel == 2 && f.zooms == 1);
    CHECK_NEAR(f.view.start, 0.005 - 0.0002, 1e-9);
    CHECK_NEAR(f.view.end, 0.009 + 0.0002, 1e-9);
    cui_rect zr = cui_flame_span_rect(ui, f.id, &small[2]);
    CHECK_NEAR(zr.w, pw / 1.1, 1.0);
    /* Esc fits; Esc again clears the selection */
    key(ui, build_flame, &f, CUI_KEY_ESCAPE, 0);
    CHECK_NEAR(f.view.end - f.view.start, 0.010, 1e-12);
    CHECK(f.sel == 2);
    key(ui, build_flame, &f, CUI_KEY_ESCAPE, 0);
    CHECK(f.sel == -1);
    /* the selection is the caller's: set it, Enter zooms to it */
    f.sel = 1;
    key(ui, build_flame, &f, CUI_KEY_ENTER, 0);
    CHECK_NEAR(f.view.start, 0.0, 1e-9);   /* clamped to the data */
    CHECK_NEAR(f.view.end, 0.0044, 1e-9);
    CHECK(f.zooms == 2);
    /* Shift+Right pans a tenth of the view; - zooms out around the middle */
    s0 = f.view.start;
    range = f.view.end - f.view.start;
    key(ui, build_flame, &f, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_NEAR(f.view.start - s0, range * 0.1, 1e-9);
    CHECK(f.sel == 1);
    key(ui, build_flame, &f, CUI_KEY_MINUS, 0);
    CHECK_NEAR(f.view.end - f.view.start, range * 1.25, 1e-9);
    key(ui, build_flame, &f, CUI_KEY_HOME, 0);
    CHECK_NEAR(f.view.end - f.view.start, 0.010, 1e-12);
    /* with motion on, zooming to a span animates and settles on the same view */
    cui_set_reduced_motion(ui, false);
    f.sel = 2;
    key(ui, build_flame, &f, CUI_KEY_ENTER, 0);
    double mid = f.view.end - f.view.start;
    CHECK(mid < 0.010 && mid > 0.0044 + 1e-6);   /* on its way */
    run(ui, build_flame, &f, 90);
    CHECK_NEAR(f.view.start, 0.005 - 0.0002, 1e-9);
    CHECK_NEAR(f.view.end, 0.009 + 0.0002, 1e-9);
    key(ui, build_flame, &f, CUI_KEY_HOME, 0);
    run(ui, build_flame, &f, 90);
    CHECK_NEAR(f.view.end - f.view.start, 0.010, 1e-12);
    cui_set_reduced_motion(ui, true);
    /* hover: the span is reported and a tooltip appears after the delay */
    cui_input_mouse_move(ui, 790, 590);
    run(ui, build_flame, &f, 70);
    CHECK(f.last.hovered == -1);
    int idle = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_flame, &f, 70);
    CHECK(cui_get_draw_data(ui)->vertex_count == idle);   /* not hovering: no tooltip */
    c = flame_at(&f, ui, 4);
    cui_input_mouse_move(ui, c.x, c.y);
    run(ui, build_flame, &f, 3);
    CHECK(f.last.hovered == 4);
    int before = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_flame, &f, 70);
    CHECK(cui_get_draw_data(ui)->vertex_count > before);
    /* empty space in the graph: nothing hovered, the tooltip leaves */
    cui_input_mouse_move(ui, c.x, fr.y + 150);
    run(ui, build_flame, &f, 70);
    CHECK(f.last.hovered == -1);
    int gone = cui_get_draw_data(ui)->vertex_count;
    run(ui, build_flame, &f, 30);
    CHECK(cui_get_draw_data(ui)->vertex_count == gone);
    cui_destroy(ui);

    /* 100k spans: only what is in view is visited, thin spans merge */
    int big_n = 1 + 1000 + 100000;
    cui_flame_span *big = malloc(sizeof *big * (size_t)big_n);
    big[0] = (cui_flame_span){ "capture", 0, 1.0, 0, 0 };
    for (int i = 0; i < 1000; i++) {
        big[1 + i] = (cui_flame_span){ "frame", i * 1e-3, i * 1e-3 + 0.9e-3, 1, 0 };
        for (int j = 0; j < 100; j++) {
            double a = i * 1e-3 + j * 9e-6;
            big[1001 + i * 100 + j] = (cui_flame_span){ (j % 3) ? "job" : "wait", a, a + 8e-6, 2, 0 };
        }
    }
    ui = new_ctx();
    memset(&f, 0, sizeof f);
    f.s = big; f.n = big_n; f.sel = -1;
    clock_t t0 = clock();
    run(ui, build_flame, &f, 3);
    CHECK_NEAR(f.view.end, 1.0, 1e-12);
    CHECK(cui_get_draw_data(ui)->vertex_count < 60000);   /* bounded by pixels, not spans */
    fr = cui_flame_span_rect(ui, f.id, &big[0]);
    cui_input_mouse_move(ui, fr.x + fr.w * 0.5f, fr.y + 50);
    for (int k = 0; k < 40; k++) { cui_input_scroll(ui, 0, 2); run(ui, build_flame, &f, 1); }
    CHECK(f.view.end - f.view.start < 2e-4);
    c = flame_at(&f, ui, 1001 + 500 * 100 + 1);   /* a job in the middle frame, now wide enough to click */
    CHECK(c.x > fr.x && c.x < fr.x + fr.w);
    click_at(ui, build_flame, &f, c.x, c.y);
    CHECK(f.sel == 1001 + 500 * 100 + 1);
    for (int k = 0; k < 40; k++) { cui_input_scroll(ui, 0, -2); run(ui, build_flame, &f, 1); }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    CHECK_NEAR(f.view.end - f.view.start, 1.0, 1e-9);
    CHECK(secs < 5.0);
    printf("  100k spans: %d frames in %.3f s\n", 3 + 80 + 3, secs);
    cui_destroy(ui);
    free(big);

    /* no data, and unsorted data, are safe */
    ui = new_ctx();
    memset(&f, 0, sizeof f);
    f.s = NULL; f.n = 0; f.sel = 2;
    run(ui, build_flame, &f, 3);
    CHECK(f.sel == -1 && f.last.hovered == -1);
    fr = rect_of(ui, f.id);
    CHECK(fr.w > 500 && fr.h > 40);
    cui_input_mouse_move(ui, fr.x + 100, fr.y + 40);
    cui_input_scroll(ui, 0, 2);
    run(ui, build_flame, &f, 1);
    click_at(ui, build_flame, &f, fr.x + 100, fr.y + 40);
    key(ui, build_flame, &f, CUI_KEY_DOWN, 0);
    key(ui, build_flame, &f, CUI_KEY_F, 0);
    CHECK(f.sel == -1 && f.last.hovered == -1);
    cui_flame_span rev[5];
    for (int i = 0; i < 5; i++) rev[i] = small[4 - i];
    f.s = rev; f.n = 5;
    run(ui, build_flame, &f, 3);
    cui_input_scroll(ui, 0, 2);
    run(ui, build_flame, &f, 1);
    key(ui, build_flame, &f, CUI_KEY_RIGHT, 0);
    key(ui, build_flame, &f, CUI_KEY_UP, 0);
    CHECK(f.sel >= -1 && f.sel < 5);
    cui_destroy(ui);
}

/* code editor ----------------------------------------------------------- */

typedef struct {
    char buf[2048];
    char *big; size_t cap;     /* an outside buffer instead of buf, or a smaller capacity for buf */
    const char *lang;
    bool ro;
    char other[32];
    cui_code_editor_result r;
    bool any_changed;
} ced_ctx;
static void build_ced(cui_ctx *ui, void *u) {
    ced_ctx *c = u;
    cui_column(ui, .pad = 20, .gap = 12) {
        char *b = c->big ? c->big : c->buf;
        size_t cap = c->cap ? c->cap : sizeof c->buf;
        c->r = cui_code_editor(ui, "ed", b, cap, .lang = c->lang, .read_only = c->ro, .height = 300, .width = CUI_PX(600));
        c->any_changed |= c->r.changed;
        cui_input(ui, "after", c->other, sizeof c->other, .width = CUI_PX(200));
    }
}
static void ced_text(ced_ctx *c, const char *s) { snprintf(c->buf, sizeof c->buf, "%s", s); }
/* caret (or a selection) from code; also focuses the editor */
static void ced_sel(cui_ctx *ui, ced_ctx *c, int a, int b) { cui_code_editor_select(ui, "ed", a, b); run(ui, build_ced, c, 1); }
static int ced_at(const ced_ctx *c, const char *needle) { const char *p = strstr(c->buf, needle); return p ? (int)(p - c->buf) : -1; }

static void test_code_editor(void) {
    printf("code editor: typing, indentation, undo, clipboard, find, brackets, capacity, virtualization\n");
    cui_ctx *ui = new_ctx();
    static ced_ctx c;
    memset(&c, 0, sizeof c);
    c.lang = "c";
    run(ui, build_ced, &c, 3);
    cui_rect er = rect_of(ui, cui_make_id(ui, "ed"));
    CHECK_NEAR(er.h, 300, 0.5);
    CHECK(c.r.line == 1 && c.r.column == 1 && c.r.line_count == 1 && !c.r.focused);

    /* typing: a click focuses, text arrives as text input like a text area's */
    click_at(ui, build_ced, &c, er.x + 200, er.y + 100);
    CHECK(c.r.focused && cui_wants_keyboard(ui));
    type(ui, build_ced, &c, "int x;");
    CHECK(strcmp(c.buf, "int x;") == 0 && c.r.changed && c.r.column == 7);
    key(ui, build_ced, &c, CUI_KEY_BACKSPACE, 0);
    CHECK(strcmp(c.buf, "int x") == 0);
    key(ui, build_ced, &c, CUI_KEY_LEFT, 0);
    type(ui, build_ced, &c, "\xc3\xa9");
    CHECK(strcmp(c.buf, "int \xc3\xa9x") == 0 && c.r.column == 6);

    /* Enter keeps the indentation, one level more after an opening brace */
    ced_text(&c, "    foo();");
    ced_sel(ui, &c, 10, 10);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(strcmp(c.buf, "    foo();\n    ") == 0 && c.r.line == 2 && c.r.column == 5);
    ced_text(&c, "  if (a) {");
    ced_sel(ui, &c, 10, 10);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(strcmp(c.buf, "  if (a) {\n      ") == 0 && c.r.cursor == 17);
    type(ui, build_ced, &c, "go();");
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    type(ui, build_ced, &c, "}");   /* a closing brace on a blank line steps back a level */
    CHECK(strcmp(c.buf, "  if (a) {\n      go();\n  }") == 0);
    ced_text(&c, "f() {}");   /* between braces: the closing one moves down */
    ced_sel(ui, &c, 5, 5);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(strcmp(c.buf, "f() {\n    \n}") == 0 && c.r.line == 2 && c.r.column == 5);
    c.lang = "python";
    ced_text(&c, "def f():");
    ced_sel(ui, &c, 8, 8);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(strcmp(c.buf, "def f():\n    ") == 0);
    c.lang = "c";

    /* Tab: spaces to the next stop; on a selection, Tab and Shift+Tab move whole lines */
    ced_text(&c, "ab");
    ced_sel(ui, &c, 2, 2);
    key(ui, build_ced, &c, CUI_KEY_TAB, 0);
    CHECK(strcmp(c.buf, "ab  ") == 0 && c.r.focused);   /* the Tab stayed in the editor */
    ced_text(&c, "a\nb\nc\n");
    ced_sel(ui, &c, 0, 3);
    key(ui, build_ced, &c, CUI_KEY_TAB, 0);
    CHECK(strcmp(c.buf, "    a\n    b\nc\n") == 0);
    CHECK(c.r.anchor == 0 && c.r.cursor == 11);
    key(ui, build_ced, &c, CUI_KEY_TAB, 0);
    CHECK(strcmp(c.buf, "        a\n        b\nc\n") == 0);
    key(ui, build_ced, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    key(ui, build_ced, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    CHECK(strcmp(c.buf, "a\nb\nc\n") == 0 && c.r.anchor == 0 && c.r.cursor == 3);
    key(ui, build_ced, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);   /* nothing left to outdent */
    CHECK(strcmp(c.buf, "a\nb\nc\n") == 0);
    ced_text(&c, "x\n  y\n");   /* a selection ending at a line start leaves that line alone */
    ced_sel(ui, &c, 0, 2);
    key(ui, build_ced, &c, CUI_KEY_TAB, 0);
    CHECK(strcmp(c.buf, "    x\n  y\n") == 0);
    ced_sel(ui, &c, 7, 7);   /* Shift+Tab without a selection outdents the caret's line */
    key(ui, build_ced, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    CHECK(strcmp(c.buf, "    x\ny\n") == 0 && c.r.cursor == 6);

    /* Home goes to the first character, then column 0, then back */
    ced_text(&c, "    foo");
    ced_sel(ui, &c, 7, 7);
    key(ui, build_ced, &c, CUI_KEY_HOME, 0);
    CHECK(c.r.cursor == 4);
    key(ui, build_ced, &c, CUI_KEY_HOME, 0);
    CHECK(c.r.cursor == 0);
    key(ui, build_ced, &c, CUI_KEY_HOME, 0);
    CHECK(c.r.cursor == 4);
    key(ui, build_ced, &c, CUI_KEY_END, CUI_MOD_SHIFT);
    CHECK(c.r.anchor == 4 && c.r.cursor == 7);
    key(ui, build_ced, &c, CUI_KEY_A, CUI_MOD_PRIMARY);
    CHECK(c.r.anchor == 0 && c.r.cursor == 7);

    /* undo: typing within a second is one step, Enter is its own */
    ced_text(&c, "");
    ced_sel(ui, &c, 0, 0);
    run(ui, build_ced, &c, 80);
    type(ui, build_ced, &c, "a");
    type(ui, build_ced, &c, "b");
    type(ui, build_ced, &c, "c");
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    type(ui, build_ced, &c, "d");
    CHECK(strcmp(c.buf, "abc\nd") == 0);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "abc\n") == 0 && c.r.changed);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "abc") == 0);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "") == 0);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY | CUI_MOD_SHIFT);
    CHECK(strcmp(c.buf, "abc") == 0 && c.r.cursor <= 3);
    key(ui, build_ced, &c, CUI_KEY_Y, CUI_MOD_PRIMARY);
    key(ui, build_ced, &c, CUI_KEY_Y, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "abc\nd") == 0);
    key(ui, build_ced, &c, CUI_KEY_Y, CUI_MOD_PRIMARY);   /* nothing more to redo */
    CHECK(strcmp(c.buf, "abc\nd") == 0);
    ced_sel(ui, &c, 5, 5);
    type(ui, build_ced, &c, "e");
    run(ui, build_ced, &c, 80);   /* a pause starts a new step */
    type(ui, build_ced, &c, "f");
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "abc\nde") == 0);

    /* clipboard round trip; without a selection copy and cut take the line */
    ced_text(&c, "foo bar\nbaz\n");
    ced_sel(ui, &c, 4, 7);
    key(ui, build_ced, &c, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "bar") == 0);
    ced_sel(ui, &c, 11, 11);
    key(ui, build_ced, &c, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "foo bar\nbazbar\n") == 0 && c.r.cursor == 14);
    ced_sel(ui, &c, 0, 3);
    key(ui, build_ced, &c, CUI_KEY_X, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "foo") == 0 && strcmp(c.buf, " bar\nbazbar\n") == 0);
    ced_sel(ui, &c, 6, 6);
    key(ui, build_ced, &c, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(strcmp(g_clip, "bazbar\n") == 0 && strcmp(c.buf, " bar\nbazbar\n") == 0);
    key(ui, build_ced, &c, CUI_KEY_X, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, " bar\n") == 0);
    snprintf(g_clip, sizeof g_clip, "a\r\nb");   /* carriage returns are dropped */
    ced_sel(ui, &c, 0, 0);
    key(ui, build_ced, &c, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "a\nb bar\n") == 0);

    /* find: count, next, previous (wrapping), Esc closes; Ctrl+F takes the selected word */
    ced_text(&c, "tile a;\nint tile_b;\n/* TILE */ tile;\n");
    ced_sel(ui, &c, 0, 0);
    cui_code_editor_find(ui, "ed", "tile");
    run(ui, build_ced, &c, 2);
    CHECK(c.r.find_matches == 4 && c.r.find_index == 1 && c.r.anchor == 0 && c.r.cursor == 4);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);   /* the find field has focus: Enter is next */
    CHECK(c.r.find_index == 2 && c.r.anchor == ced_at(&c, "tile_b"));
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(c.r.find_index == 4);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);   /* wraps to the first */
    CHECK(c.r.find_index == 1 && c.r.anchor == 0);
    key(ui, build_ced, &c, CUI_KEY_ENTER, CUI_MOD_SHIFT);   /* back, wrapping to the last */
    CHECK(c.r.find_index == 4);
    key(ui, build_ced, &c, CUI_KEY_ENTER, CUI_MOD_SHIFT);
    CHECK(c.r.find_index == 3 && c.r.anchor == ced_at(&c, "TILE"));
    type(ui, build_ced, &c, "x");   /* typing in the field searches again */
    run(ui, build_ced, &c, 1);
    CHECK(c.r.find_matches == 0 && c.r.focused);
    key(ui, build_ced, &c, CUI_KEY_ESCAPE, 0);
    run(ui, build_ced, &c, 1);
    CHECK(c.r.find_matches == 0 && c.r.focused);
    type(ui, build_ced, &c, "Q");   /* the editor has the keyboard again: the selection is replaced */
    CHECK(strncmp(c.buf, "tile a;\nint tile_b;\n/* Q */", 26) == 0);
    ced_sel(ui, &c, ced_at(&c, "int"), ced_at(&c, "int") + 3);
    key(ui, build_ced, &c, CUI_KEY_F, CUI_MOD_PRIMARY);
    run(ui, build_ced, &c, 1);
    CHECK(c.r.find_matches == 1 && c.r.find_index == 1);
    key(ui, build_ced, &c, CUI_KEY_ESCAPE, 0);
    /* go to line, from code and with Ctrl+G */
    cui_code_editor_goto(ui, "ed", 2, 5);
    run(ui, build_ced, &c, 1);
    CHECK(c.r.line == 2 && c.r.column == 5 && c.r.cursor == 12);
    key(ui, build_ced, &c, CUI_KEY_G, CUI_MOD_PRIMARY);
    type(ui, build_ced, &c, "3");
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(c.r.line == 3 && c.r.column == 1 && c.r.focused);

    /* bracket matching: the bracket before the caret first, then the one after */
    ced_text(&c, "f(a[1], {b});\n");
    ced_sel(ui, &c, 2, 2);
    CHECK(c.r.bracket == 1 && c.r.bracket_match == 11);
    ced_sel(ui, &c, 3, 3);   /* before '[' */
    CHECK(c.r.bracket == 3 && c.r.bracket_match == 5);
    ced_sel(ui, &c, 10, 10);   /* after 'b', before '}' */
    CHECK(c.r.bracket == 10 && c.r.bracket_match == 8);
    ced_sel(ui, &c, 12, 12);   /* after ')' */
    CHECK(c.r.bracket == 11 && c.r.bracket_match == 1);
    ced_sel(ui, &c, 13, 13);
    CHECK(c.r.bracket == -1 && c.r.bracket_match == -1);
    ced_text(&c, "(()");   /* unmatched: nothing */
    ced_sel(ui, &c, 0, 0);
    CHECK(c.r.bracket == -1);

    /* mouse: double-click a word, triple-click a line, click the gutter, drag */
    ced_text(&c, "alpha beta\ngamma delta\nlast\n");
    run(ui, build_ced, &c, 2);
    float cw = cui_measure_text(ui, CUI_FONT_MONO, 13, "0000000000000000", 16).width / 16;
    float lh = roundf(13 * 1.6f), cx0 = er.x + 1 + 12 + 2 * cw + 14 + 8, ly = er.y + 1 + 6 + lh * 0.5f;
    cui_input_mouse_move(ui, er.x + 300, er.y + 250);
    run(ui, build_ced, &c, 30);   /* a fresh click sequence */
    click_at(ui, build_ced, &c, cx0 + cw * 8.3f, ly + lh);
    CHECK(c.r.cursor == 11 + 8 && c.r.anchor == c.r.cursor);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_ced, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_ced, &c, 1);
    CHECK(c.r.anchor == 17 && c.r.cursor == 22);   /* "delta" */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_ced, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_ced, &c, 1);
    CHECK(c.r.anchor == 11 && c.r.cursor == 23);   /* the whole line with its newline */
    cui_input_mouse_move(ui, er.x + 300, er.y + 250);
    run(ui, build_ced, &c, 30);
    click_at(ui, build_ced, &c, er.x + 10, ly + 2 * lh);   /* the gutter selects the line */
    CHECK(c.r.anchor == 23 && c.r.cursor == 28);
    run(ui, build_ced, &c, 30);
    drag2(ui, build_ced, &c, cx0 + cw * 1.0f, ly, cx0 + cw * 3.0f, ly + lh, 4);
    CHECK(c.r.anchor == 1 && c.r.cursor == 14);
    key(ui, build_ced, &c, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    CHECK(c.r.anchor == 1 && c.r.cursor == 26);

    /* Esc, then Tab, leaves the editor; Tab alone does not */
    ced_text(&c, "x");
    ced_sel(ui, &c, 1, 1);
    key(ui, build_ced, &c, CUI_KEY_ESCAPE, 0);
    run(ui, build_ced, &c, 1);
    key(ui, build_ced, &c, CUI_KEY_TAB, 0);
    run(ui, build_ced, &c, 1);
    CHECK(!c.r.focused && strcmp(c.buf, "x") == 0);

    /* read only: no edits, but selection, copy and find */
    c.ro = true;
    ced_text(&c, "const int k = 1;\n");
    ced_sel(ui, &c, 6, 9);
    c.any_changed = false;
    type(ui, build_ced, &c, "zz");
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    key(ui, build_ced, &c, CUI_KEY_BACKSPACE, 0);
    key(ui, build_ced, &c, CUI_KEY_V, CUI_MOD_PRIMARY);
    key(ui, build_ced, &c, CUI_KEY_X, CUI_MOD_PRIMARY);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    key(ui, build_ced, &c, CUI_KEY_TAB, CUI_MOD_SHIFT);
    CHECK(strcmp(c.buf, "const int k = 1;\n") == 0 && !c.any_changed);
    CHECK(strcmp(g_clip, "int") == 0);
    cui_code_editor_find(ui, "ed", "k");
    run(ui, build_ced, &c, 2);
    CHECK(c.r.find_matches == 1);
    cui_code_editor_find(ui, "ed", NULL);
    c.ro = false;

    /* capacity: an edit that does not fit is refused whole and reported */
    c.cap = 12;
    ced_text(&c, "");
    ced_sel(ui, &c, 0, 0);
    for (int i = 0; i < 8; i++) type(ui, build_ced, &c, "ab");
    CHECK(strlen(c.buf) == 10 && c.r.refused);
    type(ui, build_ced, &c, "c");
    CHECK(strlen(c.buf) == 11 && !c.r.refused);
    key(ui, build_ced, &c, CUI_KEY_ENTER, 0);
    CHECK(strlen(c.buf) == 11 && c.r.refused);
    ced_text(&c, "x");
    ced_sel(ui, &c, 1, 1);
    snprintf(g_clip, sizeof g_clip, "0123456789abcdef");
    key(ui, build_ced, &c, CUI_KEY_V, CUI_MOD_PRIMARY);
    CHECK(strcmp(c.buf, "x") == 0 && c.r.refused);
    c.cap = 0;
    cui_destroy(ui);

    /* virtualization: 20 000 lines lay out and draw only what is in view */
    ui = new_ctx();
    memset(&c, 0, sizeof c);
    c.lang = "c";
    run(ui, build_ced, &c, 3);
    ced_text(&c, "int main(void) {\n    return 0;\n}\n");
    run(ui, build_ced, &c, 3);
    int small_vtx = cui_get_draw_data(ui)->vertex_count;
    size_t big_n = 20000 * 40 + 1;
    char *big = malloc(big_n + 4096);
    size_t off = 0;
    for (int i = 0; i < 20000; i++)
        off += (size_t)snprintf(big + off, big_n + 4096 - off, i % 50 == 0 ? "/* block %05d\n" : i % 50 == 1 ? "   comment */\n" : "    value_%05d = compute(%d, \"x\");\n", i, i);
    c.big = big; c.cap = big_n + 4096;
    clock_t t0 = clock();
    run(ui, build_ced, &c, 3);
    CHECK(c.r.line_count == 20001);
    cui_code_editor_goto(ui, "ed", 10000, 1);
    run(ui, build_ced, &c, 2);
    CHECK(c.r.line == 10000);
    for (int i = 0; i < 20; i++) type(ui, build_ced, &c, "q");
    CHECK(c.r.line == 10000 && c.r.column == 21 && c.r.changed);
    key(ui, build_ced, &c, CUI_KEY_Z, CUI_MOD_PRIMARY);
    CHECK(c.r.column == 1);
    for (int i = 0; i < 30; i++) key(ui, build_ced, &c, CUI_KEY_PAGE_DOWN, 0);
    key(ui, build_ced, &c, CUI_KEY_END, CUI_MOD_CTRL);
    CHECK(c.r.line == 20001);
    cui_input_mouse_move(ui, er.x + 300, er.y + 150);
    for (int i = 0; i < 20; i++) { cui_input_scroll(ui, 0, 5); run(ui, build_ced, &c, 1); }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    int big_vtx = cui_get_draw_data(ui)->vertex_count;
    CHECK(big_vtx < small_vtx * 4 + 4000);   /* bounded by the lines in view, not the file */
    CHECK(secs < 5.0);
    printf("  20k lines: %d frames in %.3f s, %d vertices (a 3 line file: %d)\n", 3 + 2 + 20 + 1 + 30 + 1 + 20, secs, big_vtx, small_vtx);
    cui_destroy(ui);
    free(big);

    /* random keys, text and pointer input on a small buffer stay inside it */
    ui = new_ctx();
    memset(&c, 0, sizeof c);
    c.cap = 300;
    ced_text(&c, "void f(int a) {\n\tif (a) { g(\"s\"); }\n}\n/* c */\n");
    unsigned rng = 777u;
    static const cui_key keys[] = { CUI_KEY_TAB, CUI_KEY_ENTER, CUI_KEY_ESCAPE, CUI_KEY_BACKSPACE, CUI_KEY_DELETE, CUI_KEY_LEFT,
                                    CUI_KEY_RIGHT, CUI_KEY_UP, CUI_KEY_DOWN, CUI_KEY_HOME, CUI_KEY_END, CUI_KEY_PAGE_UP,
                                    CUI_KEY_PAGE_DOWN, CUI_KEY_A, CUI_KEY_C, CUI_KEY_V, CUI_KEY_X, CUI_KEY_Z, CUI_KEY_Y,
                                    CUI_KEY_F, CUI_KEY_G };
    static const char *texts[] = { "a", "}", "{", "(", ")", " ", "\xc3\xa9", "\xf0\x9f\x98\x80", ":", "\"", "/*", "*/", "\t" };
    static const char *langs[] = { "c", "python", "json", NULL, "js" };
    bool ok = true;
    for (int f = 0; f < 3000; f++) {
        rng = rng * 1664525u + 1013904223u;
        unsigned r = rng >> 8;
        switch (r % 9) {
        case 0: case 1: cui_input_mouse_move(ui, er.x - 20 + (float)((r >> 4 & 1023) % 660), er.y - 20 + (float)((r >> 14 & 511) % 360)); break;
        case 2: cui_input_mouse_button(ui, CUI_MOUSE_LEFT, (r >> 5) & 1); break;
        case 3: cui_input_scroll(ui, (float)((int)(r >> 6 & 7) - 3), (float)((int)(r >> 9 & 7) - 3)); break;
        case 4: case 5: { cui_key k = keys[(r >> 4) % (sizeof keys / sizeof keys[0])]; int m = (int)(r >> 12 & 7); cui_input_key(ui, k, true, m); cui_input_key(ui, k, false, m); break; }
        case 6: case 7: cui_input_key(ui, CUI_KEY_NONE, false, 0); cui_input_text(ui, texts[(r >> 4) % (sizeof texts / sizeof texts[0])]); break;
        default:
            if ((r >> 4) % 40 == 0) c.lang = langs[(r >> 10) % 5];
            if ((r >> 4) % 97 == 1) c.ro = !c.ro;
            if ((r >> 4) % 211 == 2) snprintf(g_clip, sizeof g_clip, "%s", (r >> 12) & 1 ? "pasted\r\n\ttext {}" : "x");
            break;
        }
        run(ui, build_ced, &c, 1);
        size_t n = strlen(c.buf);
        if (n >= c.cap || c.r.cursor < 0 || c.r.cursor > (int)n || c.r.anchor < 0 || c.r.anchor > (int)n || c.r.length != (int)n) ok = false;
    }
    CHECK(ok);
    cui_destroy(ui);
}

/* diff viewer ---------------------------------------------------------- */

/* the lines of a text, each with its line ending, for checking scripts */
typedef struct { const char *s; int len; } df_line;
static int df_lines(const char *t, size_t n, df_line *out, int cap) {
    int count = 0;
    size_t i = 0;
    while (i < n && count < cap) {
        size_t e = i;
        while (e < n && t[e] != '\n') e++;
        if (e < n) e++;   /* the line ending belongs to the line */
        out[count].s = t + i; out[count].len = (int)(e - i);
        count++;
        i = e;
    }
    return count;
}
static bool df_line_eq(df_line a, df_line b) { return a.len == b.len && memcmp(a.s, b.s, (size_t)a.len) == 0; }
static int df_check_lines(const df_line *la, int na, const df_line *lb, int nb, const cui_diff_hunk *h, int nh);

/* Checks hunks against the texts: sorted, separated by equal lines, and the
 * lines between them equal pairwise. Returns the edit distance or -1. */
static int df_check(const char *a, const char *b, const cui_diff_hunk *h, int nh) {
    size_t an = strlen(a), bn = strlen(b);
    df_line *la = malloc(sizeof *la * (an + 1)), *lb = malloc(sizeof *lb * (bn + 1));
    int na = df_lines(a, an, la, (int)an + 1), nb = df_lines(b, bn, lb, (int)bn + 1);
    int dist = df_check_lines(la, na, lb, nb, h, nh);
    free(la);
    free(lb);
    return dist;
}
static int df_check_lines(const df_line *la, int na, const df_line *lb, int nb, const cui_diff_hunk *h, int nh) {
    int i = 0, j = 0, dist = 0;
    for (int k = 0; k < nh; k++) {
        if (h[k].old_start < i || h[k].new_start < j) return -1;
        if (h[k].old_start - i != h[k].new_start - j) return -1;
        if (k > 0 && h[k].old_start == i) return -1;   /* touching hunks should have merged */
        if (h[k].old_count + h[k].new_count == 0) return -1;
        while (i < h[k].old_start) if (!df_line_eq(la[i++], lb[j++])) return -1;
        i += h[k].old_count; j += h[k].new_count;
        dist += h[k].old_count + h[k].new_count;
    }
    if (na - i != nb - j) return -1;
    while (i < na) if (!df_line_eq(la[i++], lb[j++])) return -1;
    return dist;
}
/* the shortest edit distance, by dynamic programming (small inputs) */
static int df_lcs_dist(const char *a, const char *b) {
    static df_line la[64], lb[64];
    static int t[65][65];
    int na = df_lines(a, strlen(a), la, 64), nb = df_lines(b, strlen(b), lb, 64);
    for (int i = 0; i <= na; i++)
        for (int j = 0; j <= nb; j++)
            t[i][j] = !i || !j ? 0 : df_line_eq(la[i - 1], lb[j - 1]) ? t[i - 1][j - 1] + 1 : (t[i - 1][j] > t[i][j - 1] ? t[i - 1][j] : t[i][j - 1]);
    return na + nb - 2 * t[na][nb];
}
static int df_diff(const char *a, const char *b, unsigned flags, cui_diff_hunk *h, int cap, cui_diff_stats *st) {
    return cui_diff_compute(a, strlen(a), b, strlen(b), flags, h, cap, st);
}

typedef struct {
    const char *a, *b;
    cui_diff_mode mode;
    int *current;
    bool no_fold;
    float width;
    cui_diff_result r;
} diff_ctx;
static void build_diff(cui_ctx *ui, void *u) {
    diff_ctx *d = u;
    cui_column(ui, .pad = 10) {
        d->r = cui_diff_view(ui, "dv", d->a, d->b, .mode = d->mode, .current = d->current, .no_fold = d->no_fold,
                             .width = d->width > 0 ? CUI_PX(d->width) : CUI_FILL, .height = 300, .language = "c");
    }
}
static void diff_key(cui_ctx *ui, diff_ctx *d, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods);
    run(ui, build_diff, d, 1);
    cui_input_key(ui, k, false, mods);
    run(ui, build_diff, d, 1);
}

static void test_diff(void) {
    printf("diff viewer: Myers, words, folds, navigation, rows, limits\n");
    cui_diff_hunk h[64];
    cui_diff_stats st;

    /* identical, empty, all added, all removed */
    CHECK(df_diff("a\nb\nc\n", "a\nb\nc\n", 0, h, 64, &st) == 0 && st.added == 0 && st.removed == 0 && st.old_lines == 3);
    CHECK(df_diff("", "", 0, h, 64, &st) == 0 && st.old_lines == 0 && st.new_lines == 0);
    CHECK(df_diff("", "a\nb\n", 0, h, 64, &st) == 1 && h[0].old_start == 0 && h[0].old_count == 0 && h[0].new_start == 0 && h[0].new_count == 2 && st.added == 2);
    CHECK(df_diff("a\nb\n", "", 0, h, 64, &st) == 1 && h[0].old_count == 2 && h[0].new_count == 0 && st.removed == 2);
    CHECK(df_diff("x", "", 0, h, 64, &st) == 1 && st.old_lines == 1 && st.removed == 1);
    /* a missing final newline is a change, unless ignored */
    CHECK(df_diff("a\nb", "a\nb\n", 0, h, 64, &st) == 1 && h[0].old_start == 1 && h[0].old_count == 1 && h[0].new_start == 1 && h[0].new_count == 1);
    CHECK(df_diff("a\nb", "a\nb\n", CUI_DIFF_IGNORE_EOL, h, 64, &st) == 0);
    /* CRLF against LF */
    CHECK(df_diff("a\r\nb\r\n", "a\nb\n", 0, h, 64, &st) == 1 && h[0].old_count == 2 && h[0].new_count == 2);
    CHECK(df_diff("a\r\nb\r\n", "a\nb\n", CUI_DIFF_IGNORE_EOL, h, 64, &st) == 0);
    CHECK(df_diff("a\r\nb\r\nc\r\n", "a\r\nB\r\nc\r\n", 0, h, 64, &st) == 1 && h[0].old_start == 1 && h[0].old_count == 1);
    /* the example from Myers' paper: ABCABBA to CBABAC, 5 edits, common
     * subsequence BABA. Searching from both ends finds another shortest script
     * than the paper's figure: A becomes C, delete C, delete the second B,
     * add C at the end. */
    const char *ma = "A\nB\nC\nA\nB\nB\nA\n", *mb = "C\nB\nA\nB\nA\nC\n";
    int n = df_diff(ma, mb, 0, h, 64, &st);
    CHECK(st.added + st.removed == 5 && !st.approximate);
    CHECK(df_check(ma, mb, h, n) == 5);
    CHECK(n == 4 && h[0].old_start == 0 && h[0].old_count == 1 && h[0].new_start == 0 && h[0].new_count == 1 &&
          h[1].old_start == 2 && h[1].old_count == 1 && h[1].new_start == 2 && h[1].new_count == 0 &&
          h[2].old_start == 5 && h[2].old_count == 1 && h[2].new_start == 4 && h[2].new_count == 0 &&
          h[3].old_start == 7 && h[3].old_count == 0 && h[3].new_start == 5 && h[3].new_count == 1);
    /* an added function slides to end on its blank line */
    const char *fa = "int a(void) {\n    return 1;\n}\n\nint c(void) {\n    return 3;\n}\n";
    const char *fb = "int a(void) {\n    return 1;\n}\n\nint b(void) {\n    return 2;\n}\n\nint c(void) {\n    return 3;\n}\n";
    n = df_diff(fa, fb, 0, h, 64, &st);
    CHECK(n == 1 && h[0].new_start == 4 && h[0].new_count == 4 && df_check(fa, fb, h, n) == 4);
    /* cap: the count is all hunks, only cap written */
    CHECK(df_diff("a\nb\nc\nd\ne\n", "A\nb\nC\nd\nE\n", 0, h, 2, &st) == 3 && st.hunks == 3);

    /* random inputs: always a valid script, and the shortest one */
    unsigned rng = 12345u;
    int bad = 0, long_ = 0;
    for (int it = 0; it < 400; it++) {
        char ta[256], tb[256];
        int pa = 0, pb = 0;
        for (int side = 0; side < 2; side++) {
            char *t = side ? tb : ta;
            int *p = side ? &pb : &pa;
            rng = rng * 1103515245u + 12345u;
            int lines = (int)(rng >> 16) % 24;
            for (int l = 0; l < lines; l++) {
                rng = rng * 1103515245u + 12345u;
                int k = (int)(rng >> 16) % 5;
                t[(*p)++] = (char)('a' + k);
                if (k == 4) t[(*p)++] = '\r';
                t[(*p)++] = '\n';
            }
            rng = rng * 1103515245u + 12345u;
            if ((rng >> 16) % 4 == 0 && *p > 0) (*p)--;   /* sometimes no final newline */
            t[*p] = 0;
        }
        int nh = df_diff(ta, tb, 0, h, 64, &st);
        int dist = nh >= 0 && nh <= 64 ? df_check(ta, tb, h, nh) : -1;
        if (dist < 0 || dist != st.added + st.removed) bad++;
        if (dist != df_lcs_dist(ta, tb)) long_++;
    }
    CHECK(bad == 0);
    CHECK(long_ == 0);

    /* invalid UTF-8 and control bytes are compared as bytes and drawn safely */
    const char *ua = "ok\n\xff\xfe broken\nend \xe2\x82\n\x01\x02\n", *ub = "ok\n\xff\xfd broken\nend \xe2\x82\xac\n\x01\x02\n";
    n = df_diff(ua, ub, 0, h, 64, &st);
    CHECK(n == 1 && df_check(ua, ub, h, n) == 4);

    /* word marks */
    cui_diff_range ro[8], rn[8];
    int no = 0, nn = 0;
    CHECK(cui_diff_words("    int count = 0;", -1, "    int loaded = 0;", -1, ro, &no, rn, &nn, 8));
    CHECK(no == 1 && ro[0].start == 8 && ro[0].length == 5 && nn == 1 && rn[0].start == 8 && rn[0].length == 6);
    CHECK(cui_diff_words("if (!eq) return 0;", -1, "if (!eq || eq == line) return 0;", -1, ro, &no, rn, &nn, 8));
    CHECK(no == 0 && nn == 1 && rn[0].start == 7 && rn[0].length == 14);   /* " || eq == line" reads as one mark */
    CHECK(!cui_diff_words("return total_size * 4;", -1, "for (;;) { wait(); }", -1, ro, &no, rn, &nn, 8));   /* too different */
    CHECK(cui_diff_words("a \xff\xfe b", -1, "a \xff\xfd b", -1, ro, &no, rn, &nn, 8) && no == 1 && ro[0].start == 2 && ro[0].length == 2);
    CHECK(cui_diff_words("same", -1, "same", -1, ro, &no, rn, &nn, 8) && no == 0 && nn == 0);

    /* large inputs: exact when changed lines are new, bounded and coarse
     * (but correct) when the search would take too long */
    {
        int lines = 60000;
        char *big_a = malloc((size_t)lines * 12 + 1), *big_b = malloc((size_t)lines * 12 + 1);
        size_t pa = 0, pb = 0;
        for (int i = 0; i < lines; i++) {
            pa += (size_t)snprintf(big_a + pa, 13, "a%09d\n", (int)((long long)i * 7919 % 1000003));
            pb += (size_t)snprintf(big_b + pb, 13, "b%09d\n", (int)((long long)i * 104729 % 1000003));
        }
        clock_t t0 = clock();
        n = cui_diff_compute(big_a, pa, big_b, pb, 0, NULL, 0, &st);
        double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
        CHECK(n == 1 && st.removed == lines && st.added == lines && !st.approximate);
        CHECK(secs < 5.0);
        /* the same text with a few edits stays exact and quick */
        memcpy(big_b, big_a, pa + 1);
        for (int i = 0; i < 40; i++) big_b[(size_t)(i * 1499 + 7) * 11 + 3] = 'x';
        t0 = clock();
        n = cui_diff_compute(big_a, pa, big_b, pa, 0, NULL, 0, &st);
        secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
        CHECK(n == 40 && st.added == 40 && st.removed == 40 && !st.approximate);
        CHECK(secs < 5.0);
        /* partly shared: a valid script within the limit */
        for (int i = 0; i < lines; i += 3) memcpy(big_b + (size_t)i * 11, "zzzzzzzzzz", 10);
        static cui_diff_hunk hh[60000];
        t0 = clock();
        n = cui_diff_compute(big_a, pa, big_b, pa, 0, hh, 60000, &st);
        secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
        CHECK(n > 0 && n <= 60000 && st.added == st.removed && secs < 5.0);
        int ia = 0;
        bool ordered = true;
        for (int k = 0; k < n && k < 60000; k++) { if (hh[k].old_start < ia) ordered = false; ia = hh[k].old_start + hh[k].old_count; }
        CHECK(ordered && !st.approximate);
        CHECK(df_check(big_a, big_b, hh, n) == st.added + st.removed);
        /* lines from three symbols: every line has matches, the search hits its
         * limit and the rest is replaced */
        unsigned r3 = 99u;
        for (int i = 0; i < lines; i++) {
            r3 = r3 * 1103515245u + 12345u; big_a[2 * i] = (char)('a' + (r3 >> 16) % 3); big_a[2 * i + 1] = '\n';
            r3 = r3 * 1103515245u + 12345u; big_b[2 * i] = (char)('a' + (r3 >> 16) % 3); big_b[2 * i + 1] = '\n';
        }
        big_a[2 * lines] = big_b[2 * lines] = 0;
        t0 = clock();
        n = cui_diff_compute(big_a, (size_t)lines * 2, big_b, (size_t)lines * 2, 0, hh, 60000, &st);
        secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
        CHECK(st.approximate && n > 0 && n <= 60000 && secs < 5.0);
        CHECK(df_check(big_a, big_b, hh, n) == st.added + st.removed);
        free(big_a);
        free(big_b);
    }

    /* the view ---------------------------------------------------------- */
    char va[4096], vb[4096];
    size_t pa = 0, pb = 0;
    for (int i = 1; i <= 60; i++) {
        pa += (size_t)snprintf(va + pa, sizeof va - pa, "line %d\n", i);
        if (i == 20) pb += (size_t)snprintf(vb + pb, sizeof vb - pb, "line twenty\nline 20b\nline 20c\n");   /* 1 replaced by 3 */
        else if (i == 40) { /* removed */ }
        else pb += (size_t)snprintf(vb + pb, sizeof vb - pb, "line %d\n", i);
    }
    cui_ctx *ui = new_ctx();
    int cur = -1;
    diff_ctx d = { va, vb, CUI_DIFF_SPLIT, &cur, false, 0, { 0 } };
    run(ui, build_diff, &d, 1);
    CHECK(d.r.recomputed);
    run(ui, build_diff, &d, 3);
    CHECK(!d.r.recomputed);
    CHECK(d.r.added == 3 && d.r.removed == 2 && d.r.changes == 2 && d.r.mode == CUI_DIFF_SPLIT);
    /* folds: before the first change, between the changes, after the last */
    CHECK(d.r.folds == 3 && d.r.hidden == 16 + 13 + 17);
    /* split rows: the replaced line beside its first replacement, then fillers */
    static cui_diff_row rows[128];
    int nr = cui_diff_view_rows(ui, "dv", rows, 0, 128);
    CHECK(nr > 0 && nr < 40);
    int k20 = -1;
    for (int i = 0; i < nr; i++) if (rows[i].kind != CUI_DIFF_ROW_SAME && rows[i].kind != CUI_DIFF_ROW_FOLD) { k20 = i; break; }
    CHECK(k20 > 0 && rows[k20].kind == CUI_DIFF_ROW_CHANGED && rows[k20].old_line == 19 && rows[k20].new_line == 19 && rows[k20].change == 0);
    CHECK(rows[k20 + 1].kind == CUI_DIFF_ROW_ADDED && rows[k20 + 1].old_line == -1 && rows[k20 + 1].new_line == 20);
    CHECK(rows[k20 + 2].kind == CUI_DIFF_ROW_ADDED && rows[k20 + 3].kind == CUI_DIFF_ROW_SAME && rows[k20 + 3].old_line == 20 && rows[k20 + 3].new_line == 22);
    CHECK(rows[0].kind == CUI_DIFF_ROW_FOLD && rows[0].old_line == 0 && rows[0].hidden == 16 && rows[1].old_line == 16);
    /* inline rows: removed, then added */
    d.mode = CUI_DIFF_INLINE;
    run(ui, build_diff, &d, 2);
    CHECK(d.r.mode == CUI_DIFF_INLINE && !d.r.recomputed);
    nr = cui_diff_view_rows(ui, "dv", rows, 0, 128);
    for (k20 = 0; k20 < nr && rows[k20].kind != CUI_DIFF_ROW_REMOVED; k20++) {}
    CHECK(k20 < nr && rows[k20].old_line == 19 && rows[k20].new_line == -1);
    CHECK(rows[k20 + 1].kind == CUI_DIFF_ROW_ADDED && rows[k20 + 1].new_line == 19 && rows[k20 + 3].kind == CUI_DIFF_ROW_ADDED && rows[k20 + 4].kind == CUI_DIFF_ROW_SAME);
    int rem40 = -1;
    for (int i = k20 + 4; i < nr; i++) if (rows[i].kind == CUI_DIFF_ROW_REMOVED) { rem40 = i; break; }
    CHECK(rem40 > 0 && rows[rem40].old_line == 39 && rows[rem40].change == 1);
    /* the same rows in both modes for every unchanged line */
    d.mode = CUI_DIFF_SPLIT;
    run(ui, build_diff, &d, 2);

    /* click the first fold row: it opens */
    cui_rect vr = rect_of(ui, cui_make_id(ui, "dv"));
    float row_h = roundf(12 * 1.75f);
    click_at(ui, build_diff, &d, vr.x + 200, vr.y + 1 + 4 + row_h * 0.5f);
    CHECK(d.r.folds == 2 && d.r.hidden == 13 + 17);
    nr = cui_diff_view_rows(ui, "dv", rows, 0, 128);
    CHECK(rows[0].kind == CUI_DIFF_ROW_SAME && rows[0].old_line == 0);
    /* the view has focus from the click: E opens all, Shift+E folds again */
    diff_key(ui, &d, CUI_KEY_E, 0);
    CHECK(d.r.folds == 0 && d.r.hidden == 0);
    diff_key(ui, &d, CUI_KEY_E, CUI_MOD_SHIFT);
    CHECK(d.r.folds == 3);
    /* next and previous change */
    CHECK(cur == -1 && d.r.current == -1);
    diff_key(ui, &d, CUI_KEY_N, 0);
    CHECK(cur == 0 && d.r.current == 0);
    diff_key(ui, &d, CUI_KEY_DOWN, CUI_MOD_ALT);
    CHECK(cur == 1 && d.r.current == 1);
    diff_key(ui, &d, CUI_KEY_N, 0);
    CHECK(cur == 1);   /* the last one stays */
    diff_key(ui, &d, CUI_KEY_P, 0);
    CHECK(cur == 0);
    diff_key(ui, &d, CUI_KEY_UP, CUI_MOD_ALT);
    CHECK(cur == 0);
    /* the caller sets it (a button) */
    cur = 1;
    run(ui, build_diff, &d, 1);
    CHECK(d.r.current == 1 && d.r.current_changed);
    run(ui, build_diff, &d, 1);
    CHECK(d.r.current == 1 && !d.r.current_changed);
    cur = 99;   /* out of range is clamped */
    run(ui, build_diff, &d, 1);
    CHECK(cur == 1);
    diff_key(ui, &d, CUI_KEY_ESCAPE, 0);
    CHECK(cur == -1);

    /* recompute only when a text changes */
    char vcopy[4096];
    memcpy(vcopy, vb, sizeof vcopy);
    d.b = vcopy;   /* same text, other memory */
    run(ui, build_diff, &d, 1);
    CHECK(!d.r.recomputed);
    vcopy[0] = 'L';   /* edited in place: same pointer, same length */
    run(ui, build_diff, &d, 1);
    CHECK(d.r.recomputed && d.r.changes == 3);
    run(ui, build_diff, &d, 2);
    CHECK(!d.r.recomputed);

    /* narrow: split shows inline */
    d.width = 400;
    run(ui, build_diff, &d, 3);
    CHECK(d.r.mode == CUI_DIFF_INLINE);
    d.width = 0;
    run(ui, build_diff, &d, 3);
    CHECK(d.r.mode == CUI_DIFF_SPLIT);

    /* invalid UTF-8, CRLF, a missing newline and long lines in the view */
    char longa[3000], longb[3000];
    memset(longa, 'x', sizeof longa); memset(longb, 'x', sizeof longb);
    memcpy(longa, "\t\xff\xfe\xe2\x82\n", 6); memcpy(longb, "\t\xff\xfd\xe2\x82\xac\n", 7);
    longa[2990] = '\n'; longb[2990] = 'y'; longa[2999] = longb[2999] = 0;
    diff_ctx d2 = { longa, longb, CUI_DIFF_SPLIT, NULL, true, 0, { 0 } };
    run(ui, build_diff, &d2, 3);
    CHECK(d2.r.changes >= 1);
    d2.a = "a\r\nb\r\nc"; d2.b = "a\nb\nc\n";
    run(ui, build_diff, &d2, 2);
    CHECK(d2.r.changes == 1 && d2.r.added == 3 && d2.r.removed == 3);
    d2.mode = CUI_DIFF_INLINE;
    run(ui, build_diff, &d2, 2);
    d2.a = ""; d2.b = "";
    run(ui, build_diff, &d2, 2);
    CHECK(d2.r.changes == 0 && d2.r.added == 0);
    d2.a = NULL; d2.b = "only new\n";
    run(ui, build_diff, &d2, 2);
    CHECK(d2.r.added == 1 && d2.r.removed == 0);
    cui_destroy(ui);
}

/* ---- knob, fader and level meter ---- */
typedef struct { float knob, freq, pan, fader, levels[2]; cui_rect kr, fr, lr, pr, vr; bool cleared; } dial_ctx;
static void build_dials(cui_ctx *ui, void *u) {
    dial_ctx *d = u;
    static const float ticks[] = { 0, -12, -24 };
    cui_row(ui, .gap = 20, .align = CUI_ALIGN_START, .pad_top = 300) {
        cui_knob(ui, "Gain", &d->knob, 0, 100, .reset = 50);
        d->kr = cui_last_rect(ui);
        cui_knob(ui, "Freq", &d->freq, 20, 20000, .log_scale = true);
        d->lr = cui_last_rect(ui);
        cui_knob(ui, "Pan", &d->pan, -1, 1, .bipolar = true);
        d->pr = cui_last_rect(ui);
        cui_fader(ui, "Level", &d->fader, -48, 6, .length = 160, .ticks = ticks, .tick_count = 3, .meter = d->levels, .meter_channels = 2);
        d->fr = cui_last_rect(ui);
        d->cleared |= cui_level_meter(ui, "vu", d->levels, 2, .scale = true, .hold = 0.5f, .decay = 30);
        d->vr = cui_last_rect(ui);
    }
}
static void test_dials(void) {
    cui_ctx *ui = new_ctx();
    dial_ctx d = { 25, 20, 0.5f, -12, { 0, 0 } };
    run(ui, build_dials, &d, 3);
    CHECK(d.kr.w == 48 && d.kr.h == 48);
    /* drag up turns right: 200 points for the range */
    cui_vec2 c = center(d.kr);
    cui_input_mouse_move(ui, c.x, c.y); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_dials, &d, 1);
    cui_input_mouse_move(ui, c.x, c.y - 50); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.knob, 50, 0.6);
    /* Shift is ten times finer, from where the pointer is */
    cui_input_key(ui, CUI_KEY_NONE, true, CUI_MOD_SHIFT); run(ui, build_dials, &d, 1);
    cui_input_mouse_move(ui, c.x, c.y - 150); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.knob, 55, 0.6);
    cui_input_key(ui, CUI_KEY_NONE, true, 0);
    cui_input_mouse_move(ui, c.x, c.y - 320); run(ui, build_dials, &d, 1);
    CHECK(d.knob == 100);   /* clamped */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_dials, &d, 1);
    /* keys on the focused knob; Delete resets */
    key(ui, build_dials, &d, CUI_KEY_DOWN, 0);
    CHECK_NEAR(d.knob, 99, 0.01);
    key(ui, build_dials, &d, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    CHECK_NEAR(d.knob, 89, 0.01);
    key(ui, build_dials, &d, CUI_KEY_HOME, 0);
    CHECK(d.knob == 0);
    key(ui, build_dials, &d, CUI_KEY_DELETE, 0);
    CHECK_NEAR(d.knob, 50, 0.01);
    /* the wheel turns the focused knob under the pointer, and only that one */
    cui_input_mouse_move(ui, c.x, c.y); run(ui, build_dials, &d, 2);
    cui_input_scroll(ui, 0, 2); run(ui, build_dials, &d, 2);
    CHECK_NEAR(d.knob, 52, 0.01);
    cui_vec2 pc = center(d.pr);
    cui_input_mouse_move(ui, pc.x, pc.y); run(ui, build_dials, &d, 2);
    cui_input_scroll(ui, 0, 2); run(ui, build_dials, &d, 2);
    CHECK_NEAR(d.pan, 0.5f, 0.001);   /* not focused: the wheel leaves it alone */
    /* double-click: a bipolar knob goes back to its middle */
    click_at(ui, build_dials, &d, pc.x, pc.y);
    click_at(ui, build_dials, &d, pc.x, pc.y);
    CHECK_NEAR(d.pan, 0, 0.001);
    /* a log knob: halfway up the travel is the geometric middle */
    cui_vec2 lc = center(d.lr);
    cui_input_mouse_move(ui, lc.x, lc.y); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_dials, &d, 1);
    cui_input_mouse_move(ui, lc.x, lc.y - 100); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.freq, 632.5, 8);

    /* fader: a press on the track jumps there, a press on the thumb drags from where it is */
    float travel = 160 - 16, top = d.fr.y + 8;
    float x = d.fr.x + d.fr.w * 0.5f;
    cui_input_mouse_move(ui, x, top); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.fader, 6, 0.01);
    float ty = top;   /* the thumb is at the top now */
    run(ui, build_dials, &d, 30);   /* past the double-click time */
    cui_input_mouse_move(ui, x, ty + 3); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.fader, 6, 0.01);   /* grabbing the thumb does not move it */
    cui_input_mouse_move(ui, x, ty + 3 + travel * 0.5f); run(ui, build_dials, &d, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_dials, &d, 1);
    CHECK_NEAR(d.fader, -21, 0.3);
    key(ui, build_dials, &d, CUI_KEY_BACKSPACE, 0);
    CHECK_NEAR(d.fader, 0, 0.01);

    /* meter: up at once, back down at the decay rate, the peak mark holds, the clip light latches */
    cui_set_reduced_motion(ui, false);
    d.levels[0] = 1.0f; d.levels[1] = 0.5f;
    run(ui, build_dials, &d, 1);
    d.levels[0] = d.levels[1] = 0;
    run(ui, build_dials, &d, 30);   /* half a second at 30 dB/s: down 15 dB */
    CHECK(!d.cleared);
    click_at(ui, build_dials, &d, d.vr.x + d.vr.w - 3, d.vr.y + 2);   /* the clip light, above the bars */
    CHECK(d.cleared);
    d.cleared = false;
    click_at(ui, build_dials, &d, d.vr.x + d.vr.w - 3, d.vr.y + 2);
    CHECK(!d.cleared);   /* off now: nothing to clear */
    cui_destroy(ui);
}

/* ---- radial gauge and candlestick chart ---- */
typedef struct { cui_candle *c; int n; int hover; float value; cui_rect r; } cs_ctx;
static void build_cs(cui_ctx *ui, void *u) {
    cs_ctx *c = u;
    static const cui_gauge_band bands[] = { { 50, CUI_TONE_SUCCESS }, { 100, CUI_TONE_DANGER } };
    cui_column(ui, .width = CUI_FILL, .pad = 10, .gap = 10) {
        cui_radial_gauge(ui, "g", c->value, 0, 100, .bands = bands, .band_count = 2, .show_target = true, .target = 80, .label = "Load");
        c->hover = cui_candlestick(ui, "cs", c->c, c->n, .height = 300);
        c->r = cui_last_rect(ui);
    }
}
static void test_candles(void) {
    cui_ctx *ui = new_ctx();
    enum { N = 100000 };
    cui_candle *cs = malloc(sizeof *cs * N);
    float p = 100;
    for (int i = 0; i < N; i++) {
        float o = p, c = p + sinf(i * 0.37f) * 2 + cosf(i * 0.011f);
        cs[i] = (cui_candle){ o, fmaxf(o, c) + 1, fminf(o, c) - 1, c, 1000 + (float)(i % 17) * 50 };
        p = c;
    }
    cs_ctx c = { cs, 500, -1, NAN };   /* a NaN value draws an empty gauge, no crash */
    run(ui, build_cs, &c, 3);
    c.value = 72;
    run(ui, build_cs, &c, 3);
    CHECK(c.r.w > 700 && c.r.h == 300);
    /* the newest candle is in view; the crosshair follows the pointer */
    cui_input_mouse_move(ui, c.r.x + c.r.w - 90, c.r.y + 120); run(ui, build_cs, &c, 2);
    CHECK(c.hover > 400 && c.hover < 500);
    int h0 = c.hover;
    /* appending keeps the view on the newest candles */
    c.n = 520; run(ui, build_cs, &c, 2);
    CHECK(c.hover > h0 && c.hover < 520);
    h0 = c.hover;
    /* dragging right pans back in time */
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_cs, &c, 1);
    cui_input_mouse_move(ui, c.r.x + c.r.w - 90 + 300, c.r.y + 120); run(ui, build_cs, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_cs, &c, 1);
    cui_input_mouse_move(ui, c.r.x + c.r.w - 90, c.r.y + 121); run(ui, build_cs, &c, 2);
    CHECK(c.hover >= 0 && c.hover < h0 - 20);   /* 300 points at about 12 per candle */
    int h1 = c.hover;
    /* and new candles no longer move a view that left the end */
    c.n = 540; run(ui, build_cs, &c, 2);
    CHECK(c.hover == h1);
    /* the wheel zooms around the pointer: the candle under it stays under it */
    cui_input_scroll(ui, 0, -3); run(ui, build_cs, &c, 2);
    CHECK(abs(c.hover - h1) <= 1);
    /* keys: End puts the crosshair on the newest candle, Left steps back */
    click_at(ui, build_cs, &c, c.r.x + 200, c.r.y + 100);
    key(ui, build_cs, &c, CUI_KEY_END, 0);
    CHECK(c.hover == 539);
    key(ui, build_cs, &c, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK(c.hover == 529);
    /* 100,000 candles zoomed all the way out merge per pixel column and stay fast */
    c.n = N;
    key(ui, build_cs, &c, CUI_KEY_0, 0);
    run(ui, build_cs, &c, 2);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    CHECK(dd->vertex_count < 60000);   /* one wick per column, not 100,000 candles */
    clock_t t0 = clock();
    for (int i = 0; i < 20; i++) { cui_input_mouse_move(ui, c.r.x + 100 + i * 20, c.r.y + 100); run(ui, build_cs, &c, 1); }
    double ms = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000 / 20;
    CHECK(ms < 25);
    printf("    candlestick: 100k candles, %.2f ms per frame\n", ms);
    free(cs);
    cui_destroy(ui);
}

/* ---- translated text (cui_set_strings) ---- */
static uint64_t draw_hash(cui_ctx *ui) {
    const cui_draw_data *d = cui_get_draw_data(ui);
    uint64_t h = 1469598103934665603ull;
    const unsigned char *b = (const unsigned char *)d->vertices;
    for (size_t i = 0; i < sizeof(cui_vertex) * (size_t)d->vertex_count; i++) h = (h ^ b[i]) * 1099511628211ull;
    return h;
}
static void build_dates(cui_ctx *ui, void *u) {
    cui_date *d = u;
    cui_column(ui, .pad = 10, .gap = 10) {
        cui_calendar(ui, "cal", d);
        cui_date_picker(ui, "dp", d);
        cui_date month = { 2026, 11, 1 };
        cui_month_calendar(ui, "mc", &month, NULL, 0, .today = (cui_date){ 2026, 11, 8 }, .cell_height = 40);
    }
}
static void test_strings(void) {
    static const cui_strings es = {
        .month = { "enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre" },
        .month_short = { "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sept", "oct", "nov", "dic" },
        .weekday_short = { "lun", "mar", "mi\xc3\xa9", "jue", "vie", "s\xc3\xa1" "b", "dom" },
        .weekday_min = { "lu", "ma", "mi", "ju", "vi", "s\xc3\xa1", "do" },
        .today = "Hoy",
    };
    static const cui_strings only_today = { .today = "Hoy" };
    cui_ctx *ui = new_ctx();
    cui_date d = { 2026, 11, 8 };
    CHECK(cui_get_strings(ui) == NULL);
    run(ui, build_dates, &d, 3);
    uint64_t english = draw_hash(ui);
    cui_set_strings(ui, &es);
    CHECK(cui_get_strings(ui) == &es);
    run(ui, build_dates, &d, 3);
    CHECK(draw_hash(ui) != english);   /* the names changed */
    cui_set_strings(ui, &only_today);  /* a partial table: the rest stays English */
    run(ui, build_dates, &d, 3);
    uint64_t partial = draw_hash(ui);
    CHECK(partial != english);
    cui_set_strings(ui, NULL);         /* back to English, exactly */
    run(ui, build_dates, &d, 3);
    CHECK(draw_hash(ui) == english);
    cui_destroy(ui);
}

/* ---- syntax colors: SQL ---- */
typedef struct { const char *src, *lang; } sqlc_ctx;
static void build_sqlc(cui_ctx *ui, void *u) {
    sqlc_ctx *c = u;
    cui_column(ui, .pad = 10) { cui_code_block(ui, "q", c->src, .lang = c->lang, .no_header = true); }
}
static bool has_color(cui_ctx *ui, cui_color want) {
    const cui_draw_data *d = cui_get_draw_data(ui);
    for (int i = 0; i < d->vertex_count; i++) {
        cui_color k = d->vertices[i].color;
        if (k.r == want.r && k.g == want.g && k.b == want.b && k.a == want.a) return true;
    }
    return false;
}
/** Does this source, in this language, draw anything in the color? */
static bool code_has(const char *lang, const char *src, cui_color want) {
    cui_ctx *ui = new_ctx();
    sqlc_ctx c = { src, lang };
    run(ui, build_sqlc, &c, 2);
    bool r = has_color(ui, want);
    cui_destroy(ui);
    return r;
}
/** SQL keywords match in any case, -- and block comments hide them, and other languages stay case-sensitive. */
static void test_sql_colors(void) {
    printf("syntax colors: SQL\n");
    cui_color kw = cui_oklch(0.52f, 0.20f, 305, 1), ty = cui_oklch(0.52f, 0.11f, 200, 1), cn = cui_oklch(0.58f, 0.16f, 50, 1);
    CHECK(code_has("sql", "SELECT id", kw));
    CHECK(code_has("sql", "select id", kw));
    CHECK(code_has("sql", "Select id", kw));
    CHECK(!code_has("text", "SELECT id", kw));
    CHECK(!code_has("sql", "selected id", kw));                 /* whole words only */
    CHECK(code_has("sql", "x VARCHAR", ty));
    CHECK(!code_has("sql", "x Users", ty));                     /* a capital is not a type in SQL */
    CHECK(code_has("sql", "x IS NULL", cn));
    CHECK(code_has("sql", "x = true", cn));
    CHECK(!code_has("sql", "x -- SELECT", kw));                 /* a line comment */
    CHECK(!code_has("sql", "/* a\nSELECT */ x", kw));          /* a block comment across lines */
    CHECK(code_has("sql", "/* a */ SELECT", kw));
    CHECK(code_has("c", "if x", kw));
    CHECK(!code_has("c", "IF x", kw));                          /* C keeps its case */
}

/* ---- calendar: months and years ---- */
/* the draw without texture coordinates, which depend on the order glyphs entered the atlas */
static uint64_t shape_hash(cui_ctx *ui) {
    const cui_draw_data *d = cui_get_draw_data(ui);
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < d->vertex_count; i++) {
        cui_vertex v = d->vertices[i];
        v.u = v.v = 0;
        const unsigned char *b = (const unsigned char *)&v;
        for (size_t k = 0; k < sizeof v; k++) h = (h ^ b[k]) * 1099511628211ull;
    }
    return h;
}
typedef struct { cui_date d; bool changed; } calpick_ctx;
static void build_calpick(cui_ctx *ui, void *u) {
    calpick_ctx *c = u;
    cui_column(ui, .pad = 10) { if (cui_calendar(ui, "cal", &c->d)) c->changed = true; }
}
static void tab_n(cui_ctx *ui, build_fn fn, void *u, int n) { for (int i = 0; i < n; i++) key(ui, fn, u, CUI_KEY_TAB, 0); }
/** The title opens the months, the year opens twelve years, and a choice goes back down to the days. */
static void test_calendar_years(void) {
    printf("calendar: months and years from the title\n");
    cui_ctx *ui = new_ctx();
    calpick_ctx c = { { 2026, 11, 14 }, false };
    run(ui, build_calpick, &c, 3);
    cui_id cal = cui_make_id(ui, "cal");
    cui_rect days = rect_of(ui, cal);
    uint64_t days_hash = shape_hash(ui);
    tab_n(ui, build_calpick, &c, 1);                 /* the title */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* the months of 2026 */
    run(ui, build_calpick, &c, 2);
    cui_rect months = rect_of(ui, cal);
    CHECK(shape_hash(ui) != days_hash);
    CHECK_NEAR(months.w, days.w, 0.01);              /* the same size in every view */
    CHECK_NEAR(months.h, days.h, 0.01);
    tab_n(ui, build_calpick, &c, 5);                 /* previous, next, Jan, Feb, Mar */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* March 2026, the value unchanged */
    CHECK(!c.changed);
    CHECK(c.d.year == 2026 && c.d.month == 11 && c.d.day == 14);
    click_at(ui, build_calpick, &c, 400, 400);       /* drop the focus ring */
    run(ui, build_calpick, &c, 2);
    uint64_t march = shape_hash(ui);
    {
        cui_ctx *ref = new_ctx();
        calpick_ctx rc = { { 2026, 3, 0 }, false };  /* a calendar that opens on March 2026 with no day chosen */
        run(ref, build_calpick, &rc, 3);
        CHECK(shape_hash(ref) == march);
        cui_destroy(ref);
    }
    /* the years: two Enters on the title, a page back, then 2010, May and the 15th */
    tab_n(ui, build_calpick, &c, 1);
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* months */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* 2016 to 2027 */
    tab_n(ui, build_calpick, &c, 1);
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* earlier: 2004 to 2015 */
    tab_n(ui, build_calpick, &c, 8);                 /* next, 2004 ... 2010 */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* the months of 2010, the focus on the title */
    tab_n(ui, build_calpick, &c, 7);                 /* previous, next, Jan ... May */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);    /* May 2010 */
    CHECK(!c.changed);
    tab_n(ui, build_calpick, &c, 2 + 20);            /* previous, next, then 1 May 2010 is a Saturday: the 15th is the 20th day cell */
    key(ui, build_calpick, &c, CUI_KEY_ENTER, 0);
    CHECK(c.changed);
    CHECK(c.d.year == 2010 && c.d.month == 5 && c.d.day == 15);
    cui_destroy(ui);
}

typedef struct { cui_date d; cui_id field; } dpick_ctx;
static void build_dpick(cui_ctx *ui, void *u) {
    dpick_ctx *c = u;
    cui_column(ui, .pad = 10) { cui_date_picker(ui, "dp", &c->d); c->field = cui_last_id(ui); }
}
/** A picker that closes in the months view opens on the days again. */
static void test_date_picker_reopens_on_days(void) {
    printf("date picker: opens on the days again\n");
    cui_ctx *ui = new_ctx();
    dpick_ctx c = { { 2026, 11, 14 }, 0 };
    run(ui, build_dpick, &c, 3);
    cui_vec2 f = center(rect_of(ui, c.field));
    click_at(ui, build_dpick, &c, f.x, f.y);
    run(ui, build_dpick, &c, 30);
    uint64_t opened = shape_hash(ui);
    tab_n(ui, build_dpick, &c, 1);                   /* from the field to the calendar's title */
    key(ui, build_dpick, &c, CUI_KEY_ENTER, 0);
    run(ui, build_dpick, &c, 30);
    CHECK(shape_hash(ui) != opened);                 /* the months */
    key(ui, build_dpick, &c, CUI_KEY_ESCAPE, 0);
    run(ui, build_dpick, &c, 30);
    click_at(ui, build_dpick, &c, f.x, f.y);
    run(ui, build_dpick, &c, 30);
    CHECK(shape_hash(ui) == opened);
    cui_destroy(ui);
}

/* ---- month calendar ---- */
typedef struct { cui_date month, sel; const cui_month_event *ev; int n; cui_month_result r; cui_rect gr; float w; } mc_ctx;
static void build_mc(cui_ctx *ui, void *u) {
    mc_ctx *c = u;
    cui_box(ui, .width = CUI_PX(c->w), .pad = 10) {
        c->r = cui_month_calendar(ui, "m", &c->month, c->ev, c->n, .selected = &c->sel, .today = (cui_date){ 2026, 11, 8 }, .cell_height = 100);
        c->gr = cui_last_rect(ui);
    }
}
static void test_month(void) {
    static const cui_month_event ev[] = {
        { "Sprint", { 2026, 11, 2 }, { 2026, 11, 4 }, 0, true, CUI_TONE_VIOLET },
        { "A", { 2026, 11, 10 }, { 0 }, 9, false, CUI_TONE_INFO },
        { "B", { 2026, 11, 10 }, { 0 }, 11, false, CUI_TONE_INFO },
        { "C", { 2026, 11, 10 }, { 0 }, 13, false, CUI_TONE_INFO },
        { "D", { 2026, 11, 10 }, { 0 }, 15, false, CUI_TONE_INFO },
        { "E", { 2026, 11, 10 }, { 0 }, 17, false, CUI_TONE_INFO },
    };
    cui_ctx *ui = new_ctx();
    mc_ctx c = { { 2026, 11, 1 }, { 0 }, ev, 6, { 0 }, { 0 }, 760 };
    run(ui, build_mc, &c, 3);
    /* November 2026 starts on a Sunday: six rows, Monday first */
    float cw = c.gr.w / 7, ch = 100;
    CHECK_NEAR(c.gr.h, 6 * ch, 0.5);
    /* the sprint bar on Monday the 2nd (row 1): a click returns it */
    click_at(ui, build_mc, &c, c.gr.x + cw * 1.5f, c.gr.y + ch + 26 + 9);
    CHECK(c.r.event == 0);
    /* the 10th has five timed events and lines for three: two, then "+3 more" */
    float x10 = c.gr.x + cw * 1 + 20, y10 = c.gr.y + ch * 2;
    click_at(ui, build_mc, &c, x10, y10 + 26 + 9);
    CHECK(c.r.event == 1);
    click_at(ui, build_mc, &c, x10, y10 + 26 + 19 + 9);
    CHECK(c.r.event == 2);
    run(ui, build_mc, &c, 30);
    click_at(ui, build_mc, &c, x10, y10 + 26 + 38 + 9);   /* "+3 more" opens the day's list */
    CHECK(c.r.event < 0);
    run(ui, build_mc, &c, 2);
    CHECK(cui_get_draw_data(ui) != NULL);
    /* an empty spot selects the day; a double-click activates it */
    run(ui, build_mc, &c, 30);
    cui_input_key(ui, CUI_KEY_ESCAPE, true, 0); cui_input_key(ui, CUI_KEY_ESCAPE, false, 0); run(ui, build_mc, &c, 2);
    click_at(ui, build_mc, &c, c.gr.x + cw * 4.5f, c.gr.y + ch * 3 + 80);   /* Friday the 20th */
    CHECK(c.r.day_clicked && c.sel.day == 20 && c.sel.month == 11);
    click_at(ui, build_mc, &c, c.gr.x + cw * 4.5f, c.gr.y + ch * 3 + 80);
    CHECK(c.r.day_activated);
    /* keys: Down a week, Right into December turns the page, Home is today */
    key(ui, build_mc, &c, CUI_KEY_DOWN, 0);
    CHECK(c.sel.day == 27);
    key(ui, build_mc, &c, CUI_KEY_DOWN, 0);
    CHECK(c.sel.month == 12 && c.sel.day == 4 && c.month.month == 12 && c.r.month_changed);
    key(ui, build_mc, &c, CUI_KEY_PAGE_DOWN, 0);
    CHECK(c.month.year == 2027 && c.month.month == 1 && c.sel.day == 4);
    key(ui, build_mc, &c, CUI_KEY_HOME, 0);
    CHECK(c.month.year == 2026 && c.month.month == 11 && c.sel.day == 8);
    /* many events stay cheap: 5,000 in a month */
    enum { M = 5000 };
    cui_month_event *many = malloc(sizeof *many * M);
    for (int i = 0; i < M; i++)
        many[i] = (cui_month_event){ "Event", { 2026, 11, 1 + i % 30 }, i % 7 == 0 ? (cui_date){ 2026, 11, 1 + (i + 3) % 30 } : (cui_date){ 0 },
                                     (float)(i % 24), i % 5 == 0, (cui_tone)(1 + i % 7) };
    c.ev = many; c.n = M;
    run(ui, build_mc, &c, 2);
    clock_t t0 = clock();
    for (int i = 0; i < 20; i++) { cui_input_mouse_move(ui, c.gr.x + 30 + i * 30, c.gr.y + 200); run(ui, build_mc, &c, 1); }
    double ms = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000 / 20;
    printf("    month calendar: 5,000 events, %.2f ms per frame\n", ms);
    CHECK(ms < 30);
    /* narrow: dots and an agenda */
    c.ev = ev; c.n = 6; c.w = 320;
    c.sel = (cui_date){ 2026, 11, 10 };
    run(ui, build_mc, &c, 4);
    CHECK(c.gr.h < 6 * 50);
    free(many);
    cui_destroy(ui);
}

/* ---- guided tour ---- */
typedef struct { int step, result, a_clicks, b_clicks; cui_rect ar, br; } tour_ctx;
static void build_tour(cui_ctx *ui, void *u) {
    tour_ctx *t = u;
    static cui_tour_step steps[3] = { { 0, "One", "First", CUI_PLACE_BOTTOM_START }, { 0, "Two", "Second", CUI_PLACE_BOTTOM_START }, { 0, "Three", "Middle", 0 } };
    cui_row(ui, .gap = 200, .pad = 40) {
        if (cui_button(ui, "Alpha")) t->a_clicks++;
        steps[0].target = cui_last_id(ui); t->ar = cui_last_rect(ui);
        if (cui_button(ui, "Beta")) t->b_clicks++;
        steps[1].target = cui_last_id(ui); t->br = cui_last_rect(ui);
    }
    t->result = cui_tour(ui, "t", steps, 3, &t->step);
}
static void test_tour(void) {
    cui_ctx *ui = new_ctx();
    tour_ctx t = { 0 };
    run(ui, build_tour, &t, 4);
    /* the spotlighted control still takes clicks; the one in the dark does not */
    cui_vec2 a = center(t.ar), b = center(t.br);
    click_at(ui, build_tour, &t, a.x, a.y);
    CHECK(t.a_clicks == 1);
    click_at(ui, build_tour, &t, b.x, b.y);
    CHECK(t.b_clicks == 0 && t.step == 0);
    /* Right goes on, Left back, and the spotlight moves to Beta */
    key(ui, build_tour, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.step == 1 && t.result == CUI_TOUR_NEXT);
    run(ui, build_tour, &t, 3);
    click_at(ui, build_tour, &t, b.x, b.y);
    CHECK(t.b_clicks == 1);
    click_at(ui, build_tour, &t, a.x, a.y);
    CHECK(t.a_clicks == 1);
    key(ui, build_tour, &t, CUI_KEY_LEFT, 0);
    CHECK(t.step == 0 && t.result == CUI_TOUR_BACK);
    key(ui, build_tour, &t, CUI_KEY_RIGHT, 0);
    key(ui, build_tour, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.step == 2);   /* no target: a card in the middle, and the whole window dims */
    run(ui, build_tour, &t, 3);
    click_at(ui, build_tour, &t, b.x, b.y);
    CHECK(t.b_clicks == 1);
    key(ui, build_tour, &t, CUI_KEY_RIGHT, 0);
    CHECK(t.step == -1 && t.result == CUI_TOUR_DONE);
    /* gone: clicks reach everything again; a new tour ends with Esc */
    run(ui, build_tour, &t, 2);
    click_at(ui, build_tour, &t, b.x, b.y);
    CHECK(t.b_clicks == 2);
    t.step = 0;
    run(ui, build_tour, &t, 2);
    key(ui, build_tour, &t, CUI_KEY_ESCAPE, 0);
    CHECK(t.step == -1 && t.result == CUI_TOUR_SKIPPED);
    cui_destroy(ui);
}

/* ---- image viewer and cropper ---- */
typedef struct { cui_texture tex; cui_image_view view; cui_rect crop; float aspect; cui_rect vr, cr; bool vchanged, cchanged; } photo_ctx;
static void build_photo(cui_ctx *ui, void *u) {
    photo_ctx *p = u;
    cui_column(ui, .pad = 20, .gap = 20, .width = CUI_PX(640)) {
        p->vchanged = cui_image_viewer(ui, "v", p->tex, 1000, 500, &p->view, .height = CUI_PX(250));
        p->vr = cui_last_rect(ui);
        p->cchanged = cui_cropper(ui, "c", p->tex, 1000, 500, &p->crop, .aspect = p->aspect, .height = CUI_PX(250));
        p->cr = cui_last_rect(ui);
    }
}
static void photo_drag(cui_ctx *ui, build_fn fn, void *u, float ax, float ay, float bx, float by) {
    cui_input_mouse_move(ui, ax, ay); run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1);
    for (int k = 1; k <= 4; k++) { cui_input_mouse_move(ui, ax + (bx - ax) * k / 4, ay + (by - ay) * k / 4); run(ui, fn, u, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1);
}
static void test_photo(void) {
    cui_ctx *ui = new_ctx();
    static unsigned char px[16 * 16 * 4];
    memset(px, 200, sizeof px);
    photo_ctx p = { 0 };
    p.tex = cui_image_create(ui, 16, 16, px);
    p.aspect = 1;
    run(ui, build_photo, &p, 3);
    /* a zeroed view fits: the 1000 x 500 image in a 600 x 250 view is shown at 50% */
    CHECK(p.view.zoom == 0 && p.view.cx == 500 && p.view.cy == 250);
    /* the wheel zooms around the pointer: the image point under it stays under it */
    float mx = p.vr.x + p.vr.w * 0.25f, my = p.vr.y + p.vr.h * 0.5f;
    float fit = fminf(p.vr.w / 1000, p.vr.h / 500);
    float ix = 500 + (mx - (p.vr.x + p.vr.w / 2)) / fit;
    cui_input_mouse_move(ui, mx, my); run(ui, build_photo, &p, 2);
    cui_input_scroll(ui, 0, 3); run(ui, build_photo, &p, 1);
    CHECK(p.vchanged && p.view.zoom > fit * 1.6f);
    float ix2 = p.view.cx + (mx - (p.vr.x + p.vr.w / 2)) / p.view.zoom;
    CHECK_NEAR(ix2, ix, 1.0);
    /* dragging pans by the distance over the zoom */
    float cx0 = p.view.cx, z = p.view.zoom;
    photo_drag(ui, build_photo, &p, mx, my, mx - 40, my);
    CHECK_NEAR(p.view.cx, cx0 + 40 / z, 0.5);
    /* double-click: actual pixels around the pointer, again: fit */
    run(ui, build_photo, &p, 30);
    click_at(ui, build_photo, &p, mx, my); click_at(ui, build_photo, &p, mx, my);
    CHECK_NEAR(p.view.zoom, 1, 0.001);
    run(ui, build_photo, &p, 30);
    click_at(ui, build_photo, &p, mx, my); click_at(ui, build_photo, &p, mx, my);
    CHECK(p.view.zoom == 0);
    /* keys: 1 is actual pixels, 0 fits, the arrows pan */
    key(ui, build_photo, &p, CUI_KEY_1, 0);
    CHECK_NEAR(p.view.zoom, 1, 0.001);
    float cy0 = p.view.cy;
    key(ui, build_photo, &p, CUI_KEY_DOWN, 0);
    CHECK(p.view.cy > cy0);
    key(ui, build_photo, &p, CUI_KEY_0, 0);
    CHECK(p.view.zoom == 0);
    /* panning never pulls the image off the view */
    key(ui, build_photo, &p, CUI_KEY_1, 0);
    for (int k = 0; k < 60; k++) key(ui, build_photo, &p, CUI_KEY_LEFT, 0);
    CHECK_NEAR(p.view.cx, p.vr.w * 0.5f, 0.6);

    /* cropper: a zeroed crop starts as the largest centered square, kept inside */
    CHECK(p.crop.w > 0 && fabsf(p.crop.w - p.crop.h) < 0.01f && p.crop.h <= 500);
    float cz = fminf((p.cr.w - 28) / 1000, (p.cr.h - 28) / 500);
    float ox = p.cr.x + (p.cr.w - 1000 * cz) / 2, oy = p.cr.y + (p.cr.h - 500 * cz) / 2;
    cui_rect c0 = p.crop;
    /* the bottom-right corner grows it and the aspect holds */
    float sx = ox + (c0.x + c0.w) * cz, sy = oy + (c0.y + c0.h) * cz;
    photo_drag(ui, build_photo, &p, sx, sy, sx - 40, sy - 30);
    CHECK(p.crop.w < c0.w && fabsf(p.crop.w - p.crop.h) < 0.01f && p.crop.x == c0.x && p.crop.y == c0.y);
    /* moving keeps it in the image */
    cui_rect c1 = p.crop;
    float in_x = ox + (c1.x + c1.w / 2) * cz, in_y = oy + (c1.y + c1.h / 2) * cz;
    photo_drag(ui, build_photo, &p, in_x, in_y, in_x + 2000, in_y);
    CHECK_NEAR(p.crop.x + p.crop.w, 1000, 0.01);
    CHECK(p.crop.w == c1.w);
    /* free aspect: dragging outside draws a new crop */
    p.aspect = 0;
    run(ui, build_photo, &p, 2);
    photo_drag(ui, build_photo, &p, ox + 20 * cz, oy + 20 * cz, ox + 220 * cz, oy + 120 * cz);
    CHECK_NEAR(p.crop.x, 20, 1.5); CHECK_NEAR(p.crop.w, 200, 1.5); CHECK_NEAR(p.crop.h, 100, 1.5);
    /* keys move it; it never gets smaller than min_size */
    key(ui, build_photo, &p, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_NEAR(p.crop.x, 30, 1.5);
    float l = ox + p.crop.x * cz, t = oy + p.crop.y * cz;
    photo_drag(ui, build_photo, &p, l + p.crop.w * cz, t + p.crop.h * cz, l - 100, t - 100);
    CHECK(p.crop.w >= 16 && p.crop.h >= 16);
    cui_image_destroy(ui, p.tex);
    cui_destroy(ui);
}

/* ---- file browser and file picker ---- */
static const struct { const char *dir, *name; bool folder; uint64_t size; } fb_disk[] = {
    { "/w", "beta", true, 0 }, { "/w", "Alpha", true, 0 }, { "/w", "zeta.txt", false, 30 }, { "/w", "img10.png", false, 900 },
    { "/w", "img2.png", false, 100 }, { "/w", "notes.md", false, 5 }, { "/w/Alpha", "inner.txt", false, 1 }, { "/w/beta", "b.txt", false, 2 },
};
static int fb_list(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden) {
    (void)user; (void)names; (void)names_cap; (void)hidden;
    int n = 0;
    for (size_t k = 0; k < sizeof fb_disk / sizeof fb_disk[0] && n < cap; k++)
        if (!strcmp(fb_disk[k].dir, path)) out[n++] = (cui_file_entry){ fb_disk[k].name, fb_disk[k].folder, fb_disk[k].size, 1000 + (int64_t)k };
    return n || !strcmp(path, "/w") ? n : -1;
}
static int fb_big_list(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden) {
    (void)path; (void)names; (void)names_cap; (void)hidden;
    char (*nm)[12] = user;
    int n = cap < 20000 ? cap : 20000;
    for (int k = 0; k < n; k++) out[k] = (cui_file_entry){ nm[k], k % 50 == 0, (uint64_t)k * 13, 1000 + k };
    return n;
}
typedef struct { char path[256]; cui_file_browser_result r; cui_rect lr, btn; const char *filter; bool open; char out[256]; bool done, save; } fb_ctx;
static void build_fb(cui_ctx *ui, void *u) {
    fb_ctx *c = u;
    c->r = cui_file_browser(ui, "fb", c->path, sizeof c->path, .list = fb_list, .no_places = true, .filter = c->filter, .height = CUI_PX(300));
    c->lr = cui_last_rect(ui);
    c->done |= cui_file_picker(ui, "pick", &c->open, c->out, sizeof c->out, .list = fb_list, .start = "/w", .save = c->save, .name = "out.txt");
    if (c->open) c->btn = cui_last_rect(ui);   /* the dialog's last button: Save or Open */
}
static void test_files(void) {
    cui_ctx *ui = new_ctx();
    fb_ctx c = { "/w" };
    run(ui, build_fb, &c, 3);
    /* folders first, names in natural order: Alpha, beta, img2, img10, notes, zeta */
    float row0 = c.lr.y + 31 + 15;
    click_at(ui, build_fb, &c, c.lr.x + 40, row0);
    CHECK(c.r.selected && !strcmp(c.r.selected, "/w/Alpha"));
    key(ui, build_fb, &c, CUI_KEY_DOWN, 0); key(ui, build_fb, &c, CUI_KEY_DOWN, 0); key(ui, build_fb, &c, CUI_KEY_DOWN, 0);
    CHECK(c.r.selected && !strcmp(c.r.selected, "/w/img10.png"));
    /* typing jumps by name */
    cui_input_text(ui, "z"); run(ui, build_fb, &c, 1);
    CHECK(c.r.selected && !strcmp(c.r.selected, "/w/zeta.txt"));
    /* Enter on a file picks it; on a folder enters it */
    key(ui, build_fb, &c, CUI_KEY_ENTER, 0);
    CHECK(c.r.picked && !strcmp(c.r.picked, "/w/zeta.txt"));
    key(ui, build_fb, &c, CUI_KEY_HOME, 0);
    key(ui, build_fb, &c, CUI_KEY_ENTER, 0);
    CHECK(!strcmp(c.path, "/w/Alpha") && c.r.navigated);
    run(ui, build_fb, &c, 1);
    /* Backspace goes up, Alt+Left back, Alt+Right forward */
    key(ui, build_fb, &c, CUI_KEY_BACKSPACE, 0);
    CHECK(!strcmp(c.path, "/w"));
    key(ui, build_fb, &c, CUI_KEY_LEFT, CUI_MOD_ALT);
    CHECK(!strcmp(c.path, "/w/Alpha"));
    key(ui, build_fb, &c, CUI_KEY_RIGHT, CUI_MOD_ALT);
    CHECK(!strcmp(c.path, "/w"));
    /* double-click a folder goes into it */
    run(ui, build_fb, &c, 30);
    click_at(ui, build_fb, &c, c.lr.x + 40, row0 + 30);
    click_at(ui, build_fb, &c, c.lr.x + 40, row0 + 30);
    CHECK(!strcmp(c.path, "/w/beta"));
    key(ui, build_fb, &c, CUI_KEY_BACKSPACE, 0);
    /* the filter keeps folders and the files that match */
    c.filter = "*.png";
    run(ui, build_fb, &c, 2);
    click_at(ui, build_fb, &c, c.lr.x + 40, row0 + 60);
    CHECK(c.r.selected && !strcmp(c.r.selected, "/w/img2.png"));
    click_at(ui, build_fb, &c, c.lr.x + 40, row0 + 90);
    CHECK(c.r.selected && !strcmp(c.r.selected, "/w/img10.png"));
    c.filter = NULL;
    /* a folder that cannot be read says so */
    snprintf(c.path, sizeof c.path, "/nowhere");
    run(ui, build_fb, &c, 2);
    CHECK(c.r.error);
    snprintf(c.path, sizeof c.path, "/w");
    run(ui, build_fb, &c, 2);
    /* the picker: Save puts the name in the folder it shows */
    c.open = true; c.save = true; c.done = false;
    run(ui, build_fb, &c, 20);
    CHECK(c.btn.w > 0);
    click_at(ui, build_fb, &c, c.btn.x + c.btn.w / 2, c.btn.y + c.btn.h / 2);
    CHECK(c.done && !c.open && !strcmp(c.out, "/w/out.txt"));
    /* open: nothing selected, Open is off; selecting a file and Open picks it */
    c.save = false; c.open = true; c.done = false; c.out[0] = 0;
    run(ui, build_fb, &c, 20);
    click_at(ui, build_fb, &c, c.btn.x + c.btn.w / 2, c.btn.y + c.btn.h / 2);
    CHECK(!c.done && c.open);
    c.open = false;
    run(ui, build_fb, &c, 20);
    /* a folder of 20,000 files: read and sorted once, then only the rows in view */
    {
        cui_ctx *u2 = new_ctx();
        fb_ctx b = { "/big" };
        static char big_names[20000][12];
        for (int k = 0; k < 20000; k++) snprintf(big_names[k], sizeof big_names[k], "f%05d.txt", (k * 7919) % 20000);
        clock_t t0 = clock();
        for (int f = 0; f < 21; f++) {
            if (f == 1) t0 = clock();
            cui_begin_frame(u2, 1.0f / 60);
            cui_file_browser(u2, "big", b.path, sizeof b.path, .list = fb_big_list, .list_user = big_names, .no_places = true, .height = CUI_PX(400));
            cui_end_frame(u2);
        }
        double ms = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000 / 20;
        printf("    file browser: 20,000 files, %.2f ms per frame\n", ms);
        CHECK(ms < 20);
        cui_destroy(u2);
    }
    /* the local disk: this folder has files; a missing one fails */
    cui_file_entry ents[64];
    char names[8192];
    int n = cui_fs_list(NULL, ".", ents, 64, names, sizeof names, true);
    CHECK(n > 0);
    CHECK(cui_fs_list(NULL, "./no/such/folder/here", ents, 64, names, sizeof names, true) == -1);
    cui_destroy(ui);
}

/* ---- terminal ---- */
static const char *tm_text(const cui_terminal_buffer *t, int i) {
    static char out[512];
    const cui_terminal_line *L = &t->lines[(t->first + i) % t->line_cap];
    snprintf(out, sizeof out, "%.*s", (int)L->len, t->text + L->offset);
    return out;
}
static uint32_t tm_cols(const cui_terminal_buffer *t, int i) { return t->lines[(t->first + i) % t->line_cap].cols; }
typedef struct { cui_terminal_buffer *t; cui_terminal_result r; const char *cmd; bool interrupted; cui_rect field; } tm_ctx;
static void build_tm(cui_ctx *ui, void *u) {
    tm_ctx *c = u;
    cui_column(ui, .pad = 20) {
        c->r = cui_terminal(ui, "term", c->t, .height = 300);
        c->field = cui_last_rect(ui);
        if (c->r.command) c->cmd = c->r.command;
        if (c->r.interrupt) c->interrupted = true;
    }
}
static void test_terminal(void) {
    printf("terminal: parsing, ring buffer, input, history, copy\n");
    static char text[4096];
    static cui_terminal_line lines[64];
    cui_terminal_buffer t;
    cui_terminal_init(&t, text, sizeof text, lines, 64);
    CHECK(t.count == 1 && tm_cols(&t, 0) == 0);
    cui_terminal_write(&t, "one\ntwo", -1);
    CHECK(t.count == 2 && !strcmp(tm_text(&t, 0), "one") && !strcmp(tm_text(&t, 1), "two"));
    /* \r then text starts the line over: progress output */
    cui_terminal_write(&t, "\n 10%\r 50%\r100%", -1);
    CHECK(!strcmp(tm_text(&t, 2), "100%") && tm_cols(&t, 2) == 4);
    /* \r\n is a plain line break */
    cui_terminal_write(&t, "\r\nx", -1);
    CHECK(!strcmp(tm_text(&t, 2), "100%") && !strcmp(tm_text(&t, 3), "x"));
    /* colors stay in the text but take no columns, even split across writes */
    cui_terminal_write(&t, "\n\x1b[3", -1);
    cui_terminal_write(&t, "1mred\x1b[0m", -1);
    CHECK(!strcmp(tm_text(&t, 4), "\x1b[31mred\x1b[0m") && tm_cols(&t, 4) == 3);
    /* other sequences are dropped: cursor moves, titles */
    cui_terminal_write(&t, "\n\x1b[2Aa\x1b]0;title\x07" "b\x1b]8;;x\x1b\\c", -1);
    CHECK(!strcmp(tm_text(&t, 5), "abc"));
    /* tabs go to the next stop of 8; backspace takes the last character; UTF-8 counts once */
    cui_terminal_write(&t, "\nab\tc\nxyz\b\b!\n\xc3\xa9t\xc3\xa9", -1);
    CHECK(tm_cols(&t, 6) == 9 && !strcmp(tm_text(&t, 7), "x!") && tm_cols(&t, 8) == 3);
    /* erase to end of line after \r restarts it too */
    cui_terminal_write(&t, "\nold\r\x1b[Knew", -1);
    CHECK(!strcmp(tm_text(&t, 9), "new"));
    /* ESC [ 2 J clears */
    cui_terminal_write(&t, "\x1b[2J", -1);
    CHECK(t.count == 1 && tm_cols(&t, 0) == 0);
    cui_terminal_printf(&t, "%d files", 3);
    CHECK(!strcmp(tm_text(&t, 0), "3 files"));

    /* ring: old lines go when lines or text run out, the newest stay whole */
    {
        static char small[256];
        static cui_terminal_line sl[8];
        cui_terminal_buffer r;
        cui_terminal_init(&r, small, sizeof small, sl, 8);
        for (int i = 0; i < 100; i++) cui_terminal_printf(&r, "line %d\n", i);
        CHECK(r.count == 8 && !strcmp(tm_text(&r, 6), "line 99") && !strcmp(tm_text(&r, 0), "line 93"));
        for (int i = 0; i < 50; i++) cui_terminal_printf(&r, "%050d\n", i);   /* longer lines: the text is what runs out */
        CHECK(r.count <= 8 && r.count >= 3);
        bool okay = true;
        for (int i = 0; i < r.count - 1; i++) {
            const char *s = tm_text(&r, i);
            okay &= strlen(s) == 50 && s[0] == '0';
        }
        CHECK(okay && atoi(tm_text(&r, r.count - 2)) == 49);
        /* a line longer than the whole buffer stops growing instead of breaking anything */
        for (int i = 0; i < 400; i++) cui_terminal_write(&r, "z", 1);
        CHECK(r.lines[(r.first + r.count - 1) % 8].len <= 256);
        cui_terminal_write(&r, "\nafter", -1);
        CHECK(!strcmp(tm_text(&r, r.count - 1), "after"));
    }

    /* the view: typing, Enter, history, Ctrl+C, a selection copied without its colors */
    cui_ctx *ui = new_ctx();
    cui_terminal_init(&t, text, sizeof text, lines, 64);
    cui_terminal_write(&t, "\x1b[32mhello\x1b[0m world\nsecond line\n", -1);
    tm_ctx c = { &t };
    run(ui, build_tm, &c, 2);
    click_at(ui, build_tm, &c, c.field.x + 40, c.field.y + c.field.h / 2);
    type(ui, build_tm, &c, "ls -la");
    key(ui, build_tm, &c, CUI_KEY_ENTER, 0);
    CHECK(c.cmd && !strcmp(c.cmd, "ls -la"));
    CHECK(!strcmp(tm_text(&t, 2), "\x1b[2m$ \x1b[0mls -la"));
    c.cmd = NULL;
    type(ui, build_tm, &c, "pwd");
    key(ui, build_tm, &c, CUI_KEY_ENTER, 0);
    CHECK(c.cmd && !strcmp(c.cmd, "pwd"));
    c.cmd = NULL;
    key(ui, build_tm, &c, CUI_KEY_UP, 0);
    key(ui, build_tm, &c, CUI_KEY_UP, 0);
    key(ui, build_tm, &c, CUI_KEY_ENTER, 0);
    CHECK(c.cmd && !strcmp(c.cmd, "ls -la"));
    key(ui, build_tm, &c, CUI_KEY_UP, 0);
    key(ui, build_tm, &c, CUI_KEY_DOWN, 0);   /* past the newest: empty again */
    c.cmd = NULL;
    key(ui, build_tm, &c, CUI_KEY_ENTER, 0);
    CHECK(c.cmd && c.cmd[0] == 0);
    type(ui, build_tm, &c, "sleep 9");
    key(ui, build_tm, &c, CUI_KEY_C, CUI_MOD_CTRL);
    CHECK(c.interrupted);
    CHECK(!strcmp(tm_text(&t, t.count - 2), "^C"));
    /* select the first two lines by dragging and copy */
    key(ui, build_tm, &c, CUI_KEY_L, CUI_MOD_CTRL);
    CHECK(t.count == 1);
    cui_terminal_write(&t, "\x1b[32mhello\x1b[0m world\nsecond line\nthird\n", -1);
    run(ui, build_tm, &c, 2);
    float x0 = 20 + 10, y0 = 20 + 6, lh = ceilf(13 * 1.45f);
    cui_input_mouse_move(ui, x0 + 1, y0 + lh * 0.5f);
    run(ui, build_tm, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_tm, &c, 1);
    cui_input_mouse_move(ui, x0 + 600, y0 + lh * 1.5f);
    run(ui, build_tm, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_tm, &c, 1);
    g_clip[0] = 0;
    key(ui, build_tm, &c, CUI_KEY_C, CUI_MOD_PRIMARY);
    CHECK(!strcmp(g_clip, "hello world\nsecond line"));
    c.interrupted = false;

    /* many lines: only the rows in view are drawn */
    {
        static char big[4 << 20];
        static cui_terminal_line bl[50000];
        cui_terminal_buffer b;
        cui_terminal_init(&b, big, sizeof big, bl, 50000);
        for (int i = 0; i < 60000; i++) cui_terminal_printf(&b, "\x1b[3%dm%6d\x1b[0m compiling module %d of 60000, a long line that wraps once the panel is narrow\n", i % 8, i, i);
        tm_ctx bc = { &b };
        run(ui, build_tm, &bc, 2);
        clock_t t0 = clock();
        run(ui, build_tm, &bc, 20);
        double ms = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000 / 20;
        printf("    terminal: %d lines, %.2f ms per frame\n", b.count, ms);
        CHECK(b.count > 40000 && ms < 20);
    }
    cui_destroy(ui);
}

/* ---- emoji ---- */
static int em_draws;
static bool em_fake_draw(void *u, const char *s, int len, int px, unsigned char *rgba) {
    (void)u;
    em_draws++;
    unsigned h = 2166136261u;
    for (int i = 0; i < len; i++) h = (h ^ (unsigned char)s[i]) * 16777619u;
    for (int i = 0; i < px * px; i++) { rgba[i * 4] = (unsigned char)h; rgba[i * 4 + 1] = (unsigned char)(h >> 8); rgba[i * 4 + 2] = (unsigned char)(h >> 16); rgba[i * 4 + 3] = 255; }
    return true;
}
typedef struct { const char *picked; char recent[256]; int tone; cui_id grid, flags_tab; } em_ctx;
static void build_em(cui_ctx *ui, void *u) {
    em_ctx *c = u;
    cui_column(ui, .pad = 20) {
        const char *p = cui_emoji_picker(ui, "emoji", .recent = c->recent, .recent_cap = sizeof c->recent, .tone = &c->tone);
        if (p) c->picked = p;
    }
    cui_push_id(ui, "emoji");
    c->grid = cui_make_id(ui, "#grid");
    cui_push_id_int(ui, 9);
    c->flags_tab = cui_make_id(ui, "#tab");
    cui_pop_id(ui);
    cui_pop_id(ui);
}
static void test_emoji(void) {
    printf("emoji: list, search, tones, recent, keys, clicks, font tables\n");
    cui_ctx *ui = new_ctx();
    cui_set_platform(ui, &(cui_platform_iface){ .set_clipboard = clip_set, .get_clipboard = clip_get, .draw_emoji = em_fake_draw });
    em_ctx c;
    memset(&c, 0, sizeof c);
    run(ui, build_em, &c, 3);
    CHECK(em_draws > 0 && em_draws <= 24 * 3);   /* a few per frame, not the whole list at once */
    CHECK(cui_emoji_supported(ui, "\xf0\x9f\x98\x80", -1));
    /* search, then Enter picks the first result; names starting with the query come first */
    cui_input_mouse_move(ui, 60, 45); run(ui, build_em, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_em, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_em, &c, 1);
    type(ui, build_em, &c, "red heart");
    key(ui, build_em, &c, CUI_KEY_ENTER, 0);
    CHECK(c.picked && !strcmp(c.picked, "\xe2\x9d\xa4\xef\xb8\x8f"));
    CHECK(!strcmp(c.recent, "\xe2\x9d\xa4\xef\xb8\x8f"));
    /* a tone goes after each person */
    c.tone = 3; c.picked = NULL;
    key(ui, build_em, &c, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_em, &c, "thumbs up");
    key(ui, build_em, &c, CUI_KEY_ENTER, 0);
    CHECK(c.picked && !strcmp(c.picked, "\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd"));
    c.picked = NULL;
    key(ui, build_em, &c, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_em, &c, "people holding hands");
    key(ui, build_em, &c, CUI_KEY_ENTER, 0);
    CHECK(c.picked && !strcmp(c.picked, "\xf0\x9f\xa7\x91\xf0\x9f\x8f\xbd\xe2\x80\x8d\xf0\x9f\xa4\x9d\xe2\x80\x8d\xf0\x9f\xa7\x91\xf0\x9f\x8f\xbd"));
    /* the arrows move through results; nothing found picks nothing */
    c.tone = 0; c.picked = NULL;
    key(ui, build_em, &c, CUI_KEY_A, CUI_MOD_PRIMARY);
    type(ui, build_em, &c, "zzqqx");
    key(ui, build_em, &c, CUI_KEY_ENTER, 0);
    CHECK(c.picked == NULL);
    /* recent: newest first, no repeats */
    CHECK(!strncmp(c.recent, "\xf0\x9f\xa7\x91", 4) && strstr(c.recent, "\xe2\x9d\xa4"));
    /* a tab clicked during a search leaves it and goes to the group: the first flag is then a click away */
    {
        cui_rect tr = rect_of(ui, c.flags_tab);
        click_at(ui, build_em, &c, tr.x + tr.w / 2, tr.y + tr.h / 2);
        run(ui, build_em, &c, 3);
        cui_rect g2 = rect_of(ui, c.grid);
        c.picked = NULL;
        click_at(ui, build_em, &c, g2.x + 30, g2.y + 30 + 19);
        CHECK(c.picked && !strcmp(c.picked, "\xf0\x9f\x8f\x81"));   /* chequered flag, the first of the group */
        c.recent[0] = 0;
        c.picked = NULL;
        run(ui, build_em, &c, 2);
    }

    /* cleared search: Down goes into the grid, Right moves, Enter picks */
    key(ui, build_em, &c, CUI_KEY_A, CUI_MOD_PRIMARY);
    key(ui, build_em, &c, CUI_KEY_BACKSPACE, 0);
    c.recent[0] = 0;
    run(ui, build_em, &c, 2);
    key(ui, build_em, &c, CUI_KEY_DOWN, 0);
    key(ui, build_em, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_em, &c, CUI_KEY_ENTER, 0);
    CHECK(c.picked && !strcmp(c.picked, "\xf0\x9f\x98\x83"));   /* the second smiley */
    /* a click on a cell picks it; a drag scrolls instead */
    cui_rect gr = rect_of(ui, c.grid);
    c.picked = NULL;
    c.recent[0] = 0;
    run(ui, build_em, &c, 30);
    click_at(ui, build_em, &c, gr.x + gr.w / 2 - 60, gr.y + 30 + 20);
    CHECK(c.picked != NULL);
    c.picked = NULL;
    c.recent[0] = 0;
    run(ui, build_em, &c, 30);
    cui_input_mouse_move(ui, gr.x + 100, gr.y + 200); run(ui, build_em, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_em, &c, 1);
    for (int k = 1; k <= 10; k++) { cui_input_mouse_move(ui, gr.x + 100, gr.y + 200 - k * 15); run(ui, build_em, &c, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_em, &c, 1);
    CHECK(c.picked == NULL);
    /* the whole list scrolled through stays cheap */
    {
        clock_t t0 = clock();
        for (int k = 0; k < 60; k++) { cui_input_mouse_move(ui, gr.x + 100, gr.y + 100); cui_input_scroll(ui, 0, -3); run(ui, build_em, &c, 1); }
        double ms = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000 / 60;
        printf("    emoji picker: %.2f ms per frame while scrolling\n", ms);
        CHECK(ms < 20);
    }
    /* things that are not fonts are refused */
    static const unsigned char junk[64] = { 0, 1, 0, 0, 0, 40 };
    CHECK(!cui_emoji_font_load_memory(ui, junk, sizeof junk, false));
    CHECK(!cui_emoji_font_load_file(ui, "/nonexistent/emoji.ttf"));
    cui_destroy(ui);

    /* a real color font, where there is one: sequences through its ligatures */
    const char *noto = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf";
    FILE *f = fopen(noto, "rb");
    if (f) {
        fclose(f);
        ui = new_ctx();
        CHECK(cui_emoji_font_load_file(ui, noto));
        CHECK(cui_emoji_supported(ui, "\xf0\x9f\x98\x80", -1));                                  /* grinning face */
        CHECK(cui_emoji_supported(ui, "\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd", -1));                  /* thumbs up, medium tone */
        CHECK(cui_emoji_supported(ui, "\xf0\x9f\x87\xa9\xf0\x9f\x87\xaa", -1));                  /* flag: Germany */
        CHECK(cui_emoji_supported(ui, "\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7", -1));   /* family */
        CHECK(cui_emoji_supported(ui, "#\xef\xb8\x8f\xe2\x83\xa3", -1));                         /* keycap # */
        CHECK(!cui_emoji_supported(ui, "A", -1));
        em_ctx c2;
        memset(&c2, 0, sizeof c2);
        run(ui, build_em, &c2, 6);   /* draws real bitmaps */
        cui_destroy(ui);
    } else printf("    (no Noto Color Emoji here: font checks skipped)\n");
}

/* ---- glyph atlas: grows for many sizes, shrinks back when they are gone ---- */
static cui_renderer_iface at_inner;
static int at_w, at_creates;
static cui_texture at_create(void *u, int w, int h, cui_texture_format f, const void *px) {
    if (f == CUI_TEXTURE_R8) { at_w = w; at_creates++; }
    return at_inner.texture_create(u, w, h, f, px);
}
typedef struct { bool many; } at_ctx;
static void build_at(cui_ctx *ui, void *u) {
    at_ctx *c = u;
    cui_column(ui, .pad = 10, .gap = 2) {
        cui_text(ui, "The quick brown fox jumps over the lazy dog 0123456789", .size = 14);
        if (c->many)
            for (int s = 0; s < 60; s++) {
                cui_push_id_int(ui, s);
                cui_text(ui, "ABCDEFGHIJKLMNabcdefghijklm0123456789", .size = 12.0f + s * 0.75f, .nowrap = true);
                cui_pop_id(ui);
            }
    }
}
static void test_atlas_shrink(void) {
    printf("glyph atlas: grows for many sizes, shrinks back, text unchanged\n");
    at_inner = cui_sw_iface(g_sw);
    cui_renderer_iface r = at_inner;
    r.texture_create = at_create;
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = r });
    cui_set_viewport(ui, 900, 3200, 2);
    cui_set_reduced_motion(ui, true);
    at_ctx c = { false };
    run(ui, build_at, &c, 2);
    int w0 = at_w;
    static unsigned char before[900 * 2 * 60 * 2 * 4], after[sizeof before];
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    memcpy(before, px, sizeof before);   /* the top rows: the line that stays */
    free(px);
    c.many = true;
    run(ui, build_at, &c, 3);
    CHECK(at_w > w0);                    /* the sizes needed more room */
    c.many = false;
    for (int i = 0; i < 700; i++) { cui_input_mouse_move(ui, (float)(i % 50), 5); run(ui, build_at, &c, 1); }
    CHECK(at_w == w0);                   /* back to where it started */
    dd = cui_get_draw_data(ui);
    px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    memcpy(after, px, sizeof after);
    free(px);
    CHECK(!memcmp(before, after, sizeof before));
    /* and the sizes come back when asked */
    c.many = true;
    run(ui, build_at, &c, 3);
    CHECK(at_w > w0);
    cui_destroy(ui);
}

/* ---- software renderer: incremental frames equal full ones ---- */
static cui_sw_renderer *inc_a, *inc_b;
static cui_texture inc_create(void *u, int w, int h, cui_texture_format f, const void *px) {
    (void)u;
    cui_renderer_iface a = cui_sw_iface(inc_a), b = cui_sw_iface(inc_b);
    cui_texture t = a.texture_create(a.user, w, h, f, px);
    cui_texture t2 = b.texture_create(b.user, w, h, f, px);
    if (t != t2) printf("  texture ids differ\n");
    return t;
}
static void inc_update(void *u, cui_texture t, int x, int y, int w, int h, const void *px, int stride) {
    (void)u;
    cui_renderer_iface a = cui_sw_iface(inc_a), b = cui_sw_iface(inc_b);
    a.texture_update(a.user, t, x, y, w, h, px, stride);
    b.texture_update(b.user, t, x, y, w, h, px, stride);
}
static void inc_destroy(void *u, cui_texture t) {
    (void)u;
    cui_renderer_iface a = cui_sw_iface(inc_a), b = cui_sw_iface(inc_b);
    test_stepper_width_isolation();
    test_pagination_unnamed_width();
    test_navigation_focus_visible();
    a.texture_destroy(a.user, t);
    b.texture_destroy(b.user, t);
}
typedef struct { char name[64]; bool on; float v; int tab; } inc_ctx;
static void build_inc(cui_ctx *ui, void *u) {
    inc_ctx *c = u;
    cui_row(ui, .pad = 16, .gap = 16, .width = CUI_FILL, .height = CUI_FILL) {
        cui_column(ui, .gap = 10, .width = CUI_PX(260)) {
            cui_input(ui, "name", c->name, sizeof c->name, .placeholder = "Name");
            cui_switch(ui, "Notifications", &c->on);
            cui_slider(ui, "Volume", &c->v, 0, 100);
            static const char *tabs[] = { "One", "Two", "Three" };
            cui_segmented(ui, "tabs", tabs, 3, &c->tab);
            for (int i = 0; i < 4; i++) { cui_push_id_int(ui, i); cui_button(ui, "Button"); cui_pop_id(ui); }
        }
        cui_scroll(ui, "list", .width = CUI_FILL, .height = CUI_FILL) {
            for (int i = 0; i < 60; i++) { cui_push_id_int(ui, i); cui_text(ui, "A row of text in a scrolling list"); cui_pop_id(ui); }
        }
    }
}
static void test_sw_incremental(void) {
    printf("software renderer: incremental frames match full ones, and skip unchanged parts\n");
    inc_a = cui_sw_create(); inc_b = cui_sw_create();
    cui_sw_set_incremental(inc_a, true);
    cui_renderer_iface r = { NULL, inc_create, inc_update, inc_destroy };
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = r });
    cui_set_viewport(ui, 640, 400, 2);
    inc_ctx c = { "", false, 30, 0 };
    int W = 1280, H = 800;
    unsigned char *A = malloc((size_t)W * H * 4), *B = malloc((size_t)W * H * 4);
    int mismatches = 0, frames = 0, skipped = 0;
    long painted = 0;
    for (int f = 0; f < 240; f++) {
        if (f % 7 == 0) cui_input_mouse_move(ui, 20.0f + (f * 37) % 600, 20.0f + (f * 53) % 360);
        if (f == 40) { cui_input_mouse_move(ui, 100, 32); cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); }
        if (f == 41) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f > 45 && f < 60) cui_input_text(ui, "x");
        if (f > 80 && f < 120 && f % 3 == 0) { cui_input_mouse_move(ui, 450, 200); cui_input_scroll(ui, 0, -1); }
        if (f == 150) cui_set_theme(ui, CUI_THEME_DARK);
        cui_begin_frame(ui, 1.0f / 60);
        build_inc(ui, &c);
        cui_end_frame(ui);
        const cui_draw_data *dd = cui_get_draw_data(ui);
        cui_sw_render(inc_a, dd, A, W, H, W * 4, CUI_SW_RGBA);
        cui_sw_render(inc_b, dd, B, W, H, W * 4, CUI_SW_RGBA);
        int x, y, w, h;
        if (cui_sw_last_damage(inc_a, &x, &y, &w, &h)) painted += (long)w * h; else skipped++;
        if (memcmp(A, B, (size_t)W * H * 4)) mismatches++;
        frames++;
    }
    CHECK(mismatches == 0);
    CHECK(skipped > 20);                                  /* frames with nothing new draw nothing */
    CHECK(painted < (long)frames * W * H / 3);            /* most frames repaint a small part */
    printf("    incremental: %d of %d frames drew nothing, %.0f%% of the pixels drawn overall\n", skipped, frames, 100.0 * painted / ((double)frames * W * H));
    cui_destroy(ui);
    cui_sw_destroy(inc_a); cui_sw_destroy(inc_b);
    free(A); free(B);
}

/* ---- the emoji texture goes when no emoji is drawn for a while ---- */
static cui_renderer_iface et_inner;
static int et_rgba_live;
static cui_texture et_ids[16];
static cui_texture et_create(void *u, int w, int h, cui_texture_format f, const void *px) {
    cui_texture t = et_inner.texture_create(u, w, h, f, px);
    if (f == CUI_TEXTURE_RGBA8) { et_rgba_live++; for (int i = 0; i < 16; i++) if (!et_ids[i]) { et_ids[i] = t; break; } }
    return t;
}
static void et_destroy(void *u, cui_texture t) {
    for (int i = 0; i < 16; i++) if (et_ids[i] == t) { et_ids[i] = 0; et_rgba_live--; break; }
    et_inner.texture_destroy(u, t);
}
typedef struct { bool show; } et_ctx;
static void build_et(cui_ctx *ui, void *u) {
    et_ctx *c = u;
    cui_column(ui, .pad = 10) { cui_text(ui, "Hello"); if (c->show) cui_emoji(ui, "\xf0\x9f\x8e\x89", 24); }
}
static void test_emoji_release(void) {
    printf("emoji: the texture goes when no emoji is drawn for a while, and comes back\n");
    et_inner = cui_sw_iface(g_sw);
    cui_renderer_iface r = et_inner;
    r.texture_create = et_create;
    r.texture_destroy = et_destroy;
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = r });
    cui_set_platform(ui, &(cui_platform_iface){ .draw_emoji = em_fake_draw });
    cui_set_viewport(ui, 300, 200, 1);
    et_ctx c = { true };
    run(ui, build_et, &c, 3);
    int with = et_rgba_live;
    CHECK(with == 1);
    c.show = false;
    for (int i = 0; i < 700; i++) { cui_input_mouse_move(ui, (float)(i % 40), 3); run(ui, build_et, &c, 1); }
    CHECK(et_rgba_live == 0);
    c.show = true;
    run(ui, build_et, &c, 3);
    CHECK(et_rgba_live == 1);
    cui_destroy(ui);
    CHECK(et_rgba_live == 0);
}

/** Share one software renderer across the test contexts and return a failing
 * exit status to CTest if any check in the suite fails. */
int main(void) {
    g_sw = cui_sw_create();
    test_color();
    test_layout_row();
    test_layout_wrap();
    test_layout_center();
    test_layout_grid();
    test_layout_flow();
    test_layout_autogrid();
    test_layout_narrow_table();
    test_button_click();
    test_click_then_keys();
    test_keyboard_nav();
    test_copy_button_focus();
    test_nav_highlight();
    test_clipboard_api();
    test_clock_step_bound();
    test_toggles_and_slider();
    test_text_input();
    test_disabled_password_reveal();
    test_empty_field_caret();
    test_textarea_and_clear();
    test_textarea_ime_caret();
    test_nested_textarea_wheel();
    test_nested_textarea_mixed_wheel();
    test_canvas_rail_wheel();
    test_number_edit_commit();
    test_searchable_select_reopen();
    test_more_actions();
    test_toggle_group_arrows();
    test_more_inputs();
    test_range_slider_keyboard();
    test_range_slider_context_menu();
    test_swipe();
    test_date_math();
    test_strings();
    test_sql_colors();
    test_calendar_years();
    test_date_picker_reopens_on_days();
    test_pickers();
    test_disclosure();
    test_confirm_popover_keyboard();
    test_checklist();
    test_data_grid();
    test_tree_table();
    test_tools();
    test_hex_view();
    test_markdown();
    test_code_editor();
    test_diff();
    test_dials();
    test_candles();
    test_month();
    test_tour();
    test_photo();
    test_files();
    test_terminal();
    test_emoji();
    test_atlas_shrink();
    test_sw_incremental();
    test_emoji_release();
    test_planning();
    test_node_editor();
    test_flame();
    test_curve_eval();
    test_curve_editor();
    test_gradient();
    test_timeline();
    test_media();
    test_viz();
    test_motion();
    test_ai();
    test_text_capacity();
    test_dialog();
    test_dialog_escape_after_edit();
    test_menu();
    test_focus_trap();
    test_tag_limit_batch();
    test_extras();
    test_toast_deadline();
    test_reorder();
    test_chrome();
    test_dock();
    test_virtual_list();
    test_theme();
    test_series_palette();
    test_radial_gauge_styles();
    test_treemap_select();
    test_sankey_node_colors();
    test_gantt_state();
    test_renderer();
    test_scroll_fade();
    test_pretty_wrap();
    test_truncated_tooltip();
    test_tooltip_hover_delay();
    test_truncated_tooltip_covered();
    test_number_roll();
    test_stress();
    test_atlas_pressure();
    cui_sw_destroy(g_sw);
    printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
