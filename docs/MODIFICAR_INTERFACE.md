# Como modificar a interface

**Português** · [English](MODIFY_INTERFACE.md) · [Русский](MODIFY_INTERFACE.ru.md)

Guia prático para mudar a interface do C3 nesta source: como as janelas são construídas, como acrescentar uma
camada de recursos (o C3 **não tem uma**) e receitas passo a passo (barra de atalhos nova, botão, janela, cor de
chat, linha de tooltip). Os dois alvos que existem, `NWindowDecomp` (hook) e `NWindowStandalone`, são fiéis ao
original e é neles que os juízes comparam com ele: toda mudança de interface vai num **terceiro** alvo, que você
cria (seção 2), e fica atrás de `#if NW_FEATURES`.

## 1. Como as janelas são construídas

### A hierarquia

| Classe | Arquivo | Papel |
|---|---|---|
| `NCObject : UObject` (0x28) | `include/nwindow/NCObject.h` | base de tudo; `FindTexture(TEXT("Pacote.Grupo.Nome"))` carrega uma textura |
| `NCWnd` (0xC8 bytes) | `include/nwindow/NCWnd.h` | toda janela: posição (`LocalX/Y` relativas ao pai, `ScreenX/Y`, `Width`, `Height` absolutas), estilo, filhos, foco, barra de título (`FrameCtrl`), ícone minimizado |
| `NControl : NCWnd` (0x1D4) | `include/nwindow/NControl.h` | todo controle; tem um `ID` e avisa o pai com `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, ... | `include/nwindow/NCButton.h` | botões com texturas normal/pressionada (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | os outros controles |
| `NC*Wnd` | `include/nwindow/` | as janelas do jogo (inventário, chat, barra de atalhos, status...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | dono de todas as janelas: registro, ordem z, teste de clique, foco, despacho de mensagens, cursores |
| `NConsoleWnd : UL2ConsoleWnd` (0x39E8) | `include/nwindow/NConsoleWnd.h` | o console da engine (405 slots: eventos de rede e de UI); `GConsole` |
| `NCConsole` (0x228) | `include/nwindow/NCConsole.h` | a **área de trabalho** do jogo (`GConsole->GetDesktop()`, `GDesktop`): pai de toda janela do jogo, com um ponteiro para cada uma (`ShortCutWnd` +0x138, `ChatWnd`...) |

### Criar uma janela ou um controle

O padrão do código original (ex.: `NCSystemMenuWnd::OnCreate` em `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
NCButton* Button = new NCButton(TEXT("L2UI_CH3.SystemMenuWnd.systemicon1"),
                                TEXT("L2UI_CH3.SystemMenuWnd.systemicon1_down"), 0);
Button->ID = NID_SYSMENU_BUTTON_C;      // o que chega no NWM_COMMAND do pai
Button->SetPosition(13, 29, 36, 36);    // relativa ao pai
CreateChild(Button, NWS_VISIBLE);       // registra, torna a posição absoluta, manda WM_CREATE -> OnCreate
```

- O `new` de uma classe do pacote é o `operator new` do UE2 (o objeto é um `UObject`, alocado com o tamanho da
  `UClass` registrada).
- `CreateChild(filho, estilo)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + posição de tela + `WM_CREATE`. A
  área de trabalho registra as janelas do jogo com `AddChildWindow(janela, estilo)`, `HideWindow()` e o anexo
  inline (`AttachInline` em `src/nwindow/NCConsole.cpp`).
