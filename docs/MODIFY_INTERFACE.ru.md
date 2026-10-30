# Как изменять интерфейс

[Português](MODIFICAR_INTERFACE.md) · [English](MODIFY_INTERFACE.md) · **Русский**

Практическое руководство по изменению интерфейса C3 в этих исходниках: как устроены окна, как добавить слой новых
функций (в C3 его **нет**) и пошаговые рецепты (новая панель ярлыков, кнопка, окно, цвет чата, строка подсказки).
Две существующие цели, `NWindowDecomp` (хуки) и `NWindowStandalone`, точно повторяют оригинал, и именно на них судьи
сравнивают с ним: любое изменение интерфейса идёт в **третью** цель, которую вы создаёте сами (раздел 2), и
находится за `#if NW_FEATURES`.

## 1. Как устроены окна

### Иерархия

| Класс | Файл | Роль |
|---|---|---|
| `NCObject : UObject` (0x28) | `include/nwindow/NCObject.h` | база всего; `FindTexture(TEXT("Пакет.Группа.Имя"))` загружает текстуру |
| `NCWnd` (0xC8 байт) | `include/nwindow/NCWnd.h` | любое окно: позиция (`LocalX/Y` относительно родителя, `ScreenX/Y`, `Width`, `Height` абсолютные), стиль, дочерние окна, фокус, заголовок (`FrameCtrl`), значок свёрнутого окна |
| `NControl : NCWnd` (0x1D4) | `include/nwindow/NControl.h` | любой контрол; у него есть `ID`, и он сообщает родителю через `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, ... | `include/nwindow/NCButton.h` | кнопки с обычной/нажатой текстурой (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | остальные контролы |
| `NC*Wnd` | `include/nwindow/` | игровые окна (инвентарь, чат, панель ярлыков, статус...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | владелец всех окон: регистрация, z-порядок, проверка попадания, фокус, рассылка сообщений, курсоры |
| `NConsoleWnd : UL2ConsoleWnd` (0x39E8) | `include/nwindow/NConsoleWnd.h` | консоль движка (405 слотов: сетевые события и события интерфейса); `GConsole` |
| `NCConsole` (0x228) | `include/nwindow/NCConsole.h` | игровой **рабочий стол** (`GConsole->GetDesktop()`, `GDesktop`): родитель всех игровых окон, с указателем на каждое (`ShortCutWnd` +0x138, `ChatWnd`...) |

### Создание окна или контрола

Образец из оригинального кода (например, `NCSystemMenuWnd::OnCreate` в `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
NCButton* Button = new NCButton(TEXT("L2UI_CH3.SystemMenuWnd.systemicon1"),
                                TEXT("L2UI_CH3.SystemMenuWnd.systemicon1_down"), 0);
Button->ID = NID_SYSMENU_BUTTON_C;      // то, что придёт в NWM_COMMAND родителя
Button->SetPosition(13, 29, 36, 36);    // относительно родителя
CreateChild(Button, NWS_VISIBLE);       // регистрирует, делает позицию абсолютной, шлёт WM_CREATE -> OnCreate
```

- `new` для класса пакета — это `operator new` из UE2 (объект — `UObject`, память выделяется по размеру
  зарегистрированного `UClass`).
- `CreateChild(дочернее, стиль)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + экранная позиция + `WM_CREATE`.
  Рабочий стол регистрирует игровые окна через `AddChildWindow(окно, стиль)`, `HideWindow()` и встроенное
  присоединение (`AttachInline` в `src/nwindow/NCConsole.cpp`).
