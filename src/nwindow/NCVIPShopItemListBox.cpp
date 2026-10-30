/*=============================================================================
    NCVIPShopItemListBox.cpp - VIP shop item list. C3 stock: 0x10093a40 (OnCreate), 0x10093a70 (the item's name line,
    shared with the item page's needed items), 0x100957b0 (paint), 0x10096310 (OnDestroy), 0x10096460 (AddEntry),
    0x10097240 .. 0x10097350 (constructors, destructor); the other bodies are the ones the other list boxes use (hooked
    with them: 0x100862c0, 0x100963e0, 0x10086300, 0x101241c0).
=============================================================================*/
#include "nwindow/NCVIPShopItemListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10097240 - C3: the lines are 4 pixels apart
NCVIPShopItemListBox::NCVIPShopItemListBox()
{
    LineGap = 4;
}
NW_HOOK_CTOR(0x10097240, NCVIPShopItemListBox)

// 0x10097300 - the same
NCVIPShopItemListBox::NCVIPShopItemListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
    LineGap = 4;
}
NW_HOOK_CTOR_ARGS(0x10097300, NCVIPShopItemListBox, (INT A, INT B, INT C), (A, B, C))

// 0x100972a0
NW_HOOK_DTOR(0x100972a0, NCVIPShopItemListBox)

// stock body shared with NCChatListBox's (0x101241c0): the base's answer as a plain 0 / 1
INT NCVIPShopItemListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10093a40 (vslot 37) - C3: the frame texture of the icons
INT NCVIPShopItemListBox::OnCreate()
{
    NCListBox::OnCreate();
    IconBoxTex = FindTexture(TEXT("L2ui_ch3.etc.iconbox"));
    return 1;
}
NW_HOOK_METHOD(0x10093a40, NCVIPShopItemListBox, OnCreate)

// 0x10096310 (vslot 38) - the rows freed, then the array and the base (the stock code walks the array a second time
// without doing anything)
INT NCVIPShopItemListBox::OnDestroy()
{
    guard(NCVIPShopItemListBox::OnDestroy);
    for (INT i = 0; i < Entries.Num(); i++)
    {
        if (Entries(i))
            appFree(Entries(i));
    }
    Entries.Empty();
    NCListBox::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10096310, NCVIPShopItemListBox, OnDestroy)

