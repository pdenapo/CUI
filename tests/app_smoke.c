/* Opens a real window with the native backend, runs the gallery for a few
 * frames, injects input, saves a screenshot and exits.
 *   cui_app_smoke [out.png] */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include "../examples/gallery_ui.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "app_smoke.png";
    setvbuf(stdout, NULL, _IONBF, 0);
    cui_app *app = cui_app_create(&(cui_app_desc){ .title = "CUI smoke test", .width = 1000, .height = 700, .continuous = true });
    if (!app) { fprintf(stderr, "could not create app\n"); return 1; }
    printf("backend=%s renderer=%s\n", cui_app_backend(app), cui_app_is_software(app) ? "software" : "opengl");
    static gallery g;
    gallery_init(&g);
    g.page = PAGE_SELECTION;
    int frame = 0;
    bool switched = false;
    double t0 = 0, t1 = 0;
    while (cui_app_frame(app)) {
        cui_ctx *ui = cui_app_ui(app);
        /* click the Bluetooth switch after the layout settled, wherever the gallery put it.
         * The position goes in again with the press and the release, so a real pointer
         * moving over the window in between cannot pull the click away. */
        float cx = g.bluetooth_rect.x + g.bluetooth_rect.w * 0.5f, cy = g.bluetooth_rect.y + g.bluetooth_rect.h * 0.5f;
        if (frame >= 20 && frame <= 24) cui_input_mouse_move(ui, cx, cy);
        if (frame == 22) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (frame == 24) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        bool before = g.bluetooth;
        gallery_frame(ui, &g);
        if (g.bluetooth != before) switched = true;
        if (frame % 20 == 0) printf("frame %d\n", frame);
        if (frame == 110) t0 = cui_time(ui);
        if (frame == 149) t1 = cui_time(ui);
        /* NULL names the running app, as a cui_app_run callback has to: quit that way */
        if (++frame == 150) { cui_app_set_title(NULL, "CUI smoke test, done"); cui_app_quit(NULL); }
        if (frame > 160) break;
    }
    bool quit_by_null = frame == 150;
    cui_vec2 sz = cui_app_size(app), sz_null = cui_app_size(NULL);
    bool null_ok = quit_by_null && sz.x == sz_null.x && sz.y == sz_null.y && cui_app_ui(NULL) == cui_app_ui(app);
    printf("NULL as the running app: %s\n", null_ok ? "ok" : "FAILED");
    const cui_draw_data *dd = cui_get_draw_data(cui_app_ui(app));
    int w = dd->width_px, h = dd->height_px;
    unsigned char *px = malloc((size_t)w * h * 4);
    bool ok = cui_app_screenshot(app, px, w, h) && cui_write_png(out, px, w, h, w * 4);
    printf("size=%.0fx%.0f screenshot=%s switch_clicked=%s (switch at %.0f,%.0f %.0fx%.0f)\n", sz.x, sz.y, ok ? out : "failed", switched ? "yes" : "no",
           g.bluetooth_rect.x, g.bluetooth_rect.y, g.bluetooth_rect.w, g.bluetooth_rect.h);
    free(px);
    cui_app_destroy(app);
    /* a continuous app runs at the display's rate, not as fast as the machine can draw */
    double fps = t1 > t0 ? 39.0 / (t1 - t0) : 0;
    bool paced = fps > 0 && fps < 250;
    printf("animation: %.0f frames per second%s\n", fps, paced ? "" : " (not paced)");
    return (ok && switched && paced && null_ok) ? 0 : 1;
}
