/*=============================================================================
    NCPawnView_CharacterSelectWnd.cpp - the pawn viewer's character window and its
    animation list. Stock: 0x10088140 .. 0x100881ec, 0x10088450 .. 0x10088592,
    0x10089140 .. 0x100892cf, 0x10089f10 .. 0x1008a007, 0x1008c3c0 .. 0x1008c719,
    0x1008cd20 .. 0x1008d13f, 0x1008d960 .. 0x1008dc73; OnDestroy and five of the list's
    slots are bodies shared with other classes.
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_CHARSELECT_FRAME = (INT)0xAFF12011,
    NID_CHARSELECT_BYACTION = 0x101,
    NID_CHARSELECT_NAME = 0x1F04,
    NID_CHARSELECT_ANINAME = 0x1F05,   // handled, no control sends it
};

// UpdateAnimList's groups: the animations whose name holds these come first, in this order
static const TCHAR GAnimActionKeys[5][10] = {
    TEXT("run"), TEXT("walk"), TEXT("atk"), TEXT("wait"), TEXT("social")};
static const TCHAR GAnimWeaponKeys[6][10] = {
    TEXT("1hs"), TEXT("2hs"), TEXT("dual"), TEXT("pole"), TEXT("bow"), TEXT("hand")};

// An object made as the stock code makes it: StaticAllocateObject of its class, then the constructor at Ctor (the
// class's own C++ constructor, hooked there with the same arguments).
// The standalone has no stock code: the constructor that runs is T's own (each one used here is hooked at Ctor with the
// same arguments, so the hook build runs the same one through the stock address).
template <class T, class... A> static FORCEINLINE T* NewStock(DWORD Ctor, A... a)
{
#if NW_HOOK_MODE
    void* Mem = T::operator new(sizeof(T));
    return Mem ? ::nw::StockCallAt<T*>(Ctor, Mem, a...) : NULL;
#else
    (void)Ctor;
    return new T(a...);
#endif
}

// a sequence of the animation set in this client: 0x44 bytes, the name first (UE2Engine.h has the C2 record, 0x40)
struct NAnimSeq
{
    FName Name;
    BYTE _Unknown4[0x44 - 0x4];
};

// a child window goes (repeated inline in the stock OnDestroys)
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
    NCPawnView_CharacterSelect_List.
-----------------------------------------------------------------------------*/

// 0x100c9910 - NCDebugWnd's default constructor, then this class's vtable
NCPawnView_CharacterSelect_List::NCPawnView_CharacterSelect_List()
{
}
NW_HOOK_CTOR(0x100C9910, NCPawnView_CharacterSelect_List)

// 0x100c99b0 (ret 8) - NCDebugWnd's default constructor, the size of the list and no animation playing
NCPawnView_CharacterSelect_List::NCPawnView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines)
{
    MaxLines = InMaxLines;
    VisibleLines = InVisibleLines;
    PlayingIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x100C99B0, NCPawnView_CharacterSelect_List, (INT A, INT B), (A, B))


// stock body shared with NCNpcView_CharacterSelect_List's (0x1009c940, hooked there)
INT NCPawnView_CharacterSelect_List::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCDebugWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100c38d0 (vslot 37)
INT NCPawnView_CharacterSelect_List::OnCreate()
{
    guard(NCPawnView_CharacterSelect_List::OnCreate);
    NCDebugWnd::OnCreate();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c38d0, NCPawnView_CharacterSelect_List, OnCreate)

// 0x100c39d0 (vslot 38)
INT NCPawnView_CharacterSelect_List::OnDestroy()
{
    guard(NCPawnView_CharacterSelect_List::OnDestroy);
    NCDebugWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c39d0, NCPawnView_CharacterSelect_List, OnDestroy)

// 0x100c3950 (vslot 56)
INT NCPawnView_CharacterSelect_List::OnPaint(NCGDevice* Device)
{
    guard(NCPawnView_CharacterSelect_List::OnPaint);
    NCDebugWnd::OnPaint(Device);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c3950, NCPawnView_CharacterSelect_List, OnPaint)

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a730)
INT NCPawnView_CharacterSelect_List::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCListBox::OnMouseWheel(wParam, lParam);
    return 1;
}

