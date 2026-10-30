/*=============================================================================
    NMsnProtocol.cpp - the messenger's protocol layer (C3 only): the contact records (NMsnFriend, NMsnGroup,
    NMsnServer) and the free helpers. Stock: 0x1012C2C0 .. 0x1012C710 (friend record), 0x1012DA80 .. 0x1012DBD0 (group
    record), 0x10127350 / 0x101273B0 (server address). The connection classes are in Msn.cpp, MSNSocket.cpp,
    DispatchProcess.cpp, NotificationProcess.cpp, SwitchboardSession.cpp, FriendList.cpp and MsnCommand.cpp.
=============================================================================*/
#include "nwindow/NMsnProtocol.h"

/*-----------------------------------------------------------------------------
    NMsnFriend.
-----------------------------------------------------------------------------*/

// 0x1012c2c0 (ret 0xC) - a contact known by its account only: offline (FLN), group 0 (the string comes by value)
NMsnFriend::NMsnFriend(const FString& InAccount)
{
    GroupID = 0;
    Account = InAccount;
    Status = TEXT("FLN");
}
NW_HOOK_CTOR_ARGS(0x1012c2c0, NMsnFriend, (FString InAccount), (InAccount))

// 0x1012c380 (ret 0x18) - account and friendly name, offline, group 0
NMsnFriend::NMsnFriend(const FString& InAccount, const FString& InName)
{
    GroupID = 0;
    Account = InAccount;
    Name = InName;
    Status = TEXT("FLN");
}
NW_HOOK_CTOR_ARGS(0x1012c380, NMsnFriend, (FString InAccount, FString InName), (InAccount, InName))

// 0x1012c460 (ret 4) - the copy: the five strings are built empty, then assigned (the group first, then the strings in the
// order Status / PrevStatus... of the stock)
NMsnFriend::NMsnFriend(const NMsnFriend& Other)
{
    GroupID = Other.GroupID;
    Status = Other.Status;
    PrevStatus = Other.PrevStatus;
    Account = Other.Account;
    Name = Other.Name;
    DisplayName = Other.DisplayName;
}
NW_HOOK_CTOR_ARGS(0x1012c460, NMsnFriend, (const NMsnFriend& Other), (Other))

// 0x1012c530 - the five strings go in reverse
NMsnFriend::~NMsnFriend()
{
}
NW_HOOK_DTOR(0x1012c530, NMsnFriend)

// 0x1012c5b0 (ret 4) - the same assignments on an existing record
NMsnFriend& NMsnFriend::operator=(const NMsnFriend& Other)
{
    GroupID = Other.GroupID;
    Status = Other.Status;
    PrevStatus = Other.PrevStatus;
    Account = Other.Account;
    Name = Other.Name;
    DisplayName = Other.DisplayName;
    return *this;
}
NW_HOOK_METHOD(0x1012c5b0, NMsnFriend, operator=)

// 0x1012c610 (ret 4: the result is the hidden argument) - the friendly name unescaped (InternetCanonicalizeUrlW, 0x104
// characters) into DisplayName, and a copy of it
FString NMsnFriend::GetDisplayName()
{
    TCHAR Buffer[0x104];
    appMemset(Buffer, 0, 0x208);
    MsnUrlDecode(*Name, Buffer);
    DisplayName = Buffer;
    return DisplayName;
}
NW_HOOK_METHOD(0x1012c610, NMsnFriend, GetDisplayName)

// 0x1012c800 (ret 0xC) - a new friendly name (by value)
void NMsnFriend::SetName(FString NewName)
{
    Name = NewName;
}
typedef void (NMsnFriend::*SigMsnFriendSetName)(FString);
NW_HOOK_VARARGS(0x1012c800, NMsnFriend, SetName, SigMsnFriendSetName)

// 0x1012c850 (ret 0xC) - a new status (by value): the old one is kept as PrevStatus
void NMsnFriend::SetStatus(FString NewStatus)
{
    PrevStatus = Status;
    Status = NewStatus;
}
typedef void (NMsnFriend::*SigMsnFriendSetStatus)(FString);
NW_HOOK_VARARGS(0x1012c850, NMsnFriend, SetStatus, SigMsnFriendSetStatus)

// 0x1012c6d0 / 0x1012c6e0 / 0x1012c6f0 / 0x1012c700 (stdcall, ret 4) - is the contact on the forward / allow / block /
// reverse list
INT __stdcall MsnListHasFL(INT Lists)
{
    return (Lists & MSNLIST_FL) == MSNLIST_FL;
}
NW_HOOK_FUNC(0x1012c6d0, MsnListHasFL)
INT __stdcall MsnListHasAL(INT Lists)
{
    return (Lists & MSNLIST_AL) == MSNLIST_AL;
}
NW_HOOK_FUNC(0x1012c6e0, MsnListHasAL)
INT __stdcall MsnListHasBL(INT Lists)
{
    return (Lists & MSNLIST_BL) == MSNLIST_BL;
}
NW_HOOK_FUNC(0x1012c6f0, MsnListHasBL)
INT __stdcall MsnListHasRL(INT Lists)
{
    return (Lists & MSNLIST_RL) == MSNLIST_RL;
}
NW_HOOK_FUNC(0x1012c700, MsnListHasRL)

