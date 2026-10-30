/*=============================================================================
    NCSSQSealStatusGaraTreeCtrl.cpp - the scrolling window of one NCSSQSealStatusGaraTree: NCGaraTreeCtrl code with its own
    tree. C3 stock: 0x100ED090 / 0x100ED0F0 (constructor, destructor), 0x100ED140 (OnCreate),
    0x100EC910 (paint) and the bodies shared with NCQuestGaraTreeCtrl.
=============================================================================*/
#include "nwindow/NCSSQSealStatusGaraTreeCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"

// 0x100ED090 - the base, with a bigger Unknown0C8 (16)
NCSSQSealStatusGaraTreeCtrl::NCSSQSealStatusGaraTreeCtrl()
{
    Unknown0C8 = 16;
    RootItem = NULL;
}
NW_HOOK_CTOR(0x100ED090, NCSSQSealStatusGaraTreeCtrl)

// 0x100ED0F0
NW_HOOK_DTOR(0x100ED0F0, NCSSQSealStatusGaraTreeCtrl)

// 0x100ED140 (vslot 37) - NCScrollWnd, then the tree (id 0xFFF0A0F1), the window width less 16 pixels
INT NCSSQSealStatusGaraTreeCtrl::OnCreate()
{
    NCScrollWnd::OnCreate();
    SSQTree = new NCSSQSealStatusGaraTree();
    SSQTree->ID = 0xFFF0A0F1;
    SSQTree->SetPosition(0, 0, (INT)(Width - 16.0f), (INT)Height);
    CreateChild(SSQTree, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100ED140, NCSSQSealStatusGaraTreeCtrl, OnCreate)

// 0x100EC910 (vslot 56) - the scroll range follows the tree height, and the entries are put where they show
INT NCSSQSealStatusGaraTreeCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCSSQSealStatusGaraTreeCtrl::OnPaint);
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
NW_HOOK_METHOD(0x100EC910, NCSSQSealStatusGaraTreeCtrl, OnPaint)

// the bodies the linker shared with NCQuestGaraTreeCtrl (they work on the tree at +0x114): stock stubs
#if NW_HOOK_MODE
NW_NAKED INT NCSSQSealStatusGaraTreeCtrl::OnDestroy() NW_STOCK(0x100EAF80)
NW_NAKED INT NCSSQSealStatusGaraTreeCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam) NW_STOCK(0x100E3ED0)
NW_NAKED INT NCSSQSealStatusGaraTreeCtrl::ScrollUp() NW_STOCK(0x100ECAD0)
NW_NAKED INT NCSSQSealStatusGaraTreeCtrl::ScrollDown() NW_STOCK(0x100DB2B0)
NW_NAKED INT NCSSQSealStatusGaraTreeCtrl::OnScrollBar(INT Value) NW_STOCK(0x100ECB90)
NW_NAKED void NCSSQSealStatusGaraTreeCtrl::ArrangeItem(NCGaraTVItem* Item) NW_STOCK(0x100ECA00)
#else
NW_SLOT_IS("?OnDestroy@NCSSQSealStatusGaraTreeCtrl@@UAEHXZ", "?OnDestroy@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQSealStatusGaraTreeCtrl::OnDestroy = NCQuestGaraTreeCtrl::OnDestroy (0x100eaf80)
NW_SLOT_IS("?OnMouseWheel@NCSSQSealStatusGaraTreeCtrl@@UAEHIJ@Z", "?OnMouseWheel@NCQuestGaraTreeCtrl@@UAEHIJ@Z")   // NCSSQSealStatusGaraTreeCtrl::OnMouseWheel = NCQuestGaraTreeCtrl::OnMouseWheel (0x100e3ed0)
NW_SLOT_IS("?ScrollUp@NCSSQSealStatusGaraTreeCtrl@@UAEHXZ", "?ScrollUp@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQSealStatusGaraTreeCtrl::ScrollUp = NCQuestGaraTreeCtrl::ScrollUp (0x100ecad0)
NW_SLOT_IS("?ScrollDown@NCSSQSealStatusGaraTreeCtrl@@UAEHXZ", "?ScrollDown@NCQuestGaraTreeCtrl@@UAEHXZ")   // NCSSQSealStatusGaraTreeCtrl::ScrollDown = NCQuestGaraTreeCtrl::ScrollDown (0x100db2b0)
NW_SLOT_IS("?OnScrollBar@NCSSQSealStatusGaraTreeCtrl@@UAEHH@Z", "?OnScrollBar@NCQuestGaraTreeCtrl@@UAEHH@Z")   // NCSSQSealStatusGaraTreeCtrl::OnScrollBar = NCQuestGaraTreeCtrl::OnScrollBar (0x100ecb90)
NW_SLOT_IS("?ArrangeItem@NCSSQSealStatusGaraTreeCtrl@@UAEXPAVNCGaraTVItem@@@Z", "?ArrangeItem@NCQuestGaraTreeCtrl@@UAEXPAVNCGaraTVItem@@@Z")   // NCSSQSealStatusGaraTreeCtrl::ArrangeItem = NCQuestGaraTreeCtrl::ArrangeItem (0x100eca00)
#endif

// 0x100EB0B0 (vslot 72) - NCSSQMainEventGaraTreeCtrl body, shared
#if NW_HOOK_MODE
NW_NAKED void NCSSQSealStatusGaraTreeCtrl::ResetScroll() NW_STOCK(0x100EB0B0)
#else
NW_SLOT_IS("?ResetScroll@NCSSQSealStatusGaraTreeCtrl@@UAEXXZ", "?ResetScroll@NCSSQMainEventGaraTreeCtrl@@UAEXXZ")   // NCSSQSealStatusGaraTreeCtrl::ResetScroll = NCSSQMainEventGaraTreeCtrl::ResetScroll (0x100eb0b0)
#endif

// 0x100ECCA0 (name ours) - the seal's heading (kind 1, the plus / minus sign picture, 14 x 14, the record's numbers
// in the places the rows read them) goes under the root, and the details entry (kind 2, the same seal) under it
NCGaraTVItem* NCSSQSealStatusGaraTreeCtrl::AddEntry(const NSSQSealRecord* Record)
{
    NGaraTVItemData Heading;
    Heading.Kind = 1;
    Heading.IconW = 14;
    Heading.IconH = 14;
    Heading.Icon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
    Heading.ExpandedIcon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
    Heading.LevelMin = Record->Low;
    Heading.IconSet = Record->Owner;
    Heading.Unknown18 = Record->Total2;
    Heading.QuestID = Record->Seal;
    Heading.Unknown14 = Record->Total1;
    Heading.LevelMax = Record->High;
    NCGaraTVItem* Group = SSQTree->AddItem(&Heading, RootItem);
    NGaraTVItemData Details;
    Details.QuestID = Record->Seal;
    Details.Kind = 2;
    return SSQTree->AddItem(&Details, Group);
}
NW_HOOK_METHOD(0x100ECCA0, NCSSQSealStatusGaraTreeCtrl, AddEntry)

NW_CHECK_VTABLE(NCSSQSealStatusGaraTreeCtrl, 0x1019C2A0, 74)
