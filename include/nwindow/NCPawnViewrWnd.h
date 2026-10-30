#pragma once
// PawnView - the developers' pawn viewer (not in the normal game: NConsoleWnd's
// InitializePawnViewer makes it, the console's PawnViewerWnd[0]). It dresses and animates the
// pawn the player controls:
//
// NCPawnViewrWnd - the main window (UClass "CPawnViewrWnd", 0x27D0): three pages over the
// pawn (combined animations, hair meshes, a crowd simulator), the item table by category as
// icon grids or as text lists (the "GUI" check box picks), a search of the item names, and the
// weapon of a weapon type for the animations ("Use"). Each item worn goes into the record of
// the default character chosen in NCPawnView_CharacterSelectWnd (Characters, 18 races /
// genders / class types), which dresses the pawn (User::SetPawnResource).
//
// NCPawnView_CharacterSelectWnd - a top-level window (UClass "CPawnView_CharacterSelectWnd",
// 0xBC): the default character in a combo box (the player's pawn is replaced by a new one of
// its class) and its mesh's animation sequences in NCPawnView_CharacterSelect_List (a double
// click plays one in a loop, a second one stops it), sorted by weapon or by action.
//
// NCPawnView_InvenItemWnd / NCPawnView_InvenTextWnd - a category's items as icons (a right or
// double click puts the item on or takes it off) and as text lines (a click does).
//
// NCPawnViewr_MixAnimWnd - three animations taken from the list and a speed: "Start" plays
// them as the three steps of a skill (APawn::SetMagicInfo / InitSkillProcess).
//
// NCPawnViewr_HairChangeWnd - hair style and color, then the two hair meshes (a / b) found for
// them in LineageWarriors, and their textures; "OK" puts them on the pawn.
//
// NCPawnViewr_Simulator - spawns N random characters around the pawn that attack each other,
// cast, cancel and get replaced, by the percentages typed (its per-frame part never runs:
// nothing sets bRunning).

#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NViewerData.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class APawn;
class NCButton;
class NCCheckBox;
class NCComboBox;
class NCEditBox;
class NCTabControl;
class NCPawnViewrWnd;

class NCPawnView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCPawnView_InvenItemWnd, NCInvenItemWnd, 0, 0x10298FB0)

    NCPawnView_InvenItemWnd();                                       // 0x100c95a0
    NCPawnView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100b0080 (C1 only: no C2 match)
    // ~NCPawnView_InvenItemWnd                                      // 0x100b0030 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10097bc0 (shared)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100c3d20
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100c3e10 (C2: an item let go outside drops into the world)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x100c6d10
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100c2200
};
NW_ASSERT_SIZE(NCPawnView_InvenItemWnd, 0x164);

class NCPawnView_InvenTextWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnView_InvenTextWnd, NCWnd, 0, 0x10297B10)

    NCListBox* List;                 // +0xC8 item names; a line's param is its NItemEntry

    NCPawnView_InvenTextWnd();                                       // 0x100c9500
    // ~NCPawnView_InvenTextWnd                                      // 0x100abb70 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c36a0
    virtual INT OnCreate();                                          // 37 0x100c7650
    virtual void ShowWindow() {}                                     // 44 0x10001b00 (folded)
    virtual INT OnInvenItemList(INT Index);                          // 68 0x100c7430
};
NW_ASSERT_SIZE(NCPawnView_InvenTextWnd, 0xCC);

class NCPawnView_CharacterSelect_List : public NCDebugWnd
{
    NW_DECLARE_CLASS(NCPawnView_CharacterSelect_List, NCDebugWnd, 0, 0x10298550)

    INT PlayingIndex;                // +0x220 line whose animation plays, -1 = none

    NCPawnView_CharacterSelect_List();                               // 0x100c9910
    NCPawnView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines); // 0x100b0850 (C1 only: no C2 match)
    // ~NCPawnView_CharacterSelect_List                              // 0x100b0800 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b2380 (shared)
    virtual INT OnCreate();                                          // 37 0x100c38d0
    virtual INT OnDestroy();                                         // 38 0x100c39d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c3950
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x100c7dd0 (C2: plays the animation once)
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100b2540 (shared)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 72 0x100c7dc0
    virtual INT ScrollUp();                                          // 73 0x100c3a50 (shared)
    virtual INT ScrollDown();                                        // 74 0x100c3a60 (shared)
    virtual INT SetTopIndex(INT Index);                              // 75 0x100b2520 (shared)

    INT PlayAnim(INT bLoop);                                         // 0x100c5250 (name ours): the animation of the selected line plays on the
                                                                     //   pawn (looped or once); on the line already playing it stops
};
NW_ASSERT_OFFSET(NCPawnView_CharacterSelect_List, PlayingIndex, 0x220);
NW_ASSERT_SIZE(NCPawnView_CharacterSelect_List, 0x224);

