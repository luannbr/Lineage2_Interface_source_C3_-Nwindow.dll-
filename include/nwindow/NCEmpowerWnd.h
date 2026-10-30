#pragma once
// NCEmpowerWnd - the clan member privileges ("pledge power") window (UClass "CEmpowerWnd", 0x10C;
// C2 only; NCConsole +0xA0): ten check boxes in three groups (4 + 3 + 3), a check box that
// heads each group and sets or clears all of its group's, and two buttons. The ten boxes are the
// bits of the privileges the server keeps for a rank (Power: bit i of the 32 bytes is box i);
// the console fills it (SetPower, when the server answers) after setting MemberID, Mode and
// Name. Mode 1 only shows them (every box disabled), mode 2 lets them be changed and OK
// sends the new bits for MemberID.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCCheckBox;

class NCEmpowerWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCEmpowerWnd, NCWnd, 0, 0x1027D450)

    NCCheckBox* Checks[13];          // +0xC8 (names ours) 0..9 the privileges (groups 0-3, 4-6, 7-9),
                                     //   10..12 the heads of the three groups
    NCButton* OKButton;              // +0xFC text 140 (id 0xFF0F1A11)
    NCButton* CancelButton;          // +0x100 text 141 (the same id: it does what OK does)
    UTexture* BackTex;               // +0x104 "l2ui.actionwnd.empowerback"
    BYTE Power[32];                  // +0x108 the privileges as bits (name ours)
    INT MemberID;                    // +0x128 whose privileges these are, 0 = none (name ours)
    INT Mode;                        // +0x12C 1 view, 2 edit (name ours)
    FString Name;                    // +0x130 shown above the boxes (name ours)

    NCEmpowerWnd();                                                  // 0x1007d310
    // ~NCEmpowerWnd                                                 // 0x10070e00 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007be30
    virtual INT OnCreate();                                          // 37 0x1007c8a0
    virtual INT OnDestroy();                                         // 38 0x1007bef0
    virtual void ShowWindow();                                       // 44 0x1007c650
    virtual void HideWindow();                                       // 45 0x1007c710
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007c720
    virtual INT OnOK();                                              // 68 0x1007c360 (name ours)
    virtual INT OnCancel();                                          // 69 0x1007bee0 (name ours; NCOptionWnd's body: HideWindow)
    virtual INT OnHeadA();                                           // 70 0x1007c3b0 (name ours) the first group's head
    virtual INT OnHeadB();                                           // 71 0x1007c450 (name ours)
    virtual INT OnHeadC();                                           // 72 0x1007c4e0 (name ours)

    // names ours
    __declspec(noinline) void ClearChecks();                         // 0x1007c0c0 every box unchecked
    __declspec(noinline) void ShowPower();                           // 0x1007c120 the boxes (and heads) of the bits set, checked
    __declspec(noinline) void CollectPower();                        // 0x1007c280 the boxes into Power
    __declspec(noinline) void Reset();                               // 0x1007c570 everything back to nothing
    void SetPower(INT P1, INT P2, const BYTE* NewPower);             // 0x1007d2e0 (ret 0xC; the first two are ignored)
    void SetMember(INT InMemberID, const TCHAR* InName, INT InMode); // 0x10047610 (name ours) whose privileges (the member's id, name, 1 view / 2 edit)
};
NW_ASSERT_OFFSET(NCEmpowerWnd, Checks, 0xC8);
NW_ASSERT_OFFSET(NCEmpowerWnd, OKButton, 0xFC);
NW_ASSERT_OFFSET(NCEmpowerWnd, BackTex, 0x104);
NW_ASSERT_OFFSET(NCEmpowerWnd, Power, 0x108);
NW_ASSERT_OFFSET(NCEmpowerWnd, MemberID, 0x128);
NW_ASSERT_OFFSET(NCEmpowerWnd, Name, 0x130);
NW_ASSERT_SIZE(NCEmpowerWnd, 0x13C);

#pragma pack(pop)
