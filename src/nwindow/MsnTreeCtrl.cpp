/*=============================================================================
    MsnTreeCtrl.cpp - the messenger's scrolling person tree (C3 only; C3 addresses). Stock: 0x10141B60 (WindowProc, shared),
    0x10141BC0 .. 0x10141F80 (the virtuals), 0x10142810 (OnCreate), 0x101428D0 / 0x10142A40 (add a group, a person), 0x10143520
    (remove a person), 0x101438A0 / 0x10143910 (constructor, destructor).
=============================================================================*/
#include "nwindow/MsnTreeCtrl.h"
#include "nwindow/L2FriendTreeCtrl.h"
#include "nwindow/MsnTree.h"
#include "nwindow/NMsnTree.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"

enum { NID_MSNTREE_TREE = 0xA00 };   // the inner tree's id (name ours)

// 0x101438a0 - NCTreeCtrl's constructor, four words cleared, and the scroll bar 16 px wide again
MsnTreeCtrl::MsnTreeCtrl()
{
    Unknown114 = 0;
    Unknown118 = 0;
    FriendBook = NULL;
    Unknown120 = 0;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x101438A0, MsnTreeCtrl)

// the person list's array of record pointers: its destructor is a function of its own in the stock code (0x10123570: the elements'
// Remove with UnTemplate.h's checks, then FArray's destructor)
class NMsnPersonPtrArray : public TArray<NMsnPersonRec*>
{
public:
#if NW_HOOK_MODE
    __declspec(noinline) ~NMsnPersonPtrArray() { ::nw::StockCallAt<void>(0x10123570, this); }
#else
    __declspec(noinline) ~NMsnPersonPtrArray() {}   // the base destructor does the work (0x10123570 is that template's out-of-line copy)
#endif
};

// 0x10143910
NW_HOOK_DTOR(0x10143910, MsnTreeCtrl)

// 0x10141b60 (vslot 25) - one body for both tree controls (hooked as L2FriendTreeCtrl's)
#if NW_HOOK_MODE
NW_NAKED INT MsnTreeCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10141B60)
#else
INT MsnTreeCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)   // the standalone: that body, L2FriendTreeCtrl::WindowProc's (0x10141B60)
{
    return ((L2FriendTreeCtrl*)(NCTreeCtrl*)this)->L2FriendTreeCtrl::WindowProc(Msg, wParam, lParam);
}
#endif

// 0x10142810 (vslot 37) - NCScrollWnd's (not NCTreeCtrl's), then the person tree, as wide as the window less the scroll bar
// (16), as high as the window
INT MsnTreeCtrl::OnCreate()
{
    NCScrollWnd::OnCreate();
    MsnTreeWnd = new MsnTree();
    MsnTreeWnd->ID = NID_MSNTREE_TREE;
    MsnTreeWnd->SetPosition(0, 0, (INT)(Width - 16.0f), (INT)Height);
    CreateChild(MsnTreeWnd, 0x22);
    return 1;
}
NW_HOOK_METHOD(0x10142810, MsnTreeCtrl, OnCreate)

// 0x10141bc0 (vslot 38)
INT MsnTreeCtrl::OnDestroy()
{
    if (MsnTreeWnd)
    {
        RemoveChildWindow(MsnTreeWnd->Entry);
        delete MsnTreeWnd;
        MsnTreeWnd = NULL;
    }
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10141BC0, MsnTreeCtrl, OnDestroy)

// 0x10141c00 (vslot 56) - the content height follows the tree (and 21 more)
INT MsnTreeCtrl::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    SetContentHeightEx(MsnTreeWnd->ContentHeight + 21);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10141C00, MsnTreeCtrl, OnPaint)

// 0x10141d70 (vslot 69) - one line up: the tree window moves down by a line
INT MsnTreeCtrl::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (0 < ScrollY)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX = DeltaX - ScrollX;
        DeltaY = DeltaY - ScrollY;
        MsnTreeWnd->MoveWindow(0, LineStep);
        MsnTreeWnd->ScrollOffset -= LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x10141D70, MsnTreeCtrl, ScrollUp)

