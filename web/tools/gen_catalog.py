#!/usr/bin/env python3
"""Builds site/catalog.js: every documented component with its docs (as HTML),
the live scenes that show it, and the C source of each scene with file and
line references.
    gen_catalog.py <repo-root> <out.js>"""
import html, json, re, sys

ROOT, OUT = sys.argv[1], sys.argv[2]
import os
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTRA_REL = os.path.relpath(os.path.join(HERE, 'host', 'extra_scenes.inc'), ROOT)

def read(rel):
    return open(f"{ROOT}/{rel}", encoding="utf-8").read()

def slug(title):
    s = title.lower().strip()
    s = re.sub(r"[^\w\- ]", "", s)
    return s.replace(" ", "-")

# ---------------------------------------------------------------- scenes
SHOTS = "tests/component_shots.c"
EXTRA = EXTRA_REL

def scene_table():
    src = read(SHOTS)
    body = re.search(r"static const scene scenes\[\] = \{(.*?)\n    \};", src, re.S).group(1)
    extra = re.search(r"#define WEB_EXTRA_SCENES(.*?)\n\n|#define WEB_EXTRA_SCENES(.*)$", read(EXTRA), re.S)
    extra = (extra.group(1) or extra.group(2)) if extra else ""
    out = {}
    for text, file in ((body, SHOTS), (extra, EXTRA)):
        for m in re.finditer(r'\{\s*"([^"]+)",\s*(\w+),\s*([\d.]+),\s*([\d.]+)((?:,\s*\w+)*)\s*\}', text):
            flags = [f.strip() for f in m.group(5).split(",") if f.strip()]
            out[m.group(1)] = {"fn": m.group(2), "w": float(m.group(3)), "h": float(m.group(4)), "file": file,
                               "dark_only": len(flags) > 1 and flags[1] == "true",
                               "accent": flags[2] if len(flags) > 2 else "CUI_ACCENT_NEUTRAL"}
    return out

def brace_block(src, start):
    """Returns the end index of the {...} block whose first '{' is at or after start."""
    i = src.index("{", start)
    depth, n = 0, len(src)
    in_str = in_chr = in_line = in_block = False
    while i < n:
        c = src[i]
        nxt = src[i + 1] if i + 1 < n else ""
        if in_line:
            if c == "\n": in_line = False
        elif in_block:
            if c == "*" and nxt == "/": in_block = False; i += 1
        elif in_str:
            if c == "\\": i += 1
            elif c == '"': in_str = False
        elif in_chr:
            if c == "\\": i += 1
            elif c == "'": in_chr = False
        else:
            if c == "/" and nxt == "/": in_line = True; i += 1
            elif c == "/" and nxt == "*": in_block = True; i += 1
            elif c == '"': in_str = True
            elif c == "'": in_chr = True
            elif c == "{": depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0: return i + 1
        i += 1
    return n

def find_function(src, fn):
    """Finds a scene function: SCENE(name) { ... } or static void name(...) { ... }."""
    pats = []
    if fn.startswith("scene_"):
        pats.append(r"^SCENE\(" + re.escape(fn[6:]) + r"\)\s*\{")
    pats.append(r"^static\s+void\s+" + re.escape(fn) + r"\s*\([^;{]*\)\s*\{")
    for p in pats:
        m = re.search(p, src, re.M)
        if m:
            end = brace_block(src, m.start())
            # keep the comment block directly above
            start = m.start()
            above = src[:start].rstrip("\n").rsplit("\n", 1)
            while above and above[-1].lstrip().startswith(("/*", "*", "//")) and not above[-1].strip().endswith("{"):
                start = src.rfind(above[-1], 0, start)
                above = src[:start].rstrip("\n").rsplit("\n", 1)
            return start, end
    return None

def scene_sources(table):
    files = {SHOTS: read(SHOTS), EXTRA: read(EXTRA)}
    out = {}
    for name, s in table.items():
        src = files[s["file"]]
        parts, seen = [], set()
        queue = [s["fn"]]
        while queue and len(parts) < 4:
            fn = queue.pop(0)
            if fn in seen: continue
            seen.add(fn)
            loc = find_function(src, fn)
            if not loc: continue
            a, b = loc
            line = src.count("\n", 0, a) + 1
            text = src[a:b]
            parts.append({"line": line, "text": text})
            # helpers the scene builds with (body_x, x_body, *_body ...)
            for h in re.findall(r"\b(body_\w+|\w+_body)\s*\(", text):
                if h not in seen and h not in queue and h != "frame_wrap": queue.append(h)
            for h in re.findall(r"\bframe_wrap\s*\(\s*ui\s*,\s*(\w+)\s*\)", text):
                if h not in seen: queue.append(h)
            for h in re.findall(r"\bstage\s*\(\s*ui\s*,\s*&?\w+\s*,\s*(\w+)\s*\)", text):
                if h not in seen: queue.append(h)
            for h in re.findall(r"\b(scene_\w+)\s*\(\s*ui\s*,", text):
                if h not in seen and h != fn: queue.append(h)
        # a SCENE() scene is two pieces in the same place; show them in file order
        parts.sort(key=lambda p: p["line"])
        out[name] = {"file": s["file"], "line": parts[0]["line"] if parts else 1,
                     "source": "\n\n".join(p["text"] for p in parts),
                     "w": s["w"], "h": s["h"], "dark_only": s["dark_only"]}
    return out

# ---------------------------------------------------------------- markdown
REPO_URL = "https://github.com/yappologistic/CUI"

def doc_link(label, target, anchors):
    """A markdown link as the site shows it: a guide or component page, the web, or a file in the repository."""
    anchor = target.split("#", 1)[1] if "#" in target else None
    if target.startswith("http"):
        return f'<a href="{target}" target="_blank" rel="noreferrer">{label}</a>'
    if anchor and anchor in anchors:
        return f'<a href="#/c/{anchors[anchor]}">{label}</a>'
    doc = re.match(r"^(?:\.\./)?(?:docs/)?([\w-]+)\.md(?:#([\w-]+))?$", target)
    if doc and doc.group(1) in GUIDE_IDS:
        if doc.group(2) and GUIDE_SECTIONS.get(doc.group(2), "").startswith(doc.group(1) + "/"):   # a section, or the section of a subsection
            return f'<a href="#/guide/{GUIDE_SECTIONS[doc.group(2)]}">{label}</a>'
        sec = re.sub(r"^\d+-", "", doc.group(2) or "")   # "## 7. Files and links" is the section files-and-links
        return f'<a href="#/guide/{doc.group(1)}' + (f'/{sec}' if sec else '') + f'">{label}</a>'
    if doc and doc.group(1) in ("components", "components-more"):
        return f'<a href="#/components">{label}</a>'
    if target.startswith("#") and anchor in GUIDE_SECTIONS:   # a section of a guide, linked from inside it
        return f'<a href="#/guide/{GUIDE_SECTIONS[anchor]}">{label}</a>'
    repo = re.match(r"^\.\./([\w./-]+)(#[\w-]+)?$", target)   # a file or folder of the repository, from docs/
    if repo:
        kind = "blob" if "." in repo.group(1).rsplit("/", 1)[-1] else "tree"
        return f'<a href="{REPO_URL}/{kind}/master/{repo.group(1)}{repo.group(2) or ""}" target="_blank" rel="noreferrer">{label}</a>'
    if doc:   # a doc that is not a guide on the site, such as CONTRIBUTING.md or architecture.md
        frag = f"#{doc.group(2)}" if doc.group(2) else ""
        return f'<a href="{REPO_URL}/blob/master/docs/{doc.group(1)}.md{frag}" target="_blank" rel="noreferrer">{label}</a>'
    return f'<span class="doc-ref" title="{html.escape(target)}">{label}</span>'

