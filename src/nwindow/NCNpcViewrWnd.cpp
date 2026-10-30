/*=============================================================================
    NCNpcViewrWnd.cpp - the NPC viewer, its NPC list and item grids. Stock (C3): 0x100b1e70 ..
    0x100b6800 (the grids' WindowProc, 0x10097bc0, also serves the skill viewer's and the pawn
    viewer's).
=============================================================================*/
#include "nwindow/NCNpcViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NString.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// UObject::StaticLoadClass flags (UE2)
enum { LOAD_NoFail = 0x1 };

// sysstring labels of the selection window's edit lines (names ours)
enum { STR_NpcViewSpeed = 432, STR_NpcViewRate = 316 };

template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

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
static FORCEINLINE void ClearGrid(NCNpcView_InvenItemWnd* Grid)
{
    ::nw::VirtualCall<void>(Grid, 72);
}

// the NPCs listed: those without a mesh name, or with an NPC mesh when that check is on, or a monster mesh when that
// one is
static FORCEINLINE UBOOL IsListedNpc(NCNpcView_CharacterSelectWnd* Wnd, NViewNpcData* Npc)
{
    if (!NameIndex(Npc->MeshName))
        return 1;
    if (Wnd->NpcCheck->bChecked && appStristr(*Npc->MeshName, TEXT("lineagenpcs")))
        return 1;
    return Wnd->MonsterCheck->bChecked && appStristr(*Npc->MeshName, TEXT("lineagemonsters"));
}

// the engine's box trace that puts a spawned pawn on the ground, as the viewers repeat it (NCPawnViewrWnd::
// OnSpawnCharacter, NCNpcView_CharacterSelect_List::OnLButtonDblClick, OnSummon, OnSpawnTest): 30 tries, 30 units
// longer both ways each time, from 50 units above the pawn's feet; a hit closer than 1.9 units (the time times 12) is
// raised to the middle of 1.9 .. 2.4
static FORCEINLINE void DropToGround(ULevel* Level, APawn* Pawn, const FVector& Where)
{
    FVector End = Where;
    FVector Start = Where;
    Start.Z += 50.0f;
    for (INT i = 0; i < 30; i++)
    {
        Start.Z += 30.0f;
        End.Z -= 30.0f;
        FCheckResult Hit(1.0f);
        Level->SingleLineCheck(Hit, Pawn, End, Start, 0x86,
                               FVector(Pawn->CollisionRadius, Pawn->CollisionRadius, Pawn->CollisionHeight));
        if (Hit.Location.Z != 0.0f)
        {
            DOUBLE Step = Hit.Time * 12.0;
            if (Step < 1.9f)
                Hit.Location.Z = (FLOAT)(((DOUBLE)2.4f + 1.9f) * 0.5f - Step + Hit.Location.Z);
            Level->FarMoveActor(Pawn, Hit.Location, 0, 0, 0);
            break;
        }
    }
}

