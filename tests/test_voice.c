/* Tests for the voice components: the voice waveform (resampling, scrolling, silence,
 * progress colors, seeking), the voice note (play, seek, speed), the composer's recorder
 * (record, cancel, send, focus) and the voice session (state, owners, buttons). Runs
 * headless with the software renderer; drawing is checked by looking at the pixels where
 * dots must or must not be. */
#include <cui/cui.h>
#include <cui/cui_render.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failed, g_checks;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { g_checks++; long long a_ = (long long)(a), b_ = (long long)(b); if (a_ != b_) { g_failed++; \
    printf("  FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)
#define CHECK_NEAR(a, b, eps) do { g_checks++; double a_ = (double)(a), b_ = (double)(b); if (!(fabs(a_ - b_) <= (eps))) { g_failed++; \
    printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #a, a_, b_); } } while (0)

static cui_sw_renderer *g_sw;

static cui_ctx *new_ctx(float w, float h, bool dark, cui_accent accent) {
    cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT, .accent = accent });
    cui_set_viewport(ui, w, h, 1);
    cui_set_reduced_motion(ui, true);
    cui_input_mouse_leave(ui);
    return ui;
}

typedef void (*build_fn)(cui_ctx *ui, void *user);
static void run(cui_ctx *ui, build_fn fn, void *user, int frames) {
    for (int i = 0; i < frames; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_column(ui, .pad = 20, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND) fn(ui, user);
        cui_end_frame(ui);
    }
}
static void move(cui_ctx *ui, build_fn fn, void *u, float x, float y) { cui_input_mouse_move(ui, x, y); run(ui, fn, u, 1); }
static void press(cui_ctx *ui, build_fn fn, void *u) { cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true); run(ui, fn, u, 1); }
static void release(cui_ctx *ui, build_fn fn, void *u) { cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false); run(ui, fn, u, 1); }
static void click(cui_ctx *ui, build_fn fn, void *u, float x, float y) { move(ui, fn, u, x, y); press(ui, fn, u); release(ui, fn, u); }
static void click_rect(cui_ctx *ui, build_fn fn, void *u, cui_rect r) { click(ui, fn, u, r.x + r.w / 2, r.y + r.h / 2); }
static void key(cui_ctx *ui, build_fn fn, void *u, cui_key k, int mods) {
    cui_input_key(ui, k, true, mods); cui_input_key(ui, k, false, mods);
    run(ui, fn, u, 1);
}
static cui_id scoped_id(cui_ctx *ui, const char *scope, const char *name) {
    cui_push_id(ui, scope);
    cui_id id = cui_make_id(ui, name);
    cui_pop_id(ui);
    return id;
}

/* the frame rendered into memory, at scale 1, so a point is a pixel */
typedef struct { unsigned char *px; int w, h; } image;
static image render(cui_ctx *ui) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    image im = { malloc((size_t)dd->width_px * dd->height_px * 4), dd->width_px, dd->height_px };
    cui_sw_render(g_sw, dd, im.px, im.w, im.h, im.w * 4, CUI_SW_RGBA);
    return im;
}
static const unsigned char *pixel(image im, float x, float y) {
    int xi = (int)x, yi = (int)y;
    if (xi < 0) xi = 0;
    if (yi < 0) yi = 0;
    if (xi >= im.w) xi = im.w - 1;
    if (yi >= im.h) yi = im.h - 1;
    return im.px + ((size_t)yi * im.w + xi) * 4;
}
/* how far a pixel is from the white background of the light theme */
static int ink(image im, float x, float y) { const unsigned char *p = pixel(im, x, y); return 765 - (p[0] + p[1] + p[2]); }
/* a hue, not a gray: the blue accent has blue well above red */
static bool bluish(image im, float x, float y) { const unsigned char *p = pixel(im, x, y); return p[2] > p[0] + 40; }
static bool grayish(image im, float x, float y) {
    const unsigned char *p = pixel(im, x, y);
    int lo = p[0] < p[1] ? p[0] : p[1], hi = p[0] > p[1] ? p[0] : p[1];
    lo = lo < p[2] ? lo : p[2]; hi = hi > p[2] ? hi : p[2];
    return hi - lo < 16;
}

