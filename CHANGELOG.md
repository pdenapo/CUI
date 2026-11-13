# Changelog

What changed in CUI, newest first. Every component mentioned here runs live in the docs.

## Unreleased

### Added

- SQL syntax colors: `.lang = "sql"` in the code block, the code editor and the diff viewer. Keywords, types and `NULL`, `TRUE` and `FALSE` match in any case, as SQL does; `--` starts a comment and `/* */` comments run across lines. The keywords cover standard SQL and common extensions such as `ilike`, `qualify`, `pivot` and `pragma` ([#33](https://github.com/yappologistic/CUI/issues/33)).
- `cui_set_clipboard_text` and `cui_get_clipboard_text`: copy from a menu item or a status bar ("Copy path") and read the clipboard for a Paste item, through the same platform hooks the copy button and text fields use ([#32](https://github.com/yappologistic/CUI/issues/32)).
- `cui_app_follow_system_theme(app, follow)` starts or stops following the system's light or dark setting while the app runs, for a theme switcher with Light, Dark and System. Stopping keeps the current theme, so a `cui_set_theme` after it lasts (on macOS the system setting used to undo it every frame); starting applies the system setting at once ([#34](https://github.com/yappologistic/CUI/issues/34)).
- **Dither**, four components drawn in dots on an 8×8 ordered dither, in theme colors, the same in the OpenGL and the software renderer. A dot is the color or nothing, never blended, so fills stay crisp and lower densities are always a subset of higher ones.
  - **Dither progress** (`cui_progress(..., .dither = true)`): solid up to the value, then dots that thin out over 60 points past the edge, drifting slowly forward; indeterminate, a soft band of dots sweeps the track. The drift asks for a frame only when it moves a dot, and reduced motion holds it still. See [Progress](docs/components.md#progress-meter-spinner-and-skeleton).
  - **Pattern meter** (`cui_pattern_meter`): a stacked usage bar whose parts differ in color and in density, with one dot between parts, free space in the track color and a legend of dithered swatches. Hovering a part or its legend entry, or stepping with Left and Right, steps the others back and shows its name, value and share. `.grayscale` tells the parts apart by density alone, for print and color blindness. See [Pattern meter](docs/components.md#pattern-meter).
  - **Dither area chart** (`cui_line_chart(..., .dither = true)`): every series filled with dots that thin out from its line to the bottom of its band; with `.stacked` (new for line charts) each band fades within itself and bands never mix into a third color. Lines, crosshair and tooltip stay as they are. See [Charts](docs/components.md#charts).
  - **Veil** (`cui_veil`): covers a picture, a secret or a spoiler with slowly moving muted dots that keep its space, with an optional label and hint; a click, Enter or Space dissolves it dot by dot in about a quarter of a second, and it comes back the same way. The content takes no clicks and no keyboard focus while covered. See [Veil](docs/components-more.md#veil).
- **Dither spinners**, loading animations in the same dots, built on the dither primitive and drawn the same in every renderer. They ask for a frame only when their dots move and only while on screen, and reduced motion holds them on a still frame.
  - **Spinner styles** (`cui_spinner(ui, size, .style = ...)`): comet ring, dot matrix, fill and drain, radar sweep, ripple, upload, download and transfer, orbit and bouncing dots (with trails), document scan, pulse, spiral and plasma, in the text color or `.color`. Under 32 points a dot is one point, so a 14 point spinner sits sharp in a button or a chip. `cui_spinner(ui, 20)` is still the plain ring. See [Dither spinners](docs/components.md#dither-spinners).
  - **Status spinner** (`cui_status_spinner`): the comet ring while a `cui_status` is busy, then a disc in the success or danger color with a check or a cross that holds for a moment and dissolves away (or stays, with `.keep`). It keeps its space, and returns false once there is nothing left to show. See [Status spinner](docs/components-more.md#status-spinner).
  - **Skeleton group** (`cui_skeleton_group`): placeholders built inside it share one shine that passes over all of them; with `.dither = true` they are drawn in dots with a band of brighter dots sweeping across. See [Skeleton group](docs/components-more.md#skeleton-group).
- **Creative controls**, eight components for editors, generators and design tools; six of them sit in a control well, a rounded panel with a label, a value and the control. See [Creative controls](docs/components-more.md#creative-controls) and the gallery's new *Creative controls* page.
  - **Status tag** (`cui_status_tag`): an icon, a label and a tinted fill for pending, in progress (a slowly turning dashed ring), submitted, in review, success, failed and expired, or your own icon and tone. A small size, and an optional remove x.
  - **Ratio slider** (`cui_ratio_slider`): a whole split into two named parts with a bar to drag; the names and percentages stay readable at any split. Arrow keys, Shift for steps of ten, Home and End.
  - **Gradient slider** (`cui_gradient_slider`): the track is the value, a hue wheel, a white balance, an opacity over a checkerboard or your own stops, and the thumb is filled with the color under it. An optional tool button.
  - **Step slider** (`cui_step_slider`): a few stops marked with dots; the thumb snaps to each and glides between them. Optional names for the stops and an action button.
  - **Palette strip** (`cui_palette_strip`): joined swatches you lock with a click, and a shuffle that recolors the rest from a seed you keep, so the same seed gives the same palettes. The swatch under the pointer widens and shows its hex code.
  - **Aspect ratio picker** (`cui_aspect_picker`): each ratio drawn in its own proportions, a photo set or your own, an optional dashed Custom, and the size in pixels.
  - **Option field with action** (`cui_option_action`): a compact segmented choice and the icon button of the action it feeds.
  - **Seed field** (`cui_seed_field`): a seed in the monospaced font; the dice rolls a new one with rolling digits, and the lock keeps it.
- **Canvas and bars**, six components for editors with a canvas:
  - **Grouped toolbar** (`cui_toolbar`): `.style = CUI_TOOLBAR_GROUPED` puts items in raised groups (`cui_toolbar_group`) inside one pill, with an inset radio set (`cui_toolbar_radio`) and a filled primary action (`cui_toolbar_primary`); `CUI_TOOLBAR_FLOATING` is one bordered pill for bars over content. Both keep the toolbar's keys and overflow menu, and every style gains cycle pickers (`cui_toolbar_cycle`), colored buttons (`.tone`) and a corner mark for buttons that open a flyout (`.flyout`, `cui_flyout_mark`).
  - **Format bar** (`cui_format_bar`): one call edits a `cui_text_format`: a color grid, bold, underline, link, a text style menu that previews each style at its size, alignment and list flyouts, and delete behind a small confirmation. Menu items take a `font` and `text_size` for such previews.
  - **Live cursors** (`cui_live_cursors`): other people's pointers over a canvas with name flags and chat bubbles, gliding to each new position at any frame rate; `cui_presence_pill` shows who is here and who is speaking, and `cui_selection_ring` marks what someone has selected.
  - **Canvas controls** (`cui_canvas_controls`): a floating zoom rail (in, a percentage that resets to 100%, out, fit and toggles of your own) that drives the node editor's `cui_node_view`, stepping through zoom presets and keeping the middle in place.
  - **Result card** (`cui_result_card`): a picture cropped to rounded corners with a status chip and a title and caption on a fade; its actions go in a floating toolbar under it.
  - **Selection bar** (`cui_selection_bar`): a bar that slides up under a list while something is selected, with the count, a clear button and labeled actions, and returns the action picked; Esc clears the selection.
- **Voice components**, built on one dot waveform that only moves with real audio:
  - **Voice waveform** (`cui_voice_wave`): a voice drawn as fine dots, 13 rows mirrored around a center line, each dot as bright as the level covers it. It scrolls as you push levels, or shows a whole clip with its played part in the accent, a playhead and seeking by click, drag or arrow keys. Silence draws only a faint center line, and two voices can share one timeline. `cui_voice_resample` keeps a short waveform for a message. See [Voice](docs/components-more.md#voice) and the gallery's new *Voice* page.
  - **Voice recorder** in the chat composer: `.mic` adds a microphone button, and while the app sets `.recording` the composer shows a recording dot, the time, the mic levels scrolling by, Cancel and Send, at the same height, returning `CUI_COMPOSER_RECORD`, `CUI_COMPOSER_CANCEL` and `CUI_COMPOSER_VOICE`.
  - **Voice note** (`cui_voice_note`): a recorded message in a bubble with play and pause, its waveform as the seek bar, played and total time, and a 1x, 1.5x and 2x speed chip (`CUI_PLAYER_SPEED`). `.mine` tints it with the accent. The app plays the audio and passes the position.
  - **Voice session** (`cui_voice_session`): a spoken conversation with an assistant on one scrolling timeline, you in the foreground and the assistant in the accent, with a state label (Listening, Thinking, Speaking, Muted: `CUI_VOICE_MUTED` is new), a legend, the live transcript with the unspoken part dimmed, and Mute and End buttons.

### Changed

- **Renderers: a sixth shading mode**, `CUI_SHADE_DITHER`, for the dither components. A renderer of your own needs its few lines from the reference shader in [Rendering](docs/rendering.md) to draw them; everything else draws as before.
- **Line charts take `stacked`**: each series sits on the ones before it, and the axis covers their sum. It used to apply to bar charts only.
- **A calmer series palette.** The six chart colors keep their order (blue, orange, aqua, violet, yellow, magenta), so neighbours stay apart under color blindness, but now share one lightness and about one chroma in each mode. No series shouts over another or over the rest of the interface, and the light colors keep 4.2:1 or more on white.
- **Bar, donut and line charts** step back so the data leads: faint dashed gridlines over a solid zero line, smaller muted axis labels, slimmer bars with a small gap inside a group, and on hover a soft band behind the column with a compact chip of its values. The donut ring is thinner, its segments are parted by real gaps, and each legend row has a thin bar showing its share.
- **Heatmap and calendar heatmap** count in five steps from the empty surface up to the accent (a gray ramp under the neutral accent), so zero reads as empty and the steps read at a glance. Heatmap cells are smaller with real gaps, a Less to More key replaces the gradient scale, and hovering a cell rings it and lights its row and column labels.
- **Radial gauge** drops the needle and the tick ring for three calmer styles, with the value as the hero: a segmented arc lit in the band colors only up to the value, a clean arc with round caps, its ends labelled and a target marker, and a slim ring with a line of detail. New options: `style` (`CUI_GAUGE_AUTO` picks the segments when there are `bands`, the arc otherwise), `detail` and `color`, and a band `name` shown as a chip while the value is in that band. The default sweep is now 270 degrees.
- **Treemap and radar.** Treemap tiles are neutral, graded by size, with 3 point gaps, a label, a value and a share figure, and only the hovered or selected tile in the accent; small tiles keep a compact value. New `selected` option: a click selects a tile and a second click clears it. The radar has hairline rings, light fills under crisp outlines, points only on the series you point at (the last one otherwise) with a chip of the hovered value, and ring values kept off the lines.
- **Sankey** nodes are slim (6 points) and colored by meaning, with the new `node_colors` option and the palette or neutral gray for the rest. Flows fade from their source's color to their target's, hovering a node or a flow lights its whole path upstream and downstream, and labels sit beside the nodes as the name over the value and share, with no plates over the flows.
- **Gantt chart** bars are colored by state (done, active, planned) with their progress inside and a percentage at the end; planned bars are dashed and tinted. A new `state` field on `cui_gantt_task` sets it, and its zero value works it out from the progress and `today`. Dependencies are soft curves with small arrowheads, today is an accent line with a "Today" chip, weekends are faintly shaded, and a milestone is an outlined diamond with its name and date.
- **Uptime bars**: healthy days are a slightly softer green, so incidents stand out more.
- **Calendar and date picker**: the title opens the months of the year, and the year opens twelve years at a time, so a distant year takes a few clicks instead of a click per month. The arrows turn a month, a year or twelve years, the calendar keeps its size, and the picker opens on the days again ([#27](https://github.com/yappologistic/CUI/issues/27)).
- The voice orb (`cui_voice_orb`) is deprecated in favor of the voice components. It keeps working, but it has left the gallery and the docs site.

### Fixed

- The wheel scrolls a text area before its enclosing scroll view, then reaches the outer view at the text area's edges. Diagonal input can scroll the text vertically and the outer view horizontally ([#58](https://github.com/yappologistic/CUI/issues/58)).
- Tag input enforces its tag limit for every tag in a batch ([#58](https://github.com/yappologistic/CUI/issues/58)).
- Pasting a different complete one-time code emits another completion event ([#58](https://github.com/yappologistic/CUI/issues/58)).
- A select opens with Enter without also picking the highlighted item, and a searchable select returns focus to its trigger when its list closes, with animations on too, so Enter reopens it ([#58](https://github.com/yappologistic/CUI/issues/58)).
- Number fields reject nonfinite drafts and display the accepted value on Enter or focus loss ([#58](https://github.com/yappologistic/CUI/issues/58)).
- A confirmation popover opened with Enter waits for a separate activation.
- After Escape leaves a text field in a dialog, the next Escape can close the dialog.
- A disabled password field's reveal button ignores clicks and is skipped by Tab.
- Timed toasts expire on schedule after idle, and their actions stop accepting input once dismissal begins.
- The first tooltip waits for a full hover, and an idle window shows it at the requested wakeup.
- Each arrow press in a toggle group moves focus by one item.
- The range slider supports adjusting either thumb with the keyboard and shows which thumb has focus.
- Breadcrumb links and numbered page buttons show a focus ring when reached by keyboard.
- Pagination uses its parent's inner width to choose the compact layout, including in unnamed boxes.
- Steppers with repeated captions adapt independently to the width of their own rows.
- macOS: Quit from the app menu (Cmd-Q), the Dock or AppleScript ended the process from inside the event loop, so nothing after `cui_app_run` ran: no cleanup, no saving, and libraries that check their resources at exit could crash. It now ends `cui_app_run` (or makes `cui_app_frame` return false) as closing the window does, at once even when the app is idle ([#29](https://github.com/yappologistic/CUI/issues/29)).
- Nav items: when no item of a group was selected, the highlight stayed on the item selected before, and the next selection glided over from there. The highlight now goes away, and the next selection appears in place ([#35](https://github.com/yappologistic/CUI/issues/35)).
- **Text areas told the input method the whole area as the caret**, so the IME candidate window opened at the area's top-left corner or under its bottom edge. `set_text_input_rect` now gets the caret's line, one line tall, as in single-line fields and the code editor ([#56](https://github.com/yappologistic/CUI/issues/56)).
- **Windows: the IME opens at the caret.** The Win32 backend implements `set_text_input_rect`: it sets the IMM composition window to the caret, the candidate list under the line with the line excluded so it is never covered, and a hidden system caret for input methods, magnifiers and screen readers that follow it, in client pixels at the window's DPI. Before, Chinese, Japanese and Korean input methods opened their windows at the top-left of the screen. Committed text arrives once, as before, including characters outside the BMP ([#56](https://github.com/yappologistic/CUI/issues/56)).
- **Docs site: input methods work in the live previews.** Browsers place an IME only at a focused text input, and the previews are canvases. While a text field in a preview is being edited, the keyboard now goes to a hidden text area kept at the field's caret: the candidate window opens there, the text being composed shows in place, and the committed text goes to the field ([#56](https://github.com/yappologistic/CUI/issues/56)).
- **macOS: the IME opens at the caret.** The Cocoa view answered `firstRectForCharacterRange` with the view's top-left corner, so the candidate window opened there. It now returns the caret from `set_text_input_rect` and tells the input context when it moves ([#56](https://github.com/yappologistic/CUI/issues/56)).
- **SDL2: the IME candidate window follows the caret.** The SDL2 backend now implements `cui_platform_iface.set_text_input_rect` (`SDL_SetTextInputRect`) and asks for the system candidate list with `SDL_HINT_IME_SHOW_UI`. Before, the candidate window never moved with the caret ([#56](https://github.com/yappologistic/CUI/issues/56)).
- **An empty text field showed no caret.** At offset 0 the caret bar stands in the field's padding, left of the text, and the text's clip rectangle cut all of it away: an empty focused field looked dead and never blinked. The bar is now drawn against the field's clip, so it shows and blinks like anywhere else, with or without a placeholder (`tests/test_main.c` covers it).

## 2026-11-11 · 1.4: instruments

Five components that read like an instrument, a new example that puts them together, three guides and a reference of every function. Linux desktops can now match a window to its app, and fixes cover the single file after `windows.h` and the keyboard in three components.

### Added

- **Instruments**, five components for interfaces that read like an instrument: values as dots, labels in the monospaced font, hairlines and one accent. See [Instruments](docs/components-more.md#instruments) and the gallery's new *Instruments* page.
  - **Dot-matrix chart** (`cui_dot_chart`): values as a grid of dots, lit up to each column's value and dim above it. Series stack, an accent series stands out, the pointer or the arrow keys show a readout, and a click returns where it landed.
  - **Chart annotations** (`cui_chart_annotation`): vertical lines, dashed thresholds and labelled points over the dot chart, the line chart and the bar chart. Labels that would overlap move to the other side of their line, then down.
  - **Event track** (`cui_event_track`): moments on a time axis as dots you drag, add with a click and remove with a double-click, snapped to a step, with full keyboard control. It lines up with a dot chart above it.
  - **Readout** (`cui_readout`): a label, a value with its unit on the value's baseline, and a line of context.
  - **Dot-matrix display** (`cui_dot_display`): text in a 5 by 7 dot font that shrinks to fit or scrolls.
  - In a right-to-left interface the readout and the display's placement follow the reading direction, the chart and the track keep time running left to right, and Persian or Arabic labels are shaped with `CUI_TEXT_SHAPING`.
- `cui_axis_units`: `CUI_AXIS_HOURS` labels an axis as times of day, with 30 reading 06:00 the next morning.
- **Caffeine**, a new example ([`examples/caffeine.c`](examples/caffeine.c)) that puts the instruments on one screen: how much caffeine is still in you at bedtime, with cups you drag. It runs on the site and ships in the release downloads.
- **Three new guides.** [Building an app](docs/apps.md) covers the window, when frames happen, what components return, work on other threads and shipping. [Recipes](docs/recipes.md) has six complete programs: a form that validates, a table sorted by its headers, a list with a detail pane, a long task on another thread, a clock, and settings that are remembered. [Questions](docs/faq.md) says what CUI is and is not, how it compares with other libraries, and what it cannot do.
- **A function reference.** The site has a page that lists every public function of `cui.h`, `cui_app.h` and `cui_render.h` (496 of them) under the headers' own sections: its signature, the comment written beside it, and a link to the component page or guide that explains it. It has a filter, and site search finds a function by name.
- **Complete programs on component pages.** A page's usage block is a fragment. 71 of the 147 pages now also have the example as a whole program, with its includes, a frame function and `main`, under "Complete program". The site build compiles each one and leaves out any that does not build.
- `cui_app_quit`, `cui_app_set_title`, `cui_app_set_size`, `cui_app_size`, `cui_app_ui`, `cui_app_is_software` and `cui_app_native_window` take `NULL` for the app that is running, as `cui_app_file_dialog` already did. A `cui_app_run` callback had no way to close its window or change its title: now it is `cui_app_quit(NULL)`.
- `tests/doc_programs.sh`, run on every push: it builds every complete program in the docs with warnings as errors.
- `cui_app_desc.app_id` lets a Linux desktop show your app's name and icon for its window in the taskbar, the dock and Alt+Tab. Name your `.desktop` file after the ID (for example `org.example.Notes.desktop`). It sets `WM_CLASS` on X11, and the Wayland app ID with SDL2 or GLFW 3.4. Before this, every window of the X11 backend had the class `CUI`. See [Your app's name and icon on Linux](docs/getting-started.md#your-apps-name-and-icon-on-linux).

### Changed

- **Every component has a description.** Each section of the component docs now opens with one sentence that says what the component is and what it is for. The site shows it under the page title, on the cards that link to the page and in search results, where it used to show whatever sentence came first ("Weeks start on Monday." for the date picker).
- **Site search reads the whole site.** It now searches the text of the guides, the options and struct fields of every component and the changelog, and shows the words around a match. It used to look at titles and summaries only, so "install", "find_package" or the name of an option found nothing.
- **Every page has its own address for search engines.** The site was one page as far as a search engine could tell, so nobody searching for "C date picker" could land on the date picker. The build now writes a page per component and per guide (`c/date-picker-and-calendar/`, `guide/tutorial/`) with its text, title and description, plus `sitemap.xml` and `robots.txt`. Opened in a browser, such a page becomes the live site at that page.
- **Links to the site show a preview.** Every page carries a title, a description and an image for the cards that chat apps and social sites draw; a component's card shows the component.
- The site shows the version it documents beside the name.
- **Fuller component pages.** Sections that showed a snippet and a sentence now say what the call returns and when, which variables are yours and what the library writes into them, and the limits people run into, each checked against the code.
- **Every option is described.** All 1,277 fields of the options and data structs on the component pages now have a description: comments added in `cui.h` for the 395 that had none (checked against the code), and a shared description for fields that mean the same everywhere, such as `width` or `disabled`. A field whose type is an enum lists its values.
- **A new README.** It opens with what CUI is in plain words, says what it is not (it draws its own components and does not wrap the system's), compares it with the libraries people know, and lists the limitations in full, including the eight components that still need a pointer. It also says how the code is written and what tests it.
- A component page lists only its own functions, and no longer repeats its option names in a sentence above the options table.
- Release notes link to the files they mention with full addresses, so the links work from the release page.
- **The shared library's name carries the minor version** (`libcui.so.1.4` instead of `libcui.so.1`). Option and description structs gain fields in minor releases, and an app built against older headers passes the library a smaller struct than it reads. With the old name, an app built against 1.3 could load a newer library and read garbage past the end of `cui_app_desc` (the new `app_id`). Now each minor version is its own library, and an app keeps the one it was built against. Rebuild against the new headers to move to a new minor version. Static builds and the single-file version are not affected.

### Fixed

- The path bar's "..." menu had the same fault as the split button: after opening it from the keyboard, Enter on the highlighted segment closed the menu without choosing it. Its docs now list its keys.
- The split button's Enter key chooses the highlighted menu action when the chevron has keyboard focus. It previously closed the menu without choosing an action ([#25](https://github.com/yappologistic/CUI/issues/25)).
- The copy button keeps keyboard focus while it shows the confirmation check, so Enter or Space can copy again without pressing Tab ([#23](https://github.com/yappologistic/CUI/issues/23)).
- The single-file distribution failed to compile after `#include <windows.h>` with the Microsoft SDK, which defines `small` as a macro; two variables had that name ([#17](https://github.com/yappologistic/CUI/issues/17)). CI now compiles it with the macros a full `windows.h` defines.
- The values of an enum on a component page's "Data and results" section showed as `[object HTMLElement]` (17 pages).
- Eight component pages said things their component does not do: the icon button's icon swap (it animates on `cui_button`, not on `cui_icon_button`), a tooltip placed after a badge, a hover card on an avatar, Tab inside the time picker and the color picker, and the keys of the onboarding checklist, comments and JSON viewer. The pages now say what the code does, and the README lists the time picker and color picker among the parts that still need a pointer.

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
