/* Renders small focused scenes of each component for the documentation.
 *   cui_component_shots <out-dir> [scene] */
#include <cui/cui.h>
#include <cui/cui_render.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icon_list.h"

typedef void (*scene_fn)(cui_ctx *ui, int frame);

static bool b_on = true, b_off = false, b_check = true, b_check2 = false;
static float f_vol = 62, f_lo = 25, f_hi = 70;
static int i_seg = 1, i_tab = 0, i_radio = 1, i_rating = 4, i_sel = 2, i_page = 2;
static char s_name[64] = "Maya Okafor", s_email[64] = "", s_pw[32] = "secret", s_search[32] = "", s_bio[256] = "Writing software that feels calm.";
static double d_qty = 4;
static cui_date d_date = { 2026, 11, 14 };
static bool open_true = true;

/* Scenes lay out at whatever width they get: containers fill up to their
 * documented width (UPTO), side by side parts wrap below each other, and the
 * frame margin tightens on a phone. */
#define UPTO(w) .width = CUI_FILL, .max_width = (w)
#define FLEX(lo, hi) .width = CUI_FILL, .min_width = (lo), .max_width = (hi)
/* the scene's documented width; the frame tightens only when a scene is shown narrower than that, on a phone */
static float g_scene_w = 0;
static float frame_pad(cui_ctx *ui) {
    float vw = cui_viewport_size(ui).x;
    return vw < 360 && (g_scene_w <= 0 || vw < g_scene_w - 0.5f) ? 16 : 28;
}

static void frame_wrap(cui_ctx *ui, void (*body)(cui_ctx *ui)) {
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 16, .align = CUI_ALIGN_START, .justify = CUI_JUSTIFY_CENTER) {
        body(ui);
    }
}

#define SCENE(name) static void body_##name(cui_ctx *ui); static void scene_##name(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_##name); } static void body_##name(cui_ctx *ui)

SCENE(buttons) {
    cui_row(ui, .gap = 8, .wrap = true) {
        cui_button(ui, "Primary", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_PLUS);
        cui_button(ui, "Secondary");
        cui_button(ui, "Outline", .variant = CUI_BUTTON_OUTLINE);
        cui_button(ui, "Ghost", .variant = CUI_BUTTON_GHOST);
        cui_button(ui, "Danger", .variant = CUI_BUTTON_DANGER);
        cui_button(ui, "Saving", .variant = CUI_BUTTON_PRIMARY, .loading = true);
        cui_icon_button(ui, CUI_ICON_SETTINGS, .tooltip = "Settings");
    }
}
SCENE(segmented) {
    const char *items[] = { "List", "Board", "Timeline" };
    const char *icons[] = { CUI_ICON_LIST, CUI_ICON_LAYOUT_DASHBOARD, CUI_ICON_HISTORY };
    cui_segmented(ui, "s", items, 3, &i_seg, .icons = icons);
}
SCENE(toggles) {
    cui_row(ui, .gap = 40, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .gap = 14) {
            cui_checkbox(ui, "Email notifications", &b_check, .description = "Product news, monthly.");
            cui_checkbox(ui, "Marketing", &b_check2);
        }
        cui_column(ui, .gap = 14) {
            cui_switch(ui, "Wi-Fi", &b_on);
            cui_switch(ui, "Bluetooth", &b_off);
        }
        cui_column(ui, .gap = 14) {
            cui_radio(ui, "Monthly", &i_radio, 0);
            cui_radio(ui, "Yearly", &i_radio, 1);
        }
    }
}
SCENE(slider) {
    cui_column(ui, .gap = 20, UPTO(420)) {
        cui_slider(ui, "Volume", &f_vol, 0, 100, .show_value = true);
        cui_range_slider(ui, "Price", &f_lo, &f_hi, 0, 100);
    }
}
SCENE(inputs) {
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 240, .gap = 16, UPTO(620)) {
        cui_input(ui, "name", s_name, sizeof s_name, .label = "Name");
        cui_input(ui, "email", s_email, sizeof s_email, .label = "Email", .icon = CUI_ICON_MAIL, .placeholder = "you@company.com", .hint = "Used to sign in and for receipts.");
        cui_input(ui, "pw", s_pw, sizeof s_pw, .label = "Password", .password = true, .error = "Use 8 or more characters.");
        cui_search(ui, "q", s_search, sizeof s_search, .label = "Search");
    }
}
SCENE(textarea) {
    cui_column(ui, UPTO(480)) { cui_textarea(ui, "bio", s_bio, sizeof s_bio, .label = "Bio", .height = 90, .hint = "Shown on your profile."); }
}
SCENE(pickers) {
    static const char *items[] = { "Canada", "Denmark", "Germany", "Japan" };
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_END, .wrap = true) {
        cui_select(ui, "country", items, 4, &i_sel, .label = "Country", .width = CUI_PX(200));
        cui_date_picker(ui, "date", &d_date, .label = "Due date", .width = CUI_PX(200));
        cui_number(ui, "qty", &d_qty, 0, 10, .label = "Seats", .width = CUI_PX(160));
    }
}
static void scene_select_open(cui_ctx *ui, int f) {
    static const char *items[] = { "Canada", "Denmark", "Germany", "Japan", "Kenya" };
    if (f < 0) {
        f = -f - 1;
        if (f == 3) cui_input_mouse_move(ui, 60, 75);
        if (f == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f == 10) cui_input_mouse_move(ui, 80, 175);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_START) {
        cui_select(ui, "open", items, 5, &i_sel, .label = "Country", .width = CUI_PX(240));
    }
}
static void scene_calendar(cui_ctx *ui, int f) {
    if (f < 0) return;
    (void)f;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_START) {
        cui_card(ui, .pad = 16) { cui_calendar(ui, "cal", &d_date); }
    }
}
/* the months of the year, after a click on the calendar's title */
static void scene_calendar_months(cui_ctx *ui, int f) {
    static cui_date d = { 2026, 11, 14 };
    if (f < 0) {
        f = -f - 1;
        if (f == 3) cui_input_mouse_move(ui, 80, 59);
        if (f == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f == 6) cui_input_mouse_move(ui, 300, 340);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_START) {
        cui_card(ui, .pad = 16) { cui_calendar(ui, "cal", &d); }
    }
}
SCENE(tabs) {
    const char *t[] = { "Overview", "Analytics", "Reports", "Settings" };
    cui_column(ui, UPTO(520), .gap = 20) {
        cui_tabs(ui, "tabs", t, 4, &i_tab);
        const char *t2[] = { "Account", "Password", "Billing" };
        const char *ic[] = { CUI_ICON_USER, CUI_ICON_LOCK, CUI_ICON_CREDIT_CARD };
        int k = 0;
        cui_tabs(ui, "pill", t2, 3, &k, .variant = CUI_TABS_PILL, .icons = ic);
    }
}
SCENE(navigation) {
    cui_column(ui, .gap = 18, .width = CUI_FILL) {
        const char *c[] = { "Home", "Projects", "CUI" };
        cui_breadcrumb(ui, c, 3);
        cui_pagination(ui, "p", &i_page, 10);
        const char *steps[] = { "Account", "Workspace", "Invite", "Done" };
        cui_box(ui, UPTO(560)) { cui_stepper(ui, steps, 4, 1); }
    }
}
static void scene_sidebar(cui_ctx *ui, int f) {
    if (f < 0) return;
    (void)f;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_SURFACE_MUTED, .pad = 12, .gap = 2) {
        cui_nav_section(ui, "Workspace");
        cui_nav_item(ui, "Inbox", .icon = CUI_ICON_INBOX, .badge = "12", .selected = true);
        cui_nav_item(ui, "Projects", .icon = CUI_ICON_FOLDER);
        cui_nav_item(ui, "Calendar", .icon = CUI_ICON_CALENDAR);
        cui_nav_item(ui, "Settings", .icon = CUI_ICON_SETTINGS);
    }
}
static void scene_menu(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        if (f == 3) cui_input_mouse_move(ui, 60, 46);
        if (f == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f == 10) cui_input_mouse_move(ui, 80, 130);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_START) {
        cui_menu(ui, "Options", .icon = CUI_ICON_SETTINGS) {
            cui_menu_label(ui, "Project");
            cui_menu_item(ui, "Rename", .icon = CUI_ICON_PENCIL, .shortcut = "F2");
            cui_menu_item(ui, "Duplicate", .icon = CUI_ICON_COPY, .shortcut = "Ctrl D");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Delete", .icon = CUI_ICON_TRASH_2, .danger = true);
        }
    }
}
static void scene_dialog(cui_ctx *ui, int f) {
    if (f < 0) return;
    (void)f;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 10) {
        cui_heading(ui, "Projects", 3);
        cui_skeleton(ui, CUI_PCT(80), 14); cui_skeleton(ui, CUI_PCT(60), 14); cui_skeleton(ui, CUI_PCT(70), 14);
    }
    cui_dialog(ui, "Delete project?", &open_true, .description = "This removes the project for everyone. You cannot undo this.", .width = 380) {
        cui_dialog_footer(ui) { cui_button(ui, "Cancel"); cui_button(ui, "Delete project", .variant = CUI_BUTTON_DANGER); }
    }
}
static void scene_toast(cui_ctx *ui, int f) {
    if (f < 0) return;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND) {}
    if (f == 1) cui_toast(ui, "Upload failed", .description = "The file is larger than 25 MB.", .tone = CUI_TONE_DANGER);
    if (f == 6) cui_toast(ui, "Changes saved", .description = "Your profile is up to date.", .tone = CUI_TONE_SUCCESS);
}
static void scene_tooltip(cui_ctx *ui, int f) {
    if (f < 0) { if (f == -3) cui_input_mouse_move(ui, 46, 102); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .justify = CUI_JUSTIFY_END) {
        cui_row(ui, .gap = 4) {
            cui_icon_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .shortcut = "Ctrl B");
            cui_icon_button(ui, CUI_ICON_ITALIC, .tooltip = "Italic");
            cui_icon_button(ui, CUI_ICON_LINK, .tooltip = "Insert link");
        }
    }
}
SCENE(alerts) {
    cui_column(ui, .gap = 10, UPTO(560)) {
        cui_alert(ui, "Payment received", .description = "Your invoice was paid on November 5.", .tone = CUI_TONE_SUCCESS);
        cui_alert(ui, "Storage almost full", .description = "You have used 92% of your plan.", .tone = CUI_TONE_WARNING, .dismissible = true);
    }
}
SCENE(feedback) {
    cui_column(ui, .gap = 18, UPTO(460)) {
        cui_progress(ui, 0.68f, .label = "Uploading", .show_value = true);
        cui_meter(ui, "Storage", 18.4f, 20, "GB");
        cui_row(ui, .gap = 14, .wrap = true, .align = CUI_ALIGN_CENTER) { cui_spinner(ui, 18); cui_spinner(ui, 24); cui_skeleton(ui, CUI_PX(180), 12); }
    }
}
SCENE(dither_progress) {
    cui_column(ui, .gap = 22, UPTO(560)) {
        cui_progress(ui, 0.62f, .label = "Exporting clip.mp4", .show_value = true, .dither = true);
        cui_progress(ui, 0, .label = "Syncing library", .indeterminate = true, .dither = true);
    }
}
/* every dither spinner style in a tile with what it is for, then the inline sizes */
typedef struct { const char *name, *use; cui_spinner_style style; float size; } ds_tile;
static void ds_tiles(cui_ctx *ui, const ds_tile *t, int n) {
    cui_box(ui, .dir = CUI_GRID, .columns = 3, .min_column_width = 200, .gap = 12, UPTO(740)) {
        for (int k = 0; k < n; k++)
            cui_column(ui, .width = CUI_FILL, .pad = 14, .gap = 6, .align = CUI_ALIGN_CENTER, .radius = 14, .border = 1, .bg = CUI_COLOR_BACKGROUND) {
                cui_column(ui, .height = CUI_PX(72), .justify = CUI_JUSTIFY_CENTER) { cui_spinner(ui, t[k].size, .style = t[k].style); }
                cui_text(ui, t[k].name, .font = CUI_FONT_MEDIUM, .size = 13);
                cui_text(ui, t[k].use, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .align = CUI_TEXT_CENTER, .width = CUI_FILL);
            }
    }
}
static void ds_inline(cui_ctx *ui) {
    cui_row(ui, .gap = 14, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER, .wrap = true, UPTO(740)) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 14, .pad_y = 8, .radius = 10, .bg = CUI_COLOR_FOREGROUND) {
            cui_spinner(ui, 14, .style = CUI_SPINNER_COMET, .color = CUI_COLOR_BACKGROUND);
            cui_text(ui, "Saving\xe2\x80\xa6", .font = CUI_FONT_MEDIUM, .size = 13, .color = CUI_COLOR_BACKGROUND);
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 11, .pad_y = 6, .radius = 999, .border = 1, .bg = CUI_COLOR_SURFACE) {
            cui_spinner(ui, 14, .style = CUI_SPINNER_ORBIT);
            cui_text(ui, "Syncing 24 files", .size = 12.5f, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad_x = 11, .pad_y = 6, .radius = 999, .border = 1, .bg = CUI_COLOR_SURFACE) {
            cui_spinner(ui, 14, .style = CUI_SPINNER_RIPPLE);
            cui_text(ui, "Connecting", .size = 12.5f, .color = CUI_COLOR_TEXT_SECONDARY);
        }
    }
}
SCENE(dither_spinners) {
    static const ds_tile t[] = {
        { "Comet ring", "A head with a tail that thins out dot by dot.", CUI_SPINNER_COMET, 56 },
        { "Dot matrix", "Busy states in dense layouts; a wave runs through a 5\xc3\x97" "5 grid.", CUI_SPINNER_DOT_MATRIX, 56 },
        { "Fill and drain", "Steps in a job: fills from the bottom, then dissolves away.", CUI_SPINNER_FILL_DRAIN, 52 },
        { "Radar sweep", "Searching or scanning; found points light up as it passes.", CUI_SPINNER_RADAR, 64 },
        { "Ripple", "Connecting, pairing or waiting for the other side.", CUI_SPINNER_RIPPLE, 64 },
        { "Transfer", "Chevrons stream up for sending, down for receiving.", CUI_SPINNER_TRANSFER, 64 },
        { "Orbit", "Syncing; two dots circle and leave trails that dissolve.", CUI_SPINNER_ORBIT, 60 },
        { "Bouncing dots", "Typing and chat waits; each dot leaves a short trail.", CUI_SPINNER_BOUNCE, 56 },
        { "Document scan", "Indexing or importing: a scan line reads the page.", CUI_SPINNER_SCAN, 64 },
        { "Pulse", "Recording or live: a soft disc breathes in density.", CUI_SPINNER_PULSE, 56 },
        { "Spiral", "A calm indefinite wait for long background work.", CUI_SPINNER_SPIRAL, 64 },
        { "Plasma", "Generating images or audio: slow moving noise in a tile.", CUI_SPINNER_PLASMA, 56 },
    };
    ds_tiles(ui, t, (int)(sizeof t / sizeof t[0]));
    ds_inline(ui);
}
/* the status spinner while busy and a moment after each result, large and inline */
SCENE(status_spinner) {
    static const cui_status st[3] = { CUI_STATUS_BUSY, CUI_STATUS_SUCCESS, CUI_STATUS_ERROR };
    static const char *names[3] = { "Uploading", "Uploaded", "Upload failed" };
    cui_row(ui, .gap = 48, .align = CUI_ALIGN_END, .justify = CUI_JUSTIFY_CENTER, .wrap = true, UPTO(560)) {
        for (int k = 0; k < 3; k++) {
            cui_push_id_int(ui, k);
            cui_column(ui, .gap = 12, .align = CUI_ALIGN_CENTER) {
                cui_status_spinner(ui, "big", st[k], .size = 56);
                cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                    cui_status_spinner(ui, "small", st[k], .size = 16);
                    cui_text(ui, names[k], .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
                }
            }
            cui_pop_id(ui);
        }
    }
}
/* placeholders for a message and a card, drawn in dots under one passing band */
SCENE(dither_skeleton) {
    cui_row(ui, .gap = 20, .wrap = true, .align = CUI_ALIGN_START, UPTO(640)) {
        cui_column(ui, .width = CUI_FILL, .min_width = 240, .pad = 16, .radius = 14, .border = 1) {
            cui_skeleton_group(ui, .dither = true, .gap = 12) {
                cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                    cui_skeleton(ui, CUI_PX(32), 32);
                    cui_column(ui, .gap = 8, .width = CUI_FILL) { cui_skeleton(ui, CUI_PCT(60), 8); cui_skeleton(ui, CUI_PCT(40), 8); }
                }
                cui_skeleton(ui, CUI_FILL, 8);
                cui_skeleton(ui, CUI_PCT(80), 8);
            }
        }
        cui_column(ui, .width = CUI_FILL, .min_width = 240, .pad = 16, .radius = 14, .border = 1) {
            cui_skeleton_group(ui, .dither = true, .gap = 10) {
                cui_skeleton(ui, CUI_FILL, 72);
                cui_skeleton(ui, CUI_PCT(70), 10);
                cui_skeleton(ui, CUI_PCT(45), 10);
            }
        }
    }
}
static cui_id pm_bar;   /* the first meter's bar, for the pointer */
static void pm_meter(cui_ctx *ui, const char *id, bool gray) {
    static const cui_meter_part parts[] = { { "Photos", 81 }, { "Video", 58 }, { "Apps", 41 }, { "Docs", 32 } };
    cui_pattern_meter(ui, id, parts, 4, .total = 256, .label = "Storage", .unit = "GB", .grayscale = gray);
}
SCENE(pattern_meter) {
    cui_column(ui, .gap = 28, UPTO(600)) {
        pm_meter(ui, "storage", false);
        pm_bar = cui_last_id(ui);
        pm_meter(ui, "storage-gray", true);
    }
}
/* the pointer on the second part of the first meter */
static void scene_pattern_meter_hover(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, pm_bar);
        if (-f - 1 == 4) cui_input_mouse_move(ui, r.x + r.w * 0.42f, r.y + r.h * 0.5f);
        return;
    }
    frame_wrap(ui, body_pattern_meter);
}
SCENE(badges) {
    cui_column(ui, .gap = 16) {
        cui_row(ui, .gap = 8, .wrap = true) {
            cui_badge(ui, "Draft");
            cui_badge(ui, "New", .tone = CUI_TONE_ACCENT);
            cui_badge(ui, "Active", .tone = CUI_TONE_SUCCESS, .dot = true);
            cui_badge(ui, "Pending", .tone = CUI_TONE_WARNING, .dot = true);
            cui_badge(ui, "Failed", .tone = CUI_TONE_DANGER, .dot = true);
            cui_badge(ui, "Beta", .solid = true);
        }
        cui_row(ui, .gap = 14, .wrap = true) {
            cui_avatar(ui, "Jonas Berg", .size = 40, .status = "online");
            cui_avatar(ui, "Priya Nair", .size = 40);
            static const char *team[] = { "Kwame Asante", "Rosa Delgado", "Daniel Mensah", "Elif Kaya", "Ben Carter", "Omar Haddad" };
            cui_avatar_group(ui, team, 6, 4, 32);
            cui_kbd(ui, "Ctrl Shift P");
        }
    }
}
SCENE(cards) {
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 260, .gap = 16, UPTO(640)) {
        static const float s1[] = { 3, 5, 4, 7, 6, 9, 11 };
        cui_metric(ui, "Weekly installs", "3,912", .delta = 8.1f, .delta_label = "+8.1% vs last week", .spark = s1, .spark_count = 7, .icon = CUI_ICON_DOWNLOAD);
        cui_card(ui, .gap = 12) {
            cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
                cui_column(ui, .gap = 2, .width = CUI_FILL) {
                    cui_label(ui, "Team plan");
                    cui_text(ui, "5 of 8 seats used", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
                }
                cui_badge(ui, "Renews Dec 1");
            }
            cui_progress(ui, 5.0f / 8, .height = 5);
            cui_row(ui, .gap = 8) {
                cui_button(ui, "Add seats", .size = CUI_SIZE_SM);
                cui_button(ui, "Billing", .size = CUI_SIZE_SM, .variant = CUI_BUTTON_GHOST);
            }
        }
    }
}
SCENE(empty) {
    cui_card(ui, .pad = 0, UPTO(520)) {
        cui_empty_state(ui, "No projects yet", .icon = CUI_ICON_FOLDER_OPEN, .description = "A project holds your screens, their builds and who can see them.", .action = "New project");
    }
}
SCENE(table) {
    cui_table_column cols[] = { { "Name", CUI_FILL, CUI_TEXT_LEFT, true }, { "Role", CUI_PX(100), CUI_TEXT_LEFT, false },
                                { "Status", CUI_PX(110), CUI_TEXT_LEFT, false }, { "Amount", CUI_PX(100), CUI_TEXT_RIGHT, false } };
    static cui_table_sort sort = { 0, false };
    static const char *names[] = { "Jonas Berg", "Priya Nair", "Aiko Tanaka" };
    static const char *roles[] = { "Owner", "Admin", "Editor" };
    static const char *st[] = { "Active", "Active", "Invited" };
    static const char *am[] = { "$12,384", "$9,846", "$1,962" };
    cui_box(ui, UPTO(620)) {
        cui_table(ui, "t", cols, 4, .sort = &sort, .selectable = true) {
            for (int i = 0; i < 3; i++) {
                cui_table_row(ui, i == 1);
                cui_table_cell(ui); cui_avatar(ui, names[i], .size = 24); cui_text(ui, names[i], .font = CUI_FONT_MEDIUM, .nowrap = true);
                cui_table_cell_text(ui, roles[i]);
                cui_table_cell(ui); cui_badge(ui, st[i], .tone = i == 2 ? CUI_TONE_WARNING : CUI_TONE_SUCCESS, .dot = true);
                cui_table_cell_text(ui, am[i]);
            }
        }
    }
}
SCENE(tree) {
    static bool a = true, b = true, c = false;
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .wrap = true, .width = CUI_FILL) {
        cui_card(ui, .pad = 10, .gap = 1, FLEX(220, 260)) {
            if (cui_tree_begin(ui, "src", &a, NULL)) {
                if (cui_tree_begin(ui, "widgets", &b, NULL)) {
                    cui_tree_opts o = { 0 }; o.leaf = true;
                    cui_tree_begin(ui, "button.c", NULL, &o);
                    o.selected = true;
                    cui_tree_begin(ui, "slider.c", NULL, &o);
                    cui_tree_end(ui);
                }
                if (cui_tree_begin(ui, "assets", &c, NULL)) cui_tree_end(ui);
                cui_tree_end(ui);
            }
        }
        cui_card(ui, .pad = 8, .gap = 2, FLEX(240, 320)) {
            cui_list_item(ui, "Design review", .avatar = "Jonas Berg", .subtitle = "Tomorrow at 10:00", .trailing = "45m");
            cui_list_item(ui, "Release 1.0", .avatar = "Priya Nair", .subtitle = "Ship it", .trailing = "2h", .selected = true);
        }
    }
}
SCENE(accordion) {
    static bool a = true, b = false;
    cui_card(ui, .pad_y = 4, .gap = 0, UPTO(520)) {
        if (cui_accordion_begin(ui, "Is it dependency free?", &a)) { cui_secondary(ui, "Yes. Plain C99, native windowing, embedded fonts."); cui_accordion_end(ui); }
        if (cui_accordion_begin(ui, "Can I bring my own renderer?", &b)) { cui_secondary(ui, "Yes."); cui_accordion_end(ui); }
    }
}
SCENE(chips) {
    static bool c[4] = { true, false, true, false };
    cui_column(ui, .gap = 14) {
        cui_row(ui, .gap = 8, .wrap = true) {
            const char *n[] = { "Design", "Engineering", "Marketing", "Sales" };
            for (int i = 0; i < 4; i++) cui_chip(ui, n[i], &c[i]);
        }
        cui_rating(ui, "r", &i_rating, 5);
    }
}
SCENE(line_chart) {
    static const float a[] = { 18, 22, 21, 27, 31, 29, 36, 41, 39, 46, 52, 58 }, b[] = { 12, 13, 15, 14, 18, 19, 21, 22, 24, 25, 27, 29 };
    static const char *m[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    cui_series s[2] = { { "Subscriptions", a, 12, { 0 } }, { "Cloud spend", b, 12, { 0 } } };
    cui_card(ui, UPTO(680)) { cui_line_chart(ui, "l", s, 2, .labels = m, .label_count = 12, .area = true, .smooth = true, .height = 200, .format = "$%.0fk"); }
}
/* hovers a point inside the rect an id had last frame, at fractions of its size */
static void hover_id(cui_ctx *ui, cui_id id, int f, int at, float fx, float fy) {
    cui_rect r = cui_get_rect(ui, id);
    if (f == at && r.w > 0) cui_input_mouse_move(ui, r.x + r.w * fx, r.y + r.h * fy);
}
static void body_dither_area(cui_ctx *ui) {
    static const float api[] = { 22, 19, 17, 21, 30, 38, 41, 37, 34, 39, 33, 26 }, web[] = { 18, 15, 13, 16, 24, 33, 36, 40, 38, 42, 35, 28 };
    static const char *h[] = { "00:00", "02:00", "04:00", "06:00", "08:00", "10:00", "12:00", "14:00", "16:00", "18:00", "20:00", "22:00" };
    cui_series s[2] = { { "API", api, 12, CUI_COLOR_SERIES(3) }, { "Web", web, 12, CUI_COLOR_SERIES(0) } };
    cui_card(ui, UPTO(680)) { cui_line_chart(ui, "area", s, 2, .labels = h, .label_count = 12, .dither = true, .stacked = true, .smooth = true, .height = 210, .format = "%.0fk"); }
}
/* the pointer over the afternoon, for the crosshair and the tooltip */
static void scene_dither_area(cui_ctx *ui, int f) {
    if (f < 0) { hover_id(ui, cui_make_id(ui, "area") ^ 0x9107u, -f - 1, 30, 0.66f, 0.5f); return; }
    frame_wrap(ui, body_dither_area);
}
static void body_bar_chart(cui_ctx *ui);
static void scene_bar_chart(cui_ctx *ui, int f) {
    /* April's column, to show the hover band and its value chip */
    if (f < 0) { hover_id(ui, cui_make_id(ui, "b") ^ 0x9107u, -f - 1, 40, 0.56f, 0.7f); return; }
    frame_wrap(ui, body_bar_chart);
}
static void body_bar_chart(cui_ctx *ui) {
    static const float a[] = { 42, 51, 38, 64, 58, 71 }, b[] = { 28, 33, 30, 41, 46, 49 };
    static const char *m[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun" };
    cui_series s[2] = { { "Desktop", a, 6, { 0 } }, { "Mobile", b, 6, { 0 } } };
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_STRETCH, .width = CUI_FILL, .wrap = true) {
        cui_card(ui, .width = CUI_GROW(1), .min_width = 280) { cui_bar_chart(ui, "b", s, 2, .labels = m, .label_count = 6, .height = 180); }
        cui_card(ui, .align = CUI_ALIGN_CENTER) {
            static const float v[] = { 46, 27, 17, 10 };
            static const char *l[] = { "Windows", "macOS", "Linux", "Other" };
            cui_donut_chart(ui, "d", v, l, 4, .height = 150, .format = "%.0f%%");
        }
    }
}
static void scene_palette(cui_ctx *ui, int f) {
    if (f < 0) return;
    static bool open = true;
    (void)f;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND) {}
    static const cui_command cmds[] = {
        { "New project", CUI_ICON_PLUS, "Actions", "Ctrl N" }, { "Invite teammate", CUI_ICON_USER_PLUS, "Actions", NULL },
        { "Toggle dark mode", CUI_ICON_MOON, "Preferences", "Ctrl J" }, { "Open settings", CUI_ICON_SETTINGS, "Preferences", NULL },
    };
    cui_command_palette(ui, &open, cmds, 4);
}

static void scene_icons(cui_ctx *ui, int f) {
    if (f < 0) return;
    int n = (int)(sizeof icon_list / sizeof icon_list[0]);
    bool narrow = cui_viewport_size(ui).x < 360;   /* two columns on a phone */
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = narrow ? 12 : 24) {
        cui_box(ui, .dir = CUI_GRID, .columns = 6, .min_column_width = narrow ? 120 : 140, .gap = 4) {
            for (int i = 0; i < n; i++) {
                cui_row(ui, .gap = 10, .pad = 8, .radius = 10) {
                    cui_icon(ui, icon_list[i].icon, 18, CUI_COLOR_FOREGROUND);
                    cui_text(ui, icon_list[i].name, .size = 12, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
                }
            }
        }
    }
}

SCENE(radio_cards) {
    static int sel = 1;
    static const cui_choice places[] = {
        { "This computer", "Nothing leaves the machine.", CUI_ICON_MONITOR, NULL },
        { "Team server", "Synced every few minutes.", CUI_ICON_SERVER, "Default" },
        { "Both", "Local, plus a nightly upload.", CUI_ICON_LAYERS, NULL },
    };
    cui_box(ui, UPTO(880)) { cui_radio_cards(ui, "places", places, 3, &sel); }
}
SCENE(tag_otp) {
    static char tags[128] = "design, c99, native";
    static char otp[8] = "418";
    static char pw[32] = "correct horse";
    cui_column(ui, .gap = 16, UPTO(460)) {
        cui_tag_input(ui, "tags", tags, sizeof tags, .label = "Topics");
        cui_otp_input(ui, "otp", otp, 6);
        cui_column(ui, .gap = 8) { cui_input(ui, "pw", pw, sizeof pw, .label = "Password", .password = true); cui_password_strength(ui, pw); }
    }
}
SCENE(button_extras) {
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_button_group(ui) {
            cui_button(ui, "Bold", .icon = CUI_ICON_BOLD);
            cui_button(ui, "Italic", .icon = CUI_ICON_ITALIC);
            cui_button(ui, "Underline", .icon = CUI_ICON_UNDERLINE);
        }
        cui_copy_button(ui, "copy", "text");
        cui_hold_button(ui, "Hold to delete", .icon = CUI_ICON_TRASH_2);
    }
}
SCENE(timeline) {
    cui_row(ui, .gap = 32, .align = CUI_ALIGN_START, .wrap = true, .width = CUI_FILL) {
        cui_column(ui, .gap = 0, FLEX(260, 380)) {
            cui_timeline_item(ui, "Deployed to production", .time = "2m ago", .description = "Release 1.0.3 is live.", .icon = CUI_ICON_ROCKET, .tone = CUI_TONE_SUCCESS);
            cui_timeline_item(ui, "Review requested", .time = "1h ago", .icon = CUI_ICON_MESSAGE_SQUARE, .tone = CUI_TONE_INFO);
            cui_timeline_item(ui, "Build failed", .time = "3h ago", .icon = CUI_ICON_CIRCLE_ALERT, .tone = CUI_TONE_DANGER, .last = true);
        }
        cui_progress_ring(ui, "Onboarding", 0.72f, 110);
    }
}

/* ---- extended components ---- */

static void click_id(cui_ctx *ui, cui_id id, int f, int at) {
    cui_rect r = cui_get_rect(ui, id);
    if (f == at) cui_input_mouse_move(ui, r.x + r.w / 2, r.y + r.h / 2);
    if (f == at + 1) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    if (f == at + 2) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
}

SCENE(split_button) {
    static const char *items[] = { "Save as draft", "Save and publish", "Schedule\xe2\x80\xa6" };
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_split_button(ui, "Save", items, 3, .icon = CUI_ICON_SAVE);
        cui_split_button(ui, "Export", items, 3, .variant = CUI_BUTTON_OUTLINE, .icon = CUI_ICON_DOWNLOAD);
        cui_split_button(ui, "Run", items, 3, .variant = CUI_BUTTON_SECONDARY, .icon = CUI_ICON_PLAY);
    }
}
SCENE(toggle_group) {
    static bool fmt[4] = { true, true, false, false };
    static bool al[3] = { false, true, false };
    static const char *labels[] = { "Bold", "Italic", "Underline", "Code" };
    static const char *icons[] = { CUI_ICON_BOLD, CUI_ICON_ITALIC, CUI_ICON_UNDERLINE, CUI_ICON_CODE };
    static const char *views[] = { "List", "Board", "Calendar" };
    static const char *vicons[] = { CUI_ICON_LIST, CUI_ICON_LAYOUT_DASHBOARD, CUI_ICON_CALENDAR };
    cui_row(ui, .gap = 14, .wrap = true) {
        cui_toggle_group(ui, "fmt", labels, 4, fmt, .icons = icons, .icon_only = true);
        cui_toggle_group(ui, "views", views, 3, al, .icons = vicons, .exclusive = true);
    }
}
static int morph_frame;
static void body_morph(cui_ctx *ui) {
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_confirm_button(ui, "Delete project", .icon = CUI_ICON_TRASH_2);
        cui_confirm_button(ui, "Remove member", .icon = CUI_ICON_USER, .timeout = 60);
        cui_status_button(ui, "Save changes", CUI_STATUS_BUSY, .icon = CUI_ICON_SAVE);
        cui_status_button(ui, "Publish", CUI_STATUS_SUCCESS, .success_label = "Published");
    }
}
static void scene_morph(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "Remove member"), -f - 1, 20); return; }
    morph_frame = f;
    frame_wrap(ui, body_morph);
}
static void body_user_menu(cui_ctx *ui) {
    cui_box(ui, UPTO(270)) {
        cui_user_menu(ui, "Maya Okafor", .detail = "maya@fieldwork.studio", .status = "online", .placement = CUI_PLACE_BOTTOM_START) {
            cui_menu_item(ui, "Profile", .icon = CUI_ICON_USER, .shortcut = "Ctrl P");
            cui_menu_item(ui, "Billing", .icon = CUI_ICON_CREDIT_CARD);
            cui_menu_item(ui, "Settings", .icon = CUI_ICON_SETTINGS, .shortcut = "Ctrl ,");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Log out", .icon = CUI_ICON_LOG_OUT);
        }
    }
}
static void scene_user_menu(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "Maya Okafor") ^ 0x05E4u, -f - 1, 20); if (-f - 1 == 26) cui_input_mouse_move(ui, 600, 20); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_user_menu(ui); }
}
static void body_inline_edit(cui_ctx *ui) {
    static char title[64] = "Q3 roadmap", owner[64] = "Jonas Berg", due[64] = "End of September";
    cui_column(ui, .gap = 10, .width = CUI_FILL) {
        cui_inline_edit(ui, "t", title, sizeof title, .font = CUI_FONT_DISPLAY, .size = 26);
        cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER) {
            cui_box(ui, .width = CUI_PX(64)) { cui_muted(ui, "Owner"); }
            cui_inline_edit(ui, "o", owner, sizeof owner);
        }
        cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER) {
            cui_box(ui, .width = CUI_PX(64)) { cui_muted(ui, "Due"); }
            cui_inline_edit(ui, "d", due, sizeof due);
        }
    }
}
static void scene_inline_edit(cui_ctx *ui, int f) {
    if (f < 0) {
        int fr = -f - 1;
        /* hover the title, and open the due date for editing */
        cui_rect t = cui_get_rect(ui, cui_make_id(ui, "t")), d = cui_get_rect(ui, cui_make_id(ui, "d"));
        if (fr == 6 && d.w > 0) { cui_input_mouse_move(ui, d.x + 20, d.y + d.h / 2); cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); }
        if (fr == 7) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (fr == 12 && t.w > 0) cui_input_mouse_move(ui, t.x + t.w - 30, t.y + t.h / 2);
        return;
    }
    frame_wrap(ui, body_inline_edit);
}
SCENE(scrub) {
    static float a = 80, b = 12, c = 24;
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_scrub(ui, "Opacity", &a, 0, 100, .suffix = "%");
        cui_scrub(ui, "Radius", &b, 0, 48, .step = 0.5f, .format = "%.1f", .suffix = "px");
        cui_scrub(ui, "Blur", &c, 0, 80, .suffix = "px", .width = CUI_PX(140));
    }
}
SCENE(shortcut_rec) {
    static cui_key_combo a = { CUI_MOD_CTRL | CUI_MOD_SHIFT, CUI_KEY_P }, b = { 0, CUI_KEY_NONE };
    static double money = 1249.5;
    cui_row(ui, .gap = 12, .align = CUI_ALIGN_END, .wrap = true) {
        cui_shortcut_recorder(ui, "a", &a);
        cui_shortcut_recorder(ui, "b", &b, .placeholder = "Record shortcut");
        cui_box(ui, .width = CUI_PX(220)) { cui_money_input(ui, "m", &money, .code = "USD"); }
    }
}

