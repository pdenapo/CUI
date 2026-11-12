/* Opens a real window with the native backend, runs the gallery for a few
 * frames, injects input, saves a screenshot and exits.
 *   cui_app_smoke [out.png] */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include "../examples/gallery_ui.h"

#include <stdio.h>
#include <stdlib.h>

#define SMOKE_APP_ID "io.github.yappologistic.CUI.SmokeTest"

#ifdef CUI_SMOKE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#if defined(CUI_SMOKE_SDL2)
#define SDL_MAIN_HANDLED
#include <SDL.h>
#elif defined(CUI_SMOKE_GLFW)
#include <GLFW/glfw3.h>
#endif

static int smoke_ignore_x_error(Display *dpy, XErrorEvent *e) { (void)dpy; (void)e; return 0; }

/* Walks the window tree as a window manager sees it (a manager may have
 * reparented the window into a frame) for a window whose WM_CLASS class is
 * the app ID. The X11 and GLFW backends set the instance part to the ID as
 * well, so it is compared too; with SDL2 it is not, because SDL3 under
 * sdl2-compat puts the program's name there. */
#if defined(CUI_SMOKE_SDL2)
#define SMOKE_INSTANCE_IS_ID 0
#else
#define SMOKE_INSTANCE_IS_ID 1
#endif
static bool smoke_find_wm_class(Display *dpy, Window w, int depth) {
    XClassHint ch = { 0 };
    bool found = false;
    if (XGetClassHint(dpy, w, &ch)) {
        found = ch.res_name && ch.res_class && strcmp(ch.res_class, SMOKE_APP_ID) == 0;
        if (found) printf("WM_CLASS=\"%s\", \"%s\"\n", ch.res_name, ch.res_class);
        if (found && SMOKE_INSTANCE_IS_ID && strcmp(ch.res_name, SMOKE_APP_ID) != 0) {
            printf("WM_CLASS: the instance part is not %s\n", SMOKE_APP_ID);
            found = false;
        }
        if (ch.res_name) XFree(ch.res_name);
        if (ch.res_class) XFree(ch.res_class);
    }
    Window root, parent, *kids = NULL;
    unsigned n = 0;
    if (!found && depth > 0 && XQueryTree(dpy, w, &root, &parent, &kids, &n)) {
        for (unsigned i = 0; i < n && !found; i++) found = smoke_find_wm_class(dpy, kids[i], depth - 1);
        if (kids) XFree(kids);
    }
    return found;
}

/* The window system the backend picked; only X11 has a WM_CLASS to read. */
static const char *smoke_window_system(void) {
#if defined(CUI_SMOKE_SDL2)
    const char *driver = SDL_GetCurrentVideoDriver();
    return driver ? driver : "no video driver";
#elif defined(CUI_SMOKE_GLFW) && defined(GLFW_PLATFORM_X11)   /* GLFW 3.4 */
    return glfwGetPlatform() == GLFW_PLATFORM_X11 ? "x11" : "a platform other than X11";
#elif defined(CUI_SMOKE_GLFW)   /* GLFW 3.3 cannot say which one it picked */
    return getenv("WAYLAND_DISPLAY") ? "X11 or Wayland (GLFW 3.3 does not say)" : "x11";
#else
    return "x11";
#endif
}

/* Desktops match a window to its .desktop file by WM_CLASS. */
static bool smoke_app_id_ok(cui_app *app) {
#if defined(CUI_SMOKE_SDL2)
    /* SDL2 read the ID from the environment in SDL_Init; it must not stay
     * there for child processes or a later app to inherit */
    const char *left = getenv("SDL_VIDEO_X11_WMCLASS");
    if (left && strcmp(left, SMOKE_APP_ID) == 0) {
        printf("SDL_VIDEO_X11_WMCLASS was left set to %s\n", left);
        return false;
    }
#endif
    const char *system = smoke_window_system();
    if (strcmp(system, "x11") != 0) {
        printf("WM_CLASS not checked: %s runs on %s\n", cui_app_backend(app), system);
        return true;
    }
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return false;
    XErrorHandler old = XSetErrorHandler(smoke_ignore_x_error);   /* other clients' windows may close mid-walk */
    bool found = smoke_find_wm_class(dpy, DefaultRootWindow(dpy), 3);
    XSync(dpy, False);
    XSetErrorHandler(old);
    XCloseDisplay(dpy);
    if (!found) printf("WM_CLASS: no window carries %s\n", SMOKE_APP_ID);
    return found;
}
#endif

