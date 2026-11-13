/* Disclosure and feedback: hover card, confirm popover, expandable text and
 * card, table of contents, banner, countdown and onboarding checklist. */
#include "cui_internal.h"


/* ------------------------------------------------------------------------ */
/* Hover card                                                               */
/* ------------------------------------------------------------------------ */

typedef struct { double enter_t, leave_t; cui_id anchor; } cui__hc_state;

bool cui_hover_card_begin(cui_ctx *ui, const char *id_str, const cui_hover_card_opts *o) {
    cui_hover_card_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    float delay = o->delay > 0 ? o->delay : 0.45f;
    cui_id pid = cui_make_id(ui, id_str) ^ 0x4C4Du;
    cui__hc_state *st = cui_state(ui, pid ^ 0x1u, sizeof *st);
    cui_id anchor = ui->last_id;
    cui_popup *p = cui__popup_get(ui, pid, true);
    if (!p) return false;
    bool on_anchor = anchor && ui->hot == anchor;
    bool on_card = p->open && (ui->hot_layer == pid);
    if (on_anchor || on_card) {
        if (st->enter_t == 0) st->enter_t = ui->time;
        st->leave_t = 0;
    } else {
        st->enter_t = 0;
        if (st->leave_t == 0) st->leave_t = ui->time;
    }
    if (!p->open && on_anchor) {
        double waited = ui->time - st->enter_t;
        if (waited >= delay && !ui->mdown[0]) { cui__popup_set(ui, pid, true); p->at_mouse = false; }
        else cui__wakeup_after(ui, (float)(delay - waited) + 0.01f);
    }
    if (p->open && !on_anchor && !on_card) {
        double gone = ui->time - st->leave_t;
        if (gone > 0.2) p->open = false;   /* a short grace period lets the pointer cross the gap */
        else cui__wakeup_after(ui, (float)(0.2 - gone) + 0.01f);
    }
    if (p->open && on_anchor && ui->mpressed[0]) p->open = false;   /* clicking the anchor means something else */
    return cui__popup_begin(ui, pid, anchor, o->placement, o->width > 0 ? o->width : 300, 16, 10, false);
}

void cui_hover_card_end(cui_ctx *ui) { cui__popup_end(ui); }

/* ------------------------------------------------------------------------ */
/* Confirm popover                                                          */
/* ------------------------------------------------------------------------ */

/** Require a fresh Enter when an activation opens the question in this frame. */
bool cui_confirm_popover_ex(cui_ctx *ui, const char *id, const cui_confirm_popover_opts *o) {
    cui_confirm_popover_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id pid = cui_make_id(ui, id);   /* resolved out here: the popover pushes its own id scope */
    cui_popup *p = cui__popup_get(ui, pid, false);
    bool just_opened = p && p->open && ui->frame - p->opened_frame <= 1;
    cui_popover_opts po;
    memset(&po, 0, sizeof po);
    po.placement = o->placement;
    po.width = 290;
    po.pad = 16;
    po.gap = 6;
    if (!cui_popover_begin(ui, id, &po)) return false;
    if (p && p->opened_frame == ui->frame) ui->kpressed[CUI_KEY_ENTER] = 0;
    bool confirmed = false;
    if (o->title) {
        int t = cui__text_node(ui, o->title, -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body), CUI_C(ui, FOREGROUND), true);
        ui->nodes[t].line_height = 1.4f;
    }
    if (o->description) {
        int t = cui__text_node(ui, o->description, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_SECONDARY), true);
        ui->nodes[t].line_height = 1.45f;
    }
    int row = cui__node_new(ui, 0);
    ui->nodes[row].dir = CUI_ROW; ui->nodes[row].gap = 8; ui->nodes[row].justify = CUI_JUSTIFY_END;
    ui->nodes[row].w = CUI_FILL; ui->nodes[row].pad[0] = 8;
    cui__open(ui, row);
    cui_button_opts bo;
    memset(&bo, 0, sizeof bo);
    bo.size = CUI_SIZE_SM;
    if (cui_button_ex(ui, o->cancel_label ? o->cancel_label : "Cancel", &bo)) cui__popup_set(ui, pid, false);
    bo.variant = o->danger ? CUI_BUTTON_DANGER : CUI_BUTTON_PRIMARY;
    if (cui_button_ex(ui, o->confirm_label ? o->confirm_label : "Confirm", &bo)) confirmed = true;
    /* move focus into the question so Enter answers it instead of re-pressing the trigger */
    if (just_opened) ui->focus = ui->last_id;
    cui__close(ui);
    if (!ui->edit && cui_key_pressed(ui, CUI_KEY_ENTER) && p && p->open) { confirmed = true; ui->kpressed[CUI_KEY_ENTER] = 0; }
    cui_popover_end(ui);
    if (confirmed) cui__popup_set(ui, pid, false);
    return confirmed;
}

/* ------------------------------------------------------------------------ */
/* Expandable text                                                          */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring h; bool open, init; } cui__et_state;
typedef struct { float fade; float line_h; cui_color c; } cui__fade_paint;

static void cui__paint_text_fade(cui_ctx *ui, cui_node *n, void *data) {
    cui__fade_paint *p = data;
    if (p->fade <= 0.01f) return;
    cui_rect r = n->rect;
    /* the last line dims but stays legible, descenders included */
    float h = CUI_MIN(r.h, p->line_h * 1.3f);
    cui_color clear = p->c;
    clear.a = 0;
    cui_draw_rect_gradient(ui, (cui_rect){ r.x, r.y + r.h - h, r.w, h }, CUI_CORNERS(0), clear, cui_alpha(p->c, p->fade * 0.82f));
}

