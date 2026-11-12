/* More charts: heatmaps, treemap, radar, funnel, sankey, waffle, split bar,
 * bar list, activity rings and scatter. Each one builds a single node whose
 * paint callback draws the marks; hover is resolved at build time from the
 * previous frame's rect using the same geometry the paint uses. */
#include "cui_internal.h"


typedef struct { cui_spring intro; float hover_t; int hover, shown; int emph; } cui__viz_state;   /* emph: the radar's hovered series */

typedef struct {
    float alpha;
    float x, y;              /* anchor (pointer) */
    cui_rect bounds;         /* keep inside */
    const char *title;
    int n;
    const char *label[8], *value[8];
    cui_color sw[8];
} cui__tip;

static cui_color cui__viz_color(cui_ctx *ui, cui_color c, int k) {
    if (cui__color_set(c)) return cui_color_of(ui, c);
    return ui->theme.colors[CUI_TOKEN_SERIES_1 + (k % 6)];
}

static cui_color cui__viz_base(cui_ctx *ui, cui_color c) {
    if (cui__color_set(c)) return cui_color_of(ui, c);
    return ui->accent == CUI_ACCENT_NEUTRAL ? CUI_C(ui, SERIES_1) : CUI_C(ui, ACCENT);
}

static cui_color cui__viz_empty(cui_ctx *ui) {
    return ui->mode == CUI_THEME_DARK ? cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, FOREGROUND), 0.07f) : cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, FOREGROUND), 0.055f);
}

static bool cui__viz_light_text(cui_color c) {
    float l = (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.0f;
    return l < 0.52f;
}

static void cui__tip_draw(cui_ctx *ui, const cui__tip *t) {
    if (!t || t->alpha <= 0.01f || !t->title) return;
    float fs = ui->theme.text_sm, pad = 10, lh = 18;
    float fh = cui__font_line_height(ui, CUI_FONT_MEDIUM, fs);
    float w = cui__text_width(ui, CUI_FONT_MEDIUM, fs, t->title, -1, false);
    for (int k = 0; k < t->n; k++) {
        float lw = t->label[k] ? cui__text_width(ui, CUI_FONT_BODY, fs, t->label[k], -1, false) : 0;
        float vw = t->value[k] ? cui__text_width(ui, CUI_FONT_MEDIUM, fs, t->value[k], -1, true) : 0;
        w = CUI_MAX(w, (t->sw[k].a ? 14 : 0) + lw + 18 + vw);
    }
    float W = w + pad * 2, H = pad * 2 + lh * (float)(1 + t->n) + (t->n ? 2 : 0);
    float x = t->x + 14, y = t->y - H - 10;
    if (x + W > t->bounds.x + t->bounds.w + 20) x = t->x - 14 - W;
    if (y < t->bounds.y - 30) y = t->y + 18;
    cui_rect r = { x, y, W, H };
    cui__push_xform(ui, 1, 0, 0, t->alpha, 0, 0);
    cui__draw_elevation(ui, r, CUI_CORNERS(12), CUI_ELEVATION_RAISED, 1);
    cui_draw_rect_border(ui, r, CUI_CORNERS(12), CUI_C(ui, SURFACE_RAISED), ui->mode == CUI_THEME_DARK ? CUI_C(ui, BORDER) : cui__rgba(0, 0, 0, 18), 1);
    float ty = y + pad;
    cui__draw_text_line(ui, CUI_FONT_MEDIUM, fs, x + pad, ty + (lh - fh) * 0.5f, CUI_C(ui, FOREGROUND), t->title, -1, false);
    ty += lh + 2;
    for (int k = 0; k < t->n; k++) {
        float lx = x + pad;
        if (t->sw[k].a) { cui_draw_rect(ui, (cui_rect){ lx, ty + lh * 0.5f - 4, 8, 8 }, CUI_CORNERS(2.5f), t->sw[k]); lx += 14; }
        if (t->label[k]) cui__draw_text_line(ui, CUI_FONT_BODY, fs, lx, ty + (lh - fh) * 0.5f, CUI_C(ui, TEXT_SECONDARY), t->label[k], -1, false);
        if (t->value[k]) {
            float vw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, t->value[k], -1, true);
            cui__draw_text_line(ui, CUI_FONT_MEDIUM, fs, x + W - pad - vw, ty + (lh - fh) * 0.5f, CUI_C(ui, FOREGROUND), t->value[k], -1, true);
        }
        ty += lh;
    }
    cui__pop_xform(ui);
}

static void cui__tip_row(cui__tip *t, const char *label, const char *value, cui_color sw) {
    if (t->n >= 8) return;
    t->label[t->n] = label; t->value[t->n] = value; t->sw[t->n] = sw; t->n++;
}

/* Starts a chart: state, intro, hover fade. hover is the mark under the pointer this frame. */
static cui__viz_state *cui__viz_state_get(cui_ctx *ui, cui_id id) {
    cui__viz_state *st = cui_state(ui, id, sizeof *st);
    cui__spring(ui, &st->intro, 1.0f, 0.7f, 0.0f);
    return st;
}

static void cui__viz_hover(cui_ctx *ui, cui__viz_state *st, int hover) {
    st->hover = hover;
    if (hover >= 0) st->shown = hover;
    cui__tween(ui, &st->hover_t, hover >= 0 ? 1.0f : 0.0f, CUI__T_HOVER);
}

static int cui__viz_node(cui_ctx *ui, cui_id id, cui_size w, cui_size h, cui__paint_fn paint, void *data, bool hovered) {
    int n = cui__node_new(ui, id);
    cui_node *nd = &ui->nodes[n];
    nd->w = w; nd->h = h;
    nd->paint = paint; nd->paint_data = data;
    nd->flags |= CUI_NF_PAINT_AFTER;
    if (hovered) nd->flags |= CUI_NF_ON_TOP;   /* the tooltip over the neighbours */
    cui__mark_interactive(ui, n, 0);
    ui->last_id = id;
    return n;
}

/* Legend: swatch and name per entry, wrapped to width. Returns its height. */
static float cui__viz_legend(cui_ctx *ui, float x, float y, float w, const char *const *names, const cui_color *colors, int n, int hl, bool draw) {
    float fs = cui__fs(ui, ui->theme.text_xs), lh = 18, cx = x, cy = y;
    float fh = cui__font_line_height(ui, CUI_FONT_BODY, fs);
    for (int k = 0; k < n; k++) {
        const char *nm = names[k] ? names[k] : "";
        float iw = 14 + cui__text_width(ui, CUI_FONT_BODY, fs, nm, -1, false);
        if (cx > x && cx + iw > x + w) { cx = x; cy += lh + 4; }
        if (draw) {
            float a = (hl >= 0 && hl != k) ? 0.45f : 1.0f;
            cui_draw_rect(ui, (cui_rect){ cx, cy + lh * 0.5f - 4, 8, 8 }, CUI_CORNERS(2.5f), cui_alpha(colors[k], a));
            cui__draw_text_line(ui, CUI_FONT_BODY, fs, cx + 13, cy + (lh - fh) * 0.5f, cui_alpha(CUI_C(ui, TEXT_SECONDARY), a), nm, -1, false);
        }
        cx += iw + 16;
    }
    return n ? cy - y + lh : 0;
}

static float *cui__copy_floats(cui_ctx *ui, const float *v, int n) {
    float *o = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, n));
    if (v && n > 0) memcpy(o, v, sizeof(float) * (size_t)n);
    else if (n > 0) memset(o, 0, sizeof(float) * (size_t)n);
    return o;
}

static cui_viz_item *cui__copy_items(cui_ctx *ui, const cui_viz_item *v, int n) {
    cui_viz_item *o = cui__frame_alloc(ui, sizeof(cui_viz_item) * (size_t)CUI_MAX(1, n));
    if (v && n > 0) memcpy(o, v, sizeof(cui_viz_item) * (size_t)n);
    for (int k = 0; k < n; k++) if (!(o[k].value >= 0)) o[k].value = 0;   /* negatives and NaN count as nothing */
    return o;
}

static const char *cui__pct(cui_ctx *ui, float part, float total) {
    if (total <= 0) return "0%";
    float p = part / total * 100;
    return p < 10 && p != floorf(p) ? cui__frame_printf(ui, "%.1f%%", (double)p) : cui__frame_printf(ui, "%.0f%%", (double)p);
}

/* ------------------------------------------------------------------------ */
/* Heatmap                                                                  */
/* ------------------------------------------------------------------------ */

/* Both heatmaps count in five steps: the empty surface, then four even mixes (in OKLab)
 * toward the base, the theme accent unless the caller picks a color. Steps read faster
 * than a continuous ramp, and zero looks empty rather than faintly on. Under the neutral
 * accent the steps are a gray ramp up to its gray, a step softer than the text color. */
#define CUI__HEAT_STEPS 5
static cui_color cui__heat_base(cui_ctx *ui, cui_color c) {
    return cui__color_set(c) ? cui_color_of(ui, c) : CUI_C(ui, ACCENT);
}

static cui_color cui__heat_step(cui_ctx *ui, cui_color base, int level) {
    static const float t[CUI__HEAT_STEPS] = { 0, 0.27f, 0.52f, 0.76f, 1.0f };
    return cui_mix(cui__viz_empty(ui), base, t[CUI_CLAMP(level, 0, CUI__HEAT_STEPS - 1)]);
}

/* t is the value's place in its range, 0 to 1; only the bottom few percent count as empty */
static int cui__heat_level(float t) {
    if (!(t >= 0.05f)) return 0;
    return CUI_CLAMP(1 + (int)floorf(t * 4 - 0.0001f), 1, 4);
}

static float cui__heat_radius(float cell) { return cui__clampf(cell * 0.22f, 2, 6); }

/* The step key, right-aligned to `right`: "Less", a square per step, "More". Returns its
 * left edge, or `right` when it was not drawn because it would cross `min_x`. */
static float cui__heat_key(cui_ctx *ui, cui_color base, float right, float y, float sz, float min_x, bool words) {
    float fs = cui__fs(ui, ui->theme.text_xs), pitch = sz + 4, rad = cui__heat_radius(sz) * 0.8f;
    float more_w = cui__text_width(ui, CUI_FONT_BODY, fs, "More", -1, false), less_w = cui__text_width(ui, CUI_FONT_BODY, fs, "Less", -1, false);
    float w = CUI__HEAT_STEPS * pitch - 4 + (words ? more_w + less_w + 16 : 0);
    if (right - w < min_x) return right;
    float x = right;
    if (words) { x -= more_w; cui__draw_text_at(ui, CUI_FONT_BODY, fs, x, y + sz * 0.5f, CUI__AT_LEFT, CUI_C(ui, TEXT_MUTED), "More", false); x -= 8; }
    for (int lv = CUI__HEAT_STEPS - 1; lv >= 0; lv--) { x -= sz; cui_draw_rect(ui, (cui_rect){ x, y, sz, sz }, CUI_CORNERS(rad), cui__heat_step(ui, base, lv)); x -= 4; }
    x += 4;
    if (words) { x -= 8; cui__draw_text_at(ui, CUI_FONT_BODY, fs, x, y + sz * 0.5f, CUI__AT_RIGHT, CUI_C(ui, TEXT_MUTED), "Less", false); x -= less_w; }
    return x;
}

/* the hovered cell's ring, a little outside it */
static void cui__heat_ring(cui_ctx *ui, cui_rect c, float rad, float alpha) {
    float o = c.w >= 18 ? 2.5f : 1.5f;
    cui_draw_rect_border(ui, (cui_rect){ c.x - o, c.y - o, c.w + 2 * o, c.h + 2 * o }, CUI_CORNERS(rad + o), cui__rgba(0, 0, 0, 0),
                         cui_alpha(CUI_C(ui, FOREGROUND), alpha), c.w >= 18 ? 1.75f : 1.5f);
}

typedef struct { float lw, cell, gap, gx, gy, grid_h; } cui__hm_geo;

static cui__hm_geo cui__hm_layout(cui_ctx *ui, cui_rect r, int rows, int cols, const cui_heatmap_opts *o) {
    cui__hm_geo g;
    float fs = cui__fs(ui, ui->theme.text_xs);
    g.lw = 0;
    if (o->row_labels)
        for (int i = 0; i < rows; i++) g.lw = CUI_MAX(g.lw, cui__text_width(ui, CUI_FONT_BODY, fs, o->row_labels[i] ? o->row_labels[i] : "", -1, false) + 12);
    float avail = (r.w > 0 ? r.w : 480) - g.lw;
    int nc = CUI_MAX(1, cols);
    /* modest cells with real gaps between them, so they read as a field and not as buttons */
    g.gap = 5;
    g.cell = o->cell > 0 ? o->cell : cui__clampf((avail - g.gap * (float)(cols - 1)) / (float)nc, 12, 28);
    if (g.cell < 20) {
        g.gap = 3;
        if (o->cell <= 0) g.cell = cui__clampf((avail - g.gap * (float)(cols - 1)) / (float)nc, 12, 28);
    }
    if (o->cell <= 0 && r.w > 0 && g.lw + cols * (g.cell + g.gap) - g.gap > r.w + 0.5f) {
        /* a narrow container: tighter gaps and smaller cells rather than spilling out */
        g.gap = 2;
        g.cell = cui__clampf((avail - g.gap * (float)(cols - 1)) / (float)nc, 6, 12);
    }
    g.gx = r.x + g.lw; g.gy = r.y;
    g.grid_h = rows * (g.cell + g.gap) - g.gap;
    return g;
}

typedef struct {
    float *v; int rows, cols; float lo, hi;
    cui_heatmap_opts o; cui_color base;
    float intro; int hover; cui__tip tip;
} cui__hm_paint;

static void cui__paint_heatmap(cui_ctx *ui, cui_node *n, void *data) {
    cui__hm_paint *p = data;
    cui__hm_geo g = cui__hm_layout(ui, n->rect, p->rows, p->cols, &p->o);
    float fs = cui__fs(ui, ui->theme.text_xs);
    float rad = cui__heat_radius(g.cell);
    bool on = p->hover >= 0 && p->tip.alpha > 0.5f;
    int hi_row = on ? p->hover / p->cols : -1, hi_col = on ? p->hover % p->cols : -1;
    /* small cells (a phone): every other row label, so they do not run into each other */
    int every = (int)ceilf(cui__font_line_height(ui, CUI_FONT_BODY, fs) / (g.cell + g.gap));
    if (every < 1) every = 1;
    for (int i = 0; i < p->rows; i++) {
        if (p->o.row_labels && p->o.row_labels[i] && (i % every == 0 || i == hi_row)) {
            /* the hovered row's label speaks up, the rest stay muted */
            bool hl = i == hi_row;
            cui__draw_text_at(ui, hl ? CUI_FONT_MEDIUM : CUI_FONT_BODY, fs, g.gx - 10, g.gy + i * (g.cell + g.gap) + g.cell * 0.5f, CUI__AT_RIGHT,
                              hl ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_MUTED), p->o.row_labels[i], false);
        }
        for (int j = 0; j < p->cols; j++) {
            float t = (p->v[i * p->cols + j] - p->lo) / (p->hi - p->lo);
            float a = cui__clampf(p->intro * 1.8f - (float)j / (float)p->cols * 0.8f, 0, 1);
            cui_rect c = { g.gx + j * (g.cell + g.gap), g.gy + i * (g.cell + g.gap), g.cell, g.cell };
            cui_draw_rect(ui, c, CUI_CORNERS(rad), cui_mix(cui__viz_empty(ui), cui__heat_step(ui, p->base, cui__heat_level(t)), a));
        }
    }
    if (p->hover >= 0 && p->tip.alpha > 0.01f) {
        int i = p->hover / p->cols, j = p->hover % p->cols;
        cui__heat_ring(ui, (cui_rect){ g.gx + j * (g.cell + g.gap), g.gy + i * (g.cell + g.gap), g.cell, g.cell }, rad, p->tip.alpha);
    }
    float by = g.gy + g.grid_h;
    if (p->o.col_labels) {
        float last = -1e9f;
        for (int j = 0; j < p->cols; j++) {
            const char *s = p->o.col_labels[j];
            if (!s) continue;
            bool hl = j == hi_col;
            cui_font f = hl ? CUI_FONT_MEDIUM : CUI_FONT_BODY;
            float cx = g.gx + j * (g.cell + g.gap) + g.cell * 0.5f, w = cui__text_width(ui, f, fs, s, -1, false);
            if (cx - w * 0.5f < last + 6 && !hl) continue;
            cui__draw_text_at(ui, f, fs, cx, by + 13, CUI__AT_CENTER, hl ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_MUTED), s, false);
            last = cx + w * 0.5f;
        }
        by += 22;
    }
    /* the range on the left, the step key on the right */
    float right = g.gx + p->cols * (g.cell + g.gap) - g.gap, ky = by + 10, sz = CUI_MIN(12.0f, g.cell);
    const char *range = cui__frame_printf(ui, "%s \xe2\x80\x93 %s", cui__fmt_value(ui, p->o.format, p->lo), cui__fmt_value(ui, p->o.format, p->hi));
    float rw = cui__text_width(ui, CUI_FONT_BODY, fs, range, -1, true);
    float left = cui__heat_key(ui, p->base, right, ky, sz, g.gx + rw + 16, true);
    if (left == right) left = cui__heat_key(ui, p->base, right, ky, sz, g.gx + rw + 12, false);
    float range_x = p->o.row_labels ? g.gx - g.lw : g.gx;
    if (range_x + rw + 12 <= left) cui__draw_text_at(ui, CUI_FONT_BODY, fs, range_x, ky + sz * 0.5f, CUI__AT_LEFT, CUI_C(ui, TEXT_MUTED), range, true);
    cui__tip_draw(ui, &p->tip);
}