def inline(text, anchors):
    # a link whose label is code, [`examples/todo.c`](../examples/todo.c): made first, since
    # the split on backticks below would cut it in two
    kept = []
    def code_link(m):
        kept.append(doc_link(f"<code>{html.escape(m.group(1))}</code>", m.group(2), anchors))
        return f"\x00{len(kept) - 1}\x00"
    text = re.sub(r"\[`([^`]+)`\]\(([^)]+)\)", code_link, text)
    parts = re.split(r"(`[^`]+`)", text)
    out = []
    for p in parts:
        if p.startswith("`") and p.endswith("`") and len(p) > 1:
            out.append(f"<code>{html.escape(p[1:-1])}</code>")
            continue
        p = html.escape(p, quote=False)
        # images in running text or table cells (component screenshots are shown live instead)
        p = re.sub(r"!\[([^\]]*)\]\((images/[\w/\-]+\.png)\)", r'<img class="doc-img" src="\2" alt="\1" loading="lazy">', p)
        p = re.sub(r"!\[([^\]]*)\]\(([^)]+)\)", "", p)
        p = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", lambda m: doc_link(m.group(1), m.group(2), anchors), p)
        p = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", p)
        p = re.sub(r"(?<![\w*])\*([^*\s][^*]*)\*(?![\w*])", r"<em>\1</em>", p)
        p = re.sub(r"(?<![\w])_([^_\s][^_]*)_(?![\w])", r"<em>\1</em>", p)
        out.append(p)
    return re.sub(r"\x00(\d+)\x00", lambda m: kept[int(m.group(1))], "".join(out))

# the guide: docs/*.md in reading order (components have their own pages)
GUIDES = ["getting-started", "tutorial", "apps", "recipes", "layout", "theming", "icons", "custom-components", "integration", "rendering", "performance", "faq", "design"]
GUIDE_IDS = set(GUIDES)
# each guide's section anchors (as GitHub makes them from the "## " headings), for links within a guide
def guide_sections():
    out = {}
    anchor_of = lambda t: re.sub(r"[^\w\- ]", "", t.strip().lower()).replace(" ", "-")
    for gid in GUIDES:
        sec = None
        for ln in open(f"{ROOT}/docs/{gid}.md", encoding="utf-8"):
            if ln.startswith("## "):
                sec = re.sub(r"^\d+-", "", anchor_of(ln[3:]))
                out.setdefault(anchor_of(ln[3:]), f"{gid}/{sec}")
            elif ln.startswith("### ") and sec:   # a subsection: the site opens its section
                out.setdefault(anchor_of(ln[4:]), f"{gid}/{sec}")
    return out
GUIDE_SECTIONS = guide_sections()

IMG_RE = re.compile(r"images/((?:components|showcase)/)?([\w\-]+?)(?:-dark)?\.png")

def md_to_html(lines, anchors, scenes):
    """Returns (html, scene names in order, static images) for a section body."""
    out, found, statics = [], [], []
    i = 0
    para = []
    def flush():
        if para:
            out.append("<p>" + inline(" ".join(para), anchors) + "</p>")
            para.clear()
    while i < len(lines):
        ln = lines[i]
        s = ln.strip()
        if s.startswith("```"):
            flush()
            lang = s[3:].strip() or "text"
            body = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                body.append(lines[i]); i += 1
            out.append(f'<pre class="code" data-lang="{lang}"><code>{html.escape(chr(10).join(body))}</code></pre>')
            i += 1
            continue
        imgs = IMG_RE.findall(s) if ("<picture" in s or s.startswith("![") or "<img" in s) else []
        if imgs:
            flush()
            for kind, name in imgs:
                if name in scenes:
                    if name not in found: found.append(name)
                else:
                    alt = re.search(r'alt="([^"]*)"', s) or re.search(r"!\[([^\]]*)\]", s)
                    entry = {"src": f"images/{kind}{name}", "alt": alt.group(1) if alt else name}
                    if entry not in statics: statics.append(entry)
            i += 1
            continue
        if s.startswith("|"):
            flush()
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")]); i += 1
            head, body = rows[0], [r for r in rows[1:] if not all(re.fullmatch(r":?-+:?", c) for c in r)]
            t = "<table><thead><tr>" + "".join(f"<th>{inline(c, anchors)}</th>" for c in head) + "</tr></thead><tbody>"
            for r in body: t += "<tr>" + "".join(f"<td>{inline(c, anchors)}</td>" for c in r) + "</tr>"
            out.append(t + "</tbody></table>")
            continue
        m = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", ln)
        if m and not s.startswith("**"):
            flush()
            ordered = m.group(2)[0].isdigit()
            items = []
            while i < len(lines):
                m = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", lines[i])
                if m: items.append(m.group(3)); i += 1
                elif lines[i].startswith("  ") and lines[i].strip() and items: items[-1] += " " + lines[i].strip(); i += 1
                else: break
            tag = "ol" if ordered else "ul"
            out.append(f"<{tag}>" + "".join(f"<li>{inline(it, anchors)}</li>" for it in items) + f"</{tag}>")
            continue
        h = re.match(r"^(#{3,6})\s+(.*)", s)
        if h:
            flush()
            out.append(f"<h4>{inline(h.group(2), anchors)}</h4>")
            i += 1
            continue
        if not s or s == "---" or s.startswith("<"):
            flush(); i += 1; continue
        para.append(s)
        i += 1
    flush()
    return "\n".join(out), found, statics

# ---------------------------------------------------------------- sections
def sections(rel, level):
    """Yields (title, line, body_lines, group) for each heading of `level` (## or ###)."""
    lines = read(rel).split("\n")
    marker = "#" * level + " "
    group, cur = None, None
    res, groups = [], {}
    for n, ln in enumerate(lines, 1):
        if level == 3 and ln.startswith("## "):
            if cur: res.append(cur); cur = None
            group = ln[3:].strip()
            groups[group] = {"line": n, "body": []}
            continue
        if ln.startswith(marker):
            if cur: res.append(cur)
            cur = {"title": ln[len(marker):].strip(), "line": n, "body": [], "group": group, "file": f"docs/{rel.split('/')[-1]}"}
            continue
        if ln.startswith("# ") or (level == 2 and ln.startswith("## ")):
            continue
        if cur: cur["body"].append(ln)
        elif group: groups[group]["body"].append(ln)
    if cur: res.append(cur)
    return res, groups

