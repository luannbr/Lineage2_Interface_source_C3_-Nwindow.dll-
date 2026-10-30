#pragma once
// NCTaiwanIME - the IME of the Taiwanese client (UClass "CTaiwanIME", 0x620C). The composition
// is read as the Korean IME reads it, but only with a Taiwanese IME (see IsTaiwanIME); the
// candidate list goes into an NCCandidateBox as the Japanese IME's does, laid out by the IME
// type the console reports. The reading string (ReadingStr, NIM_READING) comes from the IME's
// GetReadingString export or, for the old Microsoft IMEs without it, from the IME's private
// data (IMN_PRIVATE).

#include "nwindow/NCIME.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// the IME's GetReadingString (NCTaiwanIME::LoadReadingString; name ours)
#if NW_HOOK_MODE
#define GTaiwanGetReadingString NW_GLOBAL(NGetReadingString, 0x1026A080)
#else
extern NGetReadingString GTaiwanGetReadingString;
#endif

class NCTaiwanIME : public NCIME
{
    NW_DECLARE_CLASS(NCTaiwanIME, NCIME, 0, 0x1026BF78)

    TArray<NImeText> Clauses;        // +0x422C as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4238 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x423C
    BYTE Unknown420C[0x2000];        // +0x4240 not used   // C2: offset derived, no evidence   // C2: offset derived, no evidence

    NCTaiwanIME(INT InMode = 0);                                     // 0x10018120 (C1 only: no C2 match)
    // ~NCTaiwanIME                                                  // 0x10018190 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10014790
    virtual INT OnCreate();                                          // 37 0x10018330
    virtual INT OnDestroy();                                         // 38 0x10018400 (shared)
    virtual void ShowWindow();                                       // 44 0x100146f0 (shared)
    virtual void Deactivate();                                       // 70 0x10014e20
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 71 0x1001ab80
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam);       // 74 0x10014d50
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 75 0x100184f0
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 76 0x1000bd60 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 80 0x10015b20

    // names ours
    __declspec(noinline) void FreeCandList();                        // 0x10014710 (shared by the three IMEs; the stock callers call it)
    void FillCandidateBox();                                         // 0x100159b0 (shared)
    void ReadCandList();                                             // 0x100193f0
    void ReadReadingString();                                        // 0x10014a90
    UBOOL IsTaiwanIME();                                             // 0x10014cb0 (this unused)
    static void LoadReadingString();                                 // 0x10014870
    static DWORD GetImeId();                                         // 0x10014920 the old IMEs'
                                                                     //   version, 0 for the others
};
NW_ASSERT_OFFSET(NCTaiwanIME, Clauses, 0x422C);
NW_ASSERT_OFFSET(NCTaiwanIME, CandidateBox, 0x423C);
NW_ASSERT_SIZE(NCTaiwanIME, 0x6240);

#pragma pack(pop)