- Стили (`NWS_*` в `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (отдельный список, поверх остальных), `NWS_TITLEBAR`
  (`OnCreate` создаёт заголовок `NCFrameCtrl`), `NWS_MINIMIZABLE`, `NWS_NOCLOSE` и стили C3: `NWS_DRAGABLE`,
  `NWS_CLIPTOPARENT`, `NWS_NOFOCUSNEXT`...
- Уничтожение: в `OnDestroy` владельца — `RemoveChildWindow(Дочернее->Entry); delete Дочернее;` (см.
  `NCSystemMenuWnd::OnDestroy`). `NCWnd::OnDestroy` уничтожает только заголовок и значок.

### Сообщения, отрисовка, подсказки

- Всё приходит через `WindowProc(Msg, wParam, lParam)` (слот 25) с номерами сообщений Win32; `NCWnd` распределяет
  их по виртуальным методам (номера слотов **C3**): `OnCreate` (37), `OnDestroy` (38), `ShowWindow` (44),
  `HideWindow` (45), `OnPaint(NCGDevice*)` (56), `OnDrawToolTip` (57), `OnKeyDown` (59), `OnMouseMove` (61),
  `OnLButtonDown/Up` (62/63), `OnRButtonDown` (64), `OnMove` (66, `WM_MOVE`).
- Контрол сообщает родителю через `NWM_COMMAND` (`0x501`), `wParam` = `ID` контрола; родитель обрабатывает это в
  своём `WindowProc` (пример: `NCSystemMenuWnd::WindowProc`).
- Отрисовка (`OnPaint`) использует `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture`, `DrawStretchedTex`, `DrawRect`, `DrawLine`, `DrawText(...)`, `EndPaint`, `ReleaseClip`.
  Текстуры загружаются один раз в `OnCreate` через `FindTexture` (пакеты клиента `L2UI_CH3.*` и `L2UI.*`).
- Подсказка: `OnDrawToolTip(NCGDevice*)` окна под мышью, вызывается после отрисовки.

### Консоль и рабочий стол

`NConsoleWnd` (объект, который движок создаёт по `L2.ini`) получает события сервера и игры (свои 405 слотов) и
владеет рабочим столом `NCConsole`. `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`) создаёт все игровые окна
(позиции — от размера экрана), а `NCConsole::OnDestroy` уничтожает их (`DESTROY_AT`). `NConsoleWnd::Tick`
(`src/nwindow/NConsoleWndInput.cpp`) выполняется каждый кадр.

## 2. Добавление слоя новых функций (в C3 его нет)

В C1 новые функции живут в третьей цели, `NWindowL2`, с `NW_FEATURES=1` и `src/features/` (см. проект для C1).
**Ничего из этого в этом дереве нет.** Блоки `#if NW_FEATURES` в `src/nwindow` (`NCAbnormalStatusWnd.cpp`,
`NCActionItemWnd.cpp`, `NCChatWnd.cpp`, `NCEquipItemWnd.cpp`, `NCItemWnd.cpp`, `NCommandParser.cpp`,
`NConsoleWndEvents.cpp`, `NConsoleWndInput.cpp`, `NConsoleWndLogin.cpp`, `NConsoleWndWindows.cpp`,
`NCVideoOptionWnd.cpp`, `NCVirtualWndMain.cpp`, `NCWnd.cpp`; и `include/nwindow/NCVideoOptionWnd.h`) — унаследованные от C1 точки вызова, они неактивны:
они подключают заголовки `features/*.h`, которых здесь нет. Как построить слой поверх существующих классов:

1. **Цель CMake.** В `CMakeLists.txt` — вторая объектная библиотека с теми же исходниками, что у
   `nwd_standalone_objs`, плюс `src/features/*.cpp`, с теми же флагами и `NW_FEATURES=1`, и DLL через уже
   существующую функцию: `nwd_standalone_dll_of(nwd_l2_objs NWindowL2 NWindow "${CMAKE_BINARY_DIR}/l2")`. Две
   точные цели остаются без `NW_FEATURES`.
2. **Унаследованные точки.** Прежде чем включать `NW_FEATURES`, решите судьбу каждого унаследованного блока: удалите
   его или перенесите функцию из C1 и адаптируйте к C3 (адреса, раскладки и номера слотов C1 здесь **не**
   подходят). Иначе новая цель не скомпилируется.
3. **Код функции** — в `src/features/<Функция>.cpp` + `include/features/<Функция>.h`, внутри `namespace nwf` и
   `#if NW_FEATURES`. В декомпилированном коде — только точка вызова, в `#if NW_FEATURES ... #endif` с комментарием
   `// phase 6: <Функция> ...`; точный код без неё должен читаться так же. Следите за `__LINE__` в `check()`: не
   меняйте число строк выше `check()` (оставляйте пустые строки, как в `NConsoleWndLogin.cpp`).
4. **Точки подключения**, которые вы создаёте сами: `DllMain` (`src/standalone/NWindowPackage.cpp`) — установка и
   откат того, что трогает другие модули; начало `NConsoleWnd::Tick` — работа каждый кадр (под обработчиком
   исключений, как в C1); `NCConsole::OnCreate`/`OnDestroy` — создание/уничтожение окон;
   `WindowProc`/`OnPaint`/`OnDrawToolTip` окон, которые меняет функция.
5. **Настройки**: в C3 нет механизма настроек для функций. C1 использует реестр
   (`HKCU\Software\Lineage2C1\NWindow\<Раздел>`) и никаких файлов рядом с DLL; сделайте так же со своим ключом.
