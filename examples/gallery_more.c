/* Extended gallery pages. Each page keeps its own state in statics. */
#include "gallery_more.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void section(cui_ctx *ui, const char *title, const char *desc) {
    /* more room above a heading than below it, so it reads as the start of what follows */
    cui_column(ui, .gap = 4, .pad_top = 18) {
        cui_heading(ui, title, 4);
        if (desc) cui_secondary(ui, desc);
    }
}

/* ---- actions ---------------------------------------------------------- */

static void page_actions(cui_ctx *ui) {
    static bool fmt[4] = { true, false, false, false };
    static bool align[3] = { true, false, false };
    static cui_status save_status = CUI_STATUS_IDLE;
    static double status_at;
    static cui_reaction reactions[4] = {
        { CUI_ICON_THUMBS_UP, 12, true }, { CUI_ICON_HEART, 4, false }, { CUI_ICON_ROCKET, 7, false }, { CUI_ICON_SMILE, 2, false },
    };
    static int dial_choice = -1;
    static int theme_mode = -1;
    static int deleted;
    static int split_choice = -1;

    section(ui, "Split button", "A main action with related actions one click away.");
    cui_row(ui, .gap = 12, .wrap = true) {
        static const char *items[] = { "Save as draft", "Save and publish", "Schedule\xe2\x80\xa6" };
        static const char *icons[] = { CUI_ICON_FILE_TEXT, CUI_ICON_SEND, CUI_ICON_CLOCK };
        int r = cui_split_button(ui, "Save", items, 3, .item_icons = icons, .icon = CUI_ICON_SAVE);
        if (r >= 0) split_choice = r;
        cui_split_button(ui, "Export", items, 3, .variant = CUI_BUTTON_OUTLINE, .icon = CUI_ICON_DOWNLOAD);
        if (split_choice >= 0) cui_muted(ui, split_choice == 0 ? "Saved" : items[split_choice - 1]);
    }

    section(ui, "Toggle group", "Independent toggles for toolbars, or exclusive like radio buttons.");
    cui_row(ui, .gap = 12, .wrap = true) {
        static const char *labels[] = { "Bold", "Italic", "Underline", "Code" };
        static const char *icons[] = { CUI_ICON_BOLD, CUI_ICON_ITALIC, CUI_ICON_UNDERLINE, CUI_ICON_CODE };
        cui_toggle_group(ui, "format", labels, 4, fmt, .icons = icons, .icon_only = true);
        static const char *alabels[] = { "Left", "Center", "Right" };
        static const char *aicons[] = { CUI_ICON_ALIGN_LEFT, CUI_ICON_LIST, CUI_ICON_MENU };
        cui_toggle_group(ui, "align", alabels, 3, align, .icons = aicons, .exclusive = true);
    }

    section(ui, "Morphing buttons", "Confirm before destroying, and show the result of work in place.");
    cui_row(ui, .gap = 12, .wrap = true) {
        if (cui_confirm_button(ui, "Delete project", .icon = CUI_ICON_TRASH_2)) deleted++;
        if (cui_status_button(ui, "Save changes", save_status, .icon = CUI_ICON_SAVE, .success_label = "Saved")) {
            save_status = CUI_STATUS_BUSY;
            status_at = cui_time(ui);
        }
        double age = cui_time(ui) - status_at;
        if (save_status == CUI_STATUS_BUSY && age > 1.4) { save_status = CUI_STATUS_SUCCESS; status_at = cui_time(ui); }
        else if (save_status == CUI_STATUS_SUCCESS && age > 1.6) save_status = CUI_STATUS_IDLE;
        if (save_status != CUI_STATUS_IDLE) cui_request_redraw(ui);
        if (deleted) cui_muted(ui, "Project deleted");
    }

    section(ui, "Reactions", "Counts roll when they change.");
    cui_reactions(ui, "reactions", reactions, 4);

    section(ui, "Theme", "A compact switcher and a single toggle that morphs between sun and moon.");
    cui_row(ui, .gap = 16, .wrap = true) {
        cui_theme_switcher(ui, &theme_mode, true);
        cui_theme_toggle(ui);
    }

    section(ui, "User menu", "Account trigger for sidebars and headers.");
    cui_box(ui, .width = CUI_PX(260)) {
        cui_user_menu(ui, "Maya Okafor", .detail = "maya@fieldwork.studio", .status = "online", .placement = CUI_PLACE_BOTTOM_START) {
            cui_menu_item(ui, "Profile", .icon = CUI_ICON_USER, .shortcut = "Ctrl P");
            cui_menu_item(ui, "Billing", .icon = CUI_ICON_CREDIT_CARD);
            cui_menu_item(ui, "Settings", .icon = CUI_ICON_SETTINGS, .shortcut = "Ctrl ,");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Log out", .icon = CUI_ICON_LOG_OUT);
        }
    }

    section(ui, "Swipe actions", "Drag a row sideways or swipe with two fingers.");
    cui_card(ui, .pad = 6, .gap = 6) {
        static const cui_swipe_action left[] = { { CUI_ICON_CHECK, "Done", CUI_TONE_SUCCESS } };
        static const cui_swipe_action right[] = { { CUI_ICON_ARCHIVE, "Archive", CUI_TONE_INFO }, { CUI_ICON_TRASH_2, "Delete", CUI_TONE_DANGER } };
        static const char *titles[] = { "Review the design tokens", "Reply to Jonas about the release", "Book the offsite venue" };
        for (int i = 0; i < 3; i++) {
            cui_push_id_int(ui, i);
            cui_swipe_item_begin(ui, "row", left, 1, right, 2);
            cui_icon(ui, CUI_ICON_CIRCLE, 16, CUI_COLOR_TEXT_MUTED);
            cui_text(ui, titles[i], .width = CUI_FILL, .nowrap = true);
            cui_text(ui, "Today", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
            int a = cui_swipe_item_end(ui);
            if (a >= 0) cui_toast(ui, a == 0 ? "Marked as done" : a == 100 ? "Archived" : "Deleted", .tone = a == 101 ? CUI_TONE_DANGER : CUI_TONE_SUCCESS);
            cui_pop_id(ui);
        }
    }

    section(ui, "Speed dial", "A floating action button that fans out related actions.");
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_CENTER, .height = CUI_PX(80)) {
        static const cui_speed_action acts[] = { { CUI_ICON_FILE_TEXT, "New note" }, { CUI_ICON_IMAGE, "Upload image" }, { CUI_ICON_MIC, "Record memo" } };
        int c = cui_speed_dial(ui, "dial", acts, 3, .tooltip = "Create");
        if (c >= 0) dial_choice = c;
        if (dial_choice >= 0) cui_muted(ui, acts[dial_choice].label);
    }
}

/* ---- smart inputs ------------------------------------------------------- */

static void page_inputs(cui_ctx *ui) {
    static char title[128] = "Q3 roadmap";
    static char owner[64] = "Jonas Berg";
    static float opacity = 80, radius = 12, blur = 24;
    static double amount = 1249.5, salary = 98000;
    static cui_key_combo combo = { CUI_MOD_PRIMARY | CUI_MOD_SHIFT, CUI_KEY_P };
    static cui_key_combo combo2 = { 0, CUI_KEY_NONE };
    static char url[96] = "cui.dev", weight[16] = "72";

    section(ui, "Inline edit", "Click the text to edit it in place. Enter saves, Esc cancels.");
    cui_column(ui, .gap = 6) {
        cui_inline_edit(ui, "title", title, sizeof title, .font = CUI_FONT_DISPLAY, .size = 26);
        cui_inline_edit(ui, "owner", owner, sizeof owner, .placeholder = "Add an owner");
    }

    section(ui, "Scrub fields", "Drag sideways to change the value, Shift for fine steps, click to type.");
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_scrub(ui, "Opacity", &opacity, 0, 100, .suffix = "%");
        cui_scrub(ui, "Radius", &radius, 0, 48, .step = 0.5f, .format = "%.1f", .suffix = "px");
        cui_scrub(ui, "Blur", &blur, 0, 80, .suffix = "px", .width = CUI_PX(140));
    }

    section(ui, "Money", "Grouped digits at rest, plain digits while typing.");
    cui_row(ui, .gap = 12, .align = CUI_ALIGN_START) {
        cui_box(ui, .width = CUI_PX(240)) { cui_money_input(ui, "amount", &amount, .label = "Amount", .code = "USD"); }
        cui_box(ui, .width = CUI_PX(240)) { cui_money_input(ui, "salary", &salary, .label = "Salary", .symbol = "\xe2\x82\xac", .code = "EUR", .decimals = -1, .hint = "Gross per year"); }
    }

    section(ui, "Prefix and suffix", "Any text field can carry muted text before and after the value.");
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_input(ui, "url", url, sizeof url, .prefix = "https://", .width = CUI_PX(260));
        cui_input(ui, "weight", weight, sizeof weight, .suffix = "kg", .width = CUI_PX(120));
    }

    section(ui, "Shortcut recorder", "Click, then press a combination. Backspace clears.");
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_shortcut_recorder(ui, "palette-key", &combo);
        cui_shortcut_recorder(ui, "search-key", &combo2, .placeholder = "Record shortcut");
    }

    static char message[160] = "Release notes are up ";
    static char recent[256];
    static int tone;
    section(ui, "Emoji picker", "Search by name, jump by group, pick a skin tone. Recent emoji come first.");
    cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .max_width = 480) {
        cui_input(ui, "message", message, sizeof message, .width = CUI_FILL);
        if (cui_icon_button(ui, CUI_ICON_SMILE, .tooltip = "Emoji")) cui_popover_toggle(ui, "emoji-pop");
        cui_popover(ui, "emoji-pop", .placement = CUI_PLACE_BOTTOM_END, .width = 340, .pad = 0) {
            const char *e = cui_emoji_picker(ui, "emoji", .recent = recent, .recent_cap = sizeof recent, .tone = &tone, .height = 260);
            if (e && strlen(message) + strlen(e) < sizeof message) { strcat(message, e); cui_popover_close(ui, "emoji-pop"); }
        }
    }
}


/* ---- pickers ------------------------------------------------------------- */

static void page_pickers(cui_ctx *ui) {
    static bool langs[8] = { true, false, true, false, false, true, false, false };
    static cui_date_range range = { { 2026, 11, 3 }, { 2026, 11, 14 } };
    static cui_date_range stay = { { 2026, 12, 18 }, { 2026, 12, 27 } };
    static int hour = 9, minute = 30, h2 = 18, m2 = 45;
    static int day = 13, month = 10, year = 6;
    static int dh = 7, dm = 25;
    static float height = 172, temp = 21.5f;
    static float lo = 40, hi = 160;
    static const float bins[] = { 2, 4, 7, 12, 18, 26, 31, 29, 34, 40, 37, 30, 24, 22, 18, 15, 11, 9, 7, 5, 4, 3, 2, 1 };

    section(ui, "Multi-select", "Chips for the chosen items, checkboxes in the list.");
    static const char *items[] = { "C", "C++", "Rust", "Zig", "Go", "Swift", "Kotlin", "Odin" };
    cui_row(ui, .gap = 12, .align = CUI_ALIGN_START) {
        cui_box(ui, .width = CUI_PX(320)) { cui_multi_select(ui, "langs", items, 8, langs, .label = "Languages", .searchable = true); }
    }

    section(ui, "Date range", "Presets on the left, two months to pick a start and an end.");
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_date_range_picker(ui, "range", &range, .width = CUI_PX(300));
        cui_date_range_picker(ui, "stay", &stay, .width = CUI_PX(300), .no_presets = true);
    }
    cui_row(ui) { cui_card(ui, .pad = 16) { cui_range_calendar(ui, "inline-range", &stay, 2); } }   /* as wide as its months */

    section(ui, "Time", "A field with hour and minute columns, 24 hour or 12 hour.");
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_time_picker(ui, "start", &hour, &minute);
        cui_time_picker(ui, "end", &h2, &m2, .twelve_hour = true, .minute_step = 15);
    }

    section(ui, "Wheels and dial", "Drag, scroll or use the arrow keys. Several wheels make a date reel.");
    cui_row(ui, .gap = 32, .align = CUI_ALIGN_CENTER) {
        static const char *days[31], *months[12], *years[12];
        static char dbuf[31][4], ybuf[12][6];
        static const char *mn[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        for (int k = 0; k < 31; k++) { snprintf(dbuf[k], sizeof dbuf[k], "%d", k + 1); days[k] = dbuf[k]; }
        for (int k = 0; k < 12; k++) { months[k] = mn[k]; snprintf(ybuf[k], sizeof ybuf[k], "%d", 2020 + k); years[k] = ybuf[k]; }
        cui_card(ui, .dir = CUI_ROW, .gap = 0, .pad = 8) {
            cui_wheel_picker(ui, "day", days, 31, &day, .width = CUI_PX(64), .loop = true, .join_right = true);
            cui_wheel_picker(ui, "month", months, 12, &month, .width = CUI_PX(128), .loop = true, .join_left = true, .join_right = true);
            cui_wheel_picker(ui, "year", years, 12, &year, .width = CUI_PX(80), .join_left = true);
        }
        cui_time_dial(ui, "dial", &dh, &dm, 232);
    }

    section(ui, "Ruler and histogram", "A tape that slides under a fixed marker, and a range over its data.");
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START) {
        cui_column(ui, .width = CUI_FILL, .gap = 20) {
            cui_ruler_slider(ui, "Height", &height, 120, 220, .unit = "cm");
            cui_ruler_slider(ui, "Temperature", &temp, 10, 30, .step = 0.5f, .major_every = 4, .spacing = 14, .format = "%.1f", .unit = "\xc2\xb0" "C");
        }
        cui_card(ui, .width = CUI_FILL, .gap = 6) {
            cui_histogram_slider(ui, "Price", bins, 24, &lo, &hi, 0, 240, .format = "$%.0f");
            cui_muted(ui, "1,204 listings in range");
        }
    }
}

/* ---- capture ------------------------------------------------------------- */

static void page_capture(cui_ctx *ui) {
    static cui_texture avatar;
    static char last[256] = "";
    section(ui, "Dropzone", "Drop files from your desktop, or click to browse.");
    int n = cui_dropzone(ui, "files", .description = "PNG, JPG or PDF, up to 25 MB", .accept = "png;jpg;jpeg;pdf");
    if (n > 0) snprintf(last, sizeof last, "%s", cui_dropped_file(ui, 0));
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_box(ui, .width = CUI_FILL) { cui_dropzone(ui, "compact", .compact = true, .title = "Attach a file", .description = "Any type"); }
        cui_box(ui, .width = CUI_FILL) { cui_dropzone(ui, "images", .compact = true, .title = "Add images", .description = "PNG or JPG", .icon = CUI_ICON_IMAGE, .accept = "png;jpg;jpeg"); }
    }
    if (last[0]) cui_muted(ui, last);

    section(ui, "Avatar upload", "Drop an image on the avatar.");
    cui_row(ui, .gap = 20, .align = CUI_ALIGN_CENTER) {
        cui_avatar_upload(ui, "avatar", "Maya Okafor", &avatar, 88);
        cui_column(ui, .gap = 4) {
            cui_label(ui, "Profile photo");
            cui_secondary(ui, "At least 256 by 256 pixels.");
        }
    }

    section(ui, "Signature", "Draw with the mouse or a pen. Faster strokes are thinner, like ink.");
    cui_box(ui, .width = CUI_PX(520)) { cui_signature_pad(ui, "signature", .height = 180); }
}


/* ---- disclosure and feedback ---------------------------------------------- */

static cui_texture canvas_picture(cui_ctx *ui);

