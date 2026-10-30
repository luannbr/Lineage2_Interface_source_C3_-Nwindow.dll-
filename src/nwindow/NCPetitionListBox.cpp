/*=============================================================================
    NCPetitionListBox.cpp - petition conversation list (C3 addresses). Stock: 0x100d08a0, 0x100d1c50,
    0x100d2c00 .. 0x100d2cc2 (WindowProc is NCChatListBox's body 0x101241c0).
=============================================================================*/
#include "nwindow/NCPetitionListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// 0x100d2c00 - NCListBox's constructor and the vtable
NCPetitionListBox::NCPetitionListBox()
{
}
NW_HOOK_CTOR(0x100d2c00, NCPetitionListBox)

// 0x100d2ca0 (ret 0xc)
NCPetitionListBox::NCPetitionListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x100d2ca0, NCPetitionListBox, (INT A, INT B, INT C), (A, B, C))

// 0x100d2c50
NW_HOOK_DTOR(0x100d2c50, NCPetitionListBox)

// 0x101241c0 (vslot 25) - NCChatListBox's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCPetitionListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x101241c0)
#else
NW_SLOT_IS("?WindowProc@NCPetitionListBox@@UAEHIIJ@Z", "?WindowProc@NCChatListBox@@UAEHIIJ@Z")   // NCPetitionListBox::WindowProc = NCChatListBox::WindowProc (0x101241c0)
#endif

// 0x100d08a0 (vslot 37) - the scroll controls on the right edge: the buttons 15 x 15 at the top and the bottom, the
// slider between them; the text starts at the left edge
INT NCPetitionListBox::OnCreate()
{
    NCListBox::OnCreate();
    ScrollUpButton->SetPosition((INT)(Width - 15.0f), 0, 15, 15);
    ScrollDownButton->SetPosition((INT)(Width - 15.0f), (INT)(Height - 15.0f), 15, 15);
    ScrollBar->SetPosition((INT)(Width - 15.0f), 15, 15, (INT)(Height - 30.0f));
    TextX = 0;
    TextY = 0;
    return 1;
}
NW_HOOK_METHOD(0x100d08a0, NCPetitionListBox, OnCreate)

// 0x100d0940 (vslot 56; the db names it NCChatListBox::OnPaint, the guard string is NCChatListBox's too) - guarded. The
// lines are 17 px apart (the line gap is set to 7 and the text column to 2 here, as the stock does at every paint), each
// centered on the height of an "A" of the normal font; the canvas does not wrap them (the list wrapped them when they
// were added) and the visible count is the one the list was made with
INT NCPetitionListBox::OnPaint(NCGDevice* Device)
{
    guard(NCChatListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    LineGap = 7;
    TextX = 2;
    SIZE CharSize;
    NCGDevice::GetCharSize(&CharSize, TEXT('A'), 0);
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Items.Num(); i++, Line++)
    {
        NListItem Item = Items(i);
        DWORD Color = Item.Text->Color;
        INT Indent = Item.Text->Indent;
        Device->DrawText(TextX + Indent, TextY - CharSize.cy / 2 + 9 + Line * 17, Color, Item.Text->GetAt(0), 0, 0, 0,
                         L2FONT_Normal, 1, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100d0940, NCPetitionListBox, OnPaint)

// one wrapped line at the end of the list (inlined twice in the stock AddString); C3: the items are 12 bytes
static FORCEINLINE void AppendWrappedLine(NCListBox* List, NString* Str, INT Param)
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

// 0x100d1c50 (vslot 70) - the text is wrapped to the list width - first line up to Width - 16 - 6, the next ones up to
// Width - 16 - 6 - 12, indented 12 px - and, unlike NCListBox::AddString, the view always follows the last line
void NCPetitionListBox::AddString(const TCHAR* Text, DWORD Color, INT Param)
{
    if (!Text)
        return;
    TCHAR* Line = NCGDevice::FirstWrappedLine(Text, (INT)(Width - 16.0f - 6.0f), 0);
    if (!Line)
        return;
    NString* Str = new NString(Line);
    Str->Color = Color;
    AppendWrappedLine(this, Str, Param);
    for (Line = NCGDevice::NextWrappedLine((INT)(Width - 16.0f - 6.0f - 12.0f), 0); Line;
         Line = NCGDevice::NextWrappedLine((INT)(Width - 16.0f - 6.0f - 12.0f), 0))
    {
        NString* Next = new NString(Line);
        Next->Color = Color;
        Next->Indent = 12;
        AppendWrappedLine(this, Next, Param);
    }
}
NW_HOOK_METHOD(0x100d1c50, NCPetitionListBox, AddString)

NW_CHECK_VTABLE(NCPetitionListBox, 0x101976f8, 76)