bool cui_expandable_text_ex(cui_ctx *ui, const char *id_str, const char *text, const cui_expandable_text_opts *o) {
    cui_expandable_text_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    int lines = o->lines > 0 ? o->lines : 3;
    float size = cui__fs(ui, o->size > 0 ? o->size : ui->theme.text_body);
    float lh = size * 1.45f;
    cui_id id = cui_make_id(ui, id_str);
    cui_id clip_id = id ^ 0xC11Fu;
    cui__et_state *st = cui_state(ui, id, sizeof *st);
    cui_rect pr = cui__prev_rect(ui, clip_id);
    int total = lines;
    if (pr.w > 0) total = cui__text_wrap(ui, CUI_FONT_BODY, size, text, (int)strlen(text), pr.w, NULL, 0);
    bool expandable = total > lines;
    float clamp_h = lines * lh, full_h = total * lh;
    float target = (st->open || !expandable) ? full_h : clamp_h;
    if (!st->init) { st->h.x = target; st->init = true; }
    cui__spring(ui, &st->h, target, CUI_SPRING_SMOOTH);
    bool toggled = false;
    int col = cui__node_new(ui, id);
    ui->nodes[col].gap = 6;
    ui->nodes[col].w = CUI_FILL;
    cui__open(ui, col);
    int clip = cui__node_new(ui, clip_id);
    cui_node *cn = &ui->nodes[clip];
    cn->w = CUI_FILL;
    cn->flags |= CUI_NF_CLIP | CUI_NF_PAINT_AFTER;
    if (expandable || pr.w <= 0) cn->h = CUI_PX(CUI_MAX(lh, st->h.x) + (expandable ? 2 * (1.0f - (st->open ? 1.0f : 0.0f)) : 0));
    cui__fade_paint *fp = cui__frame_alloc(ui, sizeof *fp);
    float openness = full_h > clamp_h ? cui__clampf((st->h.x - clamp_h) / (full_h - clamp_h), 0, 1) : 1;
    fp->fade = expandable ? 1.0f - openness : 0;
    fp->line_h = lh;
    fp->c = cui__color_set(o->fade_to) ? cui_color_of(ui, o->fade_to) : CUI_C(ui, BACKGROUND);
    cn->paint = cui__paint_text_fade;
    cn->paint_data = fp;
    cui__open(ui, clip);
    int t = cui__text_node(ui, text, -1, CUI_FONT_BODY, size, cui__color_set(o->color) ? cui_color_of(ui, o->color) : CUI_C(ui, FOREGROUND), true);
    ui->nodes[t].line_height = 1.45f;
    ui->nodes[t].w = CUI_FILL;
    cui__close(ui);
    if (expandable) {
        cui_push_id(ui, id_str);
        if (cui_link(ui, st->open ? "Show less" : "Show more")) { st->open = !st->open; toggled = true; }
        cui_pop_id(ui);
    }
    cui__close(ui);
    ui->last_id = id;
    return toggled;
}

/* ------------------------------------------------------------------------ */
/* Expandable card                                                          */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring t; float hover, content_h; bool init; } cui__ec_state;

bool cui_expandable_card_begin(cui_ctx *ui, const char *title, bool *open, const cui_expandable_card_opts *o) {
    cui_expandable_card_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, title) ^ 0xEC4Du;
    cui__ec_state *st = cui_state(ui, id, sizeof *st);
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    if (r.clicked) *open = !*open;
    if (!st->init) { st->t.x = *open ? 1.0f : 0.0f; st->init = true; }
    cui__spring(ui, &st->t, *open ? 1.0f : 0.0f, CUI__SPRING_EXPAND);
    cui__tween(ui, &st->hover, r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
    float t = cui__clampf(st->t.x, 0, 1);
    int wrap = cui__node_new(ui, 0);
    cui_node *wn = &ui->nodes[wrap];
    wn->w = CUI_FILL; wn->self_align = CUI_ALIGN_STRETCH;
    wn->bg = CUI_C(ui, SURFACE);
    wn->border = 1;
    wn->border_color = cui_mix(CUI_C(ui, BORDER), CUI_C(ui, BORDER_STRONG), cui__clampf(st->hover + t * 0.6f, 0, 1));
    wn->radius = CUI_CORNERS(ui->theme.radius_control + 2);
    wn->flags |= CUI_NF_CLIP;
    wn->elevation = CUI_ELEVATION_RESTING;
    wn->elevation_alpha = t;
    cui__open(ui, wrap);
    int head = cui__node_new(ui, id);
    cui_node *hn = &ui->nodes[head];
    hn->dir = CUI_ROW; hn->align = CUI_ALIGN_CENTER; hn->gap = 12;
    hn->pad[0] = hn->pad[2] = 14; hn->pad[1] = hn->pad[3] = 16;
    hn->w = CUI_FILL;
    hn->bg = cui_alpha(CUI_C(ui, HOVER), st->hover * 0.6f);
    cui__mark_interactive(ui, head, CUI_IF_FOCUSABLE);
    if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
    cui__open(ui, head);
    if (o->icon) {
        int ib = cui__node_new(ui, 0);
        cui_node *in = &ui->nodes[ib];
        in->w = CUI_PX(20); in->h = CUI_PX(36);   /* a bare icon: no tile behind it */
        in->dir = CUI_STACK; in->align = CUI_ALIGN_CENTER; in->justify = CUI_JUSTIFY_CENTER;
        cui__open(ui, ib);
        cui__icon_node(ui, o->icon, 18, CUI_C(ui, TEXT_SECONDARY));
        cui__close(ui);
    }
    int col = cui__node_new(ui, 0);
    ui->nodes[col].w = CUI_FILL; ui->nodes[col].gap = 2;
    cui__open(ui, col);
    int tt = cui__text_node(ui, title, cui__display_len(title), CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body), CUI_C(ui, FOREGROUND), true);
    ui->nodes[tt].line_height = 1.35f;
    if (o->description) {
        int dt = cui__text_node(ui, o->description, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_SECONDARY), true);
        ui->nodes[dt].line_height = 1.4f;
    }
    cui__close(ui);
    if (o->badge) {
        cui_badge_opts bo;
        memset(&bo, 0, sizeof bo);
        bo.tone = o->badge_tone; bo.size = CUI_SIZE_SM;
        cui_badge_ex(ui, o->badge, &bo);
    }
    cui__chevron_node(ui, t * CUI_PI, 16, cui_mix(CUI_C(ui, TEXT_MUTED), CUI_C(ui, FOREGROUND), st->hover));
    cui__close(ui);
    cui__focus_ring(ui, head, r.focus_visible ? 1.0f : 0.0f);
    if (t <= 0.001f && !*open) { cui__close(ui); return false; }
    int line = cui__leaf(ui, 0, CUI_FILL, CUI_PX(1));
    ui->nodes[line].bg = cui_alpha(CUI_C(ui, BORDER), t);
    cui_id cid = id ^ 0xC0u;
    cui_rect cr = cui__prev_rect(ui, cid);
    if (cr.h > 0) st->content_h = cr.h;
    int clip = cui__node_new(ui, 0);
    cui_node *cn = &ui->nodes[clip];
    cn->flags |= CUI_NF_CLIP;
    cn->w = CUI_FILL;
    if (t < 0.999f && st->content_h > 0) cn->h = CUI_PX(st->content_h * t);
    cn->opacity = CUI_MAX(0.001f, t);
    cui__open(ui, clip);
    int content = cui__node_new(ui, cid);
    cui_node *ct = &ui->nodes[content];
    ct->gap = 10; ct->w = CUI_FILL;
    ct->pad[0] = 14; ct->pad[1] = ct->pad[3] = 16; ct->pad[2] = 16;
    ct->ty = (1.0f - t) * -6;
    cui__open(ui, content);
    bool closing = !*open;
    if (ui->acc_depth < 32) ui->acc_dis[ui->acc_depth] = closing;
    ui->acc_depth++;
    if (closing) cui_push_disabled(ui, true);
    return true;
}

