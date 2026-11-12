/* Voice: the voice waveform (a voice drawn as fine dots), the voice note and the
 * voice session. The composer's recorder (cui_ai.c) draws its strip with the
 * voice waveform too. None of them record or play anything: the app owns the
 * audio and passes levels, positions and states; the components draw them and
 * report what the user did. */
#include "cui_internal.h"


/* ------------------------------------------------------------------------ */
/* Voice waveform                                                           */
/* ------------------------------------------------------------------------ */

#define CUI__VW_ROWS 13            /* odd: one center row, six above and six below */
#define CUI__VW_PITCH 3.6f         /* default distance between dot centers */
#define CUI__VW_FADE_COLS 8.0f     /* a scrolling strip fades in over its oldest columns */
#define CUI__VW_MAX_LIVE 2048      /* more columns than any screen is wide */

static bool cui__vw_heard(float v) { return v >= 0; }   /* false for CUI_VOICE_SILENT, and for NaN */

/* The loudest level in column c of n over a clip of count levels, and whose it is.
 * A stretch with no audio stays silent (-1). */
static float cui__vw_bucket(const float *lv, const uint8_t *own, int count, int c, int n, int *owner) {
    int a = (int)((long long)c * count / n), b = (int)((long long)(c + 1) * count / n);
    if (b <= a) b = a + 1;
    if (b > count) b = count;
    float v = -1;
    *owner = own && a < count ? own[a] : 0;
    for (int i = a; i < b; i++)
        if (cui__vw_heard(lv[i]) && lv[i] > v) { v = lv[i]; if (own) *owner = own[i]; }
    return v;
}

int cui_voice_resample(const float *levels, int count, float *out, int columns) {
    if (!out || columns <= 0) return 0;
    for (int c = 0; c < columns; c++) {
        int owner;
        out[c] = levels && count > 0 ? cui__vw_bucket(levels, NULL, count, c, columns, &owner) : CUI_VOICE_SILENT;
    }
    return columns;
}

typedef struct {
    float *lv; uint8_t *own; int count;
    float pitch;
    bool clip, has_progress, show_head;
    float progress, hover_x;        /* hover_x < 0: no hover */
    cui_color voice[2];             /* lit dots: you, the reply */
    cui_color played, rest, unlit, head;
} cui__vw_paint;

typedef struct { bool dragging; } cui__vw_state;

/* the columns that fit in w, and the x of the first one's left edge, centered in the rect */
static int cui__vw_columns(cui_rect r, float pitch, float *left) {
    int n = (int)floorf(r.w / pitch);
    *left = r.x + (r.w - n * pitch) * 0.5f;
    return n;
}

static void cui__paint_voice_wave(cui_ctx *ui, cui_node *nd, void *data) {
    const cui__vw_paint *p = data;
    cui_rect r = nd->rect;
    float P = p->pitch, left;
    int n = cui__vw_columns(r, P, &left);
    if (n <= 0) return;
    const int half = CUI__VW_ROWS / 2;
    float cy = r.y + r.h * 0.5f, dot = P * 0.32f;   /* 2.3 points across at the default pitch */
    for (int c = 0; c < n; c++) {
        float x = left + (c + 0.5f) * P, v = -1, fade = 1;
        int owner = 0;
        if (p->clip) v = p->count > 0 ? cui__vw_bucket(p->lv, p->own, p->count, c, n, &owner) : -1;
        else {
            int i = p->count - n + c;   /* newest at the right; before the first level there is nothing yet */
            if (i >= 0) { v = cui__vw_heard(p->lv[i]) ? p->lv[i] : -1; owner = p->own ? p->own[i] : 0; }
            fade = CUI_MIN(1.0f, c / CUI__VW_FADE_COLS + 0.1f);
        }
        if (!cui__vw_heard(v)) { cui_draw_circle(ui, (cui_vec2){ x, cy }, dot, p->unlit); continue; }
        cui_color lit = p->voice[owner == CUI_VOICE_REPLY];
        if (p->has_progress) lit = (c + 0.5f) / n <= p->progress ? p->played : p->rest;
        /* each dot shows how much of it the level covers, so the edge of the shape is smooth */
        float reach = CUI_MIN(v, 1.0f) * (half + 0.5f);
        for (int d = 0; d <= half; d++) {
            float a = cui__clampf(reach - d + 0.5f, 0, 1);
            if (d == 0 && a < 1) cui_draw_circle(ui, (cui_vec2){ x, cy }, dot, p->unlit);
            if (a <= 0) break;
            cui_color col = cui_alpha(lit, a * fade);
            cui_draw_circle(ui, (cui_vec2){ x, cy - d * P }, dot, col);
            if (d) cui_draw_circle(ui, (cui_vec2){ x, cy + d * P }, dot, col);
        }
    }
    if (p->has_progress) {
        float w = 1.5f, span = n * P;
        if (p->hover_x >= left && p->hover_x <= left + span)
            cui_draw_rect(ui, (cui_rect){ p->hover_x - w * 0.5f, r.y, w, r.h }, CUI_CORNERS(w * 0.5f), cui_alpha(p->head, 0.3f));
        float hx = left + CUI_MIN(span, p->progress * span);
        if (p->show_head) cui_draw_rect(ui, (cui_rect){ hx - w * 0.5f, r.y, w, r.h }, CUI_CORNERS(w * 0.5f), p->head);
    }
}

