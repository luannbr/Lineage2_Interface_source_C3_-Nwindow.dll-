/*=============================================================================
    NCoreHelpers.cpp - small anonymous stock functions that belong to no class file (wave 4, lane core): the
    helpers the console's messenger / broadcast / replay code calls and the records the GM tools build. Names ours.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NControl.h"
#include "nwindow/NCoreHelpers.h"

/*-----------------------------------------------------------------------------
    The messenger windows' small control (a picture, a text and a flag after NControl's fields; the class is the
    messenger lanes').
-----------------------------------------------------------------------------*/

// NCoreSkinControl (the title bar control of the messenger and the broadcast GM window): nwindow/NCoreHelpers.h

// 0x1001bdd0 - the picture found by its name (when given), the text kept (when given) and the flag stored
void NCoreSkinControl::SetSkin(const TCHAR* TexName, const TCHAR* Text, ANSICHAR Flag)
{
    if (TexName)
        nw::Field<UTexture*>(this, 0x1D4) = FindTexture(TexName);
    if (Text)
        nw::Field<FString>(this, 0x1E0) = Text;
    nw::Field<INT>(this, 0x1F4) = (BYTE)Flag;
}
NW_HOOK_METHOD(0x1001BDD0, NCoreSkinControl, SetSkin)

/*-----------------------------------------------------------------------------
    Util: the messenger's static helpers.
-----------------------------------------------------------------------------*/

// 0x10143c30 (cdecl, the result through the hidden pointer) - Util::GetLanguageID: the system's language as "0x%04x". The
// user's locale (the low word of GetUserDefaultLCID) stands for itself when it is one of those the messenger knows (a table
// of 0x401 .. 0x41B, 0x41D, 0x41F, 0x424, 0x804, 0x816 and 0xC0A); Chinese Hong Kong (0xC04) is the Taiwan one (0x404);
// any other is English (0x409)
static FString* __cdecl NUtilGetLanguageID(FString* Result)
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
NW_HOOK_FUNC(0x10143C30, NUtilGetLanguageID)

// 0x10143c00 (cdecl) - whether the character is a Latin letter (A-Z or a-z)
static __declspec(noinline) INT __cdecl NUtilIsLetter(TCHAR C)
{
    return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
}
NW_HOOK_FUNC(0x10143C00, NUtilIsLetter)

// 0x10144410 (cdecl; both strings by value, the function destroys them) - Util::MsnStrcmp: the localized texts compared
// character by character; a character that is no letter comes before a letter, otherwise the difference of the characters
// decides; when one text is a prefix of the other the longer is greater
INT __cdecl NUtilMsnStrcmp(FString A, FString B)      // (stage 5, sa-msn-ui: not static, MsnTree.cpp calls it; declared in MsnUiStock.h)
{
    FString X = A.Locs();
    FString Y = B.Locs();
    INT LenX = X.Len();
    INT LenY = Y.Len();
    INT Count = LenX > LenY ? LenY : LenX;
    for (INT i = 0; i < Count; i++)
    {
        INT LetterX = NUtilIsLetter(X.GetCharData()[i]);
        INT LetterY = NUtilIsLetter(Y.GetCharData()[i]);
        if (!LetterX)
        {
            if (LetterY)
                return -1;
        }
        else if (!LetterY)
            return 1;
        INT Difference = X.GetCharData()[i] - Y.GetCharData()[i];
        if (Difference)
            return Difference;
    }
    if (LenX > LenY)
        return 1;
    if (LenX < LenY)
        return -1;
    return 0;
}
NW_HOOK_FUNC(0x10144410, NUtilMsnStrcmp)

// 0x10144920 (cdecl; the names by value) - Util::MsnCompareFriend: a friend who is on (the number after the name is not 0)
// comes before one who is not; otherwise the names decide (MsnStrcmp)
INT __cdecl NUtilMsnCompareFriend(FString NameA, INT OnlineA, FString NameB, INT OnlineB)      // (stage 5, sa-msn-ui: not static, FriendList.cpp / MsnTree.cpp call it; MsnUiStock.h)
{
    if (OnlineA)
    {
        if (!OnlineB)
            return -1;
    }
    else if (OnlineB)
        return 1;
    return NUtilMsnStrcmp(NameA, NameB);
}
NW_HOOK_FUNC(0x10144920, NUtilMsnCompareFriend)

/*-----------------------------------------------------------------------------
    NSpawnTestParam.
-----------------------------------------------------------------------------*/

// The record the NPC viewer's spawn test builds on its stack (NCNpcView_CharacterSelectWnd::OnSpawnTest, a GM tool):
// 0x30 bytes, the fields at 0 except the scale, the id and the -1. The only user is the stock OnSpawnTest.
struct NSpawnTestParam
{
    INT Unknown00;                   // +0x00 (the second argument)
    INT Zero[8];                     // +0x04 .. +0x20 (0)
    FLOAT Scale;                     // +0x24 (the first argument; the caller passes 1.0)
    INT Unknown28;                   // +0x28 (-1)
    INT Unknown2C;                   // +0x2C (0)

    NSpawnTestParam(FLOAT InScale, INT InUnknown00);
};
NW_ASSERT_SIZE(NSpawnTestParam, 0x30);

// 0x100b2590 (ret 8)
NSpawnTestParam::NSpawnTestParam(FLOAT InScale, INT InUnknown00)
{
    Unknown00 = InUnknown00;
    Zero[0] = 0;
    Zero[1] = 0;
    Zero[2] = 0;
    Zero[3] = 0;
    Zero[4] = 0;
    Zero[5] = 0;
    Zero[6] = 0;
    Zero[7] = 0;
    Scale = InScale;
    Unknown28 = -1;
    Unknown2C = 0;
}
NW_HOOK_CTOR_ARGS(0x100b2590, NSpawnTestParam, (FLOAT A, INT B), (A, B))
