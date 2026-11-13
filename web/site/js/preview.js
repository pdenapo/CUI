// A live CUI canvas: one wasm instance, one cui_ctx, real input, drawn with
// WebGL2 (the port of the OpenGL renderer) or the software rasterizer.

import { loadModule } from './runtime.js';
import { GLRenderer } from './gl.js';

// cui_key values (include/cui/cui.h)
const MODKEYS = new Set(['Shift', 'Control', 'Alt', 'Meta']);
const KEYS = { Tab: 1, Enter: 2, NumpadEnter: 2, Escape: 3, Backspace: 4, Delete: 5, Space: 6,
  ArrowLeft: 7, ArrowRight: 8, ArrowUp: 9, ArrowDown: 10, Home: 11, End: 12, PageUp: 13, PageDown: 14,
  Minus: 51, Equal: 52, Comma: 53, Period: 54, Slash: 55 };
for (let i = 0; i < 26; i++) KEYS['Key' + String.fromCharCode(65 + i)] = 15 + i;
for (let i = 0; i < 10; i++) { KEYS['Digit' + i] = 41 + i; KEYS['Numpad' + i] = 41 + i; }
for (let i = 1; i <= 12; i++) KEYS['F' + i] = 55 + i;
const CURSORS = ['default', 'pointer', 'text', 'ew-resize', 'ns-resize', 'move', 'not-allowed'];
// browser shortcuts that keep working while a preview has focus
const PASS = new Set(['F5', 'F11', 'F12']);
const PASS_CTRL = new Set(['KeyR', 'KeyT', 'KeyW', 'KeyL', 'Tab', 'PageUp', 'PageDown']);

export const settings = {
  renderer: 'gpu',      // 'gpu' | 'software'
  speed: 1,             // time scale for inspecting motion
  reducedMotion: false,
  pixelRatio: 'auto',   // 'auto' | 1 | 2
  dark: false,
  accent: 0,
};
const ratio = () => settings.pixelRatio === 'auto' ? (window.devicePixelRatio || 1) : settings.pixelRatio;
// A scene narrower than its natural width is laid out at the width it gets,
// down to its minimum content width (and never below this); only then is it
// scaled down.
const MIN_LAYOUT_W = 280;

// ---- shared frame loop ------------------------------------------------------
const live = new Set();
let raf = 0, gpuOwnerSeq = 1;
function schedule() { if (!raf) raf = requestAnimationFrame(tick); }
function tick(now) {
  raf = 0;
  let again = false;
  for (const p of live) if (p.step(now)) again = true;
  if (again) schedule();
}
// wakeups (caret blink, tooltip delays) without a busy loop
setInterval(() => { for (const p of live) if (p.wakeAt && performance.now() >= p.wakeAt) { schedule(); break; } }, 50);

let activeGL = 0;
const GL_BUDGET = 12;   // browsers cap live WebGL contexts at about 16

export class Preview {
  // opts: { module: 'scenes' | app name, content: name, width, height, fill: bool, dark?: bool, onStats }
  constructor(opts) {
    this.opts = opts;
    this.el = document.createElement('div');
    this.el.className = 'canvas-wrap';
    this.canvas = document.createElement('canvas');
    this.canvas.tabIndex = 0;
    this.canvas.setAttribute('aria-label', `Live preview: ${opts.content}`);
    // Browsers open the IME candidate window only at a focused DOM input, so
    // while a text field in the preview is being edited, the keyboard goes to
    // this hidden text area, kept at the field's caret (placeIme). It forwards
    // keys like the canvas does, and committed compositions as text.
    this.ime = document.createElement('textarea');
    this.ime.className = 'ime-proxy';
    this.ime.tabIndex = -1;
    this.ime.setAttribute('aria-label', `Text input for the live preview: ${opts.content}`);
    for (const [k, v] of [['autocomplete', 'off'], ['autocorrect', 'off'], ['autocapitalize', 'off'], ['spellcheck', 'false']]) this.ime.setAttribute(k, v);
    this.el.append(this.canvas, this.ime);
    this.composing = false;
    this.dirty = true;
    this.last = 0;
    this.visible = true;
    this.frames = 0;
    this.stats = { build: 0, paint: 0, tris: 0, fps: 0 };
    this.fpsWindow = [];
    this.buttons = 0;
    this.disposed = false;
  }

