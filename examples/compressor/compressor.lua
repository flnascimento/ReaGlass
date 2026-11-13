-- Aurora Comp: a fictional compressor UI built with ReaGlass.
-- The page does the DSP visualisation; Lua stores the parameters in REAPER's ExtState,
-- so the panel reopens exactly as you left it. Replace the "param" handler with
-- reaper.TrackFX_SetParam(...) to drive a real plugin.
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local SECTION = "ReaGlass_AuroraComp"

local win = reaper.RG_Create("aurora_comp", "Aurora Comp", 760, 470, false)
if win == 0 then reaper.ShowMessageBox("Could not create window", "Aurora Comp", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "index.html"):gsub("\\", "/"))

local params = {}   -- name -> value (strings, as received)
local names = { "threshold", "ratio", "attack", "release", "knee", "makeup", "mix" }
for _, n in ipairs(names) do
  local v = reaper.GetExtState(SECTION, n)
  if v ~= "" then params[n] = v end
end

local function state_string()
  local out = {}
  for n, v in pairs(params) do out[#out + 1] = n .. "=" .. v end
  return table.concat(out, ";")
end

local function loop()
  if not reaper.RG_IsOpen(win) then return end
  while true do
    local ev = reaper.RG_Poll(win)
    if ev == "" then break end
    local data = reaper.RG_Payload(win)
    if ev == "ready" then
      reaper.RG_Send(win, "state", state_string())      -- restore saved knobs
    elseif ev == "param" then
      local name, value = data:match("^(%w+)=(.+)$")
      if name then
        params[name] = value
        reaper.SetExtState(SECTION, name, value, true)   -- persist across sessions
      end
    end
  end
  reaper.defer(loop)
end
loop()