/* ------------------------------------------------------------------------ */
/* resampling                                                               */

static void test_resample(void) {
    printf("voice waveform: resampling keeps the loudest of each stretch, silence stays silent\n");
    float out[8];
    const float a[6] = { 0.1f, 0.5f, 0.3f, 0.2f, 0.9f, 0.4f };
    CHECK_EQ(cui_voice_resample(a, 6, out, 3), 3);
    CHECK_NEAR(out[0], 0.5, 1e-6); CHECK_NEAR(out[1], 0.3, 1e-6); CHECK_NEAR(out[2], 0.9, 1e-6);
    CHECK_EQ(cui_voice_resample(a, 6, out, 1), 1);
    CHECK_NEAR(out[0], 0.9, 1e-6);
    /* fewer levels than columns: each level fills its share */
    const float b[2] = { 0.25f, 0.75f };
    cui_voice_resample(b, 2, out, 4);
    CHECK_NEAR(out[0], 0.25, 1e-6); CHECK_NEAR(out[1], 0.25, 1e-6); CHECK_NEAR(out[2], 0.75, 1e-6); CHECK_NEAR(out[3], 0.75, 1e-6);
    /* a stretch with no audio stays silent; one with any sound takes the sound */
    const float c[4] = { CUI_VOICE_SILENT, CUI_VOICE_SILENT, CUI_VOICE_SILENT, 0.0f };
    cui_voice_resample(c, 4, out, 2);
    CHECK(out[0] < 0);
    CHECK_NEAR(out[1], 0.0, 1e-6);
    const float d[2] = { NAN, 0.4f };
    cui_voice_resample(d, 2, out, 2);
    CHECK(out[0] < 0);   /* not a number is no audio */
    CHECK_NEAR(out[1], 0.4, 1e-6);
    /* no input, no room */
    CHECK_EQ(cui_voice_resample(NULL, 0, out, 2), 2);
    CHECK(out[0] < 0 && out[1] < 0);
    CHECK_EQ(cui_voice_resample(a, 6, out, 0), 0);
    CHECK_EQ(cui_voice_resample(a, 6, NULL, 3), 0);
}

/* ------------------------------------------------------------------------ */
/* drawing: scrolling, silence, progress colors                             */

typedef struct {
    const float *lv; int n;
    const uint8_t *own;
    float progress; bool seek, clip;
    int changes;
} wave_case;

/* 100 points wide at pitch 10: exactly ten columns, the first one's left edge at the rect's */
static void build_wave(cui_ctx *ui, void *u) {
    wave_case *c = u;
    if (cui_voice_wave(ui, "w", c->lv, c->n, .pitch = 10, .width = CUI_PX(100), .clip = c->clip, .owners = c->own,
                       .progress = c->seek ? &c->progress : NULL)) c->changes++;
}

/* the dot of column col, d rows above the center line */
static float col_x(cui_rect r, int col) { return r.x + col * 10 + 5; }
static float row_y(cui_rect r, int d) { return r.y + r.h / 2 - d * 10; }

