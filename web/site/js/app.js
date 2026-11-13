// CUI live components: the page around the live canvases.
import { Preview, settings, allLive } from './preview.js';
import { loadModule } from './runtime.js';
import { ditherArt } from './dither.js';

const CAT = window.CUI_CATALOG;
const $ = (s, el = document) => el.querySelector(s);
const h = (tag, attrs = {}, ...kids) => {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs || {})) {
    if (v === undefined || v === null || v === false) continue;
    if (k === 'class') el.className = v;
    else if (k === 'html') el.innerHTML = v;
    else if (k.startsWith('on')) el.addEventListener(k.slice(2), v);
    else el.setAttribute(k, v === true ? '' : v);
  }
  for (const kid of kids.flat()) if (kid !== null && kid !== undefined && kid !== false) el.append(kid.nodeType ? kid : document.createTextNode(kid));
  return el;
};
const svg = (paths, extra = '') => `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" ${extra}>${paths}</svg>`;
const I = {
  // landing page: the same Lucide shapes CUI draws from its icon font
  table: svg('<rect x="3" y="3" width="18" height="18" rx="2"/><path d="M3 9h18M3 15h18M9 3v18"/>'),
  command: svg('<path d="M15 6v12a3 3 0 1 0 3-3H6a3 3 0 1 0 3 3V6a3 3 0 1 0-3 3h12a3 3 0 1 0-3-3"/>'),
  workflow: svg('<rect x="3" y="3" width="8" height="8" rx="2"/><path d="M7 11v4a2 2 0 0 0 2 2h4"/><rect x="13" y="13" width="8" height="8" rx="2"/>'),
  sliders: svg('<path d="M21 4h-7M10 4H3M21 12h-9M8 12H3M21 20h-5M12 20H3M14 2v4M8 10v4M16 18v4"/>'),
  chart: svg('<path d="M3 3v16a2 2 0 0 0 2 2h16"/><path d="m19 9-5 5-4-4-3 3"/>'),
  message: svg('<path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>'),
  kanban: svg('<path d="M6 5v11M12 5v6M18 5v14"/>'),
  window: svg('<rect x="2" y="4" width="20" height="16" rx="2"/><path d="M2 9h20M6 4v5"/>'),
  terminal: svg('<path d="m4 17 6-6-6-6M12 19h8"/>'),
  grid: svg('<rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/>'),
  contrast: svg('<circle cx="12" cy="12" r="10"/><path d="M12 18a6 6 0 0 0 0-12v12z"/>'),
  pen: svg('<path d="M21.2 6.8 17.2 2.8 3 17v4h4z"/><path d="m14 6 4 4"/>'),
  plug: svg('<path d="M12 22v-5M9 8V2M15 8V2M18 8v5a6 6 0 0 1-12 0V8z"/>'),
  ruler: svg('<path d="M21.3 15.3a2.4 2.4 0 0 1 0 3.4l-2.6 2.6a2.4 2.4 0 0 1-3.4 0L2.7 8.7a2.41 2.41 0 0 1 0-3.4l2.6-2.6a2.41 2.41 0 0 1 3.4 0Z"/><path d="m14.5 12.5 2-2M11.5 9.5l2-2M8.5 6.5l2-2M17.5 15.5l2-2"/>'),
  layers: svg('<path d="m12 2 10 5-10 5L2 7z"/><path d="m2 17 10 5 10-5M2 12l10 5 10-5"/>'),
  sun: svg('<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>'),
  moon: svg('<path d="M20 14.5A8 8 0 0 1 9.5 4 8 8 0 1 0 20 14.5z"/>'),
  eye: svg('<path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z"/><circle cx="12" cy="12" r="3"/>'),
  code: svg('<path d="m16 18 6-6-6-6M8 6l-6 6 6 6"/>'),
  reset: svg('<path d="M3 12a9 9 0 1 0 3-6.7L3 8"/><path d="M3 3v5h5"/>'),
  camera: svg('<path d="M14.5 4h-5L7.5 7H4a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-3.5z"/><circle cx="12" cy="13" r="3.5"/>'),
  expand: svg('<path d="M15 3h6v6M9 21H3v-6M21 3l-7 7M3 21l7-7"/>'),
  copy: svg('<rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/>'),
  check: svg('<path d="M20 6 9 17l-5-5"/>'),
  split: svg('<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M12 4v16"/>'),
  grip: svg('<path d="M21 15l-6 6M21 9 9 21"/>'),
  arrow: svg('<path d="M5 12h14M13 6l6 6-6 6"/>'),
  left: svg('<path d="M19 12H5M11 18l-6-6 6-6"/>'),
  wrap: svg('<path d="M3 6h18M3 12h15a3 3 0 1 1 0 6h-4M3 18h7"/><path d="m16 16-2 2 2 2"/>'),
  search: svg('<circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/>'),
  list: svg('<path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/>'),
  monitor: svg('<rect x="2" y="4" width="20" height="13" rx="2"/><path d="M8 21h8M12 17v4"/>'),
};
const ACCENTS = [
  ['Neutral', '#353535', '#bebebe'], ['Violet', '#7747ff', '#a78bff'], ['Blue', '#0562ef', '#6ba3ff'], ['Green', '#00864b', '#53dca6'],
  ['Amber', '#f3ad20', '#ffcb62'], ['Orange', '#c25300', '#ffad5e'], ['Coral', '#d1403a', '#ff9386'], ['Rose', '#d23286', '#ff8bc5'],
];
const BG = { light: '#ffffff', dark: '#121212' };   // CUI_TOKEN_BACKGROUND

// ---------------------------------------------------------------- storage
const store = {
  get(k, d) { try { const v = localStorage.getItem(k); return v ? JSON.parse(v) : d; } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem(k, JSON.stringify(v)); } catch (e) {} },
};
const saved = store.get('cui-web-settings', {});
const prefs = { theme: saved.theme || 'system', accent: saved.accent ?? 0, accentTouched: !!saved.accentTouched,
                renderer: saved.renderer || 'gpu', speed: saved.speed || 1, reducedMotion: !!saved.reducedMotion,
                pixelRatio: saved.pixelRatio || 'auto', pair: !!saved.pair, wrap: !!saved.wrap,
                view: saved.view === 'list' ? 'list' : 'grid' };   // the components index: cards or compact rows
const savePrefs = () => store.set('cui-web-settings', prefs);

const media = matchMedia('(prefers-color-scheme: dark)');
const isDark = () => prefs.theme === 'dark' || (prefs.theme === 'system' && media.matches);
function syncSettings() {
  document.body.classList.toggle('reduce-motion', !!prefs.reducedMotion);
  Object.assign(settings, { renderer: prefs.renderer, speed: prefs.speed, reducedMotion: prefs.reducedMotion,
                            pixelRatio: prefs.pixelRatio, dark: isDark(), accent: prefs.accent, accentTouched: prefs.accentTouched });
}
syncSettings();

function applyTheme() {
  const dark = isDark();
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  settings.dark = dark;
  $('#theme-toggle').innerHTML = dark ? I.moon : I.sun;
  $('#theme-toggle').title = `Theme: ${prefs.theme} (click to switch)`;
  for (const p of allLive()) p.setTheme(dark);
  thumbQueue.length = 0;
  document.querySelectorAll('img[data-thumb]').forEach((img) => { thumbIO?.unobserve(img); if (!staticThumb(img)) loadThumb(img); });
  document.querySelectorAll('.stage:not(.pair)').forEach((s) => { if (!s.dataset.fixedBg) s.style.setProperty('--stage-bg', dark ? BG.dark : BG.light); });
  document.querySelectorAll('details.shots').forEach((d) => d.reload?.());
  renderAccents();
}
media.addEventListener('change', () => { if (prefs.theme === 'system') applyTheme(); });

function renderAccents() {
  const host = $('#accents');
  if (!host) return;
  host.replaceChildren(...ACCENTS.map(([name, l, d], i) => h('button', {
    class: 'swatch', role: 'radio', 'aria-checked': prefs.accentTouched && prefs.accent === i ? 'true' : 'false',
    'aria-label': `${name} accent`, title: `${name} accent`, style: `--c:${isDark() ? d : l}`,
    onclick: () => {
      const reset = prefs.accentTouched && prefs.accent === i;
      prefs.accent = i; prefs.accentTouched = !reset; savePrefs(); syncSettings(); renderAccents();
      for (const p of allLive()) p.setAccent(reset ? (p.content?.accent >= 0 ? p.content.accent : 0) : i);
      thumbQueue.length = 0;
      document.querySelectorAll('img[data-thumb]').forEach((img) => { thumbIO?.unobserve(img); if (!staticThumb(img)) loadThumb(img); });
      toast(reset ? 'Accent: each scene’s own' : `Accent: ${name}`);
    },
  })));
}

// ---------------------------------------------------------------- helpers
function toast(text) {
  const t = h('div', { class: 'toast' }, text);
  $('#toasts').append(t);
  setTimeout(() => t.remove(), 2200);
}
async function copy(text, what = 'Copied') {
  try { await navigator.clipboard.writeText(text); toast(what); }
  catch (e) { toast('Copy failed: clipboard not available'); }
}
const esc = (s) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

// C syntax highlighting: comments, strings, numbers, keywords, CUI_ constants, cui_ calls, .fields
const KW = new Set('static const int float double bool char void if else for while do return switch case break continue default struct typedef sizeof unsigned signed enum true false NULL long short uint8_t uint32_t size_t'.split(' '));
function highlight(src) {
  const re = /(\/\*[\s\S]*?\*\/|(?<!:)\/\/[^\n]*)|("(?:[^"\\\n]|\\.)*")|('(?:[^'\\\n]|\\.)*')|(\b\d+(?:\.\d+)?f?\b|\b0x[0-9a-fA-F]+\b)|(\.[a-z_][a-z0-9_]*(?=\s*=))|(\b[A-Za-z_][A-Za-z0-9_]*\b)/g;
  let out = '', last = 0, m;
  while ((m = re.exec(src))) {
    out += esc(src.slice(last, m.index));
    const t = esc(m[0]);
    if (m[1]) out += `<span class="tok-com">${t}</span>`;
    else if (m[2] || m[3]) out += `<span class="tok-str">${t}</span>`;
    else if (m[4]) out += `<span class="tok-num">${t}</span>`;
    else if (m[5]) out += `<span class="tok-field">${t}</span>`;
    else if (KW.has(m[6])) out += `<span class="tok-kw">${t}</span>`;
    else if (/^CUI_[A-Z0-9_]+$/.test(m[6])) out += `<span class="tok-const">${t}</span>`;
    else if (/^cui_\w+$/.test(m[6]) && src[re.lastIndex] === '(') out += `<span class="tok-fn">${t}</span>`;
    else out += t;
    last = re.lastIndex;
  }
  return out + esc(src.slice(last));
}

// ---------------------------------------------------------------- thumbnails
// Rendered live from the scenes module (software renderer, one shared
// instance), so they always match the current code.
let thumbMod = null;
const thumbCache = new Map();
const thumbQueue = [];
let thumbBusy = false;
async function renderThumb(module, name, dark, accent) {
  const key = `${module}:${name}:${dark}:${accent}`;
  if (thumbCache.has(key)) return thumbCache.get(key);
  const p = (async () => {
    const mod = module === 'scenes' ? (thumbMod ||= await loadModule('wasm/scenes.wasm')) : await loadModule(`wasm/${module}.wasm`);
    const x = mod.exports;
    const c = name ? mod.find(name) : mod.contents[0];
    if (!c) return '';
    const scale = module === 'scenes' ? Math.min(1.5, Math.max(0.5, 560 / c.width)) : 0.5;
    const a = accent >= 0 ? accent : (c.accent >= 0 ? c.accent : 0);
    const inst = x.inst_new(c.index, c.width, c.height, scale, c.theme === 1 ? 1 : dark, a, 0);
    if (module !== 'scenes') x.inst_frame(inst, 1 / 60, 0);
    x.inst_paint(inst);
    const px = mod.pixels(inst);
    const cv = document.createElement('canvas');
    cv.width = px.w; cv.height = px.h;
    cv.getContext('2d').putImageData(new ImageData(new Uint8ClampedArray(px.data), px.w, px.h), 0, 0);
    x.inst_free(inst);
    const blob = await new Promise((r) => cv.toBlob(r, 'image/png'));
    return URL.createObjectURL(blob);
  })();
  thumbCache.set(key, p);
  return p;
}
function pumpThumbs() {
  if (thumbBusy || !thumbQueue.length) return;
  thumbBusy = true;
  const img = thumbQueue.shift();
  const run = async () => {
    try {
      const dark = isDark() ? 1 : 0, accent = prefs.accentTouched ? prefs.accent : -1;
      const want = `${dark}:${accent}`;
      if (img.isConnected && img.dataset.state !== want) {
        img.src = await renderThumb(img.dataset.module || 'scenes', img.dataset.thumb, dark, accent);
        img.dataset.state = want;
      }
    } catch (e) { console.warn('thumb', e); }
    thumbBusy = false;
    (window.requestIdleCallback || setTimeout)(pumpThumbs);
  };
  run();
}
const thumbIO = 'IntersectionObserver' in window ? new IntersectionObserver((es) => {
  for (const e of es) if (e.isIntersecting) { thumbIO.unobserve(e.target); thumbQueue.push(e.target); }
  pumpThumbs();
}, { rootMargin: '300px' }) : null;
function loadThumb(img) {
  if (thumbIO) thumbIO.observe(img); else { thumbQueue.push(img); pumpThumbs(); }
}
// Thumbnails use the rendered doc images when one exists (instant, and the
// same pixels as the live scene); anything else is rendered live.
const staticThumb = (img) => {
  const name = img.dataset.thumb, dark = isDark();
  if (!name || img.dataset.module !== 'scenes' || img.dataset.live || (prefs.accentTouched && prefs.accent)) return false;
  const dir = CAT.showcase.some((x) => x.id === name) ? 'showcase' : 'components';
  const file = `${dir}/${name}${dark ? '-dark' : ''}`;
  if (CAT.shots && !CAT.shots.includes(file)) return false;
  img.onerror = () => { img.onerror = null; img.dataset.live = '1'; loadThumb(img); };
  img.src = `images/${file}.png`;
  img.dataset.state = `${dark ? 1 : 0}:-1`;
  return true;
};
const thumb = (name, module = 'scenes') => {
  const img = h('img', { alt: '', 'data-thumb': name || '', 'data-module': module, loading: 'lazy', decoding: 'async' });
  if (!staticThumb(img)) loadThumb(img);
  return img;
};

