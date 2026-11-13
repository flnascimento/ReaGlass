// reaper_reaglass: expõe janelas HTML/CSS/JS ao ReaScript (Lua/EEL/Python) como reaper.RG_*.
#ifdef _WIN32
#include <windows.h>
#endif
#include <reaper_plugin.h>
#include <cstdint>
#include "window.h"

static int I(void* p) { return (int)(intptr_t)p; }
static void* R(intptr_t v) { return (void*)v; }
static const char* S(void* p) { return p ? (const char*)p : ""; }

// Cada função: implementação vararg (ReaScript) + definição de assinatura/ajuda.
#define VARARG(name) static void* vararg_##name(void** a, int n)
#define REG(rec, name, def)                                              \
  (rec)->Register("API_RG_" #name, (void*)&vararg_##name);                         \
  (rec)->Register("APIvararg_RG_" #name, (void*)&vararg_##name);         \
  (rec)->Register("APIdef_RG_" #name, (void*)def)

VARARG(Create) { return R(rg::Create(S(a[0]), S(a[1]), I(a[2]), I(a[3]), n > 4 && I(a[4]))); }
VARARG(Close)  { rg::Close(I(a[0])); return nullptr; }
VARARG(IsOpen) { return R(rg::IsOpen(I(a[0])) ? 1 : 0); }
VARARG(Show)   { rg::Show(I(a[0])); return nullptr; }
VARARG(SetTitle) { rg::SetTitle(I(a[0]), S(a[1])); return nullptr; }
VARARG(Navigate) { rg::Navigate(I(a[0]), S(a[1])); return nullptr; }
VARARG(SetHtml)  { rg::SetHtml(I(a[0]), S(a[1])); return nullptr; }
VARARG(Eval)     { rg::Eval(I(a[0]), S(a[1])); return nullptr; }
VARARG(Send)     { rg::Send(I(a[0]), S(a[1]), S(a[2])); return nullptr; }
VARARG(Poll)     { return (void*)rg::Poll(I(a[0])); }
VARARG(Payload)  { return (void*)rg::Payload(I(a[0])); }

static void RegisterAll(reaper_plugin_info_t* rec) {
  REG(rec, Create,
      "int\0const char*,const char*,int,int,bool\0key,title,width,height,debug\0"
      "Cria (ou reutiliza, pela chave 'key') uma janela HTML. Retorna o handle (0 = erro).");
  REG(rec, Close, "void\0int\0win\0Destroi a janela.");
  REG(rec, IsOpen, "bool\0int\0win\0True se a janela esta visivel (o usuario pode fecha-la).");
  REG(rec, Show, "void\0int\0win\0Mostra a janela novamente.");
  REG(rec, SetTitle, "void\0int,const char*\0win,title\0Muda o titulo.");
  REG(rec, Navigate, "void\0int,const char*\0win,url\0Abre uma URL (https://, file:///...).");
  REG(rec, SetHtml, "void\0int,const char*\0win,html\0Carrega HTML direto de uma string.");
  REG(rec, Eval, "void\0int,const char*\0win,js\0Executa JavaScript na pagina.");
  REG(rec, Send, "void\0int,const char*,const char*\0win,event,payload\0"
      "Lua -> JS: dispara reaper.on(event, fn) na pagina; payload e string.");
  REG(rec, Poll, "const char*\0int\0win\0"
      "JS -> Lua: retira a proxima mensagem enviada por reaper.send(); retorna o nome do evento ou \"\".");
  REG(rec, Payload, "const char*\0int\0win\0Payload (string) da ultima mensagem lida por RG_Poll.");
}

#undef REG

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t* rec) {
  if (!rec) { rg::Shutdown(); return 0; }
  if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) return 0;
  rg::Init(rec->hwnd_main);
  RegisterAll(rec);
  return 1;
}
