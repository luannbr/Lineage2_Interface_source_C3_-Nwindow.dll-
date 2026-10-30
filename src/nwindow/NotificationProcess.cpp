/*=============================================================================
    NotificationProcess.cpp - the connection to the notification server (C3 only; names ours). Stock: 0x1013C270, 0x1013C2C0,
    0x1013C450 .. 0x1013C5C0 (small members), 0x1013C480, 0x1013CFC0, 0x1013D120 .. 0x1013DE40 (the answers), 0x1013E1C0 .. 0x1013E230,
    0x1013E9A0 (the string map), 0x10140890 / 0x10140970 (the constructor's and the destructor's bodies). The constructor and the
    destructor set the vtable 0x101AB190: the hook build writes it by hand, the standalone build (stage 5) gets it from the compiler (its
    constructor and the vtable's overrides at the end of the file).
=============================================================================*/
#include <windows.h>
#include <wininet.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include "nwindow/NotificationProcess.h"
#include "nwindow/SwitchboardSession.h"
#include "nwindow/MsnUtil.h"
#include "nwindow/NChatMsg.h"

// wininet is an import of the stock image; the hook DLL does not link it otherwise
#pragma comment(lib, "wininet.lib")

/*-----------------------------------------------------------------------------
    NMsnPassport.
-----------------------------------------------------------------------------*/

// The blocks the login code makes (the buffers, the host and path copies, the text it hands back) come from the STOCK C runtime's
// heap: this code and its callers were linked with it, and the text is never freed by the callers. Two runtimes live in the
// process, so the stock entries are called, never ours. The standalone has one runtime, its own: malloc, free and _wcsdup of
// this module (the text that comes back is never freed by the callers, as in the stock)
static void* StockMalloc(UINT Size)
{
#if NW_HOOK_MODE
    typedef void*(__cdecl * Fn)(UINT);
    return ((Fn)::nw::Addr(0x1014D174))(Size);   // _malloc
#else
    return malloc(Size);
#endif
}
static void StockFree(void* Block)
{
#if NW_HOOK_MODE
    typedef void(__cdecl * Fn)(void*);
    ((Fn)::nw::Addr(0x1014D05C))(Block);   // _free
#else
    free(Block);
#endif
}
static TCHAR* StockWcsdup(const TCHAR* Text)
{
#if NW_HOOK_MODE
    typedef TCHAR*(__cdecl * Fn)(const TCHAR*);
    return ((Fn)::nw::Addr(0x1014DF1E))(Text);   // __wcsdup
#else
    return _wcsdup(Text);
#endif
}
// the stock's _snwprintf (0x1014DF4C) and isdigit (0x1014DE6B), cdecl; the standalone's are the C runtime's
#if NW_HOOK_MODE
#define NMSN_SNWPRINTF ((INT(__cdecl*)(TCHAR*, INT, const TCHAR*, ...))::nw::Addr(0x1014DF4C))
#define NMSN_ISDIGIT ((INT(__cdecl*)(INT))::nw::Addr(0x1014DE6B))
#else
#define NMSN_SNWPRINTF _snwprintf
#define NMSN_ISDIGIT isdigit
#endif
// the characters Count of the text at Text are taken off its front (stdcall, 0x10141440, SwitchboardSession.cpp)
static void StockShiftLeft(TCHAR* Text, INT Count)
{
    MsnShiftText(Text, Count);
}

