/* Creative controls: the status tag and the ratio slider, and the controls that sit in a
 * control well (a rounded panel with a label, a value and the control under them): the
 * gradient slider, the step slider, the palette strip, the aspect ratio picker, the option
 * field with action and the seed field. The well and its action button are shared here. */
#include "cui_internal.h"

/* ------------------------------------------------------------------------ */
/* Control well                                                             */
/* ------------------------------------------------------------------------ */

#define CUI__WELL_BODY_H 32.0f      /* the control row, so wells side by side line up */
#define CUI__WELL_TOOL_SALT 0x7001u /* the action button's id, from the well's */

static cui_color cui__well_bg(cui_ctx *ui) {
    return cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, SURFACE_MUTED), ui->mode == CUI_THEME_DARK ? 0.45f : 0.75f);
}

/* a field sunk into the well: darker than the well in both themes */
static cui_color cui__well_inset(cui_ctx *ui) {
    cui_color w = cui__well_bg(ui);
    return ui->mode == CUI_THEME_DARK ? cui_mix(w, CUI_C(ui, BACKGROUND), 0.7f) : cui_mix(w, CUI_C(ui, FOREGROUND), 0.045f);
}

/* Opens the well (or, plain, a bare column) with its label row, then the row the control
 * goes in; cui__well_end closes both. Returns the well's node. */
static int cui__well_begin(cui_ctx *ui, cui_id id, const char *label, const char *value, bool plain) {
    int w = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[w];
    n->gap = 10;
    n->w = CUI_FILL;
    n->self_align = CUI_ALIGN_STRETCH;
    if (!plain) {
        n->pad[0] = n->pad[1] = n->pad[2] = 12;
        n->pad[3] = 14;
        n->bg = cui__well_bg(ui);
        n->border = 1;
        n->border_color = ui->mode == CUI_THEME_DARK ? CUI_C(ui, BORDER_SUBTLE) : CUI_C(ui, BORDER);
        n->radius = CUI_CORNERS(ui->theme.radius_panel + 2);
    }
    cui__open(ui, w);
    bool named = label && cui__display_len(label) > 0;
    if (named || value) {
        int row = cui__node_new(ui, 0);
        cui_node *rn = &ui->nodes[row];
        rn->dir = CUI_ROW; rn->align = CUI_ALIGN_CENTER; rn->gap = 12; rn->w = CUI_FILL;
        cui__open(ui, row);
        if (named) cui__label_text(ui, label, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body), CUI_C(ui, FOREGROUND));
        cui_spacer(ui);
        if (value) {
            int t = cui__text_node(ui, value, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_body), CUI_C(ui, TEXT_MUTED), false);
            ui->nodes[t].flags |= CUI_NF_TABULAR;
            ui->nodes[t].line_height = 1.3f;
        }
        cui__close(ui);
    }
    int body = cui__node_new(ui, 0);
    cui_node *b = &ui->nodes[body];
    b->dir = CUI_ROW; b->align = CUI_ALIGN_CENTER; b->gap = 10; b->w = CUI_FILL;
    b->min_h = CUI__WELL_BODY_H;
    cui__open(ui, body);
    return w;
}

static void cui__well_end(cui_ctx *ui) {
    cui__close(ui);
    cui__close(ui);
}

/* the icon button at the end of a well's row; `on` shows it pressed */
static bool cui__well_tool(cui_ctx *ui, cui_id id, const char *icon, const char *tooltip, bool on, bool disabled) {
    cui_button_opts b;
    memset(&b, 0, sizeof b);
    b.variant = CUI_BUTTON_GHOST;
    b.size = CUI_SIZE_SM;
    b.tooltip = tooltip;
    b.selected = on;
    b.disabled = disabled;
    if (on) b.color = CUI_COLOR_ACCENT;
    return cui__icon_button_id(ui, id, icon, &b);
}

/* records the reading direction a node was laid out in, for the next frame's keys and pointer */
static void cui__paint_note_rtl(cui_ctx *ui, cui_node *n, void *data) {
    (void)ui;
    *(bool *)data = n->rtl;
}

/* Left and Right as previous (-1) and next (+1) the way they are drawn: swapped right to left */
static int cui__arrow_step(cui_ctx *ui, bool rtl) {
    int d = 0;
    if (cui_key_pressed(ui, CUI_KEY_RIGHT)) d += rtl ? -1 : 1;
    if (cui_key_pressed(ui, CUI_KEY_LEFT)) d += rtl ? 1 : -1;
    return d;
}

/* a round thumb as the sliders draw it: two soft shadows, a light disc with a hairline edge */
static void cui__draw_round_thumb(cui_ctx *ui, cui_rect tr) {
    bool dark = ui->mode == CUI_THEME_DARK;
    cui_corners rad = CUI_CORNERS(tr.w * 0.5f);
    cui_draw_shadow(ui, tr, rad, 2, 0, (cui_vec2){ 0, 1 }, cui__rgba(0, 0, 0, dark ? 90 : 38));
    cui_draw_shadow(ui, tr, rad, 6, 0, (cui_vec2){ 0, 2 }, cui__rgba(0, 0, 0, dark ? 50 : 16));
    cui_draw_rect_border(ui, tr, rad, CUI_C(ui, CONTROL_THUMB), cui__rgba(0, 0, 0, 22), 0.5f);
}

/* x mirrored inside r when it was laid out right to left */
static float cui__mirror_x(cui_rect r, bool rtl, float x) { return rtl ? 2.0f * r.x + r.w - x : x; }

/* ------------------------------------------------------------------------ */
/* Status tag                                                               */
/* ------------------------------------------------------------------------ */

typedef struct { float x, focus; cui_spring press; } cui__stag_state;
typedef struct { cui_color c; float size, angle; } cui__stag_ring;

#define CUI__STAG_TURN 2.4f   /* seconds for one turn of the "in progress" ring */

static const char *const cui__stag_names[] = { "Pending", "In progress", "Submitted", "In review", "Success", "Failed", "Expired", "" };

static cui_color cui__stag_color(cui_ctx *ui, cui_tag_status s) {
    cui_theme *t = &ui->theme;
    switch (s) {
    case CUI_TAG_PENDING: return cui_mix(t->tone_amber, CUI_C(ui, DANGER), 0.45f);   /* orange, between amber and red */
    case CUI_TAG_IN_PROGRESS: return t->tone_sky;
    case CUI_TAG_SUBMITTED: return t->tone_violet;
    case CUI_TAG_IN_REVIEW: return t->tone_amber;
    case CUI_TAG_SUCCESS: return CUI_C(ui, SUCCESS);
    case CUI_TAG_FAILED: return CUI_C(ui, DANGER);
    default: return CUI_C(ui, TEXT_MUTED);   /* expired, custom */
    }
}

static const char *cui__stag_icon(cui_tag_status s) {
    switch (s) {
    case CUI_TAG_PENDING: return CUI_ICON_TRIANGLE_ALERT;
    case CUI_TAG_SUBMITTED: return CUI_ICON_SEND;
    case CUI_TAG_IN_REVIEW: return CUI_ICON_EYE;
    case CUI_TAG_SUCCESS: return CUI_ICON_CIRCLE_CHECK;
    case CUI_TAG_FAILED: return CUI_ICON_CIRCLE_X;
    case CUI_TAG_EXPIRED: return CUI_ICON_CLOCK;
    default: return NULL;   /* in progress draws its ring; custom has only yours */
    }
}

/* the "in progress" icon: a ring of eight dashes, turned by angle */
static void cui__paint_stag_ring(cui_ctx *ui, cui_node *n, void *data) {
    cui__stag_ring *p = data;
    cui_vec2 c = { n->rect.x + n->rect.w * 0.5f, n->rect.y + n->rect.h * 0.5f };
    float rad = p->size * 0.375f, th = CUI_MAX(1.25f, p->size * 0.075f), slot = 2 * CUI_PI / 8;
    for (int k = 0; k < 8; k++) {
        float a0 = p->angle + k * slot;
        cui_draw_arc(ui, c, rad, a0, a0 + slot * 0.5f, th, p->c);
    }
}

