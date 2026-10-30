/*=============================================================================
    NSSQHelpers.cpp - the Seven Signs windows' text pickers and request. C3 stock: 0x10060D30 .. 0x10060ECF.
=============================================================================*/
#include "nwindow/NSSQHelpers.h"

// sysstring line of "nothing" used by the pickers' default
enum { STR_Dash = 27 };

// 0x10060D30
const TCHAR* __fastcall SSQSealName(void* Console, void* Unused, INT Seal)
{
    switch (Seal)
    {
    case 1:
        return *GL2GameData.SysString[816];
    case 2:
        return *GL2GameData.SysString[817];
    case 3:
        return *GL2GameData.SysString[818];
    default:
        return *GL2GameData.SysString[STR_Dash];
    }
}
NW_HOOK_FUNC(0x10060D30, SSQSealName)

// 0x10060D80
const TCHAR* __fastcall SSQCabalName(void* Console, void* Unused, INT Cabal)
{
    switch (Cabal)
    {
    case 1:
        return *GL2GameData.SysString[815];
    case 2:
        return *GL2GameData.SysString[814];
    default:
        return *GL2GameData.SysString[STR_Dash];
    }
}
NW_HOOK_FUNC(0x10060D80, SSQCabalName)

// 0x10060DC0
const TCHAR* __fastcall SSQEventName(void* Console, void* Unused, INT Event)
{
    switch (Event)
    {
    case 1:
        return *GL2GameData.SysString[819];
    case 2:
        return *GL2GameData.SysString[820];
    case 3:
        return *GL2GameData.SysString[821];
    case 4:
        return *GL2GameData.SysString[844];
    case 5:
        return *GL2GameData.SysString[822];
    default:
        return *GL2GameData.SysString[STR_Dash];
    }
}
NW_HOOK_FUNC(0x10060DC0, SSQEventName)

// 0x10060E70
const TCHAR* __fastcall SSQSealStateName(void* Console, void* Unused, INT State)
{
    switch (State)
    {
    case 1:
        return *GL2GameData.SystemMsg[1178].Message;
    case 2:
        return *GL2GameData.SystemMsg[1179].Message;
    case 3:
        return *GL2GameData.SystemMsg[1180].Message;
    default:
        return *GL2GameData.SystemMsg[27].Message;
    }
}
NW_HOOK_FUNC(0x10060E70, SSQSealStateName)

// 0x10060EC0
void NConsoleSSQ::RequestSSQStatus(INT Page)
{
    Engine->NetHandler->RequestSSQStatus(Page);
}
NW_HOOK_METHOD(0x10060EC0, NConsoleSSQ, RequestSSQStatus)
