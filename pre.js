/*
 * pre.js — prepended to Emscripten output before Module initialises.
 *
 * Wires Emscripten's stdout to xterm.js character by character using
 * FS.init(stdin, stdout, stderr).  This lets all printf/putchar output —
 * including ANSI/VT100 escape sequences — go directly to the terminal
 * emulator in the browser.
 */
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
    function stdin()       { return null; }
    function stdout(code)  { if (window.__xtermWrite) window.__xtermWrite(code); }
    function stderr(code)  { /* discard */ }
    FS.init(stdin, stdout, stderr);
});
