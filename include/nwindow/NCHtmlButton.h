#pragma once
// NCHtmlButton / NCHtmlPushButton - the buttons of the HTML pages (UClass "CHtmlButton" 0x220,
// "CHtmlPushButton" 0x228). They paint like NCButton but clip to the window they live in, so a
// page scrolled under the viewer's edge does not show them: NCHtmlButton to the enclosing
// NCHtmlViewer, NCHtmlPushButton to its first ancestor that is not a control (the scroll
// buttons of an HTML edit box). A click on an NCHtmlButton reports Command to the viewer.

#include "nwindow/NCButton.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCHtmlButton : public NCButton
{
    NW_DECLARE_CLASS(NCHtmlButton, NCButton, 0, 0x10277BA0)

    const TCHAR* Command;      // +0x29C (name ours) sent on a click (NWM_COMMAND lParam): the
                               //   <button> action

    NCHtmlButton();                                                             // 0x100455a0
    NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, const TCHAR* InCommand,
                 UBOOL bInAcceptKey);                                           // 0x100308e0
    NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);       // 0x10030910 (C1 only: no C2 match) (C1 only: no C2 match)
    // ~NCHtmlButton                                                            // 0x100455f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 25 0x10041760 (shared)
    virtual INT OnPaint(NCGDevice* Device);                                     // 56 0x10041d60
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 63 0x10041540
};
NW_ASSERT_OFFSET(NCHtmlButton, Command, 0x29C);
NW_ASSERT_SIZE(NCHtmlButton, 0x2A0);

class NCHtmlPushButton : public NCPushButton
{
    NW_DECLARE_CLASS(NCHtmlPushButton, NCPushButton, 0, 0x10276C30)

    NCHtmlPushButton();                                                         // 0x100456a0
    NCHtmlPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);   // 0x10030cc0 (C1 only: no C2 match) (C1 only: no C2 match)
    // ~NCHtmlPushButton                                                        // 0x100456f0

    virtual INT OnPaint(NCGDevice* Device);                                     // 56 0x100423f0
};
NW_ASSERT_SIZE(NCHtmlPushButton, 0x2A8);

#pragma pack(pop)