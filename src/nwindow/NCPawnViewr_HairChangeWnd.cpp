/*=============================================================================
    NCPawnViewr_HairChangeWnd.cpp - the pawn viewer's hair page (C2: redesigned: a style suffix and a color, the "a" and
    "b" meshes of a hair number that exist, a face list). Stock: 0x100ab940 .. 0x100aba9a, 0x100adc50 .. 0x100af23e,
    0x100afea0 .. 0x100affa1.
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

// NWM_COMMAND ids (names ours)
enum
{
    NID_HAIR_STYLE = 0x104,
    NID_HAIR_MESHA = 0x105,
    NID_HAIR_MESHB = 0x106,
    NID_HAIR_RESET = 0x108,
    NID_HAIR_FACE = 0x109,
    NID_HAIR_TOGETHER = 0x110,
    NID_HAIR_COLOR = 0x112,
};

// sysstring line of the button
enum { STR_Cancel = 141 };

// the mesh names' prefixes by the pawn's mesh type (APawn::MeshType); built on the stack from
// these literals by each stock user
static const TCHAR GHairMeshPrefixes[20][20] = {
    TEXT("MFighter"), TEXT("FFighter"), TEXT("MDarkElf"), TEXT("FDarkElf"), TEXT("MDwarf"),
    TEXT("FDwarf"),   TEXT("MElf"),     TEXT("FElf"),     TEXT("MMagic"),   TEXT("FMagic"),
    TEXT("MOrc"),     TEXT("FOrc"),     TEXT("MShaman"),  TEXT("FShaman")};

// the player's pawn: its hair meshes' prefix, and whether its hair can change
static FORCEINLINE const TCHAR* MeshPrefix(APawn* Pawn)
{
    return GHairMeshPrefixes[Pawn->MeshType];
}
static FORCEINLINE UBOOL HairLocked(APawn* Pawn) { return Pawn->Unknown678 & 4; }

// UObject::StaticFindObject, then StaticLoadObject when not loaded yet (inline each time)
static FORCEINLINE UObject* FindOrLoad(UClass* FindClass, UClass* LoadClass, const TCHAR* Name)
{
    UObject* Object = UObject::StaticFindObject(FindClass, ANY_PACKAGE, Name, 0);
    if (!Object)
        Object = UObject::StaticLoadObject(LoadClass, NULL, Name, NULL, 0, NULL);
    return Object;
}

// An object made as the stock code makes it: StaticAllocateObject of its class, then the constructor at Ctor (the
// class's own C++ constructor, hooked there with the same arguments).
// The standalone has no stock code: the constructor that runs is T's own (each one used here is hooked at Ctor with the
// same arguments, so the hook build runs the same one through the stock address).
template <class T, class... A> static FORCEINLINE T* NewStock(DWORD Ctor, A... a)
{
#if NW_HOOK_MODE
    void* Mem = T::operator new(sizeof(T));
    return Mem ? ::nw::StockCallAt<T*>(Ctor, Mem, a...) : NULL;
#else
    (void)Ctor;
    return new T(a...);
#endif
}

// the lists of mesh numbers of this client's code hold 15 entries and the end mark (the header has 10 each: the
// lists overlap, as in the stock object)
static FORCEINLINE INT* MeshAList(NCPawnViewr_HairChangeWnd* Wnd) { return (INT*)((BYTE*)Wnd + 0xF4); }
static FORCEINLINE INT* MeshBList(NCPawnViewr_HairChangeWnd* Wnd) { return (INT*)((BYTE*)Wnd + 0x11C); }

// the number of lines of a combo box (its Items array is not typed in NCComboBox.h: Num is at +0x1E0)
static FORCEINLINE INT ComboCount(NCComboBox* Combo) { return *(INT*)((BYTE*)Combo + 0x1E0); }

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100c99f0 - the combo boxes and the button are cleared (not the check box), the two textures emptied
NCPawnViewr_HairChangeWnd::NCPawnViewr_HairChangeWnd()
{
    HairStyleCombo = NULL;
    HairColorCombo = NULL;
    MeshACombo = NULL;
    MeshBCombo = NULL;
    FaceCombo = NULL;
    ResetButton = NULL;
    memset(MeshATexture, 0, sizeof(MeshATexture));
    memset(MeshBTexture, 0, sizeof(MeshBTexture));
}
NW_HOOK_CTOR(0x100c99f0, NCPawnViewr_HairChangeWnd)

// 0x100c9ad0
NW_HOOK_DTOR(0x100c9ad0, NCPawnViewr_HairChangeWnd)

// 0x100c3a70 (vslot 25) - C2 no longer refuses the messages of a disabled window here
INT NCPawnViewr_HairChangeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_HAIR_STYLE)
            return OnStyleCombo((INT)lParam) != 0;
        if (wParam == NID_HAIR_COLOR)
            return OnColorCombo((INT)lParam) != 0;
        if (wParam == NID_HAIR_MESHA)
            return OnMeshA((INT)lParam) != 0;
        if (wParam == NID_HAIR_MESHB)
            return OnMeshB((INT)lParam) != 0;
        if (wParam == NID_HAIR_FACE)
            return OnFace((INT)lParam) != 0;
        if (wParam == NID_HAIR_RESET)
            return OnReset() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100c3a70, NCPawnViewr_HairChangeWnd, WindowProc)

// 0x100c7de0 (vslot 37) - the button, the style (0..4) and color (0..9) combos, the two mesh combos, the "together"
// check box and the face combo
INT NCPawnViewr_HairChangeWnd::OnCreate()
{
    TCHAR Text[64];
    NCWnd::OnCreate();
    ResetButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI.Control.DefaultButton"),
                                     TEXT("L2UI.Control.DefaultButton_click"), 0);
    ResetButton->ID = NID_HAIR_RESET;
    ResetButton->SetPosition(95, 200, 35, 16);
    CreateChild(ResetButton, NWS_VISIBLE);
    ResetButton->SetTextID(STR_Cancel);

    HairStyleCombo = NewStock<NCComboBox>(0x10005b20);
    HairStyleCombo->ID = NID_HAIR_STYLE;
    HairStyleCombo->SetPosition(55, 30, 35, 16);
    CreateChild(HairStyleCombo, NWS_VISIBLE);
    HairColorCombo = NewStock<NCComboBox>(0x10005b20);
    HairColorCombo->ID = NID_HAIR_COLOR;
    HairColorCombo->SetPosition(155, 30, 35, 16);
    CreateChild(HairColorCombo, NWS_VISIBLE);
    for (INT i = 0; i < 5; i++)
    {
        appSprintf(Text, TEXT("%d"), i);
        HairStyleCombo->AddItem(Text);
    }
    for (INT i = 0; i < 10; i++)
    {
        appSprintf(Text, TEXT("%d"), i);
        HairColorCombo->AddItem(Text);
    }

    MeshACombo = NewStock<NCComboBox>(0x10005b20);
    MeshACombo->ID = NID_HAIR_MESHA;
    MeshACombo->SetPosition(55, 55, 155, 16);
    CreateChild(MeshACombo, NWS_VISIBLE);
    MeshBCombo = NewStock<NCComboBox>(0x10005b20);
    MeshBCombo->ID = NID_HAIR_MESHB;
    MeshBCombo->SetPosition(55, 105, 155, 16);
    CreateChild(MeshBCombo, NWS_VISIBLE);

    TogetherCheck = NewStock<NCCheckBox>(0x100049e0, TEXT("AH & BH TOGETHER"), 0, 0, 0);
    TogetherCheck->ID = NID_HAIR_TOGETHER;
    TogetherCheck->SetPosition(20, 12, 150, 16);
    CreateChild(TogetherCheck, NWS_VISIBLE);
    TogetherCheck->bChecked = 1;

    FaceCombo = NewStock<NCComboBox>(0x10005b20);
    FaceCombo->ID = NID_HAIR_FACE;
    FaceCombo->SetPosition(55, 155, 155, 16);
    CreateChild(FaceCombo, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100c7de0, NCPawnViewr_HairChangeWnd, OnCreate)

// 0x100c62a0 (vslot 38) - the color combo and the check box are left alone
INT NCPawnViewr_HairChangeWnd::OnDestroy()
{
    DestroyChild(this, HairStyleCombo);
    DestroyChild(this, MeshACombo);
    DestroyChild(this, MeshBCombo);
    DestroyChild(this, FaceCombo);
    DestroyChild(this, ResetButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100c62a0, NCPawnViewr_HairChangeWnd, OnDestroy)

// 0x100c5ea0 (vslot 56) - the labels, the number of meshes found, the textures chosen and the face label, each
// centered in its box; the origin goes on before the clip
INT NCPawnViewr_HairChangeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewr_HairChangeWnd::OnPaint);
    TCHAR Text[64];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    NCGDevice::GetTextExtent(&Size, TEXT("HELM:"), 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 15, (16 - Size.cy) / 2 + 30, 0xFFDCDCDC, TEXT("HELM:"),
                     0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("COLOR:"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 95, (16 - Size.cy) / 2 + 30, 0xFFDCDCDC,
                     TEXT("COLOR:"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("AH(%d):"), ComboCount(MeshACombo));
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 15, (16 - Size.cy) / 2 + 55, 0xFFDCDCDC, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("BH(%d):"), ComboCount(MeshBCombo));
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 15, (16 - Size.cy) / 2 + 105, 0xFFDCDCDC, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (appStrlen(MeshATexture) > 0)
    {
        NCGDevice::GetTextExtent(&Size, MeshATexture, 0, 0, 0, 0, 0);
        Device->DrawText((220 - Size.cx) / 2 + 15, (16 - Size.cy) / 2 + 80, 0xFF808000,
                         MeshATexture, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (appStrlen(MeshBTexture) > 0)
    {
        NCGDevice::GetTextExtent(&Size, MeshBTexture, 0, 0, 0, 0, 0);
        Device->DrawText((220 - Size.cx) / 2 + 15, (16 - Size.cy) / 2 + 130, 0xFF808000,
                         MeshBTexture, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    appSprintf(Text, TEXT("FACE(%d):"), ComboCount(FaceCombo));
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 10, (16 - Size.cy) / 2 + 155, 0xFFDCDCDC, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c5ea0, NCPawnViewr_HairChangeWnd, OnPaint)

// 0x100c5810 - the two meshes chosen and their textures on the player's pawn (skeletal meshes loaded when not found),
// and the hair number and color into the chosen default character's record. With no mesh line chosen the index is -1
// and the number read before MeshANumbers is the color
__declspec(noinline) void NCPawnViewr_HairChangeWnd::OnOK()
{
    TCHAR Name[256];
    APawn* Pawn = NPawnView::PlayerPawn();
    if (!Pawn)
        return;
    if (MeshANumbers[MeshACombo->Selected] < 0 || MeshBNumbers[MeshBCombo->Selected] < 0)
        return;
    appSprintf(Name, TEXT("LineageWarriors.%s"), MeshACombo->GetItemText(MeshACombo->Selected));
    Pawn->HairMeshes[1] =
        (UMesh*)FindOrLoad(UMesh::StaticClass(), USkeletalMesh::StaticClass(), Name);
    Pawn->HairMaterials[1] =
        (UMaterial*)FindOrLoad(UMaterial::StaticClass(), UMaterial::StaticClass(), MeshATexture);
    appSprintf(Name, TEXT("LineageWarriors.%s"), MeshBCombo->GetItemText(MeshBCombo->Selected));
    Pawn->HairMeshes[0] =
        (UMesh*)FindOrLoad(UMesh::StaticClass(), USkeletalMesh::StaticClass(), Name);
    Pawn->HairMaterials[0] =
        (UMaterial*)FindOrLoad(UMaterial::StaticClass(), UMaterial::StaticClass(), MeshBTexture);
    INT Index = NPawnView::Viewer()->CharSelectWnd->NameCombo->Selected;
    NPawnView::Viewer()->Characters[Index].HairStyle = HairNumber;
    NPawnView::Viewer()->Characters[Index].HairColor = HairColor;
}
NW_HOOK_METHOD(0x100c5810, NCPawnViewr_HairChangeWnd, OnOK)

// 0x100c53a0 - the faces 0..14 of the pawn's mesh type that exist (loaded when needed) into the face combo; the combo
// goes to its first line when it had none. C3: 15 candidates, the list (FaceNumbers: 10 ints) is not that long
__declspec(noinline) void NCPawnViewr_HairChangeWnd::FillFaceList()
{
    TCHAR Path[100];
    TCHAR Name[100];
    if (HairLocked(NPawnView::PlayerPawn()))
        return;
    if (FaceCombo->Selected < 0)
        FaceCombo->SetSelected(0);
    FaceCombo->ClearItems();
    INT* Next = FaceNumbers;
    Face = FaceCombo->Selected;
    for (INT i = 0; i < 15; i++)
    {
        appSprintf(Name, TEXT("%s_m000_t0%d_f"), MeshPrefix(NPawnView::PlayerPawn()), i);
        appSprintf(Path, TEXT("%s.face.%s"), MeshPrefix(NPawnView::PlayerPawn()), Name);
        if (FindOrLoad(UMaterial::StaticClass(), UMaterial::StaticClass(), Path))
        {
            FaceCombo->AddItem(Name);
            *Next++ = i;
        }
    }
}
NW_HOOK_METHOD(0x100c53a0, NCPawnViewr_HairChangeWnd, FillFaceList)

// 0x100c5510 - the "a" and "b" hair meshes 0..14 of the style suffix chosen that exist in LineageWarriors (loaded when
// needed) into the mesh combos, each list ended by "No Mesh" (number 15); the textures emptied. C3: the lists are
// 15 long where the header's arrays are 10 (they overlap, as in the stock object)
__declspec(noinline) void NCPawnViewr_HairChangeWnd::FillMeshLists()
{
    TCHAR Path[100];
    TCHAR Name[100];
    if (HairLocked(NPawnView::PlayerPawn()))
        return;
    INT NumA = 0;
    INT NumB = 0;
    if (HairStyleCombo->Selected < 0)
        HairStyleCombo->SetSelected(0);
    if (HairColorCombo->Selected < 0)
        HairColorCombo->SetSelected(0);
    MeshACombo->ClearItems();
    MeshBCombo->ClearItems();
    memset(MeshATexture, 0, sizeof(MeshATexture));
    memset(MeshBTexture, 0, sizeof(MeshBTexture));
    HairStyle = HairStyleCombo->Selected;
    HairColor = HairColorCombo->Selected;
    INT* ListA = MeshAList(this);
    INT* ListB = MeshBList(this);
    INT i;
    for (i = 0; i < 15; i++)
    {
        appSprintf(Name, TEXT("%s_m00%d_m0%d_ah"), MeshPrefix(NPawnView::PlayerPawn()), i, HairStyle);
        appSprintf(Path, TEXT("LineageWarriors.%s"), Name);
        if (FindOrLoad(UMesh::StaticClass(), UMesh::StaticClass(), Path))
        {
            MeshACombo->AddItem(Name);
            ListA[NumA++] = i;
        }
        appSprintf(Name, TEXT("%s_m00%d_m0%d_bh"), MeshPrefix(NPawnView::PlayerPawn()), i, HairStyle);
        appSprintf(Path, TEXT("LineageWarriors.%s"), Name);
        if (FindOrLoad(UMesh::StaticClass(), UMesh::StaticClass(), Path))
        {
            MeshBCombo->AddItem(Name);
            ListB[NumB++] = i;
        }
    }
    MeshACombo->AddItem(TEXT("No Mesh"));
    ListA[NumA] = i;
    MeshBCombo->AddItem(TEXT("No Mesh"));
    ListB[NumB] = i;
}
NW_HOOK_METHOD(0x100c5510, NCPawnViewr_HairChangeWnd, FillMeshLists)

// 0x100c3b70 - the line of a mesh number in the "a" list (the line of "No Mesh" for 15), -1 when it is not there
__declspec(noinline) INT NCPawnViewr_HairChangeWnd::FindMeshA(INT Number)
{
    INT* List = MeshAList(this);
    INT i = 0;
    for (;;)
    {
        if (List[i] == Number)
            return i;
        if (List[i] == 15)
            break;
        i++;
        if (i > 14)
            return -1;
    }
    return i;
}
NW_HOOK_METHOD(0x100c3b70, NCPawnViewr_HairChangeWnd, FindMeshA)

// 0x100c3ba0 - the same in the "b" list
__declspec(noinline) INT NCPawnViewr_HairChangeWnd::FindMeshB(INT Number)
{
    INT* List = MeshBList(this);
    INT i = 0;
    for (;;)
    {
        if (List[i] == Number)
            return i;
        if (List[i] == 15)
            break;
        i++;
        if (i > 14)
            return -1;
    }
    return i;
}
NW_HOOK_METHOD(0x100c3ba0, NCPawnViewr_HairChangeWnd, FindMeshB)

// 0x100c5a00 (vslot 68) - the button: suffix, color, meshes and face all back to 0
INT NCPawnViewr_HairChangeWnd::OnReset()
{
    HairStyle = 0;
    HairStyleCombo->SetSelected(0);
    HairColor = 0;
    HairColorCombo->SetSelected(0);
    FillMeshLists();
    HairNumber = 0;
    MeshACombo->SetSelected(0);
    OnMeshA(HairNumber);
    MeshBCombo->SetSelected(HairNumber);
    OnMeshB(HairNumber);
    Face = 0;
    FaceCombo->SetSelected(0);
    OnFace(Face);
    return 1;
}
NW_HOOK_METHOD(0x100c5a00, NCPawnViewr_HairChangeWnd, OnReset)

// 0x100c5ab0 (vslot 69) - the style combo goes back to its first line (the argument is not looked at)
INT NCPawnViewr_HairChangeWnd::OnStyleCombo(INT Index)
{
    if (!HairLocked(NPawnView::PlayerPawn()))
        HairStyleCombo->SetSelected(0);
    return 1;
}
NW_HOOK_METHOD(0x100c5ab0, NCPawnViewr_HairChangeWnd, OnStyleCombo)

// 0x100c5af0 (vslot 70) - a color picked: the two meshes chosen get its textures
INT NCPawnViewr_HairChangeWnd::OnColorCombo(INT Index)
{
    if (!HairLocked(NPawnView::PlayerPawn()))
    {
        HairColor = Index;
        OnMeshA(MeshACombo->Selected);
        OnMeshB(MeshBCombo->Selected);
        OnOK();
    }
    return 1;
}
NW_HOOK_METHOD(0x100c5af0, NCPawnViewr_HairChangeWnd, OnColorCombo)

// 0x100c5b60 (vslot 71) - the "a" mesh's texture name, from the mesh number, color and style suffix ("No Mesh" empties
// it); the mesh goes on the pawn, and with the check box on the "b" mesh of the same number is picked too
INT NCPawnViewr_HairChangeWnd::OnMeshA(INT Index)
{
    INT* List = MeshAList(this);
    if (!HairLocked(NPawnView::PlayerPawn()))
    {
        if (List[Index] == 15)
            memset(MeshATexture, 0, sizeof(MeshATexture));
        else
        {
            HairNumber = List[Index];
            const TCHAR* Prefix = MeshPrefix(NPawnView::PlayerPawn());
            appSprintf(MeshATexture, TEXT("%s.%s_m00%d_t0%d_m0%d_ah"), Prefix, Prefix,
                       List[Index], HairColor, HairStyle);
        }
        OnOK();
        if (TogetherCheck->bChecked && List[Index] != 15)
        {
            INT Other = FindMeshB(List[Index]);
            if (Other != MeshBCombo->Selected)
            {
                MeshBCombo->SetSelected(Other);
                OnMeshB(Other);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c5b60, NCPawnViewr_HairChangeWnd, OnMeshA)

// 0x100c5c90 (vslot 72)
INT NCPawnViewr_HairChangeWnd::OnMeshB(INT Index)
{
    INT* List = MeshBList(this);
    if (!HairLocked(NPawnView::PlayerPawn()))
    {
        if (List[Index] == 15)
            memset(MeshBTexture, 0, sizeof(MeshBTexture));
        else
        {
            HairNumber = List[Index];
            const TCHAR* Prefix = MeshPrefix(NPawnView::PlayerPawn());
            appSprintf(MeshBTexture, TEXT("%s.%s_m00%d_t0%d_m0%d_bh"), Prefix, Prefix,
                       List[Index], HairColor, HairStyle);
        }
        OnOK();
        if (TogetherCheck->bChecked && List[Index] != 15)
        {
            INT Other = FindMeshA(List[Index]);
            if (Other != MeshACombo->Selected)
            {
                MeshACombo->SetSelected(Other);
                OnMeshA(Other);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c5c90, NCPawnViewr_HairChangeWnd, OnMeshB)

// 0x100c5dc0 (vslot 73) - a face picked: its number into the chosen default character's record, which dresses the
// player's pawn again
INT NCPawnViewr_HairChangeWnd::OnFace(INT Index)
{
    if (!HairLocked(NPawnView::PlayerPawn()))
    {
        Face = FaceNumbers[Index];
        if (Face < 0)
        {
            Face = 0;
            FaceCombo->SetSelected(0);
        }
        APawn* Pawn = NPawnView::PlayerPawn();
        if (Pawn)
        {
            INT Character = NPawnView::Viewer()->CharSelectWnd->NameCombo->Selected;
            NPawnView::Viewer()->Characters[Character].Pawn = Pawn;
            NPawnView::Viewer()->Characters[Character].Face = Face;
            NPawnView::Viewer()->Characters[Character].SetPawnResource();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100c5dc0, NCPawnViewr_HairChangeWnd, OnFace)

// 0x100c82c0 - the values of the character chosen in the character window into the hair controls: the suffix 0, the
// color, the hair number into both mesh combos, the face
__declspec(noinline) void NCPawnViewr_HairChangeWnd::SyncFromCharacter()
{
    INT Index = NPawnView::Viewer()->CharSelectWnd->NameCombo->Selected;
    HairStyle = 0;
    HairStyleCombo->SetSelected(0);
    HairColor = NPawnView::Viewer()->Characters[Index].HairColor;
    HairColorCombo->SetSelected(HairColor);
    FillMeshLists();
    HairNumber = NPawnView::Viewer()->Characters[Index].HairStyle;
    MeshACombo->SetSelected(HairNumber);
    OnMeshA(HairNumber);
    MeshBCombo->SetSelected(HairNumber);
    OnMeshB(HairNumber);
    FillFaceList();
    Face = NPawnView::Viewer()->Characters[Index].Face;
    FaceCombo->SetSelected(Face);
    OnFace(Face);
}
NW_HOOK_METHOD(0x100c82c0, NCPawnViewr_HairChangeWnd, SyncFromCharacter)

NW_CHECK_VTABLE(NCPawnViewr_HairChangeWnd, 0x10195818, 74)
