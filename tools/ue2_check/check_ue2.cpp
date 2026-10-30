// check_ue2.cpp - compile-only translation unit for include/ue2/UE2Core.h and UE2Engine.h (C2).
//
// tools/ue2_check/run_check.py compiles it with the project's flags (NWD_CXX_FLAGS / NWD_DEFS of CMakeLists.txt,
// NW_HOOK_MODE=1 and 0) and requires: no diagnostic at all, which includes every static_assert of the headers
// (NW_ASSERT_OFFSET / NW_ASSERT_SIZE of the engine classes). The other two checks of the same script work on units it
// generates from the headers (tools/ue2_check/gen_checks.py): the link test of every ENGINE_API / CORE_API name
// against the import libraries of third_party/ue_import/*.def, and the slot and signature test of every declared
// virtual against the C2 DLLs.
//
// The extra asserts below are the facts of the C2 binaries the headers rest on, written once more here from the
// DLL analysis (tools/ue2_vtable_diff.py, tools/ue2_layouts.py), so that a header edit that drifts away from them
// fails here even if its own NW_ASSERT was edited along.
#include "ue2/UE2Core.h"
#include "ue2/UE2Engine.h"

// UObject keeps its C1 size; the C3 vtable has 25 slots (ClearL2Game at 22, as in C2); class sizes from the
// IMPLEMENT_CLASS initializers of the C3 Engine.dll (tools/ue2_engine_classes.py)
static_assert(sizeof(UObject) == 0x28, "UObject is 0x28 bytes in C3");
static_assert(sizeof(AActor) == 0x398, "AActor is 0x398 bytes in C3 (C2 0x394, C1 0x5DC)");
static_assert(sizeof(APawn) == 0x16CC, "APawn is 0x16CC bytes in C3 (C2 0x14F4)");
static_assert(sizeof(AController) == 0x538, "AController is 0x538 bytes in C3 (C2 0x534)");
static_assert(sizeof(UCanvas) == 0x1A4, "UCanvas is 0x1A4 bytes in C3 (C2 0x13C)");
static_assert(sizeof(UEngine) == 0x184, "UEngine is 0x184 bytes in C3 (C2 0x174)");
static_assert(sizeof(UGameEngine) == 0x36C, "UGameEngine is 0x36C bytes in C3 (C2 0x330)");
static_assert(sizeof(FParser) == 0x418, "FParser::operator= copies 0x106 dwords in both builds");

// the by-value sizes that changed in C3 (tools/ue2_ret_diff.py: the `ret N` of the by-value exports; the code
// that copies them): User 0x230 (C2 0x1F8), FL2GameData 0x7AF90 (C2 0x605FC), UL2ConsoleWnd 0x392C (C2 0x3518);
// PetInfo stays 0xAC (UNetworkHandler::GetPetInfo still copies 0x2B dwords)
