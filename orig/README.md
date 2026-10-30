# Binários de referência (fora do git) / Reference binaries (not in git)

**PT.** Esta pasta não vem com nenhum arquivo do jogo: nenhum binário da NCSoft é publicado. Copie os
arquivos abaixo do **seu próprio** cliente Lineage II C3 (*Rise of Darkness*, pasta `system`) para cá. O build
do modo hook (`NWindowDecomp`) não precisa deles; o `NWindowStandalone` precisa de `orig/NWindow.dll` (os 7
cursores coloridos são extraídos dele na compilação); as ferramentas de verificação e geração precisam do
`NWindow.dll`, do `Core.dll` e do `Engine.dll`. O `NWindow.dll` tem de ter o md5 da tabela: toda ferramenta
confere (`tools/nwd/image.py: STOCK_MD5`).

**EN.** This folder ships without any game file: no NCSoft binary is published. Copy the files below from
**your own** Lineage II C3 (*Rise of Darkness*) client (its `system` folder) into it. The hook-mode build
(`NWindowDecomp`) does not need them; `NWindowStandalone` needs `orig/NWindow.dll` (the 7 colour cursors are
extracted from it at build time); the verification and generation tools need `NWindow.dll`, `Core.dll` and
`Engine.dll`. The `NWindow.dll` must have the md5 in the table: every tool checks it
(`tools/nwd/image.py: STOCK_MD5`).

| Arquivo / File | md5 | TimeDateStamp (PE) | Uso / Use |
|---|---|---|---|
| `NWindow.dll` | `f0362880cdec6eedc6b8f860a0bc9945` | `0x4329071F` (15/09/2005) | **o** binário que o projeto reconstrói / **the** binary this project rebuilds (2.732.052 bytes) |
| `Core.dll` | `eeae5768a1a8b88dda87e2c48cf744ca` | `0x43290661` (15/09/2005) | declarações / declarations: `include/ue2`, `third_party/ue_import/Core.def` |
| `Engine.dll` | `9ad6c90219110e94433d1357a6fe1a49` | `0x4331F89F` (22/09/2005) | idem (`Engine.def`; mais de 8.192 exports / over 8,192 exports: `tools/engine_vtables.py`, `tools/gen_import_defs.py`); `tools/c3/*` (formatos de pacote / packet formats) |
| `WinDrv.dll` | `bad3cc260cfd56729084008dc109bfc6` | `0x4327C98A` (14/09/2005) | virtuais do `UViewport` / `UViewport` virtuals (`tools/gen_engine_decls.py`) |
| `ALAudio.dll` | `ae90d95dcdb976db804ef903089a1a87` | `0x4327C990` (14/09/2005) | puras do `UAudioSubsystem` / `UAudioSubsystem` pure virtuals (`tools/gen_engine_decls.py`) |
| `D3DDrv.dll` | `4d348a854f9f904cd2d97c32b9abf8cd` | `0x4327C980` (14/09/2005) | puras do `URenderDevice` / `URenderDevice` pure virtuals (`tools/gen_engine_decls.py`) |
| `Window.dll`, `IpDrv.dll`, `npkcrypt.dll`, `l2.exe` | (do seu cliente / from your client) | | só consulta / reference only |

O cliente do C3 **não tem `Network.dll`**: o código de rede está no `Engine.dll`. / The C3 client has **no
`Network.dll`**: the network code lives in `Engine.dll`.

**Nota sobre a base / About the base.** O PE foi compilado em 15/09/2005 (o `Core.dll` no mesmo dia, o
`Engine.dll` em 22/09/2005), mas o arquivo vem de um pacote de cliente da comunidade; sem uma cópia original
para comparar, não dá para afirmar que ele não tem remendos. O projeto reconstrói o binário com o md5 acima. /
The PE was built on 15/09/2005, but the file comes from a community client package; with no pristine copy to
compare, it cannot be proved free of patches. The project rebuilds the binary with the md5 above.

Para recriar a pasta / To fill the folder (`cmd`, from the repository root):

```bat
set C3=<your C3 client>\system
for %f in (NWindow Core Engine WinDrv ALAudio D3DDrv) do copy "%C3%\%f.dll" orig\
```

A pasta de teste (`--system` / `NWD_CLIENT_SYSTEM` de `tools/install_test.py`, `tools/cycle.py`,
`tools/smoke_standalone.py`) é uma **cópia** da `system` inteira; nunca instale na pasta com que você joga. /
The test folder is a **copy** of the whole `system` folder; never install into the folder you play from:

```bat
xcopy /E /I /Y "%C3%" "<your C3 client>\system_nwtest"
```
