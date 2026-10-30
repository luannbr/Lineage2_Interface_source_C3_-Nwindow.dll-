#pragma once
// NCNpcViewrWnd - the NPC viewer (UClass "CNpcViewrWnd", 0x2750), a developer tool the console
// builds for its pawn viewer mode (NConsoleWnd::InitializePawnViewer, the console's
// PawnViewerWnd[1]): the item table in a grid, the items whose name holds the text searched in a second
// one; a right click (or a double click) on an item dresses the pawn shown with it. Beside it a
// top-level NCNpcView_CharacterSelectWnd lists the NPC table (NPCs and monsters, by name or by
// mesh): a double click turns the player's pawn into that NPC, "Summon" spawns it next to the
// player; the pawn's animations play from a combo box and two edit lines set its speed.
//
// NCNpcView_CharacterSelect_List - that NPC list (UClass "CNpcView_CharacterSelect_List",
// 0x1E8, a debug list): each line's parameter is its NViewNpcData.
//
// NCNpcView_InvenItemWnd - the item grids (UClass "CNpcView_InvenItemWnd", 0x13C): nothing
// can be dragged.

#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NViewerData.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCCheckBox;
class NCComboBox;
class NCEditBox;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids of the viewer's controls (names ours)
enum
{
    NID_NPCVIEW_FRAME = 0xF2,
    NID_NPCVIEW_SEARCH = 0xFC,
    NID_NPCVIEW_SEARCHEDIT = 0xFD,         // new in C2
    NID_NPCVIEW_TAB = 0xFE001A,
    NID_NPCVIEW_SELECT_FRAME = 0xF3,
    NID_NPCVIEW_SELECT_SEARCHEDIT = 0xF4,
    NID_NPCVIEW_SELECT_SEARCH = 0xF5,
    NID_NPCVIEW_SELECT_RESET = 0xF6,
    NID_NPCVIEW_SELECT_RATE = 0xF7,
    NID_NPCVIEW_SELECT_SPEED = 0xF8,
    NID_NPCVIEW_SELECT_APPLY = 0xF9,
    NID_NPCVIEW_SELECT_SUMMON = 0xFA,
    NID_NPCVIEW_SELECT_MESHNAME = 0xFB,
    NID_NPCVIEW_SELECT_SPAWNTEST = 0x100,  // new in C2 (0xFF); C3: 0x100 (0xFF, 0x101 .. 0x104 are edit lines)
    NID_NPCVIEW_SELECT_ANIM = 0x1F05,
};

class NCNpcView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCNpcView_InvenItemWnd, NCInvenItemWnd, 0, 0x10294288)

    NCNpcView_InvenItemWnd();                                        // 0x100b5090
    NCNpcView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100b5130 (unnamed in the db)
    // ~NCNpcView_InvenItemWnd                                       // 0x100b50e0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10097bc0 (also the
                                                                     //   skill viewer's grids')
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 1; } // 61 0x100b1e70 (folded)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; } // 63 0x1007d6b0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x100b25d0
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100b1ea0
};
NW_ASSERT_SIZE(NCNpcView_InvenItemWnd, 0x164);

class NCNpcView_CharacterSelect_List : public NCDebugWnd
{
    NW_DECLARE_CLASS(NCNpcView_CharacterSelect_List, NCDebugWnd, 0, 0x10293318)

    NCNpcView_CharacterSelect_List();                                // 0x100b5270
    NCNpcView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines); // 0x100b5310
    // ~NCNpcView_CharacterSelect_List                               // 0x100b52c0

    // 24, 61, 67, 68 and 69 are also NCPawnView_CharacterSelect_List's
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b2380
    virtual INT OnCreate();                                          // 37 0x100b23a0
    virtual INT OnDestroy();                                         // 38 0x100b24a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b2420
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100b2f60 (new in C3: the line's names go to
                                                                     //   the selection window's five text lines)
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100b2540
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 72 0x100b2970
    virtual INT ScrollUp();                                          // 73 0x100c3a50
    virtual INT ScrollDown();                                        // 74 0x100c3a60
    virtual INT SetTopIndex(INT Index);                              // 75 0x100b2520

    void ToggleNames(UBOOL bMeshNames);                              // 0x100b3140 (guard name)
    // name ours: the scroll bar's range (an inline method the stock compiler emitted)
    void SetScrollRange(INT Total, INT Page);                        // 0x100b2560
};
NW_ASSERT_SIZE(NCNpcView_CharacterSelect_List, 0x220);

