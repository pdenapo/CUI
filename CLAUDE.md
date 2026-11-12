# CUI: guide for AI agents

CUI is an immediate-mode UI library in C99: about 210 components, a software and an OpenGL renderer, Win32, Cocoa, X11, SDL2 and GLFW backends. The human docs are the source of truth; this file only points at them and lists what agents most often get wrong.

## Read first

- `docs/architecture.md`: the frame lifecycle, where things live, the outline every component follows, how to change code safely.
- `docs/CONTRIBUTING.md`: disclosure and evidence rules, code style, "Keeping the code readable", repository and release rules. The section "If you are an AI agent" applies to you.
- `src/cui_internal.h`: starts with a table of contents of the internal API and the file that implements each part.

## Commands

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j8
ctest --test-dir build --output-on-failure                 # also with -DCUI_TEXT_SHAPING=ON
./build/tests/cui_component_shots /tmp/shots [scene]       # component images
./build/tests/cui_screenshots /tmp/shots                   # gallery pages
```

Behavior fingerprint for refactors (deterministic; compare old and new library builds):

```sh
cc -O1 -DCUI_SOAK_NO_X11 -DSOAK_APP='"../examples/mail.c"' -I include -I src tests/soak.c build/libcui.a -lm -lpthread -ldl -o soak-mail
./soak-mail --minutes 2 --seed 5 --hash
```

## Rules

- **Search before you write a helper.** Shared logic lives once in `cui_internal.h`: `CUI_C` for theme colors, `cui__emphasis` for the selected or active color, `cui__fs` for font sizes, `CUI__T_*` and `CUI__SPRING_*` for motion, `cui__draw_text_at` for text placed by a point. Do not copy a helper into another file.
- **Follow the component outline** in `docs/architecture.md`: options struct with a zero default, `_ex` function plus macro in `cui.h`, one state struct, paint data in frame memory, theme tokens instead of raw colors.
- **Split long functions by phase** (input, build, paint) with shared values in one struct. Do not add loose fields to `cui_ctx`; begin/end components use `ui->building`.
- **Names are unique across all files**, statics included, and no name is a word the Windows SDK defines as a macro (`small`, `near`, `far`, `interface`, `ERROR`... see `tests/windows_sdk_words.h`): `tools/amalgamate.py` compiles the library as one file, and CI compiles it with those macros defined.
- **The core never calls into component files.** Optional parts install a hook (`ui->emoji_hook`).
- **Refactors must not change behavior.** Show it: ctest (shaping off and on), screenshots byte-identical before and after, and the soak `--hash` fingerprint unchanged. Keep refactors and behavior changes in separate commits.
- **Size matters.** Check `examples/hello` in a `MinSizeRel` build when you add code to the core.
- **Never commit secrets or private links:** no API keys, tokens, session URLs or private URLs in code, docs, commit messages or test data.
- **Commits** end with a `Co-Authored-By` trailer naming the model that wrote them, and nothing else.
- **Every visible change** gets a line in `CHANGELOG.md`, and its docs page and images in the same commit.