// what the double click on a line and the test button both end with: the pawn's speed into the selection window and its
// speed edit line, the factor 1 into its rate line, the animation combo box filled with the viewer pawn's animations
static FORCEINLINE void ShowPawnInViewer(APawn* Pawn)
{
    NpcViewer()->CharSelectWnd->Speed = (INT)Pawn->Speed;
    TCHAR Speed[128];
    appSprintf(Speed, TEXT("%d"), (INT)Pawn->Speed);
    NpcViewer()->CharSelectWnd->SpeedEdit->ClearText();
    NpcViewer()->CharSelectWnd->SpeedEdit->InsertText(Speed, 0);
    NpcViewer()->CharSelectWnd->Rate = 1.0f;
    NpcViewer()->CharSelectWnd->RateEdit->ClearText();
    NpcViewer()->CharSelectWnd->RateEdit->InsertText(TEXT("1"), 0);
    APawn* ViewPawn = ViewActor()->Pawn;
    if (ViewPawn)
    {
        UMesh* Mesh = ViewPawn->Mesh;
        if (Mesh)
        {
            Mesh->MeshGetInstance(ViewPawn);
            UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
            if (Anim)
            {
                // C3: the sequences are at +0x60 of the set, 0x44 bytes each (the header has the C2 +0x44, 0x40)
                INT Num = *(INT*)((BYTE*)Anim + 0x64);
                BYTE* Seqs = *(BYTE**)((BYTE*)Anim + 0x60);
                FName Sequence = NAME_None;
                NpcViewer()->CharSelectWnd->AniCombo->ClearItems();
                for (INT i = 0; i < Num; i++)
                {
                    Sequence = *(FName*)(Seqs + i * 0x44);
                    NpcViewer()->CharSelectWnd->AniCombo->AddItem(*Sequence);
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    NCNpcView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x100b5090 - the grid's defaults and this class's vtable
NCNpcView_InvenItemWnd::NCNpcView_InvenItemWnd()
{
}
NW_HOOK_CTOR(0x100B5090, NCNpcView_InvenItemWnd)

// 0x100b5130 (ret 0x10) - the 4-argument constructor: the grid of six arguments with both gaps 1; the NPC viewer builds
// its two grids with it
NCNpcView_InvenItemWnd::NCNpcView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x100B5130, NCNpcView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10097bc0 (vslot 25) - the stock body is shared with the skill viewer's and the pawn viewer's grids (their
// vtables point here too); C3 no longer refuses the messages of a disabled window
INT NCNpcView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10097bc0, NCNpcView_InvenItemWnd, WindowProc)

// 0x100b25d0 (vslot 64) - the item under the mouse dresses the viewer's pawn, when it has an NPC id: its id in the
// slot of its body part (a two-handed one empties both hands, a one-handed one the two-handed slot; the same item
// again takes it off), the pawn's resources again
INT NCNpcView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_InvenItemWnd::OnRButtonDown);
    APawn* Pawn = NpcViewer()->Npcs[0].Pawn;
    if (Pawn && Pawn->NpcID)
    {
        Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
        if (Selected >= 0 && Selected < Items.Num())
        {
            User& Npc = NpcViewer()->Npcs[0];
            INT Slot = Items(Selected)->BodyPart;
            if (Slot == BODYSLOT_LRHand)
            {
                Npc.Equipped[BODYSLOT_RHand] = 0;
                Npc.Equipped[BODYSLOT_LHand] = 0;
            }
            else if (Slot == BODYSLOT_RHand || Slot == BODYSLOT_LHand)
                Npc.Equipped[BODYSLOT_LRHand] = 0;
            NItemEntry* Entry = Items(Selected);
            INT& Worn = Npc.Equipped[Entry->BodyPart];
            Worn = (Worn == Entry->ID) ? 0 : Entry->ID;
            Npc.SetPawnResource();
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b25d0, NCNpcView_InvenItemWnd, OnRButtonDown)

// 0x100b1ea0 (vslot 75) - as the right button
INT NCNpcView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_InvenItemWnd::OnLButtonDblClick);
    OnRButtonDown(wParam, lParam);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1ea0, NCNpcView_InvenItemWnd, OnLButtonDblClick)

/*-----------------------------------------------------------------------------
    NCNpcView_CharacterSelect_List.
-----------------------------------------------------------------------------*/

// 0x100b5270 - NCDebugWnd's default constructor, then this class's vtable
NCNpcView_CharacterSelect_List::NCNpcView_CharacterSelect_List()
{
}
NW_HOOK_CTOR(0x100B5270, NCNpcView_CharacterSelect_List)

// 0x100b5310 - NCDebugWnd's default constructor, then the size of the list
NCNpcView_CharacterSelect_List::NCNpcView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines)
{
    MaxLines = InMaxLines;
    VisibleLines = InVisibleLines;
}
NW_HOOK_CTOR_ARGS(0x100b5310, NCNpcView_CharacterSelect_List, (INT A, INT B), (A, B))

// 0x100b52c0
NW_HOOK_DTOR(0x100b52c0, NCNpcView_CharacterSelect_List)

// 0x100b2380 (vslot 25) - also NCPawnView_CharacterSelect_List's (hooked once, here); C3 no longer refuses the
// messages of a disabled window here
INT NCNpcView_CharacterSelect_List::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCDebugWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100b2380, NCNpcView_CharacterSelect_List, WindowProc)

// 0x100b23a0 (vslot 37)
INT NCNpcView_CharacterSelect_List::OnCreate()
{
    guard(NCNpcView_CharacterSelect_List::OnCreate);
    NCDebugWnd::OnCreate();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b23a0, NCNpcView_CharacterSelect_List, OnCreate)

// 0x100b24a0 (vslot 38)
INT NCNpcView_CharacterSelect_List::OnDestroy()
{
    guard(NCNpcView_CharacterSelect_List::OnDestroy);
    NCDebugWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b24a0, NCNpcView_CharacterSelect_List, OnDestroy)

// 0x100b2420 (vslot 56)
INT NCNpcView_CharacterSelect_List::OnPaint(NCGDevice* Device)
{
    guard(NCNpcView_CharacterSelect_List::OnPaint);
    NCDebugWnd::OnPaint(Device);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b2420, NCNpcView_CharacterSelect_List, OnPaint)

// 0x100b2540 (vslot 67) - also NCPawnView_CharacterSelect_List's, as 67 .. 69
INT NCNpcView_CharacterSelect_List::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCDebugWnd::OnMouseWheel(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x100b2540, NCNpcView_CharacterSelect_List, OnMouseWheel)

// 0x100c3a50 (vslot 73)
INT NCNpcView_CharacterSelect_List::ScrollUp()
{
    NCDebugWnd::ScrollUp();
    return 1;
}
NW_HOOK_METHOD(0x100c3a50, NCNpcView_CharacterSelect_List, ScrollUp)

// 0x100c3a60 (vslot 74)
INT NCNpcView_CharacterSelect_List::ScrollDown()
{
    NCDebugWnd::ScrollDown();
    return 1;
}
NW_HOOK_METHOD(0x100c3a60, NCNpcView_CharacterSelect_List, ScrollDown)

// 0x100b2520 (vslot 75)
INT NCNpcView_CharacterSelect_List::SetTopIndex(INT Index)
{
    NCDebugWnd::SetTopIndex(Index);
    return 1;
}
NW_HOOK_METHOD(0x100b2520, NCNpcView_CharacterSelect_List, SetTopIndex)

// 0x100b2560 - kept out of line, the stock callers call it
__declspec(noinline) void NCNpcView_CharacterSelect_List::SetScrollRange(INT Total, INT Page)
{
    ScrollBar->SetScrollRange(Total, Page);
}
NW_HOOK_METHOD(0x100b2560, NCNpcView_CharacterSelect_List, SetScrollRange)

// 0x100b2f60 (vslot 62) - C3: the line chosen also fills the selection window's text lines: the first the NPC's mesh
// (or its name when the mesh check is on), then its four texture names
INT NCNpcView_CharacterSelect_List::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NCListBox::OnLButtonDown(wParam, lParam);
    NViewNpcData* Npc = (NViewNpcData*)GetItemParam(Selected);
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[0], TEXT(""));
    if (NpcViewer()->CharSelectWnd->MeshNameCheck->bChecked == 0)
        appStrcpy(NpcViewer()->CharSelectWnd->InfoText[0], *Npc->MeshName);
    else
        appStrcpy(NpcViewer()->CharSelectWnd->InfoText[0], GL2GameData.GetNpcName(Npc->ID));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[1], TEXT(""));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[2], TEXT(""));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[3], TEXT(""));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[4], TEXT(""));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[1], **(L2FName*)((BYTE*)Npc + 0x2C));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[2], **(L2FName*)((BYTE*)Npc + 0x30));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[3], **(L2FName*)((BYTE*)Npc + 0x34));
    appStrcpy(NpcViewer()->CharSelectWnd->InfoText[4], **(L2FName*)((BYTE*)Npc + 0x38));
    return 1;
}
NW_HOOK_METHOD(0x100b2f60, NCNpcView_CharacterSelect_List, OnLButtonDown)