static void page_disclosure(cui_ctx *ui) {
    static bool banner = true;
    static bool card1 = true, card2 = false;
    static cui_check_step steps[] = {
        { "Create your workspace", "Pick a name and invite your team.", "Create", true },
        { "Connect a repository", "Link GitHub or GitLab to sync issues and pull requests.", "Connect", false },
        { "Import your data", "Bring tasks over from a CSV or another tool.", "Import", false },
        { "Turn on notifications", "Get pinged when something needs you.", "Enable", false },
    };
    static int deleted;
    if (cui_banner(ui, "CUI 1.1 is out: data grid, kanban and forty new components.", &banner, .action = "Read the notes", .dismissible = true))
        cui_toast(ui, "Opening release notes");
    if (!banner && cui_button(ui, "Show the banner again", .size = CUI_SIZE_SM)) banner = true;

    section(ui, "Hover card", "Rest the pointer on a name.");
    cui_row(ui, .gap = 6, .wrap = true) {
        cui_text(ui, "Assigned to");
        cui_link(ui, "@jonas");
        cui_hover_card(ui, "grace-card", .width = 300) {
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_START) {
                cui_avatar(ui, "Jonas Berg", .size = 44, .status = "online");
                cui_column(ui, .gap = 4, .width = CUI_FILL) {
                    cui_text(ui, "Jonas Berg", .font = CUI_FONT_MEDIUM);
                    cui_text(ui, "Engineer on the rendering team. Joined March 2024.", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
                    cui_row(ui, .gap = 6, .pad_top = 4, .wrap = true) {
                        cui_icon(ui, CUI_ICON_CALENDAR, 12, CUI_COLOR_TEXT_MUTED);
                        cui_text(ui, "Usually replies within an hour", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
                    }
                }
            }
        }
        cui_text(ui, "and due Friday.");
    }

    section(ui, "Confirm popover", "Ask right where the action is.");
    cui_row(ui, .gap = 12, .wrap = true) {
        if (cui_button(ui, "Delete branch", .icon = CUI_ICON_TRASH_2)) cui_popover_open(ui, "confirm-delete");
        if (cui_confirm_popover(ui, "confirm-delete", .title = "Delete feature/grid?", .description = "The branch and its 12 commits are removed from the remote.",
                                .confirm_label = "Delete branch", .danger = true)) deleted++;
        if (deleted) cui_muted(ui, "Branch deleted");
    }

    section(ui, "Expandable text", "Long text clamps to a few lines with a fade.");
    cui_expandable_text(ui, "about",
        "CUI is a component library for C. It draws everything itself, so an app looks the same on Windows, macOS and Linux, and it "
        "ships the parts most apps need: inputs, pickers, menus, dialogs, tables, charts and a command palette. Every component "
        "supports the keyboard, light and dark themes and reduced motion. The core is plain C99 with no dependencies, and it "
        "renders through OpenGL or a built-in software rasterizer, so it also runs in virtual machines and over remote desktop.",
        .lines = 2);

    section(ui, "Expandable cards", "A header that opens a body with a spring.");
    cui_column(ui, .gap = 10) {
        cui_expandable_card(ui, "Billing", &card1, .description = "Pro plan, renews on December 1", .icon = CUI_ICON_CREDIT_CARD, .badge = "Active", .badge_tone = CUI_TONE_SUCCESS) {
            /* plain label and value pairs: a card inside the card would only add edges */
            cui_row(ui, .gap = 40, .wrap = true) {
                cui_column(ui, .gap = 2) { cui_muted(ui, "This month"); cui_text(ui, "$84.00", .font = CUI_FONT_DISPLAY, .size = 22, .tabular = true); }
                cui_column(ui, .gap = 2) { cui_muted(ui, "Seats"); cui_text(ui, "7 of 10", .font = CUI_FONT_DISPLAY, .size = 22, .tabular = true); }
            }
            cui_row(ui, .gap = 8, .wrap = true) { cui_button(ui, "Change plan", .size = CUI_SIZE_SM); cui_button(ui, "Invoices", .size = CUI_SIZE_SM, .variant = CUI_BUTTON_GHOST); }
        }
        cui_expandable_card(ui, "Security", &card2, .description = "Two-factor authentication is on", .icon = CUI_ICON_SHIELD) {
            cui_text(ui, "Recovery codes were generated 3 months ago.", .color = CUI_COLOR_TEXT_SECONDARY);
        }
    }

    section(ui, "Countdown", "Digits roll as the time passes.");
    {
        static double target;
        if (target == 0) target = cui_time(ui) + 2 * 86400 + 5 * 3600 + 42 * 60 + 18;
        cui_row(ui, .gap = 24, .align = CUI_ALIGN_CENTER) {
            cui_countdown(ui, "launch", target - cui_time(ui));
            cui_countdown(ui, "sale", target - cui_time(ui) - 86400 * 2, .compact = true, .hide_days = true);
        }
    }

    section(ui, "Onboarding checklist", "Progress, with the next step open.");
    cui_box(ui, .width = CUI_PX(460)) {
        int a = cui_checklist(ui, "onboarding", steps, 4, .title = "Set up your workspace");
        if (a >= 0) { steps[a].done = true; cui_toast(ui, steps[a].title, .tone = CUI_TONE_SUCCESS); }
    }

    section(ui, "Undo toast", "A draining line shows how long Undo stays.");
    if (cui_button(ui, "Archive 3 conversations", .icon = CUI_ICON_ARCHIVE)) cui_toast(ui, "3 conversations archived", .action = "Undo", .show_timer = true);

    section(ui, "Veil", "Covers a picture, a secret or a spoiler with moving dots, and dissolves when clicked.");
    {
        static bool photo, key, spoiler;
        cui_column(ui, .gap = 12, .pad = 12, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 16, .width = CUI_FILL, .max_width = 420) {
            cui_veil(ui, "photo", &photo, .label = "Sensitive image", .hint = "Click to show", .toggle = true, .radius = 10) {
                cui_image(ui, canvas_picture(ui), .width = CUI_PX(234), .height = CUI_PX(150), .radius = 10);
            }
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad = 8, .pad_x = 10, .bg = CUI_COLOR_SURFACE_MUTED, .radius = 10, .width = CUI_FILL) {
                cui_text(ui, "sk-live-", .font = CUI_FONT_MONO, .size = 12.5f, .color = CUI_COLOR_TEXT_SECONDARY);
                cui_box(ui, .width = CUI_FILL) {
                    cui_veil(ui, "key", &key, .color = CUI_COLOR_SURFACE) { cui_text(ui, "4f9a82c1e07d55b3a9", .font = CUI_FONT_MONO, .size = 12.5f); }
                }
                if (cui_button(ui, key ? "Hide" : "Reveal", .size = CUI_SIZE_SM)) key = !key;
            }
            cui_row(ui, .gap = 4, .wrap = true, .align = CUI_ALIGN_CENTER) {
                cui_text(ui, "Finished it last night. The ending:");
                cui_veil(ui, "spoiler", &spoiler, .toggle = true, .radius = 4) { cui_text(ui, "the keeper was the narrator"); }
            }
        }
    }
}

/* ---- docs page with a table of contents ---------------------------------- */

static void page_reader(cui_ctx *ui) {
    static const cui_toc_item toc[] = {
        { "Introduction", "intro", 0 }, { "Install", "install", 0 }, { "With CMake", "cmake", 1 }, { "Without CMake", "nocmake", 1 },
        { "First window", "window", 0 }, { "Immediate mode", "imm", 0 }, { "Layout", "layout", 0 }, { "Next steps", "next", 0 },
    };
    static const char *para = "Components are functions. They draw themselves, handle input and return what happened. Your variables are the "
                              "source of truth, so there is nothing to keep in sync, and conditional interface is just an if statement. ";
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_scroll(ui, .id = "reader", .height = CUI_PX(560), .width = CUI_FILL, .gap = 14, .fade_edges = true) {
            for (int k = 0; k < 8; k++) {
                cui_toc_anchor(ui, toc[k].anchor);
                cui_heading(ui, toc[k].title, toc[k].level ? 4 : 3);
                for (int p = 0; p < 3; p++) { cui_push_id_int(ui, k * 10 + p); cui_text(ui, para, .color = CUI_COLOR_TEXT_SECONDARY); cui_pop_id(ui); }
            }
            cui_gap(ui, 200);
        }
        cui_column(ui, .width = CUI_PX(200), .gap = 10) {
            cui_text(ui, "On this page", .size = 12, .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_TEXT_MUTED);
            cui_toc(ui, "reader", toc, 8);
        }
    }
}


/* ---- data grid and code ---------------------------------------------------- */

typedef struct { char name[48]; char email[64]; const char *team; int commits; float score; } person;
#define PEOPLE 2000
static person people[PEOPLE];
static int order_ix[PEOPLE];
static bool people_sel[PEOPLE];
static cui_table_sort people_sort;

static int cmp_people(const void *a, const void *b) {
    const person *x = &people[*(const int *)a], *y = &people[*(const int *)b];
    int r = 0;
    switch (people_sort.column) {
    case 0: r = strcmp(x->name, y->name); break;
    case 2: r = strcmp(x->team, y->team); break;
    case 3: r = x->commits - y->commits; break;
    default: r = x->score < y->score ? -1 : x->score > y->score; break;
    }
    return people_sort.descending ? -r : r;
}

static void page_grid(cui_ctx *ui) {
    static bool init;
    static cui_table_sort last_sort = { -1, false };
    static const char *first[] = { "Maya", "Priya", "Jonas", "Lena", "Sam", "Tomas", "Aiko", "Daniel",
                                    "Rosa", "Omar", "Elif", "Ben", "Nora", "Kwame", "Hana", "Felix" };
    static const char *last[] = { "Okafor", "Nair", "Berg", "Fischer", "Whitfield", "Rivera", "Tanaka", "Mensah",
                                   "Delgado", "Haddad", "Kaya", "Carter", "Lindqvist", "Asante", "Novak", "Moreau" };
    static const char *teams[] = { "Engine", "Design", "Platform", "Docs", "Growth" };
    if (!init) {
        for (int k = 0; k < PEOPLE; k++) {
            /* a cheap hash, so neighbouring rows do not repeat each other */
            uint32_t h = (uint32_t)k * 2654435761u;
            h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
            const char *fn = first[h % 16], *ln = last[(h >> 4) % 16];
            snprintf(people[k].name, sizeof people[k].name, "%s %s", fn, ln);
            snprintf(people[k].email, sizeof people[k].email, "%s.%c%u@cui.dev", fn, ln[0], (h >> 8) % 90 + 10);
            for (char *c = people[k].email; *c; c++) if (*c >= 'A' && *c <= 'Z') *c += 32;
            people[k].team = teams[(h >> 12) % 5];
            people[k].commits = (int)((h >> 16) % 1400) + 12;
            people[k].score = (float)((h >> 3) % 1000) / 10.0f;
            order_ix[k] = k;
        }
        people_sort.column = 3; people_sort.descending = true;
        init = true;
    }
    if (people_sort.column != last_sort.column || people_sort.descending != last_sort.descending) {
        qsort(order_ix, PEOPLE, sizeof(int), cmp_people);
        last_sort = people_sort;
    }
    static const char *const team_opts[] = { "Engine", "Design", "Platform", "Docs", "Growth" };
    static const char *const status_opts[] = { "Active", "Away", "Invited" };
    static const char *const role_opts[] = { "Owner", "Admin", "Member" };
    static const cui_filter_field fields[] = {
        { "Team", CUI_ICON_USERS, team_opts, 5 },
        { "Status", CUI_ICON_CIRCLE_DOT, status_opts, 3 },
        { "Role", CUI_ICON_KEY_ROUND, role_opts, 3 },
    };
    static uint32_t filters[3] = { 1u << 0 | 1u << 2, 0, 0 };
    static char search[64];
    section(ui, "Filter bar", "Chips for active filters, values in a list, and a search.");
    cui_filter_bar(ui, "filters", fields, 3, filters, search, sizeof search);

    section(ui, "Data grid", "2,000 rows, virtualized. Sort, resize columns, select with Shift and Ctrl, edit names with a double click.");
    static const cui_grid_column cols[] = {
        { "Name", 200, CUI_TEXT_LEFT, true }, { "Email", 196, CUI_TEXT_LEFT, false }, { "Team", 110, CUI_TEXT_LEFT, true },
        { "Commits", 96, CUI_TEXT_RIGHT, true }, { "Score", 130, CUI_TEXT_RIGHT, true },
    };
    int n_sel = 0;
    for (int k = 0; k < PEOPLE; k++) n_sel += people_sel[k];
    int f0, l0;
    if (cui_data_grid_begin(ui, "people", cols, 5, PEOPLE, &f0, &l0, .height = CUI_PX(420), .selected = people_sel, .sort = &people_sort)) {
        for (int r = f0; r < l0; r++) {
            person *p = &people[order_ix[r]];
            cui_data_grid_row(ui, r);
            cui_data_grid_cell(ui);
            cui_avatar(ui, p->name, .size = 24);
            cui_data_grid_edit(ui, p->name, sizeof p->name);
            cui_data_grid_cell_text(ui, p->email);
            cui_data_grid_cell(ui);
            cui_badge(ui, p->team);
            {
                char buf[16];
                snprintf(buf, sizeof buf, "%d", p->commits);
                cui_data_grid_cell_text(ui, buf);
            }
            cui_data_grid_cell(ui);
            cui_progress(ui, p->score / 100, .width = CUI_PX(70), .height = 5);
            {
                char buf[16];
                snprintf(buf, sizeof buf, "%.1f", (double)p->score);
                cui_text(ui, buf, .tabular = true, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
            }
        }
        cui_data_grid_end(ui);
    }
    cui_muted(ui, n_sel ? "Rows selected. Press Esc to clear." : "Click a row, Shift+click to select a range, Ctrl+A for all.");

    section(ui, "Tree table", "Tree cells inside a table: disclosure, indentation, totals.");
    {
        static bool open_src = true, open_ui = false;
        cui_table_column tc[] = { { "Name", CUI_FILL, CUI_TEXT_LEFT, false }, { "Files", CUI_PX(80), CUI_TEXT_RIGHT, false }, { "Size", CUI_PX(100), CUI_TEXT_RIGHT, false } };
        cui_table(ui, "tree", tc, 3) {
            cui_table_row(ui, false);
            cui_table_cell(ui); cui_tree_cell(ui, "src", 0, true, &open_src, CUI_ICON_FOLDER);
            cui_table_cell_text(ui, "42"); cui_table_cell_text(ui, "1.2 MB");
            if (open_src) {
                cui_table_row(ui, false);
                cui_table_cell(ui); cui_tree_cell(ui, "ui", 1, true, &open_ui, CUI_ICON_FOLDER);
                cui_table_cell_text(ui, "18"); cui_table_cell_text(ui, "640 KB");
                if (open_ui) {
                    cui_table_row(ui, false);
                    cui_table_cell(ui); cui_tree_cell(ui, "button.c", 2, false, NULL, CUI_ICON_FILE);
                    cui_table_cell_text(ui, "1"); cui_table_cell_text(ui, "24 KB");
                }
                cui_table_row(ui, false);
                cui_table_cell(ui); cui_tree_cell(ui, "main.c", 1, false, NULL, CUI_ICON_FILE);
                cui_table_cell_text(ui, "1"); cui_table_cell_text(ui, "8 KB");
            }
            cui_table_row(ui, false);
            cui_table_cell(ui); cui_tree_cell(ui, "README.md", 0, false, NULL, CUI_ICON_FILE_TEXT);
            cui_table_cell_text(ui, "1"); cui_table_cell_text(ui, "12 KB");
        }
    }
}

static void page_code(cui_ctx *ui) {
    section(ui, "Code block", "Highlighting for C, C++, JavaScript, TypeScript, Python, Rust, Go, JSON and shell.");
    cui_code_block(ui, "c-sample",
        "#include <cui/cui.h>\n"
        "\n"
        "/* A counter with a button */\n"
        "static int clicks = 0;\n"
        "\n"
        "void frame(cui_ctx *ui) {\n"
        "    if (cui_button(ui, \"Click me\", .variant = CUI_BUTTON_PRIMARY)) clicks++;\n"
        "    cui_textf(ui, \"Clicked %d times\", clicks);\n"
        "    float ratio = clicks / 10.0f;   // grows with each click\n"
        "}\n",
        .lang = "c", .title = "counter.c", .line_numbers = true, .highlight_from = 7);
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_box(ui, .width = CUI_GROW(1)) {
            cui_code_block(ui, "py", "def fib(n: int) -> int:\n    # the slow way, on purpose\n    if n < 2:\n        return n\n    return fib(n - 1) + fib(n - 2)\n\nprint([fib(k) for k in range(10)])\n",
                           .lang = "python", .title = "fib.py");
        }
        cui_box(ui, .width = CUI_GROW(1)) {
            cui_code_block(ui, "sh", "git clone https://github.com/yappologistic/CUI.git\ncmake -S CUI -B build\ncmake --build build\n./build/examples/gallery  # run it\n",
                           .lang = "shell");
        }
    }
    section(ui, "JSON viewer", "Collapsible, with counts. Click a brace to fold it.");
    cui_json_view(ui, "json",
        "{\"name\": \"cui\", \"version\": \"1.1.0\", \"private\": false, \"downloads\": 128400,\n"
        " \"platforms\": [\"windows\", \"macos\", \"linux\"],\n"
        " \"renderer\": {\"backend\": \"opengl\", \"fallback\": \"software\", \"vsync\": true, \"msaa\": null},\n"
        " \"authors\": [{\"name\": \"Maya\", \"commits\": 412}, {\"name\": \"Jonas\", \"commits\": 388}]}",
        .expand_depth = 2);
    cui_json_view(ui, "broken", "{\"ok\": true, \"oops\": [1, 2,}");
}


/* ---- conversation and media ------------------------------------------------- */

static void make_scene(unsigned char *px, int w, int h, bool color) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float fx = (float)x / w, fy = (float)y / h;
            float r = 0.56f + fy * 0.34f, g = 0.72f + fy * 0.2f, b = 0.88f + fy * 0.06f;   /* clear daytime sky */
            float sun = (fx - 0.72f) * (fx - 0.72f) * 15.4f + (fy - 0.24f) * (fy - 0.24f) * 6.0f;   /* round at a 1.6 aspect */
            if (sun < 0.024f) { r = 1.0f; g = 0.97f; b = 0.86f; }
            float m1 = 0.5f + 0.1f * sinf(fx * 6.0f + 0.6f) + 0.03f * sinf(fx * 23.0f);
            float m2 = 0.68f + 0.06f * sinf(fx * 4.0f + 2.1f) + 0.02f * sinf(fx * 31.0f);
            if (fy > m1) { r = 0.5f; g = 0.6f; b = 0.62f - fy * 0.08f; }        /* far hills in haze */
            if (fy > m2) { r = 0.3f - (fy - m2) * 0.2f; g = 0.45f - (fy - m2) * 0.25f; b = 0.3f; }   /* near fields */
            if (!color) { float l = r * 0.3f + g * 0.59f + b * 0.11f; l = l * 0.85f + 0.05f; r = g = b = l; }
            unsigned char *p = px + (y * w + x) * 4;
            p[0] = (unsigned char)(fminf(1, fmaxf(0, r)) * 255); p[1] = (unsigned char)(fminf(1, fmaxf(0, g)) * 255);
            p[2] = (unsigned char)(fminf(1, fmaxf(0, b)) * 255); p[3] = 255;
        }
}

