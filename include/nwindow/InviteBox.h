#pragma once
// InviteBox - the list of friends the invite window (InviteWnd) offers (UClass "InviteBox", 0x224; C3 only, the
// messenger's conversation windows). A list box of 16 visible lines (256 kept) whose lines are drawn with a small
// icon (the friend's status icon, NListItem.Unknown8), a highlight bar under the selected one (two textures) and the
// name cut to 250 pixels; a double click is reported to the parent as WM_LBUTTONDBLCLK (0x203) and invites at once.

#include "nwindow/NCListBox.h"
#include "nwindow/NChatTypes.h"
#pragma pack(push, 4)

class InviteBox : public NCListBox
{
    NW_DECLARE_CLASS(InviteBox, NCListBox, 0, 0x102BC5F0)

    UTexture* SelectTex2;            // +0x220 "L2UI_CH3.ListCtrl.TextSelect2": the right end of the highlight bar
                                     //   (the left part is NCListBox::HighlightTex, "L2UI_CH3.ListCtrl.TextSelect")

    InviteBox();                                                     // 0x1011D500
    InviteBox(INT InMaxLines, INT InVisibleLines, INT InUnknown20C); // 0x1011D5B0 (anonymous in the db) (ret 0xC)
    // ~InviteBox                                                    // 0x1011D560

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119A10
    virtual INT OnCreate();                                          // 37 0x1011A260
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1011A2B0
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 72 0x10119A70 the parent hears it (0x203)

    // 0x1011B450 (ret 0x10, name ours): a friend's line: Text (cut to fit with "..."), the person as the line's Param, the
    // status Icon
    void AddFriend(UTexture* Icon, const TCHAR* Text, NChatUser* User, DWORD Color);
};
NW_ASSERT_OFFSET(InviteBox, SelectTex2, 0x220);
NW_ASSERT_SIZE(InviteBox, 0x224);

#pragma pack(pop)