void cui_expandable_card_end(cui_ctx *ui) {
    if (ui->acc_depth > 0) {
        ui->acc_depth--;
        if (ui->acc_depth < 32 && ui->acc_dis[ui->acc_depth]) cui_pop_disabled(ui);
    }
    cui__close(ui);   /* content */
    cui__close(ui);   /* clip */
    cui__close(ui);   /* card */
}

/* ------------------------------------------------------------------------ */
/* Table of contents                                                        */
/* ------------------------------------------------------------------------ */

static cui_id cui__anchor_id(const char *anchor) { return cui__hash_str(anchor, 0x70C0A7u); }

void cui_toc_anchor(cui_ctx *ui, const char *anchor) {
    int n = cui__leaf(ui, cui__anchor_id(anchor), CUI_FILL, CUI_PX(0));
    ui->nodes[n].self_align = CUI_ALIGN_STRETCH;
}

typedef struct { cui_spring y, h; float progress; bool init; float hover[64]; } cui__toc_state;
typedef struct { float y, h, progress, rail_h; cui_color track, fill, mark; } cui__toc_paint;

static void cui__paint_toc_rail(cui_ctx *ui, cui_node *n, void *data) {
    cui__toc_paint *p = data;
    cui_rect r = n->rect;
    cui_draw_rect(ui, (cui_rect){ r.x, r.y, 1, r.h }, CUI_CORNERS(0), p->track);
    cui_draw_rect(ui, (cui_rect){ r.x, r.y, 1, r.h * cui__clampf(p->progress, 0, 1) }, CUI_CORNERS(0), p->fill);
    if (p->h > 0) cui_draw_rect(ui, (cui_rect){ r.x - 0.5f, r.y + p->y, 2, p->h }, CUI_CORNERS(1), p->mark);
}

int cui_toc(cui_ctx *ui, const char *scroll_id, const cui_toc_item *items, int count) {
    if (count > 64) count = 64;
    cui_id scid = cui_make_id(ui, scroll_id);
    cui_id tid = scid ^ 0x70CCu;
    cui__toc_state *st = cui_state(ui, tid, sizeof *st);
    cui_scroll_state *ss = cui_state(ui, scid, sizeof *ss);
    cui_rect view = cui__prev_rect(ui, scid);
    float range = ss->content_h - ss->view_h;
    float progress = range > 1 ? cui__clampf(ss->y / range, 0, 1) : 0;
    int active = 0;
    for (int k = 0; k < count; k++) {
        cui_id aid = cui__anchor_id(items[k].anchor);
        if (!cui__has_prev_rect(ui, aid)) continue;
        cui_rect ar = cui__prev_rect(ui, aid);
        if (ar.y - view.y <= view.h * 0.3f + 1) active = k;
    }
    if (range > 1 && progress >= 0.995f) active = count - 1;
    cui__tween(ui, &st->progress, progress, 0.08f);
    int clicked = -1;
    int row = cui__node_new(ui, tid);
    ui->nodes[row].dir = CUI_ROW; ui->nodes[row].gap = 14; ui->nodes[row].align = CUI_ALIGN_STRETCH;
    cui__open(ui, row);
    int rail = cui__leaf(ui, 0, CUI_PX(1), CUI_FILL);
    int list = cui__node_new(ui, 0);
    ui->nodes[list].gap = 2;
    ui->nodes[list].w = CUI_FILL;
    cui__open(ui, list);
    cui_rect rail_r = cui__prev_rect(ui, tid);
    float mark_y = 0, mark_h = 0;
    bool have_mark = false;
    for (int k = 0; k < count; k++) {
        cui_id iid = cui__hash(&k, sizeof k, tid);
        cui_interact r = cui__interact(ui, iid, CUI_IF_FOCUSABLE);
        cui__tween(ui, &st->hover[k], r.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
        if (r.clicked) clicked = k;
        bool on = k == active;
        int it = cui__node_new(ui, iid);
        cui_node *in = &ui->nodes[it];
        in->dir = CUI_ROW; in->align = CUI_ALIGN_CENTER;
        in->pad[0] = in->pad[2] = 5;
        in->pad[3] = 12.0f * (float)CUI_CLAMP(items[k].level, 0, 4);
        in->w = CUI_FILL;
        cui__mark_interactive(ui, it, CUI_IF_FOCUSABLE);
        if (r.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
        cui__open(ui, it);
        cui_color fg = on ? CUI_C(ui, FOREGROUND) : cui_mix(CUI_C(ui, TEXT_SECONDARY), CUI_C(ui, FOREGROUND), st->hover[k] * 0.7f);
        int t = cui__text_node(ui, items[k].title, -1, on ? CUI_FONT_MEDIUM : CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), fg, true);
        ui->nodes[t].line_height = 1.4f;
        ui->nodes[t].w = CUI_FILL;
        cui__close(ui);
        cui__focus_ring(ui, it, r.focus_visible ? 1.0f : 0.0f);
        if (on && cui__has_prev_rect(ui, iid) && rail_r.h > 0) {
            cui_rect ir = cui__prev_rect(ui, iid);
            mark_y = ir.y - rail_r.y; mark_h = ir.h;
            have_mark = true;
        }
    }
    cui__close(ui);
    cui__close(ui);
    if (have_mark) {
        if (!st->init) { st->y.x = mark_y; st->h.x = mark_h; st->init = true; }
        cui__spring(ui, &st->y, mark_y, CUI_SPRING_MORPH);
        cui__spring(ui, &st->h, mark_h, CUI_SPRING_MORPH);
    }
    cui__toc_paint *p = cui__frame_alloc(ui, sizeof *p);
    p->y = st->y.x; p->h = st->init ? st->h.x : 0; p->progress = st->progress;
    p->track = CUI_C(ui, BORDER);
    p->fill = CUI_C(ui, BORDER_STRONG);
    p->mark = cui__emphasis(ui);
    ui->nodes[rail].paint = cui__paint_toc_rail;
    ui->nodes[rail].paint_data = p;
    if (clicked >= 0) {
        cui_id aid = cui__anchor_id(items[clicked].anchor);
        if (cui__has_prev_rect(ui, aid) && view.h > 0) {
            cui_rect ar = cui__prev_rect(ui, aid);
            float y = ar.y - view.y + ss->y - 12;
            cui_scroll_to(ui, scroll_id, CUI_MAX(0.0f, y));
        }
        active = clicked;
    }
    ui->last_id = tid;
    return active;
}

/* ------------------------------------------------------------------------ */
/* Banner                                                                   */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring t; float h; bool init; } cui__banner_state;
typedef struct { cui_color c; } cui__line_paint;
static void cui__paint_bottom_line(cui_ctx *ui, cui_node *n, void *data) {
    cui__line_paint *p = data;
    cui_rect r = n->rect;
    cui_draw_rect(ui, (cui_rect){ r.x, r.y + r.h - 1, r.w, 1 }, CUI_CORNERS(0), p->c);
}

