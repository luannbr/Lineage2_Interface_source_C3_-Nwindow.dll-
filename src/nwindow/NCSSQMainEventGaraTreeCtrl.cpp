/*=============================================================================
    NCSSQMainEventGaraTreeCtrl.cpp - the scrolling window of one NCSSQMainEventGaraTree: NCGaraTreeCtrl code with its own
    tree. C3 stock: 0x100EB640 / 0x100EB6A0 (constructor, destructor), 0x100EB6F0 (OnCreate),
    0x100EAFC0 (paint) and the bodies shared with NCQuestGaraTreeCtrl.
=============================================================================*/
#include "nwindow/NCSSQMainEventGaraTreeCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"

// 0x100EB640 - the base, with a bigger Unknown0C8 (16)
NCSSQMainEventGaraTreeCtrl::NCSSQMainEventGaraTreeCtrl()
{
    Unknown0C8 = 16;
    RootItem = NULL;
}
NW_HOOK_CTOR(0x100EB640, NCSSQMainEventGaraTreeCtrl)

// 0x100EB6A0
NW_HOOK_DTOR(0x100EB6A0, NCSSQMainEventGaraTreeCtrl)

// 0x100EB6F0 (vslot 37) - NCScrollWnd, then the tree (id 0xA00), the window width less 16 pixels
INT NCSSQMainEventGaraTreeCtrl::OnCreate()
{
    NCScrollWnd::OnCreate();
    SSQTree = new NCSSQMainEventGaraTree();
    SSQTree->ID = 0xA00;
    SSQTree->SetPosition(0, 0, (INT)(Width - 16.0f), (INT)Height);
    CreateChild(SSQTree, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100EB6F0, NCSSQMainEventGaraTreeCtrl, OnCreate)

// 0x100EAFC0 (vslot 56) - the scroll range follows the tree height, and the entries are put where they show
INT NCSSQMainEventGaraTreeCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCSSQMainEventGaraTreeCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    SetContentHeightEx(SSQTree->ContentHeight);
    if (SSQTree->Root)
        ArrangeItem(SSQTree->Root);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100EAFC0, NCSSQMainEventGaraTreeCtrl, OnPaint)