static void test_wave_drawing(void) {
    printf("voice waveform: newest at the right, silence draws only the baseline, played columns in the accent\n");
    cui_ctx *ui = new_ctx(300, 200, false, CUI_ACCENT_BLUE);
    float lv[10];
    wave_case c;
    memset(&c, 0, sizeof c);
    /* scrolling, one loud level: only the rightmost column stands up */
    lv[0] = 1.0f;
    c.lv = lv; c.n = 1;
    run(ui, build_wave, &c, 2);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "w"));
    CHECK_NEAR(R.w, 100, 0.5);
    CHECK_NEAR(R.h, 140, 0.5);   /* 14 pitches */
    image im = render(ui);
    CHECK(ink(im, col_x(R, 9), row_y(R, 3)) > 300);
    CHECK(ink(im, col_x(R, 9), row_y(R, -6)) > 300);   /* mirrored to the bottom row */
    CHECK(ink(im, col_x(R, 8), row_y(R, 3)) < 10);
    CHECK(ink(im, col_x(R, 0), row_y(R, 0)) > 5);      /* the faint baseline where there is nothing yet */
    CHECK(ink(im, col_x(R, 0), row_y(R, 0)) < 120);
    free(im.px);
    /* a quiet level reaches only the center rows */
    lv[0] = 0.3f;   /* reach 1.95 rows: rows 0 and 1 full, row 2 almost half */
    run(ui, build_wave, &c, 1);
    im = render(ui);
    CHECK(ink(im, col_x(R, 9), row_y(R, 1)) > 300);
    CHECK(ink(im, col_x(R, 9), row_y(R, 3)) < 10);
    free(im.px);
    /* no audio: only the faint baseline, never a lit dot */
    for (int k = 0; k < 10; k++) lv[k] = CUI_VOICE_SILENT;
    c.n = 10;
    run(ui, build_wave, &c, 1);
    im = render(ui);
    for (int k = 0; k < 10; k++) {
        CHECK(ink(im, col_x(R, k), row_y(R, 1)) < 10);
        CHECK(ink(im, col_x(R, k), row_y(R, 0)) < 120);
    }
    free(im.px);
    /* clip mode with progress: (column + 0.5) / 10 <= progress is played */
    for (int k = 0; k < 10; k++) lv[k] = 1.0f;
    c.clip = true; c.seek = true; c.progress = 0.35f;
    run(ui, build_wave, &c, 2);
    im = render(ui);
    CHECK(bluish(im, col_x(R, 0) + 2, row_y(R, 2)));
    CHECK(bluish(im, col_x(R, 3) + 2, row_y(R, 2)));     /* 0.35: exactly on the boundary, played */
    CHECK(grayish(im, col_x(R, 4) + 2, row_y(R, 2)));
    CHECK(ink(im, col_x(R, 4) + 2, row_y(R, 2)) > 60);
    CHECK(grayish(im, col_x(R, 9) + 2, row_y(R, 2)));
    free(im.px);
    c.progress = 0.349f;
    run(ui, build_wave, &c, 1);
    im = render(ui);
    CHECK(bluish(im, col_x(R, 2) + 2, row_y(R, 2)));
    CHECK(grayish(im, col_x(R, 3) + 2, row_y(R, 2)));    /* just short of the boundary */
    free(im.px);
    c.progress = 1;
    run(ui, build_wave, &c, 1);
    im = render(ui);
    CHECK(bluish(im, col_x(R, 9) - 2, row_y(R, 2)));
    free(im.px);
    c.progress = 0;
    run(ui, build_wave, &c, 1);
    im = render(ui);
    CHECK(grayish(im, col_x(R, 0) + 2, row_y(R, 2)));
    free(im.px);
    CHECK_EQ(c.changes, 0);   /* drawing never seeks */
    cui_destroy(ui);
}

static void test_wave_owners(void) {
    printf("voice waveform: two voices on one timeline keep their colors when scrolling and resampled\n");
    cui_ctx *ui = new_ctx(300, 200, false, CUI_ACCENT_BLUE);
    float lv[20];
    uint8_t own[20];
    for (int k = 0; k < 20; k++) { lv[k] = 1.0f; own[k] = k < 15 ? CUI_VOICE_YOU : CUI_VOICE_REPLY; }
    wave_case c;
    memset(&c, 0, sizeof c);
    c.lv = lv; c.n = 20; c.own = own;
    run(ui, build_wave, &c, 2);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "w"));
    image im = render(ui);
    /* scrolling shows the newest ten: levels 10..19, of which 15..19 are the reply's */
    CHECK(grayish(im, col_x(R, 4), row_y(R, 2)));
    CHECK(ink(im, col_x(R, 4), row_y(R, 2)) > 300);
    CHECK(bluish(im, col_x(R, 5), row_y(R, 2)));
    free(im.px);
    /* the whole clip in ten columns: two levels each, 15 and up the reply's; column 7 holds 14 and 15 */
    c.clip = true;
    run(ui, build_wave, &c, 1);
    im = render(ui);
    CHECK(grayish(im, col_x(R, 6), row_y(R, 2)));
    CHECK(bluish(im, col_x(R, 8), row_y(R, 2)));
    free(im.px);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* seeking                                                                  */

