/*=============================================================================
    NCCreditWnd.cpp - the credits (C2 only). Stock: 0x10079010 .. 0x10079b3f.
=============================================================================*/
#include <intrin.h>
#include "nwindow/NCCreditWnd.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCMessageWnd.h"

#if NW_HOOK_MODE
// the [Credit] key names, TCHAR[5][255] ("CreditSound1".."CreditSound5") in the stock image
#define GCreditSoundKeys (reinterpret_cast<const TCHAR (*)[255]>(::nw::Addr(0x10205C58)))
#else
extern const TCHAR GCreditSoundKeys[5][255];
#endif

// the window at +0xD0 of the god window (C2: the line under the login screens, NConsoleWnd::MessageWnd)
static FORCEINLINE NCWnd* LineWnd()
{
    return *(NCWnd**)((BYTE*)GConsole->GodWnd + 0xD0);
}

// 0x10079a50 - NControl's constructor is inlined in the stock code; the sound names are left as allocated
NCCreditWnd::NCCreditWnd()
{
    BackTex = NULL;
    Viewer = NULL;
    Elapsed = 0;
    Page = 0;
    MusicHandle = 0;
    SavedVolume = 0;
    PageSeconds = 0;
}
NW_HOOK_CTOR(0x10079a50, NCCreditWnd)

// 0x10079ae0
NW_HOOK_DTOR(0x10079ae0, NCCreditWnd)

// the table of pages (FL2GameData +0x3CE4 holds a pointer to it)
#define GCreditTable ((NCreditTable*)GL2GameData.CreditTable)

// 0x10079400 (ret 4) - TMap::Find: the pair of the key's hash chain with the key
NCreditPage** NCreditTable::Find(const UINT* Key)
{
    INT Index = Hash[(HashCount - 1) & *Key];
    if (Index != -1)
    {
        do
        {
            if (Pairs[Index].Key == *Key)
                return &Pairs[Index].Value;
            Index = Pairs[Index].HashNext;
        } while (Index != -1);
    }
    return NULL;
}
NW_HOOK_METHOD(0x10079400, NCreditTable, Find)

// 0x100791c0 - the credits' own font goes (a member that ignores its `this`)
void NCCreditWnd::FreeFont()
{
    NCGDevice::DestroyFont(2);
}
NW_HOOK_METHOD(0x100791c0, NCCreditWnd, FreeFont)

// 0x10079010 (ret 4) - the text's rectangle (400 x 550) by the page's Align: 1..9 are the nine places of a
// three by three grid over the screen (30 pixels from the edges; the middle ones centered); any other value
// is the top left
void NCCreditWnd::PlaceViewer(INT Align)
{
    INT ScreenW = GSEKScreenX;
    INT ScreenH = GSEKScreenY;
    switch (Align)
    {
    default:
        Viewer->SetPosition(30, 30, 400, 550);
        return;
    case 2:
        Viewer->SetPosition(ScreenW / 2 - 200, 30, 400, 550);
        return;
    case 3:
        Viewer->SetPosition(ScreenW - 430, 30, 400, 550);
        return;
    case 4:
        Viewer->SetPosition(30, ScreenH / 2 - 275, 400, 550);
        return;
    case 5:
        Viewer->SetPosition(ScreenW / 2 - 200, ScreenH / 2 - 275, 400, 550);
        return;
    case 6:
        Viewer->SetPosition(ScreenW - 430, ScreenH / 2 - 275, 400, 550);
        return;
    case 7:
        Viewer->SetPosition(30, ScreenH - 580, 400, 550);
        return;
    case 8:
        Viewer->SetPosition(ScreenW / 2 - 200, ScreenH - 580, 400, 550);
        return;
    case 9:
        Viewer->SetPosition(ScreenW - 430, ScreenH - 580, 400, 550);
        return;
    }
}
NW_HOOK_METHOD(0x10079010, NCCreditWnd, PlaceViewer)

// 0x10079450 - the next page: its picture (kept when the record names none), its text file and its place; past the
// last one the credits end (the login screen and the window under it come back, the intro music again and the
// volume), as on Escape
void NCCreditWnd::NextPage()
{
    Elapsed = 0;
    Page++;
    FString Path;
    if (Page < PageCount)
    {
        NCreditPage* Record = *GCreditTable->Find((const UINT*)&Page);
        PageSeconds = Record->Seconds;
        if (Record->Texture.Num() && Record->Texture.Num() - 1 > 0)
            BackTex = FindTexture(*Record->Texture);
        Path = FString(TEXT("..\\L2Text\\")) + Record->File;
        Viewer->LoadHtmlFile((TCHAR*)*Path);
        PlaceViewer(Record->Align);
    }
    else
    {
        if (GConsole)
        {
            if (GConsole->LoginWnd)
                GConsole->LoginWnd->ShowWindow();
            if (NCWnd* Line = LineWnd())
                Line->ShowWindow();
        }
        HideWindow();
        if (MusicHandle && GConsole && GConsole->Engine && GConsole->Engine->Audio)
        {
            GConsole->Engine->Audio->StopMusic(MusicHandle, 2.0f, 0);
            GConsole->PlayIntroMusic();
            GConsole->Engine->Audio->SetSoundVolume(SavedVolume);
        }
        KillFocus();
        Page = 0;
        Elapsed = 0;
        PageSeconds = 0;
        FreeFont();
    }
}
NW_HOOK_METHOD(0x10079450, NCCreditWnd, NextPage)

