-- Exemplo: janela HTML com ponte JS <-> Lua. Rode como ReaScript no REAPER.
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local win = reaper.RG_Create("hello", "Hello ReaGlass", 420, 300, true)
if win == 0 then reaper.ShowMessageBox("Falha ao criar janela", "ReaGlass", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "index.html"):gsub("\\", "/"))

local last_pos
local function loop()
  if not reaper.RG_IsOpen(win) then return end   -- usuario fechou
  while true do
    local ev = reaper.RG_Poll(win)
    if ev == "" then break end
    local data = reaper.RG_Payload(win)
    if ev == "play" then reaper.Main_OnCommand(1007, 0)
    elseif ev == "ping" then reaper.RG_Send(win, "pong", "recebi: " .. data) end
  end
  local pos = string.format("%.2f", reaper.GetPlayPosition())
  if pos ~= last_pos then last_pos = pos; reaper.RG_Send(win, "pos", pos) end
  reaper.defer(loop)
end
loop()
