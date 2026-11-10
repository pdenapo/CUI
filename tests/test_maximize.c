/* Smoke test for cui_app_maximize: open a window, maximize, run a few
 * frames, check the client size did not shrink, then exit.
 * Needs a real display (skip in headless CI if create fails). */
#include <cui/cui_app.h>

#include <stdio.h>
#include <string.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    cui_app *app = cui_app_create(&(cui_app_desc){
        .title = "CUI maximize test",
        .width = 640,
        .height = 480,
        .continuous = true
    });
    if (!app) {
        fprintf(stderr, "skip: could not create app (no display?)\n");
        return 0; /* soft skip: no display is not a failure of maximize */
    }

    printf("backend=%s\n", cui_app_backend(app));

    cui_vec2 before = cui_app_size(app);
    cui_app_maximize(app);

    int frames = 0;
    while (cui_app_frame(app) && frames < 30) {
        cui_ctx *ui = cui_app_ui(app);
        cui_column(ui, .pad = 16) {
            cui_text(ui, "maximize test");
        }
        frames++;
    }

    cui_vec2 after = cui_app_size(app);
    printf("size before=%.0fx%.0f after=%.0fx%.0f frames=%d\n",
           before.x, before.y, after.x, after.y, frames);

    cui_app_destroy(app);

    /* Maximize should not shrink the window. On some WMs the size update
     * is async; allow equality if the WM ignored the request. */
    if (after.x + 1.0f < before.x || after.y + 1.0f < before.y) {
        fprintf(stderr, "fail: window shrank after maximize\n");
        return 1;
    }
    return 0;
}