// ---------------------------------------------------------------- preview blocks
let blocks = [];

// Full / Tablet / Phone for a preview: a radio group with one tab stop, arrows
// move the choice. set(null) shows a size dragged by hand (none of the three).
const DEVICE_WIDTHS = [['Full', 0, 'Natural size, following the page'], ['Tablet', 768, 'Tablet: 768 px wide'], ['Phone', 375, 'Phone: 375 px wide']];
function widthPresets(onPick) {
  const seg = h('div', { class: 'seg pv-width', role: 'radiogroup', 'aria-label': 'Preview width' });
  const buttons = DEVICE_WIDTHS.map(([label, w, title]) => h('button', { type: 'button', role: 'radio', title, 'data-w': w, onclick: () => pick(w, true) }, label));
  seg.append(...buttons);
  const set = (w) => buttons.forEach((b, i) => {
    const on = DEVICE_WIDTHS[i][1] === w;
    b.setAttribute('aria-checked', String(on));
    b.tabIndex = on || (w === null && i === 0) ? 0 : -1;
  });
  const pick = (w, fire) => { set(w); if (fire) onPick(w); };
  seg.addEventListener('keydown', (e) => {
    const i = buttons.indexOf(document.activeElement);
    if (i < 0) return;
    const d = e.key === 'ArrowRight' || e.key === 'ArrowDown' ? 1 : e.key === 'ArrowLeft' || e.key === 'ArrowUp' ? -1 : 0;
    const j = e.key === 'Home' ? 0 : e.key === 'End' ? buttons.length - 1 : d ? (i + d + buttons.length) % buttons.length : -1;
    if (j < 0) return;
    e.preventDefault();
    buttons[j].focus();
    pick(DEVICE_WIDTHS[j][1], true);
  });
  set(0);
  seg.set = set;
  return seg;
}
function disposeBlocks() { blocks.forEach((b) => b.dispose()); blocks = []; }

// A preview with a toolbar (Preview / Code) and tools.
// opts: { module, content, title, fill, codeSource, codeFile, codeLine, docRef, compact, pair }
function previewBlock(opts) {
  const sc = opts.module === 'scenes' ? CAT.scenes[opts.content] : null;
  const block = { previews: [], mounted: false, dispose() { this.unmount(); this.io?.disconnect(); } };
  const root = h('section', { class: 'preview fullscreen-host' });
  block.root = root;
  const stage = h('div', { class: 'stage' + (opts.fill ? ' flush' : '') + (opts.tight ? ' tight' : '') });
  const status = h('div', { class: 'pv-status' });
  const codeHost = h('div', { class: 'hidden' });
  let mode = 'preview';
  let pair = opts.bare ? false : (opts.pair ?? (prefs.pair && !opts.fill && !sc?.dark_only));

  const tabs = h('div', { class: 'tabs', role: 'tablist' },
    h('button', { role: 'tab', 'aria-selected': 'true', html: I.eye + 'Preview', onclick: () => setMode('preview') }),
    h('button', { role: 'tab', 'aria-selected': 'false', html: I.code + 'Code', onclick: () => setMode('code') }));
  const tool = (icon, label, fn, pressed, cls) => h('button', { class: 'icon-btn' + (cls ? ' ' + cls : ''), title: label, 'aria-label': label, html: icon, onclick: fn,
                                                          'aria-pressed': pressed === undefined ? null : String(pressed) });
  const pairBtn = tool(I.split, 'Light and dark side by side', () => { pair = !pair; pairBtn.setAttribute('aria-pressed', String(pair)); widthBox?.classList.toggle('hidden', pair); remount(); }, pair);
  const codeBtn = opts.leading ? tool(I.code, 'Show the code', () => setMode(mode === 'code' ? 'preview' : 'code'), false) : null;
  // device widths for one running preview: the same resize as the corner grip, in one step
  const widthSeg = opts.fill || opts.bare || opts.widths === false ? null : widthPresets((w) => {
    block.width = w;
    const p = block.previews[0];
    if (p && !pair) p.setWidth(w);
  });
  const widthBox = widthSeg ? h('div', { class: 'pv-width-box' + (pair ? ' hidden' : '') }, widthSeg,
    codeBtn || !sc?.dark_only ? h('span', { class: 'sep' }) : null) : null;
  const tools = h('div', { class: 'pv-tools' },
    widthBox,
    codeBtn,
    opts.fill || sc?.dark_only ? null : pairBtn,
    opts.fill && sc?.dark_only ? null : h('span', { class: 'sep' }),
    tool(I.reset, 'Reset: restart this preview from its first frame', () => { remount(); toast('Preview reset'); }),
    tool(I.camera, 'Save a PNG of this preview', async () => {
      const p = block.previews[0];
      if (!p) return;
      const blob = await p.snapshot();
      const a = h('a', { href: URL.createObjectURL(blob), download: `${opts.content || opts.module}${p.dark ? '-dark' : ''}.png` });
      a.click();
    }),
    tool(I.expand, 'Full screen', () => { if (document.fullscreenElement) document.exitFullscreen(); else root.requestFullscreen?.(); }));
  const bar = h('div', { class: 'pv-bar' }, opts.leading || tabs, tools);
  if (opts.bare) root.append(stage); else root.append(bar, stage, codeHost);

  function setMode(m) {
    mode = m;
    tabs.children[0].setAttribute('aria-selected', String(m === 'preview'));
    tabs.children[1].setAttribute('aria-selected', String(m === 'code'));
    codeBtn?.setAttribute('aria-pressed', String(m === 'code'));
    stage.classList.toggle('hidden', m !== 'preview');
    status.classList.toggle('hidden', m !== 'preview');
    codeHost.classList.toggle('hidden', m !== 'code');
    if (m === 'code' && !codeHost.firstChild) {
      if (typeof opts.codeSource === 'function') {
        codeHost.append(h('div', { class: 'loading' }, h('i', { class: 'spinner' }), 'Loading source'));
        opts.codeSource().then((src) => codeHost.replaceChildren(codeView(src || '/* source not found */', opts.codeFile, opts.codeLine)));
      } else codeHost.append(codeView(opts.codeSource, opts.codeFile, opts.codeLine));
    }
  }

  function updateStatus(p) {
    if (!p || !p.inst) return;
    const s = p.stats;
    status.replaceChildren(...[
      h('span', { class: 'live' + (p.animating ? '' : ' idle') }, h('i'), p.animating ? 'Animating' : 'Live'),
      h('span', {}, `${p.w} × ${p.h}`),
      h('span', {}, `build ${s.build.toFixed(2)} ms`),
      h('span', {}, `paint ${s.paint.toFixed(2)} ms`),
      h('span', {}, `${s.tris.toLocaleString()} triangles`),
      h('span', {}, p.rendererName() + (p.fit < 1 ? `, shown at ${Math.round(p.fit * 100)}%` : '')),
      settings.speed !== 1 ? h('span', {}, `${settings.speed}× speed`) : null,
      p.hasKeyboard()
        ? h('span', { class: 'hint kbd-on' }, 'Keyboard goes to the preview. Click outside to leave.')
        : h('span', { class: 'hint' }, opts.fill ? 'Click inside to use the keyboard' : 'Click to focus, then Tab through it. Drag the corner to resize.')].filter(Boolean));
  }

  function mountOne(host, dark, label) {
    const p = new Preview({ module: opts.module, content: opts.content, fill: opts.fill, dark, ownAccent: opts.fixedAccent,
                            container: host, padding: opts.fill || opts.tight ? 0 : opts.bare ? 24 : (pair ? 48 : 56), onStats: updateStatus,
                            width: opts.width, height: opts.height });
    host.append(p.el);
    if (label) host.append(h('span', { class: 'half-label' }, label));
    block.previews.push(p);
    p.start().then(() => {
      host.querySelector('.loading')?.remove();
      updateStatus(p);
      if (opts.fill) fitFill(p, host);
      else if (!pair && !opts.bare) { addResize(p); if (block.width) p.setWidth(block.width); }
    }).catch((e) => {
      console.error(e);
      host.querySelector('.loading')?.remove();
      host.append(h('div', { class: 'error' }, String(e && e.stack || e)));
    });
    return p;
  }
  function addResize(p) {
    const grip = h('div', { class: 'resize', title: 'Drag to resize the viewport. Double-click for the original size.', html: I.grip });
    p.el.append(grip);
    grip.addEventListener('pointerdown', (e) => {
      e.preventDefault(); e.stopPropagation();
      grip.setPointerCapture(e.pointerId);
      const sx = e.clientX, sy = e.clientY, w0 = p.w, h0 = p.h, f = p.fit;
      const move = (ev) => { p.resize(w0 + (ev.clientX - sx) / f, h0 + (ev.clientY - sy) / f); block.width = 0; widthSeg?.set(null); };
      const up = () => { grip.removeEventListener('pointermove', move); grip.removeEventListener('pointerup', up); };
      grip.addEventListener('pointermove', move);
      grip.addEventListener('pointerup', up);
    });
    grip.addEventListener('dblclick', () => { p.resize(p.natural.w, p.natural.h); block.width = 0; widthSeg?.set(0); });
  }
  function fitFill(p, host) {
    const ro = new ResizeObserver(() => {
      const r = host.getBoundingClientRect();
      const w = opts.fixedWidth ? opts.fixedWidth() : r.width;
      if (w > 0 && r.height > 0) p.resize(w, r.height);
    });
    ro.observe(host);
    block.ro = ro;
  }

  function mount() {
    if (block.mounted) return;
    block.mounted = true;
    stage.replaceChildren();
    stage.className = 'stage' + (opts.fill ? ' flush' : '') + (pair ? ' pair' : '');
    if (pair) {
      const wide = sc && sc.w > 520;
      stage.classList.toggle('stack', wide);
      const a = h('div', { class: 'half', style: `background:${BG.light}` }, h('div', { class: 'loading' }, h('i', { class: 'spinner' }), 'Starting'));
      const b = h('div', { class: 'half', style: `background:${BG.dark}` });
      stage.append(a, b);
      mountOne(a, false, 'Light');
      mountOne(b, true, 'Dark');
    } else {
      stage.style.setProperty('--stage-bg', sc?.dark_only || (opts.forceDark) ? BG.dark : (isDark() ? BG.dark : BG.light));
      if (sc?.dark_only) stage.dataset.fixedBg = '1';
      stage.append(h('div', { class: 'loading' }, h('i', { class: 'spinner' }), 'Starting'));
      if (!opts.fill && sc) stage.style.minHeight = `${Math.min(sc.h, 900) * Math.min(1, (stage.clientWidth || 900) / (sc.w + 56)) + 58}px`;
      mountOne(stage, undefined);
    }
  }
  block.unmount = function () {
    block.ro?.disconnect();
    block.previews.forEach((p) => p.dispose());
    block.previews = [];
    block.mounted = false;
  };
  function remount() { block.unmount(); mount(); }
  block.remount = remount;

  // mount when near the viewport; free GPU contexts when far away
  block.io = new IntersectionObserver((es) => {
    for (const e of es) {
      if (e.isIntersecting) mount();
      else if (block.mounted && !opts.fill) {
        const r = e.boundingClientRect;
        if (r.bottom < -1600 || r.top > innerHeight + 1600) block.unmount();
      }
    }
  }, { rootMargin: '400px 0px', threshold: 0 });
  block.io.observe(root);
  block.farIO = null;
  block.el = root;
  block.refit = () => block.previews.forEach((p) => p.refit());
  blocks.push(block);
  if (opts.mode === 'code') setMode('code');
  return root;
}

