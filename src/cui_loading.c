/* Loading animations drawn in dots: the dither styles of cui_spinner and the status spinner.
 *
 * Every style fills a small grid of cells with densities (one array per color layer) for a
 * time that moves in steps, and draws each layer with cui__draw_dither_cells, so a cell is
 * on where its 8x8 Bayer threshold is under its density. The time steps at the rate the motion
 * needs, and the paint callback asks for a wakeup at the next step: a spinner that is scrolled
 * away or covered paints nothing and wakes nothing. Trails are computed from where the dots
 * were over the last half second, not kept in a buffer, so a spinner keeps no state. */
#include "cui_internal.h"

#define CUI__DSPIN_TAU (2.0f * CUI_PI)
#define CUI__DSPIN_INLINE 32.0f       /* points: below this height a cell is one point, from it two */
#define CUI__DSPIN_MAX_CELLS 128      /* cells on the longer side; a bigger spinner gets bigger cells */
#define CUI__DSPIN_TRAIL_DT (1.0f / 60.0f)   /* seconds between the samples of a trail */
#define CUI__DSPIN_TRAIL_MAX 40       /* samples a trail looks back at most */
#define CUI__DSPIN_PERIOD 3600.0      /* seconds after which the clock of the motion wraps */

/* the color layers, drawn in this order: a base under the dots, then the dots */
enum { CUI__DSPIN_TRACK, CUI__DSPIN_EDGE, CUI__DSPIN_INK, CUI__DSPIN_ALT, CUI__DSPIN_LAYERS };

typedef struct {
    float aspect;              /* width over height */
    float fps;                 /* steps a second the motion needs */
    float still;               /* the time shown under reduced motion: a frame that reads as busy */
} cui__dspin_info;

/* indexed by cui_spinner_style */
static const cui__dspin_info cui__dspin_infos[] = {
    { 1, 0, 0 },               /* arc: drawn by cui_widgets.c */
    { 1, 30, 1.06f },          /* comet: the head at the top right */
    { 1, 15, 0.9f },           /* dot matrix */
    { 1, 20, 0.78f },          /* fill and drain: half full */
    { 1, 24, 1.1f },           /* radar: a point just passed */
    { 1, 12, 0.5f },           /* ripple */
    { 25.0f / 32.0f, 15, 0 },  /* upload */
    { 25.0f / 32.0f, 15, 0 },  /* download */
    { 50.0f / 32.0f, 15, 0 },  /* transfer */
    { 1, 30, 0.8f },           /* orbit */
    { 40.0f / 28.0f, 30, 0.4f },   /* bounce */
    { 28.0f / 32.0f, 15, 1.6f },   /* scan: the line mid page */
    { 1, 15, 0.35f },          /* pulse */
    { 1, 20, 0 },              /* spiral */
    { 1, 12, 0 },              /* plasma */
};

typedef struct {
    int w, h;                  /* cells */
    float t;                   /* seconds */
    float *v[CUI__DSPIN_LAYERS];   /* w * h densities each */
} cui__dspin_grid;

static inline void cui__dspin_set(cui__dspin_grid *g, int layer, int x, int y, float d) { g->v[layer][y * g->w + x] = d; }
static inline void cui__dspin_max(cui__dspin_grid *g, int layer, int x, int y, float d) {
    float *p = &g->v[layer][y * g->w + x];
    if (d > *p) *p = d;
}

/* how far behind the head an angle is, 0 at the head to 1 a full turn back */
static float cui__dspin_tail(float a, float head) {
    float v = fmodf(head - a, CUI__DSPIN_TAU);
    if (v < 0) v += CUI__DSPIN_TAU;
    return v / CUI__DSPIN_TAU;
}

/* distance from (x, y) outside a rounded square of side 2c + 1 cells and corner rr, centered at (c, c); 0 inside its straight part */
static float cui__dspin_rbox(float x, float y, float cx, float cy, float rr) {
    float qx = CUI_MAX(0.0f, fabsf(x - cx) - (cx - rr)), qy = CUI_MAX(0.0f, fabsf(y - cy) - (cy - rr));
    return sqrtf(qx * qx + qy * qy);
}

