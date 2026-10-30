#pragma once
// NCDynamicLinker - a link of an HTML page that the layout broke across lines (UClass
// "CDynamicLinker", 0x224 in C3; NCHtmlViewer::AddFileLink and AddActionLink create it with the item's
// geometry): OnCreate makes one, two or three child links (NCFileLinker when bFile, else NCActionLinker) over the
// pieces: the first line's part, the lines between, the last line's part, as many as the link's height asks for;
// a click on any of them is passed on to the parent with the child's code. The geometry fields are the
// constructor's arguments (the HTML layout's measures of the link).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCActionLinker;
class NCFileLinker;

class NCDynamicLinker : public NControl
{
    NW_DECLARE_CLASS(NCDynamicLinker, NControl, 0, 0x10275790)

    // C3 layout (the constructor 0x10045070, OnCreate 0x10045170, OnDestroy 0x10041c60; names ours)
    INT AbsX;                        // +0x1D4 the link's left (the first argument)
    INT AbsY;                        // +0x1D8
    INT X;                           // +0x1DC added to the width of the first piece
    INT Y;                           // +0x1E0
    INT ParentAbsX;                  // +0x1E4 the pieces after the first start at ParentAbsX - AbsX
    INT ParentAbsY;                  // +0x1E8
    INT W;                           // +0x1EC the right edge of a one-line link
    INT H;                           // +0x1F0 the link's height (it decides the number of pieces)
    INT LayoutWidth;                 // +0x1F4 the right edge of the first line when there are several pieces, and the width of
                                     //   the middle piece of three
    INT TailWidth;                   // +0x1F8 the width of the last piece (and of the second of two)
    TCHAR* Target;                   // +0x1FC the string every piece hands on (the file or the action code); NULL: no pieces
    TCHAR* Message;                  // +0x200 copied into each piece's Message (the chat line / the question), or NULL
    INT bFile;                       // +0x204 1: NCFileLinker pieces (AddFileLink), 0: NCActionLinker pieces
    NCActionLinker* ActionLinks[3];  // +0x208 the pieces of an action link
    NCFileLinker* FileLinks[3];      // +0x214 the pieces of a file link
    INT LinkCount;                   // +0x220 pieces made

    NCDynamicLinker();                                               // 0x10044f50 (the class constructor InternalConstructor calls)
    // the one the HTML viewer calls (13 arguments, ret 0x34)
    NCDynamicLinker(INT InAbsX, INT InAbsY, INT InX, INT InY, INT InW, INT InH, INT InLayoutWidth, INT InTailWidth,
                    INT InParentAbsX, INT InParentAbsY, TCHAR* InTarget, TCHAR* InMessage, INT InbFile); // 0x10045070
    // ~NCDynamicLinker                                              // 0x10045020

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100414c0
    virtual INT OnCreate();                                          // 37 0x10045170
    virtual INT OnDestroy();                                         // 38 0x10041c60
    virtual INT OnLink(INT Code);                                    // 69 0x10041520 (name ours) a piece was clicked: the parent is told
};
NW_ASSERT_OFFSET(NCDynamicLinker, AbsX, 0x1D4);
NW_ASSERT_OFFSET(NCDynamicLinker, H, 0x1F0);
NW_ASSERT_OFFSET(NCDynamicLinker, Target, 0x1FC);
NW_ASSERT_OFFSET(NCDynamicLinker, bFile, 0x204);
NW_ASSERT_OFFSET(NCDynamicLinker, ActionLinks, 0x208);
NW_ASSERT_OFFSET(NCDynamicLinker, FileLinks, 0x214);
NW_ASSERT_OFFSET(NCDynamicLinker, LinkCount, 0x220);
NW_ASSERT_SIZE(NCDynamicLinker, 0x224);

#pragma pack(pop)