CATEGORIES = [
    ("basics", "Basics"), ("actions", "Actions"), ("inputs", "Inputs"), ("selection", "Selection"),
    ("pickers", "Pickers"), ("navigation", "Navigation"), ("overlays", "Overlays"), ("feedback", "Feedback"),
    ("data", "Data display"), ("charts", "Charts"), ("planning", "Planning"), ("social", "Conversation and media"),
    ("motion", "Motion"), ("ai", "AI interfaces"), ("devtools", "Developer tools"), ("instruments", "Instruments"),
    ("canvas", "Canvas and bars"),
]
MAIN_CAT = {
    "Text": "basics", "Badge, avatar and keycap": "basics", "Card and metric": "basics", "Image": "basics",
    "Button": "actions", "Icon button": "actions", "Button group, copy and hold to confirm": "actions",
    "Text field": "inputs", "Text area": "inputs", "Number field": "inputs", "Tag input": "inputs",
    "One time code": "inputs", "Password strength": "inputs",
    "Segmented control": "selection", "Checkbox, switch and radio": "selection", "Slider": "selection",
    "Chips and rating": "selection", "Radio cards": "selection",
    "Select and combobox": "pickers", "Date picker and calendar": "pickers", "Color picker": "pickers",
    "Tabs": "navigation", "Sidebar navigation": "navigation", "Breadcrumb, pagination and stepper": "navigation",
    "Dropdown and context menu": "overlays", "Popover": "overlays", "Tooltip": "overlays", "Dialog": "overlays",
    "Sheet": "overlays", "Command palette": "overlays",
    "Toast": "feedback", "Alert": "feedback", "Progress, meter, spinner and skeleton": "feedback", "Empty state": "feedback",
    "Table": "data", "List and tree": "data", "Accordion": "data", "Timeline and progress ring": "data",
    "Reorderable list": "data", "Charts": "charts",
}
MORE_CAT = {
    "Actions": "actions", "Inputs": "inputs", "Pickers": "pickers", "Capture": "inputs", "Disclosure": "overlays",
    "Feedback": "feedback", "Data": "data", "Code": "data", "Conversation": "social", "Media": "social",
    "Planning": "planning", "More charts": "charts", "Motion": "motion", "AI interfaces": "ai",
    "App chrome": "navigation",
    "Tool panels": "data", "Developer tools": "devtools", "Instruments": "instruments", "Voice": "ai",
    "Creative controls": "selection",
    "Canvas and bars": "canvas",
}
# deprecated components: their docs stay in the guide for existing code, but they get no page on the site
DEPRECATED = {"Voice orb"}
ITEM_CAT = {"Table of contents": "navigation", "Status tag": "feedback"}
# sections whose docs have no screenshot: the scene that shows them
NO_IMAGE = {
    "Text": ["text"], "Icon button": ["icon-button"], "Color picker": ["color-picker"], "Popover": ["popover"],
    "Sheet": ["sheet"], "Image": ["image"], "Reorderable list": ["reorder"], "One time code": ["tag-otp"],
    "Password strength": ["tag-otp"], "Confirm button": ["confirm-button"], "Reactions": ["reactions"],
    "Speed dial": ["speed-dial"], "Theme switcher and toggle": ["theme-switcher"], "Swipe actions": ["swipe-actions"],
    "Money field": ["money-field"], "Prefix and suffix": ["prefix-suffix"], "Rolling numbers": ["rolling-numbers"],
}
EXTRA_FOR = {"Toast": ["toast-demo"], "Dialog": ["dialog-demo"], "Voice recorder": ["voice-demo"], "Voice note": ["voice-demo"],
             # the four chrome parts together in one window, after each part alone
             "Menu bar": ["app-chrome"], "Toolbar": ["app-chrome"], "Document tabs": ["app-chrome"], "Status bar": ["app-chrome"]}
# live variants shown before the doc scene, when the doc scene only poses a moment
PREPEND = {"Kanban board": ["kanban-live"], "Docking": ["docking-live"]}

SHOWCASE = [
    ("hero-data", "Data grid", "Virtualized rows, filters, sorting, resizable columns and spreadsheet style selection."),
    ("hero-board", "Kanban", "The lifted card follows the pointer and the column opens a slot for it."),
    ("hero-desk", "Dock, tiles and player", "Icons grow under the pointer, the waveform seeks on click."),
    ("hero-pick", "Pickers", "A date range with presets over two months, and wheels that spin with momentum."),
    ("hero-palette", "Command palette", "Fuzzy search, groups and shortcuts."),
    ("hero-charts", "Charts", "Calendar heatmap, sankey and radar, with a live task plan."),
    ("hero-social", "Conversation", "Grouped bubbles, threads, a rolling counter, presence."),
    ("hero-effects", "Spotlight cards", "The surface and edge nearest the pointer light up."),
    ("hero-ai", "Assistant", "Reasoning, tool calls, citations and a composer, beside a task plan and a frame time chart."),
    ("hero-plan", "Planning", "Week calendar and gantt chart together."),
    ("stage-mosaic", "Build settings", "Forms, a calendar with time slots, metrics, a bar chart, switches and chips on one page."),
    ("stage-mosaic-pro", "Activity", "A chat with tool calls, a contribution heatmap, a sankey, a task plan, control tiles and a radar chart."),
]
APPS = [
    ("gallery", "Gallery", "The native gallery app: every component on its own page, live.", "examples/gallery_ui.c"),
    ("dashboard", "Dashboard", "Larkspur Supply: KPI strip with rolling figures, charts and an orders table.", "examples/dashboard.c"),
    ("finance", "Finance", "Halden: accounts, budgets and transactions.", "examples/finance.c"),
    ("music", "Music", "Tempo: library, now playing and queue.", "examples/music.c"),
    ("mail", "Mail", "Folders, a message list and a reading pane in split views.", "examples/mail.c"),
    ("notes", "Notes", "A notes app with a list, an editor and export.", "examples/notes.c"),
    ("settings", "Settings", "Account, notifications, appearance and billing settings.", "examples/settings.c"),
    ("tracker", "Tracker", "Orbit: an issue tracker with boards and filters.", "examples/tracker.c"),
    ("assistant", "Assistant", "Muse: an AI assistant with reasoning, tools and attachments.", "examples/assistant.c"),
    ("todo", "To do", "The app the tutorial builds: a list you add to, tick off, filter and clear, in about 130 lines.", "examples/todo.c"),
    ("caffeine", "Caffeine", "How much caffeine is still in you at bedtime: drag the cups, and the dot chart, the peak and the verdict follow.", "examples/caffeine.c"),
]

# ---------------------------------------------------------------- options (from the header)
HEADER = "include/cui/cui.h"

def strip_comments(s):
    return re.sub(r"//[^\n]*", "", re.sub(r"/\*.*?\*/", " ", s, flags=re.S))

def header_index():
    """Maps each public name (macro or function) to the options struct it takes,
    and each options struct to its parsed fields."""
    src = read(HEADER)
    lines = src.split("\n")
    fn_struct, line_of = {}, {}
    # named-argument macros: #define cui_x(ui, ...) cui_x_ex(..., CUI__OPTS(cui_x_opts, __VA_ARGS__))
    i = 0
    while i < len(lines):
        ln = lines[i]
        m = re.match(r"\s*#define\s+(cui_\w+)\(", ln)
        if m:
            body = ln
            while body.rstrip().endswith("\\") and i + 1 < len(lines):
                i += 1; body = body.rstrip()[:-1] + " " + lines[i]
            o = re.search(r"CUI__OPTS\(\s*(cui_\w+)\s*,", body)
            if o: fn_struct.setdefault(m.group(1), o.group(1))
        i += 1
    # functions whose last parameter is the options struct: cui_x_ex(..., const cui_x_opts *opts)
    flat = strip_comments(src)
    for m in re.finditer(r"CUI_API\b[^;{]*?\b(cui_\w+)\s*\(([^;{]*?)\)\s*(?:CUI_\w+\([^)]*\)\s*)?;", flat, re.S):
        params = [p.strip() for p in m.group(2).split(",")]
        last = params[-1] if params else ""
        o = re.match(r"const\s+(cui_\w+)\s*\*\s*\w*$", last)
        if o and (o.group(1).endswith("_opts") or "opts" in last.split()[-1]):
            fn_struct.setdefault(m.group(1), o.group(1))
    structs = {}
    for m in re.finditer(r"typedef\s+struct\s+(cui_\w+)\s*\{", src):
        end = brace_block(src, m.start())
        body = src[src.index("{", m.start()) + 1:end - 1]
        entry = {"line": src.count("\n", 0, m.start()) + 1, "fields": parse_fields(body)}
        if re.match(r"\s*int\s+_\s*;", body): structs[m.group(1)] = entry   # a named-argument struct
        else: DATA_STRUCTS[m.group(1)] = entry                              # data the caller owns, or a result
    for m in re.finditer(r"typedef\s+enum\s+(cui_\w+)\s*\{([^}]*)\}", flat, re.S):
        ENUMS[m.group(1)] = re.findall(r"\b(CUI_\w+)\b(?!\s*\()", re.sub(r"=[^,]*", "", m.group(2)))
    # what each function takes and returns: the cui_ types among them
    for m in re.finditer(r"CUI_API\s+((?:const\s+)?\w+(?:\s*\*)*)\s*(cui_\w+)\s*\(([^;{]*?)\)\s*(?:CUI_\w+\([^)]*\)\s*)?;", flat, re.S):
        ret = re.sub(r"\bconst\b|\*|\s", "", m.group(1))
        params = re.findall(r"\b(cui_\w+)\b\s*\**\s*(?:const\s*)?\w*\s*(?:,|$)", m.group(3))
        FN_SIG[m.group(2)] = (ret, params)
    for entry in list(structs.values()) + list(DATA_STRUCTS.values()): fill_docs(entry["fields"])
    return fn_struct, structs