/* word wrap for code: one preference for the whole site, a toggle on every code block */
function applyWrap() {
  document.body.classList.toggle('wrap-code', prefs.wrap);
  document.querySelectorAll('.wrap-btn').forEach((b) => b.setAttribute('aria-pressed', String(prefs.wrap)));
}
function wrapButton(cls = 'icon-btn code-wrap', label = '') {
  return h('button', { class: `${cls} wrap-btn`, title: 'Wrap long lines', 'aria-label': 'Wrap long lines', 'aria-pressed': String(prefs.wrap),
    html: I.wrap + label, onclick: () => { prefs.wrap = !prefs.wrap; savePrefs(); applyWrap(); } });
}
function addCopy(root) {
  root.querySelectorAll('pre.code').forEach((pre) => {
    if (pre.querySelector('.code-copy')) return;
    pre.classList.add('copyable');
    pre.append(h('div', { class: 'code-tools' }, wrapButton(),
      h('button', { class: 'icon-btn code-copy', title: 'Copy', 'aria-label': 'Copy code', html: I.copy, onclick: () => copy(pre.querySelector('code').textContent, 'Code copied') })));
  });
}
// highlighted source split into lines, closing and reopening a token that spans a line break
function htmlLines(html) {
  const out = [], open = [];
  let cur = '', m;
  const re = /(<span[^>]*>)|(<\/span>)|(\n)|([^<\n]+|<)/g;
  while ((m = re.exec(html))) {
    if (m[1]) { open.push(m[1]); cur += m[1]; }
    else if (m[2]) { open.pop(); cur += m[2]; }
    else if (m[3]) { out.push(cur + '</span>'.repeat(open.length)); cur = open.join(''); }
    else cur += m[4];
  }
  out.push(cur);
  return out;
}
function codeView(source, file, line) {
  // one row per line, so the numbers stay beside their line when long lines wrap
  const rows = htmlLines(highlight(source || '')).map((l, i) =>
    h('span', { class: 'row' }, h('span', { class: 'ln' }, String(line + i)), h('code', { class: 'src', html: l + '\n' })));
  return h('div', { class: 'code-view' },
    h('div', { class: 'code-head' }, h('code', {}, `${file}:${line}`), h('span', { class: 'spacer' }),
      wrapButton('btn ghost sm', 'Wrap'),
      h('button', { class: 'btn ghost sm', html: I.copy + 'Copy code', onclick: () => copy(source, 'Code copied') }),
      h('button', { class: 'btn ghost sm', html: I.copy + 'Copy path', onclick: () => copy(`${file}:${line}`, 'Path copied') })),
    h('pre', {}, rows));
}

