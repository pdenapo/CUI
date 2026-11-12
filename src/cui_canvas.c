/* Canvas and bars: the format bar, live cursors with the presence pill and the
 * selection ring, canvas controls, the result card and the selection bar.
 * The format bar is a floating toolbar (cui_chrome.c) with a few items of its
 * own; the overlays (cursors, rings, the zoom rail) are absolute nodes laid
 * over the previous component, painted after its siblings (CUI_NF_OVERLAY). */
#include "cui_internal.h"

/* ------------------------------------------------------------------------ */
/* Shared pieces                                                            */
/* ------------------------------------------------------------------------ */

/* Glyphs the icon font does not carry: the four alignments and "fit", drawn
 * on a 24 point grid like the Lucide icons beside them. */
typedef enum { CUI__CV_ALIGN_LEFT, CUI__CV_ALIGN_CENTER, CUI__CV_ALIGN_RIGHT, CUI__CV_ALIGN_JUSTIFY, CUI__CV_FIT } cui__cv_glyph_kind;
typedef struct { int anchor; int kind; float size; cui_color c; } cui__cv_glyph;

static void cui__paint_cv_glyph(cui_ctx *ui, cui_node *n, void *data) {
    (void)n;
    cui__cv_glyph *p = data;
    const cui_node *a = &ui->nodes[p->anchor];
    float s = p->size / 24.0f;
    float ox = roundf(a->rect.x + a->rect.w * 0.5f) - 12 * s, oy = roundf(a->rect.y + a->rect.h * 0.5f) - 12 * s;
    float t = CUI_MAX(1.5f, 1.75f * s);
    cui_color c = p->c;
    c.a = (uint8_t)(c.a * a->opacity);
    if (p->kind == CUI__CV_FIT) {
        /* four corner brackets */
        static const float k[4][2] = { { 3, 3 }, { 21, 3 }, { 3, 21 }, { 21, 21 } };
        for (int i = 0; i < 4; i++) {
            float cx = ox + k[i][0] * s, cy = oy + k[i][1] * s, dx = (k[i][0] < 12 ? 5 : -5) * s, dy = (k[i][1] < 12 ? 5 : -5) * s;
            cui_draw_rect(ui, (cui_rect){ CUI_MIN(cx, cx + dx) - t * 0.5f, cy - t * 0.5f, fabsf(dx) + t, t }, CUI_CORNERS(t * 0.5f), c);
            cui_draw_rect(ui, (cui_rect){ cx - t * 0.5f, CUI_MIN(cy, cy + dy) - t * 0.5f, t, fabsf(dy) + t }, CUI_CORNERS(t * 0.5f), c);
        }
        return;
    }
    static const float spans[4][3][2] = {
        { { 3, 21 }, { 3, 15 }, { 3, 17 } }, { { 3, 21 }, { 7, 17 }, { 5, 19 } },
        { { 3, 21 }, { 9, 21 }, { 7, 21 } }, { { 3, 21 }, { 3, 21 }, { 3, 21 } },
    };
    for (int row = 0; row < 3; row++) {
        float x0 = ox + spans[p->kind][row][0] * s, x1 = ox + spans[p->kind][row][1] * s, y = oy + (6 + 6 * row) * s;
        cui_draw_rect(ui, (cui_rect){ x0 - t * 0.5f, y - t * 0.5f, x1 - x0 + t, t }, CUI_CORNERS(t * 0.5f), c);
    }
}

/* lay a glyph over node `anchor` (a button built with an empty icon) */
static void cui__cv_glyph_on(cui_ctx *ui, int anchor, int kind, float size, cui_color c) {
    int m = cui__absolute(ui, 0, 0, 0, 0, 0);
    ui->nodes[m].abs_anchor = anchor;
    ui->nodes[m].flags |= CUI_NF_OVERLAY;
    cui__cv_glyph *p = CUI_PAINT_DATA(ui, cui__cv_glyph);
    p->anchor = anchor; p->kind = kind; p->size = size; p->c = c;
    ui->nodes[m].paint = cui__paint_cv_glyph;
    ui->nodes[m].paint_data = p;
    ui->last_node = anchor;
}

/* text that reads on a person's color: dark on the light tones of a dark theme, light on the deeper ones */
static cui_color cui__cv_ink(cui_color bg) { return cui__legible(cui__rgba(255, 255, 255, 255), bg, 4.5f); }

/* an overlay node over the previous component, painted after its siblings; returns -1 without one */
static int cui__cv_overlay(cui_ctx *ui, cui__paint_fn paint, void *data) {
    int anchor = ui->last_node;
    if (anchor < 0 || anchor >= ui->node_count) return -1;
    int m = cui__absolute(ui, 0, 0, 0, 0, 0);
    ui->nodes[m].abs_anchor = anchor;
    ui->nodes[m].flags |= CUI_NF_OVERLAY;
    ui->nodes[m].paint = paint;
    ui->nodes[m].paint_data = data;
    ui->last_node = anchor;
    return m;
}

/* Popups opened from a trigger built here, the way menus open: a click
 * toggles, Down opens from the keyboard, and Enter on an open trigger is left
 * to the popup. Returns true on the frame it opened. */
static bool cui__cv_popup_trigger(cui_ctx *ui, cui_id pid, cui_id anchor, bool clicked) {
    cui_popup *p = cui__popup_get(ui, pid, true);
    if (!p) return false;
    bool was = p->open;
    bool keys = ui->focus == anchor && ui->focus_visible;
    bool key_activate = keys && (cui_key_pressed(ui, CUI_KEY_ENTER) || cui_key_pressed(ui, CUI_KEY_SPACE));
    if (clicked && !(p->open && key_activate)) cui__popup_set(ui, pid, !p->open);
    if (!p->open && keys && cui_key_pressed(ui, CUI_KEY_DOWN)) cui__popup_set(ui, pid, true);
    if (p->open && !was) {
        /* the press that opened it does not also pick what starts highlighted inside */
        ui->kpressed[CUI_KEY_ENTER] = 0;
        ui->kpressed[CUI_KEY_SPACE] = 0;
        ui->kpressed[CUI_KEY_DOWN] = 0;
    }
    return p->open && !was;
}

/* A flyout closed by Esc or a click outside gives keyboard focus back to its trigger. */
static void cui__cv_popup_refocus(cui_ctx *ui, cui_id pid, cui_id trigger, const cui_id *inside, int n) {
    cui_popup *p = cui__popup_get(ui, pid, false);
    if (!p || p->open) return;
    for (int k = 0; k < n; k++) if (inside[k] && ui->focus == inside[k]) { ui->focus = trigger; return; }
}

/* ------------------------------------------------------------------------ */
/* Format bar                                                               */
/* ------------------------------------------------------------------------ */

static const cui_text_style cui__fmt_default_styles[] = {
    { "Header", 44, 17, CUI_FONT_MEDIUM }, { "Title", 56, 22, CUI_FONT_MEDIUM },
    { "Subtitle", 28, 15, CUI_FONT_MEDIUM }, { "Normal text", 16, 13, CUI_FONT_BODY },
};
static const cui_tone cui__fmt_default_colors[] = {
    CUI_TONE_NEUTRAL, CUI_TONE_ROSE, CUI_TONE_DANGER, CUI_TONE_WARNING,
    CUI_TONE_SUCCESS, CUI_TONE_TEAL, CUI_TONE_INFO, CUI_TONE_VIOLET,
};
#define CUI__FMT_STYLE_MIN_W 118.0f
#define CUI__FMT_SIZE_W 40.0f

typedef struct { float hover, focus; } cui__fmt_trigger_state;
typedef struct { float chevron; } cui__fmt_state;

/* everything the format bar's phases share */
typedef struct {
    cui_ctx *ui;
    cui_id id;                 /* the bar, inside the toolbar's id scope from here on */
    cui_text_format *f;
    const cui_format_bar_opts *o;
    const cui_text_style *styles;
    int nstyles;
    const cui_tone *colors;
    int ncolors;
    cui__fmt_state *st;
    int result;
} cui__fmt_frame;

static cui_color cui__fmt_tone(cui_ctx *ui, cui_tone t) {
    return t == CUI_TONE_NEUTRAL ? CUI_C(ui, FOREGROUND) : cui__tone_color(ui, t);
}

/* A ghost trigger built by hand (a swatch, the style name): the node is left
 * open for its content. */
