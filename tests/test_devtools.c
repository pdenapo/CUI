/* Tests for the developer tool components: the key-value editor, problems
 * panel, pipeline, query builder, schedule picker and cron, keymap, commit
 * graph, dependency graph, network waterfall and trace viewer. Runs headless.
 * Interactions find their targets by moving the pointer over the component
 * until it reports what is under it, so the tests do not depend on pixel
 * positions. */
#include <cui/cui.h>
#include <cui/cui_render.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_failed, g_checks;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { g_checks++; long long a_ = (long long)(a), b_ = (long long)(b); if (a_ != b_) { g_failed++; \
    printf("  FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)
#define CHECK_STR(a, b) do { g_checks++; const char *a_ = (a), *b_ = (b); if (!a_ || strcmp(a_, b_)) { g_failed++; \
    printf("  FAIL %s:%d: %s = \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #a, a_ ? a_ : "(null)", b_); } } while (0)

static cui_sw_renderer *g_sw;

static cui_ctx *new_ctx(float w, float h) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw) });
    cui_set_viewport(ui, w, h, 1);
    cui_set_reduced_motion(ui, true);
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 20, .width = CUI_FILL, .height = CUI_FILL) fn(ui, user);
        cui_end_frame(ui);
    }
}
static void move(cui_ctx *ui, build_fn fn, void *u, float x, float y) { cui_input_mouse_move(ui, x, y); run(ui, fn, u, 1); }
static void click(cui_ctx *ui, build_fn fn, void *u, float x, float y) {
    move(ui, fn, u, x, y);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1);
}
static void double_click(cui_ctx *ui, build_fn fn, void *u, float x, float y) {
    move(ui, fn, u, x, y);
    for (int k = 0; k < 2; k++) {
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1);
        cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1);
    }
}
static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods); cui_input_key(ui, k, false, mods);
    run(ui, fn, u, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
}
static void type(cui_ctx *ui, build_fn fn, void *u, const char *s) { cui_input_text(ui, s); run(ui, fn, u, 1); }

/* moves the pointer over `r` until *hovered reads `want`; false when it never does */
static bool find(cui_ctx *ui, build_fn fn, void *u, cui_rect r, const int *hovered, int want, float *x, float *y) {
    for (float yy = r.y + 3; yy < r.y + r.h; yy += 6)
        for (float xx = r.x + 4; xx < r.x + r.w; xx += 14) {
            move(ui, fn, u, xx, yy);
            if (*hovered == want) { *x = xx; *y = yy; return true; }
        }
    return false;
}

/* ------------------------------------------------------------------------ */
/* cron                                                                     */

static void test_cron(void) {
    printf("cron: next run, lists, ranges, steps, names, day of month or weekday, offsets, invalid, never\n");
    int64_t sun = 1794096000;   /* Sunday 2026-11-08 00:00 UTC */
    int64_t t = 0;
    CHECK(cui_cron_next("30 9 * * 1-5", sun, 0, &t)); CHECK_EQ(t, 1794216600);               /* Monday 09:30 */
    CHECK(cui_cron_next("30 9 * * 1-5", 1794216600, 0, &t)); CHECK_EQ(t, 1794216600 + 86400);  /* strictly after */
    CHECK(cui_cron_next("30 9 * * MON-FRI", 1794562200, 0, &t)); CHECK_EQ(t, 1794821400);   /* Friday to Monday */
    CHECK(cui_cron_next("*/15 * * * *", 1794216600, 0, &t)); CHECK_EQ(t, 1794216600 + 900);
    CHECK(cui_cron_next("0 0 1 1 *", sun, 0, &t)); CHECK_EQ(t, 1798761600);                   /* New Year 2027 */
    CHECK(cui_cron_next("0 12 * * mon", sun, 0, &t)); CHECK_EQ(t, 1794225600);
    CHECK(cui_cron_next("0 12 * * 0", sun, 0, &t)); CHECK_EQ(t, sun + 12 * 3600);              /* Sunday is 0 ... */
    CHECK(cui_cron_next("0 12 * * 7", sun, 0, &t)); CHECK_EQ(t, sun + 12 * 3600);              /* ... and 7 */
    /* both day fields restricted: either one matches, as in cron */
    CHECK(cui_cron_next("0 12 30 * MON", 1796040000 - 86400, 0, &t)); CHECK_EQ(t, 1796040000);
    CHECK(cui_cron_next("0 12 30 * MON", 1796040000, 0, &t)); CHECK_EQ(t, 1796644800);
    CHECK(cui_cron_next("0 0 29 2 *", sun, 0, &t)); CHECK_EQ(t, 1835395200);                  /* the next leap day */
    CHECK(cui_cron_next("5,35 */6 * JAN,NOV *", sun, 0, &t)); CHECK_EQ(t, sun + 300);
    CHECK(cui_cron_next("  0   9 * * *  ", sun, 210, &t)); CHECK_EQ(t, sun + 5 * 3600 + 1800);  /* 09:00 at UTC+3:30 */
    CHECK(cui_cron_next("0 9 * * *", sun, -300, &t)); CHECK_EQ(t, sun + 14 * 3600);           /* 09:00 at UTC-5 */
    /* never runs: false, and quickly */
    clock_t c0 = clock();
    for (int i = 0; i < 50; i++) CHECK(!cui_cron_next("0 0 31 2 *", sun, 0, &t));
    CHECK((double)(clock() - c0) / CLOCKS_PER_SEC < 1.0);
    static const char *bad[] = { "60 * * * *", "* 24 * * *", "* * 0 * *", "* * * 13 *", "* * * * 8", "* * * *", "* * * * * *",
                                 "a b c d e", "5-1 * * * *", "*/0 * * * *", "1,,2 * * * *", "", "-1 * * * *", "* * * FOO *" };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        bool ok = cui_cron_next(bad[i], sun, 0, &t);
        if (ok) printf("  accepted \"%s\"\n", bad[i]);
        CHECK(!ok);
    }
    CHECK(!cui_cron_next(NULL, sun, 0, &t));

    cui_ctx *ui = new_ctx(400, 300);
    cui_begin_frame(ui, 1.0f / 60.0f);
    CHECK_STR(cui_cron_describe(ui, "30 9 * * 1-5"), "Every weekday at 09:30");
    CHECK_STR(cui_cron_describe(ui, "0 10 * * 0,6"), "Every weekend at 10:00");
    CHECK_STR(cui_cron_describe(ui, "*/15 * * * *"), "Every 15 minutes");
    CHECK_STR(cui_cron_describe(ui, "* * * * *"), "Every minute");
    CHECK_STR(cui_cron_describe(ui, "7 * * * *"), "Every hour at :07");
    CHECK_STR(cui_cron_describe(ui, "0 6 * * *"), "Every day at 06:00");
    CHECK_STR(cui_cron_describe(ui, "0 9 1 * *"), "On day 1 of every month at 09:00");
    CHECK_STR(cui_cron_describe(ui, "0 9 * * 1,3"), "Every Monday and Wednesday at 09:00");
    CHECK_STR(cui_cron_describe(ui, "0 9 * * 1,3,5"), "Every Mon, Wed and Fri at 09:00");
    CHECK_STR(cui_cron_describe(ui, "0 9 * 6 *"), "Custom schedule");
    CHECK(cui_cron_describe(ui, "nope") == NULL);
    cui_end_frame(ui);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* schedule picker                                                          */

typedef struct { char cron[64]; cui_schedule_result r; } sc_ctx;
static void build_sc(cui_ctx *ui, void *u) {
    sc_ctx *c = u;
    c->r = cui_schedule_picker(ui, "sched", c->cron, sizeof c->cron, .now = 1794096000, .utc = true, .next_count = 3);
}
static void test_schedule(void) {
    printf("schedule picker: reads the expression, next run, leaves it alone, invalid, narrow, right to left\n");
    sc_ctx c;
    memset(&c, 0, sizeof c);
    snprintf(c.cron, sizeof c.cron, "30 9 * * 1-5");
    cui_ctx *ui = new_ctx(700, 500);
    run(ui, build_sc, &c, 3);
    CHECK(c.r.valid);
    CHECK_EQ(c.r.next, 1794216600);
    CHECK(!c.r.changed);
    CHECK_STR(c.cron, "30 9 * * 1-5");   /* showing it does not rewrite it */
    snprintf(c.cron, sizeof c.cron, "*/10 * * * *");   /* the app changes it */
    run(ui, build_sc, &c, 2);
    CHECK(c.r.valid && c.r.next == 1794096000 + 600);
    snprintf(c.cron, sizeof c.cron, "61 * * * *");
    run(ui, build_sc, &c, 2);
    CHECK(!c.r.valid && c.r.next == 0);
    snprintf(c.cron, sizeof c.cron, "0 0 31 2 *");
    run(ui, build_sc, &c, 2);
    CHECK(c.r.valid && c.r.next == 0);   /* valid, but never runs */
    cui_set_viewport(ui, 330, 600, 1);
    snprintf(c.cron, sizeof c.cron, "0 8 * * 1,2,3");
    run(ui, build_sc, &c, 3);
    CHECK(c.r.valid && !c.r.changed);
    cui_destroy(ui);
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .direction = CUI_DIRECTION_RTL });
    cui_set_viewport(ui, 700, 500, 1);
    run(ui, build_sc, &c, 3);
    CHECK(c.r.valid && !c.r.changed);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* query builder                                                            */

static const char *const qb_status[] = { "Open", "In progress", "Closed" };
static const cui_query_field qb_fields[] = {
    { "Status", CUI_QUERY_CHOICE, qb_status, 3 }, { "Assignee", CUI_QUERY_TEXT }, { "Priority", CUI_QUERY_NUMBER }, { "Has PR", CUI_QUERY_BOOL },
};

typedef struct { cui_query_node nodes[16]; int n; bool changed, read_only, disabled; cui_size w; } qb_ctx;
/** Build the same query in editable, read-only and disabled modes. */
static void build_qb(cui_ctx *ui, void *u) {
    qb_ctx *c = u;
    cui_push_disabled(ui, c->disabled);
    c->changed = cui_query_builder(ui, "filter", qb_fields, 4, c->nodes, &c->n, 16, .read_only = c->read_only);
    cui_pop_disabled(ui);
}