// ---------------------------------------------------------------- sidebar
const order = CAT.categories.flatMap((c) => c.items);
function sidebarMode() {
  const k = (location.hash.match(/^#\/?([^/?]*)/) || [])[1] || '';
  return (k === 'guide' || k === 'reference') ? 'guide' : (k === 'c' || k === 'components') ? 'components' : null;
}
function renderGuideNav(q) {
  const nav = $('#nav');
  const [, gid, sec] = location.hash.match(/^#\/guide\/([^/]+)\/?(.*)$/) || [];
  const groups = [['Guide', CAT.guides.filter((g) => g.id !== 'design')], ['Principles', CAT.guides.filter((g) => g.id === 'design')]];
  const out = [];
  for (const [title, list] of groups) {
    const items = list.filter((g) => !q || g.title.toLowerCase().includes(q) || g.sections.some((x) => x.title.toLowerCase().includes(q)));
    if (!items.length) continue;
    out.push(h('div', { class: 'nav-section' }, h('div', { class: 'nav-title' }, title),
      items.flatMap((g) => [
        h('a', { href: `#/guide/${g.id}`, 'aria-current': g.id === gid && !sec ? 'page' : null }, h('span', { class: 'label' }, g.title)),
        g.id === gid || q ? h('div', { class: 'nav-sub' }, g.sections.filter((x) => !q || x.title.toLowerCase().includes(q) || g.title.toLowerCase().includes(q))
          .map((x) => h('a', { href: `#/guide/${g.id}/${x.id}`, 'aria-current': g.id === gid && x.id === sec ? 'page' : null }, h('span', { class: 'label' }, x.title)))) : null])));
  }
  out.push(h('div', { class: 'nav-section' }, h('div', { class: 'nav-title' }, 'Reference'),
    h('a', { href: '#/reference', 'aria-current': /^#\/reference/.test(location.hash) ? 'page' : null }, h('span', { class: 'label' }, 'All functions')),
    h('a', { href: '#/components' }, h('span', { class: 'label' }, 'All components'))));
  nav.replaceChildren(...(out.length ? out : [h('div', { class: 'empty' }, `Nothing matches “${q}”.`)]));
}
function renderNav(filter = '') {
  const q = filter.trim().toLowerCase();
  const nav = $('#nav');
  const mode = sidebarMode();
  document.body.classList.toggle('no-sidebar', !mode);
  if (!mode) return;
  $('#filter').placeholder = mode === 'guide' ? 'Filter the guide' : 'Filter components';
  if (mode === 'guide') return renderGuideNav(q);
  const sections = [];
  const route = location.hash;
  const match = (it) => !q || it.title.toLowerCase().includes(q) || it.summary.toLowerCase().includes(q) || it.apis.some((a) => a.includes(q));
  for (const c of CAT.categories) {
    const items = c.items.map((id) => CAT.items[id]).filter(match);
    if (!items.length) continue;
    sections.push(h('div', { class: 'nav-section' },
      h('div', { class: 'nav-title' }, c.title),
      items.map((it) => h('a', { href: `#/c/${it.id}`, 'aria-current': route === `#/c/${it.id}` || route.startsWith(`#/c/${it.id}/`) ? 'page' : null },
        h('span', { class: 'label' }, it.title), newBadge(it.id, 'nav-new')))));
  }
  nav.replaceChildren(...(sections.length ? sections : [h('div', { class: 'empty' }, `Nothing matches “${filter}”.`)]));
  const cur = nav.querySelector('[aria-current="page"]');
  if (cur && !filter) cur.scrollIntoView({ block: 'nearest' });
}
$('#filter').addEventListener('input', (e) => renderNav(e.target.value));
$('#filter').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') { const a = $('#nav a:not([aria-current])') || $('#nav a'); if (a) location.hash = a.getAttribute('href'); }
  if (e.key === 'Escape') { e.target.value = ''; renderNav(); }
});

// ---------------------------------------------------------------- views
const main = $('#main');
// the release these docs describe, beside the name
if (CAT.version) $('.brand').append(h('span', { class: 'brand-ver', title: `These docs describe CUI ${CAT.version}` }, CAT.version));
function setNav(which) {
  const nav = $('#topnav');
  nav.querySelectorAll('a').forEach((a) => a.setAttribute('aria-current', a.dataset.nav === which ? 'page' : 'false'));
  // on a phone the links scroll sideways: keep the current one in view
  const cur = nav.querySelector('a[aria-current="page"]');
  if (cur && nav.scrollWidth > nav.clientWidth) {
    const l = cur.offsetLeft - nav.offsetLeft, r = l + cur.offsetWidth;
    if (l < nav.scrollLeft || r > nav.scrollLeft + nav.clientWidth - 24) nav.scrollLeft = Math.max(0, r - nav.clientWidth + 32);
  }
  navFades();
}
// fade an edge of the scrolling nav only while there are links beyond it
function navFades() {
  const nav = $('#topnav');
  nav.classList.toggle('more-l', nav.scrollLeft > 1);
  nav.classList.toggle('more-r', nav.scrollLeft + nav.clientWidth < nav.scrollWidth - 1);
}
$('#topnav').addEventListener('scroll', navFades, { passive: true });
addEventListener('resize', navFades);

const FIRST_WINDOW = `#include <cui/cui.h>
#include <cui/cui_app.h>

int main(void) {
    cui_app *app = cui_app_create(&(cui_app_desc){ .title = "My app" });
    if (!app) return 1;
    bool notify = true;
    int clicks = 0;
    while (cui_app_frame(app)) {
        cui_ctx *ui = cui_app_ui(app);
        cui_column(ui, .pad = 24, .gap = 12) {
            cui_heading(ui, "Welcome", 2);
            cui_switch(ui, "Email me when a build fails", &notify);
            if (cui_button(ui, "Count", .variant = CUI_BUTTON_PRIMARY)) clicks++;
            cui_textf(ui, "Clicked %d times", clicks);
        }
    }
    cui_app_destroy(app);
    return 0;
}`;

const INSTALL = [
  ['CMake', `include(FetchContent)
FetchContent_Declare(cui
    GIT_REPOSITORY https://github.com/yappologistic/CUI.git
    GIT_TAG master)
FetchContent_MakeAvailable(cui)
target_link_libraries(my_app PRIVATE cui::cui)`],
  ['Git', 'git clone https://github.com/yappologistic/CUI.git'],
];

function installBlock() {
  let cur = 0;
  const code = h('code', {});
  const show = () => { code.textContent = INSTALL[cur][1]; tabs.querySelectorAll('button').forEach((b, i) => b.setAttribute('aria-selected', String(i === cur))); };
  const tabs = h('div', { class: 'install-tabs', role: 'tablist' }, INSTALL.map(([label], i) => h('button', { role: 'tab', onclick: () => { cur = i; show(); } }, label)));
  const box = h('div', { class: 'install' }, tabs,
    h('div', { class: 'install-code' }, h('pre', {}, code),
      h('div', { class: 'code-tools' }, wrapButton(),
        h('button', { class: 'icon-btn code-copy', title: 'Copy', 'aria-label': 'Copy', html: I.copy, onclick: () => copy(INSTALL[cur][1], 'Copied') }))));
  show();
  return box;
}

const FACTS = [
  ['0.1 to 0.25 ms a frame', 'A full screen builds, lays out and paints in a fraction of a millisecond. An idle app uses no CPU.'],
  ['About 250 KB', 'That is a complete app, window and fonts included. CUI draws its own components, so there is no browser, no runtime and no dependency to ship.'],
  ['Win32, Cocoa and X11', 'Or SDL2 and GLFW, drawn with OpenGL or the software renderer. The core only emits triangles, so your own renderer works too.'],
  ['Keyboard and reduced motion', 'Controls take Tab, Enter, Space and the arrow keys, and each component page lists its keys. Every animation has a reduced motion path, and text meets 4.5:1 contrast.'],
];
const ART = 'img/tetons.jpg';

// the hexagon C, drawn from simple geometry: three blues and a white C whose
// mouth follows the dark wedge. viewBox has room for the rounded corners.
const C_LOGO = `<svg class="c-logo" viewBox="-14 -14 388 432" role="img" aria-label="C">
<defs><mask id="c-hex"><polygon points="180,0 360,101 360,303 180,404 0,303 0,101" fill="#fff" stroke="#fff" stroke-width="28" stroke-linejoin="round"/></mask></defs>
<g mask="url(#c-hex)">
<polygon points="180,202 0,303 0,101 180,-20 380,101" fill="#659ad2"/>
<polygon points="180,202 380,303 180,424 0,303" fill="#00599c"/>
<polygon points="180,202 380,101 380,303" fill="#004482"/>
<path d="M282.9 144.3A118 118 0 1 0 282.9 259.7L230.6 230.4A58 58 0 1 1 230.6 173.6Z" fill="#fff"/>
</g></svg>`;
// the landing page: the hero's controls, the showcase, the bento and the
// playground are all the library itself, compiled to WebAssembly
const STRIP = [['~190', 'components'], ['~250 KB', 'a complete app'], ['< 0.25 ms', 'per frame'], ['Win · Mac · Linux · Web', 'one C99 codebase'], ['0', 'dependencies']];
const SHOWCASE = [['stage-mosaic', 'Overview', 'layers'], ['app-chrome', 'Editor', 'window'], ['hero-data', 'Data', 'table'],
                  ['hero-plan', 'Planning', 'kanban'], ['hero-ai', 'Assistant', 'message'], ['hero-charts', 'Charts', 'chart']];
const BENTO = [
  ['data-grid', 'data-grid-and-filter-bar', 'table', 'Data grid', 'Thousands of rows, sorting, filters and spreadsheet selection.', 'wide'],
  ['command-palette', 'command-palette', 'command', 'Command palette', 'Fuzzy search with groups and shortcuts.', ''],
  ['node-editor', 'node-editor', 'workflow', 'Node editor', 'Pan, zoom and wire nodes together.', 'wide'],
  ['inspector', 'property-inspector', 'sliders', 'Property inspector', 'Typed editors in folding groups.', 'tall'],
  ['line-chart', 'charts', 'chart', 'Charts', 'Lines, bars, donuts and a dozen more.', ''],
  ['chat', 'chat', 'message', 'Chat', 'Bubbles, replies and reactions.', ''],
];

function dropBlock(root) {
  const b = blocks.find((x) => x.root === root);
  if (b) { b.dispose(); blocks.splice(blocks.indexOf(b), 1); }
}

// one large stage; the tabs swap which scene runs in it
function showcase() {
  const host = h('div', { class: 'show-stage' });
  const items = SHOWCASE.filter(([scene]) => CAT.scenes[scene]);
  let cur = -1, root = null;
  const ind = h('span', { class: 'slide-ind', 'aria-hidden': 'true' });
  const tabs = h('div', { class: 'show-tabs', role: 'tablist', 'aria-label': 'Showcase' }, ind,
    items.map(([, label, icon], i) => h('button', { role: 'tab', 'aria-selected': 'false', html: I[icon] + `<span>${label}</span>`, onclick: () => pick(i) })));
  function pick(i) {
    if (i === cur) return;
    const first = cur < 0;
    cur = i;
    tabs.querySelectorAll('button').forEach((b, k) => b.setAttribute('aria-selected', String(k === i)));
    slideTo(ind, tabs.querySelectorAll('button')[i], first);
    if (root) dropBlock(root);
    const scene = items[i][0], sc = CAT.scenes[scene];
    root = previewBlock({ module: 'scenes', content: scene, title: scene, item: items[i][1], tight: true,
                          codeSource: sc.source, codeFile: sc.file, codeLine: sc.line, widths: false });
    if (!first) root.classList.add('swap-in');
    host.replaceChildren(root);
  }
  // the first scene starts once the hero has settled, so its startup does not
  // compete with the hero's entrance on load
  const first = tabs.querySelectorAll('button')[0];
  if (first) { first.setAttribute('aria-selected', 'true'); requestAnimationFrame(() => slideTo(ind, first, true)); }
  setTimeout(() => { if (host.isConnected && cur < 0) pick(0); }, motionOff() ? 0 : 700);
  keepSliding(tabs, ind, () => tabs.querySelectorAll('button')[Math.max(cur, 0)]);
  return h('div', { class: 'previews home-show' }, tabs, host);
}

function bentoTile([scene, item, icon, title, text, size]) {
  const sc = CAT.scenes[scene];
  if (!sc) return null;
  const it = CAT.items[item];
  return h('div', { class: 'tile' + (size ? ' ' + size : '') },
    h('div', { class: 'tile-head' },
      h('span', { class: 'tile-icon', html: I[icon] }),
      h('div', { class: 'tile-text' }, h('h3', {}, title), h('p', {}, text)),
      it ? h('a', { class: 'demo-link', href: `#/c/${item}`, 'aria-label': `Open ${title}`, html: 'Docs' + I.arrow }) : null),
    previewBlock({ module: 'scenes', content: scene, bare: true, item: it?.title, codeSource: sc.source, codeFile: sc.file, codeLine: sc.line }));
}

// light or dark and an accent, for one live page of components
function playground() {
  const scene = CAT.scenes['stage-mosaic-pro'] ? 'stage-mosaic-pro' : 'stage-mosaic';
  const sc = CAT.scenes[scene];
  const root = previewBlock({ module: 'scenes', content: scene, title: scene, item: 'Overview', tight: true,
                              codeSource: sc.source, codeFile: sc.file, codeLine: sc.line, widths: false });
  let dark = null, accent = -1;
  const apply = () => {
    const b = blocks.find((x) => x.root === root);
    if (!b) return;
    const stage = root.querySelector('.stage');
    for (const p of b.previews) {
      if (dark !== null) p.setTheme(dark);
      if (accent >= 0) p.setAccent(accent);
    }
    if (dark !== null && stage) stage.style.setProperty('--stage-bg', dark ? BG.dark : BG.light);
  };
  const segInd = h('span', { class: 'slide-ind', 'aria-hidden': 'true' });
  const checked = () => mode.querySelector('button[aria-checked="true"]');
  const mode = h('div', { class: 'seg', role: 'radiogroup', 'aria-label': 'Theme' }, segInd,
    [['Light', false, I.sun], ['Dark', true, I.moon]].map(([label, d, icon]) => h('button', {
      role: 'radio', 'aria-checked': String(isDark() === d), html: icon + `<span>${label}</span>`,
      onclick: (e) => { dark = d; mode.querySelectorAll('button').forEach((b) => b.setAttribute('aria-checked', String(b === e.currentTarget))); slideTo(segInd, e.currentTarget); apply(); renderSwatches(); },
    })));
  keepSliding(mode, segInd, checked);
  const swatches = h('div', { class: 'play-swatches', role: 'radiogroup', 'aria-label': 'Accent' });
  const renderSwatches = () => swatches.replaceChildren(...ACCENTS.map(([name, l, d], i) => h('button', {
    class: 'swatch', role: 'radio', 'aria-checked': String(accent === i), 'aria-label': `${name} accent`, title: name,
    style: `--c:${(dark ?? isDark()) ? d : l}`,
    onclick: () => { accent = i; renderSwatches(); apply(); },
  })));
  renderSwatches();
  return h('div', { class: 'home-play-body' }, h('div', { class: 'play-controls' }, mode, swatches), root);
}


// a pill that slides under the selected tab; first placement does not animate
function slideTo(ind, btn, instant = false) {
  if (!btn) return;
  if (instant) ind.classList.add('no-anim');
  ind.style.width = `${btn.offsetWidth}px`;
  ind.style.transform = `translateX(${btn.offsetLeft}px)`;
  ind.classList.add('on');
  if (instant) requestAnimationFrame(() => requestAnimationFrame(() => ind.classList.remove('no-anim')));
}
// keep it under the selection when fonts load or the row reflows
function keepSliding(row, ind, current) {
  const ro = new ResizeObserver(() => { if (!row.isConnected) { ro.disconnect(); return; } const b = current(); if (b) slideTo(ind, b, true); });
  ro.observe(row);
}
// numbers in the strip count up once, when they come into view
function countUp(root) {
  const els = [...root.querySelectorAll('.strip-v')].filter((el) => /\d{2,}/.test(el.textContent));
  if (motionOff() || !els.length) return;
  const io = new IntersectionObserver((es) => {
    if (!es.some((e) => e.isIntersecting)) return;
    io.disconnect();
    for (const el of els) {
      const m = el.textContent.match(/^(\D*)(\d+)(.*)$/);
      if (!m) continue;
      const [, pre, num, post] = m, end = +num, t0 = performance.now(), dur = 1100;
      const step = (now) => {
        const p = Math.min(1, (now - t0) / dur), e = 1 - Math.pow(1 - p, 4);
        el.textContent = pre + Math.round(end * e) + post;
        if (p < 1) requestAnimationFrame(step);
      };
      requestAnimationFrame(step);
    }
  }, { threshold: 0.6 });
  io.observe(root);
}
// a soft light that follows the pointer across a card
function spotlight(root, sel) {
  if (motionOff()) return;
  root.querySelectorAll(sel).forEach((el) => el.addEventListener('pointermove', (e) => {
    const r = el.getBoundingClientRect();
    el.style.setProperty('--mx', `${e.clientX - r.left}px`);
    el.style.setProperty('--my', `${e.clientY - r.top}px`);
  }, { passive: true }));
}

// one line to copy; the full CMake and Git steps live in Getting started
function heroCommand() {
  const cmd = 'git clone https://github.com/yappologistic/CUI';
  const btn = h('button', { class: 'cmd-copy', 'aria-label': 'Copy the command', title: 'Copy', html: I.copy,
    onclick: () => navigator.clipboard?.writeText(cmd).then(() => { btn.innerHTML = I.check; toast('Copied'); setTimeout(() => { btn.innerHTML = I.copy; }, 1400); }) });
  return h('div', { class: 'hero-cmd' }, h('code', {}, h('span', { class: 'cmd-prompt', 'aria-hidden': 'true' }, '$'), cmd), btn);
}

function viewHome() {
  setNav('home');
  const guide = (id) => CAT.guides.find((g) => g.id === id);
  const next = [['getting-started', 'terminal', 'Build CUI, open a window and learn the frame loop.'], ['layout', 'grid', 'Rows, columns, sizes, grids, scrolling and split views.'],
                ['theming', 'contrast', 'Light and dark, eight accents, tokens, fonts and motion.'], ['custom-components', 'pen', 'Draw your own components with the same tools.'],
                ['integration', 'plug', 'Bring your own window, input and renderer.'], ['design', 'ruler', 'The rules every component follows.']];
  const head = (num, title, text, link) => h('div', { class: 'feature-head' },
    h('span', { class: 'feature-num' }, num),
    h('h2', {}, title),
    h('p', {}, text),
    link ? h('a', { class: 'text-link', href: link[0], html: link[1] + I.arrow }) : null);
  const page = h('div', { class: 'page home' },
    h('div', { class: 'hero' },
      h('h1', { class: 'hero-title', 'aria-label': 'Native UI components for C' },
        ...['Native', 'UI', 'components', 'for'].map((w) => h('span', { class: 'hw' }, w)),
        h('span', { class: 'hw hw-logo', html: C_LOGO })),
      h('p', { class: 'lede' }, 'About 210 components, compiled to machine code with no browser or runtime inside. Windows, macOS, Linux and the web, from one C99 codebase.'),
      h('div', { class: 'hero-actions' },
        h('a', { class: 'btn primary', href: '#/guide/getting-started', html: 'Get started' + I.arrow }),
        h('a', { class: 'btn', href: '#/components' }, 'Browse components')),
      heroCommand()),
    showcase(),
    h('ul', { class: 'home-strip', 'aria-label': 'At a glance' },
      STRIP.map(([v, k]) => h('li', {}, h('span', { class: 'strip-v' }, v), h('span', { class: 'strip-k' }, k)))),
    h('section', { class: 'home-feature home-demos home-bento' },
      head('01', 'Everything a desktop app needs', 'Not a box of primitives: complete components, designed together, that look finished without styling. Each tile below is running; click into one and use it.',
        ['#/components', 'Browse all components']),
      h('div', { class: 'bento' }, BENTO.map(bentoTile))),
    h('section', { class: 'home-feature home-split' },
      head('02', 'Your interface is plain C', 'Components are functions that draw themselves and return what happened. Your variables are the state, and the whole screen is described again every frame, so there is nothing to keep in sync.',
        ['#/guide/getting-started/how-immediate-mode-works', 'How immediate mode works']),
      (() => { const w = h('div', {}, h('pre', { class: 'code', html: '<code>' + highlight(FIRST_WINDOW) + '</code>' })); addCopy(w); return w.firstChild; })()),
    h('section', { class: 'home-feature home-demos home-play' },
      head('03', 'Make it yours', 'Light and dark, eight accents, and semantic tokens your own drawing follows too. Try them on a live page of components.',
        ['#/guide/theming', 'Theming']),
      playground()),
    h('section', { class: 'home-feature home-facts' },
      head('04', 'Small, fast and careful', 'Native apps should start instantly and stay out of the way. These are the numbers and rules CUI is held to.',
        ['#/guide/design', 'Design principles']),
      h('ul', { class: 'facts' }, FACTS.map(([t, d]) => h('li', {}, h('h3', {}, t), h('p', {}, d))))),
    h('section', { class: 'home-feature home-demos home-guides' },
      head('05', 'Learn it in an afternoon', 'Six short guides take you from your first window to your own components and your own renderer.',
        ['#/guide/getting-started', 'Start with the first guide']),
      h('ul', { class: 'guide-cards' }, next.filter(([id]) => guide(id)).map(([id, icon, text], i) =>
        h('li', {}, h('a', { class: 'guide-card', href: `#/guide/${id}` },
          h('span', { class: 'gc-top' }, h('span', { class: 'tile-icon', html: I[icon] }), h('span', { class: 'gc-num' }, String(i + 1).padStart(2, '0'))),
          h('span', { class: 'gc-title' }, guide(id).title),
          h('span', { class: 'gc-text' }, text),
          h('span', { class: 'gc-go', html: 'Read' + I.arrow })))))),
    h('section', { class: 'home-cta' },
      ditherArt({ src: ART, focusY: 0.95, gamma: 1.15, reveal: 'view', loop: true, reduced: motionOff }),
      h('div', { class: 'home-cta-body' },
        h('h2', {}, 'Start with one window'),
        h('p', {}, 'Build CUI, open a window and draw your first components in a few minutes.'),
        h('div', { class: 'hero-actions' },
          h('a', { class: 'btn primary', href: '#/guide/getting-started', html: 'Get started' + I.arrow }),
          h('a', { class: 'btn', href: 'https://github.com/yappologistic/CUI', target: '_blank', rel: 'noreferrer' }, 'Source on GitHub')))),
    h('p', { class: 'art-credit' }, 'Photograph: Ansel Adams, ', h('i', {}, 'The Tetons and the Snake River'), ', 1942. U.S. National Archives, public domain.'));
  const shell = h('div', { class: 'home-shell' },
    h('div', { class: 'home-art' }, ditherArt({ src: ART, focusY: 0.3, gamma: 1.25, loop: true, reduced: motionOff })),
    page);
  homeMotion(shell);
  countUp(shell.querySelector('.home-strip'));
  spotlight(shell, '.bento > .tile, .guide-card');
  return shell;
}

const motionOff = () => prefs.reducedMotion || matchMedia('(prefers-reduced-motion: reduce)').matches;

/* Scroll reveals, shared by every page: each element lifts in once as it
 * enters, in the site's one easing. Selectors with '>' are groups (cards, list
 * rows) and stagger within their parent. Nothing moves with reduced motion. */
function revealOnScroll(root, selectors) {
  if (motionOff()) return null;
  const items = [];
  for (const sel of selectors) {
    const seen = new Map();
    root.querySelectorAll(sel).forEach((el) => {
      if (el.classList.contains('reveal')) return;
      el.classList.add('reveal');
      if (sel.includes('>')) {
        const i = seen.get(el.parentNode) || 0;
        seen.set(el.parentNode, i + 1);
        el.style.setProperty('--d', `${Math.min(i, 8) * 45}ms`);
      }
      items.push(el);
    });
  }
  const io = new IntersectionObserver((es) => es.forEach((e) => {
    if (!e.isIntersecting) return;
    e.target.classList.add('in');
    io.unobserve(e.target);
  }), { rootMargin: '0px 0px -6% 0px', threshold: 0 });
  items.forEach((el) => io.observe(el));
  return io;
}

// what lifts in on each kind of page; the page header has its own entrance in CSS
const DOC_REVEAL = ['.doc h2', '.doc pre.code', '.doc table', '.doc .doc-img', '.figure', '.pagers'];
const PAGE_REVEAL = {
  components: ['.section-title', '.grid > .card'],
  guide: DOC_REVEAL,
  c: ['.previews', ...DOC_REVEAL, '.kbd-list', '.opt-block', '.related-grid > .rel-card'],
  showcase: ['.previews > div'],
  accents: ['.accent-grid > *'],
  apps: ['.grid > .card'],
};
function pageMotion(view, kind) {
  const sel = PAGE_REVEAL[kind];
  if (sel) revealOnScroll(view, sel);
}

/* The overview adds two things: the hero art drifts slower than the page and
 * fades, and its sections reveal like every other page. */
function homeMotion(shell) {
  if (motionOff()) { shell.classList.add('still'); return; }
  const io = revealOnScroll(shell, ['.feature-head', '.home-split > :last-child', '.home-show', '.home-cta', '.art-credit',
                                    '.bento > .tile', '.home-play-body', '.facts > li', '.guide-cards > li', '.home-strip > li']);
  const art = shell.querySelector('.home-art .dither');
  let ticking = false;
  const onScroll = () => {
    if (!shell.isConnected) { removeEventListener('scroll', onScroll); io?.disconnect(); return; }
    if (ticking) return;
    ticking = true;
    requestAnimationFrame(() => {
      ticking = false;
      const y = Math.min(scrollY, 900);
      art.style.transform = `translate3d(0, ${(y * 0.35).toFixed(1)}px, 0)`;
      art.style.opacity = String(Math.max(0, 1 - y / 760).toFixed(3));
    });
  };
  addEventListener('scroll', onScroll, { passive: true });
}

/* Two quiet scroll cues on every page: the top bar gains a hairline once the
 * page moves under it, and guide pages show how far through you are. */
const progressBar = h('div', { class: 'read-progress', 'aria-hidden': 'true' });
document.querySelector('.topbar').append(progressBar);
function scrollCues() {
  const y = scrollY;
  document.body.classList.toggle('scrolled', y > 4);
  if (document.body.classList.contains('guide-mode')) {
    const max = document.documentElement.scrollHeight - innerHeight;
    progressBar.style.transform = `scaleX(${max > 0 ? Math.min(1, y / max).toFixed(4) : 0})`;
  }
}
let cueTick = false;
addEventListener('scroll', () => { if (!cueTick) { cueTick = true; requestAnimationFrame(() => { cueTick = false; scrollCues(); }); } }, { passive: true });

function siteFooter() {
  return h('footer', { class: 'site-footer' },
    h('span', {}, 'CUI is MIT licensed. Every preview on this site is drawn by the library itself.'),
    h('nav', {}, h('a', { href: '#/guide/getting-started' }, 'Guide'), h('a', { href: '#/components' }, 'Components'), h('a', { href: '#/changelog' }, 'Changelog'),
      h('a', { href: 'https://github.com/yappologistic/CUI', target: '_blank', rel: 'noreferrer' }, 'Source')));
}

// components added in the latest round; their cards and sidebar rows say "New".
// Keep this to recent additions: drop ids from here once they have settled in.
const NEW = new Set(['status-tag', 'ratio-slider', 'gradient-slider', 'step-slider', 'palette-strip', 'aspect-ratio-picker',
                     'option-field-with-action', 'seed-field', 'grouped-toolbar', 'format-bar', 'live-cursors', 'canvas-controls',
                     'result-card', 'selection-bar', 'voice-waveform', 'voice-recorder', 'voice-note', 'voice-session',
                     'pattern-meter', 'veil', 'dither-spinners', 'status-spinner', 'skeleton-group'].filter((id) => CAT.items[id]));
const newBadge = (id, cls = 'new-badge') => (NEW.has(id) ? h('span', { class: cls, title: 'Recently added' }, 'New') : null);

// the query part of a hash: #/components?q=grid&cat=charts
const hashQuery = () => new URLSearchParams((location.hash.split('?')[1]) || '');
const GITHUB = 'https://github.com/yappologistic/CUI/blob/master/';
const ghLink = (file, line, label) => h('a', { href: `${GITHUB}${file}#L${line}`, target: '_blank', rel: 'noreferrer', title: `Open ${file}:${line} on GitHub` }, label ?? h('code', {}, `${file}:${line}`));

function viewComponents() {
  setNav('components');
  const hq = hashQuery();
  let q = hq.get('q') || '', cat = CAT.categories.some((c) => c.id === hq.get('cat')) ? hq.get('cat') : '';
  const hay = (it) => `${it.title} ${it.summary} ${it.apis.join(' ')}`.toLowerCase();
  const input = h('input', { class: 'filter', type: 'search', placeholder: 'Filter components', 'aria-label': 'Filter components', autocomplete: 'off', value: q });
  const chipAll = h('button', { class: 'cat-chip', 'aria-pressed': 'false', onclick: () => setCat('') });
  const chips = CAT.categories.map((c) => h('button', { class: 'cat-chip', 'data-cat': c.id, 'aria-pressed': 'false', onclick: () => setCat(cat === c.id ? '' : c.id) }));
  const viewSeg = h('div', { class: 'seg view-seg', role: 'radiogroup', 'aria-label': 'Layout' },
    [['Grid', 'grid', I.grid], ['List', 'list', I.list]].map(([label, v, icon]) => h('button', {
      type: 'button', role: 'radio', 'data-v': v, 'aria-checked': String(prefs.view === v), title: `Show as a ${label.toLowerCase()}`, html: icon + `<span>${label}</span>`,
      onclick: () => { prefs.view = v; savePrefs(); settle(); page.dataset.view = v; viewSeg.querySelectorAll('button').forEach((b) => b.setAttribute('aria-checked', String(b.dataset.v === v))); },
    })));
  const empty = h('div', { class: 'empty-state hidden' });
  const page = h('div', { class: 'page comp-index', 'data-view': prefs.view },
    h('h1', {}, 'Components'),
    h('p', { class: 'lede' }, 'Each page shows the component running, the C that draws it, its options and its keyboard support.'),
    h('div', { class: 'comp-tools' },
      h('div', { class: 'filter-wrap comp-filter', html: I.search }, input),
      viewSeg),
    h('div', { class: 'cat-chips', role: 'group', 'aria-label': 'Category' }, chipAll, chips));
  const sections = [];
  for (const c of CAT.categories) {
    const count = h('span', { class: 'count' }, `${c.items.length}`);
    const cards = c.items.map((id) => {
      const it = CAT.items[id];
      const card = h('a', { class: 'card', href: `#/c/${id}` },
        h('div', { class: 'thumb' }, thumb(it.scenes[0])),
        h('div', { class: 'body' }, h('h3', {}, h('span', { class: 'card-title' }, it.title), newBadge(id)),
          h('p', {}, it.summary),
          it.apis[0] ? h('code', { class: 'card-api' }, it.apis[0]) : null));
      card.hay = hay(it);
      return card;
    });
    const sec = h('section', { class: 'comp-sec', 'data-cat': c.id },
      h('div', { class: 'section-title' }, h('h2', {}, c.title), count),
      h('div', { class: 'grid' }, cards));
    sections.push({ c, sec, cards, count });
    page.append(sec);
  }
  page.append(empty);
  function apply() {
    const terms = q.trim().toLowerCase().split(/\s+/).filter(Boolean);
    let shown = 0;
    const per = {};
    for (const { c, sec, cards, count } of sections) {
      let n = 0;
      for (const card of cards) {
        const ok = terms.every((t) => card.hay.includes(t));
        card.classList.toggle('hidden', !ok);
        if (ok) n++;
      }
      per[c.id] = n;
      count.textContent = terms.length ? `${n} of ${c.items.length}` : `${c.items.length}`;
      const vis = n > 0 && (!cat || cat === c.id);
      sec.classList.toggle('hidden', !vis);
      if (vis) shown += n;
    }
    const total = Object.values(per).reduce((a, b) => a + b, 0);
    chipAll.replaceChildren('All', h('span', { class: 'n' }, String(total)));
    chipAll.setAttribute('aria-pressed', String(!cat));
    chips.forEach((b, i) => {
      const c = CAT.categories[i];
      b.replaceChildren(c.title, h('span', { class: 'n' }, String(per[c.id])));
      b.setAttribute('aria-pressed', String(cat === c.id));
      b.classList.toggle('none', per[c.id] === 0);
    });
    const what = [q.trim() && `“${q.trim()}”`, cat && CAT.categories.find((c) => c.id === cat).title].filter(Boolean).join(' in ');
    empty.replaceChildren(h('h3', {}, 'No components match'), h('p', {}, `Nothing matches ${what}.`),
      h('button', { class: 'btn', onclick: () => { q = ''; cat = ''; input.value = ''; apply(); input.focus(); } }, 'Clear the filter'));
    empty.classList.toggle('hidden', shown > 0);
    // keep the address in step, without a new history entry or a re-render
    const p = new URLSearchParams();
    if (q.trim()) p.set('q', q.trim());
    if (cat) p.set('cat', cat);
    const want = '#/components' + (p.toString() ? `?${p}` : '');
    if (location.hash !== want && location.hash.startsWith('#/components')) { history.replaceState(null, '', want); lastRoute = want; }
  }
  // once the reader filters, results show at once rather than lifting in on scroll
  const settle = () => page.querySelectorAll('.reveal:not(.in)').forEach((el) => el.classList.add('in'));
  function setCat(c) { cat = c; settle(); apply(); }
  input.addEventListener('input', () => { q = input.value; settle(); apply(); });
  input.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && (input.value || cat)) { e.preventDefault(); e.stopPropagation(); q = ''; input.value = ''; apply(); }
    if (e.key === 'Enter') { const a = page.querySelector('.comp-sec:not(.hidden) .card:not(.hidden)'); if (a) location.hash = a.getAttribute('href'); }
  });
  apply();
  return page;
}