// 0x100b2970 (vslot 72) - the NPC of the line becomes the player's pawn: a player controller spawned for it, its pawn
// class loaded by the NPC's class name (and logged), the pawn spawned 60 units above the player's, which it replaces
// (the old one destroyed; the controller kept is the viewport's). The viewer's pawn gets its resources and is moved
// onto the ground by the box trace of NCNpcView_CharacterSelectWnd::OnSummon, the selection window's edit lines its
// speed and the factor 1, the animation combo box its animations.
INT NCNpcView_CharacterSelect_List::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_CharacterSelect_List::OnLButtonDblClick);
    FName ControllerClassName = NAME_None;
    if (Selected < 0)
        return 1;
    ULevel* Level = GameLevel();
    APlayerController* Viewer = ViewActor();
    ControllerClassName = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* Controller = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, *ControllerClassName, NULL, LOAD_NoFail, NULL));
    if (!Controller)
        appFailAssert("pController", "Src\\NNpcViewerWnd.cpp", 502);
    NViewNpcData* Npc = (NViewNpcData*)GetItemParam(Selected);
    NpcViewer()->Npcs[0].NpcID = Npc->ID;
    Controller->PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL, *Npc->ClassName, NULL, 0, NULL);
    if (GLog)
        GLog->Logf2(TEXT("PawnClass = %s"), *Npc->ClassName);
    if (!Controller->PawnClass)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    Location.Z += 60.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(Controller->PawnClass, NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1,
                                            Controller, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(ViewActor()->Pawn, 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = Viewer;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Pawn->NpcID = Npc->ID;
    NpcViewer()->Npcs[0].Pawn = Pawn;
    NpcViewer()->Npcs[0].SetPawnResource();
    DropToGround(Level, Pawn, ViewActor()->Pawn->Location);
    ShowPawnInViewer(Pawn);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b2970, NCNpcView_CharacterSelect_List, OnLButtonDblClick)

// 0x100b3140 - every line's text again: the NPC's mesh, or its name (C3: the name is the FString at +4)
void NCNpcView_CharacterSelect_List::ToggleNames(UBOOL bMeshNames)
{
    guard(NCNpcView_CharacterSelect_List::ToggleNames);
    for (INT i = 0; i < Items.Num(); i++)
    {
        NListItem& Item = Items(i);
        Item.Text->Empty();
        NViewNpcData* Npc = (NViewNpcData*)Item.Param;
        if (bMeshNames == 0)
            Item.Text->Insert(*Npc->Name, 0);
        else
            Item.Text->Insert(*Npc->MeshName, 0);
    }
    unguard;
}
NW_HOOK_METHOD(0x100b3140, NCNpcView_CharacterSelect_List, ToggleNames)

/*-----------------------------------------------------------------------------
    NCNpcView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x100b5340 - the speed factor starts at 1
NCNpcView_CharacterSelectWnd::NCNpcView_CharacterSelectWnd()
{
    Rate = 1.0f;
}
NW_HOOK_CTOR(0x100b5340, NCNpcView_CharacterSelectWnd)

// 0x100b47d0 (vslot 25) - the controls: the animation combo (a plain call, not a slot), search and reset, apply,
// mesh-name check, summon and the test spawner; Enter in the search line (0xF4) goes through slot 74, a body shared
// with other classes (code 1: the search)
INT NCNpcView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_NPCVIEW_SELECT_ANIM)
            return OnAniNameCombo((INT)lParam) != 0;
        if (wParam == NID_NPCVIEW_SELECT_SEARCH)
            return OnSearch() != 0;
        if (wParam == NID_NPCVIEW_SELECT_RESET)
            return OnReset() != 0;
        if (wParam == NID_NPCVIEW_SELECT_APPLY)
            return OnApply() != 0;
        if (wParam == NID_NPCVIEW_SELECT_MESHNAME)
            return OnToggleNames() != 0;
        if (wParam == NID_NPCVIEW_SELECT_SUMMON)
            return OnSummon() != 0;
        if (wParam == NID_NPCVIEW_SELECT_SPAWNTEST)
            return OnSpawnTest() != 0;
        if (wParam == NID_NPCVIEW_SELECT_SEARCHEDIT)
            return ::nw::VirtualCall<INT>(this, 74, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b47d0, NCNpcView_CharacterSelectWnd, WindowProc)

// (C3: the stock body of slot 74 is NCMSViewrWnd::vfn070's, 0x100b2570, a function shared with other classes) - Enter
// (code 1) in the search line searches
INT NCNpcView_CharacterSelectWnd::OnSearchEdit(INT Code)
{
    if (Code == 1)
        OnSearch();
    return 1;
}

// 0x100b53e0 (vslot 37) - a title bar (the window is 350 x 560), the animation combo box, the NPC list and its search
// line, buttons and checks, then the mesh and texture edit lines with the labels OnPaint draws, the height and radius
// lines and the speed and rate lines; the title "NpcViewer"; NCWnd::OnCreate runs first
INT NCNpcView_CharacterSelectWnd::OnCreate()
{
    guard(NCNpcView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();

    FrameCtrl = new NCFrameCtrl(0, 0, 0);
    FrameID = NID_NPCVIEW_SELECT_FRAME;
    FrameCtrl->ID = NID_NPCVIEW_SELECT_FRAME;
    Width = 350.0f;
    Height = 560.0f;
    FrameCtrl->SetPosition(0, 0, 350, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    AniCombo = new NCComboBox;
    AniCombo->ID = NID_NPCVIEW_SELECT_ANIM;
    AniCombo->SetPosition(25, 30, 125, 18);
    CreateChild(AniCombo, NWS_VISIBLE);

    List = new NCNpcView_CharacterSelect_List(3010, 20);
    List->SetPosition(10, 50, 285, 250);
    CreateChild(List, NWS_VISIBLE);

    SearchEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    SearchEdit->ID = NID_NPCVIEW_SELECT_SEARCHEDIT;
    SearchEdit->SetPosition(10, 380, 80, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);

    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                      TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_NPCVIEW_SELECT_SEARCH;
    SearchButton->SetPosition(95, 380, 28, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(420);

    ResetButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                     TEXT("L2UI.Control.DefaultButton_click"), 0);
    ResetButton->ID = NID_NPCVIEW_SELECT_RESET;
    ResetButton->SetPosition(128, 380, 45, 16);
    CreateChild(ResetButton, NWS_VISIBLE);
    ResetButton->SetTextID(422);

    SummonButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                      TEXT("L2UI.Control.DefaultButton_click"), 0);
    SummonButton->ID = NID_NPCVIEW_SELECT_SUMMON;
    SummonButton->SetPosition(180, 380, 28, 16);
    CreateChild(SummonButton, NWS_VISIBLE);
    SummonButton->SetText(TEXT("\x00BC\x00D2\x0020\x00C8\x00AF"));                  // "summon"

    MeshNameCheck = new NCCheckBox(TEXT("Mesh\x00C0\x00CC\x00B8\x00A7"), 0, 0, 0);
    MeshNameCheck->ID = NID_NPCVIEW_SELECT_MESHNAME;
    MeshNameCheck->SetPosition(180, 32, 100, 16);
    CreateChild(MeshNameCheck, NWS_VISIBLE);
    MeshNameCheck->bChecked = 0;

    MonsterCheck = new NCCheckBox(TEXT("Monster"), 0, 0, 0);
    MonsterCheck->SetPosition(10, 400, 100, 16);
    CreateChild(MonsterCheck, NWS_VISIBLE);
    MonsterCheck->bChecked = 1;

    NpcCheck = new NCCheckBox(TEXT("NPC"), 0, 0, 0);
    NpcCheck->SetPosition(110, 400, 100, 16);
    CreateChild(NpcCheck, NWS_VISIBLE);
    NpcCheck->bChecked = 1;

    MeshEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    MeshEdit->ID = 0xFE;
    MeshEdit->SetPosition(60, 418, 230, 16);
    CreateChild(MeshEdit, NWS_VISIBLE);

    TexEdit[0] = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    TexEdit[0]->ID = 0xFF;
    TexEdit[0]->SetPosition(60, 438, 230, 16);
    CreateChild(TexEdit[0], NWS_VISIBLE);

    TexEdit[1] = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    TexEdit[1]->ID = 0x100;
    TexEdit[1]->SetPosition(60, 458, 230, 16);
    CreateChild(TexEdit[1], NWS_VISIBLE);

    TexEdit[2] = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    TexEdit[2]->ID = 0x101;
    TexEdit[2]->SetPosition(60, 478, 230, 16);
    CreateChild(TexEdit[2], NWS_VISIBLE);

    TexEdit[3] = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    TexEdit[3]->ID = 0x102;
    TexEdit[3]->SetPosition(60, 498, 230, 16);
    CreateChild(TexEdit[3], NWS_VISIBLE);

    HeightEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    HeightEdit->ID = 0x103;
    HeightEdit->SetPosition(60, 518, 32, 16);
    CreateChild(HeightEdit, NWS_VISIBLE);

    RadiusEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    RadiusEdit->ID = 0x104;
    RadiusEdit->SetPosition(160, 518, 32, 16);
    CreateChild(RadiusEdit, NWS_VISIBLE);

    SpawnTestButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                         TEXT("L2UI.Control.DefaultButton_click"), 0);
    SpawnTestButton->ID = NID_NPCVIEW_SELECT_SPAWNTEST;
    SpawnTestButton->SetPosition(200, 518, 32, 16);
    CreateChild(SpawnTestButton, NWS_VISIBLE);
    SpawnTestButton->SetText(TEXT("\x00BC\x00D2\x0020\x00C8\x00AF"));               // "summon"

    SpeedEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    SpeedEdit->ID = NID_NPCVIEW_SELECT_SPEED;
    SpeedEdit->SetPosition(60, 535, 32, 16);
    CreateChild(SpeedEdit, NWS_VISIBLE);

    RateEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    RateEdit->ID = NID_NPCVIEW_SELECT_RATE;
    RateEdit->SetPosition(160, 535, 32, 16);
    CreateChild(RateEdit, NWS_VISIBLE);

    ApplyButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                     TEXT("L2UI.Control.DefaultButton_click"), 0);
    ApplyButton->ID = NID_NPCVIEW_SELECT_APPLY;
    ApplyButton->SetPosition(200, 535, 32, 16);
    CreateChild(ApplyButton, NWS_VISIBLE);
    ApplyButton->SetTextID(218);

    SetTitle(TEXT("NpcViewer"));
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b53e0, NCNpcView_CharacterSelectWnd, OnCreate)

// 0x100b3300 (vslot 38) - the controls are deleted (the height, radius and test lines are left), the list without
// leaving the child list (and its field keeps the pointer)
INT NCNpcView_CharacterSelectWnd::OnDestroy()
{
    DestroyChild(this, AniCombo);
    DestroyChild(this, MeshEdit);
    DestroyChild(this, TexEdit[0]);
    DestroyChild(this, TexEdit[1]);
    DestroyChild(this, TexEdit[2]);
    DestroyChild(this, TexEdit[3]);
    DestroyChild(this, FrameCtrl);
    DestroyChild(this, ResetButton);
    DestroyChild(this, SearchEdit);
    DestroyChild(this, SearchButton);
    DestroyChild(this, SpeedEdit);
    DestroyChild(this, RateEdit);
    DestroyChild(this, SummonButton);
    DestroyChild(this, MeshNameCheck);
    DestroyChild(this, MonsterCheck);
    DestroyChild(this, NpcCheck);
    DestroyChild(this, ApplyButton);
    if (List)
        delete List;
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100b3300, NCNpcView_CharacterSelectWnd, OnDestroy)

// 0x100b3610 (vslot 56) - the title and the background, the five text lines of the selected NPC, the labels of the
// edit lines (centered in 64 pixels)
INT NCNpcView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNpcView_CharacterSelectWnd::OnPaint);
    SIZE Size;
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (TitleTex)
        Device->DrawStretchedTex(1, 0, 350, 20, 0, 0, 240, 20, TitleTex, 0xFF);
    if (BackTex)
        Device->DrawStretchedTex(0, 20, 350, 560, 0, 0, 242, 155, BackTex, 0xFF);
    Device->DrawText(10, 301, 0xFFFFFF00, InfoText[0], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 317, 0xFFFFFF00, InfoText[1], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 333, 0xFFFFFF00, InfoText[2], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 349, 0xFFFFFF00, InfoText[3], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 365, 0xFFFFFF00, InfoText[4], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Mesh"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 418, 0xFFDCDCDC, TEXT("Mesh"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Tex1"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 438, 0xFFDCDCDC, TEXT("Tex1"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Tex2"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 458, 0xFFDCDCDC, TEXT("Tex2"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Tex3"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 478, 0xFFDCDCDC, TEXT("Tex3"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Tex4"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 498, 0xFFDCDCDC, TEXT("Tex4"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_NpcViewSpeed], 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Height"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 518, 0xFFDCDCDC, TEXT("Height"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("Radius"), 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 100, (16 - Size.cy) / 2 + 518, 0xFFDCDCDC, TEXT("Radius"), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2, (16 - Size.cy) / 2 + 535, 0xFFDCDCDC, *GL2GameData.SysString[STR_NpcViewSpeed],
                     0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_NpcViewRate], 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 100, (16 - Size.cy) / 2 + 535, 0xFFDCDCDC,
                     *GL2GameData.SysString[STR_NpcViewRate], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b3610, NCNpcView_CharacterSelectWnd, OnPaint)

// 0x100b40b0 (vslot 68; the "TEST" button, id 0x100; C2 only, name ours; C3: the body is another one) - the mesh named
// in the first edit line loaded, then the double click's work for a fixed class ("LineageWarrior.LineagePawn", NPC id
// 0): a controller and the pawn spawned at the player's place, which they replace; the pawn's box from the height and
// radius lines (25 and 30 when empty), its mesh the line's and its textures the (up to four) non-empty lines, in
// order; moved onto the ground by the same trace and the viewer's lines filled. Nothing is done when the mesh does
// not load.
INT NCNpcView_CharacterSelectWnd::OnSpawnTest()
{
    if (MeshEdit->CommitText())
    {
        UObject* Mesh = UObject::StaticLoadObject(UMesh::StaticClass(), NULL, MeshEdit->CommitText(), NULL, 0, NULL);
        APlayerController* Viewer = ViewActor();
        if (Mesh && Viewer && Viewer->Pawn && Viewer->XLevel)
        {
            ULevel* Level = GameLevel();
            AController* Controller = (AController*)Level->SpawnActor(
                UObject::StaticLoadClass(AController::StaticClass(), NULL, TEXT("Engine.PlayerController"), NULL,
                                         LOAD_NoFail, NULL));
            if (!Controller)
                appFailAssert("pController", "Src\\NNpcViewerWnd.cpp", 1148);
            NpcViewer()->Npcs[0].NpcID = 0;
            Controller->PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL,
                                                             TEXT("LineageWarrior.LineagePawn"), NULL, 0, NULL);
            if (Controller->PawnClass)
            {
                FVector Location = ViewActor()->Pawn->Location;
                APawn* Pawn = (APawn*)Level->SpawnActor(Controller->PawnClass, NAME_None, Location,
                                                        FRotator(0, 0, 0), NULL, 1, 1, Controller, NULL);
                if (Pawn)
                {
                    Level->DestroyActor(ViewActor()->Pawn, 0);
                    ViewActor()->Pawn = Pawn;
                    Pawn->Controller = Viewer;
                    Pawn->Role = ROLE_Authority;
                    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
                    Pawn->NpcID = 0;
                    NpcViewer()->Npcs[0].Pawn = Pawn;
                    if (HeightEdit->CommitText())
                        Pawn->CollisionHeight = appAtof(HeightEdit->CommitText());
                    else
                        Pawn->CollisionHeight = 25.0f;
                    if (RadiusEdit->CommitText())
                        Pawn->CollisionRadius = appAtof(RadiusEdit->CommitText());
                    else
                        Pawn->CollisionRadius = 30.0f;
                    Pawn->SetMeshes(7, FName(MeshEdit->CommitText(), FNAME_Add), 0);
                    INT Count = 0;
                    if (TexEdit[0]->CommitText())
                    {
                        Pawn->SetTexes(7, FName(TexEdit[0]->CommitText(), FNAME_Add), 0);
                        Count = 1;
                    }
                    if (TexEdit[1]->CommitText())
                    {
                        Pawn->SetTexes(7, FName(TexEdit[1]->CommitText(), FNAME_Add), Count);
                        Count++;
                    }
                    if (TexEdit[2]->CommitText())
                    {
                        Pawn->SetTexes(7, FName(TexEdit[2]->CommitText(), FNAME_Add), Count);
                        Count++;
                    }
                    if (TexEdit[3]->CommitText())
                        Pawn->SetTexes(7, FName(TexEdit[3]->CommitText(), FNAME_Add), Count);
                    DropToGround(Level, Pawn, Location);
                    ShowPawnInViewer(Pawn);
                }
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100b40b0, NCNpcView_CharacterSelectWnd, OnSpawnTest)

// 0x100b4dc0 (vslot 69) - the NPCs listed whose name (or mesh, when checked) holds the text; a number typed that is an
// NPC's id lists that NPC alone (the walk stops there)
INT NCNpcView_CharacterSelectWnd::OnSearch()
{
    const TCHAR* Text = SearchEdit->CommitText();
    INT Count = 0;
    if (Text)
    {
        List->Clear();
        NDataMap<NViewNpcData>* Map = GL2GameData.NpcTable;
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            NViewNpcData* Npc = Map->Pairs(i).Value;
            if (!Npc || !Npc->HasName() || !IsListedNpc(this, Npc))
                continue;
            if (appAtoi(Text) == Npc->ID)
            {
                Count++;
                List->AddString(GL2GameData.GetNpcName(Map->Pairs(i).Key), 0xFFDCDCDC, (INT)Npc);
                break;
            }
            if (MeshNameCheck->bChecked && NameIndex(Npc->MeshName))
            {
                if (appStristr(*Npc->MeshName, Text))
                {
                    Count++;
                    List->AddString(*Npc->MeshName, 0xFFDCDCDC, (INT)Npc);
                    continue;
                }
            }
            if (!MeshNameCheck->bChecked)
            {
                if (appStristr(GL2GameData.GetNpcName(Map->Pairs(i).Key), Text))
                {
                    Count++;
                    List->AddString(GL2GameData.GetNpcName(Map->Pairs(i).Key), 0xFFDCDCDC, (INT)Npc);
                }
            }
        }
        List->SetScrollRange(Count, 20);
        List->TopIndex = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100b4dc0, NCNpcView_CharacterSelectWnd, OnSearch)

// 0x100b5060 (vslot 70) - the whole table again
INT NCNpcView_CharacterSelectWnd::OnReset()
{
    List->Clear();
    NpcViewer()->AddAllCharacter();
    return 1;
}
NW_HOOK_METHOD(0x100b5060, NCNpcView_CharacterSelectWnd, OnReset)

// 0x100b3b50 (vslot 71) - a factor above 0 goes to the viewer pawn's speed factor and its animation's rate (the first
// channel of its mesh instance, as SetAnimRate does it without the channel check); a speed above 0 (truncated) to its
// speed, divided by the factor when that is above 0
INT NCNpcView_CharacterSelectWnd::OnApply()
{
    if (RateEdit->CommitText())
    {
        FLOAT Value = (FLOAT)_wtof(RateEdit->CommitText());
        if (Value > 0.0f)
        {
            Rate = Value;
            NpcViewer()->Npcs[0].Pawn->SpeedRate = Value;
            USkeletalMeshInstance* MeshInstance =
                static_cast<USkeletalMeshInstance*>(NpcViewer()->Npcs[0].Pawn->MeshInstance);
            if (MeshInstance)
            {
                // the channels array (UE2's Blends) is at +0x178 of the instance (the header has the C1 +0x114)
                MeshAnimChannel& Channel = **(MeshAnimChannel**)((BYTE*)MeshInstance + 0x178);
                Channel.AnimRate = Channel.Unknown20 * Rate;
            }
        }
    }
    if (SpeedEdit->CommitText())
    {
        DOUBLE Value = _wtof(SpeedEdit->CommitText());
        if (Value > 0.0)
        {
            Speed = (INT)Value;
            FLOAT NewSpeed = (FLOAT)Speed;
            if (Rate > 0.0f)
                NewSpeed = NewSpeed / Rate;
            NpcViewer()->Npcs[0].Pawn->Speed = NewSpeed;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100b3b50, NCNpcView_CharacterSelectWnd, OnApply)

// 0x100b3c70 (vslot 72) - the list's lines by mesh or by name, as checked
INT NCNpcView_CharacterSelectWnd::OnToggleNames()
{
    List->ToggleNames(MeshNameCheck->bChecked);
    return 1;
}
NW_HOOK_METHOD(0x100b3c70, NCNpcView_CharacterSelectWnd, OnToggleNames)

// 0x100b3c90 (vslot 73) - the NPC of the selected line spawned where the player stands, with the player's rotation
// and a player controller of its own (both with the authority role), a User record made for its resources; then moved
// down onto the ground and its spawn event run. The class name is read as an engine FName here (the double click
// reads it as an L2FName). Nothing checks that the pawn was spawned.
INT NCNpcView_CharacterSelectWnd::OnSummon()
{
    APlayerController* Viewer = ViewActor();
    if (!Viewer || !Viewer->Pawn || !Viewer->XLevel)
        return 1;
    INT Index = List->Selected;
    if (Index < 0)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    ULevel* Level = ViewActor()->XLevel;
    AController* Controller = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, TEXT("Engine.PlayerController"), NULL, LOAD_NoFail,
                                 NULL),
        NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1);
    NViewNpcData* Npc = (NViewNpcData*)List->GetItemParam(Index);
    FName ClassName = NAME_None;
    if (Npc)
        ClassName = *(FName*)&Npc->ClassName;
    UClass* PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL, *ClassName, NULL, 0, NULL);
    if (!PawnClass)
        return 1;
    APawn* Pawn = (APawn*)Level->SpawnActor(PawnClass, NAME_None, Location, ViewActor()->Pawn->Rotation, NULL, 1, 0,
                                            Controller);
    Controller->Pawn = Pawn;
    Pawn->Controller = Controller;
    Controller->Role = ROLE_Authority;
    Controller->Pawn->Role = ROLE_Authority;
    Controller->Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Controller->Pawn->NpcID = Npc->ID;
    User Record;
    Record.Unknown8 = 1;
    Record.NpcID = Npc->ID;
    Record.ObjectID = Npc->ID;
    Record.Pawn = Controller->Pawn;
    Record.SetPawnResource();
    DropToGround(Level, Controller->Pawn, Location);
    Record.Pawn->SpawnEnterEvent();
    return 1;
}
NW_HOOK_METHOD(0x100b3c90, NCNpcView_CharacterSelectWnd, OnSummon)

// 0x100b3200 - the animation picked (the combo box's notice, lParam) plays looped on the player's pawn at the speed
// factor. Items(Index) is read up to Index == Num (">=" in the stock code).
INT NCNpcView_CharacterSelectWnd::OnAniNameCombo(INT Index)
{
    guard(NCNpcView_CharacterSelectWnd::OnAniNameCombo);
    // the combo box's Items array (the header keeps it as filler): data at +0x1DC, count at +0x1E0
    TArray<NString*>& ComboItems = *(TArray<NString*>*)((BYTE*)AniCombo + 0x1DC);
    if (ComboItems.Num() >= Index)
    {
        TCHAR* Name = ComboItems(Index)->GetAt(0);
        APawn* Pawn = ViewActor()->Pawn;
        Pawn->Unknown13D4 &= ~1;
        Pawn->AnimFlags |= 0x10;
        Pawn->PlayAnim(0, FName(Name, FNAME_Add), Rate, 0.f, 1, 0);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b3200, NCNpcView_CharacterSelectWnd, OnAniNameCombo)

/*-----------------------------------------------------------------------------
    NCNpcViewrWnd.
-----------------------------------------------------------------------------*/

// 0x100b5160 - the tabs, the textures and the level are made in OnCreate; the nineteen User records are built by
// their constructors, the grids cleared (the selection window's pointer is not)
NCNpcViewrWnd::NCNpcViewrWnd() : TabCtrl(NULL), BackTex(NULL), OptionBackTex(NULL), Level(NULL)
{
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
}
NW_HOOK_CTOR(0x100b5160, NCNpcViewrWnd)

// 0x100b1f30 (vslot 25) - the search button; Enter in the search line reports through the edit's id into slot 69, a
// body shared with other classes (code 1: the button)
INT NCNpcViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_NPCVIEW_SEARCH)
            return OnSearchBtnClick() != 0;
        if (wParam == NID_NPCVIEW_SEARCHEDIT)
            return ::nw::VirtualCall<INT>(this, 69, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b1f30, NCNpcViewrWnd, WindowProc)