/** Rendering a read-only query does not initialize or normalize caller storage. */
static void test_query_read_only(void) {
    printf("query builder: read-only preserves caller rules\n");
    cui_ctx *ui = new_ctx(800, 600);
    qb_ctx empty = { .read_only = true };
    run(ui, build_qb, &empty, 1);
    CHECK(empty.n == 0 && !empty.nodes[0].group && !empty.changed);
    qb_ctx rules = { .nodes = {{ .group = true }, { .depth = 1 }}, .n = 2, .read_only = true };
    run(ui, build_qb, &rules, 1);
    CHECK(rules.n == 2 && rules.nodes[1].value[0] == '\0' && !rules.changed);
    empty.read_only = false;
    run(ui, build_qb, &empty, 1);
    CHECK(empty.n == 1 && empty.nodes[0].group && empty.changed);
    /* disabled: the rules stay as given, and the layout is the editable one */
    qb_ctx on = { .nodes = {{ .group = true }, { .depth = 1 }}, .n = 2 };
    qb_ctx off = { .nodes = {{ .group = true }, { .depth = 1 }}, .n = 2, .disabled = true };
    run(ui, build_qb, &on, 2);
    cui_rect on_r = cui_get_rect(ui, cui_make_id(ui, "filter"));
    run(ui, build_qb, &off, 2);
    cui_rect off_r = cui_get_rect(ui, cui_make_id(ui, "filter"));
    CHECK(off.n == 2 && off.nodes[1].value[0] == '\0' && !off.changed);
    CHECK(on.nodes[1].value[0] != '\0');
    CHECK(on_r.h > 0 && off_r.h == on_r.h);
    cui_destroy(ui);
}

static void test_query(void) {
    printf("query builder: operators, text, matching with nested any/all groups, empty, outer group made\n");
    CHECK_STR(cui_query_op_name(CUI_QUERY_TEXT, 0), "is");
    CHECK_STR(cui_query_op_name(CUI_QUERY_TEXT, 2), "contains");
    CHECK_STR(cui_query_op_name(CUI_QUERY_NUMBER, 3), ">=");
    CHECK_STR(cui_query_op_name(CUI_QUERY_BOOL, 1), "is false");
    CHECK(cui_query_op_name(CUI_QUERY_TEXT, 6) == NULL);
    CHECK(cui_query_op_name(CUI_QUERY_BOOL, -1) == NULL);
    /* Status is Open, and (Assignee is me, or Priority > 2) */
    cui_query_node q[5] = {
        { 0, true, false }, { 1, false, false, 0, 0, "Open" }, { 1, true, true }, { 2, false, false, 1, 0, "me" }, { 2, false, false, 2, 2, "2" },
    };
    const char *r1[] = { "Open", "me", "1", "false" }, *r2[] = { "Open", "bob", "3", "false" }, *r3[] = { "Open", "bob", "2", "true" },
               *r4[] = { "Closed", "me", "5", "true" };
    CHECK(cui_query_match(qb_fields, 4, q, 5, r1));
    CHECK(cui_query_match(qb_fields, 4, q, 5, r2));
    CHECK(!cui_query_match(qb_fields, 4, q, 5, r3));
    CHECK(!cui_query_match(qb_fields, 4, q, 5, r4));
    char text[256];
    size_t len = cui_query_text(qb_fields, 4, q, 5, text, sizeof text);
    CHECK(len > 0 && len == strlen(text));
    CHECK(strstr(text, "Status") && strstr(text, "Assignee") && strstr(text, "Priority > 2"));
    char tiny[8];
    size_t full = cui_query_text(qb_fields, 4, q, 5, tiny, sizeof tiny);
    CHECK(full == len && strlen(tiny) == 7);   /* cut, but the whole length is returned */
    /* text operators, numbers that are not numbers, bools */
    cui_query_node t[2] = { { 0, true, false }, { 1, false, false, 1, 2, "ali" } };
    const char *v1[] = { "Open", "Kalina", "0", "true" }, *v2[] = { "Open", "Bob", "0", "true" };
    CHECK(cui_query_match(qb_fields, 4, t, 2, v1));
    CHECK(!cui_query_match(qb_fields, 4, t, 2, v2));
    t[1].op = 4; snprintf(t[1].value, sizeof t[1].value, "Ka");   /* starts with */
    CHECK(cui_query_match(qb_fields, 4, t, 2, v1));
    t[1].op = 5; snprintf(t[1].value, sizeof t[1].value, "ina");   /* ends with */
    CHECK(cui_query_match(qb_fields, 4, t, 2, v1));
    cui_query_node b[2] = { { 0, true, false }, { 1, false, false, 3, 0, "" } };
    CHECK(cui_query_match(qb_fields, 4, b, 2, v1));
    b[1].op = 1;
    CHECK(!cui_query_match(qb_fields, 4, b, 2, v1));
    cui_query_node e[1] = { { 0, true, false } };
    CHECK(cui_query_match(qb_fields, 4, e, 1, v1));   /* no conditions: everything matches */

    /* the widget makes the outer group, and edits in place */
    qb_ctx c;
    memset(&c, 0, sizeof c);
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_qb, &c, 3);
    CHECK(c.n >= 1 && c.nodes[0].group && c.nodes[0].depth == 0);
    memcpy(c.nodes, q, sizeof q); c.n = 5;
    run(ui, build_qb, &c, 3);
    CHECK(c.n == 5 && !c.changed);
    cui_set_viewport(ui, 340, 700, 1);
    run(ui, build_qb, &c, 3);
    CHECK(c.n == 5 && !c.changed);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* key-value editor                                                         */

typedef struct { cui_kv_pair p[4]; int n; bool changed; } kv_ctx;
static void build_kv(cui_ctx *ui, void *u) {
    kv_ctx *c = u;
    c->changed = cui_kv_editor(ui, "kv", c->p, &c->n, 4);
}
static cui_rect kv_field(cui_ctx *ui, int row, const char *which) {
    cui_push_id(ui, "kv"); cui_push_id_int(ui, row);
    cui_id id = cui_make_id(ui, which);
    cui_pop_id(ui); cui_pop_id(ui);
    return cui_get_rect(ui, id);
}
static void test_kv(void) {
    printf("key-value editor: type to add, edit, toggle, remove, capacity, duplicates, narrow\n");
    kv_ctx c;
    memset(&c, 0, sizeof c);
    snprintf(c.p[0].key, 64, "Accept"); snprintf(c.p[0].value, 256, "text/html"); c.p[0].enabled = true;
    c.n = 1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_kv, &c, 3);
    cui_rect k1 = kv_field(ui, 1, "key");
    CHECK(k1.w > 50);
    click(ui, build_kv, &c, k1.x + 10, k1.y + k1.h / 2);
    CHECK(c.n == 1);   /* focusing the empty row adds nothing */
    type(ui, build_kv, &c, "X-Trace");
    run(ui, build_kv, &c, 1);
    CHECK(c.n == 2 && !strcmp(c.p[1].key, "X-Trace") && c.p[1].enabled);
    /* Tab to the value, type */
    key(ui, build_kv, &c, CUI_KEY_TAB, 0);
    type(ui, build_kv, &c, "on");
    CHECK(!strcmp(c.p[1].value, "on"));
    /* the checkbox turns a pair off */
    cui_rect k0 = kv_field(ui, 0, "key");
    cui_rect comp = cui_get_rect(ui, cui_make_id(ui, "kv"));
    click(ui, build_kv, &c, comp.x + 8, k0.y + k0.h / 2);
    CHECK(!c.p[0].enabled);
    click(ui, build_kv, &c, comp.x + 8, k0.y + k0.h / 2);
    CHECK(c.p[0].enabled);
    /* the x at the end of the row removes it */
    click(ui, build_kv, &c, comp.x + comp.w - 14, k0.y + k0.h / 2);
    run(ui, build_kv, &c, 1);
    CHECK(c.n == 1 && !strcmp(c.p[0].key, "X-Trace"));
    /* full: no empty row */
    for (int i = 1; i < 4; i++) { snprintf(c.p[i].key, 64, "K%d", i); c.p[i].enabled = true; }
    c.n = 4;
    run(ui, build_kv, &c, 2);
    CHECK(kv_field(ui, 4, "key").w == 0);
    /* narrow: key above value */
    cui_set_viewport(ui, 360, 700, 1);
    run(ui, build_kv, &c, 3);
    cui_rect nk = kv_field(ui, 0, "key"), nv = kv_field(ui, 0, "value");
    CHECK(nv.y > nk.y + nk.h - 1 && fabsf(nv.x - nk.x) < 1);
    CHECK(c.n == 4);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* problems panel                                                           */

static const cui_diagnostic pb_diags[] = {
    { CUI_SEVERITY_ERROR, "use of undeclared identifier 'frame_time'", "src/timing.c", 42, 17, "clang", "undeclared-var" },
    { CUI_SEVERITY_WARNING, "unused variable 'scale'", "src/timing.c", 88, 11, "clang", "-Wunused-variable" },
    { CUI_SEVERITY_ERROR, "incompatible pointer types", "src/layout.c", 210, 33, "clang", NULL },
    { CUI_SEVERITY_INFO, "could be static", "src/layout.c", 402, 6, "clang-tidy", NULL },
    { CUI_SEVERITY_HINT, "loop variable copied", NULL, 0, 0, NULL, NULL },
};
typedef struct { cui_problems_result r; int activated; int n; const cui_diagnostic *d; } pb_ctx;
static void build_pb(cui_ctx *ui, void *u) {
    pb_ctx *c = u;
    c->r = cui_problems(ui, "problems", c->d, c->n, .height = 320);
    if (c->r.activated >= 0) c->activated = c->r.activated;
}
static void test_problems(void) {
    printf("problems panel: counts, select, keys, activate, fold, filter by text, empty, 20k diagnostics\n");
    pb_ctx c;
    memset(&c, 0, sizeof c);
    c.d = pb_diags; c.n = 5; c.activated = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_pb, &c, 3);
    CHECK(c.r.shown == 5 && c.r.selected == -1);
    cui_rect pr = cui_get_rect(ui, cui_make_id(ui, "problems"));
    /* click down the panel until a diagnostic is selected */
    float y = pr.y + 40;
    for (; y < pr.y + pr.h && c.r.selected < 0; y += 8) click(ui, build_pb, &c, pr.x + pr.w * 0.5f, y);
    CHECK(c.r.selected >= 0);
    int s0 = c.r.selected;
    key(ui, build_pb, &c, CUI_KEY_DOWN, 0);
    CHECK(c.r.selected != s0 && c.r.selected >= 0);
    key(ui, build_pb, &c, CUI_KEY_ENTER, 0);
    CHECK(c.activated == c.r.selected);
    c.activated = -1;
    double_click(ui, build_pb, &c, pr.x + pr.w * 0.5f, y - 8);
    CHECK(c.activated == s0);
    /* Up from the first diagnostic of a file reaches its header; Left folds it */
    for (int i = 0; i < 8; i++) key(ui, build_pb, &c, CUI_KEY_UP, 0);
    CHECK(c.r.selected == -1);   /* a header is selected */
    key(ui, build_pb, &c, CUI_KEY_LEFT, 0);
    CHECK(c.r.shown == 5);       /* folding hides rows, not counts */
    key(ui, build_pb, &c, CUI_KEY_DOWN, 0);
    CHECK(c.r.selected == -1);   /* the next row is the next file's header */
    key(ui, build_pb, &c, CUI_KEY_DOWN, 0);
    CHECK(c.r.selected >= 0);
    /* filter by text: the search field takes typing */
    c.n = 0;
    run(ui, build_pb, &c, 2);
    CHECK(c.r.shown == 0 && c.r.selected == -1);
    /* many: only the rows in view are built */
    static cui_diagnostic many[20000];
    static char names[200][24];
    for (int i = 0; i < 200; i++) snprintf(names[i], sizeof names[i], "src/f%03d.c", i);
    for (int i = 0; i < 20000; i++) many[i] = (cui_diagnostic){ (cui_severity)(i % 4), "something is off here", names[i % 200], i + 1, 3, "gcc", NULL };
    c.d = many; c.n = 20000;
    clock_t c0 = clock();
    run(ui, build_pb, &c, 30);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 30;
    printf("  20k diagnostics: %.2f ms a frame\n", ms);
    CHECK(c.r.shown == 20000);
    CHECK(ms < 40);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* pipeline                                                                 */

