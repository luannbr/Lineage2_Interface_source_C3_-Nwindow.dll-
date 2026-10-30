/*=============================================================================
    Msn.cpp - the messenger's manager (C3 only). Stock: 0x1012AB60 .. 0x1012C0E0. The notifications to the window take
    their records by value (the callee destroys them, like every by-value argument of the stock build); the window's
    functions are msn2's (MsnWnd.cpp, and the window helpers of SwitchboardSession.cpp: MsnProtoOps.h). Stage 5: they are called
    as functions of ours (they were stock addresses).
=============================================================================*/
#include "nwindow/Msn.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/DispatchProcess.h"
#include "nwindow/NotificationProcess.h"
#include "nwindow/SwitchboardSession.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/MsnProtoOps.h"

// 0x1012ae20 (ret 0x44, guard name) - a friend came: the window gets a copy of the record
void Msn::NotifyAddFriend(NMsnFriend Friend)
{
    guard(Msn::NotifyAddFriend);
    if (Wnd)
        Wnd->NotifyAddFriend(MsnFriendArg(Friend));
    unguard;
}
typedef void (Msn::*SigMsnNotifyAddFriend)(NMsnFriend);
NW_HOOK_VARARGS(0x1012ae20, Msn, NotifyAddFriend, SigMsnNotifyAddFriend)

// 0x1012aed0 (ret 0xC, guard name) - a friend went (the account by value): the window gets a copy of the account
void Msn::NotifyRemoveFriend(FString Account)
{
    guard(Msn::NotifyRemoveFriend);
    if (Wnd)
        Wnd->NotifyRemoveFriend(MsnStringCopy(Account));
    unguard;
}
typedef void (Msn::*SigMsnNotifyRemoveFriend)(FString);
NW_HOOK_VARARGS(0x1012aed0, Msn, NotifyRemoveFriend, SigMsnNotifyRemoveFriend)

// 0x1012af80 (ret 0x1C, guard name) - a group came: the window gets a copy of the record
void Msn::NotifyAddGroup(NMsnGroup Group)
{
    guard(Msn::NotifyAddGroup);
    if (Wnd)
        Wnd->NotifyAddGroup(MsnGroupArg(Group));
    unguard;
}
typedef void (Msn::*SigMsnNotifyAddGroup)(NMsnGroup);
NW_HOOK_VARARGS(0x1012af80, Msn, NotifyAddGroup, SigMsnNotifyAddGroup)

// 0x1012b1d0 (ret 4, guard name) - the messenger's sounds, when the option is on: 0 a friend came online, 1 a message
// came, 2 new e-mail (the interface sound of that name is played by the console)
void Msn::PlayAlarmSound(INT Kind)
{
    guard(Msn::PlayAlarmSound);
    if (GMsnSoundAlarm)
    {
        if (Kind == 0)
            GConsole->PlaySoundName(TEXT("MessengerSound.online"));
        else if (Kind == 1)
            GConsole->PlaySoundName(TEXT("MessengerSound.type"));
        else if (Kind == 2)
            GConsole->PlaySoundName(TEXT("MessengerSound.newemail"));
    }
    unguard;
}
NW_HOOK_METHOD(0x1012b1d0, Msn, PlayAlarmSound)

// 0x1012b720 (guard name) - the dispatch connection is over: its destructor (0x10140280), its memory, the pointer cleared
void Msn::NotifyDispatchProcessEnd()
{
    guard(Msn::NotifyDispatchProcessEnd);
    if (Dispatch)
    {
        Dispatch->Destroy();
        appFree(Dispatch);
        Dispatch = NULL;
    }
    unguard;
}
NW_HOOK_METHOD(0x1012b720, Msn, NotifyDispatchProcessEnd)

// 0x1012ab60 (ret 0xC; name ours) - new e-mail text for the window's 0x10130FF0 (a copy: the window destroys it)
void Msn::NotifyNewMail(FString Text)
{
    Wnd->OnNewMail(MsnStringCopy(Text));
}
typedef void (Msn::*SigMsnNotifyNewMail)(FString);
NW_HOOK_VARARGS(0x1012ab60, Msn, NotifyNewMail, SigMsnNotifyNewMail)

