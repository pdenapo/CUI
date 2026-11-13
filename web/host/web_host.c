/* The browser side of CUI: contexts, input and software rendering into a
 * pixel buffer that host/runtime.js copies onto a canvas. Shared by every
 * wasm module (component scenes, gallery, demo apps). */
#include "../../src/cui_internal.h"
#include <cui/cui_render.h>
#include "web_host.h"

#define EXPORT(name) __attribute__((export_name(#name)))

void js_clipboard_set(const char *s, int n) WASM_IMPORT(js_clipboard_set);
double js_perf_now(void) WASM_IMPORT(js_perf_now);
/* GPU textures live in the page's WebGL2 renderer (site/js/gl.js). */
int js_tex_create(int owner, int w, int h, int fmt, const void *px) WASM_IMPORT(js_tex_create);
void js_tex_update(int owner, int tex, int x, int y, int w, int h, const void *px, int stride) WASM_IMPORT(js_tex_update);
void js_tex_destroy(int owner, int tex) WASM_IMPORT(js_tex_destroy);
/* Emoji come from the browser's own emoji font, drawn on a canvas (site/js/runtime.js). */
int js_draw_emoji(const char *s, int n, int px, unsigned char *rgba) WASM_IMPORT(js_draw_emoji);
static bool draw_emoji(void *u, const char *s, int n, int px, unsigned char *rgba) { (void)u; return js_draw_emoji(s, n, px, rgba) != 0; }
static cui_texture gpu_create(void *user, int w, int h, cui_texture_format fmt, const void *px) { return (cui_texture)js_tex_create((int)(uintptr_t)user, w, h, (int)fmt, px); }
static void gpu_update(void *user, cui_texture t, int x, int y, int w, int h, const void *px, int stride) { js_tex_update((int)(uintptr_t)user, (int)t, x, y, w, h, px, stride); }
static void gpu_destroy(void *user, cui_texture t) { js_tex_destroy((int)(uintptr_t)user, (int)t); }

typedef struct web_inst {
    cui_ctx *ui;
    cui_sw_renderer *sw;
    unsigned char *px;
    size_t px_cap;
    int pw, ph;
    int content;
    int frame;
    float build_ms, paint_ms;
    int scroll_consumed;
    int vertex_count, index_count;
    float caret[4];        /* the focused text field's caret (x, y, w, h in points), for the page's IME anchor */
    bool caret_live;       /* a text field reported it in the latest frame */
} web_inst;

/* ---- clipboard: the page keeps this copy current on paste ---- */
static char *clip_buf;
static size_t clip_cap;
EXPORT(web_clipboard_buffer) char *web_clipboard_buffer(int n) {
    if ((size_t)n + 1 > clip_cap) { free(clip_buf); clip_cap = (size_t)n + 64; clip_buf = malloc(clip_cap); }
    clip_buf[0] = 0;
    return clip_buf;
}
static void clip_set(void *user, const char *utf8) {
    (void)user;
    int n = (int)strlen(utf8);
    char *b = web_clipboard_buffer(n);
    memcpy(b, utf8, (size_t)n + 1);
    js_clipboard_set(utf8, n);
}
static const char *clip_get(void *user) { (void)user; return clip_buf ? clip_buf : ""; }
static void set_cursor(void *user, cui_cursor c) { (void)user; (void)c; }
static void set_text_rect(void *user, cui_rect r) {
    web_inst *in = user;
    in->caret[0] = r.x; in->caret[1] = r.y; in->caret[2] = r.w; in->caret[3] = r.h;
    in->caret_live = true;
}

/* ---- memory for strings passed in from JS ---- */
EXPORT(web_alloc) void *web_alloc(int n) { return malloc((size_t)n); }
EXPORT(web_free) void web_free(void *p) { free(p); }

/* ---- content catalog ---- */
EXPORT(content_count) int content_count(void) { return web_content_count(); }
EXPORT(content_name) const char *content_name(int i) { return web_content_name(i); }
EXPORT(content_width) float content_width(int i) { return web_content_width(i); }
EXPORT(content_height) float content_height(int i) { return web_content_height(i); }
EXPORT(content_accent) int content_accent(int i) { return web_content_accent(i); }
EXPORT(content_theme) int content_theme(int i) { return web_content_theme(i); }