static const char *const pl_stages[] = { "Build", "Test", "Deploy" };
static const cui_pipeline_job pl_jobs[] = {
    { "linux", 0, CUI_JOB_PASSED, 142 }, { "windows", 0, CUI_JOB_PASSED, 236 },
    { "unit", 1, CUI_JOB_RUNNING, 64 }, { "lint", 1, CUI_JOB_FAILED, 12, true },
    { "production", 2, CUI_JOB_PENDING, -1 }, { "orphan", 7, CUI_JOB_PASSED, 1 },   /* a stage that does not exist */
};
typedef struct { cui_pipeline_result r; int sel; } pl_ctx;
static void build_pl(cui_ctx *ui, void *u) {
    pl_ctx *c = u;
    c->r = cui_pipeline(ui, "run", pl_stages, 3, pl_jobs, 6, .selected = &c->sel);
}
static void test_pipeline(void) {
    printf("pipeline: hover and click every job, wide and stacked, right to left\n");
    for (int pass = 0; pass < 3; pass++) {
        pl_ctx c;
        memset(&c, 0, sizeof c);
        c.sel = -1;
        cui_ctx *ui = pass == 2 ? cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .direction = CUI_DIRECTION_RTL }) : new_ctx(800, 500);
        cui_set_reduced_motion(ui, true);
        cui_set_viewport(ui, pass == 1 ? 380 : 800, 700, 1);
        run(ui, build_pl, &c, 3);
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "run"));
        CHECK(r.w > 0);
        for (int j = 0; j < 5; j++) {
            float x, y;
            bool found = find(ui, build_pl, &c, r, &c.r.hovered, j, &x, &y);
            if (!found) printf("  pass %d: job %d not found\n", pass, j);
            CHECK(found);
            if (!found) continue;
            click(ui, build_pl, &c, x, y);
            CHECK(c.sel == j);
        }
        cui_destroy(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* keymap                                                                   */

typedef struct { cui_keymap_result r; cui_key clicked; int mods; const cui_keymap_binding *b; int n; } km_ctx;
static void build_km(cui_ctx *ui, void *u) {
    km_ctx *c = u;
    static const char *cats[] = { "File", "Edit" };
    c->r = cui_keymap(ui, "keys", c->b, c->n, .categories = cats, .category_count = 2, .mods = &c->mods);
    if (c->r.clicked) c->clicked = c->r.clicked;
}
static void test_keymap(void) {
    printf("keymap: conflicts, clicking a key, modifiers, narrow, right to left\n");
    static const cui_keymap_binding b[] = {
        { { CUI_MOD_PRIMARY, CUI_KEY_S }, "Save", 0 }, { { CUI_MOD_PRIMARY, CUI_KEY_R }, "Run", 1 },
        { { CUI_MOD_PRIMARY, CUI_KEY_R }, "Rename", 1 }, { { 0, CUI_KEY_F5 }, "Continue", 1 }, { { CUI_MOD_SHIFT, CUI_KEY_F5 }, "Stop", 1 },
    };
    km_ctx c;
    memset(&c, 0, sizeof c);
    c.b = b; c.n = 5; c.mods = CUI_MOD_PRIMARY;
    cui_ctx *ui = new_ctx(900, 600);
    run(ui, build_km, &c, 3);
    CHECK_EQ(c.r.conflicts, 1);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "keys"));
    /* click across the keyboard until a key reports itself */
    for (float y = r.y + r.h * 0.4f; y < r.y + r.h && !c.clicked; y += 9)
        for (float x = r.x + 20; x < r.x + r.w && !c.clicked; x += 23) click(ui, build_km, &c, x, y);
    CHECK(c.clicked != CUI_KEY_NONE);
    c.b = b; c.n = 2;
    run(ui, build_km, &c, 2);
    CHECK_EQ(c.r.conflicts, 0);
    c.n = 0; c.b = NULL;
    run(ui, build_km, &c, 2);
    CHECK_EQ(c.r.conflicts, 0);
    cui_set_viewport(ui, 330, 600, 1);
    c.b = b; c.n = 5;
    run(ui, build_km, &c, 3);
    CHECK_EQ(c.r.conflicts, 1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* commit graph                                                             */

static const cui_commit cg[] = {
    { "a91f3c2e", "Merge branch 'feature'", "Maya", "2 hours ago", "main, origin/main", { 1, 3 } },
    { "7be04d19", "Docs", "Maya", "3 hours ago", NULL, { 2, -1 } },
    { "c3d8a7f0", "Fix the ruler", "Jonas", "5 hours ago", "tag: v1.2.0", { 4, -1 } },
    { "e8a0c55b", "Feature work", "Priya", "yesterday", "feature", { 4, -1 } },
    { "0aa4c9e8", "Start", "Omid", "5 days ago", NULL, { -1, 99 } },   /* a parent out of range is ignored */
};
typedef struct { cui_commit_graph_result r; int sel, activated; const cui_commit *c; int n; } cg_ctx;
static void build_cg(cui_ctx *ui, void *u) {
    cg_ctx *c = u;
    c->r = cui_commit_graph(ui, "log", c->c, c->n, .selected = &c->sel, .height = 300);
    if (c->r.activated >= 0) c->activated = c->r.activated;
}
static void test_commits(void) {
    printf("commit graph: hover, click, keys, activate, bad parents, 50k commits\n");
    cg_ctx c;
    memset(&c, 0, sizeof c);
    c.c = cg; c.n = 5; c.sel = -1; c.activated = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_cg, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "log"));
    float x, y;
    CHECK(find(ui, build_cg, &c, r, &c.r.hovered, 2, &x, &y));
    click(ui, build_cg, &c, x, y);
    CHECK(c.sel == 2);
    key(ui, build_cg, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 3);
    key(ui, build_cg, &c, CUI_KEY_UP, 0); key(ui, build_cg, &c, CUI_KEY_UP, 0); CHECK(c.sel == 1);
    key(ui, build_cg, &c, CUI_KEY_ENTER, 0); CHECK(c.activated == 1);
    double_click(ui, build_cg, &c, x, y); CHECK(c.activated == 2);
    /* a long linear history with a branch every so often */
    static cui_commit many[50000];
    for (int i = 0; i < 50000; i++) {
        many[i] = (cui_commit){ "0123abcd", "Commit message", "Someone", "today", i % 997 == 0 ? "tag: x" : NULL, { i + 1 < 50000 ? i + 1 : -1, i % 50 == 0 && i + 7 < 50000 ? i + 7 : -1 } };
    }
    c.c = many; c.n = 50000; c.sel = -1;
    clock_t c0 = clock();
    run(ui, build_cg, &c, 1);
    double first = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000;
    c0 = clock();
    run(ui, build_cg, &c, 30);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 30;
    printf("  50k commits: layout %.1f ms, then %.2f ms a frame\n", first, ms);
    CHECK(ms < 20);
    c.n = 0;
    run(ui, build_cg, &c, 2);
    CHECK(c.sel == -1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* dependency graph                                                         */

typedef struct { cui_graph_result r; int sel, activated; const cui_graph_node *nodes; int n; const cui_graph_edge *edges; int e; bool td; } gv_ctx;
static void build_gv(cui_ctx *ui, void *u) {
    gv_ctx *c = u;
    c->r = cui_graph_view(ui, "deps", c->nodes, c->n, c->edges, c->e, .selected = &c->sel, .height = 360, .top_down = c->td);
    if (c->r.activated >= 0) c->activated = c->r.activated;
}
static void test_graph(void) {
    printf("dependency graph: every node reachable, click, keys, cycles, bad edges, top down, 400 nodes\n");
    static const cui_graph_node nodes[] = { { "app" }, { "ui", "libcui" }, { "net" }, { "tls" }, { "zlib" }, { "platform" } };
    static const cui_graph_edge edges[] = { { 0, 1 }, { 0, 2 }, { 2, 3 }, { 1, 4 }, { 3, 4 }, { 1, 5 }, { 0, 5 }, { 4, 0 } /* a cycle */, { 3, 3 }, { 9, 1 }, { -1, 2 } };
    gv_ctx c;
    memset(&c, 0, sizeof c);
    c.nodes = nodes; c.n = 6; c.edges = edges; c.e = 11; c.sel = -1; c.activated = -1;
    cui_ctx *ui = new_ctx(900, 600);
    for (int td = 0; td < 2; td++) {
        c.td = td != 0;
        run(ui, build_gv, &c, 4);
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "deps"));
        for (int v = 0; v < 6; v++) {
            float x, y;
            bool found = find(ui, build_gv, &c, r, &c.r.hovered, v, &x, &y);
            if (!found) printf("  %s: node %d not found\n", td ? "top down" : "left to right", v);
            CHECK(found);
            if (found && v == 1) {
                click(ui, build_gv, &c, x, y);
                CHECK(c.sel == 1);
                double_click(ui, build_gv, &c, x, y);
                CHECK(c.activated == 1);
            }
        }
        move(ui, build_gv, &c, r.x + 2, r.y + 2);
        key(ui, build_gv, &c, CUI_KEY_ESCAPE, 0);
        CHECK(c.sel == -1);
    }
    /* a wide random acyclic graph: lays out once, then frames are cheap */
    static cui_graph_node big[400];
    static cui_graph_edge bige[900];
    static char labels[400][12];
    srand(7);
    for (int i = 0; i < 400; i++) { snprintf(labels[i], sizeof labels[i], "mod%03d", i); big[i] = (cui_graph_node){ labels[i], NULL, (cui_tone)(i % 5) }; }
    for (int k = 0; k < 900; k++) { int a = rand() % 399; bige[k] = (cui_graph_edge){ a, a + 1 + rand() % (400 - a - 1) }; }
    c.nodes = big; c.n = 400; c.edges = bige; c.e = 900; c.sel = -1; c.td = false;
    clock_t c0 = clock();
    run(ui, build_gv, &c, 1);
    double first = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000;
    c0 = clock();
    run(ui, build_gv, &c, 20);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 20;
    printf("  400 nodes, 900 edges: layout %.1f ms, then %.2f ms a frame\n", first, ms);
    CHECK(first < 500 && ms < 60);
    c.n = 0; c.e = 0;
    run(ui, build_gv, &c, 2);
    CHECK(c.sel == -1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* network waterfall                                                        */

static const cui_net_request wf[] = {
    { "https://example.com/", "GET", 200, "document", 18400, 0.000, { 0.002, 0.018, 0.024, 0.031, 0.142, 0.012 } },
    { "https://example.com/app.js", "GET", 200, "script", 186000, 0.233, { 0.005, 0, 0, 0, 0.068, 0.094 } },
    { "https://example.com/api/session", "POST", 401, "fetch", 312, 0.402, { 0.002, 0, 0, 0, 0.083, 0.001 } },
    { "https://example.com/collect", NULL, 0, NULL, 0, 0.7, { 0, 0, 0, 0, 0, 0 } },
};
typedef struct { cui_waterfall_result r; int sel, activated; const cui_net_request *q; int n; } wf_ctx;
static void build_wf(cui_ctx *ui, void *u) {
    wf_ctx *c = u;
    c->r = cui_waterfall(ui, "net", c->q, c->n, .selected = &c->sel, .dom_loaded = 0.39, .loaded = 0.68);
    if (c->r.activated >= 0) c->activated = c->r.activated;
}
static void test_waterfall(void) {
    printf("network waterfall: hover, click, keys, activate, narrow, 30k requests\n");
    wf_ctx c;
    memset(&c, 0, sizeof c);
    c.q = wf; c.n = 4; c.sel = -1; c.activated = -1;
    cui_ctx *ui = new_ctx(900, 500);
    for (int narrow = 0; narrow < 2; narrow++) {
        cui_set_viewport(ui, narrow ? 360 : 900, 500, 1);
        run(ui, build_wf, &c, 3);
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "net"));
        for (int i = 0; i < 4; i++) {
            float x, y;
            bool found = find(ui, build_wf, &c, r, &c.r.hovered, i, &x, &y);
            CHECK(found);
            if (found && i == 2) { click(ui, build_wf, &c, x, y); CHECK(c.sel == 2 && c.r.clicked == 2); }
        }
        key(ui, build_wf, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 3);
        key(ui, build_wf, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 3);
        key(ui, build_wf, &c, CUI_KEY_HOME, 0); CHECK(c.sel == 0);
        key(ui, build_wf, &c, CUI_KEY_ENTER, 0); CHECK(c.activated == 0);
        key(ui, build_wf, &c, CUI_KEY_ESCAPE, 0); CHECK(c.sel == -1);
    }
    static cui_net_request many[30000];
    for (int i = 0; i < 30000; i++) many[i] = (cui_net_request){ "https://example.com/img/x.webp", "GET", 200, "image", 1000 + i, i * 0.001, { 0.001, 0, 0, 0, 0.02, 0.01 } };
    c.q = many; c.n = 30000; c.sel = -1;
    clock_t c0 = clock();
    run(ui, build_wf, &c, 30);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 30;
    printf("  30k requests: %.2f ms a frame\n", ms);
    CHECK(ms < 30);
    c.n = 0;
    run(ui, build_wf, &c, 2);
    CHECK(c.sel == -1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* trace viewer                                                             */

typedef struct {
    cui_trace_result r;
    cui_trace_track tracks[3];
    cui_trace_span *s; int n;
    cui_trace_view view;
    int sel;
} tr_ctx;
static void build_tr(cui_ctx *ui, void *u) {
    tr_ctx *c = u;
    static const cui_trace_marker marks[] = { { "vsync", 0.0, CUI_TONE_NEUTRAL }, { "GC", 0.012, CUI_TONE_WARNING } };
    c->r = cui_trace_view(ui, "trace", c->tracks, 3, c->s, c->n, &c->view, &c->sel, .markers = marks, .marker_count = 2, .height = 300);
}
static void test_trace(void) {
    printf("trace viewer: sort, fit, hover, click, tree keys, fold a lane, zoom, pan, measure, 200k spans\n");
    static cui_trace_span sp[] = {
        { "Paint", 0.011, 0.015, 0, 1 }, { "Frame", 0.000, 0.016, 0, 0 }, { "Update", 0.001, 0.009, 0, 1 },
        { "Physics", 0.002, 0.005, 0, 2 }, { "Submit", 0.016, 0.018, 1, 0 }, { "Decode", 0.003, 0.010, 2, 0 },
    };
    cui_trace_sort(sp, 6);
    CHECK(!strcmp(sp[0].name, "Frame") && !strcmp(sp[1].name, "Update") && !strcmp(sp[2].name, "Paint") && !strcmp(sp[3].name, "Physics"));
    tr_ctx c;
    memset(&c, 0, sizeof c);
    c.tracks[0].name = "Main"; c.tracks[1].name = "Render"; c.tracks[2].name = "Worker";
    c.s = sp; c.n = 6; c.sel = -1;
    cui_ctx *ui = new_ctx(900, 500);
    run(ui, build_tr, &c, 3);
    CHECK(c.view.start == 0 && fabs(c.view.end - 0.018) < 1e-12);   /* fits the spans and markers */
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "trace"));
    float x, y;
    CHECK(find(ui, build_tr, &c, r, &c.r.hovered, 3, &x, &y));   /* Physics */
    click(ui, build_tr, &c, x, y);
    CHECK(c.sel == 3 && c.r.selection_changed);
    key(ui, build_tr, &c, CUI_KEY_UP, 0); CHECK(c.sel == 1);     /* Update */
    key(ui, build_tr, &c, CUI_KEY_RIGHT, 0); CHECK(c.sel == 2);  /* Paint */
    key(ui, build_tr, &c, CUI_KEY_UP, 0); CHECK(c.sel == 0);     /* Frame */
    key(ui, build_tr, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 1);
    key(ui, build_tr, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 3);
    key(ui, build_tr, &c, CUI_KEY_ESCAPE, 0); CHECK(c.sel == -1);
    /* the wheel zooms in around the pointer */
    move(ui, build_tr, &c, r.x + r.w * 0.6f, y);
    double t_at = c.view.start + 0.5 * (c.view.end - c.view.start);
    (void)t_at;
    cui_input_scroll(ui, 0, 4);
    run(ui, build_tr, &c, 1);
    CHECK(c.view.end - c.view.start < 0.018 * 0.7);
    CHECK(c.view.start >= 0 && c.view.end <= 0.018 + 1e-12);
    key(ui, build_tr, &c, CUI_KEY_F, 0);
    run(ui, build_tr, &c, 2);
    CHECK(fabs(c.view.end - c.view.start - 0.018) < 1e-9);
    /* Shift+drag measures */
    double mark0 = c.view.mark_start, mark1 = c.view.mark_end;
    cui_input_mouse_move(ui, r.x + r.w * 0.4f, y); run(ui, build_tr, &c, 1);
    cui_input_key(ui, CUI_KEY_NONE, false, CUI_MOD_SHIFT);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_tr, &c, 1);
    for (int k = 1; k <= 4; k++) {
        cui_input_mouse_move(ui, r.x + r.w * (0.4f + 0.05f * k), y);
        run(ui, build_tr, &c, 1);
        CHECK(c.view.mark_start != mark0 || c.view.mark_end != mark1);
        CHECK(c.r.view_changed);
        mark0 = c.view.mark_start; mark1 = c.view.mark_end;
    }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_tr, &c, 1);
    CHECK(!c.r.view_changed);
    cui_input_key(ui, CUI_KEY_NONE, false, 0);
    CHECK(c.view.mark_end > c.view.mark_start);
    CHECK(fabs(c.view.end - c.view.start - 0.018) < 1e-9);   /* measuring does not pan */
    key(ui, build_tr, &c, CUI_KEY_ESCAPE, 0);
    CHECK(!(c.view.mark_end > c.view.mark_start));
    CHECK(c.r.view_changed);
    /* a plain drag pans */
    cui_input_scroll(ui, 0, 4); run(ui, build_tr, &c, 1);
    double s0 = c.view.start;
    cui_input_mouse_move(ui, r.x + r.w * 0.6f, r.y + r.h - 20); run(ui, build_tr, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_tr, &c, 1);
    for (int k = 1; k <= 4; k++) { cui_input_mouse_move(ui, r.x + r.w * 0.6f - 30 * k, r.y + r.h - 20); run(ui, build_tr, &c, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_tr, &c, 1);
    CHECK(c.view.start > s0);
    /* a click on a lane's name folds it to its top row */
    cui_rect r2 = cui_get_rect(ui, cui_make_id(ui, "trace"));
    for (float yy = r2.y + 30; yy < r2.y + 120 && !c.tracks[0].collapsed; yy += 6) click(ui, build_tr, &c, r2.x + 30, yy);
    CHECK(c.tracks[0].collapsed);
    CHECK(!find(ui, build_tr, &c, r2, &c.r.hovered, 3, &x, &y));   /* Physics is hidden */
    click(ui, build_tr, &c, r2.x + 30, r2.y + 40);
    /* many spans: only what is in view is visited */
    int n = 200000;
    cui_trace_span *big = malloc(sizeof *big * (size_t)n);
    for (int i = 0; i < n; i++) {
        int t = i % 3, d = (i / 3) % 4;
        int k = i / 12;
        big[i] = (cui_trace_span){ "work", k * 1e-4 + d * 1e-6, k * 1e-4 + 8e-5 - d * 1e-6, t, d, 0 };
    }
    cui_trace_sort(big, n);
    c.s = big; c.n = n; c.sel = -1; c.view.start = c.view.end = 0;
    c.tracks[0].collapsed = false;
    clock_t c0 = clock();
    run(ui, build_tr, &c, 30);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 30;
    printf("  200k spans, all in view: %.2f ms a frame\n", ms);
    CHECK(ms < 30);
    free(big);
    c.s = NULL; c.n = 0;
    run(ui, build_tr, &c, 2);
    CHECK(c.sel == -1);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* everything, at three widths, both directions, both themes, under random input */

static unsigned g_seed = 1;
static unsigned rnd(void) { g_seed = g_seed * 1103515245u + 12345u; return (g_seed >> 8) & 0xFFFFFF; }

typedef struct {
    kv_ctx kv; pb_ctx pb; pl_ctx pl; qb_ctx qb; sc_ctx sc; km_ctx km; cg_ctx cg; gv_ctx gv; wf_ctx wf; tr_ctx tr;
} all_ctx;
static void build_all(cui_ctx *ui, void *u) {
    all_ctx *a = u;
    cui_scroll(ui, "page", .width = CUI_FILL, .height = CUI_FILL) {
        cui_column(ui, .gap = 16, .width = CUI_FILL) {
            build_kv(ui, &a->kv); build_pb(ui, &a->pb); build_pl(ui, &a->pl); build_qb(ui, &a->qb); build_sc(ui, &a->sc);
            build_km(ui, &a->km); build_cg(ui, &a->cg); build_gv(ui, &a->gv); build_wf(ui, &a->wf); build_tr(ui, &a->tr);
        }
    }
}
static void test_random(void) {
    printf("all ten under random input: 3 widths x 2 directions x 2 themes\n");
    static cui_trace_span sp[] = {
        { "Frame", 0.000, 0.016, 0, 0 }, { "Update", 0.001, 0.009, 0, 1 }, { "Physics", 0.002, 0.005, 0, 2 }, { "Decode", 0.003, 0.010, 2, 0 },
    };
    static const cui_keymap_binding kb[] = { { { CUI_MOD_PRIMARY, CUI_KEY_S }, "Save", 0 }, { { 0, CUI_KEY_F5 }, "Run", 1 } };
    static const cui_graph_node gn[] = { { "a" }, { "b" }, { "c" } };
    static const cui_graph_edge ge[] = { { 0, 1 }, { 1, 2 }, { 0, 2 }, { 2, 0 } };
    static const cui_key keys[] = { CUI_KEY_UP, CUI_KEY_DOWN, CUI_KEY_LEFT, CUI_KEY_RIGHT, CUI_KEY_ENTER, CUI_KEY_ESCAPE, CUI_KEY_TAB,
                                    CUI_KEY_BACKSPACE, CUI_KEY_F, CUI_KEY_EQUAL, CUI_KEY_MINUS, CUI_KEY_HOME, CUI_KEY_END, CUI_KEY_SPACE, CUI_KEY_DELETE };
    static const char *texts[] = { "a", "X-", " ", "\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85", "*/5", "9" };
    long frames = 0;
    for (int cfg = 0; cfg < 12; cfg++) {
        static const float widths[3] = { 340, 760, 1280 };
        bool rtl = cfg & 1, dark = (cfg >> 1) & 1;
        float w = widths[cfg / 4];
        all_ctx *a = calloc(1, sizeof *a);
        a->kv.n = 0;
        a->pb.d = pb_diags; a->pb.n = 5; a->pb.activated = -1;
        a->pl.sel = -1;
        snprintf(a->sc.cron, sizeof a->sc.cron, "30 9 * * 1-5");
        a->km.b = kb; a->km.n = 2; a->km.mods = CUI_MOD_PRIMARY;
        a->cg.c = cg; a->cg.n = 5; a->cg.sel = -1;
        a->gv.nodes = gn; a->gv.n = 3; a->gv.edges = ge; a->gv.e = 4; a->gv.sel = -1;
        a->wf.q = wf; a->wf.n = 4; a->wf.sel = -1;
        a->tr.tracks[0].name = "Main"; a->tr.tracks[1].name = "Render"; a->tr.tracks[2].name = "Worker";
        a->tr.s = sp; a->tr.n = 4; a->tr.sel = -1;
        cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                                 .direction = rtl ? CUI_DIRECTION_RTL : CUI_DIRECTION_INHERIT });
        cui_set_viewport(ui, w, 900, 1);
        g_seed = (unsigned)cfg * 7919u + 3;
        for (int f = 0; f < 700; f++) {
            unsigned r = rnd();
            switch (r % 10) {
            case 0: case 1: case 2: cui_input_mouse_move(ui, (float)(rnd() % (unsigned)w), (float)(rnd() % 900)); break;
            case 3: cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); break;
            case 4: cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); break;
            case 5: cui_input_scroll(ui, (rnd() % 3) == 0 ? 1.0f : 0.0f, (float)((int)(rnd() % 7) - 3)); break;
            case 6: { cui_key k = keys[rnd() % (sizeof keys / sizeof keys[0])]; int m = rnd() % 4 == 0 ? CUI_MOD_SHIFT : 0;
                      cui_input_key(ui, k, true, m); cui_input_key(ui, k, false, m); break; }
            case 7: cui_input_text(ui, texts[rnd() % (sizeof texts / sizeof texts[0])]); break;
            case 8: cui_input_key(ui, CUI_KEY_NONE, false, rnd() % 2 ? CUI_MOD_SHIFT : 0); break;
            default: break;
            }
            run(ui, build_all, a, 1);
            frames++;
            /* everything the app owns stays in range */
            if (a->kv.n < 0 || a->kv.n > 4 || a->qb.n < 1 || a->qb.n > 16 || a->pl.sel >= 6 || a->cg.sel >= 5 || a->gv.sel >= 3 || a->wf.sel >= 4 || a->tr.sel >= 4) {
                printf("  out of range at config %d frame %d\n", cfg, f);
                CHECK(false);
                break;
            }
            if (strlen(a->sc.cron) >= sizeof a->sc.cron) { CHECK(false); break; }
        }
        const cui_draw_data *dd = cui_get_draw_data(ui);
        unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
        cui_sw_render(g_sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
        free(px);
        cui_destroy(ui);
        free(a);
    }
    printf("  %ld frames\n", frames);
    CHECK(frames == 12 * 700);
}

