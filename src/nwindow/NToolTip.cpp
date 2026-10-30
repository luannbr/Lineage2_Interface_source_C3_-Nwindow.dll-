/*=============================================================================
    NToolTip.cpp - the C2 tool tips: NToolTipInfo (what a tool tip shows: the texts, the property lines,
    the numbers; built by SetText / Fill / FillMacro / FillMatchRow and measured by Layout) and
    NToolTipSkin (the nine textures every window gets in OnCreate and the two draw routines that paint a
    box and the text on it). Stock: 0x10002230 / 0x10002560 / 0x100022d0 / 0x1001aca0 (the object's
    members), 0x100019b0 .. 0x10028a70 (the line array's template code), 0x100273c0 .. 0x10029ef8 (the
    builders and the layout), 0x10029f00 .. 0x1002a4a1 (the skin).
=============================================================================*/
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NMacroEntry.h"

#define TIPSTR(ID) NCObject::SysString(ID)

// the sysstring ids the tips use (names ours)
enum
{
    TIPSTR_Weight = 52,
    TIPSTR_PAtk = 94,
    TIPSTR_PDef = 95,
    TIPSTR_Evasion = 97,
    TIPSTR_MAtk = 98,
    TIPSTR_MDef = 99,
    TIPSTR_AtkSpd = 111,
    TIPSTR_Level = 88,             // "Lv"
    TIPSTR_SkillSub = 91,          // " (<this>:N)" after a shortcut skill's level
    TIPSTR_ShieldRate = 317,       // "DEF Rate"
    TIPSTR_MPCost = 320,
    TIPSTR_Range = 321,
    TIPSTR_Price = 322,
    TIPSTR_MPBonus = 388,
    TIPSTR_SoulShots = 404,
    TIPSTR_Each = 468,             // "For Each"
    TIPSTR_Adena = 469,
    TIPSTR_SpiritShots = 496,
    TIPSTR_Count = 641,            // NTTO_Count's label
    TIPSTR_RecipeA = 736,          // a recipe's two numbers
    TIPSTR_RecipeB = 737,
    TIPSTR_Wanted = 808,           // the buy list's amount wanted
    TIPSTR_Classes1 = 670,         // the etc items' (henna) two lines
    TIPSTR_Classes2 = 671,
    TIPSTR_Rate = 744,
    TIPSTR_PetKind = 969,          // a pet collar's first line (970 / 971: its two values)
};

/*-----------------------------------------------------------------------------
    The line array's template code (TArray<NToolTipLine>): the stock module keeps these out of line.
-----------------------------------------------------------------------------*/

// 0x1002c4a0 - the line copied after the last one (the stock code is `Add(1, 0x418)` and a `rep movsd` of 0x106)
NW_HOOK_METHOD(0x1002c4a0, NToolTipLineArray, AddItem)

// 0x10002310 - the copy
NW_HOOK_CTOR_ARGS(0x10002310, NToolTipLineArray, (const NToolTipLineArray& A), (A))

// 0x1001d830 - the assignment
NW_HOOK_METHOD(0x1001d830, NToolTipLineArray, operator=)

// 0x100023c0 - the destructor (Remove(0, Num) with UnTemplate.h's check()s, then ~FArray)
NW_HOOK_DTOR(0x100023c0, NToolTipLineArray)

// 0x100019f0 - new(Array): room for a new last line
static void* NewToolTipLine(size_t Size, NToolTipLineArray& Array)
{
    return operator new(Size, Array);
}
NW_HOOK_FUNC(0x100019f0, NewToolTipLine)

/*-----------------------------------------------------------------------------
    NToolTipInfo: the object.
-----------------------------------------------------------------------------*/

// 0x100024f0 - C3: a new dword at +0x38 (Unknown38), cleared with the numbers
NToolTipInfo::NToolTipInfo()
    : Enchant(0), Grade(0), Count(0), Extra24(0), bShowCount(0), Type(-1), Level(0), ItemID(0), ShortCutType(0),
      Style(0), Options(0)
{
    FullBox.X = 0;
    FullBox.Y = 0;
    FullBox.W = 0;
    FullBox.H = 0;
    TitleBox.X = 0;
    TitleBox.Y = 0;
    TitleBox.W = 0;
    TitleBox.H = 0;
}
NW_HOOK_CTOR(0x100024f0, NToolTipInfo)

// 0x10002610 - the copy a by-value argument is made of: the strings, the numbers, the lines' array, the boxes
NToolTipInfo::NToolTipInfo(const NToolTipInfo& Src)
    : Name(Src.Name), Name2(Src.Name2), Enchant(Src.Enchant), Grade(Src.Grade), Count(Src.Count),
      Extra24(Src.Extra24), bShowCount(Src.bShowCount), Type(Src.Type), Level(Src.Level),
      ItemID(Src.ItemID), ShortCutType(Src.ShortCutType), Lines(Src.Lines), Description(Src.Description),
      Style(Src.Style), Options(Src.Options), FullBox(Src.FullBox), TitleBox(Src.TitleBox)
{
}
NW_HOOK_CTOR_ARGS(0x10002610, NToolTipInfo, (const NToolTipInfo& Src), (Src))

// 0x100025a0 - ~FString (Description), the lines' array, ~FString (Name2), ~FString (Name)
NToolTipInfo::~NToolTipInfo()
{
}
NW_HOOK_DTOR(0x100025a0, NToolTipInfo)

// 0x1001dd10 - the copy assignment (compiler-generated in the stock build, kept out of line): the strings,
// the numbers, the lines' array (0x1001d830), the boxes
NToolTipInfo& NToolTipInfo::operator=(const NToolTipInfo& Src)
{
    Name = Src.Name;
    Name2 = Src.Name2;
    Enchant = Src.Enchant;
    Grade = Src.Grade;
    Count = Src.Count;
    Extra24 = Src.Extra24;
    bShowCount = Src.bShowCount;
    Type = Src.Type;
    Level = Src.Level;
    ItemID = Src.ItemID;
    ShortCutType = Src.ShortCutType;
    Lines = Src.Lines;
    Description = Src.Description;
    Style = Src.Style;
    Options = Src.Options;
    FullBox = Src.FullBox;
    TitleBox = Src.TitleBox;
    return *this;
}
NW_HOOK_METHOD(0x1001dd10, NToolTipInfo, operator=)

/*-----------------------------------------------------------------------------
    NToolTipInfo: numbers.
-----------------------------------------------------------------------------*/

// the buffer of FormatNumber (0x101A5B40 in the stock image; only that routine touches it)
typedef TCHAR NNumberBuffer[32];
#if NW_HOOK_MODE
#define GNumberBuffer NW_GLOBAL(NNumberBuffer, 0x1026FE30)
#else
extern TCHAR GNumberBuffer[32];
#endif