  get dark() { return this.opts.dark ?? settings.dark; }

  async start() {
    const o = this.opts;
    this.mod = await loadModule(`wasm/${o.module}.wasm`, { clipboardSet: (t) => navigator.clipboard?.writeText(t).catch(() => {}) });
    if (this.disposed) return;
    const x = this.mod.exports;
    const c = o.content ? this.mod.find(o.content) : this.mod.contents[0];
    if (!c) throw new Error(`no content "${o.content}" in ${o.module}.wasm`);
    this.content = c;
    this.natural = { w: c.width, h: c.height };
    this.w = o.width || c.width;
    this.h = o.height || c.height;
    const first = this.autoWidth();          // a first guess; refined once the scene can be measured
    if (first) this.w = first;
    const useGL = settings.renderer === 'gpu' && activeGL < GL_BUDGET;
    if (useGL) {
      try { this.gl = new GLRenderer(this.canvas); activeGL++; } catch (e) { console.warn(e); this.gl = null; }
    }
    if (!this.gl) this.ctx2d = this.canvas.getContext('2d', { alpha: false });
    const owner = this.gl ? gpuOwnerSeq++ : 0;
    if (owner) this.mod.registerOwner(owner, this.gl);
    this.scale = this.fitScale();
    const dark = c.theme === 1 && o.module === 'scenes' ? 1 : (this.dark ? 1 : 0);
    const accent = o.ownAccent ? (c.accent >= 0 ? c.accent : 0) : o.accent ?? (settings.accentTouched ? settings.accent : (c.accent >= 0 ? c.accent : settings.accent));
    this.inst = x.inst_new(c.index, this.w, this.h, this.scale, dark, accent, owner);
    x.inst_set_reduced_motion(this.inst, settings.reducedMotion ? 1 : 0);
    this.forcedDark = c.theme === 1 && o.module === 'scenes';
    if (this.autoWidth()) this.layoutAuto(false);
    this.sizeCanvas();
    this.bind();
    this.watchContainer();
    // paint the warm-up frame right away, then go live
    this.paint();
    live.add(this);
    this.observe();
    schedule();
  }

  rendererName() { return this.gl ? 'WebGL2' : 'Software'; }

  // width available inside the container (its content box)
  availWidth() {
    const box = this.opts.container;
    if (!box) return 0;
    const cs = getComputedStyle(box);
    const pad = (parseFloat(cs.paddingLeft) || 0) + (parseFloat(cs.paddingRight) || 0);
    return Math.floor(box.clientWidth - (pad || (this.opts.padding ?? 56)));
  }

  // The logical width a scene should lay out at in its container, or 0 to
  // keep the current size (fill mode, an explicit width, a size set by hand,
  // or a container wide enough for the natural width).
  autoWidth() {
    const o = this.opts, nat = this.natural;
    if (o.fill || o.width || this.userSized || !nat) return 0;
    const avail = this.availWidth();
    if (!(avail > 0) || avail >= nat.w) return nat.w;
    const x = this.mod && this.inst ? this.mod.exports : null;
    const minContent = x && x.inst_min_width ? Math.ceil(x.inst_min_width(this.inst)) : 0;
    const floor = Math.min(nat.w, Math.max(MIN_LAYOUT_W, minContent));
    return Math.max(avail, floor);
  }

  // Lays the scene out at its auto width and grows its height to the content
  // there (wrapped rows, stacked cards and open popovers get taller).
  layoutAuto(redraw = true) {
    const x = this.mod.exports, nat = this.natural;
    let w = this.autoWidth();
    if (!w) return;
    let h = nat.h;
    if (w < nat.w && x.inst_content_height) {
      // Start at the width the container has, not at the minimum measured at the natural size:
      // toolbars, path bars and inspectors collapse or stack once they see a narrower width,
      // and only then report how narrow they can go. A few frames let them settle.
      const avail = this.availWidth();
      if (avail > 0 && avail < w) w = Math.max(avail, MIN_LAYOUT_W);
      for (let pass = 0; pass < 3; pass++) {
        x.inst_resize(this.inst, w, h, ratio());
        for (let f = 0; f < 3; f++) x.inst_frame(this.inst, 0, 0);
        const w2 = this.autoWidth();          // the minimum content width is known now
        const h2 = Math.max(nat.h, Math.ceil(x.inst_content_height(this.inst)));
        if (w2 === w && h2 === h) break;
        w = w2; h = h2;
      }
    }
    this.w = w; this.h = h;
    this.scale = this.fitScale();
    x.inst_resize(this.inst, this.w, this.h, this.scale);
    if (redraw) { this.sizeCanvas(); this.poke(); this.opts.onStats?.(this); }
  }

