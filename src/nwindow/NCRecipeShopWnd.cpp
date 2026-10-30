/*=============================================================================
    NCRecipeShopWnd.cpp - the dwarven recipe shop's manage window (new in C2) and its title bar.
    Stock (C3): 0x100DE920 (the title bar's close), 0x100DE940 .. 0x100DF3B3 (WindowProc, the cancel / quit handlers,
    paint, destroy, start, add product, message), 0x100E0920 .. 0x100E0960 (the grids' forwarders), 0x100E1640 /
    0x100E1690 (the title bar's constructor, destructor), 0x100E17C0 .. 0x100E1850 (constructor, destructor), 0x100E18A0
    (OnCreate), 0x101059E0 (SetTitle).
=============================================================================*/
#include "nwindow/NCRecipeShopWnd.h"
#include "nwindow/NCRecipeShopItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NClasses.gen.h"

enum { DIALOG_RECIPE_PRICE = 0x23 };    // NDialogInfo::ID (see NCRecipeShopItemWnd.cpp)
enum { DIALOG_RECIPE_MESSAGE = 0x24 };  // NDialogInfo::ID (name ours): the shop's message (text); the dialog box answers it
enum { MSG_PriceToBuy = 0x142 };
enum { MSG_ShopMessage = 0x14e };       // systemmsg line the message box shows
enum { SOUND_Close = 6 };               // NConsoleWnd::PlayInterfaceSound (the window closing)

// sysstring labels (SysString index = (stock offset - 0x65A8C) / 12)
enum
{
    STR_RecipeList = 643,               // 0x678B0: the upper grid's label at (11, 32)
    STR_ProductionList = 661,           // 0x67988: the lower one's at (11, 198)
    STR_StartButton = 0x1AC,
    STR_MessageButton = 0x180,
    STR_QuitButton = 0x181,
};

// a child window goes (repeated inline in the stock OnDestroy)
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
    The title bar.
-----------------------------------------------------------------------------*/

// 0x100e1640 - the frame's default constructor, then the class's vtable
NCRecipeShopFrameCtrl::NCRecipeShopFrameCtrl()
{
}
NW_HOOK_CTOR(0x100e1640, NCRecipeShopFrameCtrl)

// 0x100e1690
NW_HOOK_DTOR(0x100e1690, NCRecipeShopFrameCtrl)

// 0x1000f900 (vslot 25, ret 0xC): the stock body is NCResizeFrame's (NCFrameCtrl's with the result as 0 / 1, hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeShopFrameCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x1000f900)
#else
NW_SLOT_IS("?WindowProc@NCRecipeShopFrameCtrl@@UAEHIIJ@Z", "?WindowProc@NCResizeFrame@@UAEHIIJ@Z")   // NCRecipeShopFrameCtrl::WindowProc = NCResizeFrame::WindowProc (0x1000f900)
#endif

// 0x100de920 (vslot 70) - the server is told the shop setup is left, then the frame's close (a tail call, its result is kept)
INT NCRecipeShopFrameCtrl::OnClose()
{
    GConsole->RequestRecipeShopManageQuit();
    return NCFrameCtrl::OnClose();
}
NW_HOOK_METHOD(0x100de920, NCRecipeShopFrameCtrl, OnClose)

NW_CHECK_VTABLE(NCRecipeShopFrameCtrl, 0x101994f0, 72)

/*-----------------------------------------------------------------------------
    The window.
-----------------------------------------------------------------------------*/

// 0x100e17c0 - the grids, the buttons and the two spare numbers cleared (+0xC8 .. +0xF4)
NCRecipeShopWnd::NCRecipeShopWnd()
{
    RecipeItems = NULL;
    ProductionItems = NULL;
    Unknown0A0 = 0;
    BackTex = NULL;
    Unknown0A8 = 0;
    UpButton = NULL;
    DownButton = NULL;
    StartButton = NULL;
    MessageButton = NULL;
    QuitButton = NULL;
    Unknown0C0 = 0;
    Unknown0C4 = 0;
}
NW_HOOK_CTOR(0x100e17c0, NCRecipeShopWnd)

// 0x100e1850
NW_HOOK_DTOR(0x100e1850, NCRecipeShopWnd)

// 0x100dea30 - both grids emptied (the stock code tail-calls the second Clear)
__declspec(noinline) void NCRecipeShopWnd::ClearLists()
{
    RecipeItems->Clear();
    ProductionItems->Clear();
}
NW_HOOK_METHOD(0x100dea30, NCRecipeShopWnd, ClearLists)

