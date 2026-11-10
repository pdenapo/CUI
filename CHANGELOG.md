# Changelog

What changed in CUI, newest first. Every component mentioned here runs live in the docs.

## Unreleased

### Changed

- **Every component has a description.** Each section of the component docs now opens with one sentence that says what the component is and what it is for. The site shows it under the page title, on the cards that link to the page and in search results, where it used to show whatever sentence came first ("Weeks start on Monday." for the date picker).
- **Site search reads the whole site.** It now searches the text of the guides, the options and struct fields of every component and the changelog, and shows the words around a match. It used to look at titles and summaries only, so "install", "find_package" or the name of an option found nothing.
- **Every page has its own address for search engines.** The site was one page as far as a search engine could tell, so nobody searching for "C date picker" could land on the date picker. The build now writes a page per component and per guide (`c/date-picker-and-calendar/`, `guide/tutorial/`) with its text, title and description, plus `sitemap.xml` and `robots.txt`. Opened in a browser, such a page becomes the live site at that page.
- **Links to the site show a preview.** Every page carries a title, a description and an image for the cards that chat apps and social sites draw; a component's card shows the component.
- The site shows the version it documents beside the name.
- **A function reference.** The site has a page that lists every public function of `cui.h`, `cui_app.h` and `cui_render.h` (496 of them) under the headers' own sections: its signature, the comment written beside it, and a link to the component page or guide that explains it. It has a filter, and site search finds a function by name.
- **Complete programs on component pages.** A page's usage block is a fragment. 71 of the 147 pages now also have the example as a whole program, with its includes, a frame function and `main`, under "Complete program". The site build compiles each one and leaves out any that does not build.
- **More options are described.** Fields that mean the same everywhere (`width`, `icon`, `disabled`, `format` and others) now have a description on every options table, and a field whose type is an enum lists its values.
- **A new README.** It opens with what CUI is in plain words, says what it is not (it draws its own components and does not wrap the system's), compares it with the libraries people know, and lists the limitations in full, including the eight components that still need a pointer. It also says how the code is written and what tests it.
- A component page lists only its own functions, and no longer repeats its option names in a sentence above the options table.
- Release notes link to the files they mention with full addresses, so the links work from the release page.

### Fixed

- The values of an enum on a component page's "Data and results" section showed as `[object HTMLElement]` (17 pages).

## 2026-11-09 · 1.3.1: first steps, a tutorial and ready-to-run examples

A release for people starting out. Every step of the getting-started guide was followed as written, and what did not work is fixed.

### Added

- **Tutorial**: [build a to-do app](docs/tutorial.md) in about 130 lines, step by step: your data as the state, a list that grows, ids for repeated rows, a filter, a dialog and saving to a file. The finished app is `examples/todo.c`, and a test types into it and checks that it does what the tutorial says.
- **Ready-to-run examples**: each release now attaches the example apps built for Windows, macOS and Linux, so you can try them without a compiler.
- `cui_app_target(my_app)` for CMake projects. It gives a window app no console on Windows and keeps `int main(void)` as the entry point, and in release builds it links only the library code the app calls. A first window is about 250 KB with it and about 470 KB without.
- **Component pages on the site list their data.** Under the options, each page now shows the structs you fill in and the structs you get back, with their fields and the values of their enums (105 tables on 63 pages).
- **Every component page says what the keyboard does.** 119 pages list their keys, 20 are marked as display only, and 8 say plainly that they still need a pointer: the rating, the histogram slider's handles, swipe actions, the reorderable list, the JSON viewer, the agent plan's steps, blame and the time histogram.
- [From C++](docs/integration.md#from-c): how to call CUI from C++, with an example the tests compile.
- `tests/first_steps.sh`, run on every push: it takes the code and the commands out of the getting-started guide and runs them, without CMake, as one file, with CMake and from an installed copy.

### Changed

- **Metric cards** put the change under the value when the card is too narrow for both, instead of cutting both short.
- The install snippets name a release (`GIT_TAG v1.3.1`), so a build does not change under you.
- The docs say what "native" means here: compiled code with no browser or runtime, drawing its own components. They also say how to get the small sizes they quote.

### Fixed

- An installed CUI could not be used through `find_package(cui)`: the package did not look for the thread library it links.
- The getting-started guide's "Without CMake" command left out `src/platform/cui_dialogs.c` and did not link.
- With the single-file distribution, the guide's first window did not compile: it needs `#include "cui.h"`, which the guide now says. The guide's one-line build also gave a 1.6 MB program; it now shows the two commands that give about 265 KB.
- The Windows link line in the guide and in `cui.pc` lacked `comdlg32` and `ole32`.

## 2026-11-09 · 1.3: developer tools

### Added

- **Developer tools**, ten components for the tools developers build. Each works at phone width and in a right-to-left interface, builds or draws only what is in view, and allocates only when its data changes. See [Developer tools](docs/components-more.md#developer-tools) and the gallery's new *Developer tools* page.
  - **Trace viewer**: lanes of nested spans for threads, processes or services, with markers across every lane. It zooms around the pointer, pans, measures a range with Shift+drag, folds a lane from its name and walks the call tree with the arrows. 200,000 spans take under a millisecond a frame.
  - **Network waterfall**: requests as a table with timing bars showing queued, DNS, connect, TLS, waiting and download phases, plus DOMContentLoaded and load lines, a breakdown on hover, a summary and a legend.
  - **Dependency graph**: nodes and edges laid out in layers, with cycles broken. Long edges are routed between the nodes, and the order within each layer is chosen to cross fewer edges. Hovering a node lights up its neighbours. It zooms, pans and fits, left to right or top down.
  - **Commit graph**: history with branch lanes, merges, and badges for branches, remote branches and tags. Fifty thousand commits lay out in a few milliseconds.
  - **Pipeline**: the stages of a CI or CD run, with their jobs, status, durations and a summary for each stage. Jobs allowed to fail do not fail their stage, and the stages stack on a phone.
  - **Problems panel**: diagnostics grouped by file, with toggles that filter by severity and show counts, a text filter, folding, keys, and activation to open the file.
  - **Key-value editor**: headers, parameters or environment variables. Typing in the last row adds a pair, and pairs can be switched off, kept secret, or flagged as duplicate keys.
  - **Query builder**: conditions in nested all/any groups, with operators that fit each field's type. `cui_query_text` writes the query as a sentence and `cui_query_match` tests a record.
  - **Schedule picker**: a cron expression edited as every N minutes, hourly, daily, weekly, monthly or custom, with the next runs listed. `cui_cron_next` and `cui_cron_describe` are public.
  - **Keymap**: shortcuts drawn on a keyboard by modifier set, with categories, conflicts, the bindings on hover, and the clicked key returned for rebinding.
- `tests/test_devtools.c`: the developer tools under clicks, keys and random input, at three widths, in both directions and both themes, with timing checks on large inputs.
- **Ten more developer tools**, to the same standard. They are on the gallery's new *More developer tools* page.
  - **Test explorer**: test results as a tree. Suites sum up their tests, and failures show their messages. It has run buttons, Run all, status filters with counts, a name filter and keys.
  - **Stack trace**: an exception with its frames. Library frames fold, and frames open to show their source, highlighted, with the failing line marked. A button copies the trace.
  - **Blame**: source with who last changed each line. Runs of a commit share a gutter entry with author, age and hash, newer changes are shaded stronger, and a hover marks a commit's lines. 100,000 lines take a tenth of a millisecond a frame.
  - **Coverage**: a folder tree built from file paths. Each folder sums up its files, with lines, branches and functions against thresholds, sortable columns and folding.
  - **Regex tester**: matches and groups highlighted in a test text, with the flags i, m and s, errors under the pattern, and a list of the matches and groups. Its engine is public as `cui_regex_search`: groups, named groups, lazy quantifiers, classes and anchors, with a step budget, so a pattern that backtracks for too long stops instead of freezing.
  - **HTTP request bar**: the method, the URL and Send or Cancel, plus `cui_response_status` for a response's status, time and size. `cui_url_params` and `cui_url_set_params` keep a parameters table in step with the URL.
  - **API explorer**: endpoints grouped by tag, with method colors, path parameters marked, deprecated endpoints struck through, parameters, Try it, a filter and keys.
  - **Feature flags**: flags against environments. Each environment has a switch and a share of users set from a popover, and a phone shows one environment at a time.
  - **Uptime bars**: a status page. Each service has its state, its uptime and a bar per day colored by availability, with hover details. Narrow panels keep the latest days.
  - **Time histogram**: event counts over time, stacked by series. Drag to select whole buckets, and hover for the counts. The time labels adapt from seconds to weeks.
- The code editor takes `marks`, ranges of bytes to highlight in the accent or a series color. The highlighter takes `"text"` for no highlighting.

### Fixed

- `CUI_VERSION_STRING` said "1.1.0" in 1.2. It now matches `CUI_VERSION_MAJOR`, `MINOR` and `PATCH`, and the gallery shows the version from them.

## 2026-11-09 · 1.2: phones, tablets, right to left and performance

### Added

- **Right-to-left layout** (pull request #2, by Omid Saadat): `cui_direction` with `CUI_DIRECTION_INHERIT`, `LTR`, `RTL` and `AUTO` (from the first strong character) on `cui_ctx_desc`, `cui_box` and `cui_text_opts`, inherited by children. Under RTL, rows, alignment, padding, grids, stacks and horizontal scrolling mirror, and so do directional icons, switches, progress fills, placeholders and popovers; clicks land on the mirrored rectangles. With `CUI_TEXT_SHAPING=ON` a container's direction is the base direction of its paragraphs. `cui_text_align` gains `CUI_TEXT_START` and `CUI_TEXT_END`. Sliders fill from the right and follow the pointer and arrow keys that way; radio cards swap the arrow keys. Code, numbers, times, key combinations, the week calendar and the gantt keep left to right inside an RTL screen. See [Layout](docs/layout.md).
- `cui_state_on_free(ui, id, fn)`: a function that frees what a piece of state holds (textures, buffers) when the state is dropped or the context is destroyed.
- `cui_request_wakeup(ui, seconds)`: a frame within that many seconds with no input, for content that changes on a timer (a clock, polling, messages arriving). An app that used to redraw continuously for that can now sleep in between.
- **Soak test** (`tests/soak.c`): runs an app the way a person would, clicking what it drew, typing, scrolling, dragging, resizing and leaving it idle, for as long as asked. It records library memory, allocations per frame, frame times, textures and process memory, and fails if anything is left allocated after `cui_destroy`. It runs headless at many times real speed, or in a real window driven by real input events.
- **Emoji picker and color emoji**: the 1,914 emoji of Unicode 17 with names, groups, search, recent emoji, skin tones and a preview. It has keys and touch scrolling. Emoji come from the system's color emoji font (sbix, CBDT or COLR version 0), read from the file as needed, with sequences found through the font's ligatures. `cui_emoji` and `cui_draw_emoji` draw an emoji anywhere. The built-in list adds about 16 KB.
- **Terminal**: output over an input line. It reads ANSI colors (16, 256 and 24-bit), bold, dim, underline and inverse, `\r` progress bars, tabs and clear sequences, and drops the escapes it does not draw. Lines wrap at the panel width, only the rows in view are drawn, and it follows new output. It has drag-to-copy without the colors, command history, Ctrl+C to interrupt and Ctrl+L to clear. The app owns a fixed ring buffer, so writing never allocates.
- **File browser and file picker**: a folder view with sorting, natural name order, a filter, back, forward and up, a typeable path bar, places, type-ahead and keys. The open, save and folder dialog is drawn by CUI. Listing goes through a callback (the local disk by default), and big folders are read once and built only where they show.
- **Image viewer and cropper**: zoom around the pointer, pan, double-click for actual pixels, keys, eased motion and a zoom pill. The cropper has corner and edge handles, aspect lock, a round mask for avatars, thirds lines and the size while dragging.
- **Guided tour**: a spotlight that glides from control to control with a card saying what each one does. It has Back, Next and Done, progress dots and keys, and can show a centered card for steps without a target. The spotlighted control can still be tried, and the button text can be translated.
- **Month calendar**: a month grid with events. Multi-day and all-day events are bars that share lanes across a week; timed events show in time order with a dot and the time, and a full day says "+N more" and opens a list. It supports keyboard navigation across months, double-click to add, localized month and day names, any first weekday, and a phone layout with colored dots and an agenda of the selected day.
- **Radial gauge**: a value on a dial with colored bands, labelled ticks, a target marker and a needle that springs to new values, or a filled arc without the needle.
- **Candlestick chart**: open, high, low and close with volume, overlay lines and a last price tag. It zooms around the pointer, pans by dragging, follows new candles, and shows a crosshair with each candle's values. Zoomed out, candles merge per pixel column, so 100,000 of them build in about a millisecond.
- **Knob, fader and level meter**: the controls of audio, synth and mixer panels. Knobs turn by dragging up and down with a fine mode on Shift, can be bipolar or logarithmic, and reset on double-click. Faders have a labelled scale and an optional meter beside the track. Level meters have peak hold, a falling bar, a clip light and LED segments. The wheel only turns a control that has focus.
- `cui_set_strings(ui, &table)`: month names, short month names, weekday names and "Today" in another language, switched at runtime with one call (issue #11, by pdenapo). A partial table falls back to English for what it leaves out. The date picker, date range picker, calendar, month and week calendars, gantt, calendar heatmap and file browser use it. The names used to be copied in seven files; they now live in one small file that only these components link. See [Integration](docs/integration.md#text-in-other-languages).
- `web/tools/build.sh app my_app.c`: your own app compiled to WebAssembly and opened in the browser at `#/app/my_app`, beside its source (issue #14). Any app whose `main` calls `cui_app_run` builds unchanged. See [Getting started](docs/getting-started.md#run-in-the-browser).
- **Single-file distribution** (issue #10, by r-lyeh): `tools/amalgamate.py` writes `cui.h` + `cui.c` (add one file to any build) and `cui_single.h` (stb style, with `CUI_IMPLEMENTATION`), with the window layer picked by the preprocessor and macros for what CMake options used to choose. Releases attach the files, and CI builds an app from both forms with gcc and clang, warnings as errors. See [Getting started](docs/getting-started.md#one-file).

### Changed

- **`CUI_TEXT_LEFT` is now 3** (`CUI_TEXT_START` is 0 and means left in a left-to-right box, so the default look is the same). Code that uses the names only needs a rebuild; code that stores alignments as numbers changes meaning. `cui_ctx_desc`, `cui_box` and `cui_text_opts` gained a `direction` field.
- **Animation runs at the display's rate.** An animating window used to draw as fast as it could whenever the swap did not wait for the display: under software OpenGL or with vsync forced off by a driver, an always-animating app drew 150 frames a second on three cores, and the software renderer was capped at 120. Frames now come at the display's refresh rate (asked from the system, 60 when it cannot tell). OpenGL notices a swap that does not wait and paces by the clock, and pointer events no longer start frames early. A mixer panel on the software renderer went from 68 frames a second at 10.6% of a core to a steady 60 at 6.8%. On Windows, timed waits use a high-resolution timer, and a raw-input flag that Wine leaves set no longer wakes every wait.
- **Dialogs, sheets and soft shadows paint about twice as fast in software.** A dialog or sheet animating over a dimmed window repaints the whole window each frame, and most of that went into its shadow and the dim layer. The inside of a shadow, where it is a flat color, is now drawn as one run; long runs of one translucent color use a per-channel table that gives the same bytes; the shadow falloff comes from a table instead of `erf` for every pixel; and the blend is inlined. The Mail example's worst frame went from 318 ms to 60 ms (one thread at 2560 × 1600), and the Assistant's sheet from 93 ms to 61 ms. Screenshots are unchanged except for 123 shadow pixels that moved by one level out of 255.
- **Smaller builds.** MinGW's linker keeps whole files (it cannot drop a function with an unwind record), so release builds of CUI now also carry link-time optimization data and the examples link with `-flto=auto`: `assistant.exe` went from 740 KB to 420 KB and `hello.exe` from 418 KB to 300 KB. Apps that do not use LTO link as before. Release builds on Linux and BSD leave out unwind tables, 6 to 7% of an app (`hello` 254 KB to 242 KB); `-DCUI_KEEP_UNWIND_TABLES=ON` keeps them.
- **Smaller Windows builds.** The core called into the emoji code to release its texture, and Windows linkers keep a whole file once anything in it is used, so every Windows app carried the emoji picker and the PNG decoder. The core now reaches emoji through a hook set when they are first used: `hello.exe` went from 478 KB to 407 KB.
- **No GPU, no OpenGL.** When the OpenGL driver draws on the CPU (Mesa llvmpipe, SwiftShader and the like, common in virtual machines and remote sessions), X11 and Win32 apps on the Auto renderer use CUI's software renderer instead, which redraws only what changed. Notes under llvmpipe went from 27% of a core to 4%. `cui_gl_is_software` tells, and `CUI_RENDERER=opengl` keeps OpenGL.
- **Full software repaints are about twice as fast.** An opaque solid, or the inside of an opaque rounded rectangle, is filled as one run of a color instead of pixel by pixel, which was most of the work on a typical screen of panels and cards. The output is identical, byte for byte, on every screenshot.
- **Per-frame buffers shrink again.** Vertices, indices, draw commands, nodes and the frame arena shrink once they have been more than four times bigger than needed for 600 frames, so a heavy page no longer leaves its buffers behind.
- **Long lists keep less state.** Scrolling through thousands of rows used to leave three state entries per row for five seconds, and the table never shrank: paging through a 5,000 row grid held 15,000 entries. Now an entry that holds only last frame's rectangle expires after 30 frames, hover fades take state only once a row is hovered, buttons and links keep none until touched, and the table packs itself when mostly empty. The same paging holds about 1,700 live entries, and 150 at rest.
- **Emoji cost less.** The emoji texture is released once no emoji has been drawn for 600 frames, which frees 1 to 4 MB after the picker closes. Small emoji use a quarter of the texture. Bitmap emoji are scaled with precomputed weights, which makes drawing new ones about three times faster, and at most 12 are drawn per frame, so scrolling the picker stays near 2 ms a frame.
- **The software renderer draws only what changed.** `cui_sw_set_incremental` keeps the last frame in the buffer and repaints only the 64 pixel tiles whose triangles changed, so a hover or a caret blink costs a few tiles rather than the window. New glyphs and emoji arriving in a texture redraw only what samples them. A frame where nothing changed draws nothing, and `cui_sw_last_damage` tells the backend which part to copy to the screen. The app's software path and the browser build use it: with the same input, the Mail example used 3.9% of a core instead of 12.9%. Output is identical to a full repaint (tested frame by frame).
- **The glyph atlas shrinks again.** It still grows when it fills up, but glyphs not drawn for five seconds (sizes from before a zoom or a change of display scale) are dropped, and the rest move into the smallest atlas they fit. A long session no longer keeps a 2048 or 4096 pixel atlas it no longer needs.
- Truncated right-to-left text puts its ellipsis on the left, where the text ends (with text shaping).
- Text field placeholders are shaped like other labels, so Arabic and other joined scripts read correctly in them (with text shaping). The text being edited stays unshaped, as before.
- **Every component page fits a phone.** Layouts that used to need more than a phone's width now rearrange instead of shrinking the preview. The calendar, countdown, radar chart, activity rings and donut legend scale down. The waffle legend moves under its grid, and the banner puts its action under the message. The player drops its previous button, and the stepper keeps only the current label. Pagination collapses to the first, current and last pages, and the status bar keeps icons and drops labels. Toolbars, path bars and inspectors fold, and the dock shrinks its icons and stops magnifying.
- **Docking** shows every panel as a tab of one stack when the dockspace is too narrow for panes side by side. The saved layout is untouched, and the panes come back when there is room.
- **Segmented controls and button groups** tighten their padding before a label truncates.
- **Sankey, funnel and heatmaps** keep their labels readable on narrow charts. Sankey labels stack over their values, funnel step rates shorten, heatmaps label every other row, and the calendar heatmap drops its Less and More words.
- **Gantt charts** use a slimmer task list on phones, so the timeline keeps most of the width. Tables fade at the edge that has more columns.
- Prompt suggestions and radio cards drop to fewer columns before their text gets cramped. The command palette's key hints wrap to a second line.
- Signatures scale down with a narrower pad instead of being cut off.
- The docs site lays out each preview at the width it has before measuring it, so components that adapt are shown at full size.

### Fixed

- In radio cards, one press of an arrow key moved two cards: the card that took focus acted on the same press.
- The gallery's image compare page made new textures each time it was visited and never freed the old ones, about 2 MB per visit. Found by the soak test.
- Joined wheel pickers in the demo sit flush, so the selection band runs across all three.
- A demo sankey read past the end of its link array.
- The X11 backend declared one typedef twice, which C99 does not allow (clang warned about it).

### Documentation

- [How CUI is built](docs/architecture.md): the frame lifecycle, where things live, the outline every component follows and how to change code without changing behavior. Contributing gains rules for keeping the code readable, and the largest components were split into named phases with no change in what they draw.
- Integration explains what a window costs in memory with each renderer, measured, and how much of it is the graphics driver rather than CUI: a bare OpenGL window takes 76 MB on its own on the test machine, CUI's own memory is 2 MB or so (issue #9).
- Getting started lists what to install on Windows, macOS and three Linux families, builds and runs an example from a fresh checkout, explains running in the browser and where Android and iOS stand, and has a troubleshooting table for the common build and startup problems (issue #3).

## 2026-11-07 · 1.1: text, code and workspace

### Added

- **Docking**: drag panels by their tabs to stack them or split any side, with drop zones and a preview while dragging, resizable dividers, closing and reopening in place, and a layout that saves and loads as a short readable string. It builds on the existing document tabs and split view.
- **Tree table and path bar**: an expandable tree with columns on top of the data grid, for file explorers, scene graphs and debugger variables. It handles large trees, loads folders lazily, and follows the usual tree keys. The path bar collapses long paths, offers sibling menus and switches to a text field for typing a path.
- **Timeline editor**: grouped tracks with keyframes, a time ruler with in and out markers, a scrubbable playhead, clips, box selection, snapping to frames and zoom around the pointer. Its keys are curve editor keys, so the selected track's curve can be edited right beside it.
- **Markdown view**: headings, emphasis, links, nested and task lists, quotes, tables and fenced code with the existing highlighter. It renders partial text sensibly while an answer streams in, reports link clicks and task toggles instead of acting on them, and AI chat messages can render their body as markdown.
- **Code editor**: an editable code block with syntax highlighting, a line number gutter, selection, undo and redo, auto-indent, bracket matching, find (Ctrl+F) and go to line (Ctrl+G). Only the lines in view are laid out, so long files stay fast.
- **Diff viewer**: split and inline modes, word-level marks inside changed lines, folded unchanged stretches, next and previous change, and syntax colors. The diff runs only when a text changes, and `cui_diff_compute` gives the same diff without any UI.

### Changed

- **Component pages** in the docs are now organized into Usage, Behavior, Keyboard, Options and Related, with an "On this page" index. The options table is generated from `cui.h`, keys are listed on their own, related components are linked, screenshots sit behind a toggle, source links open the exact lines on GitHub, and every live preview can switch between full, tablet and phone widths.
- **The components index** has a filter box, category chips, "New" labels and a compact list view.
- A focused editor can keep the Tab key for indenting; Esc gives Tab back to focus movement.
- **Data grid**: columns can declare a priority and hide on narrow widths, rows can select without a checkbox column, a cursor set from code scrolls into view, and truncated cells show their full text on hover.
- **Changelog**: this page, also on the docs site.
- The README lists every component group, including the new app chrome, code and tool panels, and leads its showcase with docking.
- Version 1.1.0, with about 160 components.

### Fixed

- The data grid's sticky header no longer disappears once the rows scroll past it.
- A scroll area squeezed to zero width no longer produces invalid values in its edge fade.

## 2026-11-06 · Tools and a new landing page

### Added

- **App chrome**: a menu bar with full keyboard access, a toolbar that collapses into a "more" menu, a status bar, and editor-style document tabs that close, reorder and scroll.
- **Tool panels**: a property inspector with typed editors in folding groups, and a log console over a caller-owned ring buffer with level filters, search and follow.
- **Node editor**: a pannable, zoomable canvas of nodes, typed pins and curved links, with box selection, a minimap and ordinary components inside nodes.
- **Flame graph**: a profiler timeline that stays fast with 100,000 spans, with zoom around the pointer, hover timings and zoom to a span.
- **Hex viewer**: synced offset, hex and ASCII columns, virtualized for large files, with highlighted ranges, copy and optional editing.
- **Curve and gradient editors**: keyframes with bezier handles and a color ramp with stops. `cui_curve_eval` and `cui_gradient_eval` sample exactly what the editors draw.
- **Arabic-script shaping and bidirectional text**, optional, contributed in pull request #1 by omid3098.

### Changed

- **A new landing page** for the live docs: a lighter hero ending in the C mark, a tabbed live showcase, a bento of live components, a theme playground, guide cards and motion throughout, all off with reduced motion.
- The phone navigation fades the edge it scrolls past instead of cutting a word in half.
- A truncated label no longer shows its tooltip through a menu, popover or dialog open above it.
- The smallest complete app now measures about 250 KB stripped.

## 2026-11-05 · A calmer design and the live docs

### Changed

- **A more timeless look**: one concentric radius scale, quieter edges, flat solid controls, and shared motion, state, radius, edge and icon tokens.
- **Responsive layout**: rows wrap, grids pick their column count, and components adapt to narrow widths.
- Keyboard focus behaves after a click, modifier keys release correctly, and avatar stacks are tidier.

### Added

- **Live docs** on GitHub Pages: every component running in the browser, compiled to WebAssembly, with its C source and docs beside it. The site has a raster C mark and favicon.
- A shorter, code-first README.
- A contributing guide with disclosure and evidence rules for people and AI agents, and issue and pull request forms.

## 2026-11-05 · 1.0

The first public version.

### Added

- **The core**: layout, draw list, text with kerning and font fallback, themes with light and dark and eight accents, an OpenGL renderer and a software renderer.
- **Backends** for Win32, Cocoa, X11, SDL2 and GLFW, with native file dialogs, URL opening and OS file drag and drop.
- **Components** for actions, inputs, pickers, selection, navigation, overlays, feedback, data display, a virtualized data grid, a command palette, kanban, week calendar and gantt charts, a dozen kinds of chart, conversation and media components, text effects and AI interface components.
- **Example apps**: gallery, dashboard, settings, notes, mail, tracker, music, finance and an AI assistant, each one C file.
- **Tests and CI** on Linux, Windows and macOS: unit, interaction and render tests, a randomized input fuzzer, and AddressSanitizer and UndefinedBehaviorSanitizer builds.
- **Documentation** with screenshots rendered by the tests, so visual changes and their docs move together.

### Changed

- Contrast meets WCAG AA, wrapped text avoids lone last words, and tabular figures line up.
- The library stays small: compressed fonts and dead code stripping, so apps carry only what they use.
