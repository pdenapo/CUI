/* CUI component gallery. Every component in the library, on one screen. */
#include "gallery_ui.h"
#include "gallery_more.h"

#include <stdio.h>
#include <string.h>

static const char *page_names[PAGE_COUNT] = {
    "Overview", "Buttons", "Inputs", "Selection", "Navigation", "Overlays", "Feedback", "Data display", "Charts"
};
static const char *page_icons[PAGE_COUNT] = {
    CUI_ICON_LAYOUT_DASHBOARD, CUI_ICON_ZAP, CUI_ICON_TYPE, CUI_ICON_SLIDERS_HORIZONTAL, CUI_ICON_COMPASS,
    CUI_ICON_LAYERS, CUI_ICON_BELL, CUI_ICON_DATABASE, CUI_ICON_CHART_LINE
};

const char *gallery_page_name(int page) { return page >= 0 && page < PAGE_COUNT ? page_names[page] : gallery_more_name(page - PAGE_COUNT); }
int gallery_page_total(void) { return PAGE_COUNT + gallery_more_count(); }

void gallery_init(gallery *g) {
    memset(g, 0, sizeof *g);
    g->page = PAGE_OVERVIEW;
    snprintf(g->name, sizeof g->name, "Maya Okafor");
    snprintf(g->email, sizeof g->email, "maya@fieldwork.studio");
    snprintf(g->bio, sizeof g->bio, "Mathematician and writer. I like engines that compute and interfaces that stay out of the way.");
    g->quantity = 3;
    g->date = (cui_date){ 2026, 11, 14 };
    g->color = CUI_RGB(0x7747ff);
    g->newsletter = true;
    g->wifi = true;
    g->plan = 1;
    g->volume = 64;
    g->brightness = 40;
    g->lo = 20; g->hi = 75;
    g->rating = 4;
    g->chips[0] = true; g->chips[2] = true;
    g->framework = -1;
    g->plan_card = 1;
    snprintf(g->tags, sizeof g->tags, "design, c99, native");
    snprintf(g->otp, sizeof g->otp, "418");
    g->country = 2;
    g->step = 1;
    g->acc[0] = true;
    g->tree_src = true; g->tree_ui = true;
    g->selected_row = 1;
    g->sort.column = 0;
    g->selected_file = 2;
    for (int i = 0; i < 5; i++) g->task_order[i] = i;
}

/* ---- small helpers ---- */

static void section(cui_ctx *ui, const char *title, const char *desc) {
    /* more room above a heading than below it, so it reads as the start of what follows */
    cui_column(ui, .gap = 4, .pad_top = 18) {
        cui_heading(ui, title, 4);
        if (desc) cui_secondary(ui, desc);
    }
}

typedef struct { cui_color color; bool selected; } swatch_data;

static void paint_swatch(cui_ctx *ui, cui_rect r, void *user) {
    swatch_data *d = user;
    cui_vec2 c = { r.x + r.w / 2, r.y + r.h / 2 };
    if (d->selected) cui_draw_circle(ui, c, r.w / 2, cui_color_of(ui, CUI_COLOR_FOREGROUND));
    cui_draw_circle(ui, c, r.w / 2 - (d->selected ? 2.0f : 0.0f), cui_color_of(ui, CUI_COLOR_SURFACE_MUTED));
    cui_draw_circle(ui, c, r.w / 2 - 4, d->color);
}

