<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="web/site/img/cui-mark-dark.svg">
  <img src="web/site/img/cui-mark-light.svg" width="64" height="64" alt="">
</picture>

# CUI

**A UI library for C that draws its own components.**

[![CI](https://img.shields.io/github/actions/workflow/status/yappologistic/CUI/ci.yml?branch=master&label=CI&style=flat-square&labelColor=24292f)](https://github.com/yappologistic/CUI/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/yappologistic/CUI?style=flat-square&labelColor=24292f&color=57606a)](https://github.com/yappologistic/CUI/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-57606a.svg?style=flat-square&labelColor=24292f&color=57606a)](LICENSE)

[Live site](https://yappologistic.github.io/CUI/) ·
[Getting started](docs/getting-started.md) ·
[Tutorial](docs/tutorial.md) ·
[Components](docs/components.md) ·
[Downloads](https://github.com/yappologistic/CUI/releases)

<br>

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/showcase/components-dark.png">
  <img src="docs/images/showcase/components.png" alt="CUI components: a sign up form, metric cards, a calendar with time slots, a bar chart, team members and settings">
</picture>

<sub>Every image on this page is drawn by CUI, from plain C.</sub>

</div>

CUI is an immediate-mode UI library written in C99. You describe the screen with function calls every frame. CUI lays it out, handles the mouse and keyboard, and draws it with OpenGL or its own software renderer, in a window on Windows, macOS or Linux, or in a browser through WebAssembly.

- **About 210 components**, from buttons and text fields to a data grid, charts, a code editor and docking panels.
- **No dependencies.** One CMake target, or two files (`cui.h` and `cui.c`) dropped into your project.
- **Small.** A complete app, window, fonts and all, is about 250 KB. There is no browser or runtime inside.
- **MIT licensed.**

## Try it first

- **In your browser:** the [live site](https://yappologistic.github.io/CUI/) runs every component and the example apps. It is the same C code, compiled to WebAssembly.
- **On your computer, without a compiler:** each [release](https://github.com/yappologistic/CUI/releases) has the example apps built for Windows, macOS and Linux.
- **In code:** the [tutorial](docs/tutorial.md) builds a to-do app in about 130 lines.

## A small program

```c
#include <cui/cui.h>
#include <cui/cui_app.h>

int main(void) {
    cui_app *app = cui_app_create(&(cui_app_desc){
        .title = "Hello", .width = 420, .height = 300 });
    if (!app) return 1;
    bool notifications = true;
    int clicks = 0;

    while (cui_app_frame(app)) {
        cui_ctx *ui = cui_app_ui(app);
        cui_column(ui, .pad = 32, .gap = 16) {
            cui_heading(ui, "Hello, CUI", 2);
            cui_secondary(ui, "A native window, a few components "
                              "and no dependencies.");
            cui_switch(ui, "Notifications", &notifications);
            cui_row(ui, .gap = 8) {
                if (cui_button(ui, "Click me", .variant = CUI_BUTTON_PRIMARY))
                    clicks++;
                cui_textf(ui, "Clicked %d times", clicks);
            }
        }
    }
    cui_app_destroy(app);
    return 0;
}
```

<table>
<tr>
<td width="40%" valign="middle"><img src="docs/images/showcase/hello-dark.png" alt="The hello example: a heading, a line of text, a switch and a button"></td>
<td valign="middle">

**A component is a function call.** It draws itself, handles input and returns what happened: `cui_button` returns true on the frame it is clicked.

**Options are named arguments.** Leave one out and it takes its default.

**Your variables are the state.** The screen is described again every frame, so there are no widgets to create, keep or update.

</td>
</tr>
</table>

## What it is, and what it is not

CUI draws every component itself. It does not wrap the system's buttons and menus, so an app looks the same on every system and does not look like the system's own apps. "Native" here means compiled code in a real window, with nothing interpreted and no web view.

How it sits beside the libraries you may know:

- **Dear ImGui and Nuklear** are also immediate-mode. They are aimed mostly at tools and debug panels inside a program that already has a renderer. CUI uses the same model for the app itself: it opens the window, and its components come with the things end users expect, such as full text editing, animation, light and dark themes and right-to-left layouts.
- **Qt, GTK and wxWidgets** keep a tree of widgets for you and are far more mature. They have screen reader support, complete input-method support and years of use behind them. If you need those today, use one of them.
- **Electron and web views** ship a browser with your app. CUI ships about 250 KB.

CUI is a good fit for a desktop tool, an editor, a dashboard or a game's interface written in C or C++, where you want it small and want it to look finished without designing every control. Read [Limitations](#limitations) before you decide.

## Install

With CMake (GCC, Clang, MSVC and MinGW are tested):

```cmake
include(FetchContent)
FetchContent_Declare(cui
    GIT_REPOSITORY https://github.com/yappologistic/CUI.git
    GIT_TAG v1.4.0)
FetchContent_MakeAvailable(cui)
target_link_libraries(my_app PRIVATE cui::cui)
cui_app_target(my_app)   # no console window on Windows, and only the code your app uses
```

Without CMake, take `cui.h` and `cui.c` from a [release](https://github.com/yappologistic/CUI/releases) and compile them with your program. [Getting started](docs/getting-started.md) has the exact commands for both, and CI runs those commands on every push.

To build the library and run an example:

```sh
git clone https://github.com/yappologistic/CUI.git
cmake -S CUI -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/examples/tracker
```

Linux needs the X11 development headers (`libx11-dev`). Windows and macOS need nothing beyond the system SDK.

## What is in it

| Group | Components |
|---|---|
| **Basics and actions** | text, buttons, icon buttons, split and toggle buttons, menus, command palette |
| **Inputs** | text field, text area, number, money, tags, one time code, select, combobox, multi-select, shortcut recorder, dropzone |
| **Selection and pickers** | checkbox, switch, radio, slider, chips, date and date range, time, color, emoji, knob and fader |
| **Navigation and windows** | tabs, sidebar, breadcrumb, menu bar, toolbar, status bar, document tabs, docking panels |
| **Overlays and feedback** | dialog, sheet, popover, tooltip, toast, alert, progress, skeleton, empty state, guided tour |
| **Data** | table, data grid for very large data, tree, tree table, list, file browser with open and save dialogs |
| **Code and text** | code block, code editor, diff viewer, markdown view, JSON viewer, terminal, log console, hex viewer |
| **Charts** | line, bar, donut, sparkline, gauge, candlestick, heatmap, treemap, radar, funnel, sankey, scatter |
| **Planning** | kanban board, week and month calendars, gantt chart, node editor |
| **Developer tools** | trace viewer, network waterfall, flame graph, dependency and commit graphs, CI pipeline, test explorer, regex tester, query builder |
| **Instruments** | dot-matrix chart, chart annotations, event track on a time axis, readout, dot-matrix display |
| **Creative controls** | status tag, ratio slider, gradient and step sliders, palette strip, aspect ratio picker, seed field |
| **Canvas and bars** | grouped and floating toolbars, format bar, live cursors, canvas zoom controls, result card, selection bar |
| **Conversation and AI** | chat, comments, composer, reasoning and tool calls, agent plan, sources and citations, voice recorder, voice notes, voice session |
| **Your own** | a canvas with a 2D drawing API, per-component state and spring animation |

Every component has a page with a live example, its options, its data structures and what it does with the keyboard: on the [site](https://yappologistic.github.io/CUI/#/components), or in [Components](docs/components.md) and [More components](docs/components-more.md).

<details>
<summary><b>Pictures of the larger components</b></summary>
<br>

<table>
<tr>
<td colspan="2"><picture><source media="(prefers-color-scheme: dark)" srcset="docs/images/components/docking-dark.png"><img src="docs/images/components/docking.png" alt="A docked workspace with a scene tree, a viewport, a log console and a property inspector, with a tab being dragged to a new place"></picture></td>
</tr>
<tr>
<td colspan="2" align="center"><sub><b>Docking</b>: drag a tab to stack it or split any side; the layout saves as a short string · <a href="docs/components-more.md#docking">docs</a></sub></td>
</tr>
<tr>
<td colspan="2"><img src="docs/images/showcase/hero-data.png" alt="A data grid with a filter bar, two selected rows, team tags and score bars"></td>
</tr>
<tr>
<td colspan="2" align="center"><sub><b>Data grid</b>: 2,000 virtualized rows, filters, sorting, resizable columns and spreadsheet style selection</sub></td>
</tr>
<tr>
<td width="50%"><img src="docs/images/showcase/hero-board.png" alt="A kanban board with a card being dragged between columns"></td>
<td width="50%"><img src="docs/images/showcase/hero-palette-dark.png" alt="A command palette with grouped commands and shortcuts"></td>
</tr>
<tr>
<td align="center"><sub><b>Kanban</b>: the lifted card follows the pointer and the column opens a slot for it</sub></td>
<td align="center"><sub><b>Command palette</b>: fuzzy search, groups and shortcuts</sub></td>
</tr>
<tr>
<td colspan="2"><img src="docs/images/showcase/hero-pick-dark.png" alt="A date range picker with presets and two months, and a wheel time picker"></td>
</tr>
<tr>
<td colspan="2" align="center"><sub><b>Pickers</b>: a date range with presets over two months, and wheels that spin with momentum</sub></td>
</tr>
</table>

<details>
<summary><b>More: charts, planning, conversation, effects</b></summary>
<br>

<table>
<tr>
<td width="50%"><img src="docs/images/showcase/hero-charts.png" alt="A contribution heatmap, a sankey of test runs, a radar chart and a task plan"></td>
<td width="50%"><img src="docs/images/showcase/hero-desk-dark.png" alt="Control tiles, a now playing card, a waveform and a dock"></td>
</tr>
<tr>
<td align="center"><sub><b>Charts</b>: calendar heatmap, sankey and radar, with a live task plan</sub></td>
<td align="center"><sub><b>Dock, tiles and player</b>: icons grow under the pointer, the waveform seeks on click</sub></td>
</tr>
<tr>
<td><img src="docs/images/showcase/hero-social-dark.png" alt="Chat bubbles, threaded comments, a rolling counter and presence avatars"></td>
<td><img src="docs/images/showcase/hero-effects-dark.png" alt="Three cards for choosing a renderer, one lit by the pointer"></td>
</tr>
<tr>
<td align="center"><sub><b>Conversation</b>: grouped bubbles, threads, a rolling counter, presence</sub></td>
<td align="center"><sub><b>Spotlight cards</b>: the surface and edge nearest the pointer light up</sub></td>
</tr>
<tr>
<td colspan="2"><img src="docs/images/showcase/hero-plan-dark.png" alt="A gantt chart with dependencies, progress and a today line"></td>
</tr>
<tr>
<td colspan="2" align="center"><sub><b>Gantt</b>: drag to reschedule, pull the ends to resize, arrows follow dependencies</sub></td>
</tr>
</table>

</details>

</details>

## Example apps

Complete apps in [`examples/`](examples), each a single C file. They run in the browser on the [site](https://yappologistic.github.io/CUI/#/apps).

<table>
<tr>
<td colspan="2"><img src="docs/images/showcase/assistant.png" alt="An AI assistant built with CUI"></td>
</tr>
<tr>
<td colspan="2" align="center"><sub><b>Muse</b>: reasoning, tool calls, sources, a plan and a composer with a model picker · <a href="examples/assistant.c">assistant.c</a></sub></td>
</tr>
<tr>
<td width="50%"><img src="docs/images/showcase/tracker.png" alt="An issue tracker built with CUI"></td>
<td width="50%"><img src="docs/images/showcase/dashboard-dark.png" alt="An analytics dashboard built with CUI"></td>
</tr>
<tr>
<td align="center"><sub><b>Orbit</b>: grouped issues, custom glyphs, an activity pane · <a href="examples/tracker.c">tracker.c</a></sub></td>
<td align="center"><sub><b>Larkspur</b>: figures, charts and a sortable table · <a href="examples/dashboard.c">dashboard.c</a></sub></td>
</tr>
</table>

<details>
<summary><b>All examples</b></summary>
<br>

| Example | What it shows |
|---|---|
| [`hello`](examples/hello.c) | The smallest app |
| [`todo`](examples/todo.c) | The app the [tutorial](docs/tutorial.md) builds: a list, a filter, a dialog, saving to a file |
| [`caffeine`](examples/caffeine.c) | The instruments together: a dot-matrix chart with annotations, cups you drag on a time axis, readouts and a dot-matrix verdict |
| [`assistant`](examples/assistant.c) | AI assistant: reasoning, tool calls, sources, plan, composer with a model picker |
| [`tracker`](examples/tracker.c) | Issue tracker: grouped list, custom status glyphs, detail pane, activity |
| [`music`](examples/music.c) | Music player: cover art drawn on a canvas, track list, transport bar |
| [`finance`](examples/finance.c) | Personal finance: top navigation, charts, painted card, budgets, goals |
| [`dashboard`](examples/dashboard.c) | Sidebar, top bar, KPIs, charts and a sortable table |
| [`mail`](examples/mail.c) | Three pane inbox, folders, reading pane, compose dialog |
| [`notes`](examples/notes.c) | Split view, search, context menus, dialogs, shortcuts, file export |
| [`settings`](examples/settings.c) | Forms, validation, preferences and a danger zone |
| [`gallery`](examples/gallery_ui.c) | Every component, light and dark, all accents |

<table>
<tr>
<td width="50%"><img src="docs/images/showcase/music.png" alt="Music player"></td>
<td width="50%"><img src="docs/images/showcase/finance.png" alt="Personal finance app"></td>
</tr>
<tr>
<td><img src="docs/images/showcase/mail-dark.png" alt="Mail client"></td>
<td><img src="docs/images/showcase/notes-dark.png" alt="Notes app"></td>
</tr>
</table>

</details>

## Themes

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/showcase/accents-dark.png">
  <img src="docs/images/showcase/accents.png" alt="The same controls in all eight accents">
</picture>

```c
cui_set_theme(ui, CUI_THEME_DARK);      /* animates */
cui_set_accent(ui, CUI_ACCENT_BLUE);    /* neutral, violet, blue, green, amber, orange, coral, rose */
```

Colors are named tokens (`CUI_COLOR_SURFACE`, `CUI_COLOR_ACCENT` and so on), so your own drawing follows the theme too. See [Theming](docs/theming.md).

## Size and speed

| Measure | Result |
|---|---|
| `hello` on Linux, the whole executable, stripped | about 250 KB |
| `assistant` (the AI app above), the whole executable | about 360 KB |
| The whole library: every component, both renderers, the X11 backend, fonts and icons | about 1,130 KB |
| Build, lay out and paint one frame (gallery pages, 1240 × 820 points at 2×) | 0.09 to 0.75 ms, most under 0.25 |
| A real app in use (Notes, Mail, an IDE; software renderer, 1280 × 800) | 2 to 3% of one core, 11 to 13 MB of memory |
| Library memory over an hour of use | 2 to 9 MB, flat, with nothing left after `cui_destroy` |
| CPU while nothing happens | 0%: the app waits for events |

An app carries only the components it calls, which is why `hello` is a quarter of the library. The times are wall clock on a 4 core cloud VM. `cui_bench` in [`tests/`](tests) reproduces them, and [Performance](docs/performance.md) describes the hour-long runs behind the memory and CPU rows.

## Platforms

| Platform | Backend | Renderer | Tested in CI |
|---|---|---|---|
| Windows 7+ | Win32 (per monitor DPI, dark title bar) | OpenGL 3.3, GDI software fallback | MSVC, MinGW |
| macOS 10.13+ | Cocoa (Retina, native menus, text input) | OpenGL 3.3+, layer software fallback | Apple clang |
| Linux, BSD | X11 (XIM text input, HiDPI) | OpenGL 3.3 (GLX), XPutImage fallback | GCC, Clang |
| Wayland and others | SDL2 or GLFW | OpenGL 3.3 | GCC |
| Browser | WebAssembly, plain clang ([how](docs/getting-started.md#run-in-the-browser)) | WebGL2, or software | Docs site, every push |
| Anything | Your own | Your own (one shader) | |

To use CUI inside an engine with your own window and renderer, see [Integration](docs/integration.md) and [Rendering](docs/rendering.md). The core only emits triangles.

## Limitations

- **No screen reader support.** There is no accessibility tree yet. This rules CUI out for apps that must be accessible to blind users.
- **The keyboard does not reach everything.** Most components work from the keyboard and each one's page lists its keys. Some parts still need a pointer: the rating, the histogram slider's handles, swipe actions, the reorderable list, the JSON viewer, the agent plan's steps, blame, the time histogram, and the choices inside the open time picker and color picker.
- **Complex scripts are partial.** Text is UTF-8 with kerning and font fallback, and layouts mirror for right-to-left languages. Arabic-script shaping and bidirectional text are an optional, experimental build option for static text ([details](docs/integration.md#optional-arabic-script-text-shaping)). CJK works once you add a fallback font.
- **Input methods compose in the system's window.** Committed text arrives; the text being composed is not drawn inline. The window opens at the caret on Windows, macOS, SDL2 and the docs site, but not yet on the X11 and GLFW backends.
- **No native Wayland backend.** Use the SDL2 or GLFW backend, or XWayland.
- **No Android or iOS.** Layouts adapt to phone widths and touch, and apps run in mobile browsers, but there is no platform layer for phones.
- **It is new.** Version 1.0 was released in November 2026, so it has little real-world use behind it yet. The [changelog](CHANGELOG.md) lists what each release changed.

## How it is built and tested

Most of CUI's code is written by AI models under the maintainer's direction. Every commit names the model that wrote it in a `Co-Authored-By` trailer, and the [contribution rules](docs/CONTRIBUTING.md) ask the same of everyone who sends a change.

Because of that, the tests are what the project stands on. Every push runs 19 CI jobs:

- unit, interaction and render tests on Windows (MSVC and MinGW), macOS and Linux (GCC and Clang, with the X11, SDL2 and GLFW backends);
- a fuzzer that drives the gallery and the example apps with random input under AddressSanitizer and UndefinedBehaviorSanitizer;
- real windows opened on all three systems;
- the single-file build with GCC and Clang;
- the commands of the getting-started guide, taken from the guide and run as written.

The images in [`docs/images`](docs/images) are rendered by the tests, so a visual change and its documentation move together.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Documentation

| Guide | What it covers |
|---|---|
| [Getting started](docs/getting-started.md) | Install, build, the frame loop, your first screens |
| [Tutorial](docs/tutorial.md) | Build a small, complete app step by step |
| [Building an app](docs/apps.md) | The window, when frames happen, what components return, threads, shipping |
| [Recipes](docs/recipes.md) | Complete programs: a form, a sorted table, a background task, saved settings |
| [Layout](docs/layout.md) | Rows, columns, sizes, alignment, grids, scrolling, split views |
| [Components](docs/components.md) and [more](docs/components-more.md) | Every component: what it is, usage, options, keyboard |
| [Theming](docs/theming.md) | Tokens, light and dark, accents, fonts, motion |
| [Icons](docs/icons.md) | The built-in Lucide icons, and how to use another icon font |
| [Custom components](docs/custom-components.md) | The canvas, the drawing API, state and animation |
| [Integration](docs/integration.md) | Your own window, input and renderer; C++; binary size |
| [Rendering](docs/rendering.md) | The vertex format and the reference shader |
| [Performance](docs/performance.md) | How memory and speed are tested, and the results |
| [Function reference](https://yappologistic.github.io/CUI/#/reference) | Every public function with its signature, on the site |
| [Questions](docs/faq.md) | What CUI is and is not, how it compares, what it cannot do |
| [How CUI is built](docs/architecture.md) | The code map and the outline every component follows |
| [Design principles](docs/design.md) | The visual rules every component follows |

The [live site](https://yappologistic.github.io/CUI/) has the same guides with every component running beside its page. It is rebuilt from `master` on every push; to run it locally, use `web/tools/serve.sh` (needs clang with the `wasm32` target).

## Contributing

Bug reports and pull requests are welcome, from people and from AI agents. Every report needs an honest account of who wrote it (and which model, if an AI did) and evidence you produced yourself: the commands you ran and their output. [Contributing](docs/CONTRIBUTING.md) explains what that means.

## Credits

Type: [Inter](https://rsms.me/inter/) and [Geist](https://vercel.com/font) (SIL Open Font License). Icons: [Lucide](https://lucide.dev) (ISC). Font rasterization, image decoding and PNG output: [stb](https://github.com/nothings/stb) (public domain). See [THIRD_PARTY.md](THIRD_PARTY.md). The site's overview uses Ansel Adams' *The Tetons and the Snake River* (1942, U.S. National Archives, public domain).

## License

MIT. See [LICENSE](LICENSE).