// the doc text of a component, split into its sections: the leading code is Usage, the rest Behavior
function splitDoc(it) {
  const tmp = h('div', { html: it.html });
  const usage = [];
  while (tmp.firstElementChild && tmp.firstElementChild.matches('pre.code')) usage.push(tmp.firstElementChild), tmp.firstElementChild.remove();
  return { usage, rest: [...tmp.childNodes].filter((n) => n.nodeType === 1 || n.textContent.trim()) };
}

function optionsBlock(it) {
  const many = it.options.length > 1;
  const blocks = it.options.map((o) => h('div', { class: 'opt-block' },
    many ? h('h3', { class: 'opt-fn' }, h('code', {}, o.fn.join(', ')), h('span', { class: 'opt-struct' }, ghLink('include/cui/cui.h', o.line, o.struct))) : null,
    h('div', { class: 'opt-scroll' }, h('table', { class: 'opt-table' },
      h('thead', {}, h('tr', {}, h('th', {}, 'Option'), h('th', {}, 'Type'), h('th', {}, 'Default'), h('th', {}, 'Description'))),
      h('tbody', {}, o.fields.map((f) => h('tr', {},
        h('td', {}, h('code', {}, `.${f.name}`)),
        h('td', { class: 'opt-type' }, h('code', {}, f.type)),
        h('td', { class: 'opt-def' + (f.default ? '' : ' none') }, f.default ? h('code', {}, f.default) : '—'),
        h('td', { class: 'opt-doc', html: f.doc || '' }))))))));
  const fn = it.options[0].fn[0];
  return [...blocks, h('p', { class: 'opt-caption' }, 'Options are named arguments: ', h('code', {}, `${fn}(ui, ..., .field = value)`),
    '. Unset options are zero, so — means 0, false or NULL.',
    many ? null : [' Declared as ', ghLink('include/cui/cui.h', it.options[0].line, it.options[0].struct), ' in cui.h.'])];
}

// the structs a component's functions take and return, so the page says what to fill in and read back
function typesBlock(it) {
  const role = { data: 'You fill this in', result: 'Returned' };
  return it.types.map((t) => h('div', { class: 'opt-block' },
    h('h3', { class: 'opt-fn' }, h('code', {}, t.struct), h('span', { class: 'opt-struct' }, role[t.role] || '', ' · ', ghLink('include/cui/cui.h', t.line, 'cui.h'))),
    h('div', { class: 'opt-scroll' }, h('table', { class: 'opt-table' },
      h('thead', {}, h('tr', {}, h('th', {}, 'Field'), h('th', {}, 'Type'), h('th', {}, 'Description'))),
      h('tbody', {}, t.fields.map((f) => h('tr', {},
        h('td', {}, h('code', {}, f.name)),
        h('td', { class: 'opt-type' }, h('code', {}, f.type)),
        h('td', { class: 'opt-doc', html: f.doc || '' })))))),
    t.enums.map((e) => h('p', { class: 'opt-caption' }, h('code', {}, e.name), ': ', e.values.flatMap((v, i) => (i ? [', ', h('code', {}, v)] : [h('code', {}, v)]))))));
}

