#pragma once
// NCListCtrl - list with columns (UClass "CListCtrl", 0x24C): a header button per column
// (NCHeaderCtrl; a click sorts the rows by that column, the next click reverses the order),
// rows of cells under them (text, color, optional icon), scroll buttons and a slider on the
// right. The owner builds each row (NListRow, one NListCell per column) and hands it over
// with AddRow; from then on the list owns it. A double click reports the row (NWM_COMMAND,
// lParam = index); the selected row shows its tool tip while the mouse is over it.
// The server list of the login screen, the clan members, party matching and the siege
// lists derive from it (NCServerSelectListCtrl, NCMatchListCtrl, NCSiegeInfoListCtrl).
//
// C3 (read from the stock code, 0x1001f3c0 .. 0x10021540): the rows are 17 pixels high and every second one has a
// band texture under it, the selected row a two-part bar, the cells' text starts 10 pixels after the icon, the
// headers are drawn from caps and the tool tip is the window skin's.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPushButton;
class NCSliderBar;

// NWM_COMMAND id of the header buttons (NotifyID = column index)
enum { NID_LISTCTRL_HEADER = 0xF5 };

// A cell (name ours; 0x1824 bytes in C3, 0x1820 in C2, 0xA1C in C1: the texts got longer). What the list draws: Icon
// centered in the column, then Text after IconW + 10 pixels, in Color.
struct NListCell
{
    TCHAR Text[2048];          // +0x0000 (C1: 1024)
    UTexture* Icon;            // +0x1000
    DWORD Color;               // +0x1004 (C3: 0xFFDCDCDC; C2: white)
    TCHAR Text2[1024];         // +0x1008 not drawn by the list (the owners keep data here) (C1: 256)
    INT Param1;                // +0x1808 (-1) for the owner
    INT Param2;                // +0x180C (-1)
    INT Param3;                // +0x1810 new in C2 (SetParams' third argument; the constructor leaves it)
    INT UnknownA10;            // +0x1814 (0)
    INT IconW;                 // +0x1818
    INT IconH;                 // +0x181C
    UBOOL bTextVisible;        // +0x1820 (name ours) new in C3: 1 in the row's constructor; OnPaint draws the text
                               //   only while it is set

    // inlined in NListRow's constructor (0x10020b80)
    NListCell()
    {
        Text[0] = 0;
        Icon = NULL;
        Color = 0xFFDCDCDC;
        Text2[0] = 0;
        Param1 = -1;
        Param2 = -1;
        UnknownA10 = 0;
        IconW = 0;
        IconH = 0;
        bTextVisible = 1;
    }
    void SetText(const TCHAR* InText);                   // 0x1001f3c0
    void SetNumber(INT Value);                           // 0x1001f3f0 "%d"
    void SetIcon(UTexture* InIcon, INT W, INT H);        // 0x1001f410 (NULL keeps the old one)
    void SetText2(const TCHAR* InText);                  // 0x1001f440
    void SetParams(INT InParam1, INT InParam2, INT InParam3 = 0); // 0x1001f470 (C1: two arguments)
    void SetColor(DWORD InColor);                        // 0x1001f4a0
};
NW_ASSERT_OFFSET(NListCell, Icon, 0x1000);
NW_ASSERT_OFFSET(NListCell, Text2, 0x1008);
NW_ASSERT_OFFSET(NListCell, Param1, 0x1808);
NW_ASSERT_OFFSET(NListCell, IconW, 0x1818);
NW_ASSERT_OFFSET(NListCell, bTextVisible, 0x1820);
NW_ASSERT_SIZE(NListCell, 0x1824);

// A row (name ours; 0x88C bytes in C3, 0x888 in C2, created by the owner with new).
struct NListRow
{
    NListCell* Cells;          // +0x000 one per column
    TCHAR Text[1024];          // +0x004 not drawn by the list
    INT Param1;                // +0x804 (-1) for the owner
    INT Param2;                // +0x808 (-1)
    INT Param3;                // +0x80C new in C2 (SetParams' third argument)
    NToolTipInfo ToolTip;      // +0x810 shown while the selected row is under the mouse (C1: TCHAR[1024]); the
                               //   constructor / destructor are NToolTipInfo's (0x100024f0 / 0x100025a0)

