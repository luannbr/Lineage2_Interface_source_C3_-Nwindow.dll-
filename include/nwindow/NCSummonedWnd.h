#pragma once
// NCSummonedWnd - the summon window (UClass "CSummonedWnd", 0x160): the pet window without
// food, exp, weight, name button and items: name and level, HP / MP bars, and the pages
// Information (NCSummonedDetailStatusWnd) and Action (NCSummonedActionWnd, the summon actions
// of actionname-*.dat, category 5). Both pages share their code with the pet's.
//
// Desktop (NCConsole) +0x108 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCActionItemWnd;
class NCTabControl;

enum { NID_SUMMON_FRAME = 0xFE001E };  // the tabs use NID_PET_TABS (0xFE001B)

// The Information page (UClass "CSummonedDetailStatusWnd", 0x14C)
class NCSummonedDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedDetailStatusWnd, NCWnd, 0, 0x1029A970)

    UTexture* BackTex;               // +0xC8 "L2UI.PetWnd.Summon02"
    PetInfo Summon;                  // +0xCC (0xAC in C2) the handler's record, read again by each paint   // C2: offset derived, no evidence

    NCSummonedDetailStatusWnd();                                     // 0x100cfbd0
    // ~NCSummonedDetailStatusWnd                                    // 0x100b4770 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cbc30
    virtual INT OnDestroy();                                         // 38 0x100cbc60 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50) (folded)
};
NW_ASSERT_OFFSET(NCSummonedDetailStatusWnd, Summon, 0xCC);
NW_ASSERT_SIZE(NCSummonedDetailStatusWnd, 0x178);

// The Action page (UClass "CSummonedActionWnd", 0x9C)
class NCSummonedActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedActionWnd, NCWnd, 0, 0x1029C840)

    UTexture* BackTex;               // +0xC8 "L2UI.PetWnd.Summon03"   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    NCActionItemWnd* Items;          // +0xCC   // C2: offset derived, no evidence

    NCSummonedActionWnd();                                           // 0x100cfc90
    // ~NCSummonedActionWnd                                          // 0x100b4e90 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cf020
    virtual INT OnDestroy();                                         // 38 0x100ce0e0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50) (folded)

    void LoadActions();                                              // 0x100ce230 (C1: 0x10092f50) (name ours)
};
NW_ASSERT_OFFSET(NCSummonedActionWnd, Items, 0xCC);
NW_ASSERT_SIZE(NCSummonedActionWnd, 0xD0);

class NCSummonedWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedWnd, NCWnd, 0, 0x1029A448)

    UTexture* BackTex;               // +0xC8 "L2UI.PetWnd.Summon01"   // C2: offset derived, no evidence
    UTexture* HPTex;                 // +0xCC "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0xD0 "L2UI.StatusWnd.statusMP1"
    UTexture* BackTex2;              // +0xD4 (C3) "L2UI_CH3.SummonWnd.summon_back2"
    NCSummonedDetailStatusWnd* DetailWnd; // +0xD8
    NCSummonedActionWnd* ActionWnd;  // +0xDC
    NCTabControl* TabCtrl;           // +0xE0
    PetInfo Summon;                  // +0xE4 (0xAC in C2) the handler's record, read again by each paint   // C2: offset derived, no evidence

    NCSummonedWnd();                                                 // 0x100cfd30
    // ~NCSummonedWnd                                                // 0x100930f0 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cfe00
    virtual INT OnDestroy();                                         // 38 0x100ce400
    virtual void ShowWindow();                                       // 44 0x100ce9b0
    virtual void HideWindow();                                       // 45 0x100cea30 (new in C2)
    virtual void Minimize();                                         // 49 0x100cbce0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ce500

    __declspec(noinline) void ClearActions();                        // 0x100cbd40 (name ours, C2 only): the page's icons dropped
};
NW_ASSERT_OFFSET(NCSummonedWnd, DetailWnd, 0xD8);
NW_ASSERT_OFFSET(NCSummonedWnd, Summon, 0xE4);
NW_ASSERT_SIZE(NCSummonedWnd, 0x190);

#pragma pack(pop)