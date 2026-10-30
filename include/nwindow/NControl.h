#pragma once
// NControl - base of every control (buttons, edit boxes, lists...) (UClass "Control", 0x1A0).
// A control reports to its parent with NWM_COMMAND (wParam = the control's ID, lParam = a code
// that depends on the control: NotifyID for buttons, 1/0 for press/release...).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NControl : public NCWnd
{
    NW_DECLARE_CLASS(NControl, NCWnd, 0, 0x10263D80)

    INT ID;                    // +0xC8 set by the parent that creates the control
    UTexture* DisableTexture;  // +0xCC "NWindow.Disable" (OnCreate)
    INT NotifyID;              // +0xD0 id sent to the parent, -1 = ID (name ours)
    TCHAR Caption[128];        // +0xD4 (name ours)

    NControl();                                                     // 0x10002820
    // ~NControl                                                    // 0x10001be0 (C1 only: no C2 match)

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 25 0x10119f60 (folded with
                                                                    //    identical overrides)
    virtual INT OnCreate();                                         // 37 0x10005d60
    virtual INT OnDestroy();                                        // 38 0x10005d90
    virtual const TCHAR* GetValue() { return NULL; }                // 68 0x10001af0 (folded) (name
                                                                    //    ours) what an HTML form
                                                                    //    control holds

    // talking to the parent window (names ours)
    UBOOL NotifyParent(LPARAM Code);                                // 0x10005c40 NWM_COMMAND
    void ReleaseFocus();                                            // 0x10005c80
    void HideParent();                                              // 0x10005cb0
    UBOOL SendTo(NWndEntry* Target, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10005ce0
    UBOOL SendToParent(UINT Msg, WPARAM wParam, LPARAM lParam);    // 0x10005d20
    void SetCaption(const TCHAR* Text);                             // 0x10005e10
    const TCHAR* GetCaption();                                      // 0x10005e30

protected:
    NCWnd* ParentWnd() { return Entry->Parent ? Entry->Parent->Wnd : NULL; }
};
NW_ASSERT_OFFSET(NControl, ID, 0xC8);
NW_ASSERT_OFFSET(NControl, NotifyID, 0xD0);
NW_ASSERT_OFFSET(NControl, Caption, 0xD4);
NW_ASSERT_SIZE(NControl, 0x1D4);

#pragma pack(pop)