typedef struct { cui_texture before, after; } cmp_textures;
static void cmp_textures_free(cui_ctx *ui, void *data) {
    cmp_textures *tx = data;
    cui_image_destroy(ui, tx->before);
    cui_image_destroy(ui, tx->after);
}

static void page_social(cui_ctx *ui) {
    static int likes[3] = { 12, 3, 0 };
    static bool liked[3] = { true, false, false };
    static int slide = 1;
    static cui_texture before, after;   /* this frame's: from the state below */
    static float split = 0.45f;
    static int deck = 0;
    static double revenue = 48210;
    static const cui_presence_user who[] = {
        { "Maya Okafor", { 0x7c, 0x5c, 0xff, 255 }, 0, false }, { "Jonas Berg", { 0x22, 0xc5, 0x8b, 255 }, 0, false },
        { "Priya Nair", { 0xf5, 0x9e, 0x0b, 255 }, 0, false }, { "Lena Fischer", { 0x38, 0xbd, 0xf8, 255 }, 0, true },
        { "Aiko Tanaka", { 0xec, 0x48, 0x99, 255 }, 0, false }, { "Daniel Mensah", { 0x10, 0xb9, 0x81, 255 }, 0, false },
        { "Rosa Delgado", { 0xa8, 0x55, 0xf7, 255 }, 0, false },
    };
    /* textures belong to one context, so they live in its state store; they are freed with
     * the state, a few seconds after the page is left, and made again on the way back */
    cui_id tid = cui_make_id(ui, "cmp-textures");
    cmp_textures *tx = cui_state(ui, tid, sizeof *tx);
    if (!tx->before) {
        static unsigned char a[640 * 400 * 4], b[640 * 400 * 4];
        make_scene(a, 640, 400, false);
        make_scene(b, 640, 400, true);
        tx->before = cui_image_create(ui, 640, 400, a);
        tx->after = cui_image_create(ui, 640, 400, b);
        cui_state_on_free(ui, tid, cmp_textures_free);
    }
    before = tx->before; after = tx->after;
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Chat", "Grouped bubbles, read receipts and a typing indicator.");
            cui_card(ui, .gap = 0, .pad = 16) {
                cui_chat_divider(ui, "Today");
                cui_chat_message(ui, "Did the data grid make it into 1.1?", .author = "Jonas Berg", .time = "9:41", .last_in_group = false);
                cui_chat_message(ui, "I need it for the billing page.", .author = "Jonas Berg", .grouped = true, .last_in_group = true);
                cui_chat_message(ui, "It did. 2,000 rows scroll at 60 fps on my laptop.", .mine = true);
                cui_chat_message(ui, "Shift and Ctrl clicks select like a spreadsheet.", .mine = true, .grouped = true, .last_in_group = true, .status = "Read", .time = "9:43");
                cui_typing_indicator(ui, "Jonas");
            }
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Comments", "Threads with replies, likes and badges.");
            cui_card(ui, .gap = 4, .pad = 14) {
                cui_comment(ui, "c1", "Tomas Rivera", "The new numbers match the staging run. I would still test the import on a large file before Friday.", .time = "2h", .likes = &likes[0], .liked = &liked[0], .badge = "Author");
                cui_comment(ui, "c2", "Priya Nair", "Agreed. I have a 2 GB export we can use.", .time = "1h", .depth = 1, .likes = &likes[1], .liked = &liked[1]);
                cui_comment(ui, "c3", "Maya Okafor", "Scheduled it for 3 pm.", .time = "40m", .depth = 1, .likes = &likes[2], .liked = &liked[2], .edited = true);
            }
        }
    }
    section(ui, "Presence", "Who is here now. Hover to spread them out.");
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_CENTER) {
        cui_presence(ui, "here", who, 7, 5, 30);
        cui_counter(ui, "rev", revenue, .format = "$%.0f", .size = 34, .group_thousands = true);
        if (cui_button(ui, "Add a sale", .size = CUI_SIZE_SM, .icon = CUI_ICON_PLUS)) revenue += 1287;
    }

    section(ui, "Ticker", "A continuous strip of figures. It pauses under the pointer.");
    static const cui_ticker_item tick[] = {
        { "Revenue", "$48.2k", 6.9f }, { "Active users", "12,408", 3.1f }, { "Churn", "1.7%", -0.6f, true }, { "Latency", "182 ms", -8.4f, true },
        { "Builds", "1,204", 12.0f }, { "Uptime", "99.98%", 0 },
    };
    cui_ticker(ui, "tick", tick, 6);

    section(ui, "Carousel", "Drag, flick, scroll sideways or use the arrows and dots.");
    cui_carousel(ui, "tour", 4, &slide, .height = 200, .auto_advance = 6) {
        static const char *titles[] = { "Faster exports", "ARM64 runners", "Shortcuts everywhere", "Smaller downloads" };
        static const char *descs[] = { "A 900 file project now exports in 40 seconds, down from four minutes.",
                                       "Linux and macOS jobs can run on ARM64 machines in every plan.",
                                       "Every action in the editor has a key, listed under Help, Shortcuts.",
                                       "The desktop app is 38 MB lighter after dropping the bundled browser." };
        static const unsigned tops[] = { 0x1f2a37, 0x0f3d3e, 0x3a2e22, 0x2b2f3a };
        for (int k = 0; k < 4; k++) cui_carousel_slide(ui) {
            cui_box(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_RGB(tops[k]), .radius = 16, .pad = 28, .gap = 8, .justify = CUI_JUSTIFY_END) {
                cui_text(ui, titles[k], .font = CUI_FONT_DISPLAY, .size = 24, .color = CUI_RGB(0xffffff));
                cui_text(ui, descs[k], .color = CUI_RGBA(0xffffffc0));
            }
        }
    }

    static cui_image_view view;
    static cui_rect crop;
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(3), .min_width = 280, .gap = 10) {
            section(ui, "Image viewer", "Wheel to zoom around the pointer, drag to pan, double-click for 100%.");
            cui_image_viewer(ui, "viewer", after, 640, 400, &view, .height = CUI_PX(260));
        }
        cui_column(ui, .width = CUI_GROW(2), .min_width = 240, .gap = 10) {
            section(ui, "Cropper", "Drag the corners; the square keeps its shape.");
            cui_cropper(ui, "cropper", after, 640, 400, &crop, .aspect = 1, .height = CUI_PX(260));
        }
    }
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Before and after", "Drag the divider.");
            cui_image_compare(ui, "cmp", before, after, &split, .height = 230, .aspect = 640.0f / 400.0f);
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10, .align = CUI_ALIGN_CENTER) {
            section(ui, "Card stack", "Flick the top card away, left or right.");
            static const char *names[] = { "Kyoto", "Lisbon", "Reykjavik", "Oaxaca", "Hanoi" };
            static const unsigned cols[] = { 0xc8553d, 0x2d3047, 0x5b8e7d, 0xb7791f, 0x1f3a5f };
            cui_card_stack_begin(ui, "deck", 3, &(cui_card_stack_opts){ .width = CUI_PX(260), .height = 200 });
            for (int k = 0; k < 3; k++) {
                int idx = (deck + k) % 5;
                cui_card_stack_item(ui, k) {
                    cui_box(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_RGB(cols[idx]), .radius = 25, .pad = 20, .justify = CUI_JUSTIFY_END, .gap = 4) {
                        cui_text(ui, names[idx], .font = CUI_FONT_DISPLAY, .size = 22, .color = CUI_RGB(0xffffff));
                        cui_text(ui, "Swipe right to save", .size = 13, .font = CUI_FONT_MEDIUM, .color = CUI_RGBA(0xfffffff0));
                    }
                }
            }
            int dir = cui_card_stack_end(ui);
            if (dir) { deck = (deck + 1) % 5; cui_toast(ui, dir > 0 ? "Saved" : "Skipped"); }
        }
    }
}

/* ---- planning ------------------------------------------------------------ */

typedef struct { int id; const char *title, *tag, *who; cui_tone tone; int comments, files; float progress; } kb_card;

static kb_card kb_cards[] = {
    { 1, "Audit the color tokens for contrast", "Design", "Maya Okafor", CUI_TONE_VIOLET, 3, 0, 0 },
    { 2, "Virtualize the activity feed", "Perf", "Jonas Berg", CUI_TONE_TEAL, 1, 2, 0 },
    { 3, "Dark mode screenshots for docs", "Docs", "Aiko Tanaka", CUI_TONE_ROSE, 0, 4, 0 },
    { 4, "Keyboard nav in the date range picker", "A11y", "Priya Nair", CUI_TONE_INFO, 5, 0, 0.6f },
    { 5, "IME composition on Windows", "Platform", "Daniel Mensah", CUI_TONE_WARNING, 2, 1, 0.3f },
    { 6, "Spring presets", "Motion", "Rosa Delgado", CUI_TONE_VIOLET, 4, 0, 1 },
    { 7, "Glyph atlas eviction", "Perf", "Lena Fischer", CUI_TONE_TEAL, 0, 0, 1 },
    { 8, "Retina screenshots", "Docs", "Aiko Tanaka", CUI_TONE_ROSE, 1, 3, 1 },
};
static int kb_cols[4][8] = { { 1, 2, 3 }, { 4, 5 }, { 6 }, { 7, 8 } };
static int kb_len[4] = { 3, 2, 1, 2 };

static kb_card *kb_find(int id) {
    for (size_t k = 0; k < sizeof kb_cards / sizeof kb_cards[0]; k++) if (kb_cards[k].id == id) return &kb_cards[k];
    return NULL;
}

static void kb_apply(const cui_kanban_move *m) {
    int *from = kb_cols[m->from_column], *to = kb_cols[m->to_column];
    int at = -1;
    for (int k = 0; k < kb_len[m->from_column]; k++) if (from[k] == m->card) at = k;
    if (at < 0 || (m->to_column != m->from_column && kb_len[m->to_column] >= 8)) return;
    for (int k = at; k < kb_len[m->from_column] - 1; k++) from[k] = from[k + 1];
    kb_len[m->from_column]--;
    int idx = m->to_index < 0 ? 0 : m->to_index > kb_len[m->to_column] ? kb_len[m->to_column] : m->to_index;
    for (int k = kb_len[m->to_column]; k > idx; k--) to[k] = to[k - 1];
    to[idx] = m->card;
    kb_len[m->to_column]++;
}

static void page_planning(cui_ctx *ui) {
    static const char *col_names[4] = { "Backlog", "In progress", "Review", "Done" };
    static const unsigned col_dots[4] = { 0x94a3b8, 0x3b82f6, 0xf59e0b, 0x22c55e };
    section(ui, "Kanban", "Drag cards within and between columns. Columns can carry a work-in-progress limit.");
    cui_kanban_begin(ui, "board");
    for (int c = 0; c < 4; c++) {
        if (cui_kanban_column_begin(ui, col_names[c], kb_len[c], .dot = CUI_RGB(col_dots[c]), .limit = c == 1 ? 3 : 0, .width = 264))
            cui_toast(ui, "New card", .description = col_names[c]);
        for (int k = 0; k < kb_len[c]; k++) {
            kb_card *cd = kb_find(kb_cols[c][k]);
            if (!cd) continue;
            if (cui_kanban_card_begin(ui, cd->id)) cui_toast(ui, cd->title);
            cui_badge(ui, cd->tag, .tone = cd->tone, .size = CUI_SIZE_SM);
            cui_text(ui, cd->title, .font = CUI_FONT_MEDIUM, .size = 13.5f, .line_height = 1.35f);
            if (cd->progress > 0 && cd->progress < 1) cui_progress(ui, cd->progress, .height = 4);
            cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_avatar(ui, cd->who, .size = 22);
                cui_spacer(ui);
                static const char *nums[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
                if (cd->files) { cui_icon(ui, CUI_ICON_PAPERCLIP, 12, CUI_COLOR_TEXT_MUTED); cui_text(ui, nums[cd->files % 10], .size = 12, .color = CUI_COLOR_TEXT_MUTED); }
                if (cd->comments) { cui_icon(ui, CUI_ICON_MESSAGE_SQUARE, 12, CUI_COLOR_TEXT_MUTED); cui_text(ui, nums[cd->comments % 10], .size = 12, .color = CUI_COLOR_TEXT_MUTED); }
                if (cd->progress >= 1) cui_icon(ui, CUI_ICON_CIRCLE_CHECK, 14, CUI_COLOR_SUCCESS);
            }
            cui_kanban_card_end(ui);
        }
        cui_kanban_column_end(ui);
    }
    cui_kanban_move mv;
    if (cui_kanban_end(ui, &mv)) kb_apply(&mv);

    section(ui, "Week calendar", "Drag events to move them in 15 minute steps, or pull the bottom edge to resize.");
    static cui_cal_event events[] = {
        { "Standup", "Zoom", 0, 9.5f, 9.75f, CUI_TONE_INFO }, { "Design review", "Room 2", 0, 11, 12.5f, CUI_TONE_VIOLET },
        { "Lunch with Maya", NULL, 1, 12.5f, 13.5f, CUI_TONE_SUCCESS }, { "Standup", "Zoom", 1, 9.5f, 9.75f, CUI_TONE_INFO },
        { "Release planning", "Room 1", 2, 10, 11.5f, CUI_TONE_WARNING }, { "1:1 Jonas", NULL, 2, 10.5f, 11, CUI_TONE_ROSE },
        { "Focus time", "No meetings", 2, 14, 17, CUI_TONE_TEAL }, { "Standup", "Zoom", 3, 9.5f, 9.75f, CUI_TONE_INFO },
        { "Customer call", "Northlight", 3, 15, 16, CUI_TONE_DANGER }, { "Demo day", "All hands", 4, 16, 17.5f, CUI_TONE_VIOLET },
        { "Gym", NULL, 5, 10, 11.5f, CUI_TONE_SUCCESS },
    };
    cui_date today = cui_date_today();
    int td = cui_date_to_days(today);
    int monday = td - ((td + 3) % 7 + 7) % 7;
    int ev = cui_week_calendar(ui, "week", cui_date_from_days(monday), events, (int)(sizeof events / sizeof events[0]), .start_hour = 8, .end_hour = 19, .height = CUI_PX(420));
    if (ev >= 0) cui_toast(ui, events[ev].title, .description = events[ev].detail);

    section(ui, "Gantt", "Drag bars to reschedule, pull either end to change the length. Curves follow dependencies.");
    static cui_gantt_task tasks[] = {
        /* no tones: each bar takes its state's color, worked out from its progress and today */
        { "Research", "Maya Okafor", 0, 5, 1, 0, -1, false },
        { "Wireframes", "Aiko Tanaka", 4, 10, 1, 0, 0, false },
        { "Design system", "Rosa Delgado", 8, 17, 0.7f, 0, 1, false },
        { "Core engine", "Jonas Berg", 6, 20, 0.55f, 0, 0, false },
        { "Platform backends", "Daniel Mensah", 14, 26, 0.2f, 0, 3, false },
        { "Beta", NULL, 27, 27, 0, 0, 4, true },
        { "Docs and launch", "Priya Nair", 28, 36, 0, 0, 5, false },
    };
    cui_date origin = cui_date_from_days(monday - 7);
    int tk = cui_gantt(ui, "plan", tasks, (int)(sizeof tasks / sizeof tasks[0]), origin, 42, .today = 7 + (td - monday));
    if (tk >= 0) {
        static char dur[32];
        snprintf(dur, sizeof dur, "%d days", tasks[tk].end - tasks[tk].start);
        cui_toast(ui, tasks[tk].name, .description = tasks[tk].milestone ? "Milestone" : dur);
    }
    section(ui, "Month calendar", "Bars for days off and trips, timed events in time order, +N more when a day is full.");
    static cui_date month, picked;
    cui_date now = cui_date_today();
    int base = cui_date_to_days((cui_date){ now.year, now.month, 1 });
    static cui_month_event mev[12];
    static const char *const titles[12] = { "Standup", "Design review", "Offsite", "1:1", "Release", "Planning",
                                            "Customer call", "Lunch", "Retro", "Holiday", "Demo", "Board meeting" };
    for (int k = 0; k < 12; k++) {
        bool span = k == 2 || k == 9;
        mev[k] = (cui_month_event){ titles[k], cui_date_from_days(base + (k * 5) % 27), span ? cui_date_from_days(base + (k * 5) % 27 + 2) : (cui_date){ 0 },
                                    9.0f + (float)(k % 8), span || k == 4, (cui_tone)(1 + k % 8) };
    }
    cui_month_result mr = cui_month_calendar(ui, "month", &month, mev, 12, .selected = &picked);
    if (mr.event >= 0) cui_toast(ui, mev[mr.event].title, .description = "Opened from the month calendar");
}

/* ---- more charts --------------------------------------------------------- */