// 0x100de940 (vslot 25) - the buttons' handlers
INT NCRecipeShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_RECIPESHOP_START)
            return OnStart() != 0;
        if (wParam == NID_RECIPESHOP_UP)
            return OnRemoveProduct() != 0;
        if (wParam == NID_RECIPESHOP_DOWN)
            return OnAddProduct() != 0;
        if (wParam == NID_RECIPESHOP_MESSAGE)
            return OnMessage() != 0;
        if (wParam == NID_RECIPESHOP_QUIT)
            return OnQuit() != 0;
        if (wParam == NID_RECIPESHOP_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100de940, NCRecipeShopWnd, WindowProc)

// 0x100dea50 (vslot 45) - closed with the grids emptied
void NCRecipeShopWnd::HideWindow()
{
    ClearLists();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100dea50, NCRecipeShopWnd, HideWindow)

// 0x100dea60 (vslot 71) - the server is told the setup is cancelled, the window closes emptied (C3: with the sound)
INT NCRecipeShopWnd::OnCancel()
{
    GConsole->RequestRecipeShopManageCancel();
    ClearLists();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100dea60, NCRecipeShopWnd, OnCancel)

// 0x100deaa0 (vslot 72) - the same for the close button: the server is told the shop is left
INT NCRecipeShopWnd::OnQuit()
{
    GConsole->RequestRecipeShopManageQuit();
    ClearLists();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100deaa0, NCRecipeShopWnd, OnQuit)

// 0x100dee60 (vslot 56) - the background and the two grid labels (C3: DrawTexture, lighter text)
INT NCRecipeShopWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, 256, 381, 0, 0, 256, 381, BackTex, 0xFF, 1);
    Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_RecipeList), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ProductionList), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100dee60, NCRecipeShopWnd, OnPaint)