    NListRow(INT NumCells);                              // 0x10020b80 (ret 4)
    ~NListRow();                                         // 0x10020c60 (frees Cells, then the tool tip)
    void SetText(const TCHAR* InText);                   // 0x1001f680
    void SetParams(INT InParam1, INT InParam2, INT InParam3 = 0); // 0x1001f6b0 (C1: two arguments)
    const TCHAR* GetToolTip();                           // C2: no stock function (C1: NULL when the text is empty)
};
NW_ASSERT_OFFSET(NListRow, Text, 0x4);
NW_ASSERT_OFFSET(NListRow, Param1, 0x804);
NW_ASSERT_OFFSET(NListRow, ToolTip, 0x810);
NW_ASSERT_SIZE(NListRow, 0x88C);

// What a column is made of (name ours; 0x4814 bytes in C3 (0x1814 in C2), built on the owner's stack and handed to
// NCListCtrl::AddColumn). The header is drawn from three textures per state (left cap, middle, right cap): the three
// normal names, then the three pushed ones; the two last strings name one texture each for a header drawn whole
// (NCHeaderCtrl +0x1D4 / +0x1D8). The owners' stack code clears the first character of all eight strings.
struct NListColumnInfo
{
    INT Width;                 // +0x000
    INT Height;                // +0x004 of the header; the rows start under it
    TCHAR Title[1024];         // +0x008
    UBOOL bClickable;          // +0x808 (1) a click on the header sorts
    UBOOL bNumeric;            // +0x80C (0) sort by value (_wtol) instead of by text
    INT Unknown810;            // +0x810 (left as it was on the stack)
    TCHAR NormalTex[3][1024];  // +0x814, +0x1014, +0x1814 the normal state: left cap, middle, right cap
                               //   ("NWindow.BlackTexture" in the header when empty)
    TCHAR PushedTex[3][1024];  // +0x2014, +0x2814, +0x3014 the pushed state
    TCHAR WholeNormalTex[1024];  // +0x3814 (names ours) a texture for the whole normal header (67 x 15 texels), none
                                 //   when empty
    TCHAR WholePushedTex[1024];  // +0x4014 the same for the pushed state

    // inlined in the owners
    NListColumnInfo()
    {
        Title[0] = 0;
        bClickable = 1;
        bNumeric = 0;
        NormalTex[0][0] = 0;
        PushedTex[0][0] = 0;
        NormalTex[1][0] = 0;
        PushedTex[1][0] = 0;
        NormalTex[2][0] = 0;
        PushedTex[2][0] = 0;
        WholeNormalTex[0] = 0;
        WholePushedTex[0] = 0;
    }
    void SetTitleID(INT TextID);                         // 0x1001f4b0 sysstring line
    void SetTitle(const TCHAR* InTitle);                 // 0x1001f510
    // C3: Index 0, 1 or 2 picks the cap (any other Index is ignored)
    void SetNormalTex(const TCHAR* Name, INT Index = 0); // 0x1001f540 (ret 8)
    void SetPushedTex(const TCHAR* Name, INT Index = 0); // 0x1001f5e0 (ret 8)
};
NW_ASSERT_OFFSET(NListColumnInfo, Title, 0x8);
NW_ASSERT_OFFSET(NListColumnInfo, bClickable, 0x808);
NW_ASSERT_OFFSET(NListColumnInfo, NormalTex, 0x814);
NW_ASSERT_OFFSET(NListColumnInfo, PushedTex, 0x2014);
NW_ASSERT_OFFSET(NListColumnInfo, WholeNormalTex, 0x3814);
NW_ASSERT_OFFSET(NListColumnInfo, WholePushedTex, 0x4014);
NW_ASSERT_SIZE(NListColumnInfo, 0x4814);

