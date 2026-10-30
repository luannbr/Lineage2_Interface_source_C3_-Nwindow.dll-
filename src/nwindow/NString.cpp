/*=============================================================================
    NString.cpp - growable text buffer (name ours). Stock: 0x10021cf0 .. 0x10021ef5.
=============================================================================*/
#include "nwindow/NString.h"

#include <wchar.h>

// 0x10029f40
NString::NString()
{
    Capacity = 0;
    Data = NULL;
    Length = 0;
    Color = 0;
    Indent = 0;
}
NW_HOOK_CTOR(0x10029f40, NString)

// 0x1002a120 (ret 4) - empty, then Text inserted at 0
NString::NString(const TCHAR* Text)
{
    Capacity = 0;
    Data = NULL;
    Length = 0;
    Color = 0;
    Indent = 0;
    Insert(Text, 0);
}
NW_HOOK_CTOR_ARGS(0x1002a120, NString, (const TCHAR* A), (A))

// 0x1002a150
NString::~NString()
{
    Empty();
}
NW_HOOK_DTOR(0x1002a150, NString)

// 0x10029f60
TCHAR* NString::GetAt(UINT Index)
{
    if (Index > Length)
        return NULL;
    return Data + Index;
}
NW_HOOK_METHOD(0x10029f60, NString, GetAt)

// 0x10029f80
TCHAR* NString::AllocBuffer(INT Count)
{
    return new TCHAR[Count];
}
NW_HOOK_METHOD(0x10029f80, NString, AllocBuffer)

// 0x10029fb0 (ret 8) - the tail moved (or copied, when the buffer grows) is Length*2 - Pos bytes: the stock arithmetic
// mixes characters and bytes, so with Pos > 0 it touches up to Pos bytes past the text (and past the buffer when it is
// nearly full). Kept as is: it is the stock behaviour
INT NString::Insert(const TCHAR* Text, UINT Pos)
{
    if (!Text)
        return 0;
    UINT Len = (UINT)wcslen(Text);
    if (!Len)
        return 0;
    if (Pos > Length)
        return 0;
    if (!Data)
    {
        Data = AllocBuffer(256);
        Capacity = 256;
    }
    if (Length + Len < Capacity)
    {
        appMemmove(Data + Pos + Len, Data + Pos, Length * 2 - Pos);
        Length += Len;
        appMemcpy(Data + Pos, Text, Len * 2);
    }
    else
    {
        TCHAR* Old = Data;
        Capacity += Len > 256 ? Len : 256;
        Data = AllocBuffer(Capacity);
        appMemcpy(Data, Old, Pos * 2);
        appMemcpy(Data + Pos, Text, Len * 2);
        appMemcpy(Data + Pos + Len, Old + Pos, Length * 2 - Pos);
        Length += Len;
        delete[] Old;
    }
    Data[Length] = 0;
    return Len;
}
NW_HOOK_METHOD(0x10029fb0, NString, Insert)

// 0x1002a0e0 (ret 4)
INT NString::Append(const TCHAR* Text)
{
    return Insert(Text, GetLength());
}
NW_HOOK_METHOD(0x1002a0e0, NString, Append)

// 0x1002a100 - Capacity stays; the next Insert reallocates
void NString::Empty()
{
    if (Data)
    {
        delete[] Data;
        Data = NULL;
    }
    Length = 0;
}
NW_HOOK_METHOD(0x1002a100, NString, Empty)