- Estilos (`NWS_*` em `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (lista à parte, por cima), `NWS_TITLEBAR`
  (`OnCreate` cria uma barra `NCFrameCtrl`), `NWS_MINIMIZABLE`, `NWS_NOCLOSE`, e os do C3: `NWS_DRAGABLE`,
  `NWS_CLIPTOPARENT`, `NWS_NOFOCUSNEXT`...
- Destruir: no `OnDestroy` do dono, `RemoveChildWindow(Filho->Entry); delete Filho;` (veja
  `NCSystemMenuWnd::OnDestroy`). `NCWnd::OnDestroy` só destrói a barra de título e o ícone.

### Mensagens, pintura, tooltips

- Tudo chega por `WindowProc(Msg, wParam, lParam)` (slot 25) com números de mensagem do Win32; o `NCWnd` despacha
  para as virtuais (números de slot do **C3**): `OnCreate` (37), `OnDestroy` (38), `ShowWindow` (44), `HideWindow`
  (45), `OnPaint(NCGDevice*)` (56), `OnDrawToolTip` (57), `OnKeyDown` (59), `OnMouseMove` (61),
  `OnLButtonDown/Up` (62/63), `OnRButtonDown` (64), `OnMove` (66, `WM_MOVE`).
- Um controle avisa o pai com `NWM_COMMAND` (`0x501`), `wParam` = o `ID` do controle; o pai trata no seu
  `WindowProc` (ex.: `NCSystemMenuWnd::WindowProc`).
- A pintura (`OnPaint`) usa o `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture`, `DrawStretchedTex`, `DrawRect`, `DrawLine`, `DrawText(...)`, `EndPaint`,
  `ReleaseClip`. As texturas são carregadas uma vez no `OnCreate` com `FindTexture` (os pacotes `L2UI_CH3.*` e
  `L2UI.*` do cliente).
- Tooltip: `OnDrawToolTip(NCGDevice*)` da janela sob o mouse, chamado depois da pintura.

### Console e área de trabalho

`NConsoleWnd` (o objeto que a engine cria a partir do `L2.ini`) recebe os eventos do servidor e do jogo (os seus
405 slots) e é dono da área de trabalho `NCConsole`. `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`) cria
toda janela do jogo (posições a partir do tamanho da tela) e `NCConsole::OnDestroy` as destrói (`DESTROY_AT`).
`NConsoleWnd::Tick` (`src/nwindow/NConsoleWndInput.cpp`) roda a cada quadro.

## 2. Acrescentar uma camada de recursos (o C3 não tem)

No C1 os recursos novos ficam num terceiro alvo, `NWindowL2`, com `NW_FEATURES=1` e `src/features/` (veja o
projeto do C1). **Nada disso existe nesta árvore.** Os blocos `#if NW_FEATURES` que aparecem em `src/nwindow`
(`NCAbnormalStatusWnd.cpp`, `NCActionItemWnd.cpp`, `NCChatWnd.cpp`, `NCEquipItemWnd.cpp`, `NCItemWnd.cpp`,
`NCommandParser.cpp`, `NConsoleWndEvents.cpp`, `NConsoleWndInput.cpp`, `NConsoleWndLogin.cpp`,
`NConsoleWndWindows.cpp`, `NCVideoOptionWnd.cpp`, `NCVirtualWndMain.cpp`, `NCWnd.cpp`; e `include/nwindow/NCVideoOptionWnd.h`) são pontos de chamada
herdados do C1, inertes: incluem cabeçalhos `features/*.h` que não estão aqui. O caminho para criar a camada, em
cima das classes que já existem:

1. **Alvo CMake.** No `CMakeLists.txt`, uma segunda biblioteca de objetos com as mesmas fontes do
   `nwd_standalone_objs` mais `src/features/*.cpp`, os mesmos flags e `NW_FEATURES=1`, e o DLL com a função que já
   existe: `nwd_standalone_dll_of(nwd_l2_objs NWindowL2 NWindow "${CMAKE_BINARY_DIR}/l2")`. Os dois alvos fiéis
   continuam sem `NW_FEATURES`.
2. **Os pontos herdados.** Antes de ligar `NW_FEATURES`, decida cada bloco herdado: apague-o, ou traga o recurso do
   C1 e adapte-o ao C3 (endereços, layouts e números de slot do C1 **não** valem aqui). Sem isso o alvo novo não
   compila.
3. **Código do recurso** em `src/features/<Recurso>.cpp` + `include/features/<Recurso>.h`, em `namespace nwf` e
   `#if NW_FEATURES`. No código decompilado, só o ponto de chamada, em `#if NW_FEATURES ... #endif` com
   `// phase 6: <Recurso> ...`; o código fiel tem de ler igual sem ele. Cuidado com o `__LINE__` dos `check()`: não
   mude a contagem de linhas acima de um `check()` (deixe linhas em branco, como em `NConsoleWndLogin.cpp`).
4. **Pontos de ligação** que você mesmo cria: o `DllMain` (`src/standalone/NWindowPackage.cpp`) para instalar e
   desfazer o que mexe em outros módulos; o começo de `NConsoleWnd::Tick` para o trabalho por quadro (sob um
   tratador de exceção, como no C1); `NCConsole::OnCreate`/`OnDestroy` para criar/destruir janelas; os
   `WindowProc`/`OnPaint`/`OnDrawToolTip` das janelas que o recurso muda.
5. **Configuração**: não há leitor de configuração de recursos no C3. O C1 usa o registro
   (`HKCU\Software\Lineage2C1\NWindow\<Seção>`) e nenhum arquivo ao lado do DLL; faça o mesmo com uma chave própria.
6. **Classes novas** (janela com campos próprios): precisam de uma `UClass` registrada só no alvo novo, com o
   export `autoclass<Classe>` depois dos ordinais originais; o `tools/verify_standalone.py --l2` aceita exatamente
   as classes extras de `tests/standalone/l2_extras.csv` (hoje vazio). Os macros `NW_DECLARE_L2_CLASS` /
   `NW_IMPLEMENT_L2_CLASS` do C1 não estão nesta árvore; o modelo é o `NW_IMPLEMENT_CLASS` de
   `include/nwindow/NWindow.h`.
7. **Prova**: o ciclo completo verde (ele prova que hook e standalone não mudaram), `verify_standalone.py --dll
   build/l2/Release/NWindow.dll --l2`, depois um teste no jogo numa cópia da pasta `system`.

## 3. Receita: uma barra de atalhos a mais (como no C5 / Interlude)

Nada desta receita está implementado no C3; é o método que o projeto do C1 implementou e **testou no jogo**
(num ramo experimental), adaptado aos fatos do C3 desta source.

**A ideia (a do C5):** a barra extra **não tem slots próprios**. É mais uma instância de `NCShortCutWnd` mostrando
outra página da mesma tabela de atalhos. No C3 a tabela é de **12 × 10 = 120** atalhos (12 por página, F1..F12,
10 páginas; `new NCShortCutWnd(12, 10, 32)` em `NCConsole::OnCreate`). O slot de um atalho é `página × 12 +
posição`, um número plano que o servidor guarda e devolve; uma barra que só mostra outra página não muda nada no
protocolo, no servidor nem nas texturas. A classe mantém o tamanho (0x270) e não precisa de `UClass` nova.

**Passos** (num alvo com `NW_FEATURES`, seção 2):

1. **Arquivos do recurso:** `include/features/ShortCutBars.h` e `src/features/ShortCutBars.cpp`, com funções como
   `ShortCutBars_Create(NCConsole* Desktop, NCShortCutWnd* Main)`, `ShortCutBars_Destroy`, `ShortCutBars_OnTick`,
   `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`, `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`. O
   estado (as barras, o botão) fica em globais do `.cpp`, nunca em campos novos de `NCShortCutWnd`.
2. **Criar** em `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`), logo depois da barra principal (`W = new
   NCShortCutWnd(12, 10, 32)` ... `AddChildWindow(..., 0x85022)`, `HideWindow()`, o anexo inline): para cada barra
   extra, `new NCShortCutWnd(Main->Cols, Main->Pages, Main->IconSize)`, `SetPosition` e o mesmo registro. O
   `OnCreate` original fez tabelas próprias: libere-as (`delete[] Bar->Items`, `delete[] Bar->Keys`) e aponte para
   as da principal (`Bar->Items = Main->Items`, `Bar->Keys = Main->Keys`). `Bar->Page` = a primeira página
   mostrada. Esconda o `RotateButton` e o `PadButton` das extras (a principal gira todas; o modo game pad,
   `bPadMode`, fica só nela).