bool cui_banner_ex(cui_ctx *ui, const char *text, bool *open, const cui_banner_opts *o) {
    cui_banner_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    bool shown = open ? *open : true;
    cui_id id = cui_make_id(ui, text) ^ 0xBA22u;
    cui__banner_state *st = cui_state(ui, id, sizeof *st);
    if (!st->init) { st->t.x = shown ? 1.0f : 0.0f; st->init = true; }
    cui__spring(ui, &st->t, shown ? 1.0f : 0.0f, CUI__SPRING_EXPAND);
    float t = cui__clampf(st->t.x, 0, 1);
    if (t <= 0.002f && !shown) return false;
    cui_id inner_id = id ^ 0x1u;
    cui_rect ir = cui__prev_rect(ui, inner_id);
    if (ir.h > 0) st->h = ir.h;
    cui_tone tone = o->tone ? o->tone : CUI_TONE_ACCENT;
    cui_color tc = cui__tone_color(ui, tone);
    int clip = cui__node_new(ui, id);
    cui_node *cn = &ui->nodes[clip];
    cn->w = CUI_FILL; cn->self_align = CUI_ALIGN_STRETCH;
    cn->flags |= CUI_NF_CLIP;
    if (t < 0.999f && st->h > 0) cn->h = CUI_PX(st->h * t);
    cui__open(ui, clip);
    int row = cui__node_new(ui, inner_id);
    cui_node *rn = &ui->nodes[row];
    rn->dir = CUI_ROW; rn->align = CUI_ALIGN_CENTER; rn->gap = 10;
    rn->w = CUI_FILL;
    rn->pad[0] = rn->pad[2] = 9; rn->pad[1] = 10; rn->pad[3] = 16;
    bool neutral = tone == CUI_TONE_ACCENT && ui->accent == CUI_ACCENT_NEUTRAL;   /* a gray tint reads heavy: keep it light */
    rn->bg = cui_mix(CUI_C(ui, SURFACE), tc, ui->mode == CUI_THEME_DARK ? (neutral ? 0.1f : 0.16f) : (neutral ? 0.045f : 0.09f));
    rn->opacity = CUI_MAX(0.001f, t);
    cui__line_paint *lp = cui__frame_alloc(ui, sizeof *lp);
    lp->c = cui_mix(CUI_C(ui, BORDER), tc, 0.25f);
    rn->paint = cui__paint_bottom_line;
    rn->paint_data = lp;
    cui__open(ui, row);
    /* on a narrow screen the message wraps; when that would leave it a sliver the
       action moves under it and the banner reads left to right */
    float fsm = cui__fs(ui, ui->theme.text_sm);
    float aw = o->action ? cui__text_width(ui, CUI_FONT_MEDIUM, fsm, o->action, -1, false) + 40 : 0;
    bool stack = o->action && ir.w > 0 && ir.w - 26 - 10 - aw - (o->icon ? 26 : 0) - (o->dismissible ? 34 : 0) < 170;
    if (stack) rn->align = CUI_ALIGN_START;
    else cui_spacer(ui);
    cui_color fg = cui_mix(CUI_C(ui, FOREGROUND), tc, ui->mode == CUI_THEME_DARK ? 0.25f : 0.35f);
    if (o->icon) cui__icon_node(ui, o->icon, 16, tc);
    int body = -1;
    if (stack) {
        body = cui__node_new(ui, 0);
        ui->nodes[body].w = CUI_FILL; ui->nodes[body].gap = 2; ui->nodes[body].align = CUI_ALIGN_START;
        cui__open(ui, body);
    }
    int tt = cui__text_node(ui, text, cui__display_len(text), CUI_FONT_MEDIUM, fsm, fg, true);
    ui->nodes[tt].line_height = 1.35f;
    bool action = false;
    if (o->action) {
        int keep = cui__node_new(ui, 0);
        ui->nodes[keep].flags |= CUI_NF_KEEP_W;
        cui__open(ui, keep);
        cui_push_id(ui, text);
        cui_button_opts bo;
        memset(&bo, 0, sizeof bo);
        bo.size = CUI_SIZE_SM; bo.variant = CUI_BUTTON_LINK; bo.icon_right = CUI_ICON_ARROW_RIGHT;
        bo.color = tc;
        action = cui_button_ex(ui, o->action, &bo);
        cui_pop_id(ui);
        cui__close(ui);
    }
    if (stack) cui__close(ui);
    else cui_spacer(ui);
    if (o->dismissible && open) {
        cui_push_id(ui, text);
        cui_button_opts xo;
        memset(&xo, 0, sizeof xo);
        xo.size = CUI_SIZE_SM; xo.tooltip = "Dismiss";
        if (cui_icon_button_ex(ui, CUI_ICON_X, &xo)) *open = false;
        ui->nodes[ui->last_node].w = CUI_PX(24); ui->nodes[ui->last_node].h = CUI_PX(24);
        cui_pop_id(ui);
    }
    cui__close(ui);
    cui__close(ui);
    ui->last_id = id;
    return action;
}