DATA_STRUCTS, ENUMS, FN_SIG = {}, {}, {}

# What a field means when cui.h gives it no comment of its own: fields that mean the same
# in every component (name, type). A comment in the header always wins.
SIZE_DOC = "`CUI_FILL`, `CUI_FIT`, `CUI_PX(n)` or `CUI_PCT(n)`; unset uses the component's own."
GENERIC_DOCS = {
    ("width", "cui_size"): "Width: " + SIZE_DOC,
    ("height", "cui_size"): "Height: " + SIZE_DOC,
    ("width", "float"): "Width in points; 0 uses the component's own.",
    ("height", "float"): "Height in points; 0 uses the component's own.",
    ("size", "float"): "Size in points; 0 uses the component's own.",
    ("gap", "float"): "Space between the items, in points.",
    ("pad", "float"): "Padding inside, in points.",
    ("icon", "const char *"): "An icon: a `CUI_ICON_*` constant, or a string from another icon font.",
    ("icons", "const char *const *"): "One icon per item, in the same order as the items.",
    ("color", "cui_color"): "A theme color such as `CUI_COLOR_ACCENT`, or `CUI_RGB(0x7747ff)`; unset uses the theme's.",
    ("label", "const char *"): "A label shown above or beside the control.",
    ("placeholder", "const char *"): "Muted text shown while the field is empty.",
    ("description", "const char *"): "A second line of smaller text.",
    ("tooltip", "const char *"): "Text shown when the pointer rests on it.",
    ("format", "const char *"): 'A printf format for the values, such as `"%.0f ms"`.',
    ("unit", "const char *"): "A unit shown after the value.",
    ("shortcut", "const char *"): 'The shortcut shown beside it, as text such as `"Ctrl N"`.',
    ("disabled", "bool"): "Greyed out, and takes no input.",
    ("read_only", "bool"): "Shows the value but does not let it be edited.",
    ("full_width", "bool"): "Stretches to the width of its container.",
    ("autofocus", "bool"): "Takes the keyboard focus when it first appears.",
    ("striped", "bool"): "Shades every other row.",
    ("sortable", "bool"): "The column's header can be clicked to sort by it.",
    ("no_border", "bool"): "Draws no border.",
    ("no_legend", "bool"): "Hides the legend.",
    ("no_grid", "bool"): "Hides the grid lines.",
    ("hide_value", "bool"): "Hides the value's text.",
    ("show_value", "bool"): "Shows the value as text.",
    ("indeterminate", "bool"): "An animation with no amount, for work of unknown length.",
    ("dismissible", "bool"): "Shows a close button.",
    ("danger", "bool"): "Drawn in the danger color, for destructive actions.",
    ("decimals", "int"): "Digits after the decimal point.",
    ("step", "float"): "The amount one step changes the value by.",
    ("step", "double"): "The amount one step changes the value by.",
}
def fill_docs(fields):
    for f in fields:
        if f["doc"]: continue
        base = re.sub(r"\bconst\b|\*|\[[^\]]*\]|\s", "", f["type"])
        if base in ENUMS and "*" not in f["type"] and 1 < len(ENUMS[base]) <= 12:
            vals = ENUMS[base]
            f["doc"] = comment_html("One of " + ", ".join(f"`{v}`" for v in vals[:-1]) + f" or `{vals[-1]}`.")
        elif (f["name"], f["type"]) in GENERIC_DOCS:
            f["doc"] = comment_html(GENERIC_DOCS[(f["name"], f["type"])])
# value types every page uses; their fields are in the Layout and Theming guides
CORE_TYPES = {"cui_ctx", "cui_rect", "cui_vec2", "cui_color", "cui_size", "cui_corners", "cui_box", "cui_texture",
              "cui_font", "cui_id", "cui_date", "cui_key_combo"}

# structs a page is about that no function of the page takes directly
EXTRA_TYPES = {"chart-annotations": ["cui_chart_annotation"],
               "canvas-controls": ["cui_canvas_toggle"], "format-bar": ["cui_text_style"]}

def item_types(it, owners):
    """The structs an item's own functions take (data) and return (results), with the
    enums their fields use: what a caller has to fill in and read back."""
    out, seen = [], set()
    def add(st, role, fn):
        if st in CORE_TYPES or st not in DATA_STRUCTS or not DATA_STRUCTS[st]["fields"]: return
        if st in seen:
            e = next(x for x in out if x["struct"] == st)
            if fn not in e["fn"]: e["fn"].append(fn)
            return
        seen.add(st)
        fields = DATA_STRUCTS[st]["fields"]
        enums = []
        for f in fields:
            base = re.sub(r"\bconst\b|\*|\[[^\]]*\]|\s", "", f["type"])
            if base in ENUMS and base not in [e["name"] for e in enums]: enums.append({"name": base, "values": ENUMS[base]})
            if base in DATA_STRUCTS and base not in CORE_TYPES: nested.append((base, role, fn))
        out.append({"struct": st, "role": role, "fn": [fn], "line": DATA_STRUCTS[st]["line"],
                    "fields": [{"name": f["name"], "type": f["type"], "doc": f["doc"]} for f in fields], "enums": enums})
    nested = []
    for st in EXTRA_TYPES.get(it["id"], []): add(st, "data", st)
    for a in it["apis"]:
        if it["id"] not in owners.get(a, set()): continue
        for c in [a, a + "_ex", a + "_begin", a + "_begin_ex"]:
            if c not in FN_SIG: continue
            ret, params = FN_SIG[c]
            for st in params:
                if not st.endswith("_opts"): add(st, "data", a)
            add(ret, "result", a)
    while nested:   # a struct inside a struct (a diagnostic's position, a trace's view)
        st, role, fn = nested.pop(0)
        add(st, role, fn)
    out.sort(key=lambda x: (x["role"] != "data", ))
    return out

def comment_text(c):
    c = re.sub(r"^\s*/\*+|\*+/\s*$|^\s*//", "", c.strip())
    c = re.sub(r"\n\s*\*\s?", " ", c)
    return re.sub(r"\s+", " ", c).strip()

def comment_html(c):
    parts = re.split(r"(`[^`]+`)", c)
    return "".join(f"<code>{html.escape(p[1:-1])}</code>" if p.startswith("`") and p.endswith("`") and len(p) > 1
                   else html.escape(p, quote=False) for p in parts)

DEFAULT_RE = re.compile(r"(?<![\w_])(?<!the )(?<!by )[Dd]efaults?(?: to| is)?\s*[:=]?\s+")