// (C3: the stock body of slot 69 is NCAuthWnd::OnPasswordNotify's, 0x100b2360) - Enter (code 1) in the search line
// clicks the button
INT NCNpcViewrWnd::OnSearchEdit(INT Code)
{
    if (Code == 1)
        OnSearchBtnClick();
    return 1;
}

// 0x100b62a0 (vslot 37) - the two item grids under their tabs ("item", "search"), the search line and button, the
// selection window (top-level), the first NPC record cleared (not a player, NPC id 1), then both tables loaded.
// NCWnd::OnCreate is not called.
INT NCNpcViewrWnd::OnCreate()
{
    guard(NCNpcViewrWnd::OnCreate);
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_NPCVIEW_FRAME;
    FrameCtrl->ID = NID_NPCVIEW_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    OptionBackTex = FindTexture(TEXT("L2UI.etcWnd.OptionBack"));

    ItemWnd[0] = new NCNpcView_InvenItemWnd(6, 4, 80, 32);
    ItemWnd[0]->SetPosition(16, 49, 218, 150);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);
    ItemWnd[1] = new NCNpcView_InvenItemWnd(6, 4, 80, 32);
    ItemWnd[1]->SetPosition(16, 49, 218, 150);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_NPCVIEW_TAB;
    TabCtrl->SetPosition(15, 22, 140, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(2, ItemWnd[0], 70, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(420, ItemWnd[1], 70, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);

    SearchEdit = new NCEditBox(1, (const TCHAR*)NULL, 1, 64, 0);
    SearchEdit->ID = NID_NPCVIEW_SEARCHEDIT;
    SearchEdit->SetPosition(70, 240, 50, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);
    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                      TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_NPCVIEW_SEARCH;
    SearchButton->SetPosition(140, 240, 32, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(420);

    for (INT i = 0; i < 19; i++)
        Npcs[i].Pawn = NULL;
    CharSelectWnd = new NCNpcView_CharacterSelectWnd;
    CharSelectWnd->SetPosition(34, 34, 350, 480);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE);
    memset(&Npcs[0], 0, sizeof(User));
    Npcs[0].Unknown8 = 1;
    Npcs[0].NpcID = 1;
    AddAllItem();
    AddAllCharacter();
    Level = GameLevel();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b62a0, NCNpcViewrWnd, OnCreate)

// 0x100b2840 (vslot 38) - the selection window deleted (top-level, not cleared); the search line and button are left to
// NCWnd::OnDestroy
INT NCNpcViewrWnd::OnDestroy()
{
    guard(NCNpcViewrWnd::OnDestroy);
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, FrameCtrl);
    delete CharSelectWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b2840, NCNpcViewrWnd, OnDestroy)

