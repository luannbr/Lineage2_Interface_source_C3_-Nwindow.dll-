/*=============================================================================
    NTemplates.cpp - the template code the stock module keeps out of line: TArray methods
    for the element types of the windows (UnTemplate.h, with its check()s), UE2's Cast and
    TMap's Rehash, FString::Len. Each stock instance is hooked to ours through a typedef;
    instances that differ only in their exception tables (one per class that uses them)
    all come here. Stock: 0x10003a10, 0x10003bb0, 0x100039f0, 0x10004ce0, 0x10017620,
    0x10017eb0, 0x10018460, 0x10019880, 0x1001a010, 0x1001b380, 0x10041b20, 0x100424e0,
    0x1005fce0, 0x1006e700, 0x10070b40, 0x100745e0, 0x10075bc0, 0x10078bb0, 0x100792e0,
    0x1007a760, 0x100887e0, 0x10088800, 0x100a1660, 0x100ad570, 0x100b36e0, 0x100bcaa0,
    0x10022400, 0x10022980, 0x100259f0, 0x10028700, 0x10028a90, 0x10029fc0, 0x10036e70,
    0x1003cb40, and the console's 0x1004dd20, 0x1004dda0, 0x10055660, 0x100556f0, 0x10055780,
    0x10045e60.
=============================================================================*/
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCCandidateBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NControl.h"
#include "nwindow/NString.h"
#include "nwindow/NConsoleFns.h"

/*-----------------------------------------------------------------------------
    TArray of pointers: one out-of-line Remove, copy constructor and new(Array) for all of
    them; destructors per class.
-----------------------------------------------------------------------------*/

typedef TArray<void*> TPtrArray;
typedef TArray<NListRow*> TListRowArray;
typedef TArray<NItemEntry*> TItemEntryArray;
typedef TArray<NString*> TStringPtrArray;
struct NCoolTimeSlot { NCCoolTimeIcon* Icon; INT Unknown4; };   // C3: 8-byte elements (stock Remove(0, n, 8))
typedef TArray<NCoolTimeSlot> TCoolTimeArray;
typedef TArray<NVIPNeededItem*> TVIPNeededItemArray;
typedef TArray<NSiegeClanInfo*> TSiegeClanArray;
typedef TArray<NTrainEntry*> TTrainEntryArray;
typedef TArray<AActor*> TActorArray;

// 0x10131560 (cdecl) - UE2's `new(Array) T*`: a place for one more pointer at the end of the array, its address (the
// size is ignored)
static void* NewPtrArrayElement(size_t Size, TPtrArray& Array)
{
    INT Index = Array.Add();
    return &((void**)Array.GetData())[Index];
}
NW_HOOK_FUNC(0x10131560, NewPtrArrayElement)

// 0x100c6cf0 (ret 4) - the pointer array emptied to Slack places (the instance of the 4-byte elements)
NW_HOOK_METHOD(0x100c6cf0, TPtrArray, Empty)
// 0x10075630 - the default constructor (three zeros; the dynamic initializers of the cool time texture arrays and the
// constructors of NCWnd / NConsoleWnd call it)
NW_HOOK_CTOR(0x10075630, TPtrArray)
// 0x10030b70 (ret 4) - the index of the first element equal to the one the argument points to, -1 when there is none
NW_HOOK_METHOD(0x10030b70, TPtrArray, FindItemIndex)
// 0x100370d0 (ret 8) - Remove(Index, Count) with the three checks of UnTemplate.h (515 .. 517)
NW_HOOK_METHOD(0x100370d0, TPtrArray, Remove)
// 0x100318e0 (ret 4) - RemoveItem: every element equal to the one the argument points to goes (through the Remove above),
// the number removed comes back (NCTVItem's children, the focus stack)
NW_HOOK_METHOD(0x100318e0, TPtrArray, RemoveItem)
// 0x1011b200 (ret 4) - AddItem: the element appended, its index back
NW_HOOK_METHOD(0x1011b200, TPtrArray, AddItem)

