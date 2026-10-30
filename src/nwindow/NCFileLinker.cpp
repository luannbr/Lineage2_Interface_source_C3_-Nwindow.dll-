/*=============================================================================
    NCFileLinker.cpp - the file link of the HTML viewer. Stock: 0x10030620 .. 0x100306df
    (WindowProc and OnLButtonDown are bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCFileLinker.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NConsoleWnd.h"

// 0x10044dd0 - NControl's stores (inline in the stock constructor) and the page; Message is left as allocated
NCFileLinker::NCFileLinker(TCHAR* InFile)
{
    File = InFile;
}
NW_HOOK_CTOR_ARGS(0x10044dd0, NCFileLinker, (TCHAR* A), (A))

// 0x10044e40
NW_HOOK_DTOR(0x10044e40, NCFileLinker)


// stock body shared with NCButton's (0x100051f0)
INT NCFileLinker::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100413f0 (vslot 62) - C3: a link with a message runs it as a chat line (the "/command" the page gave); else the page
// goes to the viewer
INT NCFileLinker::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (Message[0])
    {
        GDesktop->ChatWnd->ProcessChatLine(Message);
        return 1;
    }
    NotifyParent((LPARAM)File);
    return 1;
}
NW_HOOK_METHOD(0x100413f0, NCFileLinker, OnLButtonDown)

NW_CHECK_VTABLE(NCFileLinker, 0x10180398, 69)