#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>
#include <signal.h>
#include <unistd.h>
static volatile int smoke_quitting;
static void smoke_exit_guard(void) { if (smoke_quitting) { printf("quit from the menu: the process exited inside the event loop\n"); _exit(3); } }
static void smoke_hang(int sig) { (void)sig; printf("quit from the menu: cui_app_frame did not return\n"); _exit(4); }
/* Quit as the Dock or AppleScript sends it: terminate: arrives while the app waits for
 * events, with no event of its own. cui_app_frame has to return false, and the process
 * has to go on to the code after the loop. */
static bool smoke_quit_menu(void) {
    cui_app *app = cui_app_create(&(cui_app_desc){ .title = "CUI quit test", .width = 400, .height = 300 });
    if (!app) return false;
    id nsapp = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    ((void (*)(id, SEL, SEL, id, double))objc_msgSend)(nsapp, sel_registerName("performSelector:withObject:afterDelay:"),
                                                       sel_registerName("terminate:"), nil, 0.4);
    atexit(smoke_exit_guard);
    signal(SIGALRM, smoke_hang);
    smoke_quitting = 1;
    alarm(10);
    int frames = 0;
    while (cui_app_frame(app) && frames < 100000) { cui_text(cui_app_ui(app), "waiting for Quit"); frames++; }
    alarm(0);
    smoke_quitting = 0;
    cui_app_destroy(app);
    printf("quit from the menu: cui_app_frame returned after %d frames\n", frames);
    return frames < 100000;
}
#endif

/* cui_app_follow_system_theme (issue #34): stopping keeps a cui_set_theme from being
 * undone by the OS setting, starting goes back to the OS setting at once. */
static bool smoke_follow_theme(void) {
    cui_app *app = cui_app_create(&(cui_app_desc){ .title = "CUI theme test", .width = 400, .height = 300,
                                                   .continuous = true, .follow_system_theme = true });
    if (!app) return false;
    cui_ctx *ui = cui_app_ui(app);
    cui_theme_mode os = cui_get_theme_mode(ui);          /* the OS setting, or light when unknown */
    cui_theme_mode other = os == CUI_THEME_DARK ? CUI_THEME_LIGHT : CUI_THEME_DARK;
    int frame = 0;
    cui_theme_mode kept = os, back = os;
    while (cui_app_frame(app) && frame < 40) {
        if (frame == 2) { cui_app_follow_system_theme(NULL, false); cui_set_theme(ui, other); }   /* NULL: the running app */
        if (frame == 20) kept = cui_get_theme_mode(ui);
        if (frame == 21) cui_app_follow_system_theme(app, true);
        if (frame == 22) back = cui_get_theme_mode(ui);
        cui_text(ui, "theme");
        frame++;
    }
    cui_app_destroy(app);
    bool stop_ok = kept == other;
#if defined(_WIN32) || defined(__APPLE__)
    bool start_ok = back == os;                            /* these platforms always report a setting */
#else
    bool start_ok = back == os || back == other;           /* X11 may not know the setting (Xvfb) */
#endif
    printf("follow system theme: stop keeps the app's theme %s, start returns to the OS setting %s\n",
           stop_ok ? "ok" : "FAILED", start_ok ? (back == os ? "ok" : "(OS setting unknown here)") : "FAILED");
    return stop_ok && start_ok;
}

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "app_smoke.png";
    setvbuf(stdout, NULL, _IONBF, 0);
    cui_app *app = cui_app_create(&(cui_app_desc){ .title = "CUI smoke test", .width = 1000, .height = 700, .continuous = true,
                                                   .app_id = SMOKE_APP_ID });
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
    bool app_id_ok = true;
#ifdef CUI_SMOKE_X11
    app_id_ok = smoke_app_id_ok(app);
#endif
#ifdef CUI_SMOKE_X11
    /* An X server resets when its last client disconnects, and a window opened during the reset
     * fails. Xvfb does this by default, so hold a connection while the next test's app starts. */
    Display *keep = XOpenDisplay(NULL);
#endif
    cui_app_destroy(app);
    /* a continuous app runs at the display's rate, not as fast as the machine can draw */
    double fps = t1 > t0 ? 39.0 / (t1 - t0) : 0;
    bool paced = fps > 0 && fps < 250;
    printf("animation: %.0f frames per second%s\n", fps, paced ? "" : " (not paced)");
    bool theme_ok = smoke_follow_theme();
#ifdef CUI_SMOKE_X11
    if (keep) XCloseDisplay(keep);
#endif
    bool quit_ok = true;
#ifdef __APPLE__
    quit_ok = smoke_quit_menu();
#endif
    return (ok && switched && paced && null_ok && app_id_ok && theme_ok && quit_ok) ? 0 : 1;
}