/* ------------------------------------------------------------------------ */
/* Countdown                                                                */
/* ------------------------------------------------------------------------ */

typedef struct { int shown, prev; float t; bool init; } cui__roll_state;

/* Two-digit tile whose digits roll when the value changes. */
static void cui__roll_tile(cui_ctx *ui, cui_id sid, int value, float w, float h, float fs, bool compact) {
    cui__roll_state *st = cui_state(ui, sid, sizeof *st);
    if (!st->init) { st->shown = st->prev = value; st->t = 1; st->init = true; }
    if (value != st->shown) { st->prev = st->shown; st->shown = value; st->t = 0; }
    cui__tween(ui, &st->t, 1.0f, 0.3f);
    int box = cui__node_new(ui, 0);
    cui_node *bn = &ui->nodes[box];
    bn->w = CUI_PX(w); bn->h = CUI_PX(h);
    bn->radius = CUI_CORNERS(compact ? 8 : 12);
    bn->bg = CUI_C(ui, SURFACE);
    bn->border = 1; bn->border_color = CUI_C(ui, BORDER);
    bn->dir = CUI_STACK; bn->align = CUI_ALIGN_CENTER; bn->justify = CUI_JUSTIFY_CENTER;
    bn->flags |= CUI_NF_CLIP;
    cui__open(ui, box);
    float travel = h * 0.6f;
    if (st->t < 1) {
        int a = cui__text_node(ui, cui__frame_printf(ui, "%02d", st->prev), -1, CUI_FONT_DISPLAY, fs, CUI_C(ui, FOREGROUND), false);
        ui->nodes[a].flags |= CUI_NF_TABULAR; ui->nodes[a].line_height = 1.0f;
        ui->nodes[a].ty = -travel * st->t;   /* counting down: old digits leave upward */
        ui->nodes[a].opacity = CUI_MAX(0.001f, 1 - st->t);
    }
    int b = cui__text_node(ui, cui__frame_printf(ui, "%02d", st->shown), -1, CUI_FONT_DISPLAY, fs, CUI_C(ui, FOREGROUND), false);
    ui->nodes[b].flags |= CUI_NF_TABULAR; ui->nodes[b].line_height = 1.0f;
    ui->nodes[b].ty = travel * (1 - st->t);
    ui->nodes[b].opacity = CUI_MAX(0.001f, st->t);
    cui__close(ui);
}

static void cui__countdown_ex_body(cui_ctx *ui, const char *id_str, double seconds_left, const cui_countdown_opts *o);

/* left to right in any interface: see cui__keep_ltr */
void cui_countdown_ex(cui_ctx *ui, const char *id_str, double seconds_left, const cui_countdown_opts *o) {
    int first = ui->node_count;
    cui__countdown_ex_body(ui, id_str, seconds_left, o);
    cui__keep_ltr(ui, first);
}

static void cui__countdown_ex_body(cui_ctx *ui, const char *id_str, double seconds_left, const cui_countdown_opts *o) {
    cui_countdown_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    if (seconds_left < 0) seconds_left = 0;
    long long total = (long long)seconds_left;
    int v[4] = { (int)(total / 86400), (int)(total / 3600 % 24), (int)(total / 60 % 60), (int)(total % 60) };
    if (o->hide_days) v[1] += v[0] * 24;
    static const char *names[4] = { "Days", "Hours", "Minutes", "Seconds" };
    cui_id id = cui_make_id(ui, id_str);
    float w = o->compact ? 44 : 68, h = o->compact ? 44 : 72, fs = cui__fs(ui, o->compact ? 20 : 32), gap = o->compact ? 6 : 12;
    int units = o->hide_days ? 3 : 4;
    cui_rect pr = cui__prev_rect(ui, id);
    if (!o->compact && pr.w > 0 && pr.w < units * w + (units - 1) * gap) {
        /* a phone: tighter gaps, then smaller tiles */
        gap = 8;
        float k = CUI_CLAMP(floorf((pr.w - (units - 1) * gap) / units), 52.0f, 68.0f) / 68.0f;
        w = floorf(68 * k); h = floorf(72 * k); fs = cui__fs(ui, floorf(32 * k));
    }
    int row = cui__node_new(ui, id);
    ui->nodes[row].dir = CUI_ROW; ui->nodes[row].gap = gap; ui->nodes[row].align = CUI_ALIGN_START;
    ui->nodes[row].self_align = CUI__ALIGN_NOSTRETCH;
    if (!o->compact) { ui->nodes[row].flags |= CUI_NF_ADAPTIVE; ui->nodes[row].min_w = units * 52 + (units - 1) * 8; }
    cui__open(ui, row);
    bool first = true;
    for (int k = o->hide_days ? 1 : 0; k < 4; k++) {
        if (!first && o->compact) {
            int c = cui__text_node(ui, ":", 1, CUI_FONT_DISPLAY, fs, CUI_C(ui, TEXT_MUTED), false);
            ui->nodes[c].line_height = h / fs;
        }
        first = false;
        int col = cui__node_new(ui, 0);
        ui->nodes[col].gap = 8; ui->nodes[col].align = CUI_ALIGN_CENTER;
        cui__open(ui, col);
        cui__roll_tile(ui, cui__hash(&k, sizeof k, id), v[k], w, h, fs, o->compact);
        if (!o->compact) {
            int lt = cui__text_node(ui, names[k], -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_xs), CUI_C(ui, TEXT_MUTED), false);
            ui->nodes[lt].line_height = 1.2f;
        }
        cui__close(ui);
    }
    cui__close(ui);
    double frac = seconds_left - floor(seconds_left);
    if (seconds_left > 0) cui__wakeup_after(ui, (float)(frac > 0.001 ? frac : 1.0) + 0.005f);
    ui->last_id = id;
}