/* seeking: a press or drag on the dots, and the arrow keys */
static bool cui__vw_seek(cui_ctx *ui, cui_id id, cui_interact r, float pitch, float step, float *progress) {
    cui__vw_state *st = cui_state(ui, id, sizeof *st);
    cui_rect pr = cui__prev_rect(ui, id);
    float before = *progress, left;
    int n = cui__vw_columns(pr, pitch, &left);
    if (r.hovered && ui->mpressed[0]) st->dragging = true;
    if (!ui->mdown[0]) st->dragging = false;
    if (st->dragging && n > 0) {
        *progress = cui__clampf((ui->mx - left) / (n * pitch), 0, 1);
        ui->active = id;
    }
    if (r.keyboard_focused) {
        if (cui_key_pressed(ui, CUI_KEY_RIGHT)) *progress = cui__clampf(*progress + step, 0, 1);
        if (cui_key_pressed(ui, CUI_KEY_LEFT)) *progress = cui__clampf(*progress - step, 0, 1);
        if (cui_key_pressed(ui, CUI_KEY_HOME)) *progress = 0;
        if (cui_key_pressed(ui, CUI_KEY_END)) *progress = 1;
    }
    if (r.hovered || st->dragging) cui__set_cursor(ui, CUI_CURSOR_HAND);
    return *progress != before;
}

/* the color of your voice: the foreground, or a quieter gray when it shares a timeline
 * with a reply under the neutral accent, where the reply is the foreground */
static cui_color cui__vw_you(cui_ctx *ui, bool shared) {
    return shared && ui->accent == CUI_ACCENT_NEUTRAL ? CUI_C(ui, TEXT_MUTED) : CUI_C(ui, FOREGROUND);
}

