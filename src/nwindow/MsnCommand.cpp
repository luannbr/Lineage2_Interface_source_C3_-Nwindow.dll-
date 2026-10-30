/*=============================================================================
    MsnCommand.cpp - one line of the messenger's wire protocol and the parsed line a socket hands over (C3 only).
    Stock: 0x10127520, 0x10127560, 0x10127770 (argument getters), 0x10129b30 .. 0x10129d80 (constructors and
    destructors), 0x1012a0e0, 0x1012e020, the line's text (ToString 0x10127420), its length (0x101275F0), AddArg / AddIntArg
    (0x10129770 / 0x10129810), ParseArgs (0x101295A0), the parser (0x10129EB0), the tables the line constructor fills (0x10127AD0,
    0x10127DF0) and their lookups (0x10127630, 0x101276D0). Stage 5: the functions the chat lane had written for its own NMsnCommand
    (NChatMsg.cpp) are this file's MsnCommand / MsnLine methods now (one definition of the command).
=============================================================================*/
#include "nwindow/MsnCommand.h"

/*-----------------------------------------------------------------------------
    MsnCommand.
-----------------------------------------------------------------------------*/

// 0x10129b30 - no verb, no transaction (-1), no arguments
MsnCommand::MsnCommand()
{
    TrID = -1;
}
NW_HOOK_CTOR(0x10129b30, MsnCommand)

// 0x10129b50 (ret 4) - the verb and the transaction, then a copy of every argument string (new, 0xC bytes each)
MsnCommand::MsnCommand(const MsnCommand& Other)
{
    Verb = Other.Verb;
    TrID = Other.TrID;
    Args.Empty();
    for (INT i = 0; i < Other.Args.Num(); i++)
    {
        FString* Arg = new FString(*Other.Args(i));
        Args(Args.Add()) = Arg;
    }
}
NW_HOOK_CTOR_ARGS(0x10129b50, MsnCommand, (const MsnCommand& Other), (Other))

// 0x1012e020 (ret 0xC) - a command of that verb (the string comes by value)
MsnCommand::MsnCommand(const FString& InVerb)
{
    TrID = -1;
    Verb = InVerb;
}
NW_HOOK_CTOR_ARGS(0x1012e020, MsnCommand, (FString InVerb), (InVerb))

// 0x10129c30 - the argument strings freed, the array emptied (then the array's destructor and the verb's)
MsnCommand::~MsnCommand()
{
    for (INT i = 0; i < Args.Num(); i++)
        if (Args(i))
            delete Args(i);
    Args.Empty();
}
NW_HOOK_DTOR(0x10129c30, MsnCommand)

// 0x1012a0e0 - what the senders call on a command they are done with: the array emptied first, so the argument strings are
// NOT freed (the stock leaks them), then the destructor's body
void MsnCommand::Release()
{
    Args.Empty();
    this->~MsnCommand();
}
NW_HOOK_METHOD(0x1012a0e0, MsnCommand, Release)

// 0x10127520 (ret 8) - a copy of argument Index, an empty string when there is none (an index below the count is read as is,
// negative ones included)
FString MsnCommand::GetArg(INT Index)
{
    if (Args.Num() > Index)
        return FString(*Args(Index));
    return FString();
}
NW_HOOK_METHOD(0x10127520, MsnCommand, GetArg)

// 0x10127560 (ret 4) - argument Index as a number, -1 when there is none
INT MsnCommand::GetArgInt(INT Index)
{
    if (Args.Num() > Index)
    {
        FString Text(*Args(Index));
        return appAtoi(*Text);
    }
    return -1;
}
NW_HOOK_METHOD(0x10127560, MsnCommand, GetArgInt)

// 0x10127770 (ret 8; the result is the hidden argument) - argument Index is "host:port": the host before the first colon,
// the number after it; the default server (messenger.hotmail.com:1863) when there is no such argument
NMsnServer MsnCommand::GetArgServer(INT Index)
{
    if (Args.Num() > Index)
    {
        FString Text(*Args(Index));
        INT Colon = Text.InStr(TEXT(":"), 0);
        FString Host = Text.Left(Colon);
        INT Port = appAtoi(*Text.Right(Text.Len() - Colon - 1));
        NMsnServer Server(Host, Port);
        return Server;
    }
    return NMsnServer();
}
NW_HOOK_METHOD(0x10127770, MsnCommand, GetArgServer)