  watchContainer() {
    const box = this.opts.container;
    if (!box || this.opts.fill || !('ResizeObserver' in window)) return;
    let lastW = box.clientWidth, pending = 0;
    this.ro = new ResizeObserver(() => {
      if (box.clientWidth === lastW) return;
      lastW = box.clientWidth;
      cancelAnimationFrame(pending);
      pending = requestAnimationFrame(() => { if (this.inst) this.refit(); });
    });
    this.ro.observe(box);
  }

  // logical size shown on screen: scale down only what is still wider than
  // the container after the scene was laid out at the width it has
  fitScale() {
    let fit = 1;
    if (!this.opts.fill) {
      const avail = this.availWidth();
      if (avail > 0 && this.w > avail) fit = avail / this.w;
    }
    this.fit = fit;
    return ratio() * fit;
  }

  sizeCanvas() {
    const x = this.mod.exports;
    this.canvas.style.width = `${this.w * this.fit}px`;
    this.canvas.style.height = `${this.h * this.fit}px`;
    this.canvas.dataset.fit = this.fit < 1 ? this.fit.toFixed(3) : '1';   // read by the layout checks
    const W = Math.round(this.w * this.scale), H = Math.round(this.h * this.scale);
    if (this.canvas.width !== W || this.canvas.height !== H) { this.canvas.width = W; this.canvas.height = H; }
  }

  resize(w, h) {
    if (!this.inst) return;
    // a size set by hand sticks; the natural size goes back to following the container
    const nat = this.natural;
    this.userSized = !this.opts.fill && !(Math.round(w) === nat.w && Math.round(h) === nat.h);
    if (!this.opts.fill && !this.userSized && this.autoWidth()) { this.layoutAuto(); return; }
    this.w = Math.max(80, Math.round(w));
    this.h = Math.max(60, Math.round(h));
    this.scale = this.fitScale();
    this.mod.exports.inst_resize(this.inst, this.w, this.h, this.scale);
    this.sizeCanvas();
    this.poke();
    this.opts.onStats?.(this);
  }
  // A device width preset: lays the scene out at `w` and grows its height to
  // the content there, like the corner grip but in one step. 0 goes back to
  // the natural size, which follows the container again.
  setWidth(w) {
    if (!this.inst) return;
    const x = this.mod.exports, nat = this.natural;
    if (!w) { this.resize(nat.w, nat.h); return; }
    let h = nat.h;
    if (x.inst_content_height) {
      for (let pass = 0; pass < 3; pass++) {
        x.inst_resize(this.inst, w, h, ratio());
        x.inst_frame(this.inst, 0, 0);
        const h2 = Math.max(nat.h, Math.ceil(x.inst_content_height(this.inst)));
        if (h2 === h) break;
        h = h2;
      }
    }
    this.resize(w, h);
  }
  refit() {
    if (!this.inst) return;
    if (!this.opts.fill && !this.userSized && this.autoWidth()) this.layoutAuto();
    else this.resize(this.w, this.h);
  }

  poke() { this.dirty = true; schedule(); }

  step(now) {
    if (!this.inst || !this.visible || this.disposed) return false;
    const x = this.mod.exports;
    const wake = this.wakeAt && now >= this.wakeAt;
    if (!this.dirty && !this.animating && !wake) return false;
    const dt = this.last ? Math.min((now - this.last) / 1000, 0.25) : 1 / 60;
    this.last = now;
    this.dirty = false;
    x.inst_frame(this.inst, dt * settings.speed, this.gl ? 0 : 1);
    // smoothed: the browser clock is coarse at this scale
    this.stats.build = this.frames ? this.stats.build * 0.8 + x.inst_build_ms(this.inst) * 0.2 : x.inst_build_ms(this.inst);
    this.paint(true);
    this.animating = !!x.inst_needs_redraw(this.inst);
    const w = x.inst_next_wakeup(this.inst);
    this.wakeAt = w >= 0 ? now + w * 1000 / settings.speed : 0;
    const cur = CURSORS[x.inst_cursor(this.inst)] || 'default';
    if (this.canvas.style.cursor !== cur) this.canvas.style.cursor = cur;
    this.placeIme();
    this.fpsWindow.push(now);
    while (this.fpsWindow.length && now - this.fpsWindow[0] > 1000) this.fpsWindow.shift();
    this.frames++;
    if (this.opts.onStats && (this.frames % 4 === 0 || !this.animating)) this.opts.onStats(this);
    if (!this.animating) this.last = 0;   // next input starts a fresh dt
    return this.animating;
  }