/* a dot of radius r at (px, py) with the value d, kept where it is brighter than what is there */
static void cui__dspin_disc(cui__dspin_grid *g, float px, float py, float r, float d) {
    int x0 = CUI_MAX(0, (int)floorf(px - r)), x1 = CUI_MIN(g->w - 1, (int)ceilf(px + r));
    int y0 = CUI_MAX(0, (int)floorf(py - r)), y1 = CUI_MIN(g->h - 1, (int)ceilf(py + r));
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if ((x - px) * (x - px) + (y - py) * (y - py) < r * r) cui__dspin_max(g, CUI__DSPIN_INK, x, y, d);
}

/* ---- the styles: each fills the layers for g->t ---- */

/* a ring of dots with a head and a tail that thins out over three quarters of a turn */
static void cui__dspin_comet(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, R = g->w * 0.5f - 0.5f, r0 = R - CUI_MAX(2.0f, g->w * 0.13f), head = g->t * 5.2f;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float dx = x - c, dy = y - c, r = sqrtf(dx * dx + dy * dy);
            if (r > R || r < r0) continue;
            cui__dspin_set(g, CUI__DSPIN_TRACK, x, y, 1);
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, 1 - cui__dspin_tail(atan2f(dy, dx), head) * 1.3f);
        }
}

/* a 5x5 grid of rounded dots, a wave running through it from corner to corner */
static void cui__dspin_matrix(cui__dspin_grid *g) {
    int gap = CUI_MAX(1, (int)roundf(g->w / 14.0f)), dot = CUI_MAX(1, (g->w - 4 * gap) / 5);
    int off = (g->w - (5 * dot + 4 * gap)) / 2;
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 5; i++) {
            float b = CUI_MAX(0.05f, 0.5f + 0.5f * sinf(g->t * 4.2f - (i + j) * 0.75f));
            for (int y = 0; y < dot; y++)
                for (int x = 0; x < dot; x++) {
                    if (dot >= 4 && (x == 0 || x == dot - 1) && (y == 0 || y == dot - 1)) continue;
                    int X = off + i * (dot + gap) + x, Y = off + j * (dot + gap) + y;
                    if (X < 0 || Y < 0 || X >= g->w || Y >= g->h) continue;
                    cui__dspin_set(g, CUI__DSPIN_TRACK, X, Y, 1);
                    cui__dspin_set(g, CUI__DSPIN_INK, X, Y, b);
                }
        }
}

/* a rounded tile that fills from the bottom with a soft edge, then dissolves */
static void cui__dspin_fill(cui__dspin_grid *g) {
    float s = g->h / 26.0f, c = (g->w - 1) * 0.5f, rr = 4 * s;
    float T = fmodf(g->t, 2.6f) / 2.6f;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float q = cui__dspin_rbox((float)x, (float)y, c, c, rr);
            if (q > rr) continue;
            float d;
            if (T < 0.55f) {
                float front = (1 - T / 0.55f) * (g->h + 5 * s) - 3 * s;
                d = cui__clampf((y - front) / (5 * s), 0, 1);
            } else d = 1 - (T - 0.55f) / 0.45f;
            bool edge = q > rr - 1 || x == 0 || y == 0 || x == g->w - 1 || y == g->h - 1;
            cui__dspin_set(g, edge ? CUI__DSPIN_EDGE : CUI__DSPIN_TRACK, x, y, 1);
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, d);
        }
}

