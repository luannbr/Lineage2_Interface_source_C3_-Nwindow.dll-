/*=============================================================================
    AddedConfirmDialog.cpp - the box that tells someone has added us to their contact list (C3 only). Stock:
    0x10120D10 .. 0x10121079 (OnDestroy, OnPaint), ShowWindow 0x10121080, 0x101210C0, 0x101210F0, OnCreate 0x10122280,
    HideWindow 0x10122680, 0x101226F0, 0x101227E0, AddBuddy 0x10122880, WindowProc 0x10123600, the constructor
    0x10123BB0 and the destructor 0x10123C40.
=============================================================================*/
#include "nwindow/AddedConfirmDialog.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/NCGDevice.h"

enum
{
    MSN_TEXT_ADDED_ASK = 1352,         // systemmsg lines (GL2GameData + 0x4F3BC, 0x4F3EC): "$s1 ($s2) has added you ..."
    MSN_TEXT_ADDED_NOTE = 1353,
    SYSSTR_ADDED_ALLOW = 1026,         // SysString lines of the three boxes (GL2GameData + 0x68AA4, 0x68AB0, 0x68ABC)
    SYSSTR_ADDED_BLOCK = 1027,
    SYSSTR_ADDED_ADDTOLIST = 1028,
};

// the radio button textures of the two boxes (the third is an ordinary check box); the stock function is
// NCCheckBoxTexturesView::SetTextures (NCPetitionFeedBackWnd.cpp hooks it), ret 0x10
static FORCEINLINE void SetRadioTextures(NCCheckBox* Box)
{
#if NW_HOOK_MODE
    ::nw::StockCallAt<void>(0x10004650, Box, TEXT("L2UI_CH3.RadioCtrl.radiobutton2"),
                            TEXT("L2UI_CH3.RadioCtrl.radiobutton1"), (const TCHAR*)NULL, (const TCHAR*)NULL);
#else
    // the standalone: that body (0x10004650; NCPetitionFeedBackWnd.cpp's is an inline member there, which the standalone does not
    // emit): each texture given is looked up, the NULL ones stay
    Box->CheckedTex = Box->FindTexture(TEXT("L2UI_CH3.RadioCtrl.radiobutton2"));
    Box->UncheckedTex = Box->FindTexture(TEXT("L2UI_CH3.RadioCtrl.radiobutton1"));
#endif
}

typedef TArray<NMsnBuddy*> NMsnBuddyArray;

// 0x10123570 - TArray<pointer>'s destructor, the instance the three group dialogs' ShowWindow and this dialog's
// destructor call: Remove(0, ArrayNum) with its checks, then ~FArray
NW_HOOK_DTOR(0x10123570, NMsnBuddyArray)

// 0x10121b20 (ret 4) - TArray<pointer>'s copy constructor (the group dialogs copy the protocol object's array of groups)
NW_HOOK_CTOR_ARGS(0x10121b20, NMsnBuddyArray, (const NMsnBuddyArray& A), (A))

// 0x10123bb0 - the base constructor, the protocol object kept, the queue empty, the controls and the texture none
AddedConfirmDialog::AddedConfirmDialog(NMsnLink* InMsn)
    : Msn(InMsn), AllowRadio(NULL), BlockRadio(NULL), AddToListCheck(NULL), OKButton(NULL), CancelButton(NULL),
      BackTex(NULL)
{
}
NW_HOOK_CTOR_ARGS(0x10123bb0, AddedConfirmDialog, (NMsnLink* A), (A))

// the messenger window makes the box through this (MsnWnd.cpp cannot include the dialogs' headers: see MsnWnd.h)
NCWnd* MsnNewAddedConfirmDialog(void* Link) { return new AddedConfirmDialog((NMsnLink*)Link); }

// 0x10123c40 - the queue goes (0x10123570), then NCDialogBox
NW_HOOK_DTOR(0x10123c40, AddedConfirmDialog)

