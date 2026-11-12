/* Smoke test for cui_app_maximize: open a window, maximize, run a few
 * frames, check the client size did not shrink, then exit.
 * Needs a real display (skip in headless CI if create fails). */
#include <cui/cui_app.h>

#include <stdio.h>
#include <stdlib.h>
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

    /* Maximize is honored by the common window managers and by the native
     * backends, so on those a maximized window must be larger than before.
     * On an unknown/ignoring WM we can only require that it did not shrink. */
    const char *backend = cui_app_backend(app);
    const char *desktop = getenv("XDG_CURRENT_DESKTOP");
    int supported = (backend && (strcmp(backend, "win32") == 0 ||
                                 strcmp(backend, "cocoa") == 0)) ||
                    (desktop && (strstr(desktop, "MATE") || strstr(desktop, "GNOME") ||
                                 strstr(desktop, "KDE") || strstr(desktop, "XFCE") ||
                                 strstr(desktop, "CINNAMON") || strstr(desktop, "UNITY") ||
                                 strstr(desktop, "LXQT") || strstr(desktop, "LXDE")));
    printf("maximize_supported=%d\n", supported);

    cui_app_destroy(app);

    if (after.x + 1.0f < before.x || after.y + 1.0f < before.y) {
        fprintf(stderr, "fail: window shrank after maximize\n");
        return 1;
    }
    if (supported && !(after.x > before.x + 1.0f || after.y > before.y + 1.0f)) {
        fprintf(stderr, "fail: window did not grow after maximize\n");
        return 1;
    }
    return 0;
}