/*-----------------------------------------------------------------------------
    NMsnGroup.
-----------------------------------------------------------------------------*/

// 0x1012da80 (ret 0x10) - a group of that name and id
NMsnGroup::NMsnGroup(const FString& InName, INT InGroupID)
{
    Name = InName;
    GroupID = InGroupID;
}
NW_HOOK_CTOR_ARGS(0x1012da80, NMsnGroup, (FString InName, INT InGroupID), (InName, InGroupID))

// 0x1012db00 (ret 4) - the copy
NMsnGroup::NMsnGroup(const NMsnGroup& Other)
{
    Name = Other.Name;
    GroupID = Other.GroupID;
    EncodedName = Other.EncodedName;
}
NW_HOOK_CTOR_ARGS(0x1012db00, NMsnGroup, (const NMsnGroup& Other), (Other))

// 0x1012db80 - the two strings in reverse
NMsnGroup::~NMsnGroup()
{
}
NW_HOOK_DTOR(0x1012db80, NMsnGroup)

// 0x1012dbd0 (ret 0xC) - the name (by value, as the server sends it) and the same with every %20 turned into a space: the stock passes
// the literal "%20" (0x101A9978) as the text to look for and " " (0x101A9328) as its replacement (ours had them the other way round;
// C4 fixed the same line in d6d5fc2, found by its t_msn10.py)
void NMsnGroup::SetName(FString NewName)
{
    Name = NewName;
    MsnStrReplace(&NewName, FString(TEXT("%20")), FString(TEXT(" ")));
    EncodedName = NewName;
}
typedef void (NMsnGroup::*SigMsnGroupSetName)(FString);
NW_HOOK_VARARGS(0x1012dbd0, NMsnGroup, SetName, SigMsnGroupSetName)

/*-----------------------------------------------------------------------------
    NMsnServer.
-----------------------------------------------------------------------------*/

// 0x10127350 - the default: the Hotmail dispatch server, port 1863 (0x747)
NMsnServer::NMsnServer()
{
    Host = TEXT("messenger.hotmail.com");
    Port = 1863;
}
NW_HOOK_CTOR(0x10127350, NMsnServer)

// 0x101273b0 (ret 0x10) - that host and port
NMsnServer::NMsnServer(const FString& InHost, INT InPort)
{
    Host = InHost;
    Port = InPort;
}
NW_HOOK_CTOR_ARGS(0x101273b0, NMsnServer, (FString InHost, INT InPort), (InHost, InPort))

/*-----------------------------------------------------------------------------
    NMsnStrStrMap: UE2's TMap<FString, FString> (the header lines of a message).
-----------------------------------------------------------------------------*/

// 0x101272d0 (ret 8) - a pair of the two texts (the chain link is written by the map)
NMsnStrStrPair::NMsnStrStrPair(const TCHAR* InKey, const TCHAR* InValue) : Key(InKey), Value(InValue)
{
}
NW_HOOK_CTOR_ARGS(0x101272d0, NMsnStrStrPair, (const TCHAR* InKey, const TCHAR* InValue), (InKey, InValue))

// 0x1011b220 - TMapBase::Rehash of this map: NMsnHeaderMap::Rehash (NChatMsg.cpp, hooked there), the same 0x14-byte map of string pairs
void NMsnStrStrMap::Rehash()
{
    ((NMsnHeaderMap*)this)->Rehash();
}

// 0x10127a50 (ret 8) - a new pair at the end, entered at the head of its hash chain; when the hash is narrower than half the pairs it
// doubles and is rebuilt; the value of the pair comes back
FString* NMsnStrStrMap::Add(const TCHAR* Key, const TCHAR* Value)
{
    INT Index = Pairs.Add(1);
    NMsnStrStrPair* Pair = new (&Pairs(Index)) NMsnStrStrPair(Key, Value);
    INT iHash = MsnStrHash(*Pair->Key) & (HashCount - 1);
    Pair->HashNext = Hash[iHash];
    Hash[iHash] = Pairs.Num() - 1;
    if (HashCount * 2 + 8 < Pairs.Num())
    {
        HashCount *= 2;
        Rehash();
    }
    return &Pair->Value;
}
NW_HOOK_METHOD(0x10127a50, NMsnStrStrMap, Add)