// 0x10141e30 (vslot 70) - one line down
INT MsnTreeCtrl::ScrollDown()
{
    if (MinScrollY < ScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX = DeltaX - ScrollX;
        DeltaY = DeltaY - ScrollY;
        MsnTreeWnd->MoveWindow(0, -LineStep);
        MsnTreeWnd->ScrollOffset += LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x10141E30, MsnTreeCtrl, ScrollDown)

// 0x10141f00 (vslot 71) - the bar was moved to Value
INT MsnTreeCtrl::OnScrollBar(INT Value)
{
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    ScrollY = -Value;
    DeltaY = DeltaY - ScrollY;
    DeltaX = DeltaX - ScrollX;
    MsnTreeWnd->MoveWindow(0, -DeltaY);
    MsnTreeWnd->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x10141F00, MsnTreeCtrl, OnScrollBar)

// 0x10141f80 (vslot 67) - three lines per notch of the wheel
INT MsnTreeCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SHORT Notches = (SHORT)((SHORT)HIWORD(wParam) / 120);
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    if (Notches < 1)
    {
        if (MinScrollY < ScrollY)
        {
            ScrollY = ScrollY + LineStep * Notches * 3;
            if (ScrollY < MinScrollY)
                ScrollY = MinScrollY;
            ScrollBar->SetScrollPos(-ScrollY);
        }
    }
    else if (ScrollY < 0)
    {
        ScrollY = ScrollY + LineStep * Notches * 3;
        if (0 < ScrollY)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
    }
    DeltaY = DeltaY - ScrollY;
    DeltaX = DeltaX - ScrollX;
    MsnTreeWnd->MoveWindow(0, -DeltaY);
    MsnTreeWnd->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x10141F80, MsnTreeCtrl, OnMouseWheel)

// 0x10141ce0 (vslot 72) - the tree emptied and brought back to its place, the scroll at the top
void MsnTreeCtrl::Clear()
{
    MsnTreeWnd->Clear();
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    MsnTreeWnd->GetLocalRect(&R);
    MsnTreeWnd->MoveWindow(-R.X, -R.Y);
    MsnTreeWnd->ScrollOffset = 0;
    ScrollY = 0;
    ScrollX = 0;
    DeltaY = 0;
    DeltaX = 0;
    MinScrollY = 0;
    ::nw::Field<INT>(ScrollBar, 0x1D8) = 0;
}
NW_HOOK_METHOD(0x10141CE0, MsnTreeCtrl, Clear)

// 0x101428d0 (name ours; ret 0x1C) - a group entry: a plus / minus picture, the name, the id, 220 x 14 for the text; the
// record is freed by this function
void __fastcall MsnTreeCtrl::AddGroupBody(MsnTreeCtrl* Self, void* Unused, NMsnGroupArg Group)
{
    NTVItemData Data;
    UTexture* Plus = Self->FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
    UTexture* Minus = Self->FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
    Data.ExpandedTex = Minus;
    Data.CollapsedTex = Plus;
    Data.Icon2Tex = Self->FindTexture(TEXT("NWindow.BlackTexture"));
    appStrcpy(Data.Text, *Group.Name());
    Data.Unknown04 = Group.ID();
    Data.bCenterOnIcon2 = 0;
    Data.Icon2Box.X = 0;
    Data.Icon2Box.Y = 0;
    Data.Icon2Box.W = 0;
    Data.IconBox.X = 0;
    Data.IconBox.Y = 0;
    Data.TextBox.X = 0;
    Data.TextBox.Y = 0;
    Data.Unknown2848 = 2;
    Data.Icon2Box.H = 15.0f;
    Data.IconBox.W = 15.0f;
    Data.IconBox.H = 15.0f;
    Data.TextBox.W = 220.0f;
    Data.TextBox.H = 14.0f;
    if (Self->MsnTreeWnd)
        Self->MsnTreeWnd->InsertGroupItem(&Data);
    MsnGroupFree(&Group);
}
NW_HOOK_FUNC(0x101428D0, MsnTreeCtrl::AddGroupBody)

// 0x10142a40 (name ours; ret 0x44) - a person entry under his group: the name, the account (Text3), the presence code (Tag), a
// small picture, 220 x 14 for the text; the record is freed by this function
void __fastcall MsnTreeCtrl::AddFriendBody(MsnTreeCtrl* Self, void* Unused, NMsnPersonRec Person)
{
    NTVItemData Data;
    Data.Icon2Tex = Self->FindTexture(TEXT("L2UI_CH3.Msn.msnicon4"));
    {
        BYTE NameRaw[sizeof(FString)];
        FString& Name = *Person.GetDisplayName((FString*)NameRaw);
        appStrcpy(Data.Text, *Name);
        Name.~FString();
    }
    appStrcpy(Data.Text3, *Person.Field20());
    appStrcpy(Data.Tag, *Person.Status());
    INT GroupID = Person.GroupID();
    Data.Unknown04 = GroupID;
    Data.Unknown2848 = 3;
    Data.bCenterOnIcon2 = 1;
    Data.Icon2Box.X = 0;
    Data.Icon2Box.Y = 0;
    Data.Icon2Box.W = 14.0f;
    Data.Icon2Box.H = 14.0f;
    Data.TextBox.X = 0;
    Data.TextBox.Y = 0;
    Data.TextBox.W = 220.0f;
    Data.TextBox.H = 14.0f;
    NCTVItem* GroupItem = (NCTVItem*)((NMsnTreeCtrlView*)Self)->FindGroupNode(GroupID);      // the group entry with that id, 0x101420B0
    if (GroupItem)
        Self->MsnTreeWnd->InsertFriendItem(&Data, GroupItem);
    MsnPersonFree(&Person);
}
NW_HOOK_FUNC(0x10142A40, MsnTreeCtrl::AddFriendBody)

