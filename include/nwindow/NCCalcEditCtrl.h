#pragma once
// NCCalcEditCtrl - the display and the keyboard of the calculator window (UClass "CCalcEditCtrl",
// 0x278; C2 only): a digits-only edit box whose characters drive a four-function calculator.
// It holds the first operand (Accumulator, a double), the operator and a state:
//   0 typing the first number, 1 cleared (the display shows 0), 2 an operator was pressed (the
//   next digit starts the second number; "=" repeats the operation on the display), 3 typing the
//   second number. Numbers are read with appAtof (a float) and shown with "%.0f"; a result below 0
//   or above 1e20 of a sum or a product clears the calculator.

#include "nwindow/NCEditBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCCalcEditCtrl : public NCEditBox
{
    NW_DECLARE_CLASS(NCCalcEditCtrl, NCEditBox, 0, 0x102B59A8)

    DOUBLE Accumulator;              // +0x2A4 the first operand (name ours)
    INT Operator;                    // +0x2AC -1 none, 0 +, 1 -, 2 *, 3 / (name ours)
    INT State;                       // +0x2B0 see above (name ours)

    NCCalcEditCtrl();                                                // 0x1010ab40
    // ~NCCalcEditCtrl                                               // 0x1010abc0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10109a40
    virtual INT OnChar(WPARAM wParam, LPARAM lParam);                // 58 0x1010a6b0

    // names ours; out of line like the stock functions (the callers call them)
    __declspec(noinline) void SetAccumulator(const TCHAR* Text);                          // 0x10109a60 the number in Text (none: 0)
    __declspec(noinline) void SetOperator(INT Op);                                        // 0x10109aa0
    __declspec(noinline) DOUBLE GetCurrent();                                             // 0x10109ab0 the number on the display (empty: 0)
    __declspec(noinline) void ShowNumber(DOUBLE Value);                                   // 0x10109b10 the display shows Value, "%.0f"
    __declspec(noinline) DOUBLE Compute(DOUBLE A, DOUBLE B, INT Op);                      // 0x10109b60 (ignores `this`)
    __declspec(noinline) void ShowZero();                                                 // 0x10109be0 the display shows "0"
    __declspec(noinline) void Reset();                                                    // 0x1010a020 operands and operator cleared, display 0
    __declspec(noinline) UBOOL IsOverflow(DOUBLE A, DOUBLE B, INT Op, DOUBLE Result);     // 0x1010a060 (a sum or a product of
                                                                     //   positive numbers that is negative or past 1e20 clears everything: 1)
};
NW_ASSERT_OFFSET(NCCalcEditCtrl, Accumulator, 0x2A4);
NW_ASSERT_OFFSET(NCCalcEditCtrl, Operator, 0x2AC);
NW_ASSERT_OFFSET(NCCalcEditCtrl, State, 0x2B0);
NW_ASSERT_SIZE(NCCalcEditCtrl, 0x2B4);

#pragma pack(pop)