// 0x100b2220 (vslot 44) - with the selection window
void NCNpcViewrWnd::ShowWindow()
{
    guard(NCNpcViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x100b2220, NCNpcViewrWnd, ShowWindow)

// 0x100b2190 (vslot 45)
void NCNpcViewrWnd::HideWindow()
{
    guard(NCNpcViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x100b2190, NCNpcViewrWnd, HideWindow)

// 0x100b2720 (vslot 56) - the backgrounds of the grids and of the search line (the origin goes on before the clip)
INT NCNpcViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNpcViewrWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, 240, 185, 0, 0, 240, 185, BackTex);
    if (OptionBackTex)
        Device->DrawTexture(0, 205, 240, 70, 0, 274, 240, 70, OptionBackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b2720, NCNpcViewrWnd, OnPaint)

// 0x100b4c40 (vslot 68) - the items of classes 0 and 2 whose name holds the text (NULL is not checked) into the second
// grid, the second tab shown (C3: the same record filling as AddAllItem's)
INT NCNpcViewrWnd::OnSearchBtnClick()
{
    guard(NCNpcViewrWnd::OnSearchBtnClick);
    const TCHAR* Text = SearchEdit->CommitText();
    ClearGrid(ItemWnd[1]);
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Item = Map->Pairs(i).Value;
        if (!Item || NameIndex(Item->Name) == 0)
            continue;
        if (appStrstr(*Item->Name, Text) && (Item->Class == 0 || Item->Class == 2))
        {
            NItemInfo Info;
            FillItemInfo(Item, &Info);
            AddItem(1, &Info);
            ItemWnd[1]->MaxItems++;
        }
    }
    TabCtrl->SetTab(1, 1);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b4c40, NCNpcViewrWnd, OnSearchBtnClick)