class NCNpcView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCNpcView_CharacterSelectWnd, NCWnd, 0, 0x10293838)

    NCNpcView_CharacterSelect_List* List; // +0xC8
    NCComboBox* AniCombo;                // +0xCC the pawn's animations
    // C3 (names ours): five edit lines for the mesh and its four textures and two for the height and the radius,
    // labeled by OnPaint ("Mesh", "Tex1" .. "Tex4", "Height", "Radius"); nothing in the stock code reads them
    NCEditBox* MeshEdit;                 // +0xD0 id 0xFE
    NCEditBox* TexEdit[4];               // +0xD4 .. +0xE0 ids 0xFF, 0x100 (the test button has it too), 0x101, 0x102
    NCEditBox* HeightEdit;               // +0xE4 id 0x103
    NCEditBox* RadiusEdit;               // +0xE8 id 0x104
    NCEditBox* SearchEdit;               // +0xEC
    NCButton* SearchButton;              // +0xF0
    NCButton* ResetButton;               // +0xF4 the whole table again
    NCEditBox* RateEdit;                 // +0xF8 the pawn's speed factor (Rate)
    NCEditBox* SpeedEdit;                // +0xFC its speed (Speed)
    NCButton* ApplyButton;               // +0x100 the two edit lines to the pawn
    NCCheckBox* MeshNameCheck;           // +0x104 labeled "Mesh name" (in Korean): the list
                                         //   shows the meshes
    NCButton* SummonButton;              // +0x108 labeled "Summon" (in Korean)
    NCButton* SpawnTestButton;           // +0x10C (new in C2) "TEST", id 0x100: the stress test (OnSpawnTest)
    NCCheckBox* NpcCheck;                // +0x110 the NPCs are listed ("NPC")
    NCCheckBox* MonsterCheck;            // +0x114 the monsters are listed ("Monster")
    UTexture* TitleTex;                  // +0x118 "sek.cbui168"
    UTexture* BackTex;                   // +0x11C "sek.cbui144"
    FLOAT Rate;                          // +0x120 (1.0)
    INT Speed;                           // +0x124
    TCHAR InfoText[5][256];              // +0x128 .. +0xB28 (C3: the selected line's name and its four mesh and
                                         //   texture names, drawn in yellow by OnPaint; set by the list's OnLButtonDown)

    NCNpcView_CharacterSelectWnd();                                  // 0x100b5340
    // ~NCNpcView_CharacterSelectWnd                                 // 0x1009d760 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b47d0
    virtual INT OnCreate();                                          // 37 0x100b53e0
    virtual INT OnDestroy();                                         // 38 0x100b3300
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b3610
    // names ours
    virtual INT OnSpawnTest();                                       // 68 0x100b40b0 (C2 only, name ours) button 0xFF: one skeleton archer, OnSummon's body
                                                                     //   spawned around the player (a developer stress test)
    virtual INT OnSearch();                                          // 69 0x100b4dc0
    virtual INT OnReset();                                           // 70 0x100b5060
    virtual INT OnApply();                                           // 71 0x100b3b50
    virtual INT OnToggleNames();                                     // 72 0x100b3c70
    virtual INT OnSummon();                                          // 73 0x100b3c90
    virtual void NewVirtual74();  // 74 0x100b2570 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnSearchEdit(INT Code);                              // C3: slot gone (was 73), no longer virtual (C2 only, name ours) Enter in the search line searches
                                                                     //   (the body is NCMSViewrWnd::OnIDEdit's, hooked there)

    INT OnAniNameCombo(INT Index);                                   // 0x100b3200 (guard name)
};
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, List, 0xC8);
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, MeshEdit, 0xD0);
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, SearchEdit, 0xEC);
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, TitleTex, 0x118);
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, InfoText, 0x128);
NW_ASSERT_SIZE(NCNpcView_CharacterSelectWnd, 0xB28);

class NCNpcViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCNpcViewrWnd, NCWnd, 0, 0x10293D60)

    NCNpcView_InvenItemWnd* ItemWnd[2];  // +0xC8 the tabs: all the items, the search's
    NCNpcView_CharacterSelectWnd* CharSelectWnd; // +0xD0 top-level
    NCTabControl* TabCtrl;               // +0xD4
    NCEditBox* SearchEdit;               // +0xD8
    NCButton* SearchButton;              // +0xDC
    UTexture* BackTex;                   // +0xE0 "L2UI.QuestWnd.questBack3"
    UTexture* OptionBackTex;             // +0xE4 "L2UI.etcWnd.OptionBack"   // C2: offset derived, no evidence
    User Npcs[19];                       // +0xE8 the default NPC records (0x1F8 each in C2)   // C2: offset derived, no evidence
    ULevel* Level;                       // +0x2A78 the game's when created   // C2: offset derived, no evidence

    NCNpcViewrWnd();                                                 // 0x100b5160
    // ~NCNpcViewrWnd                                                // 0x1009cdd0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b1f30
    virtual INT OnCreate();                                          // 37 0x100b62a0
    virtual INT OnDestroy();                                         // 38 0x100b2840
    virtual void ShowWindow();                                       // 44 0x100b2220
    virtual void HideWindow();                                       // 45 0x100b2190
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b2720
    virtual INT OnSearchBtnClick();                                  // 68 0x100b4c40 (guard name)
    virtual void NewVirtual69();  // 69 0x100b2360 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnSearchEdit(INT Code);                              // C3: slot gone (was 68), no longer virtual (C2 only, name ours) Enter in the search line clicks the
                                                                     //   button (the body is NCAuthWnd::OnPasswordNotify's, hooked there)

    // names from the guards
    INT AddItem(INT Tab, NItemInfo* Info);                           // 0x100b22c0
    void AddAllCharacter();                                          // 0x100b4910
    void AddAllItem();                                               // 0x100b4b10
    void FillItemInfo(NViewItemData* Data, NItemInfo* Info);         // 0x100b1fb0 (name ours): the record into an NItemInfo
};
NW_ASSERT_OFFSET(NCNpcViewrWnd, CharSelectWnd, 0xD0);
NW_ASSERT_OFFSET(NCNpcViewrWnd, Npcs, 0xE8);
NW_ASSERT_OFFSET(NCNpcViewrWnd, Level, 0x2A78);
NW_ASSERT_SIZE(NCNpcViewrWnd, 0x2A7C);

// the console's NPC viewer (the second of its pawn viewer windows)
FORCEINLINE NCNpcViewrWnd* NpcViewer() { return static_cast<NCNpcViewrWnd*>(GConsole->PawnViewerWnd[1]); }

#pragma pack(pop)