bool cui_status_tag_ex(cui_ctx *ui, const char *label, cui_tag_status status, const cui_status_tag_opts *o) {
    cui_status_tag_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if ((int)status < 0 || status > CUI_TAG_CUSTOM) status = CUI_TAG_CUSTOM;
    const char *text = label ? label : cui__stag_names[status];
    bool sm = o->size == CUI_SIZE_SM, dark = ui->mode == CUI_THEME_DARK;
    float h = sm ? 24 : 30, icon_size = sm ? 14 : 16, gap = sm ? 5 : 6;
    cui_color tone = o->tone != CUI_TONE_NEUTRAL ? cui__tone_color(ui, o->tone) : cui__stag_color(ui, status);
    cui_color under = cui_mix(CUI_C(ui, SURFACE), tone, dark ? 0.14f : 0.10f);
    cui_color fg = cui__legible(tone, under, 4.5f);

    /* input: only a removable tag is a control */
    cui_id id = 0;
    cui__stag_state *st = NULL;
    cui_interact r;
    memset(&r, 0, sizeof r);
    bool removed = false;
    if (o->removable) {
        id = cui_make_id(ui, o->id ? o->id : text);
        r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
        st = cui_state(ui, id, sizeof *st);
        removed = r.clicked || (r.keyboard_focused && (cui_key_pressed(ui, CUI_KEY_DELETE) || cui_key_pressed(ui, CUI_KEY_BACKSPACE)));
        cui__tween(ui, &st->x, (r.hovered || r.focus_visible) ? 1.0f : 0.0f, CUI__T_HOVER);
        cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
        if (st->press.x == 0 && st->press.v == 0) st->press.x = 1;
        cui__spring(ui, &st->press, r.pressed ? CUI__PRESS : 1.0f, CUI_SPRING_SNAPPY);
    }

    /* build */
    int i = cui__node_new(ui, id);
    cui_node *n = &ui->nodes[i];
    n->dir = CUI_ROW; n->align = CUI_ALIGN_CENTER;
    n->h = CUI_PX(h);
    n->pad[3] = sm ? 6 : 9;
    n->pad[1] = sm ? 8 : 11;
    n->radius = CUI_CORNERS(sm ? 7 : 9);
    n->bg = cui_alpha(tone, dark ? 0.14f : 0.10f);
    n->border = 1;
    n->border_color = cui_alpha(tone, dark ? 0.18f : 0.16f);
    n->self_align = CUI__ALIGN_NOSTRETCH;
    if (st) {
        n->scale = st->press.x;
        cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
        if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    }
    cui__open(ui, i);
    const char *icon = o->icon ? o->icon : cui__stag_icon(status);
    bool has_icon = true;
    if (icon) cui__icon_node(ui, icon, icon_size, fg);
    else if (status == CUI_TAG_IN_PROGRESS) {
        int ring = cui__leaf(ui, 0, CUI_PX(icon_size), CUI_PX(icon_size));
        cui__stag_ring *p = CUI_PAINT_DATA(ui, cui__stag_ring);
        p->c = fg; p->size = icon_size; p->angle = 0;
        if (ui->theme.motion > 0) {
            p->angle = (float)fmod(ui->time, CUI__STAG_TURN) / CUI__STAG_TURN * 2 * CUI_PI;
            ui->animating = true;
        }
        ui->nodes[ring].paint = cui__paint_stag_ring;
        ui->nodes[ring].paint_data = p;
    } else has_icon = false;
    if (has_icon) cui__leaf(ui, 0, CUI_PX(gap), CUI_PX(1));
    int t = cui__text_node(ui, text, cui__display_len(text), CUI_FONT_MEDIUM, cui__fs(ui, sm ? ui->theme.text_sm : ui->theme.text_body), fg, false);
    ui->nodes[t].line_height = 1.2f;
    if (st && st->x > 0.01f) {
        /* the x slides open on hover and focus */
        cui__leaf(ui, 0, CUI_PX(gap * st->x), CUI_PX(1));
        int xb = cui__node_new(ui, 0);
        cui_node *xn = &ui->nodes[xb];
        xn->dir = CUI_STACK; xn->align = CUI_ALIGN_CENTER; xn->justify = CUI_JUSTIFY_CENTER;
        xn->w = CUI_PX(14 * st->x); xn->h = CUI_PX(14);
        xn->flags |= CUI_NF_CLIP;
        cui__open(ui, xb);
        cui__icon_node(ui, CUI_ICON_X, 12, cui_alpha(fg, 0.75f * st->x));
        cui__close(ui);
    }
    cui__close(ui);
    if (st) {
        cui__focus_ring(ui, i, st->focus);
        ui->last_id = id;
    }
    return removed;
}

/* ------------------------------------------------------------------------ */
/* Ratio slider                                                             */
/* ------------------------------------------------------------------------ */

#define CUI__RATIO_HANDLE 18.0f   /* the gap the bar sits in */
#define CUI__RATIO_PAD 14.0f      /* text inset at each part's outer edge */
#define CUI__RATIO_GAP 8.0f       /* between a part's name and its percentage */

typedef struct { cui_spring t; float hover, focus, active; bool init, dragging, rtl; } cui__ratio_state;
typedef struct {
    float t, grow, focus, fs;
    int pa;                    /* the first part's percentage */
    bool names;                /* room for the names */
    const char *a, *b;
    int alen, blen;
    cui_color a_bg, a_name, a_pct, b_bg, b_name, b_pct, bar;
    bool *rtl_out;
} cui__ratio_paint;

/* The shares the parts need so each keeps its name and percentage readable at this width.
 * When both names do not fit, only the percentages are kept. */
static void cui__ratio_limits(cui_ctx *ui, const char *a, const char *b, float width, float *lo, float *hi, bool *names) {
    float fs = cui__fs(ui, ui->theme.text_lg);
    float pct = cui__text_width(ui, CUI_FONT_MONO, fs, "100%", 4, true);
    float avail = width - CUI__RATIO_HANDLE;
    float na = 2 * CUI__RATIO_PAD + cui__text_width(ui, CUI_FONT_MONO, fs, a, cui__display_len(a), false) + CUI__RATIO_GAP + pct;
    float nb = 2 * CUI__RATIO_PAD + cui__text_width(ui, CUI_FONT_MONO, fs, b, cui__display_len(b), false) + CUI__RATIO_GAP + pct;
    *names = true;
    if (avail <= 0) { *lo = 0.05f; *hi = 0.95f; return; }
    if (na + nb > avail) { *names = false; na = nb = 2 * CUI__RATIO_PAD + pct; }
    *lo = na / avail;
    *hi = 1 - nb / avail;
    if (*lo > *hi) *lo = *hi = 0.5f;
}

/* one part's name and percentage, from its outer edge inward */
static void cui__ratio_text(cui_ctx *ui, const cui__ratio_paint *p, cui_rect part, bool from_left, const char *name, int len,
                            int pct, cui_color nc, cui_color pc) {
    char num[8], nm[64];
    snprintf(num, sizeof num, "%d%%", pct);
    snprintf(nm, sizeof nm, "%.*s", len, name);
    float cy = part.y + part.h * 0.5f;
    float x = from_left ? part.x + CUI__RATIO_PAD : part.x + part.w - CUI__RATIO_PAD;
    cui__text_anchor at = from_left ? CUI__AT_LEFT : CUI__AT_RIGHT;
    cui_push_clip(ui, part);
    if (p->names && len > 0) {
        cui__draw_text_at(ui, CUI_FONT_MONO, p->fs, x, cy, at, nc, nm, false);
        float w = cui__text_width(ui, CUI_FONT_MONO, p->fs, nm, -1, false) + CUI__RATIO_GAP;
        x += from_left ? w : -w;
    }
    cui__draw_text_at(ui, CUI_FONT_MONO, p->fs, x, cy, at, pc, num, true);
    cui_pop_clip(ui);
}

