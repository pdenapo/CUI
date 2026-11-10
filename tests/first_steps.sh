#!/bin/sh
# The first steps of docs/getting-started.md, run as a newcomer would run them: the code
# and the commands are taken from the guide itself, so the guide cannot drift from what
# works. Linux only (the commands under test are the Linux ones).
#
#   tests/first_steps.sh [work-dir]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${1:-$ROOT/build/first-steps}
rm -rf "$WORK"; mkdir -p "$WORK"
GUIDE=$ROOT/docs/getting-started.md
say() { printf '\n== %s\n' "$1"; }

# block <heading> <language> <n>: the n-th fenced block of that language under a heading
block() {
    python3 - "$GUIDE" "$1" "$2" "$3" <<'EOF'
import re, sys
text, heading, lang, n = open(sys.argv[1]).read(), sys.argv[2], sys.argv[3], int(sys.argv[4])
m = re.search(r"^#+ " + re.escape(heading) + r"\s*$", text, re.M)
if not m: sys.exit(f"no heading '{heading}' in the guide")
rest = text[m.end():]
nxt = re.search(r"^#+ ", rest, re.M)
if nxt: rest = rest[:nxt.start()]
blocks = re.findall(r"^```" + lang + r"\n(.*?)^```", rest, re.M | re.S)
if len(blocks) < n: sys.exit(f"'{heading}' has {len(blocks)} {lang} blocks, wanted {n}")
sys.stdout.write(blocks[n - 1])
EOF
}

say "the guide's first window"
block "2. Your first window" c 1 > "$WORK/main.c"
grep -q cui_app_create "$WORK/main.c"

say "the release tag in the install snippets is this version"
VER=$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' "$ROOT/CMakeLists.txt" | head -1)
for f in "$ROOT/README.md" "$GUIDE"; do
    grep -q "GIT_TAG v$VER" "$f" || { echo "$f: the FetchContent snippet does not name v$VER"; exit 1; }
done
echo "v$VER"

say "Without CMake: the command as written"
D=$WORK/nocmake; mkdir -p "$D"; cp "$WORK/main.c" "$D/"; ln -s "$ROOT/include" "$D/include"; ln -s "$ROOT/src" "$D/src"
block "Without CMake" sh 1 > "$D/build.sh"
(cd "$D" && sh -e build.sh && test -x my_app)

say "One file: the two commands as written, with the include the guide says to use"
D=$WORK/onefile; mkdir -p "$D"
python3 "$ROOT/tools/amalgamate.py" "$D" > /dev/null
sed -e 's|#include <cui/cui.h>|#include "cui.h"|' -e '/#include <cui\/cui_app.h>/d' "$WORK/main.c" > "$D/main.c"
block "One file" sh 1 > "$D/build.sh"
(cd "$D" && sh -e build.sh && test -x my_app)
size=$(stat -c %s "$D/my_app")
echo "my_app: $size bytes"
[ "$size" -lt 400000 ] || { echo "the guide says about 265 KB"; exit 1; }

say "With CMake: the guide's CMakeLists.txt, and cui_app_target keeps the app small"
D=$WORK/cmake; mkdir -p "$D"; cp "$WORK/main.c" "$D/"; ln -s "$ROOT" "$D/CUI"
block "With CMake (recommended)" cmake 1 > "$D/CMakeLists.txt"
grep -q cui_app_target "$D/CMakeLists.txt"
cmake -S "$D" -B "$D/build" -DCMAKE_BUILD_TYPE=MinSizeRel -DCUI_BUILD_EXAMPLES=OFF -DCUI_BUILD_TESTS=OFF > /dev/null
cmake --build "$D/build" -j 4 > /dev/null
strip "$D/build/my_app"
size=$(stat -c %s "$D/build/my_app")
echo "my_app: $size bytes"
[ "$size" -lt 350000 ] || { echo "the guide says about 250 KB"; exit 1; }

say "An installed CUI through find_package"
D=$WORK/installed; mkdir -p "$D/app"; cp "$WORK/main.c" "$D/app/"
cmake -S "$ROOT" -B "$D/build" -DCMAKE_BUILD_TYPE=Release -DCUI_BUILD_EXAMPLES=OFF -DCUI_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$D/prefix" > /dev/null
cmake --build "$D/build" -j 4 > /dev/null
cmake --install "$D/build" > /dev/null
cat > "$D/app/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.14)
project(my_app C)
find_package(cui REQUIRED)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE cui::cui)
cui_app_target(my_app)
EOF
cmake -S "$D/app" -B "$D/app/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$D/prefix" > /dev/null
cmake --build "$D/app/build" > /dev/null
test -x "$D/app/build/my_app"

say "all first steps work"
