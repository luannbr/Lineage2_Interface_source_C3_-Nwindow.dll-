#pragma once
// NSSQHelpers - the small stock helpers the Seven Signs windows call (all of them anonymous in the db): four texts the
// windows pick by a number (the console is passed in ecx and ignored, so they are __fastcall free functions here) and
// the request the windows send to the server. Names ours.

#include "nwindow/NConsoleWnd.h"

// 0x10060D30 (ret 4): the name of a seal (1, 2, 3: sysstrings 816 .. 818), the "-" line for any other number
const TCHAR* __fastcall SSQSealName(void* Console, void* Unused, INT Seal);
// 0x10060D80 (ret 4): the cabal (1: sysstring 815, 2: 814), the "-" line for any other number
const TCHAR* __fastcall SSQCabalName(void* Console, void* Unused, INT Cabal);
// 0x10060DC0 (ret 4): the event the quest-like row stands for (1 .. 5: sysstrings 819, 820, 821, 844, 822)
const TCHAR* __fastcall SSQEventName(void* Console, void* Unused, INT Event);
// 0x10060E70 (ret 4): the text of a seal's state (1, 2, 3: system messages 1178, 1179, 1180; others 27)
const TCHAR* __fastcall SSQSealStateName(void* Console, void* Unused, INT State);

// 0x10060EC0 (ret 4): the console's request for the Seven Signs status of a page (2 the event list, 3 the seals, 1 and
// 4 the status window's two pages): the engine's net handler (UL2NetHandler::RequestSSQStatus)
struct NConsoleSSQ : public NConsoleWnd
{
    void RequestSSQStatus(INT Page);
};
