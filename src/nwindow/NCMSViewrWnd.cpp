/*=============================================================================
    NCMSViewrWnd.cpp - the skill viewer, its character selection and skill grids. Stock (C3):
    0x10097bc0 .. 0x1009a3c0 and 0x1009adcc (the selection window's OnDestroy is NCTrainWnd's
    body, the grids' WindowProc NCNpcView_InvenItemWnd's: unhooked copies here; the
    TArray<AActor*>::AddItem instance 0x1011b200 is left out).
=============================================================================*/
#include "nwindow/NCMSViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NString.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

// TArray<AActor*>::AddItem is one out-of-line function in the stock image (0x1011B200): the pawn's skill targets are
// added by a call to it, not by the array's inlined body
static __declspec(noinline) void AddMagicTarget(APawn* Pawn, AActor* Target)
{
    Pawn->MagicTargets.AddItem(Target);
}

// UObject::StaticLoadClass flags (UE2)
enum { LOAD_NoFail = 0x1 };

// sysstring texts (names ours): the selection window's labels, the search's tabs and check
// boxes, "search"
enum
{
    STR_MSViewTitle = 157,
    STR_MSViewLabel1 = 50,
    STR_MSViewLabel2 = 88,
    STR_MSViewLabel3 = 90,
    STR_MSViewLabel4 = 91,
    STR_MSViewLabel5 = 92,
    STR_MSViewLabel6 = 101,
    STR_MSViewRace0 = 170,               // the character names: the races (170 .. 174),
    STR_MSViewFighter = 175,             //   then fighter or mystic, male or female
    STR_MSViewMale = 177,
    STR_MSViewActive = 120,
    STR_MSViewPassive = 121,
    STR_MSViewMagic = 122,
    STR_MSViewSearch = 420,
};

// The characters the viewer offers (race, sex, class id; 18 entries at 0x10218270 for the 18
// names of AddAllCharacter, which clears 19: the last one is whatever follows the table).
struct NMSViewCharacter
{
    INT Race;
    INT Sex;
    INT ClassID;
};
#if NW_HOOK_MODE
#define GMSViewCharacters (reinterpret_cast<NMSViewCharacter*>(::nw::Addr(0x10218270)))
#else
extern NMSViewCharacter GMSViewCharacters[];
#endif

// A level of a skill as this client lays it out: the engine header's FL2MagicSkillData is C2's, where these fields
// sit elsewhere. The offsets are the viewers' own reads (NConsoleWndData.cpp has the same view for the getters).
struct NSkillDataC3
{
    FString Name;                       // +0x00
    FString Description;                // +0x0C
    INT SkillID;                        // +0x18
    INT Level;                          // +0x1C
    INT OperateType;                    // +0x20 2: passive, 3: toggle, else active
    BYTE _Unknown24[0x2C - 0x24];
    INT bInstant;                       // +0x2C (name ours) set: the skill is cast with the magic info path
    FLOAT HitTime;                      // +0x30
    BYTE _Unknown34[0x38 - 0x34];
    INT bMagic;                         // +0x38 shown as "Magic" instead
    BYTE _Unknown3C[0x40 - 0x3C];
    L2FName EffectName;                 // +0x40 (the script's skill visual effect; "none" when there is none)
};
NW_ASSERT_OFFSET(NSkillDataC3, SkillID, 0x18);
NW_ASSERT_OFFSET(NSkillDataC3, OperateType, 0x20);
NW_ASSERT_OFFSET(NSkillDataC3, HitTime, 0x30);
NW_ASSERT_OFFSET(NSkillDataC3, bMagic, 0x38);
NW_ASSERT_OFFSET(NSkillDataC3, EffectName, 0x40);

// the index of an L2FName (0: no name; the type keeps it private)
static FORCEINLINE INT NameIndex(const L2FName& Name)
{
    return *(const INT*)&Name;
}

// a dword of an NItemInfo by offset (this client moved the fields after +0x178C up by 4: NItemInfo.h only grew at
// the end)
static FORCEINLINE INT& InfoInt(NItemInfo* Info, DWORD Offset)
{
    return *(INT*)((BYTE*)Info + Offset);
}