/* ======================================================================== */
/* the second batch                                                         */
/* ======================================================================== */

static int rx(const char *p, int fl, const char *s, int from, int *a, int *b) {
    cui_regex_match m;
    int r = cui_regex_search(p, fl, s, -1, from, &m, NULL);
    if (r == 1) { *a = m.start; *b = m.end; }
    return r;
}
static void test_regex(void) {
    printf("regex: literals, classes, groups, alternation, quantifiers, lazy, anchors, flags, names, UTF-8, errors, budget\n");
    int a = 0, b = 0;
    CHECK(rx("a+b", 0, "xxaaab", 0, &a, &b) == 1 && a == 2 && b == 6);
    CHECK(rx("a+?b", 0, "aaab", 0, &a, &b) == 1 && a == 0 && b == 4);
    CHECK(rx("<.+?>", 0, "<a><b>", 0, &a, &b) == 1 && b == 3);
    CHECK(rx("<.+>", 0, "<a><b>", 0, &a, &b) == 1 && b == 6);
    CHECK(rx("^\\w+$", 0, "hello world", 0, &a, &b) == 0);
    CHECK(rx("^world$", CUI_REGEX_MULTILINE, "hello\nworld\n", 0, &a, &b) == 1 && a == 6);
    CHECK(rx("x{2,3}", 0, "xxxx", 0, &a, &b) == 1 && b == 3);
    CHECK(rx("x{2,}", 0, "xxxxx", 0, &a, &b) == 1 && b == 5);
    CHECK(rx("a{,2}", 0, "a{,2}", 0, &a, &b) == 1 && b == 5);   /* not a repeat: literal braces */
    CHECK(rx("[^aeiou\\s]+", 0, "aei bcd", 0, &a, &b) == 1 && a == 4 && b == 7);
    CHECK(rx("[a-c-]+", 0, "x-ab-c", 0, &a, &b) == 1 && a == 1 && b == 6);
    CHECK(rx("HELLO", CUI_REGEX_ICASE, "say hello", 0, &a, &b) == 1 && a == 4);
    CHECK(rx("[A-Z]+", CUI_REGEX_ICASE, "abc", 0, &a, &b) == 1 && b == 3);
    CHECK(rx("\\bcat\\b", 0, "concat cat", 0, &a, &b) == 1 && a == 7);
    CHECK(rx("\\Bcat", 0, "cat concat", 0, &a, &b) == 1 && a == 7);
    CHECK(rx("a.c", 0, "a\nc abc", 0, &a, &b) == 1 && a == 4);
    CHECK(rx("a.c", CUI_REGEX_DOTALL, "a\nc", 0, &a, &b) == 1 && a == 0);
    CHECK(rx("(foo|foobar)baz", 0, "foobarbaz", 0, &a, &b) == 1 && b == 9);   /* backtracks into the alternation */
    CHECK(rx("\\d+\\.\\d+", 0, "v 10.25", 0, &a, &b) == 1 && a == 2 && b == 7);
    CHECK(rx("\\x41+", 0, "zAAA", 0, &a, &b) == 1 && a == 1);
    CHECK(rx(".", 0, "\xc3\xa9t", 0, &a, &b) == 1 && b == 2);                  /* a whole character */
    CHECK(rx("t", 0, "\xc3\xa9t\xc3\xa9", 1, &a, &b) == 1 && a == 2);           /* `from` inside a character moves on */
    CHECK(rx("", 0, "abc", 0, &a, &b) == 1 && a == 0 && b == 0);
    CHECK(rx("x*", 0, "abc", 2, &a, &b) == 1 && a == 2 && b == 2);
    CHECK(rx("abc", 0, "ab", 0, &a, &b) == 0);
    cui_regex_match m;
    CHECK(cui_regex_search("(?<y>\\d{4})-(\\d\\d)(-(\\d\\d))?", 0, "on 2026-11", -1, 0, &m, NULL) == 1);
    CHECK(m.groups == 4 && !strcmp(m.group_name[1], "y") && m.group_start[1] == 3 && m.group_end[1] == 7 && m.group_start[2] == 8);
    CHECK(m.group_start[3] == -1 && m.group_end[3] == -1);   /* an optional group that did not take part */
    CHECK(cui_regex_search("(a)|(b)", 0, "b", -1, 0, &m, NULL) == 1 && m.group_start[1] == -1 && m.group_start[2] == 0);
    CHECK(cui_regex_search("(\\w)+", 0, "abc", -1, 0, &m, NULL) == 1 && m.group_start[1] == 2);   /* the last turn */
    static const char *bad[] = { "(", "a)", "[a-", "*a", "a{3,1}", "(?=a)", "\\", "x\\q", "[z-a]", "a**" };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        const char *err = NULL;
        int r = cui_regex_search(bad[i], 0, "a", -1, 0, &m, &err);
        if (r != -1 || !err) printf("  accepted \"%s\"\n", bad[i]);
        CHECK(r == -1 && err && err[0]);
    }
    /* patterns that backtrack for ever stop, quickly */
    clock_t c0 = clock();
    const char *err = NULL;
    CHECK(cui_regex_search("(a*)*b", 0, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac", -1, 0, &m, &err) == -2 && err);
    CHECK(cui_regex_search("(x+x+)+y", 0, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", -1, 0, &m, &err) == -2);
    static char big[200001];
    memset(big, 'a', 200000);
    CHECK(cui_regex_search(".*", 0, big, -1, 0, &m, NULL) == 1 && m.end == 200000);
    CHECK(cui_regex_search("a*$", 0, big, -1, 0, &m, NULL) == 1 && m.end == 200000);
    int r = cui_regex_search("a*b", 0, big, -1, 0, &m, NULL);
    CHECK(r == 0 || r == -2);
    CHECK((double)(clock() - c0) / CLOCKS_PER_SEC < 3.0);
    CHECK(cui_regex_search(NULL, 0, "a", -1, 0, &m, NULL) == 0);
}

/* a (?: ) group has no capture slot: matching it used to write cap[-1] */
static void test_regex_noncapture(void) {
    printf("regex: non-capturing groups match and leave the captures around them intact\n");
    int a = 0, b = 0;
    CHECK(rx("(?:a)b", 0, "ab", 0, &a, &b) == 1 && a == 0 && b == 2);
    CHECK(rx("(?:ab)+c", 0, "xababc", 0, &a, &b) == 1 && a == 1 && b == 6);
    CHECK(rx("(?:a|b|c)*d", 0, "abcabcd", 0, &a, &b) == 1 && a == 0 && b == 7);
    CHECK(rx("(?:x)y", 0, "xz", 0, &a, &b) == 0);
    cui_regex_match m;
    CHECK(cui_regex_search("(a)(?:b)(c)", 0, "abc", -1, 0, &m, NULL) == 1);
    CHECK(m.groups == 2 && m.group_start[1] == 0 && m.group_end[1] == 1 && m.group_start[2] == 2 && m.group_end[2] == 3);
    CHECK(cui_regex_search("x(?:y)?(z)?", 0, "x", -1, 0, &m, NULL) == 1 && m.groups == 1 && m.group_start[1] == -1);
}

typedef struct { char pat[128]; char text[512]; int flags; cui_regex_tester_result r; } rt_ctx;
static void build_rt(cui_ctx *ui, void *u) {
    rt_ctx *c = u;
    c->r = cui_regex_tester(ui, "rx", c->pat, sizeof c->pat, c->text, sizeof c->text, .flags = &c->flags);
}
static void test_regex_tester(void) {
    printf("regex tester: counts, groups, flags, invalid, empty matches, typing the pattern\n");
    rt_ctx c;
    memset(&c, 0, sizeof c);
    snprintf(c.pat, sizeof c.pat, "(\\w+)@(\\w+)\\.com");
    snprintf(c.text, sizeof c.text, "bob@example.com, ann@test.com\nBOB@EXAMPLE.COM");
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_rt, &c, 3);
    CHECK(c.r.valid && c.r.matches == 2);
    c.flags = CUI_REGEX_ICASE;
    run(ui, build_rt, &c, 2);
    CHECK(c.r.matches == 3);
    snprintf(c.pat, sizeof c.pat, "(unclosed");
    run(ui, build_rt, &c, 2);
    CHECK(!c.r.valid && c.r.error && c.r.matches == 0);
    snprintf(c.pat, sizeof c.pat, "x*");
    run(ui, build_rt, &c, 2);
    CHECK(c.r.valid && c.r.matches == (int)strlen(c.text) + 1);   /* an empty match at every position */
    c.pat[0] = 0;
    run(ui, build_rt, &c, 2);
    CHECK(c.r.valid && c.r.matches == 0);
    /* type a pattern into the field */
    cui_push_id(ui, "rx");
    cui_id pid = cui_make_id(ui, "pattern");
    cui_pop_id(ui);
    cui_rect pr = cui_get_rect(ui, pid);
    click(ui, build_rt, &c, pr.x + pr.w * 0.5f, pr.y + pr.h * 0.5f);
    type(ui, build_rt, &c, "\\.com");
    run(ui, build_rt, &c, 1);
    CHECK(!strcmp(c.pat, "\\.com") && c.r.matches == 3);
    cui_set_viewport(ui, 340, 700, 1);
    run(ui, build_rt, &c, 3);
    CHECK(c.r.matches == 3);
    cui_destroy(ui);
}

static void test_url(void) {
    printf("URL parameters: decode, encode, fragments, disabled pairs, capacity; HTTP reasons\n");
    cui_kv_pair p[8];
    int n = cui_url_params("https://x.dev/a?q=hello+world&tag=c%2B%2B&empty&k=v#top", p, 8);
    CHECK(n == 4);
    CHECK(!strcmp(p[0].key, "q") && !strcmp(p[0].value, "hello world"));
    CHECK(!strcmp(p[1].value, "c++") && !strcmp(p[2].key, "empty") && !p[2].value[0] && p[3].enabled);
    CHECK(cui_url_params("https://x.dev/a", p, 8) == 0);
    CHECK(cui_url_params("https://x.dev/?a=1&b=2&c=3", p, 2) == 2);
    char url[96] = "https://x.dev/search?old=1#frag";
    cui_kv_pair q[3];
    memset(q, 0, sizeof q);
    snprintf(q[0].key, 64, "q"); snprintf(q[0].value, 256, "a b&c"); q[0].enabled = true;
    snprintf(q[1].key, 64, "off"); snprintf(q[1].value, 256, "x"); q[1].enabled = false;
    snprintf(q[2].key, 64, "lang"); snprintf(q[2].value, 256, "fa"); q[2].enabled = true;
    CHECK(cui_url_set_params(url, sizeof url, q, 3));
    CHECK(!strcmp(url, "https://x.dev/search?q=a%20b%26c&lang=fa#frag"));
    n = cui_url_params(url, p, 8);   /* round trip */
    CHECK(n == 2 && !strcmp(p[0].value, "a b&c") && !strcmp(p[1].key, "lang"));
    char tiny[24] = "https://x.dev/";
    CHECK(!cui_url_set_params(tiny, sizeof tiny, q, 3));
    CHECK(!strcmp(tiny, "https://x.dev/"));   /* unchanged when it does not fit */
    CHECK(!strcmp(cui_http_reason(404), "Not Found") && !strcmp(cui_http_reason(299), ""));
    CHECK(!strcmp(cui_http_method_name(CUI_HTTP_DELETE), "DELETE") && !strcmp(cui_http_method_name(9), ""));
}

static const cui_test_item tx2_items[] = {
    { "layout", 0 }, { "rows", 1, CUI_JOB_PASSED, 0.004f }, { "wraps", 1, CUI_JOB_PASSED, 0.006f }, { "mirrors", 1, CUI_JOB_FAILED, 0.01f, "expected 412, got 388" },
    { "text", 0 }, { "shapes", 1, CUI_JOB_PASSED, 0.2f }, { "bidi", 1, CUI_JOB_SKIPPED, -1 },
    { "render", 0 }, { "gl", 1 }, { "atlas", 2, CUI_JOB_RUNNING, -1 },
};
typedef struct { cui_test_explorer_result r; int run, activated; bool run_all; } tx_ctx;
static void build_tx(cui_ctx *ui, void *u) {
    tx_ctx *c = u;
    c->r = cui_test_explorer(ui, "tests", tx2_items, 10, .height = 400);
    if (c->r.run >= 0) c->run = c->r.run;
    if (c->r.activated >= 0) c->activated = c->r.activated;
    if (c->r.run_all) c->run_all = true;
}
static void test_test_explorer(void) {
    printf("test explorer: counts, select, keys, fold, run one, run all, activate\n");
    tx_ctx c;
    memset(&c, 0, sizeof c);
    c.run = c.activated = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_tx, &c, 3);
    CHECK(c.r.passed == 3 && c.r.failed == 1 && c.r.skipped == 1 && c.r.total == 6);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "tests"));
    /* the toolbar's first button runs everything */
    click(ui, build_tx, &c, r.x + 30, r.y + 16);
    CHECK(c.run_all);
    /* click down the list until a test is selected */
    for (float y = r.y + 50; y < r.y + 200 && c.r.selected < 0; y += 7) click(ui, build_tx, &c, r.x + r.w * 0.4f, y);
    CHECK(c.r.selected == 0);   /* the first row: the layout suite */
    key(ui, build_tx, &c, CUI_KEY_DOWN, 0); CHECK(c.r.selected == 1);
    key(ui, build_tx, &c, CUI_KEY_DOWN, 0); key(ui, build_tx, &c, CUI_KEY_DOWN, 0); CHECK(c.r.selected == 3);
    key(ui, build_tx, &c, CUI_KEY_DOWN, 0); CHECK(c.r.selected == 4);   /* past the failure's message */
    key(ui, build_tx, &c, CUI_KEY_ENTER, 0); CHECK(c.activated == 4);
    key(ui, build_tx, &c, CUI_KEY_LEFT, 0);                             /* fold "text" */
    key(ui, build_tx, &c, CUI_KEY_DOWN, 0); CHECK(c.r.selected == 7);   /* straight to "render" */
    key(ui, build_tx, &c, CUI_KEY_UP, 0); key(ui, build_tx, &c, CUI_KEY_RIGHT, 0); key(ui, build_tx, &c, CUI_KEY_DOWN, 0);
    CHECK(c.r.selected == 5);
    /* the selected row shows its run button at the end */
    cui_rect r2 = cui_get_rect(ui, cui_make_id(ui, "tests"));
    for (float y = r2.y + 50; y < r2.y + r2.h && c.run < 0; y += 7) click(ui, build_tx, &c, r2.x + r2.w - 20, y);
    CHECK(c.run >= 0);
    cui_destroy(ui);
}

