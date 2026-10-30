/*=============================================================================
    MsnLogin.cpp - the manager's log-in and the commands the windows send (C3 only; C3 addresses): the sign-in (0x1012B6A0) that
    makes the dispatch connection (0x1012B600, its server 0x1012B4B0, its constructor 0x10140200), and the commands of the
    notification connection that the window functions call: OUT (0x1013D3D0), CHG (0x1013F4A0), REA (0x1013F570) and REM / ADD
    of the block list (0x1013F870). The classes are the protocol lane's (Msn.h, DispatchProcess.h, NotificationProcess.h).
    Wave 5: the answer to SYN (0x10140340) and the destructors of the pending-names map (0x10140820, 0x1013FFD0).
    Stage 5: the functions are members of their classes (they were methods of derived helper types) and call each other directly
    (they called stock addresses); the dispatch connection of the standalone build is made by its real constructor.
=============================================================================*/
#include "nwindow/DispatchProcess.h"
#include "nwindow/NotificationProcess.h"
#include "nwindow/MsnCommand.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/MsnUtil.h"
#include "nwindow/MsnProtoOps.h"

// a server address given by value (0x10 bytes: the host, the port): the callee destroys the host
struct NMsnServerArg
{
    BYTE Raw[0x10];
    FString& Host() { return *(FString*)Raw; }
    INT& Port() { return *(INT*)(Raw + 0xC); }
};
NW_ASSERT_SIZE(NMsnServerArg, 0x10);

/*-----------------------------------------------------------------------------
    The dispatch connection.
-----------------------------------------------------------------------------*/

// 0x1012b4b0 (cdecl; the address in the hidden argument is filled and given back) - the dispatch server: messenger.hotmail.com, 1863
NMsnServer* MsnDispatchServer(NMsnServer* Out)
{
    NMsnServer Server(FString(TEXT("messenger.hotmail.com")), 0x747);
    new ((void*)Out) NMsnServer(Server);
    return Out;
}
NW_HOOK_FUNC(0x1012B4B0, MsnDispatchServer)

#if NW_HOOK_MODE
struct NMsnDispatchInit : public DispatchProcess
{
    __declspec(noinline) __declspec(safebuffers) NMsnDispatchInit* Construct(Msn* InOwner, NMsnServerArg Server, INT TrID);   // 0x10140200 (ret 0x18)
};

// 0x10140200 (ret 0x18) - the base's construction (0x10140090: the owner, a copy of the server, the first transaction id), the
// vtable of the dispatch class, and the server given by value destroyed. Hook build only: the standalone build's dispatch connection is
// made by its constructor (DispatchProcess.cpp), which the compiler gives the vtable
NMsnDispatchInit* NMsnDispatchInit::Construct(Msn* InOwner, NMsnServerArg Server, INT TrID)
{
    MSNSocket::Construct(InOwner, *(const NMsnServer*)Server.Raw, TrID);   // (a copy of the server goes by value, the callee ends it)
    *(void**)this = ::nw::Addr(0x101AB164);
    Server.Host().~FString();
    return this;
}
NW_HOOK_VARARGS(0x10140200, NMsnDispatchInit, Construct, NMsnDispatchInit* (NMsnDispatchInit::*)(Msn*, NMsnServerArg, INT))
#endif

/*-----------------------------------------------------------------------------
    The manager's sign-in.
-----------------------------------------------------------------------------*/

// 0x1012b6a0 (ret 0x18) - the account and the password are kept, the dispatch connection is made; both given by value and freed here
void Msn::SignIn(NMsnStringArg InAccount, NMsnStringArg InPassword)
{
    Account = InAccount.Str();
    Password = InPassword.Str();
    StartDispatch();
    InAccount.Str().~FString();
    InPassword.Str().~FString();
}
NW_HOOK_VARARGS(0x1012B6A0, Msn, SignIn, void (Msn::*)(NMsnStringArg, NMsnStringArg))

// 0x1012b600 - the dispatch connection (a 0x64-byte object made with the dispatch server and the transaction id 1) is made and told
// to connect (virtual 8); when that fails the window is told the error 930
void Msn::StartDispatch()
{
    DispatchProcess* Process = NULL;
    void* Mem = appMalloc(0x64, TEXT("new"));
    if (Mem)
    {
        NMsnServerArg Server;
        MsnDispatchServer((NMsnServer*)&Server);
#if NW_HOOK_MODE
        Process = ((NMsnDispatchInit*)Mem)->Construct(this, Server, 1);
#else
        Process = new (Mem) DispatchProcess(this, *(const NMsnServer*)Server.Raw, 1);
        Server.Host().~FString();    // (the stock constructor ends the server it is given by value)
#endif
    }
    Dispatch = Process;
    if (!Process->Connect())
        Wnd->ShowError(FString(TEXT("930")));
}
NW_HOOK_METHOD(0x1012B600, Msn, StartDispatch)

/*-----------------------------------------------------------------------------
    The commands of the notification connection.
-----------------------------------------------------------------------------*/

// 0x1013d3d0 - "OUT"
void NotificationProcess::SendSignOut()
{
    NMsnLocalCommand Command(FString(TEXT("OUT")));
    Send(Command);
}
NW_HOOK_METHOD(0x1013D3D0, NotificationProcess, SendSignOut)

