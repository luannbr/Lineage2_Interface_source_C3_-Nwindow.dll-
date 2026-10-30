# NWindow.dll для Lineage II C3 — исходный код, восстановленный на C++

[Português](README.md) · [English](README.en.md) · **Русский**

Пофункциональное восстановление `NWindow.dll` клиента Lineage II **Chronicle 3 (Rise of Darkness)** — нативной
DLL интерфейса (окна, контролы, HTML диалогов NPC, IME, чат, консоль, мессенджер, replay). Цель — сборка от
15.09.2005 (TimeDateStamp `0x4329071F`, Visual C++ 7.0 13.00.9466, md5 `f0362880…`, 309 классов, 311 экспортов,
10 992 функции). Один и тот же исходный код собирается двумя способами:

| Цель CMake | Результат | Что это |
|---|---|---|
| `NWindowDecomp` | `build/bin/Release/NWindowDecomp.dll` | **режим хуков**: работает рядом с оригинальным `NWindow.dll` и заменяет каждую уже восстановленную функцию переходом `jmp` на нашу |
| `NWindowStandalone` | `build/standalone/Release/NWindow.dll` | **standalone**: весь `NWindow.dll` только из нашего кода; игра загружает его вместо оригинала |

Это третий проект серии: он ответвился от проекта для C2 (который, в свою очередь, ответвился от проекта для C1),
с теми же инструментами и тем же методом, направленными на бинарник C3; исходники C2 перенесены на C3 и проверены
функция за функцией.

## Как это работает

**Режим хуков.** Установщик (`tools/install_test.py`) добавляет в оригинальный `NWindow.dll` импорт
`NWindowDecomp.dll`. Наша DLL загружается раньше, чем выполняется любой код оригинала, и ставит `jmp` на вход
каждой восстановленной функции (макрос `NW_HOOK_*` рядом с каждым определением, `src/re/NwHook.h`). Всё остальное
остаётся оригинальным кодом. Так каждая новая функция проверяется в настоящей игре, а любую из них можно отключить
в `NWindowDecomp.ini` (`[Hooks] Disable=`), чтобы локализовать ошибку.

**Standalone (фаза 5).** При `NW_HOOK_MODE=0` макросы хуков пусты, и исходники сами собирают `NWindow.dll`:
309 объектов `UClass` (`src/standalone/Classes.gen.cpp`), глобальные переменные (`NwGlobals.cpp`, сгенерированы
из образа C3), экспорты с оригинальными ординалами (`src/standalone/NWindow.def`) и 7 цветных курсоров. Движок
(`Core.dll`/`Engine.dll`) загружает нашу DLL так же, как загружал оригинал. **GameGuard** (`NPG*`, статическая
библиотека nProtect внутри оригинального образа) не переносится: в standalone это документированная заглушка
(`include/nwindow/NGameGuard.h`, `src/standalone/NwGameGuard.cpp`), а на запрос GameGuard, который движок
отправляет консоли, возвращается ответ (нули, без GameMon), чтобы вход в игру продолжился
(`NConsoleWnd::GameGuardQueryX`, `src/nwindow/NConsoleWndC2.cpp`; подробности в
[docs/notas/C3_Standalone.md](docs/notas/C3_Standalone.md), раздел 2c). NKeyCrypt (`npkcrypt.dll`) — настоящий
код, загружаемый по ординалам.

Слоя новых функций (фаза 6) в C3 нет: блоки `#if NW_FEATURES` в исходниках — унаследованные от C1 точки вызова,
они неактивны (ни одна цель не определяет `NW_FEATURES`, а их заголовков в этом дереве нет). Как добавить такой
слой — в [docs/MODIFY_INTERFACE.ru.md](docs/MODIFY_INTERFACE.ru.md).

## Состояние

- Режим хуков: готово **4 970 из 6 534** функций (**93,03 %** байтов кода). 87 оставшихся без причины — это
  GameGuard (84 `NPG*` + 3 безымянные функции той же библиотеки), вне рамок проекта по решению владельца;
  остальные функции без хука в standalone генерирует компилятор (*scalar deleting destructors*,
  `InternalConstructor`, статическая инициализация). Сборка с 4 970 хуками проверена владельцем в игре
  (18.10.2026). Актуальный подсчёт: [docs/PROGRESSO.md](docs/PROGRESSO.md) (генерирует `tools/build_db.py`).
