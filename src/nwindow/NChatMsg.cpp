/*=============================================================================
    NChatMsg.cpp - the messenger's wire records (C3 only; C3 addresses). Stock: the message's builders 0x10127110 ..
    0x10127980, the command's 0x10127420 .. 0x101276d0, the header map's Rehash 0x1011b220, destructor 0x1011ebe0 and lookups
    0x101279c0 / 0x101298b0 / 0x10129950 (the first stays stock), the arrays' instances 0x1011c510 / 0x1011d380 / 0x10129a80 /
    0x10129ad0 / 0x10127330, the pairs' 0x10127270 / 0x10119fc0, the command's 0x101295a0 / 0x10129770 / 0x10129810 and the
    message's constructor 0x1011ec50, setters 0x1011a050 .. 0x1011a0f0, copy 0x1012a5c0 and destructor 0x1012a510.
=============================================================================*/
#include "nwindow/NChatMsg.h"

/*-----------------------------------------------------------------------------
    The headers.
-----------------------------------------------------------------------------*/

// 0x10127270 (ret 4) - the link, and the two strings copied
NMsnHeaderPair::NMsnHeaderPair(const NMsnHeaderPair& Src)
    : HashNext(Src.HashNext), Key(Src.Key), Value(Src.Value)
{
}
NW_HOOK_CTOR_ARGS(0x10127270, NMsnHeaderPair, (const NMsnHeaderPair& A), (A))

// 0x10119fc0 - the value, then the key
NMsnHeaderPair::~NMsnHeaderPair()
{
}
NW_HOOK_DTOR(0x10119FC0, NMsnHeaderPair)

// the stock's out-of-line instances of TArray<NMsnHeaderPair>: Remove(Index, Count) (UnTemplate.h's checks, the pairs'
// destructors, FArray::Remove), the destructor (Remove of all, ~FArray) and Empty(Slack)
NW_HOOK_METHOD(0x1011C510, NMsnHeaderArray, Remove)
NW_HOOK_DTOR(0x1011D380, NMsnHeaderArray)
NW_HOOK_METHOD(0x10129A80, NMsnHeaderArray, Empty)

// 0x10127330 (cdecl) - UE2's `new(Array) T` for the header pairs: a pair added at the end, its address
__declspec(noinline) static void* __cdecl NMsnHeaderNew(size_t Size, TArray<NMsnHeaderPair>& Array)
{
    return operator new(Size, Array);
}
NW_HOOK_FUNC(0x10127330, NMsnHeaderNew)

// The stock calls the array's Empty(Slack) (0x10129a80) as a function of its own; this is that out-of-line instance (the compiler would
// inline the template into every caller)
__declspec(noinline) static void NMsnHeaderEmpty(NMsnHeaderArray& Array, INT Slack)
{
    Array.Empty(Slack);
}

// 0x10129ad0 (ret 4) - the array's assignment: unless it is the same array, Empty(the other's count) (0x10129a80, out of line) and
// a copy of each pair added at the end through `new(Array)` above. (A member of a helper type so that the hook takes `this` in
// ecx; the array and the helper are the same object.)
NMsnHeaderArray& NMsnHeaderArrayAssigner::Assign(const NMsnHeaderArray& Other)
{
    if (this != &Other)
    {
        NMsnHeaderEmpty(*this, Other.Num());
        for (INT i = 0; i < Other.Num(); i++)
            new (NMsnHeaderNew(sizeof(NMsnHeaderPair), *this)) NMsnHeaderPair(Other(i));
    }
    return *this;
}
NW_HOOK_METHOD(0x10129AD0, NMsnHeaderArrayAssigner, Assign)