bool cui_voice_wave_ex(cui_ctx *ui, const char *id_str, const float *levels, int count, const cui_voice_wave_opts *o) {
    cui_voice_wave_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!levels) count = 0;
    count = CUI_MAX(0, count);
    float pitch = o->pitch > 0 ? CUI_CLAMP(o->pitch, 1.5f, 24.0f) : CUI__VW_PITCH;
    cui_id id = cui_make_id(ui, id_str);
    bool seekable = o->progress != NULL;
    cui_interact r = cui__interact(ui, id, seekable ? (CUI_IF_DRAG | CUI_IF_FOCUSABLE) : 0);
    bool changed = false, dragging = false;
    if (seekable) {
        changed = cui__vw_seek(ui, id, r, pitch, o->step > 0 ? o->step : 0.05f, o->progress);
        dragging = ((cui__vw_state *)cui_state(ui, id, sizeof(cui__vw_state)))->dragging;
    }
    int n = cui__leaf(ui, id, o->width.kind ? o->width : CUI_FILL, CUI_PX(pitch * (CUI__VW_ROWS + 1)));
    cui__keep_ltr(ui, n);   /* time runs left to right, like the other waveforms */
    cui__vw_paint *p = cui__frame_alloc(ui, sizeof *p);
    memset(p, 0, sizeof *p);
    /* a scrolling strip shows only its newest levels: a long session's whole history need not be copied */
    int first = !(o->clip || seekable) && count > CUI__VW_MAX_LIVE ? count - CUI__VW_MAX_LIVE : 0;
    count -= first;
    if (count) {
        p->lv = cui__frame_alloc(ui, sizeof(float) * (size_t)count);
        memcpy(p->lv, levels + first, sizeof(float) * (size_t)count);
        if (o->owners) {
            p->own = cui__frame_alloc(ui, (size_t)count);
            memcpy(p->own, o->owners + first, (size_t)count);
        }
    }
    p->count = count; p->pitch = pitch;
    p->clip = o->clip || seekable;
    p->has_progress = seekable;
    p->progress = seekable ? cui__clampf(*o->progress, 0, 1) : 0;
    if (!(p->progress >= 0)) p->progress = 0;   /* NaN */
    p->hover_x = seekable && r.hovered && !dragging ? ui->mx : -1;
    p->show_head = p->progress > 0 || r.hovered || r.focus_visible || dragging;   /* an unplayed clip has no playhead until it is reached for */
    bool custom = cui__color_set(o->color);
    p->voice[0] = custom ? cui_color_of(ui, o->color) : cui__vw_you(ui, o->owners != NULL);
    p->voice[1] = custom ? p->voice[0] : cui__emphasis(ui);
    if (!o->owners && o->owner == CUI_VOICE_REPLY) p->voice[0] = p->voice[1];
    p->played = custom ? p->voice[0] : cui__emphasis(ui);
    p->rest = CUI_C(ui, BORDER_STRONG);
    p->unlit = cui_alpha(CUI_C(ui, FOREGROUND), ui->mode == CUI_THEME_DARK ? 0.12f : 0.10f);
    p->head = cui_alpha(CUI_C(ui, FOREGROUND), 0.85f);
    cui_node *nd = &ui->nodes[n];
    nd->radius = CUI_CORNERS(CUI__R_SMALL);   /* for the focus ring */
    nd->paint = cui__paint_voice_wave;
    nd->paint_data = p;
    if (seekable) {
        cui__mark_interactive(ui, n, CUI_IF_DRAG | CUI_IF_FOCUSABLE);
        cui__focus_ring(ui, n, r.focus_visible ? 1.0f : 0.0f);
    }
    ui->last_id = id;
    return changed;
}

/* ------------------------------------------------------------------------ */
/* Voice note                                                               */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring press; } cui__vnote_state;

static float cui__vnote_speed(float s) { return s >= 1.75f ? 2.0f : s >= 1.25f ? 1.5f : 1.0f; }