static void cui__paint_ratio(cui_ctx *ui, cui_node *n, void *data) {
    cui__ratio_paint *p = data;
    if (p->rtl_out) *p->rtl_out = n->rtl;
    cui_rect r = n->rect;
    float avail = CUI_MAX(0.0f, r.w - CUI__RATIO_HANDLE), aw = avail * cui__clampf(p->t, 0, 1);
    float rad = CUI_MIN(ui->theme.radius_control, r.h * 0.5f);
    cui_rect a = { r.x, r.y, aw, r.h }, b = { r.x + aw + CUI__RATIO_HANDLE, r.y, avail - aw, r.h };
    float hx = cui__mirror_x(r, n->rtl, r.x + aw + CUI__RATIO_HANDLE * 0.5f);
    if (n->rtl) { a.x = cui__mirror_x(r, true, a.x + a.w); b.x = cui__mirror_x(r, true, b.x + b.w); }
    cui_draw_rect(ui, a, CUI_CORNERS(rad), p->a_bg);
    cui_draw_rect(ui, b, CUI_CORNERS(rad), p->b_bg);
    /* each part reads from its outer edge: the first from the start, the second from the end */
    cui__ratio_text(ui, p, a, !n->rtl, p->a, p->alen, p->pa, p->a_name, p->a_pct);
    cui__ratio_text(ui, p, b, n->rtl, p->b, p->blen, 100 - p->pa, p->b_name, p->b_pct);
    float bh = CUI_MIN(r.h - 6, 36 + 10 * p->grow);
    cui_rect bar = { hx - 2, r.y + (r.h - bh) * 0.5f, 4, bh };
    cui_draw_rect(ui, bar, CUI_CORNERS(2), p->bar);
    cui__draw_focus_ring(ui, cui__rect_inset(bar, -2), CUI_CORNERS(4), p->focus);
}

bool cui_ratio_slider_ex(cui_ctx *ui, const char *first, const char *second, float *ratio, const cui_ratio_slider_opts *o) {
    cui_ratio_slider_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!first) first = "";
    if (!second) second = "";
    cui_id id = cui__hash_str(second, cui_make_id(ui, first));
    bool disabled = o->disabled || cui_is_disabled(ui);
    cui_push_disabled(ui, disabled);
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE | CUI_IF_DRAG);
    cui_pop_disabled(ui);
    cui__ratio_state *st = cui_state(ui, id, sizeof *st);

    /* input: the split stops where a part would get too narrow for its text */
    float old = *ratio, t = cui__clampf(*ratio, 0, 1);
    cui_rect pr = cui__prev_rect(ui, id);
    float lo, hi;
    bool names;
    cui__ratio_limits(ui, first, second, pr.w, &lo, &hi, &names);
    float step = o->step > 0 ? o->step : 0.01f;
    if (!st->init) { st->t.x = cui__clampf(t, lo, hi); st->init = true; }
    bool jumped = false;
    if (r.active && ui->mdown[0] && pr.w > CUI__RATIO_HANDLE) {
        float mx = cui__mirror_x(pr, st->rtl, ui->mx);
        float v = (mx - pr.x - CUI__RATIO_HANDLE * 0.5f) / (pr.w - CUI__RATIO_HANDLE);
        if (ui->mpressed[0]) { st->dragging = true; jumped = fabsf(v - t) > 0.03f; }
        t = cui__clampf(roundf(v / step) * step, lo, hi);
        *ratio = t;
    } else {
        st->dragging = false;
        if (r.keyboard_focused) {
            float k = cui__slider_keys(ui, t, step, st->rtl);
            if (k != t) *ratio = t = cui__clampf(roundf(k / step) * step, lo, hi);
        }
    }
    float shown = cui__clampf(t, lo, hi);
    if (jumped || !st->dragging) cui__spring(ui, &st->t, shown, CUI_SPRING_SMOOTH);
    else { st->t.x = shown; st->t.v = 0; }
    cui__tween(ui, &st->hover, (r.hovered || r.active) ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    if (r.hovered || r.active) cui__set_cursor(ui, disabled ? CUI_CURSOR_NOT_ALLOWED : CUI_CURSOR_RESIZE_EW);

    /* build */
    float h = o->height > 0 ? o->height : 52;
    int i = cui__leaf(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h));
    cui_node *n = &ui->nodes[i];
    n->self_align = CUI_ALIGN_STRETCH;
    n->opacity = disabled ? CUI__DISABLED : 1.0f;
    cui__mark_interactive(ui, i, CUI_IF_FOCUSABLE);
    cui_color tone = cui__tone_color(ui, o->tone == CUI_TONE_NEUTRAL ? CUI_TONE_ACCENT : o->tone);
    cui_color under = cui_mix(CUI_C(ui, BACKGROUND), tone, 0.18f);
    cui__ratio_paint *p = CUI_PAINT_DATA(ui, cui__ratio_paint);
    p->t = st->t.x; p->grow = st->hover; p->focus = st->focus;
    p->fs = cui__fs(ui, ui->theme.text_lg);
    p->pa = (int)lroundf(cui__clampf(*ratio, 0, 1) * 100);
    p->names = names;
    p->a = first; p->alen = cui__display_len(first);
    p->b = second; p->blen = cui__display_len(second);
    p->a_bg = cui_alpha(tone, 0.18f);
    p->a_name = cui__legible(cui_mix(tone, CUI_C(ui, BACKGROUND), 0.25f), under, 3.0f);
    p->a_pct = cui__legible(tone, under, 4.5f);
    p->b_bg = CUI_C(ui, CONTROL_TRACK);
    p->b_name = cui__legible(CUI_C(ui, TEXT_MUTED), p->b_bg, 3.0f);
    p->b_pct = CUI_C(ui, FOREGROUND);
    p->bar = CUI_C(ui, FOREGROUND);
    p->rtl_out = &st->rtl;
    n->paint = cui__paint_ratio;
    n->paint_data = p;
    ui->last_id = id;
    return *ratio != old;
}

/* ------------------------------------------------------------------------ */
/* Gradient slider                                                          */
/* ------------------------------------------------------------------------ */

#define CUI__GS_PAD 12.0f     /* the thumb's center stops this far inside each end of the track */
#define CUI__GS_THUMB 22.0f
#define CUI__GS_TRACK_H 24.0f
#define CUI__GS_SWATCH 26.0f

typedef struct { cui_spring thumb; float hover, focus, active; bool init, dragging, rtl; } cui__gs_state;
typedef struct {
    const cui_gradient_stop *stops; int n;
    bool checker;
    float t, active, focus;
    cui_color light, dark, under;
    bool *rtl_out;
} cui__gs_paint;

/* hue: the six corners of the color wheel and back to red; linear between them is the HSV hue */
static const cui_gradient_stop cui__gs_hue[7] = {
    { 0.0f, { 255, 0, 0, 255 } }, { 1.0f / 6, { 255, 255, 0, 255 } }, { 2.0f / 6, { 0, 255, 0, 255 } },
    { 3.0f / 6, { 0, 255, 255, 255 } }, { 4.0f / 6, { 0, 0, 255, 255 } }, { 5.0f / 6, { 255, 0, 255, 255 } },
    { 1.0f, { 255, 0, 0, 255 } },
};
/* white balance: a cool blue through neutral to a warm amber, as photo editors show it */
static const cui_gradient_stop cui__gs_temperature[4] = {
    { 0.0f, { 0x5b, 0x8d, 0xef, 255 } }, { 1.0f / 3, { 0xc9, 0xd6, 0xe8, 255 } },
    { 2.0f / 3, { 0xf2, 0xe2, 0xb8, 255 } }, { 1.0f, { 0xe8, 0xb4, 0x4a, 255 } },
};