static void test_wave_seek(void) {
    printf("voice waveform: click and drag seek, arrows step, Home and End jump\n");
    cui_ctx *ui = new_ctx(300, 200, false, CUI_ACCENT_BLUE);
    float lv[10];
    for (int k = 0; k < 10; k++) lv[k] = 0.5f;
    wave_case c;
    memset(&c, 0, sizeof c);
    c.lv = lv; c.n = 10; c.seek = true;
    run(ui, build_wave, &c, 2);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "w"));
    click(ui, build_wave, &c, R.x + 60, R.y + R.h / 2);
    CHECK_NEAR(c.progress, 0.6, 0.011);
    CHECK(c.changes >= 1);
    /* drag on, past the end */
    move(ui, build_wave, &c, R.x + 20, R.y + 10);
    press(ui, build_wave, &c);
    CHECK_NEAR(c.progress, 0.2, 0.011);
    move(ui, build_wave, &c, R.x + 250, R.y - 40);
    CHECK_NEAR(c.progress, 1.0, 1e-6);
    release(ui, build_wave, &c);
    move(ui, build_wave, &c, R.x + 10, R.y + 10);   /* hovering after the release does not seek */
    CHECK_NEAR(c.progress, 1.0, 1e-6);
    /* keys, once it has focus */
    key(ui, build_wave, &c, CUI_KEY_HOME, 0);
    CHECK_NEAR(c.progress, 0.0, 1e-6);
    key(ui, build_wave, &c, CUI_KEY_RIGHT, 0);
    key(ui, build_wave, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.progress, 0.1, 1e-5);
    key(ui, build_wave, &c, CUI_KEY_LEFT, 0);
    CHECK_NEAR(c.progress, 0.05, 1e-5);
    key(ui, build_wave, &c, CUI_KEY_END, 0);
    CHECK_NEAR(c.progress, 1.0, 1e-6);
    key(ui, build_wave, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.progress, 1.0, 1e-6);   /* clamped */
    /* Tab reaches it */
    cui_ctx *u2 = new_ctx(300, 200, false, CUI_ACCENT_BLUE);
    wave_case c2 = c;
    c2.progress = 0.5f;
    run(u2, build_wave, &c2, 2);
    key(u2, build_wave, &c2, CUI_KEY_TAB, 0);
    key(u2, build_wave, &c2, CUI_KEY_LEFT, 0);
    CHECK_NEAR(c2.progress, 0.45, 1e-5);
    cui_destroy(u2);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* voice note                                                               */

typedef struct { cui_voice_note note; bool mine; int flags, toggles, seeks, speeds; } note_case;

static void build_note(cui_ctx *ui, void *u) {
    note_case *c = u;
    int f = cui_voice_note(ui, "note", &c->note, .mine = c->mine);
    c->flags = f;
    if (f & CUI_PLAYER_TOGGLE) c->toggles++;
    if (f & CUI_PLAYER_SEEK) c->seeks++;
    if (f & CUI_PLAYER_SPEED) c->speeds++;
}

