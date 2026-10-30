/*=============================================================================
    NCTrainListBox.cpp - trainer skill list. Stock: 0x100ba9a0 .. 0x100baad5,
    0x100baed0 .. 0x100bb49c, 0x100bc570 .. 0x100bc79f, 0x100bcb40 .. 0x100bcc42; bodies
    shared with NCVIPShopItemListBox at 0x1006bad0 and 0x1006dda0.
=============================================================================*/
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"

// 0x101091f0 - C3: the lines are 8 pixels apart (C2: 4)
NCTrainListBox::NCTrainListBox()
{
    LineGap = 8;
}
NW_HOOK_CTOR(0x101091f0, NCTrainListBox)

// 0x101092b0 - the same, and the text starts one pixel down
NCTrainListBox::NCTrainListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
    LineGap = 8;
    TextY = 1;
}
NW_HOOK_CTOR_ARGS(0x101092b0, NCTrainListBox, (INT A, INT B, INT C), (A, B, C))

// 0x10109250 - the entries' array (0x100dde60), then NCListBox (its scalar deleting destructor, 0x100ddf20, is slot 3 and
// calls this)
NW_HOOK_DTOR(0x10109250, NCTrainListBox)


// stock body shared with NCDebugWnd's (0x100576d0)
INT NCTrainListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10107360 (vslot 37) - C3: two outline textures for the icons
INT NCTrainListBox::OnCreate()
{
    NCListBox::OnCreate();
    BackTex = FindTexture(TEXT("sek.cbui315"));
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
    return 1;
}
NW_HOOK_METHOD(0x10107360, NCTrainListBox, OnCreate)

// 0x100e0980 (vslot 38; the body shared with NCRecipeListBox's: the code is the same for the lists whose rows are
// kept in the array at +0x220)
INT NCTrainListBox::OnDestroy()
{
    for (INT i = 0; i < Entries.Num(); i++)
    {
        if (Entries(i))
            appFree(Entries(i));
    }
    Entries.Empty();
    NCListBox::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100e0980, NCTrainListBox, OnDestroy)

// 0x10107850 (vslot 56) - icon (inside two outlines), name, "Lv <level>" and, from x = 76, "Required SP : <sp>".
// A NULL entry skips only the icon and the name, as in the stock code. The selected row's highlight is
// stretched from half a gap above the row to the width less 18.
INT NCTrainListBox::OnPaint(NCGDevice* Device)
{
    guard(NCTrainListBox::OnPaint);
    // Stock layout: a 16-character buffer followed by the text size. With the English strings
    // the terminator of "Unacquired state" (16 characters) lands in Size.cx, so that text is
    // drawn at TextX + 36 + 0 over "Lv"; kept in one struct so the overflow stays harmless.
    struct
    {
        TCHAR Text[16];
        SIZE Size;
    } T;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Entries.Num(); i++, Line++)
    {
        NTrainEntry* Entry = Entries(i);
        if (i == Selected && HighlightTex)
            Device->DrawStretchedTex(0, (LineGap + 32) * Line - LineGap / 2 + TextY, (INT)(Width - 16.0f - 2.0f),
                                     LineGap + 32, HighlightTex, 1, 0xFF);
        if (Entry)
        {
            if (Entry->Icon)
            {
                Device->DrawTexture(TextX - 1, (LineGap + 32) * Line + TextY - 1, 34, 34, 0, 0, 34, 34, OutlineDownTex,
                                    0xFF, 1);
                Device->DrawTexture(TextX - 1, (LineGap + 32) * Line + TextY - 1, 35, 35, 0, 0, 35, 35, OutlineTex,
                                    0xFF, 1);
                Device->DrawTexture(TextX, (LineGap + 32) * Line + TextY, 32, 32, 0, 0, 32, 32, Entry->Icon, 0xFF, 1);
            }
            if (Entry->Name[0])
                Device->DrawText(TextX + 34, (LineGap + 32) * Line + TextY + 3, 0xFFDCDCDC, Entry->Name, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }

        // "Lv" (SysString 88), then the level or "Unacquired state" (371)
        if (*SysString(88))
            appSprintf(T.Text, TEXT("%s"), *SysString(88));
        else
            T.Text[0] = 0;
        Device->DrawText(TextX + 34, (LineGap + 32) * Line + TextY + 19, 0xFFA3A3A3, T.Text, 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&T.Size, T.Text, 0, 0, 0, 0, 0);
        if (Entry->Level > 0)
            appSprintf(T.Text, TEXT("%d"), Entry->Level);
        else if (*SysString(371))
            appSprintf(T.Text, TEXT("%s"), *SysString(371));
        else
            T.Text[0] = 0;
        Device->DrawText(TextX + T.Size.cx + 36, (LineGap + 32) * Line + TextY + 19, 0xFFB09B79, T.Text, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);

        // "Required SP" (365), " : " and the SP
        if (*SysString(365))
            appSprintf(T.Text, TEXT("%s"), *SysString(365));
        else
            T.Text[0] = 0;
        Device->DrawText(76, (LineGap + 32) * Line + TextY + 19, 0xFFA3A3A3, T.Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0);
        NCGDevice::GetTextExtent(&T.Size, T.Text, 0, 0, 0, 0, 0);
        appSprintf(T.Text, TEXT(" : "));
        Device->DrawText(T.Size.cx + 76, (LineGap + 32) * Line + TextY + 19, 0xFFDCDCDC, T.Text, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (*SysString(365))
            appSprintf(T.Text, TEXT("%s : "), *SysString(365));
        else
            T.Text[0] = 0;
        NCGDevice::GetTextExtent(&T.Size, T.Text, 0, 0, 0, 0, 0);
        appSprintf(T.Text, TEXT("%d"), Entry->SP);
        Device->DrawText(T.Size.cx + 76, (LineGap + 32) * Line + TextY + 19, 0xFFB09B79, T.Text, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10107850, NCTrainListBox, OnPaint)

// 0x100862c0 (vslot 62)
INT NCTrainListBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Entries.Num())
        NotifyParent(Selected);
    return 1;
}
NW_HOOK_METHOD(0x100862c0, NCTrainListBox, OnLButtonDown)

// 0x101073b0 (vslot 69) - NCListBox::HitTest with 32 px rows (and the gap)
INT NCTrainListBox::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > (INT)Width || RY < 0 || RY > (INT)Height)
        return -1;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex; i++, Line++)
    {
        if (i >= NumLines)
            break;
        if (RX < (INT)Width && RY >= (LineGap + 32) * Line + TextY && RY < (Line + 1) * (LineGap + 32) + TextY)
        {
            Selected = i;
            break;
        }
    }
    return Selected;
}
NW_HOOK_METHOD(0x101073b0, NCTrainListBox, HitTest)

