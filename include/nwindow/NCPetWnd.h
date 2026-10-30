#pragma once
// NCPetWnd - the pet window (UClass "CPetWnd", 0x184; opened from the pet's status bar or the
// action window): name and level, HP / MP / food / exp / weight bars, the Name button (a pet
// can be named once) and three pages under tabs: Information (NCPetDetailStatusWnd, the combat
// values), Action (NCPetActionWnd, the pet's actions) and Items (NCPetInventoryWnd).
// Everything is read again from the network handler at each paint.
//
// Desktop (NCConsole) +0x104 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCActionItemWnd;
class NCButton;
class NCPetInventoryWnd;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids (names ours; the summon window uses the same tab id)
enum
{
    NID_PET_TABS = 0xFE001B,
    NID_PET_FRAME = 0xFE001C,
    NID_PET_NAME = 0xFE001D,          // the Name button
};

// The Information page (UClass "CPetDetailStatusWnd", 0x14C): P.Atk. ... Casting Spd.
class NCPetDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetDetailStatusWnd, NCWnd, 0, 0x1029BE00)

    UTexture* BackTex;               // +0xC8 "L2UI.PetWnd.Pet02"
    PetInfo Pet;                     // +0xCC (0xAC in C2) the handler's record, read again by each paint   // C2: offset derived, no evidence

    NCPetDetailStatusWnd();                                          // 0x100cf250
    // ~NCPetDetailStatusWnd                                         // 0x100b2d70 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cb9e0
    virtual INT OnDestroy();                                         // 38 0x100cba10 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50)
};
NW_ASSERT_OFFSET(NCPetDetailStatusWnd, Pet, 0xCC);
NW_ASSERT_SIZE(NCPetDetailStatusWnd, 0x178);

// The Action page (UClass "CPetActionWnd", 0x9C): the pet actions of actionname-*.dat
// (category 4) in an icon grid.
class NCPetActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetActionWnd, NCWnd, 0, 0x1029C320)

    UTexture* BackTex;               // +0xC8 "L2UI.PetWnd.pet03"   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    NCActionItemWnd* Items;          // +0xCC

    NCPetActionWnd();                                                // 0x100cf310
    // ~NCPetActionWnd                                               // 0x100b2e10 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cea40
    virtual INT OnDestroy();                                         // 38 0x100cc540
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50)

    void LoadActions();                                              // 0x100cc5f0 (name ours; an anonymous sub_ in the db)
};
NW_ASSERT_OFFSET(NCPetActionWnd, Items, 0xCC);
NW_ASSERT_SIZE(NCPetActionWnd, 0xD0);

class NCPetWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetWnd, NCWnd, 0, 0x1029B3B0)

    UTexture* BackTex;               // +0xC8 (C3) "L2UI_CH3.PetWnd.Petinterface_back1"
    UTexture* BackTex2;              // +0xCC (C3) "L2UI_CH3.PetWnd.Petinterface_back2"
    UTexture* HPTex;                 // +0xD0 "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0xD4 "L2UI.StatusWnd.statusMP1"
    UTexture* EXPTex;                // +0xD8 "L2UI.StatusWnd.statusEXP1"
    UTexture* WeightTex[4];          // +0xDC "L2UI.StatusWnd.statusWt1" .. 4 (as the status
                                     //   window: up to 50%, 66.66%, 80%, above)
    UTexture* FedTex;                // +0xEC "L2UI.PetWnd.statusEat1"
    PetInfo Pet;                     // +0xF0 (0xAC in C2) the handler's record, read again by each paint   // C2: offset derived, no evidence
    NCPetDetailStatusWnd* DetailWnd; // +0x19C
    NCPetActionWnd* ActionWnd;       // +0x1A0
    NCPetInventoryWnd* InventoryWnd; // +0x1A4
    NCTabControl* TabCtrl;           // +0x1A8
    NCButton* NameButton;            // +0x1AC
    UBOOL bCanName;                  // +0x1B0 (1) the console clears it (SetCanName)

    NCPetWnd();                                                      // 0x100cf620
    // ~NCPetWnd                                                     // 0x100b3870 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100cbb40
    virtual INT OnCreate();                                          // 37 0x100cf700
    virtual INT OnDestroy();                                         // 38 0x100ccdd0
    virtual void ShowWindow();                                       // 44 0x100cd7b0
    virtual void Minimize();                                         // 49 0x100cbba0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ccf20
    virtual INT OnName();                                            // 68 0x100cd860 (name ours)

    // names ours (the console)
    void SetCanName(UBOOL bCan);                                     // 0x100cbc00
    void ClearInventory();                                           // 0x100cbc20
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x100cf000 (C1 0x100940b0)
    UBOOL UpdateInventoryItem(INT Op, NItemInfo* Info);              // 0x100cf230 (C1 0x100940d0)
};
NW_ASSERT_OFFSET(NCPetWnd, WeightTex, 0xDC);
NW_ASSERT_OFFSET(NCPetWnd, Pet, 0xF0);
NW_ASSERT_OFFSET(NCPetWnd, DetailWnd, 0x19C);
NW_ASSERT_OFFSET(NCPetWnd, bCanName, 0x1B0);
NW_ASSERT_SIZE(NCPetWnd, 0x1B4);

#pragma pack(pop)