/* a sweep around a dial with two rings; four points light up as it passes and fade after it */
static void cui__dspin_radar(cui__dspin_grid *g) {
    static const float blips[4][2] = { { 0.6f, 0.55f }, { 2.3f, 0.8f }, { 3.9f, 0.35f }, { 5.1f, 0.7f } };   /* angle, share of the radius */
    float c = (g->w - 1) * 0.5f, R = g->w * 0.5f - 0.5f, head = g->t * 2.6f;
    float br = 1.8f * CUI_MAX(0.7f, g->w / 32.0f);
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float dx = x - c, dy = y - c, r = sqrtf(dx * dx + dy * dy);
            if (r > R) continue;
            float d = CUI_MAX(0.0f, 1 - cui__dspin_tail(atan2f(dy, dx), head) * 2.6f) * 0.8f;
            for (int k = 0; k < 4; k++) {
                float bx = c + cosf(blips[k][0]) * blips[k][1] * R - x, by = c + sinf(blips[k][0]) * blips[k][1] * R - y;
                if (bx * bx + by * by < br * br) d = CUI_MAX(d, 1 - cui__dspin_tail(blips[k][0], head) * 1.2f);
            }
            if (R - r < 0.9f || fabsf(r - R * 0.5f) < 0.5f) cui__dspin_set(g, CUI__DSPIN_TRACK, x, y, 1);
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, d);
        }
}

/* a dot with three rings spreading from it, each thinning out as it grows */
static void cui__dspin_ripple(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, R = g->w * 0.5f - 0.5f, k = CUI_MAX(1.0f, g->w / 32.0f);
    float core = 2.2f * k, ring = 1.4f * k;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float r = sqrtf((x - c) * (x - c) + (y - c) * (y - c)), d = r < core ? 1.0f : 0.0f;
            for (int n = 0; n < 3; n++) {
                float p = fmodf(g->t * 0.55f + n / 3.0f, 1.0f);
                if (fabsf(r - p * R) < ring) d = CUI_MAX(d, (1 - p) * 1.05f);
            }
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, d);
        }
}

/* chevrons pointing and streaming up (dir 1) or down (dir -1) in a lane centered at cx, fading at both ends */
static void cui__dspin_lane(cui__dspin_grid *g, float cx, int dir, int layer) {
    float s = g->h / 32.0f, half = 6 * s, period = 9 * s, band = 2.6f * s, shift = g->t * 13 * s;
    for (int y = 0; y < g->h; y++) {
        float fade = sinf(CUI_PI * y / (float)CUI_MAX(1, g->h - 1)) * 1.15f;
        int yy = dir > 0 ? y : g->h - 1 - y;
        for (int x = 0; x < g->w; x++) {
            float ax = fabsf(x - cx);
            if (ax > half) continue;
            float v = fmodf(yy - ax * 1.1f + shift, period);
            if (v < 0) v += period;
            if (v < band) cui__dspin_set(g, layer, x, y, fade);
        }
    }
}

static void cui__dspin_transfer(cui__dspin_grid *g, cui_spinner_style style) {
    if (style == CUI_SPINNER_TRANSFER) {
        cui__dspin_lane(g, g->w * 0.27f, 1, CUI__DSPIN_INK);
        cui__dspin_lane(g, g->w * 0.73f, -1, CUI__DSPIN_ALT);
    } else cui__dspin_lane(g, (g->w - 1) * 0.5f, style == CUI_SPINNER_UPLOAD ? 1 : -1, CUI__DSPIN_INK);
}

/* the number of trail samples until a dot has faded under the lowest threshold (0.5 / 64) */
static int cui__dspin_trail_len(float decay) {
    return CUI_MIN(CUI__DSPIN_TRAIL_MAX, (int)ceilf(logf(1.0f / 128.0f) / logf(decay)));
}

/* two dots circling the middle at different speeds, each leaving a trail that dissolves */
static void cui__dspin_orbit(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, k = CUI_MAX(1.0f, g->w / 30.0f), R = g->w * 0.5f - 3.5f * k;
    int n = cui__dspin_trail_len(0.86f);
    float fade = 1;
    for (int i = 0; i < n; i++, fade *= 0.86f) {
        float t = g->t - i * CUI__DSPIN_TRAIL_DT;
        cui__dspin_disc(g, c + cosf(t * 3.4f) * R, c + sinf(t * 3.4f) * R, 2.4f * k, fade);
        cui__dspin_disc(g, c + cosf(1 - t * 2.1f) * R * 0.5f, c + sinf(1 - t * 2.1f) * R * 0.5f, 1.6f * k, fade);
    }
    cui__dspin_disc(g, c, c, 1.6f * k, 1);
}

