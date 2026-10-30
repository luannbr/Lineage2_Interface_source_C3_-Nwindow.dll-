#pragma once
// The heavier half of the messenger protocol layer's helper types (C3 only; new header of the stage 5 lane sa-msn-proto; see
// MsnProtoOps.h): the functions of the conversation, task and alarm windows and of their tables that the sessions' notices reach and
// that the windows' own files call. The stock keeps them out of line; ours are methods of types derived from the window class (adding
// nothing) so that a hook can take the window as `this`. The bodies are in SwitchboardSession.cpp (NChatConsole's in ChatWnd.cpp).

#include "nwindow/MsnProtoOps.h"
#include "nwindow/ChatWnd.h"
#include "nwindow/TaskWnd.h"
#include "nwindow/Task.h"
#include "nwindow/TaskBox.h"
#include "nwindow/AlarmWnd.h"
#include "nwindow/MsnTreeCtrl.h"

// A pair of the table of windows by name (MsnWnd.h's NMsnWndStrPair): made in place from the text and the window
struct NMsnWndStrPairInit : public NMsnWndStrPair
{
    __declspec(noinline) NMsnWndStrPairInit* Construct(const TCHAR* InKey, NCWnd* const* InValue);   // 0x10131540 (ret 8)
};

// The copy of an array of pointers (the stock's 0x10142780 and 0x10121B20 are one code): room for the other's count, then each pointer
// added at the end. `this` is raw memory
struct NMsnPtrArrayCopy : public FArray
{
    __declspec(noinline) NMsnPtrArrayCopy(const TArray<void*>& Src);    // 0x10142780 (ret 4)
};

// 0x1011D1E0 (ret 4): a copy of a map by name: the pairs copied, no hash yet, the other's hash size, the hash built. `this` is raw memory
struct NMsnNameMapCopier : public NChatNameMap
{
    NChatNameMap* CopyFrom(const NChatNameMap* Src);
};

// The task windows' map (TaskWnd / L2TaskWnd: a conversation window -> its Task)
struct NMsnTaskMapOps : public NTaskMap
{
    NCWnd* FindTask(NCWnd* const* InKey);                               // 0x10124A10 (ret 4)
    __declspec(noinline) NCWnd** AddTask(NCWnd* const* InKey, NCWnd* const* InValue);        // 0x10124B00 (ret 8)
    NCWnd** SetTask(NCWnd* const* InKey, NCWnd* const* InValue);        // 0x1011C210 (ret 8)
};

// The task list window
struct NMsnTaskWndOps : public TaskWnd
{
    __declspec(noinline) void SetTaskSession(void* InSession, NCWnd* ChatWindow);                      // 0x1011C110 (ret 8)
    __declspec(noinline) Task* FindTaskOfSession(void* InSession);                                    // 0x1011D0C0 (ret 4)
    __declspec(noinline) void AddTask(SwitchboardSession* InSession, NCWnd* ChatWindow, INT bAlert);   // 0x1011EA40 (ret 0xC)
};

// The conversation window: the people of the talk, the session, the notices of people coming and going
struct NMsnChatOps : public ChatWnd
{
    __declspec(noinline) void SetPeerAccount(FString Value);                          // 0x101315A0 (ret 0xC)
    __declspec(noinline) void OnPeerTyping(NChatUser Who);                            // 0x1011E530 (ret 0x44)
    __declspec(noinline) void SetSession(SwitchboardSession* Session);                // 0x1011CE80 (ret 4)
    __declspec(noinline) void ShowJoinNotice(NChatUser User);                         // 0x1011BD80 (ret 0x44)
    __declspec(noinline) void ShowLeaveNotice(NChatUser User);                        // 0x1011BEC0 (ret 0x44)
    __declspec(noinline) void OnInvitation(NChatUser From, NMsnMessage Msg);          // 0x1011F770 (ret 0x9C)
};

// The console's requests of a conversation (ChatWnd.cpp defines them, the chat lane's; the declaration is the one ChatWnd.cpp has)
struct NChatConsole : public UL2ConsoleWnd
{
    __declspec(noinline) void SendL2FriendSay(TCHAR* Name, TCHAR* Text);              // 0x10061180 (ret 8)
    __declspec(noinline) void SendMsnChatLog(TCHAR* Account, TCHAR* Text, INT Flag);  // 0x10061190 (ret 0xC)
};