/* ---- instances ---- */
static void warm_up(web_inst *in) {
    int n = web_content_warmup(in->content);
    cui_input_mouse_leave(in->ui);
    for (int f = 0; f < n; f++) {
        web_content_input(in->content, in->ui, -(f + 1));
        cui_begin_frame(in->ui, 1.0f / 60.0f);
        in->caret_live = false;
        web_content_build(in->content, in->ui, f);
        cui_end_frame(in->ui);
    }
    in->frame = n;
}

/* gpu_owner > 0: textures go to the page's WebGL2 renderer with that id and
 * the page draws cui_get_draw_data itself. 0: the software renderer here. */
EXPORT(inst_new) web_inst *inst_new(int content, float w, float h, float scale, int dark, int accent, int gpu_owner) {
    web_inst *in = calloc(1, sizeof *in);
    cui_renderer_iface iface;
    if (gpu_owner > 0) {
        iface = (cui_renderer_iface){ .user = (void *)(uintptr_t)gpu_owner, .texture_create = gpu_create,
                                      .texture_update = gpu_update, .texture_destroy = gpu_destroy };
    } else {
        in->sw = cui_sw_create();
        cui_sw_set_threads(in->sw, 1);
        cui_sw_set_incremental(in->sw, true);   /* the pixel buffer is kept: draw only what changed */
        iface = cui_sw_iface(in->sw);
    }
    in->content = content;
    in->ui = cui_create(&(cui_ctx_desc){ .renderer = iface, .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                        .accent = accent >= 0 ? (cui_accent)accent : CUI_ACCENT_NEUTRAL });
    cui_set_platform(in->ui, &(cui_platform_iface){ .user = in, .set_clipboard = clip_set, .get_clipboard = clip_get,
                                                   .set_cursor = set_cursor, .set_text_input_rect = set_text_rect, .draw_emoji = draw_emoji });
    cui_set_viewport(in->ui, w, h, scale);
    cui_input_focus(in->ui, true);
    warm_up(in);
    return in;
}

EXPORT(inst_free) void inst_free(web_inst *in) {
    if (!in) return;
    cui_destroy(in->ui);
    if (in->sw) cui_sw_destroy(in->sw);
    free(in->px);
    free(in);
}

EXPORT(inst_resize) void inst_resize(web_inst *in, float w, float h, float scale) { cui_set_viewport(in->ui, w, h, scale); cui_request_redraw(in->ui); }

/* Layout facts from the latest frame, so the page can give a scene the width
 * it has: the narrowest the content can be laid out, and how tall it is then
 * (floating layers included, measured unclamped). */
EXPORT(inst_min_width) float inst_min_width(web_inst *in) {
    cui_ctx *ui = in->ui;
    if (ui->node_count == 0) return 0;
    float m = 0;
    for (int c = ui->nodes[0].first; c >= 0; c = ui->nodes[c].next) m = CUI_MAX(m, ui->nodes[c].minc_w);
    return m;
}
EXPORT(inst_content_height) float inst_content_height(web_inst *in) {
    cui_ctx *ui = in->ui;
    if (ui->node_count == 0) return 0;
    float h = 0;
    for (int c = ui->nodes[0].first; c >= 0; c = ui->nodes[c].next) h += ui->nodes[c].pref_h;
    for (int k = 0; k < ui->float_count; k++) {
        cui_node *f = &ui->nodes[ui->float_roots[k]];
        if (f->placement == CUI__PLACE_FULL) continue;
        float fh = CUI_MAX(f->rect.h, f->pref_h), y = f->rect.y;
        if (f->anchor && f->placement >= CUI_PLACE_BOTTOM_START && f->placement <= CUI_PLACE_BOTTOM_END) {
            cui_rect a = cui__state_entry(ui, f->anchor)->rect;   /* below its anchor, where it opens when there is room */
            y = a.y + a.h + f->offset;
        }
        h = CUI_MAX(h, (f->placement == CUI__PLACE_CENTER ? fh + 32 : y + fh + 8));
    }
    return h;
}