/* ------------------------------------------------------------------------ */
/* Onboarding checklist                                                     */
/* ------------------------------------------------------------------------ */

typedef struct { cui_spring open; float done, hover; bool init; } cui__step_state;
typedef struct { float done, hover; cui_color on, ring, glyph; } cui__circle_paint;

static void cui__paint_step_circle(cui_ctx *ui, cui_node *n, void *data) {
    cui__circle_paint *p = data;
    cui_rect r = n->rect;
    float rad = r.w * 0.5f;
    cui_vec2 c = { r.x + rad, r.y + rad };
    if (p->done < 0.999f) cui_draw_arc(ui, c, rad - 1, 0, 2 * CUI_PI, 1.5f, cui_mix(p->ring, p->on, p->hover * 0.5f));
    if (p->done > 0.001f) {
        cui_draw_circle(ui, c, rad * (0.55f + 0.45f * p->done), cui_alpha(p->on, p->done));
        cui__draw_check(ui, cui__rect_inset(r, r.w * 0.27f), p->done, 1.7f, p->glyph);
    }
}

int cui_checklist_ex(cui_ctx *ui, const char *id_str, cui_check_step *steps, int count, const cui_checklist_opts *o) {
    cui_checklist_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, id_str);
    int done = 0, current = -1;
    for (int k = 0; k < count; k++) { if (steps[k].done) done++; else if (current < 0) current = k; }
    int result = -1;
    int card = cui__node_new(ui, id);
    cui_node *cn = &ui->nodes[card];
    cn->w = CUI_FILL; cn->gap = 0;
    cn->bg = CUI_C(ui, SURFACE); cn->border = 1; cn->border_color = CUI_C(ui, BORDER);
    cn->radius = CUI_CORNERS(ui->theme.radius_panel);
    cn->pad[0] = cn->pad[2] = 8; cn->pad[1] = cn->pad[3] = 8;
    cui__open(ui, card);
    /* header */
    int head = cui__node_new(ui, 0);
    ui->nodes[head].dir = CUI_ROW; ui->nodes[head].align = CUI_ALIGN_CENTER; ui->nodes[head].gap = 14;
    ui->nodes[head].pad[0] = 10; ui->nodes[head].pad[1] = 12; ui->nodes[head].pad[2] = 12; ui->nodes[head].pad[3] = 12;
    cui__open(ui, head);
    int hc = cui__node_new(ui, 0);
    ui->nodes[hc].w = CUI_FILL; ui->nodes[hc].gap = 3;
    cui__open(ui, hc);
    int ht = cui__text_node(ui, o->title ? o->title : "Get started", -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body + 1), CUI_C(ui, FOREGROUND), true);
    ui->nodes[ht].line_height = 1.35f;
    const char *desc = o->description ? o->description : cui__frame_printf(ui, "%d of %d complete", done, count);
    int hd = cui__text_node(ui, desc, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_SECONDARY), true);
    ui->nodes[hd].line_height = 1.4f;
    cui__close(ui);
    cui_progress_ring(ui, cui__frame_printf(ui, "##%s-ring", id_str), count ? (float)done / count : 0, 46);
    cui__close(ui);
    cui_color on = CUI_C(ui, SUCCESS);
    for (int k = 0; k < count; k++) {
        cui_check_step *s = &steps[k];
        cui_id sid = cui__hash(&k, sizeof k, id);
        cui__step_state *st = cui_state(ui, sid, sizeof *st);
        bool expanded = k == current;
        if (!st->init) { st->open.x = expanded ? 1.0f : 0.0f; st->done = s->done ? 1.0f : 0.0f; st->init = true; }
        cui__spring(ui, &st->open, expanded ? 1.0f : 0.0f, CUI__SPRING_EXPAND);
        cui__tween(ui, &st->done, s->done ? 1.0f : 0.0f, CUI__T_STATE);
        cui_id circle_id = sid ^ 0xC1u;
        cui_interact cr = cui__interact(ui, circle_id, CUI_IF_FOCUSABLE);
        cui__tween(ui, &st->hover, cr.hovered ? 1.0f : 0.0f, CUI__T_HOVER);
        if (cr.clicked) s->done = !s->done;
        float ot = cui__clampf(st->open.x, 0, 1);
        int row = cui__node_new(ui, sid);
        cui_node *rn = &ui->nodes[row];
        rn->dir = CUI_ROW; rn->gap = 12; rn->align = CUI_ALIGN_START;
        rn->pad[0] = rn->pad[2] = 10; rn->pad[1] = rn->pad[3] = 12;
        rn->radius = CUI_CORNERS(ui->theme.radius_control - 2);
        rn->bg = cui_alpha(CUI_C(ui, SURFACE_MUTED), ot);
        cui__open(ui, row);
        int circ = cui__leaf(ui, circle_id, CUI_PX(22), CUI_PX(22));
        cui__circle_paint *cp = cui__frame_alloc(ui, sizeof *cp);
        cp->done = st->done; cp->hover = st->hover; cp->on = on; cp->ring = CUI_C(ui, BORDER_STRONG);
        cp->glyph = ui->mode == CUI_THEME_DARK ? cui__rgba(8, 26, 16, 255) : cui__rgba(255, 255, 255, 255);
        ui->nodes[circ].paint = cui__paint_step_circle;
        ui->nodes[circ].paint_data = cp;
        cui__mark_interactive(ui, circ, CUI_IF_FOCUSABLE);
        if (cr.hovered) cui__set_cursor(ui, CUI_CURSOR_HAND);
        cui__focus_ring(ui, circ, cr.focus_visible ? 1.0f : 0.0f);
        int col = cui__node_new(ui, 0);
        ui->nodes[col].w = CUI_FILL; ui->nodes[col].gap = 4;
        cui__open(ui, col);
        int tt = cui__text_node(ui, s->title, -1, CUI_FONT_MEDIUM, cui__fs(ui, ui->theme.text_body),
                                cui_mix(CUI_C(ui, FOREGROUND), CUI_C(ui, TEXT_MUTED), st->done), true);
        ui->nodes[tt].line_height = 1.45f;
        if (ot > 0.001f && (s->description || s->action)) {
            cui_id bid = sid ^ 0xB0u;
            cui_rect br = cui__prev_rect(ui, bid);
            int clip = cui__node_new(ui, 0);
            ui->nodes[clip].flags |= CUI_NF_CLIP;
            ui->nodes[clip].w = CUI_FILL;
            if (ot < 0.999f && br.h > 0) ui->nodes[clip].h = CUI_PX(br.h * ot);
            ui->nodes[clip].opacity = CUI_MAX(0.001f, ot);
            cui__open(ui, clip);
            int body = cui__node_new(ui, bid);
            ui->nodes[body].gap = 10; ui->nodes[body].w = CUI_FILL; ui->nodes[body].pad[2] = 2;
            cui__open(ui, body);
            if (s->description) {
                int dt = cui__text_node(ui, s->description, -1, CUI_FONT_BODY, cui__fs(ui, ui->theme.text_sm), CUI_C(ui, TEXT_SECONDARY), true);
                ui->nodes[dt].line_height = 1.45f;
            }
            if (s->action) {
                cui_button_opts bo;
                memset(&bo, 0, sizeof bo);
                bo.size = CUI_SIZE_SM; bo.variant = CUI_BUTTON_PRIMARY; bo.icon_right = CUI_ICON_ARROW_RIGHT;
                if (!expanded) cui_push_disabled(ui, true);
                cui_push_id_int(ui, k);
                if (cui_button_ex(ui, s->action, &bo) && expanded) result = k;
                cui_pop_id(ui);
                if (!expanded) cui_pop_disabled(ui);
            }
            cui__close(ui);
            cui__close(ui);
        }
        cui__close(ui);
        cui__close(ui);
    }
    cui__close(ui);
    ui->last_id = id;
    return result;
}