class NCPawnView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnView_CharacterSelectWnd, NCWnd, 0, 0x102975F0)

    NCComboBox* NameCombo;           // +0xC8 the default characters   // C2: offset derived, no evidence
    NCPawnView_CharacterSelect_List* AnimList; // +0xCC the pawn mesh's animations
    NCCheckBox* ByActionCheck;       // +0xD0 sort the animations by action (else by weapon)
    UTexture* TitleTex;              // +0xD4 "sek.cbui168"
    UTexture* BackTex;               // +0xD8 "sek.cbui144"
    UTexture* UnknownA8;             // +0xDC "sek.cbui67" (not drawn)
    UTexture* UnknownAC;             // +0xE0 "sek.cbui68" (not drawn)
    UTexture* UnknownB0;             // +0xE4 "sek.cbui69" (not drawn)
    INT UnknownB4;                   // +0xE8 (0)   // C2: offset derived, no evidence
    INT Selected;                    // +0xEC default character shown, -1 = none

    NCPawnView_CharacterSelectWnd();                                 // 0x100c9790
    // ~NCPawnView_CharacterSelectWnd                                // 0x100acda0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c3600
    virtual INT OnCreate();                                          // 37 0x100c9ca0
    virtual INT OnDestroy();                                         // 38 0x100c4e30 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c4d00
    virtual INT OnNameComboBox(INT Index);                           // 68 0x100c8990
    virtual INT OnAniNameComboBox(INT Index) { return 1; }           // 69 0x10003360 (folded)
    virtual INT OnByActionCheck();                                   // 70 0x100c8cf0 (name ours)

    void UpdateAnimList();                                           // 0x100c70c0 (name ours)
};
NW_ASSERT_OFFSET(NCPawnView_CharacterSelectWnd, TitleTex, 0xD4);
NW_ASSERT_OFFSET(NCPawnView_CharacterSelectWnd, Selected, 0xEC);
NW_ASSERT_SIZE(NCPawnView_CharacterSelectWnd, 0xF0);

class NCPawnViewr_MixAnimWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_MixAnimWnd, NCWnd, 0, 0x10298A88)

    NCButton* TakeButtons[3];        // +0xC8 "Use": the list's animation into the edit line
    NCEditBox* AnimEdits[3];         // +0xD4 the skill's three animations (disabled: filled
                                     //   by the buttons only)
    NCEditBox* SpeedEdit;            // +0xE0 ("1.9")
    NCButton* StartButton;           // +0xE4 "Start"
    FLOAT StepRate[3];               // +0xE8 (new in C3) the frames a second of each animation line (SetStepAnim)
    INT StepFrames[3];               // +0xF4 and their number of frames
    FLOAT TotalTime;                 // +0x100 0.2 + the time of the lines: shown in SpeedEdit

    NCPawnViewr_MixAnimWnd();                                        // 0x100c9860
    // ~NCPawnViewr_MixAnimWnd                                       // 0x100ad260 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c3700
    virtual INT OnCreate();                                          // 37 0x100c7790
    virtual INT OnDestroy();                                         // 38 0x100c5100
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c4fe0
    // names ours
    virtual void SetStepAnim(const TCHAR* Name, INT Index);          // 68 0x100c4ea0 (C3: new)
    virtual INT OnTake1();                                           // 69 0x100c37b0
    virtual INT OnTake2();                                           // 70 0x100c3810
    virtual INT OnTake3();                                           // 71 0x100c3870
    virtual INT OnStart();                                           // 72 0x100c7be0
};
NW_ASSERT_OFFSET(NCPawnViewr_MixAnimWnd, AnimEdits, 0xD4);
NW_ASSERT_OFFSET(NCPawnViewr_MixAnimWnd, StepRate, 0xE8);
NW_ASSERT_OFFSET(NCPawnViewr_MixAnimWnd, TotalTime, 0x100);
NW_ASSERT_SIZE(NCPawnViewr_MixAnimWnd, 0x104);