static int cui__fmt_trigger_begin(cui_ctx *ui, cui_id id, bool open, float h, float pad_l, float pad_r, bool *clicked) {
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    cui__fmt_trigger_state *ts = cui_state(ui, id, sizeof *ts);
    cui__tween(ui, &ts->hover, r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &ts->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    int i = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[i];
    n->dir = CUI_ROW; n->align = CUI_ALIGN_CENTER; n->gap = 4;
    n->h = CUI_PX(h);
    n->pad[1] = pad_r; n->pad[3] = pad_l;
    n->self_align = CUI_ALIGN_CENTER;
    n->radius = CUI_CORNERS(CUI__R_CONTROL(ui, h));
    n->bg = open ? CUI_C(ui, ACTIVE) : cui_alpha(CUI_C(ui, HOVER), ts->hover);
    cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
    if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__focus_ring(ui, i, ts->focus);
    cui__open(ui, i);
    *clicked = r.clicked;
    return i;
}

static void cui__fmt_trigger_end(cui_ctx *ui, cui_id id, int node, const char *tooltip) {
    cui__close(ui);
    ui->last_id = id;
    ui->last_node = node;
    if (tooltip) cui_tooltip(ui, tooltip);
}

/* the outer ring of a swatch in the color grid: the selection in the text color, or keyboard focus */
static void cui__paint_swatch_ring(cui_ctx *ui, cui_node *n, void *data) {
    const cui_node *a = &ui->nodes[n->abs_anchor];
    cui_rect r = a->rect;
    if (*(bool *)data) { cui__draw_focus_ring(ui, r, a->radius, 1.0f); return; }
    cui_draw_rect_border(ui, (cui_rect){ r.x - 1.5f, r.y - 1.5f, r.w + 3, r.h + 3 }, CUI_CORNERS(9.5f), cui__rgba(0, 0, 0, 0), CUI_C(ui, FOREGROUND), 1.5f);
}

/* the color swatch and the grid of colors it opens */
static void cui__fmt_colors(cui__fmt_frame *F) {
    cui_ctx *ui = F->ui;
    float h = ui->theme.control_md;
    if (!cui__toolbar_reserve(ui, 6 + 20 + 4 + 14 + 6)) return;
    cui_id tid = cui_make_id(ui, "#color"), pid = tid ^ 0xC0105u;
    cui_popup *p = cui__popup_get(ui, pid, false);
    bool clicked;
    int t = cui__fmt_trigger_begin(ui, tid, p && p->open, h, 6, 6, &clicked);
    int sw = cui__leaf(ui, 0, CUI_PX(20), CUI_PX(20));
    ui->nodes[sw].bg = cui__fmt_tone(ui, F->f->color);
    ui->nodes[sw].radius = CUI_CORNERS(6);
    ui->nodes[sw].border = 1;
    ui->nodes[sw].border_color = cui_alpha(CUI_C(ui, FOREGROUND), 0.12f);
    cui__icon_node(ui, CUI_ICON_CHEVRON_DOWN, 14, CUI_C(ui, TEXT_MUTED));
    cui__fmt_trigger_end(ui, tid, t, "Text color");
    bool opened = cui__cv_popup_trigger(ui, pid, tid, clicked);
    int n = CUI_MIN(F->ncolors, 32), focused = -1;
    cui_id ids[32];
    for (int k = 0; k < n; k++) {
        ids[k] = cui__hash(&k, sizeof k, pid);
        if (ui->focus == ids[k] && ui->focus_visible) focused = k;
    }
    if (!cui__popup_begin(ui, pid, tid, CUI_PLACE_BOTTOM_START, 0, 10, 0, false)) { cui__cv_popup_refocus(ui, pid, tid, ids, n); return; }
    int grid = cui__node_new(ui, 0);
    ui->nodes[grid].dir = CUI_GRID;
    ui->nodes[grid].columns = 4;
    ui->nodes[grid].gap = 6;
    cui__open(ui, grid);
    /* arrows move through the grid, a row at a time up and down */
    if (focused >= 0 && !cui__keys_covered(ui)) {
        int to = focused;
        if (cui_key_pressed(ui, CUI_KEY_RIGHT)) to++;
        if (cui_key_pressed(ui, CUI_KEY_LEFT)) to--;
        if (cui_key_pressed(ui, CUI_KEY_DOWN)) to += 4;
        if (cui_key_pressed(ui, CUI_KEY_UP)) to -= 4;
        if (to != focused && to >= 0 && to < n) ui->focus = ids[to];
    }
    for (int k = 0; k < n; k++) {
        cui_interact r = cui__interact(ui, ids[k], CUI_IF_FOCUSABLE);
        bool sel = F->colors[k] == F->f->color;
        int c = cui__leaf(ui, ids[k], CUI_PX(26), CUI_PX(26));
        cui_node *cn = &ui->nodes[c];
        cn->bg = cui__fmt_tone(ui, F->colors[k]);
        cn->radius = CUI_CORNERS(8);
        if (sel) {
            /* a ring with a gap of the panel's color, like a selected swatch elsewhere */
            cn->border = 2;
            cn->border_color = CUI_C(ui, SURFACE_RAISED);
        }
        cui__mark_interactive(ui, c, CUI_IF_FOCUSABLE);
        if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
        if (sel || r.focus_visible) {
            /* the outer ring: the selection in the text color, or keyboard focus */
            int ring = cui__absolute(ui, 0, 0, 0, 0, 0);
            ui->nodes[ring].abs_anchor = c;
            ui->nodes[ring].flags |= CUI_NF_OVERLAY;
            bool *focus = CUI_PAINT_DATA(ui, bool);
            *focus = r.focus_visible;
            ui->nodes[ring].paint_data = focus;
            ui->nodes[ring].paint = cui__paint_swatch_ring;
        }
        if (opened && sel && ui->focus_visible) ui->focus = ids[k];
        if (r.clicked && F->f->color != F->colors[k]) { F->f->color = F->colors[k]; F->result |= CUI_FORMAT_CHANGED; }
        if (r.clicked) { cui__popup_set(ui, pid, false); if (r.keyboard_activate || ui->focus_visible) ui->focus = tid; }
    }
    cui__close(ui);
    cui__popup_end(ui);
    cui__cv_popup_refocus(ui, pid, tid, ids, n);
}

/* the size readout and the style menu that previews each style at its size */
static void cui__fmt_style(cui__fmt_frame *F) {
    cui_ctx *ui = F->ui;
    cui_text_format *f = F->f;
    if (f->style < 0 || f->style >= F->nstyles) f->style = 0;
    if (cui__toolbar_reserve(ui, CUI__FMT_SIZE_W)) {
        int t = cui__text_node(ui, cui__frame_printf(ui, "%g", (double)f->size), -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body), CUI_C(ui, TEXT_SECONDARY), false);
        ui->nodes[t].w = CUI_PX(CUI__FMT_SIZE_W);
        ui->nodes[t].text_align = CUI_TEXT_CENTER;
        ui->nodes[t].flags |= CUI_NF_TABULAR;
        ui->nodes[t].line_height = 1.3f;
    }
    const char *name = F->styles[f->style].name ? F->styles[f->style].name : "";
    float fs = cui__fs(ui, ui->theme.text_lg - 1);
    float w = 12 + cui__text_width(ui, CUI_FONT_BODY, fs, name, -1, false) + 8 + 14 + 8;
    if (!cui__toolbar_reserve(ui, CUI_MAX(CUI__FMT_STYLE_MIN_W, w))) return;
    cui_id tid = cui_make_id(ui, "#style"), pid = tid ^ 0x57713u;
    cui_popup *p = cui__popup_get(ui, pid, false);
    bool open = p && p->open, clicked;
    cui__tween(ui, &F->st->chevron, open ? 1.0f : 0.0f, CUI__T_STATE);
    int t = cui__fmt_trigger_begin(ui, tid, open, ui->theme.control_md - 2, 12, 8, &clicked);
    ui->nodes[t].min_w = CUI__FMT_STYLE_MIN_W;
    ui->nodes[t].gap = 8;
    int lab = cui__text_node(ui, name, -1, CUI_FONT_BODY, fs, CUI_C(ui, FOREGROUND), false);
    ui->nodes[lab].w = CUI_FILL;
    ui->nodes[lab].line_height = 1.3f;
    cui__chevron_node(ui, F->st->chevron * CUI_PI, 14, CUI_C(ui, TEXT_MUTED));
    cui__fmt_trigger_end(ui, tid, t, NULL);
    if (cui__cv_popup_trigger(ui, pid, tid, clicked)) {
        /* the current style starts highlighted, so the keys move from it */
        cui__menu_nav *nav = cui_state(ui, pid ^ 0x4E4156u, sizeof *nav);
        nav->highlight = f->style;
        nav->keyboard = true;
    }
    if (!cui__popup_begin(ui, pid, tid, CUI_PLACE_BOTTOM_START, 0, 5, 0, true)) return;
    for (int k = 0; k < F->nstyles; k++) {
        const cui_text_style *s = &F->styles[k];
        cui_menu_item_opts mo;
        memset(&mo, 0, sizeof mo);
        mo.checked = k == f->style;
        mo.font = s->font;
        mo.text_size = s->preview_size > 0 ? s->preview_size : ui->theme.text_body;
        cui_push_id_int(ui, k);
        if (cui_menu_item_ex(ui, s->name ? s->name : "", &mo)) {
            if (f->style != k || f->size != s->size) F->result |= CUI_FORMAT_CHANGED;
            f->style = k;
            f->size = s->size;
            if (ui->focus_visible) ui->focus = tid;
        }
        cui_pop_id(ui);
    }
    const char *footer = F->o->footer ? F->o->footer : "Text options";
    const char *keys = F->o->footer_keys ? F->o->footer_keys : "Ctrl Alt 0";
    if (footer[0] || keys[0]) {
        cui_menu_separator(ui);
        int row = cui__node_new(ui, 0);
        cui_node *rn = &ui->nodes[row];
        rn->dir = CUI_ROW; rn->align = CUI_ALIGN_CENTER; rn->gap = 12;
        rn->w = CUI_FILL; rn->pad[0] = rn->pad[2] = 4; rn->pad[1] = rn->pad[3] = 10;
        cui__open(ui, row);
        int ft = cui__text_node(ui, footer, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_MUTED), false);
        ui->nodes[ft].w = CUI_FILL;
        ui->nodes[ft].line_height = 1.3f;
        if (keys[0]) cui_kbd(ui, keys);
        cui__close(ui);
    }
    cui__popup_end(ui);
}