typedef struct { cui_stack_trace_result r; int clicked; } st_ctx;
static const char *const st2_src[] = { "function total(cart) {", "  return cart.items.length;", "}" };
static const cui_stack_frame st2_frames[] = {
    { "total", "src/cart.js", 2, 15, false, st2_src, 3, 1 }, { "handle", "node_modules/x/a.js", 9, 1, true }, { "next", "node_modules/x/b.js", 3, 1, true },
    { "main", "src/main.js", 4, 1, false },
};
static void build_st(cui_ctx *ui, void *u) {
    st_ctx *c = u;
    c->r = cui_stack_trace(ui, "trace", st2_frames, 4, .title = "TypeError", .message = "x is undefined", .lang = "js");
    if (c->r.clicked >= 0) c->clicked = c->r.clicked;
}
static void test_stack(void) {
    printf("stack trace: first own frame open, fold and unfold, library frames, copy\n");
    st_ctx c;
    memset(&c, 0, sizeof c);
    c.clicked = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_st, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "trace"));
    float h_open = r.h;
    CHECK(h_open > 150);   /* the source of the first frame shows */
    /* click the rows from the top until frame 0 reports: it folds */
    for (float y = r.y + 50; y < r.y + r.h && c.clicked < 0; y += 6) click(ui, build_st, &c, r.x + 60, y);
    CHECK(c.clicked == 0);
    run(ui, build_st, &c, 1);
    float h_closed = cui_get_rect(ui, cui_make_id(ui, "trace")).h;
    CHECK(h_closed < h_open - 40);
    /* the library frames unfold */
    r = cui_get_rect(ui, cui_make_id(ui, "trace"));
    for (float y = r.y + r.h - 8; y > r.y + 40; y -= 6) {
        click(ui, build_st, &c, r.x + 60, y);
        run(ui, build_st, &c, 1);
        if (cui_get_rect(ui, cui_make_id(ui, "trace")).h > h_closed + 30) break;
    }
    CHECK(cui_get_rect(ui, cui_make_id(ui, "trace")).h > h_closed + 30);
    cui_destroy(ui);
}

