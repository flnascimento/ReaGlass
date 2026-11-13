# ReaGlass

**HTML, CSS and JavaScript interfaces for REAPER Lua scripts.**

> Português: [README.pt-BR.md](README.pt-BR.md)

ReaGlass is a REAPER extension that lets a Lua ReaScript open a window whose interface is a web page.
Build your panel with the tools you already know and talk to REAPER through a small message bridge.

```lua
local win = reaper.RG_Create("my_panel", "My panel", 400, 300, false)
reaper.RG_Navigate(win, "file:///C:/path/to/ui.html")
-- Lua -> page:  reaper.RG_Send(win, "event", "text")
-- page -> Lua:  reaper.RG_Poll(win) / reaper.RG_Payload(win)
```

```js
reaper.send('ping', 'hi');                       // page -> Lua
reaper.on('pong', text => console.log(text));    // Lua -> page
```

## Status

| Platform | Engine | State |
|---|---|---|
| Windows | WebView2 (Chromium) | Working |
| macOS | WKWebView | Planned |
| Linux | WebKitGTK | Planned |

The plugin is small (~280 KB) because it uses the browser engine already present in the OS.

## Install

1. Download `reaper_reaglass-x64.dll` from the [latest release](https://github.com/flnascimento/ReaGlass/releases/latest).
2. Copy it to REAPER's `UserPlugins` folder (REAPER → Options → Show REAPER resource path → `UserPlugins`).
3. Restart REAPER. `reaper.RG_Create` should now exist.
4. Requires the Microsoft Edge WebView2 Runtime (already included in Windows 10/11).

## Learn by example

- [`examples/hello`](examples/hello): minimal two-way bridge.
- [`examples/track_colors`](examples/track_colors): a polished panel that reads project state and recolors tracks.
- [`docs/API.md`](docs/API.md): complete API reference.

## For AI assistants

Point any AI at [`AGENTS.md`](AGENTS.md) / [`llms.txt`](llms.txt), or attach [`docs/API.md`](docs/API.md) to your prompt.
It contains the whole API, the message protocol and the pitfalls, so generated scripts work first time.
A Claude Code skill is in [`.claude/skills/reaglass`](.claude/skills/reaglass).

## Build (Windows)

Requires Visual Studio 2022 Build Tools. Fetch the dependencies into `deps/` (not versioned):

```bash
git clone --depth 1 https://github.com/justinfrankel/reaper-sdk deps/reaper-sdk
git clone --depth 1 https://github.com/webview/webview deps/webview
# WebView2 SDK: download the NuGet package Microsoft.Web.WebView2 and unzip it into deps/wv2
build.bat
```

Close REAPER before rebuilding: Windows locks the loaded DLL.

## License

[MIT](LICENSE): use it, change it, fork it, redistribute it, keeping the copyright notice (the credit).
Third-party licenses: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

"REAPER" is a trademark of Cockos Incorporated. ReaGlass is independent and not affiliated with or endorsed by Cockos.
