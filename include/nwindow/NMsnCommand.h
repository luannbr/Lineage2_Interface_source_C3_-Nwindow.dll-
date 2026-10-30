#pragma once
// NMsnCommand - a command of the MSN protocol being built (C3 only; name ours): the object the session's request functions
// (NMsnSession.cpp) make on their stack. A name ("ADD", "REM", "ADG", "RMG", "REG"), the transaction number and the parameters
// (texts, numbers written as "%d"), which the session then sends (a copy of the command goes to MSNSocket::SendLine 0x1013BDC0 by
// value, which releases it).
//
// There is ONE definition of the messenger's command: MsnCommand (MsnCommand.h, 0x1C bytes: the verb at +0x00, the transaction number
// at +0x0C, the arguments at +0x10; constructor 0x1012E020 (the verb by value), copy constructor 0x10129B50, Release 0x1012A0E0, AddArg
// 0x10129770, AddIntArg 0x10129810). Stage 5 (sa-msn-proto): two lanes that ported the protocol had each written a struct of the
// command (this header's 0x1C-byte proxy that called the stock by address, renamed NMsnSessionCommand by the s5 prep, and NChatMsg.h's
// NMsnCommand with the fields); this is the dialogs' view of it: the local a request builds and releases as the stock does
// (NMsnLocalCommand), with the names the dialog code uses.

#include "nwindow/MsnCommand.h"
#pragma pack(push, 4)

struct NMsnCommand : public NMsnLocalCommand
{
    // 0x1012E020 (ret 0xC): the command of that name, no transaction yet
    NMsnCommand(const TCHAR* InName) : NMsnLocalCommand(FString(InName)) {}

    INT& TrId() { return TrID; }

    // 0x10129770 (ret 0xC): a copy of the text is appended to the parameters
    void AddParam(const TCHAR* Text) { AddArg(FString(Text)); }
    void AddParam(const FString& Text) { AddArg(Text); }
    // 0x10129810 (ret 4): the number as "%d"
    void AddParam(INT Number) { AddIntArg(Number); }
};
NW_ASSERT_SIZE(NMsnCommand, 0x1C);

#pragma pack(pop)