/* the round play button: the foreground on a plain bubble, the accent on your own */
static bool cui__vnote_play(cui_ctx *ui, cui_id pid, cui__vnote_state *st, bool playing, bool mine) {
    cui_interact pr = cui__interact(ui, pid, CUI_IF_FOCUSABLE);
    if (st->press.x <= 0) st->press.x = 1;
    cui__spring(ui, &st->press, pr.pressed ? 0.9f : 1.0f, CUI_SPRING_SNAPPY);
    bool neutral = ui->accent == CUI_ACCENT_NEUTRAL;
    cui_color bg = mine ? cui__emphasis(ui) : CUI_C(ui, FOREGROUND);
    cui_color fg = mine && !neutral ? CUI_C(ui, ACCENT_FOREGROUND) : CUI_C(ui, BACKGROUND);
    int b = cui__node_new(ui, pid);
    cui_node *bn = &ui->nodes[b];
    bn->w = CUI_PX(34); bn->h = CUI_PX(34); bn->radius = CUI_CORNERS(17);
    bn->dir = CUI_STACK; bn->align = CUI_ALIGN_CENTER; bn->justify = CUI_JUSTIFY_CENTER;
    bn->bg = cui_mix(bg, CUI_C(ui, BACKGROUND), pr.hovered ? 0.12f : 0.0f);
    bn->scale = st->press.x;
    bn->flags |= CUI_NF_KEEP_W;
    cui__mark_interactive(ui, b, CUI_IF_FOCUSABLE);
    if (pr.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__open(ui, b);
    cui__icon_swap(ui, pid ^ 0x1u, playing ? CUI_ICON_PAUSE : CUI_ICON_PLAY, 16, fg);
    cui__close(ui);
    cui__focus_ring(ui, b, pr.focus_visible ? 1.0f : 0.0f);
    ui->last_id = pid;
    cui_tooltip(ui, playing ? "Pause" : "Play");
    return pr.clicked || pr.keyboard_activate;
}

/* the speed chip: 1x, 1.5x, 2x and around */
static bool cui__vnote_speed_chip(cui_ctx *ui, cui_id cid, float speed) {
    cui_interact cr = cui__interact(ui, cid, CUI_IF_FOCUSABLE);
    float hover = cui__hover_fade(ui, cid, cr.hovered, CUI__T_HOVER);
    int c = cui__node_new(ui, cid);
    cui_node *cn = &ui->nodes[c];
    cn->pad[0] = cn->pad[2] = 1; cn->pad[1] = cn->pad[3] = 6;
    cn->radius = CUI_CORNERS(CUI__R_SMALL);
    cn->bg = cui_alpha(CUI_C(ui, FOREGROUND), 0.07f + 0.05f * hover);
    cui__mark_interactive(ui, c, CUI_IF_FOCUSABLE);
    if (cr.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__open(ui, c);
    const char *label = speed == 2.0f ? "2\xc3\x97" : speed == 1.5f ? "1.5\xc3\x97" : "1\xc3\x97";
    int t = cui__text_node(ui, label, -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_xs - 1), CUI_C(ui, TEXT_SECONDARY), false);
    ui->nodes[t].flags |= CUI_NF_TABULAR; ui->nodes[t].line_height = 1.4f;
    cui__close(ui);
    cui__focus_ring(ui, c, cr.focus_visible ? 1.0f : 0.0f);
    ui->last_id = cid;
    cui_tooltip(ui, "Playback speed");
    return cr.clicked || cr.keyboard_activate;
}

int cui_voice_note_ex(cui_ctx *ui, const char *id_str, cui_voice_note *note, const cui_voice_note_opts *o) {
    cui_voice_note_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (!note) return 0;
    int result = 0;
    cui_id id = cui_make_id(ui, id_str);
    cui__vnote_state *st = cui_state(ui, id, sizeof *st);
    float dur = note->duration > 0 ? note->duration : 0;
    bool mine = o->mine;
    int root = cui__node_new(ui, id);
    cui_node *rn = &ui->nodes[root];
    rn->dir = CUI_ROW; rn->align = CUI_ALIGN_CENTER; rn->gap = 10;
    rn->pad[0] = rn->pad[2] = 8; rn->pad[3] = 8; rn->pad[1] = 12;
    cui__soft_width(rn, o->width, 330);
    rn->min_w = CUI_MAX(rn->min_w, 200);
    rn->radius = CUI_CORNERS(18);
    rn->border = 1;
    if (mine) { rn->bg = CUI_C(ui, ACCENT_SUBTLE); rn->border_color = cui_alpha(cui__emphasis(ui), 0.25f); rn->self_align = CUI_ALIGN_END; }
    else {
        rn->bg = ui->mode == CUI_THEME_DARK ? cui_mix(CUI_C(ui, SURFACE), CUI_C(ui, FOREGROUND), 0.04f) : CUI_C(ui, SURFACE_MUTED);
        rn->border_color = CUI_C(ui, BORDER_SUBTLE);
        rn->self_align = CUI__ALIGN_NOSTRETCH;
    }
    cui__open(ui, root);
    cui_push_id(ui, id_str);
    if (cui__vnote_play(ui, id ^ 0x9A1Bu, st, note->playing, mine)) { note->playing = !note->playing; result |= CUI_PLAYER_TOGGLE; }
    /* the clip is the seek bar */
    float frac = dur > 0 ? cui__clampf(note->position / dur, 0, 1) : 0;
    if (!(frac >= 0)) frac = 0;
    cui_voice_wave_opts wo;
    memset(&wo, 0, sizeof wo);
    wo.clip = true; wo.progress = &frac; wo.width = CUI_FILL;
    if (dur > 0) wo.step = CUI_MIN(1.0f, 1.0f / dur);   /* the arrows step a second */
    if (cui_voice_wave_ex(ui, "wave", note->levels, note->count, &wo)) { note->position = frac * dur; result |= CUI_PLAYER_SEEK; }
    /* the time and the speed */
    int meta = cui__node_new(ui, 0);
    cui_node *mn = &ui->nodes[meta];
    mn->dir = CUI_COLUMN; mn->align = CUI_ALIGN_END; mn->gap = 3;
    mn->flags |= CUI_NF_KEEP_W;
    cui__open(ui, meta);
    const char *played = cui__frame_printf(ui, "%s / %s", cui__mmss(ui, CUI_MIN(note->position, dur)), cui__mmss(ui, dur));
    int t = cui__text_node(ui, played, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_xs), CUI_C(ui, TEXT_SECONDARY), false);
    ui->nodes[t].flags |= CUI_NF_TABULAR; ui->nodes[t].line_height = 1.3f;
    cui__keep_ltr(ui, t);
    float speed = cui__vnote_speed(note->speed);
    if (cui__vnote_speed_chip(ui, id ^ 0x5BEEu, speed)) {
        note->speed = speed == 1.0f ? 1.5f : speed == 1.5f ? 2.0f : 1.0f;
        result |= CUI_PLAYER_SPEED;
    }
    cui__close(ui);
    cui_pop_id(ui);
    cui__close(ui);
    ui->last_id = id;
    return result;
}

