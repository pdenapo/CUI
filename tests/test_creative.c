/* Tests for the creative controls: the status tag, the ratio slider, the gradient and step
 * sliders, the palette strip, the aspect ratio picker, the option field with action and
 * the seed field. Runs headless with the software renderer and reduced motion, and drives
 * each control with the pointer and the keys, checking what it returns and writes back. */
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

static cui_ctx *new_ctx(cui_direction dir) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .direction = dir });
    cui_set_viewport(ui, 640, 400, 1);
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
static cui_rect rect_of(cui_ctx *ui, const char *id) { return cui_get_rect(ui, cui_make_id(ui, id)); }
/* a well's control row: its vertical middle, and its start after the 14 point padding */
static float row_y(cui_rect w) { return w.y + w.h - 12 - 16; }

/* ------------------------------------------------------------------------ */
/* Status tag                                                               */
/* ------------------------------------------------------------------------ */

typedef struct { int removed, plain_clicked; } tag_ctx;
static void build_tags(cui_ctx *ui, void *u) {
    tag_ctx *t = u;
    cui_row(ui, .gap = 8) {
        if (cui_status_tag(ui, "Shipped", CUI_TAG_SUCCESS)) t->plain_clicked++;
        if (cui_status_tag(ui, "Draft", CUI_TAG_CUSTOM, .icon = CUI_ICON_TAG, .tone = CUI_TONE_TEAL, .removable = true, .size = CUI_SIZE_SM)) t->removed++;
        cui_status_tag(ui, NULL, CUI_TAG_IN_PROGRESS);
    }
}

static void test_status_tag(void) {
    printf("status tag\n");
    tag_ctx t = { 0 };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_tags, &t, 3);
    cui_rect draft = rect_of(ui, "Draft");
    CHECK(draft.w > 20);
    CHECK_NEAR(draft.h, 24, 0.01);                  /* the small size */
    /* a plain tag is not a control: clicking it reports nothing */
    cui_rect r0 = draft;
    click(ui, build_tags, &t, r0.x - 30, r0.y + r0.h / 2);
    CHECK_EQ(t.plain_clicked, 0);
    /* a removable tag reports a click, Enter, Delete and Backspace */
    click(ui, build_tags, &t, draft.x + draft.w / 2, draft.y + draft.h / 2);
    CHECK_EQ(t.removed, 1);
    move(ui, build_tags, &t, 600, 380);
    key(ui, build_tags, &t, CUI_KEY_TAB, 0);
    key(ui, build_tags, &t, CUI_KEY_DELETE, 0);
    CHECK_EQ(t.removed, 2);
    key(ui, build_tags, &t, CUI_KEY_BACKSPACE, 0);
    CHECK_EQ(t.removed, 3);
    key(ui, build_tags, &t, CUI_KEY_ENTER, 0);
    CHECK_EQ(t.removed, 4);
    /* the x slides open while focused, so the tag grows */
    run(ui, build_tags, &t, 2);
    CHECK(rect_of(ui, "Draft").w > draft.w + 10);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Ratio slider                                                             */
/* ------------------------------------------------------------------------ */

typedef struct { float ratio; int changed; float width; bool disabled; } ratio_ctx;
static void build_ratio(cui_ctx *ui, void *u) {
    ratio_ctx *c = u;
    if (cui_ratio_slider(ui, "COFFEE", "MILK", &c->ratio, .tone = CUI_TONE_WARNING, .width = c->width > 0 ? CUI_PX(c->width) : CUI_FILL,
                         .disabled = c->disabled))
        c->changed++;
}
static cui_rect ratio_rect(cui_ctx *ui) { return cui_get_rect(ui, cui_last_id(ui)); }