static void body_multi(cui_ctx *ui) {
    static bool sel[6] = { true, false, true, false, false, true };
    static const char *items[] = { "C", "C++", "Rust", "Zig", "Go", "Swift" };
    cui_box(ui, UPTO(300)) { cui_multi_select(ui, "langs", items, 6, sel, .label = "Languages"); }
}
static void scene_multi(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "langs"), -f - 1, 20); if (-f - 1 == 26) cui_input_mouse_move(ui, 600, 10); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_multi(ui); }
}
static void body_range(cui_ctx *ui) {
    static cui_date_range r = { { 2026, 11, 3 }, { 2026, 11, 14 } };
    cui_box(ui, UPTO(300)) { cui_date_range_picker(ui, "range", &r, .width = CUI_FILL); }
}
static void scene_range(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "range"), -f - 1, 20); if (-f - 1 == 26) cui_input_mouse_move(ui, 10, 10); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_range(ui); }
}
static void body_time(cui_ctx *ui) {
    static int h = 9, m = 30, h2 = 18, m2 = 45;
    cui_row(ui, .gap = 12, .wrap = true) {
        cui_time_picker(ui, "start", &h, &m);
        cui_time_picker(ui, "end", &h2, &m2, .twelve_hour = true, .minute_step = 15);
    }
}
static void scene_time(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "end"), -f - 1, 20); if (-f - 1 == 26) cui_input_mouse_move(ui, 10, 10); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_time(ui); }
}
SCENE(wheels) {
    static int day = 13, month = 10, year = 6, dh = 7, dm = 25;
    static const char *days[31], *months[12], *years[12];
    static char dbuf[31][4], ybuf[12][6];
    static const char *mn[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    for (int k = 0; k < 31; k++) { snprintf(dbuf[k], sizeof dbuf[k], "%d", k + 1); days[k] = dbuf[k]; }
    for (int k = 0; k < 12; k++) { months[k] = mn[k]; snprintf(ybuf[k], sizeof ybuf[k], "%d", 2020 + k); years[k] = ybuf[k]; }
    cui_row(ui, .gap = 40, .align = CUI_ALIGN_CENTER, .wrap = true) {
        bool narrow = cui_viewport_size(ui).x < 360;
        cui_card(ui, .pad = 8) cui_row(ui, .gap = 0) {
            cui_wheel_picker(ui, "day", days, 31, &day, .width = CUI_PX(narrow ? 52 : 64), .loop = true, .join_right = true);
            cui_wheel_picker(ui, "month", months, 12, &month, .width = CUI_PX(narrow ? 112 : 128), .loop = true, .join_left = true, .join_right = true);
            cui_wheel_picker(ui, "year", years, 12, &year, .width = CUI_PX(narrow ? 68 : 80), .join_left = true);
        }
        cui_time_dial(ui, "dial", &dh, &dm, 232);
    }
}
SCENE(rulers) {
    static float height = 172, lo = 40, hi = 160;
    static const float bins[] = { 2, 4, 7, 12, 18, 26, 31, 29, 34, 40, 37, 30, 24, 22, 18, 15, 11, 9, 7, 5, 4, 3, 2, 1 };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, .width = CUI_GROW(1), .min_width = 280) { cui_ruler_slider(ui, "Height", &height, 120, 220, .unit = "cm"); }
        cui_card(ui, .width = CUI_GROW(1), .min_width = 280, .gap = 6) {
            cui_histogram_slider(ui, "Price", bins, 24, &lo, &hi, 0, 240, .format = "$%.0f");
            cui_muted(ui, "1,204 listings in range");
        }
    }
}
/* a channel's level for the demo meters: a beat with some movement, the same on every run */
static float mixer_level(int ch, int tick) {
    float t = tick / 60.0f;
    float beat = fmodf(t * 2.1f + ch * 0.37f, 1.0f);
    float env = expf(-beat * (3.0f + ch));
    float wob = 0.5f + 0.5f * sinf(t * (1.3f + ch * 0.7f) + ch);
    static const float gain[] = { 0.62f, 0.45f, 0.82f, 0.95f, 0.5f };
    return gain[ch % 5] * (0.25f + 0.75f * env) * (0.75f + 0.25f * wob);
}
SCENE(mixer) {
    static float cutoff = 2400, res = 0.35f, drive = 4.5f, pan = -0.2f, faders[4] = { -6, 0, -12.5f, -3 };
    static int tick = 0;
    tick++;
    static const char *names[4] = { "Vocals", "Guitar", "Bass", "Drums" };
    static const float marks[] = { 6, 0, -6, -12, -24, -48 };
    cui_row(ui, .gap = 24, .wrap = true, .align = CUI_ALIGN_START) {
        cui_card(ui, .pad = 18, .gap = 16) {
            cui_label(ui, "Filter");
            cui_row(ui, .gap = 14, .wrap = true) {
                cui_knob(ui, "Cutoff", &cutoff, 20, 20000, .unit = "Hz", .log_scale = true, .reset = 1000);
                cui_knob(ui, "Resonance", &res, 0, 1, .format = "%.2f");
                cui_knob(ui, "Drive", &drive, 0, 12, .format = "%.1f", .unit = "dB");
                cui_knob(ui, "Pan", &pan, -1, 1, .bipolar = true, .format = "%+.2f");
            }
            float out[2] = { mixer_level(4, tick), mixer_level(3, tick + 7) };
            cui_level_meter(ui, "out", out, 2, .horizontal = true, .length = 200, .segmented = true, .scale = true);
        }
        cui_card(ui, .pad = 18, .dir = CUI_ROW, .gap = 10, .wrap = true) {
            for (int k = 0; k < 4; k++) {
                cui_push_id_int(ui, k);
                float lv[2] = { mixer_level(k, tick), mixer_level(k, tick + 3) * 0.92f };
                cui_fader(ui, names[k], &faders[k], -48, 6, .unit = "dB", .length = 170, .ticks = k == 0 ? marks : NULL,
                          .tick_count = k == 0 ? 6 : 0, .meter = lv, .meter_channels = 2);
                cui_pop_id(ui);
            }
        }
    }
}
static void body_capture(cui_ctx *ui) {
    static cui_texture avatar;
    cui_row(ui, .gap = 20, .width = CUI_FILL, .align = CUI_ALIGN_START, .wrap = true) {
        cui_box(ui, .width = CUI_GROW(1), .min_width = 260) { cui_dropzone(ui, "files", .description = "PNG, JPG or PDF, up to 25 MB", .accept = "png;jpg;pdf", .height = 200); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 260, .gap = 16) {
            cui_signature_pad(ui, "sig", .height = 140);
            cui_row(ui, .gap = 16, .align = CUI_ALIGN_CENTER) {
                cui_avatar_upload(ui, "avatar", "Maya Okafor", &avatar, 56);
                cui_column(ui, .gap = 2) { cui_label(ui, "Profile photo"); cui_muted(ui, "Drop an image on the avatar"); }
            }
        }
    }
}
static void scene_capture(cui_ctx *ui, int f) {
    if (f < 0) {
        int fr = -f - 1;
        cui_rect sig = cui_get_rect(ui, cui_make_id(ui, "sig") ^ 0x5161u);
        if (fr >= 10 && fr <= 40 && sig.w > 0) {
            /* write a quick scribble */
            float t = (fr - 10) / 30.0f;
            float x = sig.x + 50 + t * (sig.w - 120);
            float y = sig.y + 70 + sinf(t * 14) * 18 - t * 10;
            cui_input_mouse_move(ui, x, y);
            if (fr == 10) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        }
        if (fr == 41) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        cui_rect dz = cui_get_rect(ui, cui_make_id(ui, "files"));
        if (fr >= 45) cui_input_file_hover(ui, true, dz.x + dz.w / 2, dz.y + dz.h / 2);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .justify = CUI_JUSTIFY_CENTER) { body_capture(ui); }
}

static void body_hover_card(cui_ctx *ui) {
    cui_row(ui, .gap = 6, .wrap = true) {
        cui_text(ui, "Assigned to");
        cui_link(ui, "@jonas");
        cui_hover_card(ui, "grace-card", .width = 300) {
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_START) {
                cui_avatar(ui, "Jonas Berg", .size = 44, .status = "online");
                cui_column(ui, .gap = 4, .width = CUI_FILL) {
                    cui_text(ui, "Jonas Berg", .font = CUI_FONT_MEDIUM);
                    cui_text(ui, "Engineer on the rendering team.", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
                    cui_row(ui, .gap = 6, .pad_top = 4) {
                        cui_icon(ui, CUI_ICON_CLOCK, 12, CUI_COLOR_TEXT_MUTED);
                        cui_text(ui, "Usually replies within an hour", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
                    }
                }
            }
        }
        cui_text(ui, "and due Friday.");
    }
}
static void scene_hover_card(cui_ctx *ui, int f) {
    if (f < 0) { cui_rect r = cui_get_rect(ui, cui_make_id(ui, "@jonas") ^ 0x11AC0009u); if (-f - 1 == 5) cui_input_mouse_move(ui, r.x + r.w / 2, r.y + r.h / 2); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_hover_card(ui); }
}
static void body_confirm_pop(cui_ctx *ui) {
    if (cui_button(ui, "Delete branch", .icon = CUI_ICON_TRASH_2)) cui_popover_open(ui, "confirm-delete");
    cui_confirm_popover(ui, "confirm-delete", .title = "Delete feature/grid?", .description = "The branch and its 12 commits are removed from the remote.",
                        .confirm_label = "Delete branch", .danger = true);
}
static void scene_confirm_pop(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "Delete branch"), -f - 1, 10); if (-f - 1 == 16) cui_input_mouse_move(ui, 400, 300); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_confirm_pop(ui); }
}
SCENE(expandables) {
    static bool a = true, b = false;
    cui_column(ui, .gap = 14, UPTO(620)) {
        cui_expandable_text(ui, "about",
            "CUI is a component library for C. It draws everything itself, so an app looks the same on Windows, macOS and Linux, and it "
            "ships the parts most apps need: inputs, pickers, menus, dialogs, tables, charts and a command palette.", .lines = 2);
        cui_expandable_card(ui, "Billing", &a, .description = "Pro plan, renews on December 1", .icon = CUI_ICON_CREDIT_CARD, .badge = "Active", .badge_tone = CUI_TONE_SUCCESS) {
            cui_row(ui, .gap = 8) { cui_button(ui, "Change plan", .size = CUI_SIZE_SM); cui_button(ui, "Invoices", .size = CUI_SIZE_SM, .variant = CUI_BUTTON_GHOST); }
        }
        cui_expandable_card(ui, "Security", &b, .description = "Two-factor authentication is on", .icon = CUI_ICON_SHIELD) { cui_text(ui, "Recovery codes"); }
    }
}
SCENE(banner_countdown) {
    static bool open = true;
    cui_column(ui, .gap = 20, .width = CUI_FILL) {
        cui_banner(ui, "CUI 1.1 is out: data grid, kanban and forty new components.", &open, .action = "Read the notes", .dismissible = true);
        cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .wrap = true) {
            cui_countdown(ui, "launch", 2 * 86400 + 5 * 3600 + 42 * 60 + 18);
            cui_countdown(ui, "sale", 5 * 3600 + 42 * 60 + 18, .compact = true, .hide_days = true);
        }
    }
}
SCENE(checklist) {
    static cui_check_step steps[] = {
        { "Create your workspace", "Pick a name and invite your team.", "Create", true },
        { "Connect a repository", "Link GitHub or GitLab to sync issues and pull requests.", "Connect", false },
        { "Import your data", "Bring tasks over from a CSV or another tool.", "Import", false },
        { "Turn on notifications", NULL, NULL, false },
    };
    cui_box(ui, UPTO(440)) { cui_checklist(ui, "onboarding", steps, 4, .title = "Set up your workspace"); }
}
SCENE(toc) {
    static const cui_toc_item toc[] = {
        { "Introduction", "intro", 0 }, { "Install", "install", 0 }, { "With CMake", "cmake", 1 }, { "Without CMake", "nocmake", 1 },
        { "First window", "window", 0 }, { "Next steps", "next", 0 },
    };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, .width = CUI_FILL, .min_width = 280) cui_scroll(ui, .id = "reader", .height = CUI_PX(260), .width = CUI_FILL, .gap = 10, .fade_edges = true) {
            for (int k = 0; k < 6; k++) {
                cui_toc_anchor(ui, toc[k].anchor);
                cui_heading(ui, toc[k].title, toc[k].level ? 4 : 3);
                cui_push_id_int(ui, k);
                cui_text(ui, "Components are functions. They draw themselves, handle input and return what happened.", .color = CUI_COLOR_TEXT_SECONDARY);
                cui_pop_id(ui);
            }
            cui_gap(ui, 200);
        }
        cui_column(ui, .width = CUI_PX(180), .gap = 10) {
            cui_text(ui, "On this page", .size = 12, .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_TEXT_MUTED);
            cui_toc(ui, "reader", toc, 6);
        }
    }
}
static void scene_toc_input(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "reader"));
        if (-f - 1 == 10) cui_input_mouse_move(ui, r.x + 40, r.y + 40);
        if (-f - 1 >= 11 && -f - 1 < 17) cui_input_scroll(ui, 0, -1.2f);
        if (-f - 1 == 18) cui_input_mouse_move(ui, 900, 10);
        return;
    }
    frame_wrap(ui, body_toc);
}
static void body_undo(cui_ctx *ui) { cui_button(ui, "Archive 3 conversations", .icon = CUI_ICON_ARCHIVE); }
static void scene_undo(cui_ctx *ui, int f) {
    if (f < 0) { if (-f - 1 == 5) cui_toast(ui, "3 conversations archived", .action = "Undo", .show_timer = true); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_undo(ui); }
}

/* ---- data, code, conversation, media and planning ---- */
SCENE(data_grid) {
    static const char *const teams[] = { "Engine", "Design", "Platform", "Docs" };
    static const cui_filter_field fields[] = { { "Team", CUI_ICON_USERS, teams, 4 }, { "Status", CUI_ICON_CIRCLE_DOT, teams, 0 } };
    static uint32_t filt[2] = { 1u | 4u, 0 };
    static char search[32];
    cui_filter_bar(ui, "filters", fields, 2, filt, search, sizeof search);
    static const cui_grid_column cols[] = { { "Name", 250, CUI_TEXT_LEFT, true }, { "Team", 140, CUI_TEXT_LEFT, true },
                                            { "Commits", 100, CUI_TEXT_RIGHT, true }, { "Score", 150, CUI_TEXT_RIGHT, true } };
    /* the rows the "Team is Engine or Platform" filter leaves */
    static const char *names[] = { "Jonas Berg", "Lena Fischer", "Sam Whitfield", "Daniel Mensah" };
    static const char *tm[] = { "Engine", "Platform", "Engine", "Platform" };
    static const int commits[] = { 884, 640, 597, 431 };
    static const float score[] = { 97.2f, 88.1f, 84.0f, 71.9f };
    static bool sel[4] = { false, true, true, false };
    static cui_table_sort sort = { 2, true };
    int f0, l0;
    if (cui_data_grid_begin(ui, "people", cols, 4, 4, &f0, &l0, .height = CUI_PX(202), .selected = sel, .sort = &sort)) {
        for (int r = f0; r < l0; r++) {
            char b1[16], b2[16];
            cui_data_grid_row(ui, r);
            cui_data_grid_cell(ui);
            cui_avatar(ui, names[r], .size = 24);
            cui_text(ui, names[r], .size = 13.5f);
            cui_data_grid_cell(ui);
            cui_badge(ui, tm[r]);
            snprintf(b1, sizeof b1, "%d", commits[r]);
            cui_data_grid_cell_text(ui, b1);
            cui_data_grid_cell(ui);
            cui_progress(ui, score[r] / 100, .width = CUI_PX(70), .height = 5);
            snprintf(b2, sizeof b2, "%.1f", (double)score[r]);
            cui_text(ui, b2, .tabular = true, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_data_grid_end(ui);
    }
}
/* a project explorer: a path bar over a tree table, kept in sync. Folders
 * marked lazy load their children after a short pretend delay. */
typedef struct { const char *name; int parent; int bytes; long long stamp; const char *kind; bool lazy; } tt_entry;
static const tt_entry tt_files[] = {
    { "cui", -1, 0, 202611061042LL, "Folder", false },                       /* 0 */
    { "include", 0, 0, 202611041519LL, "Folder", false },                    /* 1 */
    { "cui", 1, 0, 202611041519LL, "Folder", false },                        /* 2 */
    { "cui.h", 2, 100966, 202611061042LL, "C header", false },
    { "cui_app.h", 2, 3277, 202610281130LL, "C header", false },
    { "cui_icons.h", 2, 42906, 202610190904LL, "C header", false },
    { "cui_render.h", 2, 2458, 202609300817LL, "C header", false },
    { "src", 0, 0, 202611061042LL, "Folder", true },                         /* 7 */
    { "render", 7, 0, 202611021655LL, "Folder", true },                      /* 8 */
    { "cui_render_gl.c", 8, 22221, 202610271402LL, "C source", false },
    { "cui_render_sw.c", 8, 34816, 202611021655LL, "C source", false },
    { "platform", 7, 0, 202610301218LL, "Folder", true },                    /* 11 */
    { "cui_platform_glfw.c", 11, 14438, 202610301218LL, "C source", false },
    { "cui_platform_sdl.c", 11, 16896, 202610221034LL, "C source", false },
    { "cui_core.c", 7, 24883, 202611051847LL, "C source", false },
    { "cui_grid.c", 7, 28570, 202611061042LL, "C source", false },
    { "cui_treetable.c", 7, 27034, 202611061037LL, "C source", false },
    { "cui_widgets.c", 7, 53862, 202611030921LL, "C source", false },
    { "docs", 0, 0, 202611061040LL, "Folder", false },                       /* 18 */
    { "images", 18, 0, 202611061040LL, "Folder", true },                     /* 19 */
    { "data-grid.png", 19, 90522, 202610140912LL, "PNG image", false },
    { "path-bar.png", 19, 30925, 202611061040LL, "PNG image", false },
    { "tree-table.png", 19, 72704, 202611061040LL, "PNG image", false },
    { "components-more.md", 18, 76288, 202611061039LL, "Markdown", false },
    { "design.md", 18, 12083, 202610120944LL, "Markdown", false },
    { "tests", 0, 0, 202611061036LL, "Folder", true },                       /* 25 */
    { "CMakeLists.txt", 25, 3379, 202610290811LL, "CMake", false },
    { "component_shots.c", 25, 134758, 202611061036LL, "C source", false },
    { "test_main.c", 25, 217088, 202611061036LL, "C source", false },
    { "CMakeLists.txt", 0, 6042, 202611061042LL, "CMake", false },
    { "LICENSE", 0, 1088, 202603020900LL, "Text", false },
    { "README.md", 0, 9933, 202611011410LL, "Markdown", false },
};
#define TT_N ((int)(sizeof tt_files / sizeof tt_files[0]))
static cui_tree_node tt_nodes[TT_N];
static int tt_root = 0, tt_cursor = 16;
static cui_table_sort tt_sort = { 0, false };
static bool tt_loaded[TT_N];
static double tt_due[TT_N];          /* when a pending load finishes, 0 none */
static char tt_status[128];
static char tt_goto[256];            /* a typed path still waiting for folders to load */

static bool tt_folder(int n) { return strcmp(tt_files[n].kind, "Folder") == 0; }
static int tt_items(int n) { int c = 0; for (int k = 0; k < TT_N; k++) c += tt_files[k].parent == n; return c; }
static int tt_icase(const char *a, const char *b) {
    for (;; a++, b++) {
        int x = (unsigned char)*a, y = (unsigned char)*b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y || !x) return x - y;
    }
}
static int tt_cmp(void *user, int a, int b) {
    (void)user;
    if (tt_folder(a) != tt_folder(b)) return tt_folder(a) ? -1 : 1;   /* folders first, either way */
    int c = 0;
    switch (tt_sort.column) {
    case 1: c = tt_folder(a) ? tt_items(a) - tt_items(b) : (tt_files[a].bytes > tt_files[b].bytes) - (tt_files[a].bytes < tt_files[b].bytes); break;
    case 2: c = (tt_files[a].stamp > tt_files[b].stamp) - (tt_files[a].stamp < tt_files[b].stamp); break;
    case 3: c = tt_icase(tt_files[a].kind, tt_files[b].kind); break;
    default: break;
    }
    if (!c) c = tt_icase(tt_files[a].name, tt_files[b].name);
    return tt_sort.descending ? -c : c;
}
static void tt_link(int folder) {   /* the folder's children arrive */
    int last = -1;
    for (int k = 0; k < TT_N; k++) {
        if (tt_files[k].parent != folder) continue;
        if (last < 0) tt_nodes[folder].first_child = k; else tt_nodes[last].next_sibling = k;
        last = k;
    }
    tt_loaded[folder] = true;
    tt_nodes[folder].flags &= ~(uint32_t)CUI_TREE_LOADING;
    tt_root = cui_tree_sort(tt_nodes, TT_N, tt_root, tt_cmp, NULL);
}
static void tt_init(cui_ctx *ui) {
    for (int k = 0; k < TT_N; k++) {
        tt_nodes[k].parent = tt_files[k].parent;
        tt_nodes[k].first_child = tt_nodes[k].next_sibling = -1;
        tt_nodes[k].flags = tt_folder(k) ? CUI_TREE_HAS_CHILDREN : 0;
    }
    for (int k = 0; k < TT_N; k++) if (tt_folder(k) && !tt_files[k].lazy) tt_link(k);
    tt_link(7);                                                   /* src was opened earlier */
    tt_nodes[0].flags |= CUI_TREE_OPEN;
    tt_nodes[7].flags |= CUI_TREE_OPEN;
    tt_nodes[25].flags |= CUI_TREE_OPEN | CUI_TREE_LOADING;      /* tests is loading */
    tt_due[25] = cui_time(ui) + 2.5;
    tt_nodes[16].flags |= CUI_TREE_SELECTED;
}
static const char *tt_icon(int n) {
    if (tt_folder(n)) return (tt_nodes[n].flags & CUI_TREE_OPEN) ? CUI_ICON_FOLDER_OPEN : CUI_ICON_FOLDER;
    const char *k = tt_files[n].kind;
    if (!strcmp(k, "Markdown") || !strcmp(k, "Text")) return CUI_ICON_FILE_TEXT;
    if (!strcmp(k, "PNG image")) return CUI_ICON_IMAGE;
    return CUI_ICON_FILE;
}
static const char *tt_label(void *user, int n) { (void)user; return tt_files[n].name; }
static void tt_size(char *out, size_t cap, int n) {
    int b = tt_files[n].bytes;
    if (tt_folder(n)) { int c = tt_items(n); snprintf(out, cap, "%d item%s", c, c == 1 ? "" : "s"); }
    else if (b < 1024) snprintf(out, cap, "%d bytes", b);
    else if (b < 1024 * 1024) snprintf(out, cap, "%.1f KB", b / 1024.0);
    else snprintf(out, cap, "%.1f MB", b / (1024.0 * 1024.0));
}
static void tt_when(char *out, size_t cap, long long stamp) {
    static const char *const mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    int mi = (int)(stamp % 100), hh = (int)(stamp / 100 % 100), dd = (int)(stamp / 10000 % 100);
    int mo = (int)(stamp / 1000000 % 100), yy = (int)(stamp / 100000000 % 10000);
    if (yy == 2026 && mo == 11 && dd == 6) snprintf(out, cap, "Today %d:%02d", hh, mi);
    else if (yy == 2026 && mo == 11 && dd == 5) snprintf(out, cap, "Yesterday %d:%02d", hh, mi);
    else snprintf(out, cap, "%s %d, %d", mon[(mo + 11) % 12], dd, yy);
}
static int tt_path(int n, int *out, int cap) {   /* the root first */
    int chain[32], c = 0;
    for (; n >= 0 && c < 32; n = tt_nodes[n].parent) chain[c++] = n;
    int m = 0;
    while (c > 0 && m < cap) out[m++] = chain[--c];
    return m;
}
static void tt_reveal(int n) {   /* open the ancestors and select n alone */
    for (int p = tt_nodes[n].parent; p >= 0; p = tt_nodes[p].parent) tt_nodes[p].flags |= CUI_TREE_OPEN;
    for (int k = 0; k < TT_N; k++) tt_nodes[k].flags &= ~(uint32_t)CUI_TREE_SELECTED;
    tt_nodes[n].flags |= CUI_TREE_SELECTED;
    tt_cursor = n;
}
static void tt_request(cui_ctx *ui, int n) {   /* start loading a folder */
    tt_nodes[n].flags |= CUI_TREE_OPEN | CUI_TREE_LOADING;
    tt_due[n] = cui_time(ui) + 0.6;
}
/* Resolves "cui/src/render" (a leading "cui/" or "/" is optional). Returns
 * the node, -1 when a name is wrong (tt_status says which), -2 while a
 * folder on the way is still loading. */
static int tt_resolve(cui_ctx *ui, const char *path) {
    int cur = 0;
    char name[96];
    const char *s = path;
    while (*s == '/' || *s == ' ') s++;
    if (!strncmp(s, "~/", 2)) s += 2;
    bool first = true;
    while (*s) {
        int len = 0;
        while (s[len] && s[len] != '/') len++;
        snprintf(name, sizeof name, "%.*s", len, s);
        s += len;
        while (*s == '/') s++;
        if (!name[0]) continue;
        if (first && !tt_icase(name, tt_files[0].name)) { first = false; continue; }
        first = false;
        if (!tt_folder(cur)) { snprintf(tt_status, sizeof tt_status, "%s is a file", tt_files[cur].name); return -1; }
        if (!tt_loaded[cur]) { if (!(tt_nodes[cur].flags & CUI_TREE_LOADING)) tt_request(ui, cur); return -2; }
        int found = -1;
        for (int c = tt_nodes[cur].first_child; c >= 0; c = tt_nodes[c].next_sibling) if (!tt_icase(tt_files[c].name, name)) found = c;
        if (found < 0) { snprintf(tt_status, sizeof tt_status, "Nothing named %.40s in %.40s", name, tt_files[cur].name); return -1; }
        cur = found;
    }
    return cur;
}
static void tt_describe(void) {
    int n = 0, last = -1;
    for (int k = 0; k < TT_N; k++) if (tt_nodes[k].flags & CUI_TREE_SELECTED) { n++; last = k; }
    char sz[32];
    if (n == 1) { tt_size(sz, sizeof sz, last); snprintf(tt_status, sizeof tt_status, "%s, %s", tt_files[last].name, sz); }
    else if (n > 1) snprintf(tt_status, sizeof tt_status, "%d items selected", n);
    else snprintf(tt_status, sizeof tt_status, "Nothing selected");
}
SCENE(tree_table) {
    static bool init = false;
    if (!init) { tt_init(ui); tt_describe(); init = true; }
    /* pretend loads finish */
    for (int k = 0; k < TT_N; k++)
        if (tt_due[k] > 0 && cui_time(ui) >= tt_due[k]) { tt_due[k] = 0; tt_link(k); }
    for (int k = 0; k < TT_N; k++) if (tt_due[k] > 0) cui_request_redraw(ui);
    if (tt_goto[0]) {
        int n = tt_resolve(ui, tt_goto);
        if (n != -2) { if (n >= 0) { tt_reveal(n); tt_describe(); } tt_goto[0] = 0; }
    }
    /* the path bar shows where the cursor is */
    int chain[32];
    int depth = tt_path(tt_cursor, chain, 32);
    const char *segs[32];
    for (int k = 0; k < depth; k++) segs[k] = tt_files[chain[k]].name;
    cui_path_bar_result pr = cui_path_bar(ui, "path", segs, depth, .icon = CUI_ICON_FOLDER, .menus = true);
    if (pr.clicked >= 0) { tt_reveal(chain[pr.clicked]); tt_describe(); }
    if (pr.submitted) {
        int n = tt_resolve(ui, pr.submitted);
        if (n >= 0) { tt_reveal(n); tt_describe(); }
        else if (n == -2) { snprintf(tt_goto, sizeof tt_goto, "%s", pr.submitted); snprintf(tt_status, sizeof tt_status, "Opening %.80s", pr.submitted); }
    }
    int seg;
    if (cui_path_bar_menu_begin(ui, "path", &seg)) {   /* the children of segment seg */
        int parent = seg >= 0 && seg < depth ? chain[seg] : 0;
        for (int c = tt_nodes[parent].first_child; c >= 0; c = tt_nodes[c].next_sibling) {
            cui_push_id_int(ui, c);
            bool here = seg + 1 < depth && chain[seg + 1] == c;
            if (cui_menu_item(ui, tt_files[c].name, .icon = tt_icon(c), .checked = here)) { tt_reveal(c); tt_describe(); }
            cui_pop_id(ui);
        }
        cui_path_bar_menu_end(ui);
    }
    static const cui_grid_column cols[] = {
        { "Name", 240, CUI_TEXT_LEFT, true, false, 0 }, { "Size", 96, CUI_TEXT_RIGHT, true, false, 1 },
        { "Modified", 150, CUI_TEXT_LEFT, true, false, 2 }, { "Kind", 110, CUI_TEXT_LEFT, true, false, 3 },
    };
    cui_tree_table_begin(ui, "files", cols, 4, tt_nodes, TT_N, .height = CUI_PX(424), .root = tt_root, .sort = &tt_sort,
                         .cursor = &tt_cursor, .guides = true, .label = tt_label);
    int n;
    while ((n = cui_tree_table_row(ui)) >= 0) {
        char sz[32], when[32];
        cui_tree_table_name(ui, tt_files[n].name, tt_icon(n));
        tt_size(sz, sizeof sz, n);
        cui_data_grid_cell_text(ui, sz);
        tt_when(when, sizeof when, tt_files[n].stamp);
        cui_data_grid_cell_text(ui, when);
        cui_data_grid_cell_text(ui, tt_files[n].kind);
    }
    cui_tree_table_events ev = cui_tree_table_end(ui);
    for (int k = 0; k < ev.load_count; k++) tt_due[ev.load[k]] = cui_time(ui) + 0.6;
    if (ev.sort_changed) tt_root = cui_tree_sort(tt_nodes, TT_N, tt_root, tt_cmp, NULL);
    if (ev.selection_changed) tt_describe();
    if (ev.activated >= 0) {
        int a = ev.activated;
        if (tt_folder(a)) { if (tt_nodes[a].flags & CUI_TREE_OPEN) tt_nodes[a].flags &= ~(uint32_t)CUI_TREE_OPEN; else if (tt_loaded[a]) tt_nodes[a].flags |= CUI_TREE_OPEN; else tt_request(ui, a); }
        else snprintf(tt_status, sizeof tt_status, "Opened %s", tt_files[a].name);
    }
    cui_text(ui, tt_status, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
}

/* a scene graph path with a menu of siblings open, and a long file path in
 * a narrow bar whose middle folders collapse into "..." */
typedef struct { const char *name; int parent; const char *icon; } pb_entry;
static const pb_entry pb_scene[] = {
    { "World", -1, CUI_ICON_GLOBE }, { "Harbor", 0, CUI_ICON_LAYERS }, { "Dock_North", 1, CUI_ICON_LAYERS },
    { "Crane_02", 2, CUI_ICON_BOX }, { "Hook", 3, CUI_ICON_BOX }, { "Cable", 3, CUI_ICON_BOX },
    { "Crane_01", 2, CUI_ICON_BOX }, { "Bollards", 2, CUI_ICON_BOX }, { "Dock_South", 1, CUI_ICON_LAYERS },
    { "Lighthouse", 1, CUI_ICON_BUILDING_2 }, { "Warehouse_A", 1, CUI_ICON_BUILDING_2 }, { "Water", 1, CUI_ICON_BOX },
    { "Town", 0, CUI_ICON_LAYERS }, { "Market", 12, CUI_ICON_BUILDING_2 }, { "Station", 12, CUI_ICON_BUILDING_2 },
    { "Sky", 0, CUI_ICON_LAYERS }, { "Sun", 15, CUI_ICON_SUN }, { "Clouds", 15, CUI_ICON_CLOUD }, { "Main camera", 0, CUI_ICON_CAMERA },
};
#define PB_N ((int)(sizeof pb_scene / sizeof pb_scene[0]))
static int pb_find(int parent, const char *name, int len) {
    for (int k = 0; k < PB_N; k++)
        if (pb_scene[k].parent == parent && (int)strlen(pb_scene[k].name) == len && !strncmp(pb_scene[k].name, name, (size_t)len)) return k;
    return -1;
}
static void scene_path_bar(cui_ctx *ui, int f) {
    static int cur = 4;   /* World/Harbor/Dock_North/Crane_02/Hook */
    static char file_segs[8][48] = { "maya", "projects", "cui", "src", "render", "cui_render_sw.c" };
    static int file_n = 6;
    static char note[96] = "";
    if (f < 0) {
        /* open the menu after Harbor: its children, the siblings of Dock_North */
        int k = -f - 1;
        cui_push_id(ui, "scene-path"); cui_push_id_int(ui, 1);
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "sep"));
        cui_pop_id(ui); cui_pop_id(ui);
        if (k == 3 && r.w > 0) cui_input_mouse_move(ui, r.x + r.w / 2, r.y + r.h / 2);
        if (k == 4 && r.w > 0) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (k == 5 && r.w > 0) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (k == 7) cui_input_mouse_leave(ui);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 16, .align = CUI_ALIGN_START) {
        /* the file path: any typed path is taken as it is */
        const char *fs[8];
        for (int k = 0; k < file_n; k++) fs[k] = file_segs[k];
        cui_row(ui, UPTO(300)) {
            cui_path_bar_result r = cui_path_bar(ui, "file-path", fs, file_n, .icon = CUI_ICON_HOUSE, .text = NULL);
            if (r.clicked >= 0) file_n = r.clicked + 1;
            if (r.submitted) {
                int n = 0;
                const char *s = r.submitted;
                while (*s && n < 8) {
                    while (*s == '/') s++;
                    int len = 0;
                    while (s[len] && s[len] != '/') len++;
                    if (len) { snprintf(file_segs[n], sizeof file_segs[n], "%.*s", len, s); n++; }
                    s += len;
                }
                if (n) file_n = n;
            }
        }
        /* the scene graph: separators list the children of the node before them */
        int chain[16], depth = 0;
        for (int n = cur; n >= 0 && depth < 16; n = pb_scene[n].parent) chain[depth++] = n;
        for (int a = 0, b = depth - 1; a < b; a++, b--) { int t = chain[a]; chain[a] = chain[b]; chain[b] = t; }
        const char *segs[16];
        for (int k = 0; k < depth; k++) segs[k] = pb_scene[chain[k]].name;
        cui_row(ui, UPTO(620)) {
            cui_path_bar_result r = cui_path_bar(ui, "scene-path", segs, depth, .menus = true, .placeholder = "World/Harbor/Lighthouse");
            if (r.clicked >= 0) { cur = chain[r.clicked]; note[0] = 0; }
            if (r.submitted) {
                int n = -1;
                const char *s = r.submitted;
                bool ok = true;
                while (*s && ok) {
                    while (*s == '/') s++;
                    int len = 0;
                    while (s[len] && s[len] != '/') len++;
                    if (len) { int m = pb_find(n, s, len); if (m < 0) ok = false; else n = m; }
                    s += len;
                }
                if (ok && n >= 0) { cur = n; note[0] = 0; }
                else snprintf(note, sizeof note, "No node at %s", r.submitted);
            }
        }
        int seg;
        if (cui_path_bar_menu_begin(ui, "scene-path", &seg)) {
            int parent = seg >= 0 && seg < depth ? chain[seg] : 0;
            for (int k = 0; k < PB_N; k++) {
                if (pb_scene[k].parent != parent) continue;
                bool here = seg + 1 < depth && chain[seg + 1] == k;
                if (cui_menu_item(ui, pb_scene[k].name, .icon = pb_scene[k].icon, .checked = here)) { cur = k; note[0] = 0; }
            }
            cui_path_bar_menu_end(ui);
        }
        if (note[0]) cui_text(ui, note, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
    }
}
SCENE(code_block) {
    cui_code_block(ui, "c",
        "#include <cui/cui.h>\n"
        "\n"
        "/* A counter with a button */\n"
        "static int clicks = 0;\n"
        "\n"
        "void frame(cui_ctx *ui) {\n"
        "    if (cui_button(ui, \"Click me\", .variant = CUI_BUTTON_PRIMARY)) clicks++;\n"
        "    cui_textf(ui, \"Clicked %d times\", clicks);\n"
        "}\n",
        .lang = "c", .title = "counter.c", .line_numbers = true, .highlight_from = 7);
}
/* code editor: the buffer is static, so the live page keeps what you type;
 * the find bar and the caret are set once, then belong to the page */
static char ed_src[16384] =
    "/* tile_cache.c: a small LRU cache of decoded map tiles */\n"
    "#include <stdint.h>\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "\n"
    "#define CACHE_SLOTS 64\n"
    "\n"
    "typedef struct {\n"
    "    uint32_t key;           /* zoom << 24 | y << 12 | x */\n"
    "    uint32_t last_used;\n"
    "    uint8_t *pixels;\n"
    "} tile;\n"
    "\n"
    "static tile cache[CACHE_SLOTS];\n"
    "static uint32_t clock_now;\n"
    "\n"
    "static uint32_t tile_key(int zoom, int x, int y) {\n"
    "    return (uint32_t)zoom << 24 | (uint32_t)y << 12 | (uint32_t)x;\n"
    "}\n"
    "\n"
    "tile *tile_get(int zoom, int x, int y) {\n"
    "    uint32_t key = tile_key(zoom, x, y);\n"
    "    tile *oldest = &cache[0];\n"
    "    for (int i = 0; i < CACHE_SLOTS; i++) {\n"
    "        if (cache[i].key == key && cache[i].pixels) {\n"
    "            cache[i].last_used = ++clock_now;\n"
    "            return &cache[i];\n"
    "        }\n"
    "        if (cache[i].last_used < oldest->last_used) oldest = &cache[i];\n"
    "    }\n"
    "    /* miss: reuse the least recently used slot */\n"
    "    free(oldest->pixels);\n"
    "    oldest->pixels = decode_tile(zoom, x, y);\n"
    "    oldest->key = key;\n"
    "    oldest->last_used = ++clock_now;\n"
    "    return oldest;\n"
    "}\n"
    "\n"
    "void tile_cache_clear(void) {\n"
    "    for (int i = 0; i < CACHE_SLOTS; i++) free(cache[i].pixels);\n"
    "    memset(cache, 0, sizeof cache);\n"
    "}\n";
static void body_code_editor(cui_ctx *ui);
static void scene_code_editor(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        if (t == 3) cui_code_editor_find(ui, "editor", "last_used");
        if (t == 5) {   /* the caret just after the loop's opening brace: its partner lights up */
            const char *at = strstr(ed_src, "CACHE_SLOTS; i++) {\n        if");
            if (at) { int pos = (int)(at - ed_src) + 19; cui_code_editor_select(ui, "editor", pos, pos); }
        }
        return;
    }
    frame_wrap(ui, body_code_editor);
}
static void body_code_editor(cui_ctx *ui) {
    cui_column(ui, .width = CUI_FILL, .gap = 10) {
        cui_code_editor_result r = cui_code_editor(ui, "editor", ed_src, sizeof ed_src, .lang = "c", .height = 356,
                                                   .placeholder = "Write some C");
        cui_row(ui, .width = CUI_FILL, .gap = 16, .align = CUI_ALIGN_CENTER, .wrap = true) {
            cui_textf(ui, "Ln %d, Col %d", r.line, r.column);
            cui_textf(ui, "%d lines", r.line_count);
            cui_text(ui, "Spaces: 4", .size = 12.5f, .color = CUI_COLOR_TEXT_MUTED);
            if (r.refused) cui_text(ui, "Buffer full", .size = 12.5f, .color = CUI_COLOR_DANGER);
        }
    }
}
/* ---- tool panels ---- */
SCENE(inspector) {
    static float pos[3] = { 12.5f, 0.0f, -4.25f }, rot[3] = { 0, 45, 0 }, scale[3] = { 1, 1, 1 };
    static float roughness = 0.42f, metallic = 0.0f, emission = 1.6f;
    static cui_color base = { 0xC2, 0x6A, 0x3D, 255 };
    static int shader = 0, layer = 3;
    static bool cast = true, double_sided = false;
    static bool t_open = true, m_open = true, s_open = false;
    static char name[32] = "crate_large_02";
    static const char *const shaders[] = { "Standard PBR", "Unlit", "Toon", "Glass" };
    cui_card(ui, .width = CUI_FILL, .max_width = 460, .pad_x = 16, .pad_y = 6, .gap = 0) {
        if (cui_inspector_begin(ui, "props")) {
            if (cui_inspector_group(ui, "Transform", &t_open)) {
                cui_prop_vec3(ui, "Position", pos, .step = 0.05f, .format = "%.2f");
                cui_prop_vec3(ui, "Rotation", rot, .step = 1, .format = "%.0f");
                cui_prop_vec3(ui, "Scale", scale, .step = 0.01f, .format = "%.2f");
            }
            if (cui_inspector_group(ui, "Material", &m_open)) {
                cui_prop_text(ui, "Name", name, sizeof name);
                cui_prop_enum(ui, "Shader", shaders, 4, &shader);
                cui_prop_color(ui, "Base color", &base);
                cui_prop_float(ui, "Roughness", &roughness, .min = 0, .max = 1, .step = 0.01f);
                cui_prop_float(ui, "Metallic", &metallic, .min = 0, .max = 1, .step = 0.01f);
                cui_prop_float(ui, "Emission", &emission, .step = 0.1f, .suffix = "nits");
                cui_prop_bool(ui, "Double sided", &double_sided);
            }
            if (cui_inspector_group(ui, "Shadows", &s_open)) {
                cui_prop_bool(ui, "Cast shadows", &cast);
                cui_prop_int(ui, "Layer", &layer, .min = 0, .max = 31);
            }
            cui_inspector_end(ui);
        }
    }
}
static cui_log_line g_log_lines[512];
static char g_log_text[32 * 1024];
static cui_log_buffer g_log;
static void fill_build_log(void) {
    static const struct { cui_log_level lv; const char *text; } lines[] = {
        { CUI_LOG_INFO, "Loading project harbor_town (412 assets)" },
        { CUI_LOG_DEBUG, "Asset cache hit rate 91.4% (376 of 412)" },
        { CUI_LOG_TRACE, "vfs: mounted /content/packs/base.pak (218 MB)" },
        { CUI_LOG_INFO, "Compiling shaders: 64 variants on 8 threads" },
        { CUI_LOG_DEBUG, "terrain_blend.frag: 3 permutations, 1.8 ms each" },
        { CUI_LOG_WARN, "water_surface.frag:118: implicit float to half conversion" },
        { CUI_LOG_INFO, "Shaders compiled in 2.41 s" },
        { CUI_LOG_TRACE, "gpu: uploaded 37 textures, 148.2 MB resident" },
        { CUI_LOG_INFO, "Baking navmesh for 3 regions" },
        { CUI_LOG_DEBUG, "navmesh: harbor 18,204 polys, market 9,377 polys" },
        { CUI_LOG_WARN, "Texture props/barrel_d.png is 4096x4096; streaming budget is 2048" },
        { CUI_LOG_INFO, "Lightmaps: 12 of 12 atlases up to date" },
        { CUI_LOG_ERROR, "Missing material 'dock_planks_wet' on mesh pier_03.obj (using fallback)" },
        { CUI_LOG_DEBUG, "audio: 96 voices, 48 kHz, latency 10.7 ms" },
        { CUI_LOG_INFO, "Entering play mode" },
        { CUI_LOG_TRACE, "frame 1: 14.2 ms (update 3.1, render 9.8, present 1.3)" },
        { CUI_LOG_INFO, "Spawned 214 entities in harbor" },
        { CUI_LOG_WARN, "Physics step took 21.4 ms (budget 16.6 ms) after the crane collision" },
        { CUI_LOG_DEBUG, "Streaming in market district, 31 cells" },
        { CUI_LOG_INFO, "Autosave written to saves/slot_2.sav (1.2 MB)" },
    };
    cui_log_init(&g_log, g_log_lines, 512, g_log_text, sizeof g_log_text);
    double t = 14 * 3600 + 2 * 60 + 7.120;
    for (int k = 0; k < (int)(sizeof lines / sizeof lines[0]); k++) {
        g_log.now = t;
        cui_log_append(&g_log, lines[k].lv, "%s", lines[k].text);
        t += 0.083 + (k % 5) * 0.217 + (k % 3) * 0.061;
    }
}
SCENE(log_console) {
    static bool filled = false;
    if (!filled) { fill_build_log(); filled = true; }
    cui_log_view(ui, "log", &g_log, .height = 352, .timestamps = true);
}
/* the first 2 KB of a PNG: signature, IHDR, tEXt, pHYs, an IDAT of
 * compressed looking bytes and IEND, with real chunk CRCs */