function viewItem(id, sec) {
  const it = CAT.items[id];
  if (!it) return viewMissing();
  setNav('components');
  const cat = CAT.categories.find((c) => c.id === it.category);
  const idx = order.indexOf(id);
  const prev = CAT.items[order[idx - 1]], next = CAT.items[order[idx + 1]];
  const docRef = `${it.doc.file}:${it.doc.line}`;
  const main = h('div', { class: 'item-main' },
    h('div', { class: 'crumbs' }, h('a', { href: '#/components' }, 'Components'), '/', h('a', { href: `#/components?cat=${it.category}` }, cat?.title || '')),
    h('div', { class: 'title-row' },
      h('div', {}, h('h1', {}, it.title, newBadge(id, 'new-badge lg')), it.summary ? h('p', { class: 'lede' }, it.summary) : null),
      h('div', { class: 'title-actions' },
        h('button', { class: 'btn sm', html: I.copy + 'Copy page', title: 'Copy this page as text', onclick: () => copy(pageText(it), 'Page copied') }),
        prev ? h('a', { class: 'icon-btn framed', href: `#/c/${prev.id}`, title: `Previous: ${prev.title}`, 'aria-label': `Previous: ${prev.title}`, html: I.left }) : h('span', { class: 'icon-btn framed disabled', html: I.left }),
        next ? h('a', { class: 'icon-btn framed', href: `#/c/${next.id}`, title: `Next: ${next.title}`, 'aria-label': `Next: ${next.title}`, html: I.arrow }) : h('span', { class: 'icon-btn framed disabled', html: I.arrow }))),
    it.apis.length ? h('div', { class: 'api-line' },
      h('span', { class: 'api-label' }, it.apis.length > 1 ? 'Functions' : 'Function'),
      it.apis.slice(0, 8).map((a) => h('button', { class: 'api', title: `Copy ${a}`, onclick: () => copy(a, `${a} copied`) }, a))) : null);
  const secs = [];
  const section = (sid, title, ...kids) => {
    const el = h('section', { class: 'item-sec', id: `s-${sid}` },
      title ? h('h2', { class: 'sec-title' }, h('a', { class: 'anchor', href: `#/c/${id}/${sid}` }, title)) : null, ...kids);
    secs.push({ id: sid, title: title || 'Example', el });
    main.append(el);
    return el;
  };
  // several scenes become variants of one preview, like tabs over a single stage
  const pv = h('div', { class: 'previews' });
  const vt = h('div', { class: 'variant-tabs', role: 'tablist', 'aria-label': 'Examples' });
  const sceneLink = h('span', { class: 'scene-link' });
  const showScene = (s) => {
    const sc = CAT.scenes[s];
    const old = pv.querySelector('.preview');
    if (old) { vt.remove(); const b = blocks.find((x) => x.root === old); if (b) { b.dispose(); blocks.splice(blocks.indexOf(b), 1); } old.remove(); }
    pv.append(previewBlock({ module: 'scenes', content: s, title: s, item: it.title, docRef, codeSource: sc.source, codeFile: sc.file, codeLine: sc.line, leading: vt }));
    sceneLink.replaceChildren(' · Example scene ', ghLink(sc.file, sc.line));
  };
  if (it.scenes.length > 1) {
    it.scenes.forEach((s, i) => vt.append(h('button', { role: 'tab', 'aria-selected': String(i === 0), onclick: (e) => {
      vt.querySelectorAll('button').forEach((b) => b.setAttribute('aria-selected', String(b === e.currentTarget))); showScene(s);
    } }, sceneLabel(s, it))));
  }
  if (it.scenes.length) { showScene(it.scenes[0]); section('example', null, pv); }

  const { usage, rest } = splitDoc(it);
  const docEl = (nodes) => { const d = h('div', { class: 'doc' }); d.append(...nodes); d.querySelectorAll('pre.code code').forEach((c) => { c.innerHTML = highlight(c.textContent); }); addCopy(d); return d; };
  const program = it.program ? h('details', { class: 'program' }, h('summary', {}, 'Complete program'),
    h('p', { class: 'opt-caption' }, 'The example above as a whole program: save it as ', h('code', {}, 'main.c'), ' and build it as in ', h('a', { href: '#/guide/getting-started' }, 'Getting started'), '.'),
    docEl([h('pre', { class: 'code', 'data-lang': 'c' }, h('code', {}, it.program))])) : null;
  if (usage.length) section('usage', 'Usage', docEl(usage), program);
  else if (program) section('usage', 'Usage', program);
  if (rest.length) section('behavior', usage.length ? 'Behavior' : 'Overview', docEl(rest));
  if (it.keyboard?.length) section('keyboard', 'Keyboard', h('ul', { class: 'kbd-list' }, it.keyboard.map((k) => h('li', { html: k }))));
  else if (it.keyboard_note) section('keyboard', 'Keyboard', h('p', { class: 'kbd-note', html: it.keyboard_note }));
  else if (it.display_only) section('keyboard', 'Keyboard', h('p', { class: 'kbd-note' }, 'This component only displays: there is nothing to focus or press.'));
  if (it.options?.length) section('options', 'Options', ...optionsBlock(it));
  if (it.types?.length) section('types', 'Data and results', ...typesBlock(it));
  const rel = (it.related || []).map((r) => CAT.items[r]).filter(Boolean);
  if (rel.length) section('related', 'Related', h('div', { class: 'related-grid' }, rel.map((r) => h('a', { class: 'rel-card', href: `#/c/${r.id}` },
    h('span', { class: 'rel-title' }, r.title, newBadge(r.id)), h('span', { class: 'rel-text' }, r.summary), h('span', { class: 'rel-go', html: I.arrow })))));
  if (it.images.length) {
    const figs = it.images.map((im) => h('figure', { class: 'figure' }, h('img', { 'data-src': im.src, alt: im.alt, loading: 'lazy',
      onerror: (e) => { if (!e.target.dataset.retry) { e.target.dataset.retry = 1; e.target.src = `${im.src}.png`; } } }),
      h('figcaption', { title: `docs/${im.src}.png` }, im.alt, h('span', { class: 'fig-tag' }, 'Screenshot'))));
    const det = h('details', { class: 'shots' }, h('summary', {}, `Screenshots`, h('span', { class: 'n' }, String(figs.length)),
      h('span', { class: 'shots-note' }, 'The static images the docs use. The preview above is the same thing, live.')), figs);
    // theme-aware like the rest of the page; loaded only when opened
    const load = () => det.querySelectorAll('img[data-src]').forEach((img) => { delete img.dataset.retry; img.src = `${img.dataset.src}${isDark() ? '-dark' : ''}.png`; });
    det.addEventListener('toggle', () => { if (det.open) load(); });
    det.reload = () => { if (det.open) load(); };
    section('screenshots', 'Screenshots', det).querySelector('h2').classList.add('hidden');
    secs[secs.length - 1].el.classList.add('item-sec-shots');
  }
  main.append(h('p', { class: 'source-note' }, 'From ', ghLink(it.doc.file, it.doc.line), ' in the repository', sceneLink, '.'));
  const pager = (it2, dir) => pagerLink(it2, dir, `#/c/${it2?.id}`, dir === 'prev' ? '[' : ']');
  main.append(h('nav', { class: 'pagers', 'aria-label': 'Previous and next component' }, pager(prev, 'prev'), pager(next, 'next')));

  const tocSecs = secs.filter((x) => x.id !== 'example' || secs.length > 1);
  const toc = tocSecs.length > 2 ? h('nav', { class: 'toc', 'aria-label': 'On this page' }, h('div', { class: 'toc-title' }, 'On this page'),
    tocSecs.map((x) => h('a', { href: `#/c/${id}/${x.id}`, 'data-sec': x.id }, x.title))) : null;
  const page = h('div', { class: 'page item-page' + (toc ? '' : ' no-toc') }, main, toc);
  if (toc) {
    const io = new IntersectionObserver((es) => {
      for (const e of es) if (e.isIntersecting) {
        const sid = e.target.id.slice(2);
        toc.querySelectorAll('a').forEach((a) => a.toggleAttribute('data-active', a.dataset.sec === sid));
      }
    }, { rootMargin: '-80px 0px -70% 0px' });
    setTimeout(() => secs.forEach((x) => io.observe(x.el)));
  }
  if (sec) setTimeout(() => scrollToSection(sec, false));
  return page;
}

// open a component page section (and the screenshots, if that is where it goes)
function scrollToSection(sec, smooth) {
  const el = document.getElementById(`s-${sec}`);
  if (!el) return false;
  const det = el.querySelector('details');
  if (det && !det.open) det.open = true;
  window.scrollTo({ top: el.getBoundingClientRect().top + scrollY - 72, behavior: smooth && !motionOff() ? 'smooth' : 'auto' });
  return true;
}

function viewGuide(id, sec) {
  const g = CAT.guides.find((x) => x.id === id);
  if (!g) return viewMissing();
  setNav('guide');
  const idx = CAT.guides.indexOf(g);
  const prev = CAT.guides[idx - 1], next = CAT.guides[idx + 1];
  const body = h('article', { class: 'doc guide' },
    h('div', { html: g.intro }),
    g.sections.map((x) => h('section', { id: `s-${x.id}` }, h('h2', {}, h('a', { class: 'anchor', href: `#/guide/${g.id}/${x.id}` }, x.title)), h('div', { html: x.html }))));
  body.querySelectorAll('pre.code code').forEach((c) => { c.innerHTML = highlight(c.textContent); });
  addCopy(body);
  body.querySelectorAll('img[data-src]').forEach((img) => {
    img.src = `${img.dataset.src}${isDark() ? '-dark' : ''}.png`;
    img.onerror = () => { if (!img.dataset.retry) { img.dataset.retry = 1; img.src = `${img.dataset.src}.png`; } };
  });
  const toc = g.sections.length > 2 ? h('nav', { class: 'toc', 'aria-label': 'On this page' }, h('div', { class: 'toc-title' }, 'On this page'),
    g.sections.map((x) => h('a', { href: `#/guide/${g.id}/${x.id}`, 'data-sec': x.id }, x.title))) : null;
  const pager = (it2, dir) => pagerLink(it2, dir, `#/guide/${it2?.id}`, dir === 'prev' ? '[' : ']');
  const page = h('div', { class: 'page guide-page' },
    h('div', { class: 'guide-main' },
      h('div', { class: 'crumbs' }, h('a', { href: '#/guide/getting-started' }, 'Guide'), '/', h('span', {}, g.title)),
      h('h1', {}, g.title),
      body,
      h('p', { class: 'source-note' }, 'From ', h('code', {}, g.file), ' in the repository.'),
      h('nav', { class: 'pagers', 'aria-label': 'Previous and next page' }, pager(prev, 'prev'), pager(next, 'next'))),
    toc);
  if (toc) {
    const io = new IntersectionObserver((es) => {
      for (const e of es) if (e.isIntersecting) {
        const sid = e.target.id.slice(2);
        toc.querySelectorAll('a').forEach((a) => a.toggleAttribute('data-active', a.dataset.sec === sid));
      }
    }, { rootMargin: '-80px 0px -70% 0px' });
    setTimeout(() => body.querySelectorAll('section[id]').forEach((el) => io.observe(el)));
  }
  if (sec) setTimeout(() => { const el = document.getElementById(`s-${sec}`); if (el) window.scrollTo(0, el.getBoundingClientRect().top + scrollY - 72); });
  return page;
}

// previous / next at the foot of a page; the arrow points the way, the shortcut lives in the tooltip
function pagerLink(it2, dir, href, key) {
  if (!it2) return h('span');
  const label = dir === 'prev' ? 'Previous' : 'Next';
  return h('a', { class: 'pager ' + dir, href, title: key ? `${label}: ${it2.title} (${key})` : `${label}: ${it2.title}` },
    h('span', { class: 'pager-dir', html: dir === 'prev' ? I.left + label : label + I.arrow }),
    h('span', { class: 'pager-title' }, it2.title));
}

// a readable name for a scene variant: "toast-demo" under Toast reads "Demo"
function sceneLabel(s, it) {
  const words = s.split('-');
  const t = it.title.toLowerCase();
  const rest = words.filter((w) => !t.includes(w));
  const label = (rest.length ? rest : words).join(' ');
  return label.charAt(0).toUpperCase() + label.slice(1);
}
function pageText(it) {
  const tmp = h('div', { html: it.html });
  tmp.querySelectorAll('pre').forEach((p) => { p.textContent = '\n```c\n' + p.textContent.trim() + '\n```\n'; });
  const types = (it.types || []).map((t) => `\n## ${t.struct} (${t.role === 'result' ? 'returned' : 'you fill this in'})\n\n` +
    t.fields.map((f) => `- ${f.type} ${f.name}${f.doc ? ': ' + h('span', { html: f.doc }).textContent : ''}`).join('\n') + '\n').join('');
  return `# ${it.title}\n\n${it.summary || ''}\n\n${tmp.innerText.trim()}\n${types}`;
}