class NCPawnViewr_HairChangeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_HairChangeWnd, NCWnd, 0, 0x102999F8)

    // C2 layout from OnCreate (0x100ADD80), the constructor (0x100ADC50) and the handlers; the window was redesigned:
    // the "a" and "b" hair meshes of a hair number, found for a style suffix and a color, and a face list; names ours
    NCComboBox* HairStyleCombo;      // +0xC8 id 0x104, 0..4: the "_m0S" suffix of the mesh names (the handler puts it back to 0)
    NCComboBox* HairColorCombo;      // +0xCC id 0x112, 0..9: the "_t0C" color of the textures
    NCComboBox* MeshACombo;          // +0xD0 id 0x105, the "a" hair meshes found
    NCComboBox* MeshBCombo;          // +0xD4 id 0x106, the "b" hair meshes found
    NCComboBox* FaceCombo;           // +0xD8 id 0x109, the faces found
    NCCheckBox* TogetherCheck;       // +0xDC id 0x110 "AH & BH TOGETHER", checked: a mesh picked picks the other with its number
    NCButton* ResetButton;           // +0xE0 id 0x108
    INT HairColor;                   // +0xE4 as picked (the character record gets it)
    INT HairNumber;                  // +0xE8 the mesh number picked (the character record HairStyle)
    INT HairStyle;                   // +0xEC the suffix (HairStyleCombo)
    INT Face;                        // +0xF0
    INT MeshANumbers[10];            // +0xF4 the mesh number of each MeshACombo line, then 10 for "No Mesh"
    INT MeshBNumbers[10];            // +0x11C the same for MeshBCombo
    INT FaceNumbers[10];             // +0x144 the face number of each FaceCombo line
    TCHAR MeshATexture[256];         // +0x16C "<race>.<race>_m00N_t0C_m0S_ah"
    TCHAR MeshBTexture[256];         // +0x36C "..._bh"

    NCPawnViewr_HairChangeWnd();                                     // 0x100c99f0
    // ~NCPawnViewr_HairChangeWnd                                    // 0x100add30 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c3a70
    virtual INT OnCreate();                                          // 37 0x100c7de0
    virtual INT OnDestroy();                                         // 38 0x100c62a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c5ea0
    // names ours
    virtual INT OnReset();                                           // 68 0x100c5a00 button 0x108: everything back to 0
    virtual INT OnStyleCombo(INT Index);                             // 69 0x100c5ab0 the style combo goes back to 0
    virtual INT OnColorCombo(INT Index);                             // 70 0x100c5af0 a color picked: the picked meshes get it
    virtual INT OnMeshA(INT Index);                                  // 71 0x100c5b60
    virtual INT OnMeshB(INT Index);                                  // 72 0x100c5c90
    virtual INT OnFace(INT Index);                                   // 73 0x100c5dc0

    void OnOK();                                                     // 0x100c5810 (name ours): the picked meshes and textures on the pawn
    void FillFaceList();                                             // 0x100c53a0 (name ours): the faces that exist
    void FillMeshLists();                                            // 0x100c5510 (name ours): the meshes that exist
    INT FindMeshA(INT Number);                                       // 0x100c3b70 (name ours): the line of a mesh number
    INT FindMeshB(INT Number);                                       // 0x100c3ba0 (name ours)
    void SyncFromCharacter();                                        // 0x100c82c0 (name ours): the values of the chosen default character
};
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, HairColor, 0xE4);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshANumbers, 0xF4);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshBNumbers, 0x11C);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, FaceNumbers, 0x144);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshATexture, 0x16C);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshBTexture, 0x36C);
NW_ASSERT_SIZE(NCPawnViewr_HairChangeWnd, 0x56C);

// a pawn of the simulator (name ours; 8 bytes)
struct NSimPawn
{
    APawn* Pawn;
    INT Kind;                        // 1: attacks and casts the skill table's skills, 2: the
                                     //   bow share (casts a few fixed skills), 0: none
};

// the stock's std::vector<NSimPawn> (MSVC 7.1 layout: the empty allocator, then first, last and
// end of storage). The simulator only reserves it and writes its elements past the size.
struct NSimPawnVector
{
    INT Allocator;                   // +0x0
    NSimPawn* First;                 // +0x4
    NSimPawn* Last;                  // +0x8
    NSimPawn* End;                   // +0xC