int cui_heatmap_ex(cui_ctx *ui, const char *id_str, const float *values, int rows, int cols, const cui_heatmap_opts *o) {
    cui_heatmap_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    rows = CUI_MAX(0, rows); cols = CUI_MAX(0, cols);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui__hm_geo g = cui__hm_layout(ui, pr, rows, cols, o);
    float lo = o->min, hi = o->max;
    if (!(o->max > o->min)) {
        lo = 1e30f; hi = -1e30f;
        for (int k = 0; k < rows * cols; k++) { lo = CUI_MIN(lo, values[k]); hi = CUI_MAX(hi, values[k]); }
        if (rows * cols == 0) { lo = 0; hi = 1; }
        if (hi <= lo) hi = lo + 1;
    }
    int hover = -1;
    if (r.hovered && rows && cols) {
        int j = (int)floorf((ui->mx - g.gx) / (g.cell + g.gap)), i = (int)floorf((ui->my - g.gy) / (g.cell + g.gap));
        if (i >= 0 && i < rows && j >= 0 && j < cols) hover = i * cols + j;
    }
    cui__viz_hover(ui, st, hover);
    cui__hm_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->v = cui__copy_floats(ui, values, rows * cols);
    p->rows = rows; p->cols = cols; p->lo = lo; p->hi = hi; p->o = *o;
    p->base = cui__heat_base(ui, o->color);
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f ? st->shown : -1;
    if (p->hover >= rows * cols) p->hover = -1;
    if (p->hover >= 0) {
        int i = p->hover / cols, j = p->hover % cols;
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        const char *rl = o->row_labels && o->row_labels[i] ? o->row_labels[i] : cui__frame_printf(ui, "Row %d", i + 1);
        const char *cl = o->col_labels && o->col_labels[j] ? o->col_labels[j] : cui__frame_printf(ui, "%d", j + 1);
        p->tip.title = cui__frame_printf(ui, "%s \xc2\xb7 %s", rl, cl);
        cui__tip_row(&p->tip, "Value", cui__fmt_value(ui, o->format, values[p->hover]),
                     cui__heat_step(ui, p->base, cui__heat_level((values[p->hover] - lo) / (hi - lo))));
    }
    float h = g.grid_h + (o->col_labels ? 22 : 0) + 30;
    cui_size w = o->width.kind ? o->width : (o->cell > 0 ? CUI_PX(g.lw + cols * (g.cell + g.gap) - g.gap) : CUI_FILL);
    cui__viz_node(ui, id, w, CUI_PX(h), cui__paint_heatmap, p, hover >= 0);
    if (hover >= 0) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Calendar heatmap                                                         */
/* ------------------------------------------------------------------------ */

typedef struct {
    float *v; int days, wd0, weeks; int base_day; float maxv, total;
    float cell, gap; cui_color base; cui_calendar_heatmap_opts o;
    float intro; int hover; cui__tip tip;
} cui__cal_paint;


static int cui__cal_level(float v, float maxv) {
    if (v <= 0 || maxv <= 0) return 0;
    int l = 1 + (int)floorf(v / maxv * 4 - 0.0001f);
    return CUI_CLAMP(l, 1, 4);
}

#define CUI__CAL_LW 32.0f
#define CUI__CAL_TOP 20.0f

static void cui__paint_calendar(cui_ctx *ui, cui_node *n, void *data) {
    cui__cal_paint *p = data;
    cui_rect r = n->rect;
    float fs = cui__fs(ui, ui->theme.text_xs), step = p->cell + p->gap;
    float gx = r.x + CUI__CAL_LW, gy = r.y + CUI__CAL_TOP;
    float rad = cui__heat_radius(p->cell);
    /* the hovered day's weekday and month labels speak up */
    bool on = p->hover >= 0 && p->tip.alpha > 0.5f;
    int hi_wd = on ? ((p->base_day + p->hover + 3) % 7 + 7) % 7 : -1;
    cui_date hd = cui_date_from_days(p->base_day + (on ? p->hover : 0));
    for (int k = 0; k < 7; k++) {
        if (k % 2 && k != hi_wd) continue;
        if (k == 6 && k != hi_wd) continue;
        bool hl = k == hi_wd;
        cui__draw_text_at(ui, hl ? CUI_FONT_MEDIUM : CUI_FONT_BODY, fs, r.x, gy + k * step + p->cell * 0.5f, CUI__AT_LEFT,
                          hl ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_MUTED), cui__weekday_short(ui, k), false);
    }
    float last_end = -1e9f;
    for (int w = 0; w < p->weeks; w++) {
        int first = w * 7 - p->wd0;   /* day index of this column's Monday */
        cui_date dd = cui_date_from_days(p->base_day + CUI_MAX(first, 0));
        bool starts = first <= 0 ? (w == 0) : dd.day <= 7;
        float x = gx + w * step;
        if (starts && x > last_end + 4 && w < p->weeks - 1) {
            bool hl = on && dd.month == hd.month && dd.year == hd.year;
            cui_font f = hl ? CUI_FONT_MEDIUM : CUI_FONT_BODY;
            cui__draw_text_at(ui, f, fs, x, r.y + 8, CUI__AT_LEFT, hl ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_MUTED), cui__month_short(ui, dd.month - 1), false);
            last_end = x + cui__text_width(ui, f, fs, cui__month_short(ui, dd.month - 1), -1, false);
        }
        for (int k = 0; k < 7; k++) {
            int di = first + k;
            if (di < 0 || di >= p->days) continue;
            int lv = cui__cal_level(p->v[di], p->maxv);
            float a = cui__clampf(p->intro * 1.8f - (float)w / (float)p->weeks * 0.8f, 0, 1);
            cui_rect c = { x, gy + k * step, p->cell, p->cell };
            cui_draw_rect(ui, c, CUI_CORNERS(rad), cui_mix(cui__viz_empty(ui), cui__heat_step(ui, p->base, lv), a));
        }
    }
    if (p->hover >= 0 && p->tip.alpha > 0.01f) {
        int di = p->hover + p->wd0;
        cui__heat_ring(ui, (cui_rect){ gx + (di / 7) * step, gy + (di % 7) * step, p->cell, p->cell }, rad, p->tip.alpha);
    }
    /* the total on the left, the step key on the right; a narrow chart drops the key's words first */
    float ly = gy + 7 * step + 12;
    float right = gx + p->weeks * step - p->gap;
    float sz = CUI_MIN(p->cell, 12);
    const char *tot = p->o.unit ? cui__frame_printf(ui, "%s %s", cui__fmt_value(ui, p->o.format, p->total), p->o.unit) : NULL;
    float tot_w = tot ? cui__text_width(ui, CUI_FONT_BODY, fs, tot, -1, true) : 0;
    float min_x = tot ? gx + tot_w + 12 : r.x;
    float left = cui__heat_key(ui, p->base, right, ly, sz, min_x, true);
    if (left == right) left = cui__heat_key(ui, p->base, right, ly, sz, r.x, false);
    if (tot && gx + tot_w + 9 <= left) cui__draw_text_at(ui, CUI_FONT_BODY, fs, gx, ly + sz * 0.5f, CUI__AT_LEFT, CUI_C(ui, TEXT_SECONDARY), tot, true);
    cui__tip_draw(ui, &p->tip);
}

int cui_calendar_heatmap_ex(cui_ctx *ui, const char *id_str, cui_date start, const float *values, int days, const cui_calendar_heatmap_opts *o) {
    cui_calendar_heatmap_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    days = CUI_MAX(0, days);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    int base = cui_date_to_days(start);
    int wd0 = ((base + 3) % 7 + 7) % 7;   /* 0 = Monday */
    int weeks = (wd0 + days + 6) / 7;
    float cell = o->cell > 0 ? o->cell : 11, gap = cell >= 10 ? 3 : 2, step = cell + gap;
    float full_w = CUI__CAL_LW + weeks * step - gap;
    if (pr.w > 0 && pr.w + 0.5f < full_w) {
        /* narrower than the whole range: show the most recent weeks that fit */
        int fit = CUI_MAX(1, (int)floorf((pr.w - CUI__CAL_LW + gap) / step));
        if (fit < weeks) {
            int skip = (weeks - fit) * 7 - wd0;
            values += skip; days -= skip; base += skip;
            wd0 = 0; weeks = fit;
        }
    }
    float maxv = 0, total = 0;
    for (int k = 0; k < days; k++) { maxv = CUI_MAX(maxv, values[k]); total += values[k] > 0 ? values[k] : 0; }
    int hover = -1;
    if (r.hovered) {
        int w = (int)floorf((ui->mx - pr.x - CUI__CAL_LW) / step), k = (int)floorf((ui->my - pr.y - CUI__CAL_TOP) / step);
        if (w >= 0 && w < weeks && k >= 0 && k < 7) {
            int di = w * 7 - wd0 + k;
            if (di >= 0 && di < days) hover = di;
        }
    }
    cui__viz_hover(ui, st, hover);
    cui__cal_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->v = cui__copy_floats(ui, values, days);
    p->days = days; p->wd0 = wd0; p->weeks = weeks; p->base_day = base; p->maxv = maxv; p->total = total;
    p->cell = cell; p->gap = gap; p->base = cui__heat_base(ui, o->color); p->o = *o;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < days ? st->shown : -1;
    if (p->hover >= 0) {
        cui_date dd = cui_date_from_days(base + p->hover);
        int wd = ((base + p->hover + 3) % 7 + 7) % 7;
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = cui__frame_printf(ui, "%s, %s %d, %d", cui__weekday_short(ui, wd), cui__month_short(ui, dd.month - 1), dd.day, dd.year);
        cui__tip_row(&p->tip, o->unit ? o->unit : "Value", cui__fmt_value(ui, o->format, values[p->hover]),
                     cui__heat_step(ui, p->base, cui__cal_level(values[p->hover], maxv)));
    }
    float h = CUI__CAL_TOP + 7 * step - gap + 12 + CUI_MIN(cell, 12) + 2;
    int node = cui__viz_node(ui, id, CUI_FIT, CUI_PX(h), cui__paint_calendar, p, hover >= 0);
    cui__soft_width(&ui->nodes[node], CUI_FIT, full_w);
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Treemap                                                                  */
/* ------------------------------------------------------------------------ */

static float cui__sq_worst(const float *v, int n, float sum, float side) {
    float mx = 0, mn = 1e30f;
    for (int k = 0; k < n; k++) { mx = CUI_MAX(mx, v[k]); mn = CUI_MIN(mn, v[k]); }
    float s2 = side * side, sum2 = sum * sum;
    if (mn <= 0 || sum2 <= 0) return 1e30f;
    return CUI_MAX(s2 * mx / sum2, sum2 / (s2 * mn));
}

/* squarified layout of areas (sorted, largest first) into r */
static void cui__squarify(const float *v, int n, cui_rect r, cui_rect *out) {
    int i = 0;
    while (i < n) {
        float side = CUI_MIN(r.w, r.h);
        if (side <= 0) { for (; i < n; i++) out[i] = (cui_rect){ r.x, r.y, 0, 0 }; return; }
        int j = i;
        float sum = 0, worst = 1e30f;
        while (j < n) {
            float s2 = sum + v[j];
            float w = cui__sq_worst(v + i, j - i + 1, s2, side);
            if (j > i && w > worst) break;
            worst = w; sum = s2; j++;
        }
        float thick = sum / side;
        if (r.w >= r.h) {
            float y = r.y;
            for (int k = i; k < j; k++) { float h = thick > 0 ? v[k] / thick : 0; out[k] = (cui_rect){ r.x, y, thick, h }; y += h; }
            r.x += thick; r.w -= thick;
        } else {
            float x = r.x;
            for (int k = i; k < j; k++) { float w = thick > 0 ? v[k] / thick : 0; out[k] = (cui_rect){ x, r.y, w, thick }; x += w; }
            r.y += thick; r.h -= thick;
        }
        i = j;
    }
}

typedef struct { cui_viz_item *items; int count; int *order; float total; const char *format; float intro; int hover, selected; cui__tip tip; } cui__treemap_paint;

/* explicit colors stay; the rest are neutral tiles a little lighter (darker in light mode)
 * for more, so size reads without a hue competing with the accent that marks a tile */
static cui_color cui__tm_color(cui_ctx *ui, const cui_viz_item *items, const int *order, int idx) {
    if (cui__color_set(items[idx].color)) return cui_color_of(ui, items[idx].color);
    float top = items[order[0]].value;
    float t = top > 0 ? items[idx].value / top : 0;
    return cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, FOREGROUND), (ui->mode == CUI_THEME_DARK ? 0.045f : 0.035f) + 0.045f * t);
}

static cui_rect *cui__tm_layout(cui_ctx *ui, cui_rect r, const cui_viz_item *items, const int *order, int n, float total) {
    float *a = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, n));
    cui_rect *out = cui__frame_alloc(ui, sizeof(cui_rect) * (size_t)CUI_MAX(1, n));
    float area = r.w * r.h;
    int m = 0;
    for (int k = 0; k < n; k++) { a[k] = total > 0 ? items[order[k]].value / total * area : 0; if (a[k] > 0) m = k + 1; }
    cui__squarify(a, m, r, out);
    for (int k = m; k < n; k++) out[k] = (cui_rect){ r.x, r.y, 0, 0 };
    return out;   /* indexed by rank */
}

/* one tile's text: label, value and share where they fit, else just a compact value */
static void cui__tm_text(cui_ctx *ui, const cui__treemap_paint *p, int idx, cui_rect c, cui_color fill, bool marked, float a) {
    float fs = cui__fs(ui, ui->theme.text_sm), fx = cui__fs(ui, ui->theme.text_xs);
    const cui_viz_item *it = &p->items[idx];
    bool own = cui__color_set(it->color) && !marked;
    cui_color em = cui__emphasis(ui);
    cui_color tc = own ? (cui__viz_light_text(fill) ? cui__rgba(255, 255, 255, 255) : cui__rgba(20, 20, 20, 255)) : marked ? em : CUI_C(ui, FOREGROUND);
    cui_color mc = own ? cui_alpha(tc, 0.75f) : CUI_C(ui, TEXT_MUTED);
    const char *val = cui__fmt_value(ui, p->format, it->value);
    cui_push_clip(ui, c);
    if (c.w > 60 && c.h > 50 && it->label) {
        float lh = cui__font_line_height(ui, CUI_FONT_MEDIUM, fs);
        cui__draw_text_line(ui, CUI_FONT_MEDIUM, fs, c.x + 12, c.y + 10, cui_alpha(tc, a), it->label, -1, false);
        cui__draw_text_line(ui, CUI_FONT_BODY, fx, c.x + 12, c.y + 10 + lh + 1, cui_alpha(mc, a), val, -1, true);
        if (c.h > 90 && c.w > 70) {
            cui_color sc = marked ? em : own ? mc : cui_mix(CUI_C(ui, TEXT_MUTED), fill, 0.35f);
            cui__draw_text_at(ui, CUI_FONT_MEDIUM, fx, c.x + c.w - 12, c.y + c.h - 14, CUI__AT_RIGHT, cui_alpha(sc, a), cui__pct(ui, it->value, p->total), true);
        }
    } else {
        /* a small tile keeps a compact value, or nothing when even that does not fit */
        if (cui__text_width(ui, CUI_FONT_BODY, fx, val, -1, true) > c.w - 8) val = cui__fmt_value(ui, NULL, it->value);
        if (cui__text_width(ui, CUI_FONT_BODY, fx, val, -1, true) <= c.w - 8 && c.h >= 18)
            cui__draw_text_at(ui, CUI_FONT_BODY, fx, c.x + c.w * 0.5f, c.y + c.h * 0.5f, CUI__AT_CENTER, cui_alpha(marked ? em : mc, a), val, true);
    }
    cui_pop_clip(ui);
}

static void cui__paint_treemap(cui_ctx *ui, cui_node *n, void *data) {
    cui__treemap_paint *p = data;
    cui_rect *rc = cui__tm_layout(ui, n->rect, p->items, p->order, p->count, p->total);
    cui_color em = cui__emphasis(ui), hair = cui_alpha(CUI_C(ui, FOREGROUND), ui->mode == CUI_THEME_DARK ? 0.04f : 0.05f);
    for (int k = 0; k < p->count; k++) {
        int idx = p->order[k];
        cui_rect c = { rc[k].x + 1.5f, rc[k].y + 1.5f, rc[k].w - 3, rc[k].h - 3 };   /* 3 point gaps */
        if (c.w <= 0.5f || c.h <= 0.5f) continue;
        float a = cui__clampf(p->intro * 1.6f - (float)k / (float)p->count * 0.6f, 0, 1);
        float s = 0.92f + 0.08f * a;
        cui_rect sc = { c.x + c.w * (1 - s) * 0.5f, c.y + c.h * (1 - s) * 0.5f, c.w * s, c.h * s };
        cui_color col = cui__tm_color(ui, p->items, p->order, idx);
        /* the accent marks the selected tile, and the hovered one as the pointer arrives */
        float mark = idx == p->selected ? 1.0f : idx == p->hover ? p->tip.alpha : 0.0f;
        float rad = CUI_MIN(10.0f, CUI_MIN(c.w, c.h) * 0.25f);
        cui_color fill = mark > 0 ? cui_mix(col, em, (ui->mode == CUI_THEME_DARK ? 0.13f : 0.10f) * mark) : col;
        cui_color edge = mark > 0 ? cui_mix(hair, em, mark) : hair;
        cui_draw_rect_border(ui, sc, CUI_CORNERS(rad), cui_alpha(fill, a), cui_alpha(edge, a), mark > 0 ? 1.5f : 1.0f);
        cui__tm_text(ui, p, idx, c, fill, mark > 0.5f, a);
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_treemap_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *items_in, int count, const cui_treemap_opts *o) {
    cui_treemap_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui_viz_item *items = cui__copy_items(ui, items_in, count);
    int *order = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, count));
    float total = 0;
    for (int k = 0; k < count; k++) { order[k] = k; total += items[k].value; }
    for (int a = 1; a < count; a++) {   /* largest first, stable */
        int v = order[a], b = a - 1;
        while (b >= 0 && items[order[b]].value < items[v].value) { order[b + 1] = order[b]; b--; }
        order[b + 1] = v;
    }
    int hover = -1;
    if (r.hovered && pr.w > 0) {
        cui_rect *rc = cui__tm_layout(ui, pr, items, order, count, total);
        for (int k = 0; k < count; k++)
            if (ui->mx >= rc[k].x && ui->mx < rc[k].x + rc[k].w && ui->my >= rc[k].y && ui->my < rc[k].y + rc[k].h) hover = order[k];
    }
    cui__viz_hover(ui, st, hover);
    if (o->selected && r.clicked && hover >= 0) *o->selected = *o->selected == hover ? -1 : hover;
    cui__treemap_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->items = items; p->count = count; p->order = order; p->total = total; p->format = o->format;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < count ? st->shown : -1;
    p->selected = o->selected && *o->selected >= 0 && *o->selected < count ? *o->selected : -1;
    if (p->hover >= 0) {
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = items[p->hover].label ? items[p->hover].label : "";
        cui_color sw = cui__color_set(items[p->hover].color) ? cui__tm_color(ui, items, order, p->hover) : cui__rgba(0, 0, 0, 0);
        cui__tip_row(&p->tip, "Value", cui__fmt_value(ui, o->format, items[p->hover].value), sw);
        cui__tip_row(&p->tip, "Share", cui__pct(ui, items[p->hover].value, total), cui__rgba(0, 0, 0, 0));
    }
    cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(o->height > 0 ? o->height : 300), cui__paint_treemap, p, hover >= 0);
    if (hover >= 0) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Radar                                                                    */
/* ------------------------------------------------------------------------ */

typedef struct {
    cui_series *series; int count; const char **axes; int na;
    float maxv, step; cui_radar_opts o; float size;
    float intro; int hover, emph; cui__tip tip; float legend_h;
} cui__radar_paint;

/* where the ring values go: beside one spoke, just outside each ring's corner. The most
 * upright spoke whose labels all stay clear of the series outlines, or when none does, the
 * top spoke, where the labels that cross a line get a backing. */
typedef struct { cui_vec2 at[8]; cui_rect box[8]; bool hit[8]; cui__text_anchor anchor; } cui__radar_labels;

static bool cui__seg_hits_rect(cui_vec2 a, cui_vec2 b, cui_rect r) {
    int n = (int)(sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) / 2) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        if (cui__rect_contains(r, a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t)) return true;
    }
    return false;
}