// NCItemWnd::Clear is slot 72 of this client (the header of NCItemWnd has an extra virtual before it: slot 71 is not
// in the stock vtable), so the call goes by the slot number
static FORCEINLINE void ClearGrid(NCMSView_InvenItemWnd* Grid)
{
    ::nw::VirtualCall<void>(Grid, 72);
}

template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

/*-----------------------------------------------------------------------------
    NCMSView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x1009a0d0 - the grid's defaults and this class's vtable
NCMSView_InvenItemWnd::NCMSView_InvenItemWnd()
{
}
NW_HOOK_CTOR(0x1009A0D0, NCMSView_InvenItemWnd)

// 0x1009a170 (ret 0x10) - the 4-argument constructor (columns, visible rows, max items, cell size): the grid of six
// arguments with both gaps 1; the skill viewer builds its four grids with it
NCMSView_InvenItemWnd::NCMSView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x1009A170, NCMSView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// stock body shared with the NPC viewer's and the pawn viewer's grids (0x10097bc0, hooked in NCNpcViewrWnd.cpp); C3
// no longer refuses the messages of a disabled window here
INT NCMSView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}

// 0x10097c20 (vslot 61) - only the item under the mouse
INT NCMSView_InvenItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCMSView_InvenItemWnd::OnMouseMove);
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10097c20, NCMSView_InvenItemWnd, OnMouseMove)

// 0x10099440 (vslot 64) - with a character picked (0 .. 19), the skill under the mouse (level 1) is cast by the
// player's pawn at the controller's target (the pawn itself without one). A skill with its flag at +0x2C goes the
// magic info way: the pawn's controller stops moving, the action is stopped, the magic info is set (hit time at
// +0x30), the target added to the pawn's list, and when the skill process starts the pawn's ActionType is 5 and its
// action target the target. Without the flag the skill visual effect named at +0x40 (found, or loaded with the flags
// 0x2000) is triggered at the target; no name, or none loaded: the pawn spawns its own transient effect for the
// skill
INT NCMSView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCMSView_InvenItemWnd::OnRButtonDown);
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected < 0)
        return 1;
    if (Selected >= Items.Num())
        return 1;
    INT Character = MSViewer()->CharSelectWnd->Selected;
    if (Character < 0)
        return 1;
    if (Character > 19)
        return 1;
    INT SkillID = Items(Selected)->ID;
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, 1);
    APlayerController* Controller = ViewActor();
    AActor* Target = Controller->Target;
    APawn* Pawn = Controller->Pawn;
    if (!Data)
        return 1;
    if (!Target)
        Target = Pawn;
    if (Data->bInstant)
    {
        Pawn->Controller->StopMove();
        Pawn->NActionStop();
        Pawn->SetMagicInfo(SkillID, 1, Data->HitTime, Data->HitTime, Target, 1.0f, (FL2MagicSkillData*)Data);
        AddMagicTarget(Pawn, Target);
        *(INT*)((BYTE*)Pawn + 0x4E8) = 1;
        if (!Pawn->InitSkillProcess())
            return 1;
        Pawn->ActionType = 5;
        Pawn->ActionTarget = Target;
        return 1;
    }
    FName None(TEXT("none"), FNAME_Add);
    if (NameIndex(Data->EffectName) != *(INT*)&None)
    {
        USkillVisualEffect* Effect = Cast<USkillVisualEffect>(
            UObject::StaticFindObject(reinterpret_cast<UClass*>(USkillVisualEffect::StaticClass()), ANY_PACKAGE,
                                      *Data->EffectName, 0));
        if (!Effect)
        {
            Effect = Cast<USkillVisualEffect>(
                UObject::StaticLoadObject(reinterpret_cast<UClass*>(USkillVisualEffect::StaticClass()), NULL,
                                          *Data->EffectName, NULL, 0x2000, NULL));
            if (!Effect)
            {
                Pawn->SpawnNTransientEffect(CastToPawn(Target), SkillID);
                return 1;
            }
        }
        Effect->TriggerTransientEffect(Pawn, Target, SkillID);
    }
    else
        Pawn->SpawnNTransientEffect(CastToPawn(Target), SkillID);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10099440, NCMSView_InvenItemWnd, OnRButtonDown)

// 0x10097cc0 (vslot 75) - as the right button
INT NCMSView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCMSView_InvenItemWnd::OnLButtonDblClick);
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    UnselectAll();
    OnRButtonDown(wParam, lParam);
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10097cc0, NCMSView_InvenItemWnd, OnLButtonDblClick)

/*-----------------------------------------------------------------------------
    NCMSView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x1009a2c0 - the textures are looked up in OnCreate; no character shown
NCMSView_CharacterSelectWnd::NCMSView_CharacterSelectWnd()
{
    TitleTex = NULL;
    BackTex = NULL;
    Unknown67Tex = NULL;
    Unknown68Tex = NULL;
    Unknown69Tex = NULL;
    UnknownB0 = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x1009a2c0, NCMSView_CharacterSelectWnd)

// 0x10098840 (vslot 25) - C3 no longer refuses the messages of a disabled window here
INT NCMSView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MSVIEW_SELECT_NAME)
            return OnNameComboBox(lParam) != 0;
        if (wParam == NID_MSVIEW_SELECT_ANIM)
            return OnAniNameComboBox(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10098840, NCMSView_CharacterSelectWnd, WindowProc)

// 0x10099680 (vslot 37) - the title strip frame (6 pixels short of the width) and the two combo boxes under it
INT NCMSView_CharacterSelectWnd::OnCreate()
{
    guard(NCMSView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(0, 0, 0);
    FrameID = NID_MSVIEW_SELECT_FRAME;
    FrameCtrl->ID = NID_MSVIEW_SELECT_FRAME;
    FrameCtrl->SetPosition(2, 3, (INT)(Width - 6.0f), 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    Unknown67Tex = FindTexture(TEXT("sek.cbui67"));
    Unknown68Tex = FindTexture(TEXT("sek.cbui68"));
    Unknown69Tex = FindTexture(TEXT("sek.cbui69"));
    NameCombo = new NCComboBox;
    NameCombo->ID = NID_MSVIEW_SELECT_NAME;
    NameCombo->SetPosition(41, 28, 129, 18);
    CreateChild(NameCombo, NWS_VISIBLE);
    AniCombo = new NCComboBox;
    AniCombo->ID = NID_MSVIEW_SELECT_ANIM;
    AniCombo->SetPosition(41, 52, 129, 18);
    CreateChild(AniCombo, NWS_VISIBLE);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10099680, NCMSView_CharacterSelectWnd, OnCreate)

// 0x10098ed0 (vslot 38) - the combo boxes (the title bar is left to NCWnd::OnDestroy)
INT NCMSView_CharacterSelectWnd::OnDestroy()
{
    guard(NCMSView_CharacterSelectWnd::OnDestroy);
    DestroyChild(this, NameCombo);
    DestroyChild(this, AniCombo);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10098ed0, NCMSView_CharacterSelectWnd, OnDestroy)

// 0x10098c50 (vslot 56) - the background and the title strip (4 pixels short of the width), the title and six labels
INT NCMSView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCMSView_CharacterSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex);
    if (TitleTex)
        Device->DrawTexture(2, 3, (INT)(Width - 4.0f), 20, 0, 0, (INT)(Width - 4.0f), 20, TitleTex);
    Device->DrawText(23, 8, 0xFFDCDCDC, *GL2GameData.SysString[STR_MSViewTitle], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                     0, 0);
    Device->DrawText(8, 32, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel1], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                     0, 0);
    Device->DrawText(8, 72, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel2], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                     0, 0);
    Device->DrawText(8, 100, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel3], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    Device->DrawText(107, 100, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel4], 0, 0, 0, L2FONT_Normal, 0, 0,
                     0, 0, 0, 0);
    Device->DrawText(8, 114, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel5], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    Device->DrawText(6, 128, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel6], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10098c50, NCMSView_CharacterSelectWnd, OnPaint)

// 0x10098fa0 (vslot 68) - another character (0..19, not the one shown) becomes the player's pawn: a player controller
// spawned for it, the pawn class loaded by the character's class name, the pawn spawned 30 units above the player's,
// which it replaces (the old one destroyed, the controller kept the viewport's); then its idle animation looped and
// its animations into the combo box (the animation set is not checked for NULL here)
INT NCMSView_CharacterSelectWnd::OnNameComboBox(INT Index)
{
    guard(NCMSView_CharacterSelectWnd::OnNameComboBox);
    if (Index > 19 || Index == Selected)
        return 1;
    FName ControllerClassName = NAME_None;
    ULevel* Level = GameLevel();
    APlayerController* Viewer = ViewActor();
    ControllerClassName = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* Controller = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, *ControllerClassName, NULL, LOAD_NoFail, NULL));
    if (!Controller)
        appFailAssert("pController", "Src\\NMSViewerWnd.cpp", 1039);
    User Character = MSViewer()->Characters[Index];
    Controller->PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL, Character.GetClassNamePointer(), NULL,
                                                     0, NULL);
    if (!Controller->PawnClass)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    Location.Z += 30.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(Controller->PawnClass, NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1,
                                            Controller, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(ViewActor()->Pawn, 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = Viewer;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Selected = Index;
    MSViewer()->Characters[Index].Pawn = Pawn;
    MSViewer()->Characters[Selected].SetPawnResource();
    // the idle animation of the pawn's weapon
    Pawn->PlayAnim(0, Pawn->GetWaitAnimName(), 1.0f, 0.f, 1, 0);
    APawn* ViewPawn = ViewActor()->Pawn;
    if (ViewPawn)
    {
        UMesh* Mesh = ViewPawn->Mesh;
        if (Mesh)
        {
            Mesh->MeshGetInstance(ViewPawn);
            UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
            // C3: the sequences are at +0x60 of the set, 0x44 bytes each (the header has the C2 +0x44, 0x40)
            INT Num = *(INT*)((BYTE*)Anim + 0x64);
            BYTE* Seqs = *(BYTE**)((BYTE*)Anim + 0x60);
            FName Sequence = NAME_None;
            AniCombo->ClearItems();
            for (INT i = 0; i < Num; i++)
            {
                Sequence = *(FName*)(Seqs + i * 0x44);
                AniCombo->AddItem(*Sequence);
            }
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10098fa0, NCMSView_CharacterSelectWnd, OnNameComboBox)

// 0x10099340 (vslot 69) - the animation picked plays looped on the player's pawn. Items(Index) is read up to
// Index == Num (">=" in the stock code).
INT NCMSView_CharacterSelectWnd::OnAniNameComboBox(INT Index)
{
    guard(NCMSView_CharacterSelectWnd::OnAniNameComboBox);
    // the combo box's Items array (the header keeps it as filler): data at +0x1DC, count at +0x1E0
    TArray<NString*>& ComboItems = *(TArray<NString*>*)((BYTE*)AniCombo + 0x1DC);
    if (ComboItems.Num() >= Index)
    {
        TCHAR* Name = ComboItems(Index)->GetAt(0);
        APawn* Pawn = ViewActor()->Pawn;
        Pawn->AnimFlags |= 0x10;
        Pawn->Unknown13D4 &= ~1;
        Pawn->PlayAnim(0, FName(Name, FNAME_Add), 1.0f, 0.f, 1, 0);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10099340, NCMSView_CharacterSelectWnd, OnAniNameComboBox)

/*-----------------------------------------------------------------------------
    NCMSViewrWnd.
-----------------------------------------------------------------------------*/