// 0x1011b220 - UE2's TMapBase::Rehash for FString keys
void NMsnHeaderMap::Rehash()
{
    INT* NewHash = new (TEXT("HashMapHash")) INT[HashCount];
    for (INT i = 0; i < HashCount; i++)
        NewHash[i] = -1;
    for (INT i = 0; i < Pairs.Num(); i++)
    {
        NMsnHeaderPair& Pair = Pairs(i);
        INT iHash = NStrihash(*Pair.Key) & (HashCount - 1);
        Pair.HashNext = NewHash[iHash];
        NewHash[iHash] = i;
    }
    if (Hash)
        appFree(Hash);
    Hash = NewHash;
}
NW_HOOK_METHOD(0x1011B220, NMsnHeaderMap, Rehash)

// 0x1011ebe0 - the table freed and forgotten, then the pairs
NMsnHeaderMap::~NMsnHeaderMap()
{
    if (Hash)
        appFree(Hash);
    Hash = NULL;
    HashCount = 0;
}
NW_HOOK_DTOR(0x1011EBE0, NMsnHeaderMap)

// 0x101279c0 (ret 8) - the value of the header (case-insensitive key, hashed by NStrihash), a copy; an empty string when there is none.
// Not hooked here: ChatWnd.cpp hooks this address with the same body (NChatHeaderMap::Lookup); this is the copy the message's own functions call
__declspec(noinline) FString NMsnHeaderMap::Lookup(const FString& Key)
{
    for (INT i = Hash[NStrihash(*Key) & (HashCount - 1)]; i != -1; i = Pairs(i).HashNext)
    {
        if (Pairs(i).Key == Key)
            return Pairs(i).Value;
    }
    return FString();
}

// 0x10129950 (ret 0xC) - the header's value (0x101279c0 finds it, an empty string when there is none): is there one
UBOOL NMsnHeaderMap::HasHeader(FString Key)
{
    FString Value = Lookup(Key);
    return Value.Len() > 0;
}
typedef UBOOL (NMsnHeaderMap::*SigHasHeader)(FString);
NW_HOOK_VARARGS(0x10129950, NMsnHeaderMap, HasHeader, SigHasHeader)

// 0x101298b0 (ret 0x10) - the value of the header (the lookup takes the key as a pointer to a copy)
FString NMsnHeaderMap::GetHeader(FString Key)
{
    FString Copy(*Key);
    return Lookup(Copy);
}
typedef FString (NMsnHeaderMap::*SigGetHeader)(FString);
NW_HOOK_VARARGS(0x101298B0, NMsnHeaderMap, GetHeader, SigGetHeader)

/*-----------------------------------------------------------------------------
    NMsnMessage.
-----------------------------------------------------------------------------*/

// 0x1011ec50 - the strings and the map empty (the kind and the color are left as they were)
NMsnMessage::NMsnMessage()
{
}
NW_HOOK_CTOR(0x1011EC50, NMsnMessage)

// 0x1012a5c0 (ret 4) - the members made empty first; then the kind, the three strings, the headers (the pairs emptied, the map
// rebuilt empty, then the other message's pairs copied and the map rebuilt with its table size), the font name, the effects
// and the color. The pair arrays' Empty and operator= are the out-of-line instances
NMsnMessage::NMsnMessage(const NMsnMessage& Src)
{
    Kind = Src.Kind;
    Body = Src.Body;
    Unknown10 = Src.Unknown10;
    Cookie = Src.Cookie;
    NMsnHeaderEmpty(Headers.Pairs, 0);
    Headers.HashCount = 8;
    Headers.Rehash();
    ((NMsnHeaderArrayAssigner&)Headers.Pairs).Assign(Src.Headers.Pairs);
    Headers.HashCount = Src.Headers.HashCount;
    Headers.Rehash();
    FontName = Src.FontName;
    Effects = Src.Effects;
    Color = Src.Color;
}
NW_HOOK_CTOR_ARGS(0x1012A5C0, NMsnMessage, (const NMsnMessage& A), (A))