static uint8_t g_png[2048];
static size_t g_png_n;
static uint32_t png_crc(const uint8_t *p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return c ^ 0xFFFFFFFFu;
}
static void png_u32(uint32_t v) { for (int k = 3; k >= 0; k--) g_png[g_png_n++] = (uint8_t)(v >> (8 * k)); }
static void png_chunk(const char *type, const uint8_t *data, uint32_t len) {
    png_u32(len);
    size_t at = g_png_n;
    memcpy(g_png + g_png_n, type, 4); g_png_n += 4;
    if (len) { memcpy(g_png + g_png_n, data, len); g_png_n += len; }
    png_u32(png_crc(g_png + at, len + 4));
}
static void fill_png(void) {
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    static const uint8_t ihdr[13] = { 0, 0, 0x05, 0x00, 0, 0, 0x02, 0xD0, 8, 6, 0, 0, 0 };   /* 1280 x 720, 8 bit RGBA */
    static const uint8_t text[] = "Software\0CUI 1.0";
    static const uint8_t phys[9] = { 0, 0, 0x0B, 0x13, 0, 0, 0x0B, 0x13, 1 };              /* 2835 px/m: 72 dpi */
    static uint8_t idat[2048];
    memcpy(g_png, sig, 8); g_png_n = 8;
    png_chunk("IHDR", ihdr, 13);
    png_chunk("tEXt", text, (uint32_t)sizeof text - 1);
    png_chunk("pHYs", phys, 9);
    uint32_t n = (uint32_t)(sizeof g_png - g_png_n - 12 - 12);
    uint32_t x = 0x9E3779B9u;
    idat[0] = 0x78; idat[1] = 0x9C;   /* zlib header */
    for (uint32_t i = 2; i < n; i++) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; idat[i] = (uint8_t)(x >> 7); }
    png_chunk("IDAT", idat, n);
    png_chunk("IEND", NULL, 0);
}
static void scene_hex_viewer(cui_ctx *ui, int f) {
    static cui_hex_selection sel = { 0x10, 8, 0x17 };   /* IHDR width and height */
    static char go[20];
    static char edited[48];
    static const cui_hex_range marks[] = {
        { 0, 8, CUI_TONE_INFO, "PNG signature" },
        { 8, 25, CUI_TONE_VIOLET, "IHDR chunk, 1280 by 720 RGBA" },
        { 33, 28, CUI_TONE_TEAL, "tEXt chunk, Software" },
    };
    if (!g_png_n) fill_png();
    if (f < 0) {
        /* the pointer rests on a byte of the tEXt chunk, so its label shows */
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "hex"));
        if (-f - 1 == 3 && r.w > 0) cui_input_mouse_move(ui, r.x + 265, r.y + 96);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 12, .justify = CUI_JUSTIFY_CENTER) {
        /* editable: the live page writes typed hex digits into this copy */
        cui_hex_result r = cui_hex_view(ui, "hex", g_png, g_png_n, &sel, .ranges = marks, .range_count = 3, .edit = g_png, .height = 300);
        if (r.edited) snprintf(edited, sizeof edited, "Edited %u byte%s at %04X", (unsigned)r.edit_length, r.edit_length == 1 ? "" : "s", (unsigned)r.edit_offset);
        cui_row(ui, .width = CUI_FILL, .gap = 12, .align = CUI_ALIGN_CENTER, .wrap = true) {
            if (cui_input(ui, "goto", go, sizeof go, .placeholder = "Go to offset", .size = CUI_SIZE_SM, .width = CUI_PX(150)) & CUI_SUBMITTED)
                cui_hex_view_goto(ui, "hex", (size_t)strtoul(go, NULL, 16));
            char status[64];
            if (sel.length) snprintf(status, sizeof status, "%04X to %04X, %u byte%s selected", (unsigned)sel.start,
                                     (unsigned)(sel.start + sel.length - 1), (unsigned)sel.length, sel.length == 1 ? "" : "s");
            else snprintf(status, sizeof status, "Cursor at %04X", (unsigned)sel.cursor);
            cui_text(ui, status, .size = 12.5f, .tabular = true, .color = CUI_COLOR_TEXT_SECONDARY);
            if (edited[0]) cui_text(ui, edited, .size = 12.5f, .tabular = true, .color = CUI_COLOR_TEXT_MUTED);
        }
    }
}
/* curve and gradient editors: the arrays and selections are static, so the
 * live page keeps what you drag, add and remove (the editors edit in place) */
static cui_curve_key cv_keys[16] = {
    { 0.00f, 0.00f, 0, 0, 0, 0, CUI_CURVE_SMOOTH },
    { 0.55f, 0.66f, -0.22f, -0.42f, 0.24f, 0.46f, CUI_CURVE_FREE },
    { 1.20f, 1.08f, 0, 0, 0, 0, CUI_CURVE_SMOOTH },
    { 2.00f, 1.00f, 0, 0, 0, 0, CUI_CURVE_SMOOTH },
};
static int cv_n = 4, cv_sel = 1;
static void body_curve_editor(cui_ctx *ui);
static void scene_curve_editor(cui_ctx *ui, int f) {
    if (f < 0) {   /* hover the curve late in the overshoot, away from the keys */
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "ease"));
        if (-f - 1 == 4 && r.w > 0) cui_input_mouse_move(ui, r.x + r.w * 0.8f, r.y + 120);
        return;
    }
    frame_wrap(ui, body_curve_editor);
}
static void body_curve_editor(cui_ctx *ui) {
    cui_box(ui, UPTO(620)) {
        cui_curve_editor(ui, "ease", cv_keys, &cv_n, 16, .x_max = 2, .y_max = 1.2f, .x_label = "Time", .y_label = "Value", .selected = &cv_sel);
    }
}
static cui_gradient_stop gr_stops[12] = {
    { 0.00f, { 0xFF, 0xF1, 0xC1, 0xFF } }, { 0.24f, { 0xFF, 0xB1, 0x4A, 0xFF } },
    { 0.58f, { 0xE2, 0x55, 0x2B, 0xFF } }, { 1.00f, { 0x5C, 0x1F, 0x16, 0x00 } },
};
static int gr_n = 4, gr_sel = 2;
SCENE(gradient_editor) {
    cui_column(ui, UPTO(560), .gap = 10) {
        cui_text(ui, "Flame color over lifetime", .font = CUI_FONT_MEDIUM, .size = 13);
        cui_gradient_editor(ui, "flame", gr_stops, &gr_n, 12, .selected = &gr_sel);
    }
}
/* timeline editor: a shot with a hero, a camera move and a sound track. The
 * tracks, keys, selection and playhead are static and the scene applies what
 * the editor reports, so the live page scrubs, plays, selects, drags, adds and
 * deletes keys; the current track's curve is edited below with the curve
 * editor, on the same key array */
#define TE_CAP 32
enum { TE_HERO, TE_PX, TE_PY, TE_ROT, TE_SCALE, TE_OPACITY, TE_CAMERA, TE_ZOOM, TE_PAN, TE_AUDIO, TE_N };
static cui_curve_key te_keys[TE_N][TE_CAP] = {
    [TE_PX] = { { 0, 40 }, { 1.25f, 250 }, { 2.25f, 380 }, { 3.5f, 520 }, { 5, 600 } },
    [TE_PY] = { { 0, 0 }, { 1.25f, 0 }, { 1.75f, 72 }, { 2.25f, 0 }, { 4, 0, 0, 0, 0, 0, CUI_CURVE_STEP } },
    [TE_ROT] = { { 0, 0 }, { 1.25f, -8 }, { 1.75f, 12 }, { 2.25f, 0 }, { 3.5f, 4 } },
    [TE_SCALE] = { { 0, 1 }, { 2.25f, 1, 0, 0, 0, 0, CUI_CURVE_LINEAR }, { 2.375f, 0.86f, 0, 0, 0, 0, CUI_CURVE_LINEAR }, { 2.625f, 1 } },
    [TE_OPACITY] = { { 0, 0, 0, 0, 0, 0, CUI_CURVE_LINEAR }, { 0.5f, 1, 0, 0, 0, 0, CUI_CURVE_LINEAR },
                     { 4.5f, 1, 0, 0, 0, 0, CUI_CURVE_LINEAR }, { 5, 0, 0, 0, 0, 0, CUI_CURVE_LINEAR } },
    [TE_ZOOM] = { { 0, 1 }, { 3, 1 }, { 4.25f, 1.6f }, { 5, 1.6f } },
    [TE_PAN] = { { 0, -80, 0, 0, 0, 0, CUI_CURVE_LINEAR }, { 5, 120, 0, 0, 0, 0, CUI_CURVE_LINEAR } },
};
static bool te_sel[TE_N][TE_CAP] = { [TE_ROT] = { [2] = true } };
static cui_timeline_clip te_clips[4] = { { 0.375f, 2.25f, 0, "Footsteps", false }, { 3.0f, 4.625f, 0.75f, "Street ambience", false } };
#define TE_TRACK(name, depth, i, n) { name, NULL, depth, .keys = te_keys[i], .key_count = n, .key_capacity = TE_CAP, .key_selected = te_sel[i] }
static cui_timeline_track te_tracks[TE_N] = {
    [TE_HERO] = { "Hero", CUI_ICON_USER, 0 },
    [TE_PX] = TE_TRACK("Position X", 1, TE_PX, 5), [TE_PY] = TE_TRACK("Position Y", 1, TE_PY, 5),
    [TE_ROT] = TE_TRACK("Rotation", 1, TE_ROT, 5), [TE_SCALE] = TE_TRACK("Scale", 1, TE_SCALE, 4),
    [TE_OPACITY] = TE_TRACK("Opacity", 1, TE_OPACITY, 4),
    [TE_CAMERA] = { "Camera", CUI_ICON_VIDEO, 0, .color = 4 },
    [TE_ZOOM] = TE_TRACK("Zoom", 1, TE_ZOOM, 4), [TE_PAN] = TE_TRACK("Pan", 1, TE_PAN, 2),
    [TE_AUDIO] = { "Sound", CUI_ICON_AUDIO_LINES, 0, .color = 3, .clips = te_clips, .clip_count = 2 },
};
static const struct { float lo, hi; const char *unit; } te_range[TE_N] = {
    [TE_PX] = { 0, 640, "Pixels" }, [TE_PY] = { -10, 100, "Pixels" }, [TE_ROT] = { -20, 20, "Degrees" }, [TE_SCALE] = { 0.6f, 1.2f, "Scale" },
    [TE_OPACITY] = { 0, 1, "Opacity" }, [TE_ZOOM] = { 0.8f, 2, "Zoom" }, [TE_PAN] = { -150, 150, "Pixels" },
};
static float te_time = 1.5f, te_in = 0.5f, te_out = 4.5f;
static int te_cur_track = TE_ROT, te_cur_key = 2;
static bool te_playing;
static double te_last;
static void body_timeline_editor(cui_ctx *ui);
static void scene_timeline_editor(cui_ctx *ui, int f) {
    if (f < 0) {   /* hover the jump's peak on Position Y: its tooltip shows the time and value */
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "shot"));
        if (-f - 1 == 4 && r.w >= 552) cui_input_mouse_move(ui, r.x + 194 + 1.75f / 5 * (r.w - 204), r.y + 29 + TE_PY * 26 + 13);
        return;
    }
    frame_wrap(ui, body_timeline_editor);
}
static void body_timeline_editor(cui_ctx *ui) {
    /* playing is ours: the editor asks (Space or the button), we advance the time */
    double now = cui_time(ui);
    if (te_playing) {
        te_time += (float)(now - te_last);
        if (te_time >= te_out || te_time < te_in) te_time = te_in;
        cui_request_redraw(ui);
    }
    te_last = now;
    cui_column(ui, .width = CUI_FILL, .gap = 12) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
            if (cui_icon_button(ui, te_playing ? CUI_ICON_PAUSE : CUI_ICON_PLAY, .tooltip = te_playing ? "Pause" : "Play")) te_playing = !te_playing;
            cui_text(ui, "Shot 4", .font = CUI_FONT_MEDIUM, .size = 13);
            cui_text(ui, "5 seconds at 24 fps", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_timeline_editor_result r = cui_timeline_editor(ui, "shot", te_tracks, TE_N, &te_time, .end = 5, .fps = 24, .in = &te_in, .out = &te_out,
                                                           .current_track = &te_cur_track, .current_key = &te_cur_key, .toggles = true);
        if (r.play_toggled) te_playing = !te_playing;
        /* the current track's curve: the same keys, so edits show in both */
        cui_timeline_track *tr = te_cur_track >= 0 ? &te_tracks[te_cur_track] : NULL;
        if (tr && tr->keys) {
            cui_text(ui, tr->name, .font = CUI_FONT_MEDIUM, .size = 13);
            int before = te_cur_key;
            cui_curve_result cr = cui_curve_editor(ui, "curve", tr->keys, &tr->key_count, TE_CAP, .x_max = 5, .y_min = te_range[te_cur_track].lo,
                                                   .y_max = te_range[te_cur_track].hi, .x_label = "Time", .y_label = te_range[te_cur_track].unit,
                                                   .selected = &te_cur_key, .show_playhead = true, .playhead = te_time, .height = 170);
            bool *sel = tr->key_selected;   /* the timeline's selection follows the curve editor's */
            if (cr.added >= 0) { memmove(&sel[cr.added + 1], &sel[cr.added], (size_t)(tr->key_count - 1 - cr.added)); sel[cr.added] = false; }
            if (cr.removed >= 0) { memmove(&sel[cr.removed], &sel[cr.removed + 1], (size_t)(tr->key_count - cr.removed)); sel[tr->key_count] = false; }
            if (te_cur_key != before || cr.added >= 0 || cr.removed >= 0) {
                memset(te_sel, 0, sizeof te_sel);
                if (te_cur_key >= 0) sel[te_cur_key] = true;
            }
        } else {
            cui_text(ui, "Select a key or a track name to edit its curve here.", .size = 13, .color = CUI_COLOR_TEXT_MUTED);
        }
    }
}
SCENE(json_view) {
    cui_json_view(ui, "json",
        "{\"name\": \"cui\", \"version\": \"1.1.0\", \"private\": false, \"downloads\": 128400,\n"
        " \"platforms\": [\"windows\", \"macos\", \"linux\"],\n"
        " \"renderer\": {\"backend\": \"opengl\", \"fallback\": \"software\", \"vsync\": true, \"msaa\": null},\n"
        " \"authors\": [{\"name\": \"Maya\", \"commits\": 412}, {\"name\": \"Jonas\", \"commits\": 388}]}",
        .expand_depth = 1);
}
/* markdown view: the text is static, so ticking a task box on the live page
 * edits it, and links report where they go instead of opening */
static char md_notes[2048] =
    "## Tern 2.4 release notes\n"
    "\n"
    "The ~~beta~~ sync engine is now the default. It batches writes, so **a 2,000 item import takes 3.1 s** instead of 14 s. "
    "Run `tern migrate` once after updating; the [upgrade guide](https://tern.dev/docs/upgrade) lists every *breaking* change.\n"
    "\n"
    "### What changed\n"
    "\n"
    "- The offline banner waits 2 s before it appears\n"
    "  - It no longer flickers on a slow network\n"
    "  - Retry keeps your scroll position\n"
    "- Attachments upload in the background\n"
    "\n"
    "### Before you ship\n"
    "\n"
    "- [x] Update the changelog\n"
    "- [ ] Rerun the sync benchmark on Android\n"
    "- [ ] Tag the release\n"
    "\n"
    "> Builds made before March 4 still read the old cache format, but they write the new one.\n"
    "\n"
    "```c\n"
    "int sync_flush(sync_queue *q) {\n"
    "    if (q->count == 0) return 0;   /* nothing to send */\n"
    "    return send_batch(q->items, q->count);\n"
    "}\n"
    "```\n"
    "\n"
    "| Platform | Cold start | vs 2.3 |\n"
    "|:---------|-----------:|:------:|\n"
    "| macOS | 412 ms | -18% |\n"
    "| Windows | 498 ms | -11% |\n"
    "| Android | 731 ms | -9% |\n";
static void body_markdown(cui_ctx *ui) {
    static char status[160];
    static double stream_start = -1;   /* Replay: the notes arrive as a reply would */
    int len = (int)strlen(md_notes), shown = len;
    if (stream_start >= 0) {
        shown = (int)((cui_time(ui) - stream_start) * 420.0);
        if (shown >= len) { shown = len; stream_start = -1; }
        else cui_request_redraw(ui);
    }
    cui_column(ui, .gap = 16, UPTO(640)) {
        cui_markdown_result r = cui_markdown(ui, "notes", md_notes, .length = shown, .streaming = shown < len);
        if (r.link) snprintf(status, sizeof status, "Opened: %s", r.link);
        if (cui_markdown_apply_task(md_notes, &r))
            snprintf(status, sizeof status, "%s the task on line %d", r.task_checked ? "Checked" : "Unchecked", r.task_line);
        cui_separator(ui);
        cui_row(ui, .width = CUI_FILL, .gap = 12, .align = CUI_ALIGN_CENTER, .wrap = true) {
            if (cui_button(ui, "Replay as a stream", .size = CUI_SIZE_SM, .icon = CUI_ICON_PLAY)) { stream_start = cui_time(ui); status[0] = 0; }
            cui_text(ui, status[0] ? status : r.hovered_link ? r.hovered_link : "Click a link or a task box",
                     .size = 12.5f, .color = CUI_COLOR_TEXT_SECONDARY);
        }
    }
}
static void scene_markdown(cui_ctx *ui, int f) {
    if (f < 0) return;
    frame_wrap(ui, body_markdown);
}
/* diff viewer: two versions of a config loader. The mode toggle and the
 * selected change are static and wired to the view, so the live page keeps
 * them; folds open on a click and stay open. */
static const char *const df_old =
    "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n#include \"config.h\"\n\n#define LINE_MAX 256\n\n"
    "static void trim(char *s) {\n    size_t n = strlen(s);\n    while (n > 0 && (s[n - 1] == '\\n' || s[n - 1] == ' ')) s[--n] = '\\0';\n}\n\n"
    "static int parse_line(const char *line, char *key, char *value) {\n    const char *eq = strchr(line, '=');\n    if (!eq) return 0;\n"
    "    size_t n = (size_t)(eq - line);\n    memcpy(key, line, n);\n    key[n] = '\\0';\n    strcpy(value, eq + 1);\n    trim(value);\n    return 1;\n}\n\n"
    "int config_load(config *cfg, const char *path) {\n    FILE *f = fopen(path, \"r\");\n    char buf[LINE_MAX];\n    int count = 0;\n"
    "    while (fgets(buf, sizeof buf, f)) {\n        char key[LINE_MAX], value[LINE_MAX];\n        if (buf[0] == '#') continue;\n"
    "        if (!parse_line(buf, key, value)) continue;\n        /* keys from version 1 used dashes */\n        for (char *p = key; *p; p++)\n"
    "            if (*p == '-') *p = '_';\n        config_set(cfg, key, value);\n        count++;\n    }\n    fclose(f);\n    return count;\n}\n\n"
    "void config_free(config *cfg) {\n    for (int i = 0; i < cfg->count; i++) {\n        free(cfg->keys[i]);\n        free(cfg->values[i]);\n    }\n"
    "    free(cfg->keys);\n    free(cfg->values);\n    cfg->count = 0;\n}\n";
static const char *const df_new =
    "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n#include \"config.h\"\n#include \"log.h\"\n\n#define LINE_MAX 256\n\n"
    "static void trim(char *s) {\n    size_t n = strlen(s);\n    while (n > 0 && (s[n - 1] == '\\n' || s[n - 1] == ' ')) s[--n] = '\\0';\n}\n\n"
    "static int parse_line(const char *line, char *key, char *value) {\n    const char *eq = strchr(line, '=');\n    if (!eq || eq == line) return 0;\n"
    "    size_t n = (size_t)(eq - line);\n    memcpy(key, line, n);\n    key[n] = '\\0';\n    snprintf(value, LINE_MAX, \"%s\", eq + 1);\n    trim(value);\n    return 1;\n}\n\n"
    "int config_load(config *cfg, const char *path) {\n    FILE *f = fopen(path, \"r\");\n    if (!f) {\n        log_error(\"cannot open %s\", path);\n        return -1;\n    }\n"
    "    char buf[LINE_MAX];\n    int loaded = 0;\n"
    "    while (fgets(buf, sizeof buf, f)) {\n        char key[LINE_MAX], value[LINE_MAX];\n        if (buf[0] == '#') continue;\n"
    "        if (!parse_line(buf, key, value)) continue;\n        config_set(cfg, key, value);\n        loaded++;\n    }\n    fclose(f);\n    return loaded;\n}\n\n"
    "void config_free(config *cfg) {\n    for (int i = 0; i < cfg->count; i++) {\n        free(cfg->keys[i]);\n        free(cfg->values[i]);\n    }\n"
    "    free(cfg->keys);\n    free(cfg->values);\n    cfg->count = 0;\n}\n";
