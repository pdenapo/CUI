// Compiles the public headers as C++ and uses the *_ex functions with
// designated initializers (C++20).
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <cui/cui_render.h>

void cpp_ui(cui_ctx *ui) {
    cui_box_begin(ui, nullptr);
    cui_button_opts o{};
    o.variant = CUI_BUTTON_PRIMARY;
    o.icon = CUI_ICON_PLUS;
    if (cui_button_ex(ui, "Add", &o)) {}
    cui_text_opts t{};
    t.color = CUI_COLOR_TEXT_SECONDARY;
    cui_text_ex(ui, "From C++", &t);
    cui_color c = CUI_RGB(0x7747ff);
    cui_size s = CUI_PX(10);
    (void)c; (void)s;
    cui_box_end(ui);
}

// The app of docs/integration.md, "From C++": the same calls, with the options
// structs filled in by hand (the named-argument macros are C only).
static void frame(cui_ctx *ui, void *) {
    cui_box page{};
    page.pad = 24;
    page.gap = 12;
    cui_column_begin(ui, &page);
    cui_heading(ui, "Welcome", 2);
    static bool notify = true;
    cui_switch_ex(ui, "Email me when a build fails", &notify, nullptr);
    cui_button_opts save{};
    save.variant = CUI_BUTTON_PRIMARY;
    if (cui_button_ex(ui, "Save", &save)) { /* ... */ }
    cui_box_end(ui);
}

int cpp_app() {   // not called here: it opens a window
    cui_app_desc desc{};
    desc.title = "My app";
    return cui_app_run(&desc, frame, nullptr);
}

int main() {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx_desc d{};
    d.renderer = cui_sw_iface(sw);
    cui_ctx *ui = cui_create(&d);
    cui_begin_frame(ui, 1.0f / 60);
    cpp_ui(ui);
    frame(ui, nullptr);
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_sw_destroy(sw);
    return 0;
}