6. **Новые классы** (окно со своими полями) требуют `UClass`, зарегистрированного только в новой цели, с экспортом
   `autoclass<Класс>` после оригинальных ординалов; `tools/verify_standalone.py --l2` принимает ровно те
   дополнительные классы, что перечислены в `tests/standalone/l2_extras.csv` (сейчас пусто). Макросов C1
   `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS` в этом дереве нет; образец — `NW_IMPLEMENT_CLASS` в
   `include/nwindow/NWindow.h`.
7. **Доказательство**: полный цикл зелёный (он доказывает, что режим хуков и standalone не изменились),
   `verify_standalone.py --dll build/l2/Release/NWindow.dll --l2`, затем проверка в игре на копии папки `system`.

## 3. Рецепт: дополнительная панель ярлыков (как в C5 / Interlude)

Ничего из этого рецепта в C3 не реализовано; это метод, который проект для C1 реализовал и **проверил в игре** (в
экспериментальной ветке), адаптированный к фактам C3 из этих исходников.

**Идея (из C5):** у дополнительной панели **нет своих слотов**. Это ещё один экземпляр `NCShortCutWnd`, который
показывает другую страницу той же таблицы ярлыков. В C3 таблица содержит **12 × 10 = 120** ярлыков (12 на
странице, F1..F12, 10 страниц; `new NCShortCutWnd(12, 10, 32)` в `NCConsole::OnCreate`). Слот ярлыка —
`страница × 12 + позиция`, плоский номер, который сервер хранит и присылает обратно; панель, которая просто
показывает другую страницу, ничего не меняет ни в протоколе, ни на сервере, ни в текстурах. Размер класса (0x270)
сохраняется, новый `UClass` не нужен.

**Шаги** (в цели с `NW_FEATURES`, раздел 2):

1. **Файлы функции:** `include/features/ShortCutBars.h` и `src/features/ShortCutBars.cpp`, с функциями вроде
   `ShortCutBars_Create(NCConsole* Desktop, NCShortCutWnd* Main)`, `ShortCutBars_Destroy`, `ShortCutBars_OnTick`,
   `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`, `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`.
   Состояние (панели, кнопка) хранится в глобальных переменных `.cpp`, никогда — в новых полях `NCShortCutWnd`.
2. **Создание** в `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`), сразу после основной панели (`W = new
   NCShortCutWnd(12, 10, 32)` ... `AddChildWindow(..., 0x85022)`, `HideWindow()`, встроенное присоединение): для
   каждой дополнительной панели — `new NCShortCutWnd(Main->Cols, Main->Pages, Main->IconSize)`, `SetPosition` и та
   же регистрация. Оригинальный `OnCreate` создал собственные таблицы: освободите их (`delete[] Bar->Items`,
   `delete[] Bar->Keys`) и укажите на таблицы основной панели (`Bar->Items = Main->Items`, `Bar->Keys =
   Main->Keys`). `Bar->Page` — первая показываемая страница. Скройте `RotateButton` и `PadButton` дополнительных
   панелей (поворачивает все панели основная; режим геймпада, `bPadMode`, остаётся только у неё).
3. **Уничтожение** в `NCConsole::OnDestroy`, **до** `DESTROY_AT(0x138)` (основная панель): у дополнительных панелей
   обнулите `Items` и `Keys` (они принадлежат основной; иначе оригинальный `OnDestroy` освободит их дважды), затем
   `RemoveChildWindow(Bar->Entry)` и `delete Bar`.
4. **Следовать за основной панелью** в конце `NCShortCutWnd::Tick` (`src/nwindow/NCShortCutWnd.cpp`; тик получает
   только основная): для каждой дополнительной — та же ориентация (`bHorizontal`, и `UpdateLayout()` при её смене),
   позиция (над горизонтальной панелью, слева от вертикальной, с другой стороны, если не хватает экрана; 46 × 504
   вертикально, 504 × 46 горизонтально) через `SetPosition`, и видимость только пока видна основная.
5. **Кнопка разворачивания** (по желанию): `NCPushButton`, дочерний для основной панели (`CreateChild`), с новым `ID`
   после оригинальных (у панели они идут до `0xFF000009`, `NID_SHORTCUT_PAD`), обрабатывается в ветке
   `NWM_COMMAND` метода `NCShortCutWnd::WindowProc`. В качестве графики подойдут стрелки страниц
   (`L2UI_CH3.ShortcutWnd.*`).
6. **Перетаскивание между панелями:** в `NCShortCutWnd::OnLButtonUp` оригинальная ветка, которая **удаляет**
   ярлык, брошенный за пределами панели (`GConsole->RequestShortCutDel`), сначала проверяет, не находится ли точка над
   другой панелью; если да — переносит ярлык в слот там (регистрация + удаление по тому же протоколу; обмен, если
   слот занят), а бросок на панель, но мимо слота, ничего не делает.
