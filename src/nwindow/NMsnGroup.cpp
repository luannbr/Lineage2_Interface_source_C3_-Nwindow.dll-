/*=============================================================================
    NMsnGroup.cpp - a contact group of the messenger (C3 only). Stock: GetName 0x1012DC80, GetId 0x1012DCD0, and the
    URL decoder 0x10144300 they (and the contact record's GetDecodedName) share.
=============================================================================*/
#include <windows.h>
#include <wininet.h>
#include "nwindow/NMsnGroup.h"
#include "nwindow/NMsnLink.h"

// wininet is an import of the stock image; the hook DLL does not link it otherwise
#pragma comment(lib, "wininet.lib")

// 0x10144300 (cdecl, name ours) - Url decoded into Out (a buffer of 260 characters) with the escapes and nothing else
// touched (ICU_DECODE | ICU_NO_ENCODE); what the import answers comes back
__declspec(noinline) BOOL __cdecl DecodeMsnUrl(const TCHAR* Url, TCHAR* Out)
{
    DWORD Length = 0x104;
    return InternetCanonicalizeUrlW(Url, Out, &Length, ICU_DECODE | ICU_NO_ENCODE);
}
NW_HOOK_FUNC(0x10144300, DecodeMsnUrl)

// 0x1012dc80 (name ours) - the raw name (Name) decoded into a buffer of 260 characters (cleared first) and kept in the second string,
// which comes back
FString* NMsnGroup::GetName()
{
    TCHAR Decoded[260];
    appMemset(Decoded, 0, 0x208);
    DecodeMsnUrl(*Name, Decoded);
    EncodedName = Decoded;
    return &EncodedName;
}
NW_HOOK_METHOD(0x1012dc80, NMsnGroup, GetName)

// 0x1012dcd0 (name ours)
INT NMsnGroup::GetId()
{
    return GroupID;
}
NW_HOOK_METHOD(0x1012dcd0, NMsnGroup, GetId)

// 0x1012c6c0 (name ours) - the array of groups (a getter of the data store, folded with the same two instructions of other
// classes)
TArray<NMsnGroup*>* NMsnStore::GetGroups()
{
    return Groups;
}
NW_HOOK_METHOD(0x1012c6c0, NMsnStore, GetGroups)
