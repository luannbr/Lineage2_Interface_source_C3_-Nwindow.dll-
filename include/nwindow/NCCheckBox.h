#pragma once
// NCCheckBox - check box with a label (UClass "CCheckBox", 0x2C4). A click toggles bChecked
// and reports it to the parent (NWM_COMMAND, lParam = new state), unless bDisabled.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCCheckBox : public NControl
{
    NW_DECLARE_CLASS(NCCheckBox, NControl, 0, 0x10262E08)

    UTexture* CheckedTex;            // +0x1D4 "L2UI.Control.CheckBox_Checked"
    UTexture* UncheckedTex;          // +0x1D8 "L2UI.Control.CheckBox"
    UTexture* UncheckedDisabledTex;  // +0x1DC "L2UI.Control.CheckBox_Unable"
    UTexture* CheckedDisabledTex;    // +0x1E0 "L2UI.Control.CheckBox_Checked_unable"
    FString Label;                   // +0x1E4 (C1: TCHAR Label[128])   // C2: offset derived, no evidence
    UBOOL bChecked;                  // +0x1F0
    UBOOL bDisabled;                 // +0x1F4 grey, clicks ignored
    UBOOL bLabelLeft;                // +0x1F8 label first, box after it   // C2: offset derived, no evidence
    INT BoxW;                        // +0x1FC (12)   // C2: offset derived, no evidence
    INT BoxH;                        // +0x200 (12)   // C2: offset derived, no evidence
    DWORD TextParam;                 // +0x204 new in C2 (name ours): the constructor's fourth argument, which the label is
                                     //   drawn with (NCGDevice::DrawText's fifth argument)

    NCCheckBox(const TCHAR* InLabel = TEXT("default"), UBOOL bInChecked = 0,
               UBOOL bInLabelLeft = 0, INT InTextParam = 0);             // 0x100049e0
    NCCheckBox(INT LabelID, UBOOL bInChecked, UBOOL bInLabelLeft);           // 0x10004b50 SysString
    // ~NCCheckBox                                                       // 0x10004ae0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);      // 25 0x100127a0 (NCButton's body)
    virtual INT OnCreate();                                              // 37 0x10004600
    virtual INT OnDestroy();                                             // 38 0x100046b0
    virtual INT OnPaint(NCGDevice* Device);                              // 56 0x10004760
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);             // 62 0x10004730
};
NW_ASSERT_OFFSET(NCCheckBox, Label, 0x1E4);
NW_ASSERT_OFFSET(NCCheckBox, bChecked, 0x1F0);
NW_ASSERT_OFFSET(NCCheckBox, BoxH, 0x200);
NW_ASSERT_SIZE(NCCheckBox, 0x208);

#pragma pack(pop)