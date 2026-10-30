/*=============================================================================
    NCPawnViewrWnd.cpp - the pawn viewer's main window and its item pages. Stock:
    0x10087420 .. 0x10088108, 0x10088830 .. 0x10089102, 0x10089380 .. 0x100895e8,
    0x1008c2d0 .. 0x1008c3b8, 0x1008c750 .. 0x1008cd1d, 0x1008e230 .. 0x1008ee32 (the other
    PawnView classes sit in between); the item grid's window procedure is a body shared with
    the MSView / NpcView grids (hooked in NCNpcViewrWnd.cpp).
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NConsoleFns.h"

#include <string.h>
#include <intrin.h>
#include <wchar.h>

// NWM_COMMAND ids (names ours)
enum
{
    NID_PAWNVIEW_FRAME = 0xF2,
    NID_PAWNVIEW_F3 = 0xF3,          // handled (vfn062), no control sends it
    NID_PAWNVIEW_GUI = 0xF4,
    NID_PAWNVIEW_TEXTLIST = 0xF5,    // each text page's list
    NID_PAWNVIEW_SEARCH = 0x102,
    NID_PAWNVIEW_VIEWLIST = 0x103,
    NID_PAWNVIEW_WEAPONTYPE = 0x114,
    NID_PAWNVIEW_USE = 0x115,
    NID_PAWNVIEW_SEARCHEDIT = 0x116,
    NID_PAWNVIEW_SPAWN = 0x117,      // third row, new in C2
    NID_PAWNVIEW_FLAG = 0x118,
    NID_PAWNVIEW_GOTO = 0x119,
    NID_PAWNVIEW_RANGEEDITA = 0x11A,
    NID_PAWNVIEW_RANGEEDITB = 0x11B,
    NID_PAWNVIEW_FACECHECK = 0x11C,  // new in C3: a check box
    NID_PAWNVIEW_ATTACK = 0x11D,     // new in C3: "target attack"
    NID_PAWNVIEW_SAVE = 0x11E,       // new in C3: the chosen character's equipment into L2.ini
    NID_PAWNVIEW_LOAD = 0x11F,       // new in C3: and back
    NID_PAWNVIEW_CATEGORY = 0xFE001A,
    NID_PAWNVIEW_PAGES = 0xFE002A,
};

// sysstring lines
enum
{
    STR_Gloves = 37,
    STR_Chest = 38,
    STR_Legs = 39,
    STR_Feet = 40,
    STR_Etc = 49,
    STR_Human = 170,
    STR_Elf = 171,
    STR_DarkElf = 172,
    STR_Orc = 173,
    STR_Dwarf = 174,
    STR_Warrior = 175,
    STR_Spellcaster = 176,
    STR_Male = 177,
    STR_Female = 178,
    STR_Headgear = 230,
    STR_Search = 420,
    STR_ViewList = 422,
    STR_Use = 443,
};

// item categories: the pages of CategoryTabs (names ours)
enum
{
    PAWNVIEW_Chest,
    PAWNVIEW_Legs,
    PAWNVIEW_Gloves,
    PAWNVIEW_Feet,
    PAWNVIEW_Headgear,
    PAWNVIEW_Etc,
    PAWNVIEW_NUM_CATEGORIES,
};

// the 18 default characters of the creation screen (0x101133b8, only the viewer reads it)
struct NDefaultCharacter
{
    INT Race;
    INT Sex;
    INT ClassID;
};
#if NW_HOOK_MODE
#define GDefaultCharacters (reinterpret_cast<const NDefaultCharacter*>(::nw::Addr(0x102259D8)))
#else
extern const NDefaultCharacter GDefaultCharacters[];
#endif

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

// The same for the shortcut bar (the stock UClass at ClassVA, 0x10298030, and its constructor at Ctor, 0x100C9B20: both
// NCPawnView_ShortCutWnd's)
static FORCEINLINE NCWnd* NewStockClass(DWORD ClassVA, DWORD Ctor)
{
#if NW_HOOK_MODE
    UObject* Mem = UObject::StaticAllocateObject(reinterpret_cast<UClass*>(::nw::Addr(ClassVA)));
    return Mem ? ::nw::StockCallAt<NCWnd*>(Ctor, Mem) : NULL;
#else
    (void)ClassVA;
    (void)Ctor;
    return new NCPawnView_ShortCutWnd;
#endif
}

// the combo box's data word of a line (0x100054d0, ret 4; NComboItemFns::GetItemData, NTemplates.cpp): the line's
// NString::Color, -1 outside the list
static FORCEINLINE INT ComboItemData(NCComboBox* Combo, INT Index)
{
    return ((NComboItemFns*)Combo)->GetItemData(Index);
}

// a line of Text (none when it is NULL or empty) whose data word is Data (0x10005a00, ret 8; NComboItemFns::AddItemData)
static FORCEINLINE void ComboAddItemData(NCComboBox* Combo, const TCHAR* Text, INT Data)
{
    ((NComboItemFns*)Combo)->AddItemData(Text, Data);
}

// The character chosen in the character window (0..19: the check lets one past the 19
// records through) puts the item on, or takes it off when it wears it already; a two-handed
// item frees both hands, a one-handed one the two-handed slot. The player's pawn is dressed
// as that character. Inline in both stock callers (name ours).
static FORCEINLINE void ToggleItem(NItemEntry* Item)
{
    NCPawnViewrWnd* Viewer = NPawnView::Viewer();
    INT Index = Viewer->CharSelectWnd->Selected;
    if (Index >= 0 && Index <= 19)
    {
        User& Character = Viewer->Characters[Index];
        if (Item->BodyPart == BODYSLOT_LRHand)
        {
            Character.Equipped[BODYSLOT_RHand] = 0;
            Character.Equipped[BODYSLOT_LHand] = 0;
        }
        else if (Item->BodyPart == BODYSLOT_RHand || Item->BodyPart == BODYSLOT_LHand)
        {
            Character.Equipped[BODYSLOT_LRHand] = 0;
        }
        INT& Worn = Character.Equipped[Item->BodyPart];
        Worn = (Worn == Item->ID) ? 0 : Item->ID;
        NPawnView::Viewer()->Characters[Index].Pawn = NPawnView::PlayerPawn();
        NPawnView::Viewer()->Characters[Index].SetPawnResource();
    }
}

// NCItemWnd::Clear is slot 72 of this client: the header of NCItemWnd has an extra virtual before it (its slot 71 is
// not in the stock vtable), so the call goes by the slot number (to be replaced by Grid->Clear() once that is fixed)
static FORCEINLINE void ClearGrid(NCPawnView_InvenItemWnd* Grid)
{
    ::nw::VirtualCall<void>(Grid, 72);
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

/*-----------------------------------------------------------------------------
    NCPawnView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x100c95a0 - the grid's defaults and this class's vtable
NCPawnView_InvenItemWnd::NCPawnView_InvenItemWnd()
{
}
NW_HOOK_CTOR(0x100C95A0, NCPawnView_InvenItemWnd)

// 0x100c9640 (ret 0x10) - the 4-argument constructor: the grid of six arguments with both gaps 1; the pawn viewer builds
// its grids with it
NCPawnView_InvenItemWnd::NCPawnView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x100C9640, NCPawnView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))


// stock body shared with the skill viewer's and the NPC viewer's grids (0x100aa450, hooked as NCNpcView_InvenItemWnd's);
// C2 no longer refuses the messages of a disabled window here
INT NCPawnView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}

// 0x100c3d20 (vslot 61) - the item under the mouse; with the mouse in the window the cursor is the carried item's icon
// while dragging, else the normal one
INT NCPawnView_InvenItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnMouseMove);
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (IsFocused())
    {
        if (!bDragging)
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
        else if (Selected >= 0 && Selected < Items.Num())
        {
            SetCursor(4, Items(Selected)->GetIcon());
            LockCursor();
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c3d20, NCPawnView_InvenItemWnd, OnMouseMove)

// The item let go outside the window (C2: new) is dropped into the world where the mouse points: the character's
// engine event OnDropItem gets a one-item record (name of the guard: the stock copies OnRButtonDown's).
// The item let go outside the window is dropped into the world where the mouse points: the character's engine
// event OnDropItem gets a one-item record (name of the guard: the stock copies OnRButtonDown's).
// 0x100c3e10 (vslot 63)
INT NCPawnView_InvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnRButtonDown);
    if (Selected < 0 || Selected >= Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    if (bDragging && !bDblClicked)
    {
        NItemEntry* Entry = Items(Selected);
        if (!Entry)
        {
            bDragging = 0;
            return 1;
        }
        if (!IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        {
            DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
            // the engine's Item, 0x18 bytes here: no object id, the item id, not stackable, one of it
            struct { INT ObjectID; INT ItemID; UBOOL bStackable; INT Count; INT Unknown10; INT Unknown14; } Dropped;
            Dropped.ObjectID = 0;
            Dropped.ItemID = Entry->ID;
            Dropped.bStackable = 0;
            Dropped.Count = 1;
            Dropped.Unknown10 = 0;
            Dropped.Unknown14 = 0;
            NCPawnViewrWnd* Viewer = NPawnView::Viewer();
            static_cast<FL2NetNotify*>(GConsole->Engine)->OnDropItem(
                &Viewer->Characters[Viewer->CharSelectWnd->Selected], (Item*)&Dropped, DropLocation,
                FRotator(0, 0, 0));
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    UnlockCursor();
    SetCursor(0, NULL);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c3e10, NCPawnView_InvenItemWnd, OnLButtonUp)

// 0x100c6d10 (vslot 64) - the item under the mouse goes on the chosen character, or off; the left hand's item of the
// pawn is remembered and the one in the hands now set (C3), and the viewer's three lines of text get the item's
// names for the group of the character (the number its combo box line carries)
INT NCPawnView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnRButtonDown);
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected < 0 || Selected >= Items.Num())
        return 1;
    INT Index = NPawnView::Viewer()->CharSelectWnd->Selected;
    if (Index < 0 || Index > 19)
        return 1;
    User& Character = NPawnView::Viewer()->Characters[Index];
    INT Slot = Items(Selected)->BodyPart;
    if (Slot == BODYSLOT_LRHand)
    {
        Character.Equipped[BODYSLOT_RHand] = 0;
        Character.Equipped[BODYSLOT_LHand] = 0;
    }
    else if (Slot == BODYSLOT_RHand || Slot == BODYSLOT_LHand)
    {
        Character.Equipped[BODYSLOT_LRHand] = 0;
    }
    NItemEntry* Entry = Items(Selected);
    INT& Worn = Character.Equipped[Entry->BodyPart];
    Worn = (Worn == Entry->ID) ? 0 : Entry->ID;
    // the pawn keeps the item of its left hand (+0x5C4) in its +0x1694; then the hands' item is put there
    *(INT*)((BYTE*)Character.Pawn + 0x1694) = Character.Pawn->LeftHandItem;
    INT Hand = Items(Selected)->BodyPart;
    if (Hand == BODYSLOT_LRHand)
        Character.Pawn->LeftHandItem = Character.Equipped[BODYSLOT_LRHand];
    else if (Hand == BODYSLOT_RHand)
        Character.Pawn->LeftHandItem = Character.Equipped[BODYSLOT_RHand];
    else if (Hand == BODYSLOT_LHand)
        Character.Pawn->LeftHandItem = Character.Equipped[BODYSLOT_LHand];
    NPawnView::Viewer()->Characters[Index].Pawn = NPawnView::PlayerPawn();
    NPawnView::Viewer()->Characters[Index].SetPawnResource();
    NViewItemData* Data = NItemTable::Find(&Items(Selected)->ID);
    appStrcpy(NPawnView::Viewer()->InfoText[0],
              *Data->GetNameAt(ComboItemData(NPawnView::Viewer()->CharSelectWnd->NameCombo, Index), 0));
    for (INT i = 0; i < 2; i++)
        appStrcpy(NPawnView::Viewer()->InfoText[1 + i],
                  *Data->GetExtraNameAt(ComboItemData(NPawnView::Viewer()->CharSelectWnd->NameCombo, Index), i));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c6d10, NCPawnView_InvenItemWnd, OnRButtonDown)

// 0x100c2200 (vslot 75) - as the right button
INT NCPawnView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnLButtonDblClick);
    OnRButtonDown(wParam, lParam);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c2200, NCPawnView_InvenItemWnd, OnLButtonDblClick)

// 0x100c95f0 - the vtable, ConditionalDestroy, NCInvenItemWnd
NW_HOOK_DTOR(0x100c95f0, NCPawnView_InvenItemWnd)
// 0x100c9550 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100c9550, NCPawnView_InvenTextWnd)
// 0x100c9740 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100c9740, NCPawnViewrWnd)

NW_CHECK_VTABLE(NCPawnView_InvenItemWnd, 0x10195220, 76)

/*-----------------------------------------------------------------------------
    NCPawnView_InvenTextWnd.
-----------------------------------------------------------------------------*/

