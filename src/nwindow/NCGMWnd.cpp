/*=============================================================================
    NCGMWnd.cpp - the GM's windows (C3 addresses). Stock: 0x1007e070 .. 0x1007f9db (their small functions),
    0x10081e00 .. 0x10083fc3 (paints, OnCreates, OnDestroys, the name searches), 0x10084330 .. 0x100852c1 (the
    constructors and destructors), 0x10136330 (the map iterator's destructor). NCGMStoreWnd's functions sit in
    between; NCGMActionWnd's OnDestroy is NCPetActionWnd's body, its OnPaint NCSkillWnd's.
=============================================================================*/
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCQuestGaraTreeCtrl.h"
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCQuestItemWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NGameData.h"
#include "nwindow/NItemInfo.h"

// sysstring
enum
{
    STR_Item = 2,
    STR_Name = 50,
    STR_Lv = 88,
    STR_Quest = 118,
    STR_Skills = 119,
    STR_Warehouse = 131,
    STR_Inventory = 138,
    STR_AcceptedQuest = 324,
    STR_Description = 325,
    STR_QuestItem = 326,
    STR_Cancel = 327,
    STR_Leader = 342,
    STR_Base = 343,
    STR_HideoutExists = 344,
    STR_CastleExists = 345,
    STR_Status = 433,
    STR_Clan = 439,
    STR_NewTeleport = 899,         // C3 (names ours): the texts of the builder buttons
    STR_NewInstantMove = 900,
    STR_NewAddPetiChat = 902,
};

enum { MSG_EnterUserName = 364 };      // systemmsg "Enter user's name to search."

enum { DIALOG_SystemMessage = 0x1F };  // NDialogInfo::ID: a message, OK only

enum { CLAN_TEXT_COLOR = 0xFFB09B79, CLAN_LABEL_COLOR = 0xFFA3A3A3 };

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

/*-----------------------------------------------------------------------------
    NCGMWnd
-----------------------------------------------------------------------------*/

// 0x10085490 - NCWnd's constructor, then the ten fields cleared one by one
NCGMWnd::NCGMWnd()
{
    TopTex = NULL;
    MidTex = NULL;
    BottomTex = NULL;
    NameEdit = NULL;
    InventoryButton = NULL;
    SkillsButton = NULL;
    StatusButton = NULL;
    QuestButton = NULL;
    WarehouseButton = NULL;
    ClanButton = NULL;
}
NW_HOOK_CTOR(0x10085490, NCGMWnd)

// 0x10085510
NW_HOOK_DTOR(0x10085510, NCGMWnd)

// 0x1007e560 (vslot 25) - the buttons by id: 68 .. 75, 76 (NewVirtual76), 77 .. 79, and 80 .. 83 (placeholders)
INT NCGMWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_GM_INVENTORY)
            return OnInventory() != 0;
        if (wParam == NID_GM_SKILLS)
            return OnSkills() != 0;
        if (wParam == NID_GM_STATUS)
            return OnStatus() != 0;
        if (wParam == NID_GM_QUEST)
            return OnQuest() != 0;
        if (wParam == NID_GM_WAREHOUSE)
            return OnWarehouse() != 0;
        if (wParam == NID_GM_CLAN)
            return OnClan() != 0;
        if (wParam == NID_GM_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_GM_NEW1)
            return OnNewButton1() != 0;
        if (wParam == NID_GM_NEW7)
            return ::nw::VirtualCall<INT>(this, 76) != 0;
        if (wParam == NID_GM_NEW4)
            return OnNewButton4() != 0;
        if (wParam == NID_GM_NEW5)
            return OnNewButton5() != 0;
        if (wParam == NID_GM_NEW6)
            return OnNewButton6() != 0;
        if (wParam == NID_GM_NEW8)
            return ::nw::VirtualCall<INT>(this, 80) != 0;
        if (wParam == NID_GM_NEW9)
            return ::nw::VirtualCall<INT>(this, 81) != 0;
        if (wParam == NID_GM_NEW2)
            return ::nw::VirtualCall<INT>(this, 82) != 0;
        if (wParam == NID_GM_NEW3)
            return ::nw::VirtualCall<INT>(this, 83) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1007e560, NCGMWnd, WindowProc)

// the six view buttons: the desktop's toggle with the name typed, or a message without one (the text is fetched
// twice; C3: the message is in 0xFFDCDCDC)
#define NCGMWND_VIEW_BUTTON(Method, Toggle)                                                   \
    INT NCGMWnd::Method()                                                                     \
    {                                                                                         \
        if (NameEdit->CommitText())                                                           \
            GDesktop->Toggle((TCHAR*)NameEdit->CommitText());                                 \
        else                                                                                  \
            GConsole->ShowSystemMessage(MSG_EnterUserName, 0xFFDCDCDC);                       \
        return 1;                                                                             \
    }

// 0x1007e780 (vslot 68)
NCGMWND_VIEW_BUTTON(OnInventory, ToggleGMInventory)
NW_HOOK_METHOD(0x1007e780, NCGMWnd, OnInventory)
// 0x1007e7e0 (vslot 69)
NCGMWND_VIEW_BUTTON(OnSkills, ToggleGMMagicSkill)
NW_HOOK_METHOD(0x1007e7e0, NCGMWnd, OnSkills)
// 0x1007e840 (vslot 70)
NCGMWND_VIEW_BUTTON(OnStatus, ToggleGMDetailStatus)
NW_HOOK_METHOD(0x1007e840, NCGMWnd, OnStatus)
// 0x1007e8a0 (vslot 71)
NCGMWND_VIEW_BUTTON(OnQuest, ToggleGMQuest)
NW_HOOK_METHOD(0x1007e8a0, NCGMWnd, OnQuest)
// 0x1007e900 (vslot 72)
NCGMWND_VIEW_BUTTON(OnWarehouse, ToggleGMStore)
NW_HOOK_METHOD(0x1007e900, NCGMWnd, OnWarehouse)
// 0x1007e960 (vslot 73)
NCGMWND_VIEW_BUTTON(OnClan, ToggleGMAction)
NW_HOOK_METHOD(0x1007e960, NCGMWnd, OnClan)