// 0x100b22c0 - into a tab's grid. The bound is 8 while the viewer has two grids (the next fields would be taken for
// grids).
INT NCNpcViewrWnd::AddItem(INT Tab, NItemInfo* Info)
{
    guard(NCNpcViewrWnd::AddItem);
    if (Tab >= 0 && Tab < 8)
    {
        NCNpcView_InvenItemWnd* Grid = ItemWnd[Tab];
        if (Grid)
            Grid->AddItem(Info);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b22c0, NCNpcViewrWnd, AddItem)

// 0x100b4910 - the NPC table into the selection window's list (as NCNpcView_CharacterSelectWnd::OnSearch, without the
// text)
void NCNpcViewrWnd::AddAllCharacter()
{
    guard(NCNpcViewrWnd::AddAllCharacter);
    INT Count = 0;
    NDataMap<NViewNpcData>* Map = GL2GameData.NpcTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewNpcData* Npc = Map->Pairs(i).Value;
        if (!Npc || !Npc->HasName() || !IsListedNpc(CharSelectWnd, Npc))
            continue;
        const TCHAR* Name;
        if (CharSelectWnd->MeshNameCheck->bChecked)
            Name = *Npc->MeshName;
        else
            Name = GL2GameData.GetNpcName(Map->Pairs(i).Key);
        Count++;
        CharSelectWnd->List->AddString(Name, 0xFFDCDCDC, (INT)Npc);
    }
    CharSelectWnd->List->SetScrollRange(Count, 20);
    CharSelectWnd->List->TopIndex = 0;
    unguard;
}
NW_HOOK_METHOD(0x100b4910, NCNpcViewrWnd, AddAllCharacter)