  paint(built) {
    const x = this.mod.exports;
    const t = performance.now();
    if (this.gl) {
      this.gl.draw(this.mod, this.inst);
      this.stats.paint = this.stats.paint * 0.8 + (performance.now() - t) * 0.2;
    } else {
      if (!built) x.inst_paint(this.inst);
      const px = this.mod.pixels(this.inst);
      if (this.canvas.width !== px.w || this.canvas.height !== px.h) { this.canvas.width = px.w; this.canvas.height = px.h; }
      this.ctx2d.putImageData(new ImageData(px.data, px.w, px.h), 0, 0);
      this.stats.paint = x.inst_paint_ms(this.inst);
    }
    this.stats.tris = x.inst_triangles ? (this.gl ? Math.round(x.dd_index_count(x.inst_draw_data(this.inst)) / 3) : x.inst_triangles(this.inst)) : 0;
    this.stats.fps = this.fpsWindow.length;
  }

  // ---- input --------------------------------------------------------------
  hasKeyboard() { const a = document.activeElement; return !!a && (a === this.canvas || a === this.ime); }

  // Moves the IME anchor to the caret of the field being edited and gives it
  // the keyboard; gives the keyboard back to the canvas when no field is.
  placeIme() {
    const x = this.mod.exports, ime = this.ime, c = this.canvas;
    const p = x.inst_text_rect ? x.inst_text_rect(this.inst) : 0;
    const active = document.activeElement;
    if (!p) {
      if (active === ime && !this.composing) c.focus({ preventScroll: true });
      return;
    }
    const [rx, ry, , rh] = new Float32Array(this.mod.memory().buffer, p, 4);
    const k = this.fit;                       // CSS pixels per point
    const h = Math.max(1, rh * k);
    Object.assign(ime.style, { left: `${rx * k}px`, top: `${ry * k}px`, height: `${h}px`, lineHeight: `${h}px`,
                               fontSize: `${Math.max(10, Math.round(h * 0.7))}px` });
    if (active === c) ime.focus({ preventScroll: true });
  }

