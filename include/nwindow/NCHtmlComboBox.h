#pragma once
// NCHtmlComboBox - the <combobox> of the HTML pages (UClass "CHtmlComboBox", 0x200): an
// NCComboBox whose drop button is an NCHtmlButton (kept in its own field) and that paints
// clipped to the viewer while closed. Its items come from a ';'-separated list; its value is
// the chosen item.

#include "nwindow/NCComboBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCHtmlButton;

class NCHtmlComboBox : public NCComboBox
{
    NW_DECLARE_CLASS(NCHtmlComboBox, NCComboBox, 0, 0x10275CC0)

    INT Unknown1F8;                  // +0x22C   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    NCHtmlButton* HtmlDropButton;    // +0x230 (NCComboBox::DropButton stays NULL)   // C2: offset derived, no evidence

    NCHtmlComboBox();                                                // 0x10039030
    // ~NCHtmlComboBox                                               // 0x100325f0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100412a0
    virtual INT OnCreate();                                          // 37 0x10045bf0
    virtual INT OnDestroy();                                         // 38 0x10044cd0
    virtual void ShowWindow();                                       // 44 0x100412c0 (shared)
    virtual void HideWindow();                                       // 45 0x100412d0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10041790
    virtual const TCHAR* GetValue();                                 // 68 0x10041780

    INT SetList(TCHAR* List, TCHAR* Sel);                            // 0x10041330 (name ours)
};
NW_ASSERT_OFFSET(NCHtmlComboBox, HtmlDropButton, 0x230);
NW_ASSERT_SIZE(NCHtmlComboBox, 0x234);

#pragma pack(pop)