// 0x100c5250 - the animation of the selected line (its param is the sequence name) plays on the player's pawn, looped
// for a double click and once for the right button; on the line that plays already (the pawn's animation flag 0x10 set) it
// goes back to the idle animation of the pawn's weapon instead
__declspec(noinline) INT NCPawnView_CharacterSelect_List::PlayAnim(INT bLoop)
{
    guard(NCPawnView_CharacterSelect_List::PlayAnim);
    TCHAR* Name = (TCHAR*)GetItemParam(Selected);
    APawn* Pawn = NPawnView::PlayerPawn();
    if (PlayingIndex == Selected && (Pawn->AnimFlags & 0x10))
    {
        Pawn->AnimFlags &= ~0x10;
        Pawn->Unknown13D4 |= 1;
        Pawn->PlayAnim(0, Pawn->WaitAnimName[Pawn->WeaponType], 1.0f, 0.0f, 1, 0);
    }
    else
    {
        Pawn->AnimFlags |= 0x10;
        Pawn->Unknown13D4 &= ~1;
        Pawn->PlayAnim(0, FName(Name, FNAME_Add), Pawn->Unknown694, 0.0f, bLoop, 1);
        PlayingIndex = Selected;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c5250, NCPawnView_CharacterSelect_List, PlayAnim)

// 0x100c7dc0 (vslot 72) - a double click plays the animation in a loop
INT NCPawnView_CharacterSelect_List::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    return PlayAnim(1);
}
NW_HOOK_METHOD(0x100c7dc0, NCPawnView_CharacterSelect_List, OnLButtonDblClick)

// 0x100c7dd0 (vslot 64) - the right button plays it once
INT NCPawnView_CharacterSelect_List::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    return PlayAnim(0);
}
NW_HOOK_METHOD(0x100c7dd0, NCPawnView_CharacterSelect_List, OnRButtonDown)

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a6f0)
INT NCPawnView_CharacterSelect_List::ScrollUp()
{
    NCListBox::ScrollUp();
    return 1;
}

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a700)
INT NCPawnView_CharacterSelect_List::ScrollDown()
{
    NCListBox::ScrollDown();
    return 1;
}

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a710)
INT NCPawnView_CharacterSelect_List::SetTopIndex(INT Index)
{
    NCListBox::SetTopIndex(Index);
    return 1;
}

// 0x100c9960 - the vtable, ConditionalDestroy, NCDebugWnd
NW_HOOK_DTOR(0x100c9960, NCPawnView_CharacterSelect_List)
// 0x100c9810 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100c9810, NCPawnView_CharacterSelectWnd)

NW_CHECK_VTABLE(NCPawnView_CharacterSelect_List, 0x101956e8, 76)

