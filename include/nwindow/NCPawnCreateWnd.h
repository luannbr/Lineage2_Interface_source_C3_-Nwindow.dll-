#pragma once
// NCPawnCreateWnd - the character creation screen (UClass "CPawnCreateWnd", 0x108; NConsoleWnd
// +0x36D0): the choices panel (NCPawnSetupWnd), the chosen class's description (NCPawnInfoWnd),
// "Create Character" and "Previous" in the lower right corner, and under the stage buttons that
// turn the chosen default character and zoom on its chest. The engine keeps 18 default
// characters, one per race / class type / gender (index (Race * 2 + ClassType) * 2 + Sex);
// choosing the race, class type or gender plays a lobby event that moves the camera
// (NCPawnSetupWnd::ExecCameraEvent).
//
// The four classes share one stretch of the stock code (0x10083c00 .. 0x10087410).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// C2: what the creation screen sends (UL2ConsoleWnd::RequestCharacterCreate, 0x10052480, by value, ret 0x60): the
// name is 24 characters now (C1: 20), so every field after it is 8 bytes further and the record is 0x60 bytes. It is
// the Engine's CharacterInfo (include/ue2/UE2Engine.h, the same C2 shape), so the console's request takes it as is
// (names ours, from NCPawnSetupWnd::GetCreateInfo 0x100a80a0 and NCPawnCreateWnd::OnCreateCharacter 0x100a8fd0)
typedef CharacterInfo NCharacterInfo;
NW_ASSERT_OFFSET(NCharacterInfo, Race, 0x30);
NW_ASSERT_OFFSET(NCharacterInfo, Stats, 0x3C);
NW_ASSERT_OFFSET(NCharacterInfo, HairStyle, 0x54);
NW_ASSERT_SIZE(NCharacterInfo, 0x60);

class NString;
class NCButton;
class NCExButton;
class NCPushButton;
class NCEditBox;
class NCPawnCreateWnd;

// NCSelectCtrl - a choice between SysString items, stepped through with a button on each side
// (UClass "CSelectCtrl", 0x1B8). A step wraps around and reports the new index to the parent
// (NWM_COMMAND, lParam = index).
class NCSelectCtrl : public NControl
{
    NW_DECLARE_CLASS(NCSelectCtrl, NControl, 0, 0x102970D0)

    NCPushButton* LeftButton;        // +0x1D4 "L2ui_ch3.Button.PREV1" (C3: OnCreate 0x100c0760)
    NCPushButton* RightButton;       // +0x1D8 "L2ui_ch3.Button.NEXT1"
    UTexture* TextBoxLeft;           // +0x1DC "L2ui_ch3.etc.textbox1" (the box is drawn in three pieces by OnPaint)
    UTexture* TextBoxMiddle;         // +0x1E0 "L2ui_ch3.etc.textbox2"
    UTexture* TextBoxRight;          // +0x1E4 "L2ui_ch3.etc.textbox3"
    INT Selected;                    // +0x1E8 -1 = none
    TArray<NString*> Items;          // +0x1EC

    NCSelectCtrl();                                                  // 0x100c1400
    // ~NCSelectCtrl                                                 // 0x100c14c0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100be2e0
    virtual INT OnCreate();                                          // 37 0x100c0760
    virtual INT OnDestroy();                                         // 38 0x100c0920
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100be9d0
    // names ours
    virtual INT SelectPrev();                                        // 69 0x100be360
    virtual INT SelectNext();                                        // 70 0x100be3b0

    // names ours
    void ClearItems();                                               // 0x100c06f0
    void AddItem(INT SysStringID);                                   // 0x100c0a00

    // inlined in the stock code (name ours): an index past the end keeps the old one
    void SetSelected(INT Index)
    {
        if (Index < Items.Num())
            Selected = Index;
        if (Index < 0)
            Selected = -1;
    }
};
NW_ASSERT_OFFSET(NCSelectCtrl, LeftButton, 0x1D4);
NW_ASSERT_OFFSET(NCSelectCtrl, TextBoxLeft, 0x1DC);
NW_ASSERT_OFFSET(NCSelectCtrl, Selected, 0x1E8);
NW_ASSERT_OFFSET(NCSelectCtrl, Items, 0x1EC);
NW_ASSERT_SIZE(NCSelectCtrl, 0x1F8);

// NCPawnInfoWnd - the description of the chosen class under the choices (UClass
// "CPawnInfoWnd", 0x498): a classinfo text wrapped at 228 pixels.
class NCPawnInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnInfoWnd, NCWnd, 0, 0x10296BA8)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.LobbyWnd.select2_back" (C3: the C2 one was at +0xCC)
    UTexture* LineTex;               // +0xCC "L2UI_CH3.Etc.textbackline" (C3, name ours): the lines behind every other row
    TCHAR Text[512];                 // +0xD0

    NCPawnInfoWnd();                                                 // 0x100c1520
    // ~NCPawnInfoWnd                                                // 0x100a6f60 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (shared)
    virtual INT OnCreate();                                          // 37 0x100beb60
    virtual INT OnDestroy();                                         // 38 0x100be410 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100bebb0

    // names ours
    void SetText(const TCHAR* InText);                               // 0x100be4a0
    void ClearText();                                                // 0x100be490
};
NW_ASSERT_OFFSET(NCPawnInfoWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCPawnInfoWnd, Text, 0xD0);
NW_ASSERT_SIZE(NCPawnInfoWnd, 0x4D0);

// NCPawnSetupWnd - the choices panel (UClass "CPawnSetupWnd", 0xD4): the name, then race, class
// type, gender, hair style, hair color and face, each an NCSelectCtrl. A choice clears the ones
// after it and enables the next one; hair and face are applied to the default character on the
// stage. Out of order choices get a system message.
class NCPawnSetupWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnSetupWnd, NCWnd, 0, 0x10296680)

    NCSelectCtrl* RaceSelect;        // +0xC8 sysstring 170..174
    NCSelectCtrl* ClassTypeSelect;   // +0xCC 175 warrior, 176 spellcaster (dwarves: warrior)
    NCSelectCtrl* SexSelect;         // +0xD0 177 male, 178 female
    NCSelectCtrl* HairStyleSelect;   // +0xD4 types A..D, men; A..F, women
    NCSelectCtrl* HairColorSelect;   // +0xD8 types A, B
    NCSelectCtrl* FaceSelect;        // +0xDC types A, B
    INT UnknownE0;                   // +0xE0 (0; C3: cleared by the constructor, nothing else touches it)
    UTexture* BackTex;               // +0xE4 "L2UI.LobbyWnd.MakingBack1"
    NCEditBox* NameEdit;             // +0xE8
    INT Race;                        // +0xEC the choices, -1 = none yet
    INT ClassType;                   // +0xF0
    INT Sex;                         // +0xF4
    INT HairStyle;                   // +0xF8
    INT HairColor;                   // +0xFC
    INT Face;                        // +0x100
    INT CameraStep;                  // +0x104 last camera view played (ExecCameraEvent)

    NCPawnSetupWnd();                                                // 0x100c15d0
    // ~NCPawnSetupWnd                                               // 0x100a7190 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100be4d0
    virtual INT OnCreate();                                          // 37 0x100c16c0
    virtual INT OnDestroy();                                         // 38 0x100bef90
    virtual void ShowWindow();                                       // 44 0x100be5d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100bed40
    // a selector changed (names ours)
    virtual INT OnRace(INT Index);                                   // 68 0x100c0c80
    virtual INT OnClassType(INT Index);                              // 69 0x100c0e60
    virtual INT OnSex(INT Index);                                    // 70 0x100c10c0
    virtual INT OnHairStyle(INT Index);                              // 71 0x100bf290
    virtual INT OnHairColor(INT Index);                              // 72 0x100bf5a0
    virtual INT OnFace(INT Index);                                   // 73 0x100bf8c0

    // names ours
    INT GetCharacterType();                                          // 0x100be5f0
    void ResetPreviews();                                            // 0x100be610
    INT GetClassID();                                                // 0x100be690
    void SetMakingRace(INT InRace);                                  // 0x100be730
    void Reset();                                                    // 0x100bf1a0
    UBOOL GetCreateInfo(NCharacterInfo* Info);                        // 0x100bfc40
    void ExecCameraEvent(INT Step);                                  // 0x100bfe20
    void InitSelectors();                                            // 0x100c0aa0

private:
    NCPawnCreateWnd* CreateWnd();
};
NW_ASSERT_OFFSET(NCPawnSetupWnd, RaceSelect, 0xC8);
NW_ASSERT_OFFSET(NCPawnSetupWnd, BackTex, 0xE4);
NW_ASSERT_OFFSET(NCPawnSetupWnd, Race, 0xEC);
NW_ASSERT_OFFSET(NCPawnSetupWnd, CameraStep, 0x104);
NW_ASSERT_SIZE(NCPawnSetupWnd, 0x108);

// NCPawnCreateFunctionWnd - C3: the "Create Character" and "Previous" buttons, a window of their own in the lower
// right corner (UClass "CPawnCreateFunctionWnd", 0xD4); the C2 creation window had them itself. Its parent is the
// NCPawnCreateWnd: the buttons call that window's OnPrevious / OnCreateCharacter (vslots 71 and 72).
class NCPawnCreateFunctionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnCreateFunctionWnd, NCWnd, 0, 0x10295C38)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.LobbyWnd.lobby_menuback"
    NCButton* CreateButton;          // +0xCC id 0x1009, sysstring 162 "Create Character"
    NCButton* PreviousButton;        // +0xD0 id 0x100A, sysstring 161 "Previous"

    NCPawnCreateFunctionWnd();                                       // 0x100be230

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100bdc80
    virtual INT OnCreate();                                          // 37 0x100be040
    virtual INT OnDestroy();                                         // 38 0x100bdd00
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100bddd0
    // the names are the guards' (copied from the lobby's function window)
    virtual INT OnGameCancelClick();                                 // 68 0x100bdf20 the previous button
    virtual INT OnAccountClick();                                    // 69 0x100bdfb0 the create button
};
NW_ASSERT_OFFSET(NCPawnCreateFunctionWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCPawnCreateFunctionWnd, PreviousButton, 0xD0);
NW_ASSERT_SIZE(NCPawnCreateFunctionWnd, 0xD4);

class NCPawnCreateWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnCreateWnd, NCWnd, 0, 0x10296160)

    NCPawnSetupWnd* SetupWnd;        // +0xC8
    NCPawnInfoWnd* InfoWnd;          // +0xCC
    RecommandedDataStr Recommended;  // +0xD0 the class template the server sent (SetRecommendedData: 0x50 bytes)
    NCExButton* LeftButton;          // +0x120 turn the character while held (C3: the create / previous buttons moved
                                     //   to NCPawnCreateFunctionWnd, so these four came 4 and 8 bytes down)
    NCExButton* RightButton;         // +0x124
    NCButton* ZoomInButton;          // +0x128 one of the two is shown; both have the same id
    NCButton* ZoomOutButton;         // +0x12C
    NCPawnCreateFunctionWnd* FunctionWnd;   // +0x130 "Create Character" / "Previous" (C3)
    UBOOL bZoomed;                   // +0x134

    NCPawnCreateWnd();                                               // 0x100c1b30
    // ~NCPawnCreateWnd                                              // 0x100a8710 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100be740
    virtual INT OnCreate();                                          // 37 0x100c1bd0
    virtual INT OnDestroy();                                         // 38 0x100c0250
    virtual void ShowWindow();                                       // 44 0x100be930
    virtual void HideWindow();                                       // 45 0x100be840
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10001b10
    // names ours
    INT OnPrevious();                                        // 71 0x100be800 (the slot is NewVirtual71's: placeholder
                                                             //   in NNewVirtuals.gen.cpp, the stock body is hooked here)
    INT OnCreateCharacter();                                 // 72 0x100c0420 (the same: NewVirtual72)
    virtual INT OnTurnLeft(INT bPressed);                            // 68 0x100be950
    virtual INT OnTurnRight(INT bPressed);                           // 69 0x100be990
    virtual INT OnZoom();                                            // 70 0x100c0550
    virtual void NewVirtual71();  // 71 0x100be800 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual72();  // 72 0x100c0420 (only this client has it; defined in NNewVirtuals.gen.cpp)

    // names ours
    void SetRecommendedData(RecommandedDataStr Data);                // 0x100be7e0
    void EndRecommendedData() {}                                     // 0x10001b00 (empty, folded;
                                                                     //   the console calls it)
    void ShowViewButtons(UBOOL bShow);                               // 0x100be860
    void Reset();                                                    // 0x100c0400
};
NW_ASSERT_OFFSET(NCPawnCreateWnd, SetupWnd, 0xC8);
NW_ASSERT_OFFSET(NCPawnCreateWnd, Recommended, 0xD0);
NW_ASSERT_OFFSET(NCPawnCreateWnd, LeftButton, 0x120);
NW_ASSERT_OFFSET(NCPawnCreateWnd, ZoomOutButton, 0x12C);
NW_ASSERT_OFFSET(NCPawnCreateWnd, FunctionWnd, 0x130);
NW_ASSERT_OFFSET(NCPawnCreateWnd, bZoomed, 0x134);
NW_ASSERT_SIZE(NCPawnCreateWnd, 0x138);

#pragma pack(pop)