// 0x101270f0 (name ours) - the number of arguments (ArrayNum of Args, +0x14)
INT MsnCommand::GetArgCount()
{
    return Args.Num();
}
NW_HOOK_METHOD(0x101270f0, MsnCommand, GetArgCount)

// 0x10127420 (ret 4) - "<Verb>" and, with a transaction, " <TrID>" and each argument after a space. (The chat lane's NMsnCommand::ToWire,
// now this class's)
__declspec(noinline) FString MsnCommand::ToString()
{
    FString Result;
    Result += Verb;
    if (TrID != -1)
    {
        TCHAR Number[32];
        appSprintf(Number, TEXT(" %d"), TrID);
        Result += Number;
        for (INT i = 0; i < Args.Num(); i++)
        {
            Result += FString(TEXT(" "));
            Result += **Args(i);
        }
    }
    return FString(Result);
}
NW_HOOK_METHOD(0x10127420, MsnCommand, ToString)

// 0x101275f0 - the verb's length plus, for each argument, a space and its length. (The chat lane's NMsnCommand::GetLength, now this class's)
__declspec(noinline) INT MsnCommand::GetLength()
{
    INT Length = Verb.Len();
    for (INT i = 0; i < Args.Num(); i++)
        Length += 1 + Args(i)->Len();
    return Length;
}
NW_HOOK_METHOD(0x101275F0, MsnCommand, GetLength)

// 0x10129770 (ret 0xC) - a copy of the string (on the heap) appended to the arguments. (The chat lane's NMsnCommand::AddArg, now this class's)
__declspec(noinline) void MsnCommand::AddArg(FString Arg)
{
    FString* Copy = new FString(Arg);
    INT i = Args.Add();
    Args(i) = Copy;
}
typedef void (MsnCommand::*SigMsnCommandAddArg)(FString);
NW_HOOK_VARARGS(0x10129770, MsnCommand, AddArg, SigMsnCommandAddArg)

// 0x10129810 (ret 4) - the number as text appended to the arguments. (The chat lane's NMsnCommand::AddIntArg, now this class's)
__declspec(noinline) void MsnCommand::AddIntArg(INT Value)
{
    TCHAR Number[28];
    appSprintf(Number, TEXT("%d"), Value);
    FString* Copy = new FString(Number);
    INT i = Args.Add();
    Args(i) = Copy;
}
NW_HOOK_METHOD(0x10129810, MsnCommand, AddIntArg)

// 0x101295a0 (ret 0xC) - the line cut at its first "\r\n", then at each space: the words (each a heap string) appended to the
// arguments, then the rest if there is one. (The chat lane's NMsnCommand::ParseArgs, now this class's)
__declspec(noinline) void MsnCommand::ParseArgs(FString Line)
{
    INT Pos = Line.InStr(TEXT("\r\n"), 0);
    if (Pos != -1)
        Line = Line.Left(Pos);
    INT Len = Line.Len();
    Pos = Line.InStr(TEXT(" "), 0);
    while (Pos != -1)
    {
        FString* Word = new FString(Line.Left(Pos));
        INT i = Args.Add();
        Args(i) = Word;
        Line = Line.Mid(Pos + 1, Len - Pos - 1);
        Len = Line.Len();
        Pos = Line.InStr(TEXT(" "), 0);
    }
    if (Line.Len() > 0)
    {
        FString* Rest = new FString(Line);
        INT i = Args.Add();
        Args(i) = Rest;
    }
}
typedef void (MsnCommand::*SigMsnCommandParseArgs)(FString);
NW_HOOK_VARARGS(0x101295A0, MsnCommand, ParseArgs, SigMsnCommandParseArgs)

/*-----------------------------------------------------------------------------
    MsnLine.
-----------------------------------------------------------------------------*/

