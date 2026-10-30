#pragma once
// OptionWnd - the messenger's options window (UClass "COptionWnd", 0x104; C3 only, no C2 source; not the game's
// NCOptionWnd): the display name, the three group buttons (add, rename, remove), six check boxes (the alarms, the sound,
// "do not disturb", "hide all") and the OK and cancel buttons, under a title bar. The messenger window makes it as a child
// of the desktop (0x10139A40); the check boxes are saved to Option.ini ("Msn" section) by OK.
//
// Layout and virtuals from the stock constructor 0x10137960, OnCreate 0x10135510, OnDestroy 0x101325E0, OnPaint 0x10132810,
// WindowProc 0x10136670 and ShowWindow 0x10135E20.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnWnd;
class Msn;
class NCEditBox;
class NCButton;
class NCCheckBox;

class OptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(OptionWnd, NCWnd, 0, 0x102C3C50)

    MsnWnd* Owner;                   // +0xC8 the messenger window that made it (the constructor's first argument) (name ours)
    Msn* MsnObj;                     // +0xCC the protocol object (the second) (name ours)
    NCEditBox* NameEdit;             // +0xD0 id 0xA1A, the display name, 220 x 16 at (18, 52)
    NCButton* AddGroupButton;        // +0xD4 id 0xA1B, 76 x 23 at (16, 103), system string 984
    NCButton* RenameGroupButton;     // +0xD8 id 0xA1C, at (91, 103), system string 985
    NCButton* RemoveGroupButton;     // +0xDC id 0xA1D, at (166, 103), system string 986
    NCCheckBox* LoginAlarmBox;       // +0xE0 system string 987, 250 x 20 at (20, 156)
    NCCheckBox* MessageAlarmBox;     // +0xE4 988, at (20, 175)
    NCCheckBox* NewMailAlarmBox;     // +0xE8 989, at (20, 194)
    NCCheckBox* SoundAlarmBox;       // +0xEC 990, at (20, 213)
    NCCheckBox* DontDisturbBox;      // +0xF0 991, at (20, 232)
    NCCheckBox* HideAllBox;          // +0xF4 992, 250 x 30 at (20, 251), text parameter 200
    NCButton* OKButton;              // +0xF8 id 0xA17, 76 x 23 at (50, 289), system string 140
    NCButton* CancelButton;          // +0xFC id 0xA18, at (130, 289), system string 141
    UTexture* BackTex;               // +0x100 "L2UI_CH3.Msn.msn_optionback2"

    OptionWnd(MsnWnd* InOwner = NULL, Msn* InMsn = NULL); // 0x10137960 (ret 8); defaults = what the stock InternalConstructor pushes
    // ~OptionWnd                                                    // 0x10137A10

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10136670
    virtual INT OnCreate();                                          // 37 0x10135510
    virtual INT OnDestroy();                                         // 38 0x101325E0
    virtual void ShowWindow();                                       // 44 0x10135E20
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10132810

    // names ours (anonymous in the db)
    void LoadSettings();                                             // 0x10132940 the name and the six boxes from the stock's globals
    INT OnOKButton();                                                // 0x10132A40 saves the name and the six settings (Option.ini)
    __declspec(noinline) INT OnAddGroupButton();                                          // 0x10135E30 the owner shows its add-group dialog
    __declspec(noinline) INT OnRenameGroupButton();                                       // 0x10135E50
    __declspec(noinline) INT OnRemoveGroupButton();                                       // 0x10135E70
};
NW_ASSERT_OFFSET(OptionWnd, Owner, 0xC8);
NW_ASSERT_OFFSET(OptionWnd, MsnObj, 0xCC);
NW_ASSERT_OFFSET(OptionWnd, NameEdit, 0xD0);
NW_ASSERT_OFFSET(OptionWnd, AddGroupButton, 0xD4);
NW_ASSERT_OFFSET(OptionWnd, LoginAlarmBox, 0xE0);
NW_ASSERT_OFFSET(OptionWnd, HideAllBox, 0xF4);
NW_ASSERT_OFFSET(OptionWnd, OKButton, 0xF8);
NW_ASSERT_OFFSET(OptionWnd, BackTex, 0x100);
NW_ASSERT_SIZE(OptionWnd, 0x104);

#pragma pack(pop)
