/*=============================================================================
    NCComboBox.cpp - drop-down list. Stock (C3): 0x10004ce0 .. 0x100059f4.
=============================================================================*/
#include "nwindow/NCComboBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// 0x10005b20 - NControl's constructor is inlined; the rows are 17 high
NCComboBox::NCComboBox()
{
    TextY = 0;
    TextX = 5;
    SelectTex = NULL;
    DropButton = NULL;
    bDropped = 0;
    Selected = -1;
    Hover = -1;
    RowStep = 17;
}
NW_HOOK_CTOR(0x10005b20, NCComboBox)


// 0x10004ce0 (vslot 25) - the drop button reports here; C2: no enabled test
INT NCComboBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_COMBO_DROP)
        return DropDown() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10004ce0, NCComboBox, WindowProc)

// 0x10004e60 (vslot 30) - the list closes (the window goes back to its closed size), then the base's
void NCComboBox::KillFocus()
{
    if (bDropped)
    {
        SetWindowSize((FLOAT)ClosedW, (FLOAT)ClosedH);
        bDropped = 0;
    }
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x10004e60, NCComboBox, KillFocus)

// 0x10005570 (vslot 37) - the box textures (the CH3 set), the row height from "A", the closed size from the local
// rectangle, the drop button 15 x 15 near the right edge
INT NCComboBox::OnCreate()
{
    NControl::OnCreate();
    LeftTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox1"));
    MidTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox2"));
    RightTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox3"));
    LeftDisabledTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox1_disable"));
    MidDisabledTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox2_disable"));
    RightDisabledTex = FindTexture(TEXT("L2UI_CH3.Etc.textbox3_disable"));
    SelectTex = FindTexture(TEXT("L2UI_CH3.ListCtrl.TextSelect"));
    SelectTex2 = FindTexture(TEXT("L2UI_CH3.ListCtrl.TextSelect2"));
    NCGDevice::GetTextExtent(&RowSize, TEXT("A"), 0, 0, 0, 0, 0, L2FONT_Normal);
    NRectF Rect;
    Rect.X = 0;
    Rect.Y = 0;
    Rect.W = 0.0f;
    Rect.H = 0.0f;
    GetLocalRect(&Rect);
    ClosedW = (INT)Rect.W;
    ClosedH = (INT)Rect.H;
    DropButton = new NCButton(TEXT("L2UI_CH3.Button.downbutton"), TEXT("L2UI_CH3.Button.downbutton_down"), 0);
    DropButton->ID = NID_COMBO_DROP;
    DropButton->SetPosition((INT)(Rect.W - 15.0f - 1.0f), (INT)(Height * 0.5f - 7.0f), 15, 15);
    CreateChild(DropButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10005570, NCComboBox, OnCreate)

// 0x10005740 (vslot 38) - the drop button, then the items
INT NCComboBox::OnDestroy()
{
    guard(NCComboBox::OnDestroy);
    if (DropButton)
    {
        RemoveChildWindow(DropButton->Entry);
        delete DropButton;
        DropButton = NULL;
    }
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Item = Items(i);
        if (Item)
            delete Item;
    }
    Items.Empty();
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10005740, NCComboBox, OnDestroy)

