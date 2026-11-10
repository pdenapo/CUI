#!/usr/bin/env python3
"""Writes a real page for every component and guide, so that search engines and link
previews can read the site.

The site is one app that shows pages by their #/ address, which a search engine sees as a
single page. This writes c/<id>/index.html, guide/<id>/index.html and a few more: each
holds the page's text as plain HTML, its own title, description and preview tags, and the
app itself. A browser that opens one becomes the app at that page; a reader without
JavaScript (a crawler, a link preview) gets the text.

Also writes sitemap.xml and robots.txt.

  python3 web/tools/gen_pages.py web/site
"""
import html, json, os, re, shutil, sys

SITE = sys.argv[1]
URL = "https://yappologistic.github.io/CUI/"
C = json.loads(re.search(r"window\.CUI_CATALOG = (.*);\s*$", open(f"{SITE}/catalog.js", encoding="utf-8").read(), re.S).group(1))
TEMPLATE = open(f"{SITE}/index.html", encoding="utf-8").read()
esc = html.escape

def links(h, up):
    """The app's #/ links as addresses of the pages written here."""
    h = re.sub(r'href="#/c/([\w-]+)[^"]*"', lambda m: f'href="{up}c/{m.group(1)}/"', h)
    h = re.sub(r'href="#/guide/([\w-]+)/([\w-]+)"', lambda m: f'href="{up}guide/{m.group(1)}/#{m.group(2)}"', h)
    h = re.sub(r'href="#/guide/([\w-]+)"', lambda m: f'href="{up}guide/{m.group(1)}/"', h)
    h = re.sub(r'href="#/(components|changelog|showcase|apps)[^"]*"', lambda m: f'href="{up}{m.group(1)}/"', h)
    h = re.sub(r'<img data-src="([^"]+)"', lambda m: f'<img src="{up}{m.group(1)}.png"', h)
    return re.sub(r'<img class="doc-img" src="(images/[^"]+)"', lambda m: f'<img class="doc-img" src="{up}{m.group(1)}"', h)

def page(path, route, title, desc, body, image=None):
    up = "../" * path.count("/")
    canonical = URL + path
    desc = re.sub(r"\s+", " ", desc).strip()
    if len(desc) > 200: desc = desc[:197].rsplit(" ", 1)[0] + "…"
    head = (f'<title>{esc(title)}</title>\n<meta name="description" content="{esc(desc)}">\n'
            f'<link rel="canonical" href="{canonical}">\n'
            f'<meta property="og:type" content="website">\n<meta property="og:site_name" content="CUI">\n'
            f'<meta property="og:title" content="{esc(title)}">\n<meta property="og:description" content="{esc(desc)}">\n'
            f'<meta property="og:url" content="{canonical}">\n<meta property="og:image" content="{URL}{image or "img/social.png"}">\n'
            f'<meta name="twitter:card" content="summary_large_image">')
    t = TEMPLATE
    t = re.sub(r"<title>.*?</title>\n<meta name=\"description\"[^>]*>\n(?:<link rel=\"canonical\"[^>]*>\n)?(?:<meta (?:property|name)=\"(?:og|twitter):[^>]*>\n)*", lambda m: head + "\n", t, count=1, flags=re.S)
    for a in ('href="img/', 'href="style.css"', 'href="responsive.css"', 'src="catalog.js"', 'src="js/'):
        t = t.replace(a, a.replace('="', f'="{up}'))
    t = re.sub(r'<main class="main" id="main">.*?</main>', lambda m: f'<main class="main" id="main"><div class="page static">{links(body, up).replace("{UP}", up)}</div></main>', t, flags=re.S)
    # the app shows pages by their #/ address from the site's root: take that address before
    # the app starts (the files above are already on their way, so their paths still hold)
    boot = f"<script>history.replaceState(null, '', new URL('{up}' + (location.hash || '{route}'), location.href));</script>\n"
    t = t.replace("</body>", boot + "</body>")
    os.makedirs(f"{SITE}/{path}", exist_ok=True)
    open(f"{SITE}/{path}index.html", "w", encoding="utf-8").write(t)
    return canonical

def table(heads, rows):
    return ("<table><thead><tr>" + "".join(f"<th>{h}</th>" for h in heads) + "</tr></thead><tbody>"
            + "".join("<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>" for r in rows) + "</tbody></table>")

