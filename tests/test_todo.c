/* The tutorial's app (examples/todo.c) does what docs/tutorial.md says: typing and Enter
 * adds a task, the checkbox finishes it, the filter hides it, the dialog clears it, and
 * the list survives a save and a load. The example is included with its main renamed. */
#define main example_main
#include "../examples/todo.c"
#undef main

#include <cui/cui_render.h>
#include <stdlib.h>

static int failed, checks;
#define CHECK(c) do { checks++; if (!(c)) { failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void run(cui_ctx *ui, app_state *a, int n) { for (int i = 0; i < n; i++) { cui_begin_frame(ui, 1.0f / 60.0f); frame(ui, a); cui_end_frame(ui); } }
static void click(cui_ctx *ui, app_state *a, float x, float y) {
    cui_input_mouse_move(ui, x, y); run(ui, a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, a, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, a, 2);
}
static void key(cui_ctx *ui, app_state *a, cui_key k) { cui_input_key(ui, k, true, 0); cui_input_key(ui, k, false, 0); run(ui, a, 2); }

/* cui_test_todo <tmp-dir> <image-dir>: also writes the tutorial's pictures of the app */
static void shot(const char *dir, bool dark) {
    static const char *titles[] = { "Write the release notes", "Record the demo video", "Reply to the Hacker News thread", "Book the offsite venue" };
    static app_state s;
    memset(&s, 0, sizeof s);
    for (int i = 0; i < 4; i++) { snprintf(s.tasks[i].title, sizeof s.tasks[i].title, "%s", titles[i]); s.tasks[i].done = i == 0; }
    s.count = 4;
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT });
    cui_set_viewport(ui, 520, 360, 2);
    cui_input_mouse_leave(ui);
    run(ui, &s, 30);
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    char path[600];
    snprintf(path, sizeof path, "%s/tutorial-todo%s.png", dir, dark ? "-dark" : "");
    cui_write_png(path, px, dd->width_px, dd->height_px, dd->width_px * 4);
    free(px);
    cui_destroy(ui);
    cui_sw_destroy(sw);
}

int main(int argc, char **argv) {
    static app_state a, b;
    if (argc > 2) { shot(argv[2], false); shot(argv[2], true); }
    snprintf(a.path, sizeof a.path, "%s/todo_test.txt", argc > 1 ? argv[1] : ".");
    remove(a.path);
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw) });
    cui_set_viewport(ui, 520, 640, 1);
    cui_set_reduced_motion(ui, true);
    run(ui, &a, 3);

    /* type into the field and press Enter, twice; an empty field adds nothing */
    cui_rect field = cui_get_rect(ui, cui_make_id(ui, "new-task"));
    CHECK(field.w > 100);
    click(ui, &a, field.x + 20, field.y + field.h / 2);
    key(ui, &a, CUI_KEY_ENTER);
    CHECK(a.count == 0);
    cui_input_text(ui, "Buy milk"); run(ui, &a, 1);
    key(ui, &a, CUI_KEY_ENTER);
    CHECK(a.count == 1 && !strcmp(a.tasks[0].title, "Buy milk") && !a.draft[0]);
    cui_input_text(ui, "Buy milk"); run(ui, &a, 1);   /* the same title again: the rows still have their own ids */
    key(ui, &a, CUI_KEY_ENTER);
    CHECK(a.count == 2);

    /* the first row's checkbox finishes the first task only */
    float row_y = field.y + field.h + 16 + 36 + 16 + 14;
    for (float y = row_y; y < row_y + 30 && !a.tasks[0].done; y += 6) click(ui, &a, field.x + 10, y);
    CHECK(a.tasks[0].done && !a.tasks[1].done);

    /* saved as it changed, and it loads back */
    snprintf(b.path, sizeof b.path, "%s", a.path);
    load(&b);
    CHECK(b.count == 2 && b.tasks[0].done && !b.tasks[1].done && !strcmp(b.tasks[1].title, "Buy milk"));

    /* the dialog clears what is done */
    a.confirm_clear = true;
    run(ui, &a, 3);
    key(ui, &a, CUI_KEY_ESCAPE);
    CHECK(!a.confirm_clear && a.count == 2);   /* Esc cancels */
    a.confirm_clear = true;
    run(ui, &a, 3);
    cui_vec2 vp = cui_viewport_size(ui);
    /* Clear is the last button of the footer: sweep the dialog's lower half from its right edge */
    for (float y = vp.y * 0.52f; y < vp.y * 0.68f && a.confirm_clear; y += 6)
        for (float x = vp.x * 0.5f + 180; x > vp.x * 0.5f + 60 && a.confirm_clear; x -= 10) click(ui, &a, x, y);
    CHECK(a.count == 1 && !a.tasks[0].done);
    memset(&b, 0, sizeof b);
    snprintf(b.path, sizeof b.path, "%s", a.path);
    load(&b);
    CHECK(b.count == 1);

    /* the filter hides, it does not delete */
    a.filter = 2;
    run(ui, &a, 2);
    CHECK(a.count == 1);

    remove(a.path);
    cui_destroy(ui);
    cui_sw_destroy(sw);
    printf("%d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}