// 0x1009a1a0 - the tabs, the textures and the level are made in OnCreate; the nineteen User records are built by
// their constructors, the grids cleared (the selection window's pointer is not)
NCMSViewrWnd::NCMSViewrWnd() : TabCtrl(NULL), BackTex(NULL), HtmlBackTex(NULL), Level(NULL)
{
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    ItemWnd[2] = NULL;
    ItemWnd[3] = NULL;
}
NW_HOOK_CTOR(0x1009a1a0, NCMSViewrWnd)

// 0x10097d80 (vslot 25) - the two buttons search; Enter in the ID or name line reports through the edit's id into
// slot 70 / 71, bodies shared with other classes (code 1: the other search). C3 no longer refuses the messages of a
// disabled window here.
INT NCMSViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MSVIEW_SEARCHNAME)
            return OnSearchName() != 0;
        if (wParam == NID_MSVIEW_SEARCHID)
            return OnSearchID() != 0;
        if (wParam == NID_MSVIEW_IDEDIT)
            return ::nw::VirtualCall<INT>(this, 70, (INT)lParam) != 0;
        if (wParam == NID_MSVIEW_NAMEEDIT)
            return ::nw::VirtualCall<INT>(this, 71, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10097d80, NCMSViewrWnd, WindowProc)

