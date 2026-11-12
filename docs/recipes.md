# Recipes

Small, complete programs for things most apps need. Each one compiles as it stands: save it as `main.c` and build it as in [Getting started](getting-started.md). CI builds every program on this page on each push, so they cannot drift from the library.

## A form that validates

A form checks its fields every frame, because the fields are your variables. The one decision is when to show the errors: here, only after the first try, so the form does not open covered in red.

```c
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <string.h>

typedef struct {
    char name[64], email[96];
    bool terms;
    bool tried;      /* Create was pressed at least once: from then on, show what is wrong */
    bool created;
} form;

static const char *email_error(const char *s) {
    const char *at = strchr(s, '@');
    if (!s[0]) return "Enter your email.";
    if (!at || at == s || !strchr(at, '.')) return "This does not look like an email address.";
    return NULL;
}

static void frame(cui_ctx *ui, void *user) {
    form *f = user;
    const char *name_err = f->name[0] ? NULL : "Enter your name.";
    const char *mail_err = email_error(f->email);
    bool valid = !name_err && !mail_err && f->terms;

    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_CENTER, .bg = CUI_COLOR_BACKGROUND) {
        cui_column(ui, .width = CUI_FILL, .max_width = 420, .pad = 24, .gap = 16) {
            cui_heading(ui, "Create an account", 3);
            /* an error is shown only after the first try, so the form does not open covered in red */
            cui_input(ui, "name", f->name, sizeof f->name, .label = "Name", .autofocus = true,
                      .error = f->tried ? name_err : NULL);
            int r = cui_input(ui, "email", f->email, sizeof f->email, .label = "Email", .placeholder = "you@company.com",
                              .error = f->tried ? mail_err : NULL);
            cui_checkbox(ui, "I accept the terms", &f->terms);
            bool create = cui_button(ui, "Create account", .variant = CUI_BUTTON_PRIMARY, .full_width = true);
            if (create || (r & CUI_SUBMITTED)) {          /* the button, or Enter in the last field */
                f->tried = true;
                if (valid) f->created = true;
            }
            if (f->tried && !f->terms) cui_alert(ui, "Accept the terms to continue.", .tone = CUI_TONE_WARNING);
            if (f->created) cui_alert(ui, "Account created.", .tone = CUI_TONE_SUCCESS);
        }
    }
}

int main(void) {
    static form f;
    return cui_app_run(&(cui_app_desc){ .title = "Form", .width = 480, .height = 520 }, frame, &f);
}
```

`.error` shows its text in red under the field and turns the field's border red. `cui_input` returns flags: `CUI_SUBMITTED` is Enter, so the last field submits the form the way the button does.

## A table sorted by its headers

The table does not own or reorder your data. A click on a sortable header writes the column and the direction into your `cui_table_sort`, and you sort.

```c
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *name, *owner; int stars; } repo;

static repo repos[] = {
    { "atlas", "Maya", 412 }, { "birch", "Jonas", 97 }, { "cinder", "Priya", 1280 },
    { "delta", "Aiko", 33 }, { "ember", "Tomas", 640 },
};
enum { REPOS = sizeof repos / sizeof repos[0] };

static cui_table_sort sort = { 0, false };   /* the table writes the clicked column and direction here */

static int compare(const void *a, const void *b) {
    const repo *x = a, *y = b;
    int c = sort.column == 0 ? strcmp(x->name, y->name) : sort.column == 1 ? strcmp(x->owner, y->owner) : x->stars - y->stars;
    return sort.descending ? -c : c;
}

static void frame(cui_ctx *ui, void *user) {
    static int selected = -1;
    static const cui_table_column cols[] = {
        { "Repository", CUI_FILL, CUI_TEXT_LEFT, true },      /* title, width, alignment, sortable */
        { "Owner", CUI_PX(140), CUI_TEXT_LEFT, true },
        { "Stars", CUI_PX(90), CUI_TEXT_RIGHT, true },
    };
    (void)user;
    /* the table shows the rows in the order you give them: sorting is yours */
    qsort(repos, REPOS, sizeof repos[0], compare);

    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 24, .gap = 12, .bg = CUI_COLOR_BACKGROUND) {
        cui_table(ui, "repos", cols, 3, .sort = &sort, .selectable = true) {
            for (int i = 0; i < REPOS; i++) {
                if (cui_table_row(ui, i == selected)) selected = i;
                cui_table_cell_text(ui, repos[i].name);
                cui_table_cell_text(ui, repos[i].owner);
                cui_table_cell_textf(ui, "%d", repos[i].stars);
            }
        }
        if (selected >= 0) cui_textf(ui, "Selected: %s", repos[selected].name);
    }
}

int main(void) {
    return cui_app_run(&(cui_app_desc){ .title = "Table", .width = 560, .height = 380 }, frame, NULL);
}
```

