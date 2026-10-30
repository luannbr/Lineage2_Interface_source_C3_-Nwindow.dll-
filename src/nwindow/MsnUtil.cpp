/*=============================================================================
    MsnUtil.cpp - the messenger's small helpers (C3 only; C3 addresses): the client description (0x10143D70 / 0x10143DF0 /
    0x10143E50), the character set conversions (0x10144220 / 0x10144240), the address canonicalizer (0x101442D0), and the
    protocol object's thin calls that pass a string or a flag to its session (0x1012A6F0, 0x1012A830, 0x1012AA80, 0x1012AAF0), which
    the notification connection's commands carry out (MsnLogin.cpp: 0x1013D3D0, 0x1013F4A0, 0x1013F570, 0x1013F870), and the window
    tables' functions. Stage 5: they call those functions directly (they were stock addresses), and the locale of the CVR line
    (0x10143C30) has a copy here.
=============================================================================*/
#include "nwindow/MsnUtil.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/MsnProtoOps.h"

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

/*-----------------------------------------------------------------------------
    The client description.
-----------------------------------------------------------------------------*/

// 0x10143d70 (cdecl) - the platform of the system as the protocol names it
FString* MsnPlatformName(FString* Out)
{
    TCHAR Name[32];
    OSVERSIONINFOA Info;
    Name[0] = 0;
    Info.dwOSVersionInfoSize = sizeof(Info);
#pragma warning(suppress : 4996)  // deprecated in current SDKs; the stock code calls it
    GetVersionExA(&Info);
    if (Info.dwPlatformId == VER_PLATFORM_WIN32s)
        appStrcpy(Name, TEXT("win32"));
    else if (Info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        appStrcpy(Name, TEXT("win"));
    else if (Info.dwPlatformId == VER_PLATFORM_WIN32_NT)
        appStrcpy(Name, TEXT("winnt"));
    new (Out) FString(Name);
    return Out;
}
NW_HOOK_FUNC(0x10143D70, MsnPlatformName)

// 0x10143df0 (cdecl) - "major.minor" of the system
FString* MsnSystemVersion(FString* Out)
{
    TCHAR Text[32];
    OSVERSIONINFOA Info;
    Text[0] = 0;
    Info.dwOSVersionInfoSize = sizeof(Info);
#pragma warning(suppress : 4996)
    GetVersionExA(&Info);
    appSprintf(Text, TEXT("%d.%d"), Info.dwMajorVersion, Info.dwMinorVersion);
    new (Out) FString(Text);
    return Out;
}
NW_HOOK_FUNC(0x10143DF0, MsnSystemVersion)

// 0x10143e50 (cdecl) - the processor family as the protocol names it
FString* MsnProcessorName(FString* Out)
{
    TCHAR Name[32];
    SYSTEM_INFO Info;
    Name[0] = 0;
    Info.wProcessorArchitecture = 0;
    GetSystemInfo(&Info);
    if (Info.wProcessorArchitecture == 0)
        appStrcpy(Name, TEXT("i386"));
    else if (Info.wProcessorArchitecture == 1)
        appStrcpy(Name, TEXT("mips"));
    else if (Info.wProcessorArchitecture == 2)
        appStrcpy(Name, TEXT("alpha"));
    else if (Info.wProcessorArchitecture == 3)
        appStrcpy(Name, TEXT("ppc"));
    new (Out) FString(Name);
    return Out;
}
NW_HOOK_FUNC(0x10143E50, MsnProcessorName)

// 0x10143c30 (cdecl, the result through the hidden pointer) - Util::GetLanguageID: the system's language as "0x%04x". The
// user's locale (the low word of GetUserDefaultLCID) stands for itself when it is one of those the messenger knows (a table
// of 0x401 .. 0x41B, 0x41D, 0x41F, 0x424, 0x804, 0x816 and 0xC0A); Chinese Hong Kong (0xC04) is the Taiwan one (0x404);
// any other is English (0x409). Not hooked here: NCoreHelpers.cpp hooks this address with the same body (NUtilGetLanguageID,
// static there); this is the copy the CVR line of the protocol layer calls
FString* __cdecl MsnLanguageID(FString* Result)
{
    static const BYTE DefaultFor0401[0x1B] =
    {
        0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0
    };
    INT Lcid = (WORD)GetUserDefaultLCID();
    INT Language;
    if (Lcid > 0x41D)
    {
        if (Lcid > 0x816)
        {
            if (Lcid == 0xC04)
                Language = 0x404;
            else if (Lcid == 0xC0A)
                Language = Lcid;
            else
                Language = 0x409;
        }
        else if (Lcid == 0x816 || Lcid == 0x41F || Lcid == 0x424 || Lcid == 0x804)
            Language = Lcid;
        else
            Language = 0x409;
    }
    else if (Lcid == 0x41D)
        Language = Lcid;
    else if ((UINT)(Lcid - 0x401) > 0x1A)
        Language = 0x409;
    else
        Language = DefaultFor0401[Lcid - 0x401] ? 0x409 : Lcid;
    FString Text = FString::Printf(TEXT("0x%04x"), (WORD)Language);
    ::new ((void*)Result) FString(Text);
    return Result;
}

/*-----------------------------------------------------------------------------
    Conversions.
-----------------------------------------------------------------------------*/

// 0x10144220 (cdecl)
INT MsnWideToAnsi(const TCHAR* Wide, char* Out)
{
    return WideCharToMultiByte(CP_ACP, 0, Wide, -1, Out, 0x1040, NULL, NULL);
}
NW_HOOK_FUNC(0x10144220, MsnWideToAnsi)

// 0x10144240 (cdecl)
INT MsnAnsiToWide(const char* Ansi, TCHAR* Out)
{
    return MultiByteToWideChar(CP_ACP, 0, Ansi, -1, Out, 0x1040);
}
NW_HOOK_FUNC(0x10144240, MsnAnsiToWide)

// 0x101442d0 (cdecl)
INT MsnUrlCanonicalize(const TCHAR* Url, TCHAR* Out)
{
    DWORD Length = 0x104;
    return UrlCanonicalizeW(Url, Out, &Length, 0x28001000);
}
NW_HOOK_FUNC(0x101442D0, MsnUrlCanonicalize)

// 0x10144650 (cdecl (ProductID, ProductKey), both out buffers of 0x1040 characters) - the messenger's product id and product key,
// kept enciphered (16 bytes each) in the image, deciphered with the Blowfish key "[;'.]94-31==-%&@!^+]" (21 bytes with the
// terminator; the key schedule is made again for each) and read as ANSI text. The stock reads the two blocks from its data
// (0x101AB8B4, 0x101AB8C4)
void MsnDecipherProduct(TCHAR* ProductID, TCHAR* ProductKey)
{
    static const BYTE EncryptedID[16] = {0x2e, 0xdf, 0xa0, 0xc5, 0xfb, 0x77, 0x11, 0x57,
                                         0xc5, 0xca, 0x20, 0x35, 0x09, 0x02, 0x10, 0xc5};
    static const BYTE EncryptedKey[16] = {0x6b, 0x42, 0x85, 0x43, 0xe3, 0x74, 0x24, 0x1c,
                                          0x1c, 0xb3, 0xb5, 0x2e, 0x0c, 0xcc, 0x44, 0x10};
    FBlowFish Blow;
    BYTE Key[0x15];
    BYTE Buffer[0x104];
    memcpy(Key, "[;'.]94-31==-%&@!^+]", 0x15);
    appMemset(Buffer, 0, 0x104);
    memcpy(Buffer, EncryptedID, 16);
    Blow.InitializeBlowfish(Key, 0x15);
    Blow.BlowfishDecrypt(Buffer, 0x10);
    MsnAnsiToWide((const char*)Buffer, ProductID);
    appMemset(Buffer, 0, 0x104);
    memcpy(Buffer, EncryptedKey, 16);
    Blow.InitializeBlowfish(Key, 0x15);
    Blow.BlowfishDecrypt(Buffer, 0x10);
    MsnAnsiToWide((const char*)Buffer, ProductKey);
}
NW_HOOK_FUNC(0x10144650, MsnDecipherProduct)

/*-----------------------------------------------------------------------------
    The protocol object's thin calls (it holds the session, the notification connection, at +0xC; the call goes to the session when
    there is one). The strings are given by value and freed here, the stock way (stage 5: Msn's own members; they were methods of a
    stand-in type that called the session's stock addresses).
-----------------------------------------------------------------------------*/

// 0x1012a6f0 - the session signs out
void Msn::SignOut()
{
    if (Notification)
        Notification->SendSignOut();
}
NW_HOOK_METHOD(0x1012A6F0, Msn, SignOut)

// 0x1012a830 (ret 0x10) - the account is blocked (bBlock set) or unblocked
void Msn::Block(NMsnStringArg Account, INT bBlock)
{
    if (Notification)
        Notification->SendBlock(MsnStringCopy(Account.Str()), bBlock);
    Account.Str().~FString();
}
NW_HOOK_VARARGS(0x1012A830, Msn, Block, void (Msn::*)(NMsnStringArg, INT))

// 0x1012aa80 (ret 0xC) - the presence code ("NLN", "BSY" ...) goes to the session
void Msn::SetStatus(NMsnStringArg Code)
{
    if (Notification)
        Notification->SendStatus(MsnStringCopy(Code.Str()));
    Code.Str().~FString();
}
NW_HOOK_VARARGS(0x1012AA80, Msn, SetStatus, void (Msn::*)(NMsnStringArg))

// 0x1012aaf0 (ret 0xC) - the name shown goes to the session
void Msn::SetDisplayName(NMsnStringArg Name)
{
    if (Notification)
        Notification->SendDisplayName(MsnStringCopy(Name.Str()));
    Name.Str().~FString();
}
NW_HOOK_VARARGS(0x1012AAF0, Msn, SetDisplayName, void (Msn::*)(NMsnStringArg))

/*-----------------------------------------------------------------------------
    The tables (UE2's TMap of the messenger's windows, the pairs in an array chained through a hash of the key).
-----------------------------------------------------------------------------*/

// 0x10119b20 (cdecl) - the hash of a text (case-insensitive CRC): the C++ port in NChatTypes.cpp, so that no ported code enters the
// stock function (it is not hooked: regcheck reads its partial cx as a register argument)
DWORD NStrihash(const TCHAR* Data);
static FORCEINLINE INT MsnHashText(const TCHAR* Text)
{
    return (INT)NStrihash(Text);
}

// 0x101347d0 (ret 4) - the window entered by this name
NCWnd* NMsnWndStrMap::FindWnd(const FString* Key)
{
    INT Index = Hash[MsnHashText(**Key) & (HashCount - 1)];
    while (Index != -1)
    {
        NMsnWndStrPair& Pair = Data[Index];
        if (Pair.Key == *Key)
            return Pair.Wnd;
        Index = Pair.HashNext;
    }
    return NULL;
}
NW_HOOK_METHOD(0x101347D0, NMsnWndStrMap, FindWnd)

// 0x101361b0 (ret 8) - the window entered by this name becomes Value (a new entry, made by the protocol lane's 0x10134840, when
// there is none); the place of the window in the table is given back
NCWnd** NMsnWndStrMap::SetWnd(const TCHAR* Key, NCWnd** Value)
{
    INT Index = Hash[MsnHashText(Key) & (HashCount - 1)];
    while (Index != -1)
    {
        NMsnWndStrPair& Pair = Data[Index];
        if (Pair.Key == Key)
        {
            Pair.Wnd = *Value;
            return &Data[Index].Wnd;
        }
        Index = Pair.HashNext;
    }
    return ((NMsnWndStrMapOps*)this)->AddWnd(Key, Value);   // 0x10134840 (SwitchboardSession.cpp)
}
NW_HOOK_METHOD(0x101361B0, NMsnWndStrMap, SetWnd)

// 0x1011a030 (cdecl; the size is not read) - room for one more 12-byte pair at the end of the array, its address. Hooked in
// NCBroadcastGMWnd.cpp as NewStringArrayElement (static there; the same code); this is the copy the window tables call
__declspec(noinline) static void* __cdecl MsnNewIntPair(size_t Size, FArray& Array)
{
    INT Index = Array.Add(1, 0xC);
    return (BYTE*)Array.GetData() + Index * 0xC;
}

// 0x10136230 (ret 8) - the window entered by this number becomes Value (a new pair at the end of the array, chained into the
// hash, the hash doubled when the pairs outgrow it, when there is none); the place of the window in the table is given back
NCWnd** NMsnWndIntMap::SetWnd(const INT* Key, NCWnd** Value)
{
    INT Number = *Key;
    INT Index = Hash[Number & (HashCount - 1)];
    while (Index != -1)
    {
        NMsnWndIntPair& Pair = Data[Index];
        if (Pair.Key == Number)
        {
            Pair.Wnd = *Value;
            return &Data[Index].Wnd;
        }
        Index = Pair.HashNext;
    }
    NMsnWndIntPair* Pair = (NMsnWndIntPair*)MsnNewIntPair(0xC, *(FArray*)this);   // room for one more at the end (0x1011A030)
    if (Pair)
    {
        Pair->Key = *Key;
        Pair->Wnd = *Value;
    }
    INT Slot = Pair->Key & (HashCount - 1);
    Pair->HashNext = Hash[Slot];
    Hash[Slot] = Num - 1;
    if (HashCount * 2 + 8 < Num)
    {
        HashCount = HashCount * 2;
        ((NMsnIntMap*)this)->Rehash();   // 0x100626E0
    }
    return &Pair->Wnd;
}
NW_HOOK_METHOD(0x10136230, NMsnWndIntMap, SetWnd)

// 0x10137450 (ret 8) - the entries of this name that hold this window go, from the last to the first (the removal is the chat
// lane's 0x10136400); when some went, the hash is made smaller while it is more than twice the pairs plus 8 and rebuilt; the
// number removed comes back
INT NMsnWndStrMap::RemoveWnd(const TCHAR* Key, NCWnd** Value)
{
    INT Removed = 0;
    for (INT i = Num - 1; i >= 0; i--)
    {
        if (Data[i].Key == Key && Data[i].Wnd == *Value)
        {
            NChatNameArrayRemove(*(NChatNameArray*)this, i, 1);   // 0x10136400
            Removed++;
        }
    }
    if (Removed)
    {
        if (HashCount > Num * 2 + 8)
        {
            do
                HashCount = HashCount / 2;
            while (HashCount > Num * 2 + 8);
        }
        ((NMsnStrMap*)this)->Rehash();   // 0x1012CA90 (FriendList.cpp)
    }
    return Removed;
}
NW_HOOK_METHOD(0x10137450, NMsnWndStrMap, RemoveWnd)

// 0x10137560 - all the pairs removed (through the chat lane's 0x10136400, which frees each name), then the array's destructor
void NMsnWndStrMap::FreePairs()
{
    NChatNameArrayRemove(*(NChatNameArray*)this, 0, Num);   // 0x10136400
    ((FArray*)this)->~FArray();
    Data = NULL;                     // (nothing reads it again; the store keeps this a function that returns, as the stock one does)
}
NW_HOOK_METHOD(0x10137560, NMsnWndStrMap, FreePairs)

// 0x10139760 - the hash freed, then the pairs
void NMsnWndStrMap::Free()
{
    if (Hash)
        appFree(Hash);
    Hash = NULL;
    HashCount = 0;
    FreePairs();
    Num = 0;
}
NW_HOOK_METHOD(0x10139760, NMsnWndStrMap, Free)

// 0x101364a0 - the array of pairs of numbers emptied with UnTemplate.h's checks, then freed (the pairs have nothing to free)
void NMsnWndIntMap::FreePairs()
{
    typedef TArray<NMsnWndIntPair> PairArray;
    Pairs().~PairArray();
}
NW_HOOK_METHOD(0x101364A0, NMsnWndIntMap, FreePairs)

// 0x101375b0 - the hash freed, then the pairs
void NMsnWndIntMap::Free()
{
    if (Hash)
        appFree(Hash);
    Hash = NULL;
    HashCount = 0;
    FreePairs();
}
NW_HOOK_METHOD(0x101375B0, NMsnWndIntMap, Free)

