#pragma once
// MsnTree - the messenger's person tree (UClass "CMsnTree", 0x210; C3 only, no C2 source): an NCTree whose entries are the
// groups (kind 2, a plus / minus picture and the name) and, under them, the persons (kind 3, the presence picture, the
// name, the account in Text3 and the presence code in Tag). The groups and the persons are kept sorted. The tree control of
// the messenger window (MsnTreeCtrl) makes it and gives it the entries.
//
// Layout and virtuals from the stock constructor 0x10130810, the insertion helpers 0x10130380 .. 0x10130740, OnPostPaint
// 0x101308B0 and DrawItem 0x1012FB00.

#include "nwindow/NCTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnTree : public NCTree
{
    NW_DECLARE_CLASS(MsnTree, NCTree, 0, 0x102C2CE0)

    BYTE _Unknown1FC[0x204 - 0x1FC]; // +0x1FC never touched by the constructor or the functions below
    void* ListedPersons;             // +0x204 a person list (lookup 0x1012CB40); DrawItem draws a person found in it with "msnicon5" (most
                                     //   likely the block list; nothing here writes it) (name ours)
    BYTE _Unknown208[0x210 - 0x208]; // +0x208

    MsnTree();                                                       // 0x10130810
    // ~MsnTree                                                      // 0x10130860

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101421B0 (shared with L2FriendTree)
    virtual INT OnCreate();                                          // 37 0x10141B50 (NControl's, then 1; shared)
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x101308B0 (the db says OnPostPaint)
    virtual INT DrawItem(NCGDevice* Device, NCTVItem* Item, INT X, INT Y);   // 71 0x1012FB00

    // names ours (the db: GetGroupInsertIndex .. InsertFriendItem)
    INT GetGroupInsertIndex(NTVItemData* Data, NCTVItem* Parent);    // 0x10130380 (ret 8) where a group of this text goes among the root's
    INT GetFriendInsertIndex(NTVItemData* Data, NCTVItem* Parent);   // 0x101304C0 (ret 8) the same for a person under a group
    void InsertGroupItem(NTVItemData* Data);                         // 0x10130670 (ret 4) a group under the root
    void InsertFriendItem(NTVItemData* Data, NCTVItem* Parent);      // 0x10130740 (ret 8) a person under a group
};
NW_ASSERT_SIZE(MsnTree, 0x210);

#pragma pack(pop)