def default_of(doc):
    """The value after "default" in a field comment: up to the end of its clause
    (a ';' or a new sentence), keeping commas inside parentheses and quotes."""
    m = re.search(r"([\w.\"%-]+)\s*\(default\)", doc)      # "8, 16 (default) or 32"
    if m: return m.group(1)
    m = DEFAULT_RE.search(doc)
    if not m: return ""
    rest, depth, quote, out = doc[m.end():], 0, False, ""
    for k, c in enumerate(rest):
        if c == '"': quote = not quote
        elif not quote and c == "(": depth += 1
        elif not quote and c == ")":
            if depth == 0: break          # a parenthetical that opened before "default"
            depth -= 1
        elif not quote and depth == 0 and (c == ";" or (c == "." and re.match(r"\.\s+[A-Z]", rest[k:]))): break
        out += c
    d = out.strip().rstrip(".").strip()
    return d if d and len(d) <= 60 else ""

def parse_decl(decl):
    """One declaration without its ';' -> [(name, type)], or [] when it is not a plain field."""
    decl = re.sub(r"\s+", " ", decl).strip()
    if not decl or "{" in decl or "}" in decl or decl.startswith("#"): return []
    fp = re.match(r"^(.*?)\(\s*\*\s*(\w+)\s*\)\s*\((.*)\)$", decl)
    if fp:
        return [(fp.group(2), f"{fp.group(1).strip()} (*)({fp.group(3).strip()})")]
    m = re.match(r"^((?:(?:const|unsigned|signed|struct|enum|volatile)\s+)*\w+(?:\s+const\b)?)\s*(.+)$", decl)
    if not m: return []
    base, rest = m.group(1).strip(), m.group(2)
    out = []
    for d in rest.split(","):
        dm = re.match(r"^\s*((?:\*\s*(?:const\b\s*)?)*)\s*(\w+)\s*((?:\[[^\]]*\])*)\s*$", d)
        if not dm: return []
        stars = re.sub(r"\s+", "", dm.group(1)).replace("const", "const ").strip()
        out.append((dm.group(2), (f"{base} {stars}".strip() if stars else base) + dm.group(3)))
    return out

def parse_fields(body):
    fields, pending, depth = [], "", 0
    lines = body.split("\n")
    i = 0
    while i < len(lines):
        ln = lines[i]
        # a block comment that runs on to later lines
        while ln.count("/*") > ln.count("*/") and i + 1 < len(lines):
            i += 1; ln += "\n" + lines[i]
        i += 1
        comments = re.findall(r"/\*.*?\*/|//[^\n]*", ln, re.S)
        code = strip_comments(ln).strip()
        doc = " ".join(comment_text(c) for c in comments).strip()
        if not code:
            if not ln.strip(): pending = ""
            elif doc: pending = doc
            continue
        opens, closes = code.count("{"), code.count("}")
        if depth or opens or closes:
            # nested struct or union: skip it rather than guess at its fields
            depth += opens - closes
            pending = ""
            continue
        decls = [d for d in code.split(";") if d.strip()]
        line_doc = doc or pending
        pending = ""
        single = len(decls) > 1        # one-line struct: a trailing comment belongs to the last field
        for k, d in enumerate(decls):
            for name, ctype in parse_decl(d):
                if name == "_": continue
                fdoc = line_doc if (not single or k == len(decls) - 1) else ""
                fields.append({"name": name, "type": ctype, "default": default_of(fdoc), "doc": comment_html(fdoc)})
    return fields

def api_tokens(api):
    s = re.sub(r"_(ex|begin|end)$", "", api[4:])
    s = re.sub(r"_(ex|begin|end)$", "", s)
    return s.split("_")

def api_owners(items):
    """The items each api belongs to: the ones whose id shares the most words
    with its name (Button owns cui_button; Text only mentions it). Ties share it.
    An api only one page uses belongs to it; one several pages use and no name
    matches has no owner."""
    mentions = {}
    for it in items.values():
        for a in it["apis"]: mentions.setdefault(a, []).append(it["id"])
    owners = {}
    for a, ids in mentions.items():
        toks = api_tokens(a)
        def score(iid):
            words, joined = set(iid.split("-")), iid.replace("-", "")
            if "-".join(toks) == iid: return 10
            hit = lambda t: (t in words or t + "s" in words or (len(t) >= 4 and any(w.startswith(t) for w in words))
                             or (len(t) >= 6 and t in joined))
            return sum(1 for t in toks if hit(t)) / len(toks)
        best = max(score(i) for i in ids)
        if best > 0: owners[a] = {i for i in ids if score(i) == best}
        elif len(ids) == 1: owners[a] = set(ids)     # only one page uses it
        else: owners[a] = set()
    return owners

def field_mentions(fields, body_html, item_id):
    """How many of these fields the prose names in code spans, or the title names
    (code blocks do not count: an example may set a layout box's options without
    being about them)."""
    names = {f["name"] for f in fields}
    prose = re.sub(r"<pre\b.*?</pre>", "", body_html, flags=re.S)
    found = set(re.findall(r"<code>\.?(\w+)(?: = [^<]*)?</code>", prose)) | set(item_id.split("-"))
    return len(names & found)

# pages about a mode of another page's component, which show that component's options too
SHARED_OPTIONS = {"voice-recorder": {"cui_chat_composer"}}

# layout containers an example happens to build with; their options belong to the Layout guide
LAYOUT_STRUCTS = {"cui_box", "cui_scroll_opts", "cui_split_opts"}

def item_options(it, owners, fn_struct, structs):
    """One table per options struct the item's own functions take: functions it
    owns, or functions nobody owns whose options its prose talks about."""
    by_struct = {}
    for a in it["apis"]:
        cands = [a, a + "_ex", a + "_begin", a + "_begin_ex"]
        if a.endswith("_begin"): cands.append(a + "_ex")
        st = next((fn_struct[c] for c in cands if c in fn_struct and fn_struct[c] in structs), None)
        if not st or not structs[st]["fields"]: continue
        if st in LAYOUT_STRUCTS and not a.startswith(("cui_" + it["id"].replace("-", "_"))): continue
        own = owners.get(a, set())
        shared = a in SHARED_OPTIONS.get(it["id"], ())
        if not shared and it["id"] not in own and (own or field_mentions(structs[st]["fields"], it["html"], it["id"]) < 2): continue
        by_struct.setdefault(st, [])
        if a not in by_struct[st]: by_struct[st].append(a)
    return [{"fn": fns, "struct": st, "line": structs[st]["line"], "fields": structs[st]["fields"]}
            for st, fns in by_struct.items()]

# ---------------------------------------------------------------- keyboard (from the doc prose)
KEY = (r"Tab|Enter|Return|Esc|Escape|Space|Home|End|Page ?Up|Page ?Down|PgUp|PgDn|Delete|Backspace|Insert"
       r"|Up|Down|Left|Right|F(?:[1-9]|1[0-2])|Ctrl|Cmd|Shift|Alt|Option")
MOD = r"(?:Ctrl|Cmd|Shift|Alt|Option)"
# a key on its own, or a combo: Ctrl+A, Ctrl+Shift+Z, Shift+arrows
KEY_RE = re.compile(rf"(?<![\w-])(?:{MOD}\+)*(?:{KEY}|(?<=\+)(?:[A-Z0-9]|arrows|click|drag|wheel|scroll))(?![\w-])")
# "arrows" counts as keys only where it reads that way ("the arrows move", "Shift+arrows"), not "elbow arrows"
TRIGGER_RE = re.compile(rf"(?<![\w-])(?:(?:{MOD})\+\w|(?:{KEY.replace('|Option', '')})(?![\w-]))|\barrow keys?\b"
                        r"|(?:^|\bthe |: |\+|, )arrows\b|^Arrows\b")

def kbd_wrap(s):
    """Wraps key names in <kbd>, outside tags and code spans."""
    parts = re.split(r"(<code>.*?</code>|<[^>]+>)", s)
    def one(m):
        keys = m.group(0).split("+")
        return "+".join(f"<kbd>{k}</kbd>" if re.fullmatch(KEY, k) or len(k) == 1 else k for k in keys)
    return "".join(p if p.startswith("<") else KEY_RE.sub(one, p) for p in parts)