// 0x10020af0 - NCListCtrl's rows and columns, NCMatchWnd's copy of them
NW_HOOK_DTOR(0x10020af0, TListRowArray)
// 0x1001d7a0 - NCItemWnd's, NCEquipItemWnd's and NCShortCutWnd's items
NW_HOOK_DTOR(0x1001d7a0, TItemEntryArray)
// 0x10005a90 - NCComboBox's items, NCChatWnd's tell names, NCPawnCreateWnd's (NCSelectCtrl)
NW_HOOK_DTOR(0x10005a90, TStringPtrArray)
// 0x100835b0 - NCSkillItemWnd's cool times
NW_HOOK_DTOR(0x100835b0, TCoolTimeArray)
// 0x10096cf0 - the VIP shop's lists
NW_HOOK_DTOR(0x10096cf0, TVIPNeededItemArray)
// 0x100f9750 - NCSiegePartInfoWnd's clans
NW_HOOK_DTOR(0x100f9750, TSiegeClanArray)


typedef TArray<NCRadioButton*> TRadioButtonArray;
typedef TArray<NWndEntry*> TWndEntryArray;
typedef TArray<NControl*> TControlArray;
typedef TArray<NActionItem*> TActionItemArray;

NW_HOOK_DTOR(0x1002aa20, TRadioButtonArray)
// 0x10075620 - a tail jump into the destructor above (the unwind code of NConsoleWnd's constructor and destructor)
NW_HOOK_DTOR(0x10075620, TRadioButtonArray)
// 0x1002a430 (ret 4) - the copy constructor (the stock instance: the count, Realloc, then the elements one by one through
// 0x10131560); NCTabControl::GetTabs hands the tab buttons' array out as a copy
NW_HOOK_CTOR_ARGS(0x1002a430, TRadioButtonArray, (const TRadioButtonArray& Other), (Other))

// 0x100330f0 (ret 4) - Last(c) of those lists with its check (UnTemplate.h 408; the TArray of include/ue2 has none): the
// element c places from the end, by reference. DispatchWndMsg takes the top of the focus stack with it. The path is
// the one of this instance's assert (d:\work\2005_09_14\...).
struct TWndEntryArrayLast : public TWndEntryArray
{
    NWndEntry*& LastChecked(INT c = 0)
    {
        NW_CHECK_AT(c<ArrayNum, "d:\\work\\2005_09_14\\warfare2110\\core\\inc\\UnTemplate.h", 408);
        return ((NWndEntry**)Data)[ArrayNum - c - 1];
    }
    // UnTemplate.h's RemoveItem (the stock keeps the instance out of line): every element equal to Item goes, the
    // count removed comes back
    INT RemoveItem(NWndEntry* const& Item)
    {
        INT Original = ArrayNum;
        for (INT Index = 0; Index < ArrayNum; Index++)
            if (((NWndEntry**)Data)[Index] == Item)
                Remove(Index--);
        return Original - ArrayNum;
    }
};
NW_HOOK_METHOD(0x100330f0, TWndEntryArrayLast, LastChecked)
// 0x10037ec0 - NCHtmlViewer's controls and the page tree's children
NW_HOOK_DTOR(0x10037ec0, TControlArray)
// 0x1004a440 - NCActionItemWnd's icons
NW_HOOK_DTOR(0x1004a440, TActionItemArray)
// 0x100502f0 - a jump to 0x10004ce0 (NCChatWnd's unwind code)
NW_HOOK_DTOR(0x100502f0, TStringPtrArray)
// 0x100ddb00 - NCQuestInfoWnd's list. The stock instance removes with an element size of 4: an array of pointers
// (NCQuestInfoWnd.h still declares TArray<NQuestEntry>, the C1 layout, whose destructor makes the same calls)
NW_HOOK_DTOR(0x100ddb00, TPtrArray)
// 0x10083640 - the same instance with another unwind table: NCEquipItemWnd's, NCSkillItemWnd's, NCGMQuestWnd's and
// NCQuestGaraTreeWnd's lists
NW_HOOK_DTOR(0x10083640, TPtrArray)
// 0x1012cd40 - and the messenger's friend lists' (its unwind table; a jump from two more)
NW_HOOK_DTOR(0x1012cd40, TPtrArray)

