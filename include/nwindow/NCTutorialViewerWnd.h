#pragma once
// NCTutorialViewerWnd - the tutorial window (UClass "CTutorialViewerWnd", 0xA0): the NPC dialog
// layout around an NCTutorialHtmlViewer, pages given as text.
// NCTutorialQuestionWnd - the blinking question mark that opens a tutorial page (UClass
// "CTutorialQuestionWnd", 0xA8): a click asks the server for the page of its question and
// hides it.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCTutorialHtmlViewer;

class NCTutorialViewerWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTutorialViewerWnd, NCWnd, 0, 0x102B63F8)

    // C3 layout (OnCreate 0x1010a4d0, OnPaint 0x10109d40): three background pieces instead of the single texture
    NCTutorialHtmlViewer* HtmlViewer; // +0xC8
    UTexture* TopTex;                // +0xCC "l2ui_ch3.tutorialwnd.tutorial_back1" 12 pixels, 20 below the top
    UTexture* MidTex;                // +0xD0 "l2ui_ch3.tutorialwnd.tutorial_back2" repeated 12 pixels at a time
    UTexture* BottomTex;             // +0xD4 "l2ui_ch3.tutorialwnd.tutorial_back3" 9 pixels, at the bottom

    NCTutorialViewerWnd();                                           // 0x1010a9f0
    // ~NCTutorialViewerWnd                                          // 0x100de770 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10109830 (shared)
    virtual INT OnCreate();                                          // 37 0x1010a4d0
    virtual INT OnDestroy();                                         // 38 0x10109c90 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10109d40
    virtual INT OnClose(WPARAM wParam, LPARAM lParam);               // 68 0x10035ac0 (shared)

    void ShowPage(TCHAR* Text);                                      // 0x10109890 (name ours)
    void FitToPage(INT ContentHeight);                               // 0x101098b0 (name ours) size to the page it holds
};
NW_ASSERT_OFFSET(NCTutorialViewerWnd, MidTex, 0xD0);
NW_ASSERT_OFFSET(NCTutorialViewerWnd, BottomTex, 0xD4);
NW_ASSERT_SIZE(NCTutorialViewerWnd, 0xD8);

class NCTutorialQuestionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTutorialQuestionWnd, NCWnd, 0, 0x102B6918)

    UTexture* HelpTex;               // +0xC8 "L2UI.etcWnd.TutorialHelp"
    UTexture* HelpClickTex;          // +0xCC "L2UI.etcWnd.TutorialHelp_click"
    INT QuestionID;                  // +0xD0 (name ours) sent when clicked
    UBOOL bLit;                      // +0xD4 HelpTex, else HelpClickTex: the timer blinks it,
                                     //   a press holds it off
    INT TimerID;                     // +0xD8 (0xFA0D06, 500 ms)

    NCTutorialQuestionWnd();                                         // 0x1010aa90
    // ~NCTutorialQuestionWnd                                        // 0x100deb20 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (shared)
    virtual INT OnTimer(INT InTimerID);                              // 36 0x10109a10
    virtual INT OnCreate();                                          // 37 0x10109940
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10109f50
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10109990
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x101099c0
};
NW_ASSERT_OFFSET(NCTutorialQuestionWnd, QuestionID, 0xD0);
NW_ASSERT_SIZE(NCTutorialQuestionWnd, 0xDC);

#pragma pack(pop)