// 0x10127ad0 (name ours) - the verbs that come unprompted (AsyncVerbs at +0x1C): MSG NLN FLN BYE OUT BPR LSG LST RNG JOI. (The chat
// lane's NMsnCommand::InitListA, now this class's)
__declspec(noinline) void MsnLine::FillAsyncVerbs()
{
    AsyncVerbs.AddItem(new FString(TEXT("MSG")));
    AsyncVerbs.AddItem(new FString(TEXT("NLN")));
    AsyncVerbs.AddItem(new FString(TEXT("FLN")));
    AsyncVerbs.AddItem(new FString(TEXT("BYE")));
    AsyncVerbs.AddItem(new FString(TEXT("OUT")));
    AsyncVerbs.AddItem(new FString(TEXT("BPR")));
    AsyncVerbs.AddItem(new FString(TEXT("LSG")));
    AsyncVerbs.AddItem(new FString(TEXT("LST")));
    AsyncVerbs.AddItem(new FString(TEXT("RNG")));
    AsyncVerbs.AddItem(new FString(TEXT("JOI")));
}
NW_HOOK_METHOD(0x10127AD0, MsnLine, FillAsyncVerbs)

// 0x10127df0 (name ours) - the server's error numbers (ErrorCodes at +0x28): 201 .. 927 (80 of them). (The chat lane's
// NMsnCommand::InitListB, now this class's)
__declspec(noinline) void MsnLine::FillErrorCodes()
{
    ErrorCodes.AddItem(new FString(TEXT("201")));
    ErrorCodes.AddItem(new FString(TEXT("205")));
    ErrorCodes.AddItem(new FString(TEXT("206")));
    ErrorCodes.AddItem(new FString(TEXT("207")));
    ErrorCodes.AddItem(new FString(TEXT("208")));
    ErrorCodes.AddItem(new FString(TEXT("209")));
    ErrorCodes.AddItem(new FString(TEXT("210")));
    ErrorCodes.AddItem(new FString(TEXT("211")));
    ErrorCodes.AddItem(new FString(TEXT("212")));
    ErrorCodes.AddItem(new FString(TEXT("215")));
    ErrorCodes.AddItem(new FString(TEXT("216")));
    ErrorCodes.AddItem(new FString(TEXT("217")));
    ErrorCodes.AddItem(new FString(TEXT("218")));
    ErrorCodes.AddItem(new FString(TEXT("219")));
    ErrorCodes.AddItem(new FString(TEXT("220")));
    ErrorCodes.AddItem(new FString(TEXT("221")));
    ErrorCodes.AddItem(new FString(TEXT("222")));
    ErrorCodes.AddItem(new FString(TEXT("223")));
    ErrorCodes.AddItem(new FString(TEXT("224")));
    ErrorCodes.AddItem(new FString(TEXT("225")));
    ErrorCodes.AddItem(new FString(TEXT("227")));
    ErrorCodes.AddItem(new FString(TEXT("228")));
    ErrorCodes.AddItem(new FString(TEXT("229")));
    ErrorCodes.AddItem(new FString(TEXT("230")));
    ErrorCodes.AddItem(new FString(TEXT("280")));
    ErrorCodes.AddItem(new FString(TEXT("281")));
    ErrorCodes.AddItem(new FString(TEXT("282")));
    ErrorCodes.AddItem(new FString(TEXT("300")));
    ErrorCodes.AddItem(new FString(TEXT("301")));
    ErrorCodes.AddItem(new FString(TEXT("500")));
    ErrorCodes.AddItem(new FString(TEXT("501")));
    ErrorCodes.AddItem(new FString(TEXT("502")));
    ErrorCodes.AddItem(new FString(TEXT("503")));
    ErrorCodes.AddItem(new FString(TEXT("510")));
    ErrorCodes.AddItem(new FString(TEXT("520")));
    ErrorCodes.AddItem(new FString(TEXT("540")));
    ErrorCodes.AddItem(new FString(TEXT("600")));
    ErrorCodes.AddItem(new FString(TEXT("601")));
    ErrorCodes.AddItem(new FString(TEXT("602")));
    ErrorCodes.AddItem(new FString(TEXT("603")));
    ErrorCodes.AddItem(new FString(TEXT("604")));
    ErrorCodes.AddItem(new FString(TEXT("605")));
    ErrorCodes.AddItem(new FString(TEXT("606")));
    ErrorCodes.AddItem(new FString(TEXT("707")));
    ErrorCodes.AddItem(new FString(TEXT("710")));
    ErrorCodes.AddItem(new FString(TEXT("711")));
    ErrorCodes.AddItem(new FString(TEXT("712")));
    ErrorCodes.AddItem(new FString(TEXT("713")));
    ErrorCodes.AddItem(new FString(TEXT("714")));
    ErrorCodes.AddItem(new FString(TEXT("715")));
    ErrorCodes.AddItem(new FString(TEXT("716")));
    ErrorCodes.AddItem(new FString(TEXT("717")));
    ErrorCodes.AddItem(new FString(TEXT("718")));
    ErrorCodes.AddItem(new FString(TEXT("720")));
    ErrorCodes.AddItem(new FString(TEXT("723")));
    ErrorCodes.AddItem(new FString(TEXT("724")));
    ErrorCodes.AddItem(new FString(TEXT("725")));
    ErrorCodes.AddItem(new FString(TEXT("729")));
    ErrorCodes.AddItem(new FString(TEXT("730")));
    ErrorCodes.AddItem(new FString(TEXT("731")));
    ErrorCodes.AddItem(new FString(TEXT("732")));
    ErrorCodes.AddItem(new FString(TEXT("733")));
    ErrorCodes.AddItem(new FString(TEXT("800")));
    ErrorCodes.AddItem(new FString(TEXT("910")));
    ErrorCodes.AddItem(new FString(TEXT("911")));
    ErrorCodes.AddItem(new FString(TEXT("912")));
    ErrorCodes.AddItem(new FString(TEXT("913")));
    ErrorCodes.AddItem(new FString(TEXT("914")));
    ErrorCodes.AddItem(new FString(TEXT("915")));
    ErrorCodes.AddItem(new FString(TEXT("917")));
    ErrorCodes.AddItem(new FString(TEXT("918")));
    ErrorCodes.AddItem(new FString(TEXT("919")));
    ErrorCodes.AddItem(new FString(TEXT("920")));
    ErrorCodes.AddItem(new FString(TEXT("921")));
    ErrorCodes.AddItem(new FString(TEXT("922")));
    ErrorCodes.AddItem(new FString(TEXT("923")));
    ErrorCodes.AddItem(new FString(TEXT("924")));
    ErrorCodes.AddItem(new FString(TEXT("925")));
    ErrorCodes.AddItem(new FString(TEXT("926")));
    ErrorCodes.AddItem(new FString(TEXT("927")));
}
NW_HOOK_METHOD(0x10127DF0, MsnLine, FillErrorCodes)

