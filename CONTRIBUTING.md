# Contributing / Contribuindo

Forks and pull requests are welcome. / Forks e pull requests são bem-vindos.

- Keep the Lua API in `src/main.cpp` thin; platform code lives behind `src/window.h`.
- New platforms: add `src/window_cocoa.mm` (macOS) or `src/window_gtk.cpp` (Linux) implementing `src/window.h`.
- When you add or change an `RG_*` function, update `docs/API.md`, `docs/API.pt-BR.md` and `.claude/skills/reaglass/SKILL.md`.
- By contributing you agree your work is released under the MIT license.
- Keep the copyright notice in redistributed copies (see `LICENSE`).