static cui__radar_labels cui__radar_place_labels(cui_ctx *ui, const cui__radar_paint *p, cui_vec2 c, float R, const cui_vec2 *dir, int rings, float fs) {
    int na = p->na;
    cui_vec2 *pts = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)na * (size_t)CUI_MAX(1, p->count));
    for (int k = 0; k < p->count; k++)
        for (int i = 0; i < na; i++) {
            float v = i < p->series[k].count ? cui__clampf(p->series[k].values[i] / p->maxv, 0, 1.15f) : 0;
            pts[k * na + i] = (cui_vec2){ c.x + dir[i].x * R * v, c.y + dir[i].y * R * v };
        }
    cui__radar_labels best_l, top_l;
    memset(&best_l, 0, sizeof best_l);
    memset(&top_l, 0, sizeof top_l);
    float best = 1e30f;
    for (int i = 0; i < na; i++)
        for (int side = 0; side < 2; side++) {
            cui_vec2 d = dir[i], perp = { side ? d.y : -d.y, side ? -d.x : d.x };
            cui__text_anchor an = perp.x > 0.3f ? CUI__AT_LEFT : perp.x < -0.3f ? CUI__AT_RIGHT : CUI__AT_CENTER;
            float off = an == CUI__AT_CENTER ? 9 : 5;
            cui__radar_labels l;
            l.anchor = an;
            float cost = fabsf(d.x) * 0.5f + (perp.x < 0 ? 0.05f : 0);   /* upright spokes, then the right side, read best */
            bool clear = true;
            for (int g = 1; g < rings; g++) {
                float rr = R * g / rings + 6;
                cui_vec2 at = { c.x + d.x * rr + perp.x * off, c.y + d.y * rr + perp.y * off };
                l.at[g - 1] = at;
                float w = cui__text_width(ui, CUI_FONT_BODY, fs, cui__fmt_value(ui, p->o.format, p->step * g), -1, true);
                float x0 = an == CUI__AT_LEFT ? at.x : an == CUI__AT_RIGHT ? at.x - w : at.x - w * 0.5f;
                cui_rect box = { x0 - 2, at.y - 7, w + 4, 14 };
                l.box[g - 1] = box;
                l.hit[g - 1] = false;
                for (int k = 0; k < p->count && !l.hit[g - 1]; k++)
                    for (int j = 0; j < na; j++)
                        if (cui__seg_hits_rect(pts[k * na + j], pts[k * na + (j + 1) % na], box)) { clear = false; l.hit[g - 1] = true; break; }
            }
            if (i == 0 && side == 0) top_l = l;
            if (clear && cost < best - 1e-4f) { best = cost; best_l = l; }
        }
    return best < 1e29f ? best_l : top_l;
}

static void cui__paint_radar(cui_ctx *ui, cui_node *n, void *data) {
    cui__radar_paint *p = data;
    cui_rect r = n->rect;
    float R = p->size * 0.5f;
    cui_vec2 c = { r.x + r.w * 0.5f, r.y + 26 + R };
    float fs = cui__fs(ui, ui->theme.text_xs);
    int na = p->na;
    if (na < 3) return;
    cui_vec2 *dir = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)na);
    for (int i = 0; i < na; i++) { float a = -CUI_PI * 0.5f + 2 * CUI_PI * i / na; dir[i].x = cosf(a); dir[i].y = sinf(a); }
    /* grid */
    int rings = (int)lroundf(p->maxv / p->step);
    rings = CUI_CLAMP(rings, 1, 8);
    cui_vec2 *pts = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)na);
    cui__radar_labels lab = cui__radar_place_labels(ui, p, c, R, dir, rings, fs * 0.92f);
    /* hairline rings and spokes, a step stronger at the rim */
    bool dark = ui->mode == CUI_THEME_DARK;
    cui_color hair = cui_alpha(CUI_C(ui, FOREGROUND), dark ? 0.07f : 0.08f), rim = cui_alpha(CUI_C(ui, FOREGROUND), dark ? 0.12f : 0.14f);
    for (int g = 1; g <= rings; g++) {
        float rr = R * g / rings;
        for (int i = 0; i < na; i++) { pts[i].x = c.x + dir[i].x * rr; pts[i].y = c.y + dir[i].y * rr; }
        cui_draw_polyline(ui, pts, na, 1, g == rings ? rim : hair, true);
    }
    float ha = p->hover >= 0 ? p->tip.alpha : 0;
    for (int i = 0; i < na; i++) {
        bool h = i == p->hover && ha > 0.01f;
        cui_draw_line(ui, c, (cui_vec2){ c.x + dir[i].x * R, c.y + dir[i].y * R }, 1, h ? cui_mix(hair, CUI_C(ui, BORDER_STRONG), ha) : hair);
        float lx = c.x + dir[i].x * (R + 14), ly = c.y + dir[i].y * (R + 12);
        cui__text_anchor al = dir[i].x > 0.3f ? CUI__AT_LEFT : dir[i].x < -0.3f ? CUI__AT_RIGHT : CUI__AT_CENTER;
        if (al == 1) ly += dir[i].y * 4;
        cui__draw_text_at(ui, h ? CUI_FONT_MEDIUM : CUI_FONT_BODY, fs, lx, ly, al, h ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_MUTED), p->axes[i] ? p->axes[i] : "", false);
    }
    /* ring values beside their spoke, clear of the rings, the spoke and the data. The outer
     * ring meets the axis names, so it goes without. */
    for (int g = 1; g < rings; g++)
        if (!lab.hit[g - 1])
            cui__draw_text_at(ui, CUI_FONT_BODY, fs * 0.92f, lab.at[g - 1].x, lab.at[g - 1].y, lab.anchor, CUI_C(ui, TEXT_MUTED), cui__fmt_value(ui, p->o.format, p->step * g), true);
    /* series: every one lightly filled with a crisp outline; points only on the emphasized one */
    cui_vec2 chip_at = { 0, 0 };
    float chip_v = 0;
    bool chip = false;
    for (int k = 0; k < p->count; k++) {
        cui_series *s = &p->series[k];
        cui_color col = cui__viz_color(ui, s->color, k);
        for (int i = 0; i < na; i++) {
            float v = i < s->count ? cui__clampf(s->values[i] / p->maxv, 0, 1.15f) : 0;
            float e = cui__clampf(p->intro, 0, 1);
            pts[i].x = c.x + dir[i].x * R * v * e; pts[i].y = c.y + dir[i].y * R * v * e;
        }
        cui_vec2 *ring = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)na);
        memcpy(ring, pts, sizeof(cui_vec2) * (size_t)na);
        cui_cvert *vv = cui__frame_alloc(ui, sizeof(cui_cvert) * (size_t)(na + 1));
        uint32_t *idx = cui__frame_alloc(ui, sizeof(uint32_t) * (size_t)na * 3);
        cui_color fill = cui_alpha(col, dark ? 0.12f : 0.10f);
        vv[0].x = c.x; vv[0].y = c.y; vv[0].c = fill;
        for (int i = 0; i < na; i++) { vv[i + 1].x = ring[i].x; vv[i + 1].y = ring[i].y; vv[i + 1].c = fill; }
        for (int i = 0; i < na; i++) { idx[i * 3] = 0; idx[i * 3 + 1] = (uint32_t)(i + 1); idx[i * 3 + 2] = (uint32_t)((i + 1) % na + 1); }
        cui__draw_triangles(ui, vv, na + 1, idx, na * 3);
        cui_draw_polyline(ui, ring, na, 1.75f, col, true);
        if (k != p->emph) continue;
        for (int i = 0; i < na; i++) {
            bool h = i == p->hover && ha > 0.01f;
            float rr = h ? 4.0f : 3.2f;
            cui_draw_circle(ui, ring[i], rr + 1.75f, col);
            cui_draw_circle(ui, ring[i], rr, CUI_C(ui, SURFACE));
            if (h && i < s->count) { chip = true; chip_at = ring[i]; chip_v = s->values[i]; }
        }
    }
    /* a value no spoke could keep clear of the data sits on a backing of the surface, over the lines */
    for (int g = 1; g < rings; g++)
        if (lab.hit[g - 1]) {
            cui_draw_rect(ui, lab.box[g - 1], CUI_CORNERS(5), CUI_C(ui, SURFACE));
            cui__draw_text_at(ui, CUI_FONT_BODY, fs * 0.92f, lab.at[g - 1].x, lab.at[g - 1].y, lab.anchor, CUI_C(ui, TEXT_MUTED), cui__fmt_value(ui, p->o.format, p->step * g), true);
        }
    if (chip) {
        /* the hovered value in a small chip above its point, toward the middle, clear of the axis names */
        const char *vs = cui__fmt_value(ui, p->o.format, chip_v);
        float cf = fs, w = cui__text_width(ui, CUI_FONT_MEDIUM, cf, vs, -1, true) + 14, h = 18;
        float x = chip_at.x > c.x + 1 ? chip_at.x - 8 - w : chip_at.x + 8, y = chip_at.y - 22;
        cui__push_xform(ui, 1, 0, 0, ha, 0, 0);
        cui_draw_rect_border(ui, (cui_rect){ x, y, w, h }, CUI_CORNERS(6), CUI_C(ui, SURFACE_RAISED), dark ? CUI_C(ui, BORDER) : cui__rgba(0, 0, 0, 22), 1);
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, cf, x + w * 0.5f, y + h * 0.5f, CUI__AT_CENTER, CUI_C(ui, FOREGROUND), vs, true);
        cui__pop_xform(ui);
    }
    if (p->count >= 2 && !p->o.no_legend) {
        const char **names = cui__frame_alloc(ui, sizeof(char *) * (size_t)p->count);
        cui_color *cols = cui__frame_alloc(ui, sizeof(cui_color) * (size_t)p->count);
        float lw = 0;
        for (int k = 0; k < p->count; k++) { names[k] = p->series[k].name; cols[k] = cui__viz_color(ui, p->series[k].color, k); }
        for (int k = 0; k < p->count; k++) lw += 14 + cui__text_width(ui, CUI_FONT_BODY, fs, names[k] ? names[k] : "", -1, false) + 16;
        lw = CUI_MIN(lw - 16, r.w);
        cui__viz_legend(ui, r.x + (r.w - lw) * 0.5f, c.y + R + 34, lw + 1, names, cols, p->count, -1, true);
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_radar_chart_ex(cui_ctx *ui, const char *id_str, const cui_series *series, int count, const char *const *axes, int na, const cui_radar_opts *o) {
    cui_radar_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count); na = CUI_MAX(0, na);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    float size = o->size > 0 ? o->size : 260;
    /* when the parent is narrower than the plot and its 90px label gutters, the plot shrinks
       and the gutters keep just enough room for the side axis names */
    float full = size, min_size = CUI_MIN(size, 110.0f), tight = 0, fsx = cui__fs(ui, ui->theme.text_xs);
    for (int i = 0; i < na; i++)
        if (axes && axes[i] && fabsf(cosf(-CUI_PI * 0.5f + 2 * CUI_PI * i / na)) > 0.3f)
            tight = CUI_MAX(tight, cui__text_width(ui, CUI_FONT_MEDIUM, fsx, axes[i], -1, false) + 18);
    if (pr.w > 0 && pr.w < full + 180) size = CUI_CLAMP(floorf(pr.w - 2 * tight), min_size, full);
    float maxv = 0;
    for (int k = 0; k < count; k++)
        for (int i = 0; i < series[k].count && i < na; i++) maxv = CUI_MAX(maxv, series[k].values[i]);
    float lo, hi, step;
    if (o->max > 0) { hi = o->max; step = o->max / 4; }
    else { cui__nice_range(0, maxv > 0 ? maxv : 1, 4, &lo, &hi, &step); }
    int hover = -1;
    if (r.hovered && na >= 3) {
        float R = size * 0.5f;
        float cx = pr.x + pr.w * 0.5f, cy = pr.y + 26 + R;
        float dx = ui->mx - cx, dy = ui->my - cy;
        if (dx * dx + dy * dy < (R + 36) * (R + 36)) {
            float a = atan2f(dy, dx) + CUI_PI * 0.5f;
            if (a < 0) a += 2 * CUI_PI;
            hover = (int)lroundf(a / (2 * CUI_PI) * na) % na;
        }
    }
    cui__viz_hover(ui, st, hover);
    cui__radar_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->series = cui__frame_alloc(ui, sizeof(cui_series) * (size_t)CUI_MAX(1, count));
    for (int k = 0; k < count; k++) { p->series[k] = series[k]; p->series[k].values = cui__copy_floats(ui, series[k].values, series[k].count); }
    p->axes = cui__frame_alloc(ui, sizeof(char *) * (size_t)CUI_MAX(1, na));
    for (int i = 0; i < na; i++) p->axes[i] = axes ? axes[i] : NULL;
    p->count = count; p->na = na; p->maxv = hi; p->step = step; p->o = *o; p->size = size;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < na ? st->shown : -1;
    /* the last series (usually the newest) shows its points; on hover, the series whose point
     * on the hovered axis is nearest the pointer, with a chip of its value there */
    p->emph = count - 1;
    if (p->hover >= 0) {
        p->tip.alpha = st->hover_t;   /* no tooltip: the alpha fades the chip */
        float R = size * 0.5f, cx = pr.x + pr.w * 0.5f, cy = pr.y + 26 + R, best_d = 1e30f;
        float a = -CUI_PI * 0.5f + 2 * CUI_PI * p->hover / CUI_MAX(1, na);
        for (int k = 0; k < count; k++) {
            if (p->hover >= series[k].count) continue;
            float v = cui__clampf(series[k].values[p->hover] / hi, 0, 1.15f) * R;
            float dx = cx + cosf(a) * v - ui->mx, dy = cy + sinf(a) * v - ui->my;
            if (dx * dx + dy * dy < best_d) { best_d = dx * dx + dy * dy; p->emph = k; }
        }
        st->emph = p->emph;
    } else if (st->hover_t > 0.01f) p->emph = CUI_CLAMP(st->emph, 0, count - 1);
    float h = size + 52 + (count >= 2 && !o->no_legend ? 30 : 0);
    int node = cui__viz_node(ui, id, CUI_FIT, CUI_PX(h), cui__paint_radar, p, hover >= 0);
    cui__soft_width(&ui->nodes[node], CUI_FIT, full + 180);
    ui->nodes[node].min_w = CUI_MIN(min_size + 2 * tight, full + 180);
    ui->nodes[node].flags |= CUI_NF_ADAPTIVE;
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Funnel                                                                   */
/* ------------------------------------------------------------------------ */

#define CUI__FN_ROW 40.0f
#define CUI__FN_LINK 14.0f

typedef struct { cui_viz_item *items; int count; float maxv; cui_funnel_opts o; cui_color base; float intro; int hover; cui__tip tip; float lw; } cui__fn_paint;

static void cui__paint_funnel(cui_ctx *ui, cui_node *n, void *data) {
    cui__fn_paint *p = data;
    cui_rect r = n->rect;
    float fs = cui__fs(ui, ui->theme.text_sm), fx = cui__fs(ui, ui->theme.text_xs);
    float rw = 64, bx = r.x + p->lw, bw = r.w - p->lw - rw;
    float cx = bx + bw * 0.5f;
    float prev_x0 = 0, prev_x1 = 0, prev_y = 0;
    for (int k = 0; k < p->count; k++) {
        float y = r.y + k * (CUI__FN_ROW + CUI__FN_LINK);
        float e = cui__clampf(p->intro * 1.6f - (float)k / (float)p->count * 0.6f, 0, 1);
        float frac = p->maxv > 0 ? p->items[k].value / p->maxv : 0;
        float w = CUI_MAX(6.0f, bw * frac * e);
        float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
        cui_color col = cui__color_set(p->items[k].color) ? cui_color_of(ui, p->items[k].color) : cui_mix(p->base, CUI_C(ui, SURFACE), 0.13f * (float)k);
        bool hov = k == p->hover && p->tip.alpha > 0.01f;
        if (p->hover >= 0 && !hov) col = cui_mix(col, CUI_C(ui, SURFACE), 0.3f * p->tip.alpha);
        if (k > 0) {
            cui_vec2 q[4] = { { prev_x0, prev_y }, { prev_x1, prev_y }, { x1, y }, { x0, y } };
            cui_draw_polygon(ui, q, 4, cui_alpha(col, 0.16f));
        }
        cui_draw_rect(ui, (cui_rect){ x0, y, w, CUI__FN_ROW }, CUI_CORNERS(8), col);
        const char *vs = cui__fmt_value(ui, p->o.format, p->items[k].value);
        float vw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, vs, -1, true);
        if (vw + 20 < w) cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, cx, y + CUI__FN_ROW * 0.5f, CUI__AT_CENTER, cui__viz_light_text(col) ? cui__rgba(255, 255, 255, 255) : cui__rgba(20, 20, 20, 255), vs, true);
        else cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, x1 + 8, y + CUI__FN_ROW * 0.5f, CUI__AT_LEFT, CUI_C(ui, FOREGROUND), vs, true);
        cui__draw_text_at(ui, hov ? CUI_FONT_MEDIUM : CUI_FONT_BODY, fs, r.x, y + CUI__FN_ROW * 0.5f, CUI__AT_LEFT, CUI_C(ui, FOREGROUND), p->items[k].label ? p->items[k].label : "", false);
        const char *pc = cui__pct(ui, p->items[k].value, p->items[0].value);
        /* the second stage's step and overall rates are the same number: say it once. The step
           rate is said plainly, shorter on a narrow chart, and left to the tooltip when even that
           would run into the bar's value */
        const char *step = NULL;
        if (k > 1) {
            float limit = (vw + 20 < w ? x1 : x1 + 8 + vw) + 6;
            const char *sp = cui__pct(ui, p->items[k].value, p->items[k - 1].value);
            step = cui__frame_printf(ui, "%s of previous", sp);
            if (r.x + r.w - cui__text_width(ui, CUI_FONT_BODY, fx, step, -1, true) < limit) step = cui__frame_printf(ui, "%s of prev.", sp);
            if (r.x + r.w - cui__text_width(ui, CUI_FONT_BODY, fx, step, -1, true) < limit) step = NULL;
        }
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, r.x + r.w, y + CUI__FN_ROW * 0.5f - (step ? 7 : 0), CUI__AT_RIGHT, CUI_C(ui, FOREGROUND), pc, true);
        if (step) cui__draw_text_at(ui, CUI_FONT_BODY, fx, r.x + r.w, y + CUI__FN_ROW * 0.5f + 9, CUI__AT_RIGHT, CUI_C(ui, TEXT_MUTED), step, true);
        prev_x0 = x0; prev_x1 = x1; prev_y = y + CUI__FN_ROW;
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_funnel_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *stages, int count, const cui_funnel_opts *o) {
    cui_funnel_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui_viz_item *items = cui__copy_items(ui, stages, count);
    float maxv = 0, lw = 0, fs = cui__fs(ui, ui->theme.text_sm);
    for (int k = 0; k < count; k++) {
        maxv = CUI_MAX(maxv, items[k].value);
        lw = CUI_MAX(lw, cui__text_width(ui, CUI_FONT_MEDIUM, fs, items[k].label ? items[k].label : "", -1, false));
    }
    lw += 20;
    int hover = -1;
    if (r.hovered) {
        int k = (int)floorf((ui->my - pr.y + CUI__FN_LINK * 0.5f) / (CUI__FN_ROW + CUI__FN_LINK));
        if (k >= 0 && k < count) hover = k;
    }
    cui__viz_hover(ui, st, hover);
    cui__fn_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->items = items; p->count = count; p->maxv = maxv; p->o = *o; p->base = cui__viz_base(ui, o->color); p->lw = lw;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < count ? st->shown : -1;
    if (p->hover >= 0) {
        int k = p->hover;
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = items[k].label ? items[k].label : "";
        cui__tip_row(&p->tip, "Count", cui__fmt_value(ui, o->format, items[k].value), cui__rgba(0, 0, 0, 0));
        cui__tip_row(&p->tip, "Of the first stage", cui__pct(ui, items[k].value, items[0].value), cui__rgba(0, 0, 0, 0));
        if (k > 0) cui__tip_row(&p->tip, "From the stage before", cui__pct(ui, items[k].value, items[k - 1].value), cui__rgba(0, 0, 0, 0));
    }
    float h = count * CUI__FN_ROW + CUI_MAX(0, count - 1) * CUI__FN_LINK;
    cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_funnel, p, hover >= 0);
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Sankey                                                                   */
/* ------------------------------------------------------------------------ */