static void test_ratio_slider(void) {
    printf("ratio slider\n");
    ratio_ctx c = { 0.37f, 0, 0, false };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_ratio, &c, 3);
    cui_rect r = ratio_rect(ui);
    CHECK_NEAR(r.h, 52, 0.01);
    CHECK_EQ(c.changed, 0);                          /* nothing touched it */
    CHECK_NEAR(c.ratio, 0.37f, 1e-6);
    /* keys: a step, Shift for ten, Home and End stop where the text still fits */
    key(ui, build_ratio, &c, CUI_KEY_TAB, 0);
    key(ui, build_ratio, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.ratio, 0.38f, 1e-4);
    key(ui, build_ratio, &c, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    CHECK_NEAR(c.ratio, 0.48f, 1e-4);
    key(ui, build_ratio, &c, CUI_KEY_LEFT, 0);
    CHECK_NEAR(c.ratio, 0.47f, 1e-4);
    key(ui, build_ratio, &c, CUI_KEY_HOME, 0);
    float lo = c.ratio;
    CHECK(lo > 0.1f && lo < 0.4f);                   /* "COFFEE 100%" needs room */
    key(ui, build_ratio, &c, CUI_KEY_END, 0);
    float hi = c.ratio;
    CHECK(hi > 0.6f && hi < 0.95f);
    CHECK(c.changed >= 5);
    /* the pointer: a drag to either end stops at the same limits */
    move(ui, build_ratio, &c, r.x + r.w / 2, r.y + r.h / 2);
    press(ui, build_ratio, &c);
    move(ui, build_ratio, &c, r.x - 40, r.y + r.h / 2);
    CHECK_NEAR(c.ratio, lo, 0.011);
    move(ui, build_ratio, &c, r.x + r.w * 0.25f, r.y + r.h / 2);
    CHECK_NEAR(c.ratio, 0.25f, 0.02);
    move(ui, build_ratio, &c, r.x + r.w + 40, r.y + r.h / 2);
    CHECK_NEAR(c.ratio, hi, 0.011);
    release(ui, build_ratio, &c);
    /* narrower, the limits close in */
    c.width = 300;
    run(ui, build_ratio, &c, 2);
    key(ui, build_ratio, &c, CUI_KEY_HOME, 0);
    CHECK(c.ratio > lo);
    /* disabled: keys do nothing */
    c.disabled = true;
    float before = c.ratio;
    run(ui, build_ratio, &c, 1);
    key(ui, build_ratio, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.ratio, before, 1e-6);
    cui_destroy(ui);

    /* right to left: the first part is on the right, and Right makes it smaller */
    ratio_ctx d = { 0.5f, 0, 0, false };
    ui = new_ctx(CUI_DIRECTION_RTL);
    run(ui, build_ratio, &d, 3);
    key(ui, build_ratio, &d, CUI_KEY_TAB, 0);
    key(ui, build_ratio, &d, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(d.ratio, 0.49f, 1e-4);
    r = ratio_rect(ui);
    click(ui, build_ratio, &d, r.x + r.w * 0.3f, r.y + r.h / 2);   /* left of the middle: the second part grows */
    CHECK(d.ratio > 0.6f);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Gradient slider                                                          */
/* ------------------------------------------------------------------------ */

typedef struct { float hue, alpha; int flags, tools; cui_color hue_color, alpha_color; } gs_ctx;
static void build_gs(cui_ctx *ui, void *u) {
    gs_ctx *c = u;
    c->flags = cui_gradient_slider(ui, "Hue", &c->hue, 0, 360, .color_out = &c->hue_color, .tool = CUI_ICON_TARGET, .tool_tooltip = "Pick");
    if (c->flags & CUI_WELL_ACTION) c->tools++;
    cui_gradient_slider(ui, "Opacity", &c->alpha, 0, 1, .track = CUI_GRADIENT_OPACITY, .color = CUI_RGB(0x2080ff), .step = 0.25f,
                        .color_out = &c->alpha_color);
}

static void test_gradient_slider(void) {
    printf("gradient slider\n");
    gs_ctx c = { 0, 0.5f, 0, 0, { 0 }, { 0 } };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_gs, &c, 3);
    /* the color under the thumb: red at 0 and 360, green at 120 */
    CHECK(c.hue_color.r == 255 && c.hue_color.g == 0 && c.hue_color.b == 0);
    c.hue = 120;
    run(ui, build_gs, &c, 1);
    CHECK(c.hue_color.r == 0 && c.hue_color.g == 255 && c.hue_color.b == 0);
    CHECK_EQ(c.alpha_color.a, 128);
    CHECK(c.alpha_color.b == 0xff && c.alpha_color.r == 0x20);
    /* keys: one percent of the range, Shift ten, End the maximum */
    key(ui, build_gs, &c, CUI_KEY_TAB, 0);
    key(ui, build_gs, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.hue, 123.6, 0.01);
    CHECK(c.flags & CUI_WELL_CHANGED);
    key(ui, build_gs, &c, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    CHECK_NEAR(c.hue, 87.6, 0.01);
    key(ui, build_gs, &c, CUI_KEY_END, 0);
    CHECK_NEAR(c.hue, 360, 0.01);
    run(ui, build_gs, &c, 1);
    CHECK_EQ(c.flags, 0);
    /* the pointer: the thumb's travel stops 12 points inside each end of the track */
    cui_rect w = rect_of(ui, "Hue");
    float x0 = w.x + 14 + 12, x1 = w.x + w.w - 12 - 30 - 10 - 12;
    click(ui, build_gs, &c, x0 + 1, row_y(w));
    CHECK(c.hue < 2);
    click(ui, build_gs, &c, (x0 + x1) / 2, row_y(w));
    CHECK_NEAR(c.hue, 180, 3);
    /* the tool button reports its clicks and leaves the value alone */
    float before = c.hue;
    click(ui, build_gs, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK_EQ(c.tools, 1);
    CHECK_NEAR(c.hue, before, 1e-6);
    /* step: the opacity snaps to quarters */
    cui_rect o = rect_of(ui, "Opacity");
    float o0 = o.x + 14 + 12, o1 = o.x + o.w - 12 - 12;
    click(ui, build_gs, &c, o0 + (o1 - o0) * 0.7f, row_y(o));
    CHECK_NEAR(c.alpha, 0.75, 1e-6);
    cui_destroy(ui);

    /* right to left: the minimum is at the right end */
    gs_ctx d = { 180, 0.5f, 0, 0, { 0 }, { 0 } };
    ui = new_ctx(CUI_DIRECTION_RTL);
    run(ui, build_gs, &d, 3);
    w = rect_of(ui, "Hue");
    click(ui, build_gs, &d, w.x + w.w - 14 - 13, row_y(w));
    CHECK(d.hue < 2);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Step slider                                                              */
/* ------------------------------------------------------------------------ */

typedef struct { int n, flags, generated; } step_ctx;
static void build_step(cui_ctx *ui, void *u) {
    step_ctx *c = u;
    static const char *const names[] = { "One", "Two", "Three", "Four" };
    c->flags = cui_step_slider(ui, "Variations", &c->n, 1, 4, .labels = names, .tool = CUI_ICON_REFRESH_CW, .tool_tooltip = "Generate");
    if (c->flags & CUI_WELL_ACTION) c->generated++;
}

static void test_step_slider(void) {
    printf("step slider\n");
    step_ctx c = { 9, 0, 0 };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_step, &c, 2);
    CHECK_EQ(c.n, 4);                                /* clamped into 1..4 */
    cui_rect w = rect_of(ui, "Variations");
    float x0 = w.x + 14 + 10, x1 = w.x + w.w - 12 - 30 - 10 - 10;
    /* a click snaps to the nearest stop */
    click(ui, build_step, &c, x0 + (x1 - x0) * 0.40f, row_y(w));
    CHECK_EQ(c.n, 2);
    click(ui, build_step, &c, x0 + (x1 - x0) * 0.55f, row_y(w));
    CHECK_EQ(c.n, 3);
    /* a drag moves stop by stop */
    press(ui, build_step, &c);
    move(ui, build_step, &c, x0 - 50, row_y(w));
    CHECK_EQ(c.n, 1);
    move(ui, build_step, &c, x1 + 50, row_y(w));
    CHECK_EQ(c.n, 4);
    release(ui, build_step, &c);
    /* keys: one stop, Home and End */
    key(ui, build_step, &c, CUI_KEY_LEFT, 0);
    CHECK_EQ(c.n, 3);
    CHECK(c.flags & CUI_WELL_CHANGED);
    key(ui, build_step, &c, CUI_KEY_HOME, 0);
    CHECK_EQ(c.n, 1);
    key(ui, build_step, &c, CUI_KEY_UP, 0);
    CHECK_EQ(c.n, 2);
    key(ui, build_step, &c, CUI_KEY_END, 0);
    CHECK_EQ(c.n, 4);
    /* the action button */
    click(ui, build_step, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK_EQ(c.generated, 1);
    CHECK_EQ(c.n, 4);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Palette strip                                                            */
/* ------------------------------------------------------------------------ */

typedef struct { cui_color colors[5]; bool locked[5]; uint32_t seed; bool seeded; int flags; } pal_ctx;
static void build_pal(cui_ctx *ui, void *u) {
    pal_ctx *c = u;
    c->flags = cui_palette_strip(ui, "Palette", c->colors, c->locked, 5, .seed = c->seeded ? &c->seed : NULL);
}
static void pal_init(pal_ctx *c, bool seeded) {
    memset(c, 0, sizeof *c);
    for (int k = 0; k < 5; k++) c->colors[k] = CUI_RGB(0x101010 * (k + 1));
    c->seed = 42;
    c->seeded = seeded;
}

static void test_palette_strip(void) {
    printf("palette strip\n");
    pal_ctx c;
    pal_init(&c, true);
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_pal, &c, 2);
    cui_rect w = rect_of(ui, "Palette");
    /* a click on the first swatch locks it, a second unlocks it */
    click(ui, build_pal, &c, w.x + 20, row_y(w));
    CHECK(c.locked[0]);
    CHECK(c.flags & CUI_WELL_CHANGED);
    click(ui, build_pal, &c, w.x + 20, row_y(w));
    CHECK(!c.locked[0]);
    click(ui, build_pal, &c, w.x + 20, row_y(w));
    /* keys: Right walks to the next swatch (the first press after a click already moves), Space locks it */
    key(ui, build_pal, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_pal, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_pal, &c, CUI_KEY_SPACE, 0);
    CHECK(c.locked[0] && !c.locked[1] && c.locked[2]);
    /* shuffle recolors the unlocked swatches only, from the seed, and moves the seed on */
    cui_color before[5];
    memcpy(before, c.colors, sizeof before);
    click(ui, build_pal, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK((c.flags & (CUI_WELL_ACTION | CUI_WELL_CHANGED)) == (CUI_WELL_ACTION | CUI_WELL_CHANGED));
    CHECK(memcmp(&c.colors[0], &before[0], sizeof(cui_color)) == 0);
    CHECK(memcmp(&c.colors[2], &before[2], sizeof(cui_color)) == 0);
    CHECK(memcmp(&c.colors[1], &before[1], sizeof(cui_color)) != 0);
    CHECK(memcmp(&c.colors[4], &before[4], sizeof(cui_color)) != 0);
    CHECK(c.seed != 42);
    cui_color first[5];
    memcpy(first, c.colors, sizeof first);
    cui_destroy(ui);

    /* the same seed gives the same colors */
    pal_init(&c, true);
    c.locked[0] = c.locked[2] = true;
    ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_pal, &c, 2);
    click(ui, build_pal, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK(memcmp(c.colors, first, sizeof first) == 0);
    cui_destroy(ui);

    /* without a seed, shuffle is only reported */
    pal_init(&c, false);
    ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_pal, &c, 2);
    memcpy(before, c.colors, sizeof before);
    click(ui, build_pal, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK_EQ(c.flags, CUI_WELL_ACTION);
    CHECK(memcmp(c.colors, before, sizeof before) == 0);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Aspect ratio picker                                                      */
/* ------------------------------------------------------------------------ */

typedef struct { int index, changed; } aspect_ctx;
static const cui_aspect g_sizes[] = { { 1, 1, NULL }, { 4, 3, NULL }, { 16, 9, "Wide" } };
static void build_aspect(cui_ctx *ui, void *u) {
    aspect_ctx *c = u;
    if (cui_aspect_picker(ui, "Aspect", &c->index, .ratios = g_sizes, .count = 3, .custom = true)) c->changed++;
}

static void test_aspect_picker(void) {
    printf("aspect picker\n");
    for (int dir = 0; dir < 2; dir++) {
        bool rtl = dir == 1;
        aspect_ctx c = { 7, 0 };
        cui_ctx *ui = new_ctx(rtl ? CUI_DIRECTION_RTL : CUI_DIRECTION_LTR);
        run(ui, build_aspect, &c, 3);
        CHECK_EQ(c.index, 3);                        /* clamped: 3 is Custom, after the three ratios */
        cui_rect w = rect_of(ui, "Aspect");
        /* four equal choices across the row: the first is at the start edge */
        float start = rtl ? w.x + w.w - 12 - 20 : w.x + 14 + 20;
        click(ui, build_aspect, &c, start, row_y(w) + 4);
        CHECK_EQ(c.index, 0);
        CHECK(c.changed >= 1);
        /* the arrow toward the end of the row moves to the next choice */
        cui_key next = rtl ? CUI_KEY_LEFT : CUI_KEY_RIGHT, prev = rtl ? CUI_KEY_RIGHT : CUI_KEY_LEFT;
        key(ui, build_aspect, &c, next, 0);
        CHECK_EQ(c.index, 1);
        key(ui, build_aspect, &c, next, 0);
        key(ui, build_aspect, &c, next, 0);
        CHECK_EQ(c.index, 3);
        key(ui, build_aspect, &c, next, 0);
        CHECK_EQ(c.index, 3);                        /* stops at the end */
        key(ui, build_aspect, &c, prev, 0);
        CHECK_EQ(c.index, 2);
        cui_destroy(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* Option field with action                                                 */
/* ------------------------------------------------------------------------ */

typedef struct { int index, flags, saves; bool off; } opt_ctx;
static void build_opt(cui_ctx *ui, void *u) {
    opt_ctx *c = u;
    static const char *const fmt[] = { "png", "jpg", "webp" };
    c->flags = cui_option_action(ui, "Save as", fmt, 3, &c->index, CUI_ICON_DOWNLOAD, .tooltip = "Download", .action_disabled = c->off);
    if (c->flags & CUI_WELL_ACTION) c->saves++;
}

static void test_option_action(void) {
    printf("option field with action\n");
    opt_ctx c = { 0, 0, 0, false };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_opt, &c, 3);
    cui_rect w = rect_of(ui, "Save as");
    float seg0 = w.x + 14, seg1 = w.x + w.w - 12 - 30 - 10;
    click(ui, build_opt, &c, seg0 + (seg1 - seg0) * 5 / 6, row_y(w));
    CHECK_EQ(c.index, 2);
    CHECK_EQ(c.saves, 0);
    click(ui, build_opt, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK_EQ(c.saves, 1);
    CHECK_EQ(c.index, 2);
    c.off = true;
    run(ui, build_opt, &c, 1);
    click(ui, build_opt, &c, w.x + w.w - 12 - 15, row_y(w));
    CHECK_EQ(c.saves, 1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* Seed field                                                               */
/* ------------------------------------------------------------------------ */

typedef struct { uint32_t seed; bool locked; int flags, rolls; } seed_ctx;
static void build_seed(cui_ctx *ui, void *u) {
    seed_ctx *c = u;
    c->flags = cui_seed_field(ui, "Seed", &c->seed, &c->locked);
    if (c->flags & CUI_WELL_ACTION) c->rolls++;
}

static void test_seed_field(void) {
    printf("seed field\n");
    seed_ctx c = { 1234567890u, false, 0, 0 };
    cui_ctx *ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_seed, &c, 3);
    cui_rect w = rect_of(ui, "Seed");
    float lock_x = w.x + w.w - 12 - 2 - 15, dice_x = lock_x - 15 - 2 - 15;
    /* the dice rolls a new ten digit seed, the same one for the same start */
    click(ui, build_seed, &c, dice_x, row_y(w));
    CHECK_EQ(c.rolls, 1);
    CHECK(c.seed != 1234567890u && c.seed >= 1000000000u);
    uint32_t first = c.seed;
    click(ui, build_seed, &c, dice_x, row_y(w));
    CHECK(c.seed != first);
    /* the lock fixes it: the dice does nothing until it is unlocked */
    click(ui, build_seed, &c, lock_x, row_y(w));
    CHECK(c.locked);
    CHECK(c.flags & CUI_WELL_CHANGED);
    uint32_t kept = c.seed;
    click(ui, build_seed, &c, dice_x, row_y(w));
    CHECK_EQ(c.seed, kept);
    CHECK_EQ(c.rolls, 2);
    click(ui, build_seed, &c, lock_x, row_y(w));
    CHECK(!c.locked);
    /* the keyboard reaches the dice: Tab, then Enter */
    move(ui, build_seed, &c, 600, 380);
    key(ui, build_seed, &c, CUI_KEY_TAB, 0);
    key(ui, build_seed, &c, CUI_KEY_ENTER, 0);
    CHECK_EQ(c.rolls, 3);
    cui_destroy(ui);

    seed_ctx d = { 1234567890u, false, 0, 0 };
    ui = new_ctx(CUI_DIRECTION_LTR);
    run(ui, build_seed, &d, 3);
    click(ui, build_seed, &d, dice_x, row_y(w));
    CHECK_EQ(d.seed, first);
    cui_destroy(ui);
}

int main(void) {
    g_sw = cui_sw_create();
    test_status_tag();
    test_ratio_slider();
    test_gradient_slider();
    test_step_slider();
    test_palette_strip();
    test_aspect_picker();
    test_option_action();
    test_seed_field();
    cui_sw_destroy(g_sw);
    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
