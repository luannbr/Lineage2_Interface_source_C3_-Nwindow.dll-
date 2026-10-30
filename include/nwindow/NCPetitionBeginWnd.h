#pragma once
// NCPetitionBeginWnd - the form that opens a petition (UClass "CPetitionBeginWnd", 0xEC; the system menu
// and the console open it): a combo box with the kinds of petition (C3: the categories of localization.ini,
// [CategoryNum] Num and [Category<n>] Title / Link), the help page of the kind chosen (an NCHtmlViewer), a
// multi-line edit for the text and OK / Cancel. OK checks the text (5 to 254 characters, a kind chosen),
// sends the petition and starts the conversation window (NCPetitionWnd) with it; Cancel withdraws one
// already open.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCComboBox;
class NCHtmlViewer;
class NCMultiEdit;

// A kind of petition as localization.ini lists it (0x204 bytes, name ours): the sysstring of its name and the
// help page the form loads when it is chosen ("..\L2text\<Link>")
struct NPetitionCategory
{
    INT TitleID;             // +0x000 [Category<n>] Title: the combo's item (AddItem(INT))
    TCHAR Link[256];         // +0x004 [Category<n>] Link: the help file
};
NW_ASSERT_SIZE(NPetitionCategory, 0x204);

class NCPetitionBeginWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetitionBeginWnd, NCWnd, 0, 0x1029E200)

    NCButton* OKButton;              // +0xC8 text 694 (name ours)
    NCButton* CancelButton;          // +0xCC text 473 (name ours)
    NCHtmlViewer* HelpViewer;        // +0xD0 the help page of the kind chosen (name ours)
    NCComboBox* TypeCombo;           // +0xD4 the kinds: 1 move .. 9 etc (0 = none chosen) (name ours)
    NCMultiEdit* TextEdit;           // +0xD8 the petition (name ours)
    UTexture* BackTex;               // +0xDC "L2UI.PetitionWnd.PetitionBeginBack"
    TArray<NPetitionCategory> Categories;  // +0xE0 (new in C3, name ours) the kinds the combo lists, in order
                                     //   (the combo's item N is Categories(N - 1): item 0 is the "choose" line)

    NCPetitionBeginWnd();                                            // 0x100567c0
    // ~NCPetitionBeginWnd                                           // 0x10056820 (the array's is 0x10055640)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d06b0
    virtual INT OnCreate();                                          // 37 0x100d2760
    virtual INT OnDestroy();                                         // 38 0x100d2030
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d13e0
    virtual INT OnSend();                                            // 68 0x100d1530 (name ours) OK
    virtual INT OnCancel();                                          // 69 0x100d16b0 (name ours)
    virtual void NewVirtual70();  // 70 0x100d16e0 (only this client has it; defined in NNewVirtuals.gen.cpp):
                                  // the body is OnTypeChosen's; WindowProc calls the slot (the combo's selection)
    INT OnTypeChosen(INT Type);                              // 0x100d16e0 (name ours): the help page of kind Type

    // name ours: the form emptied (kind 0, no text, no help page)
    __declspec(noinline) void ResetForm();                           // 0x100d0750
    // name ours (0x100d1f00, sub_100d1f00): the combo and Categories from localization.ini
    __declspec(noinline) void LoadCategories();
};
NW_ASSERT_OFFSET(NCPetitionBeginWnd, OKButton, 0xC8);
NW_ASSERT_OFFSET(NCPetitionBeginWnd, HelpViewer, 0xD0);
NW_ASSERT_OFFSET(NCPetitionBeginWnd, TextEdit, 0xD8);
NW_ASSERT_OFFSET(NCPetitionBeginWnd, BackTex, 0xDC);
NW_ASSERT_OFFSET(NCPetitionBeginWnd, Categories, 0xE0);
NW_ASSERT_SIZE(NCPetitionBeginWnd, 0xEC);

#pragma pack(pop)
