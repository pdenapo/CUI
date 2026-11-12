# Building an app

[Getting started](getting-started.md) opens a window and the [tutorial](tutorial.md) builds a small app. This page covers what comes next: the window, when frames happen, what components hand back, work on other threads, and shipping. [Recipes](recipes.md) has complete programs for the common cases.

## Two ways to run

The callback form creates the window, calls your function for every frame and cleans up. Use it unless you need the loop yourself.

```c
static void frame(cui_ctx *ui, void *user) { /* components */ }

int main(void) {
    return cui_app_run(&(cui_app_desc){ .title = "Notes", .width = 900, .height = 600 }, frame, NULL);
}
```

The loop form gives you the loop, for a program that has its own work to do between frames:

```c
cui_app *app = cui_app_create(&(cui_app_desc){ .title = "Notes" });
if (!app) return 1;
while (cui_app_frame(app)) {
    cui_ctx *ui = cui_app_ui(app);
    /* components */
}
cui_app_destroy(app);
```

`cui_app_frame` shows the previous frame, waits until there is something to do, and starts the next one. It returns false once the window is closed, or on macOS when the app is quit from its menu, with Cmd-Q, from the Dock or by AppleScript, so the code after the loop always runs. The callback form also gives smooth live resizing on Windows and macOS: the content keeps redrawing while the user drags the window's edge.

`cui_app_desc` sets up the window. Every field is optional:

| Field | Meaning |
|---|---|
| `title` | The window's title. Default "CUI". |
| `width`, `height` | The size in points, default 1080 by 720. A point is one pixel at 100% scale and two on a 2x display. |
| `min_width`, `min_height` | The smallest size the user can resize to, default 360 by 280. |
| `fixed_size` | The window cannot be resized. |
| `continuous` | Draw every frame even when nothing changes, for games and video. |
| `no_vsync` | With OpenGL, do not wait for the display. Frames are still capped at 120 Hz or the refresh rate. |
| `hidden` | Create the window without showing it, for tests. |
| `renderer` | `CUI_RENDERER_AUTO` (default), `CUI_RENDERER_OPENGL` or `CUI_RENDERER_SOFTWARE`. |
| `theme`, `accent` | The theme and accent to start with. |
| `follow_system_theme` | Use the system's light or dark setting where the platform reports it. `cui_app_follow_system_theme` turns it on or off while the app runs. |
| `app_id` | Linux: the name of the app's `.desktop` file without ".desktop", such as "org.example.Notes", so the desktop shows the app's name and icon for the window. See [Your app's name and icon on Linux](getting-started.md#your-apps-name-and-icon-on-linux). |

## The window

```c
cui_app_set_title(NULL, "Notes: release plan");
cui_app_set_size(NULL, 1200, 800);
cui_vec2 size = cui_app_size(NULL);      /* in points */
if (cui_button(ui, "Quit")) cui_app_quit(NULL);
```

`NULL` means the app that is running, which is how a `cui_app_run` callback reaches its window. With the loop form you can pass your `cui_app *` instead. `cui_app_quit` ends the loop after the current frame.

`cui_app_native_window` returns the system's handle (an `HWND`, an `NSWindow *`, an X11 `Window`, an `SDL_Window *` or a `GLFWwindow *`) for the things CUI does not wrap, such as a window icon or a system tray entry. `cui_app_backend` names the backend in use.

File dialogs, opening links and screenshots are in [Getting started](getting-started.md#7-files-and-links).

## When frames happen

A CUI app is event driven. A frame is drawn when there is input, while something animates, and when a component needs one (a blinking caret, a tooltip's delay, a toast about to leave). Otherwise the app sleeps and uses no CPU.

This means your frame function is not called on a clock. If something of yours changes with time, say so:

```c
cui_request_wakeup(ui, 1.0f);    /* a frame within a second, even with no input */
```

Call it every frame for as long as you need it. Use it for a clock, a countdown, or data you poll. `cui_request_redraw(ui)` asks for one more frame right away, and `.continuous = true` in `cui_app_desc` draws every frame. `cui_time(ui)` is the app's clock in seconds, and `cui_delta_time(ui)` the time since the last frame.

Do not do slow work in the frame function. Reading a large file or waiting on the network there freezes the window, because nothing is drawn until the function returns.

## Other threads

A `cui_ctx` is used from one thread. Run long work on another thread, share plain data with it (an atomic, or a struct behind a mutex), and read that data in the frame function. The worker never calls CUI. While it runs, call `cui_request_wakeup` each frame so the interface keeps up. [Recipes](recipes.md#a-long-task-on-another-thread) has the whole program.

## What components hand back

Every component returns what happened in this frame. The forms repeat across the library:

| Returns | Meaning | Examples |
|---|---|---|
| `bool` | true on the frame it is clicked, or on the frame it changes the value you passed | `cui_button`, `cui_checkbox`, `cui_select`, `cui_slider` |
| `int` index | the item chosen this frame, or -1 | `cui_breadcrumb`, `cui_command_palette`, `cui_reactions` |
| `int` flags | several things at once, tested with `&` | `cui_input`: `CUI_CHANGED`, `CUI_SUBMITTED`, `CUI_FOCUSED`, `CUI_BLURRED` |
| a result struct | the larger components, with one field per event | `cui_markdown`, `cui_code_editor`, `cui_trace_view` |

A value you pass by pointer (`bool *`, `int *`, `float *`, a text buffer) is yours. The component reads it to draw and writes it when the user changes it, so the same variable can be changed by your own code at any time. Each component's page lists what it returns and the fields of its result.

## Turning parts of the screen off

Everything inside a disabled scope is greyed out and takes no input:

```c
cui_disabled_scope(ui, !signed_in) {
    cui_input(ui, "comment", comment, sizeof comment, .placeholder = "Add a comment");
    cui_button(ui, "Post");
}
```

Most components also take `.disabled = true` for one control.

## Your own shortcuts and input

Ask for a shortcut in the frame function, and skip it while the user is typing:

```c
if (!cui_wants_keyboard(ui) && cui_shortcut(ui, CUI_MOD_PRIMARY, CUI_KEY_S)) save();
```

`CUI_MOD_PRIMARY` is Command on macOS and Ctrl elsewhere. `cui_key_pressed`, `cui_key_down`, `cui_mouse_pos`, `cui_mouse_down` and `cui_mouse_clicked` read the raw state, and `cui_wants_mouse` is true while the pointer is over a component or held by one. Files dropped on the window arrive through `cui_dropped_file_count` and `cui_dropped_file`.

## Shipping

Build a release with `cui_app_target` in your CMake project (see [Getting started](getting-started.md#with-cmake-recommended)):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
```

The result is one executable. The library, the fonts and the icons are inside it, so there is nothing to install beside it.

- **Windows:** the app has no console window. Add an icon and version details with your own `.rc` file, as for any Windows program.
- **macOS:** the build gives a plain executable. To get an app with an icon in the Dock, set `MACOSX_BUNDLE` on your target and supply an `Info.plist`, as for any CMake app. An unsigned app is stopped by Gatekeeper on other people's machines until you sign and notarize it.
- **Linux:** the app needs `libX11` at run time, which desktop systems have (through XWayland on Wayland). OpenGL is loaded when it is present, and the software renderer is used when it is not, so the app also starts on a machine with no GPU driver.

CUI is MIT licensed. The fonts and icons inside your executable have their own licenses, which ask you to keep their notices: ship [THIRD_PARTY.md](../THIRD_PARTY.md) and the license files it names with your app, or build with your own fonts (`CUI_NO_DEFAULT_FONTS`, see [Getting started](getting-started.md#options)).