static int df_mode = 0, df_change = 6;
static cui_diff_result df_last;   /* last frame's result, for the toolbar above the view */
SCENE(diff_view) {
    static const char *const modes[] = { "Split", "Inline" };
    cui_row(ui, .width = CUI_FILL, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_segmented(ui, "mode", modes, 2, &df_mode, .size = CUI_SIZE_SM);
        if (df_last.mode != (df_mode ? CUI_DIFF_INLINE : CUI_DIFF_SPLIT)) cui_text(ui, "Inline on narrow screens", .size = 12.5f, .color = CUI_COLOR_TEXT_MUTED);
        cui_box(ui, .width = CUI_GROW(1)) {}
        cui_row(ui, .gap = 4, .align = CUI_ALIGN_CENTER) {
            char at[48];
            if (df_last.current >= 0) snprintf(at, sizeof at, "Change %d of %d", df_last.current + 1, df_last.changes);
            else snprintf(at, sizeof at, "%d changes", df_last.changes);
            cui_text(ui, at, .size = 12.5f, .tabular = true, .color = CUI_COLOR_TEXT_SECONDARY);
            cui_gap(ui, 4);
            if (cui_icon_button(ui, CUI_ICON_ARROW_UP, .size = CUI_SIZE_SM, .tooltip = "Previous change", .shortcut = "P", .disabled = df_change <= 0))
                df_change--;
            if (cui_icon_button(ui, CUI_ICON_ARROW_DOWN, .size = CUI_SIZE_SM, .tooltip = "Next change", .shortcut = "N", .disabled = df_change >= df_last.changes - 1))
                df_change++;
        }
    }
    df_last = cui_diff_view(ui, "diff", df_old, df_new, .mode = df_mode ? CUI_DIFF_INLINE : CUI_DIFF_SPLIT, .current = &df_change,
                            .language = "c", .old_title = "config.c (main)", .new_title = "config.c", .height = 372);
}
SCENE(chat) {
    cui_card(ui, .gap = 0, .pad = 16, .width = CUI_FILL) {
        cui_chat_divider(ui, "Today");
        cui_chat_message(ui, "Did the data grid make it into 1.1?", .author = "Jonas Berg", .time = "9:41", .last_in_group = false);
        cui_chat_message(ui, "I need it for the billing page.", .author = "Jonas Berg", .grouped = true, .last_in_group = true);
        cui_chat_message(ui, "It did. 2,000 rows scroll at 60 fps on my laptop.", .mine = true);
        cui_chat_message(ui, "Shift and Ctrl clicks select like a spreadsheet.", .mine = true, .grouped = true, .last_in_group = true, .status = "Read", .time = "9:43");
        cui_typing_indicator(ui, "Jonas");
    }
}
SCENE(comments) {
    static int likes[3] = { 12, 3, 0 };
    static bool liked[3] = { true, false, false };
    cui_card(ui, .gap = 4, .pad = 14, .width = CUI_FILL) {
        cui_comment(ui, "c1", "Tomas Rivera", "The new numbers match the staging run. I would still test the import on a large file before Friday.", .time = "2h", .likes = &likes[0], .liked = &liked[0], .badge = "Author");
        cui_comment(ui, "c2", "Priya Nair", "Agreed. I have a 2 GB export we can use.", .time = "1h", .depth = 1, .likes = &likes[1], .liked = &liked[1]);
        cui_comment(ui, "c3", "Maya Okafor", "Scheduled it for 3 pm.", .time = "40m", .depth = 1, .likes = &likes[2], .liked = &liked[2], .edited = true);
    }
}
SCENE(presence_ticker) {
    static const cui_presence_user who[] = {
        { "Maya Okafor", { 0x7c, 0x5c, 0xff, 255 }, 0, false }, { "Jonas Berg", { 0x22, 0xc5, 0x8b, 255 }, 0, false },
        { "Priya Nair", { 0xf5, 0x9e, 0x0b, 255 }, 0, false }, { "Lena Fischer", { 0x38, 0xbd, 0xf8, 255 }, 0, true },
        { "Aiko Tanaka", { 0xec, 0x48, 0x99, 255 }, 0, false }, { "Daniel Mensah", { 0x10, 0xb9, 0x81, 255 }, 0, false },
        { "Rosa Delgado", { 0xa8, 0x55, 0xf7, 255 }, 0, false },
    };
    static const cui_ticker_item tick[] = {
        { "Revenue", "$48.2k", 6.9f }, { "Active users", "12,408", 3.1f }, { "Churn", "1.7%", -0.6f, true }, { "Latency", "182 ms", -8.4f, true },
        { "Builds", "1,204", 12.0f }, { "Uptime", "99.98%", 0 },
    };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_presence(ui, "here", who, 7, 5, 30);
        cui_counter(ui, "rev", 48210, .format = "$%.0f", .size = 34, .group_thousands = true);
    }
    cui_ticker(ui, "tick", tick, 6);
}
SCENE(carousel) {
    static int slide = 1;
    static const char *titles[] = { "Faster exports", "ARM64 runners", "Shortcuts everywhere" };
    static const char *descs[] = { "A 900 file project now exports in 40 seconds, down from four minutes.",
                                   "Linux and macOS jobs can run on ARM64 machines in every plan.",
                                   "Every action in the editor has a key, listed under Help, Shortcuts." };
    static const unsigned tops[] = { 0x1f2a37, 0x0f3d3e, 0x3a2e22 };
    cui_carousel(ui, "tour", 3, &slide, .height = 200) {
        for (int k = 0; k < 3; k++) cui_carousel_slide(ui) {
            cui_box(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_RGB(tops[k]), .radius = 16, .pad = 28, .gap = 8, .justify = CUI_JUSTIFY_END) {
                cui_text(ui, titles[k], .font = CUI_FONT_DISPLAY, .size = 24, .color = CUI_RGB(0xffffff));
                cui_text(ui, descs[k], .color = CUI_RGBA(0xffffffc0));
            }
        }
    }
}
static void shot_scene(unsigned char *px, int w, int h, bool color) {
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
            if (!color) { float l = r * 0.3f + g * 0.59f + b * 0.11f; r = g = b = l * 0.85f + 0.05f; }
            unsigned char *p = px + (y * w + x) * 4;
            p[0] = (unsigned char)(fminf(1, fmaxf(0, r)) * 255); p[1] = (unsigned char)(fminf(1, fmaxf(0, g)) * 255);
            p[2] = (unsigned char)(fminf(1, fmaxf(0, b)) * 255); p[3] = 255;
        }
}
SCENE(image_viewer) {
    static cui_texture photo;
    static cui_image_view view = { 1.6f, 1180, 330 };
    static cui_rect crop = { 300, 120, 620, 620 };
    bool *made = cui_state(ui, cui_make_id(ui, "iv-texture"), sizeof *made);
    if (!*made) {
        *made = true;
        static unsigned char px[1600 * 1000 * 4];
        shot_scene(px, 1600, 1000, true);
        photo = cui_image_create(ui, 1600, 1000, px);
    }
    cui_row(ui, .gap = 20, .align = CUI_ALIGN_START, .width = CUI_FILL, .wrap = true) {
        cui_column(ui, .gap = 8, .width = CUI_GROW(3), .min_width = 260) {
            cui_label(ui, "Viewer");
            cui_image_viewer(ui, "view", photo, 1600, 1000, &view, .height = CUI_PX(300));
        }
        cui_column(ui, .gap = 8, .width = CUI_GROW(2), .min_width = 240) {
            cui_label(ui, "Cropper");
            cui_cropper(ui, "crop", photo, 1600, 1000, &crop, .round = true, .height = CUI_PX(300));
        }
    }
}
SCENE(compare_stack) {
    static cui_texture before, after;
    static float split = 0.42f;
    bool *made = cui_state(ui, cui_make_id(ui, "cmp-textures"), sizeof *made);
    if (!*made) {
        *made = true;
        static unsigned char a[960 * 600 * 4], b[960 * 600 * 4];
        shot_scene(a, 960, 600, false);
        shot_scene(b, 960, 600, true);
        before = cui_image_create(ui, 960, 600, a);
        after = cui_image_create(ui, 960, 600, b);
    }
    cui_row(ui, .gap = 40, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, .width = CUI_GROW(1), .min_width = 280) { cui_image_compare(ui, "cmp", before, after, &split, .height = 230, .aspect = 480.0f / 300.0f); }
        static const char *names[] = { "Kyoto", "Lisbon", "Reykjavik" };
        static const unsigned cols[] = { 0xc8553d, 0x2d3047, 0x5b8e7d };
        cui_card_stack_begin(ui, "deck", 3, &(cui_card_stack_opts){ .width = CUI_PX(240), .height = 200 });
        for (int k = 0; k < 3; k++) cui_card_stack_item(ui, k) {
            cui_box(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_RGB(cols[k]), .radius = 25, .pad = 20, .justify = CUI_JUSTIFY_END, .gap = 4) {
                cui_text(ui, names[k], .font = CUI_FONT_DISPLAY, .size = 22, .color = CUI_RGB(0xffffff));
                cui_text(ui, "Swipe right to save", .size = 13, .font = CUI_FONT_MEDIUM, .color = CUI_RGBA(0xfffffff0));
            }
        }
        cui_card_stack_end(ui);
    }
}
static cui_id kb_ids[3][3];
static void body_kanban(cui_ctx *ui);
static void scene_kanban(cui_ctx *ui, int f) {
    if (f < 0) {   /* lift the first backlog card and carry it into "In progress" */
        int t = -f - 1;
        cui_rect a = cui_get_rect(ui, kb_ids[0][0]), b = cui_get_rect(ui, kb_ids[1][1]);
        float x0 = a.x + 90, y0 = a.y + 30, x1 = b.x + 110, y1 = b.y - 10;
        if (t == 20) cui_input_mouse_move(ui, x0, y0);
        if (t == 21) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t >= 22 && t <= 40) { float k = (t - 22) / 18.0f; cui_input_mouse_move(ui, x0 + (x1 - x0) * k, y0 + (y1 - y0) * k); }
        return;
    }
    frame_wrap(ui, body_kanban);
}
static void body_kanban(cui_ctx *ui) {
    static const char *col_names[3] = { "Backlog", "In progress", "Done" };
    static const unsigned dots[3] = { 0x94a3b8, 0x3b82f6, 0x22c55e };
    static const char *titles[3][3] = { { "Audit the color tokens for contrast", "Virtualize the activity feed", "Dark mode screenshots" },
                                        { "Keyboard nav in the date range picker", "IME composition on Windows", NULL },
                                        { "Spring presets", NULL, NULL } };
    static const char *tags[3][3] = { { "Design", "Perf", "Docs" }, { "A11y", "Platform", NULL }, { "Motion", NULL, NULL } };
    static const cui_tone tones[3][3] = { { CUI_TONE_VIOLET, CUI_TONE_TEAL, CUI_TONE_ROSE }, { CUI_TONE_INFO, CUI_TONE_WARNING }, { CUI_TONE_VIOLET } };
    static const char *who[3][3] = { { "Maya Okafor", "Jonas Berg", "Aiko Tanaka" }, { "Priya Nair", "Daniel Mensah" }, { "Rosa Delgado" } };
    static const int lens[3] = { 3, 2, 1 };
    cui_kanban_begin(ui, "board");
    for (int c = 0; c < 3; c++) {
        cui_kanban_column_begin(ui, col_names[c], lens[c], .dot = CUI_RGB(dots[c]), .limit = c == 1 ? 3 : 0, .width = 270);
        for (int k = 0; k < lens[c]; k++) {
            cui_kanban_card_begin(ui, c * 10 + k);
            kb_ids[c][k] = cui_last_id(ui);
            cui_badge(ui, tags[c][k], .tone = tones[c][k], .size = CUI_SIZE_SM);
            cui_text(ui, titles[c][k], .font = CUI_FONT_MEDIUM, .size = 13.5f, .line_height = 1.35f);
            if (c == 1) cui_progress(ui, k ? 0.3f : 0.6f, .height = 4);
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_avatar(ui, who[c][k], .size = 22);
                cui_spacer(ui);
                cui_icon(ui, CUI_ICON_MESSAGE_SQUARE, 12, CUI_COLOR_TEXT_MUTED);
                cui_text(ui, k == 1 ? "2" : "3", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
            }
            cui_kanban_card_end(ui);
        }
        cui_kanban_column_end(ui);
    }
    cui_kanban_end(ui, NULL);
}
SCENE(week_calendar) {
    static cui_cal_event events[] = {
        { "Standup", "Zoom", 0, 9.5f, 9.75f, CUI_TONE_INFO }, { "Design review", "Room 2", 0, 11, 12.5f, CUI_TONE_VIOLET },
        { "Lunch with Maya", NULL, 1, 12.5f, 13.5f, CUI_TONE_SUCCESS }, { "Standup", "Zoom", 1, 9.5f, 9.75f, CUI_TONE_INFO },
        { "Release planning", "Room 1", 2, 10, 11.5f, CUI_TONE_WARNING }, { "1:1 Jonas", NULL, 2, 10.5f, 11.5f, CUI_TONE_ROSE },
        { "Focus time", "No meetings", 2, 13.5f, 16, CUI_TONE_TEAL }, { "Standup", "Zoom", 3, 9.5f, 9.75f, CUI_TONE_INFO },
        { "Customer call", "Northlight", 3, 14, 15, CUI_TONE_DANGER }, { "Demo day", "All hands", 4, 15, 16.5f, CUI_TONE_VIOLET },
        { "Gym", NULL, 5, 10, 11.5f, CUI_TONE_SUCCESS },
    };
    cui_date wk = { 2026, 11, 2 };
    cui_week_calendar(ui, "week", wk, events, 11, .start_hour = 8, .end_hour = 17, .hour_height = 44, .height = CUI_PX(400));
}
static void body_gantt(cui_ctx *ui);
static void scene_gantt(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_gantt); }
static void body_gantt(cui_ctx *ui) {
    static cui_gantt_task tasks[] = {
        /* no tones: the bars take their state's color (done, active, planned) from progress and today */
        { "Research", "Maya Okafor", 0, 5, 1, 0, -1, false },
        { "Wireframes", "Aiko Tanaka", 4, 10, 1, 0, 0, false },
        { "Design system", "Rosa Delgado", 8, 16, 0.7f, 0, 1, false },
        { "Core engine", "Jonas Berg", 6, 19, 0.55f, 0, 0, false },
        { "Beta", NULL, 21, 21, 0, 0, 3, true },
        { "Launch", "Priya Nair", 22, 27, 0, 0, 4, false },
    };
    cui_date org = { 2026, 10, 26 };
    cui_gantt(ui, "plan", tasks, 6, org, 27, .day_width = 26.3f, .today = 10);   /* the range fits the frame: no date cut at the edge */
}

/* ---- node editor: a small material graph ---- */
enum { NP_UV = 1, NP_TEX_VEC, NP_TEX_COLOR, NP_TEX_ALPHA, NP_MIX_A, NP_MIX_B, NP_MIX_FAC, NP_MIX_OUT,
       NP_ROUGH, NP_MUL_A, NP_MUL_B, NP_MUL_OUT, NP_OUT_BASE, NP_OUT_ROUGH, NP_OUT_NORMAL };
enum { PT_COLOR = 1, PT_FLOAT = 2, PT_VECTOR = 3 };
typedef struct { int id, from, to; } ne_link;
static float ne_pos[6][2] = { { 16, 40 }, { 176, 40 }, { 430, 50 }, { 780, 150 }, { 176, 300 }, { 430, 330 } };
static ne_link ne_links[16] = { { 1, NP_UV, NP_TEX_VEC }, { 2, NP_TEX_COLOR, NP_MIX_A }, { 3, NP_MIX_OUT, NP_OUT_BASE },
                                { 4, NP_ROUGH, NP_MUL_A }, { 5, NP_MUL_OUT, NP_OUT_ROUGH } };
static int ne_link_n = 5, ne_next_link = 6;
static bool ne_alive[6] = { true, true, true, true, true, true };
static float ne_rough = 0.62f, ne_fac = 0.35f;
static cui_node_view ne_view;
static cui_id ne_mul_card, ne_alpha_pin, ne_fac_pin;
static void body_node_editor(cui_ctx *ui);
static void scene_node_editor(cui_ctx *ui, int f) {
    if (f < 0) {   /* select Multiply, then pull a wire from the texture's alpha toward the mix factor */
        int t = -f - 1;
        cui_rect m = cui_get_rect(ui, ne_mul_card), a = cui_get_rect(ui, ne_alpha_pin), b = cui_get_rect(ui, ne_fac_pin);
        float ax = a.x + a.w * 0.5f, ay = a.y + a.h * 0.5f, bx = b.x + b.w * 0.5f - 34, by = b.y + b.h * 0.5f - 8;
        if (t == 8) cui_input_mouse_move(ui, m.x + 60, m.y + 16);
        if (t == 9) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t == 10) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (t == 20) cui_input_mouse_move(ui, ax, ay);
        if (t == 21) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t >= 22 && t <= 46) { float k = (t - 22) / 24.0f; cui_input_mouse_move(ui, ax + (bx - ax) * k, ay + (by - ay) * k); }
        return;
    }
    frame_wrap(ui, body_node_editor);
}
static void body_node_editor(cui_ctx *ui) {
    static const char *titles[6] = { "UV", "Brick texture", "Mix", "Material output", "Roughness", "Multiply" };
    static const float widths[6] = { 110, 160, 150, 140, 160, 140 };
    if (cui_node_editor_begin(ui, "material", &ne_view, .height = CUI_FILL, .minimap = true)) {
        for (int i = 0; i < 6; i++) {
            if (!ne_alive[i]) continue;
            cui_node_begin(ui, i, &ne_pos[i][0], &ne_pos[i][1], titles[i], .width = widths[i]);
            switch (i) {
            case 0: cui_node_output(ui, NP_UV, "UV", .type = PT_VECTOR); break;
            case 1:
                cui_node_input(ui, NP_TEX_VEC, "Vector", .type = PT_VECTOR);
                cui_node_output(ui, NP_TEX_COLOR, "Color", .type = PT_COLOR);
                cui_node_output(ui, NP_TEX_ALPHA, "Alpha", .type = PT_FLOAT);
                ne_alpha_pin = cui_last_id(ui);
                break;
            case 2:   /* inputs without a link show a control for their value */
                cui_node_input(ui, NP_MIX_A, "Color A", .type = PT_COLOR);
                if (!cui_node_input(ui, NP_MIX_B, "Color B", .type = PT_COLOR))
                    cui_node_content(ui) {
                        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                            cui_box(ui, .width = CUI_PX(18), .height = CUI_PX(18), .radius = 5, .bg = CUI_RGB(0xb8683f)) {}
                            cui_text(ui, "#B8683F", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY, .tabular = true);
                        }
                    }
                bool linked = cui_node_input(ui, NP_MIX_FAC, "Factor", .type = PT_FLOAT);
                ne_fac_pin = cui_last_id(ui);
                if (!linked) cui_node_content(ui) { cui_slider(ui, "##fac", &ne_fac, 0, 1); }
                cui_node_output(ui, NP_MIX_OUT, "Color", .type = PT_COLOR);
                break;
            case 3:
                cui_node_input(ui, NP_OUT_BASE, "Base color", .type = PT_COLOR);
                cui_node_input(ui, NP_OUT_ROUGH, "Roughness", .type = PT_FLOAT);
                cui_node_input(ui, NP_OUT_NORMAL, "Normal", .type = PT_VECTOR);
                break;
            case 4:
                cui_node_content(ui) { cui_slider(ui, "Value", &ne_rough, 0, 1, .show_value = true, .format = "%.2f"); }
                cui_node_output(ui, NP_ROUGH, "Value", .type = PT_FLOAT);
                break;
            case 5:
                ne_mul_card = cui_last_id(ui);
                cui_node_input(ui, NP_MUL_A, "A", .type = PT_FLOAT);
                cui_node_input(ui, NP_MUL_B, "B", .type = PT_FLOAT);
                cui_node_output(ui, NP_MUL_OUT, "Result", .type = PT_FLOAT);
                break;
            }
            cui_node_end(ui);
        }
        for (int k = 0; k < ne_link_n; k++) cui_node_link(ui, ne_links[k].id, ne_links[k].from, ne_links[k].to);
        /* the graph is ours: apply what the user did */
        int a, b, id;
        while (cui_node_link_removed(ui, &id))
            for (int k = 0; k < ne_link_n; k++) if (ne_links[k].id == id) { ne_links[k] = ne_links[--ne_link_n]; break; }
        while (cui_node_link_created(ui, &a, &b))
            if (ne_link_n < 16) ne_links[ne_link_n++] = (ne_link){ ne_next_link++, a, b };
        while (cui_node_deleted(ui, &id))
            if (id >= 0 && id < 6) ne_alive[id] = false;
        cui_node_editor_end(ui);
    }
}

/* ---- flame graph: two game frames from a profiler capture ---- */
static cui_flame_span fl_spans[2400];
static int fl_n, fl_sel = -1, fl_hover = -1;
static cui_flame_view fl_view;   /* zeroed: fits the capture */
static cui_id fl_id;
static unsigned fl_seed = 41;
static double fl_rand(void) { fl_seed = fl_seed * 1103515245u + 12345u; return (double)((fl_seed >> 16) & 0x7fff) / 32767.0; }
static void fl_add(const char *name, double s, double e, int depth, uint32_t key) {
    if (fl_n < (int)(sizeof fl_spans / sizeof fl_spans[0])) fl_spans[fl_n++] = (cui_flame_span){ name, s * 1e-3, e * 1e-3, depth, key };
}
/* n children of [s, e] (milliseconds) named name, filling about `fill` of it */
static void fl_split(const char *name, double s, double e, int depth, int n, double fill) {
    double slot = (e - s) / n;
    for (int k = 0; k < n; k++) {
        double w = slot * fill * (0.55 + 0.45 * fl_rand());
        double a = s + k * slot + (slot - w) * fl_rand();
        fl_add(name, a, a + w, depth, 0);
    }
}
/* named children laid end to end with small gaps; weights give their share */
static void fl_seq(double s, double e, int depth, int n, const char *const *names, const double *w, double *out) {
    double tot = 0, x = s;
    for (int k = 0; k < n; k++) tot += w[k];
    double gap = (e - s) * 0.012;
    double len = (e - s) - gap * (n + 1);
    for (int k = 0; k < n; k++) {
        double a = x + gap, b = a + len * w[k] / tot;
        fl_add(names[k], a, b, depth, 0);
        out[2 * k] = a; out[2 * k + 1] = b;
        x = b;
    }
}
static void fl_frame(const char *title, double f0, double len) {
    double o[16];
    fl_add(title, f0, f0 + len, 0, 3);   /* frames share a color */
    static const char *const top[] = { "Update", "Render", "Present" };
    double tw[] = { 7.1, 7.6, 1.3 }, t3[6];
    fl_seq(f0, f0 + len, 1, 3, top, tw, t3);
    /* update */
    static const char *const upd[] = { "Input", "Animation", "Physics", "AI", "Scripts" };
    double uw[] = { 0.4, 1.5, 3.2, 1.1, 0.9 }, u[10];
    fl_seq(t3[0], t3[1], 2, 5, upd, uw, u);
    fl_split("Poll devices", u[0], u[1], 3, 3, 0.7);
    static const char *const anim[] = { "Blend tree", "Skinning" };
    double aw[] = { 0.45, 0.55 };
    fl_seq(u[2], u[3], 3, 2, anim, aw, o);
    fl_split("Evaluate clip", o[0], o[1], 4, 9, 0.8);
    fl_split("Skin mesh", o[2], o[3], 4, 24, 0.75);
    static const char *const phys[] = { "Broadphase", "Narrowphase", "Solver" };
    double pw[] = { 0.6, 0.9, 1.5 }, ph[6];
    fl_seq(u[4], u[5], 3, 3, phys, pw, ph);
    fl_split("Sweep axis", ph[0], ph[1], 4, 3, 0.85);
    fl_split("Collide pair", ph[2], ph[3], 4, 46, 0.7);
    double sw[8] = { 1.3, 1, 0.8, 1.1, 0.7, 0.9, 1.2, 0.6 }, is[16];
    static const char *const isl[] = { "Island 0", "Island 1", "Island 2", "Island 3", "Island 4", "Island 5", "Island 6", "Island 7" };
    fl_seq(ph[4], ph[5], 4, 8, isl, sw, is);
    for (int k = 0; k < 8; k++) {
        static const char *const st[] = { "Contacts", "Integrate" };
        double stw[] = { 0.6, 0.4 }, so[4];
        fl_seq(is[2 * k], is[2 * k + 1], 5, 2, st, stw, so);
        fl_split("Body", so[2], so[3], 6, 6, 0.8);
    }
    static const char *const ai[] = { "Perception", "Pathfinding" };
    double aiw[] = { 0.35, 0.65 }, ao[4];
    fl_seq(u[6], u[7], 3, 2, ai, aiw, ao);
    fl_split("A* search", ao[2], ao[3], 4, 6, 0.85);
    fl_split("OnUpdate", u[8], u[9], 3, 28, 0.7);
    /* render */
    static const char *const ren[] = { "Cull", "Shadow pass", "GBuffer", "Lighting", "Post" };
    double rw[] = { 0.8, 1.9, 2.6, 1.3, 0.8 }, r[10];
    fl_seq(t3[2], t3[3], 2, 5, ren, rw, r);
    fl_split("Frustum test", r[0], r[1], 3, 4, 0.8);
    static const char *const cas[] = { "Cascade 0", "Cascade 1", "Cascade 2", "Cascade 3" };
    double cw[] = { 1.4, 1.1, 0.9, 0.7 }, co[8];
    fl_seq(r[2], r[3], 3, 4, cas, cw, co);
    for (int k = 0; k < 4; k++) fl_split("Draw", co[2 * k], co[2 * k + 1], 4, 22, 0.7);
    static const char *const gb[] = { "Opaque", "Decals" };
    double gw[] = { 0.8, 0.2 }, go[4];
    fl_seq(r[4], r[5], 3, 2, gb, gw, go);
    double bo[24], bw[12];
    static const char *const batch[] = { "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch", "Batch" };
    for (int k = 0; k < 12; k++) bw[k] = 0.6 + fl_rand();
    fl_seq(go[0], go[1], 4, 12, batch, bw, bo);
    for (int k = 0; k < 12; k++) fl_split("Draw", bo[2 * k], bo[2 * k + 1], 5, 8, 0.75);
    static const char *const li[] = { "Light culling", "Deferred shading" };
    double lw[] = { 0.35, 0.65 }, lo[4];
    fl_seq(r[6], r[7], 3, 2, li, lw, lo);
    static const char *const post[] = { "Bloom", "Tone map", "TAA" };
    double pow3[] = { 0.4, 0.2, 0.4 }, po[6];
    fl_seq(r[8], r[9], 3, 3, post, pow3, po);
    /* present */
    static const char *const pre[] = { "Swap buffers" };
    double prw[] = { 1 }, pr[2];
    fl_seq(t3[4], t3[5], 2, 1, pre, prw, pr);
    static const char *const wt[] = { "Wait for GPU" };
    fl_seq(pr[0], pr[1], 3, 1, wt, prw, o);
}
static void fl_build(void) {
    if (fl_n) return;
    fl_frame("Frame 4812", 0.0, 16.4);
    fl_frame("Frame 4813", 16.6, 16.9);
    cui_flame_sort(fl_spans, fl_n);
    for (int i = 0; i < fl_n; i++) {   /* select Physics in the second frame, hover Lighting in the first */
        if (!strcmp(fl_spans[i].name, "Physics") && fl_spans[i].start > 0.0166) fl_sel = i;
        if (!strcmp(fl_spans[i].name, "Lighting") && fl_spans[i].start < 0.0166) fl_hover = i;
    }
}
static void body_flame(cui_ctx *ui);
static void scene_flame(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        if (t == 3 && fl_id && fl_hover >= 0) {
            cui_rect r = cui_flame_span_rect(ui, fl_id, &fl_spans[fl_hover]);
            if (r.w > 0) cui_input_mouse_move(ui, r.x + r.w * 0.5f, r.y + r.h * 0.5f);
        }
        return;
    }
    frame_wrap(ui, body_flame);
}
static void body_flame(cui_ctx *ui) {
    fl_build();
    cui_column(ui, .width = CUI_FILL, .gap = 10) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
            cui_text(ui, "Frames 4812 to 4813", .font = CUI_FONT_MEDIUM, .size = 13);
            /* the selection is ours: the graph writes fl_sel and we show it */
            if (fl_sel >= 0) {
                const cui_flame_span *s = &fl_spans[fl_sel];
                char line[96];
                snprintf(line, sizeof line, "%s, %.2f ms", s->name, (s->end - s->start) * 1e3);
                cui_text(ui, line, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY, .tabular = true);
            } else {
                cui_text(ui, "Click a span to select it", .size = 13, .color = CUI_COLOR_TEXT_MUTED);
            }
        }
        cui_flame_graph(ui, "capture", fl_spans, fl_n, &fl_view, &fl_sel);
        fl_id = cui_last_id(ui);
    }
}

