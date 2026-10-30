#pragma once
// NCActionItemWnd - a grid of action icons (UClass "CActionItemWnd", 0x154): one of the three
// on the Basic page of the action window (basic, party, social). A click runs the action
// (UseAction: requests to the server, social motions...); an icon dragged onto the shortcut
// bar is registered there. The selected icon's tool tip shows its name and description.

#include "nwindow/NCWnd.h"
#include "nwindow/NCItemWnd.h"   // NItemEntry
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

struct NItemInfo;

// actionname-*.dat ids the actions use (names from the English table; the shortcut bar
// uses them too)
enum
{
    ACTION_SitStand, ACTION_WalkRun, ACTION_Attack, ACTION_Exchange, ACTION_NextTarget,
    ACTION_PickUp, ACTION_Assist, ACTION_Invite, ACTION_LeaveParty, ACTION_Dismiss,
    ACTION_PrivateStoreSell, ACTION_PartyMatching, ACTION_Greeting, ACTION_Victory,
    ACTION_Advance, ACTION_PetStay, ACTION_PetAttack, ACTION_PetStop, ACTION_PetPickUp,
    ACTION_PetReturn, ACTION_PetSkill, ACTION_SummonStay, ACTION_SummonAttack,
    ACTION_SummonStop, ACTION_SocialYes, ACTION_SocialNo, ACTION_SocialBow, ACTION_SummonSkill,
    ACTION_PrivateStoreBuy, ACTION_SocialUnaware, ACTION_SocialWaiting, ACTION_SocialLaugh,
};

// An icon of the grid (name ours; 0x1A30 bytes in C2, created by AddItem): an NItemEntry (the name, the icon, the
// description and the tool tip object are its own) with the action's id and parameter after it. The unused fields stay
// as the default constructor leaves them.
struct NActionItem : public NItemEntry
{
    INT ActionID;              // +0x1A28 (-1) FL2ActionData::ActionID (NItemInfo +0x1718)
    INT Param;                 // +0x1A2C (-1) the social action of the social ones (NItemInfo +0x170C)

    __declspec(noinline) NActionItem(const NItemInfo& Info);         // 0x10043430
    __declspec(noinline) INT GetActionID();                          // 0x10040190
};
NW_ASSERT_OFFSET(NActionItem, ActionID, 0x1A34);
NW_ASSERT_SIZE(NActionItem, 0x1A3C);

class NCActionItemWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionItemWnd, NCWnd, 0, 0x10278B10)

    TArray<NActionItem*> Items;            // +0xC8 (C3: the constructor clears the three words, ~TActionItemArray 0x1004a440)
    BYTE _UnknownD4[0x10C - 0xD4];         // +0xD4 (not touched by the C3 code)
    INT Unknown10C;                        // +0x10C (0) cleared by the constructor, nothing reads it
    INT Unknown110;                        // +0x110 (0) cleared by the constructor, nothing reads it
    BYTE _Unknown114[0x120 - 0x114];       // +0x114
    UTexture* PressedTex;                  // +0x120 "l2ui_ch3.InventoryWnd.Inventory_OutLine_down" over the pressed icon
    UTexture* FrameTex;                    // +0x124 "l2ui_ch3.InventoryWnd.Inventory_OutLine" frame of each icon
    BYTE _Unknown128[0x12C - 0x128];       // +0x128
    UBOOL bPressed;                        // +0x12C mouse button down on PressedIndex
    UBOOL bDragging;                       // +0x130 an icon is being dragged
    INT Cols;                              // +0x134
    INT Rows;                              // +0x138
    INT MaxItems;                          // +0x13C
    INT IconSize;                          // +0x140 (32)
    INT PressedIndex;                      // +0x144 (-1)
    INT HoverIndex;                        // +0x148 (-1) icon under the mouse
    BYTE _Unknown14C[0x18C - 0x14C];       // +0x14C

    NCActionItemWnd(INT InCols = 5, INT InRows = 2, INT InMaxItems = 15, INT InIconSize = 32);  // 0x10043500 (C1 only: no C2 match)
    // ~NCActionItemWnd                                              // 0x10043600 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10046c20
    virtual INT OnDestroy();                                         // 38 0x1004b0e0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10047640
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x1004aa10
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10047810
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10049240
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100492c0
    // names ours
    virtual void UseAction();                                        // 68 0x10047960 the pressed icon's action

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x10046c30 icon, -1 = none
    void ClearSelection();                                           // 0x10047990
    __declspec(noinline) INT AddItem(NItemInfo* Info);               // 0x1004a5a0 0 when full
    void ClearItems();                                               // 0x10102be0 (shared with other grids)
};
NW_ASSERT_OFFSET(NCActionItemWnd, Items, 0xC8);
NW_ASSERT_OFFSET(NCActionItemWnd, PressedTex, 0x120);
NW_ASSERT_OFFSET(NCActionItemWnd, FrameTex, 0x124);
NW_ASSERT_OFFSET(NCActionItemWnd, bPressed, 0x12C);
NW_ASSERT_OFFSET(NCActionItemWnd, IconSize, 0x140);
NW_ASSERT_OFFSET(NCActionItemWnd, HoverIndex, 0x148);
NW_ASSERT_SIZE(NCActionItemWnd, 0x18C);

#pragma pack(pop)