def component(it):
    cat = next(c["title"] for c in C["categories"] if c["id"] == it["category"])
    b = [f'<p><a href="#/components">Components</a> / {esc(cat)}</p>', f'<h1>{esc(it["title"])}</h1>', f'<p>{esc(it["summary"])}</p>']
    if it["apis"]: b.append("<p>Functions: " + ", ".join(f"<code>{esc(a)}</code>" for a in it["apis"]) + "</p>")
    shot = next((s for s in it["scenes"] if f"components/{s}" in C["shots"]), None)
    if shot: b.append(f'<p><img src="{{UP}}images/components/{shot}.png" alt="{esc(it["title"])}"></p>')
    b.append("<h2>Usage</h2>" + it["html"])
    if it["keyboard"]: b.append("<h2>Keyboard</h2><ul>" + "".join(f"<li>{k}</li>" for k in it["keyboard"]) + "</ul>")
    elif it["keyboard_note"]: b.append(f'<h2>Keyboard</h2><p>{it["keyboard_note"]}</p>')
    for o in it["options"]:
        b.append(f'<h2>Options of {", ".join(f"<code>{esc(f)}</code>" for f in o["fn"])}</h2>'
                 + table(["Option", "Type", "Default", "Description"], [[f'<code>.{esc(f["name"])}</code>', f'<code>{esc(f["type"])}</code>', esc(f["default"]) or "", f["doc"] or ""] for f in o["fields"]]))
    for ty in it.get("types", []):
        b.append(f'<h2><code>{esc(ty["struct"])}</code></h2>' + table(["Field", "Type", "Description"], [[f'<code>{esc(f["name"])}</code>', f'<code>{esc(f["type"])}</code>', f["doc"] or ""] for f in ty["fields"]]))
        for e in ty["enums"]: b.append(f'<p><code>{esc(e["name"])}</code>: ' + ", ".join(f"<code>{esc(v)}</code>" for v in e["values"]) + "</p>")
    if it["related"]:
        b.append("<h2>Related</h2><ul>" + "".join(f'<li><a href="#/c/{r}">{esc(C["items"][r]["title"])}</a>: {esc(C["items"][r]["summary"])}</li>' for r in it["related"]) + "</ul>")
    return "\n".join(b), (f"images/components/{shot}.png" if shot else None)

urls = []
for d in ("c", "guide", "components", "changelog", "showcase", "apps", "reference"):
    shutil.rmtree(f"{SITE}/{d}", ignore_errors=True)

for it in C["items"].values():
    body, shot = component(it)
    path = f'c/{it["id"]}/'
    urls.append(page(path, f'#/c/{it["id"]}', f'{it["title"]} · CUI, native UI components for C', it["summary"], body, shot))

for g in C["guides"]:
    body = (f'<p>Guide</p><h1>{esc(g["title"])}</h1>{g["intro"]}'
            + "".join(f'<h2 id="{s["id"]}">{esc(s["title"])}</h2>{s["html"]}' for s in g["sections"]))
    urls.append(page(f'guide/{g["id"]}/', f'#/guide/{g["id"]}', f'{g["title"]} · CUI', g["lede"] or g["title"], body))

body = "<h1>Components</h1><p>Every component of CUI, each with a live example, its options and what it does with the keyboard.</p>"
for cat in C["categories"]:
    body += f'<h2>{esc(cat["title"])}</h2><ul>' + "".join(f'<li><a href="#/c/{i}">{esc(C["items"][i]["title"])}</a>: {esc(C["items"][i]["summary"])}</li>' for i in cat["items"]) + "</ul>"
urls.append(page("components/", "#/components", "Components · CUI, native UI components for C",
                 f'All {len(C["items"])} component pages of CUI: buttons, inputs, pickers, tables, charts, editors and developer tools, each running live.', body))

cl = C["changelog"]
body = f'<h1>Changelog</h1><p>{esc(cl["lede"])}</p>' + "".join(f'<h2 id="{r["id"]}">{esc(r["title"])}</h2><p>{esc(r["date"])}</p>{r["html"]}' for r in cl["releases"])
urls.append(page("changelog/", "#/changelog", "Changelog · CUI", cl["lede"], body))

body = "<h1>Showcase</h1><p>Larger compositions that put many components together, each running live.</p><ul>" + "".join(f'<li>{esc(s["title"])}: {esc(s["summary"])}</li>' for s in C["showcase"]) + "</ul>"
urls.append(page("showcase/", "#/showcase", "Showcase · CUI", "Larger compositions built from CUI's components, running live in the browser.", body))
body = "<h1>Apps</h1><p>Complete apps from the examples folder, running live in the browser.</p><ul>" + "".join(f'<li>{esc(a["title"])}: {esc(a["summary"])}</li>' for a in C["apps"]) + "</ul>"
urls.append(page("apps/", "#/apps", "Apps · CUI", "Complete example apps built with CUI, running live in the browser: mail, notes, a tracker, a music player and more.", body))

ref = C.get("reference", [])
if ref:
    body = (f'<h1>Function reference</h1><p>Every public function of CUI {esc(C.get("version", ""))}, in the order of the headers.</p>'
            + "".join(f'<h2 id="{s["id"]}">{esc(s["title"])}</h2>' + "".join(
                f'<p><code>{esc(f["sig"])}</code>' + (f'<br>{f["doc"]}' if f["doc"] else "")
                + (f' <a href="#/c/{f["page"]}">{esc(C["items"][f["page"]]["title"])}</a>' if f["page"] else "") + "</p>" for f in s["fns"]) for s in ref))
    urls.append(page("reference/", "#/reference", "Function reference · CUI", f'Every public function of CUI: {sum(len(s["fns"]) for s in ref)} signatures from cui.h, cui_app.h and cui_render.h, each linked to the page that explains it.', body))

urls.insert(0, URL)   # the home page is index.html itself

with open(f"{SITE}/sitemap.xml", "w") as f:
    f.write('<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'
            + "".join(f"  <url><loc>{u}</loc></url>\n" for u in urls) + "</urlset>\n")
with open(f"{SITE}/robots.txt", "w") as f:
    f.write(f"User-agent: *\nAllow: /\nSitemap: {URL}sitemap.xml\n")
print(f"  pages: {len(urls)} written, with sitemap.xml and robots.txt")
