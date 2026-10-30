#pragma once
// NCJapaneseIME - the IME of the Japanese client (UClass "CJapaneseIME", 0x620C). The
// composition is read as the Korean IME reads it (clauses included) but only with a Japanese
// keyboard layout, and the cursor comes from the IME only while some characters are still
// being typed (attribute 0). The IME's candidate list is copied (CandList) and shown in an
// NCCandidateBox, 9 a page, placed at the caret the edit box reports (NIM_CARET).

#include "nwindow/NCIME.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCJapaneseIME : public NCIME
{
    NW_DECLARE_CLASS(NCJapaneseIME, NCIME, 0, 0x1026A098)

    TArray<NImeText> Clauses;        // +0x422C as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4238 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x423C
    BYTE Unknown420C[0x2000];        // +0x4240 not used   // C2: offset derived, no evidence   // C2: offset derived, no evidence

    NCJapaneseIME(INT InMode = 0);                                   // 0x10017fb0 (C1 only: no C2 match)
    // ~NCJapaneseIME                                                // 0x10018020 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10014f40 (shared)
    virtual INT OnCreate();                                          // 37 0x10017010
    virtual INT OnDestroy();                                         // 38 0x100170d0
    virtual void ShowWindow();                                       // 44 0x100146f0
    virtual void Activate();                                         // 69 0x10014760
    virtual void Deactivate();                                       // 70 0x10018230
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 71 0x1001aaa0
    virtual INT OnImeEndComposition(WPARAM wParam, LPARAM lParam);   // 73 0x10014740
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam) { return 1; } // 74 0x1000bd60 (folded)
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 75 0x10017290
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 76 0x1000bd60 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 80 0x10015920

    // names ours
    __declspec(noinline) void FreeCandList();                        // 0x10014710 (shared by the three IMEs; the stock callers call it)
    void FillCandidateBox();                                         // 0x100159b0 (shared)
    void ReadCandList();                                             // 0x100171c0
    UBOOL IsJapaneseLayout();                                        // 0x100146d0 (this unused)
};
NW_ASSERT_OFFSET(NCJapaneseIME, Clauses, 0x422C);
NW_ASSERT_OFFSET(NCJapaneseIME, CandidateBox, 0x423C);
NW_ASSERT_SIZE(NCJapaneseIME, 0x6240);

#pragma pack(pop)