// vslot 70: the stock slot is 0x100c3310, the body shared with NCHennaListBox's (hooked there): the rows freed, the
// scroll bar and the view reset
void NCTrainListBox::Clear()
{
    for (INT i = 0; i < Entries.Num(); i++)
    {
        if (Entries(i))
            appFree(Entries(i));
    }
    Entries.Empty();
    ScrollBar->Reset();
    TopIndex = 0;
    NumLines = 0;
    Selected = -1;
}

// 0x100c1110 - the list boxes' shared body (hooked on NCRecipeListBox): the array's count into the scroll bar
__declspec(noinline) void NCTrainListBox::UpdateEntryScrollBar()
{
    NumLines = Entries.Num();
    ScrollBar->SetRange(NumLines, VisibleLines);
    ScrollBar->SetValue(TopIndex);
}

// 0x10108d60 - keeps a copy; the icon comes from Src's texture name (the black texture when
// it is not found). Unlike NCListBox::AddString nothing is dropped when full.
void NCTrainListBox::AddEntry(const NTrainEntry* Src)
{
    NTrainEntry* Entry = new NTrainEntry;
    Entry->Copy(Src);
    if (Src->IconName)
        Entry->Icon = FindTexture(Src->IconName);
    if (!Entry->Icon)
        Entry->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    if (NumLines < MaxLines)
    {
        NumLines++;
        if (bAutoScroll && NumLines > VisibleLines)
            TopIndex++;
    }
    INT i = Entries.Add();
    Entries(i) = Entry;
    UpdateEntryScrollBar();
}
NW_HOOK_METHOD(0x10108d60, NCTrainListBox, AddEntry)

// 0x10107df0
UBOOL NCTrainListBox::GetSelectedEntry(NTrainEntry* Out)
{
    if (Selected < 0 || Selected >= Entries.Num())
        return 0;
    NTrainEntry* Entry = Entries(Selected);
    if (!Entry)
        return 0;
    Out->Copy(Entry);
    return 1;
}
NW_HOOK_METHOD(0x10107df0, NCTrainListBox, GetSelectedEntry)

NW_CHECK_VTABLE(NCTrainListBox, 0x101a0d10, 76)