// 0x10123600 (vslot 25) - the three boxes set each other, OK answers the first contact, Cancel drops it
INT AddedConfirmDialog::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCDialogBox::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_ADDEDCONFIRM_ALLOW)
            return OnSelectAllow() != 0;
        if ((INT)wParam == NID_ADDEDCONFIRM_BLOCK)
            return OnSelectBlock() != 0;
        if ((INT)wParam == NID_ADDEDCONFIRM_ADDTOLIST)
            return OnSelectAllow() != 0;
        if ((INT)wParam == NID_ADDEDCONFIRM_OK)
            return OnOK() != 0;
        if ((INT)wParam == NID_ADDEDCONFIRM_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10123600, AddedConfirmDialog, WindowProc)

// 0x101210c0 (name ours) - allow on, block off, add-to-list on
INT AddedConfirmDialog::OnSelectAllow()
{
    AllowRadio->bChecked = 1;
    BlockRadio->bChecked = 0;
    AddToListCheck->bChecked = 1;
    return 1;
}
NW_HOOK_METHOD(0x101210c0, AddedConfirmDialog, OnSelectAllow)

// 0x101210f0 (name ours) - allow off, block on, add-to-list off
INT AddedConfirmDialog::OnSelectBlock()
{
    AllowRadio->bChecked = 0;
    BlockRadio->bChecked = 1;
    AddToListCheck->bChecked = 0;
    return 1;
}
NW_HOOK_METHOD(0x101210f0, AddedConfirmDialog, OnSelectBlock)

// 0x101226f0 (name ours) - the first contact of the queue, when there is one, is answered through the protocol object (its
// account, allowed when the allow box is on) and freed; it leaves the queue, and the box closes when the queue is empty
INT AddedConfirmDialog::OnOK()
{
    if (Buddies.Num() > 0)
    {
        NMsnBuddy* Buddy = Buddies(0);
        if (Buddy)
        {
            Msn->RequestAddFriend(FString(Buddy->Text20()), AllowRadio->bChecked ? 1 : 0);
            delete Buddy;
        }
        Buddies.Remove(0);
    }
    if (Buddies.Num() == 0)
        HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x101226f0, AddedConfirmDialog, OnOK)

// 0x101227e0 (name ours) - the first contact of the queue is freed without an answer and leaves the queue; the box closes
// when the queue is empty
INT AddedConfirmDialog::OnCancel()
{
    if (Buddies.Num() > 0)
    {
        NMsnBuddy* Buddy = Buddies(0);
        if (Buddy)
            delete Buddy;
        Buddies.Remove(0);
    }
    if (Buddies.Num() == 0)
        HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x101227e0, AddedConfirmDialog, OnCancel)

// 0x10122880 (name ours, ret 0x44) - a contact whose account (+0x20) is in the queue already is dropped; any other is queued
// as a copy on the heap (the argument is the caller's by-value copy, destroyed here). Inline: the hook forwarder holds it
__forceinline void AddedConfirmDialog::AddBuddy(NMsnBuddy Buddy)
{
    for (INT i = 0; i < Buddies.Num(); i++)
    {
        if (Buddies(i) && Buddies(i)->Text20() == Buddy.Text20())
            return;
    }
    Buddies.AddItem(new NMsnBuddy(Buddy));
}
NW_HOOK_METHOD(0x10122880, AddedConfirmDialog, AddBuddy)

// the messenger window queues a person through this (MsnWnd.cpp cannot include this header: see MsnWnd.h): a copy of the record goes in the
// argument (NMsnBuddy's copy constructor, 0x1012C460), which AddBuddy frees
void MsnAddedConfirmAddBuddy(NCWnd* Dialog, const NMsnPersonRec& Person)
{
    ((AddedConfirmDialog*)Dialog)->AddBuddy(*(const NMsnBuddy*)&Person);
}

// 0x10121080 (vslot 44) - shown with the answer defaults: allow and add-to-list on, block off
void AddedConfirmDialog::ShowWindow()
{
    NCWnd::ShowWindow();
    AllowRadio->bChecked = 1;
    BlockRadio->bChecked = 0;
    AddToListCheck->bChecked = 1;
}
NW_HOOK_METHOD(0x10121080, AddedConfirmDialog, ShowWindow)

