# CUI documentation

1. [Getting started](getting-started.md): install, build and the app loop
2. [Tutorial](tutorial.md): build a small, complete app step by step
3. [Building an app](apps.md): the window, when frames happen, what components return, threads, shipping
4. [Recipes](recipes.md): complete programs for a form, a sorted table, a list with a detail pane, a background task, a clock and saved settings
5. [Layout](layout.md): rows, columns, sizes, alignment, grids, scrolling and split views
6. [Components](components.md) and [More components](components-more.md): every component, what it is, its options and keyboard support
7. [Theming](theming.md): tokens, light and dark, accents, fonts and motion
8. [Icons](icons.md): the built-in Lucide icons and using any other icon
9. [Custom components](custom-components.md): canvas, drawing, state and animation
10. [Integration](integration.md): your own window, input and renderer, C++, and binary size
11. [Rendering](rendering.md): vertex format and the reference shader
12. [Performance and memory](performance.md): what an app costs, measured
13. [Questions](faq.md): what CUI is and is not, how it compares, what it cannot do
14. [Design principles](design.md): the rules the components follow
15. [How CUI is built](architecture.md): the frame lifecycle and where things live, for people changing the library
16. [Contributing](CONTRIBUTING.md): reporting issues and submitting changes, for people and AI agents

To browse this guide with every component running live, run `web/tools/serve.sh` (see [`web/README.md`](../web/README.md)).

The API itself is documented inline in [`include/cui/cui.h`](../include/cui/cui.h), [`cui_app.h`](../include/cui/cui_app.h) and [`cui_render.h`](../include/cui/cui_render.h). On the [site](https://yappologistic.github.io/CUI/), each component's page lists its options and the structs it takes and returns, and the [function reference](https://yappologistic.github.io/CUI/#/reference) lists every public function.