// 0x1002ac00 - "%d" with a comma before each group of three digits, counted from the right: written backwards
// into the static buffer (a minus sign counts as a digit: -1234 gives "-,1234"... as the stock loop does)
const TCHAR* NToolTipInfo::FormatNumber(INT Value)
{
    TCHAR Text[1024];
    appSprintf(Text, TEXT("%d"), Value);
    INT Len = wcslen(Text);
    INT Pos = Len - 1;
    INT Digit = 0;
    INT End = Pos / 3 + Len;
    GNumberBuffer[End] = 0;
    if (Pos >= 0)
    {
        TCHAR* Dest = &GNumberBuffer[End - 1];
        do
        {
            if (Digit % 3 == 0 && Pos < (INT)wcslen(Text) - 1)
            {
                *Dest = ',';
                Dest--;
            }
            *Dest = Text[Pos];
            Pos--;
            Digit++;
            Dest--;
        } while (Pos >= 0);
    }
    return GNumberBuffer;
}
NW_HOOK_FUNC(0x1002ac00, NToolTipInfo::FormatNumber)

// 0x1002acb0 - light grey up to four digits (C3: 0xFFDCDCDC, C2 white), then by the digits beyond: pink, yellow,
// green, cyan, pink...
DWORD NToolTipInfo::NumberColor(const TCHAR* Text)
{
    DWORD Color = 0xFFDCDCDC;
    INT Len = appStrlen(Text);
    INT Commas = 0;
    for (INT i = 0; i < Len; i++)
    {
        if (Text[i] == ',')
            Commas++;
    }
    INT Digits = Len - Commas;
    if (Digits > 4)
    {
        switch ((Digits - 2) % 4)
        {
        case 0:
            return 0xFFFF80FF;
        case 1:
            return 0xFFFFFF00;
        case 2:
            return 0xFF00FF00;
        case 3:
            Color = 0xFF00FFFF;
        }
        return Color;
    }
    return 0xFFDCDCDC;
}
NW_HOOK_FUNC(0x1002acb0, NToolTipInfo::NumberColor)

// 0x1002ad40 - (name ours) the count is shown after the name: the tip's bShowCount is 1, 2 or 3 (C3: a mode, C2 a
// flag); out of line like the stock one, the layout and the title drawing call it
UBOOL NToolTipInfo::ShowsCount()
{
    if (bShowCount == 1 || bShowCount == 2 || bShowCount == 3)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1002ad40, NToolTipInfo, ShowsCount)

/*-----------------------------------------------------------------------------
    NToolTipInfo: the layout.
-----------------------------------------------------------------------------*/