/* the alignment button and its flyout row */
static void cui__fmt_align(cui__fmt_frame *F) {
    cui_ctx *ui = F->ui;
    cui_text_format *f = F->f;
    static const char *names[] = { "Align left", "Align center", "Align right", "Justify" };
    int a = CUI_CLAMP((int)f->align, 0, 3);
    cui_tool_opts to;
    memset(&to, 0, sizeof to);
    to.tooltip = "Alignment";
    to.flyout = true;
    int before = ui->node_count;
    bool clicked = cui_toolbar_button_ex(ui, "", &to);
    bool shown = ui->node_count > before;
    if (!shown) {
        /* chosen from the overflow menu: no button to hang the flyout on, so step to the next */
        if (clicked) { f->align = (cui_format_align)((a + 1) % 4); F->result |= CUI_FORMAT_CHANGED; }
        return;
    }
    cui_id tid = ui->last_id, pid = tid ^ 0xA119u;
    int tnode = ui->last_node;
    cui__cv_glyph_on(ui, tnode, a, 16, CUI_C(ui, FOREGROUND));
    bool opened = cui__cv_popup_trigger(ui, pid, tid, clicked);
    cui_id ids[4];
    int focused = -1;
    for (int k = 0; k < 4; k++) {
        ids[k] = cui__hash(&k, sizeof k, pid);
        if (ui->focus == ids[k] && ui->focus_visible) focused = k;
    }
    if (!cui__popup_begin(ui, pid, tid, CUI_PLACE_BOTTOM_START, 0, 4, 0, false)) { cui__cv_popup_refocus(ui, pid, tid, ids, 4); return; }
    int row = cui__node_new(ui, 0);
    ui->nodes[row].dir = CUI_ROW; ui->nodes[row].gap = 2;
    cui__open(ui, row);
    if (focused >= 0 && !cui__keys_covered(ui)) {
        if (cui_key_pressed(ui, CUI_KEY_RIGHT) && focused < 3) ui->focus = ids[focused + 1];
        if (cui_key_pressed(ui, CUI_KEY_LEFT) && focused > 0) ui->focus = ids[focused - 1];
    }
    for (int k = 0; k < 4; k++) {
        cui_button_opts bo;
        memset(&bo, 0, sizeof bo);
        bo.variant = CUI_BUTTON_GHOST;
        bo.selected = k == a;
        bo.tooltip = names[k];
        bool hit = cui__icon_button_id(ui, ids[k], "", &bo);
        cui__cv_glyph_on(ui, ui->last_node, k, 16, CUI_C(ui, FOREGROUND));
        if (opened && k == a && ui->focus_visible) ui->focus = ids[k];
        if (hit) {
            if (f->align != (cui_format_align)k) { f->align = (cui_format_align)k; F->result |= CUI_FORMAT_CHANGED; }
            cui__popup_set(ui, pid, false);
            if (ui->focus_visible) ui->focus = tid;
        }
    }
    cui__close(ui);
    cui__popup_end(ui);
    cui__cv_popup_refocus(ui, pid, tid, ids, 4);
}

/* the list button and its menu of list kinds; picking the current kind turns the list off */
static void cui__fmt_list(cui__fmt_frame *F) {
    cui_ctx *ui = F->ui;
    cui_text_format *f = F->f;
    static const char *const icons[] = { CUI_ICON_LIST, CUI_ICON_LIST, CUI_ICON_HASH, CUI_ICON_LIST_CHECKS };
    static const char *const names[] = { "Bulleted list", "Numbered list", "Checklist" };
    int l = CUI_CLAMP((int)f->list, 0, 3);
    bool listed = l != CUI_FORMAT_LIST_NONE, on = listed;
    cui_tool_opts to;
    memset(&to, 0, sizeof to);
    to.tooltip = "List";
    to.flyout = true;
    to.toggle = &on;           /* shown pressed while there is a list; the flip is the component's own copy */
    int before = ui->node_count;
    bool clicked = cui_toolbar_button_ex(ui, CUI_ICON_LIST, &to);   /* one icon: a button's id follows its icon, and focus must stay */
    if (ui->node_count <= before) {
        if (clicked) { f->list = (cui_format_list)((l + 1) % 4); F->result |= CUI_FORMAT_CHANGED; }
        return;
    }
    cui_id tid = ui->last_id, pid = tid ^ 0x1157u;
    if (cui__cv_popup_trigger(ui, pid, tid, clicked)) {
        cui__menu_nav *nav = cui_state(ui, pid ^ 0x4E4156u, sizeof *nav);
        nav->highlight = listed ? l - 1 : 0;
        nav->keyboard = ui->focus_visible;
    }
    if (!cui__popup_begin(ui, pid, tid, CUI_PLACE_BOTTOM_START, 0, 5, 0, true)) return;
    for (int k = 0; k < 3; k++) {
        cui_menu_item_opts mo;
        memset(&mo, 0, sizeof mo);
        mo.icon = icons[k + 1];
        mo.checked = l == k + 1;
        if (cui_menu_item_ex(ui, names[k], &mo)) {
            f->list = l == k + 1 ? CUI_FORMAT_LIST_NONE : (cui_format_list)(k + 1);
            F->result |= CUI_FORMAT_CHANGED;
            if (ui->focus_visible) ui->focus = tid;
        }
    }
    cui__popup_end(ui);
}

/* delete, with its question in a small tip in the text color */
static void cui__fmt_delete(cui__fmt_frame *F) {
    cui_ctx *ui = F->ui;
    cui_tool_opts to;
    memset(&to, 0, sizeof to);
    to.tooltip = "Delete";
    to.tone = CUI_TONE_DANGER;
    int before = ui->node_count;
    bool clicked = cui_toolbar_button_ex(ui, CUI_ICON_TRASH_2, &to);
    if (ui->node_count <= before) {
        /* from the overflow menu the menu item itself was the deliberate second step */
        if (clicked) F->result |= CUI_FORMAT_DELETE;
        return;
    }
    cui_id tid = ui->last_id, pid = tid ^ 0xDE1u;
    cui_popup *p = cui__popup_get(ui, pid, true);
    if (!p) return;
    bool key_activate = ui->focus == tid && ui->focus_visible && (cui_key_pressed(ui, CUI_KEY_ENTER) || cui_key_pressed(ui, CUI_KEY_SPACE));
    if (clicked && !(p->open && key_activate)) cui__popup_set(ui, pid, !p->open);
    bool just_opened = p->open && ui->frame - p->opened_frame <= 1;
    cui_id *yes_id = cui_state(ui, pid ^ 0x7E5u, sizeof *yes_id);   /* the Delete button, to give focus back from */
    if (!cui__popup_begin(ui, pid, tid, CUI_PLACE_BOTTOM_END, 0, 6, 8, false)) { cui__cv_popup_refocus(ui, pid, tid, yes_id, 1); return; }
    if (p->opened_frame == ui->frame) ui->kpressed[CUI_KEY_ENTER] = 0;   /* the press that opened it does not answer it */
    cui_node *root = &ui->nodes[cui__current(ui)];
    root->dir = CUI_ROW;
    root->align = CUI_ALIGN_CENTER;
    root->bg = CUI_C(ui, FOREGROUND);
    root->border = 0;
    root->radius = CUI_CORNERS(10);
    root->pad[3] = 10;
    root->min_w = 0;
    int t = cui__text_node(ui, F->o->delete_question ? F->o->delete_question : "Delete this text?", -1, CUI_FONT_MEDIUM,
                           cui__fs(ui, ui->theme.text_sm), CUI_C(ui, BACKGROUND), false);
    ui->nodes[t].line_height = 1.3f;
    cui_button_opts bo;
    memset(&bo, 0, sizeof bo);
    bo.variant = CUI_BUTTON_DANGER;
    bo.size = CUI_SIZE_SM;
    bool yes = cui_button_ex(ui, "Delete", &bo);
    *yes_id = ui->last_id;
    if (just_opened) ui->focus = *yes_id;   /* Enter answers the question, not the trigger */
    if (!ui->edit && p->open && cui_key_pressed(ui, CUI_KEY_ENTER)) { yes = true; ui->kpressed[CUI_KEY_ENTER] = 0; }
    cui__popup_end(ui);
    cui__cv_popup_refocus(ui, pid, tid, yes_id, 1);
    if (yes) {
        F->result |= CUI_FORMAT_DELETE;
        cui__popup_set(ui, pid, false);
        if (ui->focus_visible) ui->focus = tid;
    }
}

