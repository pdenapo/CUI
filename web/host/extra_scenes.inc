/* Web-only scenes for documented components that have no scene in
 * tests/component_shots.c, plus a few "demo" scenes that re-trigger
 * one-shot components (toasts, dialogs) with a button. Included after
 * component_shots.c, so SCENE() and frame_wrap() are available. */

SCENE(text) {
    cui_column(ui, .gap = 10, UPTO(520)) {
        cui_heading(ui, "Release notes", 1);
        cui_heading(ui, "Version 1.4", 3);
        cui_text(ui, "Body text wraps to the available width. Tables keep their column widths when you sort, and the filter bar remembers the last query.");
        cui_secondary(ui, "Supporting text in the secondary color.");
        cui_muted(ui, "Small, muted text for timestamps and hints.");
        cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .wrap = true) {
            cui_label(ui, "Medium weight label");
            cui_textf(ui, "%d unread", 12);
            cui_code(ui, "cui_button()");
            cui_link(ui, "Read the docs");
            cui_icon(ui, CUI_ICON_ZAP, 16, cui_color_of(ui, CUI_COLOR_ACCENT));
        }
    }
}

SCENE(icon_button) {
    static bool playing = false, copied = false;
    cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_icon_button(ui, CUI_ICON_TRASH_2, .tooltip = "Delete", .shortcut = "Del");
        cui_icon_button(ui, CUI_ICON_PENCIL, .tooltip = "Rename", .shortcut = "F2");
        cui_icon_button(ui, CUI_ICON_SHARE_2, .tooltip = "Share");
        cui_separator(ui);
        if (cui_icon_button(ui, playing ? CUI_ICON_PAUSE : CUI_ICON_PLAY, .tooltip = playing ? "Pause" : "Play")) playing = !playing;
        if (cui_icon_button(ui, copied ? CUI_ICON_CHECK : CUI_ICON_COPY, .tooltip = copied ? "Copied" : "Copy")) copied = !copied;
    }
}

SCENE(color_picker) {
    static cui_color brand = { 0x77, 0x47, 0xff, 255 };
    cui_column(ui, .gap = 6, .width = CUI_PX(240)) {
        cui_label(ui, "Brand color");
        cui_color_picker(ui, "brand", &brand);
    }
}

static bool pop_active = true, pop_archived = false;
static void scene_popover(cui_ctx *ui, int f) {
    if (f < 0) {
        f = -f - 1;
        if (f == 3) cui_input_mouse_move(ui, 70, 46);
        if (f == 4) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
        if (f == 5) cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
        return;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_START) {
        if (cui_button(ui, "Filters", .icon = CUI_ICON_FILTER)) cui_popover_toggle(ui, "filters");
        cui_popover(ui, "filters", .width = 280) {
            cui_label(ui, "Show");
            cui_checkbox(ui, "Only active projects", &pop_active);
            cui_checkbox(ui, "Include archived", &pop_archived);
            cui_row(ui, .gap = 8, .justify = CUI_JUSTIFY_END) {
                if (cui_button(ui, "Reset", .variant = CUI_BUTTON_GHOST, .size = CUI_SIZE_SM)) { pop_active = true; pop_archived = false; }
                if (cui_button(ui, "Apply", .variant = CUI_BUTTON_PRIMARY, .size = CUI_SIZE_SM)) cui_popover_close(ui, "filters");
            }
        }
    }
}

static void scene_sheet(cui_ctx *ui, int f) {
    static bool open = true, comments = true, mentions = true, digest = false;
    if (f < 0) return;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 10) {
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .wrap = true) {
            cui_heading(ui, "Inbox", 3);
            cui_spacer(ui);
            if (cui_button(ui, "Notifications", .icon = CUI_ICON_BELL)) open = true;
        }
        cui_skeleton(ui, CUI_PCT(70), 14); cui_skeleton(ui, CUI_PCT(55), 14); cui_skeleton(ui, CUI_PCT(62), 14);
    }
    cui_sheet(ui, "Notifications", &open, .description = "Choose what we tell you about.") {
        cui_switch(ui, "Comments", &comments, .description = "When someone replies to you.");
        cui_switch(ui, "Mentions", &mentions, .description = "When someone mentions you.");
        cui_switch(ui, "Weekly digest", &digest, .description = "A summary every Monday.");
        cui_separator(ui);
        cui_label(ui, "Recent");
        cui_list_item(ui, "Jonas commented on Release 1.0", .avatar = "Jonas Berg", .subtitle = "2 minutes ago");
        cui_list_item(ui, "Priya invited you to Engines", .avatar = "Priya Nair", .subtitle = "1 hour ago");
    }
}