- Судьи режима хуков в последнем цикле: `hooks 4970/4970, 0 errors`, 4 738 ok + 232 объяснённых, 0 без
  объяснения, `regcheck` 0 проблем, `CHECKS: ALL OK`.
- Standalone (завершён 29–30.10.2026): линкуется без неразрешённых символов (2 198 016 байт), проходит
  `verify_standalone.py` (PE, импорты, ресурсы, 315 экспортов, 310 классов, vtable) и `smoke_standalone.py` (те же
  438 строк вывода, что и у оригинала) и **загружается в игре** вместо оригинала.
- Мессенджер (MSN) перенесён полностью, но доказан только структурно (судьи + чтение ассемблера): сервера
  мессенджера для проверки нет.
- Технические заметки (`docs/`) написаны на португальском (некоторые — на английском); комментарии в коде — на
  английском.

## Требования

- Windows, **Visual Studio 2022** с C++ для x86 (MSVC v143), **CMake ≥ 3.20**;
- **Python 3** с `pefile` и `capstone` (`pip install pefile capstone`): нужен сборке standalone (извлечение
  курсоров) и всем инструментам; `unicorn` (`pip install unicorn`) — для дифференциальных тестов в
  `tools/port/emu/`;
- **ваш собственный клиент Lineage II C3**. В репозитории нет ни одного файла игры: скопируйте в `orig/` файлы из
  [orig/README.md](orig/README.md) (у `NWindow.dll` должен быть указанный там md5);
- для судей полного цикла (`verify_hooks.py`, `regcheck.py`, `verify_standalone.py`, `build_db.py`,
  `tools/port/*`): экспорт Ghidra в `analysis/ghidra/`, создаётся командой `bash tools/ghidra/run_full.sh`
  (Ghidra 11.3.2 и JDK 21; пути в переменных `NWD_GHIDRA_HOME` и `GHIDRA_JAVA_HOME`).

## Сборка

В *Developer Command Prompt* x86 для VS 2022 (или после `vcvars32.bat`), в корне репозитория:

```bat
cmake -S . -B build -A Win32 -DNWD_GAME_SOURCES=ON
cmake --build build --config Release
cmake --build build --config Release --target NWindowStandalone
```

`-DNWD_GAME_SOURCES=ON` включает в сборку игровой код (`src/nwindow`); без него (историческое значение по
умолчанию, `OFF`) собирается только среда хуков (`src/re`), ни одна функция не заменяется, а standalone не
линкуется. Первый `--build` собирает DLL режима хуков и тестовые инструменты (`nwd_smoke`, `nwd_difftest`) и не
требует `orig/`. `NWindowStandalone` требует `orig/NWindow.dll`. Цель `NWindowStandaloneProbe` (линковка с
`/FORCE:UNRESOLVED`) служит только для диагностики, и её **никогда** нельзя загружать.

## Установка и проверка

Используйте **копию** папки `system` клиента (например, `system_nwtest`), а не ту, из которой вы играете.

**Режим хуков:**

```bat
python tools\install_test.py --system "<клиент C3>\system_nwtest"
python tools\install_test.py --system "<клиент C3>\system_nwtest" --restore
```

Первая команда записывает `NWindow.dll`, пересобранный из `orig/NWindow.dll` с дополнительным импортом,
`NWindowDecomp.dll` и `NWindowDecomp.ini` (прежний `NWindow.dll` сохраняется как `NWindow.dll.pre-decomp-bak`);
вторая возвращает то, что было. `NWindowDecomp.log` в этой папке перечисляет все установленные хуки. В `.ini`:
`[Hooks] Enabled=0` оставляет весь оригинальный код; `Disable=` принимает адрес (`1000cc40`), метод
(`NCGDevice::DrawText`) или класс (`NCGDevice::*`); `[Debug] Marker=1` рисует метки, которые рисует только наш
код (жёлтая линия сверху, `#` в пароле, красный курсор ввода), чтобы убедиться, что хуки работают.
`python tools\hook_ini.py` записывает этот `.ini` (см. `--help`).

**Standalone:** закройте игру, верните тестовую папку к оригиналу (`install_test.py --restore`), сохраните её
`NWindow.dll` и скопируйте поверх него `build\standalone\Release\NWindow.dll`. Только этот файл: ни `.ini`, ни
`.log` рядом (`npkcrypt.dll` клиента остаётся в папке). Чтобы вернуться, положите оригинал обратно.