// 0x10129ce0 - an empty command, the table of the unprompted verbs (0x10127AD0) and the table of the error numbers
// (0x10127DF0) filled, no flag
MsnLine::MsnLine()
{
    FillAsyncVerbs();
    FillErrorCodes();
    bAsync = 0;
    bError = 0;
}
NW_HOOK_CTOR(0x10129ce0, MsnLine)

// 0x10129d50 (ret 4) - the command part copied; the tables stay empty and the flags are not written
MsnLine::MsnLine(const MsnCommand& Command) : MsnCommand(Command)
{
}
NW_HOOK_CTOR_ARGS(0x10129d50, MsnLine, (const MsnCommand& Command), (Command))

// the by-value copy of a line is the same constructor
MsnLine::MsnLine(const MsnLine& Other) : MsnCommand(Other)
{
}

// 0x10129d80 - the strings of the three arrays freed and the arrays emptied, the tables' destructors, then the command's
MsnLine::~MsnLine()
{
    for (INT i = 0; i < Args.Num(); i++)
        if (Args(i))
            delete Args(i);
    Args.Empty();
    for (INT i = 0; i < AsyncVerbs.Num(); i++)
        if (AsyncVerbs(i))
            delete AsyncVerbs(i);
    AsyncVerbs.Empty();
    for (INT i = 0; i < ErrorCodes.Num(); i++)
        if (ErrorCodes(i))
            delete ErrorCodes(i);
    ErrorCodes.Empty();
}
NW_HOOK_DTOR(0x10129d80, MsnLine)