// 0x10129a00 (ret 8) - the value stored under the key (replaced when there is one, else a new pair), the value comes back
FString* NMsnStrStrMap::Set(const TCHAR* Key, const TCHAR* Value)
{
    for (INT i = Hash[MsnStrHash(Key) & (HashCount - 1)]; i != -1; i = Pairs(i).HashNext)
    {
        if (Pairs(i).Key == Key)
        {
            Pairs(i).Value = Value;
            return &Pairs(i).Value;
        }
    }
    return Add(Key, Value);
}
NW_HOOK_METHOD(0x10129a00, NMsnStrStrMap, Set)

/*-----------------------------------------------------------------------------
    NMsnMessage: the parsing of the payload.
-----------------------------------------------------------------------------*/

// 0x1012a140 (ret 0xC, the line by value) - "Name: value": a header line enters the map; the Invitation-Cookie header is kept as
// well; 0 for a line without ": ", else 1
INT NMsnMessage::ParseHeader(FString Line)
{
    INT Colon = Line.InStr(TEXT(": "), 0);
    if (Colon == -1)
        return 0;
    FString Name = Line.Left(Colon);
    FString Value = Line.Right(Line.Len() - Colon - 2);
    if (appStrcmp(*Name, TEXT("Invitation-Cookie")) == 0)
        Cookie = Value;
    MsnMessageHeaders(*this).Set(*Name, *Value);
    return 1;
}
typedef INT (NMsnMessage::*SigMsgParseHeader)(FString);
NW_HOOK_VARARGS(0x1012a140, NMsnMessage, ParseHeader, SigMsgParseHeader)

// 0x1012a270 (ret 0xC, the lines by value) - the lines of a payload: the first is the MIME version (its value after the colon is
// kept), the second names the content type, which gives the type of the message (control 1, profile 0, e-mail notification 4,
// invitation 3, plain text 2, any other 10) and how the lines go on (header lines, the plain text's body from the fifth line)
void NMsnMessage::Parse(TArray<FString*> Lines)
{
    if (Lines.Num() >= 3)
    {
        FString Mime(*Lines(0));
        INT Colon = Mime.InStr(TEXT(":"), 0);
        Unknown10 = Mime.Right(Mime.Len() - Colon - 2);
        Mime = *Lines(1);
        if (Mime.InStr(TEXT("text/x-msmsgscontrol"), 0) != -1)
        {
            Kind = 1;
            ParseHeader(*Lines(2));
        }
        else if (Mime.InStr(TEXT("text/x-msmsgsprofile"), 0) != -1)
        {
            Kind = 0;
            for (INT i = 2; i < Lines.Num(); i++)
                ParseHeader(*Lines(i));
        }
        else if (Mime.InStr(TEXT("text/x-msmsgsemailnotification"), 0) != -1)
        {
            Kind = 4;
            for (INT i = 2; i < Lines.Num(); i++)
                ParseHeader(*Lines(i));
        }
        else if (Mime.InStr(TEXT("text/x-msmsgsinvite"), 0) != -1)
        {
            Kind = 3;
            for (INT i = 2; i < Lines.Num(); i++)
                ParseHeader(*Lines(i));
        }
        else if (Mime.InStr(TEXT("text/plain"), 0) != -1)
        {
            Kind = 2;
            ParseHeader(*Lines(2));
            for (INT i = 4; i < Lines.Num(); i++)
                Body += *Lines(i);
        }
        else
            Kind = 10;
    }
}
typedef void (NMsnMessage::*SigMsgParse)(TArray<FString*>);
NW_HOOK_VARARGS(0x1012a270, NMsnMessage, Parse, SigMsgParse)

// 0x10143ed0 (cdecl; the two strings by value, destroyed on return) - every From in Str is replaced by To. The stock cuts one character too
// many after each match and keeps the whole rest (match included) when the match is at the end or one before it: reproduced. Not hooked
// here: ChatWnd.cpp hooks this address with the same body (NMsnStrReplaceImpl, static there); this is the copy the protocol layer calls
__declspec(noinline) void __cdecl MsnStrReplaceImpl(FString* Str, FString From, FString To)
{
    if (Str->Len() > 0)
    {
        FString Result;
        INT FromLen = From.Len();
        INT Pos = Str->InStr(*From, 0);
        while (Pos != -1)
        {
            Result += Str->Left(Pos);
            Result += To;
            if (Str->Len() <= Pos + FromLen + 1)
                break;
            *Str = Str->Right(Str->Len() - Pos - FromLen - 1);
            Pos = Str->InStr(*From, 0);
        }
        if (Str->Len() > 0)
            Result += *Str;
        *Str = Result;
    }
}

// 0x10144260 (cdecl) - the UTF-8 text (up to its end) converted to wide characters in a buffer of 0x1040 characters (no terminator
// is added: the callers clear the buffer first)
INT __cdecl MsnUtf8ToWide(const char* In, TCHAR* Out)
{
    INT Length = 0;
    while (In[Length])
        Length++;
    return MultiByteToWideChar(65001, 0, In, Length, (LPWSTR)Out, 0x1040);
}
NW_HOOK_FUNC(0x10144260, MsnUtf8ToWide)