// 0x1012adb0 (ret 0xC; name ours) - an error number (the string by value) for the window's dialog (0x10133BE0)
void Msn::NotifyError(FString Code)
{
    if (Wnd)
        Wnd->ShowError(Code);
}
typedef void (Msn::*SigMsnNotifyError)(FString);
NW_HOOK_VARARGS(0x1012adb0, Msn, NotifyError, SigMsnNotifyError)

// 0x1012abd0 (ret 0x44; name ours) - a friend's record for the window's 0x10137120 after the forward list was sorted
void Msn::NotifyFriendOnline(NMsnFriend Friend)
{
    Contacts->FL->Sort();
    Wnd->OnFriendPresence(MsnFriendArg(Friend));
}
typedef void (Msn::*SigMsnNotifyFriendOnline)(NMsnFriend);
NW_HOOK_VARARGS(0x1012abd0, Msn, NotifyFriendOnline, SigMsnNotifyFriendOnline)

// 0x1012ac40 (ret 0x44; name ours) - the same for the window's 0x10137290
void Msn::NotifyFriendStatus(NMsnFriend Friend)
{
    Contacts->FL->Sort();
    Wnd->RefreshFriend(MsnFriendArg(Friend));
}
typedef void (Msn::*SigMsnNotifyFriendStatus)(NMsnFriend);
NW_HOOK_VARARGS(0x1012ac40, Msn, NotifyFriendStatus, SigMsnNotifyFriendStatus)

// 0x1012acb0 (ret 0x44; name ours) - the same for the window's 0x10133AF0
void Msn::NotifyFriendName(NMsnFriend Friend)
{
    Contacts->FL->Sort();
    Wnd->RefreshGroupOf(MsnFriendArg(Friend));
}
typedef void (Msn::*SigMsnNotifyFriendName)(NMsnFriend);
NW_HOOK_VARARGS(0x1012acb0, Msn, NotifyFriendName, SigMsnNotifyFriendName)

// 0x1012ad20 (ret 0x44; name ours) - a friend's record for the window's 0x10133B60 (no sort)
void Msn::NotifyFriendBlocked(NMsnFriend Friend)
{
    Wnd->OnFriendRequest(MsnFriendArg(Friend));
}
typedef void (Msn::*SigMsnNotifyFriendBlocked)(NMsnFriend);
NW_HOOK_VARARGS(0x1012ad20, Msn, NotifyFriendBlocked, SigMsnNotifyFriendBlocked)

// 0x1012b030 (ret 0x44; name ours) - the user's own record came: signed in, the record kept, the window's 0x10131280 gets a copy
void Msn::NotifySignedIn(NMsnFriend Self)
{
    bSignedIn = 1;
    *Me = Self;
    if (Wnd)
        Wnd->OnSignedIn(MsnFriendArg(Self));
}
typedef void (Msn::*SigMsnNotifySignedIn)(NMsnFriend);
NW_HOOK_VARARGS(0x1012b030, Msn, NotifySignedIn, SigMsnNotifySignedIn)

// 0x1012b0b0 (ret 0x44; name ours) - the user's record changed: kept, the window's 0x10131360 gets a copy
void Msn::NotifyMyInfo(NMsnFriend Self)
{
    *Me = Self;
    if (Wnd)
        Wnd->SetMe(MsnFriendArg(Self));
}
typedef void (Msn::*SigMsnNotifyMyInfo)(NMsnFriend);
NW_HOOK_VARARGS(0x1012b0b0, Msn, NotifyMyInfo, SigMsnNotifyMyInfo)

// 0x1012b160 (ret 0xC; name ours) - the contact of that account in the forward list (the account by value), NULL when there is none
NMsnFriend* Msn::FindFriend(FString Account)
{
    return Contacts->FL->FindFriend(Account);
}
typedef NMsnFriend* (Msn::*SigMsnFindFriend)(FString);
NW_HOOK_VARARGS(0x1012b160, Msn, FindFriend, SigMsnFindFriend)

