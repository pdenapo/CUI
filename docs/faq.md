# Questions

Short answers to what people ask before they try CUI, including the reasons not to use it.

## What kind of library is it?

CUI is an immediate-mode UI library. You call a function for each component in every frame, and the library lays the screen out, handles input and draws it. It keeps no tree of widgets for you to create and update. The state is your own variables.

It draws everything itself, with OpenGL or its own software renderer. It does not use the system's buttons, menus or text fields.

## Then why does it say "native"?

Because the app is compiled machine code in a real window, with no browser, no web view and no interpreter inside. It does not mean native widgets. A CUI app looks the same on Windows, macOS and Linux, and does not look like the system's own apps. If your app has to look and behave exactly like a platform's own, use that platform's toolkit.

## How is it different from Dear ImGui or Nuklear?

They are immediate-mode libraries too, and CUI works the way they do. The difference is the target. Dear ImGui and Nuklear are mostly used for tools and debug panels inside a program that already has a window and a renderer. CUI is meant for the app itself, so it brings the window and the event loop, and its components carry what end users expect:

- text fields with selection, undo and the system's clipboard;
- animation, light and dark themes and eight accents;
- layouts that adapt to the window's width and mirror for right-to-left languages;
- larger components such as a data grid, a code editor, charts and docking panels.

If you want an overlay inside a game engine, Dear ImGui has many more backends and a much larger community. CUI can also draw through your own renderer ([Integration](integration.md)).

## How is it different from Qt, GTK or wxWidgets?

Those keep a tree of widgets, and they are far more mature. They have screen reader support, complete input-method support, printing, and many years of use. CUI has none of those. What CUI offers instead is size (a complete app is about 250 KB, with nothing to install), a C API with no code generation, and components that are designed together.

## What can it not do?

- **Screen readers.** There is no accessibility tree. Blind users cannot use a CUI app.
- **All of the keyboard.** Most components work from the keyboard and each page lists the keys. Some still need a pointer, such as the rating and the reorderable list; the [README](../README.md#limitations) lists them.
- **Complex scripts everywhere.** Arabic-script shaping and bidirectional text are an optional, experimental build option for static text, and editing such text is not supported. CJK needs a fallback font that you add.
- **Inline input-method composition.** The text being composed shows in the system's window, not in the field.
- **Native Wayland, Android and iOS.** On Wayland use the SDL2 or GLFW backend, or XWayland. There is no platform layer for phones.

## Is it ready for real use?

It is new. Version 1.0 came out in November 2026, so it has little real-world use behind it. What stands in for that is testing: every push runs unit, interaction and render tests on Windows, macOS and Linux, and a fuzzer that drives the example apps with random input under AddressSanitizer and UndefinedBehaviorSanitizer. [Performance](performance.md) describes hour-long runs that watch memory and CPU.

For a tool you ship to your own team, that is likely enough. For an app with paying users, try it on a small part first.

## Who writes the code?

Most of it is written by AI models under the maintainer's direction. Every commit names the model in a `Co-Authored-By` trailer, so the history shows it. The [contribution rules](CONTRIBUTING.md) ask everyone who sends a change for the same disclosure, and for evidence they produced themselves.

## How small is it, really?

`examples/hello`, a window with a few components, is about 250 KB on Linux as a stripped executable, with the fonts and icons inside. The whole library with every component is about 1,050 KB. An app carries only the components it calls, provided it is linked the way [Getting started](getting-started.md) shows. CI builds the guide's commands and fails if the result grows past its limit.

## Does it work from C++?

Yes. The headers compile as C++, and the tests build a C++ program against them. The named-argument form (`.variant = ...`) is C99, so from C++ you call the `_ex` functions with a struct: [From C++](integration.md#from-c).

## Can I use my own window or renderer?

Yes. The core takes input through `cui_input_*` calls and gives back a list of triangles, so it runs inside an engine or on top of any graphics API. [Integration](integration.md) covers the loop and [Rendering](rendering.md) the vertex format.

## Does it run in a browser?

Yes, compiled to WebAssembly with plain clang, drawing through WebGL2 or the software renderer. The [live site](https://yappologistic.github.io/CUI/) is built that way. [Getting started](getting-started.md#run-in-the-browser) has the steps.

## What does the license ask of me?

CUI is MIT licensed: keep the copyright notice. The fonts and icons inside your executable are under the SIL Open Font License and the ISC license, which ask you to keep their notices too. [THIRD_PARTY.md](../THIRD_PARTY.md) lists them.

## Where do I report a problem?

In the [issue tracker](https://github.com/yappologistic/CUI/issues). A report needs the commands you ran and what they printed; [Contributing](CONTRIBUTING.md) explains why.
