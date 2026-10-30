# NWindow.dll do Lineage II C3 — source reconstruída em C++

**Português** · [English](README.en.md) · [Русский](README.ru.md)

Reconstrução, função por função, do `NWindow.dll` do cliente Lineage II **Chronicle 3 (Rise of Darkness)**: a
DLL nativa da interface (janelas, controles, HTML das NPCs, IME, chat, console, messenger, replay). O alvo é o
build de 15/09/2005 (TimeDateStamp `0x4329071F`, Visual C++ 7.0 13.00.9466, md5 `f0362880…`, 309 classes,
311 exports, 10.992 funções). A mesma source compila de dois jeitos:

| Alvo CMake | Arquivo gerado | O que é |
|---|---|---|
| `NWindowDecomp` | `build/bin/Release/NWindowDecomp.dll` | **modo hook**: roda ao lado do `NWindow.dll` original e troca cada função já reimplementada por um `jmp` para a nossa |
| `NWindowStandalone` | `build/standalone/Release/NWindow.dll` | **standalone**: o `NWindow.dll` inteiro, só com o nosso código, carregado pelo jogo no lugar do original |

O projeto é o terceiro de uma série: nasceu do projeto do C2 (que nasceu do C1), com as mesmas ferramentas e o
mesmo método, apontados para o binário do C3; a source do C2 foi portada para o C3 e verificada função por função.

## Como funciona

**Modo hook.** O instalador (`tools/install_test.py`) acrescenta ao `NWindow.dll` original um import da
`NWindowDecomp.dll`. Ela carrega antes de qualquer código do original rodar e põe um `jmp` na entrada de cada
função que já reimplementamos (um `NW_HOOK_*` ao lado de cada definição, `src/re/NwHook.h`). O resto continua
sendo o código original. Assim cada função nova é testada no jogo de verdade, e qualquer uma pode ser
desligada no `NWindowDecomp.ini` (`[Hooks] Disable=`) para isolar um bug.

**Standalone (fase 5).** Com `NW_HOOK_MODE=0` os macros de hook ficam vazios e a source gera o `NWindow.dll`
sozinha: as 309 `UClass` (`src/standalone/Classes.gen.cpp`), as globais (`NwGlobals.cpp`, geradas da imagem do
C3), os exports com os ordinais do original (`src/standalone/NWindow.def`) e os 7 cursores coloridos. A engine
(`Core.dll`/`Engine.dll`) carrega o nosso DLL como carregava o original. O **GameGuard** (`NPG*`, a biblioteca
estática do nProtect que o original traz) não é portado: no standalone ele é um no-op documentado
(`include/nwindow/NGameGuard.h`, `src/standalone/NwGameGuard.cpp`), e a consulta de GameGuard que a engine faz ao
console recebe uma resposta (zeros, sem o GameMon), para que o login siga (`NConsoleWnd::GameGuardQueryX`, `src/nwindow/NConsoleWndC2.cpp`;
detalhes em [docs/notas/C3_Standalone.md](docs/notas/C3_Standalone.md), seção 2c). O NKeyCrypt
(`npkcrypt.dll`) é código real, carregado por ordinal.

Não há camada de recursos novos (fase 6) no C3: os blocos `#if NW_FEATURES` que aparecem na source são pontos de
chamada herdados do C1, inertes (nenhum alvo define `NW_FEATURES` e os cabeçalhos deles não estão nesta árvore).
Veja [docs/MODIFICAR_INTERFACE.md](docs/MODIFICAR_INTERFACE.md) para saber como acrescentar uma.

## Estado

- Modo hook: **4.970 de 6.534** funções a reimplementar estão feitas (**93,03 %** dos bytes de código). As 87
  que faltam sem motivo são o GameGuard (84 `NPG*` + 3 anônimas da mesma biblioteca), fora do escopo por
  decisão do dono; as outras sem gancho o compilador gera no standalone (*scalar deleting destructors*,
  `InternalConstructor`, inicialização estática). O build de 4.970 ganchos foi testado no jogo pelo dono
  (18/10/2026). Contagem viva: [docs/PROGRESSO.md](docs/PROGRESSO.md) (gerado por `tools/build_db.py`).
