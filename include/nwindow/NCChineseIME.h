#pragma once
// NCChineseIME - the IME of the Chinese client (UClass "CChineseIME", 0x6224). The composition
// is read as the Japanese IME reads it, but only with a Chinese IME (see IsChineseIME); the
// candidate list goes into an NCCandidateBox (9 a page, 10 for the IME type 9). The IME types
// 11 and 12 (NConsoleWnd::GetIMEType; 11 is never returned) also show the letters typed in an
// NCCompositeBox, rebuilt from the keyboard state on IMN_PRIVATE, and 12 takes a candidate with
// the digit keys. The reading string (ReadingStr, NIM_READING) comes from the IME's
// GetReadingString export or, for the old Microsoft Pinyin IMEs without it, from the IME's
// private data (IMN_PRIVATE).

#include "nwindow/NCIME.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCCompositeBox;

// the IME's GetReadingString (NCChineseIME::LoadReadingString; name ours), and
// NCChineseIME::IsNT's function statics in the stock code (names ours)
#if NW_HOOK_MODE
#define GChineseGetReadingString NW_GLOBAL(NGetReadingString, 0x1026A084)
#define GChineseOSVersion NW_GLOBAL(OSVERSIONINFOA, 0x1026C4A8)
#define GChineseOSVersionRead NW_GLOBAL(bool, 0x1026C53C)
#define GChineseNT NW_GLOBAL(bool, 0x1026C53D)
#define GChineseNTChecked NW_GLOBAL(bool, 0x1026C53E)
#else
extern NGetReadingString GChineseGetReadingString;
extern OSVERSIONINFOA GChineseOSVersion;
extern bool GChineseOSVersionRead;
extern bool GChineseNT;
extern bool GChineseNTChecked;
#endif

class NCChineseIME : public NCIME
{
    NW_DECLARE_CLASS(NCChineseIME, NCIME, 0, 0x1026A5B8)

    TArray<NImeText> Clauses;        // +0x422C as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4238 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x423C
    NCCompositeBox* CompositeBox;    // +0x4240 letters typed (IME types 11 and 12)
    FString LastComp;                // +0x4244 the composition before the last one (IME type 10;
                                     //   the edit boxes move the box's caret by it)
    BYTE Unknown421C[0x2000];        // +0x4250 not used   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UBOOL bCandOpened;               // +0x6250 IME type 11: a candidate list opened since the
                                     //   last key (its changes are read only then)
    UBOOL bOpenResultSent;           // +0x6254 IME type 12: an opening candidate list sent a
                                     //   result (the next opening sends none)

    NCChineseIME(INT InMode = 0);                                    // 0x1001b880
    // ~NCChineseIME                                                 // 0x1001b930

    // WindowProc: the stock body is the Japanese IME's (0x1000ee70)
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10014f40
    virtual INT OnCreate();                                          // 37 0x100194e0
    virtual INT OnDestroy();                                         // 38 0x10019650
    virtual void ShowWindow();                                       // 44 0x10015150
    virtual void Activate();                                         // 69 0x10014f10
    virtual void Deactivate();                                       // 70 0x1001a8f0
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 71 0x1001ae20
    virtual INT OnImeEndComposition(WPARAM wParam, LPARAM lParam);   // 73 0x100152b0
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam) { return 1; }   // 74 0x1000bd60 (folded)
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 75 0x10019880
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 76 0x1000bd60 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 80 0x10016030

    // names ours
    __declspec(noinline) void FreeCandList();                        // 0x10014710 (shared by the three IMEs; the stock callers call it)
    void FillCandidateBox();                                         // 0x10015ea0
    void ReadCandList();                                             // 0x10019770
    void ReadReadingString();                                        // 0x10015c60
    UBOOL IsChineseIME();                                            // 0x100150a0 (this unused)
    static void LoadReadingString();                                 // 0x10015040
    static DWORD GetImeId();                                         // 0x10015180 the old IMEs'
                                                                     //   version, 0 for the others
    static bool IsNT();                                              // 0x100155f0
};
NW_ASSERT_OFFSET(NCChineseIME, Clauses, 0x422C);
NW_ASSERT_OFFSET(NCChineseIME, CompositeBox, 0x4240);
NW_ASSERT_OFFSET(NCChineseIME, LastComp, 0x4244);
NW_ASSERT_OFFSET(NCChineseIME, bCandOpened, 0x6250);
NW_ASSERT_SIZE(NCChineseIME, 0x6258);

#pragma pack(pop)