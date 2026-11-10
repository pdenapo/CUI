#!/usr/bin/env bash
# Builds the live component site: CUI compiled to WebAssembly with plain
# clang + wasm-ld (no Emscripten, no sysroot), plus the page catalog.
#   web/tools/build.sh            build everything
#   web/tools/build.sh scenes     only the component scenes module
#   web/tools/build.sh app my_app.c [more.c...]
#                                 your own app (its main calls cui_app_run), opened at #/app/my_app
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
OUT="$HERE/build"
SITE="$HERE/site"
mkdir -p "$OUT/obj" "$SITE/wasm"

CC=${CC:-clang}
JOBS=${JOBS:-$(nproc)}
CFLAGS=(--target=wasm32 -O2 -std=c99 -g0
        -mbulk-memory -mnontrapping-fptoint -msign-ext -msimd128
        -fno-stack-protector -fno-exceptions -ffunction-sections -fdata-sections
        -isystem "$HERE/shim/include" -I "$ROOT/include" -I "$ROOT/src" -I "$ROOT/tests" -I "$ROOT/examples"
        -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-c11-extensions)
LDFLAGS=(--no-entry --gc-sections --strip-all -z stack-size=4194304 --initial-memory=16777216 --max-memory=4294967296)

compile() {   # src obj [extra flags...]
    local src=$1 obj=$2; shift 2
    if [[ ! -f "$obj" || "$src" -nt "$obj" || "$HERE/shim/include/math.h" -nt "$obj" \
          || "$ROOT/include/cui/cui.h" -nt "$obj" || "$ROOT/src/cui_internal.h" -nt "$obj" ]]; then
        "$CC" "${CFLAGS[@]}" "$@" -c "$src" -o "$obj"
    fi
}
export -f compile
export CC HERE ROOT

# 1. the library and the tiny libc
LIB_SRCS=("$ROOT"/src/cui_*.c "$ROOT/src/render/cui_render_sw.c")
pids=()
for s in "${LIB_SRCS[@]}"; do
    compile "$s" "$OUT/obj/lib_$(basename "${s%.c}").o" &
    pids+=($!)
    if (( ${#pids[@]} >= JOBS )); then wait "${pids[0]}"; pids=("${pids[@]:1}"); fi
done
compile "$HERE/shim/libc.c" "$OUT/obj/libc.o" -fno-builtin &
compile "$HERE/host/web_host.c" "$OUT/obj/web_host.o" &
compile "$HERE/host/web_app.c" "$OUT/obj/web_app.o" &
wait
rm -f "$OUT/libcui.a"
llvm-ar rcs "$OUT/libcui.a" "$OUT"/obj/lib_*.o
BASE=("$OUT/obj/web_host.o" "$OUT/libcui.a" "$OUT/obj/libc.o")

link() {   # name objs...
    local name=$1; shift
    wasm-ld "${LDFLAGS[@]}" "$@" "${BASE[@]}" -o "$SITE/wasm/$name.wasm"
    printf '  %-10s %6d KB\n' "$name" $(( $(stat -c %s "$SITE/wasm/$name.wasm") / 1024 ))
}

# your own app: built like the examples, served from the same page
if [[ "${1:-}" == "app" ]]; then
    shift
    [[ $# -ge 1 ]] || { echo "usage: web/tools/build.sh app main.c [more.c...]"; exit 2; }
    name=$(basename "${1%.c}")
    objs=()
    for s in "$@"; do
        o="$OUT/obj/user_${name}_$(basename "${s%.c}").o"
        "$CC" "${CFLAGS[@]}" -I "$(dirname "$s")" -Dmain=web_app_main -c "$s" -o "$o"
        objs+=("$o")
    done
    link "$name" "${objs[@]}" "$OUT/obj/web_app.o"
    mkdir -p "$SITE/src/user" && cp "$1" "$SITE/src/user/$name.c"   # shown beside the app
    if [[ ! -f "$SITE/catalog.js" ]]; then
        python3 "$HERE/tools/gen_catalog.py" "$ROOT" "$SITE/catalog.js"
        ln -sfn "../../assets/fonts" "$SITE/fonts"
        ln -sfn "../../docs/images" "$SITE/images"
    fi
    echo "done: serve web/site (web/tools/serve.sh) and open http://localhost:${PORT:-8765}/#/app/$name"
    exit 0
fi

# 2. the component scenes (tests/component_shots.c plus a few web-only scenes)
python3 "$HERE/tools/gen_scenes.py" "$ROOT" "$OUT/scenes_table.inc"
# always rebuild: the scene table is generated
"$CC" "${CFLAGS[@]}" -I "$OUT" -c "$HERE/host/scenes_web.c" -o "$OUT/obj/scenes_web.o"
echo "wasm modules:"
link scenes "$OUT/obj/scenes_web.o"
[[ "${1:-}" == "scenes" ]] && exit 0

# 3. the gallery and the demo apps
APPS=(gallery dashboard finance music mail notes settings tracker assistant todo caffeine hello)
for app in "${APPS[@]}"; do
    objs=()
    srcs=("$ROOT/examples/$app.c"); [[ $app == hello ]] && srcs=("$HERE/host/hello_web.c")
    [[ $app == gallery ]] && srcs+=("$ROOT/examples/gallery_ui.c" "$ROOT/examples/gallery_more.c")
    for s in "${srcs[@]}"; do
        o="$OUT/obj/app_${app}_$(basename "${s%.c}").o"
        "$CC" "${CFLAGS[@]}" -Dmain=web_app_main -c "$s" -o "$o"
        objs+=("$o")
    done
    link "$app" "${objs[@]}" "$OUT/obj/web_app.o"
done

# 4. page data: docs, scene sources, line numbers
python3 "$HERE/tools/gen_catalog.py" "$ROOT" "$SITE/catalog.js"
# fonts and doc screenshots, linked so they never go stale
# relative links, so the folder works wherever the repository is checked out
ln -sfn "../../assets/fonts" "$SITE/fonts"
ln -sfn "../../docs/images" "$SITE/images"
mkdir -p "$SITE/src" && ln -sfn "../../../examples" "$SITE/src/examples"
# 5. a page of plain text per component and guide, for search engines and link previews
python3 "$HERE/tools/gen_pages.py" "$SITE"
echo "done: $SITE"
