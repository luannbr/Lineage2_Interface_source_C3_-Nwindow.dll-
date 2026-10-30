#pragma once
// NCTrainWnd - the skill trainer window (UClass "CTrainWnd", 0x9C; desktop +0xE8): a title
// bar ("Skill Trainer", sysstring 477) over two pages in the same place, the skill list
// (NCTrainListWnd) and the details of one skill (NCTrainInfoWnd). The console drives it: a new
// list empties both pages and shows the list (vslot 151), each row comes with 153; a row
// picked asks the server for the skill, whose answer (152) fills and shows the info page and
// its required items (154). "Learn" asks the server to learn it, "List" goes back.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCTrainListBox;
struct NItemEntry;
struct NItemInfo;
struct NTrainEntry;

// NWM_COMMAND ids of the pages' controls
enum
{
    NID_TRAIN_FRAME = 4,
    NID_TRAIN_LEARN = 2,
    NID_TRAIN_LIST = 3,
    NID_TRAIN_LISTBOX = 0x1A04,
};

// A skill as the info page shows it (names ours; 0x4C0 bytes in C2, 0x2C0 in C1: Name and IconName are
// 256 characters). The console fills one from the
// server's answer and hands it to SetInfo; the page keeps its own (NCTrainInfoWnd::Info), whose
// Items are its required item entries.
struct NTrainSkillInfo
{
    TCHAR Name[256];           // +0x000
    TCHAR IconName[256];       // +0x200
    FString Desc;              // +0x400
    TCHAR Unknown20C[64];      // +0x40C drawn after the level
    UTexture* Icon;            // +0x48C
    NItemEntry* Items[4];      // +0x490 required items: the page's are made by its OnCreate
                               //   (NItemEntry::UnknownC34 1: shown) and freed here
    INT SkillID;               // +0x4A0 sent with Level by "Learn"
    INT Level;                 // +0x4A4
    INT Unknown2A8;            // +0x4A8
    INT SP;                    // +0x4AC required SP
    INT MpCost;                // +0x4B0
    INT Range;                 // +0x4B4 not shown when negative
    INT Unknown2B8;            // +0x4B8
    INT Unknown2BC;            // +0x4BC

    NTrainSkillInfo();                                               // 0x10047260
    ~NTrainSkillInfo();                                              // 0x1004ddf0 frees Items
};
NW_ASSERT_OFFSET(NTrainSkillInfo, Desc, 0x400);
NW_ASSERT_OFFSET(NTrainSkillInfo, Icon, 0x48C);
NW_ASSERT_OFFSET(NTrainSkillInfo, SkillID, 0x4A0);
NW_ASSERT_OFFSET(NTrainSkillInfo, Unknown2BC, 0x4BC);
NW_ASSERT_SIZE(NTrainSkillInfo, 0x4C0);

// NCTrainListWnd - the list page (UClass "CTrainListWnd", 0x9C): "Skill list" (sysstring 364)
// over the NCTrainListBox, the player's SP at the bottom right.
class NCTrainListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainListWnd, NCWnd, 0, 0x102B4A40)

    UTexture* BackTex;                     // +0xC8 "L2UI.SkillTrain.TrainerBack1"   // C2: offset derived, no evidence
    NCTrainListBox* ListBox;               // +0xCC

    NCTrainListWnd();                                                // 0x10109300
    // ~NCTrainListWnd                                               // 0x100dcb70 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10107490
    virtual INT OnCreate();                                          // 37 0x101093a0
    virtual INT OnDestroy();                                         // 38 0x100eb490 (NCPetActionWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10107ef0
    // name ours
    virtual INT OnListCommand(LPARAM lParam);                        // 68 0x101080d0 a row picked

    // names ours
    void Clear();                                                    // 0x100b2b90 (C1 only: no C2 match) (the list box's Clear, vslot 70)
    __declspec(noinline) void AddEntry(const NTrainEntry* Entry);                         // 0x10108f10 (the stock NCTrainWnd::AddEntry jumps to it)
};
NW_ASSERT_OFFSET(NCTrainListWnd, ListBox, 0xCC);
NW_ASSERT_SIZE(NCTrainListWnd, 0xD0);

// NCTrainInfoWnd - the skill page (UClass "CTrainInfoWnd", 0x360): "Skill explanation" (366)
// with the icon, name, level, MP cost, range and description; "Learning condition" (367) with
// the required SP and up to four required items; the player's SP; "Learn" and "List".
class NCTrainInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainInfoWnd, NCWnd, 0, 0x102B5488)

    UTexture* BackTex;                     // +0xC8 "L2UI.SkillTrain.TrainerBack3"
    NTrainSkillInfo Info;                  // +0xCC the skill shown (0x4C0 bytes in C2; C1 +0x9C, 0x2C0)
    NCButton* LearnButton;                 // +0x58C "Learn" (sysstring 368)
    NCButton* ListButton;                  // +0x590 "List" (sysstring 369)
    UTexture* OutLineDownTex;              // +0x594 (C3, name ours) "l2ui_ch3.InventoryWnd.Inventory_OutLine_down" the icon's
                                           //   shadow (34 x 34)
    UTexture* OutLineTex;                  // +0x598 "l2ui_ch3.InventoryWnd.Inventory_OutLine" its frame (35 x 35)

    NCTrainInfoWnd();                                                // 0x10109470
    // ~NCTrainInfoWnd                                               // 0x10109500

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10107500
    virtual INT OnCreate();                                          // 37 0x10108f20
    virtual INT OnDestroy();                                         // 38 0x101081d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10108240
    // names ours
    virtual INT OnLearn();                                           // 68 0x10107700
    virtual INT OnList();                                            // 69 0x10107790

    // names ours
    void SetInfo(NTrainSkillInfo* Src);                              // 0x10107570
    void Reset();                                                    // 0x10107690
    void AddRequiredItem(NItemInfo* Item);                           // 0x10108ba0 into a free row
};
NW_ASSERT_OFFSET(NCTrainInfoWnd, Info, 0xCC);
NW_ASSERT_OFFSET(NCTrainInfoWnd, LearnButton, 0x58C);
NW_ASSERT_OFFSET(NCTrainInfoWnd, OutLineTex, 0x598);
NW_ASSERT_SIZE(NCTrainInfoWnd, 0x59C);

class NCTrainWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainWnd, NCWnd, 0, 0x102B4F68)

    NCTrainListWnd* ListWnd;               // +0xC8
    NCTrainInfoWnd* InfoWnd;               // +0xCC

    NCTrainWnd();                                                    // 0x10109560
    // ~NCTrainWnd                                                   // 0x100ddaf0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10109600
    virtual INT OnDestroy();                                         // 38 0x100c4e30
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10108c90

    // the console's (names ours)
    void Reset();                                                    // 0x10107830 both pages
    void ShowList();                                                 // 0x101077e0
    void ShowInfo();                                                 // 0x10107800
    void SetSkillInfo(NTrainSkillInfo* Info);                        // 0x10107820
    void AddRequiredItem(NItemInfo* Item);                           // 0x10108d50
    void AddEntry(const NTrainEntry* Entry);                         // 0x100bca90 (C1 only: no C2 match) (C1 only: no C2 match)
};
NW_ASSERT_OFFSET(NCTrainWnd, ListWnd, 0xC8);
NW_ASSERT_SIZE(NCTrainWnd, 0xD0);

#pragma pack(pop)