static void page_viz(cui_ctx *ui) {
    static float traffic[7 * 24];
    static float days[364];
    static bool init;
    if (!init) {
        for (int d = 0; d < 7; d++)
            for (int h = 0; h < 24; h++) {
                float work = (d < 5) ? 1.0f : 0.45f;
                float day = expf(-powf((h - 13.5f) / 4.2f, 2)) * 90 * work + expf(-powf((h - 20.5f) / 2.0f, 2)) * 35;
                traffic[d * 24 + h] = day + (float)((d * 31 + h * 17) % 11);
            }
        unsigned s = 7;
        for (int k = 0; k < 364; k++) {
            s = s * 1103515245u + 12345u;
            int wd = (k + ((cui_date_to_days(cui_date_today()) - 363) % 7 + 10) % 7) % 7;
            float base = (wd >= 5 ? 0.6f : 3.2f) * (0.6f + 0.4f * sinf(k / 40.0f) * sinf(k / 40.0f));
            float r = (float)((s >> 16) % 1000) / 1000.0f;
            days[k] = r < 0.18f ? 0 : floorf(base * r * 4);
        }
        init = true;
    }
    static const char *wd[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const char *hours[24] = { "0h", NULL, NULL, "3h", NULL, NULL, "6h", NULL, NULL, "9h", NULL, NULL, "12h", NULL, NULL, "15h", NULL, NULL, "18h", NULL, NULL, "21h", NULL, NULL };
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Heatmap", "Requests by weekday and hour.");
            cui_card(ui, .pad = 18, .width = CUI_FILL) { cui_heatmap(ui, "traffic", traffic, 7, 24, .row_labels = wd, .col_labels = hours, .format = "%.0f req"); }
        }
    }
    section(ui, "Calendar heatmap", "A year of activity, one square a day.");
    cui_card(ui, .pad = 18) {
        cui_date start = cui_date_from_days(cui_date_to_days(cui_date_today()) - 363);
        cui_calendar_heatmap(ui, "activity", start, days, 364, .unit = "contributions");
    }
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1.3f), .gap = 10) {
            section(ui, "Treemap", "Bundle size by module.");
            static const cui_viz_item mods[] = {
                { "Layout", 55 }, { "Text", 42 }, { "Draw", 35 }, { "Controls", 31 }, { "Charts", 23 },
                { "Overlays", 19 }, { "Platform", 16 }, { "Theme", 9 }, { "Icons", 7 }, { "Grid", 6 },
            };
            cui_treemap(ui, "bundle", mods, 10, .height = 280, .format = "%.0f KB");
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10, .align = CUI_ALIGN_CENTER) {
            section(ui, "Radar", "Two releases across six qualities.");
            static const float a[] = { 82, 64, 90, 58, 75, 88 }, b[] = { 70, 85, 72, 80, 60, 79 };
            static const char *axes[] = { "Speed", "Size", "Polish", "Docs", "Platforms", "Tests" };
            cui_series rs[] = { { "1.0", a, 6, { 0 } }, { "1.1", b, 6, { 0 } } };
            cui_radar_chart(ui, "radar", rs, 2, axes, 6, .size = 220, .max = 100);
        }
    }
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Funnel", "Sign up conversion.");
            static const cui_viz_item stages[] = { { "Visited", 12840 }, { "Signed up", 4210 }, { "Activated", 2380 }, { "Subscribed", 860 } };
            cui_funnel(ui, "funnel", stages, 4);
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Split bar", "Where the time goes.");
            static const cui_viz_item parts[] = { { "Layout", 38 }, { "Text", 24 }, { "Paint", 21 }, { "Input", 9 }, { "Other", 8 } };
            cui_split_bar(ui, "split", parts, 5, .format = "%.0f ms");
            cui_gap(ui, 18);
            section(ui, "Waffle", "Platforms of the last 100 installs.");
            static const cui_viz_item plats[] = { { "Windows", 46 }, { "macOS", 31 }, { "Linux", 23 } };
            cui_waffle(ui, "waffle", plats, 3, .cell = 14);
        }
    }
    section(ui, "Sankey", "Where sign ups come from and where they end up.");
    static const char *nodes[] = { "Search", "Social", "Referral", "Sign up", "Bounced", "Trial", "Paid", "Churned" };
    static const cui_sankey_link links[] = {
        { 0, 3, 420 }, { 1, 3, 260 }, { 2, 3, 180 }, { 0, 4, 300 }, { 1, 4, 220 },
        { 3, 5, 860 }, { 5, 6, 580 }, { 5, 7, 280 },
    };
    cui_card(ui, .pad = 18, .width = CUI_FILL) { cui_sankey(ui, "flow", nodes, 8, links, 8, .height = 280); }
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1.4f), .gap = 10) {
            section(ui, "Bar list", "Ranked, keyed by label: press Shuffle and the rows slide to their new rank.");
            static cui_viz_item pages[] = { { "/docs", 4820 }, { "/", 3920 }, { "/gallery", 2410 }, { "/blog/fast-ui-in-c", 1830 }, { "/download", 1210 }, { "/pricing", 640 } };
            cui_bar_list(ui, "pages", pages, 6);
            if (cui_button(ui, "Shuffle", .size = CUI_SIZE_SM, .icon = CUI_ICON_SHUFFLE, .variant = CUI_BUTTON_OUTLINE))
                for (int k = 0; k < 6; k++) pages[k].value = (float)(500 + rand() % 5000);
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10, .align = CUI_ALIGN_CENTER) {
            section(ui, "Activity rings", "Past one, a ring keeps winding.");
            static const cui_viz_item rings[] = { { "Move", 0.82f }, { "Exercise", 1.24f }, { "Stand", 0.58f } };
            cui_activity_rings(ui, "rings", rings, 3, .size = 170, .center = "82%", .caption = "of goal");
        }
    }
    section(ui, "Scatter", "Frame time against node count, sized by draw calls.");
    {
        static cui_vec2 a[40], b[40];
        static float sa[40], sb[40];
        static bool pinit;
        if (!pinit) {
            for (int k = 0; k < 40; k++) {
                float n = 200 + k * 95.0f;
                a[k] = (cui_vec2){ n, 0.08f + n * 0.00006f + (float)((k * 37) % 13) * 0.004f };
                b[k] = (cui_vec2){ n * 0.9f + 150, 0.15f + n * 0.00011f + (float)((k * 53) % 17) * 0.006f };
                sa[k] = (float)(10 + (k * 7) % 40); sb[k] = (float)(20 + (k * 11) % 50);
            }
            pinit = true;
        }
        cui_scatter_series ss[] = { { "Software", a, sa, 40, { 0 } }, { "OpenGL", b, sb, 40, { 0 } } };
        cui_card(ui, .pad = 18, .width = CUI_FILL) {
            cui_scatter_chart(ui, "scatter", ss, 2, .height = 300, .x_label = "Nodes", .y_label = "Frame time (ms)", .y_format = "%.2f");
        }
        static const cui_gauge_band bands[] = { { 60, CUI_TONE_SUCCESS, "Normal" }, { 85, CUI_TONE_WARNING, "Busy" }, { 100, CUI_TONE_DANGER, "Overloaded" } };
        cui_card(ui, .pad = 18, .width = CUI_FILL, .dir = CUI_ROW, .wrap = true, .gap = 24, .align = CUI_ALIGN_CENTER) {
            cui_radial_gauge(ui, "cpu", 64, 0, 100, .unit = "%", .label = "CPU", .bands = bands, .band_count = 3, .size = 180);
            cui_radial_gauge(ui, "temp", 71, 20, 100, .unit = "C", .label = "GPU temperature", .show_target = true, .target = 85, .size = 180);
            cui_radial_gauge(ui, "mem", 0.62f * 32, 0, 32, .format = "%.1f", .unit = "GB", .label = "Memory", .style = CUI_GAUGE_RING, .detail = "of 32 GB", .size = 150);
        }
        enum { NC = 2000 };
        static cui_candle candles[NC];
        static bool cinit;
        if (!cinit) {
            float price = 64;
            for (int k = 0; k < NC; k++) {
                float o = price, c = o * (1 + sinf(k * 0.37f) * 0.012f + cosf(k * 0.043f) * 0.008f);
                candles[k] = (cui_candle){ o, fmaxf(o, c) * 1.006f, fminf(o, c) * 0.994f, c, 2000 + (float)((k * 7919) % 900) * 10 };
                price = c;
            }
            cinit = true;
        }
        cui_card(ui, .pad = 18, .width = CUI_FILL, .gap = 10) {
            cui_label(ui, "Price, 2,000 periods");
            cui_candlestick(ui, "price", candles, NC, .height = 280);
        }
    }
}

/* ---- motion -------------------------------------------------------------- */

static void page_motion(cui_ctx *ui) {
    static int replay;
    static float peaks[96];
    static float progress = 0.36f;
    static bool wifi = true, bt = false, focus = true, airdrop = false, dark_tile = true;
    static int tab = 0;
    static cui_now_playing np = { "Low Tide", "Marlow Avenue", 0, { 0xc8, 0x55, 0x3d, 255 }, 76, 222, true };
    if (!peaks[0]) for (int k = 0; k < 96; k++) peaks[k] = 0.18f + 0.8f * fabsf(sinf(k * 0.37f) * sinf(k * 0.11f + 1.3f));
    section(ui, "Text effects", "A shimmer for pending work, a typewriter for streamed text, words that settle into place.");
    cui_card(ui, .gap = 18, .pad = 20, .width = CUI_FILL) {
        cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER) {
            cui_loader(ui, .style = CUI_LOADER_DOTS, .size = 22);
            cui_shimmer_text(ui, "Thinking about your question\xe2\x80\xa6", .size = 15);
        }
        cui_push_id_int(ui, replay);
        cui_typewriter(ui, "tw", "CUI draws a full frame in a fraction of a millisecond, so text can stream in as fast as it arrives.", .caret = true, .size = 15);
        cui_text_reveal(ui, "rv", "The release build finished in 3 minutes 42 seconds.", .size = 20, .font = CUI_FONT_DISPLAY);
        cui_pop_id(ui);
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Deploying to", .font = CUI_FONT_DISPLAY, .size = 26);
            static const char *words[] = { "staging.", "eu-west.", "us-east.", "all regions." };
            cui_word_rotate(ui, "rot", words, 4, 2.2f, .font = CUI_FONT_DISPLAY, .size = 26);
        }
        if (cui_button(ui, "Replay", .size = CUI_SIZE_SM, .icon = CUI_ICON_ROTATE_CCW, .variant = CUI_BUTTON_OUTLINE)) replay++;
    }
    section(ui, "Gradient text and spotlight cards", "A light follows the pointer across a card and along its edge. A beam marks the one card that is live.");
    cui_gradient_text(ui, "Kiln 2.4", .font = CUI_FONT_DISPLAY, .size = 34, .animate = true);
    cui_row(ui, .gap = 14, .align = CUI_ALIGN_STRETCH, .width = CUI_FILL) {
        static const char *titles[] = { "Software", "OpenGL 3.3", "Your own" };
        static const char *blurbs[] = { "Plain C into a pixel buffer. No GPU needed.", "The default on desktop. Deploying now.", "The core emits triangles; bring any GPU API." };
        for (int k = 0; k < 3; k++) {
            cui_push_id_int(ui, k);
            cui_spotlight(ui, "spot", .width = CUI_GROW(1), .beam = k == 1) {
                cui_text(ui, titles[k], .font = CUI_FONT_MEDIUM, .size = 15);
                cui_text(ui, blurbs[k], .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
            }
            cui_pop_id(ui);
        }
    }
    section(ui, "Loaders", "Dots, bars, pulse, orbit and grid.");
    cui_row(ui, .gap = 36, .align = CUI_ALIGN_CENTER) {
        for (int k = 0; k < 5; k++) cui_loader(ui, .style = (cui_loader_style)k, .size = 28);
    }
    section(ui, "Dock", "Icons grow as the pointer passes.");
    cui_row(ui, .width = CUI_FILL, .justify = CUI_JUSTIFY_CENTER, .height = CUI_PX(110), .align = CUI_ALIGN_END) {
        cui_dock(ui, "dock") {
            if (cui_dock_item(ui, CUI_ICON_FOLDER, "Files", .color = CUI_RGB(0x3b82f6), .running = true)) cui_toast(ui, "Files");
            cui_dock_item(ui, CUI_ICON_MAIL, "Mail", .color = CUI_RGB(0x0ea5e9), .badge = 3, .running = true);
            cui_dock_item(ui, CUI_ICON_CALENDAR, "Calendar", .color = CUI_RGB(0xef4444));
            cui_dock_item(ui, CUI_ICON_MUSIC, "Music", .color = CUI_RGB(0xec4899), .running = true);
            cui_dock_item(ui, CUI_ICON_SQUARE_TERMINAL, "Terminal", .color = CUI_RGB(0x27272a));
            cui_dock_item(ui, CUI_ICON_PALETTE, "Design", .color = CUI_RGB(0x8b5cf6));
            cui_dock_separator(ui);
            cui_dock_item(ui, CUI_ICON_SETTINGS, "Settings");
            cui_dock_item(ui, CUI_ICON_TRASH_2, "Trash");
        }
    }
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Control tiles", "Big toggles for quick settings.");
            cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 10, .width = CUI_PX(300)) {
                cui_control_tile(ui, "Wi-Fi", &wifi, .icon = CUI_ICON_WIFI, .detail = wifi ? "Home" : "Off");
                cui_control_tile(ui, "Hotspot", &bt, .icon = CUI_ICON_RADIO);
                cui_control_tile(ui, "Focus", &focus, .icon = CUI_ICON_MOON, .detail = focus ? "Until 6 pm" : "Off");
                cui_control_tile(ui, "AirShare", &airdrop, .icon = CUI_ICON_SHARE_2, .detail = airdrop ? "Everyone" : "Contacts only");
            }
            cui_control_tile(ui, "Dark appearance", &dark_tile, .icon = CUI_ICON_SUN, .wide = true, .width = CUI_PX(300));
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 10) {
            section(ui, "Now playing", "Artwork, a scrubber and transport.");
            int ev = cui_now_playing(ui, "np", &np);
            if (ev & CUI_PLAYER_NEXT) cui_toast(ui, "Next track");
            if (np.playing) { np.position += cui_delta_time(ui); if (np.position > np.duration) np.position = 0; }
            section(ui, "Waveform", "Click or drag to seek.");
            cui_card(ui, .pad = 14, .gap = 8, .width = CUI_PX(360)) {
                cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                    cui_icon_button(ui, CUI_ICON_PLAY, .size = CUI_SIZE_SM, .variant = CUI_BUTTON_SECONDARY);
                    cui_box(ui, .width = CUI_FILL) { cui_waveform(ui, "wave", peaks, 96, &progress, .height = 34); }
                    cui_text(ui, "0:42", .size = 12, .tabular = true, .color = CUI_COLOR_TEXT_MUTED);
                }
            }
            section(ui, "Tab bar", "For compact layouts.");
            static const cui_tab_bar_item tabs[] = { { CUI_ICON_HOUSE, "Home", 0 }, { CUI_ICON_SEARCH, "Search", 0 }, { CUI_ICON_INBOX, "Inbox", 4 }, { CUI_ICON_USER, "Profile", 0 } };
            cui_tab_bar(ui, "tabs", tabs, 4, &tab, .width = CUI_PX(360));
        }
    }
}

/* ---- AI ------------------------------------------------------------------ */