/* three dots bouncing one after another, each leaving a short trail */
static void cui__dspin_bounce(cui__dspin_grid *g) {
    float s = g->h / 28.0f;
    int n = cui__dspin_trail_len(0.8f);
    float fade = 1;
    for (int i = 0; i < n; i++, fade *= 0.8f) {
        float t = g->t - i * CUI__DSPIN_TRAIL_DT;
        for (int k = 0; k < 3; k++)
            cui__dspin_disc(g, g->w * 0.5f + (k - 1) * 10 * s, g->h - 5 * s - 15 * s * fabsf(sinf(t * 3.3f + k * 0.55f)), 2.7f * s, fade);
    }
}

/* a page with lines of text; a scan line runs down it, lighting the lines it has read */
static void cui__dspin_scan(cui__dspin_grid *g) {
    static const float lines[6][2] = { { 6, 0.9f }, { 10, 0.7f }, { 14, 0.85f }, { 18, 0.55f }, { 22, 0.8f }, { 26, 0.6f } };   /* row, length */
    float s = g->h / 32.0f;
    int x0 = (int)roundf(3 * s), x1 = g->w - 1 - x0, y0 = (int)roundf(s), y1 = g->h - 1 - y0;
    float sy = fmodf(g->t * 13 * s, g->h + 10 * s) - 5 * s;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            if (x == x0 || x == x1 || y == y0 || y == y1) cui__dspin_set(g, CUI__DSPIN_EDGE, x, y, 1);
            for (int k = 0; k < 6; k++) {
                float ly = lines[k][0] * s, lx0 = x0 + 3 * s, lx1 = lx0 + (x1 - x0 - 6 * s) * lines[k][1];
                if (y < ly || y >= ly + 2 * s || x <= lx0 || x >= lx1) continue;
                float u = (y - sy) / (2.5f * s);
                float d = y < sy ? 0.15f + 0.75f * expf((y - sy) / (5 * s)) : expf(-u * u);
                cui__dspin_set(g, CUI__DSPIN_TRACK, x, y, 1);
                cui__dspin_set(g, CUI__DSPIN_INK, x, y, d);
                break;
            }
            if (fabsf(y - sy) < 0.6f && x > x0 && x < x1) cui__dspin_max(g, CUI__DSPIN_INK, x, y, 0.75f);
        }
}

/* a soft disc that grows and shrinks, denser at its middle */
static void cui__dspin_pulse(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, b = 0.5f + 0.5f * sinf(g->t * 2.8f), R = (g->w * 0.5f - 0.5f) * (0.62f + 0.38f * b);
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float r = sqrtf((x - c) * (x - c) + (y - c) * (y - c));
            if (r < R) cui__dspin_set(g, CUI__DSPIN_INK, x, y, powf(1 - r / R, 0.7f) * (0.35f + 0.65f * b));
        }
}

/* three arms turning slowly inward, thinning toward the rim */
static void cui__dspin_spiral(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, R = g->w * 0.5f - 0.5f, hole = 1.5f * CUI_MAX(1.0f, g->w / 32.0f), f = 0.62f * 32.0f / g->w;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            float dx = x - c, dy = y - c, r = sqrtf(dx * dx + dy * dy);
            if (r > R || r < hole) continue;
            float v = 0.5f + 0.5f * sinf(atan2f(dy, dx) * 3 + r * f - g->t * 5);
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, v * powf(1 - r / R, 0.35f) * 1.15f);
        }
}