// the bodies the linker shared with NCQuestGaraTreeCtrl (they work on the tree at +0x114): stock stubs
#if NW_HOOK_MODE
NW_NAKED INT NCSSQMainEventGaraTreeCtrl::OnDestroy() NW_STOCK(0x100EAF80)
NW_NAKED INT NCSSQMainEventGaraTreeCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam) NW_STOCK(0x100E3ED0)
NW_NAKED INT NCSSQMainEventGaraTreeCtrl::ScrollUp() NW_STOCK(0x100ECAD0)
NW_NAKED INT NCSSQMainEventGaraTreeCtrl::ScrollDown() NW_STOCK(0x100DB2B0)
NW_NAKED INT NCSSQMainEventGaraTreeCtrl::OnScrollBar(INT Value) NW_STOCK(0x100ECB90)
NW_NAKED void NCSSQMainEventGaraTreeCtrl::ArrangeItem(NCGaraTVItem* Item) NW_STOCK(0x100ECA00)
#else
NW_SLOT_IS("?OnDestroy@NCSSQMainEventGaraTreeCtrl@@UAEHXZ", "?OnDestroy@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQMainEventGaraTreeCtrl::OnDestroy = NCQuestGaraTreeCtrl::OnDestroy (0x100eaf80)
NW_SLOT_IS("?OnMouseWheel@NCSSQMainEventGaraTreeCtrl@@UAEHIJ@Z", "?OnMouseWheel@NCQuestGaraTreeCtrl@@UAEHIJ@Z")   // NCSSQMainEventGaraTreeCtrl::OnMouseWheel = NCQuestGaraTreeCtrl::OnMouseWheel (0x100e3ed0)
NW_SLOT_IS("?ScrollUp@NCSSQMainEventGaraTreeCtrl@@UAEHXZ", "?ScrollUp@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQMainEventGaraTreeCtrl::ScrollUp = NCQuestGaraTreeCtrl::ScrollUp (0x100ecad0)
NW_SLOT_IS("?ScrollDown@NCSSQMainEventGaraTreeCtrl@@UAEHXZ", "?ScrollDown@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQMainEventGaraTreeCtrl::ScrollDown = NCQuestGaraTreeCtrl::ScrollDown (0x100db2b0)
NW_SLOT_IS("?OnScrollBar@NCSSQMainEventGaraTreeCtrl@@UAEHH@Z", "?OnScrollBar@NCQuestGaraTreeCtrl@@UAEHH@Z")   // NCSSQMainEventGaraTreeCtrl::OnScrollBar = NCQuestGaraTreeCtrl::OnScrollBar (0x100ecb90)
NW_SLOT_IS("?ArrangeItem@NCSSQMainEventGaraTreeCtrl@@UAEXPAVNCGaraTVItem@@@Z", "?ArrangeItem@NCQuestGaraTreeCtrl@@UAEXPAVNCGaraTVItem@@@Z")   // NCSSQMainEventGaraTreeCtrl::ArrangeItem = NCQuestGaraTreeCtrl::ArrangeItem (0x100eca00)
#endif

// 0x100EB0B0 (vslot 72, name ours) - the tree is emptied and put back at the origin of the window, the scroll
// starts over (shared with NCSSQSealStatusGaraTreeCtrl)
void NCSSQMainEventGaraTreeCtrl::ResetScroll()
{
    SSQTree->Clear();
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0.0f;
    Local.H = 0.0f;
    SSQTree->GetLocalRect(&Local);
    SSQTree->MoveWindow(-Local.X, -Local.Y);
    SSQTree->ScrollOffset = 0;
    ScrollY = 0;
    ScrollX = 0;
    DeltaY = 0;
    DeltaX = 0;
    MinScrollY = 0;
    ScrollBar->ThumbPos = 0;
    RootItem = NULL;
}
NW_HOOK_METHOD(0x100EB0B0, NCSSQMainEventGaraTreeCtrl, ResetScroll)

// 0x100EB1D0 (name ours) - the group (Record->Group) is the child of the root with that id (QuestID) or a new heading:
// its picture is the plus sign (the minus sign open), 14 x 14, kind 1. The event goes under it as a kind 2 entry (the
// down / up arrows) with the numbers of the record, and the details (kind 3: the two numbers and the nine texts of each
// list that are not empty) under that. The event's entry is the result
NCGaraTVItem* NCSSQMainEventGaraTreeCtrl::AddEvent(const NSSQEventRecord* Record)
{
    NCGaraTVItem* Group = NULL;
    UBOOL bNew = 1;
    if (RootItem)
    {
        INT Num = RootItem->Children.Num();
        for (INT i = 0; i < Num; i++)
        {
            NCGaraTVItem* Child = RootItem->Children(i);
            if (Child->Data.QuestID == Record->Group)
            {
                bNew = 0;
                Group = Child;
            }
        }
    }
    if (bNew)
    {
        NGaraTVItemData Heading;
        Heading.Kind = 1;
        Heading.IconW = 14;
        Heading.IconH = 14;
        Heading.Icon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
        Heading.ExpandedIcon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
        Heading.QuestID = Record->Group;
        Group = SSQTree->AddItem(&Heading, RootItem);
        if (Group)
            Group->ParentItem = RootItem;
    }
    NGaraTVItemData Event;
    Event.Kind = 2;
    Event.IconW = 14;
    Event.IconH = 14;
    Event.Icon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndDownBtn"));
    Event.ExpandedIcon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndUpBtn"));
    Event.LevelMin = Record->Need;
    Event.IconSet = Record->Have;
    Event.QuestID = Record->Event;
    Event.LevelMax = Record->Value;
    Event.bTextCount = (Record->Type != 1);
    NCGaraTVItem* EventItem = SSQTree->AddItem(&Event, Group);
    NGaraTVItemData Details;
    Details.Kind = 3;
    Details.IconSet = Record->Have;
    Details.LevelMin = Record->Need;
    for (INT i = 0; i < 9; i++)
    {
        if (Record->NamesA[i].Len() > 0)
            Details.Strings1DD8[i] = Record->NamesA[i];
        if (Record->NamesB[i].Len() > 0)
            Details.Strings1E44[i] = Record->NamesB[i];
    }
    SSQTree->AddItem(&Details, EventItem);
    return EventItem;
}
NW_HOOK_METHOD(0x100EB1D0, NCSSQMainEventGaraTreeCtrl, AddEvent)

NW_CHECK_VTABLE(NCSSQMainEventGaraTreeCtrl, 0x1019BCB0, 74)