static void page_ai(cui_ctx *ui) {
    static char draft[512] = "";
    static int model = 0, vote = 0, removed = -1;
    static bool busy = false;
    static double started = -1;
    static const char *files[] = { "quarterly-report.pdf", "metrics.csv" };
    static int file_count = 2;
    static const cui_model models[] = {
        { "Atlas", "Longer answers, reads files and code", "New", CUI_ICON_LAYERS },
        { "Atlas Fast", "Short answers in about a second", NULL, CUI_ICON_ZAP },
        { "Atlas Mini", "Runs on your own machine", NULL, CUI_ICON_BOX },
    };
    static const cui_source sources[] = {
        { "Immediate mode GUIs and why they keep coming back", "https://fieldnotes.dev/immediate-mode", "A look at retained and immediate mode toolkits and the trade offs between them." },
        { "Notes on subpixel text positioning", "https://glyphlab.io/notes/subpixel", "Shaping, hinting and subpixel positioning explained with examples." },
        { "Spring physics for interface motion", "https://motionbook.dev/springs", "Why spring physics feel more natural than cubic beziers for interface motion." },
    };
    double now = cui_time(ui);
    if (busy && started < 0) started = now;
    if (!busy) started = -1;
    float elapsed = busy ? (float)(now - started) : 0;
    if (busy && elapsed > 9) busy = false;
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_column(ui, .width = CUI_GROW(1.6f), .gap = 14) {
            section(ui, "Conversation", "Reasoning, tool calls, an answer with sources, and the composer.");
            cui_chat_message(ui, "Why do immediate mode toolkits feel so fast? Cite a few sources.", .mine = true);
            cui_reasoning(ui, "think", .thinking = busy, .seconds = busy ? elapsed : 8) {
                cui_text(ui, "The question is about perceived speed. Two parts: how little work a frame does, and how quickly input shows up on screen.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
                cui_text(ui, "Look for sources on retained versus immediate mode, text rendering cost and motion.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
            }
            cui_tool_call(ui, "search", "web_search", .icon = CUI_ICON_GLOBE, .detail = "immediate mode gui performance", .status = CUI_TASK_DONE, .seconds = 1.4f,
                          .input = "{\"query\": \"immediate mode gui performance\", \"limit\": 5}",
                          .output = "{\"results\": [{\"title\": \"Immediate mode GUIs\", \"url\": \"https://fieldnotes.dev/immediate-mode\"}], \"took_ms\": 1380}");
            cui_tool_call(ui, "read", "read_file", .icon = CUI_ICON_FILE_TEXT, .detail = "quarterly-report.pdf", .status = busy ? CUI_TASK_RUNNING : CUI_TASK_DONE, .seconds = busy ? elapsed : 2.1f);
            if (busy) cui_agent_status(ui, "Reading quarterly-report.pdf", .status = CUI_TASK_RUNNING, .seconds = elapsed);
            cui_typewriter(ui, "answer", "They do very little work per frame: the interface is rebuilt from your data, laid out once and drawn in a handful of batches, so there is no tree of objects to keep in sync. Text is the expensive part, which is why glyphs are cached in an atlas, and motion feels quick when it is driven by springs instead of fixed curves.", .size = 15);
            cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER) {
                cui_text(ui, "Sources", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
                for (int k = 0; k < 3; k++) cui_citation(ui, k + 1, &sources[k]);
            }
            cui_source_list(ui, "sources", sources, 3);
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                int ra = cui_response_actions(ui, "actions", .copy_text = "They do very little work per frame.", .vote = &vote);
                if (ra & CUI_RESPONSE_REGENERATE) busy = true;
                cui_spacer(ui);
                static const cui_viz_item parts[] = { { "System", 3200 }, { "Files", 41800 }, { "Messages", 68400 } };
                cui_context_meter(ui, "ctx", 113400, 200000, .parts = parts, .part_count = 3);
            }
            int ev = cui_chat_composer(ui, "composer", draft, sizeof draft, .placeholder = "Ask a follow up", .busy = busy,
                                       .attachments = files, .attachment_count = file_count, .removed = &removed,
                                       .models = models, .model_count = 3, .model = &model);
            if (ev & CUI_COMPOSER_SEND) { draft[0] = 0; busy = true; }
            if (ev & CUI_COMPOSER_STOP) busy = false;
            if ((ev & CUI_COMPOSER_REMOVE) && removed >= 0 && removed < file_count) {
                for (int k = removed; k < file_count - 1; k++) files[k] = files[k + 1];
                file_count--;
            }
            if (ev & CUI_COMPOSER_ATTACH) cui_toast(ui, "Attach files", .description = "Drop files on the window, too.");
        }
        cui_column(ui, .width = CUI_GROW(1), .gap = 14) {
            section(ui, "Agent plan", "Steps and their status.");
            cui_plan_step steps[] = {
                { "Search for sources", "3 results", CUI_TASK_DONE },
                { "Read the attached report", busy ? "Page 12 of 40" : "40 pages", busy ? CUI_TASK_RUNNING : CUI_TASK_DONE },
                { "Compare the numbers with last quarter", NULL, busy ? CUI_TASK_PENDING : CUI_TASK_DONE },
                { "Draft the summary", NULL, busy ? CUI_TASK_PENDING : CUI_TASK_RUNNING },
                { "Check the citations", NULL, CUI_TASK_PENDING },
            };
            cui_agent_plan(ui, "plan", steps, 5, .title = "Research plan");
            section(ui, "Suggestions", "Ways to start.");
            static const cui_prompt_suggestion sugg[] = {
                { CUI_ICON_CODE, "Explain this code", "Walk through what it does" },
                { CUI_ICON_CHART_BAR, "Analyze a dataset", "Find trends and outliers" },
                { CUI_ICON_PENCIL, "Draft an email", "Clear and friendly" },
                { CUI_ICON_BUG, "Debug an error", "Paste the stack trace" },
            };
            int sg = cui_prompt_suggestions(ui, "sugg", sugg, 4, .columns = 1);
            if (sg >= 0) snprintf(draft, sizeof draft, "%s: ", sugg[sg].title);
        }
    }
}

/* ---- audio ---------------------------------------------------------------- */

static void page_audio(cui_ctx *ui) {
    static float cutoff = 1800, res = 0.3f, attack = 12, pan = 0.15f, faders[6] = { -4, -9, 0, -15, -6, -2 };
    static int tick;
    tick++;
    static const char *names[6] = { "Kick", "Snare", "Bass", "Keys", "Vocals", "FX" };
    static const float marks[] = { 6, 0, -6, -12, -24, -48 };
    float lv[6][2];
    for (int k = 0; k < 6; k++)
        for (int c = 0; c < 2; c++) {
            float t = (tick + c * 5) / 60.0f, beat = fmodf(t * (1.8f + 0.2f * k) + k * 0.31f, 1.0f);
            lv[k][c] = (0.3f + 0.1f * k) * (0.2f + 0.8f * expf(-beat * (2.5f + k)));
        }
    static int tour = -1;
    static cui_tour_step steps[3] = {
        { 0, "Turn a knob", "Drag up and down. Hold Shift for fine steps, double-click to reset.", CUI_PLACE_BOTTOM_START },
        { 0, "Watch the levels", "Bars fall back after each peak; the mark above them holds the peak a moment.", CUI_PLACE_TOP_START },
        { 0, "Ride the faders", "Click a fader, then use the wheel or the arrow keys.", CUI_PLACE_LEFT_START },
    };
    if (cui_button(ui, "Take the tour", .icon = CUI_ICON_SPARKLES, .variant = CUI_BUTTON_OUTLINE)) tour = 0;
    cui_row(ui, .gap = 16, .wrap = true, .align = CUI_ALIGN_START) {
        cui_card(ui, .gap = 16, .min_width = 280) {
            cui_heading(ui, "Synth", 4);
            cui_row(ui, .gap = 14, .wrap = true) {
                cui_knob(ui, "Cutoff", &cutoff, 20, 20000, .unit = "Hz", .log_scale = true, .reset = 1000);
                steps[0].target = cui_last_id(ui);
                cui_knob(ui, "Resonance", &res, 0, 1, .format = "%.2f");
                cui_knob(ui, "Attack", &attack, 1, 2000, .unit = "ms", .log_scale = true, .reset = 10);
                cui_knob(ui, "Pan", &pan, -1, 1, .bipolar = true, .format = "%+.2f");
            }
            float master[2] = { lv[0][0] * 1.4f, lv[4][1] * 1.3f };
            cui_level_meter(ui, "master", master, 2, .horizontal = true, .length = 240, .scale = true, .segmented = true);
            steps[1].target = cui_last_id(ui);
        }
        cui_card(ui, .dir = CUI_ROW, .gap = 8, .wrap = true) {
            for (int k = 0; k < 6; k++) {
                cui_push_id_int(ui, k);
                cui_fader(ui, names[k], &faders[k], -48, 6, .unit = "dB", .length = 180, .ticks = k == 0 ? marks : NULL,
                          .tick_count = k == 0 ? 6 : 0, .meter = lv[k], .meter_channels = 2);
                if (k == 2) steps[2].target = cui_last_id(ui);
                cui_pop_id(ui);
            }
        }
    }
    if (tour >= 0) cui_tour(ui, "audio-tour", steps, 3, &tour);
}

/* ---- files ---------------------------------------------------------------- */

/* the local disk, or a few made-up files where there is none (the browser build) */
static int files_list(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden) {
    int n = cui_fs_list(user, path, out, cap, names, names_cap, hidden);
    if (n >= 0 || strcmp(path, ".")) return n;
    static const char *demo[] = { "assets", "src", "README.md", "notes.txt", "cover.png", "report.pdf" };
    for (int k = 0; k < 6 && k < cap; k++) out[k] = (cui_file_entry){ demo[k], k < 2, (uint64_t)(k * 13100), 1730000000 + k * 86400 };
    return 6;
}

static void page_files(cui_ctx *ui) {
    static char folder[1024] = ".";
    static bool open_dlg, save_dlg;
    static char chosen[1024];
    section(ui, "File browser", "Double-click to open; type to jump; Backspace goes up.");
    cui_file_browser_result r = cui_file_browser(ui, "files", folder, sizeof folder, .list = files_list, .height = CUI_PX(380));
    if (r.picked) snprintf(chosen, sizeof chosen, "%s", r.picked);
    cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
        if (cui_button(ui, "Open\xe2\x80\xa6", .icon = CUI_ICON_FOLDER_OPEN)) open_dlg = true;
        if (cui_button(ui, "Save as\xe2\x80\xa6", .icon = CUI_ICON_DOWNLOAD)) save_dlg = true;
        if (chosen[0]) cui_muted(ui, chosen);
    }
    cui_file_picker(ui, "open", &open_dlg, chosen, sizeof chosen, .list = files_list);
    cui_file_picker(ui, "save", &save_dlg, chosen, sizeof chosen, .list = files_list, .save = true, .name = "untitled.txt");

    /* a small shell over the same folder */
    static cui_terminal_buffer term;
    static char term_text[64 * 1024];
    static cui_terminal_line term_lines[1024];
    if (!term.line_cap) {
        cui_terminal_init(&term, term_text, sizeof term_text, term_lines, 1024);
        cui_terminal_write(&term, "Type \x1b[1mhelp\x1b[0m for the commands.\n", -1);
    }
    section(ui, "Terminal", "Colors, \\r progress, history on Up and Down, Ctrl+C, Ctrl+L, drag to select.");
    cui_terminal_result tr = cui_terminal(ui, "shell", &term, .height = 300, .prompt = "> ");
    if (tr.command) {
        const char *c = tr.command;
        if (!strcmp(c, "help")) cui_terminal_write(&term, "  \x1b[36mls\x1b[0m      the folder above\n  \x1b[36mcolors\x1b[0m  the 16 colors\n  \x1b[36mbuild\x1b[0m   a progress bar\n  \x1b[36mecho\x1b[0m    prints its words\n  \x1b[36mclear\x1b[0m   empties the screen\n", -1);
        else if (!strcmp(c, "ls")) {
            static cui_file_entry ents[256];
            static char names[16384];
            int n = files_list(NULL, folder, ents, 256, names, sizeof names, false);
            for (int k = 0; k < n; k++) cui_terminal_printf(&term, ents[k].dir ? "\x1b[1;34m%s/\x1b[0m\n" : "%s\n", ents[k].name);
            if (n < 0) cui_terminal_write(&term, "\x1b[31mls: cannot read this folder\x1b[0m\n", -1);
        } else if (!strcmp(c, "colors")) {
            for (int k = 0; k < 16; k++) cui_terminal_printf(&term, "\x1b[%dm\xe2\x96\x88\xe2\x96\x88\x1b[0m%s", k < 8 ? 30 + k : 82 + k, k == 7 || k == 15 ? "\n" : " ");
        } else if (!strcmp(c, "build")) {
            for (int k = 0; k <= 20; k++) {
                char bar[96];
                int m = 0;
                for (int j = 0; j < 20; j++) m += snprintf(bar + m, sizeof bar - m, "%s", j < k ? "\xe2\x96\x88" : "\xe2\x96\x91");
                cui_terminal_printf(&term, "\r\x1b[32m%s\x1b[0m %3d%%", bar, k * 5);
            }
            cui_terminal_write(&term, "\n\x1b[32mdone\x1b[0m\n", -1);
        } else if (!strncmp(c, "echo", 4) && (c[4] == ' ' || !c[4])) cui_terminal_printf(&term, "%s\n", c[4] ? c + 5 : "");
        else if (!strcmp(c, "clear")) cui_terminal_clear(&term);
        else if (c[0]) cui_terminal_printf(&term, "\x1b[31m%s: command not found\x1b[0m\n", c);
    }
    if (tr.interrupt) cui_toast(ui, "Interrupted");
}

/* ---- developer tools ----------------------------------------------------- */

static cui_kv_pair kv_pairs[12];
static int kv_n = -1;
static void devtools_kv(cui_ctx *ui) {
    if (kv_n < 0) {
        kv_n = 4;
        snprintf(kv_pairs[0].key, 64, "Authorization"); snprintf(kv_pairs[0].value, 256, "Bearer 4f9c2e81d0b7a6"); kv_pairs[0].enabled = true; kv_pairs[0].secret = true;
        snprintf(kv_pairs[1].key, 64, "Accept"); snprintf(kv_pairs[1].value, 256, "application/json"); kv_pairs[1].enabled = true;
        snprintf(kv_pairs[2].key, 64, "X-Request-Id"); snprintf(kv_pairs[2].value, 256, "7d1c0a"); kv_pairs[2].enabled = false;
        snprintf(kv_pairs[3].key, 64, "Accept"); snprintf(kv_pairs[3].value, 256, "text/html"); kv_pairs[3].enabled = true;
    }
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 680) {
        cui_text(ui, "Headers", .font = CUI_FONT_MEDIUM);
        cui_kv_editor(ui, "headers", kv_pairs, &kv_n, 12);
    }
}

static const cui_diagnostic pbm_diags[] = {
    { CUI_SEVERITY_ERROR, "use of undeclared identifier 'frame_time'", "src/render/timing.c", 42, 17, "clang", "undeclared-var" },
    { CUI_SEVERITY_WARNING, "unused variable 'scale'", "src/render/timing.c", 88, 11, "clang", "-Wunused-variable" },
    { CUI_SEVERITY_ERROR, "incompatible pointer types passing 'float *' to 'double *'", "src/core/layout.c", 210, 33, "clang", "-Wincompatible-pointer-types" },
    { CUI_SEVERITY_WARNING, "implicit conversion loses floating-point precision", "src/core/layout.c", 318, 22, "clang", "-Wfloat-conversion" },
    { CUI_SEVERITY_INFO, "function 'measure' could be declared static", "src/core/layout.c", 402, 6, "clang-tidy", "misc-use-internal-linkage" },
    { CUI_SEVERITY_HINT, "loop variable copied on each iteration; consider a pointer", "src/app/main.c", 51, 9, "clang-tidy", "performance-for-range-copy" },
    { CUI_SEVERITY_WARNING, "comparison of integers of different signs", "src/app/main.c", 77, 18, "gcc", "-Wsign-compare" },
};
static void devtools_problems(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_problems(ui, "problems", pbm_diags, (int)(sizeof pbm_diags / sizeof pbm_diags[0]), .height = 300);
    }
}

static int pl_sel = 3;
static void devtools_pipeline(cui_ctx *ui) {
    static const char *stages_en[] = { "Build", "Test", "Package", "Deploy" };
        static const cui_pipeline_job jobs[] = {
        { "linux-x64", 0, CUI_JOB_PASSED, 142 }, { "macos-arm64", 0, CUI_JOB_PASSED, 201 }, { "windows-x64", 0, CUI_JOB_PASSED, 236 },
        { "unit", 1, CUI_JOB_PASSED, 64 }, { "integration", 1, CUI_JOB_RUNNING, 95 }, { "lint", 1, CUI_JOB_FAILED, 12, true },
        { "docker image", 2, CUI_JOB_PENDING, -1 }, { "release notes", 2, CUI_JOB_SKIPPED, -1 },
        { "staging", 3, CUI_JOB_PENDING, -1 }, { "production", 3, CUI_JOB_PENDING, -1 },
    };
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 920) {
        cui_pipeline(ui, "run", stages_en, 4, jobs, (int)(sizeof jobs / sizeof jobs[0]), .selected = &pl_sel);
    }
}

static const char *const qb_status[] = { "Open", "In progress", "Closed" };
static cui_query_field qb_fields[] = {
    { "Status", CUI_QUERY_CHOICE, qb_status, 3 }, { "Assignee", CUI_QUERY_TEXT }, { "Priority", CUI_QUERY_NUMBER }, { "Has PR", CUI_QUERY_BOOL },
};
static cui_query_node qb_nodes[16] = {
    { 0, true, false }, { 1, false, false, 0, 0, "Open" }, { 1, true, true }, { 2, false, false, 1, 0, "me" }, { 2, false, false, 2, 2, "2" },
};
static int qb_n = 5;
static void devtools_query(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 720) {
        cui_query_builder(ui, "filter", qb_fields, 4, qb_nodes, &qb_n, 16);
        char text[256];
        cui_query_text(qb_fields, 4, qb_nodes, qb_n, text, sizeof text);
        cui_text(ui, text, .size = 12, .color = CUI_COLOR_TEXT_MUTED);
    }
}

static char sc_cron[64] = "30 9 * * 1-5";
static void devtools_schedule(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 560) {
        cui_schedule_picker(ui, "backup", sc_cron, sizeof sc_cron, .now = 1794211200, .utc = true, .next_count = 3);
    }
}