**Полный цикл** (сборка, smoke, судьи, standalone, установка, обновление базы; останавливается на первом шаге с
ошибкой):

```bat
python tools\cycle.py --system "<клиент C3>\system_nwtest" --game-source --standalone
python tools\cycle.py --system "<клиент C3>\system_nwtest" --game-source --standalone --no-install
```

Цикл использует `build/` и копирует из папки `--system` библиотеки, нужные загрузчику (`Core.dll`, `Engine.dll` и
их зависимости), в `build/smoke_system`. Для шага установки игра из этой папки должна быть закрыта.
`tools\port\cycle.bat` запускает цикл с `--game-source --no-install`, а затем судей переноса (`verify_data.py`,
`check_breakers.py`, `checks.py`); тестовую папку он берёт из `NWD_CLIENT_SYSTEM`.

**Пути машины.** Ни один инструмент не содержит путей машины автора: папка вне репозитория задаётся опцией
командной строки или одной из этих переменных (опция важнее; инструмент, которому нужна папка и который её не
получил, останавливается и говорит, какая нужна):

| Переменная | Указывает на | Кто использует |
|---|---|---|
| `NWD_CLIENT_SYSTEM` | папку `system` тестового клиента (копию: установка пишет в неё) | `--system` по умолчанию для `install_test.py`, `cycle.py`, `smoke_standalone.py`, `difftest.py`; `hook_ini.py`; `--log` по умолчанию для `verify_hooks.py` и `tools/port/verify_args.py`; `Engine.u`/`Core.u` для `ue2_layouts.py` |
| `NWD_STOCK_SYSTEM` | (необязательно) папку `system`, из которой вы играете | `install_test.py` отказывается в неё писать |
| `NWD_GHIDRA_HOME` | установку Ghidra (с `support/analyzeHeadless.bat`; `GHIDRA` по-прежнему важнее); `GHIDRA_JAVA_HOME` = JDK 21 | `tools/ghidra/*.sh`, `tools/c2/ghidra_census.sh` |
| `NWD_C2_ROOT` | копию проекта для C2 (базовое дерево переноса; старое имя `NW_C1_ROOT` тоже работает) | инструменты `tools/port/*`, сравнивающие с базой (`gen.py`, `match.py`, `layout.py`, `eligible.py`...) |
| `NWD_C1_ROOT` | копию проекта для C1 (с его `orig/`) | `tools/c2/compare_nwindow.py`, `ue2_console_slots.py`, `ue2_gamedata_regions.py`, `ue2_ret_diff.py`, `ue2_vtable_diff.py` |
| `NWD_C2_SYSTEM` | папку `system` клиента C2 | `tools/c2/packet_formats.py` |
| `NWD_C1_SERVER` | папку развёртывания сервера C1 (страницы в `gameserver/data/html-en`) | `difftest.py`, набор страниц `server` (без неё пропускается) |

## Судьи

Цикл (`tools/cycle.py`) запускает их в таком порядке; «зелёный» — значит, цикл завершился с кодом 0.

| Судья | Команда | Что доказывает |
|---|---|---|
| smoke | `build\smoke\Release\nwd_smoke.exe build\smoke_system` | пропатченный `NWindow.dll` загружается вне игры, и каждый хук сообщает `ok` (`NWindowDecomp.log`) |
| verify | `python tools/verify_hooks.py --log build/smoke_system/NWindowDecomp.log` | каждая наша функция против оригинальной на уровне ассемблера: байты, снимаемые при `ret`, слоты vtable и вызываемые импорты; разница принимается только с объяснением в `db/verify_notes.csv` |
| regcheck | `python tools/regcheck.py --game-src` | ни один оригинальный вызывающий код не полагается на регистр, который может изменить наша функция (оригинал собран с `/GL`) |
| standalone: объявления | `python tools/verify_standalone.py --checks source` | наши `NW_DECLARE_CLASS` / `NW_CHECK_VTABLE` против оригинальной таблицы классов |
| standalone: verify | `python tools/verify_standalone.py --dll build/standalone/Release/NWindow.dll` | PE, импорты, ресурсы, экспорты, классы и vtable совпадают с оригиналом, без запуска (принятые различия — в `tests/standalone/notes.csv`) |
| standalone: smoke | `python tools/smoke_standalone.py --dll build/standalone/Release/NWindow.dll` | загружен так, как загружает движок: 309 классов зарегистрированы, вывод совпадает с оригиналом |
| db | `python tools/build_db.py --game-src` | заново создаёт `db/functions.csv` и `docs/PROGRESSO.md` |