/* slow moving noise in a rounded tile */
static void cui__dspin_plasma(cui__dspin_grid *g) {
    float c = (g->w - 1) * 0.5f, f = 28.0f / g->w, rr = 5 / f, t = g->t;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            if (cui__dspin_rbox((float)x, (float)y, c, c, rr) > rr) continue;
            float X = x * f, Y = y * f, r = sqrtf((x - c) * (x - c) + (y - c) * (y - c)) * f;
            float v = sinf(X * 0.32f + t * 1.6f) + sinf(Y * 0.27f - t * 1.9f) + sinf((X + Y) * 0.19f + t) + sinf(r * 0.45f - t * 2.4f);
            cui__dspin_set(g, CUI__DSPIN_TRACK, x, y, 1);
            cui__dspin_set(g, CUI__DSPIN_INK, x, y, powf((v + 4) / 8, 1.4f));
        }
}

/* distance from (x, y) to the segment a-b */
static float cui__dspin_seg(float x, float y, float ax, float ay, float bx, float by) {
    float vx = bx - ax, vy = by - ay, k = cui__clampf(((x - ax) * vx + (y - ay) * vy) / (vx * vx + vy * vy), 0, 1);
    float ex = x - ax - vx * k, ey = y - ay - vy * k;
    return sqrtf(ex * ex + ey * ey);
}

/* a disc that dissolves in (`fin` its density) with a check (`kind` 1) or a cross (2) cut into
 * it in the alternate color once `mark` is set */
static void cui__dspin_mark(cui__dspin_grid *g, int kind, float fin, bool mark) {
    float c = (g->w - 1) * 0.5f, R = g->w * 0.5f - 0.5f, s = g->w / 28.0f, wide = 1.7f * s;
    static const float check[2][4] = { { -6, 0.5f, -1.5f, 5 }, { -1.5f, 5, 6.5f, -4.5f } };
    static const float cross[2][4] = { { -4.5f, -4.5f, 4.5f, 4.5f }, { 4.5f, -4.5f, -4.5f, 4.5f } };
    const float (*seg)[4] = kind == 1 ? check : cross;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            if ((x - c) * (x - c) + (y - c) * (y - c) > R * R) continue;
            bool on_mark = false;
            for (int k = 0; k < 2 && mark; k++)
                on_mark |= cui__dspin_seg((float)x, (float)y, c + seg[k][0] * s, c + seg[k][1] * s, c + seg[k][2] * s, c + seg[k][3] * s) < wide;
            cui__dspin_set(g, on_mark ? CUI__DSPIN_ALT : CUI__DSPIN_INK, x, y, fin);
        }
}

/* ---- painting ---- */

typedef struct {
    cui_spinner_style style;
    float t;                   /* seconds, in steps */
    float wake;                /* seconds to the next step; 0: nothing moves */
    float cell;                /* points */
    int mark;                  /* the status spinner: 0 the ring, 1 a check, 2 a cross */
    float fin;                 /* its disc's density */
    bool mark_on;              /* the check or cross shows in the disc */
    cui_color col[CUI__DSPIN_LAYERS];
} cui__dspin_paint;

