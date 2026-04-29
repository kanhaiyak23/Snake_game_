/*
 * sound_wasm.c — Web Audio API sound synthesis for the browser build.
 *
 * All sounds are procedurally generated — no audio files.
 * AudioContext is lazily created and cached in window._audioCtx.
 *
 * Key EM_ASM constraint: commas inside JS array literals are parsed by the
 * C preprocessor as extra macro arguments.  All multi-note sequences are
 * handled via individual helper calls instead of JS array loops.
 *
 * Sounds:
 *   sound_eat(combo)   — two-phase sine "coin blip", pitch tracks combo
 *   sound_die()        — three descending sawtooth tones (death klaxon)
 *   sound_levelup()    — five-note square ascending fanfare (G4→D6)
 *   sound_game_start() — C major chord bloom (staggered sine)
 *   sound_wrap()       — FM wobble: 8 Hz LFO on 420 Hz carrier (portal)
 */
#include "sound.h"
#include <emscripten.h>

/* ── Internal note schedulers ──────────────────────────────────── */

static void sched_square(int freq, int delay_ms) {
    EM_ASM({
        var ctx = window._audioCtx ||
            (window._audioCtx = new (window.AudioContext || window.webkitAudioContext)());
        if (ctx.state === 'suspended') ctx.resume();
        var o = ctx.createOscillator();
        var g = ctx.createGain();
        o.connect(g); g.connect(ctx.destination);
        o.type = 'square';
        var t = ctx.currentTime + $1 * 0.001;
        o.frequency.setValueAtTime($0, t);
        g.gain.setValueAtTime(0.09, t);
        g.gain.exponentialRampToValueAtTime(0.001, t + 0.11);
        o.start(t); o.stop(t + 0.11);
    }, freq, delay_ms);
}

static void sched_saw(int freq, int delay_ms) {
    EM_ASM({
        var ctx = window._audioCtx ||
            (window._audioCtx = new (window.AudioContext || window.webkitAudioContext)());
        if (ctx.state === 'suspended') ctx.resume();
        var o = ctx.createOscillator();
        var g = ctx.createGain();
        o.connect(g); g.connect(ctx.destination);
        o.type = 'sawtooth';
        var t = ctx.currentTime + $1 * 0.001;
        o.frequency.setValueAtTime($0, t);
        g.gain.setValueAtTime(0.16, t);
        g.gain.exponentialRampToValueAtTime(0.001, t + 0.13);
        o.start(t); o.stop(t + 0.14);
    }, freq, delay_ms);
}

static void sched_sine(int freq, int delay_ms, int dur_ms) {
    EM_ASM({
        var ctx = window._audioCtx ||
            (window._audioCtx = new (window.AudioContext || window.webkitAudioContext)());
        if (ctx.state === 'suspended') ctx.resume();
        var o = ctx.createOscillator();
        var g = ctx.createGain();
        o.connect(g); g.connect(ctx.destination);
        o.type = 'sine';
        var t   = ctx.currentTime + $1 * 0.001;
        var dur = $2 * 0.001;
        o.frequency.setValueAtTime($0, t);
        g.gain.setValueAtTime(0.0, t);
        g.gain.linearRampToValueAtTime(0.13, t + 0.02);
        g.gain.exponentialRampToValueAtTime(0.001, t + dur);
        o.start(t); o.stop(t + dur);
    }, freq, delay_ms, dur_ms);
}

/* ── Public API ────────────────────────────────────────────────── */

void sound_eat(int combo) {
    /*
     * Two-phase sine "coin blip": base tone → 1.5× upper tone.
     * Base shifts up 40 Hz per combo level so eating streaks sound
     * progressively more intense.
     */
    int base = 660 + (combo > 1 ? (combo - 1) * 40 : 0);
    if (base > 920) base = 920;
    int high = base + base / 2;   /* exact 3:2 ratio = perfect fifth up */
    EM_ASM({
        var ctx = window._audioCtx ||
            (window._audioCtx = new (window.AudioContext || window.webkitAudioContext)());
        if (ctx.state === 'suspended') ctx.resume();
        var o = ctx.createOscillator();
        var g = ctx.createGain();
        o.connect(g); g.connect(ctx.destination);
        o.type = 'sine';
        var t = ctx.currentTime;
        o.frequency.setValueAtTime($0, t);
        o.frequency.setValueAtTime($1, t + 0.035);
        g.gain.setValueAtTime(0.0,  t);
        g.gain.linearRampToValueAtTime(0.16, t + 0.006);
        g.gain.exponentialRampToValueAtTime(0.001, t + 0.09);
        o.start(t); o.stop(t + 0.09);
    }, base, high);
}

void sound_die(void) {
    /*
     * Three descending sawtooth tones: A4 → E4 → A3.
     * Sawtooth gives a harsh buzz — classic arcade death klaxon.
     */
    sched_saw(440,   0);
    sched_saw(330,  95);
    sched_saw(220, 190);
}

void sound_levelup(void) {
    /*
     * G4→B4→D5→G5→D6 ascending square fanfare.
     * Square wave = bright retro 8-bit character.
     */
    sched_square(392,   0);
    sched_square(494,  85);
    sched_square(587, 170);
    sched_square(784, 255);
    sched_square(1175, 340);
}

void sound_game_start(void) {
    /*
     * C major chord bloom: C4→G4→C5→G5 staggered sine waves.
     * Staggering by 60 ms creates a "spreading" chord feel.
     */
    sched_sine(262,   0, 420);
    sched_sine(392,  60, 360);
    sched_sine(523, 120, 300);
    sched_sine(784, 180, 240);
}

void sound_wrap(void) {
    /*
     * FM synthesis portal wobble.
     * An 8 Hz LFO (modulator) with depth 180 Hz drives the carrier
     * frequency, producing a warbling warp-pipe effect while the
     * carrier sweeps down from 420 → 200 Hz.
     */
    EM_ASM({
        var ctx = window._audioCtx ||
            (window._audioCtx = new (window.AudioContext || window.webkitAudioContext)());
        if (ctx.state === 'suspended') ctx.resume();
        var mod     = ctx.createOscillator();
        var modGain = ctx.createGain();
        var car     = ctx.createOscillator();
        var g       = ctx.createGain();
        mod.connect(modGain);
        modGain.connect(car.frequency);
        car.connect(g);
        g.connect(ctx.destination);
        mod.type = 'sine';
        mod.frequency.setValueAtTime(8, ctx.currentTime);
        modGain.gain.setValueAtTime(180, ctx.currentTime);
        car.type = 'sine';
        car.frequency.setValueAtTime(420, ctx.currentTime);
        car.frequency.exponentialRampToValueAtTime(200, ctx.currentTime + 0.20);
        g.gain.setValueAtTime(0.11, ctx.currentTime);
        g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.20);
        mod.start(ctx.currentTime); mod.stop(ctx.currentTime + 0.20);
        car.start(ctx.currentTime); car.stop(ctx.currentTime + 0.20);
    });
}