- Juízes do modo hook no último ciclo: `hooks 4970/4970, 0 errors`, 4.738 ok + 232 explicadas, 0 sem
  explicação, `regcheck` 0 problemas, `CHECKS: ALL OK`.
- Standalone (concluído em 29–30/10/2026): linka com 0 símbolos pendentes (2.198.016 bytes), passa no
  `verify_standalone.py` (PE, imports, recursos, 315 exports, 310 classes, vtables) e no
  `smoke_standalone.py` (as mesmas 438 linhas de saída do original) e **carrega no jogo** no lugar do original.
- O messenger (MSN) está todo portado, mas só tem prova estrutural (juízes + leitura do assembly): não há
  servidor de messenger para testar.
- As notas técnicas (`docs/`) estão em português (algumas em inglês); os comentários de código, em inglês.

## Requisitos

- Windows, **Visual Studio 2022** com C++ para x86 (MSVC v143), **CMake ≥ 3.20**;
- **Python 3** com `pefile` e `capstone` (`pip install pefile capstone`): usado pelo build standalone (extração dos
  cursores) e por todas as ferramentas; `unicorn` (`pip install unicorn`) para os testes diferenciais de
  `tools/port/emu/`;
- **o seu próprio cliente Lineage II C3**. Nenhum arquivo do jogo vem no repositório: copie para `orig/` os
  arquivos listados em [orig/README.md](orig/README.md) (o `NWindow.dll` tem de ter o md5 de lá);
- para os juízes do ciclo completo (`verify_hooks.py`, `regcheck.py`, `verify_standalone.py`, `build_db.py`,
  `tools/port/*`): o export do Ghidra em `analysis/ghidra/`, recriado por `bash tools/ghidra/run_full.sh`
  (Ghidra 11.3.2 e JDK 21; caminhos nas variáveis `NWD_GHIDRA_HOME` e `GHIDRA_JAVA_HOME`).

## Compilar

Num *Developer Command Prompt* x86 do VS 2022 (ou depois de `vcvars32.bat`), na raiz do repositório:

```bat
cmake -S . -B build -A Win32 -DNWD_GAME_SOURCES=ON
cmake --build build --config Release
cmake --build build --config Release --target NWindowStandalone
```

`-DNWD_GAME_SOURCES=ON` compila a source do jogo (`src/nwindow`); sem ela (o padrão histórico, `OFF`) só o
runtime do hook (`src/re`) é compilado, nenhuma função é trocada e o standalone não linka. O primeiro `--build`
gera o DLL do modo hook e as ferramentas de teste (`nwd_smoke`, `nwd_difftest`) e não precisa de `orig/`.
`NWindowStandalone` precisa de `orig/NWindow.dll`. O alvo `NWindowStandaloneProbe` (link com
`/FORCE:UNRESOLVED`) é só diagnóstico e **nunca** deve ser carregado.

## Instalar e testar

Use uma **cópia** da pasta `system` do cliente (ex.: `system_nwtest`), nunca a que você usa para jogar.

**Modo hook:**

```bat
python tools\install_test.py --system "<cliente C3>\system_nwtest"
python tools\install_test.py --system "<cliente C3>\system_nwtest" --restore
```

O primeiro grava um `NWindow.dll` refeito a partir de `orig/NWindow.dll` com o import a mais, a
`NWindowDecomp.dll` e um `NWindowDecomp.ini` (o `NWindow.dll` que estava lá fica como
`NWindow.dll.pre-decomp-bak`); o segundo devolve o que estava lá. O `NWindowDecomp.log` dessa pasta lista cada
hook instalado. No `.ini`: `[Hooks] Enabled=0` deixa todo o código original; `Disable=` aceita um endereço
(`1000cc40`), um método (`NCGDevice::DrawText`) ou uma classe (`NCGDevice::*`); `[Debug] Marker=1` desenha
marcas que só o nosso código desenha (linha amarela no topo, `#` na senha, cursor vermelho), para conferir que os
hooks estão ativos. `python tools\hook_ini.py` escreve esse `.ini` (veja `--help`).

**Standalone:** feche o jogo, volte a pasta de teste ao original (`install_test.py --restore`), guarde o
`NWindow.dll` dela e copie por cima o `build\standalone\Release\NWindow.dll`. Só esse arquivo: nenhum `.ini` nem
`.log` ao lado (o `npkcrypt.dll` do cliente continua na pasta). Para voltar, ponha o original de volta.