/* ---- more charts ---- */
static float viz_traffic[7 * 24], viz_days[364];
static void viz_data(void) {
    static bool done;
    if (done) return;
    for (int d = 0; d < 7; d++)
        for (int h = 0; h < 24; h++) {
            float work = d < 5 ? 1.0f : 0.45f;
            viz_traffic[d * 24 + h] = expf(-powf((h - 13.5f) / 4.2f, 2)) * 90 * work + expf(-powf((h - 20.5f) / 2.0f, 2)) * 35 + (float)((d * 31 + h * 17) % 11);
        }
    unsigned sd = 7;
    for (int k = 0; k < 364; k++) {
        sd = sd * 1103515245u + 12345u;
        int wd = (k + 3) % 7;   /* the start below is a Thursday */
        float base = (wd >= 5 ? 0.6f : 3.2f) * (0.6f + 0.4f * sinf(k / 40.0f) * sinf(k / 40.0f));
        float rr = (float)((sd >> 16) % 1000) / 1000.0f;
        viz_days[k] = rr < 0.18f ? 0 : floorf(base * rr * 4);
    }
    done = true;
}
static void body_heatmap(cui_ctx *ui);
static void scene_heatmap(cui_ctx *ui, int f) {
    /* Wednesday at 14h, to show the ring, the tooltip and the lit labels */
    if (f < 0) { cui_rect r = cui_get_rect(ui, cui_make_id(ui, "traffic")); if (-f - 1 == 40 && r.w > 0) cui_input_mouse_move(ui, r.x + 40 + 14.5f * 33, r.y + 2.5f * 33); return; }
    frame_wrap(ui, body_heatmap);
}
static void body_heatmap(cui_ctx *ui) {
    viz_data();
    static const char *wd[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const char *hours[24] = { "0h", NULL, NULL, "3h", NULL, NULL, "6h", NULL, NULL, "9h", NULL, NULL, "12h", NULL, NULL, "15h", NULL, NULL, "18h", NULL, NULL, "21h", NULL, NULL };
    cui_heatmap(ui, "traffic", viz_traffic, 7, 24, .row_labels = wd, .col_labels = hours, .format = "%.0f req");
}
SCENE(calendar_heatmap) {
    viz_data();
    cui_date start = { 2025, 11, 6 };
    cui_calendar_heatmap(ui, "year", start, viz_days, 364, .unit = "contributions");
}
static void body_treemap_radar(cui_ctx *ui);
static void scene_treemap_radar(cui_ctx *ui, int f) {
    /* the 1.1 point on the Size axis, to show the radar's value chip */
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "radar"));
        if (-f - 1 == 40 && r.w > 0) cui_input_mouse_move(ui, r.x + r.w * 0.5f + 0.866f * 85, r.y + 26 + 100 - 0.5f * 85);
        return;
    }
    frame_wrap(ui, body_treemap_radar);
}
static void body_treemap_radar(cui_ctx *ui) {
    static const cui_viz_item mods[] = {
        { "Layout", 55 }, { "Text", 42 }, { "Draw", 35 }, { "Controls", 31 }, { "Charts", 23 },
        { "Overlays", 19 }, { "Platform", 16 }, { "Theme", 9 }, { "Icons", 7 }, { "Grid", 6 },
    };
    static const float a[] = { 82, 64, 90, 58, 75, 88 }, b[] = { 70, 85, 72, 80, 60, 79 };
    static const char *axes[] = { "Speed", "Size", "Polish", "Docs", "Platforms", "Tests" };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .wrap = true) {
        static int picked = 2;   /* "Draw", to show the selected tile */
        cui_box(ui, .width = CUI_GROW(1), .min_width = 300) { cui_treemap(ui, "bundle", mods, 10, .height = 290, .format = "%.0f KB", .selected = &picked); }
        cui_series rs[] = { { "1.0", a, 6, { 0 } }, { "1.1", b, 6, { 0 } } };
        cui_column(ui, .gap = 2, .align = CUI_ALIGN_CENTER) {
            cui_label(ui, "Review scores by release");
            cui_radar_chart(ui, "radar", rs, 2, axes, 6, .size = 200, .max = 100);
        }
    }
}
SCENE(funnel_split) {
    static const cui_viz_item stages[] = { { "Visited", 12840 }, { "Signed up", 4210 }, { "Activated", 2380 }, { "Subscribed", 860 } };
    static const cui_viz_item parts[] = { { "Layout", 4.1f }, { "Text", 2.6f }, { "Paint", 2.3f }, { "Input", 1.0f }, { "Other", 0.9f } };
    static const cui_viz_item plats[] = { { "Windows", 46 }, { "macOS", 31 }, { "Linux", 23 } };
    cui_row(ui, .gap = 36, .align = CUI_ALIGN_START, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, .width = CUI_GROW(1), .min_width = 280) { cui_funnel(ui, "funnel", stages, 4); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 280, .gap = 26) {
            cui_split_bar(ui, "split", parts, 5, .format = "%.1f ms");
            cui_waffle(ui, "waffle", plats, 3, .cell = 12);
        }
    }
}
static const cui_month_event month_events[] = {
    { "Design sprint", { 2026, 11, 2 }, { 2026, 11, 4 }, 0, true, CUI_TONE_VIOLET },
    { "Standup", { 2026, 11, 2 }, { 0 }, 9.5f, false, CUI_TONE_INFO },
    { "1:1 with Jonas", { 2026, 11, 3 }, { 0 }, 14, false, CUI_TONE_ROSE },
    { "Release 1.2", { 2026, 11, 6 }, { 0 }, 0, true, CUI_TONE_SUCCESS },
    { "Standup", { 2026, 11, 9 }, { 0 }, 9.5f, false, CUI_TONE_INFO },
    { "Customer call", { 2026, 11, 10 }, { 0 }, 11, false, CUI_TONE_DANGER },
    { "Lunch with Maya", { 2026, 11, 10 }, { 0 }, 12.5f, false, CUI_TONE_TEAL },
    { "Review: docking", { 2026, 11, 10 }, { 0 }, 15, false, CUI_TONE_VIOLET },
    { "Planning", { 2026, 11, 10 }, { 0 }, 16.5f, false, CUI_TONE_WARNING },
    { "Offsite, Lisbon", { 2026, 11, 12 }, { 2026, 11, 14 }, 0, true, CUI_TONE_WARNING },
    { "Standup", { 2026, 11, 16 }, { 0 }, 9.5f, false, CUI_TONE_INFO },
    { "Accessibility audit", { 2026, 11, 17 }, { 2026, 11, 20 }, 0, true, CUI_TONE_TEAL },
    { "Demo day", { 2026, 11, 20 }, { 0 }, 16, false, CUI_TONE_VIOLET },
    { "Standup", { 2026, 11, 23 }, { 0 }, 9.5f, false, CUI_TONE_INFO },
    { "Holiday", { 2026, 11, 26 }, { 2026, 11, 27 }, 0, true, CUI_TONE_SUCCESS },
    { "Board meeting", { 2026, 11, 30 }, { 0 }, 10, false, CUI_TONE_DANGER },
};
SCENE(month_calendar) {
    static cui_date month = { 2026, 11, 1 }, sel = { 2026, 11, 10 };
    cui_box(ui, UPTO(880)) {
        cui_month_calendar(ui, "month", &month, month_events, (int)(sizeof month_events / sizeof month_events[0]), .selected = &sel,
                           .today = (cui_date){ 2026, 11, 8 }, .cell_height = 96);
    }
}
static int g_tour_step = 1;
static void body_tour(cui_ctx *ui) {
    static cui_tour_step steps[4] = {
        { 0, "Welcome to Fieldnotes", "A quick look at the three things you will use every day.", 0 },
        { 0, "Export anywhere", "Send a report to PDF, Markdown or your team's drive. Shift+Export repeats the last one.", CUI_PLACE_BOTTOM_END },
        { 0, "Share with your team", "People you invite can comment without an account.", CUI_PLACE_BOTTOM_END },
        { 0, "Search everything", "Ctrl K finds notes, people and commands.", CUI_PLACE_BOTTOM_START },
    };
    cui_card(ui, UPTO(760), .pad = 1, .gap = 0) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .pad = 12, .wrap = true) {
            cui_search(ui, "q", (char[32]){ 0 }, 32, .placeholder = "Search", .width = CUI_PX(200));
            steps[3].target = cui_last_id(ui);
            cui_spacer(ui);
            cui_button(ui, "Share", .icon = CUI_ICON_USERS, .variant = CUI_BUTTON_OUTLINE);
            steps[2].target = cui_last_id(ui);
            cui_button(ui, "Export", .icon = CUI_ICON_DOWNLOAD, .variant = CUI_BUTTON_PRIMARY);
            steps[1].target = cui_last_id(ui);
        }
        cui_separator(ui);
        cui_column(ui, .pad = 20, .gap = 10, .width = CUI_FILL) {
            cui_heading(ui, "Q3 field report", 3);
            cui_text(ui, "Survey results from the coastal sites, with photos and the soil samples sent to the lab.", .color = CUI_COLOR_TEXT_SECONDARY);
            cui_skeleton(ui, CUI_PCT(90), 10);
            cui_skeleton(ui, CUI_PCT(75), 10);
            cui_skeleton(ui, CUI_PCT(82), 10);
        }
    }
    if (g_tour_step >= 0) cui_tour(ui, "intro", steps, 4, &g_tour_step);
}
static void scene_tour(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_tour); }
/* a small made-up disk for the file browser demos, the same on every run */
typedef struct { const char *dir, *name; bool folder; double size; int day; } demo_file;
static const demo_file demo_files[] = {
    { "/Users/maya", "Desktop", true, 0, 40 }, { "/Users/maya", "Documents", true, 0, 38 }, { "/Users/maya", "Downloads", true, 0, 41 },
    { "/Users/maya", "Projects", true, 0, 42 }, { "/Users/maya", "notes.md", false, 4210, 30 },
    { "/Users/maya/Projects", "fieldnotes", true, 0, 42 }, { "/Users/maya/Projects", "harbor-town", true, 0, 12 }, { "/Users/maya/Projects", "cui", true, 0, 44 },
    { "/Users/maya/Projects/fieldnotes", "assets", true, 0, 39 }, { "/Users/maya/Projects/fieldnotes", "build", true, 0, 44 },
    { "/Users/maya/Projects/fieldnotes", "src", true, 0, 44 }, { "/Users/maya/Projects/fieldnotes", "CMakeLists.txt", false, 1840, 31 },
    { "/Users/maya/Projects/fieldnotes", "README.md", false, 6320, 36 }, { "/Users/maya/Projects/fieldnotes", "cover.png", false, 482113, 33 },
    { "/Users/maya/Projects/fieldnotes", "field-report-q3.pdf", false, 2814400, 41 }, { "/Users/maya/Projects/fieldnotes", "samples-2.csv", false, 91000, 20 },
    { "/Users/maya/Projects/fieldnotes", "samples-10.csv", false, 143000, 21 }, { "/Users/maya/Projects/fieldnotes", "site-photos.zip", false, 182400000, 37 },
    { "/Users/maya/Projects/fieldnotes", "theme.mp3", false, 5400000, 10 }, { "/Users/maya/Projects/fieldnotes", "main.c", false, 12800, 44 },
    { "/Users/maya/Projects/fieldnotes/src", "main.c", false, 12800, 44 }, { "/Users/maya/Projects/fieldnotes/src", "report.c", false, 30200, 43 },
    { "/Users/maya/Projects/fieldnotes/assets", "logo.svg", false, 3100, 39 }, { "/Users/maya/Projects/fieldnotes/assets", "map.png", false, 991000, 39 },
};
static int demo_fs(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden) {
    (void)user; (void)names; (void)names_cap; (void)hidden;
    int n = 0;
    for (size_t k = 0; k < sizeof demo_files / sizeof demo_files[0] && n < cap; k++) {
        if (strcmp(demo_files[k].dir, path)) continue;
        /* 2024-09-01 plus a number of days, in the afternoon */
        out[n++] = (cui_file_entry){ demo_files[k].name, demo_files[k].folder, (uint64_t)demo_files[k].size, 1725148800 + demo_files[k].day * 86400 + 14 * 3600 + k * 600 };
    }
    if (!n && strncmp(path, "/Users/maya", 11)) return -1;
    return n;
}
SCENE(file_browser) {
    static char path[512] = "/Users/maya/Projects/fieldnotes";
    static const char *const places[] = { "/Users/maya", "/Users/maya/Desktop", "/Users/maya/Documents", "/Users/maya/Projects" };
    static bool open;
    static char picked[512];
    cui_box(ui, UPTO(900), .gap = 10) {
        cui_file_browser(ui, "files", path, sizeof path, .list = demo_fs, .places = places, .place_count = 4, .height = CUI_PX(360));
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            if (cui_button(ui, "Save as\xe2\x80\xa6", .icon = CUI_ICON_DOWNLOAD)) open = true;
            if (picked[0]) cui_muted(ui, picked);
        }
        cui_file_picker(ui, "save", &open, picked, sizeof picked, .save = true, .name = "report.pdf", .start = "/Users/maya/Documents", .list = demo_fs);
    }
}
static cui_terminal_buffer g_term;
static void term_demo_fill(void) {
    static char text[16384];
    static cui_terminal_line lines[256];
    cui_terminal_init(&g_term, text, sizeof text, lines, 256);
    cui_terminal_buffer *t = &g_term;
    cui_terminal_write(t, "\x1b[2m~/fieldnotes $ \x1b[0mcmake --build build -j8\n", -1);
    static const char *const units[] = { "cui_core.c", "cui_layout.c", "cui_text.c", "cui_widgets.c", "cui_viz.c", "cui_month.c" };
    for (int i = 0; i < 6; i++)
        cui_terminal_printf(t, "[%3d%%] \x1b[32mBuilding C object\x1b[0m src/CMakeFiles/cui.dir/%s.o\n", (i + 1) * 14, units[i]);
    cui_terminal_write(t, "\x1b[1msrc/cui_month.c:212:14: \x1b[35mwarning:\x1b[0m\x1b[1m unused variable 'span'\x1b[0m\n", -1);
    cui_terminal_write(t, "  212 |     float span = r.w / 7;\n", -1);
    cui_terminal_write(t, "      |           \x1b[32m^~~~\x1b[0m\n", -1);
    cui_terminal_write(t, "[ 92%] \x1b[1;32mLinking C static library libcui.a\x1b[0m\n", -1);
    cui_terminal_write(t, "[100%] Built target cui\n", -1);
    cui_terminal_write(t, "\x1b[2m~/fieldnotes $ \x1b[0mctest --output-on-failure\n", -1);
    /* a progress bar that redraws itself in place */
    for (int k = 0; k <= 40; k += 8) {
        char bar[64];
        int n = 0;
        for (int j = 0; j < 20; j++) n += snprintf(bar + n, sizeof bar - n, "%s", j < k / 2 ? "\xe2\x96\x88" : "\xe2\x96\x91");
        cui_terminal_printf(t, "\r\x1b[36m%s\x1b[0m %2d/40 tests", bar, k);
    }
    cui_terminal_write(t, "\n\x1b[32m100% tests passed\x1b[0m, 0 tests failed out of 40\n", -1);
    cui_terminal_write(t, "Total Test time (real) =   3.18 sec\n", -1);
}
SCENE(terminal) {
    static bool filled;
    if (!filled) { term_demo_fill(); filled = true; }
    cui_box(ui, UPTO(820)) {
        cui_terminal_result r = cui_terminal(ui, "term", &g_term, .height = 380, .prompt = "~/fieldnotes $ ");
        if (r.command && r.command[0]) cui_terminal_printf(&g_term, "%s: command not found\n", r.command);
    }
}
SCENE(emoji_picker) {
    static char recent[256] = "\xf0\x9f\x91\x8d \xf0\x9f\x8e\x89 \xe2\x9c\x85 \xf0\x9f\x98\x82 \xf0\x9f\x9a\x80 \xf0\x9f\x91\x80 \xe2\x9d\xa4\xef\xb8\x8f \xf0\x9f\x99\x8f";
    static int tone = 0;
    static char note[96] = "Shipped the fix ";
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .wrap = true, .width = CUI_FILL) {
        const char *e = cui_emoji_picker(ui, "emoji", .recent = recent, .recent_cap = sizeof recent, .tone = &tone);
        if (e && strlen(note) + strlen(e) < sizeof note) strcat(note, e);
        cui_column(ui, .gap = 10, .width = CUI_GROW(1), .min_width = 240) {
            cui_label(ui, "Status");
            cui_input(ui, "note", note, sizeof note, .width = CUI_FILL);
            cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER) {
                cui_emoji(ui, "\xf0\x9f\x8e\x89", 18); cui_text(ui, "3", .size = 13);
                cui_emoji(ui, "\xf0\x9f\x91\x80", 18); cui_text(ui, "5", .size = 13);
                cui_emoji(ui, "\xf0\x9f\x9a\x80", 18); cui_text(ui, "2", .size = 13);
            }
        }
    }
}
SCENE(gauges) {
    static const cui_gauge_band load[] = { { 60, CUI_TONE_SUCCESS, "Normal" }, { 85, CUI_TONE_WARNING, "Elevated" }, { 100, CUI_TONE_DANGER, "High" } };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
        cui_radial_gauge(ui, "cpu", 72, 0, 100, .unit = "%", .label = "CPU load", .bands = load, .band_count = 3);
        cui_radial_gauge(ui, "rps", 1840, 0, 2500, .label = "Requests / s", .show_target = true, .target = 2000);
        cui_radial_gauge(ui, "disk", 38, 0, 100, .unit = "%", .label = "Disk", .style = CUI_GAUGE_RING, .detail = "190 of 500 GB", .size = 150);
    }
}
/* a price history for the candlestick demo: a seeded random walk with a trend and some volatility clusters */
static int demo_candles(cui_candle *out, int n, const char **labels, char (*buf)[12], float *sma, int sma_n) {
    uint32_t seed = 42;
    float price = 182.0f;
    static const char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    for (int i = 0; i < n; i++) {
        float u[5];
        for (int j = 0; j < 5; j++) { seed = seed * 1664525u + 1013904223u; u[j] = (float)(seed >> 8) / 16777216.0f; }
        float vol = 0.012f + 0.018f * (0.5f + 0.5f * sinf(i * 0.07f));
        float o = price, c = o * (1 + (u[0] - 0.48f) * 2 * vol);
        float h = fmaxf(o, c) * (1 + u[1] * vol * 0.8f), l = fminf(o, c) * (1 - u[2] * vol * 0.8f);
        out[i] = (cui_candle){ o, h, l, c, (0.6f + u[3] * 0.9f + fabsf(c - o) / o * 30) * 1.2e6f };
        price = c;
        int day = i % 28 + 1, m = (8 + i / 28) % 12;
        snprintf(buf[i], 12, "%s %d", mon[m], day);
        labels[i] = buf[i];
    }
    for (int i = 0; i < n; i++) {
        if (i + 1 < sma_n) { sma[i] = NAN; continue; }
        float s = 0;
        for (int k = 0; k < sma_n; k++) s += out[i - k].close;
        sma[i] = s / sma_n;
    }
    return n;
}
SCENE(candlestick) {
    enum { N = 140 };
    static cui_candle cs[N];
    static const char *labels[N];
    static char buf[N][12];
    static float sma[N];
    static bool init;
    if (!init) { demo_candles(cs, N, labels, buf, sma, 20); init = true; }
    cui_series ov = { "SMA 20", sma, N, { 0 } };
    cui_card(ui, UPTO(820), .gap = 10, .pad = 18) {
        cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
            cui_text(ui, "NRTH", .font = CUI_FONT_MEDIUM, .size = 15);
            cui_text(ui, "Northwind Labs", .color = CUI_COLOR_TEXT_SECONDARY);
            cui_spacer(ui);
            cui_badge(ui, "Daily", .tone = CUI_TONE_NEUTRAL);
        }
        cui_candlestick(ui, "nrth", cs, N, .labels = labels, .overlays = &ov, .overlay_count = 1, .height = 300);
    }
}
static void body_sankey(cui_ctx *ui);
static void scene_sankey(cui_ctx *ui, int f) {
    /* the Search to Sign up flow, to show the path it lights */
    if (f < 0) { cui_rect r = cui_get_rect(ui, cui_make_id(ui, "flow")); if (-f - 1 == 40 && r.w > 0) cui_input_mouse_move(ui, r.x + r.w * 0.12f, r.y + 40); return; }
    frame_wrap(ui, body_sankey);
}
static void body_sankey(cui_ctx *ui) {
    static const char *nodes[] = { "Search", "Social", "Referral", "Sign up", "Bounced", "Trial", "Paid", "Churned" };
    static const cui_sankey_link links[] = {
        { 0, 3, 420 }, { 1, 3, 260 }, { 2, 3, 180 }, { 0, 4, 300 }, { 1, 4, 220 },
        { 3, 5, 860 }, { 5, 6, 580 }, { 5, 7, 280 },
    };
    /* colored by meaning: neutral sources, the funnel in the first series color, paid green, lost red */
    const cui_color colors[] = { { 0 }, { 0 }, { 0 }, CUI_COLOR_SERIES(0), CUI_COLOR_DANGER, CUI_COLOR_SERIES(0), CUI_COLOR_SUCCESS, CUI_COLOR_TEXT_MUTED };
    cui_sankey(ui, "flow", nodes, 8, links, 8, .height = 300, .node_colors = colors);
}
SCENE(bar_list_rings) {
    static const cui_viz_item pages[] = { { "/docs", 4820 }, { "/", 3920 }, { "/gallery", 2410 }, { "/blog/fast-ui-in-c", 1830 }, { "/download", 1210 } };
    static const cui_viz_item rings[] = { { "Move", 0.82f }, { "Exercise", 1.24f }, { "Stand", 0.58f } };
    cui_row(ui, .gap = 40, .align = CUI_ALIGN_CENTER, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, .width = CUI_GROW(1), .min_width = 260) { cui_bar_list(ui, "pages", pages, 5); }
        cui_activity_rings(ui, "rings", rings, 3, .size = 180, .legend = true);
    }
}
SCENE(scatter) {
    static cui_vec2 a[40], b[40];
    static float sa[40], sb[40];
    /* measured-looking runs: the software renderer scales with the node count, OpenGL pays a fixed upload
       and then grows slowly, so the two cross near a thousand nodes */
    uint32_t seed = 7;
    for (int k = 0; k < 40; k++) {
        float u[4];
        for (int j = 0; j < 4; j++) { seed = seed * 1664525u + 1013904223u; u[j] = (float)(seed >> 8) / 16777216.0f; }
        float n = 150 + 3800 * (k + u[0]) / 40.0f, m = 150 + 3800 * (k + u[1]) / 40.0f;
        a[k] = (cui_vec2){ n, 0.025f + n * 0.000072f + (u[2] - 0.5f) * 0.03f * (0.5f + n / 4000) };
        b[k] = (cui_vec2){ m, 0.085f + m * 0.000026f + (u[3] - 0.5f) * 0.022f };
        sa[k] = sb[k] = 0;
    }
    cui_scatter_series ss[] = { { "Software", a, NULL, 40, { 0 } }, { "OpenGL", b, NULL, 40, { 0 } } };
    cui_scatter_chart(ui, "scatter", ss, 2, .height = 300, .x_label = "Nodes", .y_label = "Frame time (ms)", .y_format = "%.2f");
}

/* ---- motion ---- */
SCENE(text_effects) {
    cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_loader(ui, .style = CUI_LOADER_DOTS, .size = 22);
        cui_shimmer_text(ui, "Thinking about your question\xe2\x80\xa6", .size = 15);
    }
    cui_typewriter(ui, "tw", "CUI draws a full frame in a fraction of a millisecond, so text can stream in as fast as it arrives.", .caret = true, .size = 15);
    cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_text(ui, "Deploying to", .font = CUI_FONT_DISPLAY, .size = 26);
        static const char *words[] = { "staging.", "eu-west.", "all regions." };
        cui_word_rotate(ui, "rot", words, 3, 2.2f, .font = CUI_FONT_DISPLAY, .size = 26);
    }
}
SCENE(loaders) {
    cui_row(ui, .gap = 44, .align = CUI_ALIGN_CENTER, .wrap = true) {
        for (int k = 0; k < 5; k++) cui_loader(ui, .style = (cui_loader_style)k, .size = 30);
    }
}
static void body_dock(cui_ctx *ui) {
    cui_row(ui, .width = CUI_FILL, .justify = CUI_JUSTIFY_CENTER, .height = CUI_FILL, .align = CUI_ALIGN_END) {
        cui_dock(ui, "dock") {
            cui_dock_item(ui, CUI_ICON_FOLDER, "Files", .color = CUI_RGB(0x3b82f6), .running = true);
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
}
static void scene_dock(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "dock"));
        if (t == 15) cui_input_mouse_move(ui, r.x + r.w * 0.42f, r.y + r.h * 0.5f);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_dock(ui); }
}
SCENE(tiles) {
    static bool wifi = true, bt = false, focus = true, share = false, dark = true;
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 10, UPTO(300)) {
        cui_control_tile(ui, "Wi-Fi", &wifi, .icon = CUI_ICON_WIFI, .detail = "Home");
        cui_control_tile(ui, "Hotspot", &bt, .icon = CUI_ICON_RADIO);
        cui_control_tile(ui, "Focus", &focus, .icon = CUI_ICON_MOON, .detail = "Until 6 pm");
        cui_control_tile(ui, "AirShare", &share, .icon = CUI_ICON_SHARE_2, .detail = "Contacts only");
    }
    cui_box(ui, UPTO(300)) { cui_control_tile(ui, "Dark appearance", &dark, .icon = CUI_ICON_SUN, .wide = true, .width = CUI_FILL); }
}
SCENE(player) {
    static float peaks[96], progress = 0.36f;
    static int tab = 0;
    static cui_now_playing np = { "Low Tide", "Marlow Avenue", 0, { 0xc8, 0x55, 0x3d, 255 }, 76, 222, true };
    for (int k = 0; k < 96; k++) peaks[k] = 0.18f + 0.8f * fabsf(sinf(k * 0.37f) * sinf(k * 0.11f + 1.3f));
    static const cui_tab_bar_item tabs[] = { { CUI_ICON_HOUSE, "Home", 0 }, { CUI_ICON_SEARCH, "Search", 0 }, { CUI_ICON_INBOX, "Inbox", 4 }, { CUI_ICON_USER, "Profile", 0 } };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .wrap = true, .width = CUI_FILL) {
        cui_column(ui, .gap = 16) {
            cui_now_playing(ui, "np", &np);
            cui_card(ui, .pad = 14, .gap = 8, UPTO(360)) {
                cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                    cui_icon_button(ui, CUI_ICON_PLAY, .size = CUI_SIZE_SM, .variant = CUI_BUTTON_SECONDARY);
                    cui_box(ui, .width = CUI_FILL) { cui_waveform(ui, "wave", peaks, 96, &progress, .height = 34); }
                    cui_text(ui, "0:42", .size = 12, .tabular = true, .color = CUI_COLOR_TEXT_MUTED);
                }
            }
        }
        cui_column(ui, .gap = 16, .justify = CUI_JUSTIFY_END, UPTO(340)) {
            cui_gap(ui, 80);
            cui_tab_bar(ui, "tabs", tabs, 4, &tab, .width = CUI_FILL);
        }
    }
}

/* ---- AI ---- */
static void body_reasoning_tools(cui_ctx *ui) {
    cui_reasoning(ui, "think", .thinking = true, .seconds = 6) {
        cui_text(ui, "The question is about perceived speed: how little work a frame does, and how fast input reaches the screen.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
        cui_text(ui, "Look for sources on retained and immediate mode, text rendering and motion.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
    }
    cui_tool_call(ui, "search", "web_search", .icon = CUI_ICON_GLOBE, .detail = "immediate mode gui performance", .status = CUI_TASK_DONE, .seconds = 1.4f,
                  .input = "{\"query\": \"immediate mode gui performance\", \"limit\": 5}",
                  .output = "{\"results\": [{\"title\": \"Immediate mode GUIs\", \"url\": \"https://fieldnotes.dev/immediate-mode\"}], \"took_ms\": 1380}");
    cui_tool_call(ui, "read", "read_file", .icon = CUI_ICON_FILE_TEXT, .detail = "quarterly-report.pdf", .status = CUI_TASK_RUNNING, .seconds = 3.2f);
    cui_tool_call(ui, "run", "run_tests", .icon = CUI_ICON_SQUARE_TERMINAL, .detail = "ctest --output-on-failure", .status = CUI_TASK_FAILED, .seconds = 12.0f);
}
static void scene_reasoning_tools(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "search"), -f - 1, 10); if (-f - 1 == 14) cui_input_mouse_move(ui, 900, 5); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 10) { body_reasoning_tools(ui); }
}
SCENE(agent_plan) {
    static const cui_plan_step steps[] = {
        { "Search for sources", "3 results", CUI_TASK_DONE },
        { "Open the sources", "One link timed out", CUI_TASK_FAILED },
        { "Read the attached report", "Page 12 of 40", CUI_TASK_RUNNING },
        { "Compare the numbers with last quarter", NULL, CUI_TASK_PENDING },
        { "Draft the summary", NULL, CUI_TASK_PENDING },
    };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .width = CUI_FILL, .wrap = true) {
        cui_box(ui, FLEX(280, 380)) { cui_agent_plan(ui, "plan", steps, 5, .title = "Research plan"); }
        cui_column(ui, .gap = 10) {
            cui_agent_status(ui, "Reading quarterly-report.pdf", .status = CUI_TASK_RUNNING, .seconds = 14);
            cui_agent_status(ui, "Searched 12 sites", .status = CUI_TASK_DONE, .seconds = 9);
            cui_agent_status(ui, "Could not reach the API", .status = CUI_TASK_FAILED);
        }
    }
}
static const cui_source shot_sources[] = {
    { "Immediate mode GUIs and why they keep coming back", "https://fieldnotes.dev/immediate-mode", "A look at retained and immediate mode toolkits and the trade offs between them." },
    { "Notes on subpixel text positioning", "https://glyphlab.io/notes/subpixel", "Shaping, hinting and subpixel positioning explained with examples." },
    { "Spring physics for interface motion", "https://motionbook.dev/springs", "Why spring physics feel more natural than cubic beziers for interface motion." },
};
static void body_sources(cui_ctx *ui) {
    cui_text(ui, "They do very little work per frame: the interface is rebuilt from your data, laid out once and drawn in a handful of batches.", .size = 15);
    cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_text(ui, "Sources", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
        for (int k = 0; k < 3; k++) cui_citation(ui, k + 1, &shot_sources[k]);
    }
    cui_gap(ui, 120);
    cui_source_list(ui, "sources", shot_sources, 3);
}
static void scene_sources(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "cite2"));
        if (-f - 1 == 10) cui_input_mouse_move(ui, r.x + r.w / 2, r.y + r.h / 2);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 12, .justify = CUI_JUSTIFY_END) { body_sources(ui); }
}
static const cui_model shot_models[] = {
    { "Atlas", "Longer answers, reads files and code", "New", CUI_ICON_LAYERS },
    { "Atlas Fast", "Short answers in about a second", NULL, CUI_ICON_ZAP },
    { "Atlas Mini", "Runs on your own machine", NULL, CUI_ICON_BOX },
};
static void body_composer(cui_ctx *ui) {
    static char draft[256] = "Summarize the report and compare it with last quarter.";
    static const char *files[] = { "quarterly-report.pdf", "metrics.csv" };
    static int model = 0;
    cui_chat_composer(ui, "composer", draft, sizeof draft, .attachments = files, .attachment_count = 2, .models = shot_models, .model_count = 3, .model = &model);
}
static void scene_composer(cui_ctx *ui, int f) {
    if (f < 0) return;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_composer(ui); }
}
static void body_model_picker(cui_ctx *ui) {
    static int model = 0;
    cui_model_picker(ui, "picker", shot_models, 3, &model, .placement = CUI_PLACE_BOTTOM_START);
}
static void scene_model_picker(cui_ctx *ui, int f) {
    if (f < 0) { click_id(ui, cui_make_id(ui, "picker"), -f - 1, 10); if (-f - 1 == 14) cui_input_mouse_move(ui, 600, 5); return; }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui)) { body_model_picker(ui); }
}
SCENE(voice_orb) {
    static const char *names[] = { "Idle", "Listening", "Thinking", "Speaking" };
    cui_row(ui, .gap = 18, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
        for (int k = 0; k < 4; k++) cui_column(ui, .align = CUI_ALIGN_CENTER, .gap = 6) {
            cui_push_id_int(ui, k);
            cui_voice_orb(ui, "orb", (cui_voice_state)k, k == 1 ? 0.7f : k == 3 ? 0.5f : 0.2f, .size = cui_viewport_size(ui).x < 360 ? 112 : 150);
            cui_pop_id(ui);
            cui_text(ui, names[k], .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
        }
    }
}
/* ---- voice: levels as a microphone would give them, 20 a second: words with pauses
 * between them. The scenes step with the frame clock, so the docs image is the same
 * every time and the live site scrolls. */
static float voice_noise(float t, float s) {
    return (sinf(t * 1.7f + s) * 0.5f + sinf(t * 3.1f + s * 2.3f) * 0.3f + sinf(t * 7.3f + s * 0.7f) * 0.2f) * 0.5f + 0.5f;
}
static float voice_level(float t, float s) {
    return (sinf(t * 2.6f + s) > -0.45f ? 1.0f : 0.06f) * (0.28f + voice_noise(t * 2.4f, s) * 0.72f);
}
#define VOICE_STEP 0.05f   /* seconds between levels */
static int voice_step(cui_ctx *ui) { return (int)(cui_time(ui) / VOICE_STEP); }   /* levels measured so far */
/* the newest n levels of a voice after `step` of them, newest last */
static void voice_fill(float *out, int n, int step, float rate, float seed, float gain) {
    float now = step * VOICE_STEP + 40;
    for (int k = 0; k < n; k++) out[k] = gain * voice_level((now - (n - 1 - k) * VOICE_STEP) * rate, seed);
}
/* a finished clip: louder and quieter stretches and a pause */
static void voice_clip(float *out, int n, float seed) {
    for (int k = 0; k < n; k++)
        out[k] = (0.2f + voice_noise(k * 0.3f, seed) * 0.8f) * (sinf(k * 0.45f + seed) > -0.7f ? 1.0f : 0.15f);
}
static float vw_progress = 0.45f;
static void voice_wave_row(cui_ctx *ui, const char *label, const char *detail, bool accent) {
    cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
        cui_text(ui, label, .size = 12.5f, .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_TEXT_SECONDARY, .width = CUI_FILL);
        cui_text(ui, detail, .size = 12, .color = accent ? CUI_COLOR_ACCENT : CUI_COLOR_TEXT_MUTED);
    }
}
SCENE(voice_wave) {
    static float you[200], reply[200], clip[160];
    static char rec_time[16], note_time[16];
    voice_fill(you, 200, voice_step(ui), 1.1f, 1.7f, 0.85f);
    voice_fill(reply, 200, voice_step(ui), 1.3f, 5.0f, 1.0f);
    voice_clip(clip, 160, 1.3f);
    cui_column(ui, .gap = 10, UPTO(560)) {
        snprintf(rec_time, sizeof rec_time, "0:%02d", 7 + (int)cui_time(ui) % 50);
        voice_wave_row(ui, "Recording you", rec_time, false);
        cui_voice_wave(ui, "you", you, 200);
        voice_wave_row(ui, "Reply", "Speaking", true);
        cui_voice_wave(ui, "reply", reply, 200, .owner = CUI_VOICE_REPLY);
        snprintf(note_time, sizeof note_time, "0:%02d / 0:12", (int)(vw_progress * 12));
        voice_wave_row(ui, "Voice note", note_time, false);
        cui_voice_wave(ui, "note", clip, 160, .progress = &vw_progress);
    }
}

static char vr_draft[256] = "";
static bool vr_recording = true;
SCENE(voice_recorder) {
    static float mic[240];
    voice_fill(mic, 240, voice_step(ui), 1.1f, 1.7f, 0.85f);
    int n = 120 + voice_step(ui) % 120;   /* the take grows from the right */
    cui_column(ui, .gap = 14, UPTO(560)) {
        static char idle[256] = "";
        cui_chat_composer(ui, "idle", idle, sizeof idle, .placeholder = "Message", .mic = true);
        int ev = cui_chat_composer(ui, "rec", vr_draft, sizeof vr_draft, .mic = true, .recording = vr_recording,
                                   .levels = mic + 240 - n, .level_count = n, .record_seconds = n * VOICE_STEP);
        if (ev & CUI_COMPOSER_RECORD) vr_recording = true;
        if (ev & (CUI_COMPOSER_CANCEL | CUI_COMPOSER_VOICE)) vr_recording = false;
    }
}

static cui_voice_note vn_notes[2] = { { NULL, 0, 4.8f, 12, 1.5f, true }, { NULL, 0, 0, 8, 1, false } };
SCENE(voice_note) {
    static float a[90], b[64];
    voice_clip(a, 90, 0.4f);
    voice_clip(b, 64, 2.9f);
    vn_notes[0].levels = a; vn_notes[0].count = 90;
    vn_notes[1].levels = b; vn_notes[1].count = 64;
    cui_column(ui, .gap = 10, UPTO(560)) {
        cui_voice_note(ui, "theirs", &vn_notes[0]);
        cui_voice_note(ui, "mine", &vn_notes[1], .mine = true);
    }
}

/* a spoken conversation: you ask, the assistant thinks (no sound) and answers */
typedef struct { cui_voice_owner who; float seconds; bool thinking; const char *text; } voice_turn;
static const voice_turn vs_script[] = {
    { CUI_VOICE_YOU, 3.2f, false, "What's the weather like tomorrow morning?" }, { CUI_VOICE_REPLY, 1.4f, true, NULL },
    { CUI_VOICE_REPLY, 4.2f, false, "Clear and cool, about 9 degrees at eight, warming to 15 by noon." },
    { CUI_VOICE_YOU, 2.4f, false, "Should I bring a jacket?" }, { CUI_VOICE_REPLY, 1.1f, true, NULL },
    { CUI_VOICE_REPLY, 3.0f, false, "A light one, yes. It's breezy until around ten." },
};
/* the turn at t seconds into the script (looping), and how far into it */
static const voice_turn *vs_turn_at(float t, float *into) {
    float total = 0;
    for (int k = 0; k < 6; k++) total += vs_script[k].seconds;
    t = fmodf(t, total);
    for (int k = 0; k < 6; k++) { if (t < vs_script[k].seconds) { *into = t; return &vs_script[k]; } t -= vs_script[k].seconds; }
    *into = 0;
    return &vs_script[0];
}
static bool vs_muted = false;
SCENE(voice_session) {
    static float lv[220];
    static uint8_t own[220];
    float now = 5.9f + voice_step(ui) * VOICE_STEP;
    int n = 0;
    for (int k = 219; k >= 0; k--) {   /* oldest first, starting with the conversation */
        float t = now - k * VOICE_STEP, into;
        if (t < 0) continue;
        const voice_turn *tu = vs_turn_at(t, &into);
        own[n] = (uint8_t)tu->who;
        lv[n++] = tu->thinking ? CUI_VOICE_SILENT : tu->who == CUI_VOICE_YOU ? (vs_muted ? 0 : 0.8f * voice_level(t * 1.3f, 2)) : voice_level(t * 1.4f, 5);
    }
    float into;
    const voice_turn *cur = vs_turn_at(now, &into);
    const voice_turn *said = cur->thinking ? cur - 1 : cur;
    cui_voice_state state = cur->thinking ? CUI_VOICE_THINKING : cur->who == CUI_VOICE_REPLY ? CUI_VOICE_SPEAKING : vs_muted ? CUI_VOICE_MUTED : CUI_VOICE_LISTENING;
    float reveal = into / cur->seconds * 1.15f;
    int len = (int)strlen(said->text), k = cur->thinking || reveal >= 1 ? len : (int)(len * reveal);
    static char spoken[128];
    snprintf(spoken, sizeof spoken, "%.*s", k, said->text);
    cui_column(ui, UPTO(600)) {
        cui_voice_session(ui, "call", state, lv, n, .owners = own, .transcript = spoken, .upcoming = said->text + k,
                          .speaker = said->who, .muted = &vs_muted);
    }
}

SCENE(suggestions) {
    static const cui_prompt_suggestion sugg[] = {
        { CUI_ICON_CODE, "Explain this code", "Walk through what it does" },
        { CUI_ICON_CHART_BAR, "Analyze a dataset", "Find trends and outliers" },
        { CUI_ICON_PENCIL, "Draft an email", "Clear and friendly" },
        { CUI_ICON_BUG, "Debug an error", "Paste the stack trace" },
    };
    static int vote = 1;
    static const cui_viz_item parts[] = { { "System", 3200 }, { "Files", 41800 }, { "Messages", 68400 } };
    cui_prompt_suggestions(ui, "sugg", sugg, 4);
    cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
        cui_response_actions(ui, "actions", .copy_text = "x", .vote = &vote);
        cui_spacer(ui);
        cui_context_meter(ui, "ctx", 113400, 200000, .parts = parts, .part_count = 3);
    }
}

/* A mosaic of real components in cards, for the README. One product, a
 * build service called Kiln, so the cards read as parts of one screen. */