# Components that only show something: nothing to focus or press. Every other page says what
# the keyboard does (tools/check_catalog.mjs holds every page to one or the other).
DISPLAY_ONLY = {
    "icons", "badge-avatar-and-keycap", "image", "password-strength",
    "progress-meter-spinner-and-skeleton", "timeline-and-progress-ring", "charts", "heatmap",
    "calendar-heatmap", "treemap-and-radar", "funnel-split-bar-and-waffle", "sankey",
    "bar-list-and-activity-rings", "scatter-and-bubble", "radial-gauge", "presence-counter-and-ticker",
    "text-effects", "loaders", "chat", "uptime-bars", "readout", "dot-matrix-display", "chart-annotations"
}

def keyboard_lines(body_html):
    """(lines, note): the sentences that name keys, and for a page whose "Keyboard:"
    paragraph names none ("none yet, it is dragged with the pointer") that paragraph."""
    out, note = [], ""
    blocks = re.findall(r"<(p|li)>(.*?)</\1>", body_html, re.S)
    for _, block in blocks:
        text = html.unescape(re.sub(r"<[^>]+>", "", re.sub(r"<code>.*?</code>", " ", block)))
        # a paragraph that opens with "Keyboard:" is about the keyboard all the way through: one line
        if re.match(r"\s*Keyboard:", text):
            s = re.sub(r"^\s*Keyboard:\s*", "", block.strip())
            if s: s = s[0].upper() + s[1:]
            if TRIGGER_RE.search(text):
                s = kbd_wrap(s)
                if s not in out: out.append(s)
            else: note = s
            continue
        # sentences end at . ! ? before a capital letter or a tag (not inside "e.g." or "1.2")
        for sent in re.split(r"(?<=[.!?])\s+(?=[A-Z<])", block):
            plain = re.sub(r"<code>.*?</code>", " ", sent)
            plain = html.unescape(re.sub(r"<[^>]+>", "", plain))
            if not TRIGGER_RE.search(plain): continue
            s = re.sub(r"^Keyboard:\s*", "", sent.strip())   # the section is already titled Keyboard
            if s: s = kbd_wrap(s[0].upper() + s[1:])
            if s and s not in out: out.append(s)
    return out, note

# ---------------------------------------------------------------- the function reference
REF_HEADERS = ["include/cui/cui.h", "include/cui/cui_app.h", "include/cui/cui_render.h"]

def reference(fn_struct, owners, items):
    """Every public function in the headers, in header order under the header's own section
    titles: its signature, the comment written beside it, and the page that explains it."""
    declared = set()
    for rel in REF_HEADERS: declared |= set(re.findall(r"CUI_API\b[^;(]*?\b(cui_\w+)\s*\(", strip_comments(read(rel))))
    macros = set()
    for rel in REF_HEADERS: macros |= set(re.findall(r"^\s*#define\s+(cui_\w+)\(", read(rel), re.M))
    out = []
    for rel in REF_HEADERS:
        lines = read(rel).split("\n")
        sec = {"title": {"cui.h": "Setup", "cui_app.h": "The app window", "cui_render.h": "Renderers"}[rel.split("/")[-1]], "header": rel, "fns": []}
        i = 0
        while i < len(lines):
            ln = lines[i]
            banner = re.match(r"/\* -{20,} \*/$", ln.strip())
            title = None
            if banner and i + 2 < len(lines) and re.match(r"/\* -{20,} \*/$", lines[i + 2].strip()):
                title = comment_text(lines[i + 1]); i += 2
            else:
                sub = re.match(r"/\* -{3,} (.+?) -{3,} \*/$", ln.strip())
                if sub: title = sub.group(1)
            if title:
                if sec["fns"]: out.append(sec)
                sec = {"title": title, "header": rel, "fns": []}
                i += 1; continue
            if ln.startswith("CUI_API"):
                start = i; decl = ln
                while ";" not in strip_comments(decl) and i + 1 < len(lines): i += 1; decl += " " + lines[i].strip()
                code = strip_comments(decl); code = re.sub(r"\s+", " ", code[:code.index(";")]).replace("CUI_API ", "").strip()
                code = re.sub(r"\s*CUI_\w+\([^)]*\)\s*$", "", code)      # a printf-format attribute after the parameters
                m = re.search(r"\b(cui_\w+)\s*\(", code)
                if m:
                    name = m.group(1)
                    doc = " ".join(comment_text(c) for c in re.findall(r"/\*.*?\*/", decl[decl.index(";"):], re.S))
                    if not doc:   # the comment block directly above
                        k, above = start - 1, []
                        while k >= 0 and lines[k].strip() and (lines[k].strip().startswith(("/*", "*", "//")) or lines[k].strip().endswith("*/")) and not re.match(r"/\* -{3,}", lines[k].strip()):
                            above.insert(0, lines[k]); k -= 1
                            if above[0].strip().startswith("/*"): break
                        if above and above[0].strip().startswith(("/*", "//")): doc = comment_text("\n".join(above))
                    call, sig = name, code
                    # the name people call: the named-argument macro over an _ex function, or the
                    # block macro over a _begin_ex one (cui_dialog(ui, ...) { children })
                    block = False
                    for suffix in ("_begin_ex", "_begin", "_ex"):
                        short = name[:-len(suffix)] if name.endswith(suffix) else None
                        if short and short in macros and short not in declared:
                            call, block = short, suffix != "_ex"
                            sig = re.sub(rf"\b{name}\b", short, re.sub(r",?\s*const cui_\w+ \*\s*opts\s*\)$", ", ...)", code))
                            sig = sig.replace("(, ...)", "(...)")
                            if block: sig = sig[sig.index(short):] + " { ... }"
                            break
                    if not block: sig += ";"
                    users = [it["id"] for it in items.values() if call in it["apis"]]
                    page = next((i_ for i_ in sorted(owners.get(call, ())) if i_ in items), None) or (users[0] if len(users) == 1 else None)
                    if fn_struct.get(call) in LAYOUT_STRUCTS: page = None    # the layout guide explains these
                    sec["fns"].append({"name": call, "sig": sig, "doc": comment_html(doc), "line": start + 1,
                                       "opts": fn_struct.get(call) if call != name else None, "page": page})
            i += 1
        if sec["fns"]: out.append(sec)
    # the guide that explains a section whose functions have no component page
    guide_of = {"Layout": "layout", "Theme": "theming", "Fonts": "theming", "Custom drawing": "custom-components", "Identity": "getting-started",
                "Context and frame": "integration", "Input (called by the platform layer)": "integration", "Draw data (consumed by renderers)": "rendering",
                "software renderer": "integration", "OpenGL renderer": "integration", "The app window": "getting-started"}
    used = set()
    for sct in out:
        sct["guide"] = guide_of.get(sct["title"])
        base = slug(sct["title"]); sid = base; k = 2
        while sid in used: sid = f"{base}-{k}"; k += 1
        used.add(sid); sct["id"] = sid
    return out