// 0x100c9500
NCPawnView_InvenTextWnd::NCPawnView_InvenTextWnd()
{
}
NW_HOOK_CTOR(0x100c9500, NCPawnView_InvenTextWnd)


// 0x100c36a0 (vslot 25) - a click on the list
INT NCPawnView_InvenTextWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_PAWNVIEW_TEXTLIST)
        return OnInvenItemList((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100c36a0, NCPawnView_InvenTextWnd, WindowProc)

// 0x10001b00 (vslot 44; the header's inline `{}`) - a bare `ret`: the linker folded this body into every virtual of the client that
// takes no argument and does nothing (NConsoleWnd's AuthLoginSuccess / ShowMagicWnd / GMViewShowMagicWnd, the cool time's
// destructor, EndRecommendedData ...), so the hook serves them all
NW_HOOK_METHOD(0x10001b00, NCPawnView_InvenTextWnd, ShowWindow)

// 0x100c7650 (vslot 37) - the list (3000 lines, 11 shown), created without the visible bit (0x21: bits of unknown
// meaning); NCWnd's OnCreate is not called
INT NCPawnView_InvenTextWnd::OnCreate()
{
    guard(NCPawnView_InvenTextWnd::OnCreate);
    List = NewStock<NCListBox>(0x1001f300, 3000, 11, 16);
    List->ID = NID_PAWNVIEW_TEXTLIST;
    List->SetPosition(0, 0, 222, 140);
    CreateChild(List, 0x21);
    List->TextX = 1;
    List->TextY = 1;
    List->LineGap = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c7650, NCPawnView_InvenTextWnd, OnCreate)

// 0x100c7430 (vslot 68) - the line clicked (its param is its item) goes on the chosen character, or off; the line is
// read from the viewer's list of the page shown (not this window's own); then the viewer's three lines of text get
// the item's names (C3)
INT NCPawnView_InvenTextWnd::OnInvenItemList(INT Index)
{
    guard(NCPawnView_InvenTextWnd::OnInvenItemList);
    NCPawnViewrWnd* Viewer = NPawnView::Viewer();
    NItemEntry* Entry = (NItemEntry*)Viewer->TextWnds[Viewer->CurrentPage]->List->GetItemParam(Index);
    INT Char = NPawnView::Viewer()->CharSelectWnd->Selected;
    if (Char >= 0 && Char <= 19)
    {
        User& Character = NPawnView::Viewer()->Characters[Char];
        INT Slot = Entry->BodyPart;
        if (Slot == BODYSLOT_LRHand)
        {
            Character.Equipped[BODYSLOT_RHand] = 0;
            Character.Equipped[BODYSLOT_LHand] = 0;
        }
        else if (Slot == BODYSLOT_RHand || Slot == BODYSLOT_LHand)
        {
            Character.Equipped[BODYSLOT_LRHand] = 0;
        }
        INT& Worn = Character.Equipped[Entry->BodyPart];
        Worn = (Worn == Entry->ID) ? 0 : Entry->ID;
        NPawnView::Viewer()->Characters[Char].Pawn = NPawnView::PlayerPawn();
        NPawnView::Viewer()->Characters[Char].SetPawnResource();
        NViewItemData* Data = NItemTable::Find(&Entry->ID);
        INT Group = ComboItemData(NPawnView::Viewer()->CharSelectWnd->NameCombo, Char);
        appStrcpy(NPawnView::Viewer()->InfoText[0], *Data->GetNameAt(Group, 0));
        for (INT i = 0; i < 2; i++)
        {
            Group = ComboItemData(NPawnView::Viewer()->CharSelectWnd->NameCombo, Char);
            appStrcpy(NPawnView::Viewer()->InfoText[1 + i], *Data->GetNameAt(Group, i));
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c7430, NCPawnView_InvenTextWnd, OnInvenItemList)

NW_CHECK_VTABLE(NCPawnView_InvenTextWnd, 0x10195108, 69)

/*-----------------------------------------------------------------------------
    NCPawnViewrWnd.
-----------------------------------------------------------------------------*/

// 0x100c9670 - the nineteen records are built by their constructors; the grid texture, the category tabs, the first
// seven grids and the level are cleared
NCPawnViewrWnd::NCPawnViewrWnd() : GridBackTex(NULL), CategoryTabs(NULL)
{
    for (INT i = 0; i < 7; i++)
        ItemWnds[i] = NULL;
    Level = NULL;
}
NW_HOOK_CTOR(0x100c9670, NCPawnViewrWnd)


// 0x100c2290 (vslot 25) - C3: no IsEnabled test; the third row's buttons (0x117, 0x118, 0x11D), the check box 0x11C and
// the save / load buttons (0x11E, 0x11F) are new next to the Enter keys of the two edit lines (0x116, 0x11B)
INT NCPawnViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_PAWNVIEW_CATEGORY)
            return OnCategoryTab((INT)lParam) != 0;
        if (wParam == NID_PAWNVIEW_F3)
            return vfn062() != 0;
        if (wParam == NID_PAWNVIEW_GUI)
            return OnGUICheck() != 0;
        if (wParam == NID_PAWNVIEW_FACECHECK)
            return OnFaceCheck() != 0;
        if (wParam == NID_PAWNVIEW_SEARCH)
            return OnSearch() != 0;
        if (wParam == NID_PAWNVIEW_VIEWLIST)
            return OnViewList() != 0;
        if (wParam == NID_PAWNVIEW_USE)
            return OnUseWeapon() != 0;
        if (wParam == NID_PAWNVIEW_SPAWN)
            return OnSpawnCharacter() != 0;
        if (wParam == NID_PAWNVIEW_FLAG)
            return OnToggleFlag() != 0;
        if (wParam == NID_PAWNVIEW_GOTO)
            return OnGoToMap() != 0;
        if (wParam == NID_PAWNVIEW_ATTACK)
            return OnAttackTarget() != 0;
        if (wParam == NID_PAWNVIEW_SAVE)
            return OnSaveEquipment() != 0;
        if (wParam == NID_PAWNVIEW_LOAD)
            return OnLoadEquipment() != 0;
        if (wParam == NID_PAWNVIEW_SEARCHEDIT)
            return OnSearchEdit((INT)lParam) != 0;
        if (wParam == NID_PAWNVIEW_RANGEEDITB)
            return OnRangeEdit((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100c2290, NCPawnViewrWnd, WindowProc)

// 0x100ca570 (vslot 37) - title bar, the three pages and their tabs, a grid and a text list per category (a seventh
// pair for the search results) and the category tabs (a seventh "search"), "GUI", the character window and the
// shortcut window (both registered at the top level), the weapon type and "Use", the save and load buttons, the
// search line, "Search" and "View List", and the third row (the "summon", "attack mode" and "target attack" buttons,
// the "face turn" check box, the two map number lines and "change map"); then the items, the characters and the
// hair window's values. NCWnd's OnCreate is not called. The texts are single bytes of the Korean code page widened to
// characters (this client's source was compiled that way): kept as they are.
INT NCPawnViewrWnd::OnCreate()
{
    guard(NCPawnViewrWnd::OnCreate);
    for (INT i = 0; i < 8; i++)
        DefaultWeapons[i] = -1;

    FrameCtrl = NewStock<NCFrameCtrl>(0x10010450, 1, 0, 0);
    FrameID = NID_PAWNVIEW_FRAME;
    FrameCtrl->ID = NID_PAWNVIEW_FRAME;
    FrameCtrl->SetPosition(0, 19, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    GridBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    BackTex = FindTexture(TEXT("L2UI.etcWnd.OptionBack"));

    MixAnimWnd = NewStock<NCPawnViewr_MixAnimWnd>(0x100c9860);
    MixAnimWnd->SetPosition(0, 53, 240, 236);
    CreateChild(MixAnimWnd, NWS_VISIBLE);
    HairChangeWnd = NewStock<NCPawnViewr_HairChangeWnd>(0x100c99f0);
    HairChangeWnd->SetPosition(0, 53, 240, 236);
    CreateChild(HairChangeWnd, NWS_VISIBLE);
    Simulator = NewStock<NCPawnViewr_Simulator>(0x100ca430);
    Simulator->SetPosition(0, 53, 240, 236);
    CreateChild(Simulator, NWS_VISIBLE);

    // "combined skin", "hair mesh", "skin test"
    PageTabs = NewStock<NCTabControl>(0x1002aae0);
    PageTabs->ID = NID_PAWNVIEW_PAGES;
    PageTabs->SetPosition(15, 42, 207, 16);
    CreateChild(PageTabs, NWS_VISIBLE);
    PageTabs->AddTab(TEXT("\x00C1\x00B6\x00C7\x00D5\x00BD\x00BA\x00C5\x00B3"), MixAnimWnd, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    PageTabs->AddTab(TEXT("\x00C7\x00EC\x00BE\x00EE\x00B8\x00DE\x00BD\x00AC"), HairChangeWnd, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    PageTabs->AddTab(TEXT("\x00BD\x00BA\x00C5\x00B3\x00C5\x00D7\x00BD\x00BA\x00C6\x00AE"), Simulator, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);

    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        ItemWnds[i] = NewStock<NCPawnView_InvenItemWnd>(0x100c9640, 6, 4, 80, 32);
        ItemWnds[i]->SetPosition(16, 319, 222, 140);
        CreateChild(ItemWnds[i], 0x31);   // 0x31: bits of unknown meaning, not visible
        TextWnds[i] = NewStock<NCPawnView_InvenTextWnd>(0x100c9500);
        TextWnds[i]->SetPosition(16, 319, 222, 140);
        CreateChild(TextWnds[i], 0x11);
    }
    // the seventh pair (the search results), written out in the stock code
    ItemWnds[6] = NewStock<NCPawnView_InvenItemWnd>(0x100c9640, 6, 4, 80, 32);
    ItemWnds[6]->SetPosition(16, 319, 222, 140);
    CreateChild(ItemWnds[6], 0x31);
    TextWnds[6] = NewStock<NCPawnView_InvenTextWnd>(0x100c9500);
    TextWnds[6]->SetPosition(16, 319, 222, 140);
    CreateChild(TextWnds[6], 0x11);

    CategoryTabs = NewStock<NCTabControl>(0x1002aae0);
    CategoryTabs->ID = NID_PAWNVIEW_CATEGORY;
    CategoryTabs->SetPosition(5, 290, 193, 20);
    CreateChild(CategoryTabs, NWS_VISIBLE);
    CategoryTabs->AddTab(STR_Chest, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Legs, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Gloves, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Feet, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Headgear, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Etc, NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(TEXT("\x00B0\x00CB\x00BB\x00F6"), NULL, 27, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);

    GUICheck = NewStock<NCCheckBox>(0x100049e0, TEXT("GUI"), 0, 0, 0);
    GUICheck->ID = NID_PAWNVIEW_GUI;
    GUICheck->SetPosition(195, 292, 39, 16);
    CreateChild(GUICheck, NWS_VISIBLE);
    GUICheck->bChecked = 1;

    for (INT i = 0; i < 19; i++)
        Characters[i].Pawn = NULL;

    CharSelectWnd = NewStock<NCPawnView_CharacterSelectWnd>(0x100c9790);
    CharSelectWnd->SetPosition(10, 54, 234, 503);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE | NWS_NOHITTEST);

    ShortCutWnd = NewStockClass(0x10298030, 0x100c9b20);
    ShortCutWnd->SetPosition(0, 0, 437, 44);
    CreateTopWindow(ShortCutWnd, NWS_VISIBLE | NWS_NOHITTEST);

    WeaponTypeCombo = NewStock<NCComboBox>(0x10005b20);
    WeaponTypeCombo->ID = NID_PAWNVIEW_WEAPONTYPE;
    WeaponTypeCombo->SetPosition(10, 555, 70, 17);
    CreateChild(WeaponTypeCombo, NWS_VISIBLE);
    WeaponTypeCombo->AddItem(TEXT("0(HAND)"));
    WeaponTypeCombo->AddItem(TEXT("1(1HS)"));
    WeaponTypeCombo->AddItem(TEXT("2(2HS)"));
    WeaponTypeCombo->AddItem(TEXT("3(DUAL)"));
    WeaponTypeCombo->AddItem(TEXT("4(POLE)"));
    WeaponTypeCombo->AddItem(TEXT("5(BOW)"));
    WeaponTypeCombo->AddItem(TEXT("7(DUALFIST)"));
    UseButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    UseButton->ID = NID_PAWNVIEW_USE;
    UseButton->SetPosition(85, 555, 28, 16);
    CreateChild(UseButton, NWS_VISIBLE);
    UseButton->SetTextID(STR_Use);

    // C3: "save" and "load" of the chosen character's equipment, next to "Use"
    Button11E = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button11E->ID = NID_PAWNVIEW_SAVE;
    Button11E->SetPosition(130, 555, 28, 16);
    CreateChild(Button11E, NWS_VISIBLE);
    Button11E->SetText(TEXT("\x00C0\x00FA\x00C0\x00E5"));                                // "save"
    Button11F = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button11F->ID = NID_PAWNVIEW_LOAD;
    Button11F->SetPosition(162, 555, 56, 16);
    CreateChild(Button11F, NWS_VISIBLE);
    Button11F->SetText(TEXT("\x00BA\x00D2\x00B7\x00AF\x00BF\x00C0\x00B1\x00E2"));          // "load"

    SearchEdit = NewStock<NCEditBox>(0x1000f3f0, 1, (const TCHAR*)NULL, 1, 64, 0);
    SearchEdit->ID = NID_PAWNVIEW_SEARCHEDIT;
    SearchEdit->SetPosition(10, 535, 60, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);
    SearchButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                      TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_PAWNVIEW_SEARCH;
    SearchButton->SetPosition(80, 535, 28, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetText(TEXT("\x00C3\x00A3\x00B1\x00E2"));                             // "find"
    ListButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                    TEXT("L2UI.Control.DefaultButton_click"), 0);
    ListButton->ID = NID_PAWNVIEW_VIEWLIST;
    ListButton->SetPosition(115, 535, 50, 16);
    CreateChild(ListButton, NWS_VISIBLE);
    ListButton->SetTextID(STR_ViewList);

    // the third row
    Button108 = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button108->ID = NID_PAWNVIEW_SPAWN;
    Button108->SetPosition(10, 515, 28, 16);
    CreateChild(Button108, NWS_VISIBLE);
    Button108->SetText(TEXT("\x00BC\x00D2\x00C8\x00AF"));                                // "summon"
    Button10C = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button10C->ID = NID_PAWNVIEW_FLAG;
    Button10C->SetPosition(45, 515, 50, 16);
    CreateChild(Button10C, NWS_VISIBLE);
    Button10C->SetText(TEXT("\x00B0\x00F8\x00B0\x00DD\x00B8\x00F0\x00B5\x00E5"));        // "attack mode"
    // the stock code reuses the field of the button above for this one (the pointer of that one is lost)
    Button10C = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button10C->ID = NID_PAWNVIEW_ATTACK;
    Button10C->SetPosition(105, 515, 50, 16);
    CreateChild(Button10C, NWS_VISIBLE);
    Button10C->SetText(TEXT("\x00C5\x00B8\x00B0\x00D9\x00C7\x00C7\x00B0\x00DD"));        // "target attack"
    Check11C = NewStock<NCCheckBox>(0x100049e0, TEXT("\x00BE\x00F3\x00B1\x00BC\x00C8\x00B8\x00C0\x00FC"), 0, 0, 0);
    Check11C->ID = NID_PAWNVIEW_FACECHECK;
    Check11C->SetPosition(165, 517, 70, 16);
    CreateChild(Check11C, NWS_VISIBLE);
    Check11C->bChecked = 1;
    RangeEditA = NewStock<NCEditBox>(0x1000f3f0, 3, (const TCHAR*)NULL, 1, 64, 0);
    RangeEditA->ID = NID_PAWNVIEW_RANGEEDITA;
    RangeEditA->SetPosition(10, 575, 23, 16);
    CreateChild(RangeEditA, NWS_VISIBLE);
    RangeEditB = NewStock<NCEditBox>(0x1000f3f0, 3, (const TCHAR*)NULL, 1, 64, 0);
    RangeEditB->ID = NID_PAWNVIEW_RANGEEDITB;
    RangeEditB->SetPosition(45, 575, 23, 16);
    CreateChild(RangeEditB, NWS_VISIBLE);
    Button110 = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                   TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button110->ID = NID_PAWNVIEW_GOTO;
    Button110->SetPosition(95, 575, 42, 16);
    CreateChild(Button110, NWS_VISIBLE);
    Button110->SetText(TEXT("\x00B8\x00CA\x00BA\x00AF\x00B0\x00E6"));                    // "change map"
    RangeEditA->SetTabOrder(RangeEditB->Entry, NULL);

    AddAllItem();
    AddAllCharacter();
    CharSelectWnd->OnNameComboBox(0);
    HairChangeWnd->SyncFromCharacter();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ca570, NCPawnViewrWnd, OnCreate)

// 0x100c4980 (vslot 38) - the character window, the shortcut window (C3) and two pages are deleted without leaving
// the window manager; the text pages, the simulator, the page tabs, the search line, its buttons, the weapon combo
// and "Use" are left alone
INT NCPawnViewrWnd::OnDestroy()
{
    guard(NCPawnViewrWnd::OnDestroy);
    // the six grids, written out in the stock code
    DestroyChild(this, ItemWnds[0]);
    DestroyChild(this, ItemWnds[1]);
    DestroyChild(this, ItemWnds[2]);
    DestroyChild(this, ItemWnds[3]);
    DestroyChild(this, ItemWnds[4]);
    DestroyChild(this, ItemWnds[5]);
    DestroyChild(this, GUICheck);
    DestroyChild(this, CategoryTabs);
    DestroyChild(this, UnknownF8);
    DestroyChild(this, FrameCtrl);
    if (CharSelectWnd)
        delete CharSelectWnd;
    if (ShortCutWnd)
        delete ShortCutWnd;
    if (MixAnimWnd)
        delete MixAnimWnd;
    if (HairChangeWnd)
        delete HairChangeWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c4980, NCPawnViewrWnd, OnDestroy)

// 0x100c4bf0 (vslot 44) - the character window shows too; only the current page's grid (or list) stays up
void NCPawnViewrWnd::ShowWindow()
{
    guard(NCPawnViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    OnGUICheck();
    NCWnd::ShowWindow();
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        if (CurrentPage != i || !GUICheck->bChecked)
            ItemWnds[i]->HideWindow();
        if (CurrentPage != i || GUICheck->bChecked)
            TextWnds[i]->HideWindow();
    }
    CategoryTabs->ShowCurrentPage();
    PageTabs->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x100c4bf0, NCPawnViewrWnd, ShowWindow)

// 0x100c3450 (vslot 45)
void NCPawnViewrWnd::HideWindow()
{
    guard(NCPawnViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x100c3450, NCPawnViewrWnd, HideWindow)

// 0x100c46a0 (vslot 56) - the background of the item part (the grids' or the lists'), then the upper part and the
// bottom of the option background; C3: three yellow lines of text (the viewer's own buffers) and the "_" and ".unr"
// between the two map edit lines (y 575); the origin goes on before the clip
INT NCPawnViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewrWnd::OnPaint);
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    UTexture* ItemBack = GUICheck->bChecked ? GridBackTex : BackTex;
    if (ItemBack)
        Device->DrawTexture(0, 289, 240, 185, 0, 0, 240, 185, ItemBack);
    if (BackTex)
    {
        Device->DrawTexture(0, 40, 240, 249, 0, 0, 240, 249, BackTex);
        Device->DrawTexture(0, 474, 240, 125, 0, 219, 240, 125, BackTex);
    }
    Device->DrawText(10, 466, 0xFFFFFF00, InfoText[0], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 483, 0xFFFFFF00, InfoText[1], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 500, 0xFFFFFF00, InfoText[2], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("_"), 0, 0, 0, 0, 0);
    Device->DrawText(35, (16 - Size.cy) / 2 + 575, 0xFFDCDCDC, TEXT("_"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT(".unr"), 0, 0, 0, 0, 0);
    Device->DrawText(69, (16 - Size.cy) / 2 + 575, 0xFFDCDCDC, TEXT(".unr"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c46a0, NCPawnViewrWnd, OnPaint)

// 0x100c3fc0 (vslot 69) - a category: its grid, or its list (the text page itself does not show: its ShowWindow is
// empty). C3: the page is set before the tests
INT NCPawnViewrWnd::OnCategoryTab(INT Index)
{
    HidePages();
    CurrentPage = Index;
    if (GUICheck->bChecked)
    {
        if (Index >= 0 && Index < 8 && ItemWnds[Index])
            ItemWnds[Index]->ShowWindow();
        return 1;
    }
    if (Index >= 0 && Index < 8 && ItemWnds[Index])
    {
        TextWnds[Index]->List->ShowWindow();
        TextWnds[CurrentPage]->ShowWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x100c3fc0, NCPawnViewrWnd, OnCategoryTab)

// 0x100c4910 (vslot 70) - the current category again, as a grid or as a list
INT NCPawnViewrWnd::OnGUICheck()
{
    HidePages();
    if (GUICheck->bChecked)
    {
        ItemWnds[CurrentPage]->ShowWindow();
        return 1;
    }
    TextWnds[CurrentPage]->List->ShowWindow();
    TextWnds[CurrentPage]->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x100c4910, NCPawnViewrWnd, OnGUICheck)

// the item id of a record (+0x0C; the header leaves it as filler)
static FORCEINLINE INT RecordItemID(NViewItemData* Data)
{
    return *(INT*)((BYTE*)Data + 0x0C);
}

// 0x100c8710 (vslot 71) - the matches go to the extra page 6 (its grid and its list), and the current category
// shown gets them again; the page of the category in view, the extra one and the lists are emptied first. The
// walk stops at the first empty record; a record without a name is skipped (C3)
INT NCPawnViewrWnd::OnSearch()
{
    const TCHAR* Text = SearchEdit->CommitText();
    HidePages();
    if (!GUICheck->bChecked)
    {
        TextWnds[CurrentPage]->List->ShowWindow();
        TextWnds[CurrentPage]->ShowWindow();
    }
    else
        ItemWnds[CurrentPage]->ShowWindow();
    ClearGrid(ItemWnds[CurrentPage]);
    TextWnds[CurrentPage]->List->Clear();
    ClearGrid(ItemWnds[6]);
    TextWnds[6]->List->Clear();
    if (Text)
    {
        NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            NViewItemData* Data = Map->Pairs(i).Value;
            NItemInfo Info;
            if (!Data)
                return 1;
            if (NameIndex(Data->Name) != 0 && appStrstr(*Data->Name, Text))
            {
                FillItemInfo(Data, &Info);
                AddItem(6, &Info);
                ItemWnds[6]->MaxItems++;
                INT Page = GetItemCategory(Data);
                if (CurrentPage == Page)
                {
                    AddItem(Page, &Info);
                    ItemWnds[Page]->MaxItems++;
                }
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c8710, NCPawnViewrWnd, OnSearch)

// 0x100c35c0 (vslot 74) - Enter in the search line searches
INT NCPawnViewrWnd::OnSearchEdit(INT Code)
{
    if (Code == 1)
        OnSearch();
    return 1;
}
NW_HOOK_METHOD(0x100c35c0, NCPawnViewrWnd, OnSearchEdit)

// 0x100c35e0 (vslot 75) - Enter in the second map line goes
INT NCPawnViewrWnd::OnRangeEdit(INT Code)
{
    if (Code == 1)
        OnGoToMap();
    return 1;
}
NW_HOOK_METHOD(0x100c35e0, NCPawnViewrWnd, OnRangeEdit)

// 0x100c4050 (vslot 76) - "summon": the chosen default character (the character window's name combo) spawned where the
// player stands, with the player's rotation and a player controller of its own, the record's resources set; then moved
// down onto the ground by the box trace of NCNpcView_CharacterSelectWnd::OnSummon (30 tries; a hit raises a very close
// one to the middle of 1.9 .. 2.4). Nothing checks that the pawn was spawned; its spawn event is not run
INT NCPawnViewrWnd::OnSpawnCharacter()
{
    APlayerController* Viewer = ViewActor();
    if (!Viewer || !Viewer->Pawn || !Viewer->XLevel)
        return 1;
    INT Index = NPawnView::Viewer()->CharSelectWnd->NameCombo->Selected;
    FVector Location = ViewActor()->Pawn->Location;
    ULevel* Level = ViewActor()->XLevel;
    AController* Controller = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, TEXT("Engine.PlayerController"), NULL, 1, NULL),
        NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1);
    UClass* PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL,
                                                 NPawnView::Viewer()->Characters[Index].GetClassNamePointer(), NULL, 0, NULL);
    if (!PawnClass)
        return 1;
    APawn* Pawn = (APawn*)Level->SpawnActor(PawnClass, NAME_None, Location, ViewActor()->Pawn->Rotation, NULL, 1, 0,
                                            Controller);
    Controller->Pawn = Pawn;
    Pawn->Controller = Controller;
    Controller->Role = ROLE_Authority;
    Controller->Pawn->Role = ROLE_Authority;
    Controller->Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    FVector Start = Location;
    FVector End = Location;
    Start.Z += 50.0f;
    User Record;
    appMemcpy(&Record, &NPawnView::Viewer()->Characters[Index], sizeof(User));
    Record.Pawn = Controller->Pawn;
    Record.SetPawnResource();
    for (INT i = 0; i < 30; i++)
    {
        Start.Z += 30.0f;
        End.Z -= 30.0f;
        FCheckResult Hit(1.0f);
        APawn* Summoned = Controller->Pawn;
        Level->SingleLineCheck(Hit, Summoned, End, Start, 0x86,
                               FVector(Summoned->CollisionRadius, Summoned->CollisionRadius,
                                       Summoned->CollisionHeight));
        if (Hit.Location.Z != 0.0f)
        {
            DOUBLE Step = Hit.Time * 12.0;
            if (Step < 1.9f)
                Hit.Location.Z = (FLOAT)(((DOUBLE)2.4f + 1.9f) * 0.5f - Step + Hit.Location.Z);
            Level->FarMoveActor(Controller->Pawn, Hit.Location, 0, 0, 0);
            break;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c4050, NCPawnViewrWnd, OnSpawnCharacter)

// 0x100c4440 (vslot 77) - flips bit 2 of the player controller's flags (+0x3F0)
INT NCPawnViewrWnd::OnToggleFlag()
{
    ViewActor()->Unknown638 ^= 2;
    return 1;
}
NW_HOOK_METHOD(0x100c4440, NCPawnViewrWnd, OnToggleFlag)

// 0x100c4500 (vslot 79) - the two edit lines as map coordinates (each under 100): a seamless world teleports the
// player to the middle of that map block, the other opens the map "<x>_<y>.unr"
INT NCPawnViewrWnd::OnGoToMap()
{
    TCHAR Command[128];
    if (RangeEditA->CommitText() && RangeEditB->CommitText())
    {
        APlayerController* Controller = ViewActor();
        if (Controller && Controller->Pawn && Controller->XLevel)
        {
            INT X = appAtoi(RangeEditA->CommitText());
            INT Y = appAtoi(RangeEditB->CommitText());
            if (X < 100 && Y < 100)
            {
                if (GIsL2Seamless)
                {
                    static_cast<UGameEngine*>(GConsole->Engine)->L2_Teleport(
                        ViewActor()->Pawn,
                        FVector(((FLOAT)(X - 9) * 32768.0f - 360448.0f) + 16384.0f,
                                ((FLOAT)(Y - 9) * 32768.0f - 294912.0f) + 16384.0f, -1000.0f),
                        0, 1, 0);
                    return 1;
                }
                appSprintf(Command, TEXT("open %02d_%02d.unr"), X, Y);
                static_cast<FExec*>(GConsole->Engine)->Exec(Command, *GLog);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c4500, NCPawnViewrWnd, OnGoToMap)

// 0x100c8900 (vslot 72) - every category refilled from the item table (the items of the lists' lines are not freed)
INT NCPawnViewrWnd::OnViewList()
{
    Clear();
    // the six lists, written out in the stock code
    TextWnds[0]->List->Clear();
    TextWnds[1]->List->Clear();
    TextWnds[2]->List->Clear();
    TextWnds[3]->List->Clear();
    TextWnds[4]->List->Clear();
    TextWnds[5]->List->Clear();
    AddAllItem();
    return 1;
}
NW_HOOK_METHOD(0x100c8900, NCPawnViewrWnd, OnViewList)

// 0x100c6fd0 (vslot 73) - the chosen character takes the first weapon found of the type picked ("7(DUALFIST)", the
// seventh line, is type 7); bare hands for type 0 or none found
INT NCPawnViewrWnd::OnUseWeapon()
{
    INT Type = WeaponTypeCombo->Selected;
    INT Index = CharSelectWnd->Selected;
    if (Index < 0 || Index > 19)
        return 1;
    if (Type == 6)
        Type = 7;
    User& Character = Characters[Index];
    NViewItemData* Data = NItemTable::Find(&DefaultWeapons[Type]);
    if (Data && Type != 0)
    {
        if (Data->BodyPart == BODYSLOT_LRHand)
        {
            Character.Equipped[BODYSLOT_RHand] = 0;
            Character.Equipped[BODYSLOT_LHand] = 0;
            Character.Equipped[Data->BodyPart] = DefaultWeapons[Type];
        }
        else
        {
            if (Data->BodyPart == BODYSLOT_RHand || Data->BodyPart == BODYSLOT_LHand)
                Character.Equipped[BODYSLOT_LRHand] = 0;
            Character.Equipped[Data->BodyPart] = DefaultWeapons[Type];
        }
    }
    else
    {
        Character.Equipped[BODYSLOT_RHand] = 0;
        Character.Equipped[BODYSLOT_LHand] = 0;
        Character.Equipped[BODYSLOT_LRHand] = 0;
    }
    Character.Pawn = NPawnView::PlayerPawn();
    Character.SetPawnResource();
    return 1;
}
NW_HOOK_METHOD(0x100c6fd0, NCPawnViewrWnd, OnUseWeapon)

// 0x100c24b0 - every category's grid and list hidden
__declspec(noinline) void NCPawnViewrWnd::HidePages()
{
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        ItemWnds[i]->HideWindow();
        TextWnds[i]->HideWindow();
        TextWnds[i]->List->HideWindow();
    }
}
NW_HOOK_METHOD(0x100c24b0, NCPawnViewrWnd, HidePages)

// "<race><gender><class type>" in the character window's combo box, with the number of the line (C3: the combo
// box's AddItem(Text, Param) at 0x10005a00, which its header does not carry yet: ComboAddItemData above)
static FORCEINLINE void AddCharacterName(NCPawnViewrWnd* Viewer, TCHAR* Text, INT Race,
                                         INT Sex, INT Type, INT Param)
{
    appSprintf(Text, TEXT("%s%s%s"), *GL2GameData.SysString[Race], *GL2GameData.SysString[Sex],
               *GL2GameData.SysString[Type]);
    ComboAddItemData(Viewer->CharSelectWnd->NameCombo, Text, Param);
}

// 0x100c24f0 - the records emptied and given the creation screen's race, gender and class (the 19th reads past the 18
// of the table), and the 18 names in the character window; C3: each name gets a number (a code of its race and sex)
__declspec(noinline) void NCPawnViewrWnd::AddAllCharacter()
{
    guard(NCPawnViewrWnd::AddAllCharacter);
    TCHAR Text[100];
    for (INT i = 0; i < 19; i++)
    {
        __stosd((DWORD*)&Characters[i], 0, sizeof(User) / 4);
        Characters[i].Race = GDefaultCharacters[i].Race;
        Characters[i].Sex = GDefaultCharacters[i].Sex;
        Characters[i].ClassID = GDefaultCharacters[i].ClassID;
    }
    AddCharacterName(this, Text, STR_Human, STR_Male, STR_Warrior, 0);
    AddCharacterName(this, Text, STR_Human, STR_Female, STR_Warrior, 1);
    AddCharacterName(this, Text, STR_Human, STR_Male, STR_Spellcaster, 8);
    AddCharacterName(this, Text, STR_Human, STR_Female, STR_Spellcaster, 9);
    AddCharacterName(this, Text, STR_Elf, STR_Male, STR_Warrior, 6);
    AddCharacterName(this, Text, STR_Elf, STR_Female, STR_Warrior, 7);
    AddCharacterName(this, Text, STR_Elf, STR_Male, STR_Spellcaster, 6);
    AddCharacterName(this, Text, STR_Elf, STR_Female, STR_Spellcaster, 7);
    AddCharacterName(this, Text, STR_DarkElf, STR_Male, STR_Warrior, 2);
    AddCharacterName(this, Text, STR_DarkElf, STR_Female, STR_Warrior, 3);
    AddCharacterName(this, Text, STR_DarkElf, STR_Male, STR_Spellcaster, 2);
    AddCharacterName(this, Text, STR_DarkElf, STR_Female, STR_Spellcaster, 3);
    AddCharacterName(this, Text, STR_Orc, STR_Male, STR_Warrior, 10);
    AddCharacterName(this, Text, STR_Orc, STR_Female, STR_Warrior, 11);
    AddCharacterName(this, Text, STR_Orc, STR_Male, STR_Spellcaster, 12);
    AddCharacterName(this, Text, STR_Orc, STR_Female, STR_Spellcaster, 13);
    AddCharacterName(this, Text, STR_Dwarf, STR_Male, STR_Warrior, 4);
    AddCharacterName(this, Text, STR_Dwarf, STR_Female, STR_Warrior, 5);
    unguard;
}
NW_HOOK_METHOD(0x100c24f0, NCPawnViewrWnd, AddAllCharacter)

// 0x100c2d40 - the page of an item record: armor by its body part, the rest under etc; the first weapon of each type
// met is kept for OnUseWeapon (a dual only when two-handed). One argument (the record's id is read from it)
__declspec(noinline) INT NCPawnViewrWnd::GetItemCategory(NViewItemData* Data)
{
    if (Data->Class == 1)
    {
        switch (Data->BodyPart)
        {
        case 6:
            return PAWNVIEW_Headgear;
        case 9:
            return PAWNVIEW_Gloves;
        case 10:
            return PAWNVIEW_Chest;
        case 11:
            return PAWNVIEW_Legs;
        case 12:
            return PAWNVIEW_Feet;
        }
    }
    else if (Data->Class == 0)
    {
        INT Type = Data->PawnWeaponType;
        if (DefaultWeapons[Type] == -1 && (Type != 3 || Data->BodyPart == BODYSLOT_LRHand))
            DefaultWeapons[Type] = RecordItemID(Data);
    }
    return PAWNVIEW_Etc;
}
NW_HOOK_METHOD(0x100c2d40, NCPawnViewrWnd, GetItemCategory)

// 0x100c2dd0 (name ours; AddAllItem and OnSearch call it) - the record into an NItemInfo: the names and icons, the
// description, the numbers by item class (0 weapon, 1 armor, 2 etc item). A record without a name leaves the
// NItemInfo alone (C3). The icon comes from the first icon's name, "Icon.NoImage" when missing (C3). The fields
// after +0x178C of the NItemInfo are written by offset (this client moved them 4 bytes up; the header only grew)
void NCPawnViewrWnd::FillItemInfo(NViewItemData* Data, NItemInfo* Info)
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
        else if (Class == ITEMCLASS_Armor)
        {
            Info->UnknownC28 = Data->Weight;
            Info->UnknownC34[7] = Data->ArmorCrystalGrade;
            Info->SubType = Data->ArmorType;
            Info->UnknownC34[2] = Data->PDef;
            Info->UnknownC34[3] = Data->MDef;
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
NW_HOOK_METHOD(0x100c2dd0, NCPawnViewrWnd, FillItemInfo)

// 0x100c3390 - the grids emptied (not the lists)
__declspec(noinline) void NCPawnViewrWnd::Clear()
{
    guard(NCPawnViewrWnd::Clear);
    ClearGrid(ItemWnds[PAWNVIEW_Chest]);
    ClearGrid(ItemWnds[PAWNVIEW_Legs]);
    ClearGrid(ItemWnds[PAWNVIEW_Gloves]);
    ClearGrid(ItemWnds[PAWNVIEW_Feet]);
    ClearGrid(ItemWnds[PAWNVIEW_Headgear]);
    ClearGrid(ItemWnds[PAWNVIEW_Etc]);
    unguard;
}
NW_HOOK_METHOD(0x100c3390, NCPawnViewrWnd, Clear)

// 0x100c34e0 - an item into a category's grid and list (the list line gets an NItemEntry of its own, never freed); the
// page check lets 7 (the unused eighth pointer) through
__declspec(noinline) INT NCPawnViewrWnd::AddItem(INT Page, NItemInfo* Info)
{
    guard(NCPawnViewrWnd::AddItem);
    if (Page >= 0 && Page < 8 && ItemWnds[Page])
    {
        ItemWnds[Page]->AddItem(Info);
        NItemEntry* Entry = new NItemEntry(Info);
        TextWnds[Page]->List->AddString(Info->Name, 0xFFDCDCDC, (INT)Entry);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c34e0, NCPawnViewrWnd, AddItem)

// 0x100c85f0 - every record of the item table into its category; each grid grows by one for every item added, so
// none ever fills
__declspec(noinline) void NCPawnViewrWnd::AddAllItem()
{
    guard(NCPawnViewrWnd::AddAllItem);
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Data = Map->Pairs(i).Value;
        if (Data)
        {
            NItemInfo Info;
            FillItemInfo(Data, &Info);
            INT Page = GetItemCategory(Data);
            AddItem(Page, &Info);
            ItemWnds[Page]->MaxItems++;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x100c85f0, NCPawnViewrWnd, AddAllItem)

// 0x100c4470 (vslot 78, button 0x11D; name ours) - the player's pawn attacks the player's target (C3: new)
INT NCPawnViewrWnd::OnAttackTarget()
{
    APawn* Pawn = ViewActor()->Pawn;
    AActor* Target = ViewActor()->Target;
    if (Pawn && Target)
        Pawn->Action_Attack(Target, 1, 0, 0, 1, 0, 0, 1);
    return 1;
}
NW_HOOK_METHOD(0x100c4470, NCPawnViewrWnd, OnAttackTarget)

// 0x100c44c0 (vslot 80, check box 0x11C; name ours) - the check box's state into bit 0 of the pawn's flag word
// (C3: new; the word at +0x5B8, of which APawn declares the first byte)
INT NCPawnViewrWnd::OnFaceCheck()
{
    APawn* Pawn = ViewActor()->Pawn;
    DWORD& Flags = *(DWORD*)&Pawn->Unknown678;
    Flags ^= (Check11C->bChecked ^ Flags) & 1;
    return 1;
}
NW_HOOK_METHOD(0x100c44c0, NCPawnViewrWnd, OnFaceCheck)

// 0x100c3000 (vslot 81, button 0x11E; name ours) - the equipment of the chosen default character into L2.ini
// ([L2PawnViewer] ST_<slot>), then the config cache is flushed (C3: new)
INT NCPawnViewrWnd::OnSaveEquipment()
{
    TCHAR Ini[] = TEXT(".\\L2.ini");
    INT Index = NPawnView::Viewer()->CharSelectWnd->Selected;
    if (Index >= 0 && Index < 20)
    {
        User& Character = NPawnView::Viewer()->Characters[Index];
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_HEAD"), Character.Equipped[6], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_RHAND"), Character.Equipped[7], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_LHAND"), Character.Equipped[8], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_GLOVES"), Character.Equipped[9], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_CHEST"), Character.Equipped[10], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_LEGS"), Character.Equipped[11], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_FEET"), Character.Equipped[12], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_BACK"), Character.Equipped[13], Ini);
        GConfig->SetInt(TEXT("L2PawnViewer"), TEXT("ST_RLHAND"), Character.Equipped[14], Ini);
        // FConfigCache::Flush(UBOOL Read, const TCHAR* Filename) (slot 13; the header stops at 12)
        ::nw::VirtualCall<void>(GConfig, 13, (UBOOL)1, (const TCHAR*)NULL);
    }
    return 1;
}
NW_HOOK_METHOD(0x100c3000, NCPawnViewrWnd, OnSaveEquipment)

// 0x100c31d0 (vslot 82, button 0x11F; name ours) - the equipment of the chosen default character read back from
// L2.ini and put on (C3: new)
INT NCPawnViewrWnd::OnLoadEquipment()
{
    TCHAR Ini[] = TEXT(".\\L2.ini");
    INT Index = NPawnView::Viewer()->CharSelectWnd->Selected;
    if (Index >= 0 && Index < 20)
    {
        User& Character = NPawnView::Viewer()->Characters[Index];
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_HEAD"), Character.Equipped[6], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_RHAND"), Character.Equipped[7], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_LHAND"), Character.Equipped[8], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_GLOVES"), Character.Equipped[9], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_CHEST"), Character.Equipped[10], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_LEGS"), Character.Equipped[11], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_FEET"), Character.Equipped[12], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_BACK"), Character.Equipped[13], Ini);
        GConfig->GetInt(TEXT("L2PawnViewer"), TEXT("ST_RLHAND"), Character.Equipped[14], Ini);
        Character.SetPawnResource();
    }
    return 1;
}
NW_HOOK_METHOD(0x100c31d0, NCPawnViewrWnd, OnLoadEquipment)

NW_CHECK_VTABLE(NCPawnViewrWnd, 0x10195350, 83)

/*-----------------------------------------------------------------------------
    NCPawnView_ShortCutWnd (C3 only).
-----------------------------------------------------------------------------*/

// 0x100c9b20 - the texture and the two text tables cleared
NCPawnView_ShortCutWnd::NCPawnView_ShortCutWnd()
{
    BackTex = NULL;
    memset(Commands, 0, sizeof(Commands));
    memset(Names, 0, sizeof(Names));
}
NW_HOOK_CTOR(0x100C9B20, NCPawnView_ShortCutWnd)

// 0x100c9bd0
NW_HOOK_DTOR(0x100C9BD0, NCPawnView_ShortCutWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared by the classes that do not override it
INT NCPawnView_ShortCutWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100c83d0 (vslot 37) - the back (417 x 44) and an undecorated title bar of the window's size (id 0x120), then the
// ten commands and names of L2.ini. The stock hands GetString 256 and 32 as the sizes of the 128 and 16 character
// slots (so a long line spills into the next one)
INT NCPawnView_ShortCutWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.ShortcutWnd.Back1"));
    FrameCtrl = new NCFrameCtrl(0, 0, 0);
    FrameID = 0x120;
    FrameCtrl->ID = FrameID;
    FrameCtrl->SetPosition(0, 0, 0x1A1, 0x2C);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TCHAR IniFile[9] = TEXT(".\\L2.ini");
    TCHAR Key[128];
    memset(Key, 0, sizeof(Key));
    for (INT i = 0; i < 10; i++)
    {
        appSprintf(Key, TEXT("ShortcutCommand%s"), appItoa(i));
        GConfig->GetString(TEXT("L2PawnViewer"), Key, Commands[i], 0x100, IniFile);
        appSprintf(Key, TEXT("ShortcutName%s"), appItoa(i));
        GConfig->GetString(TEXT("L2PawnViewer"), Key, Names[i], 0x20, IniFile);
    }
    return 1;
}
NW_HOOK_METHOD(0x100C83D0, NCPawnView_ShortCutWnd, OnCreate)

// 0x100c6cb0 (vslot 38) - the title bar goes, then NCWnd's
INT NCPawnView_ShortCutWnd::OnDestroy()
{
    if (FrameCtrl)
    {
        RemoveChildWindow(FrameCtrl->Entry);
        if (FrameCtrl)
            delete FrameCtrl;
        FrameCtrl = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100C6CB0, NCPawnView_ShortCutWnd, OnDestroy)

// 0x100c6b40 (vslot 56) - the back, then the ten names at the buttons' x (31 + 37 * i) at y 6 in 0xFFDCDCDC; the
// text's extent is asked and not used
INT NCPawnView_ShortCutWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 0x1A1, 0x2C, 0, 0, 0x1A1, 0x2C, BackTex, 0xFF, 1);
    INT X = 0x1F;
    TCHAR* Name = &Names[0][0];
    do
    {
        if (Name)
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0, L2FONT_Normal);
            Device->DrawText(X, 6, 0xFFDCDCDC, Name, 0x20, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        }
        X += 0x25;
        Name += 16;
    } while (X < 0x191);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100C6B40, NCPawnView_ShortCutWnd, OnPaint)

// 0x100c3c30 (ret 8) - the button of the row under the point, -1 for none: the points are taken relative to the
// window; buttons are 33 wide from x 31 (0x1F) in steps of 37, between y 6 and y 38; the last range hit wins
INT NCPawnView_ShortCutWnd::HitTest(INT X, INT Y)
{
    INT RelX = X - ScreenX;
    INT RelY = Y - ScreenY;
    INT Result = -1;
    if (RelY < 6 || RelY > 0x26)
        return Result;
    if (RelX >= 0x1F && RelX <= 0x3F)
        Result = 0;
    if (RelX >= 0x44 && RelX <= 0x64)
        Result = 1;
    if (RelX >= 0x69 && RelX <= 0x89)
        Result = 2;
    if (RelX >= 0x8E && RelX <= 0xAE)
        Result = 3;
    if (RelX >= 0xB3 && RelX <= 0xD3)
        Result = 4;
    if (RelX >= 0xD8 && RelX <= 0xF8)
        Result = 5;
    if (RelX >= 0xFD && RelX <= 0x11D)
        Result = 6;
    if (RelX >= 0x122 && RelX <= 0x142)
        Result = 7;
    if (RelX >= 0x147 && RelX <= 0x167)
        Result = 8;
    if (RelX >= 0x16C && RelX <= 0x18C)
        Result = 9;
    return Result;
}
NW_HOOK_METHOD(0x100C3C30, NCPawnView_ShortCutWnd, HitTest)

// 0x100c6c40 (vslot 62) - the command of the button hit runs on the engine and on the first viewport
INT NCPawnView_ShortCutWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest((WORD)lParam, (WORD)((DWORD)lParam >> 16));
    if (Index >= 0)
    {
        TCHAR* Command = Commands[Index];
        if (Command)
        {
            static_cast<FExec*>(GConsole->Engine)->Exec(Command, *GLog);
            static_cast<FExec*>(GConsole->Engine->Client->Viewports(0))->Exec(Command, *GLog);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100C6C40, NCPawnView_ShortCutWnd, OnLButtonDown)

NW_CHECK_VTABLE(NCPawnView_ShortCutWnd, 0x10195940, 68)