/* ------------------------------------------------------------------------ */
/* Veil                                                                     */
/* ------------------------------------------------------------------------ */

#define CUI__VEIL_T 0.28f        /* seconds to dissolve or to come back */
#define CUI__VEIL_FPS 12.0f      /* steps a second of the moving noise: it is slow, so it needs no more frames */
#define CUI__VEIL_MESH 16.0f     /* points between the noise samples; the density runs linearly between them */

typedef struct { float q; bool init; } cui__veil_state;   /* q: 0 covered, 1 revealed */
typedef struct cui__veil_build { struct cui__veil_build *prev; cui_id id; bool disabled; } cui__veil_build;
typedef struct {
    cui_id id;
    float q, t, radius, focus;
    bool catch_clicks;         /* covered: the veil takes the clicks over its content */
    cui_color ground, dots;
    const char *label, *hint;
} cui__veil_paint;

/* a slow, smooth field in 0..1 over the cells (x, y) at time t */
static float cui__veil_noise(float x, float y, float t) {
    float v = sinf(x * 0.21f + t * 0.9f) * sinf(y * 0.27f - t * 0.7f) + sinf((x + y) * 0.11f + t * 0.5f);
    return 0.5f + 0.25f * v;
}

static void cui__paint_veil(cui_ctx *ui, cui_node *n, void *data) {
    cui__veil_paint *p = data;
    cui_rect r = n->rect;
    if (p->q < 1 && r.w > 0 && r.h > 0) {
        if (p->catch_clicks) {
            cui_rect clip = ui->dl.clip_stack[ui->dl.clip_depth - 1];
            float s = ui->scale;
            cui_rect cp = { clip.x / s, clip.y / s, clip.w / s, clip.h / s };
            cui__paint_add_hit(ui, p->id, cui__rect_intersect(cui__xform_rect(ui, r), cp), CUI_NF_INTERACTIVE | CUI_NF_FOCUSABLE);
        }
        cui__dither d;
        memset(&d, 0, sizeof d);
        d.origin = (cui_vec2){ r.x, r.y };
        d.shape = r;
        d.radius = CUI_CORNERS(p->radius);
        d.floor = p->q;   /* revealing turns cells off in threshold order: a dissolve */
        d.color = p->ground;
        cui__draw_dither_rect(ui, &d, r, 1, 1, false);
        /* the dots: a grid of noise samples, the density between them interpolated */
        int cols = CUI_MAX(1, CUI_MIN(64, (int)ceilf(r.w / CUI__VEIL_MESH))), rows = CUI_MAX(1, CUI_MIN(64, (int)ceilf(r.h / CUI__VEIL_MESH)));
        cui__dvert *v = cui__frame_alloc(ui, sizeof(cui__dvert) * (size_t)((cols + 1) * (rows + 1)));
        uint32_t *idx = cui__frame_alloc(ui, sizeof(uint32_t) * (size_t)(cols * rows * 6));
        for (int j = 0; j <= rows; j++)
            for (int i = 0; i <= cols; i++) {
                float x = r.x + r.w * (float)i / (float)cols, y = r.y + r.h * (float)j / (float)rows;
                float nz = cui__veil_noise((x - r.x) * 0.5f, (y - r.y) * 0.5f, p->t);
                v[j * (cols + 1) + i] = (cui__dvert){ x, y, 0.42f + 0.18f * (nz - 0.5f) };
            }
        int k = 0;
        for (int j = 0; j < rows; j++)
            for (int i = 0; i < cols; i++) {
                uint32_t a = (uint32_t)(j * (cols + 1) + i), b = a + (uint32_t)(cols + 1);
                idx[k++] = a; idx[k++] = a + 1; idx[k++] = b + 1;
                idx[k++] = a; idx[k++] = b + 1; idx[k++] = b;
            }
        d.color = p->dots;
        cui__draw_dither_mesh(ui, &d, v, (cols + 1) * (rows + 1), idx, k);
        /* the caption fades with the veil */
        float fs = cui__fs(ui, ui->theme.text_body), hs = cui__fs(ui, ui->theme.text_sm);
        float lh = p->label ? cui__font_line_height(ui, CUI_FONT_MEDIUM, fs) : 0, hh = p->hint ? cui__font_line_height(ui, CUI_FONT_BODY, hs) : 0;
        if ((p->label || p->hint) && lh + hh + 8 <= r.h) {
            float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f - (lh + hh) * 0.5f;
            cui__push_xform(ui, 1, 0, 0, 1 - p->q, 0, 0);
            cui_push_clip(ui, r);
            if (p->label) cui__draw_text_at(ui, CUI_FONT_MEDIUM, fs, cx, cy + lh * 0.5f, CUI__AT_CENTER, CUI_C(ui, FOREGROUND), p->label, false);
            if (p->hint) cui__draw_text_at(ui, CUI_FONT_BODY, hs, cx, cy + lh + hh * 0.5f, CUI__AT_CENTER, CUI_C(ui, TEXT_SECONDARY), p->hint, false);
            cui_pop_clip(ui);
            cui__pop_xform(ui);
        }
    }
    cui__draw_focus_ring(ui, r, CUI_CORNERS(p->radius), p->focus);
}

