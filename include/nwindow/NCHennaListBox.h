#pragma once
// NCHennaListBox - the tattoo list of the henna windows (UClass "CHennaListBox", 0x1F8). Rows of 43 pixels (LineGap
// more): the icon, the name, the description and "<Price> <n>" under them; a click selects one and tells the parent
// (NCHennaListWnd::OnSelect asks the server for its information). Same layout as NCTrainListBox / NCVIPShopItemListBox
// (their stock bodies are shared where identical: Clear 0x100C3310).

#include "nwindow/NCListBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// one row (name ours; 0x141C bytes), as the console hands it (NConsoleWnd's slot 139, 0x100597D0) and the list box keeps
// a copy: from the constructor-less initialisation of 0x10079BA0 / 0x1007AD60 and the readers
struct NHennaListEntry
{
    TCHAR Name[256];           // +0x000
    TCHAR IconName[256];       // +0x200 the icon's texture (AddEntry finds it)
    TCHAR Description[2048];   // +0x400
    UTexture* Icon;            // +0x1400
    INT HennaID;               // +0x1404 what the requests name
    INT ItemID;                // +0x1408 the item (the dye) the tattoo is drawn with
    INT Count;                 // +0x140C how many of it
    INT Price;                 // +0x1410 shown under the description
    INT bCanUse;               // +0x1414 (the console's last argument)
    INT Unknown1418;           // +0x1418 never set by the console

    NHennaListEntry()
    {
        Name[0] = 0;
        IconName[0] = 0;
        Description[0] = 0;
        Icon = NULL;
        HennaID = 0;
        ItemID = 0;
        Count = 0;
        Price = 0;
        bCanUse = 0;
        Unknown1418 = 0;
    }
};
NW_ASSERT_OFFSET(NHennaListEntry, IconName, 0x200);
NW_ASSERT_OFFSET(NHennaListEntry, Description, 0x400);
NW_ASSERT_OFFSET(NHennaListEntry, Icon, 0x1400);
NW_ASSERT_OFFSET(NHennaListEntry, Price, 0x1410);
NW_ASSERT_SIZE(NHennaListEntry, 0x141C);

class NCHennaListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCHennaListBox, NCListBox, 0, 0x102826B8)

    TArray<NHennaListEntry*> Entries;  // +0x220

    NCHennaListBox();                                                // 0x100882b0
    NCHennaListBox(INT InMaxLines, INT InVisibleLines, INT Unused);  // 0x10088370
    // ~NCHennaListBox                                               // 0x10088310

    // stock bodies shared with other list boxes (hooked there): WindowProc 0x100DC140, OnCreate 0x1006E930,
    // OnLButtonDown 0x10079050
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101241c0 (NCChatListBox's)
    virtual INT OnCreate();                                          // 37 0x100862b0 (NCDebugWnd's)
    virtual INT OnDestroy();                                         // 38 0x10087d80
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100867c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100862c0 (NCTrainListBox's)
    virtual INT HitTest(INT X, INT Y);                               // 69 0x100deb10
    virtual void Clear();                                            // 71 0x100963e0 (shared with the other list boxes' copies)

    // names ours
    void AddEntry(const NHennaListEntry* Src);                       // 0x10087e50 (0x1007AEF0 is NCHennaListWnd's)
    UBOOL GetSelectedEntry(NHennaListEntry* Out);                    // 0x10086ab0 0: nothing selected
    void UpdateEntryScrollBar();                                     // 0x100c1110 (C1 only: no C2 match) (shared)
};
NW_ASSERT_OFFSET(NCHennaListBox, Entries, 0x220);
NW_ASSERT_SIZE(NCHennaListBox, 0x22C);

#pragma pack(pop)