// 0x100b1fb0 (name ours; AddAllItem and OnSearchBtnClick call it) - the same as NCPawnViewrWnd::FillItemInfo
// (0x100c2dd0) without the armor branch: the NPC viewer lists weapons and etc items only. A record without a name
// leaves the NItemInfo alone. The fields after +0x178C of the NItemInfo are written by offset (this client moved them
// 4 bytes up; the header only grew)
void NCNpcViewrWnd::FillItemInfo(NViewItemData* Data, NItemInfo* Info)
{
    INT Class = Data->Class;
    if (NameIndex(Data->Name) != 0)
    {
        appStrcpy(Info->Name, *Data->Name);
        appStrcpy(Info->AdditionalName, *Data->AdditionalName);
        appStrcpy(Info->IconName, *Data->IconName);
        appStrcpy(Info->Unknown200, *Data->IconName2);
        appStrcpy(Info->Unknown300, *Data->IconName3);
        appStrcpy(Info->Description, *Data->Description);
        Info->Param = Class;
        Info->ID = Data->ItemID;
        Info->Icon = FindTexture(*Data->IconName);
        if (!Info->Icon)
        {
            appStrcpy(Info->IconName, TEXT("Icon.NoImage"));
            Info->Icon = FindTexture(TEXT("Icon.NoImage"));
        }
        if (Class == ITEMCLASS_Weapon)
        {
            Info->UnknownC28 = Data->Weight;
            Info->UnknownC30 = Data->WeaponType;
            Info->UnknownC34[0] = Data->PAtk;
            Info->UnknownC34[1] = Data->MAtk;
            Info->UnknownC34[4] = Data->ShieldPDef;
            Info->UnknownC34[7] = Data->CrystalGrade;
            Info->UnknownC7C = Data->SoulShots;
            Info->UnknownC80 = Data->SpiritShots;
            Info->BodyPart = Data->BodyPart;
        }
        else if (Class == ITEMCLASS_Etc)
        {
            Info->UnknownC28 = Data->Weight;
            Info->UnknownC34[7] = Data->Unknown8C;
            InfoInt(Info, 0x1794) = Data->bStackable;
            Info->Unknown1710 = Data->EtcType;
            if (Data->EtcType == 2)
                InfoInt(Info, 0x17A0) = 1;
            else if (Data->EtcType == 5)
                InfoInt(Info, 0x17A4) = 1;
        }
    }
}
NW_HOOK_METHOD(0x100b1fb0, NCNpcViewrWnd, FillItemInfo)