**Ciclo completo** (compila, smoke, juízes, standalone, instala, atualiza o banco; para no primeiro passo que
falhar):

```bat
python tools\cycle.py --system "<cliente C3>\system_nwtest" --game-source --standalone
python tools\cycle.py --system "<cliente C3>\system_nwtest" --game-source --standalone --no-install
```

O ciclo usa `build/` e copia da pasta `--system` as DLLs que o carregador precisa (`Core.dll`, `Engine.dll` e
as suas dependências) para `build/smoke_system`. O jogo dessa pasta tem de estar fechado para o passo de
instalação. `tools\port\cycle.bat` roda o ciclo com `--game-source --no-install` e depois os juízes do porte
(`verify_data.py`, `check_breakers.py`, `checks.py`); ele lê a pasta de teste de `NWD_CLIENT_SYSTEM`.

**Caminhos da máquina.** Nenhuma ferramenta tem um caminho da máquina do autor: uma pasta fora do repositório vem
da opção de linha de comando ou de uma destas variáveis (a opção vence; a ferramenta que precisa de uma e não tem
nenhuma para e diz qual):

| Variável | Aponta para | Usada por |
|---|---|---|
| `NWD_CLIENT_SYSTEM` | a pasta `system` do cliente de teste (uma cópia: a instalação escreve nela) | `--system` padrão de `install_test.py`, `cycle.py`, `smoke_standalone.py`, `difftest.py`; `hook_ini.py`; `--log` padrão de `verify_hooks.py` e `tools/port/verify_args.py`; `Engine.u`/`Core.u` de `ue2_layouts.py` |
| `NWD_STOCK_SYSTEM` | (opcional) a pasta `system` com que você joga | `install_test.py` se recusa a escrever nela |
| `NWD_GHIDRA_HOME` | a instalação do Ghidra (com `support/analyzeHeadless.bat`; `GHIDRA` ainda tem precedência); `GHIDRA_JAVA_HOME` = o JDK 21 | `tools/ghidra/*.sh`, `tools/c2/ghidra_census.sh` |
| `NWD_C2_ROOT` | um checkout do projeto do C2 (a base do porte; o nome antigo `NW_C1_ROOT` também vale) | `tools/port/*` que comparam com a base (`gen.py`, `match.py`, `layout.py`, `eligible.py`...) |
| `NWD_C1_ROOT` | um checkout do projeto do C1 (com o seu `orig/`) | `tools/c2/compare_nwindow.py`, `ue2_console_slots.py`, `ue2_gamedata_regions.py`, `ue2_ret_diff.py`, `ue2_vtable_diff.py` |
| `NWD_C2_SYSTEM` | a pasta `system` de um cliente C2 | `tools/c2/packet_formats.py` |
| `NWD_C1_SERVER` | a pasta de deploy de um servidor C1 (páginas em `gameserver/data/html-en`) | `difftest.py`, conjunto de páginas `server` (pulado sem ela) |

## Os juízes

O ciclo (`tools/cycle.py`) roda estes, nesta ordem; "verde" é o ciclo terminar com código 0.

| Juiz | Comando | O que prova |
|---|---|---|
| smoke | `build\smoke\Release\nwd_smoke.exe build\smoke_system` | o `NWindow.dll` remendado carrega fora do jogo e todo hook dá `ok` (`NWindowDecomp.log`) |
| verify | `python tools/verify_hooks.py --log build/smoke_system/NWindowDecomp.log` | cada função nossa contra a original no assembly: bytes desempilhados no `ret`, slots de vtable e imports chamados; diferença aceita só com explicação em `db/verify_notes.csv` |
| regcheck | `python tools/regcheck.py --game-src` | nenhum chamador original depende de um registrador que a nossa função pode mudar (o original foi compilado com `/GL`) |
| standalone: declarações | `python tools/verify_standalone.py --checks source` | os nossos `NW_DECLARE_CLASS` / `NW_CHECK_VTABLE` contra a tabela de classes original |
| standalone: verify | `python tools/verify_standalone.py --dll build/standalone/Release/NWindow.dll` | PE, imports, recursos, exports, classes e vtables iguais aos do original, sem rodar (diferenças aceitas em `tests/standalone/notes.csv`) |
| standalone: smoke | `python tools/smoke_standalone.py --dll build/standalone/Release/NWindow.dll` | carregado como a engine carrega: 309 classes registradas, saída igual à do original |
| db | `python tools/build_db.py --game-src` | regenera `db/functions.csv` e `docs/PROGRESSO.md` |