# ---------------------------------------------------------------- complete programs
# A page's Usage block is a fragment. Where the scene that draws the page's live example
# stands on its own, the page also gets it as a whole program: includes, a frame function
# and main, ready to compile. Each one is compiled here (syntax only), and a scene that
# does not compile alone is left out, so no page shows a program that does not build.
SCENE_GLOBALS = [
    ("b_on", "static bool b_on = true;"), ("b_off", "static bool b_off = false;"), ("b_check", "static bool b_check = true;"),
    ("b_check2", "static bool b_check2 = false;"), ("f_vol", "static float f_vol = 62;"), ("f_lo", "static float f_lo = 25;"),
    ("f_hi", "static float f_hi = 70;"), ("i_seg", "static int i_seg = 1;"), ("i_tab", "static int i_tab = 0;"),
    ("i_radio", "static int i_radio = 1;"), ("i_rating", "static int i_rating = 4;"), ("i_sel", "static int i_sel = 2;"),
    ("i_page", "static int i_page = 2;"), ("s_name", 'static char s_name[64] = "Maya Okafor";'), ("s_email", 'static char s_email[64] = "";'),
    ("s_pw", 'static char s_pw[32] = "secret";'), ("s_search", 'static char s_search[32] = "";'),
    ("s_bio", 'static char s_bio[256] = "Writing software that feels calm.";'), ("d_qty", "static double d_qty = 4;"),
    ("d_date", "static cui_date d_date = { 2026, 11, 14 };"), ("open_true", "static bool open_true = true;"),
]
def scene_program(title, sc):
    src = sc["source"].strip()
    m = re.search(r"^SCENE\(\w+\)\s*\{\n", src, re.M)   # after any comment that leads the scene
    if not m or re.search(r"^(?:static|SCENE)\b", src[m.end():], re.M): return None   # more than the one scene body
    body = src[m.end():src.rindex("}")].rstrip()
    body = re.sub(r"\bUPTO\(([^()]*)\)", r".width = CUI_FILL, .max_width = \1", body)
    body = re.sub(r"\bFLEX\(([^(),]*),\s*([^()]*)\)", r".width = CUI_FILL, .min_width = \1, .max_width = \2", body)
    used = lambda *names: any(re.search(rf"\b{n}\s*\(", body) for n in names)
    inc = ["#include <cui/cui.h>", "#include <cui/cui_app.h>"]
    if used("sinf", "cosf", "sqrtf", "fabsf", "powf", "floorf", "fmodf", "sin", "cos", "expf", "exp", "logf", "log", "roundf", "lround", "fminf", "fmaxf"):
        inc.append("#include <math.h>")
    if used("snprintf", "sprintf", "printf"): inc.append("#include <stdio.h>")
    if used("rand", "abs", "malloc", "free", "atoi"): inc.append("#include <stdlib.h>")
    if used("strlen", "strcmp", "strcpy", "strncpy", "memcpy", "memset", "strstr", "memmove"): inc.append("#include <string.h>")
    globs = [decl for name, decl in SCENE_GLOBALS if re.search(rf"\b{name}\b", body)]
    inner = "\n".join(("    " + l) if l.strip() else l for l in body.split("\n"))
    w, hgt = max(420, int(sc["w"] + 56)), max(320, int(sc["h"] + 56))
    return ("\n".join(inc) + "\n\n" + ("\n".join(globs) + "\n\n" if globs else "")
            + "static void frame(cui_ctx *ui, void *user) {\n    (void)user;\n"
            + "    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .bg = CUI_COLOR_BACKGROUND, .pad = 28, .gap = 16) {\n"
            + inner + "\n    }\n}\n\nint main(void) {\n"
            + f'    return cui_app_run(&(cui_app_desc){{ .title = "{title}", .width = {w}, .height = {hgt} }}, frame, NULL);\n}}\n')

def complete_programs(items, srcs):
    import shutil, subprocess, tempfile
    from concurrent.futures import ThreadPoolExecutor
    cc = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
    cands = []
    for it in items.values():
        for name in it["scenes"]:
            prog = scene_program(it["title"], srcs[name]) if name in srcs else None
            if prog: cands.append((it, prog)); break
    if not cc:
        print("  complete programs: none (no C compiler here to check them with)")
        return
    with tempfile.TemporaryDirectory() as tmp:
        def builds(pair):
            it, prog = pair
            path = os.path.join(tmp, it["id"] + ".c")
            open(path, "w", encoding="utf-8").write(prog)
            return subprocess.run([cc, "-std=c99", "-fsyntax-only", "-w", "-I", os.path.join(ROOT, "include"), path], capture_output=True).returncode == 0
        with ThreadPoolExecutor(8) as ex: ok = list(ex.map(builds, cands))
    for (it, prog), good in zip(cands, ok):
        if good: it["program"] = prog
    print(f"  complete programs: {sum(ok)} of {len(items)} pages (a scene that needs helpers or staged input has none)")

# ---------------------------------------------------------------- related components
def related_items(it, items, cats_order, owners):
    rel = []
    def add(i):
        if i != it["id"] and i in items and i not in rel and len(rel) < 4: rel.append(i)
    # 1. linked from the doc text
    for m in re.finditer(r'href="#/c/([\w-]+)"', it["html"]): add(m.group(1))
    # 2. another component named in the prose, then one whose function the prose names
    prose = re.sub(r"<pre\b.*?</pre>", " ", it["html"], flags=re.S)
    text = html.unescape(re.sub(r"<[^>]+>", " ", prose)).lower()
    for o in items.values():
        t = o["title"].lower()
        if o["id"] != it["id"] and " " in t and re.search(r"(?<![\w-])" + re.escape(t) + r"s?(?![\w-])", text): add(o["id"])
    for a in re.findall(r"<code>(cui_\w+)", prose):
        for o in sorted(owners.get(a, ())): add(o)
    # ... or one whose function the example builds with (not the basics every example uses)
    for a in it["apis"]:
        for o in sorted(owners.get(a, ())):
            if items[o]["category"] != "basics": add(o)
    # 3. the nearest neighbours in the same category
    same = cats_order.get(it["category"], [])
    if it["id"] in same:
        k = same.index(it["id"])
        for d in range(1, len(same)):
            for j in (k + d, k - d):
                if 0 <= j < len(same): add(same[j])
    return rel