static void test_voice_note(void) {
    printf("voice note: play toggles, the waveform seeks in seconds, the speed chip cycles\n");
    cui_ctx *ui = new_ctx(500, 200, false, CUI_ACCENT_BLUE);
    float lv[40];
    for (int k = 0; k < 40; k++) lv[k] = 0.3f + 0.5f * (k % 5) / 4.0f;
    note_case c;
    memset(&c, 0, sizeof c);
    c.note.levels = lv; c.note.count = 40; c.note.duration = 10;
    run(ui, build_note, &c, 2);
    cui_rect B = cui_get_rect(ui, cui_make_id(ui, "note"));
    CHECK_NEAR(B.w, 330, 0.5);
    cui_rect W = cui_get_rect(ui, scoped_id(ui, "note", "wave"));
    CHECK(W.w > 100 && W.x > B.x + 34);
    /* play: at the start of the row */
    click(ui, build_note, &c, B.x + 8 + 17, B.y + B.h / 2);
    CHECK(c.note.playing);
    CHECK_EQ(c.toggles, 1);
    click(ui, build_note, &c, B.x + 8 + 17, B.y + B.h / 2);
    CHECK(!c.note.playing);
    CHECK_EQ(c.toggles, 2);
    /* seek: a click on the waveform writes the position in seconds */
    int cols = (int)(W.w / 3.6f);
    float left = W.x + (W.w - cols * 3.6f) / 2;
    click(ui, build_note, &c, left + cols * 3.6f * 0.5f, W.y + W.h / 2);
    CHECK_NEAR(c.note.position, 5.0, 0.15);
    CHECK(c.seeks >= 1);
    /* the arrows step a second */
    c.note.position = 5;
    key(ui, build_note, &c, CUI_KEY_RIGHT, 0);
    CHECK_NEAR(c.note.position, 6.0, 1e-4);
    key(ui, build_note, &c, CUI_KEY_LEFT, 0);
    key(ui, build_note, &c, CUI_KEY_LEFT, 0);
    CHECK_NEAR(c.note.position, 4.0, 1e-4);
    key(ui, build_note, &c, CUI_KEY_HOME, 0);
    CHECK_NEAR(c.note.position, 0.0, 1e-6);
    /* the app's position moves the playhead; nothing seeks by itself */
    int seeks = c.seeks;
    c.note.position = 7.5f;
    run(ui, build_note, &c, 3);
    CHECK_EQ(c.seeks, seeks);
    CHECK_NEAR(c.note.position, 7.5, 1e-6);
    /* speed: 0 reads as 1; the chip goes 1.5, 2, 1 */
    cui_rect S = cui_get_rect(ui, cui_make_id(ui, "note") ^ 0x5BEEu);
    CHECK(S.w > 0 && S.x > W.x + W.w);
    click_rect(ui, build_note, &c, S);
    CHECK_NEAR(c.note.speed, 1.5, 1e-6);
    click_rect(ui, build_note, &c, S);
    CHECK_NEAR(c.note.speed, 2.0, 1e-6);
    click_rect(ui, build_note, &c, S);
    CHECK_NEAR(c.note.speed, 1.0, 1e-6);
    CHECK_EQ(c.speeds, 3);
    key(ui, build_note, &c, CUI_KEY_ENTER, 0);   /* the chip kept focus: Enter moves it on */
    CHECK_NEAR(c.note.speed, 1.5, 1e-6);
    /* an odd speed snaps to the nearest step before moving on */
    c.note.speed = 1.9f;
    click_rect(ui, build_note, &c, S);
    CHECK_NEAR(c.note.speed, 1.0, 1e-6);
    /* no length: the waveform cannot move the position */
    c.note.duration = 0; c.note.position = 0;
    run(ui, build_note, &c, 1);
    click(ui, build_note, &c, left + cols * 3.6f * 0.7f, W.y + W.h / 2);
    CHECK_NEAR(c.note.position, 0.0, 1e-6);
    /* yours sits at the end of the row */
    c.mine = true; c.note.duration = 10;
    run(ui, build_note, &c, 2);
    cui_rect M = cui_get_rect(ui, cui_make_id(ui, "note"));
    CHECK_NEAR(M.x + M.w, 500 - 20, 1.0);
    /* phone width: it narrows, and still has room for its waveform */
    cui_set_viewport(ui, 280, 200, 1);
    run(ui, build_note, &c, 3);
    M = cui_get_rect(ui, cui_make_id(ui, "note"));
    W = cui_get_rect(ui, scoped_id(ui, "note", "wave"));
    CHECK(M.w <= 240.5f);
    CHECK(W.w > 60);
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* the composer's recorder                                                  */

typedef struct { char draft[64]; bool recording; float lv[30]; int flags, records, cancels, voices, sends; } rec_case;

static void build_rec(cui_ctx *ui, void *u) {
    rec_case *c = u;
    int f = cui_chat_composer(ui, "comp", c->draft, sizeof c->draft, .mic = true, .recording = c->recording,
                              .levels = c->lv, .level_count = 30, .record_seconds = 4.2f);
    c->flags = f;
    if (f & CUI_COMPOSER_RECORD) { c->records++; c->recording = true; }
    if (f & CUI_COMPOSER_CANCEL) { c->cancels++; c->recording = false; }
    if (f & CUI_COMPOSER_VOICE) { c->voices++; c->recording = false; }
    if (f & CUI_COMPOSER_SEND) c->sends++;
}

static void test_recorder(void) {
    printf("composer recorder: the mic starts it, Cancel and Escape discard, Send sends the take\n");
    cui_ctx *ui = new_ctx(560, 300, false, CUI_ACCENT_BLUE);
    rec_case c;
    memset(&c, 0, sizeof c);
    for (int k = 0; k < 30; k++) c.lv[k] = 0.5f;
    run(ui, build_rec, &c, 2);
    cui_id id = cui_make_id(ui, "comp");
    cui_rect C = cui_get_rect(ui, id);
    cui_rect mic = cui_get_rect(ui, id ^ 0x41C0u), send = cui_get_rect(ui, id ^ 0x5E7Du);
    CHECK(mic.w > 0 && mic.x + mic.w <= send.x);
    /* record, then cancel */
    click_rect(ui, build_rec, &c, mic);
    CHECK_EQ(c.records, 1);
    run(ui, build_rec, &c, 2);
    cui_rect C2 = cui_get_rect(ui, id);
    CHECK_NEAR(C2.h, C.h, 0.5);   /* the recorder takes the place of the text and toolbar, at their height */
    cui_rect cancel = cui_get_rect(ui, id ^ 0xCA9Cu);
    cui_rect wave = cui_get_rect(ui, scoped_id(ui, "comp", "levels"));
    CHECK(cancel.w > 0 && wave.w > 200);
    int rc = c.records;
    click_rect(ui, build_rec, &c, mic);   /* no mic while recording: where it was, nothing records */
    CHECK_EQ(c.records, rc);
    CHECK(c.recording);
    click_rect(ui, build_rec, &c, cancel);
    CHECK_EQ(c.cancels, 1);
    CHECK_EQ(c.voices, 0);
    run(ui, build_rec, &c, 2);
    /* record, then send */
    click_rect(ui, build_rec, &c, cui_get_rect(ui, id ^ 0x41C0u));
    run(ui, build_rec, &c, 1);
    click_rect(ui, build_rec, &c, cui_get_rect(ui, id ^ 0x5E7Du));
    CHECK_EQ(c.voices, 1);
    CHECK_EQ(c.sends, 0);   /* a voice send is not a text send */
    run(ui, build_rec, &c, 2);
    /* from the keyboard: Enter on the mic, focus moves to send; Escape discards */
    int r0 = c.records, c0 = c.cancels, v0 = c.voices;
    move(ui, build_rec, &c, 5, 5);
    int tabs = 0;
    while (tabs < 8 && c.records == r0) {   /* Tab along to the mic, Enter on each stop */
        key(ui, build_rec, &c, CUI_KEY_TAB, 0);
        key(ui, build_rec, &c, CUI_KEY_ENTER, 0);
        tabs++;
    }
    CHECK_EQ(c.records, r0 + 1);
    CHECK_EQ(c.sends, 0);
    CHECK(c.recording);
    run(ui, build_rec, &c, 1);
    key(ui, build_rec, &c, CUI_KEY_ESCAPE, 0);
    CHECK_EQ(c.cancels, c0 + 1);
    CHECK(!c.recording);
    run(ui, build_rec, &c, 1);
    key(ui, build_rec, &c, CUI_KEY_ENTER, 0);   /* focus went back to the mic */
    CHECK_EQ(c.records, r0 + 2);
    run(ui, build_rec, &c, 1);
    key(ui, build_rec, &c, CUI_KEY_ENTER, 0);   /* and from the mic to send, which sends the take */
    CHECK_EQ(c.voices, v0 + 1);
    CHECK(!c.recording);
    /* without `mic`, the composer is as it was: no mic button */
    cui_destroy(ui);
}

/* ------------------------------------------------------------------------ */
/* voice session                                                            */

typedef struct { cui_voice_state state; float lv[40]; uint8_t own[40]; int n; bool muted, use_mute, no_end; int flags, mutes, ends; } session_case;

static void build_session(cui_ctx *ui, void *u) {
    session_case *c = u;
    int f = cui_voice_session(ui, "call", c->state, c->lv, c->n, .owners = c->own, .transcript = "Clear and cool,", .upcoming = " warming later.",
                              .speaker = CUI_VOICE_REPLY, .muted = c->use_mute ? &c->muted : NULL, .no_end = c->no_end);
    c->flags = f;
    if (f & CUI_VOICE_SESSION_MUTE) c->mutes++;
    if (f & CUI_VOICE_SESSION_END) c->ends++;
}

static void test_session(void) {
    printf("voice session: owners color the timeline, the state dot follows the state, Mute and End report\n");
    cui_ctx *ui = new_ctx(600, 400, false, CUI_ACCENT_BLUE);
    session_case c;
    memset(&c, 0, sizeof c);
    /* you, a silent gap while it thinks, then the assistant */
    for (int k = 0; k < 40; k++) {
        c.own[k] = k < 25 ? CUI_VOICE_YOU : CUI_VOICE_REPLY;
        c.lv[k] = k >= 15 && k < 25 ? CUI_VOICE_SILENT : 1.0f;
    }
    c.n = 40; c.state = CUI_VOICE_SPEAKING; c.use_mute = true;
    run(ui, build_session, &c, 2);
    cui_rect R = cui_get_rect(ui, cui_make_id(ui, "call"));
    cui_rect W = cui_get_rect(ui, scoped_id(ui, "call", "wave"));
    CHECK(W.w > 400 && W.y > R.y);
    int cols = (int)(W.w / 3.6f);
    float left = W.x + (W.w - cols * 3.6f) / 2, cy = W.y + W.h / 2;
    image im = render(ui);
    /* the newest 40 columns at the right: 0..14 yours, 15..24 silent, 25..39 the assistant's */
    float x_you = left + (cols - 40 + 5 + 0.5f) * 3.6f, x_gap = left + (cols - 40 + 20 + 0.5f) * 3.6f, x_reply = left + (cols - 40 + 32 + 0.5f) * 3.6f;
    CHECK(grayish(im, x_you, cy - 7.2f) && ink(im, x_you, cy - 7.2f) > 250);
    CHECK(ink(im, x_gap, cy - 7.2f) < 10);   /* thinking: a flat gap, no made-up wave */
    CHECK(bluish(im, x_reply, cy - 7.2f));
    /* the state dot: the accent while the assistant speaks */
    float dot_x = R.x + 4, dot_y = W.y - 12 - 8;
    float best_y = dot_y;
    for (float y = R.y; y < W.y; y += 1) if (bluish(im, dot_x, y)) { best_y = y; break; }
    CHECK(bluish(im, dot_x, best_y));
    free(im.px);
    c.state = CUI_VOICE_LISTENING;
    run(ui, build_session, &c, 1);
    im = render(ui);
    CHECK(!bluish(im, dot_x, best_y + 1) && ink(im, dot_x, best_y + 1) > 300);   /* the foreground while it listens */
    free(im.px);
    c.state = CUI_VOICE_MUTED;
    run(ui, build_session, &c, 1);
    im = render(ui);
    CHECK(grayish(im, dot_x, best_y + 1) && ink(im, dot_x, best_y + 1) < 500);   /* muted gray */
    free(im.px);
    /* Mute flips the app's flag and keeps its id when its label changes */
    cui_id mute = scoped_id(ui, "call", "Mute###mute"), end = scoped_id(ui, "call", "End###end");
    cui_rect MB = cui_get_rect(ui, mute), EB = cui_get_rect(ui, end);
    CHECK(MB.w > 0 && EB.w > 0 && MB.y > W.y + W.h);
    click_rect(ui, build_session, &c, MB);
    CHECK(c.muted);
    CHECK_EQ(c.mutes, 1);
    run(ui, build_session, &c, 1);
    key(ui, build_session, &c, CUI_KEY_SPACE, 0);   /* still focused after it reads Unmute */
    CHECK(!c.muted);
    CHECK_EQ(c.mutes, 2);
    click_rect(ui, build_session, &c, cui_get_rect(ui, end));
    CHECK_EQ(c.ends, 1);
    CHECK(!c.muted);
    /* no mute flag: no Mute button; no_end: no End button */
    c.use_mute = false; c.no_end = true;
    run(ui, build_session, &c, 2);
    click_rect(ui, build_session, &c, MB);
    click_rect(ui, build_session, &c, EB);
    CHECK_EQ(c.mutes, 2);
    CHECK_EQ(c.ends, 1);
    CHECK(!c.muted);
    cui_destroy(ui);
}

/* every accent and both themes build and draw without trouble, and right to left too */
static void test_themes(void) {
    printf("voice: every accent, both themes, right to left\n");
    for (int a = 0; a < CUI_ACCENT_COUNT; a++)
        for (int dark = 0; dark < 2; dark++) {
            cui_ctx *ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(g_sw), .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                                     .accent = (cui_accent)a, .direction = a == CUI_ACCENT_GREEN ? CUI_DIRECTION_RTL : CUI_DIRECTION_LTR });
            cui_set_viewport(ui, 600, 700, 1);
            cui_set_reduced_motion(ui, true);
            cui_input_mouse_leave(ui);
            session_case s;
            memset(&s, 0, sizeof s);
            for (int k = 0; k < 40; k++) { s.lv[k] = 0.6f; s.own[k] = (uint8_t)(k & 1); }
            s.n = 40; s.state = CUI_VOICE_THINKING; s.use_mute = true;
            run(ui, build_session, &s, 2);
            note_case c;
            memset(&c, 0, sizeof c);
            c.note.levels = s.lv; c.note.count = 40; c.note.duration = 3; c.mine = dark;
            run(ui, build_note, &c, 2);
            cui_rect B = cui_get_rect(ui, cui_make_id(ui, "note"));
            CHECK(B.w > 200);
            if (a == CUI_ACCENT_GREEN) {   /* right to left: the play button is at the right end of the bubble */
                cui_rect P = cui_get_rect(ui, cui_make_id(ui, "note") ^ 0x9A1Bu);
                CHECK(P.x > B.x + B.w / 2);
            }
            image im = render(ui);
            free(im.px);
            cui_destroy(ui);
        }
}

int main(void) {
    g_sw = cui_sw_create();
    test_resample();
    test_wave_drawing();
    test_wave_owners();
    test_wave_seek();
    test_voice_note();
    test_recorder();
    test_session();
    test_themes();
    cui_sw_destroy(g_sw);
    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