/*-----------------------------------------------------------------------------
    NCPawnView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x100c9790
NCPawnView_CharacterSelectWnd::NCPawnView_CharacterSelectWnd()
{
    TitleTex = NULL;
    BackTex = NULL;
    UnknownA8 = NULL;
    UnknownAC = NULL;
    UnknownB0 = NULL;
    UnknownB4 = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x100c9790, NCPawnView_CharacterSelectWnd)


// 0x100c3600 (vslot 25)
INT NCPawnView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CHARSELECT_NAME)
            return OnNameComboBox((INT)lParam) != 0;
        if (wParam == NID_CHARSELECT_ANINAME)
            return OnAniNameComboBox((INT)lParam) != 0;
        if (wParam == NID_CHARSELECT_BYACTION)
            return OnByActionCheck() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100c3600, NCPawnView_CharacterSelectWnd, WindowProc)

// 0x100c9ca0 (vslot 37) - a title bar without buttons, the textures, the character combo, the animation list (500
// lines, 18 shown) and "by action" (a Korean text of single bytes), unchecked
INT NCPawnView_CharacterSelectWnd::OnCreate()
{
    guard(NCPawnView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();
    FrameCtrl = NewStock<NCFrameCtrl>(0x10010450, 0, 0, 0);
    FrameID = NID_CHARSELECT_FRAME;
    FrameCtrl->ID = NID_CHARSELECT_FRAME;
    FrameCtrl->SetPosition(2, 3, Width - 6, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    UnknownA8 = FindTexture(TEXT("sek.cbui67"));
    UnknownAC = FindTexture(TEXT("sek.cbui68"));
    UnknownB0 = FindTexture(TEXT("sek.cbui69"));

    NameCombo = NewStock<NCComboBox>(0x10005b20);
    NameCombo->ID = NID_CHARSELECT_NAME;
    NameCombo->SetPosition(10, 28, 129, 17);
    CreateChild(NameCombo, NWS_VISIBLE);

    AnimList = NewStock<NCPawnView_CharacterSelect_List>(0x100c99b0, 500, 18);
    AnimList->SetPosition(33, 59, 166, 234);
    CreateChild(AnimList, NWS_VISIBLE);

    ByActionCheck = NewStock<NCCheckBox>(0x100049e0, TEXT("\x00BE\x00D7\x00BC\x00C7\x00BA\x00B0"), 0, 0, 0);
    ByActionCheck->ID = NID_CHARSELECT_BYACTION;
    ByActionCheck->SetPosition(150, 30, 50, 16);
    CreateChild(ByActionCheck, NWS_VISIBLE);
    ByActionCheck->bChecked = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c9ca0, NCPawnView_CharacterSelectWnd, OnCreate)

// stock body shared with NCTrainWnd's (0x10089310): the check box is left alone
INT NCPawnView_CharacterSelectWnd::OnDestroy()
{
    DestroyChild(this, NameCombo);
    DestroyChild(this, AnimList);
    NCWnd::OnDestroy();
    return 1;
}

// 0x100c4d00 (vslot 56) - the background stretched 20 px past the right edge, the title strip; the clip goes on
// before the origin (the guard names NCCharacterSelectWnd)
INT NCPawnView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCCharacterSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, (INT)(Width + 20.0f), (INT)Height, BackTex);
    if (TitleTex)
    {
        INT TitleWidth = (INT)(Width - 4.0f);
        Device->DrawTexture(2, 3, TitleWidth, 20, 0, 0, TitleWidth, 20, TitleTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c4d00, NCPawnView_CharacterSelectWnd, OnPaint)

// 0x100c8990 (vslot 68) - another default character: a new PlayerController is spawned only to own the new pawn and
// carry its class (it is left in the level), the pawn of that class replaces the player's 30 units higher under the
// player's own controller, falls to the ground, is dressed as the record (its volumes set, the name combo kept on it)
// and plays its idle animation; then the animations list and the hair window's values follow
INT NCPawnView_CharacterSelectWnd::OnNameComboBox(INT Index)
{
    guard(NCPawnView_CharacterSelectWnd::OnNameComboBox);
    if (Index > 19 || Index == Selected)
        return 1;
    ULevel* Level = GameLevel();
    APlayerController* PlayerController = ViewActor();
    FName ControllerClass = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* pController = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, *ControllerClass, NULL, 1, NULL));
    if (!pController)
        appFailAssert("pController", "Src\\NPawnViewerWnd.cpp", 1408);
    User Character = NPawnView::Viewer()->Characters[Index];
    UClass*& PawnClass = pController->PawnClass;
    PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL,
                                         Character.GetClassNamePointer(), NULL, 0, NULL);
    if (!PawnClass)
        return 1;
    FVector Location = NPawnView::PlayerPawn()->Location;
    Location.Z += 30.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(PawnClass, NAME_None, Location, FRotator(0, 0, 0),
                                            NULL, 1, 1, pController, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(NPawnView::PlayerPawn(), 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = PlayerController;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Selected = Index;
    NPawnView::Viewer()->Characters[Index].Pawn = Pawn;
    NPawnView::Viewer()->Characters[Selected].SetPawnResource();
    Pawn->SetVolumes();
    NameCombo->SetSelected(Selected);
    Pawn->PlayAnim(0, Pawn->GetCurWaitAnimName(), 1.0f, 0.0f, 1, 0);
    UpdateAnimList();
    NPawnView::Viewer()->HairChangeWnd->SyncFromCharacter();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c8990, NCPawnView_CharacterSelectWnd, OnNameComboBox)

// 0x100c8cf0 (vslot 70)
INT NCPawnView_CharacterSelectWnd::OnByActionCheck()
{
    UpdateAnimList();
    return 1;
}
NW_HOOK_METHOD(0x100c8cf0, NCPawnView_CharacterSelectWnd, OnByActionCheck)

// 0x100c70c0 - the animations of the player's pawn mesh into the list: first those whose name holds each key (by
// action, or by weapon), then the rest in the mesh's order; a line's param is the name itself. The animation set's
// array is at +0x60 here and its records are 0x44 bytes (the header has the C2 layout)
__declspec(noinline) void NCPawnView_CharacterSelectWnd::UpdateAnimList()
{
    TArray<INT> Remaining;
    APawn* Pawn = NPawnView::PlayerPawn();
    if (!Pawn)
        return;
    UMesh* Mesh = Pawn->Mesh;
    if (!Mesh)
        return;
    Mesh->MeshGetInstance(Pawn);
    UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
    TArray<NAnimSeq>& Seqs = *(TArray<NAnimSeq>*)((BYTE*)Anim + 0x60);
    INT NumSequences = Seqs.Num();
    AnimList->Clear();
    for (INT i = 0; i < NumSequences; i++)
        Remaining.AddItem(i);
    const TCHAR(*Keys)[10] = ByActionCheck->bChecked ? GAnimActionKeys : GAnimWeaponKeys;
    INT NumKeys = ByActionCheck->bChecked ? 5 : 6;
    for (INT k = 0; k < NumKeys; k++)
    {
        for (INT i = 0; i < NumSequences; i++)
        {
            FName Name = Seqs(i).Name;
            INT Index = Remaining.FindItemIndex(i);
            if (Index != -1 && appStristr(*Name, Keys[k]) && AnimList)
            {
                Remaining.Remove(Index);
                AnimList->AddString(*Name, 0xFFDCDCDC, (INT)*Name);
            }
        }
    }
    for (INT i = 0; i < Remaining.Num(); i++)
        AnimList->AddString(*Seqs(Remaining(i)).Name, 0xFFDCDCDC, (INT)*Seqs(Remaining(i)).Name);
}
NW_HOOK_METHOD(0x100c70c0, NCPawnView_CharacterSelectWnd, UpdateAnimList)

NW_CHECK_VTABLE(NCPawnView_CharacterSelectWnd, 0x101954a0, 71)
