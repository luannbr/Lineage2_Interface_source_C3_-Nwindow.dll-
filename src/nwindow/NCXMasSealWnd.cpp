/*=============================================================================
    NCXMasSealWnd.cpp - the Christmas seal window (C2 only). Stock (C3): 0x1010b020 .. 0x1010b2c0.
=============================================================================*/
#include "nwindow/NCXMasSealWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NGameData.h"

enum { NID_XMASSEAL_FRAME = 0xB000FA };   // the title bar's id (name ours)

// 0x1010b270 - the first and the last member only; the back texture comes with OnCreate
NCXMasSealWnd::NCXMasSealWnd()
{
    Unknown98 = 0;
    SealTex = NULL;
}
NW_HOOK_CTOR(0x1010b270, NCXMasSealWnd)

// 0x1010b2c0
NW_HOOK_DTOR(0x1010b2c0, NCXMasSealWnd)

// 0x1010b170 (vslot 37) - a 334 x 20 title bar of its own (C3: NCFrameCtrl(1, 0, 0); FrameID is not set)
INT NCXMasSealWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.MinimapWnd.MinimapBack"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_XMASSEAL_FRAME;
    FrameCtrl->SetPosition(0, 0, 334, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x1010b170, NCXMasSealWnd, OnCreate)

// 0x1010b020 (vslot 38) - guarded; nothing of its own to free
INT NCXMasSealWnd::OnDestroy()
{
    guard(NCXMasSealWnd::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010b020, NCXMasSealWnd, OnDestroy)

// 0x1010b0a0 (vslot 56) - the panel below the title bar, the seal inside it
INT NCXMasSealWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 334, 333, 0, 0, 334, 333, BackTex, 0xFF, 1);
    if (SealTex)
        Device->DrawTexture(3, 21, 328, 329, 0, 0, 328, 329, SealTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1010b0a0, NCXMasSealWnd, OnPaint)

// 0x1010b230 (name ours) - the item the console shows as the seal (Unknown98 keeps its id): the picture is the texture of
// the name at +0x7C of its item record (the stock reads the record without a check)
void NCXMasSealWnd::SetItem(INT ItemID)
{
    Unknown98 = ItemID;
    NViewItemData* Data = GL2GameData.ItemTable->FindRef(Unknown98);
    SealTex = FindTexture(*nw::Field<L2FName>(Data, 0x7C));
}
NW_HOOK_METHOD(0x1010b230, NCXMasSealWnd, SetItem)

NW_CHECK_VTABLE(NCXMasSealWnd, 0x101a2510, 68)
