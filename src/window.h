#pragma once
// Interface de plataforma. Uma implementação por SO (window_win32.cpp; futuramente
// window_cocoa.mm e window_gtk.cpp). A camada de API/Lua (main.cpp) só usa isto.
namespace rg {
void Init(void* mainHwnd);
void Shutdown();
int  Create(const char* key, const char* title, int w, int h, bool debug);
void Close(int id);
bool IsOpen(int id);
void Show(int id);
void SetTitle(int id, const char* title);
void Navigate(int id, const char* url);
void SetHtml(int id, const char* html);
void Eval(int id, const char* js);
void Send(int id, const char* evt, const char* payload);  // Lua -> JS
const char* Poll(int id);     // JS -> Lua: nome do evento ("" se vazio)
const char* Payload(int id);  // payload do último Poll
}  // namespace rg