static const cui_gradient_stop *cui__gs_stops(cui_ctx *ui, const cui_gradient_slider_opts *o, int *n) {
    if (o->stops && o->stop_count > 0) { *n = o->stop_count; return o->stops; }
    if (o->track == CUI_GRADIENT_TEMPERATURE) { *n = 4; return cui__gs_temperature; }
    if (o->track == CUI_GRADIENT_OPACITY) {
        cui_gradient_stop *s = cui__frame_alloc(ui, sizeof *s * 2);
        cui_color c = cui__color_set(o->color) ? cui_color_of(ui, o->color) : cui__emphasis(ui);
        c.a = 255;
        s[0].pos = 0; s[0].color = c; s[0].color.a = 0;
        s[1].pos = 1; s[1].color = c;
        *n = 2;
        return s;
    }
    *n = 7;
    return cui__gs_hue;
}

static void cui__paint_gradient_slider(cui_ctx *ui, cui_node *nd, void *data) {
    cui__gs_paint *p = data;
    if (p->rtl_out) *p->rtl_out = nd->rtl;
    cui_rect r = nd->rect;
    if (r.w <= 2 * CUI__GS_PAD || p->n <= 0) return;
    float rad = r.h * 0.5f, span = r.w - 2 * CUI__GS_PAD;
    /* the stops laid along the thumb's travel (and mirrored right to left), so the color
     * under the thumb is the value's color */
    cui_gradient_stop *s = cui__frame_alloc(ui, sizeof *s * (size_t)p->n);
    for (int k = 0; k < p->n; k++) {
        float pos = (CUI__GS_PAD + cui__clampf(p->stops[k].pos, 0, 1) * span) / r.w;
        s[k].pos = nd->rtl ? 1 - pos : pos;
        s[k].color = p->stops[k].color;
    }
    if (p->checker) cui__draw_checker(ui, r, rad, 5.0f, p->light, p->dark);
    cui__draw_gradient_ramp(ui, r, rad, s, p->n);
    float cx = cui__mirror_x(r, nd->rtl, r.x + CUI__GS_PAD + cui__clampf(p->t, 0, 1) * span);
    float ts = CUI__GS_THUMB * (1.0f + 0.12f * p->active);
    cui_rect tr = { cx - ts * 0.5f, r.y + (r.h - ts) * 0.5f, ts, ts };
    cui__draw_round_thumb(ui, tr);
    cui_rect fill = cui__rect_inset(tr, 2.5f);
    cui_draw_rect_border(ui, fill, CUI_CORNERS(fill.w * 0.5f), p->under, cui__rgba(0, 0, 0, 38), 0.75f);
    cui__draw_focus_ring(ui, tr, CUI_CORNERS(ts * 0.5f), p->focus);
}

static void cui__paint_gs_swatch(cui_ctx *ui, cui_node *n, void *data) {
    cui_color *c = data;
    cui_draw_rect_border(ui, n->rect, CUI_CORNERS(n->rect.w * 0.5f), *c, cui_alpha(CUI_C(ui, FOREGROUND), 0.15f), 1);
}

/* the value at the right of the label: the caller's text, the caller's format, or the track's */
static const char *cui__gs_value_text(cui_ctx *ui, const cui_gradient_slider_opts *o, float v, float t) {
    if (o->value) return o->value;
    if (o->format) return cui__frame_printf(ui, o->format, (double)v);
    if (o->stops && o->stop_count > 0) return cui__frame_printf(ui, "%.0f", (double)v);
    switch (o->track) {
    case CUI_GRADIENT_TEMPERATURE: return cui__frame_printf(ui, "%.0f K", (double)v);
    case CUI_GRADIENT_OPACITY: return cui__frame_printf(ui, "%.0f%%", (double)(t * 100));
    default: return cui__frame_printf(ui, "%.0f\xc2\xb0", (double)v);
    }
}

int cui_gradient_slider_ex(cui_ctx *ui, const char *label, float *value, float min, float max, const cui_gradient_slider_opts *o) {
    cui_gradient_slider_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (max <= min) max = min + 1;
    cui_id id = cui_make_id(ui, label ? label : "gradient");
    cui_id track_id = id ^ 0x67A5u;
    bool disabled = o->disabled || cui_is_disabled(ui);
    cui_push_disabled(ui, disabled);
    cui_interact r = cui__interact(ui, track_id, CUI_IF_FOCUSABLE | CUI_IF_DRAG);
    cui_pop_disabled(ui);
    cui__gs_state *st = cui_state(ui, id, sizeof *st);

    /* input, as the slider: the pointer sets the value, the keys step it */
    float old = *value, t = cui__clampf((*value - min) / (max - min), 0, 1);
    if (!st->init) { st->thumb.x = t; st->init = true; }
    cui_rect pr = cui__prev_rect(ui, track_id);
    float x0 = pr.x + CUI__GS_PAD, x1 = pr.x + pr.w - CUI__GS_PAD;
    bool jumped = false;
    if (r.active && ui->mdown[0] && x1 > x0) {
        float mt = cui__clampf((cui__mirror_x(pr, st->rtl, ui->mx) - x0) / (x1 - x0), 0, 1);
        if (ui->mpressed[0]) { st->dragging = true; jumped = fabsf(mt - t) > 0.02f; }
        t = mt;
    } else {
        st->dragging = false;
    }
    if (r.keyboard_focused) t = cui__slider_keys(ui, t, o->step > 0 ? o->step / (max - min) : 0.01f, st->rtl);
    float v = min + t * (max - min);
    if (o->step > 0) v = min + roundf((v - min) / o->step) * o->step;
    *value = v = cui__clampf(v, min, max);
    t = (v - min) / (max - min);
    if (jumped || !st->dragging) cui__spring(ui, &st->thumb, t, CUI_SPRING_SMOOTH);
    else { st->thumb.x = t; st->thumb.v = 0; }
    cui__tween(ui, &st->hover, (r.hovered || r.active) ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->active, r.active ? 1.0f : 0.0f, CUI__T_STATE);
    cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    if (r.hovered || r.active) cui__set_cursor(ui, disabled ? CUI_CURSOR_NOT_ALLOWED : CUI_CURSOR_HAND);

    int n = 0;
    const cui_gradient_stop *stops = cui__gs_stops(ui, o, &n);
    cui_color c = cui_gradient_eval(stops, n, t), solid = c;
    solid.a = 255;
    cui_color under = cui_mix(cui__well_bg(ui), solid, c.a / 255.0f);   /* what the color looks like on the well */
    if (o->color_out) *o->color_out = c;

    /* build */
    int w = cui__well_begin(ui, id, label, cui__gs_value_text(ui, o, v, t), o->plain);
    ui->nodes[w].opacity = disabled ? CUI__DISABLED : 1.0f;
    int tr = cui__leaf(ui, track_id, CUI_FILL, CUI_PX(CUI__GS_TRACK_H));
    cui__mark_interactive(ui, tr, CUI_IF_FOCUSABLE);
    cui__gs_paint *p = CUI_PAINT_DATA(ui, cui__gs_paint);
    p->stops = stops; p->n = n;
    p->checker = o->track == CUI_GRADIENT_OPACITY && !(o->stops && o->stop_count > 0);
    if (o->stops) for (int k = 0; k < n; k++) if (stops[k].color.a < 255) p->checker = true;
    p->t = st->thumb.x; p->active = st->active; p->focus = st->focus;
    p->light = CUI_C(ui, SURFACE);
    p->dark = cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, FOREGROUND), ui->mode == CUI_THEME_DARK ? 0.16f : 0.1f);
    p->under = under;
    p->rtl_out = &st->rtl;
    ui->nodes[tr].paint = cui__paint_gradient_slider;
    ui->nodes[tr].paint_data = p;
    if (o->swatch) {
        int sw = cui__leaf(ui, 0, CUI_PX(CUI__GS_SWATCH), CUI_PX(CUI__GS_SWATCH));
        cui_color *sc = CUI_PAINT_DATA(ui, cui_color);
        *sc = under;
        ui->nodes[sw].paint = cui__paint_gs_swatch;
        ui->nodes[sw].paint_data = sc;
    }
    int flags = *value != old ? CUI_WELL_CHANGED : 0;
    if (o->tool && cui__well_tool(ui, id ^ CUI__WELL_TOOL_SALT, o->tool, o->tool_tooltip, o->tool_on, disabled)) flags |= CUI_WELL_ACTION;
    cui__well_end(ui);
    ui->last_id = track_id;
    return flags;
}