3. **Destruir** em `NCConsole::OnDestroy`, **antes** do `DESTROY_AT(0x138)` (a barra principal): nas extras, ponha
   `Items` e `Keys` em `NULL` (são da principal; senão o `OnDestroy` original as libera duas vezes), depois
   `RemoveChildWindow(Bar->Entry)` e `delete Bar`.
4. **Seguir a principal** no fim de `NCShortCutWnd::Tick` (`src/nwindow/NCShortCutWnd.cpp`; só a principal recebe o
   tick): para cada extra, a mesma orientação (`bHorizontal`, e `UpdateLayout()` quando muda), a posição (acima de
   uma barra horizontal, à esquerda de uma vertical, do outro lado quando falta tela; 46 × 504 vertical, 504 × 46
   horizontal) por `SetPosition`, e visível só enquanto a principal estiver.
5. **Botão de expandir** (opcional): um `NCPushButton` filho da principal (`CreateChild`), com um `ID` depois dos
   originais (os da barra vão até `0xFF000009`, `NID_SHORTCUT_PAD`), tratado no ramo `NWM_COMMAND` de
   `NCShortCutWnd::WindowProc`. As setas da página (`L2UI_CH3.ShortcutWnd.*`) servem de arte.
6. **Arrastar entre barras:** em `NCShortCutWnd::OnLButtonUp`, o ramo original que **apaga** um atalho solto fora
   da barra (`GConsole->RequestShortCutDel`) confere antes se o ponto está sobre outra barra; se estiver, move o
   atalho para o slot dali (registro + remoção pelo mesmo protocolo; troca quando o slot está ocupado), e soltar
   sobre uma barra mas fora de um slot não faz nada.