typedef struct {
    int nn, nl;
    const char **names;
    cui_sankey_link *links;
    cui_rect *node;          /* node rects */
    float *sy, *ty;          /* each link's band top at the source and the target */
    float k;                 /* points per unit */
    float *in, *out;
    int *col; int maxcol;
    int *color;              /* palette slot of an outcome, or -1 for neutral */
    float total;             /* what leaves the sources: the whole the shares are of */
} cui__sk_geo;

/* A node's label is two lines: its name, then its value and (past the sources) its share of
 * the whole, "860 · 62%". */
static const char *cui__sk_value_text(cui_ctx *ui, const cui__sk_geo *g, int i, const char *fmt) {
    const char *val = cui__fmt_value(ui, fmt, CUI_MAX(g->in[i], g->out[i]));
    if (g->in[i] <= 0 || g->total <= 0) return val;
    return cui__frame_printf(ui, "%s \xc2\xb7 %s", val, cui__pct(ui, CUI_MAX(g->in[i], g->out[i]), g->total));
}
static void cui__sk_label_widths(cui_ctx *ui, const cui__sk_geo *g, int i, const char *fmt, float *name_w, float *value_w) {
    float fs = cui__fs(ui, ui->theme.text_sm), fx = cui__fs(ui, ui->theme.text_xs);
    *name_w = cui__text_width(ui, CUI_FONT_MEDIUM, fs, g->names && g->names[i] ? g->names[i] : "", -1, false);
    *value_w = cui__text_width(ui, CUI_FONT_BODY, fx, cui__sk_value_text(ui, g, i, fmt), -1, true);
}

static cui__sk_geo cui__sk_layout(cui_ctx *ui, cui_rect r, const char *const *names, int nn, const cui_sankey_link *links_in, int nl, float nw, const char *fmt) {
    cui__sk_geo g;
    memset(&g, 0, sizeof g);
    g.nn = nn; g.nl = nl;
    g.names = (const char **)names;
    g.links = cui__frame_alloc(ui, sizeof(cui_sankey_link) * (size_t)CUI_MAX(1, nl));
    int m = 0;
    for (int l = 0; l < nl; l++)
        if (links_in[l].from >= 0 && links_in[l].from < nn && links_in[l].to >= 0 && links_in[l].to < nn && links_in[l].from != links_in[l].to && links_in[l].value > 0)
            g.links[m++] = links_in[l];
    g.nl = nl = m;
    g.node = cui__frame_alloc(ui, sizeof(cui_rect) * (size_t)CUI_MAX(1, nn));
    g.sy = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, nl));
    g.ty = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, nl));
    g.in = cui__copy_floats(ui, NULL, nn);
    g.out = cui__copy_floats(ui, NULL, nn);
    g.col = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, nn));
    for (int i = 0; i < nn; i++) g.col[i] = 0;
    for (int l = 0; l < nl; l++) { g.out[g.links[l].from] += g.links[l].value; g.in[g.links[l].to] += g.links[l].value; }
    /* depth: longest path from a source (bounded, so cycles cannot hang) */
    for (int it = 0; it < nn; it++) {
        bool ch = false;
        for (int l = 0; l < nl; l++) {
            int a = g.links[l].from, b = g.links[l].to;
            if (g.col[b] < g.col[a] + 1 && g.col[a] + 1 < nn) { g.col[b] = g.col[a] + 1; ch = true; }
        }
        if (!ch) break;
    }
    g.maxcol = 0;
    for (int i = 0; i < nn; i++) g.maxcol = CUI_MAX(g.maxcol, g.col[i]);
    /* an outcome that ends early (a drop off) stays at its own step; only one fed from the last step sits at the far right,
       so no ribbon has to cross a column of nodes to reach it */
    /* room on the right for the last column's labels */
    float right_w = 0;
    g.total = 0;
    for (int i = 0; i < nn; i++) if (g.in[i] <= 0) g.total += g.out[i];
    if (g.maxcol > 0)
        for (int i = 0; i < nn; i++)
            if (g.col[i] == g.maxcol && (g.in[i] > 0 || g.out[i] > 0)) {
                float nw2, vw2;
                cui__sk_label_widths(ui, &g, i, fmt, &nw2, &vw2);
                right_w = CUI_MAX(right_w, CUI_MAX(nw2, vw2) + 12);
            }
    float span = r.w - right_w - nw;
    /* color goes to the outcomes, the largest first; sources and steps stay neutral, so the eye follows where things end up */
    g.color = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, nn));
    for (int i = 0; i < nn; i++) g.color[i] = -1;
    for (int slot = 0;; slot++) {
        int best = -1;
        for (int i = 0; i < nn; i++)
            if (g.color[i] < 0 && g.out[i] <= 0 && g.in[i] > 0 && (best < 0 || g.in[i] > g.in[best])) best = i;
        if (best < 0) break;
        g.color[best] = slot;
    }
    float pad = 14;
    g.k = 1e30f;
    for (int c = 0; c <= g.maxcol; c++) {
        float sum = 0; int cnt = 0;
        for (int i = 0; i < nn; i++) if (g.col[i] == c) { sum += CUI_MAX(g.in[i], g.out[i]); cnt++; }
        if (cnt && sum > 0) g.k = CUI_MIN(g.k, (r.h - pad * (cnt - 1)) / sum);
    }
    if (g.k > 1e29f || g.k < 0) g.k = 0;
    for (int c = 0; c <= g.maxcol; c++) {
        float sum = 0; int cnt = 0;
        for (int i = 0; i < nn; i++) if (g.col[i] == c) { sum += CUI_MAX(g.in[i], g.out[i]) * g.k; cnt++; }
        float y = r.y + (r.h - sum - pad * CUI_MAX(0, cnt - 1)) * 0.5f;
        float x = g.maxcol > 0 ? r.x + span * c / g.maxcol : r.x;
        for (int i = 0; i < nn; i++) {
            if (g.col[i] != c) continue;
            float h = CUI_MAX(g.in[i], g.out[i]) * g.k;
            g.node[i] = (cui_rect){ x, y, nw, CUI_MAX(2.0f, h) };
            y += h + pad;
        }
    }
    /* link bands, ordered by the other end's position so ribbons do not cross needlessly */
    int *ord = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, nl));
    for (int l = 0; l < nl; l++) ord[l] = l;
    for (int a = 1; a < nl; a++) {
        int v = ord[a], b = a - 1;
        while (b >= 0 && g.node[g.links[ord[b]].to].y > g.node[g.links[v].to].y) { ord[b + 1] = ord[b]; b--; }
        ord[b + 1] = v;
    }
    float *off = cui__copy_floats(ui, NULL, nn);
    for (int q = 0; q < nl; q++) { int l = ord[q]; int a = g.links[l].from; g.sy[l] = g.node[a].y + off[a]; off[a] += g.links[l].value * g.k; }
    for (int a = 1; a < nl; a++) {
        int v = ord[a], b = a - 1;
        while (b >= 0 && g.node[g.links[ord[b]].from].y > g.node[g.links[v].from].y) { ord[b + 1] = ord[b]; b--; }
        ord[b + 1] = v;
    }
    memset(off, 0, sizeof(float) * (size_t)nn);
    for (int q = 0; q < nl; q++) { int l = ord[q]; int b = g.links[l].to; g.ty[l] = g.node[b].y + off[b]; off[b] += g.links[l].value * g.k; }
    return g;
}

static float cui__bez(float a, float b, float t) {   /* ease between two ends with flat tangents */
    float s = t * t * (3 - 2 * t);
    return a + (b - a) * s;
}

typedef struct { cui__sk_geo g; const char *format; float nw; float intro; int hover_node, hover_link; float t; cui__tip tip; cui_color *node_c; } cui__sk_paint;

/* A node's color: the caller's, else its palette slot (outcomes), else a neutral gray. */
static cui_color cui__sk_node_color(cui_ctx *ui, const cui__sk_paint *p, const cui__sk_geo *g, int i) {
    if (p->node_c && cui__color_set(p->node_c[i])) return cui_color_of(ui, p->node_c[i]);
    return g->color[i] < 0 ? CUI_C(ui, TEXT_MUTED) : cui__viz_color(ui, (cui_color){ 0 }, g->color[i]);
}

/* The links on the hovered thing's path: everything upstream of where it starts and
 * downstream of where it ends. A link hover starts and ends at that link. */
static bool *cui__sk_path(cui_ctx *ui, const cui__sk_geo *g, int node, int link, bool *on_node) {
    bool *on = cui__frame_alloc(ui, sizeof(bool) * (size_t)CUI_MAX(1, g->nl));
    bool *up = cui__frame_alloc(ui, sizeof(bool) * (size_t)CUI_MAX(1, g->nn)), *down = cui__frame_alloc(ui, sizeof(bool) * (size_t)CUI_MAX(1, g->nn));
    memset(on, 0, sizeof(bool) * (size_t)CUI_MAX(1, g->nl));
    memset(up, 0, sizeof(bool) * (size_t)CUI_MAX(1, g->nn)); memset(down, 0, sizeof(bool) * (size_t)CUI_MAX(1, g->nn));
    if (node >= 0) up[node] = down[node] = true;
    else { up[g->links[link].from] = down[g->links[link].to] = true; on[link] = true; }
    for (int it = 0; it < g->nn; it++) {   /* bounded, so a cycle cannot hang */
        bool ch = false;
        for (int l = 0; l < g->nl; l++) {
            int a = g->links[l].from, b = g->links[l].to;
            if (up[b] && !up[a]) { up[a] = true; ch = true; }
            if (down[a] && !down[b]) { down[b] = true; ch = true; }
        }
        if (!ch) break;
    }
    for (int l = 0; l < g->nl; l++) if (up[g->links[l].to] || down[g->links[l].from]) on[l] = true;
    for (int i = 0; i < g->nn; i++) on_node[i] = up[i] || down[i];
    return on;
}

static void cui__paint_sankey(cui_ctx *ui, cui_node *n, void *data) {
    cui__sk_paint *p = data;
    cui_rect r = n->rect;
    float fs = cui__fs(ui, ui->theme.text_sm), fx = cui__fs(ui, ui->theme.text_xs);
    cui__sk_geo g = cui__sk_layout(ui, r, p->g.names, p->g.nn, p->g.links, p->g.nl, p->nw, p->format);
    bool any = (p->hover_node >= 0 || p->hover_link >= 0) && p->t > 0.01f;
    bool *node_on = cui__frame_alloc(ui, sizeof(bool) * (size_t)CUI_MAX(1, g.nn));
    bool *link_on = any ? cui__sk_path(ui, &g, p->hover_node, p->hover_link, node_on) : NULL;
    cui_color *nc = cui__frame_alloc(ui, sizeof(cui_color) * (size_t)CUI_MAX(1, g.nn));
    for (int i = 0; i < g.nn; i++) nc[i] = cui__sk_node_color(ui, p, &g, i);
    bool dark = ui->mode == CUI_THEME_DARK;
    const int seg = 28;
    cui_cvert *v = cui__frame_alloc(ui, sizeof(cui_cvert) * (size_t)(seg + 1) * 2);
    uint32_t *idx = cui__frame_alloc(ui, sizeof(uint32_t) * (size_t)seg * 6);
    for (int l = 0; l < g.nl; l++) {
        /* a flow fades from its source's color to its target's, faint at rest and strong on the hovered path */
        int a = g.links[l].from, b = g.links[l].to;
        float th = g.links[l].value * g.k;
        float x0 = g.node[a].x + g.node[a].w, x1 = g.node[b].x;
        float reach = cui__clampf(p->intro, 0, 1);
        float alpha = dark ? 0.2f : 0.18f;
        if (any) alpha = link_on[l] ? cui__lerp(alpha, dark ? 0.48f : 0.42f, p->t) : cui__lerp(alpha, 0.08f, p->t);
        for (int i = 0; i <= seg; i++) {
            float t = (float)i / seg * reach;
            float x = x0 + (x1 - x0) * t;
            float yt = cui__bez(g.sy[l], g.ty[l], t);
            cui_color c = cui_alpha(cui_mix(nc[a], nc[b], t), alpha);
            v[i * 2] = (cui_cvert){ x, yt, c };
            v[i * 2 + 1] = (cui_cvert){ x, yt + th, c };
        }
        for (int i = 0; i < seg; i++) {
            uint32_t q = (uint32_t)i * 2;
            uint32_t s6[6] = { q, q + 2, q + 3, q, q + 3, q + 1 };
            memcpy(idx + i * 6, s6, sizeof s6);
        }
        cui__draw_triangles(ui, v, (seg + 1) * 2, idx, seg * 6);
    }
    for (int i = 0; i < g.nn; i++) {
        if (g.in[i] <= 0 && g.out[i] <= 0) continue;
        cui_color col = nc[i];
        float la = 1;
        if (any && !node_on[i]) { col = cui_mix(col, CUI_C(ui, SURFACE), 0.5f * p->t); la = 1 - 0.45f * p->t; }
        cui_draw_rect(ui, g.node[i], CUI_CORNERS(CUI_MIN(3.0f, g.node[i].w * 0.5f)), col);
        /* the label beside the node: the name, then the value and share; no plate over the flows */
        float lx = g.node[i].x + g.node[i].w + 10, ly = g.node[i].y + g.node[i].h * 0.5f;
        const char *nm = g.names && g.names[i] ? g.names[i] : "";
        const char *val = cui__sk_value_text(ui, &g, i, p->format);
        float room = 1e9f;   /* up to the next column; a name that does not fit is cut with an ellipsis */
        if (g.col[i] < g.maxcol)
            for (int j = 0; j < g.nn; j++)
                if (g.col[j] == g.col[i] + 1 && (g.in[j] > 0 || g.out[j] > 0)) room = CUI_MIN(room, g.node[j].x - lx - 6);
        room = CUI_MAX(8.0f, room);
        float nh = cui__font_line_height(ui, CUI_FONT_MEDIUM, fs), vh = cui__font_line_height(ui, CUI_FONT_BODY, fx);
        float top = ly - (nh + vh) * 0.5f;
        cui__draw_text_ellipsis(ui, CUI_FONT_MEDIUM, fs, lx, top, room, cui_alpha(CUI_C(ui, FOREGROUND), la), nm, (int)strlen(nm), false);
        cui__draw_text_ellipsis(ui, CUI_FONT_BODY, fx, lx, top + nh, room, cui_alpha(CUI_C(ui, TEXT_MUTED), la), val, (int)strlen(val), true);
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_sankey_ex(cui_ctx *ui, const char *id_str, const char *const *nodes, int nn, const cui_sankey_link *links, int nl, const cui_sankey_opts *o) {
    cui_sankey_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    nn = CUI_MAX(0, nn); nl = links ? CUI_MAX(0, nl) : 0;
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    float nw = o->node_width > 0 ? o->node_width : 6;
    float h = o->height > 0 ? o->height : 300;
    int hn = -1, hl = -1;
    cui__sk_geo g = cui__sk_layout(ui, pr.w > 0 ? pr : (cui_rect){ 0, 0, 600, h }, nodes, nn, links ? links : (cui_sankey_link[1]){ { 0, 0, 0 } }, nl, nw, o->format);
    if (r.hovered && pr.w > 0) {
        for (int i = 0; i < g.nn; i++) {
            cui_rect q = g.node[i];
            if (ui->mx >= q.x - 4 && ui->mx < q.x + q.w + 4 && ui->my >= q.y - 2 && ui->my < q.y + q.h + 2) hn = i;
        }
        if (hn < 0)
            for (int l = 0; l < g.nl; l++) {
                int a = g.links[l].from, b = g.links[l].to;
                float x0 = g.node[a].x + g.node[a].w, x1 = g.node[b].x;
                if (x1 <= x0 || ui->mx < x0 || ui->mx > x1) continue;
                float t = (ui->mx - x0) / (x1 - x0);
                float yt = cui__bez(g.sy[l], g.ty[l], t), th = g.links[l].value * g.k;
                if (ui->my >= yt && ui->my <= yt + CUI_MAX(th, 3.0f)) hl = l;
            }
    }
    int hover = hn >= 0 ? hn : hl >= 0 ? nn + hl : -1;
    cui__viz_hover(ui, st, hover);
    cui__sk_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->g = g;   /* names and the filtered links; the paint lays out again in its own rect */
    p->format = o->format; p->nw = nw;
    if (o->node_colors && nn > 0) {
        p->node_c = cui__frame_alloc(ui, sizeof(cui_color) * (size_t)nn);
        memcpy(p->node_c, o->node_colors, sizeof(cui_color) * (size_t)nn);
    }
    p->intro = st->intro.x; p->t = st->hover_t;
    p->hover_node = p->hover_link = -1;
    if (st->hover_t > 0.01f) {
        if (st->shown < nn) p->hover_node = st->shown;
        else if (st->shown - nn < g.nl) p->hover_link = st->shown - nn;
    }
    p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
    if (p->hover_node >= 0) {
        int i = p->hover_node;
        p->tip.title = nodes && nodes[i] ? nodes[i] : "";
        if (g.in[i] > 0) cui__tip_row(&p->tip, "In", cui__fmt_value(ui, o->format, g.in[i]), cui__rgba(0, 0, 0, 0));
        if (g.out[i] > 0) cui__tip_row(&p->tip, "Out", cui__fmt_value(ui, o->format, g.out[i]), cui__rgba(0, 0, 0, 0));
    } else if (p->hover_link >= 0) {
        cui_sankey_link *L = &g.links[p->hover_link];
        p->tip.title = cui__frame_printf(ui, "%s \xe2\x86\x92 %s", nodes && nodes[L->from] ? nodes[L->from] : "", nodes && nodes[L->to] ? nodes[L->to] : "");
        cui__tip_row(&p->tip, "Flow", cui__fmt_value(ui, o->format, L->value), cui__sk_node_color(ui, p, &g, L->to));
        if (g.out[L->from] > 0) cui__tip_row(&p->tip, "Of the source", cui__pct(ui, L->value, g.out[L->from]), cui__rgba(0, 0, 0, 0));
    } else p->tip.title = NULL;
    cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_sankey, p, hover >= 0);
    if (hover >= 0) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return r.clicked ? hn : -1;
}

/* ------------------------------------------------------------------------ */
/* Waffle                                                                   */
/* ------------------------------------------------------------------------ */

typedef struct { cui_viz_item *items; int count; int *owner; int cols, rows; float cell, gap; float total; const char *format; float intro; int hover; cui__tip tip; float right; bool stack; } cui__wf_paint;

static void cui__paint_waffle(cui_ctx *ui, cui_node *n, void *data) {
    cui__wf_paint *p = data;
    cui_rect r = n->rect;
    float step = p->cell + p->gap;
    int cells = p->cols * p->rows;
    for (int c = 0; c < cells; c++) {
        int col = c / p->rows, row = c % p->rows;   /* fill column by column, bottom up */
        cui_rect q = { r.x + col * step, r.y + (p->rows - 1 - row) * step, p->cell, p->cell };
        int o = p->owner[c];
        float a = cui__clampf(p->intro * 2.0f - (float)c / (float)cells, 0, 1);
        cui_color fill = o >= 0 ? cui__viz_color(ui, p->items[o].color, o) : cui__viz_empty(ui);
        if (p->hover >= 0 && p->tip.alpha > 0.01f && o != p->hover) fill = cui_mix(fill, CUI_C(ui, SURFACE), 0.55f * p->tip.alpha);
        cui_draw_rect(ui, q, CUI_CORNERS(p->cell * 0.22f), cui_mix(cui__viz_empty(ui), fill, a));
    }
    /* legend on the right, or under the grid on a narrow screen */
    float fs = cui__fs(ui, ui->theme.text_sm);
    float lx = r.x + p->cols * step + 18, ly = r.y + 4;
    if (p->stack) { lx = r.x; ly = r.y + p->rows * step - p->gap + 16; }
    for (int k = 0; k < p->count; k++) {
        float a = (p->hover >= 0 && p->tip.alpha > 0.01f && k != p->hover) ? 0.5f : 1.0f;
        float cy = ly + k * 26 + 9;
        cui_draw_rect(ui, (cui_rect){ lx, cy - 5, 10, 10 }, CUI_CORNERS(3), cui_alpha(cui__viz_color(ui, p->items[k].color, k), a));
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, lx + 18, cy, CUI__AT_LEFT, cui_alpha(CUI_C(ui, TEXT_SECONDARY), a), p->items[k].label ? p->items[k].label : "", false);
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, p->stack ? r.x + CUI_MIN(r.w, p->right) : r.x + r.w, cy, CUI__AT_RIGHT, cui_alpha(CUI_C(ui, FOREGROUND), a), cui__pct(ui, p->items[k].value, p->total), true);
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_waffle_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *items_in, int count, const cui_waffle_opts *o) {
    cui_waffle_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    int cols = o->columns > 0 ? o->columns : 10, rows = o->rows > 0 ? o->rows : 10;
    cols = CUI_MIN(cols, 60); rows = CUI_MIN(rows, 60);
    int cells = cols * rows;
    float cell = o->cell > 0 ? o->cell : 18, gap = cell >= 12 ? 3 : 2, step = cell + gap;
    cui_viz_item *items = cui__copy_items(ui, items_in, count);
    float total = 0;
    for (int k = 0; k < count; k++) total += items[k].value;
    /* cells per item by largest remainder */
    int *owner = cui__frame_alloc(ui, sizeof(int) * (size_t)cells);
    int *nc = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, count));
    float *rem = cui__copy_floats(ui, NULL, count);
    int used = 0;
    for (int k = 0; k < count; k++) {
        float exact = total > 0 ? items[k].value / total * cells : 0;
        nc[k] = (int)floorf(exact); rem[k] = exact - nc[k]; used += nc[k];
    }
    while (total > 0 && used < cells) {
        int best = 0;
        for (int k = 1; k < count; k++) if (rem[k] > rem[best]) best = k;
        nc[best]++; rem[best] = -1; used++;
    }
    int c = 0;
    for (int k = 0; k < count; k++) for (int j = 0; j < nc[k] && c < cells; j++) owner[c++] = k;
    for (; c < cells; c++) owner[c] = -1;
    float fs = cui__fs(ui, ui->theme.text_sm), lw = 0;
    for (int k = 0; k < count; k++) lw = CUI_MAX(lw, cui__text_width(ui, CUI_FONT_BODY, fs, items[k].label ? items[k].label : "", -1, false));
    float gw = cols * step - gap;
    float legend_w = 18 + 18 + lw + 16 + 44;
    /* the legend drops under the grid when last frame's width had no room beside it */
    float under_w = CUI_MAX(gw, legend_w - 18);
    bool stack = pr.w > 0 && pr.w < gw + legend_w - 0.5f;
    float gh = rows * step - gap;
    int hover = -1;
    if (r.hovered) {
        int cx = (int)floorf((ui->mx - pr.x) / step), cy = (int)floorf((ui->my - pr.y) / step);
        if (cx >= 0 && cx < cols && cy >= 0 && cy < rows) hover = owner[cx * rows + (rows - 1 - cy)];
        else if (stack && ui->my > pr.y + gh + 12) {
            int k = (int)floorf((ui->my - pr.y - gh - 16) / 26);
            if (k >= 0 && k < count) hover = k;
        } else if (!stack && ui->mx > pr.x + gw + 10) {
            int k = (int)floorf((ui->my - pr.y - 4 + 4) / 26);
            if (k >= 0 && k < count) hover = k;
        }
    }
    cui__viz_hover(ui, st, hover);
    cui__wf_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->items = items; p->count = count; p->owner = owner; p->cols = cols; p->rows = rows; p->cell = cell; p->gap = gap; p->total = total; p->format = o->format;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < count ? st->shown : -1;
    if (p->hover >= 0) {
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = items[p->hover].label ? items[p->hover].label : "";
        cui__tip_row(&p->tip, "Value", cui__fmt_value(ui, o->format, items[p->hover].value), cui__viz_color(ui, items[p->hover].color, p->hover));
        cui__tip_row(&p->tip, "Share", cui__pct(ui, items[p->hover].value, total), cui__rgba(0, 0, 0, 0));
    }
    p->stack = stack; p->right = under_w;
    float h = stack ? gh + 16 + count * 26.0f : CUI_MAX(gh, count * 26.0f);
    int node = cui__viz_node(ui, id, CUI_FIT, CUI_PX(h), cui__paint_waffle, p, hover >= 0);
    cui__soft_width(&ui->nodes[node], CUI_FIT, gw + legend_w);
    ui->nodes[node].flags |= CUI_NF_ADAPTIVE;
    ui->nodes[node].min_w = under_w;
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Split bar                                                                */
/* ------------------------------------------------------------------------ */