int cui_format_bar_ex(cui_ctx *ui, const char *id_str, cui_text_format *f, const cui_format_bar_opts *o) {
    cui_format_bar_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!f) return 0;
    cui__fmt_frame F;
    memset(&F, 0, sizeof F);
    F.ui = ui; F.f = f; F.o = o;
    F.styles = o->styles && o->style_count > 0 ? o->styles : cui__fmt_default_styles;
    F.nstyles = o->styles && o->style_count > 0 ? o->style_count : CUI_COUNTOF(cui__fmt_default_styles);
    F.colors = o->colors && o->color_count > 0 ? o->colors : cui__fmt_default_colors;
    F.ncolors = o->colors && o->color_count > 0 ? o->color_count : CUI_COUNTOF(cui__fmt_default_colors);
    F.id = cui_make_id(ui, id_str);
    F.st = cui_state(ui, F.id ^ 0xF0F0u, sizeof *F.st);

    cui_toolbar_opts tb;
    memset(&tb, 0, sizeof tb);
    tb.style = CUI_TOOLBAR_FLOATING;
    cui_toolbar_begin(ui, id_str, &tb);
    cui_toolbar_group_begin(ui);
    cui__fmt_colors(&F);
    cui_toolbar_group_end(ui);

    cui_toolbar_group_begin(ui);
    cui_tool_opts to;
    memset(&to, 0, sizeof to);
    bool was_bold = f->bold, was_under = f->underline, link = f->link;
    to.tooltip = "Bold"; to.shortcut = "Ctrl B"; to.toggle = &f->bold; to.tone = CUI_TONE_ACCENT;
    cui_toolbar_button_ex(ui, CUI_ICON_BOLD, &to);
    to.tooltip = "Underline"; to.shortcut = "Ctrl U"; to.toggle = &f->underline; to.tone = CUI_TONE_NEUTRAL;
    cui_toolbar_button_ex(ui, CUI_ICON_UNDERLINE, &to);
    to.tooltip = "Link"; to.shortcut = "Ctrl K"; to.toggle = &link;
    if (cui_toolbar_button_ex(ui, CUI_ICON_LINK, &to)) F.result |= CUI_FORMAT_LINK;
    if (f->bold != was_bold || f->underline != was_under) F.result |= CUI_FORMAT_CHANGED;
    cui_toolbar_group_end(ui);

    cui_toolbar_group_begin(ui);
    cui__fmt_style(&F);
    cui_toolbar_group_end(ui);

    cui_toolbar_group_begin(ui);
    cui__fmt_align(&F);
    cui__fmt_list(&F);
    cui_toolbar_group_end(ui);

    if (!o->no_delete || !o->no_more) {
        cui_toolbar_group_begin(ui);
        if (!o->no_delete) cui__fmt_delete(&F);
        memset(&to, 0, sizeof to);
        to.tooltip = "More";
        if (!o->no_more && cui_toolbar_button_ex(ui, CUI_ICON_ELLIPSIS, &to)) F.result |= CUI_FORMAT_MORE;
        cui_toolbar_group_end(ui);
    }
    cui_toolbar_end(ui);
    ui->last_id = F.id;
    return F.result;
}

/* ------------------------------------------------------------------------ */
/* Live cursors, presence pill, selection ring                              */
/* ------------------------------------------------------------------------ */

#define CUI__CURSOR_FLAG_X 14.0f    /* the name flag, from the pointer's tip */
#define CUI__CURSOR_FLAG_Y 16.0f
#define CUI__CURSOR_SAYS_W 180.0f   /* a chat bubble wraps at this width */

typedef struct { cui_spring x, y; float show; bool init; } cui__cursor_state;
typedef struct { float x, y, show; cui_color c, ink; const char *name, *says; } cui__cursor_draw;
typedef struct { int anchor; int count; cui__cursor_draw *d; cui_node_view view; bool has_view; } cui__cursors_paint;

/* a person's color: theirs, their tone, or a tone of the theme chosen from the name, like avatars */
static cui_color cui__cv_person(cui_ctx *ui, const cui_live_user *u) {
    if (cui__color_set(u->color)) return cui_color_of(ui, u->color);
    if (u->tone != CUI_TONE_NEUTRAL) return cui__tone_color(ui, u->tone);
    static const cui_tone tones[] = { CUI_TONE_INFO, CUI_TONE_VIOLET, CUI_TONE_TEAL, CUI_TONE_WARNING, CUI_TONE_ROSE, CUI_TONE_SUCCESS };
    return cui__tone_color(ui, tones[cui__hash_str(u->name ? u->name : "", 7) % CUI_COUNTOF(tones)]);
}

/* the arrow, with a white edge and a soft shadow, its tip at (x, y) */
static void cui__draw_cursor_arrow(cui_ctx *ui, float x, float y, cui_color fill, float alpha) {
    static const float pts[4][2] = { { 1.5f, 1.5f }, { 16, 8.6f }, { 9.7f, 10.3f }, { 6.7f, 16.5f } };
    cui_vec2 p[4], sh[4];
    for (int k = 0; k < 4; k++) {
        p[k] = (cui_vec2){ x + pts[k][0] - 1.5f, y + pts[k][1] - 1.5f };
        sh[k] = (cui_vec2){ p[k].x, p[k].y + 1 };
    }
    /* the outline is concave at the notch: two convex halves */
    cui_vec2 a[3] = { p[0], p[1], p[2] }, b[3] = { p[0], p[2], p[3] };
    cui_vec2 sa[3] = { sh[0], sh[1], sh[2] }, sb[3] = { sh[0], sh[2], sh[3] };
    cui_color shade = cui__rgba(0, 0, 0, (int)(90 * alpha));
    cui_draw_polygon(ui, sa, 3, shade);
    cui_draw_polygon(ui, sb, 3, shade);
    cui_draw_polygon(ui, a, 3, cui_alpha(fill, alpha));
    cui_draw_polygon(ui, b, 3, cui_alpha(fill, alpha));
    cui_draw_polyline(ui, p, 4, 1.4f, cui__rgba(255, 255, 255, (int)(255 * alpha)), true);
}

static void cui__paint_cursors(cui_ctx *ui, cui_node *n, void *data) {
    (void)n;
    cui__cursors_paint *p = data;
    cui_rect area = ui->nodes[p->anchor].rect;
    float z = p->has_view ? (p->view.zoom > 0 ? p->view.zoom : 1) : 1;
    float ox = area.x + (p->has_view ? p->view.x : 0), oy = area.y + (p->has_view ? p->view.y : 0);
    float fs = cui__fs(ui, 11.5f), bs = cui__fs(ui, ui->theme.text_sm), blh = roundf(bs * 1.35f);
    cui_push_clip(ui, area);
    for (int k = 0; k < p->count; k++) {
        cui__cursor_draw *d = &p->d[k];
        if (d->show <= 0.01f) continue;
        float x = roundf(ox + d->x * z), y = roundf(oy + d->y * z);
        cui__draw_cursor_arrow(ui, x, y, d->c, d->show);
        if (d->name && d->name[0]) {
            float tw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, d->name, -1, false);
            cui_rect fr = { x + CUI__CURSOR_FLAG_X, y + CUI__CURSOR_FLAG_Y, ceilf(tw) + 16, 20 };
            cui_draw_shadow(ui, fr, CUI_CORNERS(7), 6, 0, (cui_vec2){ 0, 2 }, cui__rgba(0, 0, 0, (int)(70 * d->show)));
            cui_draw_rect(ui, fr, CUI_CORNERS(7), cui_alpha(d->c, d->show));
            cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, fr.x + 8, fr.y + fr.h * 0.5f, CUI__AT_LEFT, cui_alpha(d->ink, d->show), d->name, false);
        }
        if (d->says && d->says[0]) {
            cui_text_layout_line lines[6];
            int len = (int)strlen(d->says);
            int nl = cui__text_wrap(ui, CUI_FONT_BODY, bs, d->says, len, CUI__CURSOR_SAYS_W - 18, lines, 6);
            nl = CUI_MIN(nl, 6);
            float w = 0;
            for (int l = 0; l < nl; l++) w = CUI_MAX(w, lines[l].width);
            cui_rect br = { x + CUI__CURSOR_FLAG_X, y + CUI__CURSOR_FLAG_Y + 22, ceilf(w) + 18, nl * blh + 12 };
            cui_corners rad = { 4, 12, 12, 12 };
            cui_draw_rect(ui, br, rad, cui_alpha(d->c, 0.95f * d->show));
            for (int l = 0; l < nl; l++)
                cui__draw_text_line(ui, CUI_FONT_BODY, bs, br.x + 9, br.y + 6 + l * blh + (blh - bs * 1.2f) * 0.5f, cui_alpha(d->ink, d->show),
                                    d->says + lines[l].start, lines[l].end - lines[l].start, false);
        }
    }
    cui_pop_clip(ui);
}

