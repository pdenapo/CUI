#!/bin/sh
# Builds every complete program in the docs: each ```c block of docs/*.md that has both
# #include <cui/cui.h> and a main(). The docs say these compile as they stand; this is
# what holds them to it. Linux only.
#
#   tests/doc_programs.sh [work-dir]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${1:-$ROOT/build/doc-programs}
rm -rf "$WORK"; mkdir -p "$WORK/src"

python3 - "$ROOT/docs" "$WORK/src" <<'PY'
import glob, os, re, sys
docs, out = sys.argv[1], sys.argv[2]
n = 0
for path in sorted(glob.glob(docs + "/*.md")):
    name = os.path.basename(path)[:-3]
    for k, block in enumerate(re.findall(r"^```c\n(.*?)^```", open(path, encoding="utf-8").read(), re.M | re.S), 1):
        if "#include <cui/cui.h>" in block and re.search(r"\bint main\s*\(", block):
            open(f"{out}/{name}_{k}.c", "w", encoding="utf-8").write(block); n += 1
print(f"{n} complete programs in the docs")
if n < 8: sys.exit("expected at least 8: did a page lose its programs?")
PY

cmake -S "$ROOT" -B "$WORK/lib" -DCMAKE_BUILD_TYPE=Release -DCUI_BUILD_EXAMPLES=OFF -DCUI_BUILD_TESTS=OFF > /dev/null
cmake --build "$WORK/lib" -j 4 > /dev/null

fail=0
for f in "$WORK"/src/*.c; do
    if cc -std=c11 -O1 -Wall -Wextra -Werror -I "$ROOT/include" "$f" "$WORK/lib/libcui.a" -lX11 -lm -lpthread -ldl -o "${f%.c}" 2> "${f%.c}.log"; then
        echo "  ok    $(basename "$f")"
    else
        echo "  FAIL  $(basename "$f")"; sed 's/^/        /' "${f%.c}.log" | head -12; fail=1
    fi
done
[ "$fail" = 0 ] || { echo "a program in the docs does not build"; exit 1; }
echo "every complete program in the docs builds"
