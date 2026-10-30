#pragma once
// NCTestHtmlViewer - the HTML test page (UClass "CTestHtmlViewer", 0xA4; the desktop's
// TestHtmlWnd, made when GIsTestHtml is set): the NPC dialog window with an edit line and a
// button under the page, which loads ..\test_html\<the name typed>. Its class registration
// names NCWnd as the super class, though the C++ base is NCNPCHtmlViewer.

#include "nwindow/NCNPCHtmlViewer.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCEditBox;

class NCTestHtmlViewer : public NCNPCHtmlViewer
{
    NW_DECLARE_CLASS(NCTestHtmlViewer, NCNPCHtmlViewer, 0, 0x10273DE0)

    NCEditBox* FileEdit;             // +0xD0 created hidden
    NCButton* LoadButton;            // +0xD4 a white square   // C2: offset derived, no evidence

    NCTestHtmlViewer();                                              // 0x10039b10
    // ~NCTestHtmlViewer                                             // 0x10031220 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10035ad0
    virtual INT OnCreate();                                          // 37 0x10039bb0
    virtual INT OnLoad();                                            // 69 0x1003c720 (name ours)
};
NW_ASSERT_OFFSET(NCTestHtmlViewer, FileEdit, 0xD0);
NW_ASSERT_SIZE(NCTestHtmlViewer, 0xD8);

#pragma pack(pop)