void cui_live_cursors_ex(cui_ctx *ui, const char *id_str, const cui_live_user *users, int count, const cui_live_cursors_opts *o) {
    cui_live_cursors_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!users || count <= 0) return;
    cui_id id = cui_make_id(ui, id_str);
    cui__cursors_paint *p = CUI_PAINT_DATA(ui, cui__cursors_paint);
    memset(p, 0, sizeof *p);
    p->d = cui__frame_alloc(ui, sizeof(cui__cursor_draw) * (size_t)count);
    p->count = count;
    if (o->view) { p->view = *o->view; p->has_view = true; }
    for (int k = 0; k < count; k++) {
        const cui_live_user *u = &users[k];
        cui__cursor_state *st = cui_state(ui, cui__hash_str(u->name ? u->name : "", id), sizeof *st);
        if (!st->init || o->no_glide) {
            st->x.x = u->x; st->y.x = u->y; st->x.v = st->y.v = 0;
            st->show = u->away ? 0 : 1;
            st->init = true;
        }
        cui__spring(ui, &st->x, u->x, CUI_SPRING_SMOOTH);
        cui__spring(ui, &st->y, u->y, CUI_SPRING_SMOOTH);
        cui__tween(ui, &st->show, u->away ? 0.0f : 1.0f, CUI__T_STATE);
        cui__cursor_draw *c = &p->d[k];
        c->x = st->x.x; c->y = st->y.x; c->show = st->show;
        c->c = cui__cv_person(ui, u);
        c->ink = cui__cv_ink(c->c);
        c->name = u->name ? cui__frame_strdup(ui, u->name, -1) : NULL;
        c->says = u->says ? cui__frame_strdup(ui, u->says, -1) : NULL;
    }
    int m = cui__cv_overlay(ui, cui__paint_cursors, p);
    if (m >= 0) p->anchor = ui->nodes[m].abs_anchor;
}

typedef struct { cui_color c, ring; cui_texture tex; bool speaking; float size; } cui__pill_avatar;

static void cui__paint_pill_avatar(cui_ctx *ui, cui_node *n, void *data) {
    cui__pill_avatar *p = data;
    cui_vec2 c = { n->rect.x + n->rect.w * 0.5f, n->rect.y + n->rect.h * 0.5f };
    float r = p->size * 0.5f;
    cui_draw_circle(ui, c, r + 2, p->ring);   /* cuts the avatar out of the one under it */
    if (p->speaking) cui_draw_arc(ui, c, r + 2.75f, 0, 2 * CUI_PI, 1.5f, p->c);
    if (p->tex) cui_draw_image(ui, p->tex, (cui_rect){ c.x - r, c.y - r, 2 * r, 2 * r }, (cui_rect){ 0, 0, 1, 1 }, CUI_CORNERS(r), cui__rgba(255, 255, 255, 255));
    else cui_draw_circle(ui, c, r, p->c);
}

typedef struct { cui_color c; float t; bool still; } cui__pill_bars;

/* three level bars rising and falling out of step, a second apart */
static void cui__paint_pill_bars(cui_ctx *ui, cui_node *n, void *data) {
    cui__pill_bars *p = data;
    static const float still[3] = { 0.45f, 1.0f, 0.3f };
    for (int k = 0; k < 3; k++) {
        float phase = p->t + 0.3f * (float)k;
        float lvl = p->still ? still[k] : 0.5f - 0.5f * cosf(2 * CUI_PI * phase);
        float h = 3 + 9 * lvl;
        cui_rect r = { n->rect.x + k * 4.5f, n->rect.y + (n->rect.h - h) * 0.5f, 2.5f, h };
        cui_draw_rect(ui, r, CUI_CORNERS(1.25f), p->c);
    }
}

void cui_presence_pill_ex(cui_ctx *ui, const char *id_str, const cui_live_user *users, int count, const cui_presence_pill_opts *o) {
    cui_presence_pill_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    float size = o->size > 0 ? o->size : 26;
    int max_visible = o->max_visible > 0 ? o->max_visible : 5;
    cui_id id = cui_make_id(ui, id_str);
    int speaker = -1;
    for (int k = 0; k < count && users; k++) if (users[k].speaking && !users[k].away) { speaker = k; break; }
    int row = cui__node_new(ui, id);
    cui_node *rn = &ui->nodes[row];
    rn->dir = CUI_ROW; rn->align = CUI_ALIGN_CENTER; rn->gap = 8;
    rn->pad[0] = rn->pad[2] = rn->pad[3] = 5;
    rn->pad[1] = speaker >= 0 ? 10 : 5;
    rn->radius = CUI_CORNERS((size + 12) * 0.5f);
    rn->bg = CUI_C(ui, SURFACE);
    rn->border = 1;
    rn->border_color = CUI_C(ui, BORDER);
    rn->self_align = CUI__ALIGN_NOSTRETCH;
    cui__open(ui, row);
    int stack = cui__node_new(ui, 0);
    ui->nodes[stack].dir = CUI_ROW;
    ui->nodes[stack].align = CUI_ALIGN_CENTER;
    ui->nodes[stack].gap = -5;
    cui__open(ui, stack);
    int shown = CUI_MIN(count, max_visible);
    for (int k = 0; k < shown; k++) {
        const cui_live_user *u = &users[k];
        cui_id aid = cui__hash(&k, sizeof k, id);
        int a = cui__leaf(ui, aid, CUI_PX(size), CUI_PX(size));
        cui__pill_avatar *pa = CUI_PAINT_DATA(ui, cui__pill_avatar);
        pa->c = cui__cv_person(ui, u);
        pa->ring = CUI_C(ui, SURFACE);
        pa->tex = u->avatar;
        pa->speaking = k == speaker;
        pa->size = size;
        cui_node *an = &ui->nodes[a];
        an->paint = cui__paint_pill_avatar;
        an->paint_data = pa;
        an->dir = CUI_STACK; an->align = CUI_ALIGN_CENTER; an->justify = CUI_JUSTIFY_CENTER;
        an->opacity = u->away ? 0.55f : 1.0f;
        if (!u->avatar) {
            char ini[16];
            cui__initials(u->name ? u->name : "", ini);
            cui__open(ui, a);
            int t = cui__text_node(ui, cui__frame_strdup(ui, ini, -1), -1, CUI_FONT_MEDIUM, size * 0.365f, cui__cv_ink(pa->c), false);
            ui->nodes[t].line_height = 1.0f;
            cui__close(ui);
        }
        if (u->name) cui__tooltip_passive(ui, aid, u->away ? cui__frame_printf(ui, "%s (away)", u->name) : u->name);
    }
    if (count > shown) {
        int m = cui__leaf(ui, 0, CUI_PX(size), CUI_PX(size));
        cui_node *mn = &ui->nodes[m];
        mn->radius = CUI_CORNERS(size * 0.5f);
        mn->bg = CUI_C(ui, SURFACE_MUTED);
        mn->border = 2; mn->border_color = CUI_C(ui, SURFACE);
        mn->dir = CUI_STACK; mn->align = CUI_ALIGN_CENTER; mn->justify = CUI_JUSTIFY_CENTER;
        cui__open(ui, m);
        int t = cui__text_node(ui, cui__frame_printf(ui, "+%d", count - shown), -1, CUI_FONT_MEDIUM, size * 0.36f, CUI_C(ui, TEXT_SECONDARY), false);
        ui->nodes[t].line_height = 1.0f;
        cui__close(ui);
    }
    cui__close(ui);
    if (speaker >= 0) {
        bool still = ui->theme.motion <= 0;
        int b = cui__leaf(ui, 0, CUI_PX(12), CUI_PX(12));
        cui__pill_bars *pb = CUI_PAINT_DATA(ui, cui__pill_bars);
        pb->c = CUI_C(ui, SUCCESS);
        pb->t = (float)fmod(ui->time, 1.0);
        pb->still = still;
        ui->nodes[b].paint = cui__paint_pill_bars;
        ui->nodes[b].paint_data = pb;
        if (!still) ui->animating = true;   /* the bars move for as long as someone speaks */
        const char *nm = users[speaker].name ? users[speaker].name : "";
        int t = cui__text_node(ui, nm, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_SECONDARY), false);
        ui->nodes[t].line_height = 1.3f;
    }
    cui__close(ui);
    ui->last_id = id;
    ui->last_node = row;
}