typedef struct { cui_viz_item *items; int count; float total; cui_split_bar_opts o; float bh; float intro; int hover; cui__tip tip; } cui__sb_paint;

static void cui__sb_segments(cui_rect r, const cui_viz_item *items, int count, float total, float bh, float e, cui_rect *out) {
    int nz = 0;
    for (int k = 0; k < count; k++) if (items[k].value > 0) nz++;
    float gap = 2, avail = r.w - gap * (float)CUI_MAX(0, nz - 1);
    float x = r.x;
    for (int k = 0; k < count; k++) {
        float w = total > 0 ? items[k].value / total * avail * e : 0;
        out[k] = (cui_rect){ x, r.y, w, bh };
        if (items[k].value > 0) x += w + gap * e;
    }
}

static void cui__paint_split_bar(cui_ctx *ui, cui_node *n, void *data) {
    cui__sb_paint *p = data;
    cui_rect r = n->rect;
    cui_rect *seg = cui__frame_alloc(ui, sizeof(cui_rect) * (size_t)CUI_MAX(1, p->count));
    float e = cui__clampf(p->intro, 0, 1);
    cui_draw_rect(ui, (cui_rect){ r.x, r.y, r.w, p->bh }, CUI_CORNERS(p->bh * 0.5f), cui__viz_empty(ui));
    cui__sb_segments(r, p->items, p->count, p->total, p->bh, e, seg);
    int first = -1, last = -1;
    for (int k = 0; k < p->count; k++) if (p->items[k].value > 0) { if (first < 0) first = k; last = k; }
    for (int k = 0; k < p->count; k++) {
        if (p->items[k].value <= 0) continue;
        float rr = CUI_MIN(p->bh * 0.5f, 3);
        float re = p->bh * 0.5f;
        cui_corners c = { k == first ? re : rr, k == last && e > 0.99f ? re : rr, k == last && e > 0.99f ? re : rr, k == first ? re : rr };
        cui_color col = cui__viz_color(ui, p->items[k].color, k);
        if (p->hover >= 0 && p->tip.alpha > 0.01f && k != p->hover) col = cui_mix(col, CUI_C(ui, SURFACE), 0.5f * p->tip.alpha);
        cui_draw_rect(ui, seg[k], c, col);
    }
    if (!p->o.no_legend) {
        float fs = cui__fs(ui, ui->theme.text_sm), lh = 20;
        float fh = cui__font_line_height(ui, CUI_FONT_BODY, fs);
        float x = r.x, y = r.y + p->bh + 14;
        for (int k = 0; k < p->count; k++) {
            const char *lb = p->items[k].label ? p->items[k].label : "";
            const char *vs = cui__frame_printf(ui, "%s  %s", cui__fmt_value(ui, p->o.format, p->items[k].value), cui__pct(ui, p->items[k].value, p->total));
            float w = 14 + cui__text_width(ui, CUI_FONT_MEDIUM, fs, lb, -1, false) + 8 + cui__text_width(ui, CUI_FONT_BODY, fs, vs, -1, true);
            if (x > r.x && x + w > r.x + r.w) { x = r.x; y += lh + 6; }
            float a = (p->hover >= 0 && p->tip.alpha > 0.01f && k != p->hover) ? 0.5f : 1.0f;
            cui_draw_rect(ui, (cui_rect){ x, y + lh * 0.5f - 4, 8, 8 }, CUI_CORNERS(2.5f), cui_alpha(cui__viz_color(ui, p->items[k].color, k), a));
            float lw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, lb, -1, false);
            cui__draw_text_line(ui, CUI_FONT_MEDIUM, fs, x + 14, y + (lh - fh) * 0.5f, cui_alpha(CUI_C(ui, FOREGROUND), a), lb, -1, false);
            cui__draw_text_line(ui, CUI_FONT_BODY, fs, x + 14 + lw + 8, y + (lh - fh) * 0.5f, cui_alpha(CUI_C(ui, TEXT_MUTED), a), vs, -1, true);
            x += w + 22;
        }
    }
    cui__tip_draw(ui, &p->tip);
}

static float cui__sb_legend_h(cui_ctx *ui, const cui_viz_item *items, int count, float total, const char *fmt, float width) {
    float fs = cui__fs(ui, ui->theme.text_sm), lh = 20, x = 0, y = 0;
    for (int k = 0; k < count; k++) {
        const char *lb = items[k].label ? items[k].label : "";
        const char *vs = cui__frame_printf(ui, "%s  %s", cui__fmt_value(ui, fmt, items[k].value), cui__pct(ui, items[k].value, total));
        float w = 14 + cui__text_width(ui, CUI_FONT_MEDIUM, fs, lb, -1, false) + 8 + cui__text_width(ui, CUI_FONT_BODY, fs, vs, -1, true);
        if (x > 0 && x + w > width) { x = 0; y += lh + 6; }
        x += w + 22;
    }
    return count ? y + lh : 0;
}

int cui_split_bar_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *items_in, int count, const cui_split_bar_opts *o) {
    cui_split_bar_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui_viz_item *items = cui__copy_items(ui, items_in, count);
    float total = 0;
    for (int k = 0; k < count; k++) total += items[k].value;
    float bh = o->height > 0 ? o->height : 10;
    int hover = -1;
    if (r.hovered && pr.w > 0) {
        if (ui->my < pr.y + bh + 6) {
            cui_rect *seg = cui__frame_alloc(ui, sizeof(cui_rect) * (size_t)CUI_MAX(1, count));
            cui__sb_segments(pr, items, count, total, bh, 1, seg);
            for (int k = 0; k < count; k++) if (items[k].value > 0 && ui->mx >= seg[k].x - 1 && ui->mx < seg[k].x + seg[k].w + 1) hover = k;
        } else if (!o->no_legend) {
            float fs = cui__fs(ui, ui->theme.text_sm), lh = 20, x = pr.x, y = pr.y + bh + 14;
            for (int k = 0; k < count; k++) {
                const char *lb = items[k].label ? items[k].label : "";
                const char *vs = cui__frame_printf(ui, "%s  %s", cui__fmt_value(ui, o->format, items[k].value), cui__pct(ui, items[k].value, total));
                float w = 14 + cui__text_width(ui, CUI_FONT_MEDIUM, fs, lb, -1, false) + 8 + cui__text_width(ui, CUI_FONT_BODY, fs, vs, -1, true);
                if (x > pr.x && x + w > pr.x + pr.w) { x = pr.x; y += lh + 6; }
                if (ui->mx >= x && ui->mx < x + w && ui->my >= y && ui->my < y + lh) hover = k;
                x += w + 22;
            }
        }
    }
    cui__viz_hover(ui, st, hover);
    cui__sb_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->items = items; p->count = count; p->total = total; p->o = *o; p->bh = bh;
    p->intro = st->intro.x;
    p->hover = st->hover_t > 0.01f && st->shown < count ? st->shown : -1;
    if (p->hover >= 0) {
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = items[p->hover].label ? items[p->hover].label : "";
        cui__tip_row(&p->tip, "Value", cui__fmt_value(ui, o->format, items[p->hover].value), cui__viz_color(ui, items[p->hover].color, p->hover));
        cui__tip_row(&p->tip, "Share", cui__pct(ui, items[p->hover].value, total), cui__rgba(0, 0, 0, 0));
    }
    float width = pr.w > 0 ? pr.w : 400;
    float h = bh + (o->no_legend ? 0 : 14 + cui__sb_legend_h(ui, items, count, total, o->format, width));
    cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_split_bar, p, hover >= 0);
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Bar list                                                                 */
/* ------------------------------------------------------------------------ */

#define CUI__BL_ROW 34.0f
#define CUI__BL_GAP 6.0f

typedef struct { cui_spring y, w; float hover; bool init; } cui__bl_item_state;
typedef struct { const char *label; const char *value; float y, w, hover; cui_color c; } cui__bl_row;
typedef struct { cui__bl_row *rows; int n; float vw; } cui__bl_paint;

static void cui__paint_bar_list(cui_ctx *ui, cui_node *n, void *data) {
    cui__bl_paint *p = data;
    cui_rect r = n->rect;
    float fs = cui__fs(ui, ui->theme.text_sm);
    float bw = r.w - p->vw - 16;
    cui_push_clip(ui, r);
    for (int k = 0; k < p->n; k++) {
        cui__bl_row *row = &p->rows[k];
        float y = r.y + row->y;
        cui_rect bar = { r.x, y, CUI_MAX(row->w * bw, 4.0f), CUI__BL_ROW };
        cui_color fill = cui_alpha(row->c, (ui->mode == CUI_THEME_DARK ? 0.26f : 0.16f) + 0.1f * row->hover);
        if (row->hover > 0.01f) cui_draw_rect(ui, (cui_rect){ r.x - 6, y - 2, r.w + 12, CUI__BL_ROW + 4 }, CUI_CORNERS(10), cui_alpha(CUI_C(ui, HOVER), row->hover));
        cui_draw_rect(ui, bar, CUI_CORNERS(8), fill);
        cui_push_clip(ui, (cui_rect){ r.x, y, bw, CUI__BL_ROW });
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, r.x + 12, y + CUI__BL_ROW * 0.5f, CUI__AT_LEFT, CUI_C(ui, FOREGROUND), row->label, false);
        cui_pop_clip(ui);
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, r.x + r.w, y + CUI__BL_ROW * 0.5f, CUI__AT_RIGHT, CUI_C(ui, TEXT_SECONDARY), row->value, true);
    }
    cui_pop_clip(ui);
}

int cui_bar_list_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *items_in, int count, const cui_bar_list_opts *o) {
    cui_bar_list_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui_viz_item *items = cui__copy_items(ui, items_in, count);
    int *order = cui__frame_alloc(ui, sizeof(int) * (size_t)CUI_MAX(1, count));
    float maxv = 0;
    for (int k = 0; k < count; k++) { order[k] = k; maxv = CUI_MAX(maxv, items[k].value); }
    if (!o->keep_order)
        for (int a = 1; a < count; a++) {
            int v = order[a], b = a - 1;
            while (b >= 0 && items[order[b]].value < items[v].value) { order[b + 1] = order[b]; b--; }
            order[b + 1] = v;
        }
    int shown = o->max_rows > 0 ? CUI_MIN(o->max_rows, count) : count;
    int hover_rank = -1;
    if (r.hovered) {
        int k = (int)floorf((ui->my - pr.y + CUI__BL_GAP * 0.5f) / (CUI__BL_ROW + CUI__BL_GAP));
        if (k >= 0 && k < shown) hover_rank = k;
    }
    cui__bl_paint *p = cui__frame_alloc(ui, sizeof *p);
    p->rows = cui__frame_alloc(ui, sizeof(cui__bl_row) * (size_t)CUI_MAX(1, count));
    p->n = 0;
    p->vw = 0;
    float fs = cui__fs(ui, ui->theme.text_sm);
    for (int rank = 0; rank < count; rank++) {
        int k = order[rank];
        const char *lb = items[k].label ? items[k].label : "";
        cui_id iid = cui__hash_str(lb, id ^ 0xB1u);
        cui__bl_item_state *is = cui_state(ui, iid, sizeof *is);
        float ty = rank * (CUI__BL_ROW + CUI__BL_GAP), tw = maxv > 0 ? items[k].value / maxv : 0;
        if (rank >= shown) ty = shown * (CUI__BL_ROW + CUI__BL_GAP);   /* slides out below */
        if (!is->init) { is->y.x = ty; is->w.x = 0; is->init = true; }
        cui__spring(ui, &is->y, ty, CUI_SPRING_SMOOTH);
        cui__spring(ui, &is->w, tw, 0.6f, 0.0f);
        cui__tween(ui, &is->hover, rank == hover_rank ? 1.0f : 0.0f, CUI__T_HOVER);
        if (rank >= shown && fabsf(is->y.x - ty) < 0.5f) continue;   /* gone */
        cui__bl_row *row = &p->rows[p->n++];
        row->label = lb;
        row->value = cui__fmt_value(ui, o->format, items[k].value);
        row->y = is->y.x; row->w = cui__clampf(is->w.x, 0, 1.05f); row->hover = is->hover;
        row->c = cui__color_set(o->color) ? cui_color_of(ui, o->color) : cui__color_set(items[k].color) ? cui_color_of(ui, items[k].color) : cui__viz_base(ui, (cui_color){ 0 });
        p->vw = CUI_MAX(p->vw, cui__text_width(ui, CUI_FONT_MEDIUM, fs, row->value, -1, true));
    }
    float h = shown * (CUI__BL_ROW + CUI__BL_GAP) - (shown ? CUI__BL_GAP : 0);
    int n = cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_bar_list, p, false);
    ui->nodes[n].flags &= ~(uint32_t)CUI_NF_PAINT_AFTER;
    if (hover_rank >= 0) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return r.clicked && hover_rank >= 0 ? order[hover_rank] : -1;
}

/* ------------------------------------------------------------------------ */
/* Activity rings                                                           */
/* ------------------------------------------------------------------------ */

typedef struct { cui_viz_item *items; int count; cui_rings_opts o; float size, th; float *prog; int hover; cui__tip tip; float label_w; } cui__rings_paint;

#define CUI__RING_LEGEND_GAP 28
#define CUI__RING_ROW 26