7. **Перетаскивание за ручку дополнительной панели двигает всю группу:** в начале `NCShortCutWnd::WindowProc`
   `WM_MOVE` дополнительной панели передаётся основной, с точкой захвата (`FrameCtrl`, `GrabX/GrabY`), поправленной
   на смещение дополнительной панели (по вычисленной раскладке, а не по текущим позициям, которые отстают на кадр);
   без этого `NCWnd::Snap` тянет группу и мышь «прыгает».
8. **Графика:** проверьте фоны `L2UI_CH3.ShortcutWnd.shortcut_backv` / `shortcut_back` (`UpdateLayout`); если значок
   кнопки поворота нарисован прямо в текстуре, закройте эту ячейку на дополнительных панелях в конце
   `NCShortCutWnd::OnPaint` (`DrawTexture` между `BeginPaint`/`EndPaint`).

**Полезно знать:** подсказка, использование по клику, автоиспользование соулшотов C3 (`bAutoShot`, у каждого слота,
в общей таблице) и перетаскивание из инвентаря/умений работают без изменений, потому что это тот же класс и всё
вычисляется от `Page`. Клавиши (F1..F12, страницы, геймпад) остаются на основной панели. Фильтр пакетов сервера
может отбрасывать слишком частые регистрации ярлыков. Проблемы с позицией и графикой видны только в игре.

## 4. Рецепт: новая кнопка в существующем окне

Пример: ещё одна кнопка в системном меню (`NCSystemMenuWnd`; в C3 в нём 7 значков 36 × 36 при x = 13, каждые
37 пикселей начиная с y = 29, создаются через `CreateMenuButton`).

1. `include/features/MyButton.h` / `src/features/MyButton.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MyButton_Create(NCWnd* Owner)`, `MyButton_OnCommand(NCWnd* Owner, WPARAM ID)`, `MyButton_Destroy(NCWnd* Owner)`;
   указатель на кнопку — в глобальной переменной `.cpp`.
2. Создайте её в конце `NCSystemMenuWnd::OnCreate`: `#if NW_FEATURES` / `// phase 6: MyButton ...` /
   `nwf::MyButton_Create(this);` / `#endif`. Внутри: `new NCButton(обычная, нажатая, 0)`, `ID`, который не
   использует ни один контрол этого окна (см. его перечисление `NID_SYSMENU_*`, `0xF0F0xx`), `SetPosition` в
   пределах окна, `Owner->CreateChild(Button, NWS_VISIBLE)`.
3. Обработайте нажатие в ветке `NWM_COMMAND` метода `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MyButton_OnCommand(this, wParam)) return 1;` (в `#if NW_FEATURES`).
4. Уничтожьте её в `NCSystemMenuWnd::OnDestroy`, до `NCWnd::OnDestroy()`: `RemoveChildWindow(Button->Entry);
   delete Button;`.
5. Если окно должно вырасти, измените его `SetPosition` в `NCConsole::OnCreate` и отрисовку фона
   (`L2UI_CH3.SystemMenuWnd.systemmenu_back`) в его `OnPaint` — всегда в `#if NW_FEATURES`, оставляя оригинальную
   ветку нетронутой.

## 5. Рецепт: новое окно

1. Окно со своими полями или виртуальными методами — это новый класс, зарегистрированный только в цели с функциями
   (раздел 2, пункт 6): `class NCMyWnd : public NCWnd { ... };` со своим `UClass` и строкой в
   `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`).
2. Переопределите нужные виртуальные методы: `OnCreate` (сначала вызовите `NCWnd::OnCreate()`, загрузите текстуры
   через `FindTexture`, создайте заголовок — `new NCFrameCtrl(1, 0, 0)`, как в системном меню, — и контролы),
   `OnDestroy` (уничтожьте дочерние окна, затем `NCWnd::OnDestroy()`), `WindowProc` (`NWM_COMMAND`), `OnPaint`.
3. Создавайте его на рабочем столе, когда нужно (`SetPosition`, `GDesktop` как родитель в `CreateChild(this,
   NWS_VISIBLE)`, `HideWindow()`), и показывайте через `ShowWindow()` + `GDesktop->BringChildToTop(Entry)`.
4. Образцы в этих исходниках: `NCSystemMenuWnd` (маленькое, с кнопками), `NCAbnormalStatusWnd` (сетка значков с
   подсказкой), окна личной торговли (`NCPrivateShopWnd`, списки с прокруткой).

