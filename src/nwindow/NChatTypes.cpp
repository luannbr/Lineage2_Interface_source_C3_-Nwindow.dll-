/*=============================================================================
    NChatTypes.cpp - the messenger records the conversation windows keep (C3 only; C3 addresses). Stock: appStrihash
    0x10119b20, the name pair's copy 0x10119f80, the peer pair's destructor 0x10119fb0, the user 0x1011a140 / 0x1012c460 /
    0x1012c530 / 0x1012c610, the maps' Rehash 0x1011b390 / 0x1012ca90, RemoveKey 0x1011d310, the arrays' Remove 0x1011c3e0,
    the maps' destructors 0x1011d420 / 0x1011ed50 and the pair destructor 0x1012c7f0.
=============================================================================*/
#include "nwindow/NChatTypes.h"

// 0x10119b20 (cdecl) - UE2's appStrihash: each character upper-cased (a..z only) and run twice (low byte, high byte)
// through the CRC table
DWORD NStrihash(const TCHAR* Data)
{
    DWORD Hash = 0;
    while (*Data)
    {
        TCHAR Ch = *Data++;
        if (Ch >= 'a' && Ch <= 'z')
            Ch = (TCHAR)(Ch - 0x20);
        WORD B = Ch;
        Hash = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
        B = Ch >> 8;
        Hash = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
    }
    return Hash;
}
// (not hooked: regcheck reads the stock's `mov cx, [esi]` followed by a read of ecx as a register argument. The stock
// function has nine callers (NMsnHeaderMap::Rehash / Lookup, NMsnStrStrMap::Add / Set, TMapFStringWnd::Rehash,
// NMsnWndStrMapOps::MultiFind / AddWnd, NMsnWndStrMap::FindWnd / SetWnd): all are ported, hooked and call this copy, so the
// stock one is dead in hook mode and listed in db/morto.csv)

// 0x1011c140 - the base array's destructor, out of line (Remove of every element, then ~FArray)
NChatPtrArray::~NChatPtrArray()
{
}
NW_HOOK_DTOR(0x1011C140, NChatPtrArray)

/*-----------------------------------------------------------------------------
    NChatUser.
-----------------------------------------------------------------------------*/

// 0x1011a140 - the five strings empty (the two numbers are left as they were)
NChatUser::NChatUser()
{
}
NW_HOOK_CTOR(0x1011A140, NChatUser)

// 0x1012c460 (ret 4) - the strings made empty first, then every field but the first copied (the second number, then the
// status, +8, the account, the nick and the decoded nick)
NChatUser::NChatUser(const NChatUser& Src)
{
    Unknown04 = Src.Unknown04;
    Status = Src.Status;
    Name08 = Src.Name08;
    Account = Src.Account;
    Nick = Src.Nick;
    NickDecoded = Src.NickDecoded;
}
// (not hooked here: the same record is NMsnFriend, whose copy constructor is hooked at that address in NMsnProtocol.cpp)

// 0x1012c530 - the five strings in reverse
NChatUser::~NChatUser()
{
}
// (not hooked here: the same record is NMsnFriend, whose destructor is hooked at that address in NMsnProtocol.cpp)

// 0x1012c610 (ret 4) - Nick URL-decoded into a buffer of 260 characters (InternetCanonicalizeUrlW through the wrapper 0x10144300,
// cdecl (source, destination): DecodeMsnUrl, NMsnGroup.cpp), kept in NickDecoded and returned
FString NChatUser::GetDecodedName()
{
    TCHAR Buffer[260];
    appMemset(Buffer, 0, sizeof(Buffer));
    DecodeMsnUrl(*Nick, Buffer);
    NickDecoded = Buffer;
    return NickDecoded;
}
// (not hooked here: the same record is NMsnFriend, whose GetDisplayName is hooked at that address in NMsnProtocol.cpp)

/*-----------------------------------------------------------------------------
    The people of a conversation: pair, map and the arrays' out-of-line functions.
-----------------------------------------------------------------------------*/

// 0x1011a240 (ret 4; the user comes back through a hidden pointer) - a copy of the person's user
NChatUser NChatPeer::GetUser()
{
    return User;
}
NW_HOOK_METHOD(0x1011A240, NChatPeer, GetUser)

// 0x10119fb0 (`add ecx, 0x10` then a jump into ~NChatUser)
NChatPeerPair::~NChatPeerPair()
{
}
NW_HOOK_DTOR(0x10119FB0, NChatPeerPair)

// 0x1011c3e0 (ret 8) - the stock's out-of-line TArray<NChatPeerPair>::Remove(Index, Count) (UnTemplate.h's three checks, the
// pairs' destructors, the engine's FArray::Remove), and the array's destructor (0x1011d3d0: Remove of every pair, ~FArray)
NW_HOOK_METHOD(0x1011C3E0, NChatPeerArray, Remove)
NW_HOOK_DTOR(0x1011D3D0, NChatPeerArray)

// the instance above as a function of its own, for the callers of ours (the template would be inlined into them)
__declspec(noinline) void NChatPeerArrayRemove(NChatPeerArray& Array, INT Index, INT Count)
{
    Array.Remove(Index, Count);
}

// 0x1011c1d0 - after a removal the map is relaxed
NChatPeerIter::~NChatPeerIter()
{
    if (Removed)
        Map->Relax();
}
NW_HOOK_DTOR(0x1011C1D0, NChatPeerIter)

