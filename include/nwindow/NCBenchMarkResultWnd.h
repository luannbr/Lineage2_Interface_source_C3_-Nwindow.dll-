#pragma once
// NCBenchMarkResultWnd - the result page of the client's benchmark (UClass "CBenchMarkResultWnd", 0xE0; new in C3:
// NCReplayWnd +0xD0, made by its OnCreate, shown when the replay manager reaches state 5): a full-screen backdrop
// ("BenchMark_Result"), the small logo, the score text the replay manager built ("<title> -- <detail>": the part
// before the " -- " in one place, the part after it in another) over the machine report (OS, CPU, video) and one OK
// button in the lower right corner that leaves the benchmark mode.
//
// Layout and virtuals from the stock constructor 0x100E7AA0, OnCreate 0x100E7560, OnPaint 0x100E7030 and the small
// methods 0x100E6810 .. 0x100E68A0 / 0x100E6F80.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCButton;

class NCBenchMarkResultWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBenchMarkResultWnd, NCWnd, 0, 0x102A7748)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.BenchMarkWnd.BenchMark_Result" (1024 x 768, stretched over the window)
    UTexture* MiniLogoTex;           // +0xCC localization.ini "<country>_MiniLogoTexture" (232 x 120 in the upper left corner)
    NCButton* OkButton;              // +0xD0 id 0xAFF22015, text SysString 140 (name ours), lower right corner
    FString Score;                   // +0xD4 (name ours) the replay manager's score text, set by ShowWindow

    NCBenchMarkResultWnd();                                          // 0x100E7AA0
    // ~NCBenchMarkResultWnd                                         // 0x100E7B30

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100E6810
    virtual INT OnCreate();                                          // 37 0x100E7560
    virtual INT OnDestroy();                                         // 38 0x100E6F80 (guard: NCReplayPlayWnd::OnDestroy)
    virtual void ShowWindow();                                       // 44 0x100E6870 the score text taken, then NCWnd's
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E7030
    virtual INT OnOK();                                              // 68 0x100E68A0 (name ours) leaves the benchmark
};
NW_ASSERT_OFFSET(NCBenchMarkResultWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCBenchMarkResultWnd, OkButton, 0xD0);
NW_ASSERT_OFFSET(NCBenchMarkResultWnd, Score, 0xD4);
NW_ASSERT_SIZE(NCBenchMarkResultWnd, 0xE0);

#pragma pack(pop)
