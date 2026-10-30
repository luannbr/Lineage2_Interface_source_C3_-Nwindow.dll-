#pragma once
// MsnUiStockP.h - stage 5 (the standalone NWindow.dll), lane sa-msn-ui. New file. The part of MsnUiStock.h (read it first) that needs the
// protocol lane's headers (Msn.h, SwitchboardSession.h, ChatWnd.h, InviteWnd.h...): the stand-ins DEFINED in this lane's ChatWnd.cpp that
// derive from a protocol class. The protocol lane's own stand-ins come with MsnWndOps.h (MsnProtoOps.h), included here.

#include "nwindow/MsnUiStock.h"
#include "nwindow/MsnWndOps.h"
#include "nwindow/InviteWnd.h"
#pragma pack(push, 4)

// ChatWnd.cpp (this lane): the protocol object (Msn) as the conversation windows use it
struct NChatMsnRequester : public Msn
{
    __declspec(noinline) void RequestSession(FString Account);                // 0x1012A750 (ret 0xC)
    __declspec(noinline) void CloseSession(SwitchboardSession* Session);      // 0x1012A740 (ret 4)
    __declspec(noinline) NChatUser* GetMe();                                  // 0x1012B150 (Msn::GetMe is the same 4-byte body, not hooked)
};

// ChatWnd.cpp (this lane): InviteWnd's list emptied
struct NChatInviteWnd : public InviteWnd
{
    __declspec(noinline) void ClearList();   // 0x10119A90
};

#pragma pack(pop)