// 0x1012ad80 (name ours; the db's C2-mapped "MSNSocket::CloseSocket" is wrong) - with a window: the forward list sorted and the
// window's 0x10130F70 (the tree rebuilt from the lists)
void Msn::SortFriends()
{
    if (Wnd)
    {
        Contacts->FL->Sort();
        ((NMsnWndOps*)Wnd)->GiveListsToTree();
    }
}
NW_HOOK_METHOD(0x1012ad80, Msn, SortFriends)

// 0x1012b420 (name ours) - the same, unless a flag of the window's panel is set (Wnd +0x15C, its +0x124, that one's +0x1D4)
void Msn::RefreshFriends()
{
    if (Wnd)
    {
        void* Panel = ::nw::Field<void*>(::nw::Field<void*>(Wnd, 0x15C), 0x124);
        if (::nw::Field<INT>(Panel, 0x1D4) == 0)
        {
            Contacts->FL->Sort();
            ((NMsnWndOps*)Wnd)->GiveListsToTree();
        }
    }
}
NW_HOOK_METHOD(0x1012b420, Msn, RefreshFriends)

// 0x1012e090 (ret 4; the result is the hidden argument; name ours) - a copy of the account of the login
FString Msn::GetAccount()
{
    return Account;
}
NW_HOOK_METHOD(0x1012e090, Msn, GetAccount)

// 0x1013c540 (ret 4; name ours) - a copy of the password
FString Msn::GetPassword()
{
    return Password;
}
NW_HOOK_METHOD(0x1013c540, Msn, GetPassword)

// 0x1013c570 (ret 4; name ours) - a copy of the string at +0x3C
FString Msn::GetUnknown3C()
{
    return Unknown3C;
}
NW_HOOK_METHOD(0x1013c570, Msn, GetUnknown3C)

// 0x1012bd80 (guard name) - the end of every switchboard session: each is shown to the window (0x10138460), destroyed (the
// destructor 0x1012F090) and freed, then the table is emptied (twelve-byte pairs, a hash of 8)
void Msn::NotifySwitchboardSessionEnd()
{
    guard(Msn::NotifySwitchboardSessionEnd);
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (Session)
        {
            if (Wnd)
                Wnd->OnSessionRemoved(Session);
            Session->Destroy();
            appFree(Session);
        }
    }
    Sessions.Pairs.Empty();
    Sessions.HashCount = 8;
    ((NMsnIntMap*)&Sessions)->Rehash();   // the table's Rehash (0x100626E0)
    unguard;
}
NW_HOOK_METHOD(0x1012bd80, Msn, NotifySwitchboardSessionEnd)

// 0x1012c0e0 (ret 0xC; name ours) - the notification connection ended (the reason by value): the sessions go first, then the
// connection (destructor 0x10140970, freed), the signed-in flag is cleared and the window gets the reason (0x101387F0)
void Msn::NotifyNotificationProcessEnd(FString Reason)
{
    NotifySwitchboardSessionEnd();
    if (Notification)
    {
        NotificationProcess* Process = Notification;
        Process->Destroy();
        appFree(Process);
        Notification = NULL;
        bSignedIn = 0;
        Wnd->OnSignedOut(MsnStringCopy(Reason));
    }
}
typedef void (Msn::*SigMsnNotifyNotificationProcessEnd)(FString);
NW_HOOK_VARARGS(0x1012c0e0, Msn, NotifyNotificationProcessEnd, SigMsnNotifyNotificationProcessEnd)