// 0x1012a510 - the headers emptied (the pairs through the out-of-line Empty), the table size back to 8; the strings and the map
// go in reverse
NMsnMessage::~NMsnMessage()
{
    NMsnHeaderEmpty(Headers.Pairs, 0);
    Headers.HashCount = 8;
    Headers.Rehash();
}
NW_HOOK_DTOR(0x1012A510, NMsnMessage)

// 0x1011a050 / 0x1011a0a0 / 0x1011a0f0 (ret 0xC) - the string given by value is stored; the argument is destroyed
void NMsnMessage::SetBody(FString Value)
{
    Body = Value;
}
void NMsnMessage::SetFontName(FString Value)
{
    FontName = Value;
}
void NMsnMessage::SetEffects(FString Value)
{
    Effects = Value;
}
typedef void (NMsnMessage::*SigSetString)(FString);
NW_HOOK_VARARGS(0x1011A050, NMsnMessage, SetBody, SigSetString)
NW_HOOK_VARARGS(0x1011A0A0, NMsnMessage, SetFontName, SigSetString)
NW_HOOK_VARARGS(0x1011A0F0, NMsnMessage, SetEffects, SigSetString)

// 0x101271d0 (ret 0x10) - the header block of a payload of that content type
__declspec(safebuffers) FString NMsnMessage::MakeHeader(FString ContentType)
{
    TCHAR Buffer[1040];
    appSprintf(Buffer, TEXT("MIME-Version: 1.0\r\nContent-Type: %s\r\n"), *ContentType);
    return FString(Buffer);
}
typedef FString (NMsnMessage::*SigMakeHeader)(FString);
NW_HOOK_VARARGS(0x101271D0, NMsnMessage, MakeHeader, SigMakeHeader)

// 0x10127110 (ret 4) - a text of the talk: plain UTF-8 text with the font, its effects and the color ("X-MMS-IM-Format")
FString NMsnMessage::BuildText()
{
    TCHAR Buffer[1040];
    appSprintf(Buffer,
               TEXT("MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=%s; EF=%s; CO=%d; CS=0; PF=22\r\n\r\n%s"),
               *FontName, *Effects, Color, *Body);
    return FString(Buffer);
}
NW_HOOK_METHOD(0x10127110, NMsnMessage, BuildText)

// 0x10127180 (ret 4) - the refusal of an invitation: its cookie and "Cancel-Code: REJECT"
__declspec(safebuffers) FString NMsnMessage::BuildCancel()
{
    TCHAR Buffer[1040];
    appSprintf(Buffer,
               TEXT("MIME-Version: 1.0\r\nContent-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\nInvitation-Command: CANCEL\r\nInvitation-Cookie: %s\r\nCancel-Code: REJECT\r\n\r\n"),
               *Cookie);
    return FString(Buffer);
}
NW_HOOK_METHOD(0x10127180, NMsnMessage, BuildCancel)

// 0x101278c0 (ret 4) - the typing notice: the control header, "TypingUser: <account>" and an empty line
FString NMsnMessage::BuildTyping()
{
    FString Result = MakeHeader(FString(TEXT("text/x-msmsgscontrol")));
    Result += TEXT("TypingUser: ");
    Result += Body;
    Result += TEXT("\r\n\r\n");
    return Result;
}
NW_HOOK_METHOD(0x101278C0, NMsnMessage, BuildTyping)

// 0x10127980 (ret 4) - the payload by kind: typing (1), cancel (3), else the text
FString NMsnMessage::Serialize()
{
    if (Kind == NMSN_Typing)
        return BuildTyping();
    if (Kind != NMSN_Unknown2 && Kind == NMSN_CancelInvite)
        return BuildCancel();
    return BuildText();
}
NW_HOOK_METHOD(0x10127980, NMsnMessage, Serialize)