/* ------------------------------------------------------------------------ */
/* Step slider                                                              */
/* ------------------------------------------------------------------------ */

#define CUI__STEP_PAD 10.0f    /* the thumb's center stops this far inside each end */
#define CUI__STEP_THUMB 20.0f

typedef struct { cui_spring pos; float hover, focus, active; bool init, rtl; } cui__step_slider_state;
typedef struct { int stops; float pos, focus, active; cui_color rail, fill, dot, hole; bool *rtl_out; } cui__step_slider_paint;

static void cui__paint_step_slider(cui_ctx *ui, cui_node *nd, void *data) {
    cui__step_slider_paint *p = data;
    if (p->rtl_out) *p->rtl_out = nd->rtl;
    cui_rect r = nd->rect;
    float x0 = r.x + CUI__STEP_PAD, x1 = r.x + r.w - CUI__STEP_PAD, cy = r.y + r.h * 0.5f;
    if (x1 <= x0) return;
    bool rtl = nd->rtl;
    float tx = x0 + (x1 - x0) * cui__clampf(p->pos, 0, 1);
    cui_rect rail = { x0 - 2, cy - 2, x1 - x0 + 4, 4 };
    cui_draw_rect(ui, rail, CUI_CORNERS(2), p->rail);
    cui_rect fill = { x0 - 2, cy - 2, tx - x0 + 2, 4 };
    if (rtl) fill.x = cui__mirror_x(r, true, fill.x + fill.w);
    cui_draw_rect(ui, fill, CUI_CORNERS(2), p->fill);
    float gap = p->stops > 1 ? (x1 - x0) / (p->stops - 1) : 0;
    if (gap >= 8)
        for (int k = 0; k < p->stops; k++) {
            float f = (float)k / (p->stops - 1);
            float dx = cui__mirror_x(r, rtl, x0 + gap * k);
            bool inside = f < p->pos - 0.001f;   /* dots under the filled part show as small holes */
            cui_draw_circle(ui, (cui_vec2){ dx, cy }, inside ? 2.0f : 3.0f, inside ? p->hole : p->dot);
        }
    float ts = CUI__STEP_THUMB * (1.0f - 0.06f * p->active);
    float cx = cui__mirror_x(r, rtl, tx);
    cui_rect tr = { cx - ts * 0.5f, cy - ts * 0.5f, ts, ts };
    cui__draw_round_thumb(ui, tr);
    cui__draw_focus_ring(ui, tr, CUI_CORNERS(ts * 0.5f), p->focus);
}

int cui_step_slider_ex(cui_ctx *ui, const char *label, int *value, int min, int max, const cui_step_slider_opts *o) {
    cui_step_slider_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (max <= min) max = min + 1;
    int stops = max - min + 1;
    cui_id id = cui_make_id(ui, label ? label : "steps");
    cui_id track_id = id ^ 0x57E9u;
    bool disabled = o->disabled || cui_is_disabled(ui);
    cui_push_disabled(ui, disabled);
    cui_interact r = cui__interact(ui, track_id, CUI_IF_FOCUSABLE | CUI_IF_DRAG);
    cui_pop_disabled(ui);
    cui__step_slider_state *st = cui_state(ui, id, sizeof *st);

    /* input: the pointer and the keys pick a stop, never a place between two */
    int old = *value, k = CUI_CLAMP(*value, min, max) - min;
    float unit = 1.0f / (stops - 1);
    cui_rect pr = cui__prev_rect(ui, track_id);
    float x0 = pr.x + CUI__STEP_PAD, x1 = pr.x + pr.w - CUI__STEP_PAD;
    if (r.active && ui->mdown[0] && x1 > x0) {
        float mt = cui__clampf((cui__mirror_x(pr, st->rtl, ui->mx) - x0) / (x1 - x0), 0, 1);
        k = (int)lroundf(mt * (stops - 1));
    }
    if (r.keyboard_focused) k = (int)lroundf(cui__slider_keys(ui, k * unit, unit, st->rtl) * (stops - 1));
    *value = min + CUI_CLAMP(k, 0, stops - 1);
    float t = (*value - min) * unit;
    if (!st->init) { st->pos.x = t; st->init = true; }
    cui__spring(ui, &st->pos, t, CUI_SPRING_SNAPPY);
    cui__tween(ui, &st->hover, (r.hovered || r.active) ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->active, r.active ? 1.0f : 0.0f, CUI__T_STATE);
    cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    if (r.hovered || r.active) cui__set_cursor(ui, disabled ? CUI_CURSOR_NOT_ALLOWED : CUI_CURSOR_HAND);

    const char *text = o->value;
    if (!text) text = o->labels ? o->labels[*value - min] : cui__frame_printf(ui, o->format ? o->format : "%d", *value);

    /* build */
    int w = cui__well_begin(ui, id, label, text, o->plain);
    ui->nodes[w].opacity = disabled ? CUI__DISABLED : 1.0f;
    int tr = cui__leaf(ui, track_id, CUI_FILL, CUI_PX(24));
    cui__mark_interactive(ui, tr, CUI_IF_FOCUSABLE);
    cui__step_slider_paint *p = CUI_PAINT_DATA(ui, cui__step_slider_paint);
    p->stops = stops; p->pos = st->pos.x; p->focus = st->focus; p->active = st->active;
    p->rail = cui_mix(CUI_C(ui, CONTROL_TRACK), CUI_C(ui, CONTROL_TRACK_HOVER), st->hover);
    p->fill = CUI_C(ui, CONTROL_ON);
    p->dot = CUI_C(ui, BORDER_STRONG);
    p->hole = cui_alpha(CUI_C(ui, BACKGROUND), 0.55f);
    p->rtl_out = &st->rtl;
    ui->nodes[tr].paint = cui__paint_step_slider;
    ui->nodes[tr].paint_data = p;
    int flags = *value != old ? CUI_WELL_CHANGED : 0;
    if (o->tool && cui__well_tool(ui, id ^ CUI__WELL_TOOL_SALT, o->tool, o->tool_tooltip, o->tool_on, disabled)) flags |= CUI_WELL_ACTION;
    cui__well_end(ui);
    ui->last_id = track_id;
    return flags;
}

/* ------------------------------------------------------------------------ */
/* Palette strip                                                            */
/* ------------------------------------------------------------------------ */

#define CUI__PAL_MAX 16
#define CUI__PAL_WIDEN 0.6f   /* the swatch under the pointer takes this much more room */

typedef struct { float grow[CUI__PAL_MAX]; bool rtl; } cui__pal_state;
typedef struct { cui_color c, ink; cui_corners rad; float hex_a, focus; bool locked; char hex[8]; } cui__pal_paint;