// 0x1012b280 (ret 0x10; name ours) - a notification connection (0x98 bytes, constructed by 0x10140890 with the manager, the
// server and the first transaction id 1) to that server, started; error 930 to the window when it cannot connect
void Msn::StartNotification(NMsnServer Server)
{
    NotificationProcess* Process = NULL;
    void* Memory = ::operator new(0x98);
    if (Memory)
    {
#if NW_HOOK_MODE
        Process = ((NotificationProcess*)Memory)->ConstructNS(this, Server, 1);
#else
        Process = new (Memory) NotificationProcess(this, Server, 1);
#endif
    }
    Notification = Process;
    if (!Process->Connect())
        NotifyError(FString(TEXT("930")));
}
typedef void (Msn::*SigMsnStartNotification)(NMsnServer);
NW_HOOK_VARARGS(0x1012b280, Msn, StartNotification, SigMsnStartNotification)

// 0x1012b840 (ret 4; name ours) - the session connects (virtual 8) and enters the map under its socket
void Msn::AddSession(SwitchboardSession* Session)
{
    Session->Connect();
    INT Socket = Session->Sock.hSock;
    INT Value = (INT)Session;
    ((NMsnIntMap*)&Sessions)->Set(Socket, Value);   // 0x10136230
}
NW_HOOK_METHOD(0x1012b840, Msn, AddSession)

// 0x1012beb0 (ret 4; name ours) - the session of that socket: shown to the window (even a missing one), destroyed and freed
// when there is one; every pair of the socket leaves the map
void Msn::RemoveSession(INT Socket)
{
    SwitchboardSession* Session = Sessions.FindRef(Socket);
    if (Wnd)
        Wnd->OnSessionRemoved(Session);
    if (Session)
    {
        Session->Destroy();
        appFree(Session);
    }
    ((NMsnIntMap*)&Sessions)->Remove(Socket);
}
NW_HOOK_METHOD(0x1012beb0, Msn, RemoveSession)

// 0x1012b870 (ret 0x50; name ours) - every session whose id is that one: the window's 0x10136F90 gets the session and a copy of
// the record
void Msn::NotifySessionJoin(FString SessionID, NMsnFriend Friend)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            ((NMsnWndOps*)Wnd)->OnSessionJoin(Session, Friend);
    }
}
typedef void (Msn::*SigMsnNotifySessionJoin)(FString, NMsnFriend);
NW_HOOK_VARARGS(0x1012b870, Msn, NotifySessionJoin, SigMsnNotifySessionJoin)

// 0x1012b940 (ret 0x50; name ours) - the same for the window's 0x1013B480
void Msn::NotifySessionLeave(FString SessionID, NMsnFriend Friend)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            ((NMsnWndOps*)Wnd)->OnSessionLeave(Session, Friend);
    }
}
typedef void (Msn::*SigMsnNotifySessionLeave)(FString, NMsnFriend);
NW_HOOK_VARARGS(0x1012b940, Msn, NotifySessionLeave, SigMsnNotifySessionLeave)

// 0x1012bb10 (ret 0x50; name ours) - the same for the window's 0x10135E90
void Msn::NotifySessionEvent(FString SessionID, NMsnFriend Friend)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            ((NMsnWndOps*)Wnd)->OnSessionTyping(Session, Friend);
    }
}
typedef void (Msn::*SigMsnNotifySessionEvent)(FString, NMsnFriend);
NW_HOOK_VARARGS(0x1012bb10, Msn, NotifySessionEvent, SigMsnNotifySessionEvent)

// 0x1012ba10 (ret 0xA8; name ours) - the same with a message (the window's 0x1013BB60 gets the session, a copy of the record and
// a copy of the message)
void Msn::NotifySessionMessage(FString SessionID, NMsnFriend Friend, NMsnMessage Message)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            ((NMsnWndOps*)Wnd)->OnSessionMessage(Session, Friend, Message);
    }
}
typedef void (Msn::*SigMsnNotifySessionMessage)(FString, NMsnFriend, NMsnMessage);
NW_HOOK_VARARGS(0x1012ba10, Msn, NotifySessionMessage, SigMsnNotifySessionMessage)

// 0x1012bbe0 (ret 0xA8; name ours) - the same for the window's 0x1013BC90
void Msn::NotifySessionMessage2(FString SessionID, NMsnFriend Friend, NMsnMessage Message)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            ((NMsnWndOps*)Wnd)->OnSessionInvite(Session, Friend, Message);
    }
}
typedef void (Msn::*SigMsnNotifySessionMessage2)(FString, NMsnFriend, NMsnMessage);
NW_HOOK_VARARGS(0x1012bbe0, Msn, NotifySessionMessage2, SigMsnNotifySessionMessage2)