  pos(e) {
    const r = this.canvas.getBoundingClientRect();
    return [(e.clientX - r.left) * (this.w / r.width), (e.clientY - r.top) * (this.h / r.height)];
  }
  bind() {
    const c = this.canvas, ime = this.ime, x = this.mod.exports, inst = this.inst;
    const on = (t, f, o, el = c) => { el.addEventListener(t, f, o); (this.offs ||= []).push(() => el.removeEventListener(t, f, o)); };
    const onKeys = (t, f, o) => { on(t, f, o); on(t, f, o, ime); };   // the keyboard is on the canvas or the IME anchor
    let released = false;
    const takeOver = () => { if (!released) { released = true; if (x.inst_release_scripted(inst)) this.poke(); } };
    for (const t of ['pointerenter', 'pointerdown', 'wheel']) on(t, takeOver, { capture: true });
    onKeys('keydown', takeOver, { capture: true });
    // Touch: on a component page the page itself scrolls over a preview
    // (pan-y); in a full app a vertical swipe scrolls the app instead of
    // dragging, and a sideways one drags (sliders, carousels).
    c.style.touchAction = this.opts.fill ? 'none' : 'pan-y';
    let touch = null;
    const cancelPress = () => {
      x.inst_mouse_move(inst, -10000, -10000);
      for (let b = 0; b < 3; b++) if (this.buttons & (1 << b)) x.inst_mouse_button(inst, b, 0);
      this.buttons = 0;
      x.inst_frame(inst, 0, 0);
    };
    on('pointerdown', (e) => { if (e.pointerType === 'touch') touch = { x: e.clientX, y: e.clientY, mode: '' }; }, { capture: true });
    on('pointermove', (e) => {
      if (!touch || e.pointerType !== 'touch' || !this.opts.fill) return;
      const dx = e.clientX - touch.x, dy = e.clientY - touch.y;
      if (!touch.mode && Math.hypot(dx, dy) > 8) {
        touch.mode = Math.abs(dy) > Math.abs(dx) ? 'scroll' : 'drag';
        if (touch.mode === 'scroll') cancelPress();
      }
      if (touch.mode === 'scroll') {
        const [mx, my] = this.pos(e);
        x.inst_mouse_move(inst, mx, my);
        const k = this.h / c.getBoundingClientRect().height / 44;   /* pixels to wheel lines */
        x.inst_scroll(inst, (e.clientX - touch.x) * k, (e.clientY - touch.y) * k);
        touch.x = e.clientX; touch.y = e.clientY;
        this.poke();
        e.stopImmediatePropagation();
      }
    }, { capture: true });
    on('pointercancel', () => { if (touch) cancelPress(); touch = null; }, { capture: true });
    on('pointerup', (e) => { if (touch && touch.mode === 'scroll') { x.inst_mouse_leave(inst); e.stopImmediatePropagation(); this.poke(); } if (e.pointerType === 'touch') touch = null; }, { capture: true });
    on('pointermove', (e) => {
      const events = e.getCoalescedEvents ? e.getCoalescedEvents() : [e];
      const last = events[events.length - 1] || e;
      const [mx, my] = this.pos(last);
      x.inst_mouse_move(inst, mx, my);
      this.poke();
    });
    on('pointerdown', (e) => {
      if (e.pointerType === 'mouse' && e.button > 2) return;
      if (document.activeElement !== ime) c.focus({ preventScroll: true });   // clicks inside the field being edited keep the IME anchor
      try { c.setPointerCapture(e.pointerId); } catch (err) { /* synthetic pointers have no capture */ }
      const [mx, my] = this.pos(e);
      x.inst_mouse_move(inst, mx, my);
      const b = e.button === 2 ? 1 : e.button === 1 ? 2 : 0;
      this.buttons |= 1 << b;
      x.inst_mouse_button(inst, b, 1);
      this.poke();
      e.preventDefault();
    });
    const up = (e) => {
      const b = e.button === 2 ? 1 : e.button === 1 ? 2 : 0;
      if (!(this.buttons & (1 << b))) return;
      this.buttons &= ~(1 << b);
      const [mx, my] = this.pos(e);
      x.inst_mouse_move(inst, mx, my);
      x.inst_mouse_button(inst, b, 0);
      this.poke();
    };
    on('pointerup', up);
    on('pointercancel', up);
    on('pointerleave', () => { if (!this.buttons) { x.inst_mouse_leave(inst); this.poke(); } });
    on('contextmenu', (e) => e.preventDefault());
    on('wheel', (e) => {
      let k = e.deltaMode === 1 ? 1 : e.deltaMode === 2 ? 10 : 1 / 100;
      let dx = -e.deltaX * k, dy = -e.deltaY * k;
      if (e.shiftKey && !e.deltaX) { dx = dy; dy = 0; }
      const [mx, my] = this.pos(e);
      x.inst_mouse_move(inst, mx, my);
      x.inst_scroll(inst, dx, dy);
      // build now: if nothing in the preview scrolls, let the page scroll instead
      x.inst_frame(inst, 0, 0);
      if (this.opts.fill || x.inst_scroll_consumed(inst)) e.preventDefault();
      this.poke();
    }, { passive: false });
    onKeys('keydown', (e) => {
      if (e.isComposing || e.keyCode === 229) return;   // a key for the input method
      if (PASS.has(e.code) || ((e.ctrlKey || e.metaKey) && PASS_CTRL.has(e.code)) || (e.ctrlKey && e.shiftKey && e.code === 'KeyI')) return;
      const mods = (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0) | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyV') { this.pasteMods = mods; return; }   // paste event follows
      const key = KEYS[e.code];
      if (key) x.inst_key(inst, key, 1, mods);
      else if (MODKEYS.has(e.key)) x.inst_key(inst, 0, 1, mods);   // a modifier on its own
      if (e.key.length >= 1 && [...e.key].length === 1 && !e.ctrlKey && !e.metaKey) this.mod.text(inst, e.key);
      if (key || e.key.length === 1) e.preventDefault();
      this.poke();
    });
    onKeys('keyup', (e) => {
      if (e.isComposing) return;
      const key = KEYS[e.code];
      const mods = (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0) | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
      if (!key) { if (MODKEYS.has(e.key)) { x.inst_key(inst, 0, 0, mods); this.poke(); } return; }   // releasing Ctrl after a shortcut must register
      x.inst_key(inst, key, 0, mods);
      this.poke();
    });
    onKeys('paste', (e) => {
      const text = e.clipboardData?.getData('text/plain') ?? '';
      this.mod.setClipboardCache(text);
      x.inst_key(inst, KEYS.KeyV, 1, this.pasteMods || 2);
      x.inst_key(inst, KEYS.KeyV, 0, this.pasteMods || 2);
      e.preventDefault();
      this.poke();
    });
    // compositions: only the committed text goes to CUI, which draws no preedit;
    // the anchor shows it while it is being composed
    const fitText = () => { ime.style.width = '1px'; ime.style.width = `${ime.scrollWidth + 2}px`; };
    on('compositionstart', () => { this.composing = true; ime.classList.add('composing'); }, undefined, ime);
    on('compositionend', (e) => {
      this.composing = false;
      ime.classList.remove('composing');
      ime.value = '';
      ime.style.width = '';
      if (e.data) this.mod.text(inst, e.data);
      this.poke();
    }, undefined, ime);
    on('input', (e) => {
      if (this.composing || e.isComposing) { fitText(); return; }
      // text no keydown delivered (virtual keyboards, some dead keys); compositions arrive at compositionend
      if (e.data && e.inputType === 'insertText') { this.mod.text(inst, e.data); this.poke(); }
      ime.value = '';
    }, undefined, ime);
    // moving the keyboard between the canvas and the IME anchor is not a focus change for CUI
    const inside = (el) => el === c || el === ime;
    onKeys('focus', (e) => { if (inside(e.relatedTarget)) return; x.inst_focus(inst, 1); this.poke(); this.opts.onStats?.(this); });
    onKeys('blur', (e) => {
      if (inside(e.relatedTarget)) return;
      // release anything held so a key never sticks
      for (const k of Object.values(KEYS)) x.inst_key(inst, k, 0, 0);
      x.inst_focus(inst, 0);
      this.poke();
      setTimeout(() => this.opts.onStats?.(this));
    });
  }

