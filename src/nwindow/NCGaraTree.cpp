/*=============================================================================
    NCGaraTree.cpp - the tree of NCGaraTVItem entries. C3 stock: 0x100127C0 .. 0x10012E2F (the methods), 0x10013850
    .. 0x10013B10 (constructor, destructor, AddItem).
=============================================================================*/
#include "nwindow/NCGaraTree.h"
#include "nwindow/NCGDevice.h"

// 0x10013850 - the root and the sizes are 0 (NControl's constructor is inlined in the stock one)
NCGaraTree::NCGaraTree()
{
    Root = NULL;
    ScrollOffset = 0;
    ContentHeight = 0;
}
NW_HOOK_CTOR(0x10013850, NCGaraTree)

// 0x100138D0
NW_HOOK_DTOR(0x100138D0, NCGaraTree)

// 0x100127A0 (vslot 25) - the stock body is the one the linker shared with NCButton's: NControl's, as a flag
INT NCGaraTree::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100127F0 (vslot 29) - the parent window takes the focus
void NCGaraTree::SetFocus()
{
    if (Entry->Parent)
        Entry->Parent->Wnd->SetFocus();
}
NW_HOOK_METHOD(0x100127F0, NCGaraTree, SetFocus)

// 0x100127D0 (vslot 62) - the same, and the click is not taken
INT NCGaraTree::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (Entry->Parent)
        Entry->Parent->Wnd->SetFocus();
    return 0;
}
NW_HOOK_METHOD(0x100127D0, NCGaraTree, OnLButtonDown)

// 0x10012810 (vslot 38) - every entry goes
INT NCGaraTree::OnDestroy()
{
    guard(NCGaraTree::OnDestroy);
    NControl::OnDestroy();
    Clear();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10012810, NCGaraTree, OnDestroy)

// 0x100127C0 (vslot 71, name ours) - the root and what is under it
void NCGaraTree::Clear()
{
    DeleteItem(&Root);
}
NW_HOOK_METHOD(0x100127C0, NCGaraTree, Clear)

// 0x10012C50 (vslot 70, name ours) - the entries under it first, then the entry itself: its window is taken out of
// the tree's and it is deleted
void NCGaraTree::DeleteItem(NCGaraTVItem** Item)
{
    if (*Item)
    {
        if ((*Item)->Children.Num() > 0)
        {
            for (INT i = 0; i < (*Item)->Children.Num(); i++)
                DeleteItem(&(*Item)->Children(i));
        }
        if (*Item)
        {
            RemoveChildWindow((*Item)->Entry);
            if (*Item)
                delete *Item;
            *Item = NULL;
        }
    }
}
NW_HOOK_METHOD(0x10012C50, NCGaraTree, DeleteItem)

// 0x10012CC0 (vslot 56) - the entries are laid out under the clip (moved up by the control's scroll); the height they
// took becomes the window's
INT NCGaraTree::OnPaint(NCGDevice* Device)
{
    guard(NCGaraTree::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY + ScrollOffset, (INT)Width, (INT)Height);
    ContentHeight = DrawItem(Device, Root, 0, 0);
    SetWindowSize(Width, (FLOAT)ContentHeight);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10012CC0, NCGaraTree, OnPaint)

// 0x10012DC0 (vslot 72, name ours) - the entry is put at (X, Y) (indented by the +/- sign when it has children) and
// its picture and text are drawn; the children are laid out under it, the width of the picture and 5 pixels in, when
// it is expanded, and hidden when it is not. The height used is the picture's plus 5, and the children's
INT NCGaraTree::DrawItem(NCGDevice* Device, NCGaraTVItem* Item, INT X, INT Y)
{
    if (!Item)
        return 0;
    Item->ShowWindow();
    if (Item->Children.Num() > 0)
        Item->SetPosition(X, Y, Item->Data.Width + 16, Item->Data.Height);
    else
        Item->SetPosition(X, Y, Item->Data.Width, Item->Data.Height);
    Device->DrawTexture(X, Y, Item->Data.IconW, Item->Data.IconH, 0, 0, Item->Data.IconW, Item->Data.IconH,
                        Item->Data.Icon, 0xFF, 1);
    Device->DrawText(Item->Data.IconW + X + 5, Y, 0xFF0000FF, Item->Data.Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     0, 0);
    INT Used = Item->Data.IconH + 5;
    if (Item->Children.Num() > 0)
    {
        if (Item->bExpanded)
        {
            for (INT i = 0; i < Item->Children.Num(); i++)
                Used += DrawItem(Device, Item->Children(i), Item->Data.IconW + X + 5, Used + Y);
        }
        else
        {
            for (INT i = 0; i < Item->Children.Num(); i++)
                Item->Children(i)->HideWindow();
        }
    }
    return Used;
}
NW_HOOK_METHOD(0x10012DC0, NCGaraTree, DrawItem)

// 0x10013920 (vslot 69, name ours) - the entry is made with a copy of the record, its size is the picture's plus the
// text's (and 2); the first one becomes the root, the others go under ParentItem
NCGaraTVItem* NCGaraTree::AddItem(NGaraTVItemData* Data, NCGaraTVItem* ParentItem)
{
    if (!Data)
        return NULL;
    if (!ParentItem)
    {
        if (Root)
            return NULL;
        Root = new NCGaraTVItem(-1);
        Root->Data = *Data;
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Data->Text, 0, 0, 0, 0, 0);
        Root->Data.Width = Data->IconW + Size.cx + 2;
        Root->Data.Height = Data->IconH;
        Root->Data.QuestID = Data->QuestID;
        Root->ID = NID_GARATVITEM;
        Root->AttachEntry(AddChildWindow(Root, NWS_VISIBLE));
        return Root;
    }
    NCGaraTVItem* Item = new NCGaraTVItem(-1);
    Item->Data = *Data;
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Data->Text, 0, 0, 0, 0, 0);
    Item->Data.Width = Data->IconW + Size.cx + 2;
    Item->Data.Height = Data->IconH;
    Item->Data.QuestID = Data->QuestID;
    Item->ID = NID_GARATVITEM;
    CreateChild(Item, NWS_VISIBLE);
    return ParentItem->AddChildItem(Item);
}
NW_HOOK_METHOD(0x10013920, NCGaraTree, AddItem)

NW_CHECK_VTABLE(NCGaraTree, 0x10179940, 73)