static void cui__paint_rings(cui_ctx *ui, cui_node *n, void *data) {
    cui__rings_paint *p = data;
    cui_rect r = n->rect;
    cui_vec2 c = { r.x + (p->o.legend ? p->size * 0.5f : r.w * 0.5f), r.y + p->size * 0.5f };
    float a0 = -CUI_PI * 0.5f;
    for (int k = 0; k < p->count; k++) {
        float rad = p->size * 0.5f - p->th * 0.5f - k * (p->th + 3);
        if (rad <= p->th * 0.5f) break;
        cui_color col = cui__viz_color(ui, p->items[k].color, k);
        bool dim = p->hover >= 0 && p->tip.alpha > 0.01f && k != p->hover;
        if (dim) col = cui_mix(col, CUI_C(ui, SURFACE), 0.4f * p->tip.alpha);
        cui_draw_arc(ui, c, rad, 0, 2 * CUI_PI, p->th, cui_alpha(col, ui->mode == CUI_THEME_DARK ? 0.22f : 0.16f));
        float v = CUI_MAX(0.0f, p->prog[k]);
        if (v <= 0.001f) continue;
        float a1 = a0 + v * 2 * CUI_PI;
        if (v < 1) {
            cui_draw_arc(ui, c, rad, a0, a1, p->th, col);
            cui_draw_circle(ui, (cui_vec2){ c.x + cosf(a0) * rad, c.y + sinf(a0) * rad }, p->th * 0.5f, col);
        } else {
            cui_draw_arc(ui, c, rad, 0, 2 * CUI_PI, p->th, col);
            /* the tip overlaps the start: a soft shadow cast ahead of the cap tells them apart */
            float sa = a1 + p->th * 0.25f / rad;
            float sr = p->th * 0.5f;
            cui_vec2 sc = { c.x + cosf(sa) * rad, c.y + sinf(sa) * rad };
            cui_push_clip(ui, (cui_rect){ c.x - rad - p->th, c.y - rad - p->th, (rad + p->th) * 2, (rad + p->th) * 2 });
            cui_draw_shadow(ui, (cui_rect){ sc.x - sr, sc.y - sr, sr * 2, sr * 2 }, CUI_CORNERS(sr), p->th * 0.3f, 0, (cui_vec2){ 0, 0 },
                            cui__rgba(0, 0, 0, ui->mode == CUI_THEME_DARK ? 150 : 70));
            cui_pop_clip(ui);
            /* the last stretch of the arc sits on top of the shadow */
            cui_draw_arc(ui, c, rad, a1 - 0.6f, a1, p->th, col);
        }
        cui_vec2 tip = { c.x + cosf(a1) * rad, c.y + sinf(a1) * rad };
        cui_draw_circle(ui, tip, p->th * 0.5f, col);
    }
    if (p->o.center) {
        float fd = cui__fs(ui, p->size * 0.16f);
        cui__draw_text_at(ui, CUI_FONT_DISPLAY, fd, c.x, c.y - (p->o.caption ? 7 : 0), CUI__AT_CENTER, CUI_C(ui, FOREGROUND), p->o.center, true);
        if (p->o.caption) cui__draw_text_at(ui, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_xs), c.x, c.y + fd * 0.55f, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED), p->o.caption, false);
    }
    if (p->o.legend) {   /* outer ring first, as the rings read from the outside in */
        float fs = cui__fs(ui, ui->theme.text_sm);
        float lx = r.x + p->size + CUI__RING_LEGEND_GAP, ly = c.y - p->count * CUI__RING_ROW * 0.5f + CUI__RING_ROW * 0.5f;
        for (int k = 0; k < p->count; k++) {
            bool dim = p->hover >= 0 && p->tip.alpha > 0.01f && k != p->hover;
            cui_color col = cui__viz_color(ui, p->items[k].color, k);
            float y = ly + k * CUI__RING_ROW;
            cui_draw_rect(ui, (cui_rect){ lx, y - 4, 8, 8 }, CUI_CORNERS(2.5f), dim ? cui_mix(col, CUI_C(ui, SURFACE), 0.4f * p->tip.alpha) : col);
            cui__draw_text_at(ui, CUI_FONT_BODY, fs, lx + 16, y, CUI__AT_LEFT, CUI_C(ui, TEXT_SECONDARY), p->items[k].label ? p->items[k].label : "", false);
            cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, lx + 16 + p->label_w + 14, y, CUI__AT_LEFT, CUI_C(ui, FOREGROUND), cui__pct(ui, p->items[k].value, 1), true);
        }
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_activity_rings_ex(cui_ctx *ui, const char *id_str, const cui_viz_item *rings, int count, const cui_rings_opts *o) {
    cui_rings_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_CLAMP(count, 0, 8);
    cui_id id = cui_make_id(ui, id_str);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    cui__viz_state *st = cui_state(ui, id, sizeof *st + sizeof(cui_spring) * 8);
    cui_spring *sp = (cui_spring *)(st + 1);
    float size = o->size > 0 ? o->size : 150;
    cui__rings_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    float lw = 0;   /* the legend beside the rings */
    if (o->legend) {
        float fs = cui__fs(ui, ui->theme.text_sm), vw = 0;
        for (int k = 0; k < count; k++) {
            p->label_w = CUI_MAX(p->label_w, cui__text_width(ui, CUI_FONT_BODY, fs, rings[k].label ? rings[k].label : "", -1, false));
            vw = CUI_MAX(vw, cui__text_width(ui, CUI_FONT_MEDIUM, fs, cui__pct(ui, rings[k].value, 1), -1, true));
        }
        lw = CUI__RING_LEGEND_GAP + 16 + p->label_w + 14 + vw;
    }
    /* narrower than asked: the rings shrink, the legend keeps its width */
    float full = size, min_size = CUI_MIN(size, 96.0f);
    if (pr.w > 0 && pr.w < full + lw) size = CUI_CLAMP(floorf(pr.w - lw), min_size, full);
    float th = o->thickness > 0 ? o->thickness : CUI_MAX(8.0f, size * 0.1f);
    p->prog = cui__copy_floats(ui, NULL, count);
    for (int k = 0; k < count; k++) {
        float v = rings[k].value >= 0 ? CUI_MIN(rings[k].value, 4.0f) : 0;
        cui__spring(ui, &sp[k], v, 0.9f + 0.1f * k, 0.0f);
        p->prog[k] = sp[k].x;
    }
    int hover = -1;
    if (r.hovered) {
        float dx = ui->mx - (pr.x + (o->legend ? size : pr.w) * 0.5f), dy = ui->my - (pr.y + size * 0.5f);
        float dist = sqrtf(dx * dx + dy * dy);
        for (int k = 0; k < count; k++) {
            float rad = size * 0.5f - th * 0.5f - k * (th + 3);
            if (fabsf(dist - rad) <= th * 0.5f + 1.5f) hover = k;
        }
    }
    if (o->legend && r.hovered && hover < 0) {   /* a legend row picks out its ring */
        float lx = pr.x + size + CUI__RING_LEGEND_GAP, ly = pr.y + size * 0.5f - count * CUI__RING_ROW * 0.5f;
        if (ui->mx >= lx && ui->my >= ly && ui->my < ly + count * CUI__RING_ROW) hover = CUI_CLAMP((int)((ui->my - ly) / CUI__RING_ROW), 0, count - 1);
    }
    cui__viz_hover(ui, st, hover);
    p->items = cui__copy_items(ui, rings, count); p->count = count; p->o = *o; p->size = size; p->th = th;
    p->hover = st->hover_t > 0.01f && st->shown < count ? st->shown : -1;
    if (p->hover >= 0) {
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = rings[p->hover].label ? rings[p->hover].label : "";
        cui__tip_row(&p->tip, "Progress", cui__pct(ui, rings[p->hover].value, 1), cui__viz_color(ui, rings[p->hover].color, p->hover));
    }
    int node = cui__viz_node(ui, id, CUI_FIT, CUI_PX(size), cui__paint_rings, p, hover >= 0);
    cui__soft_width(&ui->nodes[node], CUI_FIT, full + lw);
    ui->nodes[node].min_w = min_size + lw;
    ui->nodes[node].flags |= CUI_NF_ADAPTIVE;
    return r.clicked ? hover : -1;
}

/* ------------------------------------------------------------------------ */
/* Scatter                                                                  */
/* ------------------------------------------------------------------------ */

typedef struct {
    float x0, x1, xs, y0, y1, ys, smax;
    cui_rect plot;
} cui__sc_geo;

static cui__sc_geo cui__sc_layout(cui_ctx *ui, cui_rect r, const cui_scatter_series *s, int count, const cui_scatter_opts *o, float legend_h) {
    cui__sc_geo g;
    float xl = 1e30f, xh = -1e30f, yl = 1e30f, yh = -1e30f;
    g.smax = 0;
    for (int k = 0; k < count; k++)
        for (int i = 0; i < s[k].count; i++) {
            xl = CUI_MIN(xl, s[k].points[i].x); xh = CUI_MAX(xh, s[k].points[i].x);
            yl = CUI_MIN(yl, s[k].points[i].y); yh = CUI_MAX(yh, s[k].points[i].y);
            if (s[k].sizes) g.smax = CUI_MAX(g.smax, s[k].sizes[i]);
        }
    if (xl > xh) { xl = 0; xh = 1; }
    if (yl > yh) { yl = 0; yh = 1; }
    /* the axes end on the first nice value past the data; padding is added only when a point would sit on the frame */
    float px = (xh - xl) * 0.015f, py = (yh - yl) * 0.03f;
    cui__nice_range(xl, xh, 5, &g.x0, &g.x1, &g.xs);
    if (g.x1 - xh < px || (xl - g.x0 < px && g.x0 != 0)) cui__nice_range(xl - (xl - g.x0 < px && g.x0 != 0 ? px : 0), xh + (g.x1 - xh < px ? px : 0), 5, &g.x0, &g.x1, &g.xs);
    cui__nice_range(yl, yh, 4, &g.y0, &g.y1, &g.ys);
    if (g.y1 - yh < py || (yl - g.y0 < py && g.y0 != 0)) cui__nice_range(yl - (yl - g.y0 < py && g.y0 != 0 ? py : 0), yh + (g.y1 - yh < py ? py : 0), 4, &g.y0, &g.y1, &g.ys);
    float fs = cui__fs(ui, ui->theme.text_xs), aw = 0;
    for (float v = g.y0; v <= g.y1 + g.ys * 0.5f; v += g.ys) aw = CUI_MAX(aw, cui__text_width(ui, CUI_FONT_BODY, fs, cui__fmt_value(ui, o->y_format, v), -1, true));
    float top = legend_h + (o->y_label ? 22 : 6);
    g.plot = (cui_rect){ r.x + aw + 10, r.y + top, r.w - aw - 14, r.h - top - (o->x_label ? 46 : 26) };
    return g;
}

static cui_vec2 cui__sc_pt(const cui__sc_geo *g, cui_vec2 v) {
    return (cui_vec2){ g->plot.x + (v.x - g->x0) / (g->x1 - g->x0) * g->plot.w, g->plot.y + g->plot.h - (v.y - g->y0) / (g->y1 - g->y0) * g->plot.h };
}

static float cui__sc_radius(const cui__sc_geo *g, const cui_scatter_series *s, int i) {
    if (!s->sizes || g->smax <= 0) return 4.5f;
    return 3.5f + sqrtf(CUI_MAX(0.0f, s->sizes[i]) / g->smax) * 11;
}

typedef struct { cui_scatter_series *s; int count; cui_scatter_opts o; float legend_h; float intro; int hs, hi; cui__tip tip; } cui__sc_paint;

static void cui__paint_scatter(cui_ctx *ui, cui_node *n, void *data) {
    cui__sc_paint *p = data;
    cui_rect r = n->rect;
    cui__sc_geo g = cui__sc_layout(ui, r, p->s, p->count, &p->o, p->legend_h);
    float fs = cui__fs(ui, ui->theme.text_xs);
    cui_rect pl = g.plot;
    for (float v = g.y0; v <= g.y1 + g.ys * 0.5f; v += g.ys) {
        float y = pl.y + pl.h - (v - g.y0) / (g.y1 - g.y0) * pl.h;
        cui_draw_rect(ui, (cui_rect){ pl.x, y - 0.5f, pl.w, 1 }, CUI_CORNERS(0), fabsf(v - g.y0) < g.ys * 0.01f ? CUI_C(ui, BORDER) : CUI_C(ui, BORDER_SUBTLE));
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, pl.x - 8, y, CUI__AT_RIGHT, CUI_C(ui, TEXT_MUTED), cui__fmt_value(ui, p->o.y_format, v), true);
    }
    for (float v = g.x0; v <= g.x1 + g.xs * 0.5f; v += g.xs) {
        float x = pl.x + (v - g.x0) / (g.x1 - g.x0) * pl.w;
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, x, pl.y + pl.h + 14, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED), cui__fmt_value(ui, p->o.x_format, v), true);
    }
    if (p->o.x_label) cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, pl.x + pl.w * 0.5f, pl.y + pl.h + 36, CUI__AT_CENTER, CUI_C(ui, TEXT_SECONDARY), p->o.x_label, false);
    if (p->o.y_label) cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, r.x, pl.y - 14, CUI__AT_LEFT, CUI_C(ui, TEXT_SECONDARY), p->o.y_label, false);
    if (p->hs >= 0 && p->tip.alpha > 0.01f) {
        cui_vec2 q = cui__sc_pt(&g, p->s[p->hs].points[p->hi]);
        cui_color gl = cui_alpha(CUI_C(ui, BORDER_STRONG), p->tip.alpha);
        cui_draw_rect(ui, (cui_rect){ q.x - 0.5f, q.y, 1, pl.y + pl.h - q.y }, CUI_CORNERS(0), gl);
        cui_draw_rect(ui, (cui_rect){ pl.x, q.y - 0.5f, q.x - pl.x, 1 }, CUI_CORNERS(0), gl);
    }
    cui_push_clip(ui, (cui_rect){ pl.x - 30, pl.y - 30, pl.w + 60, pl.h + 30 });
    for (int k = 0; k < p->count; k++) {
        cui_scatter_series *s = &p->s[k];
        cui_color col = cui__viz_color(ui, s->color, k);
        bool dim = p->hs >= 0 && p->tip.alpha > 0.01f && p->hs != k;
        for (int i = 0; i < s->count; i++) {
            cui_vec2 q = cui__sc_pt(&g, s->points[i]);
            float rr = cui__sc_radius(&g, s, i);
            float e = cui__clampf(p->intro * 1.6f - (q.x - pl.x) / CUI_MAX(1.0f, pl.w) * 0.6f, 0, 1);
            rr *= e;
            if (rr <= 0.2f) continue;
            bool h = k == p->hs && i == p->hi && p->tip.alpha > 0.01f;
            if (h) rr += 1.5f;
            cui_color fill = s->sizes ? cui_alpha(col, 0.42f) : col;
            if (dim) fill = cui_alpha(fill, 0.45f);
            cui_draw_circle(ui, q, rr + 1.5f, CUI_C(ui, SURFACE));
            cui_draw_circle(ui, q, rr, fill);
            if (s->sizes) cui_draw_arc(ui, q, rr - 0.5f, 0, 2 * CUI_PI, 1, cui_alpha(col, dim ? 0.5f : 1));
        }
    }
    cui_pop_clip(ui);
    if (p->count >= 2 && !p->o.no_legend) {
        const char **names = cui__frame_alloc(ui, sizeof(char *) * (size_t)p->count);
        cui_color *cols = cui__frame_alloc(ui, sizeof(cui_color) * (size_t)p->count);
        for (int k = 0; k < p->count; k++) { names[k] = p->s[k].name; cols[k] = cui__viz_color(ui, p->s[k].color, k); }
        cui__viz_legend(ui, r.x, r.y, r.w, names, cols, p->count, p->tip.alpha > 0.01f ? p->hs : -1, true);
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_scatter_chart_ex(cui_ctx *ui, const char *id_str, const cui_scatter_series *series, int count, const cui_scatter_opts *o) {
    cui_scatter_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str);
    cui__viz_state *st = cui__viz_state_get(ui, id);
    cui_interact r = cui__interact(ui, id, 0);
    cui_rect pr = cui__prev_rect(ui, id);
    float h = o->height > 0 ? o->height : 280;
    float legend_h = 0;
    if (count >= 2 && !o->no_legend) {
        const char **names = cui__frame_alloc(ui, sizeof(char *) * (size_t)count);
        cui_color *cols = cui__frame_alloc(ui, sizeof(cui_color) * (size_t)count);
        for (int k = 0; k < count; k++) { names[k] = series[k].name; cols[k] = cui__viz_color(ui, series[k].color, k); }
        legend_h = cui__viz_legend(ui, 0, 0, pr.w > 0 ? pr.w : 500, names, cols, count, -1, false) + 12;
    }
    int hs = -1, hi = -1;
    if (r.hovered && pr.w > 0) {
        cui__sc_geo g = cui__sc_layout(ui, pr, series, count, o, legend_h);
        float best = 24 * 24;
        for (int k = 0; k < count; k++)
            for (int i = 0; i < series[k].count; i++) {
                cui_vec2 q = cui__sc_pt(&g, series[k].points[i]);
                float rr = cui__sc_radius(&g, &series[k], i);
                float dx = ui->mx - q.x, dy = ui->my - q.y, d2 = dx * dx + dy * dy - rr * rr * 0.6f;
                if (d2 < best) { best = d2; hs = k; hi = i; }
            }
    }
    int flat = -1;
    if (hs >= 0) { flat = hi; for (int k = 0; k < hs; k++) flat += series[k].count; }
    cui__viz_hover(ui, st, flat);
    cui__sc_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->s = cui__frame_alloc(ui, sizeof(cui_scatter_series) * (size_t)CUI_MAX(1, count));
    for (int k = 0; k < count; k++) {
        p->s[k] = series[k];
        cui_vec2 *pts = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)CUI_MAX(1, series[k].count));
        if (series[k].count > 0) memcpy(pts, series[k].points, sizeof(cui_vec2) * (size_t)series[k].count);
        p->s[k].points = pts;
        if (series[k].sizes) p->s[k].sizes = cui__copy_floats(ui, series[k].sizes, series[k].count);
    }
    p->count = count; p->o = *o; p->legend_h = legend_h; p->intro = st->intro.x;
    p->hs = p->hi = -1;
    if (st->hover_t > 0.01f && st->shown >= 0) {
        int f = st->shown;
        for (int k = 0; k < count; k++) { if (f < series[k].count) { p->hs = k; p->hi = f; break; } f -= series[k].count; }
    }
    if (p->hs >= 0) {
        const cui_scatter_series *s = &series[p->hs];
        p->tip.alpha = st->hover_t; p->tip.x = ui->mx; p->tip.y = ui->my; p->tip.bounds = pr;
        p->tip.title = s->name ? s->name : "";
        cui__tip_row(&p->tip, o->x_label ? o->x_label : "x", cui__fmt_value(ui, o->x_format, s->points[p->hi].x), cui__viz_color(ui, s->color, p->hs));
        cui__tip_row(&p->tip, o->y_label ? o->y_label : "y", cui__fmt_value(ui, o->y_format, s->points[p->hi].y), cui__rgba(0, 0, 0, 0));
        if (s->sizes) cui__tip_row(&p->tip, "Size", cui__fmt_value(ui, NULL, s->sizes[p->hi]), cui__rgba(0, 0, 0, 0));
    }
    cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_scatter, p, flat >= 0);
    if (flat >= 0) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return r.clicked ? flat : -1;
}