// 0x100b2570 (slot 70 here, slot 74 of NCNpcView_CharacterSelectWnd: one body, both call slot 69) - Enter (code 1) in
// the ID line searches by ID
INT NCMSViewrWnd::OnIDEdit(INT Code)
{
    if (Code == 1)
        OnSearchID();
    return 1;
}
NW_HOOK_METHOD(0x100b2570, NCMSViewrWnd, OnIDEdit)

// (C3: the stock body of slot 71 is NCAuthWnd::OnPasswordNotify's, 0x100b2360) - Enter (code 1) in the name line
// searches by name
INT NCMSViewrWnd::OnNameEdit(INT Code)
{
    if (Code == 1)
        OnSearchName();
    return 1;
}

// 0x1009a3c0 (vslot 37) - the title bar, the name and ID search lines, the three kinds' check boxes (the passive one
// off) and the two search buttons, the four grids under their tabs ("magic", "active", "passive", "search"), the
// selection window (top-level), then the tables loaded and the first character shown. NCWnd::OnCreate is not called.
INT NCMSViewrWnd::OnCreate()
{
    guard(NCMSViewrWnd::OnCreate);
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_MSVIEW_FRAME;
    FrameCtrl->ID = NID_MSVIEW_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    HtmlBackTex = FindTexture(TEXT("L2UI.etcwnd.NPCHtml_BACK"));
    BackTex = FindTexture(TEXT("L2UI.SkillWnd.SkillBack"));

    NameEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    NameEdit->ID = NID_MSVIEW_NAMEEDIT;
    NameEdit->SetPosition(55, 30, 65, 16);
    CreateChild(NameEdit, NWS_VISIBLE);
    IDEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    IDEdit->ID = NID_MSVIEW_IDEDIT;
    IDEdit->SetPosition(55, 55, 65, 16);
    CreateChild(IDEdit, NWS_VISIBLE);

    MagicCheck = new NCCheckBox(STR_MSViewMagic, 0, 0);
    MagicCheck->SetPosition(15, 80, 65, 16);
    CreateChild(MagicCheck, NWS_VISIBLE);
    MagicCheck->bChecked = 1;
    ActiveCheck = new NCCheckBox(STR_MSViewActive, 0, 0);
    ActiveCheck->SetPosition(80, 80, 65, 16);
    CreateChild(ActiveCheck, NWS_VISIBLE);
    ActiveCheck->bChecked = 1;
    PassiveCheck = new NCCheckBox(STR_MSViewPassive, 0, 0);
    PassiveCheck->SetPosition(145, 80, 65, 16);
    CreateChild(PassiveCheck, NWS_VISIBLE);
    PassiveCheck->bChecked = 0;

    SearchNameButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                          TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchNameButton->ID = NID_MSVIEW_SEARCHNAME;
    SearchNameButton->SetPosition(140, 30, 32, 16);
    CreateChild(SearchNameButton, NWS_VISIBLE);
    SearchNameButton->SetTextID(STR_MSViewSearch);
    SearchIDButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                        TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchIDButton->ID = NID_MSVIEW_SEARCHID;
    SearchIDButton->SetPosition(140, 55, 32, 16);
    CreateChild(SearchIDButton, NWS_VISIBLE);
    SearchIDButton->SetTextID(STR_MSViewSearch);

    ItemWnd[0] = new NCMSView_InvenItemWnd(6, 7, 80, 32);
    ItemWnd[0]->SetPosition(16, 133, 220, 255);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);
    ItemWnd[1] = new NCMSView_InvenItemWnd(6, 7, 80, 32);
    ItemWnd[1]->SetPosition(16, 133, 220, 255);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);
    ItemWnd[2] = new NCMSView_InvenItemWnd(6, 7, 80, 32);
    ItemWnd[2]->SetPosition(16, 133, 220, 255);
    CreateChild(ItemWnd[2], NWS_VISIBLE | 0x20);
    ItemWnd[3] = new NCMSView_InvenItemWnd(6, 7, 80, 32);
    ItemWnd[3]->SetPosition(16, 133, 220, 255);
    CreateChild(ItemWnd[3], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_MSVIEW_TAB;
    TabCtrl->SetPosition(15, 102, 138, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_MSViewMagic, ItemWnd[0], 35, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_MSViewActive, ItemWnd[1], 35, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    TabCtrl->AddTab(STR_MSViewPassive, ItemWnd[2], 35, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    TabCtrl->AddTab(STR_MSViewSearch, ItemWnd[3], 33, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);

    for (INT i = 0; i < 19; i++)
        Characters[i].Pawn = NULL;
    CharSelectWnd = new NCMSView_CharacterSelectWnd;
    CharSelectWnd->SetPosition(34, 34, 208, 161);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE | NWS_NOHITTEST);
    AddAllItem();
    AddAllCharacter();
    Level = GameLevel();
    CharSelectWnd->OnNameComboBox(0);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1009a3c0, NCMSViewrWnd, OnCreate)

// 0x10098ac0 (vslot 38) - the selection window deleted (top-level, not cleared); the search
// controls are left to NCWnd::OnDestroy
INT NCMSViewrWnd::OnDestroy()
{
    guard(NCMSViewrWnd::OnDestroy);
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, ItemWnd[2]);
    DestroyChild(this, ItemWnd[3]);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, FrameCtrl);
    delete CharSelectWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10098ac0, NCMSViewrWnd, OnDestroy)

