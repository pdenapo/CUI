# Components

Every component, what it is for, how to use it and what it does with the keyboard. Screenshots are rendered by `tests/component_shots.c`, so they always match the code, and they follow your light or dark theme.

Looking for split buttons, data grids, kanban boards, more charts, motion effects or AI interface pieces? They are in [More components](components-more.md).

Contents:
[Text](#text) ·
[Button](#button) ·
[Icon button](#icon-button) ·
[Segmented control](#segmented-control) ·
[Checkbox, switch, radio](#checkbox-switch-and-radio) ·
[Slider](#slider) ·
[Chips and rating](#chips-and-rating) ·
[Text field](#text-field) ·
[Text area](#text-area) ·
[Select and combobox](#select-and-combobox) ·
[Date picker and calendar](#date-picker-and-calendar) ·
[Number field](#number-field) ·
[Color picker](#color-picker) ·
[Tabs](#tabs) ·
[Sidebar navigation](#sidebar-navigation) ·
[Breadcrumb, pagination, stepper](#breadcrumb-pagination-and-stepper) ·
[Dropdown and context menu](#dropdown-and-context-menu) ·
[Popover](#popover) ·
[Tooltip](#tooltip) ·
[Dialog](#dialog) ·
[Sheet](#sheet) ·
[Toast](#toast) ·
[Command palette](#command-palette) ·
[Alert](#alert) ·
[Progress, meter, spinner, skeleton](#progress-meter-spinner-and-skeleton) ·
[Badge, avatar, keycap](#badge-avatar-and-keycap) ·
[Card and metric](#card-and-metric) ·
[Empty state](#empty-state) ·
[Table](#table) ·
[List and tree](#list-and-tree) ·
[Accordion](#accordion) ·
[Charts](#charts) ·
[Image](#image) ·
[Radio cards](#radio-cards) ·
[Tag input](#tag-input) ·
[One time code](#one-time-code) ·
[Password strength](#password-strength) ·
[Button group, copy, hold](#button-group-copy-and-hold-to-confirm) ·
[Timeline and progress ring](#timeline-and-progress-ring) ·
[Reorderable list](#reorderable-list)

---

## Text

Headings, body text, labels, links and inline code, with wrapping that avoids a lone last word.

```c
cui_heading(ui, "Settings", 1);          /* 1..4: 36, 28, 22, 17 pt; Geist for 28 and up */
cui_text(ui, "Body text wraps to the available width.");
cui_secondary(ui, "Supporting text in the secondary color.");
cui_muted(ui, "Small, muted text.");
cui_label(ui, "Medium weight label");
cui_textf(ui, "%d unread", unread);
cui_code(ui, "cui_button()");
if (cui_link(ui, "Read the docs")) open_docs();
cui_icon(ui, CUI_ICON_ZAP, 16, CUI_COLOR_ACCENT);
```

`cui_text` options: `font` (`CUI_FONT_BODY`, `MEDIUM`, `DISPLAY`, `MONO`), `size`, `color`, `align`, `nowrap` (one line with an ellipsis), `tabular` (aligned digits), `width`, `line_height`.

Wrapped text breaks like a careful typesetter: when a paragraph would end on a single word under a full line, the lines are rebalanced so at least two words share the last line. The number of lines never changes, so layout does not move. A `nowrap` line that is cut short with an ellipsis shows its whole text in a tooltip when the pointer rests on it; the text stays non-interactive, so the row or card around it keeps its clicks.

Hierarchy comes from size and color, not weight: CUI uses regular and medium only.

Keyboard: text takes no focus. A link takes Tab, and Enter or Space follows it.

## Button

Starts an action when clicked. It comes in six variants and three sizes, with an optional icon and a loading state.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/buttons-dark.png"><img src="images/components/buttons.png" alt="Buttons"></picture>

```c
if (cui_button(ui, "Save", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_CHECK)) save();
cui_button(ui, "Cancel");
cui_button(ui, "Saving", .variant = CUI_BUTTON_PRIMARY, .loading = true);
```

| Option | Values |
|---|---|
| `variant` | `CUI_BUTTON_SECONDARY` (default), `PRIMARY`, `OUTLINE`, `GHOST`, `DANGER`, `LINK` |
| `size` | `CUI_SIZE_SM` (30), `CUI_SIZE_MD` (36, default), `CUI_SIZE_LG` (44) |
| `icon`, `icon_right` | Any `CUI_ICON_*` |
| `loading` | Shows a spinner, ignores clicks, keeps the label so nothing jumps |
| `disabled`, `selected`, `full_width`, `width` | |
| `color` | Text and icon color. Pair it with `selected` for toggles: `.selected = liked, .color = liked ? CUI_COLOR_ACCENT : CUI_TRANSPARENT` |
| `tooltip`, `shortcut` | Tooltip text and a shortcut hint |

Use one primary button per view. Presses scale to 0.97 on a snappy spring. Keyboard: Tab to focus, Enter or Space to press.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/button-states-dark.png"><img src="images/components/button-states.png" alt="Every button variant at rest, under the pointer, pressed, focused from the keyboard and disabled"></picture>

Every state, rendered by `tests/component_shots.c`. The focus ring only appears for keyboard focus, never after a click.

## Icon button

A round button with only an icon, ghost by default.

```c
if (cui_icon_button(ui, CUI_ICON_TRASH_2, .tooltip = "Delete", .shortcut = "Del")) remove_item();
```

Always give it a `tooltip`: it is the button's name.

Returns true on the frame it is clicked, and never while `disabled` or `loading`. `shortcut` is only a hint shown after the tooltip: it does not bind the key, so check the key yourself with `cui_shortcut`. The button's identity comes from its tooltip and its icon. Two of them with the same icon and tooltip in one scope, such as a delete button on every row of a list, need an id scope each: `cui_push_id_int(ui, row)` before the row and `cui_pop_id(ui)` after it.

A `cui_button` whose `.icon` changes (copy to check, play to pause) shrinks the old icon away while the new one scales in. An icon button is told apart by its tooltip and its icon together, so changing the icon of a `cui_icon_button` makes it a new button: the swap is not animated and keyboard focus does not stay on it.

Keyboard: Tab focuses the button, and Enter or Space presses it.

## Segmented control

A compact set of two to five choices where one is always selected, for switching between views of the same content.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/segmented-dark.png"><img src="images/components/segmented.png" alt="Segmented control"></picture>

```c
static int view = 0;
const char *items[] = { "List", "Board", "Timeline" };
const char *icons[] = { CUI_ICON_LIST, CUI_ICON_LAYOUT_DASHBOARD, CUI_ICON_HISTORY };
cui_segmented(ui, "view", items, 3, &view, .icons = icons);
```

The highlight's two edges have their own springs: the leading one runs ahead and the trailing one follows, so it stretches toward the new option and draws in as it lands. Options: `size`, `full_width`, `icons`. Keyboard: arrows move the selection.

Returns true on the frame the selection changes. `view` is your `int`, the index of the selected option. The control writes it on a click and keeps it between 0 and `count` - 1; set it from code and the highlight glides there. `items` and `icons` are your arrays, read every frame, and `icons` needs an entry or `NULL` for each option. For pages of different content use tabs, and for options that can each be on or off use a toggle group.

## Checkbox, switch and radio

The three basic choices: a checkbox for an option, a switch for a setting that applies at once, and radio buttons for one of several.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/toggles-dark.png"><img src="images/components/toggles.png" alt="Toggles"></picture>

```c
cui_checkbox(ui, "Email notifications", &email, .description = "Product news, monthly.");
cui_switch(ui, "Wi-Fi", &wifi);
cui_radio(ui, "Monthly", &billing, 0);
cui_radio(ui, "Yearly", &billing, 1);
cui_radio_group(ui, "autosave", (const char *[]){ "Every change", "Every 5 minutes", "Only when I save" }, 3, &autosave);
```

Each returns true when it changes the value. The whole row, label included, is clickable. A press dips the box; checking overshoots a touch and settles, while clearing and keyboard toggles stay plain. The switch thumb stretches with its speed in flight, keeping its volume, and rounds out as it lands. A switch applies immediately; a checkbox is for choices confirmed later with a button. Options: `description`, `disabled`, `indeterminate` (checkbox). Keyboard: Space toggles.

## Slider

Picks a number, or a range with two thumbs, by dragging along a track.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/slider-dark.png"><img src="images/components/slider.png" alt="Sliders"></picture>

```c
cui_slider(ui, "Volume", &volume, 0, 100, .show_value = true);
cui_slider(ui, "Brightness", &brightness, 0, 100, .step = 10, .show_value = true, .format = "%.0f%%");
cui_range_slider(ui, "Price", &lo, &hi, 0, 500);
```

Drag past either end and the track stretches like rubber, then springs back on release. Clicking the track glides the thumb to the spot. Options: `step`, `show_value`, `format`, `disabled`, `width`. Keyboard: arrows step (Shift for 10 steps), Home and End.

Both return true on a frame the value changes. `volume` is your `float`. The slider writes it while it is dragged and keeps it between the minimum and the maximum, so a value set outside the range is pulled back in. `format` is a `printf` format for one `double` and defaults to "%.0f". The label is also the identity: two sliders with the same label in one scope need a key, as in `"Volume##left"`. `cui_range_slider` takes two floats and no options. Its thumbs cannot cross.

Tab focuses the range slider. Space switches the focused thumb. An arrow moves it by one percent of the range, or ten percent with Shift. Home and End move it as far as it can go without crossing the other thumb. When the thumbs overlap, a chevron points toward the selected end of the track.

## Chips and rating

A chip is a small toggle for filters and tags. A rating picks a number of stars.

![Chips and rating](images/components/chips.png)

```c
static bool design;
static int stars = 3;
cui_chip(ui, "Design", &design);           /* toggle chip */
cui_rating(ui, "stars", &stars, 5);         /* click the current value again to clear */
```

`cui_chip` returns true on the frame it is toggled. Its second argument points at your `bool`, which the chip flips; with `NULL` the chip only displays, unselected. The label is also the identity, so two chips with the same label in one scope need a key: `"Design##team"` shows "Design". A chip is as wide as its label, so put a set of them in a row that wraps: `cui_row(ui, .gap = 8, .wrap = true)`.

`cui_rating` returns true on the frame a star is clicked. `stars` is your `int`, from 0 for no rating to the number of stars, which is the last argument: 0 or less gives 5, and 16 is the most. Ratings are whole stars.

Keyboard: Tab moves through the chips, and Enter or Space toggles the focused one. The rating is set with the pointer; it has no keys yet.

## Text field

A single-line field that edits text in a buffer you own, with a label, an icon, a hint and an error. The search field is a variant.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/inputs-dark.png"><img src="images/components/inputs.png" alt="Text fields"></picture>

```c
static char email[128];
int r = cui_input(ui, "email", email, sizeof email, .label = "Email", .icon = CUI_ICON_MAIL,
                  .placeholder = "you@company.com", .hint = "Used to sign in and for receipts.");
if (r & CUI_CHANGED) validate();
if (r & CUI_SUBMITTED) submit();          /* Enter */

cui_input(ui, "password", pw, sizeof pw, .label = "Password", .password = true,
          .error = strlen(pw) < 8 ? "Use 8 or more characters." : NULL);
cui_search(ui, "q", query, sizeof query, .placeholder = "Search projects");
```

You own the buffer; CUI never writes past `cap`. Returns flags: `CUI_CHANGED`, `CUI_SUBMITTED`, `CUI_FOCUSED`, `CUI_BLURRED`.

| Option | |
|---|---|
| `label`, `placeholder`, `hint`, `error` | |
| `icon` | Leading icon |
| `password` | Masks the text and adds a show/hide button that honors `disabled` |
| `clearable` | Adds a clear button when not empty (always on for `cui_search`). Hidden for `read_only` fields; `Ctrl+Z` restores the cleared text |
| `autofocus`, `read_only`, `disabled`, `size`, `width` | |

Editing: click to place the caret, drag or Shift+arrows to select, double click selects a word, triple click selects everything. Ctrl (Option on macOS) with arrows or Backspace moves or deletes by word. Home/End, Ctrl+A, cut, copy, paste, undo and redo (Ctrl+Z, Ctrl+Shift+Z or Ctrl+Y; Cmd on macOS). Esc clears the selection, then leaves the field. Full UTF-8.

## Text area

A multi-line text field that wraps and scrolls.

![Text area](images/components/textarea.png)

```c
static char bio[512], body[4096];
cui_textarea(ui, "bio", bio, sizeof bio, .label = "Bio", .height = 120);
cui_textarea(ui, "body", body, sizeof body, .height = -1);   /* fill the available height */
```

Word wraps, scrolls with the wheel, Up and Down move between visual lines. Enter inserts a newline; Ctrl+Enter submits.

Returns the same flags as the text field: `CUI_CHANGED` on a frame the text changes, `CUI_SUBMITTED` when it is submitted, and `CUI_FOCUSED` and `CUI_BLURRED` on the frames it gains and loses the focus. `bio` is your buffer and the next argument its size in bytes, the terminating zero included. The library edits it in place, never writes past it, and drops what does not fit without cutting a UTF-8 character in half. Lines are separated by `\n`. `height` is in points and defaults to 96, and a negative value fills the height left in the container. It takes the options of the text field. For source code, use the code editor.

## Select and combobox

Picks one item from a list that opens under the field. With search it becomes a combobox.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/select-dark.png"><img src="images/components/select.png" alt="Select"></picture>

```c
static int country = 2;
const char *items[] = { "Canada", "Denmark", "Germany", "Japan", "Kenya" };
cui_select(ui, "country", items, 5, &country, .label = "Country");
cui_select(ui, "lang", languages, n, &lang, .searchable = true, .placeholder = "Pick a language");
```

`searchable` adds a filter field at the top of the list (a combobox). Options: `label`, `placeholder`, `icons`, `searchable`, `disabled`, `width`. Keyboard: Up/Down open the list and move the highlight, Enter chooses, Esc closes.

Returns true on the frame the choice changes. `country` is your `int`, the index of the chosen item. A value outside 0 to `count` - 1, such as -1, means nothing is chosen, and the field then shows `placeholder`, or "Select". Choosing an item closes the list, which scrolls once it is taller than 300 points. The search keeps the items that contain the typed text, whatever the case. For several choices use the multi-select, and for two to five options that should stay visible use a segmented control.

## Date picker and calendar

Picks a date from a month grid, as a field that opens a calendar or as a calendar shown inline.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/calendar-dark.png"><img src="images/components/calendar.png" alt="Calendar"></picture>

```c
static cui_date due = { 2026, 11, 14 };
cui_date_picker(ui, "due", &due, .label = "Due date");
cui_calendar(ui, "cal", &due);             /* inline calendar */
cui_date today = cui_date_today();
```

Weeks start on Monday. Today has a ring, the selected day is filled.

Both return true on the frame a day is clicked. `due` is your `cui_date`: a year, a month from 1 to 12 and a day. A year of 0 means no date: the picker then shows `placeholder`, or "Pick a date", and the calendar opens on the current month. The picker writes the date and closes. The library keeps only the month in view, under `id`. It starts at the month of the value and after that follows the month buttons, so changing the value from code does not turn the page. `cui_date_today` takes no context and reads the local date from the system clock. For a start and an end, use the date range picker.

Keyboard: Enter or Space on the field opens the calendar and Esc closes it. Inside, Tab moves through the month buttons and the days, and Enter picks the focused day.

## Number field

A field for a number in a range, typed or stepped with its buttons and the arrow keys.

![Pickers](images/components/pickers.png)

```c
static double seats = 4, price = 19.99;
cui_number(ui, "seats", &seats, 1, 100, .label = "Seats", .step = 1);
cui_number(ui, "price", &price, 0, 1e6, .decimals = 2, .suffix = "USD");
```

Type a value, use the minus and plus buttons, or Up/Down (Shift for 10 steps). Values are clamped to the range.

Returns true on a frame the value changes, which while typing is every keystroke that reads as a different number. `seats` is your `double`. The field shows a value you set from code as soon as it is not focused, and pulls a value outside the range back in on every frame. `step` is what the buttons and the keys add or take away, 1 when not set. `decimals` is how many places are shown, 0 when not set. It does not round the value: 2.5 typed into a field with no decimals stays 2.5 in your variable.

## Color picker

A swatch that opens a saturation and brightness square, a hue bar and preset swatches.

```c
static cui_color brand = CUI_RGB(0x7747ff);
cui_color_picker(ui, "brand", &brand);
```

Returns true on a frame the color changes, which during a drag is every frame the pointer moves it. `brand` is your `cui_color`, four bytes of red, green, blue and alpha, and `CUI_RGB(0x7747ff)` builds an opaque one from a hex number. The field shows a swatch and the color as `#RRGGBB`. The panel that opens is 240 points wide and holds the square for saturation and brightness, the hue bar and eight preset swatches. There is no alpha control and no field to type a hex value into, and a color picked here is always opaque. A color you change from code shows in the swatch at once, and the panel picks it up the next time it opens.

Keyboard: Enter or Space on the swatch opens the picker and Esc closes it. Inside, the color is picked with the pointer: the square, the hue bar and the presets take no keyboard focus yet.

## Tabs

Switches between views with a row of labels and an underline that slides to the active one.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/tabs-dark.png"><img src="images/components/tabs.png" alt="Tabs"></picture>

```c
static int tab = 0;
const char *tabs[] = { "Overview", "Analytics", "Reports" };
cui_tabs(ui, "tabs", tabs, 3, &tab);
cui_tabs(ui, "account", items, 3, &sel, .variant = CUI_TABS_PILL, .icons = icons);
switch (tab) { /* content */ }
```

The underline slides to the active tab. Keyboard: Left and Right. Tabs never truncate their labels: when they are wider than their container the strip scrolls sideways, fades at the edge that has more, and brings a newly selected tab into view.

Returns true on the frame the selected tab changes. `tab` is your `int`: the tabs write it on a click and keep it between 0 and `count` - 1. Tabs draw only the strip, so showing the content of the selected tab is your `switch` below it. Set the index from code and the strip scrolls that tab into view. At most 32 tabs are shown. Options: `variant` (`CUI_TABS_UNDERLINE`, the default, or `CUI_TABS_PILL`), `icons` and `full_width`. For a compact choice between views of the same content, use a segmented control.

## Sidebar navigation

The items and section titles of an app's sidebar, with one highlight that follows the selection.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/sidebar-dark.png"><img src="images/components/sidebar.png" alt="Sidebar"></picture>

```c
cui_nav_section(ui, "Workspace");
if (cui_nav_item(ui, "Inbox", .icon = CUI_ICON_INBOX, .badge = "12", .selected = page == 0)) page = 0;
if (cui_nav_item(ui, "Projects", .icon = CUI_ICON_FOLDER, .selected = page == 1)) page = 1;
```

All items of a `group` share one highlight that glides to the selected item.

`cui_nav_item` returns true on the frame it is clicked. It does not remember the selection: you keep the current page, as `page` does above, and pass `.selected` on every frame. The label is also the identity, so two items with the same label in one scope need a key: `"Settings##team"` shows "Settings". `badge` is text, so format a count yourself. Items without a `group` share one common group; name the groups when a window has two separate lists, so each keeps its own highlight. An item fills the width of its container. `cui_nav_section` draws a title above a set of items and returns nothing.

Keyboard: Tab moves through the items, and Enter or Space selects the focused one.

## Breadcrumb, pagination and stepper

Three ways to show position: the path to a page, the page of a list, and the step of a process.

![Navigation](images/components/navigation.png)

```c
static int page = 0;
const char *steps[] = { "Account", "Plan", "Payment", "Review" };

int clicked = cui_breadcrumb(ui, (const char *[]){ "Home", "Projects", "CUI" }, 3);   /* index or -1 */
cui_pagination(ui, "pages", &page, page_count);                                        /* page is 0 based */
cui_stepper(ui, steps, 4, current);
```

`cui_breadcrumb` returns the index of the item that was clicked, or -1. The last item is the current page: it is not a link and is never returned. Each item is told apart by its text, so two levels with the same name need a key, as in `"Docs##2"`.

`cui_pagination` returns true on the frame the page changes. `page` is your `int`, counted from 0 while the buttons show numbers from 1, and it is kept between 0 and `page_count` - 1. With seven pages or fewer, all are shown. Beyond that the first, the last and the pages around the current one stay, with an ellipsis for each gap. A narrow parent leaves room for just the first, current and last page.

`cui_stepper` returns nothing. `current` is the index of the step in progress, from 0: the steps before it are marked done, and passing the number of steps marks them all done. Each stepper adapts to the width of its own row. In a narrow row only the current step keeps its label.

Tab moves through the breadcrumb links and page buttons, with a ring around the focused control. Enter or Space activates it. The stepper only displays.

## Dropdown and context menu

A menu of commands that opens from a button, or at the pointer on a right click.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/menu-dark.png"><img src="images/components/menu.png" alt="Menu"></picture>

```c
cui_menu(ui, "Options", .icon = CUI_ICON_SETTINGS) {
    cui_menu_label(ui, "Project");
    if (cui_menu_item(ui, "Rename", .icon = CUI_ICON_PENCIL, .shortcut = "F2")) rename();
    cui_menu_separator(ui);
    if (cui_menu_item(ui, "Delete", .icon = CUI_ICON_TRASH_2, .danger = true)) confirm = true;
}

cui_card(ui, .id = "file", .hoverable = true) { ... }
cui_context_menu(ui, "file-menu") {                   /* right click on the previous component */
    cui_menu_item(ui, "Copy", .icon = CUI_ICON_COPY);
}
```

Menus open with a short scale and fade from the trigger, close on a click outside, Esc or after choosing. Item options: `icon`, `shortcut`, `danger`, `disabled`, `checked`. Menu options: `icon`, `variant`, `placement`, `width`, `icon_only`. Keyboard: Down opens from the focused trigger, Up/Down move, Enter chooses.

## Popover

A small floating panel anchored to a control, for content that does not need a dialog.

```c
if (cui_button(ui, "Filters", .icon = CUI_ICON_FILTER)) cui_popover_toggle(ui, "filters");
cui_popover(ui, "filters", .width = 280) {
    cui_checkbox(ui, "Only active", &only_active);
    if (cui_button(ui, "Apply", .variant = CUI_BUTTON_PRIMARY)) cui_popover_close(ui, "filters");
}
```

Anchored to the component before it, flips to stay on screen. Options: `placement`, `width`, `pad`, `gap`.

The block runs only while the popover is on screen. You keep no state: the library stores whether it is open under `id`, and `cui_popover_toggle`, `cui_popover_open` and `cui_popover_close` change that from anywhere in the same id scope, while `cui_popover_is_open` reads it. Call `cui_popover` on every frame, right after the control it should hang from, not inside that control's `if`. A press outside closes it. `width` is in points; without it the panel is as wide as its content and at least as wide as its anchor. Use a dialog when the rest of the window must wait, and a dropdown menu for a list of commands.

Keyboard: Enter or Space on the trigger opens the popover. Tab moves through what is inside, and Esc closes it.

## Tooltip

A short label that appears when the pointer rests on a control.

![Tooltip](images/components/tooltip.png)

```c
cui_icon_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .shortcut = "Ctrl B");   /* buttons take .tooltip */
cui_link(ui, "What is beta?");
cui_tooltip(ui, "Features in beta may change");   /* for the previous component */
```

Appears after a short delay. Moving on to a neighbour while it is open, it glides there and changes its text instead of popping again.

`cui_tooltip` returns nothing and takes no `id`: call it on every frame, right after the component it belongs to. That component must be one that reacts to the pointer, such as a button, a link or a hoverable card. Plain text, a badge and an avatar do not, so a tooltip placed after one of them attaches to whatever interactive component came before. The delay is 0.55 seconds, and none when another tooltip was showing less than 0.4 seconds ago. The text is copied, up to 255 bytes. For a card with its own layout, use a hover card.

The hover delay is measured in elapsed time, including when the app sleeps between frames.

Keyboard: a tooltip takes no focus. It belongs to the control before it, which keeps its own keys.

## Dialog

A modal window for a decision or a short form, which blocks the rest of the app until it is closed.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/dialog-dark.png"><img src="images/components/dialog.png" alt="Dialog"></picture>

```c
cui_dialog(ui, "Edit profile", &open, .description = "Changes are visible to your team.") {
    cui_input(ui, "name", name, sizeof name, .label = "Name");
    cui_dialog_footer(ui) {
        if (cui_button(ui, "Cancel")) open = false;
        if (cui_button(ui, "Save", .variant = CUI_BUTTON_PRIMARY)) { save(); open = false; }
    }
}
```

Modal: the scrim blocks everything behind it. Esc, the close button or a click on the scrim close it, unless `persistent`. Options: `description`, `width` (440), `no_close_button`, `persistent`. Opens with a fade and a scale from 0.96, closes faster than it opens.

Escape first clears a text selection or leaves the focused editor. Once focus leaves the editor, the next Escape can close the dialog.

## Sheet

A panel that slides in from an edge.

```c
static bool open;
if (cui_button(ui, "Notifications")) open = true;
cui_sheet(ui, "Notifications", &open, .side = CUI_SIDE_RIGHT, .size = 400) { ... }
```

Content scrolls.

`open` is your `bool`: the sheet shows while it is true, and the close button, a click on the dimmed window behind and the close key set it to false. Call `cui_sheet` on every frame, not inside the `if` that opens it. The block runs while the sheet is on screen, which includes the slide out after `open` turns false. `side` is `CUI_SIDE_RIGHT` (the default), `CUI_SIDE_LEFT`, `CUI_SIDE_BOTTOM` or `CUI_SIDE_TOP`. `size` is the width in points for a left or right sheet and the height for a top or bottom one; it defaults to 400. `description` adds a line under the title, and the title is also the sheet's identity. Use a sheet for detail beside the page, and a dialog for a short decision.

Keyboard: Tab stays inside the open sheet, and Esc closes it.

## Toast

A short message that appears in a corner and goes away by itself, with an optional action.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/toast-dark.png"><img src="images/components/toast.png" alt="Toasts"></picture>

```c
cui_toast(ui, "Changes saved", .tone = CUI_TONE_SUCCESS);
cui_toast(ui, "Upload failed", .description = "The file is larger than 25 MB.", .tone = CUI_TONE_DANGER);
int t = cui_toast(ui, "Message archived", .action = "Undo");
if (cui_toast_action_clicked(ui, t)) undo();
```

Toasts stack in the bottom right corner. Older toasts tuck behind the newest one; hovering expands the stack and pauses the timers. Drag a toast right to dismiss it: a fling or a long drag carries its speed off the screen, a short one springs back, and dragging left meets rubbery resistance. Options: `description`, `tone`, `duration` (seconds, default 4, negative to keep), `action`. Use toasts for background events; for the result of a button press, change the button instead.

Tab reaches a visible toast's action button, and Enter or Space activates it until dismissal begins. Toasts never take focus by themselves.

## Command palette

A search box over the app's commands, opened with a shortcut.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/command-palette-dark.png"><img src="images/components/command-palette.png" alt="Command palette"></picture>

```c
static const cui_command commands[] = {
    { "New project", CUI_ICON_PLUS, "Actions", "Ctrl N" },
    { "Toggle dark mode", CUI_ICON_MOON, "Preferences", NULL },
};
if (cui_shortcut(ui, CUI_MOD_PRIMARY, CUI_KEY_K)) palette = true;
int chosen = cui_command_palette(ui, &palette, commands, 2);   /* index or -1 */
```

Fuzzy search with ranking, grouped results, Up/Down to move, Enter to run, Esc to close.

Returns the index of the chosen command in your array, on the frame it is chosen, and -1 otherwise. `palette` is your `bool`: the palette shows while it is true, and choosing a command, a click outside and the close key set it to false. Each `cui_command` is a title, an icon, a group and a shortcut, and any but the title may be `NULL`. Group headings show while the search is empty, one above each run of commands with the same group, so keep those commands next to each other in the array. The shortcut is only shown as a hint: binding it is `cui_shortcut` in your own code.

## Alert

An inline message in a tone of information, success, warning or danger, which can be dismissed.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/alerts-dark.png"><img src="images/components/alerts.png" alt="Alerts"></picture>

```c
cui_alert(ui, "Payment received", .description = "Paid on November 5.", .tone = CUI_TONE_SUCCESS);
cui_alert(ui, "Storage almost full", .tone = CUI_TONE_WARNING, .dismissible = true);
```

Tones: `CUI_TONE_NEUTRAL` (info), `SUCCESS`, `WARNING`, `DANGER`. Returns false once dismissed.

Returns true while the alert is shown, so `if (!cui_alert(...))` tells you it is gone. The library remembers the dismissal itself, under the alert's title, and a dismissed alert draws nothing from then on. To show the same text again later, give it a new key: `"Storage almost full##2"` shows the same title. An alert without `dismissible` always returns true, and whether to show it at all is your own `if`. Each tone brings its own icon, which `icon` replaces. `description` is a second, wrapping line under the title. For news about the whole view use a banner, and for something that just happened use a toast.

Keyboard: the alert only displays. Its dismiss button, when it has one, takes Tab, and Enter or Space presses it.

## Progress, meter, spinner and skeleton

Ways to show waiting and amounts: a progress bar, a meter against a limit, a spinner, a gauge and placeholder shapes for content that is loading.

![Feedback](images/components/feedback.png)

```c
cui_progress(ui, 0.68f, .label = "Uploading", .show_value = true);
cui_progress(ui, 0, .indeterminate = true);
cui_meter(ui, "Storage", 18.4f, 20, "GB");     /* turns amber and red near the limit */
cui_spinner(ui, 20);
cui_skeleton(ui, CUI_PCT(60), 12);             /* shimmering placeholder */
cui_gauge(ui, "CPU", 0.42f, 130);
```

None of these returns anything or takes the pointer. `cui_progress` takes a fraction from 0 to 1, clamped, and glides to a new value. With `indeterminate` the value is ignored and a short segment travels along the track. Options: `label` and `show_value`, which add a row above the bar with the name and a whole percentage, `tone`, `height` in points (6) and `width`. `cui_meter` takes an amount, a limit and a unit as text, prints them as "18.4 / 20 GB", and turns amber above 75% and red above 90% of the limit. `cui_spinner` takes its size in points, and 0 means 20. `cui_skeleton` takes a width and a height in points (14 when 0). `cui_gauge` takes a label, a fraction from 0 to 1 and a size in points (140 when 0).

## Badge, avatar and keycap

Small labels: a badge for a status or a count, an avatar for a person, and a keycap for a keyboard key.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/badges-dark.png"><img src="images/components/badges.png" alt="Badges and avatars"></picture>

```c
cui_badge(ui, "Active", .tone = CUI_TONE_SUCCESS, .dot = true);
cui_badge(ui, "New", .tone = CUI_TONE_INFO, .solid = true);
cui_avatar(ui, "Jonas Berg", .size = 40, .status = "online");   /* online, away, busy, offline */
cui_avatar(ui, "Maya", .image = texture);
cui_avatar_group(ui, names, count, 4, 32);
cui_kbd(ui, "Ctrl Shift P");
```

Avatars without an image show initials on a color picked from the name, so the same person always gets the same color.

None of these returns anything, keeps state or reacts to the pointer, so a tooltip or a hover card cannot hang from one directly. Badge options: `tone` (any `CUI_TONE_*`, neutral when not set), `dot`, `solid`, `icon` and `size`. `cui_avatar` takes the person's name, which gives the initials and the color. Options: `size`, the diameter in points (32), `image`, a `cui_texture` that replaces the initials, `status` and `square`. `status` is "online", "busy" or "away", and any other word gives a gray dot. `cui_avatar_group` takes an array of names, how many there are, how many to show before a "+N" (4 when 0) and the size (32 when 0). `cui_kbd` takes key names separated by spaces and draws one keycap for each.

## Card and metric

A card is a surface that groups content. A metric shows one number with its change and an optional sparkline.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/cards-dark.png"><img src="images/components/cards.png" alt="Cards"></picture>

```c
cui_card(ui, .gap = 12) { ... }
if (cui_card(ui, .id = "project", .hoverable = true)) { ... }   /* lifts on hover, returns clicks */
cui_metric(ui, "Weekly installs", "3,912", .delta = 8.1f, .spark = values, .spark_count = 7, .icon = CUI_ICON_DOWNLOAD);
cui_metric(ui, "Median build", "3m 42s", .delta = -17.6f, .lower_is_better = true);   /* a fall, in green */
```

The arrow follows the sign of `delta`; the color says whether that is good. Set `lower_is_better` for times, errors and costs.

Cards rest on a hairline border with no shadow. Card options: `pad` (24), `pad_x`, `pad_y`, `gap`, `dir`, `align`, `width`, `height`, `min_width`, `max_width`, `wrap` (row cards), `hoverable`, `muted`, `selected`. `.width = CUI_FILL, .max_width = 520` fills a phone and stops at 520 on a wide window.

Keyboard: a `hoverable` card takes Tab, and Enter or Space clicks it. Other cards and metrics only display.

## Empty state

What to show where there is nothing yet: an icon, a title, a line of help and an action.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/empty-state-dark.png"><img src="images/components/empty-state.png" alt="Empty state"></picture>

```c
if (cui_empty_state(ui, "No projects yet", .icon = CUI_ICON_FOLDER_OPEN,
                    .description = "A project holds your screens, their builds and who can see them.", .action = "New project"))
    create_project();
```

Returns true on the frame its action button is clicked, and always false when there is no `action`. It keeps no state and decides nothing: show it from your own `if`, when the list or the search result is empty. The title is the only required text. `icon` is drawn large and muted above it, `description` wraps under it, and `action` is the label of the button. The block fills the width of its container and centers its content, so give it the area the missing content would fill.

Keyboard: the empty state only displays. Its action button takes Tab, and Enter or Space presses it.

## Table

Rows and columns of your data, with sortable headers, selectable rows and any component in a cell.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/table-dark.png"><img src="images/components/table.png" alt="Table"></picture>

```c
cui_table_column cols[] = {
    { "Name", CUI_FILL, CUI_TEXT_LEFT, true },          /* title, width, align, sortable */
    { "Status", CUI_PX(120), CUI_TEXT_LEFT, false },
    { "Amount", CUI_PX(110), CUI_TEXT_RIGHT, true },
};
static cui_table_sort sort;
cui_table(ui, "orders", cols, 3, .sort = &sort, .selectable = true, .height = CUI_PX(400)) {
    for (int i = 0; i < n; i++) {
        if (cui_table_row(ui, selected == i)) selected = i;
        cui_table_cell(ui);
        cui_avatar(ui, rows[i].name, .size = 24);
        cui_text(ui, rows[i].name, .nowrap = true);
        cui_table_cell(ui);
        cui_badge(ui, rows[i].status, .tone = CUI_TONE_SUCCESS, .dot = true);
        cui_table_cell_textf(ui, "$%.2f", rows[i].amount);
    }
}
```

Cells hold any components. Clicking a sortable header updates `sort` (column and direction); you sort your data. With `height` the body scrolls under a fixed header. When the container is narrower than the fixed columns plus 96 points for each flexible one, the table scrolls sideways instead of squeezing cells. Options: `sort`, `height`, `striped`, `compact`, `selectable`.

Keyboard: Tab reaches the headers of sortable columns, and Enter or Space sorts by the focused one. Rows are selected with the pointer; for arrow keys through rows use the [data grid](#data-grid-and-filter-bar).

## List and tree

List rows with an avatar or an icon, a title and a detail line, and a tree of items that fold.

![Tree and list](images/components/tree-list.png)

```c
if (cui_list_item(ui, "Design review", .avatar = "Jonas Berg", .subtitle = "Tomorrow at 10:00", .trailing = "45m")) open();

static bool src_open = true;
if (cui_tree_begin(ui, "src", &src_open, NULL)) {
    cui_tree_begin(ui, "main.c", NULL, &(cui_tree_opts){ .leaf = true, .selected = true });
    if (cui_tree_clicked(ui)) open_file("main.c");
    cui_tree_end(ui);
}
```

Tree keyboard: Right opens, Left closes.

`cui_list_item` returns true on the frame the row is clicked. It keeps no selection: pass `.selected` from your own state. The title is also the identity, so two rows with the same title in one scope need a key (`"Notes##2"`). `avatar` is a person's name and takes the place of `icon`.

`cui_tree_begin` returns true while the node is open, and only then do you add its children and call `cui_tree_end`. A leaf never opens, so it gets no `cui_tree_end`, as `main.c` shows above. The third argument points at your `bool`, which a click on the node flips; pass `NULL` for a leaf. `cui_tree_clicked` is true on the frame the node of the last `cui_tree_begin` was clicked, so ask right after that call. Children are scoped by their parent, so the same name in two folders is fine. For a tree with columns, use the tree table.

## Accordion

Sections that fold open and closed under their headers.

![Accordion](images/components/accordion.png)

```c
static bool open = true;
if (cui_accordion_begin(ui, "Is it dependency free?", &open)) {
    cui_secondary(ui, "Yes.");
    cui_accordion_end(ui);
}
```

The content animates its height on a smooth spring and the chevron rotates.

`cui_accordion_begin` returns true while the section is open or still closing, and only then do you add its content and call `cui_accordion_end`, as the `if` above does. `open` is your `bool` and must not be `NULL`: a click on the header flips it, and setting it from code opens or closes the section with the same animation. Each section has its own `bool`, so several can be open at once; to keep one open at a time, clear the others when one turns true. The title is also the identity, so two sections with the same title in one scope need a key (`"Details##billing"`). For a card that opens, use the expandable card.

Keyboard: Tab moves through the headers, and Enter or Space opens or closes the focused one.

## Charts

Line, bar and donut charts and sparklines, with axes, a crosshair and a tooltip on hover.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/line-chart-dark.png"><img src="images/components/line-chart.png" alt="Line chart"></picture>

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/bar-donut-dark.png"><img src="images/components/bar-donut.png" alt="Bar and donut"></picture>

```c
cui_series s[] = { { "Subscriptions", subs, 12 }, { "Cloud spend", cloud, 12 } };
cui_line_chart(ui, "revenue", s, 2, .labels = months, .label_count = 12, .area = true, .smooth = true, .format = "$%.0fk");
cui_bar_chart(ui, "visits", s, 2, .labels = months, .label_count = 6);
cui_bar_chart(ui, "runs", runs, 3, .labels = weeks, .label_count = 8, .stacked = true);   /* passed, retried, failed */
cui_donut_chart(ui, "share", values, labels, 4, .format = "%.0f%%");
cui_sparkline(ui, values, 12, CUI_PX(120), 32, CUI_COLOR_SERIES(0));
```

Charts draw in once and pick nice axis ticks. Gridlines are faint dashed hairlines over a solid zero line, and the axis labels are small and muted, so the data leads. The line chart shows a crosshair that glides between points and a tooltip with every series on hover. Bars are slim, with rounded tops and a small gap inside a group; hovering a column lays a soft band behind it, brings its label forward and shows a compact chip over it with each series' value (the full tooltip when the chip would not fit). The donut is a thin ring whose segments are parted by real gaps, with the leading or hovered share in the middle; its legend puts a thin bar under each name that shows the share's size against the largest. Colors come from the theme's series palette, which starts at the accent hue. Options: `width`, `height`, `labels`, `label_count`, `area`, `smooth`, `stacked`, `no_grid`, `no_legend`, `format`, `min`, `max`. With `area` and several series, only the first is filled, so fills never tint each other into a third color.

## Image

Shows a picture loaded from a file or made from your own pixels.

```c
int w, h;
cui_texture tex = cui_image_load(ui, "photo.jpg", &w, &h);      /* PNG or JPEG */
cui_image(ui, tex, .width = CUI_PX(240), .height = CUI_PX(160), .radius = 12);
cui_texture raw = cui_image_create(ui, w, h, rgba_pixels);        /* from memory */
cui_image_destroy(ui, tex);
```

`cui_image_load` returns a `cui_texture`, or 0 when the file cannot be read or decoded. Call it once, not on every frame, keep the texture for as long as you draw it, and free it with `cui_image_destroy`. `cui_image_create` takes `w * h` pixels of four bytes each, red, green, blue and alpha, and copies them. `cui_image` returns nothing and stretches the texture over the box given by `width` and `height`, 64 by 64 points when they are not set, so work the height out from the pixel size to keep the proportions.

PNG and JPEG are decoded out of the box. Configure with `-DCUI_IMAGE_ALL_FORMATS=ON` to add BMP, GIF and TGA, or `-DCUI_STB_IMAGE=OFF` to leave decoding out and upload your own pixels with `cui_image_create`.

## Radio cards

One choice out of a few, shown as cards with a title and a line of explanation each.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/radio-cards-dark.png"><img src="images/components/radio-cards.png" alt="Radio cards"></picture>

```c
static const cui_choice places[] = {
    { "This computer", "Nothing leaves the machine.", CUI_ICON_MONITOR, NULL },     /* title, description, icon, badge */
    { "Team server", "Synced every few minutes.", CUI_ICON_SERVER, "Default" },
    { "Both", "Local, plus a nightly upload.", CUI_ICON_LAYERS, NULL },
};
static int place = 1;
cui_radio_cards(ui, "place", places, 3, &place, .columns = 3);
```

Returns true on the frame the selection changes. `place` is your `int`, the index of the selected card, and a value that matches no card, such as -1, leaves all of them unselected. `places` is your array of `cui_choice`, read every frame: a title, a description, an icon and a badge. `columns` is the number of cards in a row. When it is 0, up to three choices share one row and more are laid out in two columns. A container too narrow for columns of 220 points gets fewer of them, down to one. Use radio cards when each choice needs a sentence of explanation, and `cui_radio_group` or a segmented control when a label says it all.

Keyboard: arrows move the selection.

## Tag input

A field that turns typed words into removable chips, kept as a comma separated list.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/tag-otp-dark.png"><img src="images/components/tag-otp.png" alt="Tag input, code input and password strength"></picture>

```c
static char tags[256] = "design, c99";
cui_tag_input(ui, "topics", tags, sizeof tags, .label = "Topics", .max_tags = 8);
```

Tags live in your buffer as a comma separated list. Enter or a comma adds the typed tag, the x on a chip removes it, Backspace in an empty field removes the last one.

Returns true on the frame a tag is added or removed. `tags` is your buffer and the next argument its size in bytes. The library rewrites it as `"design, c99"`, tags joined by a comma and a space. To read the tags, split the text at the commas and trim the spaces. A tag that no longer fits in the buffer is not added. The word being typed is kept by the library and is not in your buffer until it becomes a tag.

## One time code

A row of boxes for a verification code, filled by typing or pasting.

```c
static char code[7];                        /* length + 1 */
if (cui_otp_input(ui, "code", code, 6)) verify(code);   /* true once when complete */
```

Type or paste: letters and digits fill the boxes in order, anything else is ignored. Backspace removes the last character.

Returns true once, on the frame the last box is filled, and again only after a character was removed and the code completed anew. `code` is your buffer of `length` + 1 bytes, zero terminated; start it empty, as a `static` array is. The library writes the characters as they were typed, without changing their case. To clear the boxes after a wrong code, set `code[0] = 0`. `length` is the number of boxes, at most 12, and codes of six or more get a gap in the middle. The boxes are 44 points wide and shrink to 30 in a narrow container.

Keyboard: Tab focuses the boxes as one field. Backspace removes the last character and Esc leaves the field.

## Password strength

A meter under a password field that scores the password from too short to strong.

```c
static char pw[64];
cui_input(ui, "pw", pw, sizeof pw, .label = "Password", .password = true);
int score = cui_password_strength(ui, pw);   /* 0 too short .. 4 strong */
```

Returns the score from 0 to 4 and draws four segments with a word beside them: "Too short", "Weak", "Fair", "Good" or "Strong". `pw` is your string, the same buffer the field edits; it is only read, and `NULL` or an empty string scores 0 and shows no word. The score counts simple rules: one point each for 8 characters, for 12 characters, for mixing lower and upper case, for a digit and for any other character, capped at 4, and at 1 while the password is shorter than 8. It does not know common passwords, so enforce your real policy with the field's `error`. The meter uses a fixed internal id, so two meters in one id scope need a `cui_id_scope` each.

## Button group, copy and hold to confirm

Three small helpers for actions: buttons joined into one group, a button that copies text to the clipboard, and a button that must be held to fire.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/button-extras-dark.png"><img src="images/components/button-extras.png" alt="Button group, copy and hold"></picture>

```c
cui_button_group(ui) {
    cui_button(ui, "Bold", .icon = CUI_ICON_BOLD);
    cui_button(ui, "Italic", .icon = CUI_ICON_ITALIC);
}
cui_copy_button(ui, "copy-key", api_key);                      /* confirms with a check */
if (cui_hold_button(ui, "Hold to delete", .icon = CUI_ICON_TRASH_2)) delete_everything();
```

The hold button fills while pressed (or while Space is held with keyboard focus) and fires once when full; let go early and it springs back. Options: `duration` (1.2 s), `icon`, `variant` (danger by default).

`cui_button_group` is a scope that takes no options and returns nothing: each button inside still returns its own click. Buttons in a group are ghost buttons unless they set a `variant`, and share one outline. `cui_copy_button` returns true on the frame it is clicked, after it has put the text on the clipboard; its `id` keeps the check that shows for 1.6 seconds afterwards. To copy from a menu item or a status bar instead, call `cui_set_clipboard_text(ui, text)`; `cui_get_clipboard_text(ui)` reads the clipboard for a Paste item. `cui_hold_button` returns true once, on the frame the fill completes, and not again until the button is let go. Its label is also its identity, and `duration` is in seconds. Use the hold button where a slip would cost something, and a confirm button where a second click is enough.

Keyboard: Tab moves through the buttons. Enter or Space presses a button in a group or the copy button, and holding either key fills the hold button. The copy button keeps keyboard focus when the check appears and when it goes away.

## Timeline and progress ring

A vertical list of events in order, and a ring that fills with a percentage.

<picture><source media="(prefers-color-scheme: dark)" srcset="images/components/timeline-dark.png"><img src="images/components/timeline.png" alt="Timeline and progress ring"></picture>

```c
cui_timeline_item(ui, "Deployed to production", .time = "2m ago", .description = "Release 1.0.3 is live.",
                  .icon = CUI_ICON_ROCKET, .tone = CUI_TONE_SUCCESS);
cui_timeline_item(ui, "Build failed", .time = "3h ago", .icon = CUI_ICON_CIRCLE_ALERT, .tone = CUI_TONE_DANGER, .last = true);
cui_progress_ring(ui, "Onboarding", 0.72f, 110);
```

Neither returns anything or takes the pointer. `cui_timeline_item` draws one event: call it once for each event, in order, inside a column. The title is the only required text. `time` is text you format yourself, and `description` wraps under the title. `icon` goes in the round marker, a dot when not set, and `tone` colors the marker. Set `.last = true` on the final item, or its connector line runs on below it.

`cui_progress_ring` takes a label, a fraction from 0 to 1 and a size in points. The fraction is clamped and shown in the middle as a whole percentage, the ring glides to a new value, and a size of 0 means 96. The label is also the ring's identity, so two rings with the same label in one scope need a key (`"Done##week"`).

## Reorderable list

A list whose rows the user drags into a new order.

```c
static int order[5] = { 0, 1, 2, 3, 4 };     /* your item indices, in display order */
cui_reorder_begin(ui, "tasks", order, 5);
for (int i = 0; i < 5; i++) {
    cui_reorder_item(ui, i) {
        cui_text(ui, tasks[order[i]].title, .nowrap = true, .width = CUI_FILL);
    }
}
if (cui_reorder_end(ui)) save_order(order);
```

Press a row and drag: it lifts with a shadow and follows the pointer while the other rows glide out of the way. Dropping rewrites `order`. Buttons inside rows keep working, since a drag only starts after the pointer moves a few points.

`cui_reorder_end` returns true on the frame a drop changes the order. `order` is your array of `count` item indices and must outlive the frame, which is why it is `static` above. The library only moves the numbers in it: your items stay where they are, and you read them through `order[i]`. `cui_reorder_item` takes the display position, not the item index. Rows are told apart by their item index, so each index must appear once. One list handles up to 256 rows.

Keyboard: none yet. Rows are dragged with the pointer; buttons inside rows keep their own keys.