/* ------------------------------------------------------------------------ */
/* Voice session                                                            */
/* ------------------------------------------------------------------------ */

/* the transcript: the speaker's name in the foreground, what was said in the
 * secondary color, and what is still to come dimmed */
typedef struct { int name_end, spoken_end; cui_color name, upcoming; } cui__vs_fx;

static cui_color cui__fx_voice_transcript(cui_ctx *ui, void *data, int byte, float x, float line_y, float *dy, cui_color c) {
    (void)ui; (void)x; (void)line_y; (void)dy;
    const cui__vs_fx *p = data;
    if (byte < p->name_end) return p->name;
    if (byte >= p->spoken_end) return p->upcoming;
    return c;
}

static const char *cui__vs_label(cui_voice_state s) {
    switch (s) {
    case CUI_VOICE_LISTENING: return "Listening";
    case CUI_VOICE_THINKING: return "Thinking\xe2\x80\xa6";
    case CUI_VOICE_SPEAKING: return "Speaking";
    case CUI_VOICE_MUTED: return "Muted";
    default: return "Waiting";
    }
}

/* the top line: what is happening, and whose color is whose */
static void cui__vs_header(cui_ctx *ui, cui_voice_state state, const cui_voice_session_opts *o, cui_color you, cui_color reply) {
    int top = cui__node_new(ui, 0);
    ui->nodes[top].dir = CUI_ROW; ui->nodes[top].align = CUI_ALIGN_CENTER; ui->nodes[top].gap = 8; ui->nodes[top].w = CUI_FILL;
    cui__open(ui, top);
    cui_color sc = state == CUI_VOICE_SPEAKING ? reply : state == CUI_VOICE_LISTENING ? CUI_C(ui, FOREGROUND)
                 : state == CUI_VOICE_THINKING ? CUI_C(ui, TEXT_SECONDARY) : CUI_C(ui, TEXT_MUTED);
    int dot = cui__leaf(ui, 0, CUI_PX(8), CUI_PX(8));
    ui->nodes[dot].radius = CUI_CORNERS(4); ui->nodes[dot].bg = sc;
    cui_color tc = state == CUI_VOICE_SPEAKING ? reply : state == CUI_VOICE_LISTENING ? CUI_C(ui, FOREGROUND) : CUI_C(ui, TEXT_SECONDARY);
    int lt = cui__text_node(ui, o->label ? o->label : cui__vs_label(state), -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_sm), tc, false);
    ui->nodes[lt].line_height = 1.3f;
    cui_spacer(ui);
    const char *names[2] = { o->you ? o->you : "You", o->assistant ? o->assistant : "Assistant" };
    cui_color cols[2] = { you, reply };
    int lg = cui__node_new(ui, 0);
    ui->nodes[lg].dir = CUI_ROW; ui->nodes[lg].align = CUI_ALIGN_CENTER; ui->nodes[lg].gap = 14;
    cui__open(ui, lg);
    for (int k = 0; k < 2; k++) {
        int it = cui__node_new(ui, 0);
        ui->nodes[it].dir = CUI_ROW; ui->nodes[it].align = CUI_ALIGN_CENTER; ui->nodes[it].gap = 6;
        cui__open(ui, it);
        int sw = cui__leaf(ui, 0, CUI_PX(8), CUI_PX(8));
        ui->nodes[sw].radius = CUI_CORNERS(2.5f); ui->nodes[sw].bg = cols[k];
        int nt = cui__text_node(ui, names[k], -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_xs), CUI_C(ui, TEXT_MUTED), false);
        ui->nodes[nt].line_height = 1.3f;
        cui__close(ui);
    }
    cui__close(ui);
    cui__close(ui);
}