static void cui__paint_dither_spinner(cui_ctx *ui, cui_node *n, void *data) {
    cui__dspin_paint *p = data;
    cui_rect r = n->rect;
    int cols = CUI_MAX(1, (int)roundf(r.w / p->cell)), rows = CUI_MAX(1, (int)roundf(r.h / p->cell));
    cui__dspin_grid g;
    g.w = cols; g.h = rows; g.t = p->t;
    float *buf = cui__frame_alloc(ui, sizeof(float) * (size_t)(cols * rows * CUI__DSPIN_LAYERS));
    memset(buf, 0, sizeof(float) * (size_t)(cols * rows * CUI__DSPIN_LAYERS));
    for (int k = 0; k < CUI__DSPIN_LAYERS; k++) g.v[k] = buf + (size_t)k * cols * rows;
    if (p->mark) cui__dspin_mark(&g, p->mark, p->fin, p->mark_on);
    else switch (p->style) {
    case CUI_SPINNER_COMET: cui__dspin_comet(&g); break;
    case CUI_SPINNER_DOT_MATRIX: cui__dspin_matrix(&g); break;
    case CUI_SPINNER_FILL_DRAIN: cui__dspin_fill(&g); break;
    case CUI_SPINNER_RADAR: cui__dspin_radar(&g); break;
    case CUI_SPINNER_RIPPLE: cui__dspin_ripple(&g); break;
    case CUI_SPINNER_UPLOAD: case CUI_SPINNER_DOWNLOAD: case CUI_SPINNER_TRANSFER: cui__dspin_transfer(&g, p->style); break;
    case CUI_SPINNER_ORBIT: cui__dspin_orbit(&g); break;
    case CUI_SPINNER_BOUNCE: cui__dspin_bounce(&g); break;
    case CUI_SPINNER_SCAN: cui__dspin_scan(&g); break;
    case CUI_SPINNER_PULSE: cui__dspin_pulse(&g); break;
    case CUI_SPINNER_SPIRAL: cui__dspin_spiral(&g); break;
    case CUI_SPINNER_PLASMA: cui__dspin_plasma(&g); break;
    default: break;
    }
    cui__dither d;
    memset(&d, 0, sizeof d);
    d.cell = p->cell;
    d.origin = (cui_vec2){ r.x + (r.w - cols * p->cell) * 0.5f, r.y + (r.h - rows * p->cell) * 0.5f };
    for (int k = 0; k < CUI__DSPIN_LAYERS; k++) {
        d.color = p->col[k];
        cui__draw_dither_cells(ui, &d, 0, 0, cols, rows, g.v[k]);
    }
    /* only a spinner on screen paints, so one out of view asks for no frames */
    if (p->wake > 0) cui__wakeup_after(ui, p->wake);
}

/* the cell for a spinner `h` points tall and `w` wide: one point inline, two from CUI__DSPIN_INLINE */
static float cui__dspin_cell(float w, float h) {
    float cell = h < CUI__DSPIN_INLINE ? 1.0f : 2.0f;
    return CUI_MAX(cell, ceilf(CUI_MAX(w, h) / CUI__DSPIN_MAX_CELLS));
}

/* steps the clock at `fps`: the time of this step, and in *wake the seconds to the next */
static float cui__dspin_clock(cui_ctx *ui, float fps, float *wake) {
    double steps = floor(ui->time * fps);
    *wake = (float)((steps + 1) / fps - ui->time);
    return (float)fmod(steps / fps, CUI__DSPIN_PERIOD);
}

static void cui__dspin_colors(cui_ctx *ui, cui__dspin_paint *p, cui_color color, cui_color track) {
    cui_color ink = cui__color_set(color) ? cui_color_of(ui, color) : CUI_C(ui, FOREGROUND);
    p->col[CUI__DSPIN_INK] = ink;
    p->col[CUI__DSPIN_TRACK] = cui__color_set(track) ? cui_color_of(ui, track) : cui_alpha(ink, 0.12f);
    p->col[CUI__DSPIN_EDGE] = cui_alpha(ink, 0.24f);
    p->col[CUI__DSPIN_ALT] = cui_alpha(ink, 0.7f);
}

static int cui__dspin_node(cui_ctx *ui, float w, float h, cui__dspin_paint *p) {
    int i = cui__leaf(ui, 0, CUI_PX(w), CUI_PX(h));
    p->cell = cui__dspin_cell(w, h);
    ui->nodes[i].paint = cui__paint_dither_spinner;
    ui->nodes[i].paint_data = p;
    ui->nodes[i].self_align = CUI_ALIGN_CENTER;
    return i;
}

