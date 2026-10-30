#pragma once
// NCBoardWnd - the community board (Alt+B) (UClass "CBoardWnd", 0x6B8): a title bar, a row of
// six buttons (sysstrings 377..382) that each send the bypass command the console gave them,
// and the board's page in an NCBBSHtmlViewer.
//
// NCBBSHtmlViewer - the board's page (UClass "CBBSHtmlViewer", 0x24134): an NCHtmlViewer drawn
// on "L2UI.bbs_back" and clipped inside that frame. A command the console answers with 1 locks it
// until the console clears the flag; the click sound plays even for an empty link. Its check of
// a form value lets anything of 2..3000 bytes through, bad words replaced. The console hands
// the page over in pieces (AddPiece, "keyvalue"): a page, or a post (template, fields,
// comments, thread), a write form (template, fields) or a page in three parts, assembled when
// all their pieces are in.

#include "nwindow/NCHtmlViewer.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCTabControl;

class NCBBSHtmlViewer : public NCHtmlViewer
{
    NW_DECLARE_CLASS(NCBBSHtmlViewer, NCHtmlViewer, 0, 0x10274300)

    INT Unknown110;                  // +0x148 (0)   // C2: offset derived, no evidence
    UTexture* BoardBackTex;          // +0x14C "L2UI.bbs_back", drawn without <html imgsrc>
    INT PageKind;                    // +0x150 last page assembled (names ours): 0 a whole page,
                                     //   1 a page in parts, 2 a post, 3 a write form
    UBOOL bLocked;                   // +0x154 a command was accepted (the console answered 1)
    INT NumPostPieces;               // +0x158 (0) pieces "1".."4" received
    INT NumFormPieces;               // +0x15C (0) pieces "1001", "1002" received
    INT NumPageParts;                // +0x160 (0) pieces "101".."103" received
    TCHAR PostTemplate[0x2000];      // +0x164 "1": the post page, with <!--FIELD--> marks
    TCHAR PostFields[0x2000];        // +0x4164 "2": its fields, separated by ''
    TCHAR PostComments[0x2000];      // +0x8164 "3": for <!--COMMENT-->
    TCHAR PostThread[0x2000];        // +0xC164 "4": for <!--THREAD-->
    TCHAR FormTemplate[0x2000];      // +0x10164 "1001": the write form   // C2: offset derived, no evidence
    TCHAR FormFields[0x2000];        // +0x14164 "1002": its fields   // C2: offset derived, no evidence
    TCHAR PageParts[3][0x2000];      // +0x18164 "101", "102", "103"

    NCBBSHtmlViewer();                                               // 0x10039810
    // ~NCBBSHtmlViewer                                              // 0x10033600 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10035880
    virtual INT OnCreate();                                          // 37 0x100358e0
    virtual INT OnDestroy();                                         // 38 0x100398d0
    virtual void ShowWindow();                                       // 44 0x10036a50
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10036910
    virtual TCHAR* GetControlValue(const TCHAR* Name);               // 72 0x10036a60
    virtual INT OnActionLink(TCHAR* Action);                         // 74 0x10035960
    virtual INT OnButton(TCHAR* Action);                             // 75 0x10035a10
    // name ours: NWM_COMMAND 0xF0F01411 hides the viewer (nothing in NWindow sends it)
    virtual INT OnHideCommand();                                     // 79 0x100dec70 (shared)

    // names ours
    INT ShowPage(TCHAR* Text);                                       // 0x1003be70
    void BuildPostPage();                                            // 0x1003be80
    void BuildFormPage();                                            // 0x1003c110
    void ShowPageParts();                                            // 0x1003c490
    void AddPiece(TCHAR* Text);                                      // 0x1003c520
};
NW_ASSERT_OFFSET(NCBBSHtmlViewer, bLocked, 0x154);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, PostTemplate, 0x164);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, FormTemplate, 0x10164);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, PageParts, 0x18164);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, FormFields, 0x14164);
NW_ASSERT_SIZE(NCBBSHtmlViewer, 0x24164);

class NCBoardWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBoardWnd, NCWnd, 0, 0x10272928)

    NCBBSHtmlViewer* HtmlViewer;     // +0xC8
    UTexture* BackTex;               // +0xCC "L2UI_CH3.BoardWnd.board_back1" (C3)
    UTexture* BackTex2;              // +0xD0 "L2UI_CH3.BoardWnd.board_back2"
    NCButton* Button;                // +0xD4 (C3) NWM_COMMAND 0xF0F01422: sends Commands[7]
    NCTabControl* TabCtrl;           // +0xD8 (C3) NWM_COMMAND 0xF0F01423: seven tabs, each with a command
    TCHAR Commands[8][128];          // +0xDC bypass command of each tab (0..6) and of the button (7), from the console
    UBOOL bLocked;                   // +0x8DC a command was accepted (the console answered 1)

    NCBoardWnd();                                                    // 0x10039d30
    // ~NCBoardWnd                                                   // 0x100312c0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10035b30
    virtual INT OnCreate();                                          // 37 0x10039dd0
    virtual INT OnDestroy();                                         // 38 0x10036dc0
    virtual void ShowWindow();                                       // 44 0x100aa860 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10036ed0
    // names ours
    virtual void Minimize();                                         // 49 0x10035db0 (C3: its own icon)
    virtual INT OnTab(INT Tab);                                      // 68 0x10035bb0 (name ours) sends Commands[Tab]
    virtual INT OnButton1();                                         // 69 0x10035c90 sends Commands[7]
    // names ours (called by the console's ShowBoard; C3: eight commands)
    void SetCommands(TCHAR* Command1, TCHAR* Command2, TCHAR* Command3, TCHAR* Command4,
                     TCHAR* Command5, TCHAR* Command6, TCHAR* Command7, TCHAR* Command8); // 0x10035ce0
    void LoadPage(TCHAR* Html);                                      // 0x1003c780 into HtmlViewer
};
NW_ASSERT_OFFSET(NCBoardWnd, TabCtrl, 0xD8);
NW_ASSERT_OFFSET(NCBoardWnd, Commands, 0xDC);
NW_ASSERT_OFFSET(NCBoardWnd, bLocked, 0x8DC);
NW_ASSERT_SIZE(NCBoardWnd, 0x8E0);

#pragma pack(pop)