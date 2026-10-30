/*=============================================================================
    NCIME.cpp - base of the language IMEs. Stock: 0x1000dee0 .. 0x1000e22f,
    0x1000e840 .. 0x1000e88f, 0x1000f2b0 .. 0x1000f2ff, 0x1000f6f0 .. 0x1000f758.
=============================================================================*/
#include "nwindow/NCIME.h"
#include "nwindow/NCrt.h"

#include <stdlib.h>

// (GActiveIMC: src/standalone/NwGlobals.cpp in the standalone build)

// 0x1001b5a0 - NControl's constructor is inlined in the stock code
NCIME::NCIME(INT InMode)
{
    Property = 0;
    OpenStatus = 0;
    Conversion = 0;
    Sentence = 0;
    CodePage = 0;
    bActive = 0;
    Mode = InMode;
    bNotComposing = 1;
}
NW_HOOK_CTOR_ARGS(0x1001b5a0, NCIME, (INT A), (A))

// 0x1001b440
NW_HOOK_DTOR(0x1001b440, NCIME)

// 0x10014090 (vslot 25)
INT NCIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_IME_NOTIFY)
        return OnImeNotify(wParam, lParam) != 0;
    if (Msg == WM_IME_SETCONTEXT)
        return OnImeSetContext(wParam, lParam) != 0;
    if (Msg == WM_IME_STARTCOMPOSITION)
        return OnImeStartComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_ENDCOMPOSITION)
        return OnImeEndComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_CONTROL)
        return OnImeControl(wParam, lParam) != 0;
    if (Msg == WM_INPUTLANGCHANGE)
        return OnInputLangChange(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITIONFULL)
        return OnImeCompositionFull(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITION)
        return OnImeComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_CHAR)
        return OnImeChar(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10014090, NCIME, WindowProc)

// 0x100141f0 (vslot 37) - own input context; the IME's default window stays hidden
INT NCIME::OnCreate()
{
    NControl::OnCreate();
    hIMC = ImmCreateContext();
    HWND IMEWnd = ImmGetDefaultIMEWnd(GViewportHWnd);
    if (IMEWnd)
        ::ShowWindow(IMEWnd, SW_HIDE);
    return 1;
}
NW_HOOK_METHOD(0x100141f0, NCIME, OnCreate)

// 0x10014230 (vslot 38) - NControl's OnDestroy, then 1 (C2 had a bare `return 1` here)
INT NCIME::OnDestroy()
{
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10014230, NCIME, OnDestroy)

// 0x100142f0 (vslot 69) - the game window uses this IME's context; C2 also records it for the console
void NCIME::Activate()
{
    bActive = 1;
    if (hIMC)
        ImmAssociateContext(GViewportHWnd, hIMC);
    if (GConsole)
        GActiveIMC = hIMC;
}
NW_HOOK_METHOD(0x100142f0, NCIME, Activate)

// 0x10014330 (vslot 70) - C2: no longer a bare return
void NCIME::Deactivate()
{
    if (GConsole)
        GActiveIMC = NULL;
}
NW_HOOK_METHOD(0x10014330, NCIME, Deactivate)

// 0x10014240 (vslot 72)
INT NCIME::OnImeStartComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 0;
    return 1;
}
NW_HOOK_METHOD(0x10014240, NCIME, OnImeStartComposition)

// 0x10014260 (vslot 73)
INT NCIME::OnImeEndComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 1;
    return 1;
}
NW_HOOK_METHOD(0x10014260, NCIME, OnImeEndComposition)

// 0x10014290 (vslot 74) - C3: no longer by the IME properties; the IME is asked to hide its own windows
// whenever the context is activated showing the composition (ISC_SHOWUICOMPOSITIONWINDOW, the sign bit) or
// the candidate window (ISC_SHOWUICANDIDATEWINDOW): the message goes again without any flag
INT NCIME::OnImeSetContext(WPARAM wParam, LPARAM lParam)
{
    if (wParam == 1)
    {
        if ((INT)lParam < 0 || (lParam & ISC_SHOWUICANDIDATEWINDOW))
            ImeIsUIMessage(WM_IME_SETCONTEXT, 1, 0);
    }
    return 1;
}
NW_HOOK_METHOD(0x10014290, NCIME, OnImeSetContext)

// 0x10014270 (vslot 76)
INT NCIME::OnInputLangChange(WPARAM wParam, LPARAM lParam)
{
    Property = ImeGetProperty(IGP_PROPERTY);
    return 1;
}
NW_HOOK_METHOD(0x10014270, NCIME, OnInputLangChange)

// 0x100142c0
TCHAR* NCIME::GetResultStr()
{
    return ResultStr;
}
NW_HOOK_METHOD(0x100142c0, NCIME, GetResultStr)

// 0x100142d0
TCHAR* NCIME::GetCompStr()
{
    return CompStr;
}
NW_HOOK_METHOD(0x100142d0, NCIME, GetCompStr)

// 0x100142e0
TCHAR* NCIME::GetReadingStr()
{
    return ReadingStr;
}
NW_HOOK_METHOD(0x100142e0, NCIME, GetReadingStr)

// 0x10014350
UBOOL NCIME::IsNotComposing()
{
    return bNotComposing;
}
NW_HOOK_METHOD(0x10014350, NCIME, IsNotComposing)

// 0x10014360 - NT, or Windows 4.x from 98 on; asked once
UBOOL NCIME::IsUnicodeOS()
{
    if (!GIMEOSChecked)
    {
        OSVERSIONINFOW Info;
        Info.dwOSVersionInfoSize = sizeof(Info);
#pragma warning(suppress : 4996)  // deprecated in current SDKs; the stock code calls it
        GetVersionExW(&Info);
        if (Info.dwPlatformId == VER_PLATFORM_WIN32_NT || Info.dwMajorVersion > 4 ||
            (Info.dwMajorVersion == 4 && Info.dwMinorVersion > 0))
            GIMEUnicodeOS = 1;
        GIMEOSChecked = 1;
    }
    return GIMEUnicodeOS;
}
NW_HOOK_METHOD(0x10014360, NCIME, IsUnicodeOS)

// 0x100143c0 - CodePage becomes the default ANSI code page of the keyboard layout's language
void NCIME::UpdateCodePage()
{
    ANSICHAR Buf[12];
    if (GetLocaleInfoA(LOWORD(GetKeyboardLayout(0)), LOCALE_IDEFAULTANSICODEPAGE, Buf, 10))
        CodePage = NAtol(Buf);
}
NW_HOOK_METHOD(0x100143c0, NCIME, UpdateCodePage)

// 0x100148d0 (static) - imm32.dll's four input context locks (the library is not freed)
void NCIME::LoadImmFunctions()
{
    HMODULE Imm = LoadLibraryA("imm32.dll");
    if (Imm)
    {
        GImmLockIMC = (NImmLockIMC)GetProcAddress(Imm, "ImmLockIMC");
        GImmUnlockIMC = (NImmUnlockIMC)GetProcAddress(Imm, "ImmUnlockIMC");
        GImmLockIMCC = (NImmLockIMCC)GetProcAddress(Imm, "ImmLockIMCC");
        GImmUnlockIMCC = (NImmUnlockIMCC)GetProcAddress(Imm, "ImmUnlockIMCC");
    }
}
NW_HOOK_FUNC(0x100148d0, NCIME::LoadImmFunctions)

NW_CHECK_VTABLE(NCIME, 0x1017bcc8, 80)
