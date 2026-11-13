// Janela host (Win32) + webview embutido (WebView2 via lib webview/webview).
#include "window.h"
#include <webview/webview.h>
#include <windows.h>
#include <cstdio>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace rg {

struct Window {
  std::string key;
  HWND hwnd = nullptr;
  webview_t wv = nullptr;
  bool open = false;
  std::deque<std::pair<std::string, std::string>> inbox;  // JS -> Lua
  std::pair<std::string, std::string> current;
};

static std::map<int, std::unique_ptr<Window>> g_windows;
static int g_next = 1;
static HWND g_owner = nullptr;
static const wchar_t* kClass = L"ReaGlassWindow";

static std::wstring Widen(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring w(n > 0 ? n - 1 : 0, L'\0');
  if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
  return w;
}

// Literal de string JS seguro (UTF-8 passa direto).
static std::string JsString(const std::string& s) {
  std::string o = "'";
  for (unsigned char c : s) {
    switch (c) {
      case '\\': o += "\\\\"; break;
      case '\'': o += "\\'"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      default:
        if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\x%02x", c); o += b; }
        else o += (char)c;
    }
  }
  return o + "'";
}

static std::string PercentDecode(const std::string& s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() + 0 && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) {
      o += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16);
      i += 2;
    } else o += s[i];
  }
  return o;
}

// window.reaper: ponte JS <-> Lua. send() vai como "evento|payload" percent-encoded.
static const char* kBootstrap = R"JS(
(function(){
  if (window.reaper) return;
  const handlers = {};
  window.reaper = {
    send(evt, data) {
      const s = (typeof data === 'string') ? data : JSON.stringify(data === undefined ? null : data);
      return window.__rg_send(encodeURIComponent(String(evt)) + '|' + encodeURIComponent(s));
    },
    on(evt, fn) { (handlers[evt] = handlers[evt] || []).push(fn); },
    off(evt, fn) { handlers[evt] = (handlers[evt] || []).filter(f => f !== fn); },
    _recv(evt, payload) {
      (handlers[evt] || []).forEach(f => { try { f(payload); } catch (e) { console.error(e); } });
      (handlers['*'] || []).forEach(f => { try { f(evt, payload); } catch (e) { console.error(e); } });
    }
  };
})();
)JS";

static void FitWidget(Window* win) {
  if (!win || !win->wv || !win->hwnd) return;
  HWND widget = (HWND)webview_get_native_handle(win->wv, WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET);
  RECT r; GetClientRect(win->hwnd, &r);
  if (widget) MoveWindow(widget, 0, 0, r.right, r.bottom, TRUE);
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  Window* win = (Window*)GetWindowLongPtrW(h, GWLP_USERDATA);
  switch (m) {
    case WM_SIZE: FitWidget(win); return 0;
    case WM_CLOSE:  // só esconde; destruição real em Close()/Shutdown()
      if (win) win->open = false;
      ShowWindow(h, SW_HIDE);
      return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

void Init(void* mainHwnd) {
  g_owner = (HWND)mainHwnd;
  WNDCLASSEXW wc{};
  wc.cbSize = sizeof wc;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = kClass;
  RegisterClassExW(&wc);
}

static void Destroy(Window* w) {
  if (w->hwnd) SetWindowLongPtrW(w->hwnd, GWLP_USERDATA, 0);
  if (w->wv) { webview_destroy(w->wv); w->wv = nullptr; }
  if (w->hwnd) { DestroyWindow(w->hwnd); w->hwnd = nullptr; }
  w->open = false;
}

void Shutdown() {
  for (auto& kv : g_windows) Destroy(kv.second.get());
  g_windows.clear();
  UnregisterClassW(kClass, GetModuleHandleW(nullptr));
}

static Window* Get(int id) {
  auto it = g_windows.find(id);
  return it == g_windows.end() ? nullptr : it->second.get();
}

int Create(const char* key, const char* title, int w, int h, bool debug) {
  for (auto& kv : g_windows)
    if (kv.second->key == key) {  // reexecutar o script reutiliza a janela
      Window* ex = kv.second.get();
      ShowWindow(ex->hwnd, SW_SHOW);
      ex->open = true;
      return kv.first;
    }

  auto win = std::make_unique<Window>();
  win->key = key;
  // width/height são em pixels lógicos (96 DPI); escala pela DPI do sistema.
  HDC screen = GetDC(nullptr);
  int dpi = screen ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
  if (screen) ReleaseDC(nullptr, screen);
  RECT r{0, 0, MulDiv(w, dpi, 96), MulDiv(h, dpi, 96)};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  win->hwnd = CreateWindowExW(0, kClass, Widen(title).c_str(), WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
      g_owner, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (!win->hwnd) return 0;
  SetWindowLongPtrW(win->hwnd, GWLP_USERDATA, (LONG_PTR)win.get());

  Window* raw = win.get();
  raw->wv = webview_create(debug ? 1 : 0, raw->hwnd);
  if (!raw->wv) { Destroy(raw); return 0; }

  webview_bind(raw->wv, "__rg_send",
      [](const char* id, const char* req, void* arg) {
        Window* self = (Window*)arg;
        // req = ["evento%20x|payload%20y"] (sem aspas/barras internas: percent-encoded)
        std::string s(req);
        size_t a = s.find('"'), b = s.rfind('"');
        if (a != std::string::npos && b > a) {
          std::string body = s.substr(a + 1, b - a - 1);
          size_t bar = body.find('|');
          if (bar != std::string::npos)
            self->inbox.emplace_back(PercentDecode(body.substr(0, bar)), PercentDecode(body.substr(bar + 1)));
        }
        webview_return(self->wv, id, 0, "null");
      }, raw);
  webview_init(raw->wv, kBootstrap);
  webview_set_html(raw->wv, "<html><body></body></html>");

  FitWidget(raw);
  ShowWindow(raw->hwnd, SW_SHOW);
  raw->open = true;
  int id = g_next++;
  g_windows[id] = std::move(win);
  return id;
}

void Close(int id) {
  auto it = g_windows.find(id);
  if (it == g_windows.end()) return;
  Destroy(it->second.get());
  g_windows.erase(it);
}

bool IsOpen(int id) { Window* w = Get(id); return w && w->open; }
void Navigate(int id, const char* url) { if (Window* w = Get(id)) webview_navigate(w->wv, url); }
void SetHtml(int id, const char* html) { if (Window* w = Get(id)) webview_set_html(w->wv, html); }
void Eval(int id, const char* js) { if (Window* w = Get(id)) webview_eval(w->wv, js); }
void SetTitle(int id, const char* t) { if (Window* w = Get(id)) SetWindowTextW(w->hwnd, Widen(t).c_str()); }
void Show(int id) { if (Window* w = Get(id)) { ShowWindow(w->hwnd, SW_SHOW); w->open = true; } }

void Send(int id, const char* evt, const char* payload) {
  std::string js = "window.reaper&&window.reaper._recv(" + JsString(evt) + "," + JsString(payload) + ")";
  Eval(id, js.c_str());
}

const char* Poll(int id) {
  Window* w = Get(id);
  if (!w || w->inbox.empty()) return "";
  w->current = std::move(w->inbox.front());
  w->inbox.pop_front();
  return w->current.first.c_str();
}
const char* Payload(int id) { Window* w = Get(id); return w ? w->current.second.c_str() : ""; }

}  // namespace rg