// 0x10122680 (vslot 45) - NCDialogBox's (a modal box stays), then every contact still queued is freed and the queue emptied
void AddedConfirmDialog::HideWindow()
{
    NCDialogBox::HideWindow();
    for (INT i = 0; i < Buddies.Num(); i++)
    {
        NMsnBuddy* Buddy = Buddies(i);
        if (Buddy)
            delete Buddy;
    }
    Buddies.Empty();
}
NW_HOOK_METHOD(0x10122680, AddedConfirmDialog, HideWindow)

// 0x10122280 (vslot 37) - the back texture, the allow and block boxes (300 x 12 at (44, 50) and (44, 69), radio textures), the
// add-to-list box (200 x 12 at (13, 112)), OK (132, 136) and Cancel (212, 136)
INT AddedConfirmDialog::OnCreate()
{
    NCDialogBox::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.Msn.msn_optionback3"));

    AllowRadio = new NCCheckBox(*GL2GameData.SysString[SYSSTR_ADDED_ALLOW], 0, 0, 0);
    AllowRadio->ID = NID_ADDEDCONFIRM_ALLOW;
    AllowRadio->SetPosition(44, 50, 300, 12);
    CreateChild(AllowRadio, NWS_VISIBLE);
    SetRadioTextures(AllowRadio);

    BlockRadio = new NCCheckBox(*GL2GameData.SysString[SYSSTR_ADDED_BLOCK], 0, 0, 0);
    BlockRadio->ID = NID_ADDEDCONFIRM_BLOCK;
    BlockRadio->SetPosition(44, 69, 300, 12);
    CreateChild(BlockRadio, NWS_VISIBLE);
    SetRadioTextures(BlockRadio);

    AddToListCheck = new NCCheckBox(*GL2GameData.SysString[SYSSTR_ADDED_ADDTOLIST], 0, 0, 0);
    AddToListCheck->ID = NID_ADDEDCONFIRM_ADDTOLIST;
    AddToListCheck->SetPosition(13, 112, 200, 12);
    CreateChild(AddToListCheck, NWS_VISIBLE);

    OKButton = NewDlgButton(this, NID_ADDEDCONFIRM_OK, 132, 136, 140);
    CancelButton = NewDlgButton(this, NID_ADDEDCONFIRM_CANCEL, 212, 136, 141);
    return 1;
}
NW_HOOK_METHOD(0x10122280, AddedConfirmDialog, OnCreate)

// 0x10120d10 (vslot 38) - the three boxes, OK and Cancel go, then NCDialogBox
INT AddedConfirmDialog::OnDestroy()
{
    DestroyDlgChild(this, AllowRadio);
    DestroyDlgChild(this, BlockRadio);
    DestroyDlgChild(this, AddToListCheck);
    DestroyDlgChild(this, OKButton);
    DestroyDlgChild(this, CancelButton);
    NCDialogBox::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10120d10, AddedConfirmDialog, OnDestroy)

// 0x10120e00 (vslot 56) - the back texture over the box and, while somebody is queued, the question about the first one at
// (46, 16) ($s1 the first contact's decoded name, $s2 its account) and the note at (13, 90)
INT AddedConfirmDialog::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    if (Buddies.Num() > 0)
    {
        TCHAR Text[260];
        FString Question(MsnDlgText(MSN_TEXT_ADDED_ASK));
        FString Name;
        Buddies(0)->GetDecodedName(&Name);
        Question = Question.Substitute(TEXT("$s1"), Name);
        Question = Question.Substitute(TEXT("$s2"), Buddies(0)->Text20());
        appSprintf(Text, TEXT("%s"), *Question);
        Device->DrawText(46, 16, 0xFFDCDCDC, Text, (INT)(Width - 46.0f), 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        Device->DrawText(13, 90, 0xFFDCDCDC, MsnDlgText(MSN_TEXT_ADDED_NOTE), (INT)(Width - 22.0f), 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10120e00, AddedConfirmDialog, OnPaint)

NW_CHECK_VTABLE(AddedConfirmDialog, 0x101A8100, 69)
