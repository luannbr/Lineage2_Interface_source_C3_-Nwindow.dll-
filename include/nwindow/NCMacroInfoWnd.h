#pragma once
// NCMacroInfoWnd - the macro's additional information window (C2 only; UClass "CMacroInfoWnd", 0x11C): a small
// window beside the macro editor with the description (up to 32 characters; C3: the short name moved to the editor,
// NCMacroEditWnd::ShortNameEdit) of the macro being edited, an OK and a Cancel button. OK keeps the text in the window
// (Description), from where the editor's Save copies it into the macro's slot of NCMacroListWnd (it goes to the server
// with the rest). The desktop keeps it at +0x134 (name ours; NCConsole::NewWnd100).
//
// Layout from the C3 constructor 0x100A0B40, OnCreate 0x1009F620, OnOK 0x1009B7E0 and OnDestroy 0x1009E6B0.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCMultiEdit;

// NWM_COMMAND ids (names ours)
enum
{
    NID_MACROINFO_DESC = 0x1514,       // the description's multi line edit
    NID_MACROINFO_OK = 0x1016,
    NID_MACROINFO_CANCEL = 0x1017,
};

class NCMacroInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMacroInfoWnd, NCWnd, 0, 0x1028A770)

    UTexture* BackTex;               // +0xC8 "l2ui_ch3.macrownd.macro3_back"
    NCMultiEdit* DescEdit;           // +0xCC the description's box
    NCButton* OKButton;              // +0xD0
    NCButton* CancelButton;          // +0xD4
    TCHAR Description[33];           // +0xD8 the description kept by OK (cleared by OnCreate; 0x42 bytes)
    BYTE _PadDA[2];                  // +0x11A (up to sizeof)

    NCMacroInfoWnd();                                                // 0x100A0B40
    // ~NCMacroInfoWnd                                               // 0x100A0B90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1009B6F0
    virtual INT OnCreate();                                          // 37 0x1009F620
    virtual INT OnDestroy();                                         // 38 0x1009E6B0
    virtual void ShowWindow();                                       // 44 0x1009B7A0
    // HideWindow (45): 0x100412D0, the body of NCChatWnd::HideWindow (shared, hooked there)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1009E7D0
    // names ours
    virtual INT OnOK();                                              // 68 0x1009B7E0
    virtual INT OnCancel();                                          // 69 0x1007BEE0 (shared: HideWindow())

    // names ours
    __declspec(noinline) void SetDescription(const TCHAR* Text);                          // 0x1009B770 (ret 4) the description box emptied, then Text put in it
};
NW_ASSERT_OFFSET(NCMacroInfoWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCMacroInfoWnd, DescEdit, 0xCC);
NW_ASSERT_OFFSET(NCMacroInfoWnd, CancelButton, 0xD4);
NW_ASSERT_OFFSET(NCMacroInfoWnd, Description, 0xD8);
NW_ASSERT_SIZE(NCMacroInfoWnd, 0x11C);

#pragma pack(pop)
