#pragma once
// DispatchProcess: the first connection of the messenger (C3 only; vtable 0x101AB164, 0x64 bytes, no members of its own): to
// the dispatch server (messenger.hotmail.com:1863) it says VER and CVR, then USR ... TWN I, and the server answers with an XFR
// to the notification server it should use (ProcessXFR). Then the manager drops it (Msn::NotifyDispatchProcessEnd).

#include "nwindow/MSNSocket.h"

class DispatchProcess : public MSNSocket
{
public:
    // Hook build: the stock constructor's body (0x10140200, ret 0x18: the base's Construct, then the stock vtable 0x101AB164 written by
    // hand; MsnLogin.cpp). The destructor's body: the vtable (hook build), then the base's destruction
    void Destroy();                              // 0x10140280

    // the vtable's overrides (the stock's 0x101AB164: 0 = SendVER 0x1013EC60, 1 = OnClosedImpl 0x1013C260, 5 = OnReplyImpl 0x10140290; the
    // others are the base's). Defined in the standalone build only (the hook build's objects carry the stock vtable)
    virtual void OnConnected();
    virtual void OnClosed();
    virtual void OnReply(INT RequestType, MsnLine Line);
#if !NW_HOOK_MODE
    // the real constructor of the standalone build (the compiler writes the vtable); the hook build makes the object with the stock
    // constructor's body (MsnLogin.cpp)
    DispatchProcess(Msn* InOwner, const NMsnServer& InServer, INT InTrID);
#endif

    // the slot 1 of the vtable (the peer closed): the manager deletes this connection
    void OnClosedImpl();                         // 0x1013C260 (jump to Msn::NotifyDispatchProcessEnd of the owner)

    // the answer to the USR: "XFR trid NS host:port ...": a notification connection to that server
    void ProcessXFR(MsnLine Line);               // 0x1013C620 (ret 0x3C, guard name)

    // the slot 5 of the vtable (an answer by request type; the request type is the FIRST argument, the line the second): 1 (VER) goes
    // to SendCVR, 2 (CVR) to SendUSRInitDS, 3 (USR TWN I) to ProcessXFR
    void OnReplyImpl(INT RequestType, MsnLine Line);   // 0x10140290 (ret 0x40)
};
NW_ASSERT_SIZE(DispatchProcess, 0x64);
