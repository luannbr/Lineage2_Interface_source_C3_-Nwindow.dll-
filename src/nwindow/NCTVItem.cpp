/*=============================================================================
    NCTVItem.cpp - the entry of a tree view. C3 stock: 0x1002E2D0 / 0x1002E370 (the record's constructor and copy),
    0x1002E500 .. 0x1002E56F (Expand, Collapse), 0x1002EEE0 (AddChild), 0x1002F370 (the children's array
    destructor), 0x1002F400 .. 0x1002F4EF (constructor, destructor) and 0x1002F550 (DeleteChildItem).
=============================================================================*/
#include "nwindow/NCTVItem.h"

/*-----------------------------------------------------------------------------
    NTVItemData.
-----------------------------------------------------------------------------*/

// 0x1002E2D0 - the boxes and the offsets are 0, the texts empty, the first number -1 and the text is centered on the
// second picture; Unknown2848 is left as allocated
__declspec(noinline) NTVItemData::NTVItemData()
{
    TextBox.Y = 0;
    TextBox.X = 0;
    TextBox.W = 0.0f;
    TextBox.H = 0.0f;
    Icon2Box.Y = 0;
    Icon2Box.X = 0;
    Icon2Box.W = 0.0f;
    Icon2Box.H = 0.0f;
    IconBox.Y = 0;
    IconBox.X = 0;
    IconBox.W = 0.0f;
    IconBox.H = 0.0f;
    Unknown00 = -1;
    Unknown04 = 0;
    Unknown08 = 0;
    Unknown0C = 0;
    Text[0] = 0;
    Text2[0] = 0;
    Text3[0] = 0;
    Tag[0] = 0;
    Icon2Tex = NULL;
    ExpandedTex = NULL;
    CollapsedTex = NULL;
    TextOffsetX = 0;
    CollapsedOffsetY = 0;
    bCenterOnIcon2 = 1;
}
NW_HOOK_CTOR(0x1002E2D0, NTVItemData)

// 0x1002E370 - field by field, the texts in whole
__declspec(noinline) NTVItemData& NTVItemData::operator=(const NTVItemData& Other)
{
    Unknown00 = Other.Unknown00;
    Unknown04 = Other.Unknown04;
    Unknown08 = Other.Unknown08;
    Unknown0C = Other.Unknown0C;
    for (INT i = 0; i < 1024; i++)
        Text[i] = Other.Text[i];
    for (INT i = 0; i < 2048; i++)
        Text2[i] = Other.Text2[i];
    for (INT i = 0; i < 1024; i++)
        Text3[i] = Other.Text3[i];
    for (INT i = 0; i < 1024; i++)
        Tag[i] = Other.Tag[i];
    TextBox.X = Other.TextBox.X;
    TextBox.Y = Other.TextBox.Y;
    TextBox.W = Other.TextBox.W;
    TextBox.H = Other.TextBox.H;
    Icon2Box.X = Other.Icon2Box.X;
    Icon2Box.Y = Other.Icon2Box.Y;
    Icon2Box.W = Other.Icon2Box.W;
    Icon2Box.H = Other.Icon2Box.H;
    IconBox.X = Other.IconBox.X;
    IconBox.Y = Other.IconBox.Y;
    IconBox.W = Other.IconBox.W;
    IconBox.H = Other.IconBox.H;
    TextOffsetX = Other.TextOffsetX;
    CollapsedOffsetY = Other.CollapsedOffsetY;
    Unknown2848 = Other.Unknown2848;
    bCenterOnIcon2 = Other.bCenterOnIcon2;
    Icon2Tex = Other.Icon2Tex;
    ExpandedTex = Other.ExpandedTex;
    CollapsedTex = Other.CollapsedTex;
    return *this;
}
NW_HOOK_METHOD(0x1002E370, NTVItemData, operator=)

/*-----------------------------------------------------------------------------
    NCTVItem.
-----------------------------------------------------------------------------*/

// 0x1002F400 - the record's constructor, the children's array empty, the entry closed and visible
NCTVItem::NCTVItem(INT InLevel)
{
    Parent = NULL;
    bExpanded = 0;
    Unknown2874 = 0;
    bSelected = 0;
    Level = InLevel;
    bVisible = 1;
}
NW_HOOK_CTOR_ARGS(0x1002F400, NCTVItem, (INT A), (A))

// 0x1002F450 - the children are deleted (each one's own children first), then the array is emptied; its destructor
// (0x1002F370) runs last
NCTVItem::~NCTVItem()
{
    Parent = NULL;
    for (INT i = 0; i < Children.Num(); i++)
    {
        NCTVItem* Child = Children(i);
        if (Child)
            delete Child;
    }
    Children.Empty();
}
NW_HOOK_DTOR(0x1002F450, NCTVItem)

// 0x1002F370 - the children's array: its elements removed with the template's checks, then FArray's destructor (the
// messenger's list code runs it on its own copies)
__declspec(noinline) NTVItemArray::~NTVItemArray()
{
}
NW_HOOK_DTOR(0x1002F370, NTVItemArray)

// 0x1002EEE0 (name ours) - at the end, or inserted at Index
NCTVItem* NCTVItem::AddChild(NCTVItem* Child, INT Index)
{
    if (!Child)
        return NULL;
    if (Index == -1)
    {
        INT Slot = Children.Add();
        Children(Slot) = Child;
    }
    else
    {
        Children.Insert(Index);
        Children(Index) = Child;
    }
    return Child;
}
NW_HOOK_METHOD(0x1002EEE0, NCTVItem, AddChild)

// 0x1002E500 (name ours) - open, and the children can be hit
void NCTVItem::Expand()
{
    bExpanded = 1;
    for (INT i = 0; i < Children.Num(); i++)
    {
        NCTVItem* Child = Children(i);
        if (Child)
            Child->bVisible = 1;
    }
}
NW_HOOK_METHOD(0x1002E500, NCTVItem, Expand)

// 0x1002E550 (name ours) - closed, and the children cannot
void NCTVItem::Collapse()
{
    bExpanded = 0;
    for (INT i = 0; i < Children.Num(); i++)
    {
        NCTVItem* Child = Children(i);
        if (Child)
            Child->bVisible = 0;
    }
}
NW_HOOK_METHOD(0x1002E550, NCTVItem, Collapse)

// 0x1002F550 - taken out of the children (the pointer array's RemoveItem, 0x100318E0: the stock keeps one instance
// for all of them; the hook build calls it there, the standalone the template's, the same code), then its own children,
// it and its memory go
void NCTVItem::DeleteChildItem(NCTVItem* Child)
{
    guard(NCTVItem::DeleteChildItem);
#if NW_HOOK_MODE
    ::nw::StockCallAt<INT>(0x100318E0, &Children, &Child);
#else
    Children.RemoveItem(Child);
#endif
    if (Child)
    {
        Child->~NCTVItem();
        appFree(Child);
    }
    unguard;
}
NW_HOOK_METHOD(0x1002F550, NCTVItem, DeleteChildItem)

// 0x1002F4F0 (name ours; the messenger tree's list code calls it) - every child goes (its destructor, then the memory
// freed through the allocator) and the array is emptied to no places
void NCTVItem::DeleteAllChildren()
{
    for (INT i = 0; i < Children.Num(); i++)
    {
        NCTVItem* Child = Children(i);
        if (Child)
        {
            Child->~NCTVItem();
            appFree(Child);
        }
    }
    Children.Empty();
}
NW_HOOK_METHOD(0x1002F4F0, NCTVItem, DeleteAllChildren)