def main():
    table = scene_table()
    srcs = scene_sources(table)
    main_secs, _ = sections("docs/components.md", 2)
    more_secs, more_groups = sections("docs/components-more.md", 3)
    more_secs = [s for s in more_secs if s["title"] not in DEPRECATED]
    all_secs = main_secs + more_secs
    anchors, used = {}, set()
    for s in all_secs:
        base = slug(s["title"]); sid = base; k = 2
        while sid in used: sid = f"{base}-{k}"; k += 1
        used.add(sid); s["id"] = sid; anchors[slug(s["title"])] = sid
    for g in more_groups:
        first = next((s for s in more_secs if s["group"] == g), None)
        if first: anchors.setdefault(slug(g), first["id"])

    items, cats = {}, {c: [] for c, _ in CATEGORIES}
    group_scenes = {}
    for g, info in more_groups.items():
        _, found, _ = md_to_html(info["body"], anchors, srcs)
        group_scenes[g] = found
    for s in all_secs:
        body_html, found, statics = md_to_html(s["body"], anchors, srcs)
        scenes_for = found or NO_IMAGE.get(s["title"]) or (group_scenes.get(s["group"]) if s["group"] else None) or []
        scenes_for = [x for x in PREPEND.get(s["title"], []) + scenes_for + EXTRA_FOR.get(s["title"], []) if x in srcs]
        cat = ITEM_CAT.get(s["title"]) or (MORE_CAT.get(s["group"]) if s["group"] else MAIN_CAT.get(s["title"])) or "basics"
        # the page's description is the paragraph right under its heading, before any picture or code
        summary = ""
        lead = next((n for n, l in enumerate(s["body"]) if l.strip()), None)
        if lead is not None and s["body"][lead].strip()[0] not in "<|-#`*":
            end = lead
            while end < len(s["body"]) and s["body"][end].strip(): end += 1
            summary = re.sub(r"[`*]", "", " ".join(l.strip() for l in s["body"][lead:end]))
            body_html, found, statics = md_to_html(s["body"][:lead] + s["body"][end:], anchors, srcs)
            scenes_for = found or NO_IMAGE.get(s["title"]) or (group_scenes.get(s["group"]) if s["group"] else None) or []
            scenes_for = [x for x in PREPEND.get(s["title"], []) + scenes_for + EXTRA_FOR.get(s["title"], []) if x in srcs]
        apis = sorted(set(re.findall(r"\bcui_[a-z0-9_]+(?=\()", html.unescape(body_html))))
        items[s["id"]] = {"id": s["id"], "title": s["title"], "category": cat, "summary": summary,
                          "doc": {"file": s["file"], "line": s["line"], "group": s["group"]},
                          "html": body_html, "scenes": scenes_for, "images": statics, "apis": apis[:40]}
        cats[cat].append(s["id"])

    # the icon set has its own doc page
    icon_lines = read("docs/icons.md").split("\n")
    body_html, _, statics = md_to_html(icon_lines[1:], anchors, srcs)
    items["icons"] = {"id": "icons", "title": "Icons", "category": "basics",
                      "summary": "The Lucide icon set embedded as a font, used by name through CUI_ICON_* constants.",
                      "doc": {"file": "docs/icons.md", "line": 1, "group": None}, "html": body_html,
                      "scenes": ["icons"], "images": statics, "apis": ["cui_icon"]}
    cats["basics"].insert(1, "icons")

    # options from the header, keys from the prose, and nearby components
    fn_struct, structs = header_index()
    owners = api_owners(items)
    cats_order = {c: ids for c, ids in cats.items()}
    for it in items.values():
        it["options"] = item_options(it, owners, fn_struct, structs)
        it["types"] = item_types(it, owners)
        it["keyboard"], it["keyboard_note"] = keyboard_lines(it["html"])
        it["html"] = re.sub(r"\n?<p>Keyboard:.*?</p>", "", it["html"], flags=re.S)   # said once, in the page's Keyboard section
        it["display_only"] = it["id"] in DISPLAY_ONLY and not it["keyboard"] and not it["keyboard_note"]
    for it in items.values():
        it["related"] = related_items(it, items, cats_order, owners)
    for it in items.values():
        # the page lists its own functions, not the helpers its sample happens to call
        own = [a for a in it["apis"] if it["id"] in owners.get(a, ())]
        it["apis"] = own or it["apis"]
        # "Options: `a`, `b` and `c`." only repeats the table that follows it
        if it["options"]:
            it["html"] = re.sub(r"\s*Options: (?:<code>[\w.]+</code>(?:, and |, | and )?)+\.(?=\s|</p>)", "", it["html"])
            it["html"] = re.sub(r"<p>\s*</p>\n?", "", it["html"])

    complete_programs(items, srcs)

    guides = []
    for gid in GUIDES:
        lines = read(f"docs/{gid}.md").split("\n")
        title = next((l[2:].strip() for l in lines if l.startswith("# ")), gid)
        secs, cur, intro = [], None, []
        for n, ln in enumerate(lines, 1):
            if ln.startswith("# "): continue
            if ln.startswith("## "):
                if cur: secs.append(cur)
                t = ln[3:].strip()
                cur = {"title": re.sub(r"^\d+\.\s*", "", t), "id": slug(re.sub(r"^\d+\.\s*", "", t)), "line": n, "body": []}
                continue
            (cur["body"] if cur else intro).append(ln)
        if cur: secs.append(cur)
        def figs(st): return "".join(f'<figure class="figure"><img data-src="{x["src"]}" alt="{html.escape(x["alt"])}"></figure>' for x in st)
        ih, ifound, ist = md_to_html(intro, anchors, {})
        out_secs = []
        for sec in secs:
            bh, _, st = md_to_html(sec["body"], anchors, {})
            out_secs.append({"title": sec["title"], "id": sec["id"], "line": sec["line"], "html": bh + figs(st)})
        lede = ""
        m = re.search(r"<p>(.*?)</p>", ih, re.S)
        if m: lede = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip()
        guides.append({"id": gid, "title": title, "file": f"docs/{gid}.md", "intro": ih + figs(ist), "lede": lede, "sections": out_secs})

    # the changelog: each "## date · title" is a release, newest first
    changelog = {"lede": "", "releases": []}
    if os.path.exists(os.path.join(ROOT, "CHANGELOG.md")):
        lines = read("CHANGELOG.md").split("\n")
        intro, cur = [], None
        for n, ln in enumerate(lines, 1):
            if ln.startswith("# "): continue
            if ln.startswith("## "):
                if cur: changelog["releases"].append(cur)
                head = ln[3:].strip()
                date, _, name = head.partition(" · ")
                cur = {"date": date.strip(), "title": (name or head).strip(), "id": slug(head), "line": n, "body": []}
                continue
            (cur["body"] if cur else intro).append(ln)
        if cur: changelog["releases"].append(cur)
        ih, _, _ = md_to_html(intro, anchors, {})
        changelog["lede"] = html.unescape(re.sub(r"<[^>]+>", "", ih)).strip()
        for r in changelog["releases"]:
            r["html"], _, _ = md_to_html(r.pop("body"), anchors, {})

    showcase = []
    for name, title, desc in SHOWCASE:
        if name in srcs: showcase.append({"id": name, "title": title, "summary": desc, "scenes": [name]})
    accents = [n for n in srcs if n.startswith("accent-")]
    covered = {n for it in items.values() for n in it["scenes"]} | {n for s in showcase for n in s["scenes"]} | set(accents)
    unlisted = [n for n in srcs if n not in covered]

    version = re.search(r"project\(cui\s+VERSION\s+([\d.]+)", read("CMakeLists.txt")).group(1)
    ref = reference(fn_struct, owners, items)
    catalog = {
        "version": version,
        "reference": ref,
        "categories": [{"id": c, "title": t, "items": cats[c]} for c, t in CATEGORIES if cats[c]],
        "items": items,
        "showcase": showcase,
        "accents": accents,
        "apps": [{"id": a, "title": t, "summary": d, "source": f} for a, t, d, f in APPS],
        "scenes": srcs,
        "unlisted": unlisted,
        "guides": guides,
        "changelog": changelog,
        # the doc images that exist, so thumbnails only ask for real files
        "shots": sorted(f"{d}/{n[:-4]}" for d in ("components", "showcase")
                        for n in os.listdir(os.path.join(ROOT, "docs", "images", d)) if n.endswith(".png")),
    }
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// generated by tools/gen_catalog.py, do not edit\nwindow.CUI_CATALOG = ")
        json.dump(catalog, f, ensure_ascii=False, separators=(",", ":"))
        f.write(";\n")
    n_scn = sum(1 for it in items.values() if it["scenes"])
    print(f"  catalog: {len(items)} components ({n_scn} with live scenes), {len(showcase)} showcase, {len(APPS)} apps, "
          f"{len(srcs)} scenes; unlisted scenes: {', '.join(unlisted) or 'none'}")
    n_opt = sum(1 for it in items.values() if it["options"])
    n_key = sum(1 for it in items.values() if it["keyboard"])
    print(f"  options tables: {n_opt} components ({sum(len(it['options']) for it in items.values())} tables); "
          f"keyboard lists: {n_key}; header: {len(structs)} options structs, {len(fn_struct)} functions")
    nfn = sum(len(x["fns"]) for x in ref)
    print(f"  reference: {nfn} functions in {len(ref)} sections, {sum(1 for x in ref for f in x['fns'] if f['doc'])} with a description, "
          f"{sum(1 for x in ref for f in x['fns'] if f['page'])} linked to a page")
    missing = [it["title"] for it in items.values() if not it["scenes"]]
    if missing: print("  no live scene:", ", ".join(missing))

main()