// 0x10093a70 - an item's name line at (X, Y): "+<Enchant> Name" (the enchant only for the types 0 .. 3), then the
// additional name in yellow, then the crystal grade as `D` .. `S`, then "x<Count>"; when the whole line is wider
// than MaxWidth the name is cut and ends in ".." (the other texts follow it)
void DrawShopItemName(NCGDevice* Device, INT X, INT Y, INT MaxWidth, INT ItemID, const TCHAR* Name, INT Type, INT Enchant,
                      INT Grade, INT Count)
{
    TCHAR NameText[256];
    TCHAR AddText[256];
    TCHAR GradeText[256];
    TCHAR CountText[256];
    SIZE NameSize;
    SIZE AddSize;
    SIZE CountSize;
    SIZE DotsSize;
    SIZE TmpSize;
    if (Enchant > 0 && (Type == 0 || Type == 1 || Type == 3 || Type == 2))
        appSprintf(NameText, TEXT("+%d %s"), Enchant, Name);
    else
        appSprintf(NameText, TEXT("%s"), Name);
    NCGDevice::GetTextExtent(&NameSize, NameText, 0, 0, 0, 0, 0);
    UBOOL bAdditional = 0;
    if (GConsole->HasItemAdditionalName(ItemID))
    {
        appSprintf(AddText, TEXT("%s"), GConsole->GetItemAdditionalName(ItemID));
        NCGDevice::GetTextExtent(&AddSize, AddText, 0, 0, 0, 0, 0);
        bAdditional = 1;
    }
    UBOOL bGrade = 0;
    INT AddEnd = NameSize.cx;
    if (bAdditional)
        AddEnd = AddSize.cx + 2 + NameSize.cx;
    INT GradeEnd = AddEnd;
    if (Grade >= 0 && Grade < 5 && GConsole->GetGradeText(Grade))
    {
        appSprintf(GradeText, TEXT("`%s`"), GConsole->GetGradeText(Grade));
        bGrade = 1;
        GradeEnd = AddEnd + 14;
    }
    UBOOL bCount = 0;
    INT TotalEnd = GradeEnd;
    if (Count > 0)
    {
        appSprintf(CountText, TEXT("x%d"), Count);
        NCGDevice::GetTextExtent(&CountSize, CountText, 0, 0, 0, 0, 0);
        bCount = 1;
        TotalEnd = CountSize.cx + 2 + GradeEnd;
    }
    INT Over = TotalEnd - MaxWidth;
    INT Cut = 0;
    if (Over > 0)
    {
        TmpSize.cx = 0;
        NCGDevice::GetTextExtent(&DotsSize, TEXT(".."), 0, 0, 0, 0, 0);
        INT Len = appStrlen(NameText);
        do
        {
            Len--;
            if (Len < 0)
                break;
            NameText[Len] = 0;
            NCGDevice::GetTextExtent(&TmpSize, NameText, 0, 0, 0, 0, 0);
        } while ((NameSize.cx - DotsSize.cx) - TmpSize.cx <= Over);
        // (the stock code writes the first dot at index Len even when it is -1, before the buffer)
        if (Len >= 0)
            NameText[Len] = '.';
        NameText[Len + 1] = '.';
        Cut = (NameSize.cx - DotsSize.cx) - TmpSize.cx;
        NameText[Len + 2] = 0;
    }
    Device->DrawText(X, Y, 0xFFDCDCDC, NameText, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (bAdditional)
        Device->DrawText(NameSize.cx - Cut + 2 + X, Y, 0xFFFFD969, AddText, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (bGrade)
        Device->DrawText(AddEnd - Cut + 2 + X, Y, 0xFFDCDCDC, GradeText, 0, 0, 0, L2FONT_Normal, 1, 0, 0, 0, 0, 0);
    if (bCount)
        Device->DrawText(GradeEnd - Cut + 2 + X, Y, 0xFFDCDCDC, CountText, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}
NW_HOOK_FUNC(0x10093a70, DrawShopItemName)

// 0x100957b0 (vslot 56) - rows of 32 pixels (+ 8): the highlight of the selected row, the icon in its frame, the item's name
// line (DrawShopItemName) and, under it, the description in yellow. C3 sets the text origin and the line gap here
INT NCVIPShopItemListBox::OnPaint(NCGDevice* Device)
{
    guard(NCVIPShopItemListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    TextX = 1;
    TextY = 1;
    INT Line = 0;
    LineGap = 8;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Entries.Num(); i++, Line++)
    {
        NVIPShopEntry* Entry = Entries(i);
        if (i == Selected && HighlightTex)
            Device->DrawStretchedTex(0, (LineGap + 32) * Line, (INT)(Width - 16.0f - 2.0f), 32, HighlightTex, 1, 0xFF);
        if (Entry && Entry->Icon)
        {
            Device->DrawTexture(TextX - 1, (LineGap + 32) * Line + TextY - 1, 34, 34, 0, 0, 34, 34, IconBoxTex, 0xFF, 1);
            Device->DrawTexture(TextX, (LineGap + 32) * Line + TextY, 32, 32, 0, 0, 32, 32, Entry->Icon, 0xFF, 1);
        }
        DrawShopItemName(Device, TextX + 34, (LineGap + 32) * Line + 2 + TextY,
                         (INT)(Width - (FLOAT)TextX - 32.0f - 2.0f - 15.0f), Entry->ItemID, Entry->Name, Entry->Type,
                         Entry->Enchant, Entry->Grade, Entry->Count);
        if (Entry->Desc[0])
            Device->DrawText(TextX + 34, (LineGap + 32) * Line + 16 + TextY, 0xFFB09B79, Entry->Desc, 0, 0, 0, L2FONT_Normal,
                             0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100957b0, NCVIPShopItemListBox, OnPaint)

// stock body shared with NCTrainListBox's (0x100862c0)
INT NCVIPShopItemListBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Entries.Num())
        NotifyParent(Selected);
    return 1;
}

// 0x10093db0 (the db names it NCTrainListBox::HitTest: a copy of the code, rows of 32 pixels + the gap, one more row
// shown than there are lines)
INT NCVIPShopItemListBox::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > (INT)Width || RY < 0 || RY > (INT)Height)
        return -1;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex; i++, Line++)
    {
        if (i > NumLines)
            break;
        if (RX < (INT)Width && RY >= (LineGap + 32) * Line + TextY && RY < (Line + 1) * (LineGap + 32) + TextY)
        {
            Selected = i;
            break;
        }
    }
    return Selected;
}
NW_HOOK_METHOD(0x10093db0, NCVIPShopItemListBox, HitTest)

// the rows freed, the scroll bar and the view reset: the stock slot is 0x100963e0, the body NCHennaListBox::Clear is
// hooked at (the code is the same for the list boxes whose rows are in the array at +0x220)
void NCVIPShopItemListBox::Clear()
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

// 0x10086300 - the list boxes' shared body (hooked on NCHennaListBox): the array's count into the scroll bar
__declspec(noinline) void NCVIPShopItemListBox::UpdateEntryScrollBar()
{
    NumLines = Entries.Num();
    ScrollBar->SetRange(NumLines, VisibleLines);
    ScrollBar->SetValue(TopIndex);
}

// 0x10096460 - keeps a copy (the strings only when they are not empty; the icon is Src's, IconName and BodyPart are not
// copied); nothing is dropped when full
void NCVIPShopItemListBox::AddEntry(const NVIPShopEntry* Src)
{
    NVIPShopEntry* Entry = (NVIPShopEntry*)appMalloc(sizeof(NVIPShopEntry), TEXT("new"));
    if (Entry)
        Entry = new ((void*)Entry) NVIPShopEntry;
    if (Src->Name[0])
        appStrcpy(Entry->Name, Src->Name);
    if (Src->Desc[0])
        appStrcpy(Entry->Desc, Src->Desc);
    Entry->ProductID = Src->ProductID;
    Entry->Icon = Src->Icon;
    Entry->Count = Src->Count;
    Entry->Type = Src->Type;
    Entry->ItemID = Src->ItemID;
    Entry->Enchant = Src->Enchant;
    Entry->Grade = Src->Grade;
    if (NumLines < MaxLines)
    {
        NumLines++;
        if (bAutoScroll && NumLines > VisibleLines)
            TopIndex++;
    }
    INT Index = Entries.Add();
    Entries(Index) = Entry;
    UpdateEntryScrollBar();
}
NW_HOOK_METHOD(0x10096460, NCVIPShopItemListBox, AddEntry)

NW_CHECK_VTABLE(NCVIPShopItemListBox, 0x1018c230, 76)
