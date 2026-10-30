/*=============================================================================
    InviteWnd.cpp - the invite window of a conversation (C3 only; C3 addresses). Stock: 0x1011a510 .. 0x1011a6ef
    (OnDestroy, OnPaint, the invitation), 0x1011b600 (the double click), 0x1011c610 (WindowProc), 0x1011d5f0 ..
    0x1011d6c0 (constructor, destructor), OnCreate 0x1011d6c0 .. 0x1011d9ab and the invitation's command 0x1012e6b0.
=============================================================================*/
#include "nwindow/InviteWnd.h"
#include "nwindow/InviteBox.h"
#include "nwindow/NChatTypes.h"
#include "nwindow/NChatUtil.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/MsnUiStockP.h"

enum
{
    NID_INVITE_OK = 0xA0F,
    NID_INVITE_CANCEL = 0xA10,
    NID_INVITE_FRAME = 0xA16,
};

// The conversation's SwitchboardSession (the Msn lanes' class) as far as this window knows it: the invitation (the CAL
// command, 0x1012e6b0; ret 0xC; NChatSbSender::SendInvite of ChatWnd.cpp) takes the friend's account by value and the callee
// destroys its copy

// 0x1011d5f0 (ret 8) - the base window's constructor, the two values kept, the four windows / texture empty
InviteWnd::InviteWnd(void* InMsn, void* InSwitchboard)
{
    Msn = InMsn;
    FriendList = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    BackTex = NULL;
    Switchboard = InSwitchboard;
}
NW_HOOK_CTOR_ARGS(0x1011D5F0, InviteWnd, (void* A, void* B), (A, B))

// 0x1011d670
NW_HOOK_DTOR(0x1011D670, InviteWnd)

// 0x1011c610 (vslot 25) - the double click on a friend and the two buttons
INT InviteWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnDblClick(wParam, lParam) != 0;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_INVITE_OK)
            return OnInvite() != 0;
        if (wParam == NID_INVITE_CANCEL)
            return ((NCOptionWnd*)(NCWnd*)this)->NCOptionWnd::OnCancel() != 0;   // 0x1007BEE0, NCOptionWnd::OnCancel (folded: it hides the window, whatever the class)
    }
    return 0;
}
NW_HOOK_METHOD(0x1011C610, InviteWnd, WindowProc)

// 0x1011a690 (name ours) - the friend selected in the list (its record is the line's Param) is invited through the
// conversation's session, whatever the window; then the window is hidden
INT InviteWnd::OnInvite()
{
    INT Sel = FriendList->Selected;
    if (Sel >= 0 && Sel < FriendList->NumLines)
    {
        BYTE* Friend = (BYTE*)FriendList->GetItemParam(Sel);
        if (Friend && Switchboard)
            ((NChatSbSender*)Switchboard)->SendInvite(::nw::Field<FString>(Friend, 0x20));   // 0x1012E6B0 (the account is copied into the argument)
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1011A690, InviteWnd, OnInvite)

// 0x1011b600 (name ours, ret 8) - a double click invites too
INT InviteWnd::OnDblClick(WPARAM wParam, LPARAM lParam)
{
    return OnInvite();
}
NW_HOOK_METHOD(0x1011B600, InviteWnd, OnDblClick)

// 0x1011ede0 (name ours) - the list emptied; with an Msn object and a switchboard: a copy of the switchboard's names (+0x80), and
// each contact of the Msn object's list (Msn +0x50 -> a list whose pointers are at +4, their count at +8) that is not among
// them gets a line (its decoded name in the usual color, its status icon as in the roster; offline ones are left out)
void InviteWnd::FillFriends()
{
    FriendList->Clear();
    if (Msn && Switchboard)
    {
        BYTE* Contacts = *(BYTE**)(*(BYTE**)((BYTE*)Msn + 0x50));
        BYTE NamesStorage[sizeof(NChatNameMap)];
        NChatNameMap* Inside = (NChatNameMap*)NamesStorage;
        ((NMsnNameMapCopier*)Inside)->CopyFrom((const NChatNameMap*)((BYTE*)Switchboard + 0x80));   // the map copied, 0x1011D1E0
        for (INT i = 0; i < *(INT*)(Contacts + 8); i++)
        {
            NChatUser* Friend = (*(NChatUser***)(Contacts + 4))[i];
            if (Friend && !((NMsnWndStrMap*)Inside)->FindWnd(&Friend->Account))     // 0x101347D0: the map's lookup
            {
                FString& Status = Friend->Status;
                if (Status == TEXT("NLN"))
                    FriendList->AddFriend(FindTexture(TEXT("L2UI_CH3.Msn.msnicon1")), *Friend->GetDecodedName(), Friend, 0xFFDCDCDC);
                else if (Status == TEXT("BRB") || Status == TEXT("IDL") || Status == TEXT("AWY") || Status == TEXT("LUN"))
                    FriendList->AddFriend(FindTexture(TEXT("L2UI_CH3.Msn.msnicon2")), *Friend->GetDecodedName(), Friend, 0xFFDCDCDC);
                else if (Status == TEXT("BSY") || Status == TEXT("PHN"))
                    FriendList->AddFriend(FindTexture(TEXT("L2UI_CH3.Msn.msnicon3")), *Friend->GetDecodedName(), Friend, 0xFFDCDCDC);
            }
        }
        Inside->~NChatNameMap();
    }
}
NW_HOOK_METHOD(0x1011EDE0, InviteWnd, FillFriends)

// 0x1011d6c0 (vslot 37) - the back texture, the title bar (id 0xA16, as wide as the window), the friends' list, OK and
// Cancel
INT InviteWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.Msn.msn_optionback"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_INVITE_FRAME;
    FrameCtrl->ID = NID_INVITE_FRAME;
    FrameCtrl->SetPosition(0, 0, 0x154, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    FriendList = new InviteBox(300, 10, 0xF);
    FriendList->SetPosition(9, 0x2F, 0x142, 0xB1);
    CreateChild(FriendList, NWS_VISIBLE);
    FriendList->LineGap = 5;

    OKButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    OKButton->ID = NID_INVITE_OK;
    OKButton->SetPosition(0x5D, 0xE8, 0x4C, 0x17);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetText(*GL2GameData.SysString[140]);

    CancelButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    CancelButton->ID = NID_INVITE_CANCEL;
    CancelButton->SetPosition(0xAD, 0xE8, 0x4C, 0x17);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetText(*GL2GameData.SysString[141]);
    return 1;
}
NW_HOOK_METHOD(0x1011D6C0, InviteWnd, OnCreate)

// 0x1011a510 (vslot 38) - the list and the two buttons go, then the base
INT InviteWnd::OnDestroy()
{
    NDestroyChild(this, FriendList);
    NDestroyChild(this, OKButton);
    NDestroyChild(this, CancelButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1011A510, InviteWnd, OnDestroy)

// 0x1011a5b0 (vslot 56) - the back texture under the title bar and the question at (11, 28)
INT InviteWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0x14, 0x154, 0xF0, 0, 0, 0x154, 0xF0, BackTex, 0xFF, 1);
    Device->DrawText(0xB, 0x1C, 0xFFDCDCDC, *GL2GameData.SystemMsg[1345].Message, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1011A5B0, InviteWnd, OnPaint)

NW_CHECK_VTABLE(InviteWnd, 0x101A7110, 68)