static void mosaic_signup(cui_ctx *ui) {
    static char name[64] = "linux-arm64-02", host[64] = "10.0.4.17", token[48] = "kiln_rt_7f3a9c21e0b84d55";
    static int os = 0;
    cui_card(ui, .gap = 16, .width = CUI_FILL) {
        cui_column(ui, .gap = 4) {
            cui_text(ui, "Add a runner", .font = CUI_FONT_DISPLAY, .size = 20);
            cui_text(ui, "Runners pick up jobs for the platforms you choose.", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        static const char *oses[] = { "Linux", "macOS", "Windows" };
        cui_segmented(ui, "m-os", oses, 3, &os, .full_width = true);
        cui_input(ui, "m-name", name, sizeof name, .label = "Name");
        cui_input(ui, "m-host", host, sizeof host, .label = "Address", .prefix = "ssh://", .suffix = ":22");
        cui_input(ui, "m-token", token, sizeof token, .label = "Registration token", .password = true, .hint = "Shown once. Store it with the runner.");
        cui_button(ui, "Add runner", .variant = CUI_BUTTON_PRIMARY, .full_width = true);
    }
}
static void mosaic_notifications(cui_ctx *ui) {
    static bool a = true, b = false, c = true;
    cui_card(ui, .gap = 16, .width = CUI_FILL) {
        cui_label(ui, "Email me when");
        const char *titles[] = { "A build fails", "A build recovers", "The nightly report is ready" };
        const char *descs[] = { "Only for branches you pushed to", "After at least one failure", "Every weekday at 07:00" };
        bool *vals[] = { &a, &b, &c };
        for (int i = 0; i < 3; i++) {
            cui_push_id_int(ui, i);
            cui_row(ui, .width = CUI_FILL, .gap = 12, .align = CUI_ALIGN_CENTER) {
                cui_column(ui, .gap = 1, .width = CUI_FILL) {
                    cui_text(ui, titles[i], .font = CUI_FONT_MEDIUM, .size = 13);
                    cui_text(ui, descs[i], .size = 12, .color = CUI_COLOR_TEXT_MUTED);
                }
                cui_switch(ui, "##n", vals[i]);
            }
            cui_pop_id(ui);
        }
    }
}
static void mosaic_metrics(cui_ctx *ui) {
    static const float s1[] = { 268, 255, 262, 249, 251, 240, 236, 229, 231, 222 };
    static const float s2[] = { 3, 4, 3, 5, 4, 6, 5, 6, 7, 7 };
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 20, .width = CUI_FILL) {
        cui_metric(ui, "Median build", "3m 42s", .delta = -17.6f, .delta_label = "-17.6%", .lower_is_better = true, .spark = s1, .spark_count = 10, .icon = CUI_ICON_CLOCK);
        cui_metric(ui, "Flaky tests", "7", .delta = 2, .delta_label = "+2 this week", .lower_is_better = true, .spark = s2, .spark_count = 10, .icon = CUI_ICON_TRIANGLE_ALERT);
    }
}
static void mosaic_calendar(cui_ctx *ui) {
    static cui_date d = { 2026, 11, 17 };
    static const char *slots[] = { "09:00", "11:30", "14:00", "16:30" };
    static int slot = 1;
    cui_card(ui, .dir = CUI_ROW, .gap = 20, .width = CUI_FILL, .wrap = true) {
        cui_calendar(ui, "m-cal", &d);
        cui_column(ui, .gap = 10, .width = CUI_FILL) {
            cui_label(ui, "Release 2.4");
            cui_text(ui, "Pick a window while the on-call team is in.", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
            cui_box(ui, .height = CUI_PX(4)) {}
            for (int i = 0; i < 4; i++) {
                cui_push_id_int(ui, i);
                if (cui_button(ui, slots[i], .variant = slot == i ? CUI_BUTTON_PRIMARY : CUI_BUTTON_OUTLINE, .full_width = true)) slot = i;
                cui_pop_id(ui);
            }
        }
    }
}
static void mosaic_chart(cui_ctx *ui) {
    static const char *days[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const float a[] = { 412, 488, 365, 530, 471, 96, 74 };
    static const float b[] = { 236, 301, 254, 322, 289, 41, 38 };
    cui_card(ui, .gap = 14, .width = CUI_FILL) {
        cui_column(ui, .gap = 2, .width = CUI_FILL) {
            cui_label(ui, "Build minutes");
            cui_text(ui, "Linux and Windows runners, last 7 days", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_series series[2] = { { "Linux", a, 7, { 0 } }, { "Windows", b, 7, { 0 } } };
        cui_bar_chart(ui, "m-bars", series, 2, .labels = days, .label_count = 7, .height = 190);
    }
}
static void mosaic_team(cui_ctx *ui) {
    static const char *names[] = { "Jonas Berg", "Priya Nair", "Tomas Rivera" };
    static const char *mails[] = { "jonas@northlight.io", "priya@harbor.dev", "tomas@kiln.app" };
    static const char *roles[] = { "Required", "Required", "Optional" };
    cui_card(ui, .gap = 16, .width = CUI_FILL) {
        cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
            cui_box(ui, .width = CUI_FILL) { cui_label(ui, "Reviewers"); }
            cui_button(ui, "Add", .icon = CUI_ICON_USER_PLUS, .size = CUI_SIZE_SM);
        }
        for (int i = 0; i < 3; i++) {
            cui_push_id_int(ui, i);
            cui_row(ui, .width = CUI_FILL, .gap = 12, .align = CUI_ALIGN_CENTER) {
                cui_avatar(ui, names[i], .size = 36, .status = i == 0 ? "online" : i == 1 ? "away" : NULL);
                cui_column(ui, .gap = 1, .width = CUI_FILL) {
                    cui_text(ui, names[i], .font = CUI_FONT_MEDIUM, .size = 13, .nowrap = true);
                    cui_text(ui, mails[i], .size = 12, .color = CUI_COLOR_TEXT_MUTED, .nowrap = true);
                }
                cui_button(ui, roles[i], .size = CUI_SIZE_SM, .variant = CUI_BUTTON_OUTLINE, .icon_right = CUI_ICON_CHEVRON_DOWN);
            }
            cui_pop_id(ui);
        }
    }
}
static void mosaic_share(cui_ctx *ui) {
    static char link[96] = "https://kiln.app/r/2.4.0-rc.2";
    static int access = 1;
    cui_card(ui, .gap = 14, .width = CUI_FILL) {
        cui_column(ui, .gap = 2) {
            cui_label(ui, "Release candidate");
            cui_text(ui, "Builds for all three platforms, signed.", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_row(ui, .width = CUI_FILL, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_input(ui, "m-link", link, sizeof link, .read_only = true, .width = CUI_FILL);
            cui_copy_button(ui, "m-copy", link);
        }
        const char *opts[] = { "Public", "Team only" };
        cui_segmented(ui, "m-access", opts, 2, &access, .full_width = true);
    }
}
static void mosaic_progress(cui_ctx *ui) {
    static float goal = 80;
    cui_card(ui, .gap = 16, .width = CUI_FILL) {
        cui_row(ui, .width = CUI_FILL, .gap = 18, .align = CUI_ALIGN_CENTER) {
            cui_progress_ring(ui, "##m-ring", 0.763f, 84);
            cui_column(ui, .gap = 4, .width = CUI_FILL) {
                cui_label(ui, "Line coverage");
                cui_text(ui, "76.3% on main, 3,912 of 5,127 lines.", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
            }
        }
        cui_slider(ui, "Fail below", &goal, 50, 100, .step = 1, .show_value = true, .format = "%.0f%%");
    }
}
static void mosaic_storage(cui_ctx *ui) {
    cui_card(ui, .gap = 12, .width = CUI_FILL) {
        cui_row(ui, .width = CUI_FILL, .gap = 10, .align = CUI_ALIGN_CENTER) {
            cui_icon(ui, CUI_ICON_DATABASE, 16, CUI_COLOR_TEXT_SECONDARY);
            cui_text(ui, "Build cache", .font = CUI_FONT_MEDIUM, .width = CUI_FILL);
            cui_button(ui, "Clear", .size = CUI_SIZE_SM, .variant = CUI_BUTTON_GHOST);
        }
        cui_meter(ui, "Used", 68.4f, 100, "GB");
    }
}
static void mosaic_topics(cui_ctx *ui) {
    static bool sel[6] = { true, false, true, true, false, false };
    static const char *names[] = { "x86-64", "ARM64", "Debug", "Release", "ASan", "MinGW" };
    cui_card(ui, .gap = 12, .width = CUI_FILL) {
        cui_label(ui, "Build matrix");
        for (int r = 0; r < 2; r++)
            cui_row(ui, .gap = 8, .wrap = true) {
                for (int i = r * 3; i < r * 3 + 3; i++) { cui_push_id_int(ui, i); cui_chip(ui, names[i], &sel[i]); cui_pop_id(ui); }
            }
    }
}
SCENE(mosaic) {
    cui_row(ui, .width = CUI_FILL, .gap = 20, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { mosaic_signup(ui); mosaic_notifications(ui); mosaic_storage(ui); }
        cui_column(ui, .width = CUI_GROW(1.25f), .min_width = 300, .gap = 20) { mosaic_metrics(ui); mosaic_calendar(ui); mosaic_chart(ui); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { mosaic_team(ui); mosaic_share(ui); mosaic_progress(ui); mosaic_topics(ui); }
    }
}

/* A mosaic of the extended components, for the README. */
static void pro_assistant(cui_ctx *ui) {
    static char draft[128] = "Compare it with last quarter";
    static int model = 0, vote = 1;
    cui_card(ui, .gap = 12, .width = CUI_FILL) {
        cui_chat_message(ui, "Why does the new build feel so much faster?", .mine = true);
        cui_reasoning(ui, "think", .seconds = 6) { cui_text(ui, "hidden"); }
        cui_tool_call(ui, "search", "search_commits", .icon = CUI_ICON_GIT_BRANCH, .detail = "last 30 days", .status = CUI_TASK_DONE, .seconds = 0.8f);
        cui_text(ui, "Most of the gain comes from caching glyphs in an atlas and drawing text in one batch, so a frame now takes 0.24 ms, down from about 1 ms.", .size = 14);
        cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER) {
            static const cui_source a = { "Glyph atlas", "https://github.com/cui/pull/214", "Cache rasterized glyphs" };
            static const cui_source b = { "Batching", "https://docs.cui.dev/rendering", "One draw call per texture" };
            cui_citation(ui, 1, &a);
            cui_citation(ui, 2, &b);
            cui_spacer(ui);
            cui_response_actions(ui, "act", .copy_text = "x", .vote = &vote, .no_share = true);
        }
        cui_chat_composer(ui, "composer", draft, sizeof draft, .models = shot_models, .model_count = 3, .model = &model);
    }
}
static void pro_board(cui_ctx *ui) {
    static const char *titles[] = { "Keyboard navigation in the date range picker", "Spring presets for menus" };
    static const char *tags[] = { "A11y", "Motion" };
    static const cui_tone tones[] = { CUI_TONE_INFO, CUI_TONE_VIOLET };
    static const char *who[] = { "Priya Nair", "Rosa Delgado" };
    cui_kanban_begin(ui, "pro-board");
    cui_kanban_column_begin(ui, "In progress", 2, .dot = CUI_RGB(0x3b82f6), .limit = 3, .width = 460);
    for (int k = 0; k < 2; k++) {
        cui_kanban_card_begin(ui, k);
        cui_badge(ui, tags[k], .tone = tones[k], .size = CUI_SIZE_SM);
        cui_text(ui, titles[k], .font = CUI_FONT_MEDIUM, .size = 13.5f);
        if (k == 0) cui_progress(ui, 0.6f, .height = 4);
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
            cui_avatar(ui, who[k], .size = 22);
            cui_spacer(ui);
            cui_icon(ui, CUI_ICON_MESSAGE_SQUARE, 12, CUI_COLOR_TEXT_MUTED);
            cui_text(ui, k ? "2" : "5", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
        }
        cui_kanban_card_end(ui);
    }
    cui_kanban_column_end(ui);
    cui_kanban_end(ui, NULL);
}
static void pro_activity(cui_ctx *ui) {
    viz_data();
    cui_card(ui, .gap = 12, .width = CUI_FILL) {
        cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
            cui_column(ui, .gap = 2, .width = CUI_FILL) {
                cui_text(ui, "Contributions", .font = CUI_FONT_MEDIUM, .size = 15);
                cui_text(ui, "The last six months", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
            }
            cui_counter(ui, "total", 642, .size = 26, .font = CUI_FONT_DISPLAY);
        }
        cui_date start = { 2026, 5, 11 };
        cui_calendar_heatmap(ui, "half", start, viz_days + 180, 182, .cell = 11);
    }
}
static void pro_flow(cui_ctx *ui) {
    static const char *nodes[] = { "Linux", "Windows", "macOS", "Test runs", "Passed", "Retried" };
    static const cui_sankey_link links[] = { { 0, 3, 712 }, { 1, 3, 408 }, { 2, 3, 231 }, { 3, 4, 1263 }, { 3, 5, 88 } };
    cui_card(ui, .gap = 12, .width = CUI_FILL) {
        cui_text(ui, "Test runs this week", .font = CUI_FONT_MEDIUM, .size = 15);
        cui_sankey(ui, "flow", nodes, 6, links, (int)(sizeof links / sizeof links[0]), .height = 190);
    }
}
static void pro_player(cui_ctx *ui) {
    static cui_now_playing np = { "Low Tide", "Marlow Avenue", 0, { 0x2d, 0x30, 0x47, 255 }, 96, 222, true };
    cui_now_playing(ui, "np", &np, .width = CUI_FILL);
}
static void pro_plan(cui_ctx *ui) {
    static const cui_plan_step steps[] = {
        { "Profile the frame", "0.24 ms", CUI_TASK_DONE },
        { "Cache glyphs in an atlas", NULL, CUI_TASK_DONE },
        { "Batch text draws", "Merging 4 passes", CUI_TASK_RUNNING },
        { "Measure again", NULL, CUI_TASK_PENDING },
    };
    cui_agent_plan(ui, "plan", steps, 4, .title = "Speed up rendering");
}
static void pro_tiles(cui_ctx *ui) {
    static bool wifi = true, focus = false, dark = true, bt = true;
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 10, .width = CUI_FILL) {
        cui_control_tile(ui, "Wi-Fi", &wifi, .icon = CUI_ICON_WIFI, .detail = "Studio");
        cui_control_tile(ui, "Focus", &focus, .icon = CUI_ICON_MOON);
        cui_control_tile(ui, "Hotspot", &bt, .icon = CUI_ICON_RADIO, .detail = "2 devices");
        cui_control_tile(ui, "Dark mode", &dark, .icon = CUI_ICON_SUN);
    }
}
static void pro_radar(cui_ctx *ui) {
    static const float a[] = { 92, 70, 88, 64, 80 }, b[] = { 70, 86, 66, 82, 62 };
    static const char *axes[] = { "Speed", "Size", "Polish", "Docs", "Tests" };
    cui_card(ui, .gap = 6, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
        cui_series rs[] = { { "2.0", a, 5, { 0 } }, { "1.0", b, 5, { 0 } } };
        cui_radar_chart(ui, "radar", rs, 2, axes, 5, .size = 170, .max = 100);
    }
}
SCENE(mosaic_pro) {
    cui_row(ui, .width = CUI_FILL, .gap = 20, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(1.15f), .min_width = 300, .gap = 20) { pro_assistant(ui); pro_board(ui); }
        cui_column(ui, .width = CUI_GROW(1.2f), .min_width = 300, .gap = 20) { pro_activity(ui); pro_flow(ui); pro_player(ui); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { pro_plan(ui); pro_tiles(ui); pro_radar(ui); }
    }
}

/* ---- README heroes: components on a plain stage ----
 * A flat surface one step off the page color, like a desk under a window.
 * No glows, gradients or dot grids: the components are the picture. */
typedef struct { int unused; } stage_look;

static void paint_stage(cui_ctx *ui, cui_rect r, void *user) {
    (void)user;
    bool dark = cui_get_theme_mode(ui) == CUI_THEME_DARK;
    cui_color bg = cui_token_color(ui, CUI_TOKEN_BACKGROUND), fg = cui_token_color(ui, CUI_TOKEN_FOREGROUND);
    cui_draw_rect(ui, r, CUI_CORNERS(0), cui_mix(bg, fg, dark ? 0.0f : 0.045f));
}

static void stage(cui_ctx *ui, const stage_look *look, void (*body)(cui_ctx *ui)) {
    cui_box(ui, .dir = CUI_STACK, .width = CUI_FILL, .height = CUI_FILL) {
        cui_canvas(ui, "stage", CUI_FILL, CUI_FILL, paint_stage, look, sizeof *look);
        cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = cui_viewport_size(ui).x < 600 ? 16 : 40, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) { body(ui); }
    }
}

/* a titled panel up to `w` wide: it rests on the stage, so an edge and no shadow */
#define CUI_MIN_W(w) ((w) < 300 ? (w) : 300)
static void window_begin(cui_ctx *ui, const char *title, float w) {
    cui_box_begin(ui, &(cui_box){ .width = CUI_FILL, .min_width = CUI_MIN_W(w), .max_width = w, .radius = 18, .bg = CUI_COLOR_SURFACE, .border = 1, .border_color = CUI_COLOR_BORDER,
                                  .clip = true });
    cui_row(ui, .width = CUI_FILL, .pad_x = 20, .pad_y = 14, .align = CUI_ALIGN_CENTER) {
        cui_text(ui, title, .size = 13.5f, .font = CUI_FONT_MEDIUM, .width = CUI_FILL);
    }
    cui_separator(ui);
}
static void window_end(cui_ctx *ui) { cui_box_end(ui); }

static const stage_look look_plain = { 0 };

static void hero_board_body(cui_ctx *ui) { cui_box(ui, UPTO(860)) { body_kanban(ui); } }
static void scene_hero_board(cui_ctx *ui, int f) {
    if (f < 0) { scene_kanban(ui, f); return; }
    stage(ui, &look_plain, hero_board_body);
}

static void hero_ai_body(cui_ctx *ui) {
    static char draft[128] = "Compare it with last quarter";
    static int model = 0, vote = 1;
    static const cui_source srcs[] = {
        { "Glyph atlas for text", "https://github.com/cui/pull/214", "Rasterize each glyph once and reuse it from a texture." },
        { "Batching draw calls", "https://docs.cui.dev/rendering", "One draw call per texture or clip change." },
    };
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
        window_begin(ui, "Muse", 620);
        cui_column(ui, .width = CUI_FILL, .pad = 22, .gap = 12) {
            cui_chat_message(ui, "Why does the new build feel so much faster?", .mine = true);
            cui_reasoning(ui, "think", .thinking = true, .seconds = 4) {
                cui_text(ui, "Compare frame timings before and after, then look at what changed in text rendering.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
            }
            cui_tool_call(ui, "search", "search_commits", .icon = CUI_ICON_GIT_BRANCH, .detail = "rendering, last 30 days", .status = CUI_TASK_DONE, .seconds = 0.8f);
            cui_tool_call(ui, "bench", "run_benchmark", .icon = CUI_ICON_ACTIVITY, .detail = "gallery, 600 frames", .status = CUI_TASK_RUNNING, .seconds = 2.4f);
            cui_text(ui, "Most of it comes from caching glyphs in an atlas and drawing all text in one batch: a frame now takes 0.24 ms, down from about 1 ms.", .size = 14.5f);
            cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_citation(ui, 1, &srcs[0]);
                cui_citation(ui, 2, &srcs[1]);
                cui_spacer(ui);
                cui_response_actions(ui, "act", .copy_text = "x", .vote = &vote, .no_share = true);
            }
            cui_chat_composer(ui, "composer", draft, sizeof draft, .models = shot_models, .model_count = 3, .model = &model);
        }
        window_end(ui);
        cui_column(ui, .gap = 16, FLEX(260, 300)) {
            static const cui_plan_step steps[] = {
                { "Profile the frame", "0.24 ms", CUI_TASK_DONE },
                { "Cache glyphs in an atlas", NULL, CUI_TASK_DONE },
                { "Run the benchmark", "600 frames", CUI_TASK_RUNNING },
                { "Write it up", NULL, CUI_TASK_PENDING },
            };
            cui_box(ui, .width = CUI_FILL) { cui_agent_plan(ui, "plan", steps, 4, .title = "Speed up rendering"); }
            /* the evidence the answer rests on, as a chart rather than a claim */
            static const float ft[] = { 1.02f, 0.98f, 1.05f, 0.97f, 0.61f, 0.44f, 0.31f, 0.27f, 0.25f, 0.24f };
            static const char *builds[] = { "1.0", "", "", "", "1.1", "", "", "", "", "1.2" };
            cui_card(ui, .gap = 10, .pad = 18, .width = CUI_FILL) {
                cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_END) {
                    cui_box(ui, .width = CUI_FILL) { cui_label(ui, "Frame time"); }
                    cui_text(ui, "0.24 ms", .font = CUI_FONT_MEDIUM, .size = 13, .tabular = true);
                }
                cui_series fs = { "Frame time (ms)", ft, 10, { 0 } };
                cui_line_chart(ui, "ft", &fs, 1, .labels = builds, .label_count = 10, .height = 120, .area = true, .no_legend = true, .format = "%.1f", .min = 0, .max = 1.2f);
                cui_text(ui, "Milliseconds per frame, by release", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
            }
        }
    }
}
static void scene_hero_ai(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_ai_body);
}

static void hero_desk_body(cui_ctx *ui) {
    static bool wifi = true, focus = true, bt = false, dark = true;
    static cui_now_playing np = { "Low Tide", "Marlow Avenue", 0, { 0xc8, 0x55, 0x3d, 255 }, 96, 222, true };
    static float peaks[64], progress = 0.42f;
    for (int k = 0; k < 64; k++) peaks[k] = 0.2f + 0.78f * fabsf(sinf(k * 0.41f) * sinf(k * 0.13f + 1.1f));
    cui_column(ui, .gap = 64, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
        cui_row(ui, .gap = 20, .align = CUI_ALIGN_START, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
            cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 10, FLEX(260, 300)) {
                cui_control_tile(ui, "Wi-Fi", &wifi, .icon = CUI_ICON_WIFI, .detail = "Studio");
                cui_control_tile(ui, "Focus", &focus, .icon = CUI_ICON_MOON, .detail = "Until 6 pm");
                cui_control_tile(ui, "Hotspot", &bt, .icon = CUI_ICON_RADIO);
                cui_control_tile(ui, "Dark mode", &dark, .icon = CUI_ICON_SUN);
            }
            cui_column(ui, .gap = 12, FLEX(280, 360)) {
                cui_now_playing(ui, "np", &np, .width = CUI_FILL);
                cui_card(ui, .pad = 14, .gap = 8, .width = CUI_FILL) {
                    cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                        cui_icon_button(ui, CUI_ICON_PLAY, .size = CUI_SIZE_SM, .variant = CUI_BUTTON_SECONDARY);
                        cui_box(ui, .width = CUI_FILL) { cui_waveform(ui, "wave", peaks, 64, &progress, .height = 32); }
                        cui_text(ui, "0:42", .size = 12, .tabular = true, .color = CUI_COLOR_TEXT_MUTED);
                    }
                }
            }
        }
        body_dock(ui);
    }
}
static void scene_hero_desk(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "dock"));
        if (-f - 1 == 15) cui_input_mouse_move(ui, r.x + r.w * 0.47f, r.y + r.h * 0.5f);
        return;
    }
    stage(ui, &look_plain, hero_desk_body);
}

static void hero_pick_body(cui_ctx *ui) {
    cui_row(ui, .gap = 28, .align = CUI_ALIGN_START, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
        cui_column(ui, .gap = 10, FLEX(300, 730)) { body_range(ui); cui_gap(ui, cui_viewport_size(ui).x < 760 ? 480 : 330); }   /* room for the open popover */
        cui_card(ui, .pad = 18, .gap = 12) {
            cui_text(ui, "Wake up", .font = CUI_FONT_MEDIUM, .size = 15);
            static int h = 6, m = 40;
            static const char *hours[24], *mins[60];
            static char hb[24][4], mb[60][4];
            for (int k = 0; k < 24; k++) { snprintf(hb[k], 4, "%02d", k); hours[k] = hb[k]; }
            for (int k = 0; k < 60; k++) { snprintf(mb[k], 4, "%02d", k); mins[k] = mb[k]; }
            cui_row(ui, .gap = 0) {
                cui_wheel_picker(ui, "h", hours, 24, &h, .width = CUI_PX(76), .loop = true, .join_right = true);
                cui_wheel_picker(ui, "m", mins, 60, &m, .width = CUI_PX(76), .loop = true, .join_left = true);
            }
            static bool on = true;
            cui_switch(ui, "Repeat on weekdays", &on);
        }
    }
}
static void scene_hero_pick(cui_ctx *ui, int f) {
    if (f < 0) { scene_range(ui, f); return; }
    stage(ui, &look_plain, hero_pick_body);
}

static void hero_palette_body(cui_ctx *ui) {
    cui_box(ui, .width = CUI_PX(10), .height = CUI_PX(10)) {}
}
static void scene_hero_palette(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_palette_body);
    static bool open = true;
    static const cui_command cmds[] = {
        { "New project", CUI_ICON_PLUS, "Actions", "Ctrl N" }, { "Invite teammate", CUI_ICON_USER_PLUS, "Actions", NULL },
        { "Deploy to production", CUI_ICON_ROCKET, "Actions", "Ctrl D" },
        { "Toggle dark mode", CUI_ICON_MOON, "Preferences", "Ctrl J" }, { "Open settings", CUI_ICON_SETTINGS, "Preferences", "Ctrl ," },
    };
    cui_command_palette(ui, &open, cmds, 5);
}

static void hero_plan_body(cui_ctx *ui) {
    window_begin(ui, "Roadmap", 1000);
    cui_column(ui, .width = CUI_FILL, .pad = 18) { body_gantt(ui); }
    window_end(ui);
}
static void scene_hero_plan(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_plan_body);
}

static void hero_charts_body(cui_ctx *ui) {
    cui_row(ui, .width = CUI_FILL, .gap = 20, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(1.2f), .min_width = 300, .gap = 20) { pro_activity(ui); pro_flow(ui); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { pro_radar(ui); pro_plan(ui); }
    }
}
static void scene_hero_charts(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_charts_body);
}

static void hero_social_body(cui_ctx *ui) {
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .justify = CUI_JUSTIFY_CENTER, .width = CUI_FILL, .wrap = true) {
        window_begin(ui, "Messages", 520);
        cui_column(ui, .width = CUI_FILL, .pad = 20, .gap = 0) {
            cui_chat_divider(ui, "Today");
            cui_chat_message(ui, "Did the data grid make it into 1.1?", .author = "Jonas Berg", .time = "9:41", .last_in_group = false);
            cui_chat_message(ui, "I need it for the billing page.", .author = "Jonas Berg", .grouped = true, .last_in_group = true);
            cui_chat_message(ui, "It did. 2,000 rows scroll at 60 fps on my laptop.", .mine = true);
            cui_chat_message(ui, "Shift and Ctrl clicks select like a spreadsheet.", .mine = true, .grouped = true, .last_in_group = true, .status = "Read", .time = "9:43");
            cui_typing_indicator(ui, "Jonas");
        }
        window_end(ui);
        cui_column(ui, FLEX(280, 420), .gap = 18) {
            cui_box(ui, .width = CUI_FILL) { body_comments(ui); }
            static const cui_presence_user who[] = {
                { "Maya Okafor", { 0x7c, 0x5c, 0xff, 255 }, 0, false }, { "Jonas Berg", { 0x22, 0xc5, 0x8b, 255 }, 0, false },
                { "Priya Nair", { 0xf5, 0x9e, 0x0b, 255 }, 0, false }, { "Lena Fischer", { 0x38, 0xbd, 0xf8, 255 }, 0, true },
                { "Aiko Tanaka", { 0xec, 0x48, 0x99, 255 }, 0, false }, { "Daniel Mensah", { 0x10, 0xb9, 0x81, 255 }, 0, false },
            };
            cui_card(ui, .pad = 18, .gap = 10, .width = CUI_FILL) {
                cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
                    cui_column(ui, .gap = 2, .width = CUI_FILL) {
                        cui_text(ui, "Revenue today", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
                        cui_counter(ui, "rev", 48210, .format = "$%.0f", .size = 32, .group_thousands = true, .font = CUI_FONT_DISPLAY);
                    }
                    cui_presence(ui, "here", who, 6, 4, 30);
                }
            }
        }
    }
}
static void scene_hero_social(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_social_body);
}
static void hero_data_body(cui_ctx *ui) {
    window_begin(ui, "People", 860);
    cui_column(ui, .width = CUI_FILL, .pad = 18, .gap = 14) { body_data_grid(ui); }
    window_end(ui);
}
static void scene_hero_data(cui_ctx *ui, int f) {
    if (f < 0) return;
    stage(ui, &look_plain, hero_data_body);
}

/* Spotlight cards used for a real choice, not a pricing template. */
static void renderer_card(cui_ctx *ui, const char *id, const char *name, const char *blurb, const char *const *facts, int nf,
                          const char *action, bool current) {
    cui_spotlight(ui, id, .width = CUI_PX(270), .pad = 22, .gap = 14) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
            cui_text(ui, name, .font = CUI_FONT_MEDIUM, .size = 15, .width = CUI_FILL);
            if (current) cui_text(ui, "In use", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
        }
        cui_text(ui, blurb, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
        cui_separator(ui);
        for (int k = 0; k < nf; k++) cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_icon(ui, CUI_ICON_CHECK, 16, CUI_COLOR_TEXT_SECONDARY);
            cui_text(ui, facts[k], .size = 13.5f);
        }
        cui_spacer(ui);
        cui_button(ui, action, .variant = current ? CUI_BUTTON_SECONDARY : CUI_BUTTON_OUTLINE, .full_width = true, .disabled = current);
    }
}
static void hero_effects_body(cui_ctx *ui) {
    static const char *f1[] = { "No GPU or driver needed", "Same pixels as OpenGL", "Used for tests and screenshots" };
    static const char *f2[] = { "One draw call per texture", "Multisampling and vsync", "Windows, macOS and Linux" };
    static const char *f3[] = { "Vulkan, Metal or Direct3D", "Vertices, indices, one atlas", "About 200 lines of glue" };
    cui_column(ui, .gap = 22, .align = CUI_ALIGN_START) {
        cui_column(ui, .gap = 6) {
            cui_text(ui, "Choose a renderer", .font = CUI_FONT_DISPLAY, .size = 30);
            cui_text(ui, "All three draw the same frame. Pick by where the app runs.", .size = 15, .color = CUI_COLOR_TEXT_SECONDARY);
        }
        cui_row(ui, .gap = 18, .align = CUI_ALIGN_STRETCH, .wrap = true) {
            renderer_card(ui, "sw", "Software", "Plain C into a pixel buffer. A full HD frame in about 2 ms.", f1, 3, "Use software", false);
            renderer_card(ui, "gl", "OpenGL 3.3", "The default on desktop, with a software fallback.", f2, 3, "Current renderer", true);
            renderer_card(ui, "own", "Your own", "The core only emits triangles, so any GPU API works.", f3, 3, "Read the guide", false);
        }
    }
}
static void scene_hero_effects(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "own"));
        if (-f - 1 == 10) cui_input_mouse_move(ui, r.x + r.w * 0.62f, r.y + r.h * 0.16f);
        return;
    }
    stage(ui, &look_plain, hero_effects_body);
}

static void mosaic_body(cui_ctx *ui) {
    cui_row(ui, .width = CUI_FILL, .gap = 20, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { mosaic_signup(ui); mosaic_notifications(ui); mosaic_storage(ui); }
        cui_column(ui, .width = CUI_GROW(1.25f), .min_width = 300, .gap = 20) { mosaic_metrics(ui); mosaic_calendar(ui); mosaic_chart(ui); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { mosaic_team(ui); mosaic_share(ui); mosaic_progress(ui); mosaic_topics(ui); }
    }
}
static void scene_stage_mosaic(cui_ctx *ui, int f) { if (f < 0) return; stage(ui, &look_plain, mosaic_body); }
static void mosaic_pro_body(cui_ctx *ui) {
    cui_row(ui, .width = CUI_FILL, .gap = 20, .align = CUI_ALIGN_START, .wrap = true) {
        cui_column(ui, .width = CUI_GROW(1.15f), .min_width = 300, .gap = 20) { pro_assistant(ui); pro_board(ui); }
        cui_column(ui, .width = CUI_GROW(1.2f), .min_width = 300, .gap = 20) { pro_activity(ui); pro_flow(ui); pro_player(ui); }
        cui_column(ui, .width = CUI_GROW(1), .min_width = 300, .gap = 20) { pro_plan(ui); pro_tiles(ui); pro_radar(ui); }
    }
}
static void scene_stage_mosaic_pro(cui_ctx *ui, int f) { if (f < 0) return; stage(ui, &look_plain, mosaic_pro_body); }
/* one small panel of controls, rendered once per accent and put side by side */
static void accent_body(cui_ctx *ui) {
    static bool on = true, chk = true;
    static float v = 64;
    static int seg = 1;
    cui_card(ui, .pad = 18, .gap = 14, .width = CUI_PX(250)) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_button(ui, "Publish", .variant = CUI_BUTTON_PRIMARY);
            cui_badge(ui, "Draft", .tone = CUI_TONE_ACCENT);
        }
        static const char *items[] = { "Day", "Week", "Month" };
        cui_segmented(ui, "seg", items, 3, &seg, .size = CUI_SIZE_SM);
        cui_row(ui, .gap = 16, .align = CUI_ALIGN_CENTER) { cui_switch(ui, "Sync", &on); cui_checkbox(ui, "Notify", &chk); }
        cui_slider(ui, "##vol", &v, 0, 100);
        cui_progress(ui, 0.68f, .height = 6);
    }
}
static void scene_accent(cui_ctx *ui, int f) {
    if (f < 0) return;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = cui_viewport_size(ui).x < 360 ? 14 : 20, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) { accent_body(ui); }
}

/* ---- app chrome: menu bar, toolbar, status bar, document tabs ---- */
static cui_id nested_id(cui_ctx *ui, const char *scope, const char *name) {
    cui_push_id(ui, scope);
    cui_id id = cui_make_id(ui, name);
    cui_pop_id(ui);
    return id;
}
static void move_to(cui_ctx *ui, cui_rect r, float fx, float fy) { cui_input_mouse_move(ui, r.x + r.w * fx, r.y + r.h * fy); }
static bool v_sidebar = true, v_minimap = false, v_wrap = true;
static void chrome_menus(cui_ctx *ui) {
    cui_menu_bar(ui, "menubar") {
        cui_menu_bar_menu(ui, "File") {
            cui_menu_item(ui, "New file", .shortcut = "Ctrl N");
            cui_menu_item(ui, "Open\xe2\x80\xa6", .shortcut = "Ctrl O");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Save", .shortcut = "Ctrl S");
            cui_menu_item(ui, "Save as\xe2\x80\xa6", .shortcut = "Ctrl Shift S");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Close window", .shortcut = "Ctrl Shift W");
        }
        cui_menu_bar_menu(ui, "Edit") {
            cui_menu_item(ui, "Undo", .shortcut = "Ctrl Z");
            cui_menu_item(ui, "Redo", .shortcut = "Ctrl Shift Z", .disabled = true);
            cui_menu_separator(ui);
            cui_menu_item(ui, "Cut", .shortcut = "Ctrl X");
            cui_menu_item(ui, "Copy", .shortcut = "Ctrl C");
            cui_menu_item(ui, "Paste", .shortcut = "Ctrl V");
            cui_menu_separator(ui);
            cui_menu_item(ui, "Find\xe2\x80\xa6", .shortcut = "Ctrl F");
        }
        cui_menu_bar_menu(ui, "View") {
            if (cui_menu_item(ui, "Sidebar", .checked = v_sidebar)) v_sidebar = !v_sidebar;
            if (cui_menu_item(ui, "Minimap", .checked = v_minimap)) v_minimap = !v_minimap;
            if (cui_menu_item(ui, "Word wrap", .checked = v_wrap)) v_wrap = !v_wrap;
            cui_menu_separator(ui);
            cui_menu_item(ui, "Zoom in", .shortcut = "Ctrl =");
            cui_menu_item(ui, "Zoom out", .shortcut = "Ctrl \xe2\x88\x92");
        }
        cui_menu_bar_menu(ui, "Go") {
            cui_menu_item(ui, "Go to file\xe2\x80\xa6", .shortcut = "Ctrl P");
            cui_menu_item(ui, "Go to line\xe2\x80\xa6", .shortcut = "Ctrl G");
        }
        cui_menu_bar_menu(ui, "Help") {
            cui_menu_item(ui, "Keyboard shortcuts", .shortcut = "Ctrl K");
            cui_menu_item(ui, "Release notes");
        }
    }
}
static bool t_bold = true, t_italic = false, t_under = false, t_list = true;
static void chrome_tools(cui_ctx *ui, const char *id, bool border) {
    cui_toolbar(ui, id, .no_border = !border) {
        cui_toolbar_button(ui, CUI_ICON_PLUS, .tooltip = "New file", .shortcut = "Ctrl N");
        cui_toolbar_button(ui, CUI_ICON_FOLDER_OPEN, .tooltip = "Open", .shortcut = "Ctrl O");
        cui_toolbar_button(ui, CUI_ICON_SAVE, .tooltip = "Save", .shortcut = "Ctrl S");
        cui_toolbar_separator(ui);
        cui_toolbar_button(ui, CUI_ICON_UNDO_2, .tooltip = "Undo", .shortcut = "Ctrl Z");
        cui_toolbar_button(ui, CUI_ICON_REDO_2, .tooltip = "Redo", .disabled = true);
        cui_toolbar_separator(ui);
        cui_toolbar_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .shortcut = "Ctrl B", .toggle = &t_bold);
        cui_toolbar_button(ui, CUI_ICON_ITALIC, .tooltip = "Italic", .shortcut = "Ctrl I", .toggle = &t_italic);
        cui_toolbar_button(ui, CUI_ICON_UNDERLINE, .tooltip = "Underline", .shortcut = "Ctrl U", .toggle = &t_under);
        cui_toolbar_separator(ui);
        cui_toolbar_button(ui, CUI_ICON_LIST, .tooltip = "Bulleted list", .toggle = &t_list);
        cui_toolbar_button(ui, CUI_ICON_LINK, .tooltip = "Insert link", .shortcut = "Ctrl K");
        cui_toolbar_button(ui, CUI_ICON_IMAGE, .tooltip = "Insert image");
        cui_toolbar_spacer(ui);
        cui_toolbar_button(ui, CUI_ICON_PLAY, .label = "Preview");
        cui_toolbar_button(ui, CUI_ICON_SHARE_2, .label = "Share");
    }
}
static void chrome_status(cui_ctx *ui) {
    /* the bar has no fill; this box gives it one, rounded to the window */
    cui_box(ui, .width = CUI_FILL, .bg = CUI_COLOR_SURFACE_MUTED, .radius_bl = 13, .radius_br = 13)
    cui_status_bar(ui, "status") {
        cui_status_bar_item(ui, "main", .icon = CUI_ICON_GIT_BRANCH, .tooltip = "Switch branch");
        cui_status_bar_text(ui, "Saved", .icon = CUI_ICON_CHECK, .tone = CUI_TONE_SUCCESS);
        cui_status_bar_progress(ui, 0.62f, .label = "Indexing 62%");
        cui_status_bar_spacer(ui);
        cui_status_bar_item(ui, "Ln 48, Col 17###pos", .tooltip = "Go to line");
        /* clicks change the values, as in an editor; labels that change keep a key */
        static int spaces = 1, enc = 0, lang = 0;
        static const int space_n[] = { 2, 4, 8 };
        static const char *encs[] = { "UTF-8", "UTF-16" }, *langs[] = { "Markdown", "C", "Plain text" };
        char lbl[48];
        snprintf(lbl, sizeof lbl, "Spaces: %d###spaces", space_n[spaces]);
        if (cui_status_bar_item(ui, lbl)) spaces = (spaces + 1) % 3;
        snprintf(lbl, sizeof lbl, "%s###enc", encs[enc]);
        if (cui_status_bar_item(ui, lbl)) enc ^= 1;
        snprintf(lbl, sizeof lbl, "%s###lang", langs[lang]);
        if (cui_status_bar_item(ui, lbl)) lang = (lang + 1) % 3;
        cui_status_bar_item(ui, NULL, .icon = CUI_ICON_BELL, .tooltip = "Notifications");
    }
}
static int i_doc = 1;
static const cui_doc_tab doc_tabs[] = {
    { "README.md", CUI_ICON_FILE_TEXT, "docs/README.md", false },
    { "tile_cache.c", CUI_ICON_CODE, "src/tile_cache.c", true },
    { "tile_cache.h", CUI_ICON_CODE, "src/tile_cache.h", false },
    { "CHANGELOG.md", CUI_ICON_FILE_TEXT, "CHANGELOG.md", true },
    { "render_sw.c", CUI_ICON_CODE, "src/render/render_sw.c", false },
    { "atlas.c", CUI_ICON_CODE, "src/atlas.c", false },
};
/* The live pages keep what you do: the tabs below are copies that the
 * scenes reorder and close as the component reports it. */