/*-----------------------------------------------------------------------------
    TArray of structures.
-----------------------------------------------------------------------------*/

typedef TArray<NListItem> TListItemArray;
typedef TArray<FMinimapTarget> TMinimapTargetArray;
typedef TArray<NQuestEntry> TQuestEntryArray;
typedef TArray<FAbnormalStatus> TAbnormalStatusArray;
typedef TArray<NImeText> TImeTextArray;
typedef TArray<NChatBalloon> TChatBalloonArray;
typedef TArray<FName> TNameArray;

NW_HOOK_DTOR(0x1001f130, TListItemArray)
// 0x100040f0, 0x10003a10, 0x10003bb0 - the IMEs' clauses and the candidate box's items
NW_HOOK_METHOD(0x100040f0, TImeTextArray, Empty)
NW_HOOK_METHOD(0x10004110, TImeTextArray, AddItem)
NW_HOOK_DTOR(0x10004290, TImeTextArray)
// 0x100556e0, 0x100424e0 - NCConsole's chat balloons (each one's text pointer cleared)
NW_HOOK_METHOD(0x100556e0, TChatBalloonArray, Remove)
NW_HOOK_DTOR(0x10056630, TChatBalloonArray)

/*-----------------------------------------------------------------------------
    TArray<INT>: the HTML alignment stacks.
-----------------------------------------------------------------------------*/

typedef TArray<INT> TIntArray;

// 0x10038090 - a jump to the pointer arrays' destructor 0x10028700 (the stacks' atexit code)
NW_HOOK_DTOR(0x10038090, TIntArray)
// 0x10038b60 - Pop (the stock one calls its out-of-line Remove)
NW_HOOK_METHOD(0x10038b60, TIntArray, Pop)
// 0x100231d0 - the destructor instance of the arrays of 4-byte elements the windows keep (NCMultiEdit's IntList,
// NCInventoryWnd's, NCSiegeDateInfoWnd's, NToolTipInfo::Fill's and NCQuestInfoWnd's Steps)
NW_HOOK_DTOR(0x100231d0, TIntArray)

/*-----------------------------------------------------------------------------
    More destructor instances: one per class that keeps the array (the stock compiler emitted each one again).
-----------------------------------------------------------------------------*/

typedef TArray<ThaiCharacter> TThaiCharArray;
typedef TArray<NChatLimiterItem> TChatLimiterItemArray;

// 0x1000e480 - the Thai clusters of the edit boxes and of the device (20-byte elements)
NW_HOOK_DTOR(0x1000e480, TThaiCharArray)
// 0x1009f8f0 - the macro window's, the macro grid's and the macro editor's lists of pointers
NW_HOOK_DTOR(0x1009f8f0, TPtrArray)
// 0x100e12a0 - NCRecipeListBox's entries
NW_HOOK_DTOR(0x100e12a0, TPtrArray)
// 0x10088220 - NCHennaListBox's entries
NW_HOOK_DTOR(0x10088220, TPtrArray)
// 0x10109160 - NCTrainListBox's entries
NW_HOOK_DTOR(0x10109160, TTrainEntryArray)
// 0x10050210 - the chat flood control's lines (8-byte elements)
NW_HOOK_DTOR(0x10050210, TChatLimiterItemArray)

/*-----------------------------------------------------------------------------
    Cast, TMap, FString.
-----------------------------------------------------------------------------*/



// 0x10022780 - the multi-line edit box's line lengths
NW_HOOK_METHOD(0x10022780, FString, Len)

/*-----------------------------------------------------------------------------
    The console's.
-----------------------------------------------------------------------------*/