static void web_landscape(unsigned char *px, int w, int h) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float fx = (float)x / w, fy = (float)y / h;
            float r = 0.56f + fy * 0.34f, g = 0.72f + fy * 0.2f, b = 0.88f + fy * 0.06f;
            float sun = (fx - 0.72f) * (fx - 0.72f) * 15.4f + (fy - 0.24f) * (fy - 0.24f) * 6.0f;
            if (sun < 0.024f) { r = 1.0f; g = 0.97f; b = 0.86f; }
            float m1 = 0.5f + 0.1f * sinf(fx * 6.0f + 0.6f) + 0.03f * sinf(fx * 23.0f);
            float m2 = 0.68f + 0.06f * sinf(fx * 4.0f + 2.1f) + 0.02f * sinf(fx * 31.0f);
            if (fy > m1) { r = 0.5f; g = 0.6f; b = 0.62f - fy * 0.08f; }
            if (fy > m2) { r = 0.3f - (fy - m2) * 0.2f; g = 0.45f - (fy - m2) * 0.25f; b = 0.3f; }
            unsigned char *p = px + (y * w + x) * 4;
            p[0] = (unsigned char)(fminf(1, fmaxf(0, r)) * 255); p[1] = (unsigned char)(fminf(1, fmaxf(0, g)) * 255);
            p[2] = (unsigned char)(fminf(1, fmaxf(0, b)) * 255); p[3] = 255;
        }
}
SCENE(image) {
    static cui_texture tex;
    bool *made = cui_state(ui, cui_make_id(ui, "web-image-tex"), sizeof *made);
    if (!*made) {
        *made = true;
        static unsigned char px[480 * 300 * 4];
        web_landscape(px, 480, 300);
        tex = cui_image_create(ui, 480, 300, px);
    }
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_END, .wrap = true) {
        cui_image(ui, tex, .width = CUI_PX(240), .height = CUI_PX(150), .radius = 12);
        cui_image(ui, tex, .width = CUI_PX(120), .height = CUI_PX(75), .radius = 8);
        cui_image(ui, tex, .width = CUI_PX(64), .height = CUI_PX(64), .radius = 32);
    }
}

SCENE(reorder) {
    static int order[5] = { 0, 1, 2, 3, 4 };
    static const char *tasks[5] = { "Write the release notes", "Record the demo video", "Update the screenshots", "Tag version 1.0", "Post the announcement" };
    static const int tones[5] = { CUI_TONE_INFO, CUI_TONE_NEUTRAL, CUI_TONE_INFO, CUI_TONE_WARNING, CUI_TONE_TEAL };
    static const char *labels[5] = { "Docs", "Video", "Docs", "Release", "Blog" };
    cui_column(ui, UPTO(440)) {
        cui_reorder_begin(ui, "tasks", order, 5);
        for (int i = 0; i < 5; i++) {
            cui_reorder_item(ui, i) {
                int t = order[i];
                cui_text(ui, tasks[t], .nowrap = true, .width = CUI_FILL);
                cui_badge(ui, labels[t], .tone = (cui_tone)tones[t]);
            }
        }
        if (cui_reorder_end(ui)) cui_toast(ui, "Order saved");
    }
}

SCENE(confirm_button) {
    static int deleted;
    cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .wrap = true) {
        if (cui_confirm_button(ui, "Delete project", .icon = CUI_ICON_TRASH_2)) deleted++;
        if (cui_confirm_button(ui, "Revoke key", .variant = CUI_BUTTON_OUTLINE, .confirm_label = "Revoke for everyone", .timeout = 5)) deleted++;
        if (deleted) cui_muted(ui, deleted == 1 ? "Confirmed once" : "Confirmed again");
    }
}

SCENE(reactions) {
    static cui_reaction re[4] = { { CUI_ICON_THUMBS_UP, 12, true }, { CUI_ICON_HEART, 4, false }, { CUI_ICON_ROCKET, 7, false }, { CUI_ICON_SMILE, 2, false } };
    cui_reactions(ui, "reactions", re, 4);
}