static int km_mods = CUI_MOD_PRIMARY;
static void devtools_keymap(cui_ctx *ui) {
    static const cui_keymap_binding binds[] = {
        { { CUI_MOD_PRIMARY, CUI_KEY_S }, "Save", 0 }, { { CUI_MOD_PRIMARY, CUI_KEY_O }, "Open", 0 }, { { CUI_MOD_PRIMARY, CUI_KEY_N }, "New file", 0 },
        { { CUI_MOD_PRIMARY, CUI_KEY_W }, "Close tab", 0 }, { { CUI_MOD_PRIMARY, CUI_KEY_P }, "Go to file", 1 }, { { CUI_MOD_PRIMARY, CUI_KEY_G }, "Go to line", 1 },
        { { CUI_MOD_PRIMARY, CUI_KEY_F }, "Find", 2 }, { { CUI_MOD_PRIMARY, CUI_KEY_H }, "Replace", 2 }, { { CUI_MOD_PRIMARY, CUI_KEY_Z }, "Undo", 2 },
        { { CUI_MOD_PRIMARY, CUI_KEY_Y }, "Redo", 2 }, { { CUI_MOD_PRIMARY, CUI_KEY_B }, "Build", 3 }, { { CUI_MOD_PRIMARY, CUI_KEY_R }, "Run", 3 },
        { { CUI_MOD_PRIMARY, CUI_KEY_D }, "Debug", 3 }, { { CUI_MOD_PRIMARY, CUI_KEY_SLASH }, "Comment", 2 }, { { CUI_MOD_PRIMARY, CUI_KEY_E }, "Recent files", 1 },
        { { CUI_MOD_PRIMARY, CUI_KEY_R }, "Rename symbol", 2 },
        { { 0, CUI_KEY_F5 }, "Continue", 3 }, { { 0, CUI_KEY_F9 }, "Breakpoint", 3 }, { { 0, CUI_KEY_F10 }, "Step over", 3 }, { { 0, CUI_KEY_F11 }, "Step into", 3 },
        { { 0, CUI_KEY_F2 }, "Rename", 2 }, { { 0, CUI_KEY_F12 }, "Definition", 1 },
        { { CUI_MOD_PRIMARY | CUI_MOD_SHIFT, CUI_KEY_P }, "Commands", 1 }, { { CUI_MOD_PRIMARY | CUI_MOD_SHIFT, CUI_KEY_F }, "Find in files", 2 },
    };
    static const char *cats_en[] = { "File", "Navigate", "Edit", "Run and debug" };
        cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 900) {
        cui_keymap(ui, "keys", binds, (int)(sizeof binds / sizeof binds[0]), .categories = cats_en, .category_count = 4, .mods = &km_mods);
    }
}

static const cui_commit cg_commits[] = {
    { "a91f3c2e", "Merge branch 'feature/trace-viewer'", "Maya Okafor", "2 hours ago", "main, origin/main", { 1, 4 } },
    { "7be04d19", "Docs: the trace viewer and the waterfall", "Maya Okafor", "3 hours ago", NULL, { 2, -1 } },
    { "c3d8a7f0", "Fix the ruler's last tick on narrow panels", "Jonas Berg", "5 hours ago", NULL, { 3, -1 } },
    { "1f22b9e4", "Release 1.2", "Jonas Berg", "yesterday", "tag: v1.2.0", { 6, -1 } },
    { "e8a0c55b", "Trace viewer: measure a range with Shift+drag", "Priya Nair", "yesterday", "feature/trace-viewer", { 5, -1 } },
    { "4d9e1a77", "Trace viewer: lanes, spans and markers", "Priya Nair", "2 days ago", NULL, { 6, -1 } },
    { "90b7f3c1", "Merge pull request #21 from fix/atlas", "Maya Okafor", "3 days ago", NULL, { 7, 8 } },
    { "52ce8d0a", "Faster soft shadows in software", "Lena Fischer", "3 days ago", NULL, { 9, -1 } },
    { "b6f1e2d3", "Glyph atlas: compact after a zoom", "Aiko Tanaka", "4 days ago", "fix/atlas", { 9, -1 } },
    { "0aa4c9e8", "Right to left layout", "Omid Saadat", "5 days ago", NULL, { -1, -1 } },
};
static int cg_sel = 1;
static void devtools_commits(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 920) {
        cui_commit_graph(ui, "history", cg_commits, (int)(sizeof cg_commits / sizeof cg_commits[0]), .selected = &cg_sel, .height = 320);
    }
}

static const cui_graph_node gv_nodes[] = {
    { "app", "executable", CUI_TONE_VIOLET }, { "ui", "libcui.a", CUI_TONE_INFO }, { "net", "http client", CUI_TONE_INFO }, { "storage", "sqlite", CUI_TONE_TEAL },
    { "render", "software, OpenGL", CUI_TONE_INFO }, { "text", "fonts, shaping", CUI_TONE_INFO }, { "tls", "mbedtls", CUI_TONE_WARNING }, { "zlib", NULL, CUI_TONE_NEUTRAL },
    { "platform", "X11, Win32, Cocoa", CUI_TONE_INFO },
};
static const cui_graph_edge gv_edges[] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 4 }, { 1, 5 }, { 1, 8 }, { 2, 6 }, { 2, 7 }, { 3, 7 }, { 5, 7 }, { 4, 8 } };
static int gv_sel = 1;
static void devtools_graph(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 1000) {
        cui_graph_view(ui, "deps", gv_nodes, 9, gv_edges, 11, .selected = &gv_sel, .height = 340);
    }
}

static const cui_net_request wf_reqs[] = {
    { "https://shop.example.com/", "GET", 200, "document", 18400, 0.000, { 0.002, 0.018, 0.024, 0.031, 0.142, 0.012 } },
    { "https://shop.example.com/assets/app.css", "GET", 200, "stylesheet", 42100, 0.232, { 0.004, 0, 0, 0, 0.061, 0.022 } },
    { "https://shop.example.com/assets/app.js", "GET", 200, "script", 186000, 0.233, { 0.005, 0, 0, 0, 0.068, 0.094 } },
    { "https://cdn.example.net/fonts/inter.woff2", "GET", 200, "font", 48200, 0.241, { 0.003, 0.021, 0.019, 0.028, 0.044, 0.018 } },
    { "https://shop.example.com/api/session", "POST", 401, "fetch", 312, 0.402, { 0.002, 0, 0, 0, 0.083, 0.001 } },
    { "https://shop.example.com/api/products?page=1", "GET", 200, "fetch", 9600, 0.404, { 0.002, 0, 0, 0, 0.121, 0.006 } },
    { "https://shop.example.com/img/hero.avif", "GET", 200, "image", 212000, 0.410, { 0.012, 0, 0, 0, 0.052, 0.188 } },
    { "https://shop.example.com/img/p/1042.webp", "GET", 304, "image", 0, 0.538, { 0.006, 0, 0, 0, 0.038, 0 } },
    { "https://shop.example.com/img/p/1043.webp", "GET", 200, "image", 31200, 0.540, { 0.008, 0, 0, 0, 0.041, 0.024 } },
    { "https://analytics.example.org/collect", "POST", 0, "ping", 0, 0.702, { 0.004, 0.026, 0.031, 0.036, 0.12, 0 } },
};
static int wf_sel = 2;
static void devtools_waterfall(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 1000) {
        cui_waterfall(ui, "net", wf_reqs, (int)(sizeof wf_reqs / sizeof wf_reqs[0]), .selected = &wf_sel, .height = 340, .dom_loaded = 0.395, .loaded = 0.68);
    }
}

static cui_trace_track tr_tracks[] = { { "Main thread" }, { "Render" }, { "Worker 1" }, { "Worker 2" }, { "GPU" } };
static cui_trace_span tr_spans[400];
static int tr_n = 0;
static cui_trace_view tr_view = { 0.0042, 0.0418, 0, 0.0161, 0.0236 };
static int tr_sel = -1;
static void tr_add(const char *name, double s, double e, int track, int depth) { if (tr_n < 400) tr_spans[tr_n++] = (cui_trace_span){ name, s, e, track, depth, 0 }; }
static void tr_build(void) {
    for (int f = 0; f < 3; f++) {
        double t = f * 0.0166;
        tr_add("Frame", t + 0.0004, t + 0.0151, 0, 0);
        tr_add("Input", t + 0.0006, t + 0.0019, 0, 1);
        tr_add("Update", t + 0.0021, t + 0.0092, 0, 1);
        tr_add("Physics", t + 0.0023, t + 0.0051, 0, 2);
        tr_add("Animation", t + 0.0053, t + 0.0068, 0, 2);
        tr_add("Scripts", t + 0.0070, t + 0.0090, 0, 2);
        tr_add("Layout", t + 0.0094, t + 0.0121, 0, 1);
        tr_add("Measure", t + 0.0095, t + 0.0108, 0, 2);
        tr_add("Paint", t + 0.0123, t + 0.0149, 0, 1);
        tr_add("Record", t + 0.0124, t + 0.0139, 0, 2);
        tr_add("Submit", t + 0.0152, t + 0.0163, 1, 0);
        tr_add("Upload", t + 0.0153, t + 0.0157, 1, 1);
        tr_add("Encode", t + 0.0157, t + 0.0162, 1, 1);
        tr_add("Decode image", t + 0.0012, t + 0.0074, 2, 0);
        tr_add("Inflate", t + 0.0013, t + 0.0049, 2, 1);
        tr_add("Parse JSON", t + 0.0081, t + 0.0118, 3, 0);
        tr_add("Draw", t + 0.0164, t + 0.0166 + 0.0105, 4, 0);
    }
    cui_trace_sort(tr_spans, tr_n);
}
static const cui_trace_marker tr_marks[] = { { "vsync", 0.0, CUI_TONE_NEUTRAL }, { "vsync", 0.0166, CUI_TONE_NEUTRAL }, { "GC pause", 0.0273, CUI_TONE_WARNING }, { "vsync", 0.0332, CUI_TONE_NEUTRAL } };
static void devtools_trace(cui_ctx *ui) {
    if (!tr_n) tr_build();
    if (tr_sel < 0) for (int i = 0; i < tr_n; i++) if (!strcmp(tr_spans[i].name, "Update") && tr_spans[i].start > 0.016 && tr_spans[i].start < 0.03) tr_sel = i;
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 1000) {
        cui_trace_view(ui, "trace", tr_tracks, 5, tr_spans, tr_n, &tr_view, &tr_sel, .markers = tr_marks, .marker_count = 4, .height = 320);
    }
}

static void page_devtools(cui_ctx *ui) {
    section(ui, "Trace viewer", "Wheel zooms, drag pans, Shift+drag measures, a lane's name folds it.");
    devtools_trace(ui);
    section(ui, "Network waterfall", "Hover a bar for its timing; Up and Down move the selection.");
    devtools_waterfall(ui);
    section(ui, "Dependency graph", "Hover a node to light up its neighbours; wheel zooms, F fits.");
    devtools_graph(ui);
    section(ui, "Commit graph", "Branch lanes, merges and refs; only the rows in view are built.");
    devtools_commits(ui);
    section(ui, "Pipeline", "Stages of jobs; a stage sums up its jobs.");
    devtools_pipeline(ui);
    section(ui, "Problems", "Grouped by file; filter by severity or text, Enter opens.");
    devtools_problems(ui);
    section(ui, "Key-value editor", "Type in the empty row to add a pair; the lock hides a value.");
    devtools_kv(ui);
    section(ui, "Query builder", "Nested all and any groups.");
    devtools_query(ui);
    section(ui, "Schedule picker", "Writes a cron expression and shows the next runs.");
    devtools_schedule(ui);
    section(ui, "Keymap", "Shortcuts on a keyboard; keys bound twice are marked.");
    devtools_keymap(ui);
}

/* ---- more developer tools ------------------------------------------------ */

static char rx_pat[128] = "(?<user>[\\w.]+)@(?<host>[\\w-]+)\\.(com|org)";
static char rx_text[1024] = "Contact maya.okafor@example.com or jonas@cui-lib.org for access.\nOld address: priya@legacy.net (no longer read).\nSupport: help@example.com";
static void devtools2_regex(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_regex_tester(ui, "rx", rx_pat, sizeof rx_pat, rx_text, sizeof rx_text, .height = 110);
    }
}

static const cui_test_item tx_items[] = {
    { "layout", 0 }, { "row distributes extra space", 1, CUI_JOB_PASSED, 0.004f }, { "wraps to the next line", 1, CUI_JOB_PASSED, 0.006f },
    { "mirrors under right to left", 1, CUI_JOB_FAILED, 0.012f, "expected x = 412, got 388 (padding not mirrored)" },
    { "text", 0 }, { "shapes Persian", 1, CUI_JOB_PASSED, 0.210f }, { "ellipsis keeps whole characters", 1, CUI_JOB_PASSED, 0.003f },
    { "bidi brackets", 1, CUI_JOB_SKIPPED, -1 },
    { "renderer", 0 }, { "software", 1 }, { "rounded corners", 2, CUI_JOB_PASSED, 0.031f }, { "soft shadows", 2, CUI_JOB_RUNNING, -1 },
    { "OpenGL", 1 }, { "atlas growth", 2, CUI_JOB_PENDING, -1 }, { "slow gradient path", 2, CUI_JOB_PASSED, 1.42f },
};
static void devtools2_tests(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_test_explorer(ui, "tests", tx_items, (int)(sizeof tx_items / sizeof tx_items[0]), .height = 500);
    }
}

static const char *const st_src1[] = { "function total(cart) {", "  let sum = 0;", "  for (const item of cart.items) {", "    sum += item.price * item.qty;", "  }", "  return sum;" };
static const char *const st_src2[] = { "export function checkout(user) {", "  const cart = carts.get(user.id);", "  const amount = total(cart);", "  return charge(user, amount);" };
static const cui_stack_frame st_frames[] = {
    { "total", "src/cart.js", 3, 29, false, st_src1, 6, 1 },
    { "checkout", "src/checkout.js", 3, 18, false, st_src2, 4, 1 },
    { "handle", "node_modules/express/lib/router/layer.js", 95, 5, true },
    { "next", "node_modules/express/lib/router/route.js", 149, 13, true },
    { "dispatch", "node_modules/express/lib/router/index.js", 284, 15, true },
    { "onRequest", "src/server.js", 41, 9, false },
};
static void devtools2_stack(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_stack_trace(ui, "trace", st_frames, 6, .title = "TypeError", .message = "Cannot read properties of undefined (reading 'items')", .lang = "js");
    }
}

static int rq_method = 1;
static char rq_url[256] = "https://api.example.com/v1/orders?status=open&limit=20";
static cui_kv_pair rq_params[8];
static int rq_n = -1;
static void devtools2_request(cui_ctx *ui) {
    if (rq_n < 0) rq_n = cui_url_params(rq_url, rq_params, 8);
    cui_column(ui, .gap = 12, .width = CUI_FILL, .max_width = 760) {
        cui_request_bar_result r = cui_request_bar(ui, "req", &rq_method, rq_url, sizeof rq_url);
        if (r.changed) rq_n = cui_url_params(rq_url, rq_params, 8);
        if (cui_kv_editor(ui, "params", rq_params, &rq_n, 8)) cui_url_set_params(rq_url, sizeof rq_url, rq_params, rq_n);
        cui_response_status(ui, 201, 0.142, 18400);
    }
}

static float up_api[90], up_web[90], up_db[90], up_cdn[90];
static void devtools2_uptime(cui_ctx *ui) {
    static bool init;
    if (!init) {
        init = true;
        for (int i = 0; i < 90; i++) {
            up_api[i] = 1; up_web[i] = 1; up_db[i] = 1; up_cdn[i] = i < 20 ? -1 : 1;
            if (i % 17 == 5) up_api[i] = 0.9985f;
            if (i == 61) up_api[i] = 0.962f;
            if (i == 44) up_web[i] = 0.93f;
            if (i % 23 == 9) up_db[i] = 0.995f;
            if (i == 88) up_db[i] = 0.97f;
        }
    }
    static const char *names_en[] = { "API", "Website", "Database", "CDN" };
    const cui_uptime_service sv[] = { { names_en[0], up_api, 90 }, { names_en[1], up_web, 90 }, { names_en[2], up_db, 90 }, { names_en[3], up_cdn, 90 } };
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 860) {
        cui_uptime(ui, "status", sv, 4, .last_day = (cui_date){ 2026, 11, 9 });
    }
}

static float th_info[96], th_warn[96], th_err[96];
static double th_s0 = 1794240000 + 3600 * 5, th_s1 = 1794240000 + 3600 * 7.5;
static void devtools2_histogram(cui_ctx *ui) {
    static bool init;
    if (!init) {
        init = true;
        for (int i = 0; i < 96; i++) {
            double x = i / 96.0;
            th_info[i] = (float)(120 + 80 * sin(x * 6.28 * 2) + (i * 37 % 23));
            th_warn[i] = (float)(12 + (i * 13 % 9) + (i > 60 && i < 70 ? 40 : 0));
            th_err[i] = (float)((i * 7 % 5) + (i > 62 && i < 67 ? 30 : 0));
        }
    }
    const cui_histogram_series se[] = { { "Info", th_info, CUI_TONE_INFO }, { "Warning", th_warn, CUI_TONE_WARNING }, { "Error", th_err, CUI_TONE_DANGER } };
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 900) {
        cui_time_histogram(ui, "logs", 1794240000, 900, 96, se, 3, &th_s0, &th_s1, .utc = true, .height = 150);
    }
}

static const cui_coverage_file cv_files[] = {
    { "src/core/layout.c", 1840, 1702, 620, 541, 88, 86 }, { "src/core/state.c", 420, 401, 96, 90, 31, 31 },
    { "src/core/draw.c", 960, 610, 300, 160, 54, 40 }, { "src/text/shape.c", 2210, 1490, 802, 411, 120, 88 },
    { "src/text/atlas.c", 640, 598, 140, 131, 30, 30 }, { "src/render/gl.c", 1300, 410, 260, 60, 70, 22 },
    { "src/render/software.c", 2400, 2210, 900, 799, 140, 136 }, { "src/main.c", 120, 120, 10, 10, 4, 4 },
};
static void devtools2_coverage(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 820) {
        cui_coverage(ui, "cov", cv_files, 8, .height = 372);
    }
}

