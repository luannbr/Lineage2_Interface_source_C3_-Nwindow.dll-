#pragma once
// NCHtmlViewer - shows an HTML page (UClass "CHtmlViewer", 0x110): the NPC dialogs, the
// tutorial, the board. The page is parsed into a tree (NHtmlLayout.h) that is laid out at the
// viewer's width and drawn scrolled; links and form fields become child controls (NCFileLinker,
// NCActionLinker, NCHtmlButton, NCHtmlEdit, NCHtmlMultiEdit, NCHtmlComboBox) that move with the
// page and report back through NWM_COMMAND with the viewer's ids. A command goes to the
// console (bypass to the server, $name replaced by the value of the named control); when it
// asks for it (-h) the owner window, or else the viewer, closes.
//
// NCTutorialHtmlViewer - the same for the tutorial window (UClass "CTutorialHtmlViewer").

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NClasses.gen.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NControl;
class NHtmlBlock;
struct NHtmlTagList;

// sent to the owner window when a command asks to close (name ours)
enum { NWM_HTML_CLOSE = (INT)0xF0000004 };

class NCHtmlViewer : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCHtmlViewer, NCScrollWnd, 0, 0x10273388)

    NHtmlBlock* Doc;                 // +0x100 the page, NULL while none is loaded
    UTexture* BackTex;               // +0x104 <html imgsrc>, NULL = transparent
    INT FileLinkID;                  // +0x108 NWM_COMMAND ids of the controls it creates
    INT ActionLinkID;                // +0x10C   (0xFF00FF00 .. 0xFF00FF05)
    INT ButtonID;                    // +0x110
    INT EditID;                      // +0x114
    INT MultiEditID;                 // +0x118
    INT ComboBoxID;                  // +0x11C
    INT FontType;                    // +0x120 new in C2 (name ours): UCanvas::L2Fonts index the page is
                                     //   drawn in (0; the credits' viewer passes its own): handed to every NHtmlBlock
    TArray<NControl*> Controls;      // +0x124 the page's controls, in page order
    UBOOL bLoaded;                   // +0x130 Doc holds a page
    INT UnknownFC;                   // +0x134 (0)   // C2: offset derived, no evidence
    INT Unknown100;                  // +0x138 (0)   // C2: offset derived, no evidence
    FLOAT CreateWidth;               // +0x13C size at OnCreate (C3: the FLOAT size of the window)
    FLOAT CreateHeight;              // +0x140
    NWndEntry* Owner;                // +0x144 window a closing command goes to, NULL = itself

    NCHtmlViewer();                                                  // 0x10039260
    NCHtmlViewer(INT InFontType);                                    // 0x100331a0 (C1 only: no C2 match) (only the credit window uses it)
    // ~NCHtmlViewer                                                 // 0x10032640 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10035260
    virtual INT OnCreate();                                          // 37 0x10035360
    virtual INT OnDestroy();                                         // 38 0x100394c0
    virtual void ShowWindow();                                       // 44 0x10036840
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x10035710
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100376f0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100356e0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x10036660
    virtual INT ScrollUp();                                          // 69 0x100365f0
    virtual INT ScrollDown();                                        // 70 0x10036610
    virtual INT OnScrollBar(INT Value);                              // 71 0x10036630
    // names ours
    virtual TCHAR* GetControlValue(const TCHAR* Name);               // 72 0x10036690 checked
    virtual INT OnFileLink(TCHAR* File);                             // 73 0x1003be30
    virtual INT OnActionLink(TCHAR* Action);                         // 74 0x10035420
    virtual INT OnButton(TCHAR* Action);                             // 75 0x100354a0
    virtual INT OnEdit(LPARAM Param);                                // 76 0x10035510
    virtual INT OnMultiEdit(LPARAM Param);                           // 77 0x100355d0
    virtual INT OnComboBox(LPARAM Param);                            // 78 0x10035690

    // names ours
    void SetBackTexture(const TCHAR* Name);                          // 0x100353b0
    void SetOwner(NWndEntry* InOwner);                               // 0x100353f0
    UTexture* FindPageTexture(const TCHAR* Name);                    // 0x10035400
    void ScrollControls(INT DX, INT DY);                             // 0x10036540
    // C2: the link control (an NCDynamicLinker) is told where the item and its parent are, and
    // the width the item was laid out at and how far its last line reaches (NHtmlItem::Arrange)
    INT AddFileLink(INT AbsX, INT AbsY, INT X, INT Y, INT W, INT H, INT LayoutWidth,
                    INT TailWidth, INT ParentAbsX, INT ParentAbsY, TCHAR* File, TCHAR* Msg);   // 0x10037880 (C3: + Msg)
    INT AddActionLink(INT AbsX, INT AbsY, INT X, INT Y, INT W, INT H, INT LayoutWidth,
                      INT TailWidth, INT ParentAbsX, INT ParentAbsY, TCHAR* Action,
                      TCHAR* Msg);                                          // 0x100379c0
    INT AddButton(INT X, INT Y, INT W, INT H, TCHAR* NormalTexName, TCHAR* PushedTexName,
                  TCHAR* Action, TCHAR* Label);                      // 0x10037b00
    INT AddMultiEdit(INT X, INT Y, INT W, INT H, TCHAR* Var);        // 0x10037c70
    INT AddEdit(INT X, INT Y, INT W, INT H, TCHAR* Var, INT Type, TCHAR* Action,
                INT Length);                                         // 0x10037d70
    INT AddComboBox(INT X, INT Y, INT W, INT H, TCHAR* Var, TCHAR* List, TCHAR* Sel); // 0x100395d0
    void Clear();                                                    // 0x10038a20
    void BuildDocument(NHtmlTagList* Tags);                          // 0x1003bb00
    INT LoadHtmlFile(TCHAR* Source);                                 // 0x1003bc30
    INT LoadHtml(TCHAR* Text);                                       // 0x1003bd20
};
NW_ASSERT_OFFSET(NCHtmlViewer, Doc, 0x100);
NW_ASSERT_OFFSET(NCHtmlViewer, Controls, 0x124);
NW_ASSERT_OFFSET(NCHtmlViewer, Owner, 0x144);
NW_ASSERT_SIZE(NCHtmlViewer, 0x148);