// 0x100b4b10 - the items of classes 0 and 2 into the first grid (its capacity raised with each)
void NCNpcViewrWnd::AddAllItem()
{
    guard(NCNpcViewrWnd::AddAllItem);
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Item = Map->Pairs(i).Value;
        if (Item && (Item->Class == 0 || Item->Class == 2))
        {
            NItemInfo Info;
            FillItemInfo(Item, &Info);
            AddItem(0, &Info);
            ItemWnd[0]->MaxItems++;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x100b4b10, NCNpcViewrWnd, AddAllItem)

// 0x100b50e0 - the vtable, ConditionalDestroy, NCInvenItemWnd
NW_HOOK_DTOR(0x100b50e0, NCNpcView_InvenItemWnd)
// 0x100b5220 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100b5220, NCNpcViewrWnd)
// 0x100b5390 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100b5390, NCNpcView_CharacterSelectWnd)

NW_CHECK_VTABLE(NCNpcView_InvenItemWnd, 0x10192038, 76)
NW_CHECK_VTABLE(NCNpcView_CharacterSelect_List, 0x10192280, 76)
NW_CHECK_VTABLE(NCNpcView_CharacterSelectWnd, 0x101923b0, 75)
NW_CHECK_VTABLE(NCNpcViewrWnd, 0x10192168, 70)