static void sidebar(cui_ctx *ui, gallery *g, bool drawer) {
    cui_column(ui, .width = drawer ? CUI_FILL : CUI_PX(240), .height = CUI_FILL, .bg = drawer ? CUI_TRANSPARENT : CUI_COLOR_SURFACE_MUTED,
               .pad = drawer ? 0 : 12, .gap = 2) {
        if (!drawer) cui_row(ui, .gap = 10, .pad_x = 8, .pad_top = 6, .pad_bottom = 18) {
            cui_box(ui, .width = CUI_PX(28), .height = CUI_PX(28), .radius = 9, .bg = CUI_COLOR_FOREGROUND,
                    .dir = CUI_STACK, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) {
                cui_text(ui, "C", .font = CUI_FONT_DISPLAY, .size = 16, .color = CUI_COLOR_BACKGROUND, .line_height = 1.0f);
            }
            cui_text(ui, "CUI", .font = CUI_FONT_MEDIUM, .size = 15, .nowrap = true);
            cui_spacer(ui);
            char ver[16];
            snprintf(ver, sizeof ver, "v%d.%d", CUI_VERSION_MAJOR, CUI_VERSION_MINOR);
            cui_badge(ui, ver, .size = CUI_SIZE_SM);
        }
        cui_scroll(ui, .id = "nav", .height = CUI_FILL, .gap = 2) {
            cui_nav_section(ui, "Components");
            for (int i = 0; i < PAGE_COUNT; i++) {
                if (cui_nav_item(ui, page_names[i], .icon = page_icons[i], .selected = g->page == i, .group = "pages")) {
                    g->page = i;
                    g->drawer = false;
                    cui_scroll_to(ui, "main", 0);
                }
            }
            cui_gap(ui, 10);
            cui_nav_section(ui, "Extended");
            for (int i = 0; i < gallery_more_count(); i++) {
                if (cui_nav_item(ui, gallery_more_name(i), .icon = gallery_more_icon(i), .selected = g->page == PAGE_COUNT + i, .group = "pages")) {
                    g->page = PAGE_COUNT + i;
                    g->drawer = false;
                    cui_scroll_to(ui, "main", 0);
                }
            }
        }
        cui_column(ui, .gap = 12, .pad = 8) {
            const char *modes[2] = { "Light", "Dark" };
            const char *mode_icons[2] = { CUI_ICON_SUN, CUI_ICON_MOON };
            int mode = g->dark ? 1 : 0;
            if (cui_segmented(ui, "theme", modes, 2, &mode, .full_width = true, .icons = mode_icons, .size = CUI_SIZE_SM)) {
                g->dark = mode == 1;
                cui_set_theme(ui, g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
            }
            cui_row(ui, .justify = CUI_JUSTIFY_BETWEEN) {
                for (int a = 0; a < CUI_ACCENT_COUNT; a++) {
                    static const unsigned hex[CUI_ACCENT_COUNT] = { 0x555555, 0x7747ff, 0x0562ef, 0x00864b, 0xf3ad20, 0xc25300, 0xd1403a, 0xd23286 };
                    swatch_data d = { CUI_RGB(hex[a]), g->accent == a };
                    char id[32];
                    snprintf(id, sizeof id, "accent-%d", a);
                    cui_canvas_result r = cui_canvas(ui, id, CUI_PX(22), CUI_PX(22), paint_swatch, &d, sizeof d);
                    cui_tooltip(ui, cui_accent_name((cui_accent)a));
                    if (r.clicked) { g->accent = a; cui_set_accent(ui, (cui_accent)a); }
                }
            }
        }
    }
}

/* ---- pages ---- */

static void page_overview(cui_ctx *ui, gallery *g) {
    cui_column(ui, .gap = 14, .pad_bottom = 8) {
        cui_heading(ui, "Components for C", 1);
        cui_text(ui, "Forms, tables, charts, planning boards and chat pieces that draw themselves the same way on Windows, macOS and Linux. Every one works from the keyboard.",
                 .color = CUI_COLOR_TEXT_SECONDARY, .size = 15);
        cui_row(ui, .gap = 8, .pad_top = 6, .wrap = true) {
            if (cui_button(ui, "Browse components", .variant = CUI_BUTTON_PRIMARY, .icon_right = CUI_ICON_ARROW_RIGHT)) g->page = PAGE_BUTTONS;
            if (cui_button(ui, "Open command palette", .variant = CUI_BUTTON_OUTLINE, .icon = CUI_ICON_COMMAND)) g->palette_open = true;
        }
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 3, .min_column_width = 200, .gap = 16) {
        static const float s2[] = { 9, 7, 8, 6, 5, 6, 4, 4, 3 };
        cui_metric(ui, "Components", "150", .icon = CUI_ICON_BOX);
        cui_metric(ui, "Frame time", "0.4 ms", .delta = -18.0f, .delta_label = "-18% vs v0.9", .lower_is_better = true, .spark = s2, .spark_count = 9, .icon = CUI_ICON_ZAP);
        cui_metric(ui, "Dependencies", "0", .icon = CUI_ICON_PACKAGE);
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_card(ui, .gap = 16) {
            cui_column(ui, .gap = 4) {
                cui_heading(ui, "Create a workspace", 4);
                cui_secondary(ui, "You can invite people once it exists.");
            }
            cui_input(ui, "ov-name", g->name, sizeof g->name, .label = "Name", .placeholder = "Your name");
            cui_input(ui, "ov-email", g->email, sizeof g->email, .label = "Email", .icon = CUI_ICON_MAIL, .placeholder = "you@company.com");
            cui_checkbox(ui, "Email me a weekly summary", &g->terms);
            if (cui_button(ui, "Create workspace", .variant = CUI_BUTTON_PRIMARY, .full_width = true))
                cui_toast(ui, "Workspace created", .description = "It is empty for now. Start with a project.", .tone = CUI_TONE_SUCCESS);
        }
        cui_card(ui, .gap = 12) {
            cui_row(ui) {
                cui_column(ui, .gap = 4, .width = CUI_FILL) {
                    cui_heading(ui, "Team", 4);
                    cui_secondary(ui, "Four people, one owner.");
                }
                cui_button(ui, "Invite", .icon = CUI_ICON_USER_PLUS, .size = CUI_SIZE_SM);
            }
            static const char *people[4][3] = {
                { "Jonas Berg", "jonas@northlight.io", "Owner" }, { "Priya Nair", "priya@harbor.dev", "Admin" },
                { "Tomas Rivera", "tomas@kiln.app", "Editor" }, { "Lena Fischer", "lena@fischer.dev", "Viewer" },
            };
            for (int i = 0; i < 4; i++) {
                cui_row(ui, .gap = 12, .pad_y = 4) {
                    cui_avatar(ui, people[i][0], .size = 34, .status = i == 0 ? "online" : i == 1 ? "away" : NULL);
                    cui_column(ui, .width = CUI_FILL, .gap = 1) {
                        cui_text(ui, people[i][0], .font = CUI_FONT_MEDIUM, .nowrap = true);
                        cui_text(ui, people[i][1], .size = 12, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
                    }
                    cui_badge(ui, people[i][2], .tone = i == 0 ? CUI_TONE_ACCENT : CUI_TONE_NEUTRAL);
                }
            }
        }
    }
}

static void page_buttons(cui_ctx *ui, gallery *g) {
    section(ui, "Variants", "One primary action per view. Secondary for everything else.");
    cui_card(ui) {
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_button(ui, "Primary", .variant = CUI_BUTTON_PRIMARY);
            cui_button(ui, "Secondary");
            cui_button(ui, "Outline", .variant = CUI_BUTTON_OUTLINE);
            cui_button(ui, "Ghost", .variant = CUI_BUTTON_GHOST);
            cui_button(ui, "Danger", .variant = CUI_BUTTON_DANGER);
            cui_button(ui, "Link", .variant = CUI_BUTTON_LINK);
        }
    }
    section(ui, "Sizes and icons", NULL);
    cui_card(ui, .gap = 16) {
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_button(ui, "Small", .size = CUI_SIZE_SM);
            cui_button(ui, "Medium");
            cui_button(ui, "Large", .size = CUI_SIZE_LG);
        }
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_button(ui, "New project", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_PLUS);
            cui_button(ui, "Download", .icon = CUI_ICON_DOWNLOAD);
            cui_button(ui, "Continue", .variant = CUI_BUTTON_OUTLINE, .icon_right = CUI_ICON_ARROW_RIGHT);
            cui_icon_button(ui, CUI_ICON_COPY, .tooltip = "Copy", .shortcut = "Ctrl C");
            cui_icon_button(ui, CUI_ICON_SHARE_2, .tooltip = "Share");
            cui_icon_button(ui, CUI_ICON_HEART, .tooltip = "Like", .variant = CUI_BUTTON_OUTLINE);
            cui_icon_button(ui, CUI_ICON_TRASH_2, .tooltip = "Delete", .variant = CUI_BUTTON_SECONDARY);
        }
    }
    section(ui, "States", "Loading keeps the label so the layout does not jump.");
    cui_card(ui) {
        cui_row(ui, .gap = 8, .wrap = true) {
            if (cui_button(ui, g->loading ? "Saving" : "Save changes", .variant = CUI_BUTTON_PRIMARY, .loading = g->loading)) g->loading = true;
            if (cui_button(ui, "Reset##loading")) g->loading = false;
            cui_button(ui, "Disabled", .disabled = true);
            cui_button(ui, "Selected", .variant = CUI_BUTTON_GHOST, .selected = true, .icon = CUI_ICON_CHECK);
        }
    }
    section(ui, "Groups, copy and hold to confirm", "Hold the button until it fills to confirm a destructive action.");
    cui_card(ui) {
        cui_row(ui, .gap = 12, .wrap = true) {
            cui_button_group(ui) {
                cui_button(ui, "Bold##group", .icon = CUI_ICON_BOLD);
                cui_button(ui, "Italic##group", .icon = CUI_ICON_ITALIC);
                cui_button(ui, "Underline##group", .icon = CUI_ICON_UNDERLINE);
            }
            cui_row(ui, .gap = 6, .bg = CUI_COLOR_SURFACE_MUTED, .radius = 18, .pad_left = 14, .pad_right = 4, .pad_y = 2) {
                cui_text(ui, "npm i cui", .font = CUI_FONT_MONO, .nowrap = true);
                cui_copy_button(ui, "install", "npm i cui");
            }
            if (cui_hold_button(ui, "Hold to delete", .icon = CUI_ICON_TRASH_2)) cui_toast(ui, "Deleted", .tone = CUI_TONE_DANGER);
        }
    }
    section(ui, "Segmented control", "A highlight glides to the selected option.");
    cui_card(ui, .gap = 16) {
        const char *views[3] = { "List", "Board", "Timeline" };
        const char *view_icons[3] = { CUI_ICON_LIST, CUI_ICON_LAYOUT_DASHBOARD, CUI_ICON_HISTORY };
        cui_segmented(ui, "view", views, 3, &g->seg_view, .icons = view_icons);
        const char *ranges[4] = { "Day", "Week", "Month", "Year" };
        cui_segmented(ui, "range", ranges, 4, &g->seg_size, .size = CUI_SIZE_SM);
    }
    section(ui, "Dropdown menu", NULL);
    cui_card(ui) {
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_menu(ui, "Options", .icon = CUI_ICON_SETTINGS) {
                cui_menu_label(ui, "Project");
                cui_menu_item(ui, "Rename", .icon = CUI_ICON_PENCIL, .shortcut = "F2");
                cui_menu_item(ui, "Duplicate", .icon = CUI_ICON_COPY, .shortcut = "Ctrl D");
                cui_menu_item(ui, "Share", .icon = CUI_ICON_SHARE_2);
                cui_menu_separator(ui);
                if (cui_menu_item(ui, "Delete", .icon = CUI_ICON_TRASH_2, .danger = true)) g->confirm_open = true;
            }
            cui_menu(ui, "More actions", .icon_only = true, .icon = CUI_ICON_ELLIPSIS) {
                cui_menu_item(ui, "Archive", .icon = CUI_ICON_ARCHIVE);
                cui_menu_item(ui, "Export", .icon = CUI_ICON_DOWNLOAD);
            }
        }
    }
}