// NCHeaderCtrl - a column's header button (UClass "CHeaderCtrl", 0xA0C): drawn from textures (a whole one, or a left
// cap, a middle and a right cap per state), the title centered, pushed while the mouse button is down. Released over
// it, it tells the list (NID_LISTCTRL_HEADER, lParam = NotifyID = column index).
class NCHeaderCtrl : public NControl
{
    NW_DECLARE_CLASS(NCHeaderCtrl, NControl, 0, 0x1026D4D8)

    UTexture* WholeNormalTex;        // +0x1D4 (names ours) a texture for the whole header, 67 x 15 texels (0 = the caps)
    UTexture* WholePushedTex;        // +0x1D8
    UTexture* NormalTex[3];          // +0x1DC left cap, middle, right cap ("NWindow.BlackTexture" by default)
    UTexture* PushedTex[3];          // +0x1E8 the same while pushed
    TCHAR Title[1024];               // +0x1F4
    UBOOL bReleased;                 // +0x9F4 (1)
    UBOOL bClickable;                // +0x9F8 (1)
    UBOOL bNumeric;                  // +0x9FC (0) the column sorts by value
    UBOOL bAscending;                // +0xA00 (1) order of the next sort; each sort flips it
    INT ColumnWidth;                 // +0xA04
    INT ColumnHeight;                // +0xA08

    NCHeaderCtrl();                                                  // 0x10020e00
    // ~NCHeaderCtrl                                                 // 0x1001cc90 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100127a0 (folded)
    virtual INT OnDestroy();                                         // 38 0x1001f790
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1001fe90
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1001f6e0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1001f740

    void SetColumnInfo(NListColumnInfo* Info);                       // 0x1001f810 (name ours)
};
NW_ASSERT_OFFSET(NCHeaderCtrl, NormalTex, 0x1DC);
NW_ASSERT_OFFSET(NCHeaderCtrl, Title, 0x1F4);
NW_ASSERT_OFFSET(NCHeaderCtrl, bReleased, 0x9F4);
NW_ASSERT_OFFSET(NCHeaderCtrl, ColumnWidth, 0xA04);
NW_ASSERT_SIZE(NCHeaderCtrl, 0xA0C);

class NCListCtrl : public NControl
{
    NW_DECLARE_CLASS(NCListCtrl, NControl, 0, 0x1026D9F8)

    UBOOL bNoScrollBar;              // +0x1D4 (name ours) the constructors' last argument (0): when set the list makes
                                     //   no scroll buttons and no slider (InitScrollBar tests bit 0)
    UTexture* BackTex;               // +0x1D8 set by the owner, drawn under the rows
    UTexture* SelectTex;             // +0x1DC "L2UI_CH3.ListCtrl.TextSelect" the selected row's bar (left part)
    UTexture* SelectTex2;            // +0x1E0 "L2UI_CH3.ListCtrl.TextSelect2" (its right end)
    UTexture* ToolTipTex;            // +0x1E4 "L2UI.tooltip.mini-tool01" (not drawn here)
    UTexture* TipTopTex;             // +0x1E8 "L2UI.Tooltip.Tool02" the C2 row tool tip: top,
    UTexture* TipMidTex;             // +0x1EC "L2UI.Tooltip.Tool01"  a strip per text line,
    UTexture* TipBottomTex;          // +0x1F0 "L2UI.Tooltip.Tool03"  bottom (C3 draws it through the window's skin)
    UTexture* LineTex;               // +0x1F4 "L2UI_CH3.Etc.textbackline" the band under every second row
    TArray<NListRow*> Rows;          // +0x1F8
    TArray<NCHeaderCtrl*> Columns;   // +0x204
    NCPushButton* ScrollUpButton;    // +0x210 the scroll controls come with the first column
    NCPushButton* ScrollDownButton;  // +0x214
    NCSliderBar* ScrollBar;          // +0x218
    INT VisibleRows;                 // +0x21C (10)
    INT MaxRows;                     // +0x220 (100) AddRow refuses more
    INT MaxColumns;                  // +0x224 (3)
    INT TopRow;                      // +0x228 first row shown
    INT NumRows;                     // +0x22C rows when the scroll range was last set
    INT Selected;                    // +0x230 -1 = none
    INT HoverRow;                    // +0x234 row under the mouse, -1 = none
    INT Unknown238;                  // +0x238 (16; 15 from the sized constructor), never read
    INT HeaderHeight;                // +0x23C the rows start under the headers (AddColumn)
    INT TextX;                       // +0x240 where the cells start
    INT TextY;                       // +0x244
    INT LineGap;                     // +0x248 (1) not read by C3's code (the rows are 17 high)