static void scene_speed_dial(cui_ctx *ui, int f) {
    static int choice = -1;
    static const cui_speed_action acts[] = { { CUI_ICON_FILE_TEXT, "New note" }, { CUI_ICON_IMAGE, "Upload image" }, { CUI_ICON_MIC, "Record memo" } };
    if (f < 0) return;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .align = CUI_ALIGN_END, .justify = CUI_JUSTIFY_END) {
        cui_row(ui, .gap = 16, .align = CUI_ALIGN_CENTER, .wrap = true) {
            if (choice >= 0) cui_muted(ui, acts[choice].label);
            int c = cui_speed_dial(ui, "dial", acts, 3, .tooltip = "Create");
            if (c >= 0) choice = c;
        }
    }
}

SCENE(theme_switcher) {
    static int mode = -1;
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_CENTER, .wrap = true) {
        cui_theme_switcher(ui, &mode, true);
        cui_theme_toggle(ui);
    }
}

SCENE(swipe_actions) {
    static const cui_swipe_action left[] = { { CUI_ICON_CHECK, "Done", CUI_TONE_SUCCESS } };
    static const cui_swipe_action right[] = { { CUI_ICON_ARCHIVE, "Archive", CUI_TONE_INFO }, { CUI_ICON_TRASH_2, "Delete", CUI_TONE_DANGER } };
    static const char *titles[] = { "Review the design tokens", "Reply to Jonas about the release", "Book the offsite venue" };
    cui_card(ui, .pad = 6, .gap = 6, UPTO(480)) {
        for (int i = 0; i < 3; i++) {
            cui_push_id_int(ui, i);
            cui_swipe_item_begin(ui, "row", left, 1, right, 2);
            cui_icon(ui, CUI_ICON_CIRCLE, 16, cui_color_of(ui, CUI_COLOR_TEXT_MUTED));
            cui_text(ui, titles[i], .width = CUI_FILL, .nowrap = true);
            cui_text(ui, "Today", .size = 12, .color = CUI_COLOR_TEXT_MUTED);
            int a = cui_swipe_item_end(ui);
            if (a >= 0) cui_toast(ui, a == 0 ? "Marked as done" : a == 100 ? "Archived" : "Deleted", .tone = a == 101 ? CUI_TONE_DANGER : CUI_TONE_SUCCESS);
            cui_pop_id(ui);
        }
    }
}

SCENE(money_field) {
    static double amount = 1249.5, salary = 64000;
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_START, .wrap = true) {
        cui_money_input(ui, "amount", &amount, .label = "Amount", .code = "USD", .width = CUI_PX(240));
        cui_money_input(ui, "salary", &salary, .label = "Salary", .symbol = "\xe2\x82\xac", .code = "EUR", .decimals = -1, .width = CUI_PX(240), .hint = "Whole euros, before tax.");
    }
}

SCENE(prefix_suffix) {
    static char site[64] = "fieldwork.studio", weight[16] = "4.2";
    cui_row(ui, .gap = 16, .align = CUI_ALIGN_START, .wrap = true) {
        cui_input(ui, "site", site, sizeof site, .label = "Website", .prefix = "https://", .width = CUI_PX(cui_viewport_size(ui).x < 360 ? 240 : 280));
        cui_input(ui, "weight", weight, sizeof weight, .label = "Parcel weight", .suffix = "kg", .width = CUI_PX(160));
    }
}

SCENE(rolling_numbers) {
    static int revenue = 48210, step = 0;
    static const int deltas[] = { 1290, -340, 8800, -12, 40, -2210, 99999, -99999 };
    char buf[32], grouped[32];
    snprintf(buf, sizeof buf, "%d", revenue);
    /* thousands separators */
    int n = (int)strlen(buf), o = 0;
    for (int i = 0; i < n; i++) { grouped[o++] = buf[i]; if ((n - i - 1) % 3 == 0 && i < n - 1) grouped[o++] = ','; }
    grouped[o] = 0;
    snprintf(buf, sizeof buf, "$%s", grouped);
    cui_column(ui, .gap = 14) {
        cui_number_roll(ui, "revenue", buf, .font = CUI_FONT_DISPLAY, .size = 34);
        cui_row(ui, .gap = 8) {
            if (cui_button(ui, "Next change", .icon = CUI_ICON_REFRESH_CW, .size = CUI_SIZE_SM)) { revenue += deltas[step % 8]; step++; if (revenue < 0) revenue = 0; }
            if (cui_button(ui, "+1", .size = CUI_SIZE_SM)) revenue += 1;
            if (cui_button(ui, "-1", .size = CUI_SIZE_SM)) revenue = revenue > 0 ? revenue - 1 : 0;
        }
    }
}

