#pragma once
// AddedConfirmDialog - the box that tells someone has added us to their contact list and asks what to do (UClass
// "AddedConfirmDialog", 0x1FC; C3 only, made by MsnWnd::OnCreate with the protocol object as its owner). The contacts
// that added us queue up in the box (AddBuddy, each account once); it always asks about the first one: two radio-like
// boxes (allow / block), a third one (also add the contact to our list) and OK / Cancel. OK answers the first contact
// (the allow box decides) and drops it from the queue, Cancel just drops it; the box closes when the queue is empty
// (closing it by other means forgets the whole queue).

#include "nwindow/NMsnDlgCommon.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NMsnBuddy.h"
#pragma pack(push, 4)

enum
{
    NID_ADDEDCONFIRM_ALLOW = 0xA30,    // names ours
    NID_ADDEDCONFIRM_BLOCK = 0xA31,
    NID_ADDEDCONFIRM_ADDTOLIST = 0xA32,
    NID_ADDEDCONFIRM_OK = 0xA1E,
    NID_ADDEDCONFIRM_CANCEL = 0xA1F,
};

class AddedConfirmDialog : public NCDialogBox
{
    NW_DECLARE_CLASS(AddedConfirmDialog, NCDialogBox, 0, 0x102BE4C0)

    NMsnLink* Msn;                   // +0x1D4 the protocol object (constructor argument)
    TArray<NMsnBuddy*> Buddies;      // +0x1D8 the contacts waiting for an answer (heap records, freed with the queue)
    NCCheckBox* AllowRadio;          // +0x1E4 id 0xA30, 300 x 12 at (44, 50): drawn with the radio button textures
    NCCheckBox* BlockRadio;          // +0x1E8 id 0xA31, at (44, 69): the same
    NCCheckBox* AddToListCheck;      // +0x1EC id 0xA32, 200 x 12 at (13, 112): ordinary check box
    NCButton* OKButton;              // +0x1F0 id 0xA1E, 76 x 23 at (132, 136)
    NCButton* CancelButton;          // +0x1F4 id 0xA1F, at (212, 136)
    UTexture* BackTex;               // +0x1F8 "L2UI_CH3.Msn.msn_optionback3"

    AddedConfirmDialog(NMsnLink* InMsn = NULL);                      // 0x10123BB0
    // ~AddedConfirmDialog                                           // 0x10123C40

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10123600
    virtual INT OnCreate();                                          // 37 0x10122280
    virtual INT OnDestroy();                                         // 38 0x10120D10
    virtual void ShowWindow();                                       // 44 0x10121080
    virtual void HideWindow();                                       // 45 0x10122680 the queue forgotten
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10120E00

    // names ours
    void AddBuddy(NMsnBuddy Buddy);                                  // 0x10122880 (ret 0x44) queued unless its account (+0x20) is there already
    __declspec(noinline) INT OnSelectAllow();                                             // 0x101210C0 allow on, block off, add-to-list on (ids 0xA30, 0xA32)
    __declspec(noinline) INT OnSelectBlock();                                             // 0x101210F0 allow off, block on, add-to-list off (id 0xA31)
    __declspec(noinline) INT OnOK();                                                      // 0x101226F0 the first contact answered and dropped
    __declspec(noinline) INT OnCancel();                                                  // 0x101227E0 the first contact dropped
};
NW_ASSERT_OFFSET(AddedConfirmDialog, Msn, 0x1D4);
NW_ASSERT_OFFSET(AddedConfirmDialog, Buddies, 0x1D8);
NW_ASSERT_OFFSET(AddedConfirmDialog, AllowRadio, 0x1E4);
NW_ASSERT_OFFSET(AddedConfirmDialog, BlockRadio, 0x1E8);
NW_ASSERT_OFFSET(AddedConfirmDialog, AddToListCheck, 0x1EC);
NW_ASSERT_OFFSET(AddedConfirmDialog, OKButton, 0x1F0);
NW_ASSERT_OFFSET(AddedConfirmDialog, CancelButton, 0x1F4);
NW_ASSERT_OFFSET(AddedConfirmDialog, BackTex, 0x1F8);
NW_ASSERT_SIZE(AddedConfirmDialog, 0x1FC);

#pragma pack(pop)
