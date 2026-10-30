#pragma once
// The messenger protocol layer's helper types (C3 only; new header of the stage 5 lane sa-msn-proto). The stock keeps many small
// functions of the protocol layer out of line (constructors and destructors of the contact lists, the windows' notices of the
// sessions ...); ours are written as methods of types derived from the class they belong to, adding nothing (so that a hook can take
// the object as `this`). The types used to be declared inside the .cpp that defines them; the files that call across (Msn.cpp,
// MsnLogin.cpp, MsnUtil.cpp, SwitchboardSession.cpp, the windows' files) need the declarations, so they are here: calls of these
// functions replace the calls of their stock addresses. Every function is the stock one at the address named; the bodies are in
// MsnData.cpp, SwitchboardSession.cpp and (NChatSbSender) ChatWnd.cpp.
//
// MsnWndOps.h has the heavier half (the types derived from the chat, task and alarm windows).

#include "nwindow/Msn.h"
#include "nwindow/FriendList.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/SwitchboardSession.h"
#include "nwindow/NotificationProcess.h"

class ChatWnd;

// A contact record / a group record given by value to a window function that frees it (MsnWnd.h's NMsnPersonRec / NMsnGroupArg: plain
// bytes, so the C++ side adds no copy or destructor of its own): the copy made by the record's own copy constructor (NMsnFriend's
// 0x1012C460, NMsnGroup's 0x1012DB00, NMsnProtocol.cpp) in the argument slot, as the stock callers make it
FORCEINLINE NMsnPersonRec MsnFriendArg(const NMsnFriend& Src)
{
    NMsnPersonRec Copy;
    new ((void*)Copy.Raw) NMsnFriend(Src);
    return Copy;
}
FORCEINLINE NMsnGroupArg MsnGroupArg(const NMsnGroup& Src)
{
    NMsnGroupArg Copy;
    new ((void*)Copy.Raw) NMsnGroup(Src);
    return Copy;
}

/*-----------------------------------------------------------------------------
    The contact data (MsnData.cpp): the four lists, the groups and the tables they keep.
-----------------------------------------------------------------------------*/

struct NMsnListInit : public FriendList
{
    // (each a function of its own in the stock, so each is kept out of line: the compiler would inline these into their callers otherwise)
    __declspec(noinline) NMsnListInit* Construct(INT ListBit);       // 0x1012D6F0 (ret 4): no contact, no entry by account, the list's bit
    __declspec(noinline) void DeleteAll();                           // 0x1012D400: every contact destroyed and freed, the tables emptied
    __declspec(noinline) void Destroy();                             // 0x1012D760: the contacts go, then the table by account, then the array
};

struct NMsnGroupsInit : public NMsnGroupList
{
    __declspec(noinline) NMsnGroupsInit* Construct();                // 0x1012D7C0: no group, an empty table by id
    __declspec(noinline) void DeleteAll();                           // 0x1012D180: every group destroyed and freed, the tables emptied
    __declspec(noinline) void Destroy();                             // 0x1012D510: the groups go, then the table by id, then the array
};

struct NMsnIntTableInit : public NMsnIntMap
{
    __declspec(noinline) void Free();                                // 0x1012D4A0: the hash freed and forgotten, then the pairs
};

struct NMsnContactsInit : public NMsnContacts
{
    __declspec(noinline) NMsnContactsInit* Construct();              // 0x1012D830: the four lists and the group list made
    __declspec(noinline) void Destroy();                             // 0x1012D970: each list destroyed, freed and forgotten, then the group list
};

// 0x10123570: the destructor of an array of pointers (TArray<T*>: UnTemplate.h's checks, Remove of all, ~FArray), a function of its own in
// the stock (hooked in AddedConfirmDialog.cpp as NMsnBuddyArray's destructor; this is the copy MsnData.cpp defines for the protocol layer)
void MsnFreePtrArray(void* Array);

/*-----------------------------------------------------------------------------
    The messenger window's tables of chat windows (MsnWnd.h's NMsnWndStrMap; SwitchboardSession.cpp).
-----------------------------------------------------------------------------*/

struct NMsnWndStrMapOps : public NMsnWndStrMap
{
    NCWnd** AddWnd(const TCHAR* InKey, NCWnd* const* InValue);          // 0x10134840 (ret 8): a new pair at the end
    void MultiFind(const FString* InKey, TArray<NCWnd*>* Out);          // 0x10134760 (ret 8): every window entered under that name is added to the array
};

/*-----------------------------------------------------------------------------
    The session's senders the conversation window uses (ChatWnd.cpp defines them, the chat lane's; the session's own are
    SwitchboardSession.cpp's). The declaration is the one ChatWnd.cpp has (the bodies are its).
-----------------------------------------------------------------------------*/

struct NChatSbSender : public SwitchboardSession
{
    __declspec(noinline) void SendTyping(NMsnMessage Message);    // 0x1012E3E0 (ret 0x58)
    __declspec(noinline) void SendText(NMsnMessage Message);      // 0x1012E4D0 (ret 0x58)
    __declspec(noinline) void SendInvite(FString Account);        // 0x1012E6B0 (ret 0xC)
};

/*-----------------------------------------------------------------------------
    The messenger window (MsnWnd) as the protocol object's notices reach it (SwitchboardSession.cpp).
-----------------------------------------------------------------------------*/

struct NMsnWndOps : public MsnWnd
{
    __declspec(noinline) ChatWnd* FindFreeChat();                                                 // 0x10136F30
    __declspec(noinline) ChatWnd* FindOneToOneChat(FString Account);                              // 0x10136E30 (ret 0xC)
    __declspec(noinline) void OnSessionTyping(SwitchboardSession* Session, NMsnFriend Friend);    // 0x10135E90 (ret 0x48)
    __declspec(noinline) void ShowTaskWnd();                                                      // 0x10135F10
    __declspec(noinline) void GiveListsToTree();                                                  // 0x10130F70
    __declspec(noinline) void OnSessionJoin(SwitchboardSession* Session, NMsnFriend Friend);      // 0x10136F90 (ret 0x48)
    __declspec(noinline) void OnSessionLeave(SwitchboardSession* Session, NMsnFriend Friend);     // 0x1013B480 (ret 0x48)
    __declspec(noinline) void OnSessionMessage(SwitchboardSession* Session, NMsnFriend Friend, NMsnMessage Message);       // 0x1013BB60 (ret 0xA0)
    __declspec(noinline) void OnSessionInvite(SwitchboardSession* Session, NMsnFriend Friend, NMsnMessage Message);        // 0x1013BC90 (ret 0xA0)
    __declspec(noinline) void OnSessionNewTalk(SwitchboardSession* Session, NMsnFriend Friend, NMsnMessage Message);       // 0x1013B070 (ret 0xA0)
};