static void cui__paint_swatch(cui_ctx *ui, cui_node *n, void *data) {
    cui__pal_paint *p = data;
    cui_rect r = n->rect;
    cui_draw_rect(ui, r, p->rad, p->c);
    float fs = cui__fs(ui, 10);
    if (p->hex_a > 0.01f && cui__text_width(ui, CUI_FONT_MONO, fs, p->hex, -1, true) + 8 < r.w)
        cui__draw_text_at(ui, CUI_FONT_MONO, fs, r.x + r.w * 0.5f, r.y + r.h * 0.5f, CUI__AT_CENTER, cui_alpha(p->ink, 0.85f * p->hex_a), p->hex, true);
    if (p->locked && r.w >= 18) {
        cui_rect lk = { n->rtl ? r.x + 3 : r.x + r.w - 15, r.y + 3, 12, 12 };
        cui_draw_icon(ui, CUI_ICON_LOCK, lk, 11, cui_alpha(p->ink, 0.9f));
    }
    cui__draw_focus_ring(ui, r, p->rad, p->focus);
}

/* one step of xorshift32 on the caller's seed, as a number in 0..1 */
static float cui__pal_rand(uint32_t *seed) {
    uint32_t x = *seed ? *seed : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *seed = x;
    return (float)(x >> 8) * (1.0f / 16777216.0f);
}

/* a color near the shuffle's base hue (now and then its opposite), at any lightness, never
 * louder than a palette needs */
static cui_color cui__pal_color(uint32_t *seed, float base_hue) {
    float hue = base_hue + (cui__pal_rand(seed) - 0.5f) * 120.0f;
    if (cui__pal_rand(seed) < 0.25f) hue += 180.0f;
    float L = 0.28f + cui__pal_rand(seed) * 0.64f;
    float C = (0.02f + cui__pal_rand(seed) * 0.13f) * (1.0f - fabsf(L - 0.6f));
    return cui_oklch(L, C, fmodf(hue + 360.0f, 360.0f), 1);
}