// the senders' local command: built from the verb the sender passes in place, destroyed as the stock does (Release)
__declspec(noinline) NMsnLocalCommand::NMsnLocalCommand(FString InVerb) : MsnCommand(InVerb)
{
}

// the copy of a command that SendLine takes by value and releases (the arguments copied one by one, as the stock copy constructor does)
__declspec(noinline) NMsnLocalCommand::NMsnLocalCommand(const MsnCommand& Other) : MsnCommand(Other)
{
}

__declspec(noinline) NMsnLocalCommand::~NMsnLocalCommand()
{
    Args.Empty();
}

// 0x10127630 / 0x101276d0 (ret 0xC) - is the string (compared without case) one of the unprompted verbs / one of the error numbers; the argument
// is destroyed. (The chat lane's NMsnCommand::InListA / InListB, now this class's)
__declspec(noinline) INT MsnLine::IsAsyncVerb(FString Text)
{
    for (INT i = 0; i < AsyncVerbs.Num(); i++)
        if (Text == *AsyncVerbs(i))
            return 1;
    return 0;
}
__declspec(noinline) INT MsnLine::IsErrorCode(FString Text)
{
    for (INT i = 0; i < ErrorCodes.Num(); i++)
        if (Text == *ErrorCodes(i))
            return 1;
    return 0;
}
typedef INT (MsnLine::*SigMsnLineIsIn)(FString);
NW_HOOK_VARARGS(0x10127630, MsnLine, IsAsyncVerb, SigMsnLineIsIn)
NW_HOOK_VARARGS(0x101276D0, MsnLine, IsErrorCode, SigMsnLineIsIn)

// 0x10129eb0 (ret 0xC, the text by value) - a received line split up. An empty line flags bAsync and gives 0. A line of up to
// three characters is only the verb (bAsync). Otherwise the verb is its first three characters; an unprompted verb
// (bAsync) has the arguments after "VRB "; an error number sets bError; anything else is "NNN TrID args": the transaction id
// is the number before the next space (or the rest of the line) and the arguments follow it
INT MsnLine::Parse(FString Raw)
{
    INT Len = Raw.Len();
    if (Raw.Num() == 0 || Raw.Num() - 1 == 0)
    {
        bAsync = 1;
        return 0;
    }
    if (Len <= 3)
    {
        Verb = Raw;
        bAsync = 1;
        return 1;
    }
    Verb = Raw.Left(3);
    if (IsAsyncVerb(Verb))
    {
        ParseArgs(Raw.Right(Raw.Len() - 4));
        bAsync = 1;
    }
    else if (IsErrorCode(Verb))
    {
        bError = 1;
    }
    else
    {
        Raw = Raw.Right(Raw.Len() - 4);
        INT Space = Raw.InStr(TEXT(" "), 0);
        if (Space == -1)
        {
            TrID = appAtoi(*Raw);
        }
        else
        {
            TrID = appAtoi(*Raw.Left(Space));
            ParseArgs(Raw.Right(Raw.Len() - Space - 1));
        }
        bAsync = 0;
    }
    return 1;
}
typedef INT (MsnLine::*SigMsnLineParse)(FString);
NW_HOOK_VARARGS(0x10129eb0, MsnLine, Parse, SigMsnLineParse)

// 0x1012e770 (ret 4) - the copy constructor of an array of string pointers (the pointers copied, not the strings): the count and
// size of the original, then each pointer appended (the stock appends through new(Array) 0x10131560)
typedef TArray<FString*> TMsnStrPtrArray;
NW_HOOK_CTOR_ARGS(0x1012e770, TMsnStrPtrArray, (const TMsnStrPtrArray& Other), (Other))
