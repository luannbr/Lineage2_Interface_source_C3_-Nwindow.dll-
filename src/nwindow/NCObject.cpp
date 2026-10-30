/*=============================================================================
    NCObject.cpp - root class of the NWindow package.
=============================================================================*/
#include "nwindow/NCObject.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCVirtualWndMain.h"

#include <stdarg.h>
#include <stdio.h>

// 0x10028810 - shared by NCAbstractMain and NCCmdTarget (identical code folded by the linker).
NCObject::NCObject()
{
    SetFlags(RF_Native);
}
NW_HOOK_CTOR(0x10028810, NCObject)

// 0x10001a10 - the destructor comes from NW_DECLARE_CLASS (ConditionalDestroy, ~UObject).
NW_HOOK_DTOR(0x10001a10, NCObject)

// 0x10001aa0 - ConditionalDestroy, then NCObject's destructor (0x10001a10)
NW_HOOK_DTOR(0x10001aa0, NCCmdTarget)

// 0x10033130 - the same (the abstract owner of the windows)
NW_HOOK_DTOR(0x10033130, NCAbstractMain)

// 0x10028830 (vslot 10)
void NCObject::Destroy()
{
    ClearFlags(RF_Native);
    UObject::Destroy();
}
NW_HOOK_METHOD(0x10028830, NCObject, Destroy)

// 0x10028840 - debug print, routed by GNWindowDebug: 0 = off (returns 1), 1 = the console's debug window (category 0, the
// light grey color the stock passes), anything else = dropped
INT VARARGS NCObject::Debugf(const TCHAR* Fmt, ...)
{
    TCHAR Temp[256];
    switch (GNWindowDebug)
    {
    case 0:
        return 1;
    case 1:
    {
        INT Result = 0;
        NCDebugWnd* DebugWnd = GConsole ? GConsole->DebugWnd : NULL;
        if (DebugWnd)
        {
            va_list ArgPtr;
            va_start(ArgPtr, Fmt);
            Result = _vswprintf(Temp, Fmt, ArgPtr);
            va_end(ArgPtr);
            DebugWnd->AddLog(0, Temp, (INT)0xFFDCDCDC);
        }
        return Result;
    }
    default:
        return 0;
    }
}
NW_HOOK_VARARGS(0x10028840, NCObject, Debugf, INT (VARARGS NCObject::*)(const TCHAR*, ...))

/*-----------------------------------------------------------------------------
    IME of the game window (used by the NC*IME classes).
-----------------------------------------------------------------------------*/

// 0x100288d0
DWORD NCObject::ImeGetProperty(DWORD Index)
{
    return ImmGetProperty(GetKeyboardLayout(0), Index);
}
NW_HOOK_METHOD(0x100288d0, NCObject, ImeGetProperty)

// 0x100288f0
HIMC NCObject::ImeGetContext()
{
    return ImmGetContext(GViewportHWnd);
}
NW_HOOK_METHOD(0x100288f0, NCObject, ImeGetContext)

// 0x10028900
BOOL NCObject::ImeReleaseContext(HIMC hIMC)
{
    return ImmReleaseContext(GViewportHWnd, hIMC);
}
NW_HOOK_METHOD(0x10028900, NCObject, ImeReleaseContext)

// 0x10028920 - C3 asks the game window's IME again (C2 had a 2-byte `ret 0xc` here): ImmIsUIMessageW(GViewportHWnd,
// Msg, wParam, lParam), its answer returned
BOOL NCObject::ImeIsUIMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return ImmIsUIMessageW(GViewportHWnd, Msg, wParam, lParam);
}
NW_HOOK_METHOD(0x10028920, NCObject, ImeIsUIMessage)

// (name ours; not a function of the stock image) what the stock callers do with two instructions: the console's
// 0x10061d20 plays the interface sound of the number (NCObject.h)
void NCObject::PlayInterfaceSound(INT SoundID)
{
    GConsole->PlayInterfaceSound(SoundID);
}