/*-----------------------------------------------------------------------------
    MD5 (RFC 1321, the challenge of the login): the context's initialisation, update, final and the transform are ported. The stock
    transform (0x10126700, block in eax, context on the stack) and its two helpers (0x10126680 copies words out, 0x101266C0 reads
    the block's little-endian words; eax / ecx / edx arguments: regcheck refuses a hook on them) are only called by the stock
    Update / Final, which are hooked, so they are dead in hook mode (db/morto.csv).
-----------------------------------------------------------------------------*/

// 0x10126650 (cdecl) - the counts cleared and the four registers of the digest set to the algorithm's start values
void __cdecl NMd5Init(NMd5Context* Ctx)
{
    Ctx->BitCount[1] = 0;
    Ctx->BitCount[0] = 0;
    Ctx->State[0] = 0x67452301;
    Ctx->State[1] = 0xefcdab89;
    Ctx->State[2] = 0x98badcfe;
    Ctx->State[3] = 0x10325476;
}
NW_HOOK_FUNC(0x10126650, NMd5Init)

#define NMD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define NMD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define NMD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define NMD5_I(x, y, z) ((y) ^ ((x) | ~(z)))
#define NMD5_STEP(f, a, b, c, d, x, s, ac) \
    { (a) += f((b), (c), (d)) + (x) + (DWORD)(ac); (a) = ((a) << (s)) | ((a) >> (32 - (s))); (a) += (b); }