// 0x1002ad60 - what the draw routines need: the size of the full box's text (FullBox) and of the title line's
// (TitleBox), by Style. The title line is the name (and "+N" and the additional name, the amount) and the grade's
// mark after it, the box holds it, the lines and the description wrapped at the box's width. C3: the boxes are FLOAT
// (as the windows' own), a line takes the font's height + 6 (C2: + 1), the amount is shown by ShowsCount, the
// shortcut's kind is ShortCutType, and the styles 0x1000 / 0x2000 / 0x8000 are new
void NToolTipInfo::Layout()
{
    SIZE Space;
    SIZE Ch;
    SIZE Size;
    SIZE LineSize;
    TCHAR Buf[1026];
    FString Tmp;

    NCGDevice::GetTextExtent(&Space, TEXT(" "), 0, 0, 0, 0, 0, L2FONT_Normal);
    NCGDevice::GetCharSize(&Ch, 0x41, 0);
    LineSize.cx = 0;
    LineSize.cy = 0;
    Size.cx = 0;
    Size.cy = 0;
    DWORD S = Style;
    if ((S & 0xC9) == 0 && (S & 0x2500) == 0)
    {
        if ((S & NTT_Item) == 0)
        {
            if (S & NTT_ShortCut)
            {
                INT T = ShortCutType;
                if (T == 1)
                {
                    if (Enchant > 0 && (Type == 0 || Type == 1 || Type == 3 || Type == 2))
                    {
                        appSprintf(Buf, TEXT("+%d "), Enchant);
                        Tmp = Buf;
                    }
                    if (Name2.Num() <= 1)
                        Tmp += Name;
                    else
                    {
                        appSprintf(Buf, TEXT("%s %s"), *Name, *Name2);
                        Tmp += Buf;
                    }
                    if (ShowsCount())
                    {
                        appSprintf(Buf, TEXT(" (%s)"), FormatNumber(Count));
                        Tmp += Buf;
                    }
                    NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
                    if (GConsole->GetGradeText(Grade))
                        Size.cx += 0xC + Space.cx;
                    FullBox.W = (FLOAT)Size.cx;
                    FullBox.H = (FLOAT)Ch.cy;
                }
                else
                {
                    if (T == 2)
                    {
                        if (Extra24 < 1)
                            appSprintf(Buf, TEXT("%s %s %d"), *Name, *TIPSTR(TIPSTR_Level), Level);
                        else
                            appSprintf(Buf, TEXT("%s %s %d (%s:%d)"), *Name, *TIPSTR(TIPSTR_Level), Level,
                                       *TIPSTR(TIPSTR_SkillSub), Extra24);
                        Tmp = Buf;
                    }
                    else
                    {
                        if (T != 3 && T != 4)
                            return;
                        Tmp = Name;
                    }
                    NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
                    FullBox.W = (FLOAT)Size.cx;
                    FullBox.H = (FLOAT)Ch.cy;
                }
                return;
            }
            if (S & (NTT_Skill | NTT_SkillName))
            {
                if (Level < 1)
                    Tmp = Name;
                else
                {
                    appSprintf(Buf, TEXT("%s %s %d"), *Name, *TIPSTR(TIPSTR_Level), Level);
                    Tmp = Buf;
                }
                NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
                if (Size.cx < 0x91)
                    FullBox.W = 144.0f;
                else
                    FullBox.W = (FLOAT)Size.cx;
                FullBox.H = (FLOAT)((Ch.cy + 6) * Lines.Num()) + (FLOAT)Ch.cy;
                if (Description.Num() > 1)
                {
                    GConsole->ConvertNewLines((TCHAR*)*Description, Tmp);
                    NCGDevice::GetTextExtent(&Size, *Tmp, (INT)FullBox.W, 0, 1, 0, 0, L2FONT_Normal);
                    FullBox.H = (FLOAT)(Size.cy + 6) + FullBox.H;
                }
                return;
            }
            if (S & (NTT_Desc | NTT_Unknown8000))
            {
                FullBox.W = 174.0f;
                FullBox.H = (FLOAT)((Ch.cy + 6) * Lines.Num()) + FullBox.H;
                if (Description.Num() > 1)
                {
                    GConsole->ConvertNewLines((TCHAR*)*Description, Tmp);
                    NCGDevice::GetTextExtent(&Size, *Tmp, (INT)FullBox.W, 0, 1, 0, 0, L2FONT_Normal);
                    FullBox.H = (FLOAT)(Size.cy + 6) + FullBox.H;
                }
                return;
            }
            if (S & NTT_Unknown1000)
            {
                TitleBox.W = 174.0f;
                TitleBox.H = (FLOAT)((Ch.cy + 6) * Lines.Num()) + TitleBox.H;
            }
            return;
        }
        // an item: the title with "+N" and the amount, the lines and the description under it
        if ((Options & NTTO_NoEnchant) == 0 && Enchant > 0 && (Type == 0 || Type == 1 || Type == 3 || Type == 2))
        {
            appSprintf(Buf, TEXT("+%d "), Enchant);
            Tmp = Buf;
        }
        if (Name2.Num() <= 1)
            Tmp += Name;
        else
        {
            appSprintf(Buf, TEXT("%s %s"), *Name, *Name2);
            Tmp += Buf;
        }
        if ((Options & NTTO_NoCount) == 0)
        {
            if (ShowsCount())
            {
                appSprintf(Buf, TEXT(" (%s)"), FormatNumber(Count));
                Tmp += Buf;
            }
        }
        NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
        if (GConsole->GetGradeText(Grade))
            Size.cx += 0xC + Space.cx;
        if (Size.cx < 0x91)
            FullBox.W = 144.0f;
        else
            FullBox.W = (FLOAT)Size.cx;
        FullBox.H = (FLOAT)((Ch.cy + 6) * Lines.Num()) + (FLOAT)Ch.cy;
        if (Description.Num() > 1)
        {
            GConsole->ConvertNewLines((TCHAR*)*Description, Tmp);
            NCGDevice::GetTextExtent(&Size, *Tmp, (INT)FullBox.W, 0, 1, 0, 0, L2FONT_Normal);
            FullBox.H = (FLOAT)(Size.cy + 6) + FullBox.H;
        }
        if (Lines.Num() > 0)
        {
            NToolTipLine& First = Lines(0);
            if (First.bLabel == 0)
                appSprintf(Buf, TEXT("%s"), First.Value);
            else
                appSprintf(Buf, TEXT("%s : %s"), First.Label, First.Value);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0, L2FONT_Normal);
        }
        if ((INT)FullBox.W < Size.cx)
            FullBox.W = (FLOAT)Size.cx;
        Tmp.Empty();
        // the same title again, for the title line's box
        if ((Options & NTTO_NoEnchant) == 0 && Enchant > 0 && (Type == 0 || Type == 1 || Type == 3 || Type == 2))
        {
            appSprintf(Buf, TEXT("+%d "), Enchant);
            Tmp = Buf;
        }
        if (Name2.Num() <= 1)
            Tmp += Name;
        else
        {
            appSprintf(Buf, TEXT("%s %s"), *Name, *Name2);
            Tmp += Buf;
        }
        if ((Options & NTTO_NoCount) == 0)
        {
            if (ShowsCount())
            {
                appSprintf(Buf, TEXT(" (%s)"), FormatNumber(Count));
                Tmp += Buf;
            }
        }
        NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
        if (GConsole->GetGradeText(Grade))
            Size.cx += 0xC + Space.cx;
        TitleBox.W = (FLOAT)Size.cx;
        TitleBox.H = (FLOAT)Ch.cy;
        if ((Options & (NTTO_Price | NTTO_PriceEach)) == 0)
            return;
        // the price line (the first one) is drawn under the name: the box grows by it
        if (Lines.Num() > 0)
        {
            appSprintf(Buf, TEXT("%s : %s"), Lines(0).Label, Lines(0).Value);
            NCGDevice::GetTextExtent(&LineSize, Buf, 0, 0, 0, 0, 0, L2FONT_Normal);
        }
        if ((INT)TitleBox.W < LineSize.cx)
            TitleBox.W = (FLOAT)LineSize.cx;
        TitleBox.H = (FLOAT)(Ch.cy + 6) + TitleBox.H;
    }
    else
    {
        // a name, an additional name and a description only
        if (Name2.Num() <= 1)
            Tmp = Name;
        else
        {
            appSprintf(Buf, TEXT("%s %s"), *Name, *Name2);
            Tmp = Buf;
        }
        NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
        if (GConsole->GetGradeText(Grade))
            Size.cx += 0xC + Space.cx;
        if (Size.cx < 0x91)
            FullBox.W = 144.0f;
        else
            FullBox.W = (FLOAT)Size.cx;
        FullBox.H = (FLOAT)((Ch.cy + 6) * Lines.Num()) + (FLOAT)Ch.cy;
        if (Description.Num() > 1)
        {
            GConsole->ConvertNewLines((TCHAR*)*Description, Tmp);
            NCGDevice::GetTextExtent(&Size, *Tmp, (INT)FullBox.W, 0, 1, 0, 0, L2FONT_Normal);
            FullBox.H = (FLOAT)(Size.cy + 6) + FullBox.H;
        }
        Tmp.Empty();
        if (Name2.Num() <= 1)
            Tmp += Name;
        else
        {
            appSprintf(Buf, TEXT("%s %s"), *Name, *Name2);
            Tmp += Buf;
        }
        if (bShowCount == 1 || bShowCount == 2 || bShowCount == 3)
        {
            appSprintf(Buf, TEXT(" (%s)"), FormatNumber(Count));
            Tmp += Buf;
        }
        NCGDevice::GetTextExtent(&Size, *Tmp, 0, 0, 0, 0, 0, L2FONT_Normal);
        if (GConsole->GetGradeText(Grade))
            Size.cx += 0xC;
        TitleBox.W = (FLOAT)Size.cx;
        TitleBox.H = (FLOAT)Ch.cy;
    }
}
NW_HOOK_METHOD(0x1002ad60, NToolTipInfo, Layout)

/*-----------------------------------------------------------------------------
    NToolTipInfo: the builders.
-----------------------------------------------------------------------------*/

// 0x1002c4f0 - back to empty: the names, the lines, the description and the numbers (not Options, not ShortCutType)
void NToolTipInfo::Clear()
{
    Name.Empty();
    Name2.Empty();
    Style = 0;
    Grade = 0;
    Enchant = 0;
    Count = 0;
    Extra24 = 0;
    bShowCount = 0;
    Type = -1;
    Level = 0;
    ItemID = 0;
    Lines.Empty();
    Description.Empty();
    FullBox.H = 0;
    FullBox.W = 0;
    TitleBox.H = 0;
    TitleBox.W = 0;
}
NW_HOOK_METHOD(0x1002c4f0, NToolTipInfo, Clear)