static const char *const bl_lines[] = {
    "#include \"cui_internal.h\"", "", "/* the width a component asks for */", "static float measure(cui_node *n) {", "    float w = n->pad[1] + n->pad[3];",
    "    for (int c = n->first; c >= 0; c = next(c))", "        w += width_of(c) + n->gap;", "    return w - n->gap;", "}", "",
    "void cui__layout(cui_ctx *ui) {", "    for (int i = 0; i < ui->node_count; i++)", "        measure(&ui->nodes[i]);", "    place(ui, 0);  /* the root */", "}",
};
static const int bl_commit[] = { 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 3, 3, 4, 3, 3 };
static const cui_blame_commit bl_commits[] = {
    { "a1b2c3d4e5", "Maya Okafor", "First layout pass", 1730000000 }, { "b2c3d4e5f6", "Jonas Berg", "Measure: count padding once", 1760000000 },
    { "c3d4e5f6a7", "Priya Nair", "Gaps between children", 1790000000 }, { "d4e5f6a7b8", "Omid Saadat", "Layout entry point", 1793000000 },
    { "e5f6a7b8c9", "Aiko Tanaka", "Measure every node", 1794100000 },
};
static void devtools2_blame(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 900) {
        cui_blame(ui, "blame", bl_lines, bl_commit, 15, bl_commits, 5, .lang = "c", .now = 1794211200, .height = 320, .first_line = 120);
    }
}

static cui_feature_flag ff_flags[] = {
    { "new-checkout", "Single-page checkout with saved cards", { true, true, false }, { 0, 25, 0 } },
    { "dark-mode-default", "Start new accounts in dark mode", { true, false, false } },
    { "search-v2", "Ranked search with typo tolerance", { true, true, true }, { 0, 100, 5 } },
    { "export-pdf", "Export reports as PDF", { false, false, false } },
};
static void devtools2_flags(cui_ctx *ui) {
    static const char *envs[] = { "Development", "Staging", "Production" };
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 860) {
        cui_feature_flags(ui, "flags", ff_flags, 4, envs, 3);
    }
}

static const cui_api_param ax_p1[] = { { "id", "path", "string", true, "The user's id" }, { "fields", "query", "string", false, "Comma separated fields to return" } };
static const cui_api_param ax_p2[] = { { "limit", "query", "integer", false, "At most this many, default 20" }, { "cursor", "query", "string", false, "From a previous page" } };
static const cui_api_endpoint ax_eps[] = {
    { "GET", "/users", "List users", "Users", false, ax_p2, 2 }, { "POST", "/users", "Create a user", "Users" },
    { "GET", "/users/{id}", "Get a user", "Users", false, ax_p1, 2 }, { "PATCH", "/users/{id}", "Update a user", "Users", false, ax_p1, 1 },
    { "DELETE", "/users/{id}", "Delete a user", "Users", false, ax_p1, 1 }, { "GET", "/orders", "List orders", "Orders", false, ax_p2, 2 },
    { "PUT", "/orders/{id}/status", "Set an order's status", "Orders" }, { "GET", "/v1/orders", "List orders (old)", "Orders", true },
};
static int ax_sel = 2;
static void devtools2_api(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 820) {
        cui_api_explorer(ui, "api", ax_eps, 8, .selected = &ax_sel);
    }
}

static void page_devtools2(cui_ctx *ui) {
    section(ui, "Test explorer", "Suites fold; hover a row to run it; filter by status or name.");
    devtools2_tests(ui);
    section(ui, "Stack trace", "Library frames fold; a frame with source opens; copy the trace.");
    devtools2_stack(ui);
    section(ui, "Blame", "Hover the gutter to mark a commit's lines; click it to open the commit.");
    devtools2_blame(ui);
    section(ui, "Coverage", "Folders sum up their files; click a column to sort.");
    devtools2_coverage(ui);
    section(ui, "Regex tester", "Matches and groups are marked as you type.");
    devtools2_regex(ui);
    section(ui, "HTTP request bar", "The parameters table and the URL stay in step.");
    devtools2_request(ui);
    section(ui, "API explorer", "Click an endpoint for its parameters.");
    devtools2_api(ui);
    section(ui, "Feature flags", "A switch per environment; click a share to change it.");
    devtools2_flags(ui);
    section(ui, "Uptime", "One bar per day; hover for the day.");
    devtools2_uptime(ui);
    section(ui, "Time histogram", "Drag to select a range; click to clear it.");
    devtools2_histogram(ui);
}

/* ---- instruments ---------------------------------------------------------- */

/* one dose of caffeine `t` hours after it, in mg: absorbed in about 45 minutes, then a half-life of five hours */
static float instrument_dose(double t) {
    if (t <= 0) return 0;
    double ke = log(2.0) / 5.0, ka = log(2.0) / 0.25;
    return (float)(95 * ka / (ka - ke) * (exp(-ke * t) - exp(-ka * t)));
}

static void page_instruments(cui_ctx *ui) {
    static double cups[8] = { 8, 10.75, 12.25 };
    static int count = 3, selected = 2;
    static float curve[8][97];
    double peak = 0, peak_at = 6, bed = 0;
    for (int k = 0; k < 97; k++) {
        double at = 6 + 24.0 * k / 96, total = 0;
        for (int c = 0; c < count; c++) { curve[c][k] = instrument_dose(at - cups[c]); total += curve[c][k]; }
        if (total > peak) { peak = total; peak_at = at; }
    }
    for (int c = 0; c < count; c++) bed += instrument_dose(23 - cups[c]);
    char peak_text[32], peak_mg[16], peak_time[16], bed_text[32], big[16];
    snprintf(peak_text, sizeof peak_text, "PEAK %.0f MG", peak);
    snprintf(peak_mg, sizeof peak_mg, "%.0f", peak);
    snprintf(peak_time, sizeof peak_time, "%02d:%02d", (int)peak_at, (int)lround((peak_at - (int)peak_at) * 60) % 60);
    snprintf(bed_text, sizeof bed_text, "%.0f MG STILL IN YOU", bed);
    snprintf(big, sizeof big, "%.0f", bed);

    section(ui, "Readouts", "A label, a value with its unit, and a line of context, in the monospaced font.");
    cui_row(ui, .gap = 40, .wrap = true, .align = CUI_ALIGN_END) {
        cui_readout(ui, "AT BED", big, .unit = "MG", .size = CUI_SIZE_LG, .accent = bed > 50, .detail = bed > 50 ? "OVER THE 50 MG LINE" : "UNDER THE 50 MG LINE");
        cui_readout(ui, "PEAK", peak_mg, .unit = "MG", .detail = peak_time);
        cui_readout(ui, "CUPS", count == 1 ? "1" : count == 2 ? "2" : count == 3 ? "3" : count == 4 ? "4" : "5+", .size = CUI_SIZE_SM, .detail = "95 MG EACH");
    }
    section(ui, "Dot-matrix chart and event track", "Drag a cup along the axis, click the chart to add one, double-click a cup to remove it.");
    cui_dot_series series[8];
    for (int c = 0; c < count; c++) series[c] = (cui_dot_series){ curve[c], NULL, c == selected };
    cui_chart_annotation notes[3] = { { CUI_ANNOTATION_Y, 0, 50, "50", NULL, false },
                                      { CUI_ANNOTATION_X, 23, 0, "BED 23:00", bed_text, false },
                                      { CUI_ANNOTATION_X, peak_at, 0, peak_text, peak_time, false } };
    cui_column(ui, .width = CUI_FILL, .gap = 0) {
        cui_dot_chart_result r = cui_dot_chart(ui, "caffeine", series, count, 97, .height = 240, .x_min = 6, .x_max = 30,
                                               .x_units = CUI_AXIS_HOURS, .min = 0, .max = 260, .unit = "MG", .no_axis = true,
                                               .annotations = notes, .annotation_count = count ? 3 : 1);
        if (r.clicked && count < 8) {
            double t = floor(r.x * 4 + 0.5) / 4;
            int at = count++;
            while (at > 0 && cups[at - 1] > t) { cups[at] = cups[at - 1]; at--; }
            cups[at] = t;
            selected = at;
        }
        cui_event_track(ui, "cups", cups, &count, 8, 6, 30, .step = 0.25, .units = CUI_AXIS_HOURS, .label_step = 6, .selected = &selected);
    }
    section(ui, "Dot-matrix display", "Text in a 5 by 7 dot font; a long line can scroll.");
    cui_dot_display(ui, bed > 50 ? "OVER THE LINE AT BEDTIME" : "UNDER THE LINE AT BEDTIME", .pitch = 3, .accent = bed > 50);
}

/* ---- creative controls ----------------------------------------------- */

static void page_creative(cui_ctx *ui) {
    static const char *const tag_names[] = { "Pending", "In progress", "Submitted", "In review", "Success", "Failed", "Expired" };
    static bool tag_gone[7];
    static float coffee = 0.37f, train = 0.8f;
    static float hue = 190, kelvin = 5600, opacity = 0.72f;
    static bool picking;
    static int variations = 3, generated = 0;
    static cui_color palette[5] = {   /* plain initializers: a static array cannot hold CUI_RGB's compound literals in MSVC */
        { 0xee, 0xf0, 0xf5, 255 }, { 0x5c, 0x63, 0x70, 255 }, { 0x37, 0xc6, 0xd0, 255 }, { 0x1f, 0x3a, 0x5f, 255 }, { 0xf2, 0xbd, 0x68, 255 } };
    static bool locked[5];
    static uint32_t palette_seed = 7;
    static int aspect = 1, background = 0, format = 0, saved_as = -1;
    static uint32_t seed = 1234567890u;
    static bool seed_fixed;

    section(ui, "Status tag", "An icon, a label and a tinted fill for each state. Click a small tag to remove it.");
    cui_row(ui, .gap = 8, .wrap = true) {
        for (int k = 0; k < 7; k++) cui_status_tag(ui, tag_names[k], (cui_tag_status)k);
    }
    cui_row(ui, .gap = 6, .wrap = true, .align = CUI_ALIGN_CENTER) {
        bool any = false;
        for (int k = 0; k < 7; k++) {
            if (tag_gone[k]) continue;
            any = true;
            if (cui_status_tag(ui, tag_names[k], (cui_tag_status)k, .size = CUI_SIZE_SM, .removable = true)) tag_gone[k] = true;
        }
        if (!any && cui_button(ui, "Bring them back", .size = CUI_SIZE_SM, .variant = CUI_BUTTON_GHOST)) memset(tag_gone, 0, sizeof tag_gone);
    }

    section(ui, "Ratio slider", "Two named parts with a bar between them. Drag it or use the arrow keys (Shift for steps of ten).");
    cui_column(ui, .gap = 12, .width = CUI_FILL) {
        cui_ratio_slider(ui, "COFFEE", "MILK", &coffee, .tone = CUI_TONE_WARNING);
        cui_ratio_slider(ui, "TRAIN", "TEST", &train, .tone = CUI_TONE_INFO);
    }

    section(ui, "Gradient and step sliders", "The track is the value: a hue, a white balance, an opacity. Step sliders snap to a few stops.");
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 12, .width = CUI_FILL) {
        cui_gradient_slider(ui, "Hue", &hue, 0, 360, .swatch = true);
        if (cui_gradient_slider(ui, "White balance", &kelvin, 2500, 10000, .track = CUI_GRADIENT_TEMPERATURE, .step = 100,
                                .tool = CUI_ICON_TARGET, .tool_tooltip = "Pick from image", .tool_on = picking) & CUI_WELL_ACTION)
            picking = !picking;
        cui_gradient_slider(ui, "Opacity", &opacity, 0, 1, .track = CUI_GRADIENT_OPACITY, .color = CUI_RGB(0x2bb8a6));
        char made[48];
        snprintf(made, sizeof made, "%d \xc2\xb7 generated %d times", variations, generated);
        if (cui_step_slider(ui, "Variations", &variations, 1, 4, .tool = CUI_ICON_REFRESH_CW, .tool_tooltip = "Generate",
                            .value = generated ? made : NULL) & CUI_WELL_ACTION)
            generated++;
    }

    section(ui, "Palette strip", "Click a swatch to lock it, then shuffle the rest.");
    cui_palette_strip(ui, "Color palette", palette, locked, 5, .seed = &palette_seed, .value = "Click a swatch to lock it");

    section(ui, "Aspect ratio, options and seed", "Each ratio drawn as its own shape, a choice with the action it feeds, and a seed that rolls.");
    cui_column(ui, .gap = 12, .width = CUI_FILL) {
        cui_aspect_picker(ui, "Aspect ratio", &aspect, .custom = true);
        cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 12, .width = CUI_FILL) {
            static const char *const bg[] = { "hidden", "visible" };
            static const char *const fmt[] = { "png", "jpg", "webp" };
            static const char *const sizes[] = { "2.4 MB", "640 KB", "410 KB" };
            cui_option_action(ui, "Background", bg, 2, &background, CUI_ICON_SCISSORS, .tooltip = "Cut out", .value = "erase");
            char saved[32];
            snprintf(saved, sizeof saved, "saved, %s", sizes[format]);
            if (cui_option_action(ui, "Save as", fmt, 3, &format, CUI_ICON_DOWNLOAD, .tooltip = "Download",
                                  .value = saved_as == format ? saved : sizes[format]) & CUI_WELL_ACTION)
                saved_as = format;
        }
        cui_seed_field(ui, "Seed", &seed, &seed_fixed, .value = seed_fixed ? "Same image every run" : "Random each run");
    }
}

/* ---- canvas and bars ------------------------------------------------------- */

/* a picture for the result card, made here: a sky, a low sun and soft hills */
static cui_texture canvas_picture(cui_ctx *ui) {
    static cui_ctx *made_for;
    static cui_texture tex;
    if (made_for == ui && tex) return tex;
    enum { W = 288, H = 360 };
    static unsigned char px[W * H * 4];
    static const float hills[3][4] = { { 0.18f, 1.04f, 0.54f, 0.26f }, { 0.92f, 1.08f, 0.53f, 0.26f }, { 0.5f, 1.18f, 0.64f, 0.28f } };
    static const float tint[3][3] = { { 238, 143, 120 }, { 184, 96, 106 }, { 122, 63, 82 } };
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float acc[3] = { 0, 0, 0 };
            for (int s = 0; s < 4; s++) {   /* four samples a pixel, for smooth edges */
                float fx = x + 0.25f + 0.5f * (s & 1), fy = y + 0.25f + 0.5f * (s >> 1), v = fy / H, c[3];
                float k = v < 0.48f ? v / 0.48f : (v - 0.48f) / 0.52f;
                if (v < 0.48f) { c[0] = 118 + 67 * k; c[1] = 169 + 43 * k; c[2] = 236 + 10 * k; }
                else { c[0] = 185 + 57 * k; c[1] = 212 + 5 * k; c[2] = 246 - 43 * k; }
                float dx = fx - W * 0.66f, dy = fy - H * 0.32f, d = sqrtf(dx * dx + dy * dy) / W;
                float sun = d < 0.08f ? 1 : d < 0.24f ? 0.55f * (1 - (d - 0.09f) / 0.15f) : 0;
                if (sun > 1) sun = 1;
                c[0] += (255 - c[0]) * sun; c[1] += (247 - c[1]) * sun; c[2] += (236 - c[2]) * sun;
                for (int h = 0; h < 3; h++) {
                    float ex = (fx / W - hills[h][0]) / hills[h][2], ey = (v - hills[h][1]) / hills[h][3];
                    if (ex * ex + ey * ey < 1) { c[0] = tint[h][0]; c[1] = tint[h][1]; c[2] = tint[h][2]; }
                }
                for (int i = 0; i < 3; i++) acc[i] += c[i] * 0.25f;
            }
            unsigned char *p = &px[(y * W + x) * 4];
            p[0] = (unsigned char)acc[0]; p[1] = (unsigned char)acc[1]; p[2] = (unsigned char)acc[2]; p[3] = 255;
        }
    tex = cui_image_create(ui, W, H, px);
    made_for = ui;
    return tex;
}