SCENE(toast_demo) {
    cui_row(ui, .gap = 8, .wrap = true) {
        if (cui_button(ui, "Success", .icon = CUI_ICON_CHECK)) cui_toast(ui, "Changes saved", .description = "Your profile is up to date.", .tone = CUI_TONE_SUCCESS);
        if (cui_button(ui, "Error", .icon = CUI_ICON_X)) cui_toast(ui, "Upload failed", .description = "The file is larger than 25 MB.", .tone = CUI_TONE_DANGER);
        if (cui_button(ui, "With action", .icon = CUI_ICON_UNDO_2)) cui_toast(ui, "Project deleted", .tone = CUI_TONE_DANGER, .action = "Undo");
        if (cui_button(ui, "Plain")) cui_toast(ui, "Link copied");
    }
}

SCENE(dialog_demo) {
    static bool open = false;
    if (cui_button(ui, "Delete project", .variant = CUI_BUTTON_DANGER, .icon = CUI_ICON_TRASH_2)) open = true;
    cui_dialog(ui, "Delete project?", &open, .description = "This removes the project for everyone. You cannot undo this.", .width = 380) {
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Cancel")) open = false;
            if (cui_button(ui, "Delete project", .variant = CUI_BUTTON_DANGER)) { open = false; cui_toast(ui, "Project deleted", .tone = CUI_TONE_DANGER); }
        }
    }
}

/* The doc scene ignores drops (it only poses a drag for the screenshot);
 * this one applies cui_kanban_end's move, like the gallery app does. */
typedef struct { int id; const char *title, *tag, *who; cui_tone tone; int comments; float progress; } web_kb_card;
static web_kb_card web_kb_cards[] = {
    { 1, "Audit the color tokens for contrast", "Design", "Maya Okafor", CUI_TONE_VIOLET, 3, 0 },
    { 2, "Virtualize the activity feed", "Perf", "Jonas Berg", CUI_TONE_TEAL, 2, 0 },
    { 3, "Dark mode screenshots", "Docs", "Aiko Tanaka", CUI_TONE_ROSE, 3, 0 },
    { 4, "Keyboard nav in the date range picker", "A11y", "Priya Nair", CUI_TONE_INFO, 3, 0.6f },
    { 5, "IME composition on Windows", "Platform", "Daniel Mensah", CUI_TONE_WARNING, 2, 0.3f },
    { 6, "Spring presets", "Motion", "Rosa Delgado", CUI_TONE_VIOLET, 3, 1 },
};
static int web_kb_cols[3][6] = { { 1, 2, 3 }, { 4, 5 }, { 6 } };
static int web_kb_len[3] = { 3, 2, 1 };
static void web_kb_apply(const cui_kanban_move *m) {
    int *from = web_kb_cols[m->from_column], *to = web_kb_cols[m->to_column];
    int at = -1;
    for (int k = 0; k < web_kb_len[m->from_column]; k++) if (from[k] == m->card) at = k;
    if (at < 0) return;
    for (int k = at; k < web_kb_len[m->from_column] - 1; k++) from[k] = from[k + 1];
    web_kb_len[m->from_column]--;
    int idx = m->to_index < 0 ? 0 : m->to_index > web_kb_len[m->to_column] ? web_kb_len[m->to_column] : m->to_index;
    for (int k = web_kb_len[m->to_column]; k > idx; k--) to[k] = to[k - 1];
    to[idx] = m->card;
    web_kb_len[m->to_column]++;
}
SCENE(kanban_live) {
    static const char *names[3] = { "Backlog", "In progress", "Done" };
    static const unsigned dots[3] = { 0x94a3b8, 0x3b82f6, 0x22c55e };
    cui_kanban_begin(ui, "board");
    for (int c = 0; c < 3; c++) {
        cui_kanban_column_begin(ui, names[c], web_kb_len[c], .dot = CUI_RGB(dots[c]), .limit = c == 1 ? 3 : 0, .width = 264);
        for (int k = 0; k < web_kb_len[c]; k++) {
            web_kb_card *cd = NULL;
            for (int j = 0; j < 6; j++) if (web_kb_cards[j].id == web_kb_cols[c][k]) cd = &web_kb_cards[j];
            if (!cd) continue;
            cui_kanban_card_begin(ui, cd->id);
            cui_badge(ui, cd->tag, .tone = cd->tone, .size = CUI_SIZE_SM);
            cui_text(ui, cd->title, .font = CUI_FONT_MEDIUM, .size = 13.5f, .line_height = 1.35f);
            if (cd->progress > 0 && cd->progress < 1) cui_progress(ui, cd->progress, .height = 4);
            cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                cui_avatar(ui, cd->who, .size = 22);
                cui_spacer(ui);
                cui_icon(ui, CUI_ICON_MESSAGE_SQUARE, 13, cui_color_of(ui, CUI_COLOR_TEXT_MUTED));
                cui_textf(ui, "%d", cd->comments);
            }
            cui_kanban_card_end(ui);
        }
        cui_kanban_column_end(ui);
    }
    cui_kanban_move mv;
    if (cui_kanban_end(ui, &mv)) web_kb_apply(&mv);
}

