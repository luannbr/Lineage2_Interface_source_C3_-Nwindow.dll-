#pragma once
// NCDebugWnd - the debug log list (UClass "CDebugWnd", 0x1E8), fed by NCObject::Debugf.
// Lines are prefixed with their category; the categories come from .\L2Debug.ini
// ([DebugInfo] count, then Library%d: name%d, color%d = R=,G=,B=, IsEnable%d).

#include "nwindow/NCListBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// one debug category (names ours)
struct NDebugCategory
{
    TCHAR Name[32];            // +0x00 (entry 0: "NWindow")
    UBOOL bEnabled;            // +0x40
    DWORD Color;               // +0x44
};
NW_ASSERT_SIZE(NDebugCategory, 0x48);

#if NW_HOOK_MODE
#define GDebugCategories (reinterpret_cast<NDebugCategory*>(::nw::Addr(0x10207748)))
#define GNumDebugCategories NW_GLOBAL(INT, 0x1027BFA8)
#else
extern NDebugCategory GDebugCategories[];   // 20 in the stock image
extern INT GNumDebugCategories;
#endif

class NCDebugWnd : public NCListBox
{
    NW_DECLARE_CLASS(NCDebugWnd, NCListBox, 0, 0x1027BFB0)

    NCDebugWnd();                                                    // 0x10079eb0 (50, 10)
    NCDebugWnd(INT InMaxLines, INT InVisibleLines);                  // 0x10079f50
    // ~NCDebugWnd                                                   // 0x10079f00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101241c0
    virtual INT OnCreate();                                          // 37 0x100862b0
    virtual INT OnDestroy();                                         // 38 0x10079db0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10079b40

    // names ours
    void SetBackTexture(const TCHAR* Name);                          // 0x10079b60
    void AddLog(INT Category, const TCHAR* Text, INT Unused);        // 0x10079b90
    void LoadCategories();                                           // 0x10079c10
};
NW_ASSERT_SIZE(NCDebugWnd, 0x220);

#pragma pack(pop)