static void doc_tabs_apply(cui_doc_tab *t, int *n, int *active, cui_doc_tabs_result r) {
    if (r.move_from >= 0 && r.move_to >= 0 && r.move_from != r.move_to && r.move_from < *n && r.move_to < *n) {
        cui_doc_tab m = t[r.move_from];
        if (r.move_from < r.move_to) memmove(t + r.move_from, t + r.move_from + 1, (size_t)(r.move_to - r.move_from) * sizeof *t);
        else memmove(t + r.move_to + 1, t + r.move_to, (size_t)(r.move_from - r.move_to) * sizeof *t);
        t[r.move_to] = m;
    }
    if (r.close >= 0 && r.close < *n && *n > 1) {
        memmove(t + r.close, t + r.close + 1, (size_t)(*n - r.close - 1) * sizeof *t);
        (*n)--;
        if (*active > r.close || *active >= *n) (*active)--;
        if (*active < 0) *active = 0;
    }
}
static cui_doc_tab docs_live[6], files_live[4];
static int docs_n = -1, files_n = -1;
static void chrome_editor(cui_ctx *ui, int lines) {
    static const char *code[] = {
        "static tile *cache_lookup(tile_cache *c, uint32_t key) {",
        "    uint32_t slot = key & (c->capacity - 1);",
        "    while (c->tiles[slot].key != key) {",
        "        if (!c->tiles[slot].used) return NULL;",
        "        slot = (slot + 1) & (c->capacity - 1);",
        "    }",
        "    return &c->tiles[slot];",
        "}",
    };
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad_x = 16, .pad_y = 14, .gap = 3, .clip = true) {
        for (int i = 0; i < lines && i < 8; i++) {
            static const char *nums[] = { "41", "42", "43", "44", "45", "46", "47", "48" };
            cui_row(ui, .gap = 18) {
                cui_text(ui, nums[i], .font = CUI_FONT_MONO, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .width = CUI_PX(18), .align = CUI_TEXT_RIGHT, .nowrap = true);
                cui_text(ui, code[i], .font = CUI_FONT_MONO, .size = 12, .nowrap = true);
            }
        }
    }
}
#define WINDOW(ui) cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 14, .clip = true)
/* the bars have no fill: the window's chrome gets one, rounded to the window's inner corners */
#define CHROME_TOP(ui) cui_column(ui, .width = CUI_FILL, .bg = CUI_COLOR_SURFACE_MUTED, .radius_tl = 13, .radius_tr = 13)
static void body_menu_bar(cui_ctx *ui) {
    WINDOW(ui) {
        CHROME_TOP(ui) { chrome_menus(ui); }
        chrome_editor(ui, 8);
    }
}
static void scene_menu_bar(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        cui_rect edit = cui_get_rect(ui, nested_id(ui, "menubar", "Edit")), view = cui_get_rect(ui, nested_id(ui, "menubar", "View"));
        if (f == 3) move_to(ui, edit, 0.5f, 0.5f);
        if (f == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f == 14) move_to(ui, view, 0.5f, 0.5f);   /* hovering another menu switches to it */
        if (f == 24) cui_input_mouse_move(ui, view.x + 60, view.y + view.h + 6 + 6 + 32 * 2.5f);
        return;
    }
    frame_wrap(ui, body_menu_bar);
}
static void body_toolbar(cui_ctx *ui) {
    cui_column(ui, .gap = 18, .width = CUI_FILL, .height = CUI_FILL) {
        cui_box(ui, .width = CUI_FILL, .border = 1, .radius = 14, .clip = true, .bg = CUI_COLOR_SURFACE) { chrome_tools(ui, "wide", false); }
        cui_box(ui, .width = CUI_FILL, .max_width = 300, .border = 1, .radius = 14, .clip = true, .bg = CUI_COLOR_SURFACE) { chrome_tools(ui, "narrow", false); }
    }
}
static void scene_toolbar(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        /* the narrow toolbar's overflow menu, open */
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "narrow"));
        if (f == 6) cui_input_mouse_move(ui, r.x + r.w - 6 - 15, r.y + r.h * 0.5f);
        if (f == 7) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 8) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (f == 16) cui_input_mouse_move(ui, r.x + r.w - 90, r.y + r.h + 6 + 6 + 32 * 2.5f + 9);
        return;
    }
    frame_wrap(ui, body_toolbar);
}
static void body_status_bar(cui_ctx *ui) {
    cui_column(ui, .width = CUI_FILL, .border = 1, .radius = 14, .clip = true, .bg = CUI_COLOR_SURFACE) {
        chrome_editor(ui, 2);
        chrome_status(ui);
    }
}
static void scene_status_bar(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        if (f == 10) { cui_rect r = cui_get_rect(ui, nested_id(ui, "status", "###enc")); move_to(ui, r, 0.5f, 0.5f); }
        return;
    }
    frame_wrap(ui, body_status_bar);
}
static void body_document_tabs(cui_ctx *ui) {
    WINDOW(ui) {
        CHROME_TOP(ui) {
            if (docs_n < 0) { memcpy(docs_live, doc_tabs, sizeof docs_live); docs_n = 6; }
            cui_doc_tabs_result r = cui_doc_tabs(ui, "docs", docs_live, docs_n, &i_doc, .shortcuts = true);
            doc_tabs_apply(docs_live, &docs_n, &i_doc, r);
        }
        chrome_editor(ui, 5);
    }
}
static void scene_document_tabs(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        if (f == 50) { cui_rect r = cui_get_rect(ui, nested_id(ui, "docs", "CHANGELOG.md")); move_to(ui, r, 0.55f, 0.5f); }
        return;
    }
    frame_wrap(ui, body_document_tabs);
}
static int i_doc2 = 1;
static void body_app_chrome(cui_ctx *ui) {
    WINDOW(ui) {
        CHROME_TOP(ui) {
            chrome_menus(ui);
            chrome_tools(ui, "tools", true);
            if (files_n < 0) { memcpy(files_live, doc_tabs, sizeof files_live); files_n = 4; }
            doc_tabs_apply(files_live, &files_n, &i_doc2, cui_doc_tabs(ui, "files", files_live, files_n, &i_doc2));
        }
        chrome_editor(ui, 8);
        chrome_status(ui);
    }
}
static void scene_app_chrome(cui_ctx *ui, int f) {
    if (f < 0) return;
    frame_wrap(ui, body_app_chrome);
}
/* ---- docking: an engine style workspace; everything you do on the live page sticks ---- */
static const char *const dk_layout = "dock1 h0.22([*Scene tree,Assets],h0.6(v0.62([*Viewport],[*Log]),[*Inspector]))";
/* on a phone the app picks a stacked layout, one column of panels, rather than three cramped ones */
static const char *const dk_layout_narrow = "dock1 v0.56([*Viewport,Scene tree,Assets],[*Inspector,Log])";
static const char *const dk_titles[5] = { "Scene tree", "Assets", "Viewport", "Log", "Inspector" };
static const char *const dk_icons[5] = { CUI_ICON_LAYERS, CUI_ICON_FOLDER, CUI_ICON_BOX, CUI_ICON_SQUARE_TERMINAL, CUI_ICON_SLIDERS_HORIZONTAL };
static bool dk_open[5] = { true, true, true, true, true };
static cui_rect dk_tab[5], dk_view;
static int dk_pick = 3;   /* the selected object in the scene tree */
static void dk_paint_viewport(cui_ctx *ui, cui_rect r, void *user) {
    (void)user;
    cui_draw_rect(ui, r, CUI_CORNERS(0), cui_color_of(ui, CUI_COLOR_SURFACE_MUTED));
    /* a ground plane in perspective and one crate, outlined in the accent because it is selected */
    float hx = r.x + r.w * 0.5f, hy = r.y + r.h * 0.38f;
    cui_color line = cui_color_of(ui, CUI_COLOR_BORDER);
    for (int k = -8; k <= 8; k++) cui_draw_line(ui, (cui_vec2){ hx + k * 14.0f, hy }, (cui_vec2){ hx + k * 120.0f, r.y + r.h }, 1, line);
    for (int k = 1; k <= 6; k++) {
        float t = (float)(k * k) / 36.0f, y = hy + (r.y + r.h - hy) * t;
        cui_draw_line(ui, (cui_vec2){ r.x, y }, (cui_vec2){ r.x + r.w, y }, 1, line);
    }
    float s = (r.w < r.h ? r.w : r.h) * 0.16f, cx = hx - s * 0.2f, cy = hy + (r.y + r.h - hy) * 0.42f;
    cui_vec2 f[4] = { { cx - s, cy - s * 0.2f }, { cx + s * 0.6f, cy - s * 0.2f }, { cx + s * 0.6f, cy + s * 1.2f }, { cx - s, cy + s * 1.2f } };
    cui_vec2 top[4] = { f[0], f[1], { f[1].x + s * 0.55f, f[1].y - s * 0.45f }, { f[0].x + s * 0.55f, f[0].y - s * 0.45f } };
    cui_vec2 side[4] = { f[1], top[2], { top[2].x, f[2].y - s * 0.45f }, f[2] };
    cui_draw_polygon(ui, f, 4, cui_color_of(ui, CUI_COLOR_SURFACE_RAISED));
    cui_draw_polygon(ui, top, 4, cui_color_of(ui, CUI_COLOR_SURFACE));
    cui_draw_polygon(ui, side, 4, cui_color_of(ui, CUI_COLOR_BORDER));
    cui_color acc = cui_color_of(ui, CUI_COLOR_ACCENT);
    cui_draw_polyline(ui, f, 4, 1.5f, acc, true);
    cui_draw_polyline(ui, top, 4, 1.5f, acc, true);
    cui_draw_polyline(ui, side, 4, 1.5f, acc, true);
    cui_draw_text(ui, CUI_FONT_MEDIUM, 12, (cui_vec2){ r.x + 12, r.y + 10 }, cui_color_of(ui, CUI_COLOR_TEXT_SECONDARY), "Perspective", -1);
    cui_draw_text(ui, CUI_FONT_MONO, 11, (cui_vec2){ r.x + 12, r.y + r.h - 24 }, cui_color_of(ui, CUI_COLOR_TEXT_MUTED), "1280 x 720  16.4 ms", -1);
}
static void dk_scene_tree(cui_ctx *ui) {
    static bool open_root = true, open_pier = true, open_lights = false;
    static const char *const pier[4] = { "pier_01", "pier_02", "crate_large_01", "crate_large_02" };
    cui_scroll(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 8) {
        cui_tree(ui, "harbor_town", &open_root, .icon = CUI_ICON_LAYERS) {
            bool t = false;
            cui_tree(ui, "Terrain", &t, .icon = CUI_ICON_BOX, .leaf = true) {}
            cui_tree(ui, "Pier", &open_pier, .icon = CUI_ICON_FOLDER) {
                for (int k = 0; k < 4; k++) {
                    cui_tree(ui, pier[k], &t, .icon = CUI_ICON_BOX, .leaf = true, .selected = dk_pick == k) {}
                    if (cui_tree_clicked(ui)) dk_pick = k;
                }
            }
            cui_tree(ui, "Lights", &open_lights, .icon = CUI_ICON_SUN) {
                cui_tree(ui, "sun", &t, .icon = CUI_ICON_SUN, .leaf = true) {}
            }
        }
    }
}
static void dk_assets(cui_ctx *ui) {
    static const struct { const char *name, *icon, *size; } files[] = {
        { "barrel_d.png", CUI_ICON_IMAGE, "16.0 MB" }, { "crate_large.obj", CUI_ICON_BOX, "412 KB" },
        { "dock_planks.mat", CUI_ICON_PALETTE, "3 KB" }, { "gull_call.ogg", CUI_ICON_AUDIO_LINES, "88 KB" },
        { "harbor_town.scene", CUI_ICON_LAYERS, "1.2 MB" }, { "water_surface.frag", CUI_ICON_CODE, "6 KB" },
    };
    static int sel = 1;
    cui_scroll(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 6) {
        for (int k = 0; k < 6; k++)
            if (cui_list_item(ui, files[k].name, .icon = files[k].icon, .trailing = files[k].size, .selected = sel == k)) sel = k;
    }
}
static void dk_inspector(cui_ctx *ui) {
    static float pos[3] = { 12.5f, 0.0f, -4.25f }, rot[3] = { 0, 45, 0 };
    static float roughness = 0.42f;
    static cui_color base = { 0xC2, 0x6A, 0x3D, 255 };
    static int shader = 0;
    static bool t_open = true, m_open = true, cast = true;
    static const char *const shaders[] = { "Standard PBR", "Unlit", "Toon", "Glass" };
    cui_scroll(ui, .width = CUI_FILL, .height = CUI_FILL) {
        cui_column(ui, .width = CUI_FILL, .pad_x = 12, .pad_y = 4) {
            if (cui_inspector_begin(ui, "props")) {
                if (cui_inspector_group(ui, "Transform", &t_open)) {
                    cui_prop_vec3(ui, "Position", pos, .step = 0.05f, .format = "%.2f");
                    cui_prop_vec3(ui, "Rotation", rot, .step = 1, .format = "%.0f");
                }
                if (cui_inspector_group(ui, "Material", &m_open)) {
                    cui_prop_enum(ui, "Shader", shaders, 4, &shader);
                    cui_prop_color(ui, "Base color", &base);
                    cui_prop_float(ui, "Roughness", &roughness, .min = 0, .max = 1, .step = 0.01f);
                    cui_prop_bool(ui, "Cast shadows", &cast);
                }
                cui_inspector_end(ui);
            }
        }
    }
}
static void body_docking(cui_ctx *ui) {
    static bool filled = false;
    if (!filled) { fill_build_log(); filled = true; }
    const char *layout = cui_viewport_size(ui).x < 620 ? dk_layout_narrow : dk_layout;
    WINDOW(ui) {
        CHROME_TOP(ui) {
            cui_toolbar(ui, "dock-tools") {
                /* a View menu in miniature: each toggle shows or hides a panel, as the x on its tab does */
                for (int k = 0; k < 5; k++) cui_toolbar_button(ui, dk_icons[k], .tooltip = dk_titles[k], .toggle = &dk_open[k]);
                cui_toolbar_spacer(ui);
                if (cui_toolbar_button(ui, CUI_ICON_ROTATE_CCW, .label = "Reset layout")) cui_dock_load(ui, "workspace", layout);
            }
        }
        cui_dockspace(ui, "workspace", .layout = layout, .shortcuts = true) {
            for (int k = 0; k < 5; k++) {
                bool shown = cui_dock_panel_begin(ui, dk_titles[k], &dk_open[k], .icon = dk_icons[k]);
                dk_tab[k] = cui_last_id(ui) ? cui_last_rect(ui) : (cui_rect){ 0, 0, 0, 0 };
                if (!shown) continue;
                switch (k) {
                case 0: dk_scene_tree(ui); break;
                case 1: dk_assets(ui); break;
                case 2: dk_view = cui_canvas(ui, "viewport", CUI_FILL, CUI_FILL, dk_paint_viewport, NULL, 0).rect; break;
                case 3: cui_log_view(ui, "log", &g_log, .height = -1); break;
                default: dk_inspector(ui); break;
                }
                cui_dock_panel_end(ui);
            }
        }
    }
}
static void scene_docking(cui_ctx *ui, int f) {
    if (f < 0) {   /* carry the Log tab over the viewport's right side: the drop targets and the preview show */
        int t = -f - 1;
        cui_rect a = dk_tab[3], v = dk_view;
        float x0 = a.x + 20, y0 = a.y + a.h * 0.5f, x1 = v.x + v.w * 0.86f, y1 = v.y + v.h * 0.55f;
        if (t == 30 && a.w > 0) cui_input_mouse_move(ui, x0, y0);
        if (t == 31 && a.w > 0) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t >= 32 && t <= 50 && a.w > 0) { float k = (t - 32) / 18.0f; cui_input_mouse_move(ui, x0 + (x1 - x0) * k, y0 + (y1 - y0) * k); }
        return;
    }
    frame_wrap(ui, body_docking);
}

/* ---- share images: the newer components on the stage, 16:9 ---- */
static void tw_studio_body(cui_ctx *ui) {
    static char path[512] = "/Users/maya/Projects/fieldnotes";
    static const char *const places[] = { "/Users/maya", "/Users/maya/Desktop", "/Users/maya/Documents", "/Users/maya/Projects" };
    static bool filled;
    if (!filled) { term_demo_fill(); filled = true; }
    cui_row(ui, .gap = 20, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_box(ui, .width = CUI_PX(610)) { cui_file_browser(ui, "files", path, sizeof path, .list = demo_fs, .places = places, .place_count = 4, .height = CUI_PX(430)); }
        cui_box(ui, .width = CUI_FILL) { cui_terminal(ui, "term", &g_term, .height = 430, .prompt = "~/fieldnotes $ ", .size = 12); }
    }
}
static void scene_tw_studio(cui_ctx *ui, int f) { if (f < 0) return; stage(ui, &look_plain, tw_studio_body); }

static void tw_audio_body(cui_ctx *ui) {
    static const cui_gauge_band load[] = { { 60, CUI_TONE_SUCCESS }, { 85, CUI_TONE_WARNING }, { 100, CUI_TONE_DANGER } };
    cui_column(ui, .gap = 24, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
        body_mixer(ui);
        cui_row(ui, .gap = 28, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) {
            cui_radial_gauge(ui, "cpu", 42, 0, 100, .unit = "%", .label = "DSP load", .bands = load, .band_count = 3, .size = 150);
            cui_radial_gauge(ui, "lat", 6, 0, 20, .unit = " ms", .label = "Latency", .size = 150, .ticks = 5);
            cui_radial_gauge(ui, "disk", 63, 0, 100, .unit = "%", .label = "Session disk", .no_needle = true, .size = 150, .ticks = -1);
        }
    }
}
static void scene_tw_audio(cui_ctx *ui, int f) { if (f < 0) return; stage(ui, &look_plain, tw_audio_body); }

static void tw_markets_body(cui_ctx *ui) {
    static const cui_gauge_band risk[] = { { 40, CUI_TONE_SUCCESS }, { 70, CUI_TONE_WARNING }, { 100, CUI_TONE_DANGER } };
    cui_row(ui, .gap = 20, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
        cui_box(ui, .width = CUI_GROW(1)) {
            enum { N = 140 };
            static cui_candle cs[N];
            static const char *labels[N];
            static char buf[N][12];
            static float sma[N];
            static bool init;
            if (!init) { demo_candles(cs, N, labels, buf, sma, 20); init = true; }
            cui_series ov = { "SMA 20", sma, N, { 0 } };
            cui_card(ui, .width = CUI_FILL, .gap = 10, .pad = 18) {
                cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                    cui_text(ui, "NRTH", .font = CUI_FONT_MEDIUM, .size = 15);
                    cui_text(ui, "Northwind Labs", .color = CUI_COLOR_TEXT_SECONDARY);
                    cui_spacer(ui);
                    cui_badge(ui, "Daily", .tone = CUI_TONE_NEUTRAL);
                }
                cui_candlestick(ui, "nrth", cs, N, .labels = labels, .overlays = &ov, .overlay_count = 1, .height = 420);
            }
        }
        cui_column(ui, .gap = 16, .width = CUI_PX(250)) {
            cui_card(ui, .pad = 16, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_radial_gauge(ui, "risk", 34, 0, 100, .label = "Volatility", .bands = risk, .band_count = 3, .size = 190);
            }
            cui_card(ui, .pad = 16, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_radial_gauge(ui, "goal", 68, 0, 100, .unit = "%", .label = "Of yearly target", .no_needle = true, .size = 190, .ticks = -1);
            }
        }
    }
}
static void scene_tw_markets(cui_ctx *ui, int f) { if (f < 0) return; stage(ui, &look_plain, tw_markets_body); }

static void tw_calendar_body(cui_ctx *ui) {
    static cui_date month = { 2026, 11, 1 }, sel = { 2026, 11, 10 };
    static char recent[256] = "\xf0\x9f\x91\x8d \xf0\x9f\x8e\x89 \xe2\x9c\x85 \xf0\x9f\x98\x82 \xf0\x9f\x9a\x80 \xf0\x9f\x91\x80 \xe2\x9d\xa4\xef\xb8\x8f \xf0\x9f\x99\x8f";
    static int tone = 0;
    cui_row(ui, .gap = 24, .align = CUI_ALIGN_START, .width = CUI_FILL) {
        cui_card(ui, .pad = 18, .width = CUI_GROW(1)) {
            cui_month_calendar(ui, "month", &month, month_events, (int)(sizeof month_events / sizeof month_events[0]), .selected = &sel,
                               .today = (cui_date){ 2026, 11, 8 }, .cell_height = 76);
        }
        cui_emoji_picker(ui, "emoji", .recent = recent, .recent_cap = sizeof recent, .tone = &tone, .height = 390);
    }
}
static void scene_tw_calendar(cui_ctx *ui, int f) {
    if (f < 0) { if (-f - 1 >= 20) cui_input_mouse_move(ui, 865, 195); return; }   /* hover the party popper: the preview names it */
    stage(ui, &look_plain, tw_calendar_body);
}

/* ---- instruments ---- */

SCENE(dot_chart) {
    /* a week of requests per hour: the six days before in grey, today in the accent */
    static float base[168], today[168];
    for (int i = 0; i < 168; i++) {
        float h = (float)(i % 24), day = (float)(i / 24);
        float load = 18 + 42 * expf(-(h - 14) * (h - 14) / 18.0f) + 6 * sinf(day * 1.7f + h * 0.4f);
        base[i] = i < 144 ? load : 0;
        today[i] = i >= 144 ? load * 1.25f : 0;
    }
    cui_dot_series series[2] = { { base, "Last six days", false }, { today, "Today", true } };
    cui_box(ui, UPTO(860))
        cui_dot_chart(ui, "requests", series, 2, 168, .x_min = 0, .x_max = 168, .x_step = 24, .x_format = "%.0f h",
                      .height = 220, .unit = "REQ/S");
}

SCENE(chart_annotations) {
    /* response times from 14:00 to 15:00, one sample a minute; x is in hours */
    static float wait[61];
    for (int i = 0; i < 61; i++) wait[i] = 120 + 90 * sinf(i * 0.21f) + (i > 38 && i < 46 ? 260 : 0) + (float)(i % 7) * 6;
    const cui_chart_annotation notes[] = {
        { CUI_ANNOTATION_Y, 0, 300, "SLO", NULL, true },
        { CUI_ANNOTATION_X, 14 + 39 / 60.0, 0, "DEPLOY 4.12", "14:39", false },
        { CUI_ANNOTATION_X, 14 + 46 / 60.0, 0, "ROLLBACK", "14:46", false },
        { CUI_ANNOTATION_POINT, 14 + 42 / 60.0, 515, "PEAK 515 MS", NULL, false },
    };
    cui_dot_series series[1] = { { wait, "Wait", false } };
    cui_box(ui, UPTO(820))
        cui_dot_chart(ui, "wait", series, 1, 61, .x_min = 14, .x_max = 15, .x_units = CUI_AXIS_HOURS, .x_step = 10 / 60.0,
                      .height = 240, .min = 0, .max = 600, .unit = "MS", .annotations = notes, .annotation_count = 4);
}

SCENE(event_track) {
    static double doses[8] = { 7.5, 13, 19.25 };
    static int count = 3, selected = 1;
    cui_column(ui, .gap = 10, UPTO(720)) {
        cui_text(ui, "DOSES TODAY", .font = CUI_FONT_MONO, .color = CUI_COLOR_TEXT_MUTED, .size = 11);
        cui_event_track(ui, "doses", doses, &count, 8, 6, 30, .step = 0.25, .units = CUI_AXIS_HOURS, .label_step = 3, .selected = &selected);
    }
}

SCENE(readouts) {
    cui_row(ui, .gap = 44, .wrap = true, .align = CUI_ALIGN_END) {
        cui_readout(ui, "STILL IN YOU AT 23:00", "54", .unit = "MG", .detail = "UNDER THE LINE BY 00:10", .size = CUI_SIZE_LG);
        cui_readout(ui, "PEAK", "203", .unit = "MG", .detail = "13:06");
        cui_readout(ui, "HALF-LIFE", "5.0", .unit = "H", .detail = "ADULT AVERAGE", .size = CUI_SIZE_SM);
        cui_readout(ui, "OVER THE LINE", "2", .unit = "CUPS", .detail = "AFTER 14:00", .size = CUI_SIZE_SM, .accent = true);
    }
}

SCENE(dot_display) {
    cui_column(ui, .gap = 18) {
        cui_dot_display(ui, "23:41:07", .pitch = 6);
        cui_dot_display(ui, "LAST CUP BY 10:45", .pitch = 3);
        cui_dot_display(ui, "21.5\xc2\xb0" "C  \xc2\xb7  48% RH", .pitch = 3, .accent = true);
    }
}

/* ---- creative controls ---- */
static const char *const cc_tag_names[] = { "Pending", "In progress", "Submitted", "In review", "Success", "Failed", "Expired" };
static bool cc_tag_gone[7];
static void body_status_tag(cui_ctx *ui) {
    cui_column(ui, .gap = 18, UPTO(560), .align = CUI_ALIGN_CENTER) {
        cui_row(ui, .gap = 8, .wrap = true, .justify = CUI_JUSTIFY_CENTER) {
            for (int k = 0; k < 7; k++) cui_status_tag(ui, cc_tag_names[k], (cui_tag_status)k);
        }
        cui_column(ui, .width = CUI_FILL, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 14, .clip = true) {
            cui_row(ui, .width = CUI_FILL, .pad_x = 14, .pad_y = 10, .gap = 8, .align = CUI_ALIGN_CENTER, .bg = CUI_COLOR_SURFACE_MUTED, .radius_tl = 14, .radius_tr = 14) {
                cui_icon(ui, CUI_ICON_TAG, 16, CUI_COLOR_TEXT_SECONDARY);
                cui_text(ui, "Tags", .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_TEXT_SECONDARY);
                cui_spacer(ui);
                cui_text(ui, "small, removable", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
            }
            cui_separator(ui);
            cui_row(ui, .width = CUI_FILL, .pad = 14, .gap = 6, .wrap = true) {
                for (int k = 0; k < 7; k++)
                    if (!cc_tag_gone[k] && cui_status_tag(ui, cc_tag_names[k], (cui_tag_status)k, .size = CUI_SIZE_SM, .removable = true, .id = cc_tag_names[k] + 0))
                        cc_tag_gone[k] = true;
            }
        }
    }
}
/* the pointer rests on the removable "Submitted" tag, so its x shows */
static void scene_status_tag(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "Submitted"));
        if (-f - 1 == 3 && r.w > 0) cui_input_mouse_move(ui, r.x + r.w * 0.5f, r.y + r.h * 0.5f);
        return;
    }
    frame_wrap(ui, body_status_tag);
}

static float cc_coffee = 0.37f, cc_train = 0.8f;
SCENE(ratio_slider) {
    cui_column(ui, .gap = 14, UPTO(560)) {
        cui_ratio_slider(ui, "COFFEE", "MILK", &cc_coffee, .tone = CUI_TONE_WARNING);
        cui_ratio_slider(ui, "TRAIN", "TEST", &cc_train, .tone = CUI_TONE_INFO);
    }
}

static float cc_hue = 190, cc_kelvin = 5600, cc_opacity = 0.72f;
static int cc_variations = 3, cc_quality = 1;
static bool cc_picking;
SCENE(gradient_slider) {
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 250, .gap = 12, UPTO(620)) {
        cui_gradient_slider(ui, "Hue", &cc_hue, 0, 360, .swatch = true);
        if (cui_gradient_slider(ui, "White balance", &cc_kelvin, 2500, 10000, .track = CUI_GRADIENT_TEMPERATURE, .step = 100,
                                .tool = CUI_ICON_TARGET, .tool_tooltip = "Pick from image", .tool_on = cc_picking) & CUI_WELL_ACTION)
            cc_picking = !cc_picking;
        cui_gradient_slider(ui, "Opacity", &cc_opacity, 0, 1, .track = CUI_GRADIENT_OPACITY, .color = CUI_RGB(0x2bb8a6));   /* the brush color */
    }
}
SCENE(step_slider) {
    static const char *const quality[] = { "Draft", "Good", "Best" };
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 250, .gap = 12, UPTO(620)) {
        cui_step_slider(ui, "Variations", &cc_variations, 1, 4, .tool = CUI_ICON_REFRESH_CW, .tool_tooltip = "Generate");
        cui_step_slider(ui, "Quality", &cc_quality, 0, 2, .labels = quality);
    }
}

static cui_color cc_palette[5] = {   /* plain initializers: a static array cannot hold CUI_RGB's compound literals in MSVC */
    { 0xee, 0xf0, 0xf5, 255 }, { 0x5c, 0x63, 0x70, 255 }, { 0x37, 0xc6, 0xd0, 255 }, { 0x1f, 0x3a, 0x5f, 255 }, { 0xf2, 0xbd, 0x68, 255 } };
static bool cc_locked[5] = { false, true, false, true, false };
static uint32_t cc_palette_seed = 7;
static void body_palette_strip(cui_ctx *ui) {
    cui_column(ui, UPTO(620)) {
        cui_palette_strip(ui, "Color palette", cc_palette, cc_locked, 5, .seed = &cc_palette_seed, .value = "Click a swatch to lock it");
    }
}
/* the pointer rests on the third swatch, so it widens and shows its code */
static void scene_palette_strip(cui_ctx *ui, int f) {
    if (f < 0) {
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "Color palette"));
        if (-f - 1 == 3 && r.w > 0) cui_input_mouse_move(ui, r.x + 14 + (r.w - 14 - 12 - 40) * 0.5f, r.y + r.h - 28);
        return;
    }
    frame_wrap(ui, body_palette_strip);
}

static int cc_aspect = 1;
SCENE(aspect_picker) {
    cui_column(ui, UPTO(620)) {
        cui_aspect_picker(ui, "Aspect ratio", &cc_aspect, .custom = true);
    }
}

static int cc_background = 0, cc_format = 0;
SCENE(option_action) {
    static const char *const bg[] = { "hidden", "visible" };
    static const char *const fmt[] = { "png", "jpg", "webp" };
    static const char *const sizes[] = { "2.4 MB", "640 KB", "410 KB" };
    cui_box(ui, .dir = CUI_GRID, .columns = 2, .min_column_width = 250, .gap = 12, UPTO(620)) {
        cui_option_action(ui, "Background", bg, 2, &cc_background, CUI_ICON_SCISSORS, .tooltip = "Cut out", .value = "erase");
        cui_option_action(ui, "Save as", fmt, 3, &cc_format, CUI_ICON_DOWNLOAD, .tooltip = "Download", .value = sizes[cc_format]);
    }
}

static uint32_t cc_seed = 1234567890u, cc_seed2 = 3141592653u;
static bool cc_seed_fixed = false, cc_seed2_fixed = true;
SCENE(seed_field) {
    cui_column(ui, .gap = 12, UPTO(620)) {
        cui_seed_field(ui, "Seed", &cc_seed, &cc_seed_fixed, .value = cc_seed_fixed ? "Same image every run" : "Random each run");
        cui_seed_field(ui, "Noise seed", &cc_seed2, &cc_seed2_fixed, .value = cc_seed2_fixed ? "Same image every run" : "Random each run");
    }
}

typedef struct { const char *name; scene_fn fn; float w, h; bool dark_too; bool dark_only; cui_accent accent; } scene;

static void render(const char *dir, const scene *s, bool dark, cui_sw_renderer *sw) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                             .accent = s->accent });
    cui_set_viewport(ui, s->w, s->h, 2);
    cui_input_mouse_leave(ui);
    g_scene_w = s->w;
    for (int f = 0; f < 70; f++) { s->fn(ui, -(f + 1)); cui_begin_frame(ui, 1.0f / 60.0f); s->fn(ui, f); cui_end_frame(ui); }
    const cui_draw_data *dd = cui_get_draw_data(ui);
    unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
    cui_sw_render(sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
    char path[512];
    snprintf(path, sizeof path, "%s/%s%s.png", dir, s->name, dark ? "-dark" : "");
    cui_write_png(path, px, dd->width_px, dd->height_px, dd->width_px * 4);
    printf("wrote %s\n", path);
    free(px);
    cui_destroy(ui);
}

/* ---- state matrix: every button variant at rest, hover, pressed, focus and
 * disabled. Each cell is its own context so the states can sit side by side. */