// 0x10079660 (ret 4) - time goes on; while the credits are shown and no music plays, one of the five tracks is
// chosen at random (the old one stopped); a page that outlasted its seconds is replaced
void NCCreditWnd::Tick(FLOAT DeltaTime)
{
    Elapsed = DeltaTime + Elapsed;
    if (GConsole && GConsole->Engine)
    {
        if (!GConsole->Engine->Audio->IsMusicPlaying() && IsShowWindow())
        {
            if (MusicHandle)
                GConsole->Engine->Audio->StopMusic(MusicHandle, 0.0f, 0);
            INT Pick = appRand() % 5;
            MusicHandle = GConsole->Engine->Audio->PlayMusic(FString(SoundName[Pick]), 1.5f, 0, 0, 1.0f, 0);
        }
    }
    if ((FLOAT)PageSeconds < Elapsed)
        NextPage();
}
NW_HOOK_METHOD(0x10079660, NCCreditWnd, Tick)

// 0x10079750 - the credits' font, the first page (number 0) and the music stopped (the music comes with Tick)
void NCCreditWnd::Start()
{
    NCGDevice::LoadFont(2, TEXT("L2UI.CreditWnd.CreditFont"), TEXT("CreditFont.gly"));
    UINT Key = 0;
    NCreditPage** Slot = GCreditTable->Find(&Key);
    if (Slot)
    {
        FString Path;
        NCreditPage* Record = *Slot;
        PlaceViewer(Record->Align);
        PageSeconds = Record->Seconds;
        if (Record->Texture.Num() && Record->Texture.Num() - 1 > 0)
            BackTex = FindTexture(*Record->Texture);
        Path = FString(TEXT("..\\L2Text\\")) + Record->File;
        Viewer->LoadHtmlFile((TCHAR*)*Path);
        Viewer->ShowBars(0);
        if (GConsole && GConsole->Engine && GConsole->Engine->Audio)
            GConsole->Engine->Audio->StopAllMusic(0.0f, 0);
    }
}
NW_HOOK_METHOD(0x10079750, NCCreditWnd, Start)

// 0x100791d0 (vslot 44) - the options window hides and the volume is kept (and silenced) to be given back at the end
void NCCreditWnd::ShowWindow()
{
    if (GConsole && GConsole->Engine && GConsole->Engine->Audio)
    {
        if (NCWnd* Options = *(NCWnd**)((BYTE*)GConsole->GodWnd + 0xD4))
            Options->HideWindow();
        if (UAudioSubsystem* Audio = GConsole->Engine->Audio)
        {
            SavedVolume = Audio->GetSoundVolume();
            GConsole->Engine->Audio->SetSoundVolume(0.0f);
        }
    }
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x100791d0, NCCreditWnd, ShowWindow)

// 0x10079240 (vslot 38) - not guarded; NControl's OnDestroy is not called
INT NCCreditWnd::OnDestroy()
{
    if (Viewer)
    {
        RemoveChildWindow(Viewer->Entry);
        delete Viewer;
        Viewer = NULL;
    }
    return 1;
}
NW_HOOK_METHOD(0x10079240, NCCreditWnd, OnDestroy)

// 0x10079280 (vslot 56) - the page's picture over the whole window (a 1024 x 768 source)
INT NCCreditWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, (INT)Width, (INT)Height, 0, 0, 1024, 768, BackTex, 0xFF);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10079280, NCCreditWnd, OnPaint)

// 0x10079320 (vslot 59) - Escape ends the credits: the login screen and the window under it come back, the intro
// music starts again and the volume is given back; every other key goes to NCWnd
INT NCCreditWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_ESCAPE)
    {
        GConsole->LoginWnd->ShowWindow();
        LineWnd()->ShowWindow();
        HideWindow();
        KillFocus();
        Page = 0;
        Elapsed = 0;
        PageSeconds = 0;
        if (MusicHandle && GConsole && GConsole->Engine && GConsole->Engine->Audio)
        {
            GConsole->Engine->Audio->StopMusic(MusicHandle, 2.0f, 0);
            GConsole->PlayIntroMusic();
            GConsole->Engine->Audio->SetSoundVolume(SavedVolume);
        }
        FreeFont();
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10079320, NCCreditWnd, OnKeyDown)

// 0x10079640 (vslot 62) - a click goes to the next page
INT NCCreditWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsShowWindow())
        NextPage();
    return 1;
}
NW_HOOK_METHOD(0x10079640, NCCreditWnd, OnLButtonDown)

// 0x100798c0 (vslot 37) - the HTML viewer (the stock constructor with the argument 2), the music names and the number
// of pages; NControl's OnCreate is not called
INT NCCreditWnd::OnCreate()
{
    Viewer = new NCHtmlViewer(2);
    Viewer->SetPosition(30, 30, 400, 550);
    CreateChild(Viewer, NWS_VISIBLE);
    Viewer->ShowWindow();
    Viewer->ShowBars(0);

    appRandInit((INT)(DWORD)__rdtsc());
    for (INT i = 0; i < 5; i++)
    {
        if (!GConfig->GetString(TEXT("Credit"), GCreditSoundKeys[i], SoundName[i], 256, TEXT("ALAudio.int")))
            appStrcpy(SoundName[i], TEXT("intro"));
    }
    PageCount = 0;
    NCreditTable* Table = GCreditTable;
    for (INT i = 0; i < Table->Num; i++)
        PageCount++;
    return 1;
}
NW_HOOK_METHOD(0x100798c0, NCCreditWnd, OnCreate)

NW_CHECK_VTABLE(NCCreditWnd, 0x10186758, 69)