int inst_paint(web_inst *in);
/* Builds one frame and, when asked, paints it. Returns 1 when pixels changed. */
EXPORT(inst_frame) int inst_frame(web_inst *in, float dt, int paint) {
    cui_ctx *ui = in->ui;
    bool had_scroll = ui->scroll_dx != 0 || ui->scroll_dy != 0;
    double t0 = js_perf_now();
    cui_begin_frame(ui, dt);
    in->caret_live = false;
    web_content_build(in->content, ui, in->frame++);
    in->scroll_consumed = had_scroll && ui->scroll_dx == 0 && ui->scroll_dy == 0;
    cui_end_frame(ui);
    double t1 = js_perf_now();
    in->build_ms = (float)(t1 - t0);
    if (!paint) return 0;
    return inst_paint(in);
}

/* Paints the last built frame. */
EXPORT(inst_paint) int inst_paint(web_inst *in) {
    if (!in->sw) return 0;
    double t1 = js_perf_now();
    const cui_draw_data *dd = cui_get_draw_data(in->ui);
    size_t need = (size_t)dd->width_px * (size_t)dd->height_px * 4;
    if (need > in->px_cap) { free(in->px); in->px = malloc(need); in->px_cap = need; }
    in->pw = dd->width_px; in->ph = dd->height_px;
    in->vertex_count = dd->vertex_count; in->index_count = dd->index_count;
    cui_sw_render(in->sw, dd, in->px, in->pw, in->ph, in->pw * 4, CUI_SW_RGBA);
    in->paint_ms = (float)(js_perf_now() - t1);
    return 1;
}

EXPORT(inst_pixels) unsigned char *inst_pixels(web_inst *in) { return in->px; }
EXPORT(inst_pixel_width) int inst_pixel_width(web_inst *in) { return in->pw; }
EXPORT(inst_pixel_height) int inst_pixel_height(web_inst *in) { return in->ph; }
EXPORT(inst_build_ms) float inst_build_ms(web_inst *in) { return in->build_ms; }
EXPORT(inst_paint_ms) float inst_paint_ms(web_inst *in) { return in->paint_ms; }
EXPORT(inst_triangles) int inst_triangles(web_inst *in) { return in->index_count / 3; }
EXPORT(inst_scroll_consumed) int inst_scroll_consumed(web_inst *in) { return in->scroll_consumed; }
EXPORT(inst_needs_redraw) int inst_needs_redraw(web_inst *in) { return cui_needs_redraw(in->ui); }
EXPORT(inst_next_wakeup) float inst_next_wakeup(web_inst *in) { return cui_next_wakeup(in->ui); }
EXPORT(inst_cursor) int inst_cursor(web_inst *in) { return (int)cui_get_cursor(in->ui); }
EXPORT(inst_wants_keyboard) int inst_wants_keyboard(web_inst *in) { return cui_wants_keyboard(in->ui); }
/* The caret of the text field being edited, as four floats (x, y, w, h in
 * points), or 0 when none has focus. The page keeps a hidden text area there,
 * because browsers open the IME candidate window only at a focused DOM input. */
EXPORT(inst_text_rect) const float *inst_text_rect(web_inst *in) { return in->caret_live ? in->caret : NULL; }

EXPORT(inst_mouse_move) void inst_mouse_move(web_inst *in, float x, float y) { cui_input_mouse_move(in->ui, x, y); }
EXPORT(inst_mouse_button) void inst_mouse_button(web_inst *in, int b, int down) { cui_input_mouse_button(in->ui, (cui_mouse_button)b, down != 0); }
EXPORT(inst_mouse_leave) void inst_mouse_leave(web_inst *in) { cui_input_mouse_leave(in->ui); }
EXPORT(inst_scroll) void inst_scroll(web_inst *in, float dx, float dy) { cui_input_scroll(in->ui, dx, dy); }
EXPORT(inst_key) void inst_key(web_inst *in, int key, int down, int mods) { cui_input_key(in->ui, (cui_key)key, down != 0, mods); }
EXPORT(inst_text) void inst_text(web_inst *in, const char *utf8) { cui_input_text(in->ui, utf8); }
EXPORT(inst_focus) void inst_focus(web_inst *in, int focused) { cui_input_focus(in->ui, focused != 0); }
EXPORT(inst_set_theme) void inst_set_theme(web_inst *in, int dark) { cui_set_theme(in->ui, dark ? CUI_THEME_DARK : CUI_THEME_LIGHT); cui_request_redraw(in->ui); }
EXPORT(inst_set_accent) void inst_set_accent(web_inst *in, int a) { cui_set_accent(in->ui, (cui_accent)a); cui_request_redraw(in->ui); }
EXPORT(inst_set_reduced_motion) void inst_set_reduced_motion(web_inst *in, int on) { cui_set_reduced_motion(in->ui, on != 0); cui_request_redraw(in->ui); }
EXPORT(inst_get_theme) int inst_get_theme(web_inst *in) { return cui_get_theme_mode(in->ui) == CUI_THEME_DARK; }
EXPORT(inst_get_accent) int inst_get_accent(web_inst *in) { return (int)cui_get_accent(in->ui); }