static void page_inputs(cui_ctx *ui, gallery *g) {
    section(ui, "Text fields", "Labels above, hints and errors below.");
    cui_card(ui, .gap = 16) {
        cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
            cui_input(ui, "name", g->name, sizeof g->name, .label = "Full name", .placeholder = "Maya Okafor");
            cui_input(ui, "email", g->email, sizeof g->email, .label = "Email", .icon = CUI_ICON_AT_SIGN, .hint = "We never share your email.");
            cui_input(ui, "password", g->password, sizeof g->password, .label = "Password", .password = true, .placeholder = "At least 8 characters",
                      .error = (g->password[0] && strlen(g->password) < 8) ? "Use 8 or more characters." : NULL);
            cui_search(ui, "search", g->search, sizeof g->search, .label = "Search", .placeholder = "Search projects");
        }
        cui_textarea(ui, "bio", g->bio, sizeof g->bio, .label = "About", .height = 96, .hint = "Markdown is supported.");
    }
    section(ui, "Tags, codes and password strength", NULL);
    cui_card(ui, .gap = 16) {
        cui_tag_input(ui, "tags", g->tags, sizeof g->tags, .label = "Topics", .placeholder = "Add a topic");
        cui_column(ui, .gap = 8) {
            cui_label(ui, "Verification code");
            if (cui_otp_input(ui, "otp", g->otp, 6)) cui_toast(ui, "Code entered", .tone = CUI_TONE_SUCCESS);
        }
        cui_column(ui, .gap = 8) {
            cui_input(ui, "new-pw", g->password, sizeof g->password, .label = "New password", .password = true, .placeholder = "Try a long passphrase");
            cui_password_strength(ui, g->password);
        }
    }
    section(ui, "Pickers", NULL);
    cui_card(ui, .gap = 16) {
        cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
            static const char *countries[] = { "Canada", "Denmark", "Germany", "Iran", "Japan", "Kenya", "Netherlands", "Portugal", "United States" };
            cui_select(ui, "country", countries, 9, &g->country, .label = "Country");
            static const char *frameworks[] = { "C99", "C11", "C17", "C23", "C++17", "Objective-C", "Zig", "Odin", "Rust" };
            cui_select(ui, "framework", frameworks, 9, &g->framework, .label = "Language", .searchable = true, .placeholder = "Pick a language");
            cui_date_picker(ui, "due", &g->date, .label = "Due date");
            cui_number(ui, "qty", &g->quantity, 0, 99, .label = "Seats", .width = CUI_FILL);
        }
        cui_column(ui, .gap = 6) {
            cui_label(ui, "Brand color");
            cui_color_picker(ui, "brand", &g->color);
        }
    }
}

static void page_selection(cui_ctx *ui, gallery *g) {
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_column(ui, .gap = 12) {
            section(ui, "Checkbox", NULL);
            cui_card(ui, .gap = 14) {
                cui_checkbox(ui, "Email notifications", &g->newsletter, .description = "Product news, at most once a month.");
                cui_checkbox(ui, "Marketing", &g->marketing, .description = "Offers from our partners.");
                cui_checkbox(ui, "Disabled option", &g->terms, .disabled = true);
            }
        }
        cui_column(ui, .gap = 12) {
            section(ui, "Switch", NULL);
            cui_card(ui, .gap = 14) {
                cui_switch(ui, "Wi-Fi", &g->wifi);
                cui_switch(ui, "Bluetooth", &g->bluetooth);
                g->bluetooth_rect = cui_last_rect(ui);
                cui_switch(ui, "Airplane mode", &g->airplane, .description = "Turns off all radios.");
            }
        }
    }
    section(ui, "Radio group", NULL);
    cui_card(ui, .gap = 12) {
        cui_radio(ui, "Every change", &g->plan, 0, .description = "Save as you type. Uses the most disk.");
        cui_radio(ui, "Every 5 minutes", &g->plan, 1, .description = "A good balance for most projects.");
        cui_radio(ui, "Only when I save", &g->plan, 2, .description = "Nothing is written until you press Ctrl S.");
    }
    section(ui, "Radio cards", "For choices that need a sentence of explanation.");
    {
        static const cui_choice places[] = {
            { "This computer", "Saved next to the project. Nothing leaves the machine.", CUI_ICON_MONITOR, NULL },
            { "Team server", "Synced to your company server every few minutes.", CUI_ICON_SERVER, "Default" },
            { "Both", "A local copy, uploaded to the server every night.", CUI_ICON_LAYERS, NULL },
        };
        cui_radio_cards(ui, "places", places, 3, &g->plan_card);
    }
    section(ui, "Sliders", "Drag past either end: the track stretches like rubber.");
    cui_card(ui, .gap = 20) {
        cui_slider(ui, "Volume", &g->volume, 0, 100, .show_value = true);
        cui_slider(ui, "Brightness", &g->brightness, 0, 100, .step = 10, .show_value = true, .format = "%.0f%%");
        cui_range_slider(ui, "Price range", &g->lo, &g->hi, 0, 100);
    }
    section(ui, "Chips and rating", NULL);
    cui_card(ui, .gap = 16) {
        cui_row(ui, .gap = 8, .wrap = true) {
            static const char *chips[6] = { "Design", "Engineering", "Marketing", "Sales", "Support", "Finance" };
            for (int i = 0; i < 6; i++) cui_chip(ui, chips[i], &g->chips[i]);
        }
        cui_rating(ui, "rating", &g->rating, 5);
    }
}