// 0x101418a0 (ret 4) - the URL of the Passport login server: nexus.passport.com's answer to a GET of Url (host and path split at
// the first slash after "://", port 443) has it as "DALogin=" in its headers. A redirect (302) asks again at its Location. The
// text that comes back is "https://" and what follows "DALogin=" up to a comma (a block of the stock heap, never freed), NULL when
// anything failed (the handles are left open on the early failures, as the stock does)
const TCHAR* NMsnPassport::FetchChallenge(const TCHAR* Url)
{
    TCHAR* Headers = (TCHAR*)StockMalloc(0x4000);
    appMemset(Headers, 0, 4);
    const TCHAR* Host = appStrstr(Url, TEXT("://"));
    Host = Host ? Host + 3 : Url;
    INT HostLength = appStrlen(Host) + 1;
    TCHAR* HostCopy = (TCHAR*)StockMalloc(HostLength * 2);
    appStrncpy(HostCopy, Host, HostLength);
    TCHAR* Slash = appStrchr(HostCopy, '/');
    TCHAR* PathCopy = NULL;
    const TCHAR* Path;
    if (Slash)
    {
        INT PathLength = appStrlen(Slash) + 1;
        PathCopy = (TCHAR*)StockMalloc(PathLength * 2);
        appStrncpy(PathCopy, Slash, PathLength);
        *Slash = 0;
        Path = PathCopy;
    }
    else
        Path = TEXT("/");
    HINTERNET Internet = InternetOpenW(TEXT("MSMSGS"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0x880300);
    if (!Internet)
        return NULL;
    HINTERNET Connect = InternetConnectW(Internet, HostCopy, 443, TEXT(""), TEXT(""), INTERNET_SERVICE_HTTP, 0x280000, 0);
    if (!Connect)
        return NULL;
    HINTERNET Request = HttpOpenRequestW(Connect, TEXT("GET"), Path, NULL, TEXT(""), NULL, 0x84E8F000, 0);
    if (!Request)
        return NULL;
    if (!HttpSendRequestW(Request, Headers, appStrlen(Headers), NULL, 0))
        return NULL;
    DWORD Status;
    DWORD Length = 4;
    HttpQueryInfoW(Request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &Status, &Length, NULL);
    Length = 0x2000;
    TCHAR* Answer;
    if (Status == 302)
    {
        HttpQueryInfoW(Request, HTTP_QUERY_LOCATION, Headers, &Length, NULL);
        Answer = (TCHAR*)FetchChallenge(Headers);
    }
    else
    {
        // any other answer than 200: its status text is read first, and the length it leaves is what the headers get
        if (Status != 200 && !HttpQueryInfoW(Request, HTTP_QUERY_STATUS_TEXT, Headers, &Length, NULL))
            return NULL;
        HttpQueryInfoW(Request, HTTP_QUERY_RAW_HEADERS_CRLF, Headers, &Length, NULL);
        Answer = StockWcsdup(Headers);
    }
    InternetCloseHandle(Request);
    InternetCloseHandle(Connect);
    InternetCloseHandle(Internet);
    TCHAR* Found = appStrstr(Answer, TEXT("DALogin="));
    if (!Found)
        return NULL;
    appStrcpy(Answer, TEXT("https://"));
    StockShiftLeft(Answer + 8, (INT)(Found - Answer));
    TCHAR* Comma = appStrchr(Answer, ',');
    if (Comma)
        *Comma = 0;
    StockFree(Headers);
    StockFree(HostCopy);
    if (PathCopy)
        StockFree(PathCopy);
    return Answer;
}
NW_HOOK_METHOD(0x101418a0, NMsnPassport, FetchChallenge)

// 0x10141480 (ret 8) - the ticket for the Passport server's challenge: a GET of Challenge (the URL FetchChallenge gave) with the
// header "Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=<account, the @ as %40>,
// pwd=<password, URL-escaped>,<Argument>" (no header when there is no Argument). A certificate or authentication error opens the
// system's dialog and asks again (until the user cancels it); a redirect (302) goes to its Location; the ticket is what follows
// "from-PP='" in the answer's headers, up to the quote (a block of the stock heap), NULL when anything failed
const TCHAR* NMsnPassport::Login(const TCHAR* Challenge, const TCHAR* Argument)
{
    TCHAR* Headers = (TCHAR*)StockMalloc(0x4000);
    appMemset(Headers, 0, 4);
    TCHAR User[0x80];
    appStrcpy(User, *Account);
    TCHAR* At = appStrchr(User, '@');
    if (At)
    {
        *At = 0;
        NMSN_SNWPRINTF(Headers, 0x2000, TEXT("%s%%40%s"), User, At + 1);
        appStrcpy(User, Headers);
    }
    const TCHAR* Host = appStrstr(Challenge, TEXT("://"));
    Host = Host ? Host + 3 : Challenge;
    INT HostLength = appStrlen(Host) + 1;
    TCHAR* HostCopy = (TCHAR*)StockMalloc(HostLength * 2);
    appStrncpy(HostCopy, Host, HostLength);
    TCHAR* Slash = appStrchr(HostCopy, '/');
    TCHAR* PathCopy = NULL;
    const TCHAR* Path;
    if (Slash)
    {
        INT PathLength = appStrlen(Slash) + 1;
        PathCopy = (TCHAR*)StockMalloc(PathLength * 2);
        appStrncpy(PathCopy, Slash, PathLength);
        *Slash = 0;
        Path = PathCopy;
    }
    else
        Path = TEXT("/");
    HINTERNET Internet = InternetOpenW(TEXT("MSMSGS"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0x880300);
    if (!Internet)
        return NULL;
    HINTERNET Connect = InternetConnectW(Internet, HostCopy, 443, TEXT(""), TEXT(""), INTERNET_SERVICE_HTTP, 0x280000, 0);
    if (!Connect)
        return NULL;
    HINTERNET Request = HttpOpenRequestW(Connect, TEXT("GET"), Path, TEXT(""), TEXT(""), NULL, 0x84E8F000, 0);
    if (!Request)
        return NULL;
    if (Argument)
    {
        TCHAR Escaped[0x104];
        NMSN_SNWPRINTF(Headers, 0x2000,
            TEXT("Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%%3A%%2F%%2Fmessenger%%2Emsn%%2Ecom,sign-in=%s,pwd="), User);
        appMemset(Escaped, 0, 0x208);
        MsnUrlCanonicalize(*Password, Escaped);
        appStrcat(Headers, Escaped);
        appStrcat(Headers, TEXT(","));
        appStrcat(Headers, Argument);
    }
    else
        Headers[0] = 0;
    for (;;)
    {
        BOOL bSent = HttpSendRequestW(Request, Headers, appStrlen(Headers), NULL, 0);
        if (!bSent)
            return NULL;
        DWORD Status;
        DWORD Length = 4;
        HttpQueryInfoW(Request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &Status, &Length, NULL);
        // 0 anything else, 1 an error the system's dialog may fix (12037 .. 12038, 12040 .. 12041 are certificate errors too, 12014 is
        // the wrong password), 2 OK, 3 redirect
        INT Kind;
        if (Status > 0x2F07)
            Kind = (Status == 0x2F0B || Status == 0x2F0D) ? 1 : 0;
        else if (Status >= 0x2F05)
            Kind = 1;
        else if (Status == 200)
            Kind = 2;
        else if (Status == 302)
            Kind = 3;
        else
            Kind = (Status == 0x2EEE) ? 1 : 0;
        if (Kind == 1)
        {
            if (InternetErrorDlg(GetDesktopWindow(), Request, bSent, 7, NULL) != ERROR_CANCELLED)
                continue;
            Kind = 0;
        }
        if (Kind == 3)
        {
            Length = 0x2000;
            HttpQueryInfoW(Request, HTTP_QUERY_LOCATION, Headers, &Length, NULL);
            return Login(Headers, Argument);
        }
        if (Kind == 2)
            Headers[0] = 0;
        Length = 0x2000;
        if (Kind == 0 && !HttpQueryInfoW(Request, HTTP_QUERY_STATUS_TEXT, Headers, &Length, NULL))
            return NULL;
        HttpQueryInfoW(Request, HTTP_QUERY_RAW_HEADERS_CRLF, Headers, &Length, NULL);
        TCHAR* Ticket = StockWcsdup(Headers);
        InternetCloseHandle(Request);
        InternetCloseHandle(Connect);
        InternetCloseHandle(Internet);
        StockFree(Headers);
        StockFree(HostCopy);
        if (PathCopy)
            StockFree(PathCopy);
        if (!Ticket)
            return NULL;
        TCHAR* Found = appStrstr(Ticket, TEXT("from-PP"));
        if (!Found)
            return NULL;
        StockShiftLeft(Ticket, (INT)(Found - Ticket) + 9);
        TCHAR* Quote = appStrchr(Ticket, '\'');
        if (Quote)
            *Quote = 0;
        return Ticket;
    }
}
NW_HOOK_METHOD(0x10141480, NMsnPassport, Login)

// 0x101413a0 (ret 0x18) - the two strings made empty, then given the account and the password (copies; the callee ends the
// arguments it was given by value)
NMsnPassport::NMsnPassport(const FString& InAccount, const FString& InPassword)
{
    Account = InAccount;
    Password = InPassword;
}
NW_HOOK_CTOR_ARGS(0x101413a0, NMsnPassport, (FString InAccount, FString InPassword), (InAccount, InPassword))

// The constructor of the login object as a function of its own, taking the strings by value as the stock's does (0x101413A0; the hook
// above is the same function): the caller builds the two strings in the argument slots, this function ends them
__declspec(noinline) static NMsnPassport* MsnNewPassport(void* Memory, FString InAccount, FString InPassword)
{
    return new (Memory) NMsnPassport(InAccount, InPassword);
}

// 0x1013c270 - the password then the account go
NMsnPassport::~NMsnPassport()
{
}
NW_HOOK_DTOR(0x1013c270, NMsnPassport)

/*-----------------------------------------------------------------------------
    NMsnIntStrMap: UE2's TMap<INT, FString>.
-----------------------------------------------------------------------------*/

// 0x1013e1c0 (ret 8; the result is the hidden argument) - the string stored under the key, an empty one (FString(0, 1)) when
// there is none
FString NMsnIntStrMap::FindRef(const INT& Key)
{
    for (INT i = Hash[Key & (HashCount - 1)]; i != -1; i = Pairs(i).HashNext)
        if (Pairs(i).Key == Key)
            return Pairs(i).Value;
    return FString();
}
NW_HOOK_METHOD(0x1013e1c0, NMsnIntStrMap, FindRef)

// 0x1013e230 - TMapBase::Rehash: the chains rebuilt in a new table of HashCount heads, the old one freed
void NMsnIntStrMap::Rehash()
{
    INT* NewHash = new (TEXT("HashMapHash")) INT[HashCount];
    for (INT i = 0; i < HashCount; i++)
        NewHash[i] = -1;
    for (INT i = 0; i < Pairs.Num(); i++)
    {
        NMsnIntStrPair& Pair = Pairs(i);
        INT iHash = Pair.Key & (HashCount - 1);
        Pair.HashNext = NewHash[iHash];
        NewHash[iHash] = i;
    }
    if (Hash)
        appFree(Hash);
    Hash = NewHash;
}
NW_HOOK_METHOD(0x1013e230, NMsnIntStrMap, Rehash)

// 0x1013e9a0 (ret 8) - Remove(Index, Count) of the pairs with UnTemplate.h's checks (515 .. 517 of the stock's instance): the
// strings of the pairs destroyed first
void NMsnIntStrMap::RemovePairs(INT Index, INT Count)
{
    Pairs.Remove(Index, Count);
}
NW_HOOK_METHOD(0x1013e9a0, NMsnIntStrMap, RemovePairs)

/*-----------------------------------------------------------------------------
    The vtable's slots.
-----------------------------------------------------------------------------*/

// 0x1013c2c0 (vslot 1) - the notification connection closed: the manager ends everything, with no reason
void NotificationProcess::OnClosedImpl()
{
    Owner->NotifyNotificationProcessEnd(FString(TEXT("")));
}
NW_HOOK_METHOD(0x1013c2c0, NotificationProcess, OnClosedImpl)

// 0x10140a20 (ret 0x3C, vslot 3) - a line without a transaction. The base's handler first (MSNSocket::OnAsyncImpl, 0x10140500:
// the payload of MSG; the line goes by value), then by verb: NLN (a contact came online: argument 0 its status, 1 the account,
// 2 the name) updates the contact of the forward list and tells the window; FLN (argument 0 the account) sets the status to "FLN"
// and tells the window; RNG, LSG and LST go to their handlers; OUT (the server ends the connection, argument 0 the reason) makes the
// manager end the notification connection with that reason (the stock formats a log text "Has disconnected from NS Server %s" that
// nothing reads)
void NotificationProcess::OnAsyncImpl(MsnLine Line)
{
    MSNSocket::OnAsyncImpl(Line);
    if (Line.Verb == TEXT("NLN"))
    {
        FString Status;
        FString Account;
        FString Name;
        Status = Line.GetArg(0);
        Account = Line.GetArg(1);
        Name = Line.GetArg(2);
        NMsnFriend* Known = Owner->GetContacts()->FL->FindFriend(Account);
        if (Known)
        {
            Known->SetStatus(Status);
            Known->SetName(Name);
            Owner->NotifyFriendOnline(*Known);
        }
    }
    else if (Line.Verb == TEXT("FLN"))
    {
        FString Account = Line.GetArg(0);
        NMsnFriend* Known = Owner->GetContacts()->FL->FindFriend(Account);
        if (Known)
        {
            Known->SetStatus(FString(TEXT("FLN")));
            Owner->NotifyFriendStatus(*Known);
        }
    }
    else if (Line.Verb == TEXT("RNG"))
        HandleRNG(Line);
    else if (Line.Verb == TEXT("LSG"))
        HandleAsyncGroup(Line);
    else if (Line.Verb == TEXT("LST"))
        HandleLST(Line);
    else if (Line.Verb == TEXT("OUT"))
    {
        FString Reason = Line.GetArg(0);
        TCHAR Text[0x104];
        appSprintf(Text, TEXT("Has disconnected from NS Server %s\r\n"), *Reason);
        Owner->NotifyNotificationProcessEnd(Reason);
    }
}
typedef void (NotificationProcess::*SigNPOnAsync)(MsnLine);
NW_HOOK_VARARGS(0x10140a20, NotificationProcess, OnAsyncImpl, SigNPOnAsync)

// 0x10140df0 (ret 0x40, vslot 5) - the answer to a request, by the type kept under its transaction id (the type is the FIRST
// argument): 1 VER -> the CVR step, 2 CVR -> the USR TWN I step, 4 USR TWN I -> the Passport login, 5 USR TWN S -> the lists are
// asked for, 6 -> the SYN answer (0x10140340, stock), 7 .. 17 the answers to the user's commands (CHG, REA, XFR SB, ADD, REM, the
// list changes, the move, SYN, ADG, RMG, REG); 3 (the dispatch server's) and anything else are ignored. Every handler gets its own
// copy of the line
void NotificationProcess::OnReplyImpl(INT RequestType, MsnLine Line)
{
    switch (RequestType)
    {
    case 1:
        SendCVR(Line);
        break;
    case 2:
        SendUSRInitNS(Line);
        break;
    case 4:
        HandleUSRReply(Line);
        break;
    case 5:
        HandleLoginReply(Line);
        break;
    case 6:
        HandleSYNStart(Line);
        break;
    case 7:
        HandleCHG(Line);
        break;
    case 8:
        HandleREA(Line);
        break;
    case 9:
        HandleXFRSB(Line);
        break;
    case 10:
        HandleADD(Line);
        break;
    case 11:
        HandleREM(Line);
        break;
    case 12:
        HandleAsyncListChange(Line);
        break;
    case 13:
        HandleMoveReply(Line);
        break;
    case 14:
        HandleSYNReply(Line);
        break;
    case 15:
        HandleADG(Line);
        break;
    case 16:
        HandleRMG(Line);
        break;
    case 17:
        HandleREG(Line);
        break;
    }
}
typedef void (NotificationProcess::*SigNPOnReply)(INT, MsnLine);
NW_HOOK_VARARGS(0x10140df0, NotificationProcess, OnReplyImpl, SigNPOnReply)

/*-----------------------------------------------------------------------------
    The answers (by request type).
-----------------------------------------------------------------------------*/

// 0x1013f0b0 (ret 0x3C) - the answer to "USR TWN I": an XFR to another notification server reconnects (the stock reads the
// server and connects to the one it has); anything else is the Passport's challenge: "USR trid TWN S", the login object
// made (account and password), the challenge fetched from nexus.passport.com, the ticket asked for it; with the ticket the line
// is sent, without it error 911 goes to the window
void NotificationProcess::HandleUSRReply(MsnLine Line)
{
    if (Line.Verb == TEXT("XFR") && Line.GetArg(0) == TEXT("NS"))
    {
        Line.GetArgServer(1);
        Connect();
        return;
    }
    NMsnLocalCommand Command(FString(TEXT("USR")));
    Command.TrID = NextTrID;
    Command.AddArg(FString(TEXT("TWN")));
    Command.AddArg(FString(TEXT("S")));
    RegisterPending(5);
    NMsnPassport* Login = NULL;
    void* Memory = ::operator new(0x18);
    if (Memory)
    {
        // the constructor (account, password by value) gets the strings built in place
        Login = MsnNewPassport(Memory, Owner->GetAccount(), Owner->GetPassword());
    }
    Passport = Login;
    if (Login)
    {
        const TCHAR* Challenge = Login->FetchChallenge(TEXT("nexus.passport.com/rdr/pprdr.asp"));
        if (Challenge)
        {
            const TCHAR* Ticket = Login->Login(Challenge, *Line.GetArg(2));
            if (Ticket)
            {
                Command.AddArg(FString(Ticket));
                Send(Command);
            }
            else
                Owner->NotifyError(FString(TEXT("911")));
        }
    }
}
typedef void (NotificationProcess::*SigNPHandleUSRReply)(MsnLine);
NW_HOOK_VARARGS(0x1013f0b0, NotificationProcess, HandleUSRReply, SigNPHandleUSRReply)

// 0x1013f300 (ret 0x3C) - the answer to "USR TWN S": on USR the lists are asked for ("SYN trid serial", type 6) and the user's own
// record (account argument 1, name argument 2) goes to the manager as signed in; any other line is error 911
void NotificationProcess::HandleLoginReply(MsnLine Line)
{
    if (Line.Verb == TEXT("USR"))
    {
        NMsnLocalCommand Command(FString(TEXT("SYN")));
        Command.TrID = NextTrID;
        TCHAR Serial[0x40];
        appSprintf(Serial, TEXT("%d"), SyncSerial);
        Command.AddArg(FString(Serial));
        RegisterPending(6);
        Send(Command);
        NMsnFriend Me(Line.GetArg(1), Line.GetArg(2));
        Owner->NotifySignedIn(Me);
    }
    else
        Owner->NotifyError(FString(TEXT("911")));
}
typedef void (NotificationProcess::*SigNPHandleLoginReply)(MsnLine);
NW_HOOK_VARARGS(0x1013f300, NotificationProcess, HandleLoginReply, SigNPHandleLoginReply)

// 0x1013c480 (ret 0x3C) - the answer to SYN: with two arguments or more, argument 1 is the version of the lists
void NotificationProcess::HandleSYNReply(MsnLine Line)
{
    if (Line.Args.Num() >= 2)
    {
        FString Unused = Line.GetArg(0);
        SyncSerial = Line.GetArgInt(1);
    }
}
typedef void (NotificationProcess::*SigNPHandleSYNReply)(MsnLine);
NW_HOOK_VARARGS(0x1013c480, NotificationProcess, HandleSYNReply, SigNPHandleSYNReply)

// 0x1013d120 (ret 0x3C) - the answer to CHG: argument 0 is the presence status now; the window's list is refreshed
void NotificationProcess::HandleCHG(MsnLine Line)
{
    if (Line.Verb == TEXT("CHG"))
    {
        Status = Line.GetArg(0);
        Owner->RefreshFriends();
    }
}
typedef void (NotificationProcess::*SigNPHandleCHG)(MsnLine);
NW_HOOK_VARARGS(0x1013d120, NotificationProcess, HandleCHG, SigNPHandleCHG)

// 0x1013d270 (ret 0x3C) - the answer to REA: argument 0 the version of the lists, 1 the account, 2 the new name of the user: the
// user's record goes to the manager; any other line is shown to the manager's discard (0x1012B130)
void NotificationProcess::HandleREA(MsnLine Line)
{
    if (Line.Verb == TEXT("REA"))
    {
        INT Serial = Line.GetArgInt(0);
        FString Account = Line.GetArg(1);
        FString Name = Line.GetArg(2);
        SyncSerial = Serial;
        NMsnFriend Me(Account, Name);
        Owner->NotifyMyInfo(Me);
    }
    else
        Owner->DiscardString(Line.Verb);
}
typedef void (NotificationProcess::*SigNPHandleREA)(MsnLine);
NW_HOOK_VARARGS(0x1013d270, NotificationProcess, HandleREA, SigNPHandleREA)

// 0x1013d450 (ret 0x3C) - the answer to ADD: argument 0 the list, 1 the version, 2 the account, 3 the name; the contact is added to
// that list and, on the forward list, announced to the window
void NotificationProcess::HandleADD(MsnLine Line)
{
    if (Line.Verb == TEXT("ADD"))
    {
        FString ListName = Line.GetArg(0);
        SyncSerial = Line.GetArgInt(1);
        FString Account = Line.GetArg(2);
        FString Name = Line.GetArg(3);
        FriendList* List = Owner->GetContacts()->GetList(ListName);
        NMsnFriend Added(Account, Name);
        if (List)
        {
            List->AddFriend(Added);
            if (ListName == TEXT("FL"))
                Owner->NotifyAddFriend(Added);
        }
    }
}
typedef void (NotificationProcess::*SigNPHandleADD)(MsnLine);
NW_HOOK_VARARGS(0x1013d450, NotificationProcess, HandleADD, SigNPHandleADD)

// 0x1013d6c0 (ret 0x3C) - the answer to REM: argument 0 the list, 1 the version, 2 the account; the contact leaves that list
// and, on the forward list, the window is told
void NotificationProcess::HandleREM(MsnLine Line)
{
    if (Line.Verb == TEXT("REM"))
    {
        FString ListName = Line.GetArg(0);
        SyncSerial = Line.GetArgInt(1);
        FString Account = Line.GetArg(2);
        FriendList* List = Owner->GetContacts()->GetList(ListName);
        if (List)
            List->RemoveFriend(Account);
        if (ListName == TEXT("FL"))
            Owner->NotifyRemoveFriend(Account);
    }
}
typedef void (NotificationProcess::*SigNPHandleREM)(MsnLine);
NW_HOOK_VARARGS(0x1013d6c0, NotificationProcess, HandleREM, SigNPHandleREM)

// 0x1013d7f0 (ret 0x3C) - ADD / REM without a transaction (the server telling the user): the same arguments as the answers;
// REM takes the contact out of the list, ADD adds a contact known by its account only
void NotificationProcess::HandleAsyncListChange(MsnLine Line)
{
    INT FirstChar = appAtoi(*Line.Verb.Left(0));
    FString ListName;
    FString Account;
    if (!NMSN_ISDIGIT(FirstChar))
    {
        ListName = Line.GetArg(0);   // the stock pushes the result of the digit test (0 here) as the index
        SyncSerial = Line.GetArgInt(1);
        Account = Line.GetArg(2);
        FriendList* List = Owner->GetContacts()->GetList(ListName);
        if (List)
        {
            if (Line.Verb == TEXT("REM"))
                List->RemoveFriend(Account);
            else if (Line.Verb == TEXT("ADD"))
            {
                NMsnFriend Added(Account);
                List->AddFriend(Added);
            }
        }
    }
}
typedef void (NotificationProcess::*SigNPHandleAsyncList)(MsnLine);
NW_HOOK_VARARGS(0x1013d7f0, NotificationProcess, HandleAsyncListChange, SigNPHandleAsyncList)

// 0x1013dc20 (ret 0x3C) - the answer to ADG: argument 1 the name (unescaped), 2 the id of the new group: the group goes to the
// group list and to the window
void NotificationProcess::HandleADG(MsnLine Line)
{
    if (Line.Verb == TEXT("ADG"))
    {
        Line.GetArgInt(0);
        TCHAR Name[0x104];
        appMemset(Name, 0, 0x208);
        MsnUrlDecode(*Line.GetArg(1), Name);
        INT GroupID = Line.GetArgInt(2);
        NMsnGroupList* Groups = Owner->GetContacts()->Groups;
        NMsnGroup Group(FString(Name), GroupID);
        if (Groups)
            Groups->AddGroup(Group);
        if (Owner)
            Owner->NotifyAddGroup(Group);
    }
}
typedef void (NotificationProcess::*SigNPHandleADG)(MsnLine);
NW_HOOK_VARARGS(0x1013dc20, NotificationProcess, HandleADG, SigNPHandleADG)

// 0x1013dda0 (ret 0x3C) - the answer to RMG: argument 0 the version, 1 the id of the group that went: out of the group list; the window refreshed
void NotificationProcess::HandleRMG(MsnLine Line)
{
    if (Line.Verb == TEXT("RMG"))
    {
        INT Serial = Line.GetArgInt(0);
        INT GroupID = Line.GetArgInt(1);
        SyncSerial = Serial;
        NMsnGroupList* Groups = Owner->GetContacts()->Groups;
        if (Groups)
            Groups->RemoveGroup(GroupID);
        Owner->SortFriends();
    }
}
typedef void (NotificationProcess::*SigNPHandleRMG)(MsnLine);
NW_HOOK_VARARGS(0x1013dda0, NotificationProcess, HandleRMG, SigNPHandleRMG)

// 0x1013de40 (ret 0x3C) - the answer to REG: argument 1 the id of the group, 2 its new name (unescaped): the group renamed; the window refreshed
void NotificationProcess::HandleREG(MsnLine Line)
{
    if (Line.Verb == TEXT("REG"))
    {
        Line.GetArgInt(0);
        INT GroupID = Line.GetArgInt(1);
        TCHAR Name[0x104];
        appMemset(Name, 0, 0x208);
        MsnUrlDecode(*Line.GetArg(2), Name);
        NMsnGroupList* Groups = Owner->GetContacts()->Groups;
        if (Groups)
        {
            NMsnGroup* Group = Groups->FindGroup(GroupID);
            if (Group)
                Group->SetName(FString(Name));
        }
        Owner->SortFriends();
    }
}
typedef void (NotificationProcess::*SigNPHandleREG)(MsnLine);
NW_HOOK_VARARGS(0x1013de40, NotificationProcess, HandleREG, SigNPHandleREG)

// 0x1013d1b0 (ret 0x58) - a contact of the LST answer in one list: a contact the list already has keeps its status (the record
// takes it), the record is added (AddFriend updates a known contact), and the known contact's group becomes GroupID
void NotificationProcess::ApplyList(FriendList* List, FString Account, NMsnFriend Friend, INT GroupID)
{
    NMsnFriend* Known = List->FindFriend(Account);
    if (Known)
        Friend.SetStatus(Known->Status);
    List->AddFriend(Friend);
    if (Known)
        Known->GroupID = GroupID;
}
typedef void (NotificationProcess::*SigNPApplyList)(FriendList*, FString, NMsnFriend, INT);
NW_HOOK_VARARGS(0x1013d1b0, NotificationProcess, ApplyList, SigNPApplyList)

// 0x1013e2c0 (ret 0x3C) - an LST line: argument 0 the account, 1 the name, 2 the bits of the lists the contact is in (FL 1, AL 2, BL 4,
// RL 8), 3 the group (when there are more than three arguments, else -1). A contact on the reverse list only is somebody who
// added the user: the window is told; otherwise the contact is applied to each list its bits name
void NotificationProcess::HandleLST(MsnLine Line)
{
    FString Account = Line.GetArg(0);
    FString Name = Line.GetArg(1);
    INT Lists = Line.GetArgInt(2);
    INT GroupID = -1;
    if (Lists == 8)
    {
        NMsnFriend Contact(Account, Name);
        Owner->NotifyFriendBlocked(Contact);
        return;
    }
    if (Line.Args.Num() > 3)
        GroupID = appAtoi(*Line.GetArg(3));
    NMsnFriend Contact(Account, Name);
    Contact.GroupID = GroupID;
    NMsnContacts* Contacts = Owner->GetContacts();
    if (MsnListHasFL(Lists))
        ApplyList(Contacts->FL, Account, Contact, GroupID);
    if (MsnListHasAL(Lists))
        ApplyList(Contacts->AL, Account, Contact, GroupID);
    if (MsnListHasBL(Lists))
        ApplyList(Contacts->BL, Account, Contact, GroupID);
    if (MsnListHasRL(Lists))
        ApplyList(Contacts->RL, Account, Contact, GroupID);
}
typedef void (NotificationProcess::*SigNPHandleLST)(MsnLine);
NW_HOOK_VARARGS(0x1013e2c0, NotificationProcess, HandleLST, SigNPHandleLST)

// 0x1013df70 (ret 0x3C) - RNG: argument 0 the id of the session, 1 the switchboard server (host:port), 3 the cookie: a session is
// made for that server and id, gets the cookie, is marked as invited (it will say ANS) and started
void NotificationProcess::HandleRNG(MsnLine Line)
{
    FString SessionID = Line.GetArg(0);
    NMsnServer Server = Line.GetArgServer(1);
    FString Cookie = Line.GetArg(3);
    SwitchboardSession* Session = Owner->CreateSession(Server, SessionID);
    Session->SetCookie(Cookie);
    Session->bInvited = 1;
    Owner->AddSession(Session);
}
typedef void (NotificationProcess::*SigNPHandleRNG)(MsnLine);
NW_HOOK_VARARGS(0x1013df70, NotificationProcess, HandleRNG, SigNPHandleRNG)

// 0x1013e670 (ret 0x3C) - XFR SB: with chats asked for (Names not empty) argument 1 is the switchboard server and 3 the cookie; the
// account the user asked to call was kept under the transaction id: a session is made for them (it will say USR, then CAL)
void NotificationProcess::HandleXFRSB(MsnLine Line)
{
    if (Names.Pairs.Num() != 0)
    {
        NMsnServer Server;
        FString Cookie;
        FString Callee;
        Server = Line.GetArgServer(1);
        Cookie = Line.GetArg(3);
        Callee = Names.FindRef(Line.TrID);
        SwitchboardSession* Session = Owner->CreateSession(Server, FString(TEXT("")));
        Session->Callee = Callee;
        Session->SetCookie(Cookie);
        Session->bInvited = 0;
        Owner->AddSession(Session);
    }
}
typedef void (NotificationProcess::*SigNPHandleXFRSB)(MsnLine);
NW_HOOK_VARARGS(0x1013e670, NotificationProcess, HandleXFRSB, SigNPHandleXFRSB)

// 0x1013c2f0 (ret 0x3C) - an ADG line: argument 0 the id of the group, 1 its name (unescaped): the group goes to the group list
void NotificationProcess::HandleAsyncGroup(MsnLine Line)
{
    INT GroupID = Line.GetArgInt(0);
    TCHAR Name[0x104];
    appMemset(Name, 0, 0x208);
    MsnUrlDecode(*Line.GetArg(1), Name);
    FString GroupName(Name);
    NMsnGroupList* Groups = Owner->GetContacts()->Groups;
    NMsnGroup Group(GroupName, GroupID);
    if (Groups)
        Groups->AddGroup(Group);
}
typedef void (NotificationProcess::*SigNPHandleAsyncGroup)(MsnLine);
NW_HOOK_VARARGS(0x1013c2f0, NotificationProcess, HandleAsyncGroup, SigNPHandleAsyncGroup)

// 0x1013c6f0 (ret 0x3C, vslot 2, source "MSN\Process.cpp") - a line of the server nobody waits for. ILN (status, account, name of a
// contact that is online): the status and name go to the contact of the forward list (a new one is made if it is not there) and the
// window is told. ADD of the reverse list without a transaction: somebody added the user; if no list knows them the window is
// told. REM of the reverse list without a transaction: the contact leaves it. CHL, the server's challenge: its text (up to the CRLF)
// and the product key (the stock deciphers the product id and key with Blowfish, 0x10144650) go through MD5 (0x10126650,
// 0x10126FC0, 0x10127080) and QRY answers with the product id and "32", CRLF and the 32 hex digits of the digest
void NotificationProcess::OnCommandImpl(MsnLine Line)
{
    if (Line.Verb == TEXT("ILN"))
    {
        FString Status = Line.GetArg(0);
        FString Account = Line.GetArg(1);
        FString Name = Line.GetArg(2);
        NMsnFriend* Known = Owner->GetContacts()->FL->FindFriend(Account);
        if (Known)
        {
            Known->SetStatus(Status);
            Known->SetName(Name);
        }
        else
        {
            NMsnFriend Added(Account, Name);
            Added.SetStatus(Status);
            Owner->GetContacts()->FL->AddFriend(Added);
            Known = Owner->GetContacts()->FL->FindFriend(Account);
            if (!Known)
                appFailAssert("f", "MSN\\Process.cpp", 382);
        }
        Owner->NotifyFriendName(*Known);
    }
    else if (Line.Verb == TEXT("ADD"))
    {
        FString ListName = Line.GetArg(0);
        SyncSerial = Line.GetArgInt(1);
        FString Account = Line.GetArg(2);
        FString Name = Line.GetArg(3);
        if (ListName == TEXT("RL") && Line.TrID == 0)
        {
            NMsnFriend Added(Account, Name);
            if (!Owner->GetContacts()->FL->FindFriend(Account) && !Owner->GetContacts()->AL->FindFriend(Account) &&
                !Owner->GetContacts()->BL->FindFriend(Account) && !Owner->GetContacts()->RL->FindFriend(Account))
                Owner->NotifyFriendBlocked(Added);
        }
    }
    else if (Line.Verb == TEXT("REM") && Line.TrID == 0)
    {
        FString ListName = Line.GetArg(0);
        if (ListName == TEXT("RL"))
        {
            SyncSerial = Line.GetArgInt(1);
            FString Account = Line.GetArg(2);
            FriendList* RL = Owner->GetContacts()->RL;
            if (RL)
                RL->RemoveFriend(Account);
            Owner->DiscardString(Account);
        }
    }
    else if (Line.Verb == TEXT("CHL"))
    {
        TCHAR ProductID[0x22];
        TCHAR ProductKey[0x12];
        MsnDecipherProduct(ProductID, ProductKey);
        FString Challenge = Line.GetArg(0);
        INT Cr = Challenge.InStr(TEXT("\r\n"), 0);
        if (Cr != -1)
            Challenge = Challenge.Left(Cr);
        Challenge += ProductKey;
        char Ansi[0x104];
        appMemset(Ansi, 0, 0x104);
        MsnWideToAnsi(*Challenge, Ansi);
        INT Length = 0;
        while (Ansi[Length])
            Length++;
        DWORD Md5[0x16];
        DWORD Digest[4];
        NMd5Init((NMd5Context*)Md5);
        NMd5Update((NMd5Context*)Md5, (const BYTE*)Ansi, Length);
        NMd5Final(Digest, (NMd5Context*)Md5);
        TCHAR Hex[0x80];
        appSprintf(Hex, TEXT("32\r\n%08x%08x%08x%08x"), MsnHtonl(Digest[0]), MsnHtonl(Digest[1]), MsnHtonl(Digest[2]), MsnHtonl(Digest[3]));
        NMsnLocalCommand Command(FString(TEXT("QRY")));
        Command.TrID = NextTrID;
        Command.AddArg(FString(ProductID));
        Command.AddArg(FString(Hex));
        SendPayload(Command);
    }
}
typedef void (NotificationProcess::*SigNPOnCommand)(MsnLine);
NW_HOOK_VARARGS(0x1013c6f0, NotificationProcess, OnCommandImpl, SigNPOnCommand)

// 0x1013cfc0 (ret 4, vslot 6) - a MSG payload of the notification server: type 4 (the mail notification). With an Inbox-Unread
// header the count goes to the manager (an empty function, 0x1012F080: a lone `ret 4`, the code of every empty one-argument function of the
// image; the text is read as a number and nothing is done with it); else with a From-Addr header the address (up to the CRLF) is announced
// as new mail
void NotificationProcess::OnMessageImpl(NMsnMessage* Message)
{
    if (Message->Kind == 4)
    {
        if (Message->Headers.HasHeader(FString(TEXT("Inbox-Unread"))))
        {
            appAtoi(*Message->Headers.GetHeader(FString(TEXT("Inbox-Unread"))));
        }
        else if (Message->Headers.HasHeader(FString(TEXT("From-Addr"))))
        {
            FString From = Message->Headers.GetHeader(FString(TEXT("From-Addr")));
            INT Cr = From.InStr(TEXT("\r\n"), 0);
            if (Cr != -1)
                From = From.Left(Cr);
            Owner->NotifyNewMail(From);
        }
    }
}
typedef void (NotificationProcess::*SigNPOnMessage)(NMsnMessage*);
NW_HOOK_VARARGS(0x1013cfc0, NotificationProcess, OnMessageImpl, SigNPOnMessage)

// 0x1013da30 (ret 0x3C) - an answer with five arguments or more (list, version, account, name, group): the contact of that account in
// that list gets the group; the window removes it and adds it again
void NotificationProcess::HandleMoveReply(MsnLine Line)
{
    if (Line.Args.Num() >= 5)
    {
        FString ListName = Line.GetArg(0);
        SyncSerial = Line.GetArgInt(1);
        FString Account = Line.GetArg(2);
        FString Name = Line.GetArg(3);
        INT GroupID = Line.GetArgInt(4);
        FriendList* List = Owner->GetContacts()->GetList(ListName);
        if (List)
        {
            NMsnFriend* Known = List->FindFriend(Account);
            if (Known)
            {
                Known->GroupID = GroupID;
                Owner->NotifyRemoveFriend(Account);
                Owner->NotifyAddFriend(*Known);
            }
        }
    }
}
typedef void (NotificationProcess::*SigNPHandleMoveReply)(MsnLine);
NW_HOOK_VARARGS(0x1013da30, NotificationProcess, HandleMoveReply, SigNPHandleMoveReply)

// 0x1013c450 (ret 4; the result is the hidden argument; name ours) - a copy of the presence status
FString NotificationProcess::GetStatus()
{
    return Status;
}
NW_HOOK_METHOD(0x1013c450, NotificationProcess, GetStatus)

// 0x1013c530 - the destructor of a pair of the integer-to-string map: the string at +8 (a tail jump to FString's destructor)
NW_HOOK_DTOR(0x1013c530, NMsnIntStrPair)

#if NW_HOOK_MODE
// 0x10140890 (ret 0x18) - the base (manager, server, transaction id), the stock vtable 0x101AB190, the two strings, the map of
// pending names (a hash of 8 heads, rehashed) and the login and the version of the lists cleared; the status starts as ""
NotificationProcess* NotificationProcess::ConstructNS(Msn* InOwner, NMsnServer InServer, INT InTrID)
{
    MSNSocket::Construct(InOwner, InServer, InTrID);
    *(void**)this = ::nw::Addr(0x101AB190);
    new (&Unknown64) FString();
    new (&Status) FString();
    new (&Names.Pairs) TArray<NMsnIntStrPair>();
    Names.Hash = NULL;
    Names.HashCount = 8;
    Names.Rehash();
    Passport = NULL;
    SyncSerial = 0;
    Status = TEXT("");
    return this;
}
typedef NotificationProcess* (NotificationProcess::*SigNPConstruct)(Msn*, NMsnServer, INT);
NW_HOOK_VARARGS(0x10140890, NotificationProcess, ConstructNS, SigNPConstruct)
#else
// the standalone's constructor: the base's, then (the compiler having written the vtable and made the two strings) the map of pending names
// (a hash of 8 heads, rehashed), the login and the version of the lists cleared and the status ""
NotificationProcess::NotificationProcess(Msn* InOwner, const NMsnServer& InServer, INT InTrID) : MSNSocket(InOwner, InServer, InTrID)
{
    Names.Hash = NULL;
    Names.HashCount = 8;
    Names.Rehash();
    Passport = NULL;
    SyncSerial = 0;
    Status = TEXT("");
}
#endif

// 0x10140970 (name ours; the destructor's body) - the class' vtable, the login object destroyed and freed, the status emptied, the
// map of pending names destroyed (its hash freed and the pairs' array, 0x10140820), the two strings, then the base's destruction
void NotificationProcess::Destroy()
{
#if NW_HOOK_MODE
    *(void**)this = ::nw::Addr(0x101AB190);
#endif
    if (Passport)
    {
        Passport->~NMsnPassport();
        appFree(Passport);
        Passport = NULL;
    }
    Status = TEXT("");
    Names.Free();
    Status.~FString();
    Unknown64.~FString();
    DestroyBase();
}
NW_HOOK_METHOD(0x10140970, NotificationProcess, Destroy)

#if !NW_HOOK_MODE
/*-----------------------------------------------------------------------------
    The vtable's overrides (the stock's 0x101AB190, 11 slots; the others are MSNSocket's): the compiler builds the table in the standalone
    build, each slot being the function the hooks above sit on (the ...Impl functions take the line by value and are not virtual).
-----------------------------------------------------------------------------*/

// slot 0 (0x1013EC60): the connection is up, say VER
void NotificationProcess::OnConnected()
{
    SendVER();
}
// slot 1 (0x1013C2C0)
void NotificationProcess::OnClosed()
{
    OnClosedImpl();
}
// slot 2 (0x1013C6F0)
void NotificationProcess::OnCommand(MsnLine Line)
{
    OnCommandImpl(Line);
}
// slot 3 (0x10140A20)
void NotificationProcess::OnAsync(MsnLine Line)
{
    OnAsyncImpl(Line);
}
// slot 5 (0x10140DF0)
void NotificationProcess::OnReply(INT RequestType, MsnLine Line)
{
    OnReplyImpl(RequestType, Line);
}
// slot 6 (0x1013CFC0)
void NotificationProcess::OnMessage(NMsnMessage* Message)
{
    OnMessageImpl(Message);
}
#endif