/* ---- draw data for the WebGL2 renderer ---- */
EXPORT(web_sizeof_vertex) int web_sizeof_vertex(void) { return (int)sizeof(cui_vertex); }
EXPORT(web_sizeof_cmd) int web_sizeof_cmd(void) { return (int)sizeof(cui_draw_cmd); }
EXPORT(web_offsetof_cmd_texture) int web_offsetof_cmd_texture(void) { return (int)offsetof(cui_draw_cmd, texture); }
EXPORT(web_offsetof_cmd_index) int web_offsetof_cmd_index(void) { return (int)offsetof(cui_draw_cmd, index_offset); }
EXPORT(inst_draw_data) const cui_draw_data *inst_draw_data(web_inst *in) { return cui_get_draw_data(in->ui); }
EXPORT(dd_vertices) const void *dd_vertices(const cui_draw_data *d) { return d->vertices; }
EXPORT(dd_vertex_count) int dd_vertex_count(const cui_draw_data *d) { return d->vertex_count; }
EXPORT(dd_indices) const void *dd_indices(const cui_draw_data *d) { return d->indices; }
EXPORT(dd_index_count) int dd_index_count(const cui_draw_data *d) { return d->index_count; }
EXPORT(dd_commands) const void *dd_commands(const cui_draw_data *d) { return d->commands; }
EXPORT(dd_command_count) int dd_command_count(const cui_draw_data *d) { return d->command_count; }
EXPORT(dd_width) int dd_width(const cui_draw_data *d) { return d->width_px; }
EXPORT(dd_height) int dd_height(const cui_draw_data *d) { return d->height_px; }
EXPORT(dd_clear) uint32_t dd_clear(const cui_draw_data *d) { return (uint32_t)d->clear_color.r | (uint32_t)d->clear_color.g << 8 | (uint32_t)d->clear_color.b << 16; }
EXPORT(dd_font_atlas) int dd_font_atlas(const cui_draw_data *d) { return (int)d->font_atlas; }

/* Theme tokens as 0xRRGGBBAA, so the page around the canvases uses the
 * library's exact colors. */
EXPORT(web_theme_token) uint32_t web_theme_token(int dark, int accent, int token) {
    static cui_theme t;
    static int have = -1;
    int key = dark * 64 + accent;
    if (have != key) { cui__theme_build(&t, dark ? CUI_THEME_DARK : CUI_THEME_LIGHT, (cui_accent)accent); have = key; }
    if (token <= 0 || token >= CUI_TOKEN_COUNT) return 0;
    cui_color c = t.colors[token];
    return (uint32_t)c.r << 24 | (uint32_t)c.g << 16 | (uint32_t)c.b << 8 | c.a;
}

/* Scenes may end their scripted warm-up with a button still held (a card
 * mid-drag, for the screenshot). Before real input takes over, let go of it. */
EXPORT(inst_release_scripted) int inst_release_scripted(web_inst *in) {
    int held = 0;
    for (int b = 0; b < 3; b++)
        if (in->ui->mdown[b]) { cui_input_mouse_button(in->ui, (cui_mouse_button)b, false); held = 1; }
    if (held) {
        cui_begin_frame(in->ui, 1.0f / 60.0f);
        web_content_build(in->content, in->ui, in->frame++);
        cui_end_frame(in->ui);
    }
    return held;
}