static void page_navigation(cui_ctx *ui, gallery *g) {
    section(ui, "Tabs", "The indicator slides to the active tab.");
    cui_card(ui, .gap = 16) {
        const char *tabs[4] = { "Overview", "Analytics", "Reports", "Settings" };
        cui_tabs(ui, "tabs", tabs, 4, &g->tab);
        cui_column(ui, .gap = 6, .pad_y = 4) {
            cui_label(ui, tabs[g->tab]);
            cui_secondary(ui, "Tabs switch between views of the same thing. Content changes in place.");
        }
        const char *tabs2[3] = { "Account", "Password", "Billing" };
        const char *tab_icons[3] = { CUI_ICON_USER, CUI_ICON_LOCK, CUI_ICON_CREDIT_CARD };
        cui_tabs(ui, "tabs2", tabs2, 3, &g->tab2, .variant = CUI_TABS_PILL, .icons = tab_icons);
    }
    section(ui, "Breadcrumb and pagination", NULL);
    cui_card(ui, .gap = 18) {
        const char *crumbs[4] = { "Home", "Projects", "CUI", "Settings" };
        int c = cui_breadcrumb(ui, crumbs, 4);
        if (c >= 0) cui_toast(ui, crumbs[c]);
        cui_pagination(ui, "pages", &g->page_index, 12);
    }
    section(ui, "Stepper", NULL);
    cui_card(ui, .gap = 18) {
        const char *steps[4] = { "Account", "Workspace", "Invite team", "Done" };
        cui_stepper(ui, steps, 4, g->step);
        cui_row(ui, .gap = 8, .wrap = true) {
            if (cui_button(ui, "Back", .disabled = g->step == 0)) g->step--;
            if (cui_button(ui, g->step >= 3 ? "Finish" : "Next", .variant = CUI_BUTTON_PRIMARY, .icon_right = CUI_ICON_ARROW_RIGHT)) {
                if (g->step < 3) g->step++;
                else cui_toast(ui, "All set", .tone = CUI_TONE_SUCCESS);
            }
        }
    }
}

static void page_overlays(cui_ctx *ui, gallery *g) {
    section(ui, "Dialogs and sheets", "Esc or a click outside closes them. Focus stays inside.");
    cui_card(ui) {
        cui_row(ui, .gap = 8, .wrap = true) {
            if (cui_button(ui, "Edit profile", .icon = CUI_ICON_PENCIL)) g->dialog_open = true;
            if (cui_button(ui, "Open sheet", .icon = CUI_ICON_PANEL_LEFT)) g->sheet_open = true;
            if (cui_button(ui, "Delete project", .variant = CUI_BUTTON_DANGER, .icon = CUI_ICON_TRASH_2)) g->confirm_open = true;
            if (cui_button(ui, "Command palette", .variant = CUI_BUTTON_OUTLINE, .icon = CUI_ICON_COMMAND, .tooltip = "Search everything", .shortcut = "Ctrl K"))
                g->palette_open = true;
        }
    }
    section(ui, "Popover and context menu", NULL);
    cui_card(ui, .gap = 16) {
        cui_row(ui, .gap = 8, .wrap = true) {
            if (cui_button(ui, "Filters", .icon = CUI_ICON_FILTER)) cui_popover_toggle(ui, "filters");
            cui_popover(ui, "filters", .width = 280) {
                cui_label(ui, "Filter results");
                cui_checkbox(ui, "Only active", &g->newsletter);
                cui_checkbox(ui, "Include archived", &g->marketing);
                cui_slider(ui, "Minimum score", &g->brightness, 0, 100, .show_value = true);
                cui_row(ui, .gap = 8, .justify = CUI_JUSTIFY_END) {
                    if (cui_button(ui, "Apply", .variant = CUI_BUTTON_PRIMARY, .size = CUI_SIZE_SM)) cui_popover_close(ui, "filters");
                }
            }
        }
        cui_card(ui, .muted = true, .id = "ctx-area", .pad = 28, .align = CUI_ALIGN_CENTER, .hoverable = true) {
            cui_icon(ui, CUI_ICON_ELLIPSIS, 18, CUI_COLOR_TEXT_MUTED);
            cui_text(ui, "Right click this area", .color = CUI_COLOR_TEXT_SECONDARY, .align = CUI_TEXT_CENTER);
        }
        cui_context_menu(ui, "ctx") {
            cui_menu_item(ui, "Cut", .icon = CUI_ICON_SCISSORS, .shortcut = "Ctrl X");
            cui_menu_item(ui, "Copy", .icon = CUI_ICON_COPY, .shortcut = "Ctrl C");
            cui_menu_item(ui, "Paste", .icon = CUI_ICON_CLIPBOARD, .shortcut = "Ctrl V");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Delete", .icon = CUI_ICON_TRASH_2, .danger = true);
        }
    }
    section(ui, "Toasts", "They stack, expand on hover and pause while you read.");
    cui_card(ui) {
        cui_row(ui, .gap = 8, .wrap = true) {
            if (cui_button(ui, "Save profile")) cui_toast(ui, "Changes saved", .description = "Your profile is up to date.", .tone = CUI_TONE_SUCCESS);
            if (cui_button(ui, "Upload a 30 MB file")) cui_toast(ui, "Upload failed", .description = "The file is larger than 25 MB.", .tone = CUI_TONE_DANGER);
            if (cui_button(ui, "Archive a message")) cui_toast(ui, "Message archived", .action = "Undo");
            if (cui_button(ui, "Check for updates")) cui_toast(ui, "Update available", .description = "Version 1.1 is ready to install.", .tone = CUI_TONE_INFO);
        }
    }
    section(ui, "Tooltips", NULL);
    cui_card(ui) {
        cui_row(ui, .gap = 4, .wrap = true) {
            cui_icon_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .shortcut = "Ctrl B");
            cui_icon_button(ui, CUI_ICON_ITALIC, .tooltip = "Italic", .shortcut = "Ctrl I");
            cui_icon_button(ui, CUI_ICON_UNDERLINE, .tooltip = "Underline", .shortcut = "Ctrl U");
            cui_separator(ui);
            cui_icon_button(ui, CUI_ICON_ALIGN_LEFT, .tooltip = "Align left");
            cui_icon_button(ui, CUI_ICON_LIST, .tooltip = "Bulleted list");
            cui_icon_button(ui, CUI_ICON_LINK, .tooltip = "Insert link");
        }
    }
}