// 0x1002c4d0 - the text and its Style, then the layout (NCButton::SetToolTip passes NTT_Plain)
void NToolTipInfo::SetText(const TCHAR* InText, INT InStyle)
{
    Name = InText;
    Style = InStyle;
    Layout();
}
NW_HOOK_METHOD(0x1002c4d0, NToolTipInfo, SetText)

// 0x1002d7f0 (ret 0xC, name ours) - the buff icons' tip: a name and a description (NCStatusWnd passes NTT_Text)
void NToolTipInfo::SetNameDesc(const TCHAR* InName, const TCHAR* InDesc, INT InStyle)
{
    if (InName || InDesc)
    {
        Clear();
        Style = InStyle;
        if (InName)
            Name = InName;
        if (InDesc)
            Description = InDesc;
        Layout();
    }
}
NW_HOOK_METHOD(0x1002d7f0, NToolTipInfo, SetNameDesc)

// 0x1002d7a0 - a macro's tip: Macro is the macro object (an NMacroEntry, which starts with its own NToolTipInfo): its
// name and description (NULL when empty: the getters) fill this one
void NToolTipInfo::FillMacro(NToolTipInfo* Macro, DWORD InStyle)
{
    if (Macro != NULL)
    {
        Clear();
        Style = InStyle;
        Name = ((NMacroEntry*)Macro)->GetName();
        Description = ((NMacroEntry*)Macro)->GetDescription();
        Layout();
    }
}
NW_HOOK_METHOD(0x1002d7a0, NToolTipInfo, FillMacro)

// 0x1002d840 - a party matching row: the level (its second cell's number), the class (its first cell's), the purpose
// of the party by the row's third number; the row's text is the description. C3: the style 0x8000 is a single line
// with the class of the row's second number (the party member's)
void NToolTipInfo::FillMatchRow(NListRow* Row, DWORD InStyle)
{
    if (Row == NULL)
        return;
    Clear();
    Style = InStyle;
    NToolTipLine Line;
    if (InStyle == NTT_Unknown8000)
    {
        appStrcpy(Line.Label, *TIPSTR(391));
        appStrcpy(Line.Value, GConsole->GetClassNameString(Row->Param2));
        Lines.AddItem(Line);
        Layout();
        return;
    }
    appStrcpy(Line.Label, *TIPSTR(392));
    if (Row->Param1 == 0)
        appStrcpy(Line.Value, *TIPSTR(411));
    else
        appStrcpy(Line.Value, appItoa(Row->Cells[1].Param2));
    Lines.AddItem(Line);
    appStrcpy(Line.Label, *TIPSTR(391));
    if (Row->Param2 == 0)
        appStrcpy(Line.Value, *TIPSTR(411));
    else
        appStrcpy(Line.Value, GConsole->GetClassNameString(Row->Cells[0].Param2));
    Lines.AddItem(Line);
    INT Purpose = Row->Param3;
    FString* Text = NULL;
    if (Purpose == 0)
    {
        appStrcpy(Line.Label, *TIPSTR(489));
        Text = &TIPSTR(487);
    }
    else if (Purpose == 1)
    {
        appStrcpy(Line.Label, *TIPSTR(489));
        Text = &TIPSTR(488);
    }
    else if (Purpose == 2)
    {
        appStrcpy(Line.Label, *TIPSTR(489));
        Text = &TIPSTR(798);
    }
    else if (Purpose == 3)
    {
        appStrcpy(Line.Label, *TIPSTR(489));
        Text = &TIPSTR(799);
    }
    else if (Purpose == 4)
    {
        appStrcpy(Line.Label, *TIPSTR(489));
        Text = &TIPSTR(800);
    }
    if (Text != NULL)
    {
        appStrcpy(Line.Value, **Text);
        Lines.AddItem(Line);
    }
    Description = Row->Text;
    Layout();
}
NW_HOOK_METHOD(0x1002d840, NToolTipInfo, FillMatchRow)