`cui_table_row` returns true when the row is clicked, and its argument says whether to draw it selected. For tens of thousands of rows use the [data grid](components-more.md#data-grid-and-filter-bar), which builds only the rows in view.

## A list and a detail pane

Two panes with a divider the user can drag. The selection is an `int` of yours; the detail pane reads it.

```c
#include <cui/cui.h>
#include <cui/cui_app.h>

typedef struct { const char *title, *from, *body; } note;

static const note notes[] = {
    { "Release plan", "Maya", "Tag on Tuesday, publish the notes on Wednesday." },
    { "Flaky test", "Jonas", "The layout test fails one run in fifty on Windows." },
    { "Design review", "Priya", "Bring the two variants of the settings screen." },
};
enum { NOTES = sizeof notes / sizeof notes[0] };

static void frame(cui_ctx *ui, void *user) {
    static int current = 0;
    static float ratio = 0.36f;      /* the share of the width the list takes; dragging the divider changes it */
    (void)user;
    cui_split(ui, "main", &ratio, .min_first = 180, .min_second = 240, .width = CUI_FILL, .height = CUI_FILL) {
        cui_scroll(ui, .id = "list", .width = CUI_FILL, .height = CUI_FILL, .pad = 8, .gap = 2) {
            for (int i = 0; i < NOTES; i++) {
                cui_push_id_int(ui, i);
                if (cui_list_item(ui, notes[i].title, .subtitle = notes[i].from, .selected = i == current)) current = i;
                cui_pop_id(ui);
            }
        }
        cui_split_next(ui);          /* everything after this goes in the second pane */
        cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 24, .gap = 8) {
            cui_heading(ui, notes[current].title, 3);
            cui_muted(ui, notes[current].from);
            cui_text(ui, notes[current].body);
        }
    }
}

int main(void) {
    return cui_app_run(&(cui_app_desc){ .title = "Notes", .width = 720, .height = 420 }, frame, NULL);
}
```

`ratio` is yours too, so saving it with your settings brings the window back as the user left it. In a list drawn by a loop, wrap each row in an id scope (`cui_push_id_int`), or two rows with the same title would share their state.

## A long task on another thread

A `cui_ctx` belongs to one thread. Do the work on another thread, share plain data with it, and read that data in the frame function. Never call CUI from the worker.

```c
#define _POSIX_C_SOURCE 200809L   /* nanosleep */
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

/* what the worker and the interface share: plain atomics, no CUI calls on the worker's side */
static atomic_int progress;      /* 0 to 100 */
static atomic_bool running;

static void *work(void *arg) {
    (void)arg;
    for (int i = 1; i <= 100; i++) {
        nanosleep(&(struct timespec){ 0, 30 * 1000 * 1000 }, NULL);   /* the real work goes here */
        atomic_store(&progress, i);
    }
    atomic_store(&running, false);
    return NULL;
}

static void frame(cui_ctx *ui, void *user) {
    (void)user;
    bool busy = atomic_load(&running);
    int done = atomic_load(&progress);

    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 24, .gap = 16, .bg = CUI_COLOR_BACKGROUND) {
        if (cui_button(ui, "Start export", .variant = CUI_BUTTON_PRIMARY, .loading = busy) && !busy) {
            pthread_t t;
            atomic_store(&progress, 0);
            atomic_store(&running, true);
            if (pthread_create(&t, NULL, work, NULL) == 0) pthread_detach(t);
            else atomic_store(&running, false);
        }
        cui_progress(ui, done / 100.0f, .label = "Exporting", .show_value = true);
        if (!busy && done == 100) cui_alert(ui, "Export finished.", .tone = CUI_TONE_SUCCESS);
    }
    /* an idle app sleeps until there is input; while the worker runs, ask for a frame
     * every 50 ms so the bar keeps up */
    if (busy) cui_request_wakeup(ui, 0.05f);
}

int main(void) {
    return cui_app_run(&(cui_app_desc){ .title = "Export", .width = 460, .height = 260 }, frame, NULL);
}
```

The call that matters is `cui_request_wakeup`. An app with nothing to animate sleeps until there is input, so without it the bar would only move when the mouse does. This program uses POSIX threads; on Windows use `CreateThread`, or the pthreads that MinGW provides. For more than a number, protect the shared data with a mutex and copy it out at the start of the frame.

## Something that changes with time

A clock, a countdown or a value you poll has no input to wake the app. Ask for the next frame yourself.

```c
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <time.h>

static void frame(cui_ctx *ui, void *user) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    (void)user;
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER, .bg = CUI_COLOR_BACKGROUND) {
        cui_textf(ui, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    }
    /* nothing here animates and there is no input, so without this the app would sleep
     * and the clock would stop: ask for the next frame in a quarter of a second */
    cui_request_wakeup(ui, 0.25f);
}

int main(void) {
    return cui_app_run(&(cui_app_desc){ .title = "Clock", .width = 320, .height = 200 }, frame, NULL);
}
```

Call `cui_request_wakeup` every frame while you need it. The app still sleeps between those frames, so a clock costs four frames a second, not sixty.

## Settings that are remembered

There is no settings store in CUI: the state is your struct, so saving it is writing a file. Apply what you loaded on the first frame, and save when something changes.

```c
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <stdio.h>

typedef struct { int dark, accent; } settings;

static void load(settings *s) {
    FILE *f = fopen("settings.txt", "r");
    if (!f) return;
    if (fscanf(f, "%d %d", &s->dark, &s->accent) != 2) { s->dark = 0; s->accent = 0; }
    if (s->accent < 0 || s->accent >= CUI_ACCENT_COUNT) s->accent = 0;
    fclose(f);
}

static void save(const settings *s) {
    FILE *f = fopen("settings.txt", "w");
    if (!f) return;
    fprintf(f, "%d %d\n", s->dark, s->accent);
    fclose(f);
}

static void frame(cui_ctx *ui, void *user) {
    settings *s = user;
    static const char *accents[] = { "Neutral", "Violet", "Blue", "Green", "Amber", "Orange", "Coral", "Rose" };
    static bool applied;
    bool dark = s->dark != 0, changed = false;

    if (!applied) {              /* the first frame: apply what was loaded */
        cui_set_theme(ui, dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
        cui_set_accent(ui, (cui_accent)s->accent);
        applied = true;
    }
    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .pad = 24, .gap = 16, .bg = CUI_COLOR_BACKGROUND) {
        cui_heading(ui, "Appearance", 3);
        if (cui_switch(ui, "Dark mode", &dark)) {
            s->dark = dark;
            cui_set_theme(ui, dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
            changed = true;
        }
        if (cui_select(ui, "accent", accents, CUI_ACCENT_COUNT, &s->accent, .label = "Accent")) {
            cui_set_accent(ui, (cui_accent)s->accent);
            changed = true;
        }
    }
    if (changed) save(s);
}

int main(void) {
    static settings s;
    load(&s);
    return cui_app_run(&(cui_app_desc){ .title = "Settings", .width = 420, .height = 300 }, frame, &s);
}
```

`cui_switch` and `cui_select` return true on the frame they change the value, which is the moment to apply and save. The theme change animates.