// 0x1012bce0 (ret 0xC; name ours) - the session of that id (by value) started: the window's 0x10138420 gets it
void Msn::NotifySessionStarted(FString SessionID)
{
    for (INT i = 0; i < Sessions.Pairs.Num(); i++)
    {
        SwitchboardSession* Session = Sessions.Pairs(i).Value;
        if (*Session->GetSessionIDRef() == SessionID)
            Wnd->ShowChatOfSession(Session);
    }
}
typedef void (Msn::*SigMsnNotifySessionStarted)(FString);
NW_HOOK_VARARGS(0x1012bce0, Msn, NotifySessionStarted, SigMsnNotifySessionStarted)

// 0x1012b130 (ret 0xC; name ours) - nothing: a function taking a string by value only to destroy it (the unknown answers of
// the notification connection go through it)
void Msn::DiscardString(FString Text)
{
}
typedef void (Msn::*SigMsnDiscardString)(FString);
NW_HOOK_VARARGS(0x1012b130, Msn, DiscardString, SigMsnDiscardString)

// 0x1012b350 (ret 0x1C; name ours) - a switchboard session for that server and session id (both by value): 0xA4 bytes, constructed
// by 0x1012EF60 with the manager, the server and the id
SwitchboardSession* Msn::CreateSession(NMsnServer Server, FString SessionID)
{
    SwitchboardSession* Session = NULL;
    void* Memory = ::operator new(0xA4);
    if (Memory)
    {
#if NW_HOOK_MODE
        Session = ((SwitchboardSession*)Memory)->ConstructSession(this, Server, SessionID);
#else
        Session = new (Memory) SwitchboardSession(this, Server, SessionID);
#endif
    }
    return Session;
}
typedef SwitchboardSession* (Msn::*SigMsnCreateSession)(NMsnServer, FString);
NW_HOOK_VARARGS(0x1012b350, Msn, CreateSession, SigMsnCreateSession)

// 0x1012b540 (ret 8; name ours) - WSAAsyncSelect's connect message: the connection of that socket (the dispatch one, the notification one,
// else a session of the map) gets OnConnect
void Msn::OnSocketConnect(INT Socket, LPARAM lParam)
{
    MSNSocket* Target = NULL;
    if (Dispatch && Dispatch->Sock.hSock == (UINT)Socket)
        Target = Dispatch;
    else if (Notification && Notification->Sock.hSock == (UINT)Socket)
        Target = Notification;
    else if (Sessions.Pairs.Num() > 0)
        Target = Sessions.FindRef(Socket);
    if (Target)
        Target->OnConnect(Socket, lParam);
}
NW_HOOK_METHOD(0x1012b540, Msn, OnSocketConnect)

// 0x1012b5a0 (ret 8; name ours) - the data / close message of a socket, the same way (OnSocketMessage)
void Msn::OnSocketData(INT Socket, LPARAM lParam)
{
    MSNSocket* Target = NULL;
    if (Dispatch && Dispatch->Sock.hSock == (UINT)Socket)
        Target = Dispatch;
    else if (Notification && Notification->Sock.hSock == (UINT)Socket)
        Target = Notification;
    else if (Sessions.Pairs.Num() > 0)
        Target = Sessions.FindRef(Socket);
    if (Target)
        Target->OnSocketMessage(Socket, lParam);
}
NW_HOOK_METHOD(0x1012b5a0, Msn, OnSocketData)

// 0x1012a700 (ret 4; the result is the hidden argument; name ours) - the presence status the notification connection has (0x1013C450),
// an empty string without a connection
FString Msn::GetStatus()
{
    if (Notification)
        return Notification->GetStatus();
    return FString(TEXT(""));
}
NW_HOOK_METHOD(0x1012a700, Msn, GetStatus)