// 0x1002c650 (ret 0xc) - the tip of an item entry for a Style and Options: the names and numbers the Style needs, the
// price line(s) the Options ask for, then the property lines of the item (by its Type), the skill (type, MP cost, range)
// or the recipe, and the description; ends with the layout. C3: a shortcut slot also keeps its kind (ShortCutType) and
// the entry's trade count is 64 bits; the weapons' attack numbers come from the console's two functions
// (0x1006a5d0 / 0x1006a670, which take the weapon type and the body part too); the etc items' kinds are 7 (a pet
// collar: its type and level), 0xD / 0xE (the henna-like ones: C2's 0xC / 0xD) and 0xF (a level)
void NToolTipInfo::Fill(NItemEntry* Item, DWORD InStyle, DWORD InOptions)
{
    if (Item == NULL)
        return;
    Clear();
    Style = InStyle;
    Options = InOptions;
    if (InStyle & NTT_ItemName)
    {
        Name = Item->GetName();
        Name2 = Item->GetAdditionalName();
        Grade = Item->CrystalGrade;
    }
    else if (InStyle & NTT_Item)
    {
        Name = Item->GetName();
        Name2 = Item->GetAdditionalName();
        Enchant = Item->Enchant;
        Grade = Item->CrystalGrade;
        if (Item->TradeCount > 0)
            Count = (INT)Item->TradeCount;
        else
            Count = Item->Count;
        bShowCount = Item->bStackable;
        Type = Item->Type;
        ItemID = Item->ID;
    }
    else if (InStyle & (NTT_Skill | NTT_SkillName))
    {
        Name = Item->GetName();
        Level = Item->UnknownC18;
    }
    else if (InStyle & (NTT_Action | NTT_Name))
        Name = Item->GetName();
    else if (InStyle & NTT_ShortCut)
    {
        Type = Item->Type;
        if (Item->GetName() != NULL)
            Name = Item->GetName();
        if (Item->GetAdditionalName() != NULL)
            Name2 = Item->GetAdditionalName();
        Enchant = Item->Enchant;
        Grade = Item->CrystalGrade;
        Count = Item->Count;
        Extra24 = ::nw::Field<INT>(Item, 0x1A90);   // the shortcut slot's entry is bigger than NItemEntry
        bShowCount = Item->bStackable;
        Level = Item->UnknownC18;
        ShortCutType = Item->Unknown1910;
    }
    else if (InStyle & NTT_Recipe)
    {
        Name = Item->GetName();
        Name2 = Item->GetAdditionalName();
        Enchant = Item->Enchant;
        Grade = Item->CrystalGrade;
        Count = Item->Count;
        bShowCount = Item->bStackable;
    }

    // the price lines the Options ask for: the first lines of the tip
    NToolTipLine PriceLine;
    if (Options & NTTO_Price)
    {
        appStrcpy(PriceLine.Label, *TIPSTR(TIPSTR_Price));
        appSprintf(PriceLine.Value, TEXT("%s %s"), FormatNumber(Item->UnknownC20), *TIPSTR(TIPSTR_Adena));
        PriceLine.ValueColor = NumberColor(FormatNumber(Item->UnknownC20));
        Lines.AddItem(PriceLine);
    }
    if (Options & NTTO_PriceEach)
    {
        appStrcpy(PriceLine.Label, *TIPSTR(TIPSTR_Price));
        appSprintf(PriceLine.Value, TEXT("%s %s %s"), *TIPSTR(TIPSTR_Each), FormatNumber(Item->UnknownC20),
                   *TIPSTR(TIPSTR_Adena));
        PriceLine.ValueColor = NumberColor(FormatNumber(Item->UnknownC20));
        Lines.AddItem(PriceLine);
    }
    if (Options & NTTO_Count)
    {
        appStrcpy(PriceLine.Label, *TIPSTR(TIPSTR_Count));
        appStrcpy(PriceLine.Value, FormatNumber(Item->UnknownC20));
        PriceLine.ValueColor = NumberColor(PriceLine.Value);
        Lines.AddItem(PriceLine);
    }

    // the property lines
    NToolTipLine Line;
    DWORD S = Style;
    if (S & NTT_ItemName)
    {
        appStrcpy(Line.Label, *TIPSTR(TIPSTR_Weight));
        appStrcpy(Line.Value, appItoa(Item->Weight));
        Lines.AddItem(Line);
    }
    else if (S & NTT_Item)
    {
        if ((S & NTT_Wanted) && Item->IsStackable() && Item->UnknownC34 > 0)
        {
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_Wanted));
            appStrcpy(Line.Value, appItoa(Item->UnknownC34));
            Lines.AddItem(Line);
        }
        FString Slot;
        GConsole->GetItemSlotText(Item->Type, Item->BodyPart, Slot, Item->SubType);
        INT T = Item->Type;
        UBOOL bPending = 0;     // Line holds a line to add before the weight
        UBOOL bWeight = 1;      // the weight line follows
        if (T == 0)
        {
            // a weapon: type / slot, P.Atk, M.Atk, attack speed, soulshots, spiritshots
            if (GConsole->GetWeaponTypeName(Item->UnknownC30) != NULL)
            {
                appSprintf(Line.Value, TEXT("%s / %s"), GConsole->GetWeaponTypeName(Item->UnknownC30), *Slot);
                Line.bLabel = 0;
                Lines.AddItem(Line);
                Line.bLabel = 1;
            }
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_PAtk));
            appStrcpy(Line.Value, appItoa(((NConsoleLeftFns*)GConsole)->GetEnchantedPAtk(Item->UnknownC30, Item->BodyPart,
                                                                  Item->CrystalGrade, Item->Enchant, Item->Unknown1C80)));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_MAtk));
            appStrcpy(Line.Value, appItoa(((NConsoleLeftFns*)GConsole)->GetEnchantedMAtk(Item->UnknownC30, Item->BodyPart,
                                                                  Item->CrystalGrade, Item->Enchant, Item->Unknown1C84)));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_AtkSpd));
            appStrcpy(Line.Value, GConsole->GetAttackSpeedText(Item->Unknown1C6C));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_SoulShots));
            appSprintf(Line.Value, TEXT("X %d"), Item->Unknown1CA0);
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_SpiritShots));
            appSprintf(Line.Value, TEXT("X %d"), Item->Unknown1CA4);
            bPending = 1;
        }
        else if (T == 1 && ((Item->BodyPart & 0x100) != 0 || (Item->BodyPart & 0x80) != 0))
        {
            // a shield: P.Def, the shield rate, the evasion (no slot line)
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_PDef));
            appStrcpy(Line.Value, appItoa(GConsole->GetEnchantedDefense(Item->CrystalGrade, Item->Enchant,
                                                                        Item->Unknown1C90)));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_ShieldRate));
            appStrcpy(Line.Value, appItoa(Item->Unknown1C94));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_Evasion));
            appStrcpy(Line.Value, appItoa(Item->Unknown1C78));
            bPending = 1;
        }
        else if (T == 1 && GConsole->IsMagicArmor(Item->ID))
        {
            // a robe: slot, MP bonus, P.Def
            appStrcpy(Line.Value, *Slot);
            Line.bLabel = 0;
            Lines.AddItem(Line);
            Line.bLabel = 1;
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_MPBonus));
            appStrcpy(Line.Value, appItoa(Item->Unknown1C9C));
            Lines.AddItem(Line);
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_PDef));
            appStrcpy(Line.Value, appItoa(GConsole->GetEnchantedDefense(Item->CrystalGrade, Item->Enchant,
                                                                        Item->Unknown1C88)));
            bPending = 1;
        }
        else if (T == 1)
        {
            // other armor: slot, P.Def
            appStrcpy(Line.Value, *Slot);
            Line.bLabel = 0;
            Lines.AddItem(Line);
            Line.bLabel = 1;
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_PDef));
            appStrcpy(Line.Value, appItoa(GConsole->GetEnchantedDefense(Item->CrystalGrade, Item->Enchant,
                                                                        Item->Unknown1C88)));
            bPending = 1;
        }
        else if (T == 2)
        {
            // jewelry: slot, M.Def
            appStrcpy(Line.Value, *Slot);
            Line.bLabel = 0;
            Lines.AddItem(Line);
            Line.bLabel = 1;
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_MDef));
            appStrcpy(Line.Value, appItoa(GConsole->GetEnchantedDefense(Item->CrystalGrade, Item->Enchant,
                                                                        Item->Unknown1C8C)));
            bPending = 1;
        }
        else if (T == 3)
        {
            // the slot only (no weight)
            appStrcpy(Line.Value, *Slot);
            Line.bLabel = 0;
            Lines.AddItem(Line);
            Line.bLabel = 1;
            bWeight = 0;
        }
        else if (T == 5)
        {
            INT Kind = Item->Unknown1910;
            if (Kind == 7)
            {
                // a pet collar: what it is (the value of the first line stays what it was when UnknownC7C is neither 0
                // nor 1), then the pet's level
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_PetKind));
                if (Item->Unknown1C7C == 0)
                    appStrcpy(Line.Value, *TIPSTR(971));
                else if (Item->Unknown1C7C == 1)
                    appStrcpy(Line.Value, *TIPSTR(970));
                Lines.AddItem(Line);
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Level));
                appStrcpy(Line.Value, appItoa(Item->Enchant));
                bPending = 1;
            }
            else if (Kind == 0xF)
            {
                appStrcpy(Line.Label, *TIPSTR(972));
                appStrcpy(Line.Value, appItoa(Item->Enchant));
                bPending = 1;
            }
            else if (Kind == 0xD)
            {
                // a henna-like item: a number and the list of the classes (bits of two words)
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Classes1));
                appStrcpy(Line.Value, appItoa((WORD)Item->UnknownC3C));
                Lines.AddItem(Line);
                Line.Value[0] = 0;
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Classes2));
                TArray<INT> List;
                for (INT i = 0; i < 16; i++)
                {
                    if (Item->Enchant & (1 << i))
                        List.AddItem(i + 1);
                }
                for (INT i = 0; i < 16; i++)
                {
                    if (Item->Unknown1C7C & (1 << i))
                        List.AddItem(i + 0x11);
                }
                for (INT i = 0; i < List.Num(); i++)
                {
                    appStrcat(Line.Value, appItoa(List(i)));
                    if (i < List.Num() - 1)
                        appStrcat(Line.Value, TEXT(","));
                }
                List.Empty();
                Lines.AddItem(Line);
            }
            else if (Kind == 0xE)
            {
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Classes1));
                appStrcpy(Line.Value, appItoa(Item->Enchant));
                Lines.AddItem(Line);
                Line.Value[0] = 0;
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Classes2));
                WORD Bits = (WORD)Item->UnknownC3C;
                INT High = Bits >> 4;
                if ((Bits & 0xF) > 0)
                    appStrcat(Line.Value, appItoa(Bits & 0xF));
                if (High > 0)
                {
                    appStrcat(Line.Value, TEXT(","));
                    appStrcat(Line.Value, appItoa(High));
                }
                Lines.AddItem(Line);
                appStrcpy(Line.Label, *TIPSTR(TIPSTR_Rate));
                appStrcpy(Line.Value, appItoa(Item->Unknown1C7C * 100));
                bPending = 1;
            }
        }
        else
            bWeight = 0;
        if (bPending)
            Lines.AddItem(Line);
        if (bWeight)
        {
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_Weight));
            appStrcpy(Line.Value, appItoa(Item->Weight));
            Lines.AddItem(Line);
        }
    }
    else if (S & NTT_Skill)
    {
        // the skill's type (no label), MP cost, range
        if (GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18) != NULL)
        {
            appStrcpy(Line.Value, GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18));
            Line.bLabel = 0;
            Lines.AddItem(Line);
            Line.bLabel = 1;
        }
        if (GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18) != 0)
        {
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_MPCost));
            appStrcpy(Line.Value, appItoa(GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18)));
            Lines.AddItem(Line);
        }
        if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18) >= 0)
        {
            appStrcpy(Line.Label, *TIPSTR(TIPSTR_Range));
            appStrcpy(Line.Value, appItoa(GConsole->GetSkillRange(Item->ID, Item->UnknownC18)));
            Lines.AddItem(Line);
        }
    }
    else if (S & NTT_Recipe)
    {
        appStrcpy(Line.Label, *TIPSTR(TIPSTR_RecipeA));
        appStrcpy(Line.Value, appItoa(Item->UnknownC34));
        Lines.AddItem(Line);
        appStrcpy(Line.Label, *TIPSTR(TIPSTR_RecipeB));
        appStrcpy(Line.Value, appItoa(Item->Count));
        Lines.AddItem(Line);
    }
    Description = Item->GetDescription();
    Layout();
}
NW_HOOK_METHOD(0x1002c650, NToolTipInfo, Fill)