typedef struct { int anchor; cui_color c, ink; float radius; const char *name; } cui__selring_paint;

static void cui__paint_selection_ring(cui_ctx *ui, cui_node *n, void *data) {
    (void)n;
    cui__selring_paint *p = data;
    const cui_node *a = &ui->nodes[p->anchor];
    cui_rect r = a->rect;
    float rad = p->radius > 0 ? p->radius : a->radius.tl;
    cui_color clear = cui__rgba(0, 0, 0, 0);
    /* a soft band, then a crisp ring right at the edge */
    cui_draw_rect_border(ui, (cui_rect){ r.x - 5, r.y - 5, r.w + 10, r.h + 10 }, CUI_CORNERS(rad + 5), clear, cui_alpha(p->c, 0.18f), 3.5f);
    cui_draw_rect_border(ui, (cui_rect){ r.x - 1.5f, r.y - 1.5f, r.w + 3, r.h + 3 }, CUI_CORNERS(rad + 1.5f), clear, p->c, 1.5f);
    if (p->name && p->name[0]) {
        float fs = cui__fs(ui, ui->theme.text_xs);
        float w = ceilf(cui__text_width(ui, CUI_FONT_MEDIUM, fs, p->name, -1, false)) + 12, h = 18;
        float x = a->rtl ? r.x - 1.5f : r.x + r.w + 1.5f - w;
        cui_rect fr = { x, r.y - 1.5f - h, w, h };
        cui_corners c = a->rtl ? (cui_corners){ 6, 6, 6, 0 } : (cui_corners){ 6, 6, 0, 6 };
        cui_draw_rect(ui, fr, c, p->c);
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, fr.x + 6, fr.y + h * 0.5f, CUI__AT_LEFT, p->ink, p->name, false);
    }
}

void cui_selection_ring_ex(cui_ctx *ui, cui_color color, const cui_selection_ring_opts *o) {
    cui_selection_ring_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui__selring_paint *p = CUI_PAINT_DATA(ui, cui__selring_paint);
    p->c = cui__color_set(color) ? cui_color_of(ui, color) : cui__emphasis(ui);
    p->ink = cui__cv_ink(p->c);
    p->radius = o->radius;
    p->name = o->name ? cui__frame_strdup(ui, o->name, -1) : NULL;
    p->anchor = ui->last_node;
    cui__cv_overlay(ui, cui__paint_selection_ring, p);
}

/* ------------------------------------------------------------------------ */
/* Canvas controls                                                          */
/* ------------------------------------------------------------------------ */

static const float cui__zoom_steps[] = { 0.25f, 0.33f, 0.5f, 0.67f, 0.8f, 1, 1.25f, 1.5f, 2, 3, 4 };
#define CUI__RAIL_INSET 12.0f      /* from the canvas edge */
#define CUI__RAIL_PAD 4.0f
#define CUI__RAIL_GAP 2.0f
#define CUI__RAIL_READOUT_H 22.0f
#define CUI__RAIL_SEP_H 7.0f       /* the separator line with its room */
#define CUI__ZOOM_EASE 0.22f

typedef struct {
    float w, h;                /* the canvas, from its last layout */
    bool rtl, measured;
    float target, cur;         /* the zoom eased toward, and where the easing is */
    float px, py;              /* the canvas point at the middle, eased toward the fit target */
    float tpx, tpy;
    float written;             /* the zoom this rail wrote last, to notice others changing it */
    bool anim, fit, init;
    float hover_readout, focus_readout;
} cui__rail_state;

/* the canvas's size and direction, which only layout knows, for the next frame */
static void cui__paint_rail_measure(cui_ctx *ui, cui_node *n, void *data) {
    cui__rail_state *st = data;
    const cui_node *a = &ui->nodes[n->abs_anchor];
    if (!st->measured || fabsf(a->rect.w - st->w) > 0.5f || fabsf(a->rect.h - st->h) > 0.5f || a->rtl != st->rtl) ui->animating = true;
    st->w = a->rect.w; st->h = a->rect.h; st->rtl = a->rtl; st->measured = true;
}

/* the zoom readout: a small button, a click resets to 100% */
static bool cui__rail_readout(cui_ctx *ui, cui_id id, cui__rail_state *st, float zoom) {
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    cui__tween(ui, &st->hover_readout, r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->focus_readout, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    int i = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[i];
    n->w = CUI_PX(40); n->h = CUI_PX(CUI__RAIL_READOUT_H);
    n->dir = CUI_STACK; n->align = CUI_ALIGN_CENTER; n->justify = CUI_JUSTIFY_CENTER;
    n->radius = CUI_CORNERS(7);
    n->bg = cui_alpha(CUI_C(ui, HOVER), st->hover_readout);
    n->self_align = CUI_ALIGN_CENTER;
    cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
    if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__focus_ring(ui, i, st->focus_readout);
    cui__open(ui, i);
    int t = cui__text_node(ui, cui__frame_printf(ui, "%d%%", (int)lroundf(zoom * 100)), -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_xs),
                           cui_mix(CUI_C(ui, TEXT_SECONDARY), CUI_C(ui, FOREGROUND), st->hover_readout), false);
    ui->nodes[t].flags |= CUI_NF_TABULAR;
    ui->nodes[t].line_height = 1.0f;
    cui__close(ui);
    ui->last_id = id;
    ui->last_node = i;
    cui_tooltip(ui, "Reset to 100%");
    return r.clicked;
}

/* Ease the view toward the target zoom, keeping the canvas point at the middle
 * where it is (or easing it toward the content's middle, after fit). */
static void cui__rail_ease(cui_ctx *ui, cui__rail_state *st, cui_node_view *view) {
    float z = view->zoom > 0 ? view->zoom : 1;
    float mx = st->w * 0.5f, my = st->h * 0.5f;
    if (!st->fit) { st->px = (mx - view->x) / z; st->py = (my - view->y) / z; }
    cui__tween(ui, &st->cur, st->target, CUI__ZOOM_EASE);
    if (st->fit) { cui__tween(ui, &st->px, st->tpx, CUI__ZOOM_EASE); cui__tween(ui, &st->py, st->tpy, CUI__ZOOM_EASE); }
    view->zoom = st->cur;
    view->x = mx - st->px * st->cur;
    view->y = my - st->py * st->cur;
    if (st->cur == st->target && (!st->fit || (st->px == st->tpx && st->py == st->tpy))) { st->anim = false; st->fit = false; }
}