/* ------------------------------------------------------------------------ */
/* Radial gauge                                                             */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring fill; bool init; } cui__rg_state;
typedef struct {
    float t, lo, hi, a0, sweep, R, target_t;
    int nband;
    cui_gauge_style style;
    bool ends, target;
    const char *value, *unit, *label, *detail, *status, *format;
    cui_gauge_band *bands;
    cui_color fill, status_c;
} cui__rg_paint;

/* the color of the band a fraction of the range falls in, or `none` past the last band */
static cui_color cui__rg_band_at(cui_ctx *ui, const cui__rg_paint *p, float f, cui_color none) {
    for (int k = 0; k < p->nband; k++)
        if (f <= (p->bands[k].to - p->lo) / (p->hi - p->lo) + 1e-4f) return cui__tone_color(ui, p->bands[k].tone);
    return none;
}

/* The readout every style shares: the value as the hero with its unit, the label under it,
 * then the band's name as a tinted chip. `cy` is the value's middle. */
static void cui__rg_readout(cui_ctx *ui, const cui__rg_paint *p, float cx, float cy, float vf) {
    float uf = CUI_MAX(cui__fs(ui, ui->theme.text_sm), vf * 0.4f), lf = cui__fs(ui, ui->theme.text_sm);
    float vw = cui__text_width(ui, CUI_FONT_DISPLAY, vf, p->value, -1, true);
    float uw = p->unit ? cui__text_width(ui, CUI_FONT_BODY, uf, p->unit, -1, false) + 3 : 0;
    float vh = cui__font_line_height(ui, CUI_FONT_DISPLAY, vf), vx = cx - (vw + uw) * 0.5f;
    cui__draw_text_line(ui, CUI_FONT_DISPLAY, vf, vx, cy - vh * 0.5f, CUI_C(ui, FOREGROUND), p->value, -1, true);
    if (p->unit) {
        float uh = cui__font_line_height(ui, CUI_FONT_BODY, uf);
        cui__draw_text_line(ui, CUI_FONT_BODY, uf, vx + vw + 3, cy + vh * 0.5f - uh - vh * 0.1f, CUI_C(ui, TEXT_MUTED), p->unit, -1, false);
    }
    float y = cy + vh * 0.5f + 8;
    if (p->label) {
        cui__draw_text_at(ui, CUI_FONT_BODY, lf, cx, y, CUI__AT_CENTER, CUI_C(ui, TEXT_SECONDARY), p->label, false);
        y += cui__font_line_height(ui, CUI_FONT_BODY, lf) * 0.5f + 12;
    }
    if (p->status) {
        float fs = cui__fs(ui, ui->theme.text_xs), w = cui__text_width(ui, CUI_FONT_MEDIUM, fs, p->status, -1, false) + 16, h = 20;
        cui_draw_rect(ui, (cui_rect){ cx - w * 0.5f, y, w, h }, CUI_CORNERS(6), cui_alpha(p->status_c, ui->mode == CUI_THEME_DARK ? 0.14f : 0.12f));
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, cx, y + h * 0.5f, CUI__AT_CENTER, p->status_c, p->status, false);
    }
}

/* segments: short radial strokes, lit in their band's tone up to the value, the rest a track */
static void cui__rg_paint_segments(cui_ctx *ui, const cui__rg_paint *p, cui_vec2 c) {
    float R = p->R, rs = R - 9, half = CUI_MAX(4.0f, R * 0.075f), w = CUI_MAX(2.5f, R * 0.044f);
    int n = CUI_CLAMP((int)roundf(rs * p->sweep / 12.0f), 12, 60);
    cui_color track = cui__viz_empty(ui);
    for (int i = 0; i < n; i++) {
        float f = (i + 0.5f) / (float)n, a = p->a0 + p->sweep * f;
        bool on = f <= p->t;
        cui_color col = !on ? track : p->nband ? cui__rg_band_at(ui, p, f, p->fill) : p->fill;
        cui_vec2 d = { cosf(a), sinf(a) };
        cui_vec2 e0 = { c.x + d.x * (rs - half), c.y + d.y * (rs - half) }, e1 = { c.x + d.x * (rs + half), c.y + d.y * (rs + half) };
        cui_draw_line(ui, e0, e1, w, col);
        cui_draw_circle(ui, e0, w * 0.5f, col);   /* round caps */
        cui_draw_circle(ui, e1, w * 0.5f, col);
    }
}

/* arc: a track and a value arc with round caps, min and max under the ends, an optional target */
static void cui__rg_paint_arc(cui_ctx *ui, const cui__rg_paint *p, cui_vec2 c) {
    float R = p->R, th = CUI_MAX(5.0f, R * 0.085f), ar = R - 14;
    float a0 = p->a0, a1 = a0 + p->sweep, av = a0 + p->sweep * cui__clampf(p->t, 0, 1);
    cui_color track = cui__viz_empty(ui);
    cui_draw_arc(ui, c, ar, a0, a1, th, track);
    cui_draw_circle(ui, (cui_vec2){ c.x + cosf(a1) * ar, c.y + sinf(a1) * ar }, th * 0.5f, track);
    cui_draw_circle(ui, (cui_vec2){ c.x + cosf(a0) * ar, c.y + sinf(a0) * ar }, th * 0.5f, p->t > 0.001f ? p->fill : track);
    if (p->t > 0.001f) {
        cui_draw_arc(ui, c, ar, a0, av, th, p->fill);
        cui_draw_circle(ui, (cui_vec2){ c.x + cosf(av) * ar, c.y + sinf(av) * ar }, th * 0.5f, p->fill);
    }
    float fs = cui__fs(ui, ui->theme.text_xs);
    if (p->ends) {
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, c.x + cosf(a0) * ar, c.y + sinf(a0) * ar + th * 0.5f + 12, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED),
                          cui__fmt_value(ui, p->format, p->lo), true);
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, c.x + cosf(a1) * ar, c.y + sinf(a1) * ar + th * 0.5f + 12, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED),
                          cui__fmt_value(ui, p->format, p->hi), true);
    }
    if (p->target) {
        /* a short stroke across the arc in the warning tone, its value just outside */
        float a = a0 + p->sweep * cui__clampf(p->target_t, 0, 1);
        cui_vec2 d = { cosf(a), sinf(a) };
        cui_color tc = CUI_C(ui, WARNING);
        float r0 = ar - th * 0.5f - 4, r1 = ar + th * 0.5f + 4;
        cui_draw_line(ui, (cui_vec2){ c.x + d.x * (r0 - 1), c.y + d.y * (r0 - 1) }, (cui_vec2){ c.x + d.x * (r1 + 1), c.y + d.y * (r1 + 1) }, 4.5f, CUI_C(ui, SURFACE));
        cui_draw_line(ui, (cui_vec2){ c.x + d.x * r0, c.y + d.y * r0 }, (cui_vec2){ c.x + d.x * r1, c.y + d.y * r1 }, 2.5f, tc);
        float lr = r1 + 13;
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, c.x + d.x * lr, c.y + d.y * lr, CUI__AT_CENTER, tc, cui__fmt_value(ui, p->format, p->lo + (p->hi - p->lo) * p->target_t), true);
    }
}

/* ring: a slim full turn from the top, round caps on the value */
static void cui__rg_paint_ring(cui_ctx *ui, const cui__rg_paint *p, cui_vec2 c) {
    float th = CUI_MAX(4.0f, p->R * 0.1f), ar = p->R - th * 0.5f, a0 = -CUI_PI * 0.5f;
    float av = a0 + 2 * CUI_PI * cui__clampf(p->t, 0, 1);
    cui_draw_arc(ui, c, ar, 0, 2 * CUI_PI, th, cui__viz_empty(ui));
    if (p->t > 0.001f) {
        cui_draw_arc(ui, c, ar, a0, av, th, p->fill);
        cui_draw_circle(ui, (cui_vec2){ c.x + cosf(a0) * ar, c.y + sinf(a0) * ar }, th * 0.5f, p->fill);
        cui_draw_circle(ui, (cui_vec2){ c.x + cosf(av) * ar, c.y + sinf(av) * ar }, th * 0.5f, p->fill);
    }
}

static void cui__paint_radial_gauge(cui_ctx *ui, cui_node *n, void *data) {
    cui__rg_paint *p = data;
    cui_rect r = n->rect;
    cui_vec2 c = { r.x + r.w * 0.5f, r.y + 4 + p->R };
    float lf = cui__fs(ui, ui->theme.text_sm);
    if (p->style == CUI_GAUGE_RING) {
        cui__rg_paint_ring(ui, p, c);
        float vf = cui__fs(ui, CUI_MAX(18.0f, p->R * 0.47f));
        cui__rg_readout(ui, p, c.x, c.y - (p->label ? 8 : 0), vf);
        if (p->detail) cui__draw_text_at(ui, CUI_FONT_BODY, lf, c.x, c.y + p->R + 18, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED), p->detail, false);
        return;
    }
    if (p->style == CUI_GAUGE_SEGMENTS) cui__rg_paint_segments(ui, p, c);
    else cui__rg_paint_arc(ui, p, c);
    float vf = cui__fs(ui, CUI_MAX(18.0f, p->R * 0.4f));
    cui__rg_readout(ui, p, c.x, c.y - 4, vf);
    if (p->detail) cui__draw_text_at(ui, CUI_FONT_BODY, lf, c.x, r.y + r.h - 10, CUI__AT_CENTER, CUI_C(ui, TEXT_MUTED), p->detail, false);
}

void cui_radial_gauge_ex(cui_ctx *ui, const char *id_str, float value, float min, float max, const cui_radial_gauge_opts *o) {
    cui_radial_gauge_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!(max > min)) max = min + 1;
    cui_id id = cui_make_id(ui, id_str ? id_str : "gauge");
    cui__rg_state *st = cui_state(ui, id, sizeof *st);
    float t = value == value ? cui__clampf((value - min) / (max - min), 0, 1) : 0;
    if (!st->init) { st->fill.x = 0; st->init = true; }
    cui__spring(ui, &st->fill, t, 0.55f, 0.0f);
    float size = o->size > 0 ? o->size : 200;
    float sweep = (o->sweep > 0 ? CUI_CLAMP(o->sweep, 90, 300) : 270) * CUI_PI / 180.0f;
    float a0 = CUI_PI * 0.5f + (2 * CUI_PI - sweep) * 0.5f;
    float R = size * 0.5f - 4;
    cui__rg_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->t = st->fill.x; p->lo = min; p->hi = max; p->a0 = a0; p->sweep = sweep; p->R = R;
    p->nband = o->bands ? CUI_CLAMP(o->band_count, 0, 16) : 0;
    if (p->nband) { p->bands = cui__frame_alloc(ui, sizeof(cui_gauge_band) * (size_t)p->nband); memcpy(p->bands, o->bands, sizeof(cui_gauge_band) * (size_t)p->nband); }
    p->style = o->style == CUI_GAUGE_AUTO ? (p->nband ? CUI_GAUGE_SEGMENTS : CUI_GAUGE_ARC) : o->style;
    if (p->style > CUI_GAUGE_RING) p->style = CUI_GAUGE_ARC;
    p->ends = p->style == CUI_GAUGE_ARC && o->ticks >= 0;
    p->format = o->format ? cui__frame_strdup(ui, o->format, -1) : NULL;
    p->value = cui__fmt_value(ui, o->format, value);
    p->unit = o->unit && *o->unit ? cui__frame_strdup(ui, o->unit, -1) : NULL;
    p->label = o->label && *o->label ? cui__frame_strdup(ui, o->label, -1) : NULL;
    p->detail = o->detail && *o->detail ? cui__frame_strdup(ui, o->detail, -1) : NULL;
    p->target = o->show_target && p->style == CUI_GAUGE_ARC; p->target_t = (o->target - min) / (max - min);
    /* the value's color: the caller's, else the band it is in, else the accent (the ring takes the first series color) */
    p->fill = p->style == CUI_GAUGE_RING ? CUI_C(ui, SERIES_1) : cui__emphasis(ui);
    for (int k = 0; k < p->nband; k++)
        if (value <= p->bands[k].to || k == p->nband - 1) {
            p->fill = cui__tone_color(ui, p->bands[k].tone);
            if (p->style == CUI_GAUGE_SEGMENTS && p->bands[k].name && *p->bands[k].name) {
                p->status = cui__frame_strdup(ui, p->bands[k].name, -1);
                p->status_c = p->fill;
            }
            break;
        }
    if (cui__color_set(o->color)) p->fill = cui_color_of(ui, o->color);
    /* tall enough for the arc's lower ends with their labels, and for the readout */
    float vf = cui__fs(ui, CUI_MAX(18.0f, R * 0.4f)), vh = cui__font_line_height(ui, CUI_FONT_DISPLAY, vf);
    float lh = cui__font_line_height(ui, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm));
    float h;
    if (p->style == CUI_GAUGE_RING) h = 4 + 2 * R + 4 + (p->detail ? 26 : 0);
    else {
        float ends = R * sinf(a0) + (p->ends ? 24 : 6);
        float read = -4 + vh * 0.5f + 8 + (p->label ? lh * 0.5f + 12 : 0) + (p->status ? 24 : (p->label ? -4 : 0));
        h = 4 + R + CUI_MAX(ends, read) + (p->detail ? 22 : 0);
    }
    int node = cui__viz_node(ui, id, CUI_PX(size), CUI_PX(ceilf(h)), cui__paint_radial_gauge, p, false);
    ui->nodes[node].self_align = CUI__ALIGN_NOSTRETCH;
}

/* ------------------------------------------------------------------------ */
/* Candlestick chart                                                        */
/* ------------------------------------------------------------------------ */

#define CUI__CS_AXIS_H 22.0f   /* time labels under the plot */
#define CUI__CS_MAXOV  4
#define CUI__CS_MAXLAB 48

typedef struct {
    double first, span;        /* the view, in candles: [first, first + span) */
    float ylo, yhi, axis_w, press_x;
    double press_first;
    int count, cursor;
    bool init, sized, follow, keys;
} cui__cs_state;

typedef struct { float o, h, l, c, v; } cui__cs_bar;
typedef struct { float x; const char *s; } cui__cs_label;

typedef struct {
    cui__cs_bar *bars; int nbars, i0;  /* candles from i0, or one merged bar per pixel column */
    bool merged, volume, ohlc;
    double first, span;
    float ylo, yhi, vmax, axis_w;
    float *ov; int nov; cui_color ov_col[CUI__CS_MAXOV];
    cui__cs_label lab[CUI__CS_MAXLAB]; int nlab;
    int hover;                         /* candle index, -1 none */
    float hover_y; bool hy;            /* pointer height for the horizontal hair */
    float last; bool last_up, has_last;
    const char *format;
    cui_color up, down;
    cui_id wheel; bool wheel_on;
    cui__tip tip;
} cui__cs_paint;

typedef struct { cui_rect plot, price, vol; float cw; } cui__cs_geo;

static cui__cs_geo cui__cs_layout(cui_rect r, float axis_w, double span, bool volume) {
    cui__cs_geo g;
    g.plot = (cui_rect){ r.x, r.y + 6, CUI_MAX(10.0f, r.w - axis_w), CUI_MAX(10.0f, r.h - 6 - CUI__CS_AXIS_H) };
    float vh = volume ? CUI_MAX(24.0f, g.plot.h * 0.18f) : 0;
    g.price = (cui_rect){ g.plot.x, g.plot.y, g.plot.w, g.plot.h - (volume ? vh + 8 : 0) };
    g.vol = (cui_rect){ g.plot.x, g.plot.y + g.plot.h - vh, g.plot.w, vh };
    g.cw = (float)(g.plot.w / (span > 0 ? span : 1));
    return g;
}

static float cui__cs_y(const cui__cs_paint *p, const cui__cs_geo *g, float v) {
    return g->price.y + g->price.h - (v - p->ylo) / (p->yhi - p->ylo) * g->price.h;
}

