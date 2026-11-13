-- Tapeline Delay: a fictional delay UI built with ReaGlass.
-- Lua stores parameters in ExtState and restores them when the panel opens.
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local SECTION = "ReaGlass_TapelineDelay"

local win = reaper.RG_Create("tapeline_delay", "Tapeline Delay", 720, 470, false)
if win == 0 then reaper.ShowMessageBox("Could not create window", "Tapeline Delay", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "index.html"):gsub("\\", "/"))

local names = { "time", "feedback", "tone", "wobble", "mix", "mode" }
local params = {}
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
      reaper.RG_Send(win, "state", state_string())
    elseif ev == "param" then
      local name, value = data:match("^(%w+)=(.+)$")
      if name then params[name] = value; reaper.SetExtState(SECTION, name, value, true) end
    elseif ev == "mode" then
      params.mode = data; reaper.SetExtState(SECTION, "mode", data, true)
    end
  end
  reaper.defer(loop)
end
loop()