int cui_canvas_controls_ex(cui_ctx *ui, const char *id_str, cui_node_view *view, const cui_canvas_controls_opts *o) {
    cui_canvas_controls_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!view) return 0;
    int anchor = ui->last_node;
    if (anchor < 0 || anchor >= ui->node_count) return 0;
    const float *steps = o->steps && o->step_count > 0 ? o->steps : cui__zoom_steps;
    int nsteps = o->steps && o->step_count > 0 ? o->step_count : CUI_COUNTOF(cui__zoom_steps);
    float zmin = steps[0], zmax = steps[nsteps - 1];
    cui_id id = cui_make_id(ui, id_str);
    cui__rail_state *st = cui_state(ui, id, sizeof *st);
    if (!st->measured && ui->nodes[anchor].id && cui__has_prev_rect(ui, ui->nodes[anchor].id)) {
        cui_rect pr = cui__prev_rect(ui, ui->nodes[anchor].id);
        st->w = pr.w; st->h = pr.h;
    }
    float z = view->zoom > 0 ? view->zoom : 1;
    /* someone else (the wheel, a key, the app) changed the zoom: follow it */
    if (!st->init || fabsf(z - st->written) > 1e-4f) { st->target = st->cur = z; st->anim = st->fit = false; st->init = true; }
    int result = 0;
    cui_push_id(ui, id_str);

    /* the rail's size is known before layout: buttons, the readout, the separator */
    float bh = ui->theme.control_md;
    int nbuttons = 2 + (o->no_fit ? 0 : 1) + o->toggle_count;
    bool sep = !o->no_fit || o->toggle_count > 0;
    int items = nbuttons + 1 + (sep ? 1 : 0);
    float rail_w = bh + 2 * CUI__RAIL_PAD + 2, rail_h = nbuttons * bh + CUI__RAIL_READOUT_H + (sep ? CUI__RAIL_SEP_H : 0)
                 + (items - 1) * CUI__RAIL_GAP + 2 * CUI__RAIL_PAD + 2;
    bool at_left = st->rtl != o->start_side;
    float x = at_left ? CUI__RAIL_INSET : st->w - CUI__RAIL_INSET - rail_w;
    float y = roundf((st->h - rail_h) * 0.5f);
    int rail = cui__absolute(ui, id, CUI_MAX(0, x), CUI_MAX(0, y), rail_w, rail_h);
    cui_node *rn = &ui->nodes[rail];
    rn->abs_anchor = anchor;
    rn->flags |= CUI_NF_OVERLAY | CUI_NF_HIT_OPAQUE;   /* clicks on the rail stop at it, not at the canvas below */
    rn->dir = CUI_COLUMN; rn->align = CUI_ALIGN_CENTER; rn->gap = CUI__RAIL_GAP;
    rn->pad[0] = rn->pad[1] = rn->pad[2] = rn->pad[3] = CUI__RAIL_PAD;
    rn->radius = CUI_CORNERS(ui->theme.radius_panel);
    rn->bg = CUI_C(ui, SURFACE);
    rn->border = 1;
    rn->border_color = CUI_C(ui, BORDER);
    rn->elevation = CUI_ELEVATION_RAISED;
    rn->elevation_alpha = 1;
    rn->paint = cui__paint_rail_measure;
    rn->paint_data = st;
    cui__open(ui, rail);
    cui_button_opts bo;
    memset(&bo, 0, sizeof bo);
    bo.variant = CUI_BUTTON_GHOST;
    float target = st->target;
    bo.tooltip = "Zoom in";
    bo.disabled = target >= zmax - 1e-4f;
    if (cui_icon_button_ex(ui, CUI_ICON_PLUS, &bo)) {
        float nz = zmax;
        for (int k = 0; k < nsteps; k++) if (steps[k] > target + 1e-3f) { nz = steps[k]; break; }
        target = nz;
    }
    if (cui__rail_readout(ui, cui_make_id(ui, "#zoom"), st, z)) target = 1;
    bo.tooltip = "Zoom out";
    bo.disabled = target <= zmin + 1e-4f;
    if (cui_icon_button_ex(ui, CUI_ICON_MINUS, &bo)) {
        float nz = zmin;
        for (int k = nsteps - 1; k >= 0; k--) if (steps[k] < target - 1e-3f) { nz = steps[k]; break; }
        target = nz;
    }
    bo.disabled = false;
    if (sep) {
        int sb = cui__node_new(ui, 0);
        ui->nodes[sb].pad[0] = ui->nodes[sb].pad[2] = 3;
        cui__open(ui, sb);
        int l = cui__leaf(ui, 0, CUI_PX(20), CUI_PX(1));
        ui->nodes[l].bg = CUI_C(ui, BORDER);
        cui__close(ui);
    }
    if (!o->no_fit) {
        bo.tooltip = "Fit";
        if (cui_icon_button_ex(ui, "", &bo)) {
            result |= CUI_CANVAS_FIT;
            cui_rect c = o->content;
            if (c.w > 0 && c.h > 0 && st->w > 0 && st->h > 0) {
                float zf = CUI_MIN(st->w * 0.9f / c.w, st->h * 0.9f / c.h);
                target = cui__clampf(zf, zmin, zmax);
                st->fit = true;
                st->tpx = c.x + c.w * 0.5f; st->tpy = c.y + c.h * 0.5f;
                st->px = (st->w * 0.5f - view->x) / z; st->py = (st->h * 0.5f - view->y) / z;
                st->anim = true;
            }
        }
        cui__cv_glyph_on(ui, ui->last_node, CUI__CV_FIT, 16, CUI_C(ui, FOREGROUND));
    }
    for (int k = 0; k < o->toggle_count; k++) {
        const cui_canvas_toggle *t = &o->toggles[k];
        bo.tooltip = t->label;
        bo.selected = t->on && *t->on;
        cui_push_id_int(ui, k);
        if (cui_icon_button_ex(ui, t->icon ? t->icon : CUI_ICON_CIRCLE, &bo)) {
            if (t->on) *t->on = !*t->on;
            result |= CUI_CANVAS_TOGGLED;
        }
        cui_pop_id(ui);
    }
    bo.selected = false;
    cui__close(ui);
    cui_pop_id(ui);

    target = cui__clampf(target, zmin, zmax);
    if (target != st->target) { st->target = target; st->anim = true; result |= CUI_CANVAS_ZOOMED; }
    else if (result & CUI_CANVAS_FIT && st->fit) result |= CUI_CANVAS_ZOOMED;
    if (st->anim && st->w > 0) cui__rail_ease(ui, st, view);
    else if (st->anim) { view->zoom = st->cur = st->target; st->anim = false; }
    st->written = view->zoom > 0 ? view->zoom : 1;
    ui->last_node = anchor;
    ui->last_id = id;
    return result;
}

/* ------------------------------------------------------------------------ */
/* Result card                                                              */
/* ------------------------------------------------------------------------ */

#define CUI__RESULT_W 320.0f
#define CUI__RESULT_ASPECT 1.25f

typedef struct { float hover, focus; cui_spring press; } cui__result_state;
typedef struct { cui_texture tex; float iw, ih; cui_color edge, fallback; } cui__result_paint;

/* the picture cropped to fill the card, a fade at the bottom for the caption, and the edge */
static void cui__paint_result(cui_ctx *ui, cui_node *n, void *data) {
    cui__result_paint *p = data;
    cui_rect r = n->rect;
    cui_corners rad = n->radius;
    if (p->tex) {
        cui_rect uv = { 0, 0, 1, 1 };
        if (p->iw > 0 && p->ih > 0 && r.w > 0 && r.h > 0) {
            float ia = p->iw / p->ih, ra = r.w / r.h;
            if (ia > ra) { uv.w = ra / ia; uv.x = (1 - uv.w) * 0.5f; }
            else { uv.h = ia / ra; uv.y = (1 - uv.h) * 0.5f; }
        }
        cui_draw_image(ui, p->tex, r, uv, rad, cui__rgba(255, 255, 255, 255));
    } else {
        cui_draw_rect(ui, r, rad, p->fallback);
    }
    /* the fade is dark in both themes: it sits on the picture, not on the page */
    float fh = r.h * 0.45f;
    cui_corners fr = { 0, 0, rad.br, rad.bl };
    cui_draw_rect_gradient(ui, (cui_rect){ r.x, r.y + r.h - fh, r.w, fh }, fr, cui__rgba(10, 10, 16, 0), cui__rgba(10, 10, 16, 200));
    cui_draw_rect_border(ui, r, rad, cui__rgba(0, 0, 0, 0), p->edge, 1);
}

static void cui__paint_result_focus(cui_ctx *ui, cui_node *n, void *data) {
    const cui_node *a = &ui->nodes[n->abs_anchor];
    cui__draw_focus_ring(ui, a->rect, a->radius, *(float *)data);
}