// 0x1011b390 - UE2's TMapBase::Rehash: a table of HashCount entries, the chains rebuilt by the pairs' keys, the old table freed
void NChatPeerMap::Rehash()
{
    INT* NewHash = new (TEXT("HashMapHash")) INT[HashCount];
    for (INT i = 0; i < HashCount; i++)
        NewHash[i] = -1;
    for (INT i = 0; i < Pairs.Num(); i++)
    {
        NChatPeerPair& Pair = Pairs(i);
        INT iHash = Pair.Key & (HashCount - 1);
        Pair.HashNext = NewHash[iHash];
        NewHash[iHash] = i;
    }
    if (Hash)
        appFree(Hash);
    Hash = NewHash;
}
NW_HOOK_METHOD(0x1011B390, NChatPeerMap, Rehash)

// 0x1011d310 (ret 4) - UE2's TMapBase::Remove(Key): from the last pair down, those of that key go (through the array's
// out-of-line Remove); if any did, the table is halved while it is too big and rebuilt
INT NChatPeerMap::RemoveKey(const INT& InKey)
{
    INT Count = 0;
    for (INT i = Pairs.Num() - 1; i >= 0; i--)
    {
        if (Pairs(i).Key == InKey)
        {
            NChatPeerArrayRemove(Pairs, i, 1);
            Count++;
        }
    }
    if (Count)
        Relax();
    return Count;
}
NW_HOOK_METHOD(0x1011D310, NChatPeerMap, RemoveKey)

// 0x1011ed50 - the table freed and forgotten, then the pairs (0x1011d3d0 removes them all)
NChatPeerMap::~NChatPeerMap()
{
    if (Hash)
        appFree(Hash);
    Hash = NULL;
    HashCount = 0;
}
NW_HOOK_DTOR(0x1011ED50, NChatPeerMap)

/*-----------------------------------------------------------------------------
    The people by account: pair and map.
-----------------------------------------------------------------------------*/

// 0x10119f80 (ret 4) - the link and the pointer copied, the account copied as a string
NChatNamePair::NChatNamePair(const NChatNamePair& Src)
    : HashNext(Src.HashNext), Key(Src.Key), Value(Src.Value)
{
}
NW_HOOK_CTOR_ARGS(0x10119F80, NChatNamePair, (const NChatNamePair& A), (A))

// 0x10136400 (ret 8) - the out-of-line TArray<NChatNamePair>::Remove(Index, Count); 0x1011c340 is the array's destructor
NW_HOOK_METHOD(0x10136400, NChatNameArray, Remove)
NW_HOOK_DTOR(0x1011C340, NChatNameArray)

// 0x1011c390 (ret 4) - Empty(Slack): the pairs' destructors, the count 0, room for Slack
NW_HOOK_METHOD(0x1011C390, NChatNameArray, Empty)

// the two instances above as functions of their own, for the callers of ours (the template would be inlined into them)
__declspec(noinline) void NChatNameArrayRemove(NChatNameArray& Array, INT Index, INT Count)
{
    Array.Remove(Index, Count);
}
__declspec(noinline) void NChatNameArrayEmpty(NChatNameArray& Array, INT Slack)
{
    Array.Empty(Slack);
}

// 0x101362f0 - after a removal the map is relaxed
NChatNameIter::~NChatNameIter()
{
    if (Removed)
        Map->Relax();
}
NW_HOOK_DTOR(0x101362F0, NChatNameIter)

// 0x10131580 (cdecl) - UE2's `new(Array) T` for the name pairs: a pair added at the end, its address
__declspec(noinline) void* __cdecl NChatNameNew(size_t Size, TArray<NChatNamePair>& Array)
{
    return operator new(Size, Array);
}
NW_HOOK_FUNC(0x10131580, NChatNameNew)

// 0x1011c5b0 (ret 4) - the array's assignment: unless it is the same array, Empty(the other's count) (the instance above, called
// out of line) and a copy of each pair added at the end through the instance of `new(Array)` above. (A member of a helper type
// so that the hook takes `this` in ecx; the array and the helper are the same object.)
NChatNameArray& NChatNameArrayAssigner::Assign(const NChatNameArray& Other)
{
    if (this != &Other)
    {
        NChatNameArrayEmpty(*this, Other.Num());
        for (INT i = 0; i < Other.Num(); i++)
            new (NChatNameNew(sizeof(NChatNamePair), *this)) NChatNamePair(Other(i));
    }
    return *this;
}
NW_HOOK_METHOD(0x1011C5B0, NChatNameArrayAssigner, Assign)

// 0x1012c7f0 (`add ecx, 4` then a jump into ~FString)
NChatNamePair::~NChatNamePair()
{
}
NW_HOOK_DTOR(0x1012C7F0, NChatNamePair)

// 0x1012ca90 - UE2's TMapBase::Rehash for FString keys (appStrihash of the characters)
void NChatNameMap::Rehash()
{
    INT* NewHash = new (TEXT("HashMapHash")) INT[HashCount];
    for (INT i = 0; i < HashCount; i++)
        NewHash[i] = -1;
    for (INT i = 0; i < Pairs.Num(); i++)
    {
        NChatNamePair& Pair = Pairs(i);
        INT iHash = NStrihash(*Pair.Key) & (HashCount - 1);
        Pair.HashNext = NewHash[iHash];
        NewHash[iHash] = i;
    }
    if (Hash)
        appFree(Hash);
    Hash = NewHash;
}
// (not hooked here: the contact lists' NMsnStrMap::Rehash is hooked at that address in FriendList.cpp, the same function)

// 0x1011d420 - the table freed and forgotten, then the pairs (0x1011c340 removes them all)
NChatNameMap::~NChatNameMap()
{
    if (Hash)
        appFree(Hash);
    Hash = NULL;
    HashCount = 0;
}
NW_HOOK_DTOR(0x1011D420, NChatNameMap)