// 0x1007e9c0 (vslot 74) - the view windows shown are hidden, then this one. The first two
// tests are crossed in the stock code: the inventory being shown hides the skill window and
// the skill window being shown hides the inventory, so the inventory never closes here and
// the skill window only along with it.
INT NCGMWnd::OnClose()
{
    if (GDesktop->GMInventoryWnd->IsShowWindow())
        GDesktop->GMMagicSkillWnd->HideWindow();
    if (GDesktop->GMMagicSkillWnd->IsShowWindow())
        GDesktop->GMInventoryWnd->HideWindow();
    if (GDesktop->GMDetailStatusWnd->IsShowWindow())
        GDesktop->GMDetailStatusWnd->HideWindow();
    if (GDesktop->GMQuestWnd->IsShowWindow())
        GDesktop->GMQuestWnd->HideWindow();
    if (GDesktop->GMStoreWnd->IsShowWindow())
        GDesktop->GMStoreWnd->HideWindow();
    if (GDesktop->GMActionWnd->IsShowWindow())
        GDesktop->GMActionWnd->HideWindow();
    NCWnd::HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1007e9c0, NCGMWnd, OnClose)

// a button of the window (repeated in the stock OnCreate): 54 x 18 at (X, Y), the two "sek" textures
static FORCEINLINE NCButton* CreateGMButton(NCWnd* Parent, INT ID, INT X, INT Y)
{
    NCButton* Button = new NCButton(TEXT("sek.cbui91"), TEXT("sek.cbui93"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 54, 18);
    Parent->CreateChild(Button, NWS_VISIBLE);
    return Button;
}

// 0x100828b0 (vslot 37) - three textures, the title bar (25 high at y 19), the name's edit box and the buttons: eight
// rows of three of 54 x 18 at x 10, 74 and 138 from y 75 down in steps of 21 (C3: seventeen buttons and the Close
// one at (79, 194))
INT NCGMWnd::OnCreate()
{
    NCWnd::OnCreate();
    TopTex = FindTexture(TEXT("sek.ui02"));
    MidTex = FindTexture(TEXT("sek.cbui157"));
    BottomTex = FindTexture(TEXT("sek.ui03"));
    FrameCtrl = new NCFrameCtrl(0, 0, 0);   // C3: not decorated (the other GM windows' title bars are)
    FrameCtrl->ID = NID_GM_FRAME;
    FrameCtrl->SetPosition(0, 19, (DWORD)(INT)Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    NameEdit = new NCEditBox(0, TEXT("NWindow.ChatBack"), 1, 128, 0);
    NameEdit->ID = NID_GM_NAME;
    NameEdit->SetPosition(35, 55, 112, 14);
    CreateChild(NameEdit, NWS_VISIBLE);

    NewButton1 = CreateGMButton(this, NID_GM_NEW1, 10, 75);
    NewButton1->SetText(*GL2GameData.SysString[STR_NewTeleport]);
    NewButton8 = CreateGMButton(this, NID_GM_NEW8, 74, 75);
    NewButton8->SetText(*GL2GameData.SysString[STR_NewInstantMove]);
    NewButton7 = CreateGMButton(this, NID_GM_NEW7, 138, 75);
    NewButton7->SetTextID(688);
    StatusButton = CreateGMButton(this, NID_GM_STATUS, 10, 96);
    StatusButton->SetText(*GL2GameData.SysString[STR_Status]);
    InventoryButton = CreateGMButton(this, NID_GM_INVENTORY, 74, 96);
    InventoryButton->SetText(*GL2GameData.SysString[STR_Inventory]);
    SkillsButton = CreateGMButton(this, NID_GM_SKILLS, 138, 96);
    SkillsButton->SetText(*GL2GameData.SysString[STR_Skills]);
    QuestButton = CreateGMButton(this, NID_GM_QUEST, 10, 117);
    QuestButton->SetText(*GL2GameData.SysString[STR_Quest]);
    NewButton3 = CreateGMButton(this, NID_GM_NEW3, 74, 117);
    NewButton3->SetTextID(901);
    WarehouseButton = CreateGMButton(this, NID_GM_WAREHOUSE, 138, 117);
    WarehouseButton->SetText(*GL2GameData.SysString[STR_Warehouse]);
    ClanButton = CreateGMButton(this, NID_GM_CLAN, 10, 138);
    ClanButton->SetText(*GL2GameData.SysString[STR_Clan]);
    NewButton2 = CreateGMButton(this, NID_GM_NEW2, 74, 138);
    NewButton2->SetText(*GL2GameData.SysString[STR_NewAddPetiChat]);
    NewButton9 = CreateGMButton(this, NID_GM_NEW9, 138, 138);
    NewButton9->SetTextID(903);
    NewButton4 = CreateGMButton(this, NID_GM_NEW4, 10, 159);
    NewButton4->SetTextID(690);
    NewButton5 = CreateGMButton(this, NID_GM_NEW5, 74, 159);
    NewButton5->SetTextID(691);
    NewButton6 = CreateGMButton(this, NID_GM_NEW6, 138, 159);
    NewButton6->SetTextID(692);

    CloseButton = new NCButton;
    CloseButton->ID = NID_GM_CLOSE;
    CloseButton->SetPosition(79, 194, 50, 16);
    CreateChild(CloseButton, NWS_VISIBLE);
    CloseButton->SetPushedTex(TEXT("sek.cbui27"));
    return 1;
}
NW_HOOK_METHOD(0x100828b0, NCGMWnd, OnCreate)

// 0x10081f10 (vslot 38) - the name's box and the buttons go (the textures and the title bar are left alone)
INT NCGMWnd::OnDestroy()
{
    guard(NCGMWnd::OnDestroy);
    DestroyChild(this, NameEdit);
    DestroyChild(this, InventoryButton);
    DestroyChild(this, SkillsButton);
    DestroyChild(this, StatusButton);
    DestroyChild(this, QuestButton);
    DestroyChild(this, WarehouseButton);
    DestroyChild(this, ClanButton);
    DestroyChild(this, CloseButton);
    DestroyChild(this, NewButton1);
    DestroyChild(this, NewButton7);
    DestroyChild(this, NewButton4);
    DestroyChild(this, NewButton5);
    DestroyChild(this, NewButton6);
    DestroyChild(this, NewButton2);
    DestroyChild(this, NewButton8);
    DestroyChild(this, NewButton3);
    DestroyChild(this, NewButton9);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081f10, NCGMWnd, OnDestroy)

// 0x10082270 (vslot 56) - the middle texture (208 x 144 at y 43), the top (208 x 47) and the bottom (208 x 48 at
// y 184) and the "Name" label; no guard
INT NCGMWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (MidTex)
        Device->DrawTexture(0, 43, 208, 144, 0, 0, 208, 144, MidTex, 0xFF, 1);
    if (TopTex)
        Device->DrawTexture(0, 0, 208, 47, 0, 0, 208, 47, TopTex, 0xFF, 1);
    if (BottomTex)
        Device->DrawTexture(0, 184, 208, 48, 0, 0, 208, 48, BottomTex, 0xFF, 1);
    Device->DrawText(7, 55, 0xFFDCDCDC, *GL2GameData.SysString[STR_Name], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10082270, NCGMWnd, OnPaint)

// the builder command of a button: the text it adds to the name typed (the stock repeats it for each button)
static FORCEINLINE void BuilderWithName(const TCHAR* Prefix, const TCHAR* Name)
{
    FString Command(Prefix);
    Command += Name;
    L2ParamStack Params(10);
    Params.PushBack((void*)*Command);
    GConsole->BuilderCommand(Params);
}

// 0x1007eb20 (vslot 75, name ours) - the player named in the box: the builder command "teleportto <name>"
INT NCGMWnd::OnNewButton1()
{
    const TCHAR* Name = NameEdit->CommitText();
    if (Name)
        BuilderWithName(TEXT("teleportto "), Name);
    return 1;
}
NW_HOOK_METHOD(0x1007eb20, NCGMWnd, OnNewButton1)

// 0x1007ec80 (slot 82, name ours) - "add_peti_chat <name>"
INT NCGMWnd::OnNewButton2()
{
    const TCHAR* Name = NameEdit->CommitText();
    if (Name)
        BuilderWithName(TEXT("add_peti_chat "), Name);
    return 1;
}
NW_HOOK_METHOD(0x1007ec80, NCGMWnd, OnNewButton2)

// 0x1007ed40 (slot 83, name ours) - "debug <name>"
INT NCGMWnd::OnNewButton3()
{
    const TCHAR* Name = NameEdit->CommitText();
    if (Name)
        BuilderWithName(TEXT("debug "), Name);
    return 1;
}
NW_HOOK_METHOD(0x1007ed40, NCGMWnd, OnNewButton3)

// 0x1007ebe0 (slot 80, name ours) - the builder command "instant_move" (no name)
INT NCGMWnd::OnNewButton8()
{
    FString Command(TEXT("instant_move"));
    L2ParamStack Params(10);
    Params.PushBack((void*)*Command);
    GConsole->BuilderCommand(Params);
    return 1;
}
NW_HOOK_METHOD(0x1007ebe0, NCGMWnd, OnNewButton8)

// a question about the name typed (the two buttons below): sysmsg Msg in an OK-only box of id ID, the name its parameter
static FORCEINLINE void AskAboutName(INT ID, INT Msg, const TCHAR* Name)
{
    NDialogInfo Info;
    Info.ID = ID;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetSystemMessage(Msg);
    Info.SetParamText(Name);
    GConsole->ShowDialog(&Info);
}

// 0x100823b0 (slot 76, name ours) - the dialog 0x4B with system message 0x4C4 and the name
INT NCGMWnd::OnNewButton7()
{
    const TCHAR* Name = NameEdit->CommitText();
    if (Name)
        AskAboutName(0x4B, 0x4C4, Name);
    return 1;
}
NW_HOOK_METHOD(0x100823b0, NCGMWnd, OnNewButton7)

// 0x10082470 (slot 81, name ours) - the dialog 0x4A with system message 0x4C5 and the name
INT NCGMWnd::OnNewButton9()
{
    const TCHAR* Name = NameEdit->CommitText();
    if (Name)
        AskAboutName(0x4A, 0x4C5, Name);
    return 1;
}
NW_HOOK_METHOD(0x10082470, NCGMWnd, OnNewButton9)

// the match of a name search (the three buttons below): "ClassID:%d Name:%s" in an OK-only dialog box (no owner)
static FORCEINLINE void ShowClassID(INT ClassID, const TCHAR* Name)
{
    TCHAR Text[256];
    appSprintf(Text, TEXT("ClassID:%d Name:%s"), ClassID, Name);
    NDialogInfo Info;
    Info.ID = DIALOG_SystemMessage;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetText(Text);
    GConsole->ShowDialog(&Info);
}

// TMap's iterator as the stock keeps it on the stack (name ours): the map, its pairs, the index and the number of pairs
// removed while walking. The destructor (0x10136330) relaxes the hash and rehashes the map when something was
// removed; the searches below remove nothing, and the stock code runs it on the match path only
struct NSearchMapIterator
{
    INT* Map;            // +0x00 the map: Pairs (+0 data, +4 Num, +8 Max), Hash (+0xC), HashCount (+0x10)
    INT* PairsRef;       // +0x04
    INT Index;           // +0x08
    INT Removed;         // +0x0C

    __declspec(noinline) void Relax();   // 0x10136330: TMapBase::Relax, then the map's Rehash (0x100626e0)
};

void NSearchMapIterator::Relax()
{
    if (Removed)
    {
        while (Map[4] > Map[1] * 2 + 8)
            Map[4] /= 2;
        ((NDataMap<NViewItemData>*)Map)->Rehash();   // 0x100626e0
    }
}
NW_HOOK_METHOD(0x10136330, NSearchMapIterator, Relax)

// 0x10083ba0 (vslot 77, name ours) - the first NPC of the NPC table (GL2GameData +0x180C, by id) whose name is the one
// typed: its class id (the NPC id plus 1,000,000) and name in a dialog box. The records are the table's walk by TMap's
// iterator, from the start; none found: nothing
INT NCGMWnd::OnNewButton4()
{
    const TCHAR* Search = NameEdit->CommitText();
    if (Search)
    {
        NDataMap<NViewNpcData>* Map = GL2GameData.NpcTable;
        NSearchMapIterator It = { (INT*)Map, (INT*)Map, 0, 0 };
        for (; It.Index < Map->Pairs.Num(); It.Index++)
        {
            NViewNpcData* Npc = Map->Pairs(It.Index).Value;
            if (Npc && *Npc->Name && !appStrcmp(*Npc->Name, Search))
            {
                ShowClassID(Npc->ID + 1000000, *Npc->Name);
                It.Relax();
                return 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10083ba0, NCGMWnd, OnNewButton4)

// 0x10083d10 (vslot 78, name ours) - the same for the item table (+0x1808): the item's id (+0xC) and name
INT NCGMWnd::OnNewButton5()
{
    const TCHAR* Search = NameEdit->CommitText();
    if (Search)
    {
        NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
        NSearchMapIterator It = { (INT*)Map, (INT*)Map, 0, 0 };
        for (; It.Index < Map->Pairs.Num(); It.Index++)
        {
            NViewItemData* Item = Map->Pairs(It.Index).Value;
            if (Item && *Item->Name && !appStrcmp(*Item->Name, Search))
            {
                ShowClassID(Item->ItemID, *Item->Name);
                It.Relax();
                return 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10083d10, NCGMWnd, OnNewButton5)

// 0x10083e70 (vslot 79, name ours) - the same for the skill table (+0x3CDC: one record per level, the first level with the
// name is found). C3: the record starts with the name (an FString at +0) and the skill id is at +0x18
INT NCGMWnd::OnNewButton6()
{
    const TCHAR* Search = NameEdit->CommitText();
    if (Search)
    {
        NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
        NSearchMapIterator It = { (INT*)Map, (INT*)Map, 0, 0 };
        for (; It.Index < Map->Pairs.Num(); It.Index++)
        {
            FL2MagicSkillData* Skill = Map->Pairs(It.Index).Value;
            if (Skill)
            {
                FString& Name = *(FString*)Skill;
                if (*Name && !appStrcmp(*Name, Search))
                {
                    ShowClassID(*(INT*)((BYTE*)Skill + 0x18), *Name);
                    It.Relax();
                    return 1;
                }
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10083e70, NCGMWnd, OnNewButton6)

NW_CHECK_VTABLE(NCGMWnd, 0x101890d0, 84)

/*-----------------------------------------------------------------------------
    NCGMInvenItemWnd, NCGMEquipItemWnd
-----------------------------------------------------------------------------*/

// 0x100843e0 - NCInvenItemWnd's default constructor
NCGMInvenItemWnd::NCGMInvenItemWnd()
{
}
NW_HOOK_CTOR(0x100843e0, NCGMInvenItemWnd)

// 0x10084480 (sub_10084480, ret 0x10) - NCInvenItemWnd's six-argument constructor with the gaps 1 and 1
NCGMInvenItemWnd::NCGMInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x10084480, NCGMInvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10084430
NW_HOOK_DTOR(0x10084430, NCGMInvenItemWnd)

// 0x1007e1b0 (vslot 25) - the base's answer as a plain 0 / 1
INT NCGMInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCInvenItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e1b0, NCGMInvenItemWnd, WindowProc)

// 0x1007e280 (vslots 63, 64, 75) - NCGMSkillItemWnd::OnLButtonUp's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam) NW_STOCK(0x1007e280)
#else
NW_SLOT_IS("?OnLButtonUp@NCGMInvenItemWnd@@UAEHIJ@Z", "?OnLButtonUp@NCGMSkillItemWnd@@UAEHIJ@Z")   // NCGMInvenItemWnd::OnLButtonUp = NCGMSkillItemWnd::OnLButtonUp (0x1007e280)
#endif
#if NW_HOOK_MODE
NW_NAKED INT NCGMInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam) NW_STOCK(0x1007e280)
#else
NW_SLOT_IS("?OnRButtonDown@NCGMInvenItemWnd@@UAEHIJ@Z", "?OnLButtonUp@NCGMSkillItemWnd@@UAEHIJ@Z")   // NCGMInvenItemWnd::OnRButtonDown = NCGMSkillItemWnd::OnLButtonUp (0x1007e280)
#endif
#if NW_HOOK_MODE
NW_NAKED INT NCGMInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam) NW_STOCK(0x1007e280)
#else
NW_SLOT_IS("?OnLButtonDblClick@NCGMInvenItemWnd@@UAEHIJ@Z", "?OnLButtonUp@NCGMSkillItemWnd@@UAEHIJ@Z")   // NCGMInvenItemWnd::OnLButtonDblClick = NCGMSkillItemWnd::OnLButtonUp (0x1007e280)
#endif

NW_CHECK_VTABLE(NCGMInvenItemWnd, 0x10188370, 76)

// 0x10084330 (ret 4) - NCEquipItemWnd's constructor, the two ring flags cleared
NCGMEquipItemWnd::NCGMEquipItemWnd(INT InCellSize)
    : NCEquipItemWnd(InCellSize)
{
    bFirstRingTaken = 0;
    bFirstFingerTaken = 0;
}
NW_HOOK_CTOR_ARGS(0x10084330, NCGMEquipItemWnd, (INT A), (A))

// 0x10084390
NW_HOOK_DTOR(0x10084390, NCGMEquipItemWnd)

// 0x1007e070 (vslot 25) - the base's answer as a plain 0 / 1
INT NCGMEquipItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCEquipItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e070, NCGMEquipItemWnd, WindowProc)

// 0x1007e090 (vslot 63) - the carried item is only put down; also slots 64 and 69
INT NCGMEquipItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1007e090, NCGMEquipItemWnd, OnLButtonUp)

// 0x1007e090 (vslots 64 and 69) - the same body, shared with OnLButtonUp
#if NW_HOOK_MODE
NW_NAKED INT NCGMEquipItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam) NW_STOCK(0x1007e090)
#else
NW_SLOT_IS("?OnRButtonDown@NCGMEquipItemWnd@@UAEHIJ@Z", "?OnLButtonUp@NCGMEquipItemWnd@@UAEHIJ@Z")   // NCGMEquipItemWnd::OnRButtonDown = NCGMEquipItemWnd::OnLButtonUp (0x1007e090)
#endif
#if NW_HOOK_MODE
NW_NAKED INT NCGMEquipItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam) NW_STOCK(0x1007e090)
#else
NW_SLOT_IS("?OnLButtonDblClick@NCGMEquipItemWnd@@UAEHIJ@Z", "?OnLButtonUp@NCGMEquipItemWnd@@UAEHIJ@Z")   // NCGMEquipItemWnd::OnLButtonDblClick = NCGMEquipItemWnd::OnLButtonUp (0x1007e090)
#endif

// 0x1007e0b0 (vslot 68) - the slot by the body part bits alone (the console is not asked): the first four bits are
// tested in turn (the ear, the other ear, the finger, the other finger), the rest by value; -1 for any other
INT NCGMEquipItemWnd::GetEquipSlot(INT BodyPart, INT ObjectID)
{
    if (BodyPart & 2)
        return 8;
    if (BodyPart & 4)
        return 9;
    if (BodyPart & 0x10)
        return 0xD;
    if (BodyPart & 0x20)
        return 0xE;
    switch (BodyPart)
    {
    case 0x1:
        return 0;
    case 0x8:
        return 4;
    case 0x40:
        return 1;
    case 0x80:
        return 5;
    case 0x100:
        return 7;
    case 0x200:
        return 0xA;
    case 0x400:
        return 6;
    case 0x800:
        return 0xB;
    case 0x1000:
        return 0xC;
    case 0x2000:
        return 2;
    case 0x4000:
        return 0x10;
    case 0x8000:
        return 0xF;
    case 0x10000:
        return 3;
    }
    return -1;
}
NW_HOOK_METHOD(0x1007e0b0, NCGMEquipItemWnd, GetEquipSlot)

// 0x10082530 - NCEquipItemWnd's AddItem; then on the new entry's slot mask: the first of two bits 2 / 4 (0x30: 0x10 /
// 0x20, two earring or ring slots) found keeps its low bit and clears the other, the second one takes the other (the
// two flags remember which was given out)
UBOOL NCGMEquipItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= NumSlots)
        return 0;
    if (Info->IconName[0])
        Info->Icon = FindTexture(Info->IconName);
    if (Info->Unknown200[0])
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(Info->Unknown200);
    if (Info->Unknown300[0])
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(Info->Unknown300);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC04)
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC08)
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    if (Item->BodyPart & 6)
    {
        if (bFirstRingTaken == 0)
        {
            Item->BodyPart &= ~2;
            bFirstRingTaken = 1;
        }
        else
            Item->BodyPart &= ~4;
    }
    if (Item->BodyPart & 0x30)
    {
        if (bFirstFingerTaken == 0)
        {
            Item->BodyPart &= ~0x10;
            bFirstFingerTaken = 1;
        }
        else
            Item->BodyPart &= ~0x20;
    }
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->Unknown1C98)
        {
            bHasUnknown1C98 = 1;
            break;
        }
        bHasUnknown1C98 = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10082530, NCGMEquipItemWnd, AddItem)

