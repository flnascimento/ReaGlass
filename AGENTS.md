# AGENTS.md

This repo is **ReaGlass**: a REAPER extension (C++) that gives ReaScript/Lua windows with an
HTML/CSS/JS interface through `reaper.RG_*` functions.

## If the task is "write a REAPER script with a GUI" (using this plugin)

1. Read `docs/API.md` first (Portuguese: `docs/API.pt-BR.md`). It is the complete, authoritative API (Lua functions, the JS `window.reaper`
   bridge, a minimal example and pitfalls). Do not guess function names.
2. Copy the structure of `examples/hello` (minimal) or `examples/track_colors` (realistic).
3. Use `reaper.RG_*`, not ReaImGui or `gfx`.
4. Split responsibilities: HTML/CSS/JS = UI and UI logic; Lua = everything that touches REAPER.
5. Follow the pitfalls list in `docs/API.md` (wait for a `ready` event, drain the queue, check `RG_IsOpen`).

## If the task is "work on the plugin itself"

- Build (Windows): run `scripts/fetch-deps.ps1` once (downloads `deps/`, pinned versions), then `build.bat` (needs VS 2022 Build Tools + Git). Close REAPER before rebuilding: Windows locks the loaded DLL.
- `src/main.cpp`: registers the `RG_*` API with REAPER (`API_`, `APIvararg_`, `APIdef_`).
- `src/window.h`: platform interface. `src/window_win32.cpp` is the only implementation (WebView2).
  macOS/Linux = add `window_cocoa.mm` / `window_gtk.cpp` implementing the same interface.
- When adding or changing an API function, update `docs/API.md`, `docs/API.pt-BR.md` and the skill in `.claude/skills/`.