bool cui_result_card_ex(cui_ctx *ui, const char *id_str, cui_texture image, const char *title, const cui_result_card_opts *o) {
    cui_result_card_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, id_str);
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    cui__result_state *st = cui_state(ui, id, sizeof *st);
    if (st->press.x == 0 && st->press.v == 0) st->press.x = 1;
    cui__tween(ui, &st->hover, r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    cui__spring(ui, &st->press, r.pressed ? CUI__PRESS_WIDE : 1.0f, CUI_SPRING_SNAPPY);
    float aspect = o->aspect > 0 ? o->aspect : CUI__RESULT_ASPECT;
    cui_rect prev = cui__prev_rect(ui, id);
    float w = prev.w > 0 ? prev.w : (o->width.kind == CUI_SIZE_FIXED ? o->width.value : CUI__RESULT_W);
    int i = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[i];
    if (o->width.kind != CUI_SIZE_FIT) n->w = o->width;
    else { n->w = CUI_FILL; n->max_w = CUI__RESULT_W; }
    n->h = CUI_PX(roundf(w * aspect));
    n->self_align = CUI__ALIGN_NOSTRETCH;
    n->dir = CUI_COLUMN; n->justify = CUI_JUSTIFY_END; n->gap = 4;
    n->pad[1] = n->pad[3] = 16; n->pad[2] = 14; n->pad[0] = 12;
    n->radius = CUI_CORNERS(ui->theme.radius_surface);
    n->elevation = CUI_ELEVATION_RAISED;
    n->elevation_alpha = 1;
    n->scale = st->press.x;
    cui__result_paint *p = CUI_PAINT_DATA(ui, cui__result_paint);
    p->tex = image; p->iw = (float)o->image_w; p->ih = (float)o->image_h;
    p->edge = CUI_C(ui, BORDER);
    p->fallback = CUI_C(ui, SURFACE_MUTED);
    n->paint = cui__paint_result;
    n->paint_data = p;
    cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
    if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__open(ui, i);
    /* text on the picture stays light whatever the theme */
    cui_color white = cui__rgba(255, 255, 255, 255);
    if (o->badge && o->badge[0]) {
        int b = cui__absolute(ui, 0, 12, 12, 0, 0);
        cui_node *bn = &ui->nodes[b];
        bn->w = CUI_FIT; bn->h = CUI_FIT;
        bn->dir = CUI_ROW; bn->align = CUI_ALIGN_CENTER; bn->gap = 6;
        bn->pad[0] = bn->pad[2] = 4; bn->pad[1] = 9; bn->pad[3] = 7;
        bn->radius = CUI_CORNERS(12);
        bn->bg = cui__rgba(10, 12, 16, 140);
        cui__open(ui, b);
        int dot = cui__leaf(ui, 0, CUI_PX(6), CUI_PX(6));
        ui->nodes[dot].radius = CUI_CORNERS(3);
        cui_color tc = cui__tone_color(ui, o->badge_tone ? o->badge_tone : CUI_TONE_SUCCESS);
        ui->nodes[dot].bg = cui__shade(tc, 0.78f, 1.0f);   /* a light step of the tone, to read on the dark chip */
        int bt = cui__text_node(ui, o->badge, -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_xs + 0.5f), white, false);
        ui->nodes[bt].line_height = 1.3f;
        cui__close(ui);
    }
    if (title && title[0]) {
        int t = cui__text_node(ui, title, -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_lg - 1), white, true);
        ui->nodes[t].line_height = 1.3f;
    }
    if (o->caption && o->caption[0]) {
        int t = cui__text_node(ui, o->caption, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), cui_alpha(white, 0.78f), true);
        ui->nodes[t].line_height = 1.45f;
    }
    cui__close(ui);
    if (st->focus > 0.01f) {
        float *fa = CUI_PAINT_DATA(ui, float);
        *fa = st->focus;
        ui->last_node = i;
        cui__cv_overlay(ui, cui__paint_result_focus, fa);
    }
    ui->last_id = id;
    ui->last_node = i;
    return r.clicked;
}

/* ------------------------------------------------------------------------ */
/* Selection bar                                                            */
/* ------------------------------------------------------------------------ */

#define CUI__SELBAR_Z 900          /* over the page, under popups and dialogs */
#define CUI__SELBAR_RISE 10.0f     /* how far it slides up as it appears */

typedef struct { cui_spring t; int shown; float clear_hover, clear_focus; } cui__selbar_state;

/* the small clear button inside the count chip */
static bool cui__selbar_clear(cui_ctx *ui, cui_id id, cui__selbar_state *st) {
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    cui__tween(ui, &st->clear_hover, r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->clear_focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    int i = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[i];
    n->w = CUI_PX(20); n->h = CUI_PX(20);
    n->dir = CUI_STACK; n->align = CUI_ALIGN_CENTER; n->justify = CUI_JUSTIFY_CENTER;
    n->radius = CUI_CORNERS(6);
    n->bg = cui_alpha(CUI_C(ui, HOVER), st->clear_hover);
    cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
    if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__focus_ring(ui, i, st->clear_focus);
    cui__open(ui, i);
    cui__icon_node(ui, CUI_ICON_X, 14, cui_mix(CUI_C(ui, TEXT_MUTED), CUI_C(ui, FOREGROUND), st->clear_hover));
    cui__close(ui);
    ui->last_id = id;
    ui->last_node = i;
    cui_tooltip(ui, "Clear selection");
    return r.clicked;
}

int cui_selection_bar_ex(cui_ctx *ui, const char *id_str, int count, const cui_selection_action *actions, int action_count, const cui_selection_bar_opts *o) {
    cui_selection_bar_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (o->selected) {
        count = 0;
        for (int k = 0; k < o->item_count; k++) count += o->selected[k] ? 1 : 0;
    }
    cui_id anchor = o->anchor ? o->anchor : ui->last_id;
    cui_id id = cui_make_id(ui, id_str);
    cui__selbar_state *st = cui_state(ui, id, sizeof *st);
    int result = CUI_SELECTION_NONE;
    if (count > 0) st->shown = count;   /* while it leaves it keeps showing the last count */
    if (count > 0 && !o->no_escape && !ui->edit && !cui__keys_covered(ui) && cui_key_pressed(ui, CUI_KEY_ESCAPE)) {
        result = CUI_SELECTION_CLEARED;
        ui->kpressed[CUI_KEY_ESCAPE] = 0;
    }
    bool live = count > 0 && result == CUI_SELECTION_NONE;
    if (live) cui__spring(ui, &st->t, 1, CUI__SPRING_ENTER);
    else cui__spring(ui, &st->t, 0, CUI__SPRING_EXIT);
    float t = cui__clampf(st->t.x, 0, 1);
    if (t > 0.01f || live) {
        int root = cui__float_begin(ui, id, CUI__SELBAR_Z, anchor, CUI_PLACE_BOTTOM, 8);
        cui_node *n = &ui->nodes[root];
        n->dir = CUI_ROW; n->align = CUI_ALIGN_CENTER; n->gap = 4;
        n->pad[0] = n->pad[1] = n->pad[2] = 5; n->pad[3] = 6;
        n->radius = CUI_CORNERS(ui->theme.radius_panel);
        n->bg = CUI_C(ui, SURFACE_RAISED);
        n->border = 1;
        n->border_color = CUI_C(ui, BORDER);
        n->elevation = CUI_ELEVATION_FLOATING;
        n->elevation_alpha = 1;
        n->flags |= CUI_NF_HIT_OPAQUE;
        n->opacity = CUI_MAX(0.001f, t);
        n->ty = (1 - t) * CUI__SELBAR_RISE;
        cui_push_id(ui, id_str);
        cui_push_disabled(ui, !live);   /* a leaving bar takes no clicks */
        int chip = cui__node_new(ui, 0);
        cui_node *cn = &ui->nodes[chip];
        cn->dir = CUI_ROW; cn->align = CUI_ALIGN_CENTER; cn->gap = 6;
        cn->h = CUI_PX(32); cn->pad[1] = 6; cn->pad[3] = 10;
        cn->radius = CUI_CORNERS(9);
        cn->bg = CUI_C(ui, ACTIVE);
        cui__open(ui, chip);
        int ct = cui__text_node(ui, cui__frame_printf(ui, o->count_format ? o->count_format : "%d selected", st->shown), -1, CUI_FONT_MEDIUM,
                                cui__fs(ui, ui->theme.text_body), CUI_C(ui, FOREGROUND), false);
        ui->nodes[ct].flags |= CUI_NF_TABULAR;
        ui->nodes[ct].line_height = 1.3f;
        if (cui__selbar_clear(ui, cui_make_id(ui, "#clear"), st) && live) result = CUI_SELECTION_CLEARED;
        cui__close(ui);
        if (action_count > 0) {
            int sb = cui__node_new(ui, 0);
            ui->nodes[sb].pad[1] = ui->nodes[sb].pad[3] = 2;
            cui__open(ui, sb);
            int l = cui__leaf(ui, 0, CUI_PX(1), CUI_PX(18));
            ui->nodes[l].bg = CUI_C(ui, BORDER);
            cui__close(ui);
        }
        for (int k = 0; k < action_count && actions; k++) {
            const cui_selection_action *a = &actions[k];
            cui_button_opts bo;
            memset(&bo, 0, sizeof bo);
            bo.variant = CUI_BUTTON_GHOST;
            bo.size = CUI_SIZE_SM;
            bo.icon = a->icon;
            bo.color = a->tone != CUI_TONE_NEUTRAL ? cui__tone_color(ui, a->tone) : CUI_C(ui, TEXT_SECONDARY);
            cui_push_id_int(ui, k);
            if (cui_button_ex(ui, a->label ? a->label : "", &bo) && live) result = k;
            cui_pop_id(ui);
        }
        cui_pop_disabled(ui);
        cui_pop_id(ui);
        cui__float_end(ui);
    }
    if (result == CUI_SELECTION_CLEARED && o->selected)
        for (int k = 0; k < o->item_count; k++) o->selected[k] = false;
    ui->last_id = id;
    return result;
}