void cui_spinner_ex(cui_ctx *ui, float size, const cui_spinner_opts *o) {
    cui_spinner_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    float h = size > 0 ? size : 20;
    int style = (int)o->style;
    if (style <= CUI_SPINNER_ARC || style >= CUI_COUNTOF(cui__dspin_infos)) {
        cui__spinner_node(ui, h, cui__color_set(o->color) ? cui_color_of(ui, o->color) : CUI_C(ui, TEXT_SECONDARY));
        return;
    }
    const cui__dspin_info *in = &cui__dspin_infos[style];
    cui__dspin_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    p->style = o->style;
    cui__dspin_colors(ui, p, o->color, o->track);
    if (ui->theme.motion > 0) p->t = cui__dspin_clock(ui, in->fps, &p->wake);
    else p->t = in->still;
    cui__dspin_node(ui, roundf(h * in->aspect), h, p);
}

/* ------------------------------------------------------------------------ */
/* Status spinner                                                           */
/* ------------------------------------------------------------------------ */

#define CUI__STATUS_IN 0.35f         /* seconds for the disc to dissolve in */
#define CUI__STATUS_MARK 0.2f        /* when the check or cross shows in it */
#define CUI__STATUS_HOLD 1.8f        /* when the disc starts to dissolve away */
#define CUI__STATUS_OUT 0.6f         /* how long that takes */
#define CUI__STATUS_FPS 30.0f

typedef struct { cui_status status; double since; bool init; } cui__status_spinner_state;

bool cui_status_spinner_ex(cui_ctx *ui, const char *id_str, cui_status status, const cui_status_spinner_opts *o) {
    cui_status_spinner_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, id_str);
    cui__status_spinner_state *st = cui_state(ui, id, sizeof *st);
    if (!st->init || st->status != status) { st->status = status; st->since = ui->time; st->init = true; }
    float h = o->size > 0 ? o->size : 20;
    cui__dspin_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    cui__dspin_colors(ui, p, o->color, CUI_TRANSPARENT);
    bool motion = ui->theme.motion > 0;
    bool shown = true;
    if (status == CUI_STATUS_BUSY) {
        p->style = CUI_SPINNER_COMET;
        if (motion) p->t = cui__dspin_clock(ui, CUI__STATUS_FPS, &p->wake);
        else p->t = cui__dspin_infos[CUI_SPINNER_COMET].still;
    } else if (status == CUI_STATUS_SUCCESS || status == CUI_STATUS_ERROR) {
        float e = (float)(ui->time - st->since), end = CUI__STATUS_HOLD + CUI__STATUS_OUT;
        p->mark = status == CUI_STATUS_SUCCESS ? 1 : 2;
        p->col[CUI__DSPIN_INK] = status == CUI_STATUS_SUCCESS ? CUI_C(ui, SUCCESS) : CUI_C(ui, DANGER);
        p->col[CUI__DSPIN_ALT] = CUI_C(ui, BACKGROUND);
        if (!motion) {
            /* at once, and gone at once when the hold is over */
            p->fin = 1; p->mark_on = true;
            if (!o->keep) { shown = e < end; p->wake = shown ? end - e : 0; }
        } else if (o->keep && e >= CUI__STATUS_IN) {
            p->fin = 1; p->mark_on = true;
        } else {
            /* the steps count from the change, so every status spinner plays the same frames */
            float t = floorf(e * CUI__STATUS_FPS) / CUI__STATUS_FPS, wake = t + 1 / CUI__STATUS_FPS - e;
            p->mark_on = t >= CUI__STATUS_MARK;
            if (t < CUI__STATUS_IN) { p->fin = t / CUI__STATUS_IN; p->wake = wake; }
            else if (t < CUI__STATUS_HOLD) { p->fin = 1; p->wake = CUI__STATUS_HOLD - e; }
            else if (t < end) { p->fin = 1 - (t - CUI__STATUS_HOLD) / CUI__STATUS_OUT; p->wake = wake; }
            else shown = false;
        }
        if (!shown) p->fin = 0;
    } else shown = false;
    int i = cui__dspin_node(ui, h, h, p);
    if (!shown) ui->nodes[i].paint = NULL;
    return shown;
}
