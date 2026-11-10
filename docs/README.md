# CUI documentation

1. [Getting started](getting-started.md): install, build and the app loop
2. [Tutorial](tutorial.md): build a small, complete app step by step
3. [Layout](layout.md): rows, columns, sizes, alignment, grids, scrolling and split views
4. [Components](components.md) and [More components](components-more.md): every component with screenshots, options and keyboard support
5. [Theming](theming.md): tokens, light and dark, accents, fonts and motion
6. [Icons](icons.md): the built-in Lucide icons and using any other icon
7. [Custom components](custom-components.md): canvas, drawing, state and animation
8. [Integration](integration.md): your own window, input and renderer, C++, and binary size
9. [Rendering](rendering.md): vertex format and the reference shader
10. [Performance and memory](performance.md): what an app costs, measured
11. [Design principles](design.md): the rules the components follow
12. [How CUI is built](architecture.md): the frame lifecycle and where things live, for people changing the library
13. [Contributing](CONTRIBUTING.md): reporting issues and submitting changes, for people and AI agents

To browse this guide with every component running live, run `web/tools/serve.sh` (see [`web/README.md`](../web/README.md)).

The API itself is documented inline in [`include/cui/cui.h`](../include/cui/cui.h), [`cui_app.h`](../include/cui/cui_app.h) and [`cui_render.h`](../include/cui/cui_render.h). On the [site](https://yappologistic.github.io/CUI/), each component's page lists its options and the structs it takes and returns, and the [function reference](https://yappologistic.github.io/CUI/#/reference) lists every public function.