typedef TArray<NTimer> TTimerArray;
typedef TArray<NSysMsgParam> TSysMsgParamArray;

// 0x1006ba10 - Tick drops the killed timers
NW_HOOK_METHOD(0x1006ba10, TTimerArray, Remove)
// 0x10075590, 0x10055660 - the console's timers and system message parameters (for
// ~NConsoleWnd and the unwinding)
NW_HOOK_DTOR(0x10075590, TTimerArray)
NW_HOOK_DTOR(0x10075500, TSysMsgParamArray)

// TMap's iterator (UnTemplate.h): the map, its pairs, the index, the count removed while
// walking; with any removed, its destructor relaxes the hash (TMapBase::Relax, inline)
template <class T> struct NDataMapIterator
{
    NDataMap<T>& Map;
    TArray<typename NDataMap<T>::NPair>& Pairs;
    INT Index;
    INT Removed;

    NDataMapIterator(NDataMap<T>& InMap) : Map(InMap), Pairs(InMap.Pairs), Index(0), Removed(0) {}
    ~NDataMapIterator()
    {
        if (Removed)
        {
            while (Map.HashCount > Map.Pairs.Num() * 2 + 8)
                Map.HashCount /= 2;
            Map.Rehash();
        }
    }
};
typedef NDataMapIterator<NViewItemData> TViewItemMapIterator;


typedef NDataMap<NViewItemData> TViewItemMap;

// 0x100626e0 - TMapBase::Rehash (the one instance of the stock build: 12-byte pairs, 4-byte keys, the value a pointer; the
// item, NPC and skill tables, the messenger's maps ... all call it): the hash chains rebuilt in a new table of
// HashCount entries. NCGMWnd.cpp's map iterator calls it through the stock address
NW_HOOK_METHOD(0x100626e0, TViewItemMap, Rehash)


/*-----------------------------------------------------------------------------
    NCComboBox's item with a data word (NString::Color), and the small forwarders the windows share (names ours).
-----------------------------------------------------------------------------*/

// Stand-ins for the classes the stock functions belong to (the names of the owners are not in the headers): the functions
// are members of them, so that they are hooked as methods (thiscall, `this` in ecx) and not as free functions

// NCComboBox's items with a data word (NString::Color): NComboItemFns (nwindow/NConsoleFns.h)

// the data word the item was added with, -1 for an index outside the list
INT NComboItemFns::GetItemData(INT Index)
{
    if (Index < 0 || Index >= Items.Num())
        return -1;
    return Items(Index)->Color;
}
NW_HOOK_METHOD(0x100054d0, NComboItemFns, GetItemData)

// an item of Text (nothing when it is NULL or empty) whose data word is Data (the character lists of the pawn viewer and
// the party matching use it)
void NComboItemFns::AddItemData(const TCHAR* Text, INT Data)
{
    if (Text && *Text)
    {
        NString* Item = new NString(Text);
        Item->Color = Data;
        Items.AddItem(Item);
    }
}
NW_HOOK_METHOD(0x10005a00, NComboItemFns, AddItemData)

// the tail-jump forwarders (no arguments of their own except the console's): `this` is a window whose child at +0xCC has
// its virtual 72 / 71 called (NCPetWnd::ClearInventory, NConsoleWnd's slot 288, NCSSQMainBoard::HideWindow /
// NCTrainWnd::Reset), and the console, whose engine's network handler gets RequestListPartyWating (+0x430)
struct NForwardFns : public NCWnd
{
    INT ChildSlot72();                                               // 0x100ea340
    INT ChildSlot71();                                               // 0x101074f0
};
INT NForwardFns::ChildSlot72()
{
    return ::nw::VirtualCall<INT>(*(void**)((BYTE*)this + 0xCC), 72);
}
NW_HOOK_METHOD(0x100ea340, NForwardFns, ChildSlot72)

INT NForwardFns::ChildSlot71()
{
    return ::nw::VirtualCall<INT>(*(void**)((BYTE*)this + 0xCC), 71);
}
NW_HOOK_METHOD(0x101074f0, NForwardFns, ChildSlot71)

