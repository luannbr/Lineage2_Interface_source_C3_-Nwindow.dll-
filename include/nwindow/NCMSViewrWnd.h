#pragma once
// NCMSViewrWnd - the skill viewer (UClass "CMSViewrWnd", 0x276C; titled "SKILLVIEWER"), a
// developer tool the console builds for its pawn viewer mode (NConsoleWnd::
// InitializePawnViewer, the console's PawnViewerWnd[2]): the level-1 skills of the skill table
// in three tabs (magic, active, passive) and the results of a search by name or by id in a
// fourth; a right click (or a double click) on a skill casts it with the player's pawn at its
// target. Beside it a top-level NCMSView_CharacterSelectWnd picks the character to cast with
// (race, sex and class: the pawn is spawned anew) and plays its animations.
//
// NCMSView_InvenItemWnd - the skill grids (UClass "CMSView_InvenItemWnd", 0x13C): nothing can
// be dragged.

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
    NID_MSVIEW_FRAME = 0xF2,
    NID_MSVIEW_NAMEEDIT = 0xF3,
    NID_MSVIEW_IDEDIT = 0xF4,
    NID_MSVIEW_SEARCHNAME = 0xF5,
    NID_MSVIEW_SEARCHID = 0xF6,
    NID_MSVIEW_TAB = 0xF7,
    NID_MSVIEW_SELECT_FRAME = (INT)0xAFF12011,
    NID_MSVIEW_SELECT_NAME = 0x1F04,
    NID_MSVIEW_SELECT_ANIM = 0x1F05,
};

class NCMSView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCMSView_InvenItemWnd, NCInvenItemWnd, 0, 0x102892E0)

    NCMSView_InvenItemWnd();                                         // 0x1009a0d0
    NCMSView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1008a0c0 (C1 only: no C2 match)
    // ~NCMSView_InvenItemWnd                                        // 0x1008a070 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10097bc0 (hooked as
                                                                     //   NCNpcView_InvenItemWnd's)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10097c20
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; } // 63 0x1007d6b0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x10099440
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x10097cc0
};
NW_ASSERT_SIZE(NCMSView_InvenItemWnd, 0x164);

class NCMSView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMSView_CharacterSelectWnd, NCWnd, 0, 0x10288DC0)

    NCComboBox* NameCombo;               // +0xC8 the characters: race, sex, class   // C2: offset derived, no evidence
    NCComboBox* AniCombo;                // +0xCC the pawn's animations
    UTexture* TitleTex;                  // +0xD0 "sek.cbui168"
    UTexture* BackTex;                   // +0xD4 "sek.cbui144"
    UTexture* Unknown67Tex;              // +0xD8 "sek.cbui67" (loaded, not drawn)
    UTexture* Unknown68Tex;              // +0xDC "sek.cbui68"
    UTexture* Unknown69Tex;              // +0xE0 "sek.cbui69"
    INT UnknownB0;                       // +0xE4   // C2: offset derived, no evidence
    INT Selected;                        // +0xE8 (-1) the character shown

    NCMSView_CharacterSelectWnd();                                   // 0x1009a2c0
    // ~NCMSView_CharacterSelectWnd                                  // 0x100893b0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10098840
    virtual INT OnCreate();                                          // 37 0x10099680
    virtual INT OnDestroy();                                         // 38 0x10098ed0 (hooked as
                                                                     //   NCTrainWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10098c50
    // names from the guards
    virtual INT OnNameComboBox(INT Index);                           // 68 0x10098fa0
    virtual INT OnAniNameComboBox(INT Index);                        // 69 0x10099340
};
NW_ASSERT_OFFSET(NCMSView_CharacterSelectWnd, TitleTex, 0xD0);
NW_ASSERT_OFFSET(NCMSView_CharacterSelectWnd, Selected, 0xE8);
NW_ASSERT_SIZE(NCMSView_CharacterSelectWnd, 0xEC);

class NCMSViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMSViewrWnd, NCWnd, 0, 0x10289800)

    NCMSView_InvenItemWnd* ItemWnd[4];   // +0xC8 the tabs: magic, active, passive, the search's
    NCMSView_CharacterSelectWnd* CharSelectWnd; // +0xD8 top-level
    NCTabControl* TabCtrl;               // +0xDC
    UTexture* BackTex;                   // +0xE0 "L2UI.SkillWnd.SkillBack"
    UTexture* HtmlBackTex;               // +0xE4 "L2UI.etcwnd.NPCHtml_BACK" behind the search
    NCEditBox* NameEdit;                 // +0xE8
    NCEditBox* IDEdit;                   // +0xEC
    NCCheckBox* MagicCheck;              // +0xF0 the search lists the magic skills
    NCCheckBox* ActiveCheck;             // +0xF4 the active ones
    NCCheckBox* PassiveCheck;            // +0xF8 the passive ones
    NCButton* SearchNameButton;          // +0xFC
    NCButton* SearchIDButton;            // +0x100
    User Characters[19];                 // +0x104 the default characters (0x1F8 each in C2)   // C2: offset derived, no evidence
    ULevel* Level;                       // +0x2A94 the game's when created

    NCMSViewrWnd();                                                  // 0x1009a1a0
    // ~NCMSViewrWnd                                                 // 0x10088fa0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10097d80
    virtual INT OnCreate();                                          // 37 0x1009a3c0
    virtual INT OnDestroy();                                         // 38 0x10098ac0
    virtual void ShowWindow();                                       // 44 0x10098700
    virtual void HideWindow();                                       // 45 0x10098670
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100988f0
    // names ours
    virtual INT OnSearchName();                                      // 68 0x10099b80
    virtual INT OnSearchID();                                        // 69 0x10099e40
    virtual void NewVirtual70();  // 70 0x100b2570 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual71();  // 71 0x100b2360 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnIDEdit(INT Code);                                  // C3: slot gone (was 69), no longer virtual (new in C2: Enter in the ID line searches by ID)
    INT OnNameEdit(INT Code);                                // C3: slot gone (was 70), no longer virtual (new in C2: Enter in the name line searches by name;
                                                                     //   the stock body is shared with NCAuthWnd::OnPasswordNotify)

    // names from the guards
    INT AddItem(INT Tab, NItemInfo* Info);                           // 0x100987a0
    void AddAllCharacter();                                          // 0x10097e40
    void AddAllItem();                                               // 0x10099920
};
NW_ASSERT_OFFSET(NCMSViewrWnd, CharSelectWnd, 0xD8);
NW_ASSERT_OFFSET(NCMSViewrWnd, Characters, 0x104);
NW_ASSERT_OFFSET(NCMSViewrWnd, Level, 0x2A94);
NW_ASSERT_SIZE(NCMSViewrWnd, 0x2A98);

// the console's skill viewer (the third of its pawn viewer windows)
FORCEINLINE NCMSViewrWnd* MSViewer() { return static_cast<NCMSViewrWnd*>(GConsole->PawnViewerWnd[2]); }

#pragma pack(pop)