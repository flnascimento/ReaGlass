# ReaGlass — Referência da API

> English: [API.md](API.md)

O ReaGlass (`reaper_reaglass-x64.dll`) é uma extensão do REAPER que permite a scripts ReaScript (Lua)
abrir janelas cuja interface é **HTML + CSS + JavaScript**. Pense em "ReaImGui, mas a interface é uma página web".

- Instalação: coloque a DLL na pasta `UserPlugins` do REAPER e reinicie o REAPER.
- As funções aparecem no ReaScript como `reaper.RG_*`. Se `reaper.RG_Create` for `nil`, o plugin não foi carregado.
- Plataformas: por enquanto **só Windows (WebView2)**. macOS e Linux estão planejados (`src/window.h` é a interface de plataforma).
- Tudo roda na thread principal do REAPER. Não há callbacks para o Lua: o Lua **consulta** (poll) as mensagens.

## Modelo mental

```
Script Lua  --RG_Send(win, "evento", "texto")-->  JS:  reaper.on("evento", fn)
Script Lua  <--RG_Poll / RG_Payload------------   JS:  reaper.send("evento", dados)
```

A página web é a interface e guarda a lógica de interface. O script Lua cuida de tudo que toca o REAPER
(tracks, itens, transporte) e roda um loop `reaper.defer` que esvazia as mensagens vindas da página.

## API Lua

| Função | Retorna | Descrição |
|---|---|---|
| `RG_Create(key, title, width, height, debug)` | `int` handle (0 = erro) | Cria uma janela. `width`/`height` são pixels lógicos (96 DPI), escalados pela DPI do sistema. Chamar de novo com a mesma `key` reutiliza a janela existente (rodar o script de novo não abre duplicatas). `debug=true` habilita as ferramentas de desenvolvedor (botão direito → Inspecionar). |
| `RG_Close(win)` | – | Destrói a janela. |
| `RG_IsOpen(win)` | `bool` | `false` depois que o usuário fecha a janela. Use para encerrar o loop defer. |
| `RG_Show(win)` | – | Mostra a janela de novo depois que o usuário a fechou. |
| `RG_SetTitle(win, title)` | – | Muda o título. |
| `RG_Navigate(win, url)` | – | Carrega uma URL: `https://...` ou `file:///C:/caminho/index.html` (barras normais). |
| `RG_SetHtml(win, html)` | – | Carrega HTML de uma string. |
| `RG_Eval(win, js)` | – | Executa JavaScript na página. Sem valor de retorno. |
| `RG_Send(win, event, payload)` | – | Lua → JS. Dispara os handlers `reaper.on(event, fn)`. `payload` é uma **string**. |
| `RG_Poll(win)` | `string` | JS → Lua. Retira a próxima mensagem enviada por `reaper.send()` e retorna o **nome do evento**, ou `""` se a fila estiver vazia. |
| `RG_Payload(win)` | `string` | Payload (string) da mensagem retornada pelo **último** `RG_Poll`. |

As janelas não são destruídas quando o script termina; vivem até `RG_Close` ou o REAPER fechar.
Fechar pelo X apenas esconde a janela (`RG_IsOpen` passa a ser `false`).

## API JavaScript (dentro da página)

`window.reaper` é injetado antes dos scripts da própria página.

```js
reaper.send(event, data)   // JS -> Lua. data: string vai como está; qualquer outro tipo passa por JSON.stringify.
reaper.on(event, fn)       // Lua -> JS. fn(payloadString). Use '*' para receber tudo: fn(event, payload).
reaper.off(event, fn)
```

Payloads são sempre strings. O Lua não tem JSON nativo, então prefira formatos simples de JS → Lua
(`"3=#ff7a00;4=none"`, `"a|b|c"`, um número) e envie JSON de Lua → JS (o JS faz `JSON.parse`).

## Exemplo mínimo

`ui.html`
```html
<button onclick="reaper.send('ping', 'oi')">Ping</button>
<div id="out"></div>
<script>
  reaper.on('pong', p => document.getElementById('out').textContent = p);
  reaper.send('ready', '');           // avisa o Lua que a página carregou
</script>
```

`script.lua`
```lua
local dir = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
local win = reaper.RG_Create("meu_script", "Meu painel", 400, 300, false)
if win == 0 then reaper.ShowMessageBox("Falha ao criar janela", "Erro", 0) return end
reaper.RG_Navigate(win, "file:///" .. (dir .. "ui.html"):gsub("\\", "/"))

local function loop()
  if not reaper.RG_IsOpen(win) then return end          -- usuário fechou
  while true do
    local ev = reaper.RG_Poll(win)
    if ev == "" then break end
    local data = reaper.RG_Payload(win)
    if ev == "ping" then reaper.RG_Send(win, "pong", "recebi: " .. data) end
  end
  reaper.defer(loop)
end
loop()
```

Exemplos completos: `examples/hello` e `examples/track_colors` (interface realista com envio de estado e comandos).

## Regras e armadilhas

1. **Espere a página carregar antes de enviar.** `RG_Send`/`RG_Eval` logo após `RG_Navigate` podem rodar antes do carregamento e se perder. Faça a página chamar `reaper.send('ready','')` no fim do script e envie o estado inicial quando o Lua receber esse evento.
2. **Esvazie a fila sempre em loop** (`while ev ~= ""`), não uma vez por frame.
3. **Sempre cheque `RG_IsOpen`** no início do loop defer, senão o script continua rodando depois que a janela fecha.
4. **Não envie `RG_Send` a cada frame.** Envie só quando o valor mudar (compare com o último enviado).
5. **`RG_Poll` retorna só o nome.** Leia o payload com `RG_Payload` antes de chamar `RG_Poll` de novo.
6. **Use `file:///` com barras normais** para páginas locais; descubra a pasta do script com `debug.getinfo(1,"S")`.
7. **Não há pedido/resposta.** Modele como eventos: o JS envia um comando, o Lua aplica, o Lua devolve o novo estado.
8. **Escape as strings Lua** (aspas, barras invertidas, caracteres de controle) quando montar JSON à mão.
9. **Alterações no projeto** devem ficar dentro de `reaper.Undo_BeginBlock()/Undo_EndBlock()` no Lua.
10. Não use ReaImGui nem `gfx` nessas janelas; use `RG_*`.

## Ainda não suportado

Encaixe (dock) dentro do REAPER, opções de multimonitor/DPI, valor de retorno do JS para o Lua via `RG_Eval`,
macOS, Linux, rodar arquivos `.js` como scripts, integração com JSFX (JSFX não chama APIs de extensões).