static void page_canvas(cui_ctx *ui) {
    section(ui, "Grouped toolbar", "Raised groups in one pill, an inset set of modes and a primary action. Narrow the window and the end folds into the \xe2\x80\x9cmore\xe2\x80\x9d menu.");
    static bool numbers = true, bold, italic;
    static int mode;
    static const char *const modes[] = { CUI_ICON_MOVE, CUI_ICON_SQUARE_PEN, CUI_ICON_TYPE };
    static const char *const mode_names[] = { "Move", "Draw", "Text" };
    cui_toolbar(ui, "canvas-tools", .style = CUI_TOOLBAR_GROUPED) {
        cui_toolbar_group(ui) {
            cui_toolbar_button(ui, CUI_ICON_UNDO_2, .tooltip = "Undo", .shortcut = "Ctrl Z");
            cui_toolbar_button(ui, CUI_ICON_REDO_2, .tooltip = "Redo", .shortcut = "Ctrl Shift Z");
        }
        cui_toolbar_group(ui) {
            cui_toolbar_button(ui, NULL, .label = "123", .tooltip = "Numbers", .toggle = &numbers);
            cui_toolbar_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .toggle = &bold);
            cui_toolbar_button(ui, CUI_ICON_ITALIC, .tooltip = "Italic", .toggle = &italic);
            cui_toolbar_radio(ui, "mode", modes, 3, &mode, .labels = mode_names);
        }
        cui_toolbar_primary(ui, CUI_ICON_SPARKLES, .tooltip = "Generate");
    }

    section(ui, "Format bar", "Color, bold, underline, link, the text style (each previewed at its size), alignment and lists. Delete asks first.");
    static cui_text_format fmt = { CUI_TONE_ROSE, true, false, false, 0, 44, CUI_FORMAT_ALIGN_LEFT, CUI_FORMAT_LIST_NONE };
    static int deleted;
    int fr = cui_format_bar(ui, "format", &fmt);
    if (fr & CUI_FORMAT_DELETE) deleted++;
    if (fr & CUI_FORMAT_LINK) fmt.link = !fmt.link;
    char note[64];
    if (deleted) snprintf(note, sizeof note, "Deleted %d time%s.", deleted, deleted == 1 ? "" : "s");
    cui_muted(ui, deleted ? note : "The bar edits one struct; the app applies it to the selection.");

    section(ui, "Live cursors and canvas controls", "Three people on one canvas: their pointers glide to where they move, one talks, one has a node selected. The rail zooms in steps; click the percentage to reset.");
    static cui_node_view view = { 10, 6, 1 };
    static float nodes[3][2] = { { 36, 64 }, { 36, 206 }, { 330, 124 } };
    static bool grid = true, layers;
    cui_live_user people[3] = {
        { .name = "Paul", .tone = CUI_TONE_WARNING, .speaking = true },
        { .name = "Kate", .tone = CUI_TONE_INFO, .says = "Bumping steps to 40?" },
        { .name = "Mario", .tone = CUI_TONE_ROSE },
    };
    /* the app would send positions from the network; here each person picks a new spot every couple of seconds */
    double now = cui_time(ui);
    for (int k = 0; k < 3; k++) {
        unsigned step = (unsigned)(now / (1.6 + 0.4 * k)), h = step * 2654435761u + (unsigned)k * 40503u;
        people[k].x = 60 + (float)(h % 420);
        people[k].y = 60 + (float)((h >> 12) % 220);
        if (k == 1) people[k].says = (step % 3) ? "Bumping steps to 40?" : NULL;
    }
    cui_request_wakeup(ui, 0.4f);
    cui_canvas_toggle toggles[] = { { CUI_ICON_EYE, "Show grid", &grid }, { CUI_ICON_LAYERS, "Layers", &layers } };
    cui_box(ui, .dir = CUI_STACK, .width = CUI_FILL, .height = CUI_PX(360), .radius = 14, .clip = true) {
        if (cui_node_editor_begin(ui, "canvas-graph", &view, .height = CUI_PX(360), .no_grid = !grid)) {
            static const char *titles[3] = { "Positive", "Negative", "Generate" };
            static const char *bodies[3] = { "A black bear, minimalist style, soft gradients", "No text, no people", "Steps 30 \xc2\xb7 dpm++ 2M" };
            for (int i = 0; i < 3; i++) {
                cui_node_begin(ui, i, &nodes[i][0], &nodes[i][1], titles[i], .width = 184);
                if (i < 2) cui_node_output(ui, i + 1, i == 0 ? "Prompt" : "Avoid", .type = 1);
                else { cui_node_input(ui, 3, "Prompt", .type = 1); cui_node_input(ui, 4, "Avoid", .type = 1); }
                cui_node_content(ui) { cui_text(ui, bodies[i], .size = 12, .color = CUI_COLOR_TEXT_SECONDARY); }
                cui_node_end(ui);
                if (i == 2) cui_selection_ring(ui, cui_color_of(ui, CUI_COLOR_WARNING), .name = "Paul");
            }
            cui_node_link(ui, 1, 1, 3);
            cui_node_link(ui, 2, 2, 4);
            cui_node_editor_end(ui);
        }
        cui_rect bounds = { nodes[0][0], nodes[0][1], nodes[2][0] + 184 - nodes[0][0], 290 };
        cui_canvas_controls(ui, "canvas-zoom", &view, .content = bounds, .toggles = toggles, .toggle_count = 2);
        cui_live_cursors(ui, "canvas-people", people, 3, .view = &view);
        cui_box(ui, .pad = 12) { cui_presence_pill(ui, "canvas-here", people, 3); }
    }

    section(ui, "Result card and selection bar", "A picture with its status and caption, and its actions in a floating toolbar. Tick files and a bar slides up; Esc clears.");
    static bool saved;
    static int scale = 1, type;
    static const char *const scales[] = { "1x", "2x", "4x" }, *const types[] = { "PNG", "JPG", "WEBP" };
    static bool picked[5] = { true, true, true, false, false };
    static const char *names[] = { "hero-dark.png", "hero-light.png", "social-card.png", "icon-512.png", "thumb@2x.png" };
    static const char *sizes[] = { "2.4 MB", "2.3 MB", "812 KB", "96 KB", "140 KB" };
    static const cui_selection_action acts[] = { { CUI_ICON_MOVE, "Move" }, { CUI_ICON_COPY, "Copy" }, { CUI_ICON_TRASH_2, "Delete", CUI_TONE_DANGER } };
    cui_row(ui, .gap = 28, .wrap = true, .align = CUI_ALIGN_START, .width = CUI_FILL, .pad_bottom = 60) {
        cui_column(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .min_width = 360) {   /* room for the whole action bar */
            cui_result_card(ui, "canvas-result", canvas_picture(ui), "Final result", .caption = "Minimal illustration, soft gradients and smooth shapes against a clear sky",
                            .badge = "Done in 4.2 s", .image_w = 288, .image_h = 360, .width = CUI_PX(344));
            cui_toolbar(ui, "canvas-result-actions", .style = CUI_TOOLBAR_FLOATING) {
                cui_toolbar_button(ui, CUI_ICON_MAXIMIZE_2, .tooltip = "Expand");
                cui_toolbar_button(ui, CUI_ICON_BOOKMARK, .tooltip = "Save", .toggle = &saved, .tone = CUI_TONE_WARNING);
                cui_toolbar_button(ui, CUI_ICON_COPY, .tooltip = "Copy");
                cui_toolbar_button(ui, CUI_ICON_PANEL_LEFT, .tooltip = "Compare");
                cui_toolbar_separator(ui);
                cui_toolbar_cycle(ui, "scale", scales, 3, &scale, .tooltip = "Scale");
                cui_toolbar_cycle(ui, "type", types, 3, &type, .tooltip = "Format");
                cui_toolbar_separator(ui);
                cui_toolbar_button(ui, CUI_ICON_DOWNLOAD, .tooltip = "Download");
            }
        }
        cui_column(ui, .id = "canvas-files", .width = CUI_FILL, .min_width = 280, .max_width = 520, .pad = 4, .gap = 2, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 14) {
            for (int k = 0; k < 5; k++) {
                cui_push_id_int(ui, k);
                cui_row(ui, .width = CUI_FILL, .height = CUI_PX(44), .pad_x = 10, .gap = 12, .align = CUI_ALIGN_CENTER, .radius = 10,
                        .bg = picked[k] ? CUI_COLOR_ACCENT_SUBTLE : (cui_color){ 0 }) {
                    cui_checkbox(ui, "##pick", &picked[k]);
                    cui_icon(ui, CUI_ICON_IMAGE, 15, cui_color_of(ui, CUI_COLOR_TEXT_MUTED));
                    cui_box(ui, .width = CUI_FILL) { cui_label(ui, names[k]); }
                    cui_text(ui, sizes[k], .size = 12, .color = CUI_COLOR_TEXT_MUTED, .tabular = true);
                }
                cui_pop_id(ui);
            }
        }
        cui_selection_bar(ui, "canvas-bulk", 0, acts, 3, .selected = picked, .item_count = 5, .anchor = cui_make_id(ui, "canvas-files"));
    }
}

/* ---- voice ---------------------------------------------------------------- */

/* A stand-in for a microphone: words with pauses between them. The page measures it
 * 20 times a second, as an app would read its audio input. */
static float voice_noise(float t, float s) {
    return (sinf(t * 1.7f + s) * 0.5f + sinf(t * 3.1f + s * 2.3f) * 0.3f + sinf(t * 7.3f + s * 0.7f) * 0.2f) * 0.5f + 0.5f;
}
static float voice_level(float t, float s) {
    return (sinf(t * 2.6f + s) > -0.45f ? 1.0f : 0.06f) * (0.28f + voice_noise(t * 2.4f, s) * 0.72f);
}
#define VOICE_RATE 20.0f   /* levels a second */
#define VOICE_MAX 600      /* thirty seconds */

typedef struct { float levels[64]; cui_voice_note note; bool mine; } voice_msg;

/* a spoken conversation that plays in a loop: you ask, it thinks (no sound) and answers */
typedef struct { cui_voice_owner who; float seconds; bool thinking; const char *text; } voice_turn;
static const voice_turn voice_script[] = {
    { CUI_VOICE_YOU, 3.2f, false, "What's the weather like tomorrow morning?" }, { CUI_VOICE_REPLY, 1.4f, true, NULL },
    { CUI_VOICE_REPLY, 4.2f, false, "Clear and cool, about 9 degrees at eight, warming to 15 by noon." },
    { CUI_VOICE_YOU, 2.4f, false, "Should I bring a jacket?" }, { CUI_VOICE_REPLY, 1.1f, true, NULL },
    { CUI_VOICE_REPLY, 3.0f, false, "A light one, yes. It's breezy until around ten." },
};
static int voice_turn_at(float t, float *into) {
    float total = 0;
    for (int k = 0; k < 6; k++) total += voice_script[k].seconds;
    t = fmodf(t, total);
    for (int k = 0; k < 6; k++) { if (t < voice_script[k].seconds) { *into = t; return k; } t -= voice_script[k].seconds; }
    *into = 0;
    return 0;
}

static void page_voice(cui_ctx *ui) {
    static char draft[256] = "";
    static bool recording = false, muted = false;
    static float take[VOICE_MAX];
    static int take_n = 0;
    static voice_msg msgs[8];
    static int msg_n = 0;
    static float call[240];
    static uint8_t call_who[240];
    static int call_n = 0;
    static double last = -1, call_start = 0, next_level = 0;
    double now = cui_time(ui);
    float dt = last < 0 ? 0 : (float)(now - last);
    last = now;
    if (msg_n == 0) {   /* a message that came in earlier */
        voice_msg *m = &msgs[msg_n++];
        for (int k = 0; k < 64; k++) m->levels[k] = (0.2f + voice_noise(k * 0.3f, 0.4f) * 0.8f) * (sinf(k * 0.45f + 0.4f) > -0.7f ? 1.0f : 0.15f);
        m->note = (cui_voice_note){ m->levels, 64, 0, 9, 1, false };
    }
    /* measure the "microphone" and the call 20 times a second */
    while (next_level <= now) {
        float t = (float)next_level;
        if (recording && take_n < VOICE_MAX) take[take_n++] = 0.85f * voice_level(t * 1.1f, 1.7f);
        float into;
        int k = voice_turn_at(t - (float)call_start, &into);
        const voice_turn *tu = &voice_script[k];
        if (call_n == 240) { memmove(call, call + 1, sizeof call - sizeof call[0]); memmove(call_who, call_who + 1, sizeof call_who - 1); call_n--; }
        call_who[call_n] = (uint8_t)tu->who;
        call[call_n++] = tu->thinking ? CUI_VOICE_SILENT : tu->who == CUI_VOICE_YOU ? (muted ? 0 : 0.8f * voice_level(t * 1.3f, 2)) : voice_level(t * 1.4f, 5);
        next_level += 1.0 / VOICE_RATE;
    }
    if (next_level - now > 1) next_level = now;   /* after a pause, start from now */

    section(ui, "Voice messages", "Press the microphone to record: the strip scrolls with the level. Cancel throws the take away, send adds it below.");
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 620) {
        for (int k = 0; k < msg_n; k++) {
            voice_msg *m = &msgs[k];
            if (m->note.playing) {   /* the "player": the page plays nothing, it only moves the position */
                m->note.position += dt * (m->note.speed > 0 ? m->note.speed : 1);
                if (m->note.position >= m->note.duration) { m->note.position = 0; m->note.playing = false; }
                cui_request_wakeup(ui, 1.0f / 30.0f);
            }
            cui_push_id_int(ui, k);
            cui_voice_note(ui, "note", &m->note, .mine = m->mine);
            cui_pop_id(ui);
        }
        int ev = cui_chat_composer(ui, "composer", draft, sizeof draft, .placeholder = "Message", .mic = true, .recording = recording,
                                   .levels = take, .level_count = take_n, .record_seconds = take_n / VOICE_RATE);
        if (ev & CUI_COMPOSER_RECORD) { recording = true; take_n = 0; }
        if (ev & CUI_COMPOSER_CANCEL) recording = false;
        if (ev & CUI_COMPOSER_VOICE) {
            recording = false;
            if (msg_n == 8) { memmove(msgs + 1, msgs + 2, sizeof msgs[0] * 6); msg_n--; }
            voice_msg *m = &msgs[msg_n++];
            cui_voice_resample(take, take_n, m->levels, 64);   /* a short waveform kept with the message */
            m->note = (cui_voice_note){ m->levels, 64, 0, take_n / VOICE_RATE, 1, false };
            m->mine = true;
            for (int k = 0; k < msg_n; k++) msgs[k].note.levels = msgs[k].levels;   /* moved by the memmove */
        }
        if (ev & CUI_COMPOSER_SEND) draft[0] = 0;
        if (recording) cui_request_wakeup(ui, 1.0f / VOICE_RATE);
    }

    section(ui, "Voice session", "Both voices on one timeline, with the live transcript. While it thinks there is no sound, so the strip stays flat.");
    float into;
    int k = voice_turn_at((float)(now - call_start), &into);
    const voice_turn *cur = &voice_script[k], *said = cur->thinking ? &voice_script[k - 1] : cur;
    cui_voice_state state = cur->thinking ? CUI_VOICE_THINKING : cur->who == CUI_VOICE_REPLY ? CUI_VOICE_SPEAKING : muted ? CUI_VOICE_MUTED : CUI_VOICE_LISTENING;
    float reveal = cur->thinking ? 1 : into / cur->seconds * 1.15f;
    int len = (int)strlen(said->text), shown = reveal >= 1 ? len : (int)(len * reveal);
    char spoken[128];
    snprintf(spoken, sizeof spoken, "%.*s", shown, said->text);
    cui_card(ui, .pad = 20, .width = CUI_FILL, .max_width = 620) {
        int f = cui_voice_session(ui, "call", state, call, call_n, .owners = call_who, .transcript = spoken, .upcoming = said->text + shown,
                                  .speaker = said->who, .muted = &muted);
        if (f & CUI_VOICE_SESSION_END) { call_start = now; call_n = 0; cui_toast(ui, "Call ended", .description = "A new one starts from the top."); }
    }
    cui_request_wakeup(ui, 1.0f / VOICE_RATE);

    section(ui, "Voice waveform", "The strip on its own: the newest levels scroll in from the right, a finished clip fits the width and seeks.");
    static float clip[160], clip_progress = 0.3f;
    for (int i = 0; i < 160; i++) clip[i] = (0.2f + voice_noise(i * 0.22f, 1.3f) * 0.8f) * (sinf(i * 0.12f + 1) > -0.75f ? 1.0f : 0.1f);
    cui_column(ui, .gap = 12, .width = CUI_FILL, .max_width = 620) {
        cui_voice_wave(ui, "live", call, call_n, .owners = call_who);
        cui_voice_wave(ui, "clip", clip, 160, .progress = &clip_progress);
    }
}

/* ---- registry ------------------------------------------------------------ */

typedef struct { const char *name, *icon; void (*fn)(cui_ctx *ui); } more_page;

static const more_page pages[] = {
    { "Actions", CUI_ICON_COMMAND, page_actions },
    { "Smart inputs", CUI_ICON_SQUARE_PEN, page_inputs },
    { "Pickers", CUI_ICON_CALENDAR, page_pickers },
    { "Capture", CUI_ICON_UPLOAD, page_capture },
    { "Disclosure", CUI_ICON_LAYERS, page_disclosure },
    { "Reader", CUI_ICON_FILE_TEXT, page_reader },
    { "Data grid", CUI_ICON_GRID_3X3, page_grid },
    { "Code", CUI_ICON_CODE, page_code },
    { "Social and media", CUI_ICON_MESSAGE_SQUARE, page_social },
    { "Planning", CUI_ICON_LIST_CHECKS, page_planning },
    { "More charts", CUI_ICON_CHART_PIE, page_viz },
    { "Motion", CUI_ICON_ACTIVITY, page_motion },
    { "AI", CUI_ICON_MESSAGE_SQUARE, page_ai },
    { "Audio", CUI_ICON_AUDIO_LINES, page_audio },
    { "Files", CUI_ICON_FOLDER, page_files },
    { "Developer tools", CUI_ICON_SQUARE_TERMINAL, page_devtools },
    { "More developer tools", CUI_ICON_BUG, page_devtools2 },
    { "Instruments", CUI_ICON_CHART_LINE, page_instruments },
    { "Creative controls", CUI_ICON_PALETTE, page_creative },
    { "Canvas and bars", CUI_ICON_LAYERS, page_canvas },
    { "Voice", CUI_ICON_MIC, page_voice },
};

int gallery_more_count(void) { return (int)(sizeof pages / sizeof pages[0]); }
const char *gallery_more_name(int page) { return page >= 0 && page < gallery_more_count() ? pages[page].name : ""; }
const char *gallery_more_icon(int page) { return page >= 0 && page < gallery_more_count() ? pages[page].icon : NULL; }
void gallery_more_page(cui_ctx *ui, int page) {
    if (page < 0 || page >= gallery_more_count()) return;
    cui_push_id(ui, pages[page].name);   /* ids on different pages never meet */
    pages[page].fn(ui);
    cui_pop_id(ui);
}