// 0x10098700 (vslot 44) - with the selection window
void NCMSViewrWnd::ShowWindow()
{
    guard(NCMSViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x10098700, NCMSViewrWnd, ShowWindow)

// 0x10098670 (vslot 45)
void NCMSViewrWnd::HideWindow()
{
    guard(NCMSViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x10098670, NCMSViewrWnd, HideWindow)

// 0x100988f0 (vslot 56) - the grids' background, the search's, and the search lines' labels ("skill name:", "skill
// ID:" in Korean) centered in 55 x 16 before them (the origin goes on before the clip)
INT NCMSViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCMSViewrWnd::OnPaint);
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 100, 240, 300, 0, 0, 240, 300, BackTex);
    if (HtmlBackTex)
        Device->DrawStretchedTex(0, 0, 240, 100, 0, 0, 300, 60, HtmlBackTex, 0xFF);
    NCGDevice::GetTextExtent(&Size, TEXT("\x00BD\x00BA\x00C5\x00B3\x00B8\x00ED\x003A"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 30, 0xFFDCDCDC,
                     TEXT("\x00BD\x00BA\x00C5\x00B3\x00B8\x00ED\x003A"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("\x00BD\x00BA\x00C5\x00B3") TEXT("ID:"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 55, 0xFFDCDCDC,
                     TEXT("\x00BD\x00BA\x00C5\x00B3") TEXT("ID:"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100988f0, NCMSViewrWnd, OnPaint)

// the kind of a skill for the search (Type: its name in Korean): 0 when the search leaves it out; toggles (and other
// kinds) keep the Type of the skill before them
static FORCEINLINE UBOOL SearchKind(NCMSViewrWnd* Wnd, NSkillDataC3* Data, TCHAR* Type)
{
    if (Data->bMagic)
    {
        if (!Wnd->MagicCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\x00B8\x00B6\x00B9\x00FD"));                          // "magic"
    }
    else if (Data->OperateType >= 0 && Data->OperateType <= 1)
    {
        if (!Wnd->ActiveCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\x00BE\x00D7\x00C6\x00BC\x00BA\x00EA"));              // "active"
    }
    else if (Data->OperateType == 2)
    {
        if (!Wnd->PassiveCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\x00C6\x00D0\x00BD\x00C3\x00BA\x00EA"));              // "passive"
    }
    return 1;
}

// A skill found by a search into the fourth grid (level 1 only): the icon of its level (or "NWindow.Darkyellow"),
// "ID = n\nTYPE = kind" as its description.
static FORCEINLINE void AddFoundSkill(NCMSViewrWnd* Wnd, INT SkillID, INT Level, const TCHAR* Type)
{
    NItemInfo Info;
    if (Level != 1)
        return;
    wcscpy(Info.IconName, GConsole->GetSkillIconName(SkillID, 1));
    Info.Icon = Wnd->FindTexture(GConsole->GetSkillIconName(SkillID, 1));
    if (GConsole->GetSkillIconName(SkillID, 1))
        Info.Icon = Wnd->FindTexture(GConsole->GetSkillIconName(SkillID, 1));
    if (!Info.Icon)
        Info.Icon = Wnd->FindTexture(TEXT("NWindow.Darkyellow"));
    if (GConsole->GetSkillName(SkillID, 1))
    {
        appSprintf(Info.Description, TEXT("ID = %d\nTYPE = %s"), SkillID, Type);
        appStrcpy(Info.Name, GConsole->GetSkillName(SkillID, 1));
    }
    Info.ID = SkillID;
    Info.BodyPart = 3;
    Wnd->AddItem(3, &Info);
    Wnd->ItemWnd[3]->MaxItems++;
}

// 0x10099b80 (vslot 68) - the skills whose name holds the text, of the kinds checked
INT NCMSViewrWnd::OnSearchName()
{
    const TCHAR* Text = NameEdit->CommitText();
    if (Text && appStrlen(Text) > 0)
    {
        ClearGrid(ItemWnd[3]);
        NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
        TCHAR Type[20];
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            NSkillDataC3* Data = (NSkillDataC3*)Map->Pairs(i).Value;
            if (!Data)
                continue;
            INT SkillID = Data->SkillID;
            INT Level = Data->Level;
            if (!Data->Name.Num() || !appStrstr(*Data->Name, Text) || !SearchKind(this, Data, Type))
                continue;
            AddFoundSkill(this, SkillID, Level, Type);
        }
        TabCtrl->SetTab(3, 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x10099b80, NCMSViewrWnd, OnSearchName)

// 0x10099e40 (vslot 69) - the skill of the id typed, of the kinds checked
INT NCMSViewrWnd::OnSearchID()
{
    const TCHAR* Text = IDEdit->CommitText();
    if (Text && appStrlen(Text) > 0)
    {
        INT ID = appAtoi(Text);
        ClearGrid(ItemWnd[3]);
        NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
        TCHAR Type[20];
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            NSkillDataC3* Data = (NSkillDataC3*)Map->Pairs(i).Value;
            if (!Data)
                continue;
            INT SkillID = Data->SkillID;
            INT Level = Data->Level;
            if (SkillID != ID || !SearchKind(this, Data, Type))
                continue;
            AddFoundSkill(this, SkillID, Level, Type);
        }
        TabCtrl->SetTab(3, 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x10099e40, NCMSViewrWnd, OnSearchID)

// 0x100987a0 - into a tab's grid, unless the info's +0x1798 is set (C2 +0xC8C; the NItemInfo "worn" flag)
INT NCMSViewrWnd::AddItem(INT Tab, NItemInfo* Info)
{
    guard(NCMSViewrWnd::AddItem);
    if (InfoInt(Info, 0x1798) == 0 && Tab >= 0 && Tab < 4 && ItemWnd[Tab])
        ItemWnd[Tab]->AddItem(Info);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100987a0, NCMSViewrWnd, AddItem)

// 0x10097e40 - the characters: 19 records cleared and given the race, sex and class of the table (see
// GMSViewCharacters), then 18 names "race sex class" in the selection window
void NCMSViewrWnd::AddAllCharacter()
{
    guard(NCMSViewrWnd::AddAllCharacter);
    for (INT i = 0; i < 19; i++)
    {
        memset(&Characters[i], 0, sizeof(User));
        Characters[i].Race = GMSViewCharacters[i].Race;
        Characters[i].Sex = GMSViewCharacters[i].Sex;
        Characters[i].ClassID = GMSViewCharacters[i].ClassID;
    }
    // (race, male / female, fighter / mystic) of each name, in the stock order
    static const BYTE Names[18][3] =
    {
        { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1, 1 },
        { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 1 }, { 1, 1, 1 },
        { 2, 0, 0 }, { 2, 1, 0 }, { 2, 0, 1 }, { 2, 1, 1 },
        { 3, 0, 0 }, { 3, 1, 0 }, { 3, 0, 1 }, { 3, 1, 1 },
        { 4, 0, 0 }, { 4, 1, 0 },
    };
    TCHAR Name[104];
    for (INT n = 0; n < 18; n++)
    {
        appSprintf(Name, TEXT("%s%s%s"), *GL2GameData.SysString[STR_MSViewRace0 + Names[n][0]],
                   *GL2GameData.SysString[STR_MSViewMale + Names[n][1]],
                   *GL2GameData.SysString[STR_MSViewFighter + Names[n][2]]);
        CharSelectWnd->NameCombo->AddItem(Name);
    }
    unguard;
}
NW_HOOK_METHOD(0x10097e40, NCMSViewrWnd, AddAllCharacter)

// 0x10099920 - the level-1 skills into the first three grids by kind (magic, active, passive and toggles), their
// capacity raised with each; the icon of their level (its name copied without a NULL check) or "NWindow.Darkyellow",
// "ID = n" as their description
void NCMSViewrWnd::AddAllItem()
{
    guard(NCMSViewrWnd::AddAllItem);
    NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NSkillDataC3* Data = (NSkillDataC3*)Map->Pairs(i).Value;
        if (!Data)
            continue;
        INT SkillID = Data->SkillID;
        INT Level = Data->Level;
        NItemInfo Info;
        if (Level != 1)
            continue;
        wcscpy(Info.IconName, GConsole->GetSkillIconName(SkillID, 1));
        Info.Icon = FindTexture(GConsole->GetSkillIconName(SkillID, 1));
        if (GConsole->GetSkillIconName(SkillID, 1))
            Info.Icon = FindTexture(GConsole->GetSkillIconName(SkillID, 1));
        if (!Info.Icon)
            Info.Icon = FindTexture(TEXT("NWindow.Darkyellow"));
        if (GConsole->GetSkillName(SkillID, 1))
        {
            swprintf(Info.Description, TEXT("ID = %d"), SkillID);
            wcscpy(Info.Name, GConsole->GetSkillName(SkillID, 1));
        }
        Info.ID = SkillID;
        INT Tab;
        if (Data->bMagic)
            Tab = 0;
        else if (Data->OperateType != 0 && Data->OperateType != 1)
            Tab = 2;
        else
            Tab = 1;
        Info.BodyPart = Tab;
        AddItem(Tab, &Info);
        ItemWnd[Tab]->MaxItems++;
    }
    unguard;
}
NW_HOOK_METHOD(0x10099920, NCMSViewrWnd, AddAllItem)

// 0x1009a120 - the vtable, ConditionalDestroy, NCInvenItemWnd
NW_HOOK_DTOR(0x1009a120, NCMSView_InvenItemWnd)
// 0x1009a270 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x1009a270, NCMSViewrWnd)
// 0x1009a340 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x1009a340, NCMSView_CharacterSelectWnd)

// 0x100988c0 - Cast<USkillVisualEffect> (the item window's right click on a skill effect)
static USkillVisualEffect* CastToSkillVisualEffect(UObject* Object)
{
    if (Object && Object->IsA(USkillVisualEffect::StaticClass()))
        return (USkillVisualEffect*)Object;
    return NULL;
}
NW_HOOK_FUNC(0x100988C0, CastToSkillVisualEffect)

NW_CHECK_VTABLE(NCMSView_InvenItemWnd, 0x1018cf00, 76)
NW_CHECK_VTABLE(NCMSView_CharacterSelectWnd, 0x1018d150, 70)
NW_CHECK_VTABLE(NCMSViewrWnd, 0x1018d030, 72)
