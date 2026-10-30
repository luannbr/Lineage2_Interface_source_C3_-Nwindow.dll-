#pragma once
// NGameGuard - the game's calls into nProtect GameGuard's client library (NPGameLib, linked statically into the stock
// image at 0x10144B70 .. 0x1014C0BF: 84 functions of a third party, no UI; the entry points the game calls are the
// NPGameLibApi ones). Name ours.
//
// Hook mode: the library is still the stock code and is called in place (it is the only thing of it that hook mode still
// runs). Standalone: the library is NOT linked (docs/notas/C3_Standalone.md section 0; the C2 decision, docs/notas/C2_Fase5.md,
// "nProtect e NKeyCrypt no standalone"), the entry points do nothing, as the library itself while it has not been started
// (its object pointer 0x102C7328 is 0, or its started flag is: SetHwnd and the query return at once; Init would answer 0,
// which StartGameGuard would report as an error, so the standalone answers success: there is nothing to start and nothing
// to report). In the stock C3 image the library's object is made, and GameMon started (CreateProcessA in the library's
// PreInit), by a static initializer (0x1016afb5 -> CreateGameMonObject 0x10062990 -> PreInitNPGameMonW 0x10144bf0); the
// console's start-up call (StartGameGuard, from Initialize) is jumped over by the packager's patch (0x10078a45). GameMon's
// answers come back through the stock message handler 0x100623c0 (only the library calls it). The standalone has neither,
// so NConsoleWnd::GameGuardQueryX (src/nwindow/NConsoleWndC2.cpp) answers the engine's query itself, with zeros.
//
// The calls are cdecl.

#include "nwindow/NWindow.h"

namespace NGameGuard
{

// the library's answer to a good start (the stock keeps it at 0x101ABA98, GNPGameMonSuccess): the standalone Init answers it, so that
// NConsoleWnd::StartGameGuard goes on to SetHwnd and shows no error
enum { InitSuccess = 0x755 };

#if NW_HOOK_MODE

// NPGameLibApi::InitNPGameMon 0x10144BB0 - the start-up answer (0x755 = success)
FORCEINLINE INT Init()
{
    typedef INT(__cdecl * Fn)();
    return ((Fn)nw::Addr(0x10144BB0))();
}

// NPGameLibApi::SetHwndToGameMon 0x10144BD0 - the game window, after a good start
FORCEINLINE void SetHwnd(INT hWnd)
{
    typedef void(__cdecl * Fn)(INT);
    ((Fn)nw::Addr(0x10144BD0))(hWnd);
}

// NPGameLibApi::SendAuthQuery 0x10144DC0 - the server's authentication query (four dwords) to GameMon
FORCEINLINE INT SendAuthQuery(INT* Query)
{
    typedef INT(__cdecl * Fn)(INT*);
    return ((Fn)nw::Addr(0x10144DC0))(Query);
}

#else

// no library: documented no-ops (see above)
FORCEINLINE INT Init() { return InitSuccess; }
FORCEINLINE void SetHwnd(INT) {}
FORCEINLINE INT SendAuthQuery(INT*) { return 0; }

#endif

}  // namespace NGameGuard
