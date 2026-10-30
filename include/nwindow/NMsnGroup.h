#pragma once
// NMsnGroup - a contact group of the messenger (C3 only; 0x1C bytes; the protocol object's data store keeps them in the array at
// Msn->Store + 0x10; name ours, there is no UClass). Stage 5 (sa-msn-proto): the ONE definition. The dialogs' lane (NMsnLink.h,
// NMsnDlgCommon.h: GetName / GetId) and the protocol lane (NMsnProtocol.h, FriendList.h: the constructors, SetName and the fields) had
// each declared a struct of this name, with other member names for the same three fields (the dialogs' RawName / Name / Id are Name /
// EncodedName / GroupID here), so that the two families of headers could not be in one file. This header needs only NWindow.h;
// NMsnProtocol.h includes it.
//
// The name arrives URL-encoded (Name). SetName (the protocol lane) keeps it and also keeps in EncodedName the same text with its
// escapes replaced; GetName (the dialogs' lane) URL-decodes Name into EncodedName and hands that string out.

#include "nwindow/NWindow.h"
#pragma pack(push, 4)

struct NMsnGroup
{
    FString Name;            // +0x00 the name as the server sent it (URL-encoded)
    FString EncodedName;     // +0x0C (name ours: GetName leaves the DECODED name here) SetName: the name with its escapes replaced
    INT GroupID;             // +0x18 the group number the protocol commands use

    NMsnGroup(const FString& InName, INT InGroupID);                 // 0x1012da80 (ret 0x10, the string by value)
    NMsnGroup(const NMsnGroup& Other);                               // 0x1012db00 (ret 4)
    ~NMsnGroup();                                                    // 0x1012db80
    void SetName(FString NewName);                                   // 0x1012dbd0 (ret 0xC)
    FString* GetName();                                              // 0x1012DC80 (name ours) the URL-decoded name
    INT GetId();                                                     // 0x1012DCD0 (name ours)
};
NW_ASSERT_OFFSET(NMsnGroup, EncodedName, 0x0C);
NW_ASSERT_OFFSET(NMsnGroup, GroupID, 0x18);
NW_ASSERT_SIZE(NMsnGroup, 0x1C);

#pragma pack(pop)
