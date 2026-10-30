#pragma once
// NCNPCHtmlViewer - the NPC dialog window (UClass "CNPCHtmlViewer", 0x9C): a title bar, the
// "NPCHtml_BACK" background stretched, and an NCHtmlViewer that closes the window when a page
// command asks for it (NWM_HTML_CLOSE).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCHtmlViewer;

class NCNPCHtmlViewer : public NCWnd
{
    NW_DECLARE_CLASS(NCNPCHtmlViewer, NCWnd, 0, 0x102738B8)

    NCHtmlViewer* HtmlViewer;        // +0xC8
    UTexture* BackTex;               // +0xCC "L2UI.etcwnd.NPCHtml_BACK"

    NCNPCHtmlViewer();                                               // 0x10039950
    // ~NCNPCHtmlViewer                                              // 0x10030270 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10109830
    virtual INT OnCreate();                                          // 37 0x10039970
    virtual INT OnDestroy();                                         // 38 0x10036c10 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10036cc0
    virtual INT OnClose(WPARAM wParam, LPARAM lParam);               // 68 0x10035ac0 (name ours)

    void LoadFile(const TCHAR* FileName);                            // 0x1003c700 (name ours)
    void ShowPage(TCHAR* Text);                                      // 0x10109890 (folded with
                                                                     //   NCTutorialViewerWnd's)
};
NW_ASSERT_OFFSET(NCNPCHtmlViewer, HtmlViewer, 0xC8);
NW_ASSERT_SIZE(NCNPCHtmlViewer, 0xD0);

#pragma pack(pop)