// 0x10126700 (stock: the 64 bytes in eax, the context pushed by the caller; here cdecl (context, block)) - RFC 1321's transform: the
// block read as 16 little-endian words (stock helper 0x101266C0), four rounds of 16 steps, the four results added to the state
static __declspec(noinline) void __cdecl NMd5Transform(NMd5Context* Ctx, const BYTE* Block)
{
    DWORD x[16];
    for (UINT i = 0; i < 16; i++)
        x[i] = (DWORD)Block[4 * i] | ((DWORD)Block[4 * i + 1] << 8) | ((DWORD)Block[4 * i + 2] << 16) | ((DWORD)Block[4 * i + 3] << 24);
    DWORD a = Ctx->State[0], b = Ctx->State[1], c = Ctx->State[2], d = Ctx->State[3];

    NMD5_STEP(NMD5_F, a, b, c, d, x[0], 7, 0xd76aa478);
    NMD5_STEP(NMD5_F, d, a, b, c, x[1], 12, 0xe8c7b756);
    NMD5_STEP(NMD5_F, c, d, a, b, x[2], 17, 0x242070db);
    NMD5_STEP(NMD5_F, b, c, d, a, x[3], 22, 0xc1bdceee);
    NMD5_STEP(NMD5_F, a, b, c, d, x[4], 7, 0xf57c0faf);
    NMD5_STEP(NMD5_F, d, a, b, c, x[5], 12, 0x4787c62a);
    NMD5_STEP(NMD5_F, c, d, a, b, x[6], 17, 0xa8304613);
    NMD5_STEP(NMD5_F, b, c, d, a, x[7], 22, 0xfd469501);
    NMD5_STEP(NMD5_F, a, b, c, d, x[8], 7, 0x698098d8);
    NMD5_STEP(NMD5_F, d, a, b, c, x[9], 12, 0x8b44f7af);
    NMD5_STEP(NMD5_F, c, d, a, b, x[10], 17, 0xffff5bb1);
    NMD5_STEP(NMD5_F, b, c, d, a, x[11], 22, 0x895cd7be);
    NMD5_STEP(NMD5_F, a, b, c, d, x[12], 7, 0x6b901122);
    NMD5_STEP(NMD5_F, d, a, b, c, x[13], 12, 0xfd987193);
    NMD5_STEP(NMD5_F, c, d, a, b, x[14], 17, 0xa679438e);
    NMD5_STEP(NMD5_F, b, c, d, a, x[15], 22, 0x49b40821);

    NMD5_STEP(NMD5_G, a, b, c, d, x[1], 5, 0xf61e2562);
    NMD5_STEP(NMD5_G, d, a, b, c, x[6], 9, 0xc040b340);
    NMD5_STEP(NMD5_G, c, d, a, b, x[11], 14, 0x265e5a51);
    NMD5_STEP(NMD5_G, b, c, d, a, x[0], 20, 0xe9b6c7aa);
    NMD5_STEP(NMD5_G, a, b, c, d, x[5], 5, 0xd62f105d);
    NMD5_STEP(NMD5_G, d, a, b, c, x[10], 9, 0x02441453);
    NMD5_STEP(NMD5_G, c, d, a, b, x[15], 14, 0xd8a1e681);
    NMD5_STEP(NMD5_G, b, c, d, a, x[4], 20, 0xe7d3fbc8);
    NMD5_STEP(NMD5_G, a, b, c, d, x[9], 5, 0x21e1cde6);
    NMD5_STEP(NMD5_G, d, a, b, c, x[14], 9, 0xc33707d6);
    NMD5_STEP(NMD5_G, c, d, a, b, x[3], 14, 0xf4d50d87);
    NMD5_STEP(NMD5_G, b, c, d, a, x[8], 20, 0x455a14ed);
    NMD5_STEP(NMD5_G, a, b, c, d, x[13], 5, 0xa9e3e905);
    NMD5_STEP(NMD5_G, d, a, b, c, x[2], 9, 0xfcefa3f8);
    NMD5_STEP(NMD5_G, c, d, a, b, x[7], 14, 0x676f02d9);
    NMD5_STEP(NMD5_G, b, c, d, a, x[12], 20, 0x8d2a4c8a);

    NMD5_STEP(NMD5_H, a, b, c, d, x[5], 4, 0xfffa3942);
    NMD5_STEP(NMD5_H, d, a, b, c, x[8], 11, 0x8771f681);
    NMD5_STEP(NMD5_H, c, d, a, b, x[11], 16, 0x6d9d6122);
    NMD5_STEP(NMD5_H, b, c, d, a, x[14], 23, 0xfde5380c);
    NMD5_STEP(NMD5_H, a, b, c, d, x[1], 4, 0xa4beea44);
    NMD5_STEP(NMD5_H, d, a, b, c, x[4], 11, 0x4bdecfa9);
    NMD5_STEP(NMD5_H, c, d, a, b, x[7], 16, 0xf6bb4b60);
    NMD5_STEP(NMD5_H, b, c, d, a, x[10], 23, 0xbebfbc70);
    NMD5_STEP(NMD5_H, a, b, c, d, x[13], 4, 0x289b7ec6);
    NMD5_STEP(NMD5_H, d, a, b, c, x[0], 11, 0xeaa127fa);
    NMD5_STEP(NMD5_H, c, d, a, b, x[3], 16, 0xd4ef3085);
    NMD5_STEP(NMD5_H, b, c, d, a, x[6], 23, 0x04881d05);
    NMD5_STEP(NMD5_H, a, b, c, d, x[9], 4, 0xd9d4d039);
    NMD5_STEP(NMD5_H, d, a, b, c, x[12], 11, 0xe6db99e5);
    NMD5_STEP(NMD5_H, c, d, a, b, x[15], 16, 0x1fa27cf8);
    NMD5_STEP(NMD5_H, b, c, d, a, x[2], 23, 0xc4ac5665);

    NMD5_STEP(NMD5_I, a, b, c, d, x[0], 6, 0xf4292244);
    NMD5_STEP(NMD5_I, d, a, b, c, x[7], 10, 0x432aff97);
    NMD5_STEP(NMD5_I, c, d, a, b, x[14], 15, 0xab9423a7);
    NMD5_STEP(NMD5_I, b, c, d, a, x[5], 21, 0xfc93a039);
    NMD5_STEP(NMD5_I, a, b, c, d, x[12], 6, 0x655b59c3);
    NMD5_STEP(NMD5_I, d, a, b, c, x[3], 10, 0x8f0ccc92);
    NMD5_STEP(NMD5_I, c, d, a, b, x[10], 15, 0xffeff47d);
    NMD5_STEP(NMD5_I, b, c, d, a, x[1], 21, 0x85845dd1);
    NMD5_STEP(NMD5_I, a, b, c, d, x[8], 6, 0x6fa87e4f);
    NMD5_STEP(NMD5_I, d, a, b, c, x[15], 10, 0xfe2ce6e0);
    NMD5_STEP(NMD5_I, c, d, a, b, x[6], 15, 0xa3014314);
    NMD5_STEP(NMD5_I, b, c, d, a, x[13], 21, 0x4e0811a1);
    NMD5_STEP(NMD5_I, a, b, c, d, x[4], 6, 0xf7537e82);
    NMD5_STEP(NMD5_I, d, a, b, c, x[11], 10, 0xbd3af235);
    NMD5_STEP(NMD5_I, c, d, a, b, x[2], 15, 0x2ad7d2bb);
    NMD5_STEP(NMD5_I, b, c, d, a, x[9], 21, 0xeb86d391);

    Ctx->State[0] += a;
    Ctx->State[1] += b;
    Ctx->State[2] += c;
    Ctx->State[3] += d;
}

