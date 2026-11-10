/* Caffeine: how much is still in you at bedtime. Drag a cup along the time axis, click
 * the chart to add one, double-click a cup to remove it, and the curve, the peak and the
 * verdict follow. It shows the instrument components together: a dot-matrix chart with
 * annotations, an event track under it, readouts and a dot-matrix display. */
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <math.h>
#include <stdio.h>

#define MAX_CUPS   12
#define SAMPLES    145                   /* every 10 minutes from 06:00 to 06:00 */
#define DAY_START  6.0
#define DAY_END    30.0
#define BEDTIME    23.0
#define LIMIT_MG   50.0f                 /* what still lets you fall asleep, for this example */
#define CUP_MG     95.0
#define HALF_LIFE  5.0                   /* hours */

typedef struct {
    double cups[MAX_CUPS];               /* hours since midnight, sorted by the event track */
    int count, selected;
    float curve[MAX_CUPS][SAMPLES];      /* each cup's share, one series per cup */
} app_state;

/* one cup's caffeine at `t` hours after drinking it: absorbed over about 45 minutes, then
 * cleared with a half-life of five hours */
static double cup_level(double t) {
    if (t <= 0) return 0;
    double ke = log(2.0) / HALF_LIFE, ka = log(2.0) / 0.25;
    return CUP_MG * ka / (ka - ke) * (exp(-ke * t) - exp(-ka * t));
}

static double level_at(const double *cups, int n, double at) {
    double s = 0;
    for (int i = 0; i < n; i++) s += cup_level(at - cups[i]);
    return s;
}

static const char *clock_text(char *buf, size_t cap, double h) {
    int m = (int)lround(h * 60) % (24 * 60);
    snprintf(buf, cap, "%02d:%02d", m / 60, m % 60);
    return buf;
}

static void frame(cui_ctx *ui, void *user) {
    app_state *a = user;
    char t1[16], t2[16], line1[64], line2[64], verdict[64];

    /* the model: every cup's curve, the total's peak and what is left at bedtime */
    double peak = 0, peak_at = DAY_START;
    for (int s = 0; s < SAMPLES; s++) {
        double at = DAY_START + (DAY_END - DAY_START) * s / (SAMPLES - 1), total = 0;
        for (int c = 0; c < a->count; c++) { a->curve[c][s] = (float)cup_level(at - a->cups[c]); total += a->curve[c][s]; }
        if (total > peak) { peak = total; peak_at = at; }
    }
    double at_bed = level_at(a->cups, a->count, BEDTIME);

    /* the verdict: the latest time the last cup could be drunk and still be under the line */
    if (a->count == 0) snprintf(verdict, sizeof verdict, "NO CUPS YET");
    else if (level_at(a->cups, a->count - 1, BEDTIME) > LIMIT_MG) snprintf(verdict, sizeof verdict, "OVER THE LINE EVEN WITHOUT THE LAST CUP");
    else {
        double lo = DAY_START, hi = BEDTIME;
        for (int k = 0; k < 40; k++) {
            double mid = (lo + hi) / 2, cups[MAX_CUPS];
            for (int c = 0; c < a->count - 1; c++) cups[c] = a->cups[c];
            cups[a->count - 1] = mid;
            if (level_at(cups, a->count, BEDTIME) <= LIMIT_MG) lo = mid; else hi = mid;
        }
        snprintf(verdict, sizeof verdict, "LAST CUP BY %s", clock_text(t1, sizeof t1, floor(lo * 4) / 4));
    }

    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .align = CUI_ALIGN_CENTER) {
        cui_column(ui, .width = CUI_FILL, .max_width = 880, .height = CUI_FILL, .pad = 28, .gap = 22) {
            cui_row(ui, .width = CUI_FILL, .gap = 28, .wrap = true, .align = CUI_ALIGN_END) {
                cui_column(ui, .gap = 4) {
                    char sub[64];
                    snprintf(sub, sizeof sub, "%d CUPS \xc2\xb7 %.0f MG \xc2\xb7 HALF-LIFE %.0fH", a->count, a->count * CUP_MG, HALF_LIFE);
                    cui_text(ui, "CAFFEINE", .font = CUI_FONT_MONO);
                    cui_text(ui, sub, .font = CUI_FONT_MONO, .color = CUI_COLOR_TEXT_MUTED, .size = 12);
                }
                cui_spacer(ui);
                cui_readout(ui, "PEAK", a->count ? (snprintf(line1, sizeof line1, "%.0f", peak), line1) : "\xe2\x80\x94", .unit = "MG",
                            .detail = a->count ? clock_text(t2, sizeof t2, peak_at) : "", .size = CUI_SIZE_SM);
                cui_readout(ui, "AT BED", (snprintf(line2, sizeof line2, "%.0f", at_bed), line2), .unit = "MG", .size = CUI_SIZE_SM,
                            .accent = at_bed > LIMIT_MG, .detail = at_bed > LIMIT_MG ? "OVER THE LINE" : "UNDER THE LINE");
            }

            /* the chart: one series per cup, the selected one in the accent color */
            cui_dot_series series[MAX_CUPS];
            for (int c = 0; c < a->count; c++) series[c] = (cui_dot_series){ a->curve[c], NULL, c == a->selected };
            snprintf(line1, sizeof line1, "PEAK %.0f MG", peak);
            snprintf(line2, sizeof line2, "%.0f MG STILL IN YOU", at_bed);
            cui_chart_annotation notes[3] = {
                { CUI_ANNOTATION_Y, 0, LIMIT_MG, "50", NULL, false },
                { CUI_ANNOTATION_X, BEDTIME, 0, "BED 23:00", line2, false },
                { CUI_ANNOTATION_X, peak_at, 0, line1, clock_text(t2, sizeof t2, peak_at), false },
            };
            cui_column(ui, .width = CUI_FILL, .gap = 0) {
                cui_dot_chart_result r = cui_dot_chart(ui, "level", series, a->count, SAMPLES, .height = 300,
                                                       .x_min = DAY_START, .x_max = DAY_END, .x_units = CUI_AXIS_HOURS,
                                                       .min = 0, .max = 260, .unit = "MG", .no_axis = true,
                                                       .annotations = notes, .annotation_count = a->count ? 3 : 1);
                /* a click on the chart pours a cup there */
                if (r.clicked && a->count < MAX_CUPS) {
                    a->cups[a->count++] = floor(r.x * 4 + 0.5) / 4;
                    for (int i = a->count - 1; i > 0 && a->cups[i] < a->cups[i - 1]; i--) {
                        double t = a->cups[i]; a->cups[i] = a->cups[i - 1]; a->cups[i - 1] = t;
                        a->selected = i - 1;
                    }
                    if (a->selected < 0 || a->cups[a->selected] != floor(r.x * 4 + 0.5) / 4) a->selected = a->count - 1;
                }
                cui_event_track(ui, "cups", a->cups, &a->count, MAX_CUPS, DAY_START, DAY_END, .step = 0.25,
                                .units = CUI_AXIS_HOURS, .label_step = 6, .selected = &a->selected);
            }
            cui_spacer(ui);
            cui_dot_display(ui, verdict, .pitch = 3, .width = CUI_FILL, .accent = a->count && verdict[0] == 'O');
        }
    }
}

int main(void) {
    static app_state a = { { 8.0, 10.75, 12.25 }, 3, 2 };
    return cui_app_run(&(cui_app_desc){ .title = "Caffeine", .width = 900, .height = 700, .theme = CUI_THEME_DARK,
                                        .accent = CUI_ACCENT_BLUE }, frame, &a);
}