// 0x1013f4a0 (ret 0xC) - the status code is kept and "CHG trid code" sent (request type 7)
void NotificationProcess::SendStatus(NMsnStringArg Code)
{
    Status = Code.Str();
    NMsnLocalCommand Command(FString(TEXT("CHG")));
    Command.TrID = NextTrID;
    Command.AddArg(FString(Code.Str()));
    RegisterPending(7);
    Send(Command);
    Code.Str().~FString();
}
NW_HOOK_VARARGS(0x1013F4A0, NotificationProcess, SendStatus, void (NotificationProcess::*)(NMsnStringArg))

// 0x1013f570 (ret 0xC) - "REA trid account name": the user's account and the new name made safe for the wire (0x101442D0) (request type 8)
void NotificationProcess::SendDisplayName(NMsnStringArg Name)
{
    NMsnLocalCommand Command(FString(TEXT("REA")));
    Command.TrID = NextTrID;
    NMsnFriend* Me = Owner->GetMe();   // 0x1012B150
    Command.AddArg(FString(Me->Account));
    TCHAR Encoded[260];
    appMemset(Encoded, 0, 0x208);
    MsnUrlCanonicalize(*Name.Str(), Encoded);
    Command.AddArg(FString(Encoded));
    RegisterPending(8);
    Send(Command);
    Name.Str().~FString();
}
NW_HOOK_VARARGS(0x1013F570, NotificationProcess, SendDisplayName, void (NotificationProcess::*)(NMsnStringArg))

// 0x1013f870 (ret 0x10) - when the user's forward list has the contact: "REM trid AL|BL account" (AL when blocking: it leaves the
// allow list, else the block list) and "ADD trid BL|AL account name" (the other list), both of request type 12. The contact lists
// come from the manager's getter (0x1012B140) and the forward list from theirs (0x1012C680: the first dword, a getter the linker
// folded with others')
void NotificationProcess::SendBlock(NMsnStringArg Mail, INT bBlock)
{
    NMsnContacts* Contacts = Owner->GetContacts();
    FriendList* Forward = Contacts->FL;
    if (Forward)
    {
        NMsnFriend* Contact = Forward->FindFriend(FString(Mail.Str()));
        if (Contact)
        {
            NMsnLocalCommand Remove(FString(TEXT("REM")));
            Remove.TrID = NextTrID;
            Remove.AddArg(FString(bBlock ? TEXT("AL") : TEXT("BL")));
            Remove.AddArg(FString(Mail.Str()));
            RegisterPending(0xC);
            Send(Remove);
            NMsnLocalCommand Add(FString(TEXT("ADD")));
            Add.TrID = NextTrID;
            Add.AddArg(FString(bBlock ? TEXT("BL") : TEXT("AL")));
            Add.AddArg(FString(Mail.Str()));
            Add.AddArg(FString(Contact->Name));
            RegisterPending(0xC);
            Send(Add);
        }
    }
    Mail.Str().~FString();
}
NW_HOOK_VARARGS(0x1013F870, NotificationProcess, SendBlock, void (NotificationProcess::*)(NMsnStringArg, INT))

// 0x10140340 (ret 0x3C) - the answer to the request type 6 ("SYN trid serial"): when the serial is not the one the connection keeps,
// the four contact lists are emptied (0x1012D400 each, found by the folded getters 0x1012C680 .. 0x1012C6B0: the fields), the serial is
// kept and the user's own status code (the manager's 0x1013C570, built in the argument) is sent again (CHG)
void NotificationProcess::HandleSYNStart(MsnLine Line)
{
    INT Serial = Line.GetArgInt(0);
    if (SyncSerial != Serial)
    {
        NMsnContacts* Contacts = Owner->GetContacts();
        ((NMsnListInit*)Contacts->FL)->DeleteAll();
        ((NMsnListInit*)Contacts->AL)->DeleteAll();
        ((NMsnListInit*)Contacts->BL)->DeleteAll();
        ((NMsnListInit*)Contacts->RL)->DeleteAll();
        NMsnStringArg Code;
        SyncSerial = Serial;
        new ((void*)Code.Raw) FString(Owner->GetUnknown3C());   // (the getter builds its result in the argument)
        SendStatus(Code);
    }
}
typedef void (NotificationProcess::*SigNPHandleSYNStart)(MsnLine);
NW_HOOK_VARARGS(0x10140340, NotificationProcess, HandleSYNStart, SigNPHandleSYNStart)

/*-----------------------------------------------------------------------------
    The table of the pending names (TMap<INT, FString>, NotificationProcess +0x84).
-----------------------------------------------------------------------------*/

// 0x1013ffd0 - the pairs removed (0x1013E9A0: the strings destroyed), then the array itself
void NMsnIntStrMap::FreePairs()
{
    RemovePairs(0, Pairs.Num());
    ((FArray*)this)->~FArray();
    *(void**)this = NULL;            // (nothing reads the array again; the store keeps this a function that returns, as the stock one does)
}
NW_HOOK_METHOD(0x1013ffd0, NMsnIntStrMap, FreePairs)

// 0x10140820 - the hash freed and forgotten, then the pairs (the stock clears the hash first; the pairs' destruction does not read it,
// and a store after the call keeps this a function that returns instead of a tail jump)
void NMsnIntStrMap::Free()
{
    if (Hash)
        appFree(Hash);
    FreePairs();
    Hash = NULL;
    HashCount = 0;
}
NW_HOOK_METHOD(0x10140820, NMsnIntStrMap, Free)