// 0x10126fc0 (cdecl) - Length bytes added: the bit count raised, the block in the context filled and run through the transform each
// time it is full, whole blocks of the data run directly, the rest kept in the context
void __cdecl NMd5Update(NMd5Context* Ctx, const BYTE* Data, UINT Length)
{
    UINT Index = (Ctx->BitCount[0] >> 3) & 0x3f;
    UINT Bits = Length << 3;
    Ctx->BitCount[0] += Bits;
    if (Ctx->BitCount[0] < Bits)
        Ctx->BitCount[1]++;
    Ctx->BitCount[1] += Length >> 29;
    UINT PartLen = 64 - Index;
    UINT i;
    if (Length >= PartLen)
    {
        memcpy(&Ctx->Buffer[Index], Data, PartLen);
        NMd5Transform(Ctx, Ctx->Buffer);
        for (i = PartLen; i + 63 < Length; i += 64)
            NMd5Transform(Ctx, &Data[i]);
        Index = 0;
    }
    else
        i = 0;
    memcpy(&Ctx->Buffer[Index], &Data[i], Length - i);
}
NW_HOOK_FUNC(0x10126fc0, NMd5Update)

// 0x10127080 (cdecl (digest, context)) - the bit count saved, the data padded to 56 mod 64 (0x80 then zeros), the count added, the
// four state words written out as the digest and the context cleared
void __cdecl NMd5Final(DWORD* Digest, NMd5Context* Ctx)
{
    static const BYTE Padding[64] = {0x80};
    BYTE Bits[8];
    memcpy(Bits, Ctx->BitCount, 8);
    UINT Index = (Ctx->BitCount[0] >> 3) & 0x3f;
    NMd5Update(Ctx, Padding, Index < 56 ? 56 - Index : 120 - Index);
    NMd5Update(Ctx, Bits, 8);
    memcpy(Digest, Ctx->State, 16);
    memset(Ctx, 0, sizeof(NMd5Context));
}
NW_HOOK_FUNC(0x10127080, NMd5Final)

/*-----------------------------------------------------------------------------
    The text as the server gets it.
-----------------------------------------------------------------------------*/

// 0x101442a0 (cdecl) - the characters as UTF-8 (code page 65001, the terminator included) into a buffer of 0x1040 bytes
INT __cdecl NUtf8Encode(const WCHAR* Src, char* Dest)
{
    return WideCharToMultiByte(CP_UTF8, 0, Src, -1, Dest, 0x1040, NULL, NULL);
}
NW_HOOK_FUNC(0x101442A0, NUtf8Encode)
