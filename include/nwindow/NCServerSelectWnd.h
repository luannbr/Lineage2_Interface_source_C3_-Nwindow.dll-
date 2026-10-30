#pragma once
// NCServerSelectWnd - the server list of the login screen (UClass "CServerSelectWnd", 0xC8;
// NCLoginWnd +0xA0). The login server's list arrives a row at a time (AddServer: name, load,
// ping, type) and ends with OnServerListEnd; OK (or Enter, or a double click) logs in to the
// selected server, Cancel goes back to the login box, "Quality" orders the list: the server
// played last on top, the others by ping and, within a ping class, by users. Pings are asked
// of the network handler and kept in s_info.ini (section = server index, key "Time"), with
// the time they were taken ([Date] Time); the old ones are asked again.

#include "nwindow/NCWnd.h"
#include "nwindow/NCListCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class L2ParamStack;

// NCServerSelectListCtrl - the list itself (UClass "CServerSelectListCtrl", 0x20C): sorting by
// a column also keeps the window's LastServerRow on its server
class NCServerSelectListCtrl : public NCListCtrl
{
    NW_DECLARE_CLASS(NCServerSelectListCtrl, NCListCtrl, 0, 0x102AC998)

    NCServerSelectListCtrl();                                        // 0x100eeeb0
    NCServerSelectListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x100eef50
    // ~NCServerSelectListCtrl                                       // 0x100eef00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100ed440
    virtual INT SortByColumn(INT Column);                            // 75 0x100eeb20
};
NW_ASSERT_SIZE(NCServerSelectListCtrl, 0x24C);

class NCServerSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCServerSelectWnd, NCWnd, 0, 0x102AC470)

    UTexture* BackTex;               // +0xC8 "L2UI.lobbywnd.serverback"   // C2: offset derived, no evidence
    NCServerSelectListCtrl* ServerList; // +0xCC
    NCButton* OKButton;              // +0xD0   // C2: offset derived, no evidence
    NCButton* CancelButton;          // +0xD4   // C2: offset derived, no evidence
    NCButton* QualityButton;         // +0xD8   // C2: offset derived, no evidence
    INT LastServerID;                // +0xDC server the account played on last (login server)
    INT LastServerRow;               // +0xE0 its row, -1 = none
    INT PendingPings;                // +0xE4 ping answers still awaited
    INT TimerResult;                 // +0xE8 what SetTimer returned (never read)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UBOOL bRequested;                // +0xEC a server login went out (cleared by the timer)
    UBOOL bPingCacheOld;             // +0xF0 the saved pings are too old: ask every server
    UBOOL bPingSaved;                // +0xF4 a ping went to s_info.ini
    UBOOL UnknownC4;                 // +0xF8 set at the end of the list when bPingSaved (never read)
    BYTE RowBits[0x10C - 0xFC];      // +0xFC (C3, name ours) a bit per row: AddServer colors the name of the marked
                                     //   rows red (set by another window; 128 rows at most)

    NCServerSelectWnd();                                             // 0x100eef80
    // ~NCServerSelectWnd                                            // 0x100c8740 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100ed4a0
    virtual INT OnTimer(INT TimerID);                                // 36 0x100ed7d0
    virtual INT OnCreate();                                          // 37 0x100ef040
    virtual INT OnDestroy();                                         // 38 0x100edae0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100edc10
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x100ed7a0
    // names ours
    virtual INT OnOK();                                              // 68 0x100ee3a0
    virtual INT OnCancel();                                          // 69 0x100ee510
    virtual INT SortByQuality();                                     // 70 0x100ee570
    virtual INT OnListNotify(INT Row);                               // 71 0x100ed790

    // names ours
    void SetLastServerID(INT ServerID);                              // 0x100c81a0 (C1 only: no C2 match)
    void OnServerListEnd();                                          // 0x100ed6f0
    void ClearServers();                                             // 0x100ed760
    void AddServer(L2ParamStack& Params);                            // 0x100edd10
    // the row's number and name text (new in C2, a function of its own; name ours)
    void FormatServerLabel(INT Index, INT Flags, INT TypeIndex, TCHAR* Name, TCHAR* NameText); // 0x100ed560
    void SetPing(INT IP, INT Ping);                                  // 0x100ee2c0
    UBOOL IsPingCacheOld();                                          // 0x100ed970
    UBOOL HasNoPing(INT ServerIndex);                                // 0x100ed9c0
    void SavePing(INT ServerIndex, INT Ping);                        // 0x100eda20
    void SavePingTime();                                             // 0x100eda70

    // calendar of the ping cache (names ours)
    static UBOOL IsLeapYear(INT Year);                               // 0x100ed7f0
    static INT DaysInYears(INT Years);                               // 0x100ed830
    static INT DaysInMonths(INT Year, INT Month);                    // 0x100ed870
    static INT HoursInDays(INT Day, INT Hour);                       // 0x100ed8e0
    static INT GetTimeStamp();                                       // 0x100ed910
};
NW_ASSERT_OFFSET(NCServerSelectWnd, ServerList, 0xCC);
NW_ASSERT_OFFSET(NCServerSelectWnd, LastServerID, 0xDC);
NW_ASSERT_OFFSET(NCServerSelectWnd, bRequested, 0xEC);
NW_ASSERT_OFFSET(NCServerSelectWnd, UnknownC4, 0xF8);
NW_ASSERT_SIZE(NCServerSelectWnd, 0x10C);

class NCHtmlViewer;

// NCServerInfoWnd - the help page under the server list (UClass "CServerInfoWnd", 0xD0; NCLoginServerWnd +0xCC, new in
// C3): a backdrop and an HTML viewer (..\l2text\server_help.htm, loaded unless the Thai HTML mode is on) inside it.
class NCServerInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCServerInfoWnd, NCWnd, 0, 0x102ABF48)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.Serverselectwnd.Serverinfo_back"
    NCHtmlViewer* Viewer;            // +0xCC 7 pixels in, the window's size less 14 (name ours)

    NCServerInfoWnd();                                               // 0x10091c40 (NCWnd's constructor and the vtable)
    // ~NCServerInfoWnd                                              // 0x10091c90

    virtual INT OnCreate();                                          // 37 0x100eedc0
    virtual INT OnDestroy();                                         // 38 0x100eea70
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ee970
};
NW_ASSERT_OFFSET(NCServerInfoWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCServerInfoWnd, Viewer, 0xCC);
NW_ASSERT_SIZE(NCServerInfoWnd, 0xD0);

#pragma pack(pop)