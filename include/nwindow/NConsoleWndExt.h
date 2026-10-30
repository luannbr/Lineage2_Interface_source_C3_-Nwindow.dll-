/*=============================================================================
    NConsoleWndExt.h - the stand-in structs (function holders) whose members the console's files call by name: anonymous
    methods of other classes (and of the console) that the stock keeps out of line, each hooked at its stock address in the
    file that defines it. Stage 5 (standalone): the console's calls of these functions were `StockCallAt(va, ...)`; they are
    typed calls of these members now, in both builds (the stock entry jumps to the same member in the hook build).

    Where each struct is defined today (the bodies stay there):
      * NConsoleWndMisc.cpp (sa-console-a): NControlExt, NCConsoleExt, NCVideoOptionWndExt, NCGMQuestWndExt, NCLobbyWndExt,
        NCRadarWndExt, NCRecipeBookWndExt, NCLoginServerWndExt, NCBroadcastObserverWndExt2, NCAbnormalStatusWndExt,
        NCZoneTitleWndTick (the same
        declarations, token for token; sa-integ removed its copies), and NCQuestGaraTreeWndExt's MarkSelectedQuest,
        GetTargetName, GetTargetPos, SetTargetPos and ResetTree (its own view had no base: merged here);
      * NConsoleWndEvents.cpp: NCQuestGaraTreeCtrlExt, NCQuestGaraTreeWndExt (AddQuest);
      * NLeftHelpers.cpp: NConsoleLeftFns, NItemMapView; NTemplates.cpp: NConsoleForwardFns (the same declarations).
    The copies in those .cpp files are the same definitions as these (one definition each once their owners include this
    header instead; docs/notas/C3_Standalone.md section 2b). None of them has a virtual or a data member of its own: the
    member functions' decorated names do not depend on the base, so every caller reaches the one hooked body.
=============================================================================*/
#pragma once

#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"   // NConsoleLeftFns / NConsoleForwardFns (one definition, sa-win-a)
#include "nwindow/NCConsole.h"    // NCAbnormalStatusWndExt (one definition, sa-console-a)
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCZoneTitleWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NCQuestGaraTreeCtrl.h"
#include "nwindow/NCGaraTVItem.h"
#include "nwindow/NGameData.h"

// NConsoleWndMisc.cpp's views (the windows' small anonymous methods; `this` is the window)
struct NControlExt
{
    __declspec(noinline) void EndMouseDrag();                                 // 0x1000F7B0
};
struct NCConsoleExt
{
    __declspec(noinline) INT LogoutMessenger();                               // 0x10053780
    __declspec(noinline) void ToggleChatWnd();                                // 0x10053790
};
struct NCVideoOptionWndExt
{
    __declspec(noinline) void ToggleLowerDetail();                            // 0x100B9430
};
struct NCGMQuestWndExt
{
    __declspec(noinline) void AddQuestItem(INT ItemID, INT Count);            // 0x10082840
};
struct NCLobbyWndExt
{
    __declspec(noinline) void ClearSlots();                                   // 0x1008F490
};
struct NCRadarWndExt
{
    __declspec(noinline) void RemoveTarget(INT X, INT Y, INT Z);              // 0x100DE7C0
};
struct NCRecipeBookWndExt
{
    __declspec(noinline) void SetField16C(INT Value);                         // 0x100E24B0
};
struct NCLoginServerWndExt
{
    __declspec(noinline) void SetKey(const void* Key);                        // 0x100EDAB0
};
struct NCBroadcastObserverWndExt2
{
    __declspec(noinline) INT ToggleFlag();                                    // 0x10118510
};


// the zone title's stand-in for the method the stock keeps at 0x1010b4a0 (the console's Tick calls it by address)
struct NCZoneTitleWndTick : public NCZoneTitleWnd
{
    __declspec(noinline) void Tick(FLOAT DeltaTime);   // 0x1010B4A0 (ret 4)
};

// The two quest tree classes with the functions the db has as anonymous (their headers belong to other lanes: views of the same layout
// with the members added; the hooks are installed on the members so that the stock address is entered with `this` in ecx).
// AddQuestStep and AddQuest are NConsoleWndEvents.cpp's; ResetTree is NConsoleWndMisc.cpp's (its own view of the struct)
struct NCQuestGaraTreeCtrlExt : public NCQuestGaraTreeCtrl
{
    __declspec(noinline) NCGaraTVItem* AddQuestStep(INT QuestID, INT Step, NCGaraTVItem* Node, INT bMore);   // 0x100DB7B0 (ret 0x10)
};
struct NCQuestGaraTreeWndExt : public NCQuestGaraTreeWnd
{
    __declspec(noinline) void AddQuest(INT QuestID, INT Step);                          // 0x100DBEE0 (ret 8)
    __declspec(noinline) void ResetTree();                                              // 0x100DB610
    __declspec(noinline) void MarkSelectedQuest();                                      // 0x100DB630
    __declspec(noinline) FString GetTargetName();                                       // 0x100DA340
    __declspec(noinline) FVector GetTargetPos();                                        // 0x100DA370
    __declspec(noinline) void SetTargetPos(INT X, INT Y, INT Z);                        // 0x100DA3A0
};


// NLeftHelpers.cpp's: 0x1012b460 (ret 4; anonymous in the db) - TMapBase::FindRef of the item table (the stock windows' item
// lookups call it out of line, many of ours inline it): the record stored under the item id, NULL when there is none
struct NItemMapView : public NDataMap<NViewItemData>
{
    __declspec(noinline) NViewItemData* FindByID(const INT& Key)
    {
        return FindRef(Key);
    }
};

