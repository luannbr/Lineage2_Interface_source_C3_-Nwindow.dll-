#pragma once
// NCCountSpin - a number with two small buttons at its right that count it up and down between
// a minimum and a maximum (UClass "CCountSpin", 0x3E4). No window of NWindow.dll creates one;
// only the class's InternalConstructor builds it (0 .. 100, no label). A label is drawn first,
// the number after it; each step tells the parent (NWM_COMMAND, 1 up / 2 down), which takes it
// back by setting bCancelStep meanwhile.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPushButton;

class NCCountSpin : public NControl
{
    NW_DECLARE_CLASS(NCCountSpin, NControl, 0, 0x102642B0)

    TCHAR ValueText[16];             // +0x1D4 the value in decimal
    FString Label;                   // +0x1F4 drawn before it (C2: an FString, C1: TCHAR Label[256]); the
                                     //   constructor's default is "Default"
    INT TextX;                       // +0x200 (5)
    INT TextY;                       // +0x204 (5)
    UTexture* BackTex;               // +0x208 "NWindow.EditBack", over the whole control
    INT Min;                         // +0x20C
    INT Max;                         // +0x210
    INT Value;                       // +0x214
    NCPushButton* UpButton;          // +0x218
    NCPushButton* DownButton;        // +0x21C
    UBOOL bCancelStep;               // +0x220 (name ours) set by the parent while it is told of
                                     //   a step: the step is undone

    NCCountSpin(INT InMin = 0, INT InMax = 100, const TCHAR* InLabel = NULL);         // 0x10005c00 (C1 only: no C2 match)
    // ~NCCountSpin                                                  // 0x10005d20 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10005e60
    virtual INT OnCreate();                                          // 37 0x10006420
    virtual INT OnDestroy();                                         // 38 0x10006170
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10006020
    // names ours
    virtual INT OnUp();                                              // 69 0x10005ee0
    virtual INT OnDown();                                            // 70 0x10005f40
};
NW_ASSERT_OFFSET(NCCountSpin, ValueText, 0x1D4);
NW_ASSERT_OFFSET(NCCountSpin, Label, 0x1F4);
NW_ASSERT_OFFSET(NCCountSpin, TextX, 0x200);
NW_ASSERT_OFFSET(NCCountSpin, Min, 0x20C);
NW_ASSERT_OFFSET(NCCountSpin, UpButton, 0x218);
NW_ASSERT_SIZE(NCCountSpin, 0x224);

#pragma pack(pop)