  observe() {
    if (!('IntersectionObserver' in window)) return;
    this.io = new IntersectionObserver((es) => {
      for (const e of es) { this.visible = e.isIntersecting; if (this.visible) this.poke(); }
    }, { rootMargin: '200px' });
    this.io.observe(this.el);
  }

  // ---- settings -------------------------------------------------------------
  setTheme(dark) {
    if (!this.inst || this.forcedDark || this.opts.dark !== undefined) return;
    this.mod.exports.inst_set_theme(this.inst, dark ? 1 : 0); this.poke();
  }
  setAccent(a) { if (this.inst && this.opts.accent === undefined && !this.opts.ownAccent) { this.mod.exports.inst_set_accent(this.inst, a); this.poke(); } }
  setReducedMotion(on) { if (this.inst) { this.mod.exports.inst_set_reduced_motion(this.inst, on ? 1 : 0); this.poke(); } }

  snapshot() {
    // render synchronously so the WebGL buffer is still there for toBlob
    if (this.gl) this.gl.draw(this.mod, this.inst);
    return new Promise((res) => this.canvas.toBlob(res, 'image/png'));
  }

  dispose() {
    this.disposed = true;
    live.delete(this);
    this.io?.disconnect();
    this.ro?.disconnect();
    (this.offs || []).forEach((f) => f());
    if (this.inst) { try { this.mod.exports.inst_free(this.inst); } catch (e) {} }
    if (this.gl) {
      this.gl.dispose();
      this.gl.gl.getExtension('WEBGL_lose_context')?.loseContext();
      activeGL--;
    }
    this.inst = 0;
    this.mod = null;
  }
}

export function allLive() { return [...live]; }