int cui_palette_strip_ex(cui_ctx *ui, const char *label, cui_color *colors, bool *locked, int count, const cui_palette_strip_opts *o) {
    cui_palette_strip_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!colors || count <= 0) return 0;
    if (count > CUI__PAL_MAX) count = CUI__PAL_MAX;
    cui_id id = cui_make_id(ui, label ? label : "palette");
    cui_id row_id = id ^ 0x9A11u;
    cui__pal_state *st = cui_state(ui, id, sizeof *st);

    /* input: a click or Space locks, Left and Right walk the swatches */
    int flags = 0, to = -1;
    cui_id ids[CUI__PAL_MAX], move_from = 0;
    cui_interact ir[CUI__PAL_MAX];
    for (int k = 0; k < count; k++) {
        ids[k] = cui__hash(&k, sizeof k, row_id);
        ir[k] = cui__interact(ui, ids[k], CUI_IF_FOCUSABLE);
        if (ir[k].clicked && locked) { locked[k] = !locked[k]; flags |= CUI_WELL_CHANGED; }
        if (ir[k].focus_visible) {
            int dk = cui__arrow_step(ui, st->rtl);
            if (dk && k + dk >= 0 && k + dk < count) { to = k + dk; move_from = ids[k]; }
        }
        cui__tween(ui, &st->grow[k], (ir[k].hovered || ir[k].focus_visible) ? 1.0f : 0.0f, CUI__T_STATE);
    }
    if (to >= 0 && ui->focus == move_from) { ui->focus = ids[to]; ui->animating = true; }

    /* build */
    cui__well_begin(ui, id, label, o->value, o->plain);
    int row = cui__node_new(ui, row_id);
    cui_node *rn = &ui->nodes[row];
    float h = o->height > 0 ? o->height : 32;
    rn->dir = CUI_ROW; rn->gap = 2; rn->w = CUI_FILL; rn->h = CUI_PX(h);
    rn->paint = cui__paint_note_rtl;
    rn->paint_data = &st->rtl;
    cui__open(ui, row);
    float rad = ui->theme.radius_control;
    bool dark = ui->mode == CUI_THEME_DARK;
    cui_color ink_dark = dark ? CUI_C(ui, BACKGROUND) : CUI_C(ui, FOREGROUND), ink_light = dark ? CUI_C(ui, FOREGROUND) : CUI_C(ui, BACKGROUND);
    for (int k = 0; k < count; k++) {
        int s = cui__node_new(ui, ids[k]);
        cui_node *sn = &ui->nodes[s];
        sn->w = CUI_GROW(1.0f + CUI__PAL_WIDEN * st->grow[k]);
        sn->h = CUI_PX(h);
        cui__mark_interactive(ui, s, CUI_IF_FOCUSABLE);
        if (ir[k].hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
        cui__pal_paint *p = CUI_PAINT_DATA(ui, cui__pal_paint);
        cui_color c = colors[k];
        p->c = c;
        float y = (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255.0f;
        p->ink = cui__legible(y > 0.6f ? ink_dark : ink_light, c, 4.5f);   /* dark ink on light swatches */
        /* the strip's outer corners are round: the first swatch's start and the last one's end */
        bool start = k == 0, end = k == count - 1, left = st->rtl ? end : start, right = st->rtl ? start : end;
        p->rad.tl = p->rad.bl = left ? rad : 0;
        p->rad.tr = p->rad.br = right ? rad : 0;
        p->hex_a = st->grow[k];
        p->focus = ir[k].focus_visible ? 1.0f : 0.0f;
        p->locked = locked && locked[k];
        snprintf(p->hex, sizeof p->hex, "%02X%02X%02X", c.r, c.g, c.b);
        sn->paint = cui__paint_swatch;
        sn->paint_data = p;
    }
    cui__close(ui);
    if (!o->no_shuffle && cui__well_tool(ui, id ^ CUI__WELL_TOOL_SALT, CUI_ICON_SHUFFLE, "Shuffle", false, false)) {
        flags |= CUI_WELL_ACTION;
        if (o->seed) {
            float base = cui__pal_rand(o->seed) * 360.0f;
            for (int k = 0; k < count; k++)
                if (!(locked && locked[k])) colors[k] = cui__pal_color(o->seed, base);
            flags |= CUI_WELL_CHANGED;
        }
    }
    cui__well_end(ui);
    ui->last_id = id;
    return flags;
}

/* ------------------------------------------------------------------------ */
/* Aspect ratio picker                                                      */
/* ------------------------------------------------------------------------ */

#define CUI__ASPECT_MAX 32
#define CUI__ASPECT_BOX 22.0f   /* the longer side of every shape */

static const cui_aspect cui__aspect_photo[] = { { 1, 1, NULL }, { 4, 5, NULL }, { 3, 2, NULL }, { 16, 9, NULL }, { 9, 16, NULL }, { 21, 9, NULL } };

typedef struct { float hover[CUI__ASPECT_MAX + 1]; bool rtl; } cui__aspect_state;
typedef struct { cui_color c, fill; bool dashed; } cui__aspect_paint;

static void cui__paint_aspect_shape(cui_ctx *ui, cui_node *n, void *data) {
    cui__aspect_paint *p = data;
    cui_rect r = n->rect;
    float rad = CUI_MIN(4.0f, CUI_MIN(r.w, r.h) * 0.5f);
    if (p->dashed) cui__draw_dashed_rrect(ui, r, rad, p->c, 1.5f, 3, 2.5f);
    else cui_draw_rect_border(ui, r, CUI_CORNERS(rad), p->fill, p->c, 1.75f);
}

/* the size at the right of the label: the short side is base pixels */
static const char *cui__aspect_size(cui_ctx *ui, cui_aspect a, int base) {
    if (a.w <= 0 || a.h <= 0) return NULL;
    long w = a.w >= a.h ? lroundf((float)base * a.w / a.h) : base, h = a.w >= a.h ? base : lroundf((float)base * a.h / a.w);
    return cui__frame_printf(ui, "%ld \xc3\x97 %ld", w, h);
}

bool cui_aspect_picker_ex(cui_ctx *ui, const char *label, int *index, const cui_aspect_picker_opts *o) {
    cui_aspect_picker_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    const cui_aspect *ratios = o->ratios ? o->ratios : cui__aspect_photo;
    int n = o->ratios ? o->count : CUI_COUNTOF(cui__aspect_photo);
    if (n < 0) n = 0;
    if (n > CUI__ASPECT_MAX) n = CUI__ASPECT_MAX;
    int total = n + (o->custom ? 1 : 0);
    if (total <= 0) return false;
    cui_id id = cui_make_id(ui, label ? label : "aspect");
    cui_id row_id = id ^ 0xA5EC7u;
    cui__aspect_state *st = cui_state(ui, id, sizeof *st);

    /* input: a click picks; Left and Right move the focus and the choice with it */
    int old = *index, to = -1;
    cui_id ids[CUI__ASPECT_MAX + 1], move_from = 0;
    cui_interact ir[CUI__ASPECT_MAX + 1];
    for (int k = 0; k < total; k++) {
        ids[k] = cui__hash(&k, sizeof k, row_id);
        ir[k] = cui__interact(ui, ids[k], CUI_IF_FOCUSABLE);
        if (ir[k].clicked) *index = k;
        if (ir[k].focus_visible) {
            int dk = cui__arrow_step(ui, st->rtl);
            if (dk && k + dk >= 0 && k + dk < total) { to = k + dk; move_from = ids[k]; }
        }
        cui__tween(ui, &st->hover[k], ir[k].hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    }
    if (to >= 0 && ui->focus == move_from) { ui->focus = ids[to]; *index = to; ui->animating = true; }
    *index = CUI_CLAMP(*index, 0, total - 1);

    const char *custom = o->custom_label ? o->custom_label : "Custom";
    const char *value = o->value;
    if (!value) value = *index == n ? custom : o->base < 0 ? NULL : cui__aspect_size(ui, ratios[*index], o->base > 0 ? o->base : 1080);

    /* build */
    cui__well_begin(ui, id, label, value, o->plain);
    int row = cui__node_new(ui, row_id);
    cui_node *rn = &ui->nodes[row];
    rn->dir = CUI_ROW; rn->gap = 4; rn->w = CUI_FILL; rn->align = CUI_ALIGN_CENTER;
    rn->paint = cui__paint_note_rtl;
    rn->paint_data = &st->rtl;
    cui__open(ui, row);
    for (int k = 0; k < total; k++) {
        bool sel = k == *index, is_custom = k == n;
        if (is_custom && n > 0) {
            int sep = cui__leaf(ui, 0, CUI_PX(1), CUI_PX(28));
            ui->nodes[sep].bg = CUI_C(ui, BORDER);
        }
        int it = cui__node_new(ui, ids[k]);
        cui_node *in = &ui->nodes[it];
        in->align = CUI_ALIGN_CENTER; in->gap = 6;
        in->pad[0] = 8; in->pad[2] = 6; in->pad[1] = in->pad[3] = 2;
        in->w = CUI_GROW(1);
        in->radius = CUI_CORNERS(ui->theme.radius_control);
        in->bg = sel ? CUI_C(ui, ACTIVE) : cui_alpha(CUI_C(ui, HOVER), st->hover[k]);
        cui__mark_interactive(ui, it, CUI_IF_FOCUSABLE);
        if (ir[k].hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
        cui_color fg = sel ? CUI_C(ui, FOREGROUND) : cui_mix(CUI_C(ui, TEXT_MUTED), CUI_C(ui, TEXT_SECONDARY), st->hover[k]);
        cui__open(ui, it);
        /* every shape sits on the same baseline in a box of the same height */
        int box = cui__node_new(ui, 0);
        ui->nodes[box].h = CUI_PX(CUI__ASPECT_BOX);
        ui->nodes[box].justify = CUI_JUSTIFY_END;
        ui->nodes[box].align = CUI_ALIGN_CENTER;
        cui__open(ui, box);
        float sw = 20, sh = 14;
        if (!is_custom) {
            cui_aspect a = ratios[k];
            float big = (float)CUI_MAX(1, CUI_MAX(a.w, a.h));
            sw = CUI_MAX(4.0f, roundf(CUI_MAX(a.w, 1) * CUI__ASPECT_BOX / big));
            sh = CUI_MAX(4.0f, roundf(CUI_MAX(a.h, 1) * CUI__ASPECT_BOX / big));
        }
        int shape = cui__leaf(ui, 0, CUI_PX(sw), CUI_PX(sh));
        cui__aspect_paint *p = CUI_PAINT_DATA(ui, cui__aspect_paint);
        p->c = fg;
        p->fill = sel && !is_custom ? cui_alpha(CUI_C(ui, FOREGROUND), 0.12f) : cui__rgba(0, 0, 0, 0);
        p->dashed = is_custom;
        ui->nodes[shape].paint = cui__paint_aspect_shape;
        ui->nodes[shape].paint_data = p;
        cui__close(ui);
        const char *name = is_custom ? custom : ratios[k].label ? ratios[k].label : cui__frame_printf(ui, "%d:%d", ratios[k].w, ratios[k].h);
        int t = cui__text_node(ui, name, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_xs), fg, false);
        ui->nodes[t].flags |= CUI_NF_TABULAR;
        ui->nodes[t].line_height = 1.2f;
        cui__close(ui);
        cui__focus_ring(ui, it, ir[k].focus_visible ? 1.0f : 0.0f);
    }
    cui__close(ui);
    cui__well_end(ui);
    ui->last_id = id;
    return *index != old;
}

/* ------------------------------------------------------------------------ */
/* Option field with action                                                 */
/* ------------------------------------------------------------------------ */

int cui_option_action_ex(cui_ctx *ui, const char *label, const char *const *items, int count, int *index, const char *icon, const cui_option_action_opts *o) {
    cui_option_action_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    const char *name = label ? label : "options";
    cui_id id = cui_make_id(ui, name);
    int old = *index, flags = 0;
    cui__well_begin(ui, id, label, o->value, o->plain);
    cui_segmented_opts so;
    memset(&so, 0, sizeof so);
    so.size = CUI_SIZE_SM;
    so.full_width = true;
    cui_push_id(ui, name);
    cui_segmented_ex(ui, "choice", items, count, index, &so);
    cui_pop_id(ui);
    if (*index != old) flags |= CUI_WELL_CHANGED;
    if (icon && cui__well_tool(ui, id ^ CUI__WELL_TOOL_SALT, icon, o->tooltip, o->action_on, o->action_disabled)) flags |= CUI_WELL_ACTION;
    cui__well_end(ui);
    ui->last_id = id;
    return flags;
}

/* ------------------------------------------------------------------------ */
/* Seed field                                                               */
/* ------------------------------------------------------------------------ */

#define CUI__SEED_ROLL 0.35f      /* seconds the first digit rolls */
#define CUI__SEED_STAGGER 0.05f   /* each later digit rolls this much longer */
#define CUI__SEED_BUTTON 30.0f

typedef struct { char from[12]; double roll_at; uint32_t rolls; bool rolling; float hover, focus; cui_spring press; } cui__seed_state;
typedef struct { char from[12], to[12]; float elapsed, fs; bool rolling; cui_color c; } cui__seed_paint;
typedef struct { cui_color c; float focus; } cui__dice_paint;

/* each digit on its own wheel: while rolling, it turns past at least one full round from
 * the old digit to the new one, the later digits a little longer */
static void cui__paint_seed_digits(cui_ctx *ui, cui_node *n, void *data) {
    cui__seed_paint *p = data;
    cui_rect r = n->rect;
    float track = p->fs * 0.06f;
    float adv = cui__text_width(ui, CUI_FONT_MONO, p->fs, "0", 1, true) + track;
    int len = (int)strlen(p->to), flen = (int)strlen(p->from);
    float total = adv * len - track;
    float x0 = n->rtl ? r.x + r.w - total : r.x, cy = r.y + r.h * 0.5f, lh = r.h;
    cui_push_clip(ui, r);
    for (int k = 0; k < len; k++) {
        float cx = x0 + k * adv + (adv - track) * 0.5f;
        char ch[2] = { p->to[k], 0 };
        int d1 = p->to[k] - '0';
        if (!p->rolling || d1 < 0 || d1 > 9) {
            cui__draw_text_at(ui, CUI_FONT_MONO, p->fs, cx, cy, CUI__AT_CENTER, p->c, ch, true);
            continue;
        }
        int d0 = k < flen ? p->from[k] - '0' : 0;
        if (d0 < 0 || d0 > 9) d0 = 0;
        float q = cui__clampf(p->elapsed / (CUI__SEED_ROLL + CUI__SEED_STAGGER * k), 0, 1);
        q = 1 - (1 - q) * (1 - q) * (1 - q);
        float off = q * (float)((d1 - d0 + 10) % 10 + 10);
        int whole = (int)floorf(off);
        float frac = off - whole;
        char a[2] = { (char)('0' + (d0 + whole) % 10), 0 }, b[2] = { (char)('0' + (d0 + whole + 1) % 10), 0 };
        cui__draw_text_at(ui, CUI_FONT_MONO, p->fs, cx, cy - frac * lh, CUI__AT_CENTER, p->c, a, true);
        if (frac > 0.001f) cui__draw_text_at(ui, CUI_FONT_MONO, p->fs, cx, cy + (1 - frac) * lh, CUI__AT_CENTER, p->c, b, true);
    }
    cui_pop_clip(ui);
}

/* a die showing five: a rounded square and five pips */
static void cui__paint_dice(cui_ctx *ui, cui_node *n, void *data) {
    cui__dice_paint *p = data;
    cui_rect r = n->rect;
    float s = 12, cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f;
    cui_rect sq = { cx - s * 0.5f, cy - s * 0.5f, s, s };
    cui_draw_rect_border(ui, sq, CUI_CORNERS(3), cui__rgba(0, 0, 0, 0), p->c, 1.5f);
    float o = s * 0.25f;
    cui_vec2 pips[5] = { { cx - o, cy - o }, { cx + o, cy - o }, { cx, cy }, { cx - o, cy + o }, { cx + o, cy + o } };
    for (int k = 0; k < 5; k++) cui_draw_circle(ui, pips[k], 1.1f, p->c);
    cui__draw_focus_ring(ui, r, n->radius, p->focus);
}

int cui_seed_field_ex(cui_ctx *ui, const char *label, uint32_t *seed, bool *locked, const cui_seed_field_opts *o) {
    cui_seed_field_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, label ? label : "seed");
    cui_id dice_id = id ^ 0xD1CEu;
    cui__seed_state *st = cui_state(ui, id, sizeof *st);
    bool fixed = locked && *locked;
    int flags = 0;

    /* input: the dice rolls a new seed unless it is locked */
    cui_push_disabled(ui, fixed);
    cui_interact r = cui__interact(ui, dice_id, CUI_IF_FOCUSABLE);
    cui_pop_disabled(ui);
    if (r.clicked && !fixed) {
        snprintf(st->from, sizeof st->from, "%lu", (unsigned long)*seed);
        uint32_t salt = 0x5EEDu + st->rolls++;
        *seed = 1000000000u + cui__hash(seed, sizeof *seed, salt) % 3294967295u;   /* always ten digits */
        st->roll_at = ui->time;
        st->rolling = ui->theme.motion > 0;
        flags |= CUI_WELL_CHANGED | CUI_WELL_ACTION;
    }
    char to[12];
    snprintf(to, sizeof to, "%lu", (unsigned long)*seed);
    float elapsed = (float)(ui->time - st->roll_at);
    if (st->rolling) {
        if (elapsed >= CUI__SEED_ROLL + CUI__SEED_STAGGER * (float)strlen(to)) st->rolling = false;
        else ui->animating = true;
    }
    cui__tween(ui, &st->hover, r.hovered && !fixed ? 1.0f : 0.0f, CUI__T_HOVER);
    cui__tween(ui, &st->focus, r.focus_visible ? 1.0f : 0.0f, CUI__T_FOCUS);
    if (st->press.x == 0 && st->press.v == 0) st->press.x = 1;
    cui__spring(ui, &st->press, r.pressed && !fixed ? CUI__PRESS : 1.0f, CUI_SPRING_SNAPPY);
    if (r.hovered) cui__set_cursor(ui, fixed ? CUI_CURSOR_NOT_ALLOWED : CUI_CURSOR_HAND);

    /* build */
    cui__well_begin(ui, id, label, o->value, o->plain);
    int field = cui__node_new(ui, 0);
    cui_node *fn = &ui->nodes[field];
    fn->dir = CUI_ROW; fn->align = CUI_ALIGN_CENTER; fn->gap = 2;
    fn->w = CUI_FILL; fn->h = CUI_PX(34);
    fn->pad[3] = 12; fn->pad[1] = 2;
    fn->radius = CUI_CORNERS(ui->theme.radius_control);
    fn->bg = cui__well_inset(ui);
    cui__open(ui, field);
    int dg = cui__leaf(ui, 0, CUI_FILL, CUI_PX(20));
    cui__seed_paint *sp = CUI_PAINT_DATA(ui, cui__seed_paint);
    memcpy(sp->from, st->from, sizeof sp->from);
    memcpy(sp->to, to, sizeof sp->to);
    sp->elapsed = elapsed; sp->rolling = st->rolling;
    sp->fs = cui__fs(ui, 14);
    sp->c = CUI_C(ui, FOREGROUND);
    ui->nodes[dg].paint = cui__paint_seed_digits;
    ui->nodes[dg].paint_data = sp;
    const char *mode = fixed ? (o->fixed_text ? o->fixed_text : "fixed") : (o->random_text ? o->random_text : "random");
    cui__leaf(ui, 0, CUI_PX(4), CUI_PX(1));
    int mt = cui__text_node(ui, mode, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_xs), CUI_C(ui, TEXT_MUTED), false);
    ui->nodes[mt].line_height = 1.2f;
    cui__leaf(ui, 0, CUI_PX(4), CUI_PX(1));
    int dice = cui__leaf(ui, dice_id, CUI_PX(CUI__SEED_BUTTON), CUI_PX(CUI__SEED_BUTTON));
    cui_node *dn = &ui->nodes[dice];
    dn->radius = CUI_CORNERS(CUI__R_ITEM(ui));
    dn->bg = cui_alpha(CUI_C(ui, HOVER), st->hover);
    dn->scale = st->press.x;
    dn->opacity = fixed ? CUI__DISABLED : 1.0f;
    cui__mark_interactive(ui, dice, CUI_IF_FOCUSABLE);
    cui__dice_paint *dp = CUI_PAINT_DATA(ui, cui__dice_paint);
    dp->c = cui_mix(CUI_C(ui, TEXT_SECONDARY), CUI_C(ui, FOREGROUND), st->hover);
    dp->focus = st->focus;
    dn->paint = cui__paint_dice;
    dn->paint_data = dp;
    ui->last_id = dice_id;
    cui_tooltip(ui, "New seed");
    if (locked && cui__well_tool(ui, id ^ CUI__WELL_TOOL_SALT, fixed ? CUI_ICON_LOCK : CUI_ICON_UNLOCK,
                                 fixed ? "Use a random seed" : "Keep this seed", fixed, false)) {
        *locked = !*locked;
        flags |= CUI_WELL_CHANGED;
    }
    cui__close(ui);
    cui__well_end(ui);
    ui->last_id = id;
    return flags;
}
