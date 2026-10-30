#pragma once
// What the nine messenger dialogs share (C3 only; names ours): the owners they talk to, the texts and the two bits of
// code the stock OnCreate / OnDestroy repeat inline for each of their controls.
//
// The dialogs (OfflineConfirmDialog, AddFriendDialog, AddGroupDialog, AddedConfirmDialog, MoveGroupDialog,
// RemoveFriendDialog, RemoveGroupDialog, RenameGroupDialog, MessageDialog) are made by MsnWnd::OnCreate (0x10139A40) and
// derive from NCDialogBox (0x1D4 bytes); each has a layout of its own (the stock constructors differ), so there is no
// base class between them.

#include "nwindow/NCDialogBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCObject.h"
#include "nwindow/NGameData.h"
#include "nwindow/NMsnGroup.h"
#include "nwindow/NMsnLink.h"
#pragma pack(push, 4)

class MsnWnd;   // the messenger window (0x1B0; the msn2 lane's); the protocol object (stock class Msn) is NMsnLink

// the child style of the combo boxes (NWS_VISIBLE and the bit 0x10, name unknown)
enum { NWS_DLG_COMBO = 0x12 };

// A line of systemmsg-*.dat (GL2GameData + 0x3F638 + Id * 0x30 + 4): the dialogs' texts are the lines 1314 .. 1351
FORCEINLINE const TCHAR* MsnDlgText(INT Id)
{
    return *GL2GameData.SystemMsg[Id].Message;
}

// The push button of the dialogs (repeated inline in each stock OnCreate): the button of the new skin (76 x 23) at
// (X, Y) with its command id, created in the dialog, then its text (a SysString: 140 "OK", 141 "Cancel")
static FORCEINLINE NCButton* NewDlgButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 76, 23);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetText(*GL2GameData.SysString[TextID]);
    return Button;
}

// A control goes (repeated inline in each stock OnDestroy): out of the dialog's list, deleted, forgotten
template <class T> static FORCEINLINE void DestroyDlgChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// The combo box of a group dialog is filled from the protocol object's groups (repeated inline in the stock ShowWindow of
// the three group dialogs): the data store (0x1012B140, the getter folded with NCWnd::GetPosFlags) gives the array of
// groups (0x1012C6C0), a copy of it is made (0x10121B20), then for every group in the copy the decoded name is added with
// the group number as the item's data word (NComboItemFns::AddItemData, 0x10005A00, ret 8), and the copy is destroyed
// (0x10123570). Stage 5: the getter and the item functions are NMsnLink.cpp's copies of the stock ones (the hooks are on NCWnd's and
// NTemplates.cpp's functions of the same code)
INT MsnComboItemData(NCComboBox* Combo, INT Index);                     // 0x100054D0 (ret 4)
void MsnComboAddItemData(NCComboBox* Combo, const TCHAR* Text, INT Data);   // 0x10005A00 (ret 8)
static FORCEINLINE void FillGroupCombo(NCComboBox* Combo, NMsnLink* Msn)
{
    NMsnStore* Store = Msn->GetStore();
    TArray<NMsnGroup*> Groups(*Store->GetGroups());
    for (INT i = 0; i < Groups.Num(); i++)
    {
        NMsnGroup* Group = Groups(i);
        if (Group)
            MsnComboAddItemData(Combo, **Group->GetName(), Group->GetId());
    }
}

// The data word of the item picked in the combo box (NComboItemFns::GetItemData, 0x100054D0, ret 4)
static FORCEINLINE INT GetComboItemData(NCComboBox* Combo, INT Index)
{
    return MsnComboItemData(Combo, Index);
}

#pragma pack(pop)
