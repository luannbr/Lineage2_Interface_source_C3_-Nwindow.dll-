/*=============================================================================
    NMacroEntry.cpp - the macro object of the macro window (NMacroEntry.h) and its line.
    Stock: 0x1008B2D0 .. 0x1008B500 (the line, the getters, Set, Reset), 0x1008F840 / 0x1008F910 (the entry's
    constructor and destructor).
=============================================================================*/
#include "nwindow/NMacroEntry.h"

/*-----------------------------------------------------------------------------
    NMacroLine.
-----------------------------------------------------------------------------*/

// 0x1009adf0 - the state words and the text cleared
NMacroLine::NMacroLine()
{
    Unknown24 = 0;
    Duration = 0;
    StartTime = 0;
    Clock = 0;
    appMemset(Text, 0, sizeof(Text));
}
NW_HOOK_CTOR(0x1009adf0, NMacroLine)

// 0x1009ae20 - the vector destructor's callback: the state words cleared
NMacroLine::~NMacroLine()
{
    Unknown24 = 0;
    Duration = 0;
    StartTime = 0;
    Clock = 0;
}
NW_HOOK_DTOR(0x1009ae20, NMacroLine)

/*-----------------------------------------------------------------------------
    NMacroEntry.
-----------------------------------------------------------------------------*/

// 0x1009f980 - the tool tip, the cool time, the twelve lines and the parser are constructed, the numbers and the
// three texts cleared
NMacroEntry::NMacroEntry()
{
    bRunning = 0;
    IconType = 0;
    NumLines = 0;
    CurLine = 0;
    ID = 0;
    bSelected = 0;
    IconTex = NULL;
    appMemset(Description, 0, sizeof(Description));
    appMemset(Name, 0, sizeof(Name));
    appMemset(ShortName, 0, sizeof(ShortName));
}
NW_HOOK_CTOR(0x1009f980, NMacroEntry)

// 0x1009fa60 - no icon, nothing running, no lines; the parser, the lines, the cool time and the tool tip are destroyed
NMacroEntry::~NMacroEntry()
{
    IconTex = NULL;
    Reset();
    NumLines = 0;
    bSelected = 0;
}
NW_HOOK_DTOR(0x1009fa60, NMacroEntry)

// 0x1009ae40 - the macro's name; NULL when empty
const TCHAR* NMacroEntry::GetName()
{
    if (Name == NULL || *Name == 0)
        return NULL;
    return Name;
}
NW_HOOK_METHOD(0x1009ae40, NMacroEntry, GetName)

// 0x1009ae30 (sub_ in the db) - the slot is selected in the grid
INT NMacroEntry::IsSelected()
{
    return bSelected;
}
NW_HOOK_METHOD(0x1009ae30, NMacroEntry, IsSelected)

// 0x1009ae60 (the db names it GetDescription) - the short name; NULL when empty
const TCHAR* NMacroEntry::GetShortName()
{
    if (ShortName == NULL || *ShortName == 0)
        return NULL;
    return ShortName;
}
NW_HOOK_METHOD(0x1009ae60, NMacroEntry, GetShortName)

// 0x1009ae80 (sub_ in the db: the console lane's, NToolTipInfo::FillMacro's callee) - the description; NULL when empty
const TCHAR* NMacroEntry::GetDescription()
{
    if (Description == NULL || *Description == 0)
        return NULL;
    return Description;
}
NW_HOOK_METHOD(0x1009ae80, NMacroEntry, GetDescription)

// 0x1009aea0 - naked: the stock loop of UnselectAll (0x1009d6f0, the stock caller at 0x1009d719) keeps its pointer in eax
// across the call, so eax must come back untouched; bSelected is at +0xA34 (NW_ASSERT_OFFSET bRunning 0xA30)
NW_NAKED INT NMacroEntry::Deselect()
{
    __asm
    {
        mov dword ptr [ecx + 0xA34], 0
        ret
    }
}
NW_HOOK_METHOD(0x1009aea0, NMacroEntry, Deselect)

// 0x1009aeb0 (ret 0x3fc) - a macro as the server sends it: the id, the three texts, the icon and the number of
// lines, then each line into its place (a LineNo of 0 or 1 is the first line); what a line keeps depends on its type.
// C3: the cool time is stopped at the end
void NMacroEntry::Set(INT InID, NMacroInfo Info)
{
    ID = InID;
    appStrcpy(Name, Info.Name);
    appStrcpy(Description, Info.Description);
    appSprintf(ShortName, TEXT("%s"), Info.ShortName);
    IconType = (signed char)Info.IconType;
    NumLines = (signed char)Info.NumLines;
    for (INT i = 0; i < NumLines; i++)
    {
        NMacroInfoLine& Src = Info.Lines[i];
        INT Index = (signed char)Src.LineNo;
        if (Index > 0)
            Index--;
        NMacroLine& Dest = Lines[Index];
        Dest.Type = (signed char)Src.Type;
        switch (Dest.Type)
        {
            case NMLT_Skill:
                Dest.Param08 = Src.A;
                Dest.Param10 = (signed char)Src.B;
                break;
            case NMLT_Action:
                Dest.Param08 = Src.A;
                break;
            case NMLT_Command:
                appSprintf(Dest.Text, TEXT("%s"), Src.Text);
                break;
            case NMLT_ShortCut:
                Dest.Param0C = Src.A;
                Dest.Param08 = (signed char)Src.B;
                break;
            case NMLT_Delay:
                Dest.Param0C = Src.A;
                break;
        }
    }
    CoolTime.Stop();
}
NW_HOOK_METHOD(0x1009aeb0, NMacroEntry, Set)

// 0x1009aff0 - the lines' counters cleared, back to the first line, not running
void NMacroEntry::Reset()
{
    for (INT i = 0; i < NumLines; i++)
    {
        Lines[i].StartTime = 0;
        Lines[i].Clock = 0;
        Lines[i].Duration = 0;
    }
    CurLine = 0;
    bRunning = 0;
}
NW_HOOK_METHOD(0x1009aff0, NMacroEntry, Reset)

// 0x1009b030
void NMacroEntry::Select()
{
    bSelected = 1;
}
NW_HOOK_METHOD(0x1009b030, NMacroEntry, Select)

// 0x1009b040 - the icon texture
UTexture* NMacroEntry::GetIconTex()
{
    return IconTex;
}
NW_HOOK_METHOD(0x1009b040, NMacroEntry, GetIconTex)