static void page_feedback(cui_ctx *ui, gallery *g) {
    section(ui, "Alerts", "Color only when something succeeded, needs attention or failed.");
    cui_column(ui, .gap = 10) {
        cui_alert(ui, "A new version is available", .description = "Restart to update to 1.1.");
        cui_alert(ui, "Payment received", .description = "Your invoice was paid on November 5.", .tone = CUI_TONE_SUCCESS);
        cui_alert(ui, "Storage almost full", .description = "You have used 92% of your plan.", .tone = CUI_TONE_WARNING, .dismissible = true);
        cui_alert(ui, "Build failed", .description = "Missing symbol cui_frame in main.c line 42.", .tone = CUI_TONE_DANGER);
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_card(ui, .gap = 18) {
            cui_heading(ui, "Progress", 4);
            cui_progress(ui, 0.68f, .label = "Uploading assets", .show_value = true);
            cui_progress(ui, 0.0f, .label = "Indexing", .indeterminate = true);
            cui_progress(ui, 0.42f, .label = "Exporting clip.mp4", .show_value = true, .dither = true);
            cui_progress(ui, 0.0f, .label = "Syncing library", .indeterminate = true, .dither = true);
            cui_meter(ui, "Storage", 18.4f, 20, "GB");
            cui_row(ui, .gap = 16, .wrap = true) {
                cui_spinner(ui, 16);
                cui_spinner(ui, 22);
                cui_spinner(ui, 28);
            }
        }
        cui_card(ui, .gap = 14) {
            cui_heading(ui, "Loading", 4);
            cui_row(ui, .gap = 12, .wrap = true) {
                cui_skeleton(ui, CUI_PX(40), 40);
                cui_column(ui, .gap = 8, .width = CUI_FILL) {
                    cui_skeleton(ui, CUI_PCT(70), 12);
                    cui_skeleton(ui, CUI_PCT(45), 12);
                }
            }
            cui_skeleton(ui, CUI_FILL, 88);
            cui_skeleton(ui, CUI_PCT(90), 12);
            cui_skeleton(ui, CUI_PCT(60), 12);
            cui_skeleton_group(ui, .dither = true, .gap = 10) {
                cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                    cui_skeleton(ui, CUI_PX(32), 32);
                    cui_column(ui, .gap = 8, .width = CUI_FILL) { cui_skeleton(ui, CUI_PCT(60), 8); cui_skeleton(ui, CUI_PCT(40), 8); }
                }
                cui_skeleton(ui, CUI_FILL, 8);
            }
        }
        cui_card(ui, .gap = 14) {
            cui_heading(ui, "Dither spinners", 4);
            cui_row(ui, .gap = 20, .wrap = true, .align = CUI_ALIGN_CENTER) {
                static const cui_spinner_style styles[] = { CUI_SPINNER_COMET, CUI_SPINNER_DOT_MATRIX, CUI_SPINNER_FILL_DRAIN, CUI_SPINNER_RADAR,
                                                            CUI_SPINNER_RIPPLE, CUI_SPINNER_TRANSFER, CUI_SPINNER_ORBIT, CUI_SPINNER_BOUNCE,
                                                            CUI_SPINNER_SCAN, CUI_SPINNER_PULSE, CUI_SPINNER_SPIRAL, CUI_SPINNER_PLASMA };
                for (int k = 0; k < (int)(sizeof styles / sizeof styles[0]); k++) cui_spinner(ui, 40, .style = styles[k]);
            }
            cui_row(ui, .gap = 10, .wrap = true, .align = CUI_ALIGN_CENTER) {
                cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 14, .pad_y = 8, .radius = 10, .bg = CUI_COLOR_FOREGROUND) {
                    cui_spinner(ui, 14, .style = CUI_SPINNER_COMET, .color = CUI_COLOR_BACKGROUND);
                    cui_text(ui, "Saving\xe2\x80\xa6", .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_BACKGROUND);
                }
                cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 11, .pad_y = 6, .radius = 999, .border = 1, .bg = CUI_COLOR_SURFACE) {
                    cui_spinner(ui, 14, .style = CUI_SPINNER_ORBIT);
                    cui_text(ui, "Syncing 24 files", .color = CUI_COLOR_TEXT_SECONDARY);
                }
                cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 11, .pad_y = 6, .radius = 999, .border = 1, .bg = CUI_COLOR_SURFACE) {
                    cui_spinner(ui, 14, .style = CUI_SPINNER_RIPPLE);
                    cui_text(ui, "Connecting", .color = CUI_COLOR_TEXT_SECONDARY);
                }
            }
            /* a job that takes two seconds and then succeeds or fails, in turn */
            static double started = -1;
            static int runs;
            double since = cui_time(ui) - started;
            cui_status st = started < 0 ? CUI_STATUS_IDLE : since < 2 ? CUI_STATUS_BUSY : runs % 2 ? CUI_STATUS_SUCCESS : CUI_STATUS_ERROR;
            cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER) {
                if (cui_button(ui, "Upload", .size = CUI_SIZE_SM, .disabled = st == CUI_STATUS_BUSY)) { started = cui_time(ui); runs++; }
                cui_status_spinner(ui, "upload", st, .size = 18);
                cui_text(ui, st == CUI_STATUS_BUSY ? "Uploading" : st == CUI_STATUS_SUCCESS ? "Uploaded" : st == CUI_STATUS_ERROR ? "Upload failed" : "",
                         .color = CUI_COLOR_TEXT_SECONDARY);
            }
        }
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_card(ui, .gap = 14) {
            cui_heading(ui, "Activity", 4);
            cui_timeline_item(ui, "Deployed to production", .time = "2m ago", .description = "Release 1.0.3 is live in all regions.", .icon = CUI_ICON_ROCKET, .tone = CUI_TONE_SUCCESS);
            cui_timeline_item(ui, "Review requested", .time = "1h ago", .description = "Jonas asked you to review the theming changes.", .icon = CUI_ICON_MESSAGE_SQUARE, .tone = CUI_TONE_INFO);
            cui_timeline_item(ui, "Build failed", .time = "3h ago", .icon = CUI_ICON_CIRCLE_ALERT, .tone = CUI_TONE_DANGER, .last = true);
        }
        cui_card(ui, .gap = 14) {
            cui_heading(ui, "Goals", 4);
            cui_row(ui, .gap = 24, .justify = CUI_JUSTIFY_AROUND) {
                cui_progress_ring(ui, "Onboarding", 0.72f, 104);
                cui_progress_ring(ui, "Quota", 0.35f, 104);
            }
        }
    }
    section(ui, "Badges and avatars", NULL);
    cui_card(ui, .gap = 16) {
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_badge(ui, "Draft");
            cui_badge(ui, "Recommended", .tone = CUI_TONE_ACCENT);
            cui_badge(ui, "Active", .tone = CUI_TONE_SUCCESS, .dot = true);
            cui_badge(ui, "Pending", .tone = CUI_TONE_WARNING, .dot = true);
            cui_badge(ui, "Overdue", .tone = CUI_TONE_DANGER, .dot = true);
            cui_badge(ui, "Beta", .tone = CUI_TONE_VIOLET);
            cui_badge(ui, "New", .tone = CUI_TONE_INFO, .solid = true);
        }
        cui_row(ui, .gap = 16, .wrap = true) {
            cui_avatar(ui, "Sam Whitfield", .size = 40, .status = "online");
            cui_avatar(ui, "Daniel Mensah", .size = 40, .status = "busy");
            cui_avatar(ui, "Rosa Delgado", .size = 40, .square = true);
            static const char *team[] = { "Kwame Asante", "Hana Novak", "Elif Kaya", "Ben Carter", "Omar Haddad", "Felix Moreau", "Iris Coleman" };
            cui_avatar_group(ui, team, 7, 4, 32);
        }
    }
    cui_card(ui, .pad = 0) {
        if (cui_empty_state(ui, "No projects yet", .icon = CUI_ICON_FOLDER_OPEN,
                            .description = "A project holds your screens, their builds and who can see them.", .action = "New project"))
            cui_toast(ui, "Project created", .tone = CUI_TONE_SUCCESS);
    }
}