7. **Arrastar a alça de uma extra move o grupo:** no começo de `NCShortCutWnd::WindowProc`, um `WM_MOVE` de uma
   barra extra é repassado à principal, com o ponto de pega (`FrameCtrl`) corrigido pelo deslocamento da extra (do
   layout calculado, não das posições atuais, que atrasam um quadro); sem isso o `NCWnd::Snap` puxa o grupo e o
   mouse "pula".
8. **Arte:** confira os fundos `L2UI_CH3.ShortcutWnd.shortcut_backv` / `shortcut_back` (`UpdateLayout`); se tiverem o
   ícone do botão de girar pintado na textura, cubra essa célula nas extras no fim de `NCShortCutWnd::OnPaint`
   (`DrawTexture` entre `BeginPaint`/`EndPaint`).

**Bom saber:** tooltip, clique para usar, o auto-uso de soulshot do C3 (`bAutoShot`, por slot, na tabela
compartilhada) e o arrastar do inventário/skills funcionam sem mudança, porque é a mesma classe e tudo é calculado a
partir de `Page`. As teclas (F1..F12, páginas, game pad) ficam na principal. O filtro de pacotes do servidor pode
descartar registros de atalho enviados rápido demais. Só o jogo mostra problemas de posição e de arte.

## 4. Receita: um botão novo numa janela existente

Exemplo: mais um botão no menu do sistema (`NCSystemMenuWnd`; no C3 são 7 ícones de 36 × 36 em x = 13, um a cada
37 px a partir de y = 29, criados por `CreateMenuButton`).

1. `include/features/MeuBotao.h` / `src/features/MeuBotao.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MeuBotao_Create(NCWnd* Dono)`, `MeuBotao_OnCommand(NCWnd* Dono, WPARAM ID)`, `MeuBotao_Destroy(NCWnd* Dono)`; o
   ponteiro do botão fica numa global do `.cpp`.
2. Crie no fim de `NCSystemMenuWnd::OnCreate`: `#if NW_FEATURES` / `// phase 6: MeuBotao ...` /
   `nwf::MeuBotao_Create(this);` / `#endif`. Dentro: `new NCButton(normal, pressionada, 0)`, um `ID` que nenhum
   controle da janela usa (veja o enum `NID_SYSMENU_*`, `0xF0F0xx`), `SetPosition` dentro da área da janela,
   `Dono->CreateChild(Botao, NWS_VISIBLE)`.
3. Trate o clique no ramo `NWM_COMMAND` de `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MeuBotao_OnCommand(this, wParam)) return 1;` (em `#if NW_FEATURES`).
4. Destrua em `NCSystemMenuWnd::OnDestroy`, antes de `NCWnd::OnDestroy()`: `RemoveChildWindow(Botao->Entry);
   delete Botao;`.
5. Se a janela tiver de crescer, mude o `SetPosition` dela em `NCConsole::OnCreate` e a pintura do fundo
   (`L2UI_CH3.SystemMenuWnd.systemmenu_back`) no `OnPaint` — sempre em `#if NW_FEATURES`, deixando o ramo original
   intacto.

## 5. Receita: uma janela nova

1. Uma janela com campos ou virtuais próprios é uma classe nova, registrada só no alvo de recursos (seção 2, item
   6): `class NCMinhaWnd : public NCWnd { ... };` com a sua `UClass`, e a linha dela em
   `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`).
2. Sobrescreva as virtuais que precisar: `OnCreate` (chame `NCWnd::OnCreate()` primeiro, carregue as texturas com
   `FindTexture`, crie a barra de título — `new NCFrameCtrl(1, 0, 0)` como o menu do sistema — e os controles),
   `OnDestroy` (destrua os filhos, depois `NCWnd::OnDestroy()`), `WindowProc` (`NWM_COMMAND`), `OnPaint`.
3. Crie na área de trabalho quando precisar (`SetPosition`, `GDesktop` como pai em `CreateChild(this,
   NWS_VISIBLE)`, `HideWindow()`) e mostre com `ShowWindow()` + `GDesktop->BringChildToTop(Entry)`.
4. Modelos nesta source: `NCSystemMenuWnd` (pequena, com botões), `NCAbnormalStatusWnd` (grade de ícones com
   tooltip), as janelas de loja privada (`NCPrivateShopWnd`, listas com rolagem).