## 6. Рецепт: новый цвет чата

Сервер не присылает цвет: он присылает **тип** сообщения (`Say2`), а клиент выбирает цвет в `NConsoleWnd::Say2`
(0x10073630, `src/nwindow/NConsoleWndEvents.cpp`): цепочка `if/else` по типу (`SAY2_Shout` оранжевый
`0xFFFF7200`, `SAY2_Party` зелёный, `SAY2_Pledge`, `SAY2_Tell`, `SAY2_Market`, `SAY2_Ally`, петиции, объявления;
типы 11 и 14 появились в C3), которая заканчивается светло-серым `else` (`0xFFDCDCDC`). У `NConsoleWnd::NpcSay`,
сразу ниже, своя цепочка.

1. Тип, которого C3 не знает: добавьте в `#if NW_FEATURES` ветку перед последним `else` (или вызовите там функцию
   `nwf::ChatColors_Find(Type, &Color)`), с цветом в формате ARGB (например, `0xFF408CFF`).
2. Строка по-прежнему попадает в чат как обычная (`GetDesktop()->Say(..., Color, Type)`). Не используйте повторно
   типы с побочными эффектами (`SAY2_Tell` добавляет говорящего в список личных сообщений).
3. Изменить цвет типа, который C3 **уже** раскрашивает, — значит изменить точную ветку: делайте это под
   `#if NW_FEATURES` внутри `Say2`, сохраняя оригинальный цвет в ветке `#else`.

## 7. Рецепт: новая строка в подсказке предмета

Полную подсказку предмета рисуют функции из `include/nwindow/NItemToolTip.h`, которыми пользуются сетки предметов:
`SetTexts`/`SetStoreTexts` заполняют `ToolTipName` и `ToolTipText` (текст, который задаёт **размер** рамки), а
`DrawLines` рисует строки с шагом 13 пикселей (`DrawTypeLines`, затем `DrawTailLines`, который заканчивается
описанием).

1. Добавьте строку в обоих местах, в `#if NW_FEATURES`: в текст, задающий размер (чтобы рамка выросла), и в
   отрисовку (например, в `DrawTailLines` перед описанием, через `DrawLabel`/`DrawValue` и `Y += 13`).
2. Эти функции — шаблоны, которые каждая сетка инстанцирует с разными параметрами (инвентарь, кукла персонажа,
   обмен, склад, личные магазины, питомец): проверьте каждую копию, которую меняете.
3. Для подсказок других окон образец — `NCAbnormalStatusWnd::OnDrawToolTip` (`src/nwindow/NCAbnormalStatusWnd.cpp`):
   вычислите дополнительную высоту до отрисовки рамки и нарисуйте строку после неё.

## 8. Цикл сборки и проверки

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "<клиент C3>\system_nwtest" --game-source --standalone --no-install
python tools\verify_standalone.py --dll build\l2\Release\NWindow.dll --l2
```

1. Собирайте цель с функциями (имя — то, которое вы дали ей в разделе 2), пока не останется ошибок и новых
   предупреждений.
2. Запустите полный цикл: он доказывает, что режим хуков и точный standalone не изменились (судьи сравнивают их с
   оригиналом). Затем `verify_standalone --l2` на DLL с функциями.
3. Проверьте в игре: закройте игру, сохраните `NWindow.dll` из **копии** папки `system` и скопируйте поверх свою
   DLL. Цикл не рисует интерфейс: позиция, графика и перетаскивание видны только в игре.

## 9. Правила безопасности

- Никогда не добавляйте поля или новые виртуальные слоты в оригинальный класс: его размер зарегистрирован в движке,
  а оригинальный код обращается к нему и к его vtable по смещениям. Переопределять слот, который класс уже
  наследует, можно (внутри `#if NW_FEATURES`). Состояние функции — в глобальных переменных `.cpp` или в новом
  классе.
- В декомпилированном коде — только точка вызова, в `#if NW_FEATURES` с `// phase 6: ...`; оригинальная ветка
  остаётся как есть, а число строк выше `check()` не меняется.
- Любой патч другого модуля во время работы сначала проверяет байты и отключает функцию, если они не совпадают.
- Настройки — в реестре, никогда не в файлах рядом с DLL.
- Тот же протокол, что у оригинального клиента: сервер не меняется; соблюдайте ограничения его фильтра пакетов.
- Проверяйте на копии клиента, никогда в папке, из которой играете; ни один бинарник игры не попадает в репозиторий.
- Небольшие коммиты, по одной теме, с зелёным циклом.