typedef struct { const char *name, *email, *role, *status; int status_tone; const char *amount; } person_row;

static void page_data(cui_ctx *ui, gallery *g) {
    section(ui, "Table", "Sortable columns, row selection and components in cells.");
    static person_row rows[] = {
        { "Jonas Berg", "jonas@northlight.io", "Owner", "Active", CUI_TONE_SUCCESS, "$12,384" },
        { "Priya Nair", "priya@harbor.dev", "Admin", "Active", CUI_TONE_SUCCESS, "$9,846" },
        { "Tomas Rivera", "tomas@kiln.app", "Editor", "Invited", CUI_TONE_WARNING, "$7,215" },
        { "Lena Fischer", "lena@fischer.dev", "Viewer", "Active", CUI_TONE_SUCCESS, "$3,127" },
        { "Aiko Tanaka", "aiko@tanaka.design", "Editor", "Suspended", CUI_TONE_DANGER, "$1,962" },
    };
    int order[5] = { 0, 1, 2, 3, 4 };
    for (int i = 1; i < 5; i++) {
        int v = order[i], j = i - 1;
        while (j >= 0) {
            const char *a = g->sort.column == 0 ? rows[order[j]].name : g->sort.column == 2 ? rows[order[j]].role : rows[order[j]].status;
            const char *b = g->sort.column == 0 ? rows[v].name : g->sort.column == 2 ? rows[v].role : rows[v].status;
            int c = strcmp(a, b);
            if (g->sort.descending ? c < 0 : c > 0) { order[j + 1] = order[j]; j--; } else break;
        }
        order[j + 1] = v;
    }
    cui_table_column cols[5] = {
        { "Name", CUI_FILL, CUI_TEXT_LEFT, true },
        { "Email", CUI_GROW(1.2), CUI_TEXT_LEFT, false },
        { "Role", CUI_PX(110), CUI_TEXT_LEFT, true },
        { "Status", CUI_PX(120), CUI_TEXT_LEFT, true },
        { "Revenue", CUI_PX(110), CUI_TEXT_RIGHT, false },
    };
    cui_table(ui, "people", cols, 5, .sort = &g->sort, .selectable = true) {
        for (int k = 0; k < 5; k++) {
            person_row *r = &rows[order[k]];
            if (cui_table_row(ui, g->selected_row == order[k])) g->selected_row = order[k];
            cui_table_cell(ui);
            cui_avatar(ui, r->name, .size = 26);
            cui_text(ui, r->name, .font = CUI_FONT_MEDIUM, .nowrap = true);
            cui_table_cell(ui);
            cui_text(ui, r->email, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
            cui_table_cell_text(ui, r->role);
            cui_table_cell(ui);
            cui_badge(ui, r->status, .tone = (cui_tone)r->status_tone, .dot = true);
            cui_table_cell_text(ui, r->amount);
        }
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_column(ui, .gap = 12) {
            section(ui, "Tree view", NULL);
            cui_card(ui, .pad = 10, .gap = 1) {
                if (cui_tree_begin(ui, "src", &g->tree_src, NULL)) {
                    if (cui_tree_begin(ui, "components", &g->tree_ui, NULL)) {
                        static const char *files[] = { "button.c", "dialog.c", "slider.c", "table.c" };
                        for (int i = 0; i < 4; i++) {
                            cui_tree_opts o = { 0 };
                            o.leaf = true;
                            o.selected = g->selected_file == i;
                            cui_tree_begin(ui, files[i], NULL, &o);
                            if (cui_tree_clicked(ui)) g->selected_file = i;
                        }
                        cui_tree_end(ui);
                    }
                    if (cui_tree_begin(ui, "assets", &g->tree_assets, NULL)) {
                        cui_tree_opts o = { 0 };
                        o.leaf = true;
                        o.icon = CUI_ICON_IMAGE;
                        cui_tree_begin(ui, "logo.png", NULL, &o);
                        cui_tree_end(ui);
                    }
                    cui_tree_opts o = { 0 };
                    o.leaf = true;
                    cui_tree_begin(ui, "main.c", NULL, &o);
                    cui_tree_end(ui);
                }
            }
        }
        cui_column(ui, .gap = 12) {
            section(ui, "List", NULL);
            cui_card(ui, .pad = 8, .gap = 2) {
                cui_list_item(ui, "Design review", .avatar = "Jonas Berg", .subtitle = "Tomorrow at 10:00", .trailing = "45m");
                cui_list_item(ui, "Release 1.0", .avatar = "Priya Nair", .subtitle = "Ship it", .trailing = "2h", .selected = true);
                cui_list_item(ui, "Planning", .avatar = "Tomas Rivera", .subtitle = "Q1 roadmap", .trailing = "1h");
            }
        }
    }
    section(ui, "Reorderable list", "Drag a row. The others make room as you go.");
    cui_column(ui, .width = CUI_FILL) {   /* the rows carry their own edges: no card around them */
        static const char *tasks[5] = { "Write the release notes", "Record the demo video", "Update the screenshots", "Tag version 1.0", "Post the announcement" };
        /* one tone per label, wherever it appears */
        static const int tones[5] = { CUI_TONE_INFO, CUI_TONE_NEUTRAL, CUI_TONE_INFO, CUI_TONE_WARNING, CUI_TONE_TEAL };
        static const char *labels[5] = { "Docs", "Video", "Docs", "Release", "Blog" };
        cui_reorder_begin(ui, "tasks", g->task_order, 5);
        for (int i = 0; i < 5; i++) {
            cui_reorder_item(ui, i) {
                int t = g->task_order[i];
                cui_text(ui, tasks[t], .nowrap = true, .width = CUI_FILL);
                cui_badge(ui, labels[t], .tone = (cui_tone)tones[t]);
            }
        }
        if (cui_reorder_end(ui)) cui_toast(ui, "Order saved");
    }
    section(ui, "Accordion", NULL);
    cui_card(ui, .pad_y = 4, .gap = 0) {
        if (cui_accordion_begin(ui, "Is it really dependency free?", &g->acc[0])) {
            cui_secondary(ui, "Yes. The core is plain C99. Windows uses Win32, macOS uses Cocoa and Linux uses X11. Fonts and icons are embedded.");
            cui_accordion_end(ui);
        }
        if (cui_accordion_begin(ui, "Can I use my own renderer?", &g->acc[1])) {
            cui_secondary(ui, "CUI outputs vertices and draw commands. Port the 60 line shader to Vulkan, Metal or Direct3D and you are done.");
            cui_accordion_end(ui);
        }
        if (cui_accordion_begin(ui, "Does it respect reduced motion?", &g->acc[2])) {
            cui_secondary(ui, "Every animation has a reduced motion path that keeps the final state and feedback.");
            cui_accordion_end(ui);
        }
    }
    section(ui, "Virtual list", "Ten thousand rows. Only the visible ones are built.");
    cui_card(ui, .pad = 6) {
        int first, last;
        cui_virtual_list(ui, "big", 10000, 36, CUI_PX(220), &first, &last) {
            for (int i = first; i < last; i++) {
                char buf[64], amount[32];
                unsigned h = (unsigned)(i + 1) * 2654435761u;
                snprintf(buf, sizeof buf, "Invoice 2026-%05d", 10000 - i);
                snprintf(amount, sizeof amount, "$%u.%02u", 40 + (h >> 8) % 2400, (h >> 4) % 100);
                cui_row(ui, .height = CUI_PX(36), .pad_x = 12, .gap = 12) {
                    cui_icon(ui, CUI_ICON_FILE_TEXT, 16, CUI_COLOR_TEXT_MUTED);
                    cui_text(ui, buf, .nowrap = true, .width = CUI_FILL, .tabular = true);
                    cui_text(ui, amount, .size = 13, .tabular = true, .nowrap = true);
                    cui_text(ui, (h >> 20) % 5 == 0 ? "Due" : "Paid", .size = 12, .color = CUI_COLOR_TEXT_MUTED, .nowrap = true, .width = CUI_PX(32));
                }
            }
        }
    }
}

static void page_charts(cui_ctx *ui, gallery *g) {
    (void)g;
    static const char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static const float revenue[12] = { 18, 22, 21, 27, 31, 29, 36, 41, 39, 46, 52, 58 };
    static const float costs[12] = { 12, 13, 15, 14, 18, 19, 21, 22, 24, 25, 27, 29 };
    section(ui, "Line chart", "Hover to inspect values. The crosshair glides between points.");
    cui_card(ui) {
        cui_series s[2] = { { "Revenue", revenue, 12, { 0 } }, { "Costs", costs, 12, { 0 } } };
        cui_line_chart(ui, "revenue", s, 2, .labels = months, .label_count = 12, .area = true, .smooth = true, .height = 240, .format = "$%.0fk");
    }
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_column(ui, .gap = 12) {
            section(ui, "Bar chart", NULL);
            cui_card(ui) {
                static const float a[6] = { 42, 51, 38, 64, 58, 71 };
                static const float b[6] = { 28, 33, 30, 41, 46, 49 };
                cui_series s[2] = { { "Desktop", a, 6, { 0 } }, { "Mobile", b, 6, { 0 } } };
                cui_bar_chart(ui, "visits", s, 2, .labels = months, .label_count = 6, .height = 200);
            }
        }
        cui_column(ui, .gap = 12) {
            section(ui, "Donut chart", NULL);
            cui_card(ui, .align = CUI_ALIGN_CENTER) {
                static const float shares[4] = { 46, 27, 17, 10 };
                static const char *labels[4] = { "Windows", "macOS", "Linux", "Other" };
                cui_donut_chart(ui, "platforms", shares, labels, 4, .height = 180, .format = "%.0f%%");
            }
        }
    }
    section(ui, "Dither area chart", "Fills made of dots that thin out toward the base; stacked bands keep their own color.");
    cui_card(ui) {
        static const char *hours[12] = { "00:00", "02:00", "04:00", "06:00", "08:00", "10:00", "12:00", "14:00", "16:00", "18:00", "20:00", "22:00" };
        static const float api[12] = { 22, 19, 17, 21, 30, 38, 41, 37, 34, 39, 33, 26 };
        static const float web[12] = { 18, 15, 13, 16, 24, 33, 36, 40, 38, 42, 35, 28 };
        cui_series s[2] = { { "API", api, 12, CUI_COLOR_SERIES(3) }, { "Web", web, 12, CUI_COLOR_SERIES(0) } };
        cui_line_chart(ui, "requests", s, 2, .labels = hours, .label_count = 12, .dither = true, .stacked = true, .smooth = true, .height = 220, .format = "%.0fk");
    }
    section(ui, "Pattern meter", "Each part has a color and a density, so it reads in grayscale too. Hover a part or a legend entry.");
    cui_card(ui, .gap = 24) {
        static const cui_meter_part parts[] = { { "Photos", 81 }, { "Video", 58 }, { "Apps", 41 }, { "Docs", 32 } };
        static bool gray;
        cui_pattern_meter(ui, "storage", parts, 4, .total = 256, .label = "Storage", .unit = "GB", .grayscale = gray);
        cui_checkbox(ui, "Grayscale", &gray);
    }
    section(ui, "Stacked bars and gauges", NULL);
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16) {
        cui_card(ui) {
            static const float x[8] = { 112, 124, 131, 128, 140, 152, 149, 161 };   /* test runs per week */
            static const float y[8] = { 9, 11, 8, 12, 10, 9, 13, 8 };
            static const float z[8] = { 4, 6, 3, 5, 7, 4, 6, 3 };
            cui_series s[3] = { { "Passed", x, 8, { 0 } }, { "Retried", y, 8, { 0 } }, { "Failed", z, 8, { 0 } } };
            static const char *weeks[8] = { "W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8" };
            cui_bar_chart(ui, "runs", s, 3, .labels = weeks, .label_count = 8, .stacked = true, .height = 200);
        }
        cui_card(ui, .dir = CUI_ROW, .gap = 24, .align = CUI_ALIGN_CENTER) {
            cui_gauge(ui, "CPU", 0.42f, 130);
            cui_gauge(ui, "Memory", 0.81f, 130);
        }
    }
}