/*-----------------------------------------------------------------------------
    NToolTipSkin.
-----------------------------------------------------------------------------*/

// 0x1002dba0 - the nine texture slots, then the info
NToolTipSkin::NToolTipSkin()
{
    for (INT i = 0; i < 9; i++)
        Tex[i] = NULL;
}
NW_HOOK_CTOR(0x1002dba0, NToolTipSkin)

// 0x1002dbc0 - the arrays and strings are emptied by hand, then the members go (0x100025a0)
NToolTipSkin::~NToolTipSkin()
{
    Info.Lines.Empty();
    Info.Name.Empty();
    Info.Name2.Empty();
    Info.Description.Empty();
}
NW_HOOK_DTOR(0x1002dbc0, NToolTipSkin)

// DrawText as the tip routines call it: the font 0 and nothing else set (P5 is the width to wrap at, P7 1 for the
// description's box (C3), P9 1 for the grade's mark)
static FORCEINLINE DWORD DrawTipText(NCGDevice* Device, INT X, INT Y, DWORD Color, const TCHAR* Text,
                                     DWORD P5 = 0, INT P9 = 0, DWORD P7 = 0)
{
    return Device->DrawText(X, Y, Color, Text, P5, 0, P7, L2FONT_Normal, P9, 0, 0, 0, 0, 0, 0);
}

// the clamp of the box to the screen and the nine-slice, inlined in both draw routines: X / Y come in as the
// position of the thing the tip is about (relative to the device's origin, the window's corner), the box goes above-left
// of it moved to stay on the screen (C3: the screen is the desktop's local rectangle, a FLOAT size; the tip that does
// not fit above is put Below pixels under the point: 0 for the title box, 32 for the full one); the clip is left set
// (the callers release it). Returns the box's corner in *DrawX / *DrawY (relative to the origin)
static FORCEINLINE void NToolTipFrame(NToolTipSkin* Skin, NCGDevice* Device, INT X, INT Y, const NToolTipRect& Wnd, INT W,
                                      INT H, INT Below, const NRectF& Screen, INT* DrawX, INT* DrawY)
{
    INT Right = Wnd.X + W + X;
    if (Right > (INT)Screen.W)
        X = X - (INT)((FLOAT)Right - Screen.W);
    else if (Wnd.X + X < 0)
        X = -Wnd.X;
    INT Top = Y - H;
    if (Wnd.Y + Top < 0)
        Top = Top + H + Below;
    Device->SetClip(Wnd.X + X, Wnd.Y + Top, W, H);
    if (Skin->Tex[0])
        Device->DrawTexture(X, Top, 8, 8, 0, 0, 8, 8, Skin->Tex[0], 0xFF, 1);
    if (Skin->Tex[1])
        Device->DrawStretchedTex(X + 8, Top, W - 16, 8, Skin->Tex[1], 1, 0xFF);
    if (Skin->Tex[2])
        Device->DrawTexture(X + W - 8, Top, 8, 8, 0, 0, 8, 8, Skin->Tex[2], 0xFF, 1);
    if (Skin->Tex[3])
        Device->DrawStretchedTex(X, Top + 8, 8, H - 16, Skin->Tex[3], 1, 0xFF);
    if (Skin->Tex[4])
        Device->DrawStretchedTex(X + 8, Top + 8, W - 16, H - 16, Skin->Tex[4], 1, 0xFF);
    if (Skin->Tex[5])
        Device->DrawStretchedTex(X + W - 8, Top + 8, 8, H - 16, Skin->Tex[5], 1, 0xFF);
    if (Skin->Tex[6])
        Device->DrawTexture(X, Top + H - 8, 8, 8, 0, 0, 8, 8, Skin->Tex[6], 0xFF, 1);
    if (Skin->Tex[7])
        Device->DrawStretchedTex(X + 8, Top + H - 8, W - 16, 8, Skin->Tex[7], 1, 0xFF);
    if (Skin->Tex[8])
        Device->DrawTexture(X + W - 8, Top + H - 8, 8, 8, 0, 0, 8, 8, Skin->Tex[8], 0xFF, 1);
    *DrawX = X;
    *DrawY = Top;
}