/* voice, live: record with the composer's microphone (a pretend one), send the take, and
 * it lands above as a voice note that plays back; the page moves the position itself */
static void scene_voice_demo(cui_ctx *ui, int f) {
    static char draft[128] = "";
    static bool recording = false;
    static float take[400], kept[4][64];
    static int take_n = 0, note_n = 0, last_step = -1;
    static cui_voice_note notes[4];
    static double last = -1;
    if (f < 0) return;
    double now = cui_time(ui);
    float dt = last < 0 ? 0 : (float)(now - last);
    last = now;
    int step = voice_step(ui);
    if (recording && step != last_step && take_n < 400) take[take_n++] = 0.85f * voice_level(step * VOICE_STEP * 1.1f, 1.7f);
    last_step = step;
    if (recording) cui_request_wakeup(ui, VOICE_STEP);
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = frame_pad(ui), .gap = 10, .justify = CUI_JUSTIFY_END) {
        if (!note_n) cui_text(ui, "Press the microphone, then send.", .size = 13, .color = CUI_COLOR_TEXT_MUTED);
        for (int k = 0; k < note_n; k++) {
            cui_voice_note *n = &notes[k];
            if (n->playing) {
                n->position += dt * (n->speed > 0 ? n->speed : 1);
                if (n->position >= n->duration) { n->position = 0; n->playing = false; }
                cui_request_wakeup(ui, 1.0f / 30.0f);
            }
            cui_push_id_int(ui, k);
            cui_voice_note(ui, "note", n, .mine = true);
            cui_pop_id(ui);
        }
        int ev = cui_chat_composer(ui, "composer", draft, sizeof draft, .placeholder = "Message", .mic = true, .recording = recording,
                                   .levels = take, .level_count = take_n, .record_seconds = take_n * VOICE_STEP);
        if (ev & CUI_COMPOSER_RECORD) { recording = true; take_n = 0; }
        if (ev & CUI_COMPOSER_CANCEL) recording = false;
        if (ev & CUI_COMPOSER_VOICE) {
            recording = false;
            int k = note_n < 4 ? note_n++ : 3;
            cui_voice_resample(take, take_n, kept[k], 64);
            notes[k] = (cui_voice_note){ kept[k], 64, 0, take_n * VOICE_STEP, 1, false };
        }
        if (ev & CUI_COMPOSER_SEND) draft[0] = 0;
    }
}

/* docking, live: the doc scene's warm-up holds a tab mid-drag for its screenshot;
 * this variant skips the warm-up, so the page starts with nothing in hand */
static void scene_docking_live(cui_ctx *ui, int f) {
    if (f < 0) return;
    scene_docking(ui, f);
}

#define WEB_EXTRA_SCENES \
    { "docking-live", scene_docking_live, 960, 560, true }, \
    { "text", scene_text, 640, 280, true }, { "icon-button", scene_icon_button, 420, 150, true }, \
    { "color-picker", scene_color_picker, 380, 420, true }, { "popover", scene_popover, 420, 300, true }, \
    { "sheet", scene_sheet, 820, 460, true }, { "image", scene_image, 520, 220, true }, \
    { "reorder", scene_reorder, 520, 330, true }, { "confirm-button", scene_confirm_button, 640, 120, true }, \
    { "reactions", scene_reactions, 480, 110, true }, { "speed-dial", scene_speed_dial, 360, 300, true }, \
    { "theme-switcher", scene_theme_switcher, 420, 110, true }, { "swipe-actions", scene_swipe_actions, 560, 240, true }, \
    { "money-field", scene_money_field, 580, 150, true }, { "prefix-suffix", scene_prefix_suffix, 540, 130, true }, \
    { "rolling-numbers", scene_rolling_numbers, 420, 170, true }, { "toast-demo", scene_toast_demo, 600, 300, true }, \
    { "dialog-demo", scene_dialog_demo, 620, 340, true }, { "kanban-live", scene_kanban_live, 920, 480, true }, \
    { "voice-demo", scene_voice_demo, 620, 460, true, false, CUI_ACCENT_GREEN },