static void dialogs(cui_ctx *ui, gallery *g) {
    cui_dialog(ui, "Edit profile", &g->dialog_open, .description = "Changes are visible to your team.") {
        cui_input(ui, "dlg-name", g->name, sizeof g->name, .label = "Name");
        cui_input(ui, "dlg-email", g->email, sizeof g->email, .label = "Email", .icon = CUI_ICON_MAIL);
        cui_switch(ui, "Public profile", &g->wifi);
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Cancel")) g->dialog_open = false;
            if (cui_button(ui, "Save changes", .variant = CUI_BUTTON_PRIMARY)) {
                g->dialog_open = false;
                cui_toast(ui, "Profile updated", .tone = CUI_TONE_SUCCESS);
            }
        }
    }
    cui_dialog(ui, "Delete this project?", &g->confirm_open, .description = "This removes the project and its history for everyone. You cannot undo this.", .width = 400) {
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Cancel##confirm")) g->confirm_open = false;
            if (cui_button(ui, "Delete project", .variant = CUI_BUTTON_DANGER)) {
                g->confirm_open = false;
                cui_toast(ui, "Project deleted", .tone = CUI_TONE_DANGER, .action = "Undo");
            }
        }
    }
    cui_sheet(ui, "Notifications", &g->sheet_open, .description = "Choose what we tell you about.") {
        cui_switch(ui, "Comments", &g->newsletter, .description = "When someone replies to you.");
        cui_switch(ui, "Mentions", &g->wifi, .description = "When someone mentions you.");
        cui_switch(ui, "Weekly digest", &g->bluetooth, .description = "A summary every Monday.");
        cui_separator(ui);
        cui_label(ui, "Recent");
        cui_list_item(ui, "Jonas commented on Release 1.0", .avatar = "Jonas Berg", .subtitle = "2 minutes ago");
        cui_list_item(ui, "Priya invited you to Engines", .avatar = "Priya Nair", .subtitle = "1 hour ago");
    }
    static const cui_command commands[] = {
        { "Go to Overview", CUI_ICON_LAYOUT_DASHBOARD, "Navigation", NULL },
        { "Go to Buttons", CUI_ICON_ZAP, "Navigation", NULL },
        { "Go to Inputs", CUI_ICON_TYPE, "Navigation", NULL },
        { "Go to Charts", CUI_ICON_CHART_LINE, "Navigation", NULL },
        { "Toggle dark mode", CUI_ICON_MOON, "Preferences", "Ctrl J" },
        { "Change accent color", CUI_ICON_PALETTE, "Preferences", NULL },
        { "New project", CUI_ICON_PLUS, "Actions", "Ctrl N" },
        { "Invite teammate", CUI_ICON_USER_PLUS, "Actions", NULL },
        { "Open settings", CUI_ICON_SETTINGS, "Actions", "Ctrl ," },
    };
    int chosen = cui_command_palette(ui, &g->palette_open, commands, (int)(sizeof commands / sizeof commands[0]));
    switch (chosen) {
    case 0: g->page = PAGE_OVERVIEW; break;
    case 1: g->page = PAGE_BUTTONS; break;
    case 2: g->page = PAGE_INPUTS; break;
    case 3: g->page = PAGE_CHARTS; break;
    case 4: g->dark = !g->dark; cui_set_theme(ui, g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT); break;
    case 5: g->accent = (g->accent + 1) % CUI_ACCENT_COUNT; cui_set_accent(ui, (cui_accent)g->accent); break;
    case 6: g->page = PAGE_FEEDBACK; break;
    case 7: g->dialog_open = true; break;
    case 8: g->sheet_open = true; break;
    default: break;
    }
}