bool cui_veil_begin_ex(cui_ctx *ui, const char *id_str, bool *revealed, const cui_veil_opts *o) {
    cui_veil_opts d;
    memset(&d, 0, sizeof d);
    if (!o) o = &d;
    cui_id id = cui_make_id(ui, id_str);
    cui_interact r = cui__interact(ui, id, CUI_IF_FOCUSABLE);
    cui__veil_state *st = cui_state(ui, id, sizeof *st);
    bool shown = revealed && *revealed, changed = false;
    if (!st->init) { st->q = shown ? 1.0f : 0.0f; st->init = true; }
    /* a click reveals; covering again takes the keyboard, `toggle`, or the app */
    if (revealed && r.clicked && (r.keyboard_activate || !shown || o->toggle)) { *revealed = shown = !shown; changed = true; }
    float target = shown ? 1.0f : 0.0f;
    if (ui->theme.motion <= 0) st->q = target;
    else if (st->q != target) {
        float step = ui->dt / (CUI__VEIL_T * CUI_MAX(1.0f, ui->theme.motion));
        st->q = target > st->q ? CUI_MIN(target, st->q + step) : CUI_MAX(target, st->q - step);
        ui->animating = true;
    }
    if (r.hovered && !shown) cui__set_cursor(ui, CUI_CURSOR_HAND);
    int n = cui__node_new(ui, id);
    cui_node *nd = &ui->nodes[n];
    if (o->width.kind) nd->w = o->width;
    nd->self_align = CUI__ALIGN_NOSTRETCH;
    nd->flags |= CUI_NF_PAINT_AFTER;
    cui__mark_interactive(ui, n, CUI_IF_FOCUSABLE);
    cui__veil_paint *p = cui__frame_alloc(ui, sizeof *p);
    p->id = id;
    p->q = st->q;
    p->catch_clicks = !shown || st->q < 1;
    p->radius = o->radius;
    p->focus = r.focus_visible ? 1.0f : 0.0f;
    p->ground = cui__color_set(o->color) ? cui_color_of(ui, o->color) : CUI_C(ui, SURFACE_MUTED);
    p->dots = cui_mix(p->ground, CUI_C(ui, FOREGROUND), 0.18f);
    p->label = o->label ? cui__frame_strdup(ui, o->label, -1) : NULL;
    p->hint = o->hint ? cui__frame_strdup(ui, o->hint, -1) : NULL;
    if (st->q < 1 && ui->theme.motion > 0) {
        /* the noise moves in steps, so a covered veil asks for a dozen frames a second, not every frame */
        double steps = floor(ui->time * CUI__VEIL_FPS);
        p->t = (float)fmod(steps / CUI__VEIL_FPS, 600.0);
        cui__wakeup_after(ui, (float)((steps + 1) / CUI__VEIL_FPS - ui->time));
    }
    nd->paint = cui__paint_veil;
    nd->paint_data = p;
    cui__open(ui, n);
    cui__veil_build *b = cui__frame_alloc(ui, sizeof *b);
    b->prev = ui->building.veil;
    b->id = id;
    /* fully covered, the content is out of reach of the keyboard too; it is enabled again as soon as
     * a reveal starts, and stays enabled while the veil comes back (the veil takes the clicks then) */
    b->disabled = !shown && st->q <= 0;
    if (b->disabled) cui_push_disabled(ui, true);
    ui->building.veil = b;
    return changed;
}

void cui_veil_end(cui_ctx *ui) {
    cui__veil_build *b = ui->building.veil;
    if (!b) return;
    if (b->disabled) cui_pop_disabled(ui);
    cui__close(ui);
    ui->building.veil = b->prev;
    ui->last_id = b->id;
}