// 0x10004f50 (vslot 56) - the closed box (three textures, the selected text), and, while dropped and focused, a row
// of the three textures per item with the item's text and a highlight bar under the row the mouse is over. The
// device is clipped before the origin is set, as in the stock code
INT NCComboBox::OnPaint(NCGDevice* Device)
{
    guard(NCComboBox::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    INT TextYOfs = RowStep / 2 - RowSize.cy / 2;
    UTexture* Left;
    UTexture* Mid;
    UTexture* Right;
    DWORD Color;
    if (IsEnabled())
    {
        Left = LeftTex;
        Mid = MidTex;
        Right = RightTex;
        Color = 0xFFDCDCDC;
    }
    else
    {
        Left = LeftDisabledTex;
        Mid = MidDisabledTex;
        Right = RightDisabledTex;
        Color = 0xFF7E7E7E;
    }
    Device->DrawTexture(0, (INT)(Height * 0.5f - (FLOAT)(RowStep / 2)), 8, RowStep, 0, 0, 8, 17, Left, 0xFF, 1);
    Device->DrawTexture(8, (INT)(Height * 0.5f - (FLOAT)(RowStep / 2)), (INT)(Width - 16.0f), RowStep, 0, 0, 8, 17,
                        Mid, 0xFF, 1);
    Device->DrawTexture((INT)(Width - 8.0f), (INT)(Height * 0.5f - (FLOAT)(RowStep / 2)), 8, RowStep, 0, 0, 8, 17,
                        Right, 0xFF, 1);
    if (Items.Num() > Selected && Selected >= 0)
    {
        const TCHAR* Text = Items(Selected)->GetAt(0);
        Device->DrawText(TextX, ClosedH / 2 + TextY - RowStep / 2 + TextYOfs, Color, Text, 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0, 0);
    }
    if (bDropped && IsFocused())
    {
        for (INT i = 0; i < Items.Num(); i++)
        {
            Device->DrawTexture(0, RowStep * i + ClosedH, 8, RowStep, 0, 0, 8, 17, LeftTex, 0xFF, 1);
            Device->DrawTexture(8, RowStep * i + ClosedH, (INT)(Width - 16.0f), RowStep, 0, 0, 8, 17, MidTex, 0xFF, 1);
            Device->DrawTexture((INT)(Width - 8.0f), RowStep * i + ClosedH, 8, RowStep, 0, 0, 8, 17, RightTex, 0xFF,
                                1);
        }
        for (INT i = 0; i < Items.Num(); i++)
        {
            if (Hover == i)
            {
                Device->DrawStretchedTex(1, RowStep * i + ClosedH, (INT)(Width - 32.0f), 16, 0, 0, 16, 13, SelectTex,
                                         0xFF);
                Device->DrawStretchedTex((INT)(Width - 32.0f + 1.0f), RowStep * i + ClosedH, 32, 16, 0, 0, 32, 13,
                                         SelectTex2, 0xFF);
            }
            const TCHAR* Text = Items(i)->GetAt(0);
            Device->DrawText(TextX, RowStep * i + ClosedH + TextYOfs, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0, 0);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10004f50, NCComboBox, OnPaint)

// 0x10004d40 (vslot 61) - the item under the mouse in the open list; the coordinates are 16-bit differences, as in
// the stock code. -1 outside the list part of the box
INT NCComboBox::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    WORD RX = (WORD)(LOWORD(lParam) - (WORD)ScreenX);
    WORD RY = (WORD)(HIWORD(lParam) - (WORD)ScreenY);
    if (RX != 0 && (FLOAT)RX < Width && (INT)RY > ClosedH && (FLOAT)RY < Height)
    {
        if (RowSize.cy == 0)
            Hover = 0;
        else
            Hover = ((INT)RY - ClosedH) / RowStep;
        if (Hover >= Items.Num())
            Hover = Items.Num() - 1;
        return 1;
    }
    Hover = -1;
    return 1;
}
NW_HOOK_METHOD(0x10004d40, NCComboBox, OnMouseMove)

// 0x10004df0 (vslot 62) - open it, or pick the row under the mouse
INT NCComboBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsEnabled())
    {
        if (!bDropped)
        {
            DropDown();
            return 1;
        }
        if (Hover >= 0)
        {
            Selected = Hover;
            NotifyParent(Hover);
            bDropped = 0;
            DropDown();
        }
        KillFocus();
    }
    return 1;
}
NW_HOOK_METHOD(0x10004df0, NCComboBox, OnLButtonDown)

// 0x10005500 (vslot 69) - opens the list (when enabled) and sizes the window to it
INT NCComboBox::DropDown()
{
    if (IsEnabled())
    {
        if (!bDropped)
            bDropped = 1;
        UpdateSize();
    }
    return 1;
}
NW_HOOK_METHOD(0x10005500, NCComboBox, DropDown)

// 0x10004ea0
void NCComboBox::SetSelected(INT Index)
{
    Selected = Index;
}
NW_HOOK_METHOD(0x10004ea0, NCComboBox, SetSelected)

// 0x10004eb0 - open: tall enough for the items, then the focus (vslot 29); closed: back to the closed size
void NCComboBox::UpdateSize()
{
    if (bDropped)
    {
        SetWindowSize((FLOAT)ClosedW, (FLOAT)(RowStep * Items.Num() + ClosedH));
        SetFocus();
        return;
    }
    SetWindowSize((FLOAT)ClosedW, (FLOAT)ClosedH);
}
NW_HOOK_METHOD(0x10004eb0, NCComboBox, UpdateSize)

// 0x100054a0 (ret 4) - the item's text, NULL out of range
const TCHAR* NCComboBox::GetItemText(INT Index)
{
    if (Index >= 0 && Index < Items.Num())
        return Items(Index)->GetAt(0);
    return NULL;
}
NW_HOOK_METHOD(0x100054a0, NCComboBox, GetItemText)

// 0x10005840 - the items go and the selection with them
void NCComboBox::ClearItems()
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Item = Items(i);
        if (Item)
            delete Item;
    }
    Items.Empty();
    Selected = -1;
    Hover = -1;
}
NW_HOOK_METHOD(0x10005840, NCComboBox, ClearItems)

// 0x100058b0 (ret 4) - a SysString as an item (nothing when it is empty)
void NCComboBox::AddItem(INT SysStringID)
{
    if (*SysString(SysStringID))
    {
        NString* Item = new NString(*SysString(SysStringID));
        Items.AddItem(Item);
    }
}
NW_HOOK_METHOD_SIG(0x100058b0, NCComboBox, AddItem, void (NCComboBox::*)(INT))

// 0x10005970 (ret 4) - a text as an item (nothing when it is empty)
void NCComboBox::AddItem(const TCHAR* Text)
{
    if (Text && *Text)
    {
        NString* Item = new NString(Text);
        Items.AddItem(Item);
    }
}
NW_HOOK_METHOD_SIG(0x10005970, NCComboBox, AddItem, void (NCComboBox::*)(const TCHAR*))

// 0x10005bd0 - the items (the array of NString pointers) go, then NControl
NW_HOOK_DTOR(0x10005bd0, NCComboBox)

NW_CHECK_VTABLE(NCComboBox, 0x10177808, 70)
