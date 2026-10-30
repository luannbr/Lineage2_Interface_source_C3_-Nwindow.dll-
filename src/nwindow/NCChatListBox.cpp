/*=============================================================================
    NCChatListBox.cpp - chat message list. Stock: 0x10037630 .. 0x100376bf,
    0x10038580 .. 0x100387b5, 0x1003aa80 .. 0x1003acdd, 0x1003cb50 .. 0x1003cc14.
=============================================================================*/
#include "nwindow/NCChatListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// 0x10052300 - the list box's default constructor, then this class's vtable
NCChatListBox::NCChatListBox()
{
}
NW_HOOK_CTOR(0x10052300, NCChatListBox)

// 0x100523a0 (ret 0xC) - the list box's three-argument constructor (NCChatWnd makes its five lists with 200, 9, 15)
NCChatListBox::NCChatListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x100523A0, NCChatListBox, (INT A, INT B, INT C), (A, B, C))

// 0x10052350 - the vtable, ConditionalDestroy, then the list box's destructor
NW_HOOK_DTOR(0x10052350, NCChatListBox)


// 0x101241c0 (vslot 25) - the stock body is shared with NCDebugWnd's: a plain forward to the
// list box (no enabled check, unlike the other WindowProcs)
INT NCChatListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x101241c0, NCChatListBox, WindowProc)

// 0x1004c6b0 (vslot 37) - C3: the scroll controls are 15 x 15 on the left (the window's size comes as floats), the
// text after them
INT NCChatListBox::OnCreate()
{
    NCListBox::OnCreate();
    ScrollUpButton->SetPosition(0, 0, 15, 15);
    ScrollDownButton->SetPosition(0, (INT)(Height - 15.0f), 15, 15);
    ScrollBar->SetPosition(0, 15, 15, (INT)(Height - 30.0f));
    TextX = 23;
    TextY = 4;
    return 1;
}
NW_HOOK_METHOD(0x1004c6b0, NCChatListBox, OnCreate)

// 0x1004c730 (vslot 56) - guarded. C3: the lines are 15 px (the visible count) and drawn 11 + LineGap apart; the text
// is wrapped by the canvas at the width left of the scroll controls (Width - 22) less the line's indent, which the
// stock converts as an unsigned number; the slider's value is only set when the count changed
INT NCChatListBox::OnPaint(NCGDevice* Device)
{
    guard(NCChatListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if ((INT)Height / 15 != VisibleLines)
    {
        VisibleLines = (INT)(Height * (1.0f / 15.0f));
        if (Items.Num() >= VisibleLines)
            TopIndex = Items.Num() - VisibleLines;
        ScrollBar->SetRange(NumLines, VisibleLines);
        ScrollBar->SetValue(TopIndex);
    }
    ScrollBar->SetRange(NumLines, VisibleLines);
    if (Items.Num() < VisibleLines)
        TopIndex = 0;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Items.Num(); i++, Line++)
    {
        NListItem Item = Items(i);
        Item.Text->GetAt(0);
        DWORD Color = Item.Text->Color;
        INT Indent = Item.Text->Indent;
        Device->DrawText(TextX + Indent, (LineGap + 11) * Line + TextY, Color, Item.Text->GetAt(0),
                         (INT)(Width - 16.0f - 6.0f - (DWORD)Indent), 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004c730, NCChatListBox, OnPaint)

// 0x1012ca10 (ret 8) - the stock's out-of-line TArray<NListItem>::Remove(Index, Count): the list boxes' oldest line
// goes with it (UnTemplate.h's three checks, then the element's destructors and the engine's FArray::Remove)
typedef TArray<NListItem> TListItemArray;
NW_HOOK_METHOD(0x1012CA10, TListItemArray, Remove)

// one wrapped line at the end of the list (the stock repeats it inline): the list keeps NumLines <= MaxLines, the
// view follows the last line; the C3 element has a third field, cleared
static __forceinline void AppendWrappedLine(NCListBox* List, NString* Str, INT Param)
{
    if (List->NumLines < List->MaxLines)
    {
        if (++List->NumLines > List->VisibleLines)
            List->TopIndex++;
    }
    else
    {
        NString* Oldest = List->Items(0).Text;
        if (Oldest)
        {
            delete Oldest;
            List->Items(0).Text = NULL;
        }
        List->Items.Remove(0, 1);
    }
    INT i = List->Items.Add();
    List->Items(i).Text = Str;
    List->Items(i).Param = Param;
    List->Items(i).Unknown8 = 0;
    List->UpdateScrollBar();
}

// 0x1004e940 (vslot 70) - guarded in C1 only. The text is wrapped to the width less the scroll controls (16 + 6),
// the next lines 12 px less and indented 12 (C3: the width is a float)
void NCChatListBox::AddString(const TCHAR* Text, DWORD Color, INT Param)
{
    if (!Text)
        return;
    TCHAR* Wrapped = NCGDevice::FirstWrappedLine(Text, (INT)(Width - 16.0f - 6.0f), 0);
    if (!Wrapped)
        return;
    NString* Str = new NString(Wrapped);
    Str->Color = Color;
    AppendWrappedLine(this, Str, Param);
    for (Wrapped = NCGDevice::NextWrappedLine((INT)(Width - 16.0f - 6.0f - 12.0f), 0); Wrapped;
         Wrapped = NCGDevice::NextWrappedLine((INT)(Width - 16.0f - 6.0f - 12.0f), 0))
    {
        NString* Next = new NString(Wrapped);
        Next->Color = Color;
        Next->Indent = 12;
        AppendWrappedLine(this, Next, Param);
    }
}
NW_HOOK_METHOD(0x1004e940, NCChatListBox, AddString)

// 0x1004b550 (vslot 76) - petitions and announcements always show; system messages have their
// own switches (C2 has a third: the used items)
UBOOL NCChatListBox::IsChannelShown(INT Type, INT SubType)
{
    UBOOL bShow;
    switch (Type)
    {
    case 0:  bShow = Filter.bNormal; break;
    case 1:  bShow = Filter.bShout;  break;
    case 2:  bShow = Filter.bTell;   break;
    case 3:  bShow = Filter.bParty;  break;
    case 4:  bShow = Filter.bPledge; break;
    case 8:  bShow = Filter.bMarket; break;
    case 9:  bShow = Filter.bAlly;   break;
    case 6:
    case 7:
    case 10:
        return 1;
    case 5:
        if (SubType == 3)
            return Filter.bDamage && Filter.bSystem;
        if (SubType == 6)
            return 1;
        if (SubType == 7)
            return Filter.bUseItems && Filter.bSystem;
        return Filter.bSystem != 0;
    default:
        return 0;
    }
    return bShow && Filter.bChat;
}
NW_HOOK_METHOD(0x1004b550, NCChatListBox, IsChannelShown)

NW_CHECK_VTABLE(NCChatListBox, 0x101826a0, 77)
