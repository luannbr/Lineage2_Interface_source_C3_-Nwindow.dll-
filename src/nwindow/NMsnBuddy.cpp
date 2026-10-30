/*=============================================================================
    NMsnBuddy.cpp - the 0x44-byte contact record of the messenger, the dialogs' view of NMsnFriend (C3 only; the header says what
    is known). Its constructor, copy constructor and destructor are functions of their own in the stock (0x1011A140, 0x1012C460,
    0x1012C530: hooked as NChatUser::NChatUser and NMsnFriend's in NChatTypes.cpp / NMsnProtocol.cpp); the dialogs' record has the
    same three, out of line, not hooked (one address, one hook). Stage 5: they were calls of the stock addresses, and the decoded name
    a jump into the stock image.
=============================================================================*/
#include "nwindow/NMsnBuddy.h"

// 0x1011a140 - the five strings empty (the two numbers are left as they were)
NMsnBuddy::NMsnBuddy()
{
}

// 0x1012c460 (ret 4) - the second number, then the status (+0x14), +8, the account, the name and the decoded name copied; the first
// number is not touched
NMsnBuddy::NMsnBuddy(const NMsnBuddy& Other)
{
    Word04 = Other.Word04;
    Str14 = Other.Str14;
    Str08 = Other.Str08;
    Str20 = Other.Str20;
    Str2C = Other.Str2C;
    Str38 = Other.Str38;
}

// 0x1012c530 - the five strings in reverse
NMsnBuddy::~NMsnBuddy()
{
}

// 0x1012c610 (ret 4) - the shown name (+0x2C) URL-decoded into a buffer of 260 characters (cleared first) and kept in +0x38, and a copy of
// it built in Out (not constructed before), which comes back. (The same function as NMsnFriend::GetDisplayName, which the hook is on)
FString* NMsnBuddy::GetDecodedName(FString* Out)
{
    TCHAR Buffer[0x104];
    appMemset(Buffer, 0, 0x208);
    DecodeMsnUrl(*Str2C, Buffer);
    Str38 = Buffer;
    new ((void*)Out) FString(Str38);
    return Out;
}