// 0x1002df40 (ret 0x98) - C3: X and Y come by value. Nothing is drawn for a tip with no name and no lines; the info is
// stored (0x1001dd10), the box is the title line's size + 10, the text goes 5 inside
void NToolTipSkin::Draw(NCGDevice* Device, NToolTipInfo InInfo, INT X, INT Y, NToolTipRect Wnd)
{
    if ((InInfo.Name.Num() == 0 || InInfo.Name.Num() - 1 <= 0) && InInfo.Lines.Num() <= 0)
        return;
    NRectF Screen;
    Screen.X = 0;
    Screen.Y = 0;
    Screen.W = 0.0f;
    Screen.H = 0.0f;
    NCWnd* Desktop = GConsole->Desktop;
    Desktop->GetLocalRect(&Screen);
    Info = InInfo;
    INT W = (INT)(InInfo.TitleBox.W + 10.0f);
    INT H = (INT)(InInfo.TitleBox.H + 10.0f);
    INT DrawX, DrawY;
    NToolTipFrame(this, Device, X, Y, Wnd, W, H, 0, Screen, &DrawX, &DrawY);
    DrawTitleText(Device, DrawX + 5, DrawY + 5);
    Device->ReleaseClip();
}
NW_HOOK_METHOD(0x1002df40, NToolTipSkin, Draw)

// 0x1002dc40 (ret 0x98) - the same with the full box (a size that is not above 0 stays 0); a tip that does not fit
// above the point goes 32 pixels under it
void NToolTipSkin::DrawFull(NCGDevice* Device, NToolTipInfo InInfo, INT X, INT Y, NToolTipRect Wnd)
{
    NRectF Screen;
    Screen.X = 0;
    Screen.Y = 0;
    Screen.W = 0.0f;
    Screen.H = 0.0f;
    NCWnd* Desktop = GConsole->Desktop;
    Desktop->GetLocalRect(&Screen);
    Info = InInfo;
    INT W = 0;
    INT H = 0;
    if (InInfo.FullBox.W > 0.0f)
        W = (INT)(InInfo.FullBox.W + 10.0f);
    if (InInfo.FullBox.H > 0.0f)
        H = (INT)(InInfo.FullBox.H + 10.0f);
    INT DrawX, DrawY;
    NToolTipFrame(this, Device, X, Y, Wnd, W, H, 32, Screen, &DrawX, &DrawY);
    DrawFullText(Device, DrawX + 5, DrawY + 5);
    Device->ReleaseClip();
}
NW_HOOK_METHOD(0x1002dc40, NToolTipSkin, DrawFull)

// 0x1002c0f0 - the title line: "+N " in orange, the name, the additional name in yellow, the grade's mark, " (count)";
// then, with the 0x1000 style, the lines with their icons; with a price in the Options, the first line under it. C3:
// the text is light grey (0xFFDCDCDC), the lines are 6 apart (C2: 1)
void NToolTipSkin::DrawTitleText(NCGDevice* Device, INT X, INT Y)
{
    SIZE Ch;
    TCHAR Buf[1026];
    NToolTipLine Line;
    NCGDevice::GetCharSize(&Ch, 0x41, 0);
    INT CX = X;
    if ((Info.Options & NTTO_NoEnchant) == 0 && Info.Enchant > 0 &&
        (Info.Type == 0 || Info.Type == 1 || Info.Type == 3 || Info.Type == 2))
    {
        appSprintf(Buf, TEXT("+%d "), Info.Enchant);
        CX = DrawTipText(Device, X, Y, 0xFFB09B79, Buf) + X;
    }
    CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, *Info.Name);
    if (Info.Name2.Num() > 1)
    {
        appSprintf(Buf, TEXT(" %s"), *Info.Name2);
        CX += DrawTipText(Device, CX, Y, 0xFFFFD969, Buf);
    }
    if (GConsole->GetGradeText(Info.Grade) != NULL)
    {
        appSprintf(Buf, TEXT(" `%s`"), GConsole->GetGradeText(Info.Grade));
        CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf, 0, 1);
    }
    if ((Info.Options & NTTO_NoCount) == 0 && (Info.bShowCount == 1 || Info.bShowCount == 2 || Info.bShowCount == 3))
    {
        appSprintf(Buf, TEXT(" (%s)"), NToolTipInfo::FormatNumber(Info.Count));
        DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf);
    }
    if (Info.Style & NTT_Unknown1000)
    {
        // a list: the line's own texture at its natural size, then its label 5 px after it
        INT LineY = Y;
        for (INT i = 0; i < Info.Lines.Num(); i++)
        {
            UTexture* Icon = Info.Lines(i).Icon;
            INT IconW = Icon->USize;
            Device->DrawStretchedTex(X, LineY, IconW, Icon->VSize, Icon, 0, 0xFF);
            DrawTipText(Device, X + 5 + IconW, LineY, 0xFFDCDCDC, Info.Lines(i).Label);
            LineY += 6 + Ch.cy;
        }
    }
    if ((Info.Options & (NTTO_Price | NTTO_PriceEach)) != 0 && Info.Lines.Num() > 0)
    {
        Line = Info.Lines(0);
        INT LineY = Y + Ch.cy + 6;
        INT LX = X;
        if (Line.bLabel != 0)
        {
            INT W = DrawTipText(Device, X, LineY, 0xFFA3A3A3, Line.Label);
            if (Line.bColon == 0)
                appStrcpy(Buf, TEXT(" "));
            else
                appStrcpy(Buf, TEXT(" : "));
            LX = W + X + DrawTipText(Device, W + X, LineY, 0xFFDCDCDC, Buf);
        }
        DrawTipText(Device, LX, LineY, Line.ValueColor, Line.Value);
    }
}
NW_HOOK_METHOD(0x1002c0f0, NToolTipSkin, DrawTitleText)