void gallery_frame(cui_ctx *ui, gallery *g) {
    if (!cui_wants_keyboard(ui)) {
        if (cui_shortcut(ui, CUI_MOD_PRIMARY, CUI_KEY_K)) g->palette_open = !g->palette_open;
        if (cui_shortcut(ui, CUI_MOD_PRIMARY, CUI_KEY_J)) { g->dark = !g->dark; cui_set_theme(ui, g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT); }
    }
    /* below 760 points the page list moves into a drawer behind a menu button */
    bool narrow = cui_viewport_size(ui).x < 760;
    if (!narrow) g->drawer = false;
    cui_row(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_STRETCH, .bg = CUI_COLOR_BACKGROUND) {
        if (!narrow) { sidebar(ui, g, false); cui_separator(ui); }
        cui_scroll(ui, .id = "main", .width = CUI_FILL, .fade_edges = true) {
            cui_row(ui, .justify = CUI_JUSTIFY_CENTER, .align = CUI_ALIGN_START) {
                cui_column(ui, .width = CUI_FILL, .max_width = 920, .pad_x = narrow ? 16 : 40, .pad_top = narrow ? 12 : 36, .pad_bottom = 64, .gap = 20) {
                    if (narrow) cui_row(ui, .gap = 8, .width = CUI_FILL) {
                        if (cui_icon_button(ui, CUI_ICON_MENU, .tooltip = "Components")) g->drawer = true;
                        cui_text(ui, "CUI", .font = CUI_FONT_MEDIUM, .size = 15);
                    }
                    if (g->page != PAGE_OVERVIEW) {
                        cui_column(ui, .gap = 6, .pad_bottom = 8) {
                            cui_heading(ui, g->page < PAGE_COUNT ? page_names[g->page] : gallery_more_name(g->page - PAGE_COUNT), 2);
                        }
                    }
                    if (g->page >= PAGE_COUNT) gallery_more_page(ui, g->page - PAGE_COUNT);
                    switch (g->page) {
                    case PAGE_OVERVIEW: page_overview(ui, g); break;
                    case PAGE_BUTTONS: page_buttons(ui, g); break;
                    case PAGE_INPUTS: page_inputs(ui, g); break;
                    case PAGE_SELECTION: page_selection(ui, g); break;
                    case PAGE_NAVIGATION: page_navigation(ui, g); break;
                    case PAGE_OVERLAYS: page_overlays(ui, g); break;
                    case PAGE_FEEDBACK: page_feedback(ui, g); break;
                    case PAGE_DATA: page_data(ui, g); break;
                    case PAGE_CHARTS: page_charts(ui, g); break;
                    }
                }
            }
        }
    }
    cui_sheet(ui, "CUI", &g->drawer, .side = CUI_SIDE_LEFT, .size = 280) { sidebar(ui, g, true); }
    dialogs(ui, g);
}