typedef struct { int method; char url[128]; bool loading; int sends, cancels; } rq_ctx;
static void build_rq(cui_ctx *ui, void *u) {
    rq_ctx *c = u;
    cui_request_bar_result r = cui_request_bar(ui, "req", &c->method, c->url, sizeof c->url, .loading = c->loading);
    if (r.send) c->sends++;
    if (r.cancel) c->cancels++;
    cui_response_status(ui, 503, 1.25, 512);
}
static void test_request(void) {
    printf("request bar: Enter sends, the button sends or cancels, empty URL\n");
    rq_ctx c;
    memset(&c, 0, sizeof c);
    cui_ctx *ui = new_ctx(800, 400);
    run(ui, build_rq, &c, 3);
    cui_push_id(ui, "req");
    cui_id uid = cui_make_id(ui, "url");
    cui_pop_id(ui);
    cui_rect ur = cui_get_rect(ui, uid), r = cui_get_rect(ui, cui_make_id(ui, "req"));
    click(ui, build_rq, &c, ur.x + 20, ur.y + ur.h / 2);
    key(ui, build_rq, &c, CUI_KEY_ENTER, 0);
    CHECK(c.sends == 0);   /* nothing to send */
    type(ui, build_rq, &c, "https://api.dev/x");
    key(ui, build_rq, &c, CUI_KEY_ENTER, 0);
    CHECK(c.sends == 1);
    click(ui, build_rq, &c, r.x + r.w - 30, r.y + r.h / 2);
    CHECK(c.sends == 2);
    c.loading = true;
    run(ui, build_rq, &c, 2);
    click(ui, build_rq, &c, r.x + r.w - 30, r.y + r.h / 2);
    CHECK(c.cancels == 1 && c.sends == 2);
    cui_destroy(ui);
}