Juízes do porte (`tools\port\cycle.bat`, depois do ciclo): `tools/port/verify_data.py` (constantes, deslocamentos
de campo, strings e globais que cada função toca; notas em `db/verify_data_notes.csv`),
`tools/port/check_breakers.py` (nenhum gancho alcança um `NW_UNPORTED`) e `tools/port/checks.py` (valor de
retorno, argumentos constantes, quadros de pilha, tamanhos, CRT: imprime `CHECKS: ALL OK`). Os testes
diferenciais sob o unicorn (`tools/port/emu/t_*.py`) rodam a função original e a nossa lado a lado. O
`difftest.py` (`tests/golden/`) ainda tem os casos do C1 e se recusa a rodar na imagem do C3 (`--difftest` do
ciclo fica desligado).

## Pastas

```
orig/          binários do seu cliente (fora do git; md5 em orig/README.md)
analysis/      fatos extraídos do original (classes, EH, vtables; as vtables exportadas do Core/Engine; layouts do
               Engine.u/Core.u; formatos de pacote do Engine.dll do C3); analysis/c2/ = os mesmos dados do C2
               (a base do porte); analysis/ghidra/ = export do Ghidra (gerado, fora do git)
db/            banco de funções (functions.csv gerado) + nomes, notas e verificações dados à mão; db/c1/, db/c2/ = referência
include/ue2/   declarações do Core.dll/Engine.dll do C3, conferidas contra os exports
include/nwindow/, src/nwindow/    a source decompilada, uma classe por arquivo
src/re/        runtime do modo hook (instalação dos hooks, log, ini, compatibilidade do CRT)
src/standalone/                   IMPLEMENT_CLASS / IMPLEMENT_PACKAGE, globais, GameGuard no-op, NWindow.def
tests/         saídas de referência (golden) e listas/notas do standalone
third_party/ue_import/            .def dos exports do Core/Engine (viram .lib no build)
tools/         análise, geração, instalação, juízes, ciclo; tools/port/ = o porte C2 -> C3 e os seus juízes;
               tools/c3/ = formatos de pacote do cliente C3; tools/c2/ = ferramentas herdadas do C2
docs/          plano, convenções, progresso e notas técnicas
```

## Guias

- [docs/PLANO.md](docs/PLANO.md) — plano, método e fases; [docs/PLANO_C3.md](docs/PLANO_C3.md) — o que é do C3
  (ondas do porte, decisões); [docs/CONVENCOES.md](docs/CONVENCOES.md) — regras de código e armadilhas
- [docs/notas/C3_Standalone.md](docs/notas/C3_Standalone.md) — a fase 5 no C3 (em inglês), com o resultado;
  [docs/notas/C3_Agentes.md](docs/notas/C3_Agentes.md) — o método das ondas e as classes de bug
- [docs/MODIFICAR_INTERFACE.md](docs/MODIFICAR_INTERFACE.md) — como modificar a interface (barra de atalhos,
  botão, janela, cor de chat, linha de tooltip) · [English](docs/MODIFY_INTERFACE.md) · [Русский](docs/MODIFY_INTERFACE.ru.md)
- [docs/LLM.md](docs/LLM.md) — guia para agentes de IA que forem trabalhar neste código (em inglês)
- [docs/notas/C3_Pacotes_T1.md](docs/notas/C3_Pacotes_T1.md), [C3_Pacotes_T2.md](docs/notas/C3_Pacotes_T2.md),
  [C3_Pacotes_N.md](docs/notas/C3_Pacotes_N.md) — o que o cliente C3 lê e envia em cada pacote

## Nota legal

Projeto independente de engenharia reversa, para compatibilidade e preservação. Não é afiliado, autorizado nem
endossado pela NCSoft. *Lineage II* é marca registrada da NCSoft Corporation. **Nenhum arquivo original do
jogo está incluído** (nem DLLs, nem executáveis, nem pacotes de dados): cada usuário fornece o próprio cliente.

## Licença

[GNU General Public License v3.0](LICENSE).
