#pragma once
// NCBroadcastObserverBuffWnd - the effects on the followed player in the broadcast observer screen (UClass
// "CBroadcastObserverBuffWnd", 0x110; C3 only; NCBroadcastObserverPlayerWnd +0xF8): the player's own buff window
// (NCAbnormalStatusWnd: the icon lists, the one second timer, the blink, the tool tip) filled from the observer's
// roster instead of the console's buff packets, with bigger icons (40 pixels, ten to a row, three rows).
// Layout and virtuals from the stock constructor 0x101135D0 (NCAbnormalStatusWnd's, then the player window), OnCreate
// 0x10112A60, Refresh 0x10112B70 and OnPaint 0x10112CD0.

#include "nwindow/NCAbnormalStatusWnd.h"
#pragma pack(push, 4)

class NCBroadcastObserverPlayerWnd;

class NCBroadcastObserverBuffWnd : public NCAbnormalStatusWnd
{
    NW_DECLARE_CLASS(NCBroadcastObserverBuffWnd, NCAbnormalStatusWnd, 0, 0x102B82E0)

    NCBroadcastObserverPlayerWnd* PlayerWnd;   // +0x10C (name ours) the player window it belongs to

    NCBroadcastObserverBuffWnd(NCBroadcastObserverPlayerWnd* InPlayerWnd = NULL); // 0x101135D0 (ret 4; icon size 24); defaults = what the stock InternalConstructor pushes
    // ~NCBroadcastObserverBuffWnd                                   // 0x10113650

    virtual INT OnCreate();                                          // 37 0x10112A60 (NCWnd's OnCreate is not called)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10112CD0
    virtual void UpdateLayout();                                     // 68 0x10001B00 (folded: does nothing; the base's resizes the window)

    void Refresh();                                                  // 0x10112B70 the buffs cleared, the player's spells added
};
NW_ASSERT_OFFSET(NCBroadcastObserverBuffWnd, PlayerWnd, 0x10C);
NW_ASSERT_SIZE(NCBroadcastObserverBuffWnd, 0x110);

#pragma pack(pop)
