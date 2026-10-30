#pragma once
// NCPetitionListBox - the petition conversation list (UClass "CPetitionListBox", 0x1E8). Adds
// messages like the chat list (wrapped to the width, continuation lines indented 12 px), but
// the scroll controls stay on the right, the text starts at the left edge and the visible line
// count is fixed.

#include "nwindow/NCListBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPetitionListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCPetitionListBox, NCListBox, 0, 0x1029D7C0)

    NCPetitionListBox();                                             // 0x100d2c00
    NCPetitionListBox(INT InMaxLines, INT InVisibleLines, INT Unused); // 0x100d2ca0
    // ~NCPetitionListBox                                            // 0x100d2c50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101241c0 (NCDebugWnd's)
    virtual INT OnCreate();                                          // 37 0x100d08a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d0940
    virtual void AddString(const TCHAR* Text, DWORD Color, INT Param); // 70 0x100d1c50
};
NW_ASSERT_SIZE(NCPetitionListBox, 0x220);

#pragma pack(pop)