typedef struct { int variant, state; } cell_spec;
static cell_spec g_cell;
static void cell_body(cui_ctx *ui) {
    static const char *labels[] = { "Primary", "Secondary", "Outline", "Ghost", "Danger" };
    static const cui_button_variant vars[] = { CUI_BUTTON_PRIMARY, CUI_BUTTON_SECONDARY, CUI_BUTTON_OUTLINE, CUI_BUTTON_GHOST, CUI_BUTTON_DANGER };
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) {
        if (g_cell.variant < 0) {
            static const char *states[] = { "Rest", "Hover", "Pressed", "Keyboard focus", "Disabled" };
            cui_text(ui, states[g_cell.state], .size = 12, .font = CUI_FONT_MEDIUM, .color = CUI_COLOR_TEXT_SECONDARY);
        } else {
            cui_button(ui, labels[g_cell.variant], .variant = vars[g_cell.variant], .disabled = g_cell.state == 4);
        }
    }
}
static void render_states(const char *dir, bool dark, cui_sw_renderer *sw) {
    const float cw = 150, ch = 64, hh = 36;
    const int cols = 5, rows = 5, S = 2;
    int W = (int)(cw * cols) * S, H = (int)(hh + ch * rows) * S;
    unsigned char *big = calloc((size_t)W * H, 4);
    for (int r = -1; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            g_cell.variant = r; g_cell.state = c;
            float h = r < 0 ? hh : ch;
            cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT });
            cui_set_viewport(ui, cw, h, S);
            cui_input_mouse_leave(ui);
            for (int f = 0; f < 50; f++) {
                if (r >= 0 && f == 2 && (c == 1 || c == 2)) cui_input_mouse_move(ui, cw / 2, h / 2);
                if (r >= 0 && f == 4 && c == 2) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
                if (r >= 0 && f == 2 && c == 3) { cui_input_key(ui, CUI_KEY_TAB, true, 0); cui_input_key(ui, CUI_KEY_TAB, false, 0); }
                cui_begin_frame(ui, 1.0f / 60.0f); cell_body(ui); cui_end_frame(ui);
            }
            const cui_draw_data *dd = cui_get_draw_data(ui);
            unsigned char *px = malloc((size_t)dd->width_px * dd->height_px * 4);
            cui_sw_render(sw, dd, px, dd->width_px, dd->height_px, dd->width_px * 4, CUI_SW_RGBA);
            int ox = (int)(c * cw) * S, oy = r < 0 ? 0 : (int)(hh + r * ch) * S;
            for (int y = 0; y < dd->height_px && oy + y < H; y++)
                memcpy(big + ((size_t)(oy + y) * W + ox) * 4, px + (size_t)y * dd->width_px * 4, (size_t)dd->width_px * 4);
            free(px);
            cui_destroy(ui);
        }
    char path[512];
    snprintf(path, sizeof path, "%s/button-states%s.png", dir, dark ? "-dark" : "");
    cui_write_png(path, big, W, H, W * 4);
    printf("wrote %s\n", path);
    free(big);
}

/* ---- developer tools ---- */
static cui_kv_pair kv_pairs[12];
static int kv_n = -1;
static void body_kv(cui_ctx *ui) {
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
static void body_problems(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_problems(ui, "problems", pbm_diags, (int)(sizeof pbm_diags / sizeof pbm_diags[0]), .height = 330);
    }
}

static int pl_sel = 3;
static void body_pipeline(cui_ctx *ui) {
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
static void body_query(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 720) {
        cui_query_builder(ui, "filter", qb_fields, 4, qb_nodes, &qb_n, 16);
        char text[256];
        cui_query_text(qb_fields, 4, qb_nodes, qb_n, text, sizeof text);
        cui_text(ui, text, .size = 12, .color = CUI_COLOR_TEXT_MUTED);
    }
}

static char sc_cron[64] = "30 9 * * 1-5";
static void body_schedule(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 560) {
        cui_schedule_picker(ui, "backup", sc_cron, sizeof sc_cron, .now = 1794211200, .utc = true, .next_count = 3);
    }
}

static int km_mods = CUI_MOD_PRIMARY;
static void body_keymap(cui_ctx *ui) {
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
static void body_commits(cui_ctx *ui) {
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
static void body_graph(cui_ctx *ui) {
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
static void body_waterfall(cui_ctx *ui) {
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
static void body_trace(cui_ctx *ui) {
    if (!tr_n) tr_build();
    if (tr_sel < 0) for (int i = 0; i < tr_n; i++) if (!strcmp(tr_spans[i].name, "Update") && tr_spans[i].start > 0.016 && tr_spans[i].start < 0.03) tr_sel = i;
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 1000) {
        cui_trace_view(ui, "trace", tr_tracks, 5, tr_spans, tr_n, &tr_view, &tr_sel, .markers = tr_marks, .marker_count = 4, .height = 320);
    }
}
static void scene_dev_kv(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_kv); }
static void scene_dev_problems(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_problems); }
static void scene_dev_pipeline(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_pipeline); }
static void scene_dev_query(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_query); }
static void scene_dev_schedule(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_schedule); }
static void scene_dev_keymap(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_keymap); }
static void scene_dev_commits(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_commits); }
static void scene_dev_graph(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_graph); }
static void scene_dev_waterfall(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_waterfall); }
static void scene_dev_trace(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_trace); }

/* ---- developer tools, second set ---- */
static char rx_pat[128] = "(?<user>[\\w.]+)@(?<host>[\\w-]+)\\.(com|org)";
static char rx_text[1024] = "Contact maya.okafor@example.com or jonas@cui-lib.org for access.\nOld address: priya@legacy.net (no longer read).\nSupport: help@example.com";
static void body_regex(cui_ctx *ui) {
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
static void body_tests(cui_ctx *ui) {
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
static void body_stack(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 760) {
        cui_stack_trace(ui, "trace", st_frames, 6, .title = "TypeError", .message = "Cannot read properties of undefined (reading 'items')", .lang = "js");
    }
}

static int rq_method = 1;
static char rq_url[256] = "https://api.example.com/v1/orders?status=open&limit=20";
static cui_kv_pair rq_params[8];
static int rq_n = -1;
static void body_request(cui_ctx *ui) {
    if (rq_n < 0) rq_n = cui_url_params(rq_url, rq_params, 8);
    cui_column(ui, .gap = 12, .width = CUI_FILL, .max_width = 760) {
        cui_request_bar_result r = cui_request_bar(ui, "req", &rq_method, rq_url, sizeof rq_url);
        if (r.changed) rq_n = cui_url_params(rq_url, rq_params, 8);
        if (cui_kv_editor(ui, "params", rq_params, &rq_n, 8)) cui_url_set_params(rq_url, sizeof rq_url, rq_params, rq_n);
        cui_response_status(ui, 201, 0.142, 18400);
    }
}

static float up_api[90], up_web[90], up_db[90], up_cdn[90];
static void body_uptime(cui_ctx *ui) {
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
static void body_histogram(cui_ctx *ui) {
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
static void body_coverage(cui_ctx *ui) {
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
static void body_blame(cui_ctx *ui) {
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
static void body_flags(cui_ctx *ui) {
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
static void body_api(cui_ctx *ui) {
    cui_column(ui, .gap = 10, .width = CUI_FILL, .max_width = 820) {
        cui_api_explorer(ui, "api", ax_eps, 8, .selected = &ax_sel);
    }
}
static void scene_dev2_regex(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_regex); }
static void scene_dev2_tests(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_tests); }
static void scene_dev2_stack(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_stack); }
static void scene_dev2_request(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_request); }
static void scene_dev2_uptime(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_uptime); }
static void scene_dev2_histogram(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_histogram); }
static void scene_dev2_coverage(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_coverage); }
static void scene_dev2_blame(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_blame); }
static void scene_dev2_flags(cui_ctx *ui, int f) { if (f < 0) return; frame_wrap(ui, body_flags); }
/* the explorer with one endpoint open: a click on its row, then the pointer leaves */
static void scene_dev2_api(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        cui_rect r = cui_get_rect(ui, cui_make_id(ui, "api"));
        if (t == 3) cui_input_mouse_move(ui, r.x + r.w * 0.5f, r.y + 36 + 28 + 32 * 2 + 16);
        if (t == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (t == 6) cui_input_mouse_leave(ui);
        return;
    }
    frame_wrap(ui, body_api);
}

/* ---- canvas and bars: grouped toolbar, format bar, live cursors, canvas controls, result card, selection bar ---- */
static bool cb_num = true, cb_bold, cb_italic;
static int cb_mode;
static void cb_grouped(cui_ctx *ui, const char *id) {
    static const char *const modes[] = { CUI_ICON_MOVE, CUI_ICON_SQUARE_PEN, CUI_ICON_TYPE };
    static const char *const mode_names[] = { "Move", "Draw", "Text" };
    cui_toolbar(ui, id, .style = CUI_TOOLBAR_GROUPED) {
        cui_toolbar_group(ui) {
            cui_toolbar_button(ui, CUI_ICON_UNDO_2, .tooltip = "Undo", .shortcut = "Ctrl Z");
            cui_toolbar_button(ui, CUI_ICON_REDO_2, .tooltip = "Redo", .shortcut = "Ctrl Shift Z");
        }
        cui_toolbar_group(ui) {
            cui_toolbar_button(ui, NULL, .label = "123", .tooltip = "Numbers", .toggle = &cb_num);
            cui_toolbar_button(ui, CUI_ICON_BOLD, .tooltip = "Bold", .toggle = &cb_bold);
            cui_toolbar_button(ui, CUI_ICON_ITALIC, .tooltip = "Italic", .toggle = &cb_italic);
            cui_toolbar_radio(ui, "mode", modes, 3, &cb_mode, .labels = mode_names);
        }
        cui_toolbar_primary(ui, CUI_ICON_SPARKLES, .tooltip = "Generate");
    }
}
SCENE(grouped_toolbar) {
    cui_column(ui, .gap = 20, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
        cb_grouped(ui, "grouped");
        cui_box(ui, .width = CUI_FILL, .max_width = 300, .align = CUI_ALIGN_CENTER) { cb_grouped(ui, "narrow"); }
    }
}

static cui_text_format cb_fmt;
static void body_format_bar(cui_ctx *ui) {
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_START) { cui_format_bar(ui, "fmt", &cb_fmt); }
}
/* the style menu, open */
static void scene_format_bar(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        if (t == 0) cb_fmt = (cui_text_format){ CUI_TONE_ROSE, true, false, false, 0, 44, CUI_FORMAT_ALIGN_LEFT, CUI_FORMAT_LIST_NONE };
        cui_rect r = cui_get_rect(ui, nested_id(ui, "fmt", "#style"));
        if (t == 4) move_to(ui, r, 0.5f, 0.5f);
        if (t == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t == 6) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (t == 7) cui_input_mouse_leave(ui);
        return;
    }
    frame_wrap(ui, body_format_bar);
}

/* a small prompt graph with three people on it; the cursors are pinned so the picture holds still */
enum { CB_POS = 1, CB_NEG, CB_GEN_POS, CB_GEN_NEG };
static cui_node_view cb_view = { 10, 6, 1 };
static float cb_nodes[3][2] = { { 36, 64 }, { 36, 206 }, { 330, 124 } };
static bool cb_grid = true;
static void cb_canvas(cui_ctx *ui, const char *id, bool people, cui_node_view *view, float height) {
    static const cui_live_user users[] = {
        { .name = "Paul", .tone = CUI_TONE_WARNING, .x = 118, .y = 172, .speaking = true },
        { .name = "Kate", .tone = CUI_TONE_INFO, .x = 432, .y = 156, .says = "Bumping steps to 40?" },
        { .name = "Mario", .tone = CUI_TONE_ROSE, .x = 236, .y = 306 },
    };
    cui_canvas_toggle toggles[] = { { CUI_ICON_EYE, "Show grid", &cb_grid }, { CUI_ICON_LAYERS, "Layers", NULL } };
    cui_box(ui, .dir = CUI_STACK, .width = CUI_FILL, .height = CUI_PX(height), .radius = 14, .clip = true) {
        if (cui_node_editor_begin(ui, id, view, .height = CUI_PX(height), .no_grid = !cb_grid)) {
            static const char *titles[3] = { "Positive", "Negative", "Generate" };
            static const char *bodies[3] = { "A black bear, minimalist style, soft gradients", "No text, no people", "Steps 30 \xc2\xb7 dpm++ 2M" };
            for (int i = 0; i < 3; i++) {
                cui_node_begin(ui, i, &cb_nodes[i][0], &cb_nodes[i][1], titles[i], .width = 184);
                if (i < 2) cui_node_output(ui, i == 0 ? CB_POS : CB_NEG, i == 0 ? "Prompt" : "Avoid", .type = 1);
                else { cui_node_input(ui, CB_GEN_POS, "Prompt", .type = 1); cui_node_input(ui, CB_GEN_NEG, "Avoid", .type = 1); }
                cui_node_content(ui) { cui_text(ui, bodies[i], .size = 12, .color = CUI_COLOR_TEXT_SECONDARY); }
                cui_node_end(ui);
                if (people && i == 2) cui_selection_ring(ui, CUI_COLOR_WARNING, .name = "Paul");
            }
            cui_node_link(ui, 1, CB_POS, CB_GEN_POS);
            cui_node_link(ui, 2, CB_NEG, CB_GEN_NEG);
            cui_node_editor_end(ui);
        }
        cui_rect bounds = { cb_nodes[0][0], cb_nodes[0][1], cb_nodes[2][0] + 184 - cb_nodes[0][0], 290 };
        cui_canvas_controls(ui, "zoom", view, .content = bounds, .toggles = toggles, .toggle_count = 2);
        if (people) {
            cui_live_cursors(ui, "people", users, 3, .view = view);
            cui_box(ui, .pad = 12) { cui_presence_pill(ui, "here", users, 3); }
        }
    }
}
SCENE(live_cursors) { cb_canvas(ui, "prompt", true, &cb_view, 360); }

static cui_node_view cb_view2;
static void body_canvas_controls(cui_ctx *ui) { cb_canvas(ui, "zoomed", false, &cb_view2, 320); }
/* one step out from the rail */
static void scene_canvas_controls(cui_ctx *ui, int f) {
    if (f < 0) {
        int t = -f - 1;
        if (t == 0) cb_view2 = (cui_node_view){ 10, 6, 1 };   /* the light and dark renders each start at 100% */
        cui_rect r = cui_get_rect(ui, nested_id(ui, "zoom", "#zoom"));
        if (t == 4) cui_input_mouse_move(ui, r.x + r.w * 0.5f, r.y + r.h + 20);
        if (t == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (t == 6) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        if (t == 7) cui_input_mouse_leave(ui);
        return;
    }
    frame_wrap(ui, body_canvas_controls);
}

/* a picture made here: a sky, a low sun and soft hills */
static cui_texture cb_result_image(cui_ctx *ui) {
    static cui_ctx *made_for;
    static cui_texture tex;
    if (made_for == ui && tex) return tex;
    enum { W = 288, H = 360 };
    static unsigned char px[W * H * 4];
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float acc[3] = { 0, 0, 0 };
            for (int s = 0; s < 4; s++) {   /* four samples a pixel, for smooth edges */
            float fx = x + 0.25f + 0.5f * (s & 1), fy = y + 0.25f + 0.5f * (s >> 1);
            float v = fy / H, r, g, b;
            if (v < 0.48f) { float k = v / 0.48f; r = 118 + (185 - 118) * k; g = 169 + (212 - 169) * k; b = 236 + (246 - 236) * k; }
            else { float k = (v - 0.48f) / 0.52f; r = 185 + (242 - 185) * k; g = 212 + (217 - 212) * k; b = 246 + (203 - 246) * k; }
            float dx = fx - W * 0.66f, dy = fy - H * 0.32f, d = sqrtf(dx * dx + dy * dy) / W;
            float sun = d < 0.08f ? 1 : d < 0.24f ? 0.55f * (1 - (d - 0.09f) / 0.15f) : 0;
            if (sun > 0) { sun = sun > 1 ? 1 : sun; r += (255 - r) * sun; g += (247 - g) * sun; b += (236 - b) * sun; }
            static const float hills[3][6] = { { 0.18f, 1.04f, 1.30f, 0.62f, 0.42f, 0 }, { 0.92f, 1.08f, 1.20f, 0.58f, 0.44f, 1 }, { 0.5f, 1.18f, 1.6f, 0.70f, 0.40f, 2 } };
            static const float hc[3][3] = { { 238, 143, 120 }, { 184, 96, 106 }, { 122, 63, 82 } };
            for (int h = 0; h < 3; h++) {
                float ex = (fx / W - hills[h][0]) / (hills[h][2] * 0.5f * hills[h][4] * 2), ey = (v - hills[h][1]) / (hills[h][3] * 0.5f * hills[h][4] * 2);
                if (ex * ex + ey * ey < 1) { r = hc[h][0]; g = hc[h][1]; b = hc[h][2]; }
            }
            acc[0] += r * 0.25f; acc[1] += g * 0.25f; acc[2] += b * 0.25f;
            }
            unsigned char *p = &px[(y * W + x) * 4];
            p[0] = (unsigned char)acc[0]; p[1] = (unsigned char)acc[1]; p[2] = (unsigned char)acc[2]; p[3] = 255;
        }
    tex = cui_image_create(ui, W, H, px);
    made_for = ui;
    return tex;
}
/* a picture, a secret and a spoiler; the key is shown, the others covered */
static bool veil_photo, veil_key = true, veil_spoiler;
static void body_veil(cui_ctx *ui) {
    cui_column(ui, .gap = 12, .pad = 12, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 16, UPTO(420)) {
        cui_veil(ui, "photo", &veil_photo, .label = "Sensitive image", .hint = "Click to show", .toggle = true, .radius = 10) {
            cui_image(ui, cb_result_image(ui), .width = CUI_PX(234), .height = CUI_PX(150), .radius = 10);
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .pad = 8, .pad_x = 10, .bg = CUI_COLOR_SURFACE_MUTED, .radius = 10, .width = CUI_FILL) {
            cui_text(ui, "sk-live-", .font = CUI_FONT_MONO, .size = 12.5f, .color = CUI_COLOR_TEXT_SECONDARY);
            cui_box(ui, .width = CUI_FILL) {
                cui_veil(ui, "key", &veil_key, .color = CUI_COLOR_SURFACE) { cui_text(ui, "4f9a82c1e07d55b3a9", .font = CUI_FONT_MONO, .size = 12.5f); }
            }
            if (cui_button(ui, veil_key ? "Hide" : "Reveal", .size = CUI_SIZE_SM)) veil_key = !veil_key;
        }
        cui_row(ui, .gap = 4, .wrap = true, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Finished it last night. The ending:");
            cui_veil(ui, "spoiler", &veil_spoiler, .toggle = true, .radius = 4) { cui_text(ui, "the keeper was the narrator"); }
        }
    }
}
static void scene_veil(cui_ctx *ui, int f) {
    if (f < 0) return;
    frame_wrap(ui, body_veil);
}

static bool cb_saved;
static int cb_scale = 1, cb_type;
static void cb_result(cui_ctx *ui) {
    static const char *const scales[] = { "1x", "2x", "4x" }, *const types[] = { "PNG", "JPG", "WEBP" };
    cui_column(ui, .gap = 10, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
        cui_result_card(ui, "result", cb_result_image(ui), "Final result", .caption = "Minimal illustration, soft gradients and smooth shapes against a clear sky",
                        .badge = "Done in 4.2 s", .image_w = 288, .image_h = 360, .width = CUI_PX(344));
        cui_toolbar(ui, "result-actions", .style = CUI_TOOLBAR_FLOATING) {
            cui_toolbar_button(ui, CUI_ICON_MAXIMIZE_2, .tooltip = "Expand");
            cui_toolbar_button(ui, CUI_ICON_BOOKMARK, .tooltip = "Save", .toggle = &cb_saved, .tone = CUI_TONE_WARNING);
            cui_toolbar_button(ui, CUI_ICON_COPY, .tooltip = "Copy");
            cui_toolbar_button(ui, CUI_ICON_PANEL_LEFT, .tooltip = "Compare");
            cui_toolbar_separator(ui);
            cui_toolbar_cycle(ui, "scale", scales, 3, &cb_scale, .tooltip = "Scale");
            cui_toolbar_cycle(ui, "type", types, 3, &cb_type, .tooltip = "Format");
            cui_toolbar_separator(ui);
            cui_toolbar_button(ui, CUI_ICON_DOWNLOAD, .tooltip = "Download");
        }
    }
}
SCENE(result_card) { cui_column(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) { cb_result(ui); } }

static bool cb_picked[5] = { true, true, true, false, false };
static void cb_files(cui_ctx *ui) {
    static const char *names[] = { "hero-dark.png", "hero-light.png", "social-card.png", "icon-512.png", "thumb@2x.png" };
    static const char *sizes[] = { "2.4 MB", "2.3 MB", "812 KB", "96 KB", "140 KB" };
    static const cui_selection_action acts[] = { { CUI_ICON_MOVE, "Move" }, { CUI_ICON_COPY, "Copy" }, { CUI_ICON_TRASH_2, "Delete", CUI_TONE_DANGER } };
    cui_column(ui, .id = "files", .width = CUI_FILL, .max_width = 520, .pad = 4, .gap = 2, .bg = CUI_COLOR_SURFACE, .border = 1, .radius = 14) {
        for (int k = 0; k < 5; k++) {
            cui_push_id_int(ui, k);
            cui_row(ui, .width = CUI_FILL, .height = CUI_PX(44), .pad_x = 10, .gap = 12, .align = CUI_ALIGN_CENTER, .radius = 10,
                    .bg = cb_picked[k] ? CUI_COLOR_ACCENT_SUBTLE : (cui_color){ 0 }) {
                cui_checkbox(ui, "##pick", &cb_picked[k]);
                cui_icon(ui, CUI_ICON_IMAGE, 15, cui_color_of(ui, CUI_COLOR_TEXT_MUTED));
                cui_box(ui, .width = CUI_FILL) { cui_label(ui, names[k]); }
                cui_text(ui, sizes[k], .size = 12, .color = CUI_COLOR_TEXT_MUTED, .tabular = true);
            }
            cui_pop_id(ui);
        }
    }
    int a = cui_selection_bar(ui, "bulk", 0, acts, 3, .selected = cb_picked, .item_count = 5, .anchor = cui_make_id(ui, "files"));
    if (a == 2) for (int k = 0; k < 5; k++) cb_picked[k] = false;
}
SCENE(selection_bar) { cui_column(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER, .pad_bottom = 56) { cb_files(ui); } }

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    static const scene scenes[] = {
        { "buttons", scene_buttons, 760, 110, true }, { "segmented", scene_segmented, 420, 110, true },
        { "toggles", scene_toggles, 680, 150, true }, { "slider", scene_slider, 480, 170, true },
        { "inputs", scene_inputs, 680, 230, true }, { "textarea", scene_textarea, 540, 200, false },
        { "pickers", scene_pickers, 680, 130, false }, { "select", scene_select_open, 320, 330, true },
        { "calendar", scene_calendar, 340, 360, true }, { "calendar-months", scene_calendar_months, 340, 360, true }, { "tabs", scene_tabs, 580, 160, true },
        { "navigation", scene_navigation, 620, 200, false }, { "sidebar", scene_sidebar, 260, 210, true },
        { "menu", scene_menu, 300, 290, true }, { "dialog", scene_dialog, 620, 340, true },
        { "toast", scene_toast, 420, 220, true }, { "tooltip", scene_tooltip, 260, 150, false },
        { "alerts", scene_alerts, 620, 210, true }, { "feedback", scene_feedback, 520, 220, false },
        { "dither-progress", scene_dither_progress, 620, 150, true, false, CUI_ACCENT_NEUTRAL },
        { "dither-spinners", scene_dither_spinners, 800, 860, true, false, CUI_ACCENT_NEUTRAL },
        { "status-spinner", scene_status_spinner, 600, 170, true, false, CUI_ACCENT_NEUTRAL },
        { "dither-skeleton", scene_dither_skeleton, 680, 210, true, false, CUI_ACCENT_NEUTRAL },
        { "pattern-meter", scene_pattern_meter_hover, 660, 280, true },
        { "badges", scene_badges, 600, 160, true }, { "cards", scene_cards, 700, 230, true },
        { "empty-state", scene_empty, 580, 340, true }, { "table", scene_table, 680, 250, true },
        { "tree-list", scene_tree, 660, 230, false }, { "accordion", scene_accordion, 580, 200, false },
        { "chips", scene_chips, 520, 140, false }, { "line-chart", scene_line_chart, 740, 320, true },
        { "dither-area", scene_dither_area, 740, 340, true },
        { "bar-donut", scene_bar_chart, 840, 290, true }, { "command-palette", scene_palette, 680, 460, true },
        { "icons", scene_icons, 1040, 1120, false },
        { "radio-cards", scene_radio_cards, 940, 170, true }, { "tag-otp", scene_tag_otp, 520, 330, true },
        { "button-extras", scene_button_extras, 640, 110, true }, { "timeline", scene_timeline, 620, 300, true },
        { "mosaic", scene_mosaic, 1440, 900, true, true, CUI_ACCENT_BLUE },
        { "mosaic-pro", scene_mosaic_pro, 1440, 900, true, true, CUI_ACCENT_GREEN },
        { "hero-board", scene_hero_board, 1000, 600, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-ai", scene_hero_ai, 1080, 640, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-desk", scene_hero_desk, 1000, 600, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-pick", scene_hero_pick, 1160, 560, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-palette", scene_hero_palette, 900, 520, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-plan", scene_hero_plan, 1100, 520, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-charts", scene_hero_charts, 1000, 700, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-social", scene_hero_social, 1080, 620, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-data", scene_hero_data, 1000, 560, true, false, CUI_ACCENT_NEUTRAL },
        { "hero-effects", scene_hero_effects, 1000, 560, true, false, CUI_ACCENT_NEUTRAL },
        { "stage-mosaic", scene_stage_mosaic, 1440, 980, true, false, CUI_ACCENT_NEUTRAL },
        { "stage-mosaic-pro", scene_stage_mosaic_pro, 1440, 980, true, false, CUI_ACCENT_NEUTRAL },
        { "accent-neutral", scene_accent, 290, 252, true, false, CUI_ACCENT_NEUTRAL }, { "accent-violet", scene_accent, 290, 252, true, false, CUI_ACCENT_VIOLET },
        { "accent-blue", scene_accent, 290, 252, true, false, CUI_ACCENT_BLUE }, { "accent-green", scene_accent, 290, 252, true, false, CUI_ACCENT_GREEN },
        { "accent-amber", scene_accent, 290, 252, true, false, CUI_ACCENT_AMBER }, { "accent-orange", scene_accent, 290, 252, true, false, CUI_ACCENT_ORANGE },
        { "accent-coral", scene_accent, 290, 252, true, false, CUI_ACCENT_CORAL }, { "accent-rose", scene_accent, 290, 252, true, false, CUI_ACCENT_ROSE },
        { "split-button", scene_split_button, 560, 110, true }, { "toggle-group", scene_toggle_group, 560, 110, true },
        { "morph-buttons", scene_morph, 760, 110, true }, { "user-menu", scene_user_menu, 340, 360, true },
        { "inline-edit", scene_inline_edit, 420, 190, true }, { "scrub", scene_scrub, 600, 110, true },
        { "shortcut-recorder", scene_shortcut_rec, 760, 110, true },
        { "multi-select", scene_multi, 380, 420, true }, { "date-range", scene_range, 760, 440, true },
        { "time-picker", scene_time, 420, 330, true }, { "wheels", scene_wheels, 700, 340, true },
        { "ruler-histogram", scene_rulers, 860, 224, true }, { "mixer", scene_mixer, 760, 330, true }, { "capture", scene_capture, 900, 300, true },
        { "hover-card", scene_hover_card, 460, 220, true }, { "confirm-popover", scene_confirm_pop, 420, 260, true },
        { "expandables", scene_expandables, 680, 320, true }, { "banner-countdown", scene_banner_countdown, 820, 230, true },
        { "checklist", scene_checklist, 500, 420, true }, { "toc", scene_toc_input, 760, 320, true },
        { "undo-toast", scene_undo, 460, 200, true },
        { "data-grid", scene_data_grid, 760, 312, true }, { "code-block", scene_code_block, 720, 340, true },
        { "tree-table", scene_tree_table, 760, 576, true }, { "path-bar", scene_path_bar, 680, 330, true },
        { "json-view", scene_json_view, 560, 330, true }, { "markdown", scene_markdown, 700, 850, true },
        { "code-editor", scene_code_editor, 760, 446, true },
        { "diff-view", scene_diff_view, 900, 480, true },
        { "inspector", scene_inspector, 520, 640, true }, { "log-console", scene_log_console, 820, 440, true }, { "terminal", scene_terminal, 820, 420, true }, { "emoji-picker", scene_emoji_picker, 720, 470, true }, { "chat", scene_chat, 560, 440, true },
        { "hex-viewer", scene_hex_viewer, 760, 420, true },
        { "curve-editor", scene_curve_editor, 660, 400, true }, { "gradient-editor", scene_gradient_editor, 600, 210, true },
        { "timeline-editor", scene_timeline_editor, 960, 660, true },
        { "comments", scene_comments, 600, 370, true }, { "presence-ticker", scene_presence_ticker, 640, 170, true },
        { "carousel", scene_carousel, 700, 300, true }, { "compare-stack", scene_compare_stack, 860, 320, true },
        { "kanban", scene_kanban, 920, 480, true }, { "week-calendar", scene_week_calendar, 960, 520, true },
        { "gantt", scene_gantt, 1000, 360, true }, { "node-editor", scene_node_editor, 1000, 540, true },
        { "flame-graph", scene_flame, 1000, 300, true },
        { "regex-tester", scene_dev2_regex, 760, 330, true }, { "test-explorer", scene_dev2_tests, 800, 560, true }, { "stack-trace", scene_dev2_stack, 800, 400, true }, { "request-bar", scene_dev2_request, 800, 270, true }, { "uptime", scene_dev2_uptime, 900, 380, true }, { "time-histogram", scene_dev2_histogram, 940, 250, true }, { "coverage", scene_dev2_coverage, 860, 430, true }, { "blame", scene_dev2_blame, 940, 380, true }, { "feature-flags", scene_dev2_flags, 900, 320, true }, { "api-explorer", scene_dev2_api, 860, 510, true },
        { "kv-editor", scene_dev_kv, 720, 300, true }, { "problems", scene_dev_problems, 800, 410, true }, { "pipeline", scene_dev_pipeline, 960, 240, true }, { "query-builder", scene_dev_query, 760, 330, true }, { "schedule-picker", scene_dev_schedule, 600, 270, true }, { "keymap", scene_dev_keymap, 960, 420, true }, { "commit-graph", scene_dev_commits, 960, 380, true }, { "graph-view", scene_dev_graph, 1000, 400, true }, { "waterfall", scene_dev_waterfall, 1000, 420, true }, { "trace-view", scene_dev_trace, 1000, 380, true },
        { "heatmap", scene_heatmap, 900, 330, true }, { "calendar-heatmap", scene_calendar_heatmap, 880, 220, true },
        { "treemap-radar", scene_treemap_radar, 1000, 370, true }, { "funnel-split", scene_funnel_split, 1000, 330, true },
        { "month-calendar", scene_month_calendar, 880, 720, true }, { "file-browser", scene_file_browser, 900, 460, true }, { "image-viewer", scene_image_viewer, 900, 380, true }, { "tour", scene_tour, 820, 420, true }, { "gauges", scene_gauges, 760, 250, true }, { "candlestick", scene_candlestick, 860, 400, true }, { "sankey", scene_sankey, 900, 360, true }, { "bar-list-rings", scene_bar_list_rings, 860, 260, true },
        { "scatter", scene_scatter, 860, 360, true },
        { "text-effects", scene_text_effects, 760, 220, true }, { "loaders", scene_loaders, 520, 110, true },
        { "dock", scene_dock, 760, 200, true }, { "tiles", scene_tiles, 360, 360, true },
        { "player", scene_player, 820, 300, true },
        { "reasoning-tools", scene_reasoning_tools, 680, 640, true }, { "agent-plan", scene_agent_plan, 820, 340, true },
        { "sources", scene_sources, 680, 380, true }, { "composer", scene_composer, 640, 190, true }, { "model-picker", scene_model_picker, 380, 300, true },
        { "voice-orb", scene_voice_orb, 760, 230, true }, { "suggestions", scene_suggestions, 640, 260, true },
        { "voice-wave", scene_voice_wave, 620, 290, true, false, CUI_ACCENT_GREEN }, { "voice-recorder", scene_voice_recorder, 620, 270, true, false, CUI_ACCENT_GREEN },
        { "voice-note", scene_voice_note, 620, 190, true, false, CUI_ACCENT_GREEN }, { "voice-session", scene_voice_session, 660, 290, true, false, CUI_ACCENT_GREEN },
        { "app-chrome", scene_app_chrome, 900, 440, true }, { "menu-bar", scene_menu_bar, 640, 360, true },
        { "docking", scene_docking, 960, 560, true },
        { "toolbar", scene_toolbar, 640, 370, true }, { "status-bar", scene_status_bar, 820, 160, true },
        { "document-tabs", scene_document_tabs, 720, 230, true },
        { "dot-chart", scene_dot_chart, 900, 280, true, false, CUI_ACCENT_BLUE }, { "chart-annotations", scene_chart_annotations, 860, 300, true, false, CUI_ACCENT_BLUE },
        { "event-track", scene_event_track, 760, 130, true, false, CUI_ACCENT_BLUE }, { "readouts", scene_readouts, 860, 160, true, false, CUI_ACCENT_BLUE },
        { "dot-display", scene_dot_display, 420, 200, true, false, CUI_ACCENT_BLUE },
        { "grouped-toolbar", scene_grouped_toolbar, 620, 220, true }, { "format-bar", scene_format_bar, 640, 330, true, false, CUI_ACCENT_GREEN },
        { "live-cursors", scene_live_cursors, 760, 416, true }, { "canvas-controls", scene_canvas_controls, 760, 376, true },
        { "result-card", scene_result_card, 440, 560, true }, { "veil", scene_veil, 480, 330, true }, { "selection-bar", scene_selection_bar, 600, 400, true, false, CUI_ACCENT_GREEN },
        { "tw-studio", scene_tw_studio, 1200, 675, true, false, CUI_ACCENT_NEUTRAL }, { "tw-audio", scene_tw_audio, 960, 540, true, false, CUI_ACCENT_NEUTRAL },
        { "tw-markets", scene_tw_markets, 1200, 675, true, false, CUI_ACCENT_NEUTRAL }, { "tw-calendar", scene_tw_calendar, 1200, 675, true, false, CUI_ACCENT_NEUTRAL },
        { "status-tag", scene_status_tag, 620, 270, true }, { "ratio-slider", scene_ratio_slider, 620, 180, true },
        { "gradient-slider", scene_gradient_slider, 680, 250, true }, { "step-slider", scene_step_slider, 680, 150, true },
        { "palette-strip", scene_palette_strip, 680, 150, true }, { "aspect-picker", scene_aspect_picker, 680, 160, true },
        { "option-action", scene_option_action, 680, 150, true }, { "seed-field", scene_seed_field, 680, 240, true },
    };
    cui_sw_renderer *sw = cui_sw_create();
    if (argc <= 2 || strcmp(argv[2], "button-states") == 0) { render_states(dir, false, sw); render_states(dir, true, sw); }
    for (size_t i = 0; i < sizeof scenes / sizeof scenes[0]; i++) {
        if (argc > 2 && strcmp(argv[2], scenes[i].name) != 0) continue;   /* optional: one scene */
        if (!scenes[i].dark_only) render(dir, &scenes[i], false, sw);
        if (scenes[i].dark_too) render(dir, &scenes[i], true, sw);
    }
    cui_sw_destroy(sw);
    return 0;
}