class NCTutorialHtmlViewer : public NCHtmlViewer
{
    NW_DECLARE_CLASS(NCTutorialHtmlViewer, NCHtmlViewer, 0, 0x10272E60)

    NCTutorialHtmlViewer();                                          // 0x100396f0
    // ~NCTutorialHtmlViewer                                         // 0x10033540 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10035850
    virtual INT OnCreate();                                          // 37 0x10035870
    virtual INT OnDestroy();                                         // 38 0x10039790 (shared)
};
NW_ASSERT_SIZE(NCTutorialHtmlViewer, 0x148);

// The windows the controls of a page clip to (the loop is inlined in each control's OnPaint in
// the stock code; names ours): the enclosing HTML viewer, or the first ancestor that is not a
// control. NULL at the top of the tree (no clip is set then).
inline NCWnd* EnclosingHtmlViewer(NCWnd* Wnd)
{
    NWndEntry* E = Wnd->Entry;
    for (;;)
    {
        NCWnd* W = E->Parent->Wnd;
        if (!W || W->IsA(NCHtmlViewer::StaticClass()))
            return W;
        E = W->Entry;
    }
}
inline NCWnd* FirstNonControlParent(NCWnd* Wnd)
{
    NWndEntry* E = Wnd->Entry;
    for (;;)
    {
        NCWnd* W = E->Parent->Wnd;
        if (!W || !W->IsA(NClass::NControl()))
            return W;
        E = W->Entry;
    }
}

#pragma pack(pop)