#pragma once
// MsnUiStock.h - stage 5 (the standalone NWindow.dll), lane sa-msn-ui. New file (modelled on the C4 project's header of the same name).
//
// The messenger windows call functions of other files by the address they have in the stock image (StockCallAt / nw::Addr). Those functions
// are ported already, but most of them hang on a small stand-in struct that lives in the .cpp of the file that hooks them (the stock has no
// class for them). A typed call needs the declaration, so the ones whose definitions are this lane's are declared here, ONCE (one
// definition per name: Classes.gen.cpp, vt_audit), by the same struct name, base, method name and signature as the definition.
//
// The other stand-ins the messenger windows call are declared by their owners' headers (sa-integ, stage 5 integration; the copies that
// stood here are gone): NConsoleLeftFns / NConsoleForwardFns (NConsoleFns.h), NMsnShellWnd (NCConsole.h), the protocol object's calls
// (Msn.h: SignIn, SignOut, Block, SetStatus, SetDisplayName, Construct / Destroy), the sender of the sockets (MSNSocket.h:
// SendMessageBody), and NMsnListInit, NMsnGroupsInit, NMsnWndStrMapOps, NChatSbSender, NMsnWndOps (MsnProtoOps.h), NMsnTaskMapOps,
// NMsnTaskWndOps, NMsnNameMapCopier, NMsnChatOps, NChatConsole (MsnWndOps.h).
//
// This header is neutral (no protocol header): the stand-ins that derive from the protocol lane's classes are in MsnUiStockP.h.

#include "nwindow/NCWnd.h"
#include "nwindow/NControl.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/TaskWnd.h"
#include "nwindow/L2TaskWnd.h"
#include "nwindow/NChatTypes.h"
#pragma pack(push, 4)

// NCoreHelpers.cpp (no lane: the two lost their `static` for this): Util::MsnStrcmp and Util::MsnCompareFriend, cdecl, the strings by
// value (the function destroys them)
INT __cdecl NUtilMsnStrcmp(FString A, FString B);                                           // 0x10144410
INT __cdecl NUtilMsnCompareFriend(FString NameA, INT OnlineA, FString NameB, INT OnlineB);   // 0x10144920

// L2FriendWnd.cpp (this lane): the 0x20-byte object the friends page's constructor makes (0x1012DA10): an array, empty, and a table by
// name, empty (names ours). NConsoleWndMisc.cpp's friend list calls its Find too
struct NL2Book
{
    INT ArrayData;                   // +0x00
    INT ArrayNum;                    // +0x04
    INT ArrayMax;                    // +0x08
    NMsnWndStrMap Names;             // +0x0C

    NL2Book() : ArrayData(0), ArrayNum(0), ArrayMax(0) {}
    __declspec(noinline) __declspec(safebuffers) void* Find(NMsnStringArg Name);   // 0x1012CBD0 (ret 0xC)
};
NW_ASSERT_SIZE(NL2Book, 0x20);

// ChatWnd.cpp (this lane): the task list box's line of a task window (TaskBox's and L2TaskBox's, folded)
struct NChatTaskListBox : public NCListBox
{
    __declspec(noinline) void RemoveTask(NCWnd* Task);                           // 0x10124FC0 (ret 4)
};

// ChatWnd.cpp (this lane): TaskWnd: every task window leaves
struct NChatTaskWnd : public TaskWnd
{
    __declspec(noinline) void DestroyTasks();                                    // 0x1011D120
};

// ChatWnd.cpp (this lane): the entry of a friend's conversation in L2TaskWnd's list
struct NChatL2TaskWnd : public L2TaskWnd
{
    __declspec(noinline) void AddTask(NCWnd* ChatWindow, FString Name, INT bAlert);   // 0x10126240 (ret 0x14)
};

#pragma pack(pop)
