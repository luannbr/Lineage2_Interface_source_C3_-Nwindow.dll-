#pragma once
// NCSelectDeliverWnd - "Send package", first step (UClass "CSelectDeliverWnd", 0x8CC): a
// combo box with the characters a package can go to (8 at most, the console adds them), OK
// asks the server for what can be sent to the one chosen, Cancel closes.
//
// NCDeliverWnd - the package window (UClass "CDeliverWnd", 0xC0): the warehouse's layout
// (NCStoreWnd.h) with one mode: the inventory on top, what is sent below, 1000 adena a slot.
// OK sends the list to the character chosen (TargetID). The console opens it with the
// target and the player's adena and fills the upper grid.
//
// NCDeliverItemWnd - its grids (UClass "CDeliverItemWnd", 0x11C): the warehouse's grid
// without the mode and side fields; the tool tips are the inventory's with 128-character
// buffers (NItemToolTip.h; the box's text buffer has 1024, see DrawItemToolTip).
//
// Desktop (NCConsole) +0x138 is NCDeliverWnd, +0x13C NCSelectDeliverWnd.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCComboBox;

// NWM_COMMAND ids of NCSelectDeliverWnd (names ours; NCDeliverWnd uses NCStoreWnd's)
enum
{
    NID_SELDELIVER_FRAME = 0xE432,
    NID_SELDELIVER_TARGETS = 0xE433,  // the combo box: lParam the entry chosen
    NID_SELDELIVER_OK = 0xE434,
    NID_SELDELIVER_CANCEL = 0xE435,
};

class NCDeliverItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCDeliverItemWnd, NCItemWnd, 0, 0x1027CA00)

    NCDeliverItemWnd* Target;        // +0x148 where its items go (the other grid)   // C2: offset derived, no evidence

    NCDeliverItemWnd();                                              // 0x1007b6c0
    NCDeliverItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x1007b760
    // ~NCDeliverItemWnd                                             // 0x1007b710

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a2850 (shared)
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1007b330
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x1007b500 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 76 0x1007b670

    // names ours
    void SetTarget(NCDeliverItemWnd* InTarget);                      // 0x10088870 (shared: NCTradeItemWnd::SetKind hooks it)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1007aa40 0 when full (the console lane's)
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x1007ab50 0 when full (stock)
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x1007ac30 to Target
};
NW_ASSERT_OFFSET(NCDeliverItemWnd, Target, 0x148);
NW_ASSERT_SIZE(NCDeliverItemWnd, 0x14C);

class NCDeliverWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCDeliverWnd, NCWnd, 0, 0x1027CF28)

    NCDeliverItemWnd* InventoryItems; // +0xC8 on top
    NCDeliverItemWnd* SendItems;     // +0xCC below: what is sent
    UTexture* BackTex;               // +0xD0 "L2UI.StoreWnd.StoreBack"
    NCButton* UpButton;              // +0xD4
    NCButton* DownButton;            // +0xD8
    NCButton* OKButton;              // +0xDC
    NCButton* CancelButton;          // +0xE0
    NCWnd* UnknownB0;                // +0xE4 OnDestroy's, never created
    INT Adena;                       // +0xE8 the player's
    INT Fee;                         // +0xEC UpdateFee
    INT TargetID;                    // +0xF0 (-1) the character the package goes to

    NCDeliverWnd();                                                  // 0x1007b7a0
    // ~NCDeliverWnd                                                 // 0x1007b830

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100f0070 (shared)
    virtual INT OnCreate();                                          // 37 0x1007b880
    virtual INT OnDestroy();                                         // 38 0x1007a630
    virtual void ShowWindow();                                       // 44 0x10079fe0
    virtual void HideWindow();                                       // 45 0x10079ff0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007a2a0
    // names ours
    virtual INT OnOK();                                              // 68 0x1007a780
    virtual INT OnCancel();                                          // 69 0x1007a000
    virtual void NewVirtual70();  // 70 0x1007ad90 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual71();  // 71 0x1007af00 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual72();  // 72 0x1007a050 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnUp();                                              // 0x1007ad90 (slot 70: its placeholder jumps here)
    INT OnDown();                                            // 0x1007af00 (slot 71)
    INT OnClose();                                           // 0x1007a050 (slot 72)

    // names ours (the console calls most of these)
    void SetAdena(INT InAdena);                                      // 0x10079f90
    void SetTargetID(INT InTargetID);                                // 0x10079f90
    void Clear();                                                    // 0x10079fa0 grids emptied,
                                                                     //   target -1, adena and fee 0
    __declspec(noinline) void UpdateFee();                                                // 0x1007a020
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x1007ad80 (the console lane's)
};
NW_ASSERT_OFFSET(NCDeliverWnd, BackTex, 0xD0);
NW_ASSERT_OFFSET(NCDeliverWnd, TargetID, 0xF0);
NW_ASSERT_SIZE(NCDeliverWnd, 0xF4);

// one character a package can go to (C2: the name holds 24 characters, C1: 128)
struct NDeliverTarget
{
    INT ID;                          // +0x00 sent back with the request
    TCHAR Name[24];                  // +0x04 the combo box's line (AddTarget copies without a length check)
};
NW_ASSERT_SIZE(NDeliverTarget, 0x34);

class NCSelectDeliverWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSelectDeliverWnd, NCWnd, 0, 0x1027C4D8)

    UTexture* BackTex;               // +0xC8 "L2UI.etcwnd.NPCHtml_BACK"   // C2: offset derived, no evidence
    NCComboBox* TargetCombo;         // +0xCC
    NCButton* OKButton;              // +0xD0
    NCButton* CancelButton;          // +0xD4
    INT Selected;                    // +0xD8 (-1) entry chosen in the combo box
    NDeliverTarget Targets[8];       // +0xDC   // C2: offset derived, no evidence
    INT NumTargets;                  // +0x27C

    NCSelectDeliverWnd();                                            // 0x1007bd50
    // ~NCSelectDeliverWnd                                           // 0x1007bdb0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007a080
    virtual INT OnCreate();                                          // 37 0x1007b070
    virtual INT OnDestroy();                                         // 38 0x1007a9a0
    virtual void ShowWindow();                                       // 44 0x1007a1c0
    virtual void HideWindow();                                       // 45 0x1007a1d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007a8d0
    // names ours
    virtual INT OnSelect(LPARAM Index);                              // 68 0x1007a280
    virtual INT OnOK();                                              // 69 0x1007a1e0
    virtual INT OnCancel();                                          // 70 0x1007bee0 (folded)

    // names ours (the console)
    void AddTarget(INT ID, const TCHAR* Name);                       // 0x1007a120 8 at most
    void Clear();                                                    // 0x1007a190
};
NW_ASSERT_OFFSET(NCSelectDeliverWnd, Selected, 0xD8);
// NW_ASSERT_OFFSET(NCSelectDeliverWnd, Targets, 0xA8);   // C2: field not mapped
NW_ASSERT_OFFSET(NCSelectDeliverWnd, NumTargets, 0x27C);
NW_ASSERT_SIZE(NCSelectDeliverWnd, 0x280);

#pragma pack(pop)