// 0x1012fa70 (ret 4) - the copy constructor of the entries' array (the stock instance: the count, Realloc, then the elements one
// by one through 0x10131560); the sorted insertions and RemoveFriend copy an entry's children with it
NW_HOOK_CTOR_ARGS(0x1012FA70, NTVItemArray, (const NTVItemArray& Other), (Other))

// a copy of an entry's children (the stock's out-of-line copy of the pointer array, 0x1012FA70, ret 4)
static FORCEINLINE void CopyChildren(NTVItemArray* Dst, NTVItemArray* Src)
{
#if NW_HOOK_MODE
    ::nw::StockCallAt<void>(0x1012FA70, Dst, Src);
#else
    new ((void*)Dst) NTVItemArray(*Src);          // the copy constructor of the array (0x1012FA70: the standalone compiles the template)
#endif
}

// 0x10143520 (name ours; ret 0xC) - the first person entry whose account (Text3) is Account is deleted, in whichever group;
// Account is freed by this function
void MsnTreeCtrl::RemoveFriend(NMsnStringArg Account)
{
    NCTVItem* pRootItem = MsnTreeWnd->Root;
    NW_CHECK_AT(pRootItem, "MSN\\TreeCtrl.cpp", 0x11C);
    NTVItemArray Groups;
    CopyChildren(&Groups, &pRootItem->Children);
    for (INT g = 0; g < Groups.Num(); g++)
    {
        NCTVItem* pGroupItem = Groups(g);
        NW_CHECK_AT(pGroupItem, "MSN\\TreeCtrl.cpp", 0x122);
        NTVItemArray Friends;
        CopyChildren(&Friends, &pGroupItem->Children);
        for (INT f = 0; f < Friends.Num(); f++)
        {
            NCTVItem* pFriendItem = Friends(f);
            NW_CHECK_AT(pFriendItem, "MSN\\TreeCtrl.cpp", 0x12A);
            FString Text3(pFriendItem->Data.Text3);
            if (Text3 == Account.Str())
            {
                MsnTreeWnd->DeleteItem(&pFriendItem);
                Account.Str().~FString();
                return;
            }
        }
    }
    Account.Str().~FString();
}
NW_HOOK_METHOD(0x10143520, MsnTreeCtrl, RemoveFriend)

NW_CHECK_VTABLE(MsnTreeCtrl, 0x101AB4B8, 73)

// 0x101420f0 (name ours; ret 8) - the entry of this account (its Text3 is the account): Item itself or the first one under it
NCTVItem* MsnTreeCtrl::FindFriendItem(const TCHAR* Account, NCTVItem* Item)
{
    if (!Item)
        return NULL;
    if (appStrcmp(Item->Data.Text3, Account) == 0)
        return Item;
    NCTVItem* Found = NULL;
    for (INT i = 0; i < Item->Children.Num(); i++)
    {
        Found = FindFriendItem(Account, Item->Children(i));
        if (Found)
            break;
    }
    return Found;
}
NW_HOOK_METHOD(0x101420F0, MsnTreeCtrl, FindFriendItem)

// 0x10143320 (name ours; ret 4) - everything under the group goes (the entry stays), then every person of the list that belongs
// to the group is added again
void MsnTreeCtrl::RefreshGroup(NCTVItem** Group)
{
    if (*Group)
    {
        (*Group)->DeleteAllChildren();                                                // NCTVItem::DeleteAllChildren 0x1002F4F0
        NMsnPersonPtrArray Persons;
#if NW_HOOK_MODE
        ::nw::StockCallAt<void>(0x10142780, &Persons, (BYTE*)FriendBook + 4);        // a copy of the list's array of records
#else
        (TArray<NMsnPersonRec*>&)Persons = *(const TArray<NMsnPersonRec*>*)((BYTE*)FriendBook + 4);   // the same copy (0x10142780: the template)
#endif
        for (INT i = 0; i < Persons.Num(); i++)
        {
            NMsnPersonRec* Person = Persons(i);
            if (Person && *Group == (NCTVItem*)((NMsnTreeCtrlView*)this)->FindGroupNode(Person->GroupID()))   // 0x101420B0
                AddFriend(MsnPersonCopy(*Person));
        }
    }
}
NW_HOOK_METHOD(0x10143320, MsnTreeCtrl, RefreshGroup)