static void cui__paint_candles(cui_ctx *ui, cui_node *n, void *data) {
    cui__cs_paint *p = data;
    cui_rect r = n->rect;
    if (p->wheel_on) cui__paint_wheel_hit(ui, p->wheel, r);
    cui__cs_geo g = cui__cs_layout(r, p->axis_w, p->span, p->volume);
    float fs = cui__fs(ui, ui->theme.text_xs);
    cui_color muted = CUI_C(ui, TEXT_MUTED);
    /* price grid and axis on the right */
    float glo, ghi, gstep;
    cui__nice_range(p->ylo, p->yhi, CUI_MAX(2, (int)(g.price.h / 40)), &glo, &ghi, &gstep);
    float last_y = p->has_last ? cui__cs_y(p, &g, p->last) : -1e9f;
    if (gstep > 0)
        for (float v = glo; v <= p->yhi + gstep * 0.01f; v += gstep) {
            if (v < p->ylo - gstep * 0.01f) continue;
            float y = cui__cs_y(p, &g, v);
            cui_draw_rect(ui, (cui_rect){ g.price.x, y - 0.5f, g.price.w, 1 }, CUI_CORNERS(0), CUI_C(ui, BORDER_SUBTLE));
            if (fabsf(y - last_y) >= 16)   /* the last price's tag covers the label */
                cui__draw_text_at(ui, CUI_FONT_BODY, fs, g.plot.x + g.plot.w + 8, y, CUI__AT_LEFT, muted, cui__fmt_value(ui, p->format, v), true);
        }
    for (int k = 0; k < p->nlab; k++) {
        float x = g.plot.x + p->lab[k].x;
        cui_draw_rect(ui, (cui_rect){ x - 0.5f, g.plot.y, 1, g.plot.h }, CUI_CORNERS(0), cui_alpha(CUI_C(ui, BORDER_SUBTLE), 0.6f));
        cui__draw_text_at(ui, CUI_FONT_BODY, fs, x, g.plot.y + g.plot.h + CUI__CS_AXIS_H * 0.5f + 2, CUI__AT_CENTER, muted, p->lab[k].s, false);
    }
    cui_draw_rect(ui, (cui_rect){ g.plot.x, g.price.y + g.price.h - 0.5f, g.plot.w, 1 }, CUI_CORNERS(0), CUI_C(ui, BORDER));
    cui_push_clip(ui, g.plot);
    bool dark = ui->mode == CUI_THEME_DARK;
    float va = dark ? 0.42f : 0.32f;
    /* each bar: the candle (or merged column) at x, width bw */
    for (int k = 0; k < p->nbars; k++) {
        const cui__cs_bar *b = &p->bars[k];
        if (!(b->h >= b->l)) continue;
        float x, bw;
        if (p->merged) { x = g.plot.x + k + 0.5f; bw = 1; }
        else {
            x = g.plot.x + (float)((p->i0 + k + 0.5) - p->first) * g.cw;
            bw = CUI_MAX(1.0f, floorf(g.cw * 0.68f));
        }
        bool up = b->c >= b->o;
        cui_color col = up ? p->up : p->down;
        if (p->volume && p->vmax > 0 && b->v > 0) {
            float vh = b->v / p->vmax * g.vol.h;
            cui_draw_rect(ui, (cui_rect){ x - bw * 0.5f, g.vol.y + g.vol.h - vh, bw, vh }, CUI_CORNERS(0), cui_alpha(col, va));
        }
        float yh = cui__cs_y(p, &g, b->h), yl = cui__cs_y(p, &g, b->l);
        float yo = cui__cs_y(p, &g, b->o), yc = cui__cs_y(p, &g, b->c);
        bool hl = !p->merged && p->i0 + k == p->hover;
        if (p->merged || bw < 3) {
            cui_draw_rect(ui, (cui_rect){ x - 0.5f, yh, 1, CUI_MAX(1.0f, yl - yh) }, CUI_CORNERS(0), col);
        } else if (p->ohlc) {
            cui_draw_rect(ui, (cui_rect){ x - 0.75f, yh, 1.5f, CUI_MAX(1.0f, yl - yh) }, CUI_CORNERS(0), col);
            float tw = CUI_MAX(2.0f, bw * 0.5f);
            cui_draw_rect(ui, (cui_rect){ x - tw, yo - 0.75f, tw, 1.5f }, CUI_CORNERS(0), col);
            cui_draw_rect(ui, (cui_rect){ x, yc - 0.75f, tw, 1.5f }, CUI_CORNERS(0), col);
        } else {
            cui_draw_rect(ui, (cui_rect){ x - 0.5f, yh, 1, CUI_MAX(1.0f, yl - yh) }, CUI_CORNERS(0), col);
            float top = CUI_MIN(yo, yc), bh = CUI_MAX(1.0f, fabsf(yo - yc));
            cui_draw_rect(ui, (cui_rect){ x - bw * 0.5f, top, bw, bh }, CUI_CORNERS(bw >= 6 ? 1.5f : 0), col);
        }
        if (hl) cui_draw_rect(ui, (cui_rect){ x - bw * 0.5f - 2, yh - 2, bw + 4, CUI_MAX(1.0f, yl - yh) + 4 }, CUI_CORNERS(3), cui_alpha(CUI_C(ui, FOREGROUND), 0.06f));
    }
    /* overlay lines */
    for (int s = 0; s < p->nov; s++) {
        cui_vec2 *pts = cui__frame_alloc(ui, sizeof(cui_vec2) * (size_t)CUI_MAX(1, p->nbars));
        int m = 0;
        for (int k = 0; k <= p->nbars; k++) {
            float v = k < p->nbars ? p->ov[s * p->nbars + k] : NAN;
            if (v == v) {
                float x = p->merged ? g.plot.x + k + 0.5f : g.plot.x + (float)((p->i0 + k + 0.5) - p->first) * g.cw;
                pts[m++] = (cui_vec2){ x, cui__cs_y(p, &g, v) };
            } else {
                if (m >= 2) cui_draw_polyline(ui, pts, m, 1.5f, p->ov_col[s], false);
                m = 0;
            }
        }
    }
    cui_pop_clip(ui);
    /* the last price, across the plot and tagged on the axis */
    if (p->has_last && p->last >= p->ylo && p->last <= p->yhi) {
        float y = cui__cs_y(p, &g, p->last);
        cui_color col = p->last_up ? p->up : p->down;
        for (float x = g.plot.x; x < g.plot.x + g.plot.w; x += 6)
            cui_draw_rect(ui, (cui_rect){ x, y - 0.5f, CUI_MIN(3.0f, g.plot.x + g.plot.w - x), 1 }, CUI_CORNERS(0), cui_alpha(col, 0.7f));
        const char *s = cui__fmt_value(ui, p->format, p->last);
        float tw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, s, -1, true) + 10;
        cui_rect tag = { g.plot.x + g.plot.w + 2, y - 9, tw, 18 };
        cui_draw_rect(ui, tag, CUI_CORNERS(4), col);
        cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, tag.x + 5, y, CUI__AT_LEFT, cui__viz_light_text(col) ? cui__rgba(255, 255, 255, 255) : cui__rgba(20, 20, 20, 255), s, true);
    }
    /* crosshair */
    if (p->hover >= 0 && p->tip.alpha > 0.01f) {
        cui_color hair = cui_alpha(CUI_C(ui, BORDER_STRONG), p->tip.alpha);
        float x = g.plot.x + (float)((p->hover + 0.5) - p->first) * g.cw;
        if (x >= g.plot.x && x <= g.plot.x + g.plot.w) cui_draw_rect(ui, (cui_rect){ x - 0.5f, g.plot.y, 1, g.plot.h }, CUI_CORNERS(0), hair);
        if (p->hy && p->hover_y >= g.price.y && p->hover_y <= g.price.y + g.price.h) {
            cui_draw_rect(ui, (cui_rect){ g.plot.x, p->hover_y - 0.5f, g.plot.w, 1 }, CUI_CORNERS(0), hair);
            float v = p->ylo + (g.price.y + g.price.h - p->hover_y) / g.price.h * (p->yhi - p->ylo);
            const char *s = cui__fmt_value(ui, p->format, v);
            float tw = cui__text_width(ui, CUI_FONT_MEDIUM, fs, s, -1, true) + 10;
            cui_rect tag = { g.plot.x + g.plot.w + 2, p->hover_y - 9, tw, 18 };
            cui_draw_rect(ui, tag, CUI_CORNERS(4), cui_alpha(CUI_C(ui, FOREGROUND), p->tip.alpha));
            cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, tag.x + 5, p->hover_y, CUI__AT_LEFT, cui_alpha(CUI_C(ui, BACKGROUND), p->tip.alpha), s, true);
        }
    }
    cui__tip_draw(ui, &p->tip);
}

int cui_candlestick_ex(cui_ctx *ui, const char *id_str, const cui_candle *candles, int count, const cui_candlestick_opts *o) {
    cui_candlestick_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!candles) count = 0;
    count = CUI_MAX(0, count);
    cui_id id = cui_make_id(ui, id_str ? id_str : "candles");
    cui_id wid = id ^ 0xCA4D1u;
    cui__viz_state *vs = cui__viz_state_get(ui, id);
    cui__cs_state *st = cui_state(ui, id ^ 0xCA4D2u, sizeof *st);
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE | CUI_IF_DRAG);
    cui_rect pr = cui__prev_rect(ui, id);
    float h = o->height > 0 ? o->height : 320;
    bool volume = false;
    if (!o->no_volume) for (int i = 0; i < count && !volume; i++) volume = candles[i].volume > 0;
    if (st->axis_w <= 0) st->axis_w = 60;
    cui_rect rr = pr.w > 0 ? pr : (cui_rect){ 0, 0, 600, h };
    cui__cs_geo g = cui__cs_layout(rr, st->axis_w, st->span, volume);
    double min_span = CUI_MIN(8.0, (double)CUI_MAX(1, count)), max_span = CUI_MAX(8.0, count * 1.1);
    if (!st->init || st->span <= 0 || (!st->sized && pr.w > 0)) {
        /* about nine points per candle, from the first frame the chart has a size */
        st->span = CUI_CLAMP(g.plot.w / 9.0, min_span, (double)CUI_MAX(1, count));
        st->first = count - st->span;
        st->follow = true; st->cursor = count - 1; st->init = true; st->sized = pr.w > 0;
    }
    if (st->follow && count != st->count) st->first = count - st->span;   /* new candles come into view */
    st->count = count;
    g = cui__cs_layout(rr, st->axis_w, st->span, volume);
    /* zoom around a point (0..1 across the plot) */
#define CUI__CS_ZOOM(f, at) do { double m_ = st->first + (at) * st->span; st->span = CUI_CLAMP(st->span * (f), min_span, max_span); st->first = m_ - (at) * st->span; } while (0)
    if (ui->scroll_target == wid && (ui->scroll_dy != 0 || ui->scroll_dx != 0) && pr.w > 0) {
        float at = cui__clampf((ui->mx - g.plot.x) / g.plot.w, 0, 1);
        if (ui->scroll_dx != 0 || (ui->mods & CUI_MOD_SHIFT)) st->first -= (ui->scroll_dx != 0 ? ui->scroll_dx : ui->scroll_dy) * st->span * 0.06;
        else CUI__CS_ZOOM(pow(1.18, -ui->scroll_dy), at);
        ui->scroll_dx = ui->scroll_dy = 0;
    }
    if (r.active && ui->mdown[0] && pr.w > 0) {
        if (ui->mpressed[0]) { st->press_x = ui->mx; st->press_first = st->first; }
        st->first = st->press_first - (ui->mx - st->press_x) / g.cw;
        cui__set_cursor(ui, CUI_CURSOR_MOVE);
    }
    if (r.focused && count > 0) {
        int step = (ui->mods & CUI_MOD_SHIFT) ? 10 : 1, cur = st->keys ? st->cursor : CUI_CLAMP((int)(st->first + st->span) - 1, 0, count - 1);
        bool moved = false;
        if (cui_key_pressed(ui, CUI_KEY_LEFT)) { cur -= step; moved = true; }
        if (cui_key_pressed(ui, CUI_KEY_RIGHT)) { cur += step; moved = true; }
        if (cui_key_pressed(ui, CUI_KEY_HOME)) { cur = 0; moved = true; }
        if (cui_key_pressed(ui, CUI_KEY_END)) { cur = count - 1; moved = true; }
        if (cui_key_pressed(ui, CUI_KEY_EQUAL)) CUI__CS_ZOOM(1 / 1.4, 0.5);
        if (cui_key_pressed(ui, CUI_KEY_MINUS)) CUI__CS_ZOOM(1.4, 0.5);
        if (cui_key_pressed(ui, CUI_KEY_0)) { st->span = CUI_MAX((double)count, min_span); st->first = 0; }
        if (moved) {
            st->cursor = CUI_CLAMP(cur, 0, count - 1); st->keys = true;
            if (st->cursor < st->first) st->first = st->cursor;
            if (st->cursor + 1 > st->first + st->span) st->first = st->cursor + 1 - st->span;
        }
    }
#undef CUI__CS_ZOOM
    if (ui->mx != ui->pmx || ui->my != ui->pmy) st->keys = false;
    {
        double lo = -st->span * 0.2, hi = count - st->span * 0.8;
        if (hi < lo) hi = lo;
        st->first = CUI_CLAMP(st->first, lo, hi);
    }
    st->follow = st->first + st->span >= count - 0.01;
    g = cui__cs_layout(rr, st->axis_w, st->span, volume);
    int hover = -1;
    if (r.hovered && !r.active && ui->mx >= g.plot.x && ui->mx < g.plot.x + g.plot.w && ui->my >= g.plot.y && ui->my < g.plot.y + g.plot.h) {
        int i = (int)floor(st->first + (ui->mx - g.plot.x) / g.cw);
        if (i >= 0 && i < count) hover = i;
    }
    if (st->keys && r.focused && count > 0) hover = st->cursor;
    cui__viz_hover(ui, vs, hover);

    /* candles in view: the price range follows them */
    int i0 = CUI_MAX(0, (int)floor(st->first)), i1 = CUI_MIN(count, (int)ceil(st->first + st->span) + 1);
    int nov = o->overlays ? CUI_CLAMP(o->overlay_count, 0, CUI__CS_MAXOV) : 0;
    float lo = 1e30f, hi = -1e30f, vmax = 0;
    for (int i = i0; i < i1; i++) {
        const cui_candle *c = &candles[i];
        if (c->low == c->low) lo = CUI_MIN(lo, c->low);
        if (c->high == c->high) hi = CUI_MAX(hi, c->high);
        vmax = CUI_MAX(vmax, c->volume);
        for (int s = 0; s < nov; s++)
            if (i < o->overlays[s].count && o->overlays[s].values[i] == o->overlays[s].values[i]) { lo = CUI_MIN(lo, o->overlays[s].values[i]); hi = CUI_MAX(hi, o->overlays[s].values[i]); }
    }
    if (lo > hi) { lo = 0; hi = 1; }
    if (hi - lo < 1e-6f) { lo -= 0.5f; hi += 0.5f; }
    float pad = (hi - lo) * 0.07f;
    lo -= pad; hi += pad;
    if (!(st->yhi > st->ylo) || r.active) { st->ylo = lo; st->yhi = hi; }
    else { cui__tween(ui, &st->ylo, lo, CUI__T_STATE); cui__tween(ui, &st->yhi, hi, CUI__T_STATE); }
    /* the price axis is as wide as its widest label (next frame) */
    float fs = cui__fs(ui, ui->theme.text_xs);
    st->axis_w = ceilf(CUI_MAX(cui__text_width(ui, CUI_FONT_MEDIUM, fs, cui__fmt_value(ui, o->format ? o->format : "%.2f", hi), -1, true),
                               cui__text_width(ui, CUI_FONT_MEDIUM, fs, cui__fmt_value(ui, o->format ? o->format : "%.2f", lo), -1, true))) + 22;

    cui__cs_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->first = st->first; p->span = st->span; p->ylo = st->ylo; p->yhi = st->yhi; p->vmax = vmax;
    p->axis_w = cui__cs_layout(rr, st->axis_w, st->span, volume).plot.w > 0 ? st->axis_w : 60;
    p->volume = volume; p->ohlc = o->ohlc;
    p->format = o->format ? cui__frame_strdup(ui, o->format, -1) : "%.2f";
    p->up = cui__color_set(o->up) ? cui_color_of(ui, o->up) : CUI_C(ui, SUCCESS);
    p->down = cui__color_set(o->down) ? cui_color_of(ui, o->down) : CUI_C(ui, DANGER);
    p->wheel = wid; p->wheel_on = r.hovered || r.focused;
    g = cui__cs_layout(rr, p->axis_w, st->span, volume);
    p->merged = g.cw < 2.5f;
    if (p->merged) {
        /* one bar per pixel column: first open, highest high, lowest low, last close, summed volume */
        int cols = CUI_MAX(1, (int)g.plot.w);
        p->bars = cui__frame_alloc(ui, sizeof(cui__cs_bar) * (size_t)cols);
        p->ov = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, nov * cols));
        p->nbars = cols;
        float bvmax = 0;
        for (int k = 0; k < cols; k++) {
            int a = (int)floor(st->first + k / g.cw), b = (int)floor(st->first + (k + 1) / g.cw);
            if (b <= a) b = a + 1;
            a = CUI_MAX(a, 0); b = CUI_MIN(b, count);
            cui__cs_bar *bar = &p->bars[k];
            bar->h = -1e30f; bar->l = 1e30f; bar->v = 0;
            for (int s = 0; s < nov; s++) p->ov[s * cols + k] = NAN;
            if (a >= b) { bar->h = NAN; continue; }
            bar->o = candles[a].open; bar->c = candles[b - 1].close;
            for (int i = a; i < b; i++) { bar->h = CUI_MAX(bar->h, candles[i].high); bar->l = CUI_MIN(bar->l, candles[i].low); bar->v += candles[i].volume; }
            bvmax = CUI_MAX(bvmax, bar->v);
            for (int s = 0; s < nov; s++) if (b - 1 < o->overlays[s].count) p->ov[s * cols + k] = o->overlays[s].values[b - 1];
        }
        p->vmax = bvmax;
    } else {
        int nb = CUI_MAX(0, i1 - i0);
        p->i0 = i0; p->nbars = nb;
        p->bars = cui__frame_alloc(ui, sizeof(cui__cs_bar) * (size_t)CUI_MAX(1, nb));
        p->ov = cui__frame_alloc(ui, sizeof(float) * (size_t)CUI_MAX(1, nov * nb));
        for (int k = 0; k < nb; k++) {
            const cui_candle *c = &candles[i0 + k];
            p->bars[k] = (cui__cs_bar){ c->open, c->high, c->low, c->close, c->volume };
            for (int s = 0; s < nov; s++) p->ov[s * nb + k] = i0 + k < o->overlays[s].count ? o->overlays[s].values[i0 + k] : NAN;
        }
    }
    p->nov = nov;
    for (int s = 0; s < nov; s++) p->ov_col[s] = cui__viz_color(ui, o->overlays[s].color, s);
    /* time labels at a spacing that reads, on candles that are multiples of the step */
    if (o->labels && count > 0) {
        int step = CUI_MAX(1, (int)ceilf(90.0f / CUI_MAX(0.01f, g.cw)));
        static const int nice[] = { 1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000 };
        for (int k = 0; k < (int)(sizeof nice / sizeof nice[0]); k++) if (nice[k] >= step) { step = nice[k]; break; }
        for (int i = (i0 / step) * step; i < i1 && p->nlab < CUI__CS_MAXLAB; i += step) {
            if (i < i0 || !o->labels[i]) continue;
            float x = (float)((i + 0.5) - st->first) * g.cw;
            if (x < 20 || x > g.plot.w - 20) continue;
            p->lab[p->nlab++] = (cui__cs_label){ x, cui__frame_strdup(ui, o->labels[i], -1) };
        }
    }
    if (count > 0) { p->has_last = true; p->last = candles[count - 1].close; p->last_up = candles[count - 1].close >= candles[count - 1].open; }
    p->hover = vs->hover_t > 0.01f && vs->shown < count ? vs->shown : -1;
    p->hover_y = ui->my; p->hy = hover >= 0 && !st->keys;
    if (p->hover >= 0) {
        const cui_candle *c = &candles[p->hover];
        p->tip.alpha = vs->hover_t; p->tip.bounds = pr;
        p->tip.x = st->keys ? g.plot.x + (float)((p->hover + 0.5) - st->first) * g.cw : ui->mx;
        p->tip.y = st->keys ? g.price.y + g.price.h * 0.3f : ui->my;
        p->tip.title = o->labels && o->labels[p->hover] ? cui__frame_strdup(ui, o->labels[p->hover], -1) : cui__frame_printf(ui, "#%d", p->hover + 1);
        const char *f = p->format;
        cui__tip_row(&p->tip, "Open", cui__fmt_value(ui, f, c->open), cui__rgba(0, 0, 0, 0));
        cui__tip_row(&p->tip, "High", cui__fmt_value(ui, f, c->high), cui__rgba(0, 0, 0, 0));
        cui__tip_row(&p->tip, "Low", cui__fmt_value(ui, f, c->low), cui__rgba(0, 0, 0, 0));
        cui__tip_row(&p->tip, "Close", cui__fmt_value(ui, f, c->close), c->close >= c->open ? p->up : p->down);
        float ref = p->hover > 0 ? candles[p->hover - 1].close : c->open;
        if (ref != 0) cui__tip_row(&p->tip, "Change", cui__frame_printf(ui, "%+.2f%%", (double)((c->close - ref) / ref * 100)), cui__rgba(0, 0, 0, 0));
        if (volume) cui__tip_row(&p->tip, "Volume", cui__frame_printf(ui, c->volume >= 1e6f ? "%.2fM" : c->volume >= 1e4f ? "%.1fk" : "%.0f",
                                                                      (double)(c->volume >= 1e6f ? c->volume / 1e6f : c->volume >= 1e4f ? c->volume / 1e3f : c->volume)), cui__rgba(0, 0, 0, 0));
    }
    if (r.hovered && !r.active) cui__set_cursor(ui, CUI_CURSOR_ARROW);
    int node = cui__viz_node(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(h), cui__paint_candles, p, hover >= 0);
    cui__mark_interactive(ui, node, CUI_IF_FOCUSABLE);
    cui__focus_ring(ui, node, r.focus_visible ? 1.0f : 0.0f);
    return hover;
}
