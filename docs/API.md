# ReaGlass — API reference

> Português: [API.pt-BR.md](API.pt-BR.md)

ReaGlass (`reaper_reaglass-x64.dll`) is a REAPER extension that lets ReaScript (Lua) open
windows whose interface is **HTML + CSS + JavaScript**. Think "ReaImGui, but the UI is a web page".

- Install: put the DLL in REAPER's `UserPlugins` folder and restart REAPER.
- Functions appear in ReaScript as `reaper.RG_*`. If `reaper.RG_Create` is `nil`, the plugin is not loaded.
- Platform status: **Windows (WebView2) only** for now. macOS/Linux are planned (`src/window.h` is the platform seam).
- Everything runs on REAPER's main thread. There are no callbacks into Lua: Lua **polls** for messages.

## Mental model

```
Lua script  --RG_Send(win, "event", "text")-->  JS:  reaper.on("event", fn)
Lua script  <--RG_Poll / RG_Payload------------   JS:  reaper.send("event", dados)
```

The web page is the UI and holds UI logic. The Lua script owns everything that touches REAPER
(tracks, items, transport) and runs a `reaper.defer` loop that drains messages from the page.

## Lua API

| Function | Returns | Description |
|---|---|---|
| `RG_Create(key, title, width, height, debug)` | `int` handle (0 = error) | Creates a window. `width`/`height` are logical pixels (96 DPI) and are scaled by the system DPI. Calling again with the same `key` reuses the existing window (re-running a script does not open duplicates). `debug=true` enables the browser devtools (right-click → Inspect). |
| `RG_Close(win)` | – | Destroys the window. |
| `RG_IsOpen(win)` | `bool` | `false` after the user closes the window. Use it to stop your defer loop. |
| `RG_Show(win)` | – | Shows the window again after the user closed it. |
| `RG_SetTitle(win, title)` | – | Changes the title bar. |
| `RG_Navigate(win, url)` | – | Loads a URL: `https://...` or `file:///C:/path/index.html` (forward slashes). |
| `RG_SetHtml(win, html)` | – | Loads HTML from a string. |
| `RG_Eval(win, js)` | – | Runs JavaScript in the page. Fire-and-forget (no return value). |
| `RG_Send(win, event, payload)` | – | Lua → JS. Triggers `reaper.on(event, fn)` handlers. `payload` is a **string**. |
| `RG_Poll(win)` | `string` | JS → Lua. Pops the next message sent by `reaper.send()` and returns its **event name**, or `""` if the queue is empty. |
| `RG_Payload(win)` | `string` | Payload (string) of the message returned by the **last** `RG_Poll`. |

Windows are not destroyed when a script ends; they live until `RG_Close` or REAPER exits.
Closing the window with the X button only hides it (`RG_IsOpen` becomes `false`).

## JavaScript API (inside the page)

`window.reaper` is injected before the page's own scripts run.

```js
reaper.send(event, data)   // JS -> Lua. data: string sent as-is; anything else is JSON.stringify'd.
reaper.on(event, fn)       // Lua -> JS. fn(payloadString). Use '*' to receive all: fn(event, payload).
reaper.off(event, fn)
```

Payloads are always strings. Lua has no built-in JSON, so prefer simple formats for JS → Lua
(`"3=#ff7a00;4=none"`, `"a|b|c"`, a plain number) and send JSON for Lua → JS (JS can `JSON.parse` it).

## Minimal example

`ui.html`
```html
<button onclick="reaper.send('ping', 'hi')">Ping</button>
<div id="out"></div>
<script>
  reaper.on('pong', p => document.getElementById('out').textContent = p);
  reaper.send('ready', '');           // tells Lua the page is loaded
</script>
```

`script.lua`
```lua
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local win = reaper.RG_Create("my_script", "My panel", 400, 300, false)
if win == 0 then reaper.ShowMessageBox("Could not create window", "Error", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "ui.html"):gsub("\\", "/"))

local function loop()
  if not reaper.RG_IsOpen(win) then return end          -- user closed the window
  while true do
    local ev = reaper.RG_Poll(win)
    if ev == "" then break end
    local data = reaper.RG_Payload(win)
    if ev == "ping" then reaper.RG_Send(win, "pong", "got: " .. data) end
  end
  reaper.defer(loop)
end
loop()
```

Full examples: `examples/hello` and `examples/track_colors` (a realistic UI with state push + commands).

## Rules and pitfalls

1. **Wait for the page before sending.** `RG_Send`/`RG_Eval` right after `RG_Navigate` may run before the page has loaded and be lost. Have the page call `reaper.send('ready','')` at the end of its script, and push initial state when Lua receives it.
2. **Always drain the queue in a loop** (`while ev ~= ""`), not once per frame.
3. **Always check `RG_IsOpen`** at the top of the defer loop, otherwise the script keeps running after the window closes.
4. **Don't spam `RG_Send` every frame.** Send only when the value changed (compare to the last sent value).
5. **`RG_Poll` returns only the name.** Read the payload with `RG_Payload` before polling again.
6. **Use `file:///` with forward slashes** for local pages; resolve the script's folder with `debug.getinfo(1,"S")`.
7. **No request/response.** Model it as events: JS sends a command, Lua applies it, Lua pushes the new state back.
8. **Escape Lua strings you embed in JSON** (quotes, backslashes, control characters) when building JSON by hand.
9. **UI changes to the project** should be wrapped in `reaper.Undo_BeginBlock()/Undo_EndBlock()` in Lua.
10. Do not use ReaImGui or `gfx` for these windows; use `RG_*`.

## Not (yet) supported

Docking inside REAPER, multiple monitors/DPI options, return values from JS to Lua via `RG_Eval`,
macOS, Linux, running `.js` files as scripts, JSFX integration (JSFX cannot call extension APIs).
