-- Track Colors: pinta/descolore tracks com uma interface HTML. Rode como ReaScript.
-- Lua so aplica as cores e informa o estado; a logica de cores fica no index.html.
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local win = reaper.RG_Create("track_colors", "Track Colors", 760, 560, false)
if win == 0 then reaper.ShowMessageBox("Falha ao criar janela", "Track Colors", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "index.html"):gsub("\\", "/"))

local function esc(s) return (s:gsub('[%c"\\]', function(c)
  if c == '"' then return '\\"' elseif c == "\\" then return "\\\\" end
  return string.format("\\u%04x", c:byte()) end)) end

-- Estado das tracks como JSON: [{"i":0,"n":"Bateria","c":"#ff7a00"|"", "s":true|false}, ...]
local function snapshot()
  local out = {}
  for i = 0, reaper.CountTracks(0) - 1 do
    local tr = reaper.GetTrack(0, i)
    local _, name = reaper.GetTrackName(tr)
    local col = reaper.GetTrackColor(tr)
    local hex = ""
    if col ~= 0 then
      local r, g, b = reaper.ColorFromNative(col)
      hex = string.format("#%02x%02x%02x", r, g, b)
    end
    local sel = reaper.IsTrackSelected(tr)
    out[#out + 1] = string.format('{"i":%d,"n":"%s","c":"%s","s":%s}', i, esc(name), hex, tostring(sel))
  end
  return "[" .. table.concat(out, ",") .. "]"
end

-- payload "idx=#rrggbb;idx=none;..."
local function apply(payload)
  reaper.Undo_BeginBlock()
  for idx, val in payload:gmatch("(%d+)=([^;]+)") do
    local tr = reaper.GetTrack(0, tonumber(idx))
    if tr then
      if val == "none" then
        reaper.SetTrackColor(tr, 0)
      else
        local r, g, b = val:match("#(%x%x)(%x%x)(%x%x)")
        if r then
          reaper.SetTrackColor(tr, reaper.ColorToNative(tonumber(r, 16), tonumber(g, 16), tonumber(b, 16)) | 0x1000000)
        end
      end
    end
  end
  reaper.Undo_EndBlock("Track Colors", -1)
  reaper.UpdateArrange()
  reaper.TrackList_AdjustWindows(false)
end

-- payload: indice da track; "" = limpa a selecao
local function select_only(payload)
  for i = 0, reaper.CountTracks(0) - 1 do
    reaper.SetTrackSelected(reaper.GetTrack(0, i), tostring(i) == payload)
  end
end

local last, ready = nil, false
local function push(force)
  local s = snapshot()
  if force or s ~= last then last = s; reaper.RG_Send(win, "tracks", s) end
end

local function loop()
  if not reaper.RG_IsOpen(win) then return end
  while true do
    local ev = reaper.RG_Poll(win)
    if ev == "" then break end
    local data = reaper.RG_Payload(win)
    if ev == "ready" or ev == "refresh" then push(true)
    elseif ev == "set" then apply(data); push(true)
    elseif ev == "select" then select_only(data); push(true)
    end
  end
  push(false)
  reaper.defer(loop)
end
loop()