// 0x100def70 (vslot 38)
INT NCRecipeShopWnd::OnDestroy()
{
    DestroyChild(this, ShopFrame);
    DestroyChild(this, RecipeItems);
    DestroyChild(this, ProductionItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, StartButton);
    DestroyChild(this, QuitButton);
    DestroyChild(this, MessageButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100def70, NCRecipeShopWnd, OnDestroy)

// 0x100df0f0 (vslot 70) - Start: the shop's list goes to the server (the number of entries, then the recipe's object id
// and the price of each; the parameter stack is made for three values an entry), the window closes emptied
INT NCRecipeShopWnd::OnStart()
{
    INT Num = ProductionItems->Items.Num();
    L2ParamStack Params(Num * 3 + 1);
    Params.PushBack((void*)(INT_PTR)Num);
    for (INT i = 0; i < ProductionItems->Items.Num(); i++)
    {
        Params.PushBack((void*)(INT_PTR)ProductionItems->Items(i)->ObjectID);
        Params.PushBack((void*)(INT_PTR)ProductionItems->Items(i)->UnknownC20);
    }
    GConsole->RequestRecipeShopListSet(Params);
    ClearLists();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100df0f0, NCRecipeShopWnd, OnStart)

// 0x100df1e0 (vslot 69) - the down arrow: the price of the selected recipe is asked (the answer is the upper grid's
// OnDialogResult, which moves the recipe to the lower grid)
INT NCRecipeShopWnd::OnAddProduct()
{
    INT Sel = RecipeItems->GetSelected();
    if (Sel >= 0 && Sel < RecipeItems->Items.Num())
    {
        NItemEntry* Item = RecipeItems->Items(Sel);
        if (Item)
        {
            TCHAR Name[64];
            appSprintf(Name, TEXT("%s"), Item->GetName());
            NDialogInfo Info;
            Info.ID = DIALOG_RECIPE_PRICE;
            Info.Type = 3;
            Info.Owner = RecipeItems->Entry;
            Info.bMainBox = 1;
            Info.bInput = 1;
            Info.SetSystemMessage(MSG_PriceToBuy);
            ::nw::Field<INT>(&Info, 0x18) = Item->ID;   // C3: the recipe's id (the header names the field bProgress)
            GConsole->ShowDialog(&Info);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100df1e0, NCRecipeShopWnd, OnAddProduct)

// 0x100df2f0 (vslot 73) - Message: a text box with the shop's message so far (the player's pawn +0x1634), no owner: the
// dialog box answers it by its ID
INT NCRecipeShopWnd::OnMessage()
{
    NDialogInfo Info;
    Info.ID = DIALOG_RECIPE_MESSAGE;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MSG_ShopMessage);
    Info.Type = 0;
    const TCHAR* Message = NULL;
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        APawn* MyPawn = Me->Pawn;
        if (MyPawn)
            Message = *nw::Field<FString>(MyPawn, 0x1634);   // (name ours) the shop message
    }
    Info.SetText60(Message);
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x100df2f0, NCRecipeShopWnd, OnMessage)

// 0x100e0940 (vslot 68) - the up arrow: the selected entry of the lower grid is taken off it
INT NCRecipeShopWnd::OnRemoveProduct()
{
    INT Sel = ProductionItems->GetSelected();
    if (Sel >= 0 && Sel < ProductionItems->Items.Num())
        ProductionItems->Items.Remove(Sel, 1);
    return 1;
}
NW_HOOK_METHOD(0x100e0940, NCRecipeShopWnd, OnRemoveProduct)

// 0x100e0930 - what the console forwards (stock: the upper grid's AddItem as a tail jump)
UBOOL NCRecipeShopWnd::AddRecipeItem(NItemInfo* Info)
{
    return RecipeItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100e0930, NCRecipeShopWnd, AddRecipeItem)

// 0x100e0920 - the same for the lower grid
UBOOL NCRecipeShopWnd::AddProductionItem(NItemInfo* Info)
{
    return ProductionItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100e0920, NCRecipeShopWnd, AddProductionItem)

// 0x101059e0 (vslot 51, ret 4) - the title goes to the title bar when there is one and a text
void NCRecipeShopWnd::SetTitle(const TCHAR* Title)
{
    if (ShopFrame != NULL && Title != NULL && *Title != 0)
        ShopFrame->SetTitleText(Title);
}
NW_HOOK_METHOD(0x101059e0, NCRecipeShopWnd, SetTitle)

// 0x100e18a0 (vslot 37) - the background, the title bar (id 0xF3), the two grids (6 columns, 4 rows, 160 entries of 32
// pixels, gaps 5 and 3; the upper one takes the recipes back, the lower one asks the price), the arrows and the
// Quit / Message / Start buttons
INT NCRecipeShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.RecipeWnd.Recipe1_Back"));

    ShopFrame = new NCRecipeShopFrameCtrl;
    FrameID = NID_RECIPESHOP_FRAME;
    ShopFrame->ID = NID_RECIPESHOP_FRAME;
    ShopFrame->SetPosition(0, 0, 256, 20);
    CreateChild(ShopFrame, NWS_VISIBLE);

    RecipeItems = new NCRecipeShopItemWnd(6, 4, 0xA0, 0x20, 5, 3);
    RecipeItems->SetPosition(9, 48, 239, 139);
    CreateChild(RecipeItems, NWS_VISIBLE | 0x20);
    RecipeItems->SetMode(1);

    ProductionItems = new NCRecipeShopItemWnd(6, 4, 0xA0, 0x20, 5, 3);
    ProductionItems->SetPosition(9, 215, 239, 139);
    CreateChild(ProductionItems, NWS_VISIBLE | 0x20);
    ProductionItems->SetMode(0);

    ProductionItems->SetTarget(RecipeItems);
    RecipeItems->SetTarget(ProductionItems);

    UpButton = new NCButton(TEXT("L2UI_CH3.ScrollBar.ScrollBarUpBtn"), TEXT("L2UI_CH3.ScrollBar.ScrollBarUpOnBtn"), 0);
    UpButton->ID = NID_RECIPESHOP_UP;
    UpButton->SetPosition(112, 194, 15, 15);
    CreateChild(UpButton, NWS_VISIBLE);

    DownButton = new NCButton(TEXT("L2UI_CH3.ScrollBar.ScrollBarDownBtn"), TEXT("L2UI_CH3.ScrollBar.ScrollBarDownOnBtn"), 0);
    DownButton->ID = NID_RECIPESHOP_DOWN;
    DownButton->SetPosition(130, 194, 15, 15);
    CreateChild(DownButton, NWS_VISIBLE);

    QuitButton = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    QuitButton->ID = NID_RECIPESHOP_QUIT;
    QuitButton->SetPosition(11, 372, 76, 23);
    CreateChild(QuitButton, NWS_VISIBLE);
    QuitButton->SetTextID(STR_QuitButton);

    MessageButton = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    MessageButton->ID = NID_RECIPESHOP_MESSAGE;
    MessageButton->SetPosition(91, 372, 76, 23);
    CreateChild(MessageButton, NWS_VISIBLE);
    MessageButton->SetTextID(STR_MessageButton);

    StartButton = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    StartButton->ID = NID_RECIPESHOP_START;
    StartButton->SetPosition(171, 372, 76, 23);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_StartButton);
    return 1;
}
NW_HOOK_METHOD(0x100e18a0, NCRecipeShopWnd, OnCreate)

NW_CHECK_VTABLE(NCRecipeShopWnd, 0x10199748, 74)