    NCListCtrl(INT bInNoScrollBar = 0);                              // 0x10020ef0 (ret 4)
    NCListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused,
               INT bInNoScrollBar = 0);                              // 0x10021080 (ret 0x14)
    // ~NCListCtrl                                                   // 0x10021010

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1001f990
    virtual INT OnCreate();                                          // 37 0x1001fa70
    virtual INT OnDestroy();                                         // 38 0x10021170
    virtual void ShowWindow();                                       // 44 0x1001fe80
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10020150
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x10021330
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1001fbf0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1001fbb0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x1001fc70
    // 68 is gone (C3); 69 and 70 are placeholders defined in NNewVirtuals.gen.cpp: their stock bodies are
    // ClearAll (0x10020d20) and HitTest (0x1001fb00) below, hooked by address
    virtual void NewVirtual69();  // 69 0x10020d20 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual70();  // 70 0x1001fb00 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 71 0x1001fc20
    virtual INT ScrollUp();                                          // 72 0x1001fd30
    virtual INT ScrollDown();                                        // 73 0x1001fd70
    virtual INT SetTopRow(INT Row);                                  // 74 0x1001fdc0
    virtual INT SortByColumn(INT Column);                            // 75 0x100205c0
    // text of the row tool tip, drawn at (X, Y) in its box (the subclasses draw it)
    void DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y) {} // C2: slot gone (was 69) (folded)

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x1001fb00 (vslot 70) row, -1 = none
    INT CallHitTest(INT X, INT Y) { return ::nw::VirtualCall<INT>(this, 70, X, Y); }
    void ClearAll();                                                 // 0x10020d20 (vslot 69) rows and columns go
    TArray<NListRow*>* GetRows();                                    // 0x1001fde0
    INT GetRowCount();                                               // 0x1001fdf0
    void SetSelected(INT Index);                                     // 0x1001fe00
    void SetTextOffset(INT X, INT Y);                                // 0x1001fe20
    __declspec(noinline) void UpdateScrollRange();                   // 0x1001fe40 (ShowWindow tail-jumps to it)
    NListRow* GetSelectedRow();                                      // 0x10020530
    void SetRepeatDelay(INT Delay);                                  // 0x10020550
    void ResetScrollBar();                                           // 0x10020580 (name ours) delay 200, range, value
    INT AddRow(NListRow* Row);                                       // 0x100207a0
    void ClearRows();                                                // 0x10020c90 (name ours) rows go, columns stay
    void InitScrollBar();                                            // 0x100207f0 (guard name) buttons and slider
    INT AddColumn(INT Index, NListColumnInfo* Info);                 // 0x10021430 (name ours)
};
NW_ASSERT_OFFSET(NCListCtrl, BackTex, 0x1D8);
NW_ASSERT_OFFSET(NCListCtrl, Rows, 0x1F8);
NW_ASSERT_OFFSET(NCListCtrl, Columns, 0x204);
NW_ASSERT_OFFSET(NCListCtrl, ScrollBar, 0x218);
NW_ASSERT_OFFSET(NCListCtrl, TopRow, 0x228);
NW_ASSERT_OFFSET(NCListCtrl, Selected, 0x230);
NW_ASSERT_OFFSET(NCListCtrl, HeaderHeight, 0x23C);
NW_ASSERT_OFFSET(NCListCtrl, LineGap, 0x248);
NW_ASSERT_SIZE(NCListCtrl, 0x24C);

#pragma pack(pop)