static float up_days[90];
typedef struct { cui_uptime_result r; } up_ctx;
static void build_up(cui_ctx *ui, void *u) {
    up_ctx *c = u;
    const cui_uptime_service sv[] = { { "API", up_days, 90 }, { "Empty", NULL, 0 } };
    c->r = cui_uptime(ui, "status", sv, 2, .last_day = (cui_date){ 2026, 11, 9 });
}
static void test_uptime(void) {
    printf("uptime bars: hover reports the day, narrow shows the latest days\n");
    for (int i = 0; i < 90; i++) up_days[i] = i == 80 ? 0.9f : i < 5 ? -1 : 1;
    up_ctx c;
    memset(&c, 0, sizeof c);
    cui_ctx *ui = new_ctx(900, 400);
    run(ui, build_up, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "status"));
    int want = 89;
    float x, y;
    CHECK(find(ui, build_up, &c, r, &c.r.day, want, &x, &y) && c.r.service == 0);   /* the latest day is reachable */
    cui_set_viewport(ui, 300, 400, 1);
    run(ui, build_up, &c, 3);
    r = cui_get_rect(ui, cui_make_id(ui, "status"));
    bool seen_last = false, seen_first = false;   /* bars are thin here: sweep a point at a time */
    for (float yy = r.y + 20; yy < r.y + 70; yy += 6)
        for (float xx = r.x; xx < r.x + r.w; xx += 1) { move(ui, build_up, &c, xx, yy); if (c.r.day == 89) seen_last = true; if (c.r.day == 0) seen_first = true; }
    CHECK(seen_last);
    CHECK(!seen_first);   /* the oldest days do not fit */
    cui_destroy(ui);
}

static float th_a[48], th_b[48];
typedef struct { double s0, s1; cui_time_histogram_result r; bool disabled; } th_ctx;
/** Build the histogram with the current enabled state. */
static void build_th(cui_ctx *ui, void *u) {
    th_ctx *c = u;
    const cui_histogram_series se[] = { { "info", th_a, CUI_TONE_INFO }, { "error", th_b, CUI_TONE_DANGER } };
    cui_push_disabled(ui, c->disabled);
    c->r = cui_time_histogram(ui, "hist", 1794240000, 600, 48, se, 2, &c->s0, &c->s1, .utc = true);
    cui_pop_disabled(ui);
}

/** Disabling during a drag cancels it even if the widget becomes enabled again. */
static void test_histogram_disable(void) {
    printf("time histogram: disabling cancels a drag\n");
    cui_ctx *ui = new_ctx(800, 400);
    th_ctx c = {0};
    run(ui, build_th, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "hist"));
    move(ui, build_th, &c, r.x + r.w * 0.3f, r.y + 40);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, build_th, &c, 1);
    move(ui, build_th, &c, r.x + r.w * 0.5f, r.y + 40);
    CHECK(c.r.selection_changed && c.s1 > c.s0);
    double lo = c.s0, hi = c.s1;
    c.disabled = true;
    move(ui, build_th, &c, r.x + r.w * 0.7f, r.y + 40);
    CHECK(!c.r.selection_changed && c.s0 == lo && c.s1 == hi);
    c.disabled = false;
    move(ui, build_th, &c, r.x + r.w * 0.8f, r.y + 40);
    CHECK(!c.r.selection_changed && c.s0 == lo && c.s1 == hi);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, build_th, &c, 1);
    cui_destroy(ui);
}
static void test_histogram(void) {
    printf("time histogram: hover, drag selects whole buckets, click clears\n");
    for (int i = 0; i < 48; i++) { th_a[i] = (float)(10 + i); th_b[i] = (float)(i % 5); }
    th_ctx c;
    memset(&c, 0, sizeof c);
    cui_ctx *ui = new_ctx(800, 400);
    run(ui, build_th, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "hist"));
    float x, y;
    CHECK(find(ui, build_th, &c, r, &c.r.hovered, 47, &x, &y));
    CHECK(find(ui, build_th, &c, r, &c.r.hovered, 0, &x, &y));
    float px0 = r.x + 40, pw = r.w - 44;
    cui_input_mouse_move(ui, px0 + pw * 0.25f, r.y + 40); run(ui, build_th, &c, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, build_th, &c, 1);
    for (int k = 1; k <= 5; k++) { cui_input_mouse_move(ui, px0 + pw * (0.25f + 0.05f * k), r.y + 40); run(ui, build_th, &c, 1); }
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, build_th, &c, 1);
    CHECK(c.s1 > c.s0);
    CHECK(fmod(c.s0 - 1794240000, 600) == 0 && fmod(c.s1 - 1794240000, 600) == 0);
    CHECK(c.s0 >= 1794240000 + 600 * 11 && c.s1 <= 1794240000 + 600 * 25);
    click(ui, build_th, &c, px0 + pw * 0.8f, r.y + 40);
    CHECK(c.s0 == 0 && c.s1 == 0);
    cui_destroy(ui);
}

/* local time: a start far outside what struct tm holds made the zone offset read unset fields */
typedef struct { double start, s0, s1; } th_far_ctx;
static void build_th_far(cui_ctx *ui, void *u) {
    th_far_ctx *c = u;
    const cui_histogram_series se[] = { { "info", th_a, CUI_TONE_INFO } };
    cui_time_histogram(ui, "far", c->start, 600, 48, se, 1, &c->s0, &c->s1);
}
static void test_histogram_far_time(void) {
    printf("time histogram: local time at a start no calendar can show\n");
    th_far_ctx c = { 1e17, 0, 0 };   /* about three billion years from now */
    cui_ctx *ui = new_ctx(800, 400);
    run(ui, build_th_far, &c, 2);
    c.start = -1e17;
    run(ui, build_th_far, &c, 2);
    CHECK(cui_get_rect(ui, cui_make_id(ui, "far")).w > 300);
    CHECK(c.s0 == 0 && c.s1 == 0);
    cui_destroy(ui);
}

static const cui_coverage_file cv2_files[] = {
    { "src/core/a.c", 100, 90, 10, 9, 5, 5 }, { "src/core/b.c", 100, 50, 10, 2, 5, 1 }, { "src/text/c.c", 200, 20, 0, 0, 10, 1 }, { "main.c", 100, 100, 0, 0, 1, 1 },
};
typedef struct { cui_coverage_result r; int activated; } cv_ctx;
static void build_cv(cui_ctx *ui, void *u) {
    cv_ctx *c = u;
    c->r = cui_coverage(ui, "cov", cv2_files, 4);
    if (c->r.activated >= 0) c->activated = c->r.activated;
}
static void test_coverage(void) {
    printf("coverage: totals, folders, sort, fold, activate a file\n");
    cv_ctx c;
    memset(&c, 0, sizeof c);
    c.activated = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_cv, &c, 3);
    CHECK(fabs(c.r.lines - 52.0f) < 0.01f);   /* 260 of 500 lines */
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "cov"));
    float h_all = r.h;
    CHECK(h_all > 30 * 8);   /* all files, src, core, a, b, text, c, main */
    /* double-click each row until a file reports */
    for (float y = r.y + 40; y < r.y + r.h && c.activated < 0; y += 30) double_click(ui, build_cv, &c, r.x + r.w * 0.3f, y);
    CHECK(c.activated >= 0);
    /* a click on a folder folds it */
    click(ui, build_cv, &c, r.x + 80, r.y + 30 + 30 + 15);   /* "src" */
    run(ui, build_cv, &c, 1);
    CHECK(cui_get_rect(ui, cui_make_id(ui, "cov")).h < h_all - 60);
    cui_destroy(ui);
}

