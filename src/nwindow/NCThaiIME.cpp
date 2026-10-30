/*=============================================================================
    NCThaiIME.cpp - IME of the Thai client (new in C2). Stock: 0x100125a0 .. 0x100126b0 +
    0x100126b0 .. 0x10012773, 0x10013790 .. 0x1001382f (constructor, destructor); ShowWindow is the
    shared 0x1007b460 body.
=============================================================================*/
#include "nwindow/NCThaiIME.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NConsoleWnd.h"

// 0x1001b9b0
NCThaiIME::NCThaiIME(INT InMode)
    : NCIME(0)
{
    Mode = InMode;
}
NW_HOOK_CTOR_ARGS(0x1001b9b0, NCThaiIME, (INT A), (A))

// 0x1001ba00
NW_HOOK_DTOR(0x1001ba00, NCThaiIME)

// 0x10015660 - the class's operator new (NW_DECLARE_CLASS), out of line for the manager's OnCreate (cdecl: the size is
// not read; the object is allocated in Outer under Name with SetFlags, as the Chinese IME's at 0x100155c0)
static void* NewThaiIME(size_t Size, UObject* Outer, FName Name, DWORD SetFlags)
{
    return NCThaiIME::operator new(Size, Outer, Name, SetFlags);
}
NW_HOOK_FUNC(0x10015660, NewThaiIME)


// 0x10015370 (vslot 25) - NCIME's messages again after NCIME: the notify, the context and the layout change
INT NCThaiIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCIME::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_IME_NOTIFY)
        return OnImeNotify(wParam, lParam) != 0;
    if (Msg == WM_IME_SETCONTEXT)
        return OnImeSetContext(wParam, lParam) != 0;
    if (Msg == WM_INPUTLANGCHANGE)
        return OnInputLangChange(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10015370, NCThaiIME, WindowProc)

// 0x10015580 (vslot 69) - the stock slot is a tail jump into NCIME::Activate (0x100142f0)
void NCThaiIME::Activate()
{
    NCIME::Activate();
}
NW_HOOK_METHOD(0x10015580, NCThaiIME, Activate)

// 0x10015410 (vslot 37)
INT NCThaiIME::OnCreate()
{
    NCIME::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10015410, NCThaiIME, OnCreate)

// 0x10015420 (vslot 38) - NCWnd::OnDestroy is not called
INT NCThaiIME::OnDestroy()
{
    ImmDestroyContext(hIMC);
    return 1;
}
NW_HOOK_METHOD(0x10015420, NCThaiIME, OnDestroy)

// 0x1007b460 (vslot 43) - one body for every class that only calls NCWnd's (hooked with NCEditBox's)
void NCThaiIME::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x10015440 (vslot 71) - while active, a change of the open status (8) closes the IME again in Mode 2 (a field
// that must stay in alphanumeric mode; C3: also for the notification of all modes, lParam -1) and sets its
// conversion mode to alphanumeric; every other notification is ignored
INT NCThaiIME::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    if (bActive && wParam != IMN_CLOSESTATUSWINDOW && wParam != IMN_OPENSTATUSWINDOW)
    {
        if (wParam == IMN_SETOPENSTATUS)
        {
            if (Mode == 2 || lParam == -1)
                ImmSetOpenStatus(hIMC, 0);
            if (Mode == 2 || lParam == -1)
                ImmSetConversionStatus(hIMC, 0, 0);
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10015440, NCThaiIME, OnImeNotify)

// 0x100154b0 (vslot 70) - cancels the composition and takes the game window's current context as its own; with
// the chat line open the IME is closed, in the world (unless Mode is 1) the game window gets the context of the
// chat input box's IME back (GDesktop->ChatWnd->EditBox: the chat window's edit box)
void NCThaiIME::Deactivate()
{
    bActive = 0;
    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmNotifyIME(hIMC, NI_CLOSECANDIDATE, 0, 0);
    OpenStatus = ImmGetOpenStatus(hIMC);
    hIMC = ImmGetContext(GViewportHWnd);
    if (GIsEnterChatting)
    {
        ImmSetOpenStatus(hIMC, 0);
        bNotComposing = 1;
        return;
    }
    if (GConsole->State == L2CS_InGame && Mode != 1)
        ImmAssociateContext(GViewportHWnd, GDesktop->ChatWnd->EditBox->IMEManager->ThaiIME->hIMC);
    bNotComposing = 1;
}
NW_HOOK_METHOD(0x100154b0, NCThaiIME, Deactivate)

NW_CHECK_VTABLE(NCThaiIME, 0x1017c460, 80)