// 0x1002b800 - the full box's text: the title by Style (and ShortCutType, for a shortcut), the lines (label grey,
// " : " light grey, value in its color) and the description (wrapped at the box's width) under them
void NToolTipSkin::DrawFullText(NCGDevice* Device, INT X, INT Y)
{
    SIZE Ch;
    TCHAR Buf[1024];
    FString Tmp;
    INT CurY = Y;
    NCGDevice::GetCharSize(&Ch, 0x41, 0);
    DWORD S = Info.Style;
    if ((S & 0xC9) == 0 && (S & 0x2100) == 0)
    {
        if ((S & NTT_Item) == 0)
        {
            if ((S & (NTT_Skill | NTT_SkillName)) == 0)
            {
                if ((S & NTT_ShortCut) != 0)
                {
                    INT T = Info.ShortCutType;
                    if (T == 1)
                    {
                        INT CX = X;
                        if (Info.Enchant > 0 && (Info.Type == 0 || Info.Type == 1 || Info.Type == 3 || Info.Type == 2))
                        {
                            appSprintf(Buf, TEXT("+%d "), Info.Enchant);
                            CX = DrawTipText(Device, X, Y, 0xFFB09B79, Buf) + X;
                        }
                        CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, *Info.Name);
                        if (Info.Name2.Num() > 1)
                        {
                            appSprintf(Buf, TEXT(" %s"), *Info.Name2);
                            CX += DrawTipText(Device, CX, Y, 0xFFFFD969, Buf);
                        }
                        if (GConsole->GetGradeText(Info.Grade) != NULL)
                        {
                            appSprintf(Buf, TEXT(" `%s`"), GConsole->GetGradeText(Info.Grade));
                            CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf, 0, 1);
                        }
                        if (Info.ShowsCount())
                        {
                            appSprintf(Buf, TEXT(" (%s)"), NToolTipInfo::FormatNumber(Info.Count));
                            DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf);
                        }
                    }
                    else if (T == 2)
                    {
                        INT W1 = DrawTipText(Device, X, Y, 0xFFDCDCDC, *Info.Name);
                        appSprintf(Buf, TEXT(" %s "), *TIPSTR(TIPSTR_Level));
                        INT W2 = DrawTipText(Device, W1 + X, Y, 0xFFA3A3A3, Buf);
                        INT CX = W1 + X + W2;
                        INT W3 = DrawTipText(Device, CX, Y, 0xFFB09B79, appItoa(Info.Level));
                        if (Info.Extra24 > 0)
                        {
                            appSprintf(Buf, TEXT(" (%s:%d)"), *TIPSTR(TIPSTR_SkillSub), Info.Extra24);
                            DrawTipText(Device, CX + W3, Y, 0xFFDCDCDC, Buf);
                        }
                    }
                    else if (T == 3 || T == 4)
                        DrawTipText(Device, X, Y, 0xFFDCDCDC, *Info.Name);
                }
            }
            else
            {
                INT W1 = DrawTipText(Device, X, Y, 0xFFDCDCDC, *Info.Name);
                if (Info.Level > 0)
                {
                    appSprintf(Buf, TEXT(" %s "), *TIPSTR(TIPSTR_Level));
                    INT W2 = DrawTipText(Device, W1 + X, Y, 0xFFA3A3A3, Buf);
                    DrawTipText(Device, W1 + X + W2, Y, 0xFFB09B79, appItoa(Info.Level));
                }
                CurY = Ch.cy + Y;
            }
        }
        else
        {
            INT CX = X;
            if (Info.Enchant > 0 && (Info.Type == 0 || Info.Type == 1 || Info.Type == 3 || Info.Type == 2))
            {
                appSprintf(Buf, TEXT("+%d "), Info.Enchant);
                CX = DrawTipText(Device, X, Y, 0xFFB09B79, Buf) + X;
            }
            CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, *Info.Name);
            if (Info.Name2.Num() > 1)
            {
                appSprintf(Buf, TEXT(" %s"), *Info.Name2);
                CX += DrawTipText(Device, CX, Y, 0xFFFFD969, Buf);
            }
            if (GConsole->GetGradeText(Info.Grade) != NULL)
            {
                appSprintf(Buf, TEXT(" `%s`"), GConsole->GetGradeText(Info.Grade));
                CX += DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf, 0, 1);
            }
            if ((Info.Options & NTTO_NoCount) == 0)
            {
                if (Info.ShowsCount())
                {
                    appSprintf(Buf, TEXT(" (%s)"), NToolTipInfo::FormatNumber(Info.Count));
                    DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf);
                }
            }
            CurY = Ch.cy + Y;
        }
    }
    else
    {
        INT CX = DrawTipText(Device, X, Y, 0xFFDCDCDC, *Info.Name) + X;
        if (Info.Name2.Num() > 1)
        {
            appSprintf(Buf, TEXT(" %s"), *Info.Name2);
            CX += DrawTipText(Device, CX, Y, 0xFFFFD969, Buf);
        }
        if (GConsole->GetGradeText(Info.Grade) != NULL)
        {
            appSprintf(Buf, TEXT(" `%s`"), GConsole->GetGradeText(Info.Grade));
            DrawTipText(Device, CX, Y, 0xFFDCDCDC, Buf, 0, 1);
        }
        CurY = Ch.cy + Y;
    }
    for (INT i = 0; i < Info.Lines.Num(); i++)
    {
        CurY += 6;
        NToolTipLine Line = Info.Lines(i);
        INT CX = X;
        if (Line.bLabel != 0)
        {
            INT W = DrawTipText(Device, X, CurY, 0xFFA3A3A3, Line.Label);
            if (Line.bColon == 0)
                appStrcpy(Buf, TEXT(" "));
            else
                appStrcpy(Buf, TEXT(" : "));
            CX = W + X + DrawTipText(Device, W + X, CurY, 0xFFDCDCDC, Buf);
        }
        DrawTipText(Device, CX, CurY, Line.ValueColor, Line.Value);
        CurY += Ch.cy;
    }
    GConsole->ConvertNewLines((TCHAR*)*Info.Description, Tmp);
    DrawTipText(Device, X, CurY + 6, 0xFFB2BECF, *Tmp, (INT)Info.FullBox.W, 0, 1);
}
NW_HOOK_METHOD(0x1002b800, NToolTipSkin, DrawFullText)