function viewShowcase() {
  setNav('showcase');
  const page = h('div', { class: 'page' },
    h('h1', {}, 'Showcase'),
    h('p', { class: 'lede' }, 'Larger compositions that put many components together. Each one runs live; open Code to see the C that draws it.'),
    h('div', { class: 'hero-actions' }, h('a', { class: 'btn', href: '#/accents' }, 'See all eight accents')));
  const pv = h('div', { class: 'previews' });
  for (const s of CAT.showcase) {
    const sc = CAT.scenes[s.id];
    pv.append(h('div', {}, h('h2', {}, s.title), h('p', { class: 'lede', style: 'margin:4px 0 12px;font-size:14px' }, s.summary),
      previewBlock({ module: 'scenes', content: s.id, title: s.id, item: s.title, codeSource: sc.source, codeFile: sc.file, codeLine: sc.line })));
  }
  page.append(pv);
  return page;
}

function viewAccents() {
  setNav('showcase');
  const page = h('div', { class: 'page' },
    h('div', { class: 'crumbs' }, h('a', { href: '#/showcase' }, 'Showcase'), '/', h('span', {}, 'Accents')),
    h('h1', {}, 'Accents'),
    h('p', { class: 'lede' }, 'The same controls in each of the eight accents. These keep their own accent when you pick one in the top bar.'));
  const grid = h('div', { class: 'accent-grid' });
  for (const name of CAT.accents) {
    const sc = CAT.scenes[name];
    grid.append(previewBlock({ module: 'scenes', content: name, title: name.replace('accent-', ''), item: 'Accents',
                               codeSource: sc.source, codeFile: sc.file, codeLine: sc.line, pair: false, fixedAccent: true, widths: false }));
  }
  page.append(grid);
  return page;
}

function viewApps() {
  setNav('apps');
  return h('div', { class: 'page' },
    h('h1', {}, 'Apps'),
    h('p', { class: 'lede' }, 'Complete apps from the examples folder, running live. Each one resizes with the window, and the width presets show its smaller layouts.'),
    h('div', { class: 'grid app-grid', style: 'margin-top:28px' }, CAT.apps.map((a) => h('a', { class: 'card', href: `#/app/${a.id}` },
      h('div', { class: 'thumb', style: 'padding:0' }, thumb('', a.id)),
      h('div', { class: 'body' }, h('h3', {}, a.title), h('p', {}, a.summary))))));
}

const WIDTHS = [['Fill', 0], ['1440', 1440], ['1280', 1280], ['1024', 1024], ['768', 768], ['390', 390]];
function viewApp(id) {
  /* an app of the examples, or your own from web/tools/build.sh app */
  const a = CAT.apps.find((x) => x.id === id) || (/^[\w-]+$/.test(id) &&
    { id, title: id, summary: 'Your app, built with web/tools/build.sh app.', source: `user/${id}.c` });
  if (!a) return viewMissing();
  setNav('apps');
  let width = 0;
  const seg = h('div', { class: 'seg', role: 'radiogroup', 'aria-label': 'Viewport width' });
  const page = h('div', { class: 'page wide' });
  const holder = h('div', { class: 'app-view' });
  const build = () => {
    disposeBlocks();
    holder.replaceChildren(previewBlock({ module: a.id, title: a.source, item: a.title, fill: true, codeFile: a.source, codeLine: 1,
      codeSource: () => fetchSource(a.source), fixedWidth: width ? () => width : null }));
    const stage = holder.querySelector('.stage');
    if (width) { stage.style.justifyContent = 'flex-start'; stage.style.overflowX = 'auto'; }
  };
  seg.append(...WIDTHS.map(([label, w]) => h('button', { role: 'radio', 'aria-checked': String(w === width), onclick: (e) => {
    width = w; seg.querySelectorAll('button').forEach((b) => b.setAttribute('aria-checked', String(b === e.target))); build();
  } }, label)));
  page.append(h('div', { class: 'app-head' },
    h('a', { class: 'btn ghost sm', href: '#/apps', html: I.left + 'Apps' }),
    h('h1', {}, a.title), h('p', { class: 'lede' }, a.summary), h('span', { class: 'spacer' }), h('span', { class: 'muted', style: 'font-size:12px' }, 'Width'), seg), holder);
  setTimeout(build);
  return page;
}
const sourceCache = new Map();
async function fetchSource(path) {
  if (!sourceCache.has(path)) sourceCache.set(path, fetch(`src/${path}`).then((r) => (r.ok ? r.text() : '')).catch(() => ''));
  return sourceCache.get(path);
}
function viewMissing() {
  return h('div', { class: 'page' }, h('div', { class: 'empty-state' }, h('h3', {}, 'Nothing here'),
    h('p', {}, 'That page does not exist.'), h('a', { class: 'btn', href: '#/' }, 'Go to the overview')));
}

// the changelog: CHANGELOG.md, one section per release, newest first
// every public function, from the headers: a signature, the header's comment, and the page that explains it
function viewReference(target) {
  setNav('guide');
  const ref = CAT.reference || [];
  const count = ref.reduce((n, s) => n + s.fns.length, 0);
  const filter = h('input', { class: 'ref-filter', type: 'search', placeholder: `Filter ${count} functions`, 'aria-label': 'Filter the functions', autocomplete: 'off' });
  const body = h('article', { class: 'doc guide reference' }, ref.map((s) => h('section', { id: `s-${s.id}`, class: 'ref-sec' },
    h('h2', {}, s.title),
    s.fns.map((f) => h('div', { class: 'ref-fn', id: `f-${f.name}`, 'data-name': f.name },
      h('pre', { class: 'code ref-sig', 'data-lang': 'c' }, h('code', { html: highlight(f.sig) })),
      h('div', { class: 'ref-meta' },
        f.doc ? h('span', { class: 'ref-doc', html: f.doc }) : null,
        f.page ? h('a', { href: `#/c/${f.page}` }, CAT.items[f.page].title) : s.guide ? h('a', { href: `#/guide/${s.guide}` }, `${CAT.guides.find((g) => g.id === s.guide).title} guide`) : null,
        ghLink(s.header, f.line, h('code', {}, `${s.header.split('/').pop()}:${f.line}`))))))));
  const none = h('p', { class: 'empty', hidden: true }, 'No function matches.');
  filter.addEventListener('input', () => {
    const q = filter.value.trim().toLowerCase(); let any = false;
    body.querySelectorAll('.ref-sec').forEach((sec) => {
      let shown = 0;
      sec.querySelectorAll('.ref-fn').forEach((el) => { const on = !q || el.dataset.name.includes(q) || el.textContent.toLowerCase().includes(q); el.hidden = !on; shown += on; });
      sec.hidden = !shown; any = any || shown > 0;
    });
    none.hidden = any;
  });
  const toc = h('nav', { class: 'toc', 'aria-label': 'Sections' }, h('div', { class: 'toc-title' }, 'Sections'),
    ref.map((s) => h('a', { href: `#/reference/${s.id}`, 'data-sec': s.id }, s.title)));
  const page = h('div', { class: 'page guide-page' },
    h('div', { class: 'guide-main' },
      h('h1', {}, 'Function reference'),
      h('p', { class: 'lede' }, `Every public function of CUI ${CAT.version || ''}, in the order of the headers. A name ending in "..." takes named options: see its component's page for them.`),
      filter, none, body),
    toc);
  if (target) setTimeout(() => refScroll(target, false));
  return page;
}
function refScroll(target, smooth) {
  const el = document.getElementById(`f-${target}`) || document.getElementById(`s-${target}`);
  if (!el) return false;
  window.scrollTo({ top: el.getBoundingClientRect().top + scrollY - 84, behavior: smooth && !prefs.reducedMotion ? 'smooth' : 'auto' });
  if (el.classList.contains('ref-fn')) { el.classList.add('ref-hit'); setTimeout(() => el.classList.remove('ref-hit'), 1600); }
  return true;
}
function viewChangelog(sec) {
  setNav('changelog');
  const cl = CAT.changelog || { lede: '', releases: [] };
  const fmt = (d) => { const t = new Date(`${d}T00:00:00`); return isNaN(t) ? d : t.toLocaleDateString(undefined, { year: 'numeric', month: 'long', day: 'numeric' }); };
  const body = h('article', { class: 'doc guide changelog' },
    cl.releases.map((r, i) => h('section', { id: `s-${r.id}`, class: 'release' },
      h('div', { class: 'release-meta' }, h('time', { datetime: r.date }, fmt(r.date)), i === 0 ? h('span', { class: 'release-latest' }, 'Latest') : null),
      h('h2', {}, h('a', { class: 'anchor', href: `#/changelog/${r.id}` }, r.title)),
      h('div', { html: r.html }))));
  body.querySelectorAll('pre.code code').forEach((c) => { c.innerHTML = highlight(c.textContent); });
  const toc = cl.releases.length > 1 ? h('nav', { class: 'toc', 'aria-label': 'Releases' }, h('div', { class: 'toc-title' }, 'Releases'),
    cl.releases.map((r) => h('a', { href: `#/changelog/${r.id}`, 'data-sec': r.id }, r.title))) : null;
  const page = h('div', { class: 'page guide-page' },
    h('div', { class: 'guide-main' },
      h('h1', {}, 'Changelog'),
      cl.lede ? h('p', { class: 'lede' }, cl.lede) : null,
      body,
      h('p', { class: 'source-note' }, 'From ', h('a', { href: 'https://github.com/yappologistic/CUI/blob/master/CHANGELOG.md', target: '_blank', rel: 'noreferrer' }, h('code', {}, 'CHANGELOG.md')), ' in the repository.')),
    toc);
  if (toc) {
    const io = new IntersectionObserver((es) => {
      for (const e of es) if (e.isIntersecting) {
        const sid = e.target.id.slice(2);
        toc.querySelectorAll('a').forEach((a) => a.toggleAttribute('data-active', a.dataset.sec === sid));
      }
    }, { rootMargin: '-80px 0px -70% 0px' });
    setTimeout(() => body.querySelectorAll('section[id]').forEach((el) => io.observe(el)));
  }
  if (sec) setTimeout(() => { const el = document.getElementById(`s-${sec}`); if (el) window.scrollTo(0, el.getBoundingClientRect().top + scrollY - 72); });
  return page;
}

