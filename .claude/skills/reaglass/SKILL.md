---
name: reaglass
description: Write REAPER Lua scripts with an HTML/CSS/JS interface using the ReaGlass extension (reaper.RG_* API). Use when the user wants a REAPER script/plugin with a web-based GUI, or mentions ReaGlass / RG_Create.
---

# ReaGlass

Full reference: `docs/API.md` in this repo (read it; it is authoritative). Examples: `examples/hello`, `examples/track_colors`.

## Essentials

Lua: `win = reaper.RG_Create(key, title, w, h, debug)`, `RG_Navigate(win, "file:///.../ui.html")`,
`RG_SetHtml`, `RG_Eval`, `RG_Send(win, event, payloadString)`, `RG_Poll(win)` (returns event name or ""),
`RG_Payload(win)`, `RG_IsOpen`, `RG_Show`, `RG_Close`, `RG_SetTitle`.

JS in the page: `reaper.send(event, data)`, `reaper.on(event, fn)`, `reaper.off(event, fn)`. Payloads are strings.

## Workflow

1. Page HTML/CSS/JS = UI + UI logic. Lua = all REAPER API calls.
2. Page ends with `reaper.send('ready','')`; Lua pushes initial state when it receives `ready`.
3. `reaper.defer` loop: stop if `not RG_IsOpen`; drain `RG_Poll` in a `while` loop; send state only when changed.
4. JS → Lua payloads: simple delimited strings. Lua → JS payloads: JSON (escape strings correctly).
5. Wrap project edits in `Undo_BeginBlock/Undo_EndBlock`.
6. Never use ReaImGui or `gfx` for these windows. Windows-only for now.