static const char *const bl2_lines[] = { "int a;", "/* note", "   more */", "int b;", "int c;" };
static const int bl2_commit[] = { 0, 1, 1, 0, 9 };   /* 9 is out of range */
static const cui_blame_commit bl2_commits[] = { { "aaaa1111", "Maya Okafor", "First", 1700000000 }, { "bbbb2222", "Jonas Berg", "Note", 1790000000 } };
typedef struct { cui_blame_result r; int clicked; const char *const *lines; const int *lc; int n; } bl_ctx;
static void build_bl(cui_ctx *ui, void *u) {
    bl_ctx *c = u;
    c->r = cui_blame(ui, "blame", c->lines, c->lc, c->n, bl2_commits, 2, .lang = "c", .now = 1794211200, .height = 300);
    if (c->r.clicked >= 0) c->clicked = c->r.clicked;
}
static void test_blame(void) {
    printf("blame: hover a commit, click it, bad commit index, 100k lines\n");
    bl_ctx c;
    memset(&c, 0, sizeof c);
    c.clicked = -1; c.lines = bl2_lines; c.lc = bl2_commit; c.n = 5;
    cui_ctx *ui = new_ctx(900, 500);
    run(ui, build_bl, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "blame"));
    float x, y;
    CHECK(find(ui, build_bl, &c, (cui_rect){ r.x, r.y, 120, r.h }, &c.r.hovered, 1, &x, &y));
    click(ui, build_bl, &c, x, y);
    CHECK(c.clicked == 1);
    static const char *many[100000];
    static int many_c[100000];
    for (int i = 0; i < 100000; i++) { many[i] = i % 7 == 0 ? "/* comment" : i % 7 == 1 ? "   ends */ int x = 1;" : "    return value + other;"; many_c[i] = (i / 40) % 2; }
    c.lines = many; c.lc = many_c; c.n = 100000;
    clock_t c0 = clock();
    run(ui, build_bl, &c, 1);
    double first = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000;
    c0 = clock();
    run(ui, build_bl, &c, 30);
    double ms = (double)(clock() - c0) / CLOCKS_PER_SEC * 1000 / 30;
    printf("  100k lines: first frame %.1f ms, then %.2f ms a frame\n", first, ms);
    CHECK(ms < 20);
    cui_destroy(ui);
}

/* line_commit may be NULL (a file with no blame data); every row after the first used to read it */
static void test_blame_no_commits(void) {
    printf("blame: no commit data for the lines\n");
    bl_ctx c;
    memset(&c, 0, sizeof c);
    c.clicked = -1; c.lines = bl2_lines; c.lc = NULL; c.n = 5;
    cui_ctx *ui = new_ctx(900, 500);
    run(ui, build_bl, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "blame"));
    CHECK(r.h > 100);
    click(ui, build_bl, &c, r.x + 40, r.y + 40);
    CHECK(c.r.hovered == -1 && c.clicked == -1);   /* no commit to report */
    cui_destroy(ui);
}

typedef struct { cui_feature_flag f[3]; cui_feature_flags_result r; int changes; } ff_ctx;
static void build_ff(cui_ctx *ui, void *u) {
    ff_ctx *c = u;
    static const char *envs[] = { "Dev", "Staging", "Prod" };
    c->r = cui_feature_flags(ui, "flags", c->f, 3, envs, 3);
    if (c->r.flag >= 0) c->changes++;
}
static void test_flags(void) {
    printf("feature flags: switches change flags, narrow shows one environment\n");
    ff_ctx c;
    memset(&c, 0, sizeof c);
    c.f[0].key = "new-checkout"; c.f[0].on[0] = true;
    c.f[1].key = "search-v2"; c.f[2].key = "export";
    cui_ctx *ui = new_ctx(900, 500);
    run(ui, build_ff, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "flags"));
    /* click across the first row until a switch flips */
    bool before[3] = { c.f[0].on[0], c.f[0].on[1], c.f[0].on[2] };
    for (float x = r.x + r.w * 0.45f; x < r.x + r.w && !c.changes; x += 8)
        for (float y = r.y + 60; y < r.y + 110 && !c.changes; y += 8) click(ui, build_ff, &c, x, y);
    CHECK(c.changes == 1);
    CHECK(c.f[0].on[0] != before[0] || c.f[0].on[1] != before[1] || c.f[0].on[2] != before[2]);
    cui_set_viewport(ui, 360, 600, 1);
    run(ui, build_ff, &c, 3);
    CHECK(c.changes == 1);
    cui_destroy(ui);
}

static const cui_api_param ax2_par[] = { { "id", "path", "string", true, "The id" } };
static const cui_api_endpoint ax2_eps[] = {
    { "GET", "/users", "List users", "Users" }, { "GET", "/users/{id}", "Get a user", "Users", false, ax2_par, 1 },
    { "POST", "/orders", "Create an order", "Orders" }, { "DELETE", "/v1/x", "Old", NULL, true },
};
typedef struct { cui_api_explorer_result r; int sel, try_it; } ax_ctx;
static void build_ax(cui_ctx *ui, void *u) {
    ax_ctx *c = u;
    c->r = cui_api_explorer(ui, "api", ax2_eps, 4, .selected = &c->sel);
    if (c->r.try_it >= 0) c->try_it = c->r.try_it;
}
static void test_api(void) {
    printf("API explorer: select, open, try it, keys, fold a tag\n");
    ax_ctx c;
    memset(&c, 0, sizeof c);
    c.sel = -1; c.try_it = -1;
    cui_ctx *ui = new_ctx(800, 600);
    run(ui, build_ax, &c, 3);
    cui_rect r = cui_get_rect(ui, cui_make_id(ui, "api"));
    float h0 = r.h;
    /* below the filter and the first tag's header: the rows; the second is /users/{id} */
    for (float y = r.y + 70; y < r.y + r.h && c.sel != 1; y += 6) click(ui, build_ax, &c, r.x + r.w * 0.6f, y);
    CHECK(c.sel == 1);
    run(ui, build_ax, &c, 1);
    r = cui_get_rect(ui, cui_make_id(ui, "api"));
    CHECK(r.h > h0 + 30);   /* its parameters opened */
    key(ui, build_ax, &c, CUI_KEY_DOWN, 0); CHECK(c.sel == 2);
    key(ui, build_ax, &c, CUI_KEY_UP, 0); key(ui, build_ax, &c, CUI_KEY_UP, 0); CHECK(c.sel == 0);
    for (float y = r.y + 110; y < r.y + r.h && c.try_it < 0; y += 6) click(ui, build_ax, &c, r.x + 115, y);
    CHECK(c.try_it == 1);   /* the open endpoint's button */
    cui_destroy(ui);
}

/* the second ten, at three widths, both directions and themes, under random input */
typedef struct { rt_ctx rt; tx_ctx tx; st_ctx st; rq_ctx rq; up_ctx up; th_ctx th; cv_ctx cv; bl_ctx bl; ff_ctx ff; ax_ctx ax; } all2_ctx;
static void build_all2(cui_ctx *ui, void *u) {
    all2_ctx *a = u;
    cui_scroll(ui, "page", .width = CUI_FILL, .height = CUI_FILL) {
        cui_column(ui, .gap = 16, .width = CUI_FILL) {
            build_rt(ui, &a->rt); build_tx(ui, &a->tx); build_st(ui, &a->st); build_rq(ui, &a->rq); build_up(ui, &a->up);
            build_th(ui, &a->th); build_cv(ui, &a->cv); build_bl(ui, &a->bl); build_ff(ui, &a->ff); build_ax(ui, &a->ax);
        }
    }
}
static void test_random2(void) {
    printf("the second ten under random input: 3 widths x 2 directions x 2 themes\n");
    static const cui_key keys[] = { CUI_KEY_UP, CUI_KEY_DOWN, CUI_KEY_LEFT, CUI_KEY_RIGHT, CUI_KEY_ENTER, CUI_KEY_ESCAPE, CUI_KEY_TAB,
                                    CUI_KEY_BACKSPACE, CUI_KEY_SPACE, CUI_KEY_HOME, CUI_KEY_END, CUI_KEY_DELETE };
    static const char *texts[] = { "a", "(", "[a-", "\\d+", "*", "?", "\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85", "{2}", "|", ")" };
    long frames = 0;
    for (int cfg = 0; cfg < 12; cfg++) {
        static const float widths[3] = { 340, 760, 1280 };
        bool rtl = cfg & 1, dark = (cfg >> 1) & 1;
        float w = widths[cfg / 4];
        all2_ctx *a = calloc(1, sizeof *a);
        snprintf(a->rt.pat, sizeof a->rt.pat, "(\\w+)@");
        snprintf(a->rt.text, sizeof a->rt.text, "a@b c@d");
        a->tx.run = a->tx.activated = -1; a->st.clicked = -1; a->cv.activated = -1;
        a->bl.lines = bl2_lines; a->bl.lc = bl2_commit; a->bl.n = 5; a->bl.clicked = -1;
        a->ff.f[0].key = "x"; a->ff.f[1].key = "y"; a->ff.f[2].key = "z";
        a->ax.sel = -1; a->ax.try_it = -1;
        cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                                 .direction = rtl ? CUI_DIRECTION_RTL : CUI_DIRECTION_INHERIT });
        cui_set_viewport(ui, w, 900, 1);
        g_seed = (unsigned)cfg * 104729u + 11;
        for (int f = 0; f < 700; f++) {
            unsigned r = rnd();
            switch (r % 10) {
            case 0: case 1: case 2: cui_input_mouse_move(ui, (float)(rnd() % (unsigned)w), (float)(rnd() % 900)); break;
            case 3: cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); break;
            case 4: cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); break;
            case 5: cui_input_scroll(ui, 0, (float)((int)(rnd() % 7) - 3)); break;
            case 6: { cui_key k = keys[rnd() % (sizeof keys / sizeof keys[0])]; cui_input_key(ui, k, true, 0); cui_input_key(ui, k, false, 0); break; }
            case 7: cui_input_text(ui, texts[rnd() % (sizeof texts / sizeof texts[0])]); break;
            default: break;
            }
            run(ui, build_all2, a, 1);
            frames++;
            if (strlen(a->rt.pat) >= sizeof a->rt.pat || strlen(a->rt.text) >= sizeof a->rt.text || strlen(a->rq.url) >= sizeof a->rq.url ||
                a->ax.sel >= 4 || a->tx.r.selected >= 10) { printf("  out of range at config %d frame %d\n", cfg, f); CHECK(false); break; }
            for (int k = 0; k < 3; k++) for (int e = 0; e < 3; e++) if (a->ff.f[k].rollout[e] < 0 || a->ff.f[k].rollout[e] > 100) { CHECK(false); f = 9999; }
        }
        cui_destroy(ui);
        free(a);
    }
    printf("  %ld frames\n", frames);
    CHECK(frames == 12 * 700);
}

int main(void) {
    g_sw = cui_sw_create();
    test_cron();
    test_schedule();
    test_query();
    test_query_read_only();
    test_kv();
    test_problems();
    test_pipeline();
    test_keymap();
    test_commits();
    test_graph();
    test_waterfall();
    test_trace();
    test_random();
    test_regex();
    test_regex_noncapture();
    test_regex_tester();
    test_url();
    test_test_explorer();
    test_stack();
    test_request();
    test_uptime();
    test_histogram();
    test_histogram_far_time();
    test_histogram_disable();
    test_coverage();
    test_blame();
    test_blame_no_commits();
    test_flags();
    test_api();
    test_random2();
    cui_sw_destroy(g_sw);
    printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