// 0x1007ee00 (vslot 56) - NCEquipItemWnd's paint as code of its own (same guard name): the icons in their slots (the
// console is not asked which slot: GetEquipSlot, the two-slot items in both slots 15 / 16, the second under the "dual" cap;
// a bow in the right hand alone while an item with Unknown1C98 is equipped), the frames, and the tattoo boxes for three
// slots whatever the character may have
INT NCGMEquipItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCEquipItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 2.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        INT Slot = GetEquipSlot(Item->BodyPart, Item->ObjectID);
        UBOOL bTwoSlot = 0;
        switch (Slot)
        {
        case 0:  X = 7;    Y = 7;    break;
        case 1:  X = 0x2E; Y = 7;    break;
        case 2:  X = 0x55; Y = 7;    break;
        case 3:  X = 0x82; Y = 7;    break;
        case 4:  X = 0xA9; Y = 7;    break;
        case 5:  X = 7;    Y = 0x2D; break;
        case 6:  X = 0x2E; Y = 0x2D; break;
        case 7:  X = 0x55; Y = 0x2D; break;
        case 8:  X = 0x82; Y = 0x2D; break;
        case 9:  X = 0xA9; Y = 0x2D; break;
        case 10: X = 7;    Y = 0x53; break;
        case 11: X = 0x2E; Y = 0x53; break;
        case 12: X = 0x55; Y = 0x53; break;
        case 13: X = 0x82; Y = 0x53; break;
        case 14: X = 0xA9; Y = 0x53; break;
        case 15:
        case 16:
            bTwoSlot = 1;
            break;
        default:
            continue;
        }
        if (Item->UnknownC30 == 6 && bHasUnknown1C98)
        {
            X = 7;
            Y = 0x2D;
            bTwoSlot = 0;
        }
        if (!bTwoSlot)
        {
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
            INT FrameX = X - 1;
            INT FrameY = Y - 1;
            if (Item->IsSelected())
                Device->DrawTexture(FrameX, FrameY, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(FrameX, FrameY, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(FrameX, FrameY, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
            continue;
        }
        // a two-slot item: the icons are the item's own, or its second and third when the names are there
        UTexture* First;
        UTexture* Second;
        if (Item->Unknown30C[0] != 0)
        {
            First = Item->GetIcon2();
            Second = Item->GetIcon3();
        }
        else if (Item->Unknown20C[0] != 0)
        {
            First = Item->GetIcon2();
            Second = Item->GetIcon2();
        }
        else
        {
            First = Item->GetIcon();
            Second = Item->GetIcon();
        }
        if (Slot == 16)
        {
            Device->DrawTexture(7, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, First, 0xFF, 1);
            Device->DrawTexture(0x55, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, Second, 0xFF, 1);
            Device->DrawTexture(0x55, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            if (Item->IsSelected())
                Device->DrawTexture(6, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(6, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(6, 0x2C, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
        }
        else if (Slot == 15)
        {
            Device->DrawTexture(0x2E, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, First, 0xFF, 1);
            Device->DrawTexture(0x2E, 0x53, CellSize, CellSize, 0, 0, CellSize, CellSize, Second, 0xFF, 1);
            Device->DrawTexture(0x2E, 0x53, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            if (Item->IsSelected())
                Device->DrawTexture(0x2D, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(0x2D, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(0x2D, 0x2C, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
        }
    }
    if (HennaBackTex)
    {
        for (INT i = 0; i < 3; i++)
        {
            Device->DrawTexture(0xD7, i * 0x1B + 8, 0x1A, 0x1A, 0, 0, 0x1A, 0x1A, HennaBackTex, 0xFF, 1);
            if (i < Hennas.Num() && Hennas(i)->GetIcon())
            {
                UTexture* Icon = Hennas(i)->GetIcon();
                INT TopY = i * 0x1B + 9;
                Device->DrawStretchedTex(0xD8, TopY, 0x18, 0x18, Icon, 1, 0xFF);
                if (Hennas(i)->UnknownC34 == 0)
                    Device->DrawTexture(0xD9, TopY, 0x18, 0x18, 0, 0, 0x18, 0x18, FindTexture(TEXT("NWindow.ChatBack")), 0xFF, 1);
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007ee00, NCGMEquipItemWnd, OnPaint)

NW_CHECK_VTABLE(NCGMEquipItemWnd, 0x10188258, 70)

/*-----------------------------------------------------------------------------
    NCGMInventoryWnd
-----------------------------------------------------------------------------*/

// 0x100844b0 - NCWnd's constructor, four fields cleared
NCGMInventoryWnd::NCGMInventoryWnd()
{
    BackTex = NULL;
    TabCtrl = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
}
NW_HOOK_CTOR(0x100844b0, NCGMInventoryWnd)

// 0x10084510
NW_HOOK_DTOR(0x10084510, NCGMInventoryWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCGMInventoryWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCGMInventoryWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x1007e1d0 - the paperdoll and the two pages emptied, the two ring flags cleared
void NCGMInventoryWnd::ClearItems()
{
    EquipWnd->Clear();
    ItemWnd[0]->Clear();
    ItemWnd[1]->Clear();
    EquipWnd->bFirstRingTaken = 0;
    EquipWnd->bFirstFingerTaken = 0;
}
NW_HOOK_METHOD(0x1007e1d0, NCGMInventoryWnd, ClearItems)

// 0x1007e220 (vslot 44)
void NCGMInventoryWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x1007e220, NCGMInventoryWnd, ShowWindow)

// 0x10082740 - an item that is worn (NItemInfo +0x1798) goes to the paperdoll, any other into the page of its kind (the
// quest items, Param 3, in the second)
UBOOL NCGMInventoryWnd::AddItem(NItemInfo* Info)
{
    INT Page = 0;
    if (Info->Param == 3)
        Page = 1;
    if (NItemInfoInt(Info, 0x1798) != 0)
    {
        EquipWnd->AddItem(Info);
        return 1;
    }
    if (ItemWnd[Page])
        ItemWnd[Page]->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x10082740, NCGMInventoryWnd, AddItem)

// 0x1007f470 (vslot 56) - the background under the title bar, the adena (right-aligned at x 199) and the two icons
INT NCGMInventoryWnd::OnPaint(NCGDevice* Device)
{
    guard(NCGMInventoryWnd::OnPaint);
    TCHAR Text[128];
    TCHAR Formatted[128];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0x14, 0x100, 0x17D, 0, 0, 0x100, 0x17D, BackTex, 0xFF, 1);
    appSprintf(Text, TEXT("%d"), ItemWnd[0]->GetAdena());
    FormatNumber(Formatted, Text);
    appSprintf(Text, TEXT("%s"), Formatted);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(0xC7 - Size.cx, 0x164, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    if (AdenaTex)
        Device->DrawTexture(0x62, 0x163, 0x10, 0xC, 0, 0, 0x10, 0xC, AdenaTex, 0xFF, 1);
    if (WeightTex)
        Device->DrawTexture(0x62, 0x174, 0x10, 0xC, 0, 0, 0x10, 0xC, WeightTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007f470, NCGMInventoryWnd, OnPaint)

// 0x1007f650 (vslot 38)
INT NCGMInventoryWnd::OnDestroy()
{
    guard(NCGMInventoryWnd::OnDestroy);
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, EquipWnd);
    DestroyChild(this, TabCtrl);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007f650, NCGMInventoryWnd, OnDestroy)

// 0x10084560 (vslot 37) - the title bar, the paperdoll (32-pixel cells), the two pages (6 x 4 icons of 32 pixels, 160
// items at most) and their tabs
INT NCGMInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Back"));
    AdenaTex = FindTexture(TEXT("L2UI.WindowIcon.WindowIconAdena"));
    WeightTex = FindTexture(TEXT("L2UI.WindowIcon.Weight"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_GMINVENTORY_FRAME;
    FrameCtrl->SetPosition(0, 0, 256, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    EquipWnd = new NCGMEquipItemWnd(32);
    EquipWnd->SetPosition(8, 31, 240, 122);
    CreateChild(EquipWnd, NWS_VISIBLE | 0x20);

    ItemWnd[0] = new NCGMInvenItemWnd(6, 4, 160, 32);
    ItemWnd[0]->SetPosition(9, 189, 236, 139);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);

    ItemWnd[1] = new NCGMInvenItemWnd(6, 4, 160, 32);
    ItemWnd[1]->SetPosition(9, 189, 236, 139);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_GMINVENTORY_TABS;
    TabCtrl->SetPosition(12, 159, 189, 23);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Item, ItemWnd[0], 94, 23, TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"),
                    TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1"), 1, -1);
    TabCtrl->AddTab(STR_Quest, ItemWnd[1], 94, 23, TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"),
                    TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1"), 1, -1);
    return 1;
}
NW_HOOK_METHOD(0x10084560, NCGMInventoryWnd, OnCreate)

// 0x1007e240 (name ours) - the console's slot 143 (GMAddHennaInfo): the paperdoll's tattoo (a tail jump to NCEquipItemWnd::AddHenna)
void NCGMInventoryWnd::AddHenna(INT HennaID, INT bUsable)
{
    EquipWnd->AddHenna(HennaID, bUsable);
}
NW_HOOK_METHOD(0x1007e240, NCGMInventoryWnd, AddHenna)

// 0x1007e250 (name ours) - the console's slot 144 (GMReceiveHennaInfo): the paperdoll's tattoos gone (a tail jump to NCEquipItemWnd::ClearHennas)
void NCGMInventoryWnd::ClearHennas()
{
    EquipWnd->ClearHennas();
}
NW_HOOK_METHOD(0x1007e250, NCGMInventoryWnd, ClearHennas)

NW_CHECK_VTABLE(NCGMInventoryWnd, 0x101884a0, 68)

/*-----------------------------------------------------------------------------
    NCGMQuestWnd (C3: NCQuestGaraTreeWnd's copy, with a list of item counts at +0x180: the constructor and the
    destructor are the base's with that list; ShowWindow, the tool tip, the mouse move and the two buttons are the base's)
-----------------------------------------------------------------------------*/

enum
{
    STR_QuestTitle = 324,           // the title line (GL2GameData + 0x669BC)
    STR_QuestInfo1 = 861,           // .. 865: the five labels of the quest's info lines (+ 0x682E8 .. 0x68318)
};

// 0x1002c550 (sub_1002c550, a method of NToolTipInfo, ret 0xC; the quest tree windows' OnCreate is its other caller) -
// the tip made of lines "label" with an icon each: emptied, set to the Style given (0x1000: the title box only, with
// the lines under it), a line per text (the icon of the same number in front) and measured (Layout). Declared on a view
// of NToolTipInfo (its header is not this lane's)
struct NToolTipInfoLinesView : public NToolTipInfo
{
    __declspec(noinline) void SetIconLines(TArray<FString*>* Texts, TArray<UTexture*>* Icons, DWORD InStyle)
    {
        Clear();
        Style = InStyle;
        for (INT i = 0; i < Texts->Num(); i++)
        {
            NToolTipLine Line;
            appStrcpy(Line.Label, **(*Texts)(i));
            Line.Icon = (*Icons)(i);
            INT Index = Lines.Add(1);
            Lines(Index) = Line;
        }
        Layout();
    }
};
NW_HOOK_METHOD(0x1002c550, NToolTipInfoLinesView, SetIconLines)

// 0x10084fa0 - the base's constructor, then the list of item counts empty
NCGMQuestWnd::NCGMQuestWnd()
{
}
NW_HOOK_CTOR(0x10084fa0, NCGMQuestWnd)

// 0x10085000 - the list, then the base's destructor
NW_HOOK_DTOR(0x10085000, NCGMQuestWnd)

// 0x1007e310 (vslot 25) - NCQuestGaraTreeWnd's (0x100da270) answer as a plain 0 / 1
INT NCGMQuestWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCQuestGaraTreeWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e310, NCGMQuestWnd, WindowProc)

// 0x100eb490 (vslot 38) - NCSSQMainEventWnd's body, shared (hooked there): the control at +0xCC goes, then NCWnd's. The
// GM window made no button or check box of the base's
INT NCGMQuestWnd::OnDestroy()
{
    DestroyChild(this, TreeCtrl);
    NCWnd::OnDestroy();
    return 1;
}

// 0x100836d0 (vslot 37) - NCWnd's, the title bar (256 x 20), the background and the tool tip button's textures, the
// tree (242 x 274 at (7, 47)), and the tip of the five info lines of a quest (the labels are system strings 861 ..
// 865, the icons "QuestWndInfoIcon_1" .. "_5"); the lists made for it are emptied again and the counts are cleared
INT NCGMQuestWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_GMQUEST_FRAME;
    FrameCtrl->SetPosition(0, 0, 0x100, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndBack"));
    ToolTipBtnTex = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndToolTipBtn"));
    TreeCtrl = new NCQuestGaraTreeCtrl();
    TreeCtrl->SetPosition(7, 0x2F, 0xF2, 0x112);
    CreateChild(TreeCtrl, NWS_VISIBLE);

    TArray<FString*> Texts;
    TArray<UTexture*> Icons;
    for (INT i = 0; i < 5; i++)
    {
        FString* Text = new FString(*GL2GameData.SysString[STR_QuestInfo1 + i]);
        Texts(Texts.Add(1)) = Text;
    }
    static const TCHAR* const IconNames[5] =
    {
        TEXT("L2UI_CH3.QuestWnd.QuestWndInfoIcon_1"), TEXT("L2UI_CH3.QuestWnd.QuestWndInfoIcon_2"),
        TEXT("L2UI_CH3.QuestWnd.QuestWndInfoIcon_3"), TEXT("L2UI_CH3.QuestWnd.QuestWndInfoIcon_4"),
        TEXT("L2UI_CH3.QuestWnd.QuestWndInfoIcon_5"),
    };
    for (INT i = 0; i < 5; i++)
    {
        UTexture* Icon = FindTexture(IconNames[i]);
        Icons(Icons.Add(1)) = Icon;
    }
    ((NToolTipInfoLinesView*)&ToolTip)->SetIconLines(&Texts, &Icons, 0x1000);
    for (INT i = 0; i < Texts.Num(); i++)
    {
        Texts(i)->Empty();
        delete Texts(i);
    }
    Texts.Empty();
    Icons.Empty();
    Counts.Empty();
    return 1;
}
NW_HOOK_METHOD(0x100836d0, NCGMQuestWnd, OnCreate)

// 0x10081780 (vslot 56, guard: NCGMQuestWnd::OnPaint) - the background (256 x 335 at y 20), the title (system string 324)
// and "(N/15)", the tool tip button; the marked quest's required items: the amount held of each (+0x1DFC) is the sum
// of the entries of Counts with its item id (+0x1D34)
INT NCGMQuestWnd::OnPaint(NCGDevice* Device)
{
    guard(NCGMQuestWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0x14, 0x100, 0x14F, 0, 0, 0x100, 0x14F, BackTex, 0xFF, 1);
    Device->DrawText(0xB, 0x20, 0xFFDCDCDC, *GL2GameData.SysString[STR_QuestTitle], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    TCHAR Buffer[32];
    appSprintf(Buffer, TEXT("(%d/%d)"), NumQuests, 15);
    Device->DrawText(0xBE, 0x20, 0xFFB09B79, Buffer, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawTexture(0xE8, 0x1E, 0xF, 0xF, 0, 0, 0xF, 0xF, ToolTipBtnTex, 0xFF, 1);

    BYTE* Category = ::nw::Field<BYTE*>(::nw::Field<BYTE*>(TreeCtrl, 0x114), 0x204);
    if (Category)
    {
        INT Num = ::nw::Field<INT>(Category, 0x208C);
        if (Num > 0)
        {
            BYTE* Quest = ::nw::Field<BYTE**>(Category, 0x2088)[Num - 1];
            if (Quest)
            {
                for (INT i = 0; i < ::nw::Field<INT>(Quest, 0x1A0C); i++)
                {
                    INT* Held = &::nw::Field<INT>(Quest, 0x1DFC + i * 4);
                    *Held = 0;
                    for (INT j = 0; j < Counts.Num(); j++)
                    {
                        if (::nw::Field<INT>(Quest, 0x1D34 + i * 4) == Counts(j).ItemID)
                            *Held += Counts(j).Count;
                    }
                }
            }
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081780, NCGMQuestWnd, OnPaint)

// 0x10082870 (vslot 68, no argument) - the tree control's slot 72 (its list emptied), the counts and the number of
// quests cleared
void NCGMQuestWnd::ClearList()
{
    TreeCtrl->ResetScroll();
    Counts.Empty();
    NumQuests = 0;
}
NW_HOOK_METHOD(0x10082870, NCGMQuestWnd, ClearList)

NW_CHECK_VTABLE(NCGMQuestWnd, 0x10188c30, 71)

/*-----------------------------------------------------------------------------
    NCGMPledgeWnd
-----------------------------------------------------------------------------*/

// 0x10085100 - NCPledgeWnd's constructor and the vtable
NCGMPledgeWnd::NCGMPledgeWnd()
{
}
NW_HOOK_CTOR(0x10085100, NCGMPledgeWnd)

// 0x10085150
NW_HOOK_DTOR(0x10085150, NCGMPledgeWnd)

// 0x1007e3a0 (vslot 25) - the base's answer as a plain 0 / 1
INT NCGMPledgeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCPledgeWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e3a0, NCGMPledgeWnd, WindowProc)

// 0x1007e3c0 (vslot 38) - C3 guards it
INT NCGMPledgeWnd::OnDestroy()
{
    guard(NCGMPledgeWnd::OnDestroy);
    NCPledgeWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007e3c0, NCGMPledgeWnd, OnDestroy)

// 0x10081a40 (vslot 56) - the player's page (0x10047bc0) as the C2 client drew it: no crest, the clan's name (when it
// has one), the leader, the status text after the leader's name, the level, the castle and the hideout as the two
// texts of the stock sysstring (no ids: the GM's record has none)
INT NCGMPledgeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCGMPledgeWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 256, 335, 0, 0, 256, 335, BackTex, 0xFF, 1);

    SIZE Size;
    const TCHAR* ClanName = Info.Name;
    if (ClanName && *ClanName)
    {
        NCGDevice::GetTextExtent(&Size, ClanName, 0, 0, 0, 0, 0);  // not used
        Device->DrawText(19, 10, CLAN_TEXT_COLOR, ClanName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    if (*SysString(STR_Leader))
        Device->DrawText(19, 26, CLAN_LABEL_COLOR, *SysString(STR_Leader), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Leader = Info.LeaderName;
    if (Leader && *Leader)
        Device->DrawText(61, 26, CLAN_TEXT_COLOR, Leader, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* StatusText = Info.StatusText;
    if (StatusText && *StatusText)
    {
        NCGDevice::GetTextExtent(&Size, Leader, 0, 0, 0, 0, 0);
        Device->DrawText(Size.cx + 49, 26, CLAN_TEXT_COLOR, StatusText, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (*SysString(STR_Lv))
        Device->DrawText(192, 26, CLAN_LABEL_COLOR, *SysString(STR_Lv), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    TCHAR Number[10];
    appSprintf(Number, TEXT("%d"), Info.Level);
    Device->DrawText(208, 26, CLAN_TEXT_COLOR, Number, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    if (*SysString(STR_Base))
        Device->DrawText(19, 42, CLAN_LABEL_COLOR, *SysString(STR_Base), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Info.bHasCastle)
    {
        Device->DrawText(61, 42, CLAN_TEXT_COLOR, *SysString(STR_CastleExists), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         0);
        if (Info.bHasHideout)
        {
            NCGDevice::GetTextExtent(&Size, *SysString(STR_CastleExists), 0, 0, 0, 0, 0);
            Device->DrawText(Size.cx + 63, 42, CLAN_TEXT_COLOR, *SysString(STR_HideoutExists), 0, 0, 0, L2FONT_Normal,
                             0, 0, 0, 0, 0, 0);
        }
    }
    else if (Info.bHasHideout)
        Device->DrawText(61, 42, CLAN_TEXT_COLOR, *SysString(STR_HideoutExists), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081a40, NCGMPledgeWnd, OnPaint)

NW_CHECK_VTABLE(NCGMPledgeWnd, 0x10188e78, 82)

/*-----------------------------------------------------------------------------
    NCGMActionWnd
-----------------------------------------------------------------------------*/

// the clan page's buttons the GM window disables (repeated in OnCreate and ShowWindow): the title, the crest, the war,
// the truce, the surrender, the expel, the yield, the leave and the invite buttons, and the first new one
static FORCEINLINE void DisablePledgeButtons(NCPledgeWnd* Pledge)
{
    Pledge->TitleButton->EnableWindow(0);
    Pledge->CrestButton->EnableWindow(0);
    Pledge->WarButton->EnableWindow(0);
    Pledge->TruceButton->EnableWindow(0);
    Pledge->SurrenderButton->EnableWindow(0);
    Pledge->ExpelButton->EnableWindow(0);
    Pledge->YieldButton->EnableWindow(0);
    Pledge->LeaveButton->EnableWindow(0);
    Pledge->InviteButton->EnableWindow(0);
    Pledge->NewButton6F4->EnableWindow(0);
}

// 0x100851a0 - NCWnd's constructor and the vtable
NCGMActionWnd::NCGMActionWnd()
{
}
NW_HOOK_CTOR(0x100851a0, NCGMActionWnd)

// 0x100851f0
NW_HOOK_DTOR(0x100851f0, NCGMActionWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCGMActionWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCGMActionWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x10085240 (vslot 37) - the title bar (20 high) and the clan page under it, hidden; its buttons disabled but the
// last new one (the privileges' viewer), which is enabled with the text 883
INT NCGMActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_GMACTION_FRAME;
    FrameCtrl->SetPosition(0, 0, 256, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    PledgeWnd = new NCGMPledgeWnd;
    PledgeWnd->SetPosition(0, 20, 256, 335);
    CreateChild(PledgeWnd, NWS_VISIBLE);
    PledgeWnd->HideWindow();
    DisablePledgeButtons(PledgeWnd);
    PledgeWnd->NewButton6F8->EnableWindow(1);
    PledgeWnd->NewButton6F8->SetTextID(883);
    return 1;
}
NW_HOOK_METHOD(0x10085240, NCGMActionWnd, OnCreate)

// 0x10081e60 (vslot 38)
INT NCGMActionWnd::OnDestroy()
{
    guard(NCGMActionWnd::OnDestroy);
    DestroyChild(this, PledgeWnd);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081e60, NCGMActionWnd, OnDestroy)

// 0x1007e440 (vslot 44) - shown, the clan page's buttons are disabled again (the page enables the ones its
// privileges allow; C3: twelve, in another order)
void NCGMActionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    DisablePledgeButtons(PledgeWnd);
    PledgeWnd->Button714->EnableWindow(0);
    PledgeWnd->Button72C->EnableWindow(0);
}
NW_HOOK_METHOD(0x1007e440, NCGMActionWnd, ShowWindow)

// 0x10081e00 (vslot 56) - draws nothing but the clip and the origin; also NCSkillWnd's and NCGMSkillWnd's
INT NCGMActionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10081e00, NCGMActionWnd, OnPaint)

NW_CHECK_VTABLE(NCGMActionWnd, 0x10188fc0, 68)
