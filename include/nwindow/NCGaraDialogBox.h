#pragma once
// NCGaraDialogBox - the desktop's message box (UClass "CGaraDialogBox", 0x23C; C2 only: C1's
// CDialogBox, which kept its name in the class table of the C2 build for the thin 0x1A4 base of the
// warning boxes, NCDialogBox.h): a message, OK / Cancel, and optionally an edit line (with a
// keypad when it takes digits) and a progress bar that cancels the box when it ends.
// NConsoleWnd::ShowDialog fills it from an NDialogInfo (SetInfo). What OK and Cancel do depends
// on DialogID: answer the owner window (amounts and yes / no, messages 0xF00000xx), or answer
// the server directly (invitations, wars, trade, revival...), checking names first where one is
// typed. The console keeps two of them (MainDialogBox, SecondDialogBox) and the other windows
// reach them through an NCDialogBox* (their functions that C1 had on NCDialogBox are stubs there).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCEditBox;
class NCProgressCtrl;
struct NDialogInfo;

// the keypad's textures: pairs of names (up, down), 13 of them, in the stock image (C2: 0x1014C110)
#if NW_HOOK_MODE
#define GKeypadTextures (reinterpret_cast<const TCHAR* const (*)[2]>(::nw::Addr(0x101DF9F0)))// C3: 13 pairs of pointers (OnCreate 0x10009920)
#else
extern const TCHAR* const GKeypadTextures[][2];
#endif

class NCGaraDialogBox : public NControl
{
    NW_DECLARE_CLASS(NCGaraDialogBox, NControl, 0, 0x10265748)

    UTexture* BackTex;               // +0x1D4 "L2UI_ch3.dialog.system_back" (C3)
    UTexture* KeypadBackTex;         // +0x1D8 "L2UI_ch3.Calculate.Calculate1_back": the keypad's panel at x 256 (C3; was InputBackTex)
    UTexture* IconTex;               // +0x1DC "L2UI_ch3.dialog.warningicon" at (11, 15) (C3; was KeypadTex)
    NCButton* OKButton;              // +0x1E0
    NCButton* CancelButton;          // +0x1E4
    NCButton* Keypad[13];            // +0x1E8 0..8 = 1..9, 9 = 0, 10 = backspace, 11 = the number of Unknown238, 12 = 0
    NCProgressCtrl* Progress;        // +0x21C
    NCEditBox* EditBox;              // +0x220
    INT DialogID;                    // +0x224 (-1 = none)
    INT Type;                        // +0x228 edit line type (3: digits, the keypad shows)
    NWndEntry* Owner;                // +0x22C window that gets the answer
    UBOOL bInput;                    // +0x230 the edit line shows
    UBOOL bLargeAmount;              // +0x234 amounts up to 100,000,000 (else 1,000,000)   // C2: offset derived, no evidence
    INT Flag238;                     // +0x238 (new in C3, name ours): NDialogInfo +0x18, cleared by Reset
    UBOOL bProgress;                 // +0x23C the progress bar runs
    FString Message;                 // +0x240
    FString ParamText;               // +0x24C a name some answers carry
    INT Param1;                      // +0x258 numbers some answers carry
    INT Param2;                      // +0x25C
    INT Unknown22C[3];               // +0x260 a place (three floats): SetInfo copies the NDialogInfo's +0x78..+0x83 here;   // C2: offset derived, no evidence
                                     //   dialog 4 (drop an item) hands it to RequestDropItem
    FString ShowSoundA;              // +0x26C (new in C3, name ours): NDialogInfo +0x88; ShowWindow plays it (NConsoleWnd 0x10060ac0)
    FString ShowSoundB;              // +0x278 (new in C3, name ours): NDialogInfo +0x94; ShowWindow plays it (NConsoleWnd 0x10060b00)
    INT Unknown238;                  // +0x284 a number the keypad's key 11 types; the NDialogInfo's +0x74

    NCGaraDialogBox();                                               // 0x10009f80
    // ~NCGaraDialogBox                                              // 0x10007090 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10006c40
    virtual void SetFocus();                                         // 29 0x10006d20
    virtual INT OnCreate();                                          // 37 0x10009920
    virtual INT OnDestroy();                                         // 38 0x100078b0
    virtual void ShowWindow();                                       // 44 0x10009d90
    virtual void HideWindow();                                       // 45 0x10006de0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100076f0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x10006d40
    // names ours (C1's OnOK .. OnProgressEnd, which kept their slots 68 .. 72 here)
    virtual INT OnOK();                                              // 69 0x10007d80
    virtual INT OnCancel();                                          // 70 0x10006ed0
    virtual INT OnEditNotify(INT Code);                              // 71 0x10006df0
    virtual INT OnKeypad(INT Key);                                   // 72 0x100095d0
    virtual INT OnProgressEnd();                                     // 73 0x100073c0 (the Cancel's)

    // names ours (the other windows' stubs on NCDialogBox reach these)
    __declspec(noinline) void Reset();                               // 0x10006d90 the answer's state cleared (ParamText and the params are kept)
    void SetButtonTexts(INT OKTextID, INT CancelTextID);             // 0x10007310 sysstring ids
    NWndEntry* GetOwner();                                           // 0x100073b0
    void UpdateLayout();                                             // 0x10007a20
    void SetInfo(NDialogInfo* Info);                                 // 0x10007be0
    void SetEditText(const TCHAR* Text);                             // 0x10007380 typed into the edit line (when it shows)
};
NW_ASSERT_OFFSET(NCGaraDialogBox, BackTex, 0x1D4);
NW_ASSERT_OFFSET(NCGaraDialogBox, OKButton, 0x1E0);
NW_ASSERT_OFFSET(NCGaraDialogBox, Keypad, 0x1E8);
NW_ASSERT_OFFSET(NCGaraDialogBox, Progress, 0x21C);
NW_ASSERT_OFFSET(NCGaraDialogBox, EditBox, 0x220);
NW_ASSERT_OFFSET(NCGaraDialogBox, DialogID, 0x224);
NW_ASSERT_OFFSET(NCGaraDialogBox, Owner, 0x22C);
NW_ASSERT_OFFSET(NCGaraDialogBox, bProgress, 0x23C);
NW_ASSERT_OFFSET(NCGaraDialogBox, Message, 0x240);
NW_ASSERT_OFFSET(NCGaraDialogBox, ParamText, 0x24C);
NW_ASSERT_OFFSET(NCGaraDialogBox, Param1, 0x258);
NW_ASSERT_OFFSET(NCGaraDialogBox, Flag238, 0x238);
NW_ASSERT_OFFSET(NCGaraDialogBox, Unknown22C, 0x260);
NW_ASSERT_OFFSET(NCGaraDialogBox, ShowSoundA, 0x26C);
NW_ASSERT_OFFSET(NCGaraDialogBox, ShowSoundB, 0x278);
NW_ASSERT_OFFSET(NCGaraDialogBox, Unknown238, 0x284);
NW_ASSERT_SIZE(NCGaraDialogBox, 0x288);

#pragma pack(pop)