    // <vector>'s reserve, _Xlen and _Ucopy's _Uninitialized_copy (NStdLib.cpp; names ours)
    void Reserve(DWORD Count);                                       // 0x1008dd30
    void Xlen() const;                                               // 0x1008dcb0
    static NSimPawn* UninitializedCopy(NSimPawn* First, NSimPawn* Last, NSimPawn* Dest,
                                       void* Al, INT Tag);           // 0x1008c720

    // the vector's destructor, inline in the simulator's
    ~NSimPawnVector()
    {
        if (First)
            appFree(First);
        First = NULL;
        Last = NULL;
        End = NULL;
    }
};
NW_ASSERT_SIZE(NSimPawnVector, 0x10);

// 4-byte packing: LastTime sits at +0xC4
#pragma pack(push, 4)
class NCPawnViewr_Simulator : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_Simulator, NCWnd, 0, 0x102994D0)

    NCEditBox* CountEdit;            // +0xC8 how many pawns
    NCEditBox* SkillEdit;            // +0xCC % a second to use a skill or attack
    NCEditBox* CancelEdit;           // +0xD0 % a second to cancel
    NCEditBox* ReplaceEdit;          // +0xD4 % a second to be replaced by a new pawn
    NCEditBox* BowEdit;              // +0xD8 % of the new pawns of the bow kind
    NCButton* StartButton;           // +0xDC "Start"
    UBOOL bRunning;                  // +0xE0 never set
    INT NumPawns;                    // +0xE4
    FLOAT SkillRate;                 // +0xE8 the percentages typed
    FLOAT CancelRate;                // +0xEC
    FLOAT ReplaceRate;               // +0xF0
    FLOAT BowRate;                   // +0xF4
    DOUBLE LastTime;                 // +0xF8 appSeconds
    DOUBLE DeltaTime;                // +0x100 seconds since the last frame
    NSimPawnVector Pawns;            // +0x108
    INT SkillIDs[2000];              // +0x118 active skills with an animation (OnCreate)
    INT NumSkills;                   // +0x2058

    NCPawnViewr_Simulator();                                         // 0x100ca430
    // ~NCPawnViewr_Simulator                                        // 0x100ca4d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c3bd0
    virtual INT OnCreate();                                          // 37 0x100c8d00
    virtual INT OnDestroy();                                         // 38 0x100c6a20 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c6390
    // names ours
    virtual INT OnStart();                                           // 68 0x100ca180
    // C3: the result goes through a hidden pointer the callee pops (ret 4) and comes back in eax
    virtual NSimPawn* SpawnPawn(NSimPawn* Out);                      // 69 0x100c66d0
    virtual INT TryAction(INT Index);                                // 70 0x100c9150
    virtual INT TryCancel(INT Index);                                // 71 0x100c93f0
    virtual INT TryReplace(INT Index);                               // 72 0x100c9440
};
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, bRunning, 0xE0);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, LastTime, 0xF8);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, Pawns, 0x108);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, SkillIDs, 0x118);
NW_ASSERT_SIZE(NCPawnViewr_Simulator, 0x205C);
#pragma pack(pop)

// NCPawnView_ShortCutWnd - the pawn viewer's row of ten shortcut buttons (UClass "CPawnView_ShortCutWnd", 0xC0C; new in
// C3): a 437 x 44 strip over the screen's top-left, a name on each button; a click runs the button's command on the
// engine and on the first viewport. The commands and names come from L2.ini ([L2PawnViewer] ShortcutCommand0 ..
// ShortcutCommand9, ShortcutName0 .. ShortcutName9).
class NCPawnView_ShortCutWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnView_ShortCutWnd, NCWnd, 0, 0x10298030)

    UTexture* BackTex;               // +0xC8 "L2UI.ShortcutWnd.Back1" 417 x 44
    TCHAR Commands[10][128];         // +0xCC the console commands (ShortcutCommand<N>)
    TCHAR Names[10][16];             // +0xACC the buttons' labels (ShortcutName<N>)

    NCPawnView_ShortCutWnd();                                        // 0x100c9b20
    // ~NCPawnView_ShortCutWnd                                       // 0x100c9bd0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (NControl's body, folded)
    virtual INT OnCreate();                                          // 37 0x100c83d0
    virtual INT OnDestroy();                                         // 38 0x100c6cb0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c6b40
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100c6c40

    // name ours: the button under a point of the screen (a row of 33 x 33 buttons from x 31, 37 apart; y 6 .. 38), -1
    // for none
    INT HitTest(INT X, INT Y);                                       // 0x100c3c30
};
NW_ASSERT_OFFSET(NCPawnView_ShortCutWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCPawnView_ShortCutWnd, Commands, 0xCC);
NW_ASSERT_OFFSET(NCPawnView_ShortCutWnd, Names, 0xACC);
NW_ASSERT_SIZE(NCPawnView_ShortCutWnd, 0xC0C);

class NCPawnViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewrWnd, NCWnd, 0, 0x10299F20)

    TCHAR InfoText[3][256];          // +0xC8 (new in C3) three lines of text drawn in yellow at the bottom (OnPaint; nothing in
                                     //   the ported code writes them)
    NCPawnView_InvenItemWnd* ItemWnds[8]; // +0x6C8 a grid per category (6 made: chest, legs,
                                     //   gloves, feet, headgear, etc)
    NCPawnView_InvenTextWnd* TextWnds[8]; // +0x6E8 the same as text lists
    NCPawnView_CharacterSelectWnd* CharSelectWnd; // +0x708 top-level (not a child)
    NCWnd* ShortCutWnd;              // +0x70C (new in C3) NCPawnView_ShortCutWnd (0xC0C bytes), top-level, 437 x 44 at (0, 0);
                                     //   made by OnCreate, deleted by OnDestroy
    NCPawnViewr_MixAnimWnd* MixAnimWnd; // +0x710 the pages of PageTabs
    NCPawnViewr_HairChangeWnd* HairChangeWnd; // +0x714
    NCPawnViewr_Simulator* Simulator; // +0x718
    // C2 layout from OnCreate (0x100b19a0) and OnDestroy (0x100ac9b0); the viewer has a third row of controls
    // (an item id range, three more buttons): names ours, after the control ids
    UTexture* GridBackTex;           // +0x71C "L2UI.QuestWnd.questBack3" (under the grids)
    UTexture* BackTex;               // +0x720 "L2UI.etcWnd.OptionBack"
    NCEditBox* SearchEdit;           // +0x724 id 0x116
    NCEditBox* RangeEditA;           // +0x728 (new in C2) id 0x11A, digits only (type 3), 23 x 16 at (25, 560)
    NCEditBox* RangeEditB;           // +0x72C (new in C2) id 0x11B, digits only, at (60, 560)
    NCWnd* UnknownF8;                // +0x730 destroyed by OnDestroy, never created
    NCButton* SearchButton;          // +0x734 "Search" id 0x102
    NCButton* ListButton;            // +0x738 "View List" id 0x103
    NCButton* Button108;             // +0x73C (new in C2) id 0x117, 28 x 16 at (25, 500)
    NCButton* Button10C;             // +0x740 (new in C2) id 0x118, 50 x 16 at (70, 500)
    NCButton* Button110;             // +0x744 (new in C2) id 0x119, 42 x 16 at (110, 560)
    BYTE _Unknown748[0x74C - 0x748]; // +0x748 (never stored)
    NCButton* Button11E;             // +0x74C (new in C3) id 0x11E, 28 x 16 at (130, 555), SetText(string at 0x10195c80)
    NCButton* Button11F;             // +0x750 (new in C3) id 0x11F, 56 x 16 at (162, 555), SetText(string at 0x10195c6c)
    NCTabControl* CategoryTabs;      // +0x754 no pages: OnCategoryTab shows the grid or list
    NCTabControl* PageTabs;          // +0x758
    NCCheckBox* GUICheck;            // +0x75C "GUI": the grids, else the text lists
    NCCheckBox* Check11C;            // +0x760 (new in C3) id 0x11C, 70 x 16 at (165, 517), text at 0x10195c18; created checked
    User Characters[19];             // +0x764 the 18 default characters (race, gender, class (C3 constructor 0x100c9670; OnCreate stores
                                     //   .Pawn at 0x8B4) from the creation's table; the 19th reads past it)
    ULevel* Level;                   // +0x30F4 the engine's level when created (not read)   // C2: offset derived, no evidence
    INT CurrentPage;                 // +0x30F8 category shown
    NCComboBox* WeaponTypeCombo;     // +0x30FC "0(HAND)" .. "7(DUALFIST)"
    NCButton* UseButton;             // +0x3100 "Use": the weapon of that type on the character   // C2: offset derived, no evidence
    INT DefaultWeapons[8];           // +0x3104 first item id found of each weapon type, -1 none

    NCPawnViewrWnd();                                                // 0x100c9670
    // ~NCPawnViewrWnd                                               // 0x100ac0b0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100c2290
    virtual INT OnCreate();                                          // 37 0x100ca570
    virtual INT OnDestroy();                                         // 38 0x100c4980
    virtual void ShowWindow();                                       // 44 0x100c4bf0
    virtual void HideWindow();                                       // 45 0x100c3450
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100c46a0
    // names ours
    virtual INT vfn062() { return 1; }                               // 68 0x10005fa0 (folded) id 0xF3
    virtual INT OnCategoryTab(INT Index);                            // 69 0x100c3fc0
    virtual INT OnGUICheck();                                        // 70 0x100c4910
    virtual INT OnSearch();                                          // 71 0x100c8710
    virtual INT OnViewList();                                        // 72 0x100c8900
    virtual INT OnUseWeapon();                                       // 73 0x100c6fd0
    // C2 only (names ours): the edit lines' Enter keys and the three buttons of the third row
    virtual INT OnSearchEdit(INT Code);                              // 74 0x100c35c0 Enter in SearchEdit (id 0x116): search
    virtual INT OnRangeEdit(INT Code);                               // 75 0x100c35e0 Enter in RangeEditB (id 0x11B): go
    virtual INT OnSpawnCharacter();                                  // 76 0x100c4050 button 0x117
    virtual INT OnToggleFlag();                                      // 77 0x100c4440 button 0x118 (flips a controller bit)
    virtual INT OnAttackTarget();                                    // 78 0x100c4470 button 0x11D (C3: the pawn attacks its target)
    virtual INT OnGoToMap();                                         // 79 0x100c4500 button 0x119 (teleport / open <x>_<y>.unr)
    virtual INT OnFaceCheck();                                       // 80 0x100c44c0 check box 0x11C (C3: bit 0 of the pawn's +0x5B8)
    virtual INT OnSaveEquipment();                                   // 81 0x100c3000 button 0x11E (C3: the chosen character's equipment into L2.ini)
    virtual INT OnLoadEquipment();                                   // 82 0x100c31d0 button 0x11F (C3: and back)

    void HidePages();                                                // 0x100c24b0 (name ours)
    void AddAllCharacter();                                          // 0x100c24f0
    INT GetItemCategory(NViewItemData* Data);                        // 0x100c2d40 (name ours; C2: one argument, ret 4)
    void FillItemInfo(NViewItemData* Data, NItemInfo* Info);         // 0x100c2dd0 (name ours): the record into an NItemInfo
    void Clear();                                                    // 0x100c3390
    INT AddItem(INT Page, NItemInfo* Info);                          // 0x100c34e0
    void AddAllItem();                                               // 0x100c85f0
};
NW_ASSERT_OFFSET(NCPawnViewrWnd, InfoText, 0xC8);
NW_ASSERT_OFFSET(NCPawnViewrWnd, ItemWnds, 0x6C8);
NW_ASSERT_OFFSET(NCPawnViewrWnd, TextWnds, 0x6E8);
NW_ASSERT_OFFSET(NCPawnViewrWnd, CharSelectWnd, 0x708);
NW_ASSERT_OFFSET(NCPawnViewrWnd, MixAnimWnd, 0x710);
NW_ASSERT_OFFSET(NCPawnViewrWnd, GridBackTex, 0x71C);
NW_ASSERT_OFFSET(NCPawnViewrWnd, SearchEdit, 0x724);
NW_ASSERT_OFFSET(NCPawnViewrWnd, CategoryTabs, 0x754);
NW_ASSERT_OFFSET(NCPawnViewrWnd, Check11C, 0x760);
NW_ASSERT_OFFSET(NCPawnViewrWnd, Characters, 0x764);
NW_ASSERT_OFFSET(NCPawnViewrWnd, Level, 0x30F4);
NW_ASSERT_OFFSET(NCPawnViewrWnd, DefaultWeapons, 0x3104);
NW_ASSERT_SIZE(NCPawnViewrWnd, 0x3124);

/*-----------------------------------------------------------------------------
    The viewer's helpers (inline in the stock code, names ours).
-----------------------------------------------------------------------------*/
namespace NPawnView
{
    // the console's PawnViewerWnd[0]
    FORCEINLINE NCPawnViewrWnd* Viewer()
    {
        return static_cast<NCPawnViewrWnd*>(GConsole->PawnViewerWnd[0]);
    }
    // the pawn the player's controller drives
    FORCEINLINE APawn* PlayerPawn() { return ViewActor()->Pawn; }
}

#pragma pack(pop)