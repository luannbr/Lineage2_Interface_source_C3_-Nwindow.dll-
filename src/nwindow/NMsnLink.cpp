/*=============================================================================
    NMsnLink.cpp - the requests the messenger dialogs hand to the protocol object (C3 only). Stock: 0x1012A7C0 ..
    0x1012AA7A (six small functions; 0x1012A830 and the ones after it belong to other callers). Each one passes its
    argument on to the session (the notification connection, NMsnSession.cpp), when there is one (the session is at +0x0C); the
    by-value texts and the contact record are copied into the call and destroyed with the parameter. Also the copies of the folded
    getter of the data store and of the combo box's item-data functions the dialogs call (stage 5: they were stock addresses).
=============================================================================*/
#include "nwindow/NMsnLink.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NString.h"

// 0x1012b140 (4 bytes: mov eax, [ecx + 0x50]) - the data store of the protocol object. The getter is a function of its own in the stock
// (the linker folded it with other classes' getters: hooked in NCWnd.cpp as NCWnd::GetPosFlags, the same code); this is the copy the
// dialogs call
NMsnStore* NMsnLink::GetStore()
{
    return Store;
}

// 0x100054d0 (ret 4) - the data word the item was added with, -1 for an index outside the list. Hooked in NTemplates.cpp as
// NComboItemFns::GetItemData (the same code); this is the copy the dialogs call
__declspec(noinline) INT MsnComboItemData(NCComboBox* Combo, INT Index)
{
    if (Index < 0 || Index >= Combo->Items.Num())
        return -1;
    return Combo->Items(Index)->Color;
}

// 0x10005a00 (ret 8) - an item of Text (nothing when it is NULL or empty) whose data word is Data. Hooked in NTemplates.cpp as
// NComboItemFns::AddItemData (the same code); this is the copy the dialogs call
__declspec(noinline) void MsnComboAddItemData(NCComboBox* Combo, const TCHAR* Text, INT Data)
{
    if (Text && *Text)
    {
        NString* Item = new NString(Text);
        Item->Color = Data;
        Combo->Items.AddItem(Item);
    }
}

// 0x1012a7c0 (ret 0x10)
void NMsnLink::RequestAddFriend(FString Mail, INT bAllow)
{
    if (Session)
        Session->AddFriend(Mail, bAllow);
}
NW_HOOK_METHOD(0x1012a7c0, NMsnLink, RequestAddFriend)

// 0x1012a8a0 (ret 0x10)
void NMsnLink::RequestRemoveFriend(FString Mail, INT bBlock)
{
    if (Session)
        Session->RemoveFriend(Mail, bBlock);
}
NW_HOOK_METHOD(0x1012a8a0, NMsnLink, RequestRemoveFriend)

// 0x1012a910 (ret 0x4c)
void NMsnLink::RequestMoveFriend(NMsnBuddy Buddy, INT FromGroup, INT ToGroup)
{
    if (Session)
        Session->MoveFriend(Buddy, FromGroup, ToGroup);
}
NW_HOOK_METHOD(0x1012a910, NMsnLink, RequestMoveFriend)

// 0x1012a990 (ret 0xc)
void NMsnLink::RequestAddGroup(FString Name)
{
    if (Session)
        Session->AddGroup(Name);
}
NW_HOOK_METHOD(0x1012a990, NMsnLink, RequestAddGroup)

// 0x1012aa00 (ret 4) - a tail jump into the session's function
void NMsnLink::RequestRemoveGroup(INT GroupId)
{
    if (Session)
        Session->RemoveGroup(GroupId);
}
NW_HOOK_METHOD(0x1012aa00, NMsnLink, RequestRemoveGroup)

// 0x1012aa10 (ret 0x10)
void NMsnLink::RequestRenameGroup(INT GroupId, FString Name)
{
    if (Session)
        Session->RenameGroup(GroupId, Name);
}
NW_HOOK_METHOD(0x1012aa10, NMsnLink, RequestRenameGroup)