Судьи переноса (`tools\port\cycle.bat`, после цикла): `tools/port/verify_data.py` (константы, смещения полей,
строки и глобальные переменные, которые трогает каждая функция; заметки в `db/verify_data_notes.csv`),
`tools/port/check_breakers.py` (ни один хук не доходит до `NW_UNPORTED`) и `tools/port/checks.py` (возвращаемые
значения, константные аргументы, кадры стека, размеры, CRT; печатает `CHECKS: ALL OK`). Дифференциальные тесты под
unicorn (`tools/port/emu/t_*.py`) запускают оригинальную функцию и нашу бок о бок. `difftest.py` (`tests/golden/`)
пока содержит случаи из C1 и отказывается работать на образе C3 (`--difftest` в цикле выключен).

## Структура

```
orig/          бинарники вашего клиента (не в git; md5 в orig/README.md)
analysis/      факты, извлечённые из оригинала (классы, EH, vtable; экспортированные vtable Core/Engine; раскладки
               Engine.u/Core.u; форматы пакетов Engine.dll C3); analysis/c2/ = те же данные для C2 (база переноса);
               analysis/ghidra/ = экспорт Ghidra (генерируется, не в git)
db/            база функций (functions.csv генерируется) + имена, заметки и проверки, заданные вручную; db/c1/, db/c2/ — для справки
include/ue2/   объявления Core.dll/Engine.dll C3, сверенные с экспортами
include/nwindow/, src/nwindow/    декомпилированный код, один класс на файл
src/re/        среда режима хуков (установка хуков, лог, ini, совместимость CRT)
src/standalone/                   IMPLEMENT_CLASS / IMPLEMENT_PACKAGE, глобальные переменные, заглушка GameGuard, NWindow.def
tests/         эталонные выводы (golden) и списки/заметки standalone
third_party/ue_import/            .def экспортов Core/Engine (при сборке становятся .lib)
tools/         анализ, генерация, установка, судьи, цикл; tools/port/ = перенос C2 -> C3 и его судьи;
               tools/c3/ = форматы пакетов клиента C3; tools/c2/ = инструменты, унаследованные от C2
docs/          план, соглашения, прогресс и технические заметки
```

## Руководства

- [docs/PLANO.md](docs/PLANO.md) — план, метод и фазы; [docs/PLANO_C3.md](docs/PLANO_C3.md) — особенности C3
  (волны переноса, решения); [docs/CONVENCOES.md](docs/CONVENCOES.md) — правила кода и ловушки (на португальском)
- [docs/notas/C3_Standalone.md](docs/notas/C3_Standalone.md) — фаза 5 для C3 (на английском), с результатом;
  [docs/notas/C3_Agentes.md](docs/notas/C3_Agentes.md) — метод волн и классы ошибок
- [docs/MODIFY_INTERFACE.ru.md](docs/MODIFY_INTERFACE.ru.md) — как изменять интерфейс (панель ярлыков, кнопка,
  окно, цвет чата, строка подсказки) · [Português](docs/MODIFICAR_INTERFACE.md) · [English](docs/MODIFY_INTERFACE.md)
- [docs/LLM.md](docs/LLM.md) — руководство для ИИ-агентов, работающих с этим кодом (на английском)
- [docs/notas/C3_Pacotes_T1.md](docs/notas/C3_Pacotes_T1.md), [C3_Pacotes_T2.md](docs/notas/C3_Pacotes_T2.md),
  [C3_Pacotes_N.md](docs/notas/C3_Pacotes_N.md) — что клиент C3 читает и отправляет в каждом пакете

## Юридическое примечание

Независимый проект обратной разработки для совместимости и сохранения. Он не связан с NCSoft, не авторизован и не
одобрен ею. *Lineage II* — зарегистрированная торговая марка NCSoft Corporation. **Оригинальные файлы игры не
включены** (ни DLL, ни исполняемые файлы, ни пакеты данных): каждый пользователь использует собственный клиент.

## Лицензия

[GNU General Public License v3.0](LICENSE).