/*-----------------------------------------------------------------------------
    Timers. MFC-style signature; the console keeps the list (NConsoleWnd +0x36fc,
    records {ID, Elapse ms, bKilled, Accumulated, Owner}) and its Tick sends
    WindowProc(WM_TIMER, ID, 0) to the owner. TimerProc is ignored (callers pass 0).
-----------------------------------------------------------------------------*/

// 0x10028940 - the stock code is a tail jump that stores this over the TimerProc argument (the console's NSetTimer)
INT NCObject::SetTimer(UINT ID, UINT Elapse, void* TimerProc)
{
    return GConsole->NSetTimer(ID, Elapse, this);
}
NW_HOOK_METHOD(0x10028940, NCObject, SetTimer)

// 0x10028950
UBOOL NCObject::KillTimer(UINT ID)
{
    return GConsole->NKillTimer(ID, this);
}
NW_HOOK_METHOD(0x10028950, NCObject, KillTimer)

/*-----------------------------------------------------------------------------
    Resources.
-----------------------------------------------------------------------------*/

// 0x10028970 - texture by full name; falls back to NWindow.BlackTexture. The texture gets
// RF_Native, as NWindow does for every texture it holds.
UTexture* NCObject::FindTexture(const TCHAR* Name)
{
    if (!Name)
        return NULL;
    UTexture* Texture = (UTexture*)StaticFindObject(UTexture::StaticClass(), ANY_PACKAGE, Name);
    if (!Texture)
        Texture = (UTexture*)StaticLoadObject(UTexture::StaticClass(), NULL, Name, NULL, 0, NULL);
    if (!Texture)
    {
        Debugf(TEXT("Can not find Texture : %s"), Name);
        Texture = FindTexture(TEXT("NWindow.BlackTexture"));
        if (!Texture)
            return NULL;
    }
    Texture->SetFlags(RF_Native);
    return Texture;
}
NW_HOOK_METHOD(0x10028970, NCObject, FindTexture)

// 0x100289e0 - an actor of the named class (loaded if needed) at the origin of the player's
// level, collision ignored. C2 marks the actor RF_Native (0x4000000) when it was made.
AActor* NCObject::SpawnActor(const TCHAR* ClassName)
{
    ULevel* Level = GCanvas->Viewport->Actor->GetLevel();
    UClass* Class = StaticLoadClass(AActor::StaticClass(), NULL, ClassName, NULL, 0, NULL);
    AActor* Actor = Level->SpawnActor(Class, NAME_None, FVector(0, 0, 0), FRotator(0, 0, 0), NULL, 1);
    if (Actor)
        Actor->SetFlags(RF_Native);
    return Actor;
}
NW_HOOK_METHOD(0x100289e0, NCObject, SpawnActor)

// 0x10028a70 - seconds of the last console tick
FLOAT NCObject::GetDeltaTime()
{
    return GConsole->DeltaTime;
}
NW_HOOK_METHOD(0x10028a70, NCObject, GetDeltaTime)

/*-----------------------------------------------------------------------------
    Mouse cursor. While a window or an item is dragged the cursor is locked:
    SetCursor requests are dropped until the button goes up.
-----------------------------------------------------------------------------*/

// 0x10028a80
void NCObject::SetCursor(INT Type, UTexture* Texture)
{
    GConsole->SetCursor(Type, Texture);
}
NW_HOOK_METHOD(0x10028a80, NCObject, SetCursor)

// 0x10028a90
void NCObject::LockCursor()
{
    GConsole->LockCursor();
}
NW_HOOK_METHOD(0x10028a90, NCObject, LockCursor)

// 0x10028aa0
void NCObject::UnlockCursor()
{
    GConsole->UnlockCursor();
}
NW_HOOK_METHOD(0x10028aa0, NCObject, UnlockCursor)

NW_CHECK_VTABLE(NCObject, 0x10176368, 25)