## 6. Receita: uma cor de chat nova

O servidor não manda cor: manda o **tipo** do chat (`Say2`), e o cliente escolhe a cor em `NConsoleWnd::Say2`
(0x10073630, `src/nwindow/NConsoleWndEvents.cpp`): uma cadeia `if/else` por tipo (`SAY2_Shout` laranja
`0xFFFF7200`, `SAY2_Party` verde, `SAY2_Pledge`, `SAY2_Tell`, `SAY2_Market`, `SAY2_Ally`, petição, anúncio; os
tipos 11 e 14 são novos no C3) que termina num `else` cinza-claro (`0xFFDCDCDC`). `NConsoleWnd::NpcSay`, logo
abaixo, tem a sua própria cadeia.

1. Um tipo que o C3 não conhece: acrescente, em `#if NW_FEATURES`, um ramo antes do último `else` (ou chame uma
   função `nwf::ChatColors_Find(Type, &Color)` ali), com a cor em ARGB (ex.: `0xFF408CFF`).
2. A linha continua indo para o chat como uma normal (`GetDesktop()->Say(..., Cor, Tipo)`). Não reuse tipos com
   efeitos colaterais (o `SAY2_Tell` põe quem falou na lista de mensagens privadas).
3. Mudar a cor de um tipo que o C3 **já** colore é mudar um ramo fiel: faça em `#if NW_FEATURES` dentro do
   `Say2`, deixando a cor original no ramo `#else`.

## 7. Receita: uma linha nova no tooltip de item

O tooltip completo de item é desenhado pelas rotinas de `include/nwindow/NItemToolTip.h`, que as grades de itens
usam: `SetTexts`/`SetStoreTexts` preenchem `ToolTipName` e `ToolTipText` (o texto que **dimensiona** a caixa) e
`DrawLines` desenha as linhas, 13 pixels uma da outra (`DrawTypeLines`, depois `DrawTailLines`, que termina com a
descrição).

1. Acrescente a linha nos dois lugares, em `#if NW_FEATURES`: no texto que dimensiona (para a caixa crescer) e no
   desenho (ex.: em `DrawTailLines`, antes da descrição, com `DrawLabel`/`DrawValue` e `Y += 13`).
2. As rotinas são templates instanciados por cada grade com parâmetros diferentes (inventário, paperdoll, troca,
   armazém, lojas privadas, pet): confira toda cópia que mudar.
3. Para tooltips de outras janelas o modelo é `NCAbnormalStatusWnd::OnDrawToolTip`
   (`src/nwindow/NCAbnormalStatusWnd.cpp`): calcule a altura extra antes de desenhar a caixa e desenhe a linha
   depois.

## 8. O ciclo de build e teste

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "<cliente C3>\system_nwtest" --game-source --standalone --no-install
python tools\verify_standalone.py --dll build\l2\Release\NWindow.dll --l2
```

1. Compile o alvo de recursos (o nome é o que você deu na seção 2) até não haver erro nem aviso novo.
2. Rode o ciclo completo: ele prova que o modo hook e o standalone fiel não mudaram (os juízes os comparam com o
   original). Depois o `verify_standalone --l2` no DLL de recursos.
3. Teste no jogo: feche o jogo, guarde o `NWindow.dll` de uma **cópia** da pasta `system` e copie o seu DLL por
   cima. O ciclo não desenha a interface: posição, arte e arrastar só aparecem no jogo.

## 9. Regras de segurança

- Nunca acrescente campos nem slots virtuais novos a uma classe original: o tamanho dela está registrado na engine
  e o código original indexa nela e na sua vtable. Sobrescrever um slot que a classe já herda é permitido (dentro
  de `#if NW_FEATURES`). Estado de recurso vai em globais do `.cpp` ou numa classe nova.
- No código decompilado, só o ponto de chamada, em `#if NW_FEATURES` com `// phase 6: ...`; o ramo original fica
  como está, e a contagem de linhas acima de um `check()` não muda.
- Remendo em tempo de execução de outro módulo confere os bytes antes e desliga o recurso quando não batem.
- Configuração no registro, nunca em arquivo ao lado do DLL.
- Mesmo protocolo do cliente original: o servidor não muda; respeite os limites do filtro de pacotes dele.
- Teste numa cópia do cliente, nunca na pasta com que você joga; nenhum binário do jogo entra no repositório.
- Commits pequenos, um assunto cada, com o ciclo verde.