// ---------------------------------------------------------------- router
function route() {
  disposeBlocks();
  const hash = location.hash || '#/';
  lastRoute = hash;
  const [, kind, id] = hash.split('?')[0].match(/^#\/?([^/]*)\/?(.*)$/) || [];
  let view;
  if (!kind) view = viewHome();
  else if (kind === 'c') { const [cid, sec] = id.split('/'); view = viewItem(decodeURIComponent(cid), sec); }
  else if (kind === 'components') view = viewComponents();
  else if (kind === 'guide') { const [gid, sec] = (id || 'getting-started').split('/'); view = viewGuide(decodeURIComponent(gid || 'getting-started'), sec); }
  else if (kind === 'showcase') view = viewShowcase();
  else if (kind === 'accents') view = viewAccents();
  else if (kind === 'apps') view = viewApps();
  else if (kind === 'changelog') view = viewChangelog(id ? decodeURIComponent(id) : '');
  else if (kind === 'reference') view = viewReference(id ? decodeURIComponent(id) : '');
  else if (kind === 'app') view = viewApp(decodeURIComponent(id));
  else view = viewMissing();
  document.body.classList.toggle('app-mode', kind === 'app');
  document.body.classList.toggle('home-mode', !kind || kind === 'home');
  document.body.classList.toggle('guide-mode', kind === 'guide');
  main.replaceChildren(view, ...(kind === 'app' ? [] : [siteFooter()]));
  window.scrollTo(0, 0);
  pageMotion(view, kind);
  scrollCues();
  renderNav($('#filter').value);
  const it = kind === 'c' && CAT.items[decodeURIComponent(id.split('/')[0])];
  const gd = kind === 'guide' && CAT.guides.find((g) => g.id === (id || 'getting-started').split('/')[0]);
  document.title = it ? `${it.title} · CUI` : gd ? `${gd.title} · CUI` : kind === 'components' ? 'Components · CUI' : kind === 'changelog' ? 'Changelog · CUI' : kind === 'showcase' ? 'Showcase · CUI' : kind === 'apps' ? 'Apps · CUI' : kind === 'reference' ? 'Function reference · CUI'
    : kind === 'app' && CAT.apps.find((a) => a.id === id) ? `${CAT.apps.find((a) => a.id === id).title} app · CUI` : kind && kind !== 'home' ? 'CUI' : 'CUI: native UI components for C';
}
let lastRoute = '';
window.addEventListener('hashchange', () => {
  const m = location.hash.match(/^#\/guide\/([^/]+)\/(.+)$/), p = lastRoute.match(/^#\/guide\/([^/]+)/);
  if (m && p && m[1] === p[1]) {
    const el = document.getElementById(`s-${m[2]}`);
    if (el) { window.scrollTo({ top: el.getBoundingClientRect().top + scrollY - 72, behavior: prefs.reducedMotion ? 'auto' : 'smooth' }); lastRoute = location.hash; renderNav($('#filter').value); return; }
  }
  const rm = location.hash.match(/^#\/reference\/(.+)$/);
  if (rm && /^#\/reference/.test(lastRoute) && refScroll(decodeURIComponent(rm[1]), true)) { lastRoute = location.hash; return; }
  const lm = location.hash.match(/^#\/changelog\/(.+)$/);
  if (lm && /^#\/changelog/.test(lastRoute)) {
    const el = document.getElementById(`s-${decodeURIComponent(lm[1])}`);
    if (el) { window.scrollTo({ top: el.getBoundingClientRect().top + scrollY - 72, behavior: prefs.reducedMotion ? 'auto' : 'smooth' }); lastRoute = location.hash; return; }
  }
  // a section of the component page already open: scroll, do not rebuild the live previews
  const cm = location.hash.match(/^#\/c\/([^/]+)(?:\/(.*))?$/), cp = lastRoute.match(/^#\/c\/([^/]+)/);
  if (cm && cp && cm[1] === cp[1]) {
    if (!cm[2]) window.scrollTo({ top: 0, behavior: motionOff() ? 'auto' : 'smooth' });
    else if (!scrollToSection(cm[2], true)) { route(); return; }
    lastRoute = location.hash;
    return;
  }
  route();
});
let resizeTimer = 0;
window.addEventListener('resize', () => { clearTimeout(resizeTimer); resizeTimer = setTimeout(() => blocks.forEach((b) => b.refit()), 120); });

// ---------------------------------------------------------------- settings popover
function openSettings(anchor) {
  closePopover();
  const seg = (label, options, value, set) => {
    const s = h('div', { class: 'seg', role: 'radiogroup', 'aria-label': label });
    s.append(...options.map(([text, v]) => h('button', { role: 'radio', 'aria-checked': String(v === value), onclick: () => {
      set(v); s.querySelectorAll('button').forEach((b, i) => b.setAttribute('aria-checked', String(options[i][1] === v)));
    } }, text)));
    return s;
  };
  const sw = (checked, set) => {
    const input = h('input', { type: 'checkbox', checked: checked ? true : null, onchange: (e) => set(e.target.checked) });
    return h('label', { class: 'toggle' }, input, h('i'));
  };
  const rebuild = () => { savePrefs(); syncSettings(); route(); };
  const pop = h('div', { class: 'popover', role: 'dialog', 'aria-label': 'Settings' },
    h('div', { class: 'row' }, h('span', {}, 'Accent', h('small', {}, 'Click the active one again for each scene’s own')), h('div', { class: 'swatches', id: 'accents', role: 'radiogroup', 'aria-label': 'Accent' })),
    h('div', { class: 'row' }, h('span', {}, 'Theme'), seg('Theme', [['Light', 'light'], ['Dark', 'dark'], ['System', 'system']], prefs.theme, (v) => { prefs.theme = v; savePrefs(); applyTheme(); })),
    h('div', { class: 'row' }, h('span', {}, 'Renderer'), seg('Renderer', [['WebGL2', 'gpu'], ['Software', 'software']], prefs.renderer, (v) => { prefs.renderer = v; rebuild(); })),
    h('div', { class: 'row' }, h('span', {}, 'Motion speed'), seg('Motion speed', [['1×', 1], ['½×', 0.5], ['¼×', 0.25], ['⅒×', 0.1]], prefs.speed, (v) => { prefs.speed = v; savePrefs(); syncSettings(); })),
    h('div', { class: 'row' }, h('span', {}, 'Pixel ratio'), seg('Pixel ratio', [['Auto', 'auto'], ['1×', 1], ['2×', 2]], prefs.pixelRatio, (v) => { prefs.pixelRatio = v; rebuild(); })),
    h('div', { class: 'row' }, h('span', {}, 'Reduced motion'), sw(prefs.reducedMotion, (v) => { prefs.reducedMotion = v; savePrefs(); syncSettings(); allLive().forEach((p) => p.setReducedMotion(v)); })),
    h('div', { class: 'row' }, h('span', {}, 'Light and dark side by side'), sw(prefs.pair, (v) => { prefs.pair = v; rebuild(); })));
  document.body.append(pop);
  renderAccents();
  const r = anchor.getBoundingClientRect();
  pop.style.top = `${r.bottom + 8}px`;
  pop.style.right = `${Math.max(8, innerWidth - r.right)}px`;
  setTimeout(() => document.addEventListener('pointerdown', outside));
  function outside(e) { if (!pop.contains(e.target) && e.target !== anchor) closePopover(); }
  openSettings.close = () => { pop.remove(); document.removeEventListener('pointerdown', outside); };
}
function closePopover() { openSettings.close?.(); openSettings.close = null; }
$('#settings-open').addEventListener('click', (e) => (openSettings.close ? closePopover() : openSettings(e.currentTarget)));
$('#theme-toggle').addEventListener('click', () => { prefs.theme = isDark() ? 'light' : 'dark'; savePrefs(); applyTheme(); });

// ---------------------------------------------------------------- search palette
// Everything the palette searches, built once. `hay` is what a page is about (its title,
// description and functions) and ranks first; `body` is the rest of its text, so a word
// that only appears in a guide's prose or an option's name still finds its page.
let searchEntries = null;
function plainText(html) { const d = document.createElement('div'); d.innerHTML = html || ''; return d.textContent.replace(/\s+/g, ' ').trim(); }
function buildSearch() {
  const optText = (it) => [...it.options.flatMap((o) => [o.struct, ...o.fields.flatMap((f) => [f.name, plainText(f.doc)])]),
    ...(it.types || []).flatMap((t) => [t.struct, ...t.fields.flatMap((f) => [f.name, plainText(f.doc)]), ...t.enums.flatMap((e) => [e.name, ...e.values])])].join(' ');
  const E = (href, title, sub, group, hay, body, minor) => ({ href, title, sub, group, minor, hay: `${title} ${hay}`.toLowerCase(), body, lbody: body.toLowerCase() });
  return [
    ...CAT.guides.flatMap((g) => [E(`#/guide/${g.id}`, g.title, g.lede, 'Guide', g.lede, plainText(g.intro)),
      ...g.sections.map((x) => E(`#/guide/${g.id}/${x.id}`, x.title, g.title, 'Guide', g.title, plainText(x.html), true))]),
    ...order.map((id) => { const it = CAT.items[id]; return E(`#/c/${id}`, it.title, it.summary, CAT.categories.find((c) => c.id === it.category).title,
      `${it.summary} ${it.apis.join(' ')} ${it.scenes.join(' ')}`, `${plainText(it.html)} ${plainText(it.keyboard.join(' '))} ${plainText(it.keyboard_note)} ${optText(it)}`); }),
    ...CAT.showcase.map((x) => E('#/showcase', x.title, x.summary, 'Showcase', `${x.summary} ${x.id}`, '')),
    ...CAT.apps.map((a) => E(`#/app/${a.id}`, `${a.title} app`, a.summary, 'Apps', `${a.summary} app`, '')),
    ...(CAT.reference || []).flatMap((s) => s.fns.map((f) => E(`#/reference/${f.name}`, f.name, plainText(f.doc) || f.sig, 'Function', f.sig, plainText(f.doc), true))),
    ...CAT.changelog.releases.map((r) => E(`#/changelog/${r.id}`, `Changelog: ${r.title}`, r.date, 'Changelog', '', plainText(r.html), true)),
  ];
}
// the words around the first place a term appears, to show why a page matched
function snippet(e, terms) {
  const at = Math.min(...terms.map((t) => e.lbody.indexOf(t)).filter((i) => i >= 0));
  if (!isFinite(at)) return e.sub;
  let a = Math.max(0, at - 48), b = Math.min(e.body.length, at + 72);
  if (a > 0) a = e.body.indexOf(' ', a) + 1;
  if (b < e.body.length) b = e.body.lastIndexOf(' ', b);
  return `${a > 0 ? '… ' : ''}${e.body.slice(a, b)}${b < e.body.length ? ' …' : ''}`;
}
function openSearch() {
  if ($('.scrim')) return;
  const entries = searchEntries || (searchEntries = buildSearch());
  const input = h('input', { placeholder: 'Search the guide, components and functions', 'aria-label': 'Search' });
  const list = h('div', { class: 'palette-list', role: 'listbox' });
  let results = [], sel = 0;
  const render = () => {
    const q = input.value.trim().toLowerCase().split(/\s+/).filter(Boolean);
    // a page that is about the words comes before a page that only mentions them
    const score = (e) => {
      const title = e.title.toLowerCase(), phrase = q.join(' ');
      if (q.every((t) => e.hay.includes(t))) return (title === phrase ? 400 : title.startsWith(q[0]) ? 300 : q.every((t) => title.includes(t)) ? 250 : 200) - (e.minor ? 20 : 0);
      if (q.every((t) => e.hay.includes(t) || e.lbody.includes(t))) return (e.lbody.includes(phrase) ? 100 : 50) + 30 * q.filter((t) => title.includes(t)).length - (e.minor ? 0 : 5);
      return 0;
    };
    results = entries.filter((e) => (q.length || !e.minor)).map((e) => ({ e, s: q.length ? score(e) : 1 })).filter((r) => r.s > 0);
    if (q.length) results.sort((a, b) => b.s - a.s);
    results = results.slice(0, 60).map((r) => (r.s < 200 && q.length ? { ...r.e, sub: snippet(r.e, q) } : r.e));
    sel = Math.min(sel, Math.max(0, results.length - 1));
    list.replaceChildren();
    let group = null;
    results.forEach((r, i) => {
      if (r.group !== group && !q.length) { group = r.group; list.append(h('div', { class: 'palette-group' }, group)); }
      list.append(h('div', { class: 'palette-item', role: 'option', 'aria-selected': String(i === sel), onclick: () => go(i), onpointermove: () => { if (sel !== i) { sel = i; mark(); } } },
        h('div', { class: 't' }, h('div', {}, r.title), r.sub ? h('div', {}, r.sub) : null), q.length ? h('span', { class: 'c' }, r.group) : null));
    });
    if (!results.length) list.append(h('div', { class: 'empty-state', style: 'padding:32px' }, 'No matches'));
  };
  const mark = () => list.querySelectorAll('.palette-item').forEach((el, i) => { el.setAttribute('aria-selected', String(i === sel)); if (i === sel) el.scrollIntoView({ block: 'nearest' }); });
  const close = () => { scrim.remove(); };
  const go = (i) => { const r = results[i]; close(); if (r) location.hash = r.href; };
  input.addEventListener('input', () => { sel = 0; render(); });
  input.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowDown') { sel = Math.min(sel + 1, results.length - 1); mark(); e.preventDefault(); }
    else if (e.key === 'ArrowUp') { sel = Math.max(sel - 1, 0); mark(); e.preventDefault(); }
    else if (e.key === 'Enter') go(sel);
    else if (e.key === 'Escape') close();
  });
  const scrim = h('div', { class: 'scrim', onpointerdown: (e) => { if (e.target === scrim) close(); } },
    h('div', { class: 'palette', role: 'dialog', 'aria-label': 'Search' },
      h('div', { class: 'palette-input', html: I.search }, input), list,
      h('div', { class: 'palette-foot' }, h('span', {}, h('kbd', {}, '↑↓'), ' move'), h('span', {}, h('kbd', {}, 'Enter'), ' open'), h('span', {}, h('kbd', {}, 'Esc'), ' close'))));
  scrim.querySelector('.palette-input svg').style.cssText = '';
  document.body.append(scrim);
  render();
  input.focus();
}
$('#search-open').addEventListener('click', openSearch);
document.addEventListener('keydown', (e) => {
  const t = e.target;
  if (t instanceof HTMLCanvasElement || t.classList?.contains('ime-proxy')) return;   // the preview has the keyboard
  const typing = t instanceof HTMLInputElement || t instanceof HTMLTextAreaElement;
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); openSearch(); return; }
  if (typing) return;
  if (e.key === '/') { e.preventDefault(); openSearch(); }
  const m = location.hash.match(/^#\/c\/([^/]+)/);
  if (m && (e.key === '[' || e.key === ']')) {
    const i = order.indexOf(decodeURIComponent(m[1])) + (e.key === ']' ? 1 : -1);
    if (order[i]) location.hash = `#/c/${order[i]}`;
  }
  const gm = location.hash.match(/^#\/guide\/([^/]+)/);
  if (gm && (e.key === '[' || e.key === ']')) {
    const i = CAT.guides.findIndex((g) => g.id === gm[1]) + (e.key === ']' ? 1 : -1);
    if (CAT.guides[i]) location.hash = `#/guide/${CAT.guides[i].id}`;
  }
  if (e.key === 'Escape') closePopover();
});

// ---------------------------------------------------------------- start
window.cuiDebug = { allLive, blocks: () => blocks, settings };
applyTheme();
applyWrap();
renderNav($('#filter').value);
route();