// NConsoleForwardFns: nwindow/NConsoleFns.h
void NConsoleForwardFns::RequestListPartyWating(L2ParamStack& Params)
{
    Engine->NetHandler->RequestListPartyWating(Params);
}
NW_HOOK_METHOD(0x10061eb0, NConsoleForwardFns, RequestListPartyWating)

// the party room's requests of C3: tail jumps into the network handler's slots 269 .. 274 (the party room windows and the
// matching list call them with their parameter stack)
void NConsoleForwardFns::RequestManagePartyRoom(L2ParamStack& Params)
{
    Engine->NetHandler->RequestManagePartyRoom(Params);
}
NW_HOOK_METHOD(0x10061ec0, NConsoleForwardFns, RequestManagePartyRoom)

void NConsoleForwardFns::RequestJoinPartyRoom(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinPartyRoom(Params);
}
NW_HOOK_METHOD(0x10061ed0, NConsoleForwardFns, RequestJoinPartyRoom)

void NConsoleForwardFns::RequestOustFromPartyRoom(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustFromPartyRoom(Params);
}
NW_HOOK_METHOD(0x10061ee0, NConsoleForwardFns, RequestOustFromPartyRoom)

void NConsoleForwardFns::RequestDismissPartyRoom(L2ParamStack& Params)
{
    Engine->NetHandler->RequestDismissPartyRoom(Params);
}
NW_HOOK_METHOD(0x10061ef0, NConsoleForwardFns, RequestDismissPartyRoom)

void NConsoleForwardFns::RequestWithdrawPartyRoom(L2ParamStack& Params)
{
    Engine->NetHandler->RequestWithdrawPartyRoom(Params);
}
NW_HOOK_METHOD(0x10061f00, NConsoleForwardFns, RequestWithdrawPartyRoom)

void NConsoleForwardFns::RequestHandOverPartyMaster(L2ParamStack& Params)
{
    Engine->NetHandler->RequestHandOverPartyMaster(Params);
}
NW_HOOK_METHOD(0x10061ea0, NConsoleForwardFns, RequestHandOverPartyMaster)

/*-----------------------------------------------------------------------------
    FL2SystemMsgData (the engine's record of systemmsg-*.dat, 0x30 bytes in C3): the out-of-line copy constructor and
    destructor of its three strings (+4, +0x18, +0x24; the header of include/ue2 knows only two of them).
-----------------------------------------------------------------------------*/

struct NSystemMsgFns : public FL2SystemMsgData
{
    FL2SystemMsgData* CopyFrom(const FL2SystemMsgData* Src);         // 0x1005ed40 (the copy constructor, ret 4)
    void DestroyStrings();                                           // 0x1005edc0 (the destructor)
};

// the copy: the id, the message, the two words, the second message and the third string
FL2SystemMsgData* NSystemMsgFns::CopyFrom(const FL2SystemMsgData* Src)
{
    *(INT*)this = *(const INT*)Src;
    new ((BYTE*)this + 4) FString(*(const FString*)((const BYTE*)Src + 4));
    Type = Src->Type;
    Unknown14 = Src->Unknown14;
    new ((BYTE*)this + 0x18) FString(*(const FString*)((const BYTE*)Src + 0x18));
    new ((BYTE*)this + 0x24) FString(*(const FString*)((const BYTE*)Src + 0x24));
    return this;
}
NW_HOOK_METHOD(0x1005ed40, NSystemMsgFns, CopyFrom)

// the three strings, last first
void NSystemMsgFns::DestroyStrings()
{
    ((FString*)((BYTE*)this + 0x24))->~FString();
    ((FString*)((BYTE*)this + 0x18))->~FString();
    ((FString*)((BYTE*)this + 4))->~FString();
}
NW_HOOK_METHOD(0x1005edc0, NSystemMsgFns, DestroyStrings)