static void cui__vs_transcript(cui_ctx *ui, const cui_voice_session_opts *o) {
    const char *name = o->speaker == CUI_VOICE_REPLY ? (o->assistant ? o->assistant : "Assistant") : (o->you ? o->you : "You");
    const char *said = o->transcript ? o->transcript : "", *rest = o->upcoming ? o->upcoming : "";
    const char *line = cui__frame_printf(ui, "%s \xc2\xb7 %s%s", name, said, rest);
    cui__vs_fx *fx = cui__frame_alloc(ui, sizeof *fx);
    fx->name_end = (int)strlen(name);
    fx->spoken_end = fx->name_end + 4 + (int)strlen(said);   /* " · " is four bytes */
    fx->name = CUI_C(ui, FOREGROUND);
    fx->upcoming = cui_alpha(CUI_C(ui, TEXT_SECONDARY), 0.45f);
    float size = cui__fs(ui, ui->theme.text_sm + 0.5f);
    int t = cui__text_node(ui, line, -1, CUI_FONT_BODY, size, CUI_C(ui, TEXT_SECONDARY), true);
    cui_node *tn = &ui->nodes[t];
    tn->w = CUI_FILL; tn->line_height = 1.5f;
    tn->min_h = size * 1.5f * 2;   /* two lines kept, so the buttons stay put as a turn grows */
    tn->text_fx = cui__fx_voice_transcript;
    tn->text_fx_data = fx;
}

int cui_voice_session_ex(cui_ctx *ui, const char *id_str, cui_voice_state state, const float *levels, int count, const cui_voice_session_opts *o) {
    cui_voice_session_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    int result = 0;
    cui_id id = cui_make_id(ui, id_str);
    int root = cui__node_new(ui, id);
    cui_node *rn = &ui->nodes[root];
    rn->dir = CUI_COLUMN; rn->gap = 12;
    rn->w = o->width.kind ? o->width : CUI_FILL;
    cui__open(ui, root);
    cui_push_id(ui, id_str);
    cui_color you = cui__vw_you(ui, true), reply = cui__emphasis(ui);
    cui__vs_header(ui, state, o, you, reply);
    cui_voice_wave_opts wo;
    memset(&wo, 0, sizeof wo);
    wo.owners = o->owners;
    if (!o->owners) wo.color = you;
    cui_voice_wave_ex(ui, "wave", levels, count, &wo);
    if (o->transcript || o->upcoming) cui__vs_transcript(ui, o);
    if (o->muted || !o->no_end) {
        int ctl = cui__node_new(ui, 0);
        ui->nodes[ctl].dir = CUI_ROW; ui->nodes[ctl].gap = 10; ui->nodes[ctl].w = CUI_FILL;
        ui->nodes[ctl].justify = CUI_JUSTIFY_CENTER; ui->nodes[ctl].align = CUI_ALIGN_CENTER;
        cui__open(ui, ctl);
        cui_button_opts bo;
        memset(&bo, 0, sizeof bo);
        bo.variant = CUI_BUTTON_OUTLINE;
        if (o->muted) {
            bo.selected = *o->muted;
            if (cui_button_ex(ui, *o->muted ? "Unmute###mute" : "Mute###mute", &bo)) { *o->muted = !*o->muted; result |= CUI_VOICE_SESSION_MUTE; }
        }
        if (!o->no_end) {
            bo.selected = false;
            bo.color = CUI_COLOR_DANGER;
            if (cui_button_ex(ui, "End###end", &bo)) result |= CUI_VOICE_SESSION_END;
        }
        cui__close(ui);
    }
    cui_pop_id(ui);
    cui__close(ui);
    ui->last_id = id;
    return result;
}
