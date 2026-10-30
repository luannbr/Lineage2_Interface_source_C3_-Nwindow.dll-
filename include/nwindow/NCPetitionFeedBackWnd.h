#pragma once
// NCPetitionFeedBackWnd - the form that asks how a petition went (UClass "CPetitionFeedBackWnd", 0xE8; C3 only, a top
// window the console makes at +0x397C (NConsoleWnd::WarningDialogBox) and NCPetitionWnd's "new petition" button
// shows): five check boxes (SysString 850 .. 854, one of them chosen at a time, the third by default), a text edit
// and OK (SysString 140). OK sends the chosen option and the text to the server and hides the window.
//
// Layout and virtuals from the stock constructor 0x10077A10, OnCreate 0x100D21B0, OnDestroy 0x100D1740, OnPaint
// 0x100D1890, the options' handlers 0x100D19B0 .. 0x100D1AF0 and OnSend 0x100D1B40.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCCheckBox;
class NCMultiEdit;

// NWM_COMMAND ids of the window's controls (names ours; the title bar is 0xFF00A6)
enum
{
    NID_PETFB_OPTION1 = 0xFF00A1,      // .. NID_PETFB_OPTION5 = 0xFF00A5: the five check boxes
    NID_PETFB_OPTION2 = 0xFF00A2,
    NID_PETFB_OPTION3 = 0xFF00A3,
    NID_PETFB_OPTION4 = 0xFF00A4,
    NID_PETFB_OPTION5 = 0xFF00A5,
    NID_PETFB_FRAME = 0xFF00A6,
    NID_PETFB_OK = 0xFF00A7,
    NID_PETFB_EDIT = 0xFF00A8,
};

class NCPetitionFeedBackWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetitionFeedBackWnd, NCWnd, 0, 0x1029E728)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.PetitionWnd.petition_fb_back" (name ours)
    NCButton* OKButton;              // +0xCC text 140 (name ours)
    NCMultiEdit* TextEdit;           // +0xD0 the comment (name ours)
    NCCheckBox* Options[5];          // +0xD4 .. +0xE4 the five answers (name ours); bChecked (+0x1F0) says which

    NCPetitionFeedBackWnd();                                         // 0x10077A10
    // ~NCPetitionFeedBackWnd                                        // 0x10077A60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100D0780
    virtual INT OnCreate();                                          // 37 0x100D21B0
    virtual INT OnDestroy();                                         // 38 0x100D1740 (no guard)
    virtual void HideWindow();                                       // 45 0x100D0870 the text emptied, option 3 chosen
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100D1890
    // names ours: option N is the only box checked
    virtual INT OnOption1();                                         // 68 0x100D19B0
    virtual INT OnOption2();                                         // 69 0x100D1A00
    virtual INT OnOption3();                                         // 70 0x100D1A50
    virtual INT OnOption4();                                         // 71 0x100D1AA0
    virtual INT OnOption5();                                         // 72 0x100D1AF0
    virtual INT OnSend();                                            // 73 0x100D1B40 (name ours) OK
};
NW_ASSERT_OFFSET(NCPetitionFeedBackWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCPetitionFeedBackWnd, TextEdit, 0xD0);
NW_ASSERT_OFFSET(NCPetitionFeedBackWnd, Options, 0xD4);
NW_ASSERT_SIZE(NCPetitionFeedBackWnd, 0xE8);

#pragma pack(pop)
