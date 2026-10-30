/*=============================================================================
    NCVideoOptionWnd.cpp - the video page of the options. C3: two more controls (the glow effect box and the
    anti-aliasing combo), the labels of the check boxes belong to the boxes, the page draws eleven headings in the
    skin's text color.
=============================================================================*/
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#if NW_FEATURES
// phase 6: WindowFix (the "Full Screen" box, docs/notas/F6_WindowFix.md)
#include "features/WindowFix.h"
#endif

// 0x100bd3c0 - NCWnd's fields by the base constructor; the three spare pointers
NCVideoOptionWnd::NCVideoOptionWnd()
{
    UnusedD0[0] = NULL;
    UnusedD0[1] = NULL;
    UnusedD0[2] = NULL;
}
NW_HOOK_CTOR(0x100bd3c0, NCVideoOptionWnd)

// 0x100bd420
NW_HOOK_DTOR(0x100bd420, NCVideoOptionWnd)

// 0x100b6830 (vslot 25)
INT NCVideoOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_VIDEO_RESOLUTION)
            return OnResolutionChanged(lParam) != 0;
        if (wParam == NID_VIDEO_REFRESH)
            return OnRefreshChanged(lParam) != 0;
        if (wParam == (WPARAM)NID_VIDEO_LOWERDETAIL)
            return OnLowerDetailClicked() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b6830, NCVideoOptionWnd, WindowProc)

// one combo box of the page (repeated inline in the stock OnCreate): 125 x 17 at x 100
static FORCEINLINE NCComboBox* NewCombo(NCWnd* Parent, INT ID, INT Y, INT Flags = NWS_VISIBLE)
{
    NCComboBox* Combo = new NCComboBox;
    Combo->ID = ID;
    Combo->SetPosition(100, Y, 125, 17);
    Parent->CreateChild(Combo, Flags);
    return Combo;
}

// one check box, labelled by its sysstring line (the stock constructor 0x100049E0: label, checked, label left, 0; the
// header's (const TCHAR*) one is the C1 address and traps)
static FORCEINLINE NCCheckBox* NewBox(NCWnd* Parent, INT LabelID, INT ID, INT X, INT Y, INT W)
{
    NCCheckBox* Box = new NCCheckBox(*NCObject::SysString(LabelID), 0, 0, 0);   // 0x100049E0 (ret 0x10)
    Box->ID = ID;
    Box->SetPosition(X, Y, W, 17);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x100bbd60 (vslot 37) - C3: 125 x 17 combos; the boxes name themselves (the page draws no labels for them), the
// shadow and ground detail boxes side by side, the texture filter / lower detail pair on the right and the glow
// (sysstring 867, the stock's integer constructor 0x10004B50) on the left of them; the anti-aliasing combo is the
// last row of combos
INT NCVideoOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    ResolutionCombo = NewCombo(this, NID_VIDEO_RESOLUTION, 13, NWS_VISIBLE | 0x10);   // 0x10: meaning unknown
    RefreshCombo = NewCombo(this, NID_VIDEO_REFRESH, 32);
    GammaCombo = NewCombo(this, NID_VIDEO_GAMMA, 51);
    TextureCombo = NewCombo(this, NID_VIDEO_TEXTURE, 70);
    ModelingCombo = NewCombo(this, NID_VIDEO_MODELING, 89);
    AnimationCombo = NewCombo(this, NID_VIDEO_ANIMATION, 108);
    ShadowBox = NewBox(this, 212, NID_VIDEO_SHADOW, 39, 143, 109);
    GroundDetailBox = NewBox(this, 213, NID_VIDEO_GROUNDDETAIL, 149, 143, 109);
    TerrainRangeCombo = NewCombo(this, NID_VIDEO_TERRAINRANGE, 172);
    PawnRangeCombo = NewCombo(this, NID_VIDEO_PAWNRANGE, 191);
    ActorLimitCombo = NewCombo(this, NID_VIDEO_ACTORLIMIT, 210);
    ReflectionCombo = NewCombo(this, NID_VIDEO_REFLECTION, 229);
    AntialiasCombo = NewCombo(this, NID_VIDEO_ANTIALIAS, 248);
    CursorBox = NewBox(this, 305, NID_VIDEO_CURSOR, 39, 300, 100);
    TextureFilterBox = NewBox(this, 464, NID_VIDEO_TEXTUREFILTER, 144, 283, 100);
    LowerDetailBox = NewBox(this, 370, NID_VIDEO_LOWERDETAIL, 144, 300, 100);
    GlowBox = new NCCheckBox(867, 0, 0);   // 0x10004B50 (ret 0xc): the label is system string 867
    GlowBox->ID = NID_VIDEO_GLOW;
    GlowBox->SetPosition(39, 283, 100, 17);
    CreateChild(GlowBox, NWS_VISIBLE);
    InitItems();
    return 1;
}
NW_HOOK_METHOD(0x100bbd60, NCVideoOptionWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100b78e0 (vslot 38) - guarded; the children in the stock's order, the two new ones last
INT NCVideoOptionWnd::OnDestroy()
{
    guard(NCVideoOptionWnd::OnDestroy);
    DestroyChild(this, ResolutionCombo);
    DestroyChild(this, RefreshCombo);
    DestroyChild(this, GammaCombo);
    DestroyChild(this, TextureCombo);
    DestroyChild(this, ModelingCombo);
    DestroyChild(this, AnimationCombo);
    DestroyChild(this, ShadowBox);
    DestroyChild(this, GroundDetailBox);
    DestroyChild(this, TerrainRangeCombo);
    DestroyChild(this, PawnRangeCombo);
    DestroyChild(this, ActorLimitCombo);
    DestroyChild(this, CursorBox);
    DestroyChild(this, ReflectionCombo);
    DestroyChild(this, LowerDetailBox);
    DestroyChild(this, TextureFilterBox);
    DestroyChild(this, UnusedD0[0]);
    DestroyChild(this, UnusedD0[1]);
    DestroyChild(this, UnusedD0[2]);
    DestroyChild(this, GlowBox);
    DestroyChild(this, AntialiasCombo);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b78e0, NCVideoOptionWnd, OnDestroy)

// a heading right-aligned on x 98, in the skin's text color (repeated inline in the stock OnPaint)
static FORCEINLINE void DrawLabel(NCGDevice* Device, INT TextID, INT Y)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(TextID), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, Y, 0xFFDCDCDC, *NCObject::SysString(TextID), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     0, 0);
}

// the same, when the text pointer is not NULL
static FORCEINLINE void DrawOptionLabel(NCGDevice* Device, INT TextID, INT Y)
{
    if (*NCObject::SysString(TextID))
        DrawLabel(Device, TextID, Y);
}

// 0x100b7340 (vslot 56) - C3: no clip margin; the boxes are not headed any more (they carry their labels), a new
// heading for the anti-aliasing combo
INT NCVideoOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVideoOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    DrawOptionLabel(Device, 209, 16);    // Resolution
    DrawOptionLabel(Device, 351, 35);    // Refresh Rate
    DrawOptionLabel(Device, 210, 54);    // Gamma
    DrawOptionLabel(Device, 211, 73);    // Texture
    DrawOptionLabel(Device, 594, 92);    // Modeling Detail
    DrawOptionLabel(Device, 607, 111);   // Animation
    DrawOptionLabel(Device, 214, 175);   // Terrain Range
    DrawOptionLabel(Device, 215, 194);   // PC/NPC Range
    DrawOptionLabel(Device, 540, 213);   // PC/NPC Limit
    DrawOptionLabel(Device, 352, 232);   // Reflections
    DrawLabel(Device, 868, 251);         // Anti-Aliasing
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b7340, NCVideoOptionWnd, OnPaint)

// 0x100b7da0 (vslot 68) - the rates of the picked mode; the one in use stays selected, else the last
INT NCVideoOptionWnd::OnResolutionChanged(INT Index)
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    // the stock buffer is 256 TCHARs and the stock spills Bits[Index] to the frame; ours keeps it in a register, so
    // two more TCHARs bring the frame back to the stock size plus the /GS cookie (tools/port/check_frames.py)
    TCHAR Text[258];
    INT Rate = GRefreshRate;
    INT ModeCount = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    if (Index >= 0 && Index < ModeCount)
    {
        INT RateIndex = RefreshCombo->Selected;
        INT RateCount = GConsole->Engine->GRenDev->GetRefreshRate(100, Widths[Index], Heights[Index], Bits[Index],
                                                                  Rates);
        if (RateIndex >= 0 && RateIndex < RateCount)
            Rate = Rates[RateIndex];
        RefreshCombo->ClearItems();
        for (INT i = 0; i < RateCount; i++)
        {
            appSprintf(Text, TEXT("%dHz"), Rates[i]);
            RefreshCombo->AddItem(Text);
        }
        INT j;
        for (j = 0; j < RateCount; j++)
            if (Rates[j] == Rate)
            {
                RefreshCombo->SetSelected(j);
                break;
            }
        if (j == RateCount)
            RefreshCombo->SetSelected(RateCount - 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x100b7da0, NCVideoOptionWnd, OnResolutionChanged)

// "Lower Detail" fixes the detail controls; free again, reflections still need pixel shader 1.2 and the glow pixel
// and vertex shaders 2.0 (C3; repeated inline in LoadSettings and OnLowerDetailClicked)
static FORCEINLINE void EnableDetails(NCVideoOptionWnd* Wnd, UBOOL bEnable)
{
    Wnd->TextureCombo->EnableWindow(bEnable);
    Wnd->ModelingCombo->EnableWindow(bEnable);
    Wnd->AnimationCombo->EnableWindow(bEnable);
    Wnd->ShadowBox->bDisabled = !bEnable;
    Wnd->GroundDetailBox->bDisabled = !bEnable;
    Wnd->TerrainRangeCombo->EnableWindow(bEnable);
    Wnd->PawnRangeCombo->EnableWindow(bEnable);
    if (bEnable)
    {
        if (GConsole->Engine->GRenDev->GetRenderCaps()->PixelShaderVersion >= 12)
            Wnd->ReflectionCombo->EnableWindow(1);
        else
        {
            Wnd->ReflectionCombo->EnableWindow(0);
            Wnd->ReflectionCombo->SetSelected(0);
        }
        INT VertexShader, PixelShader;   // names ours
        GConsole->Engine->GRenDev->GetShaderVersion(VertexShader, PixelShader);
        if (VertexShader >= 20 && PixelShader >= 20)
            Wnd->GlowBox->bDisabled = 0;
        else
            Wnd->GlowBox->bDisabled = 1;
    }
    else
    {
        Wnd->ReflectionCombo->EnableWindow(0);
        Wnd->ReflectionCombo->SetSelected(0);
        Wnd->GlowBox->bDisabled = 1;
    }
}

// 0x100b9490 (vslot 70)
INT NCVideoOptionWnd::OnLowerDetailClicked()
{
    if (LowerDetailBox->bChecked)
        EnableDetails(this, 0);
    else
        EnableDetails(this, 1);
    return 1;
}
NW_HOOK_METHOD(0x100b9490, NCVideoOptionWnd, OnLowerDetailClicked)

// 0x100b68d0 - the modes and rates of the device, the one in use selected
void NCVideoOptionWnd::InitModes()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    TCHAR Text[100];
    ResolutionCombo->ClearItems();
    RefreshCombo->ClearItems();
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
    {
        appSprintf(Text, TEXT("%d*%d %dbit"), Widths[i], Heights[i], Bits[i]);
        ResolutionCombo->AddItem(Text);
        if (GSEKScreenX == Widths[i] && GSEKScreenY == Heights[i] && GColorBits == Bits[i])
            ResolutionCombo->SetSelected(i);
    }
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
    {
        appSprintf(Text, TEXT("%dHz"), Rates[j]);
        RefreshCombo->AddItem(Text);
        if (GRefreshRate == Rates[j])
            RefreshCombo->SetSelected(j);
    }
}
NW_HOOK_METHOD(0x100b68d0, NCVideoOptionWnd, InitModes)

// a check box follows its setting (repeated inline in the stock code)
static FORCEINLINE void SetBox(NCCheckBox* Box, UBOOL bValue)
{
    if (bValue)
        Box->bChecked = 1;
    else
        Box->bChecked = 0;
}

// 0x100b7ef0 - the controls follow the globals. C3: "Lower Detail" no longer leaves a disabled box alone; at the end
// the anti-aliasing level (only with a multisampling device) and the glow box
void NCVideoOptionWnd::LoadSettings()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
        if (GSEKScreenX == Widths[i] && GSEKScreenY == Heights[i] && GColorBits == Bits[i])
            ResolutionCombo->SetSelected(i);
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
        if (GRefreshRate == Rates[j])
            RefreshCombo->SetSelected(j);

    UClient* Client = GConsole->Engine->Client;
    if (Client)
    {
        FLOAT Gamma = Client->Gamma;
        if (Gamma >= 1.2f)
            GammaCombo->SetSelected(0);
        else if (Gamma >= 1.0f && Gamma < 1.2f)
            GammaCombo->SetSelected(1);
        else if (Gamma >= 0.8f && Gamma < 1.0f)
            GammaCombo->SetSelected(2);
        else if (Gamma >= 0.6f && Gamma < 0.8f)
            GammaCombo->SetSelected(3);
        else if (Gamma < 0.6f)
            GammaCombo->SetSelected(4);
    }

    if (GL2OriginPawnCR < 1.5f)
        PawnRangeCombo->SetSelected(4);
    else if (GL2OriginPawnCR >= 1.5f && GL2OriginPawnCR < 2.0f)
        PawnRangeCombo->SetSelected(3);
    else if (GL2OriginPawnCR >= 2.0f && GL2OriginPawnCR < 2.5f)
        PawnRangeCombo->SetSelected(2);
    else if (GL2OriginPawnCR >= 2.5f && GL2OriginPawnCR < 3.0f)
        PawnRangeCombo->SetSelected(1);
    else if (GL2OriginPawnCR >= 3.0f)
        PawnRangeCombo->SetSelected(0);

    if (GL2OriginTerrainCR < 5.0f)
        TerrainRangeCombo->SetSelected(4);
    else if (GL2OriginTerrainCR >= 5.0f && GL2OriginTerrainCR < 6.0f)
        TerrainRangeCombo->SetSelected(3);
    else if (GL2OriginTerrainCR >= 6.0f && GL2OriginTerrainCR < 7.0f)
        TerrainRangeCombo->SetSelected(2);
    else if (GL2OriginTerrainCR >= 7.0f && GL2OriginTerrainCR < 8.0f)
        TerrainRangeCombo->SetSelected(1);
    else if (GL2OriginTerrainCR >= 8.0f)
        TerrainRangeCombo->SetSelected(0);

    SetBox(GroundDetailBox, GL2RenderDeco);
    SetBox(ShadowBox, GIsL2Projector);
    INT Texture = GL2TextureDetail;
    if (Texture >= 2)
        Texture = 2;
    else if (Texture <= 0)
        Texture = 0;
    TextureCombo->SetSelected(Texture);
    ModelingCombo->SetSelected(GL2UseReducePawnLOD != 0);
    AnimationCombo->SetSelected(GL2UseSkipAnim != 0);
    SetBox(CursorBox, GL2UseColorCursor);
    if (GIsL2WaterEffect && GIsL2WaterEffectType == 1)
        ReflectionCombo->SetSelected(1);
    else if (GIsL2WaterEffect && GIsL2WaterEffectType == 2)
        ReflectionCombo->SetSelected(2);
    else
        ReflectionCombo->SetSelected(0);
    SetBox(TextureFilterBox, GL2UseTrilinear);
    if (GConsole->Engine->GRenDev->GetRenderCaps()->PixelShaderVersion < 12)
    {
        ReflectionCombo->EnableWindow(0);
        ReflectionCombo->SetSelected(0);
    }
    else
        ReflectionCombo->EnableWindow(1);

    if (GL2RenderActorLimitOpt >= 6)
        ActorLimitCombo->SetSelected(0);
    else if (GL2RenderActorLimitOpt == 5)
        ActorLimitCombo->SetSelected(1);
    else if (GL2RenderActorLimitOpt == 4)
        ActorLimitCombo->SetSelected(2);
    else if (GL2RenderActorLimitOpt == 3)
        ActorLimitCombo->SetSelected(3);
    else if (GL2RenderActorLimitOpt == 2)
        ActorLimitCombo->SetSelected(4);
    else if (GL2RenderActorLimitOpt <= 1)
        ActorLimitCombo->SetSelected(5);

    if (GL2KeepMinFrameRate)
    {
        LowerDetailBox->bChecked = 1;
        EnableDetails(this, 0);
    }
    else
    {
        LowerDetailBox->bChecked = 0;
        EnableDetails(this, 1);
    }
    AntialiasCombo->SetSelected(GConsole->Engine->GRenDev->GetMultiSample() > 0 ? GL2Antialiasing : 0);
    GlowBox->bChecked = GL2GlowEffect;
}
NW_HOOK_METHOD(0x100b7ef0, NCVideoOptionWnd, LoadSettings)

#if NW_FEATURES
// phase 6 - the "Full Screen" box follows the viewport, not the last click: Alt+Enter can have
// switched the mode since the page was built, and OK would otherwise switch it back
void NCVideoOptionWnd::SyncModeBox()
{
    NCCheckBox* Box = static_cast<NCCheckBox*>(UnusedD0[0]);
    if (Box)
        Box->bChecked = nwf::WindowFix_IsFullscreen(Box->bChecked != 0);
}
#endif

// a setting saved as its number (repeated inline in the stock Apply)
static FORCEINLINE void SaveInt(const TCHAR* Section, const TCHAR* Key, INT Value)
{
    GConfig->SetString(Section, Key, appItoa(Value), TEXT("Option.ini"));
}

// a setting saved "%f" through the caller's buffer (repeated inline in the stock Apply)
static FORCEINLINE void SaveFloat(TCHAR* Text, const TCHAR* Section, const TCHAR* Key, FLOAT Value)
{
    appSprintf(Text, TEXT("%f"), Value);
    GConfig->SetString(Section, Key, Text, TEXT("Option.ini"));
}

// the fog of a terrain range step (env.int [FOG]), with its defaults (repeated inline in the stock Apply)
static FORCEINLINE void LoadFogRange(const TCHAR* StartKey, const TCHAR* EndKey, FLOAT DefaultStart, FLOAT DefaultEnd,
                                     FLOAT& Start, FLOAT& End)
{
    if (!GConfig->GetFloat(TEXT("FOG"), StartKey, Start, TEXT("env.int")))
        Start = DefaultStart;
    if (!GConfig->GetFloat(TEXT("FOG"), EndKey, End, TEXT("env.int")))
        End = DefaultEnd;
}

// 0x100b8550 - the page saved into the globals, the device and Option.ini. C3: the color cursor switch only acts when
// it goes off; the glow (needs the shaders and not "Lower Detail") and the anti-aliasing level (a change re-sets the
// device's mode with its own parameters) are saved at the end
void NCVideoOptionWnd::Apply()
{
    TCHAR Text[256];
    UBOOL bFullscreen = GConsole->Engine->Client->Viewports(0)->IsFullscreen();

    INT OldTextureDetail = GL2TextureDetail;
    if (GL2KeepMinFrameRate)
        OldTextureDetail = 2;
    if (!LowerDetailBox->bChecked)
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("false"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 0;
    }
    else
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("true"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 1;
    }

    UBOOL bOldTrilinear = GConsole->Engine->GRenDev->IsUseTrilinear();
    if (!TextureFilterBox->bChecked)
    {
        GL2UseTrilinear = 0;
        GConfig->SetString(TEXT("Video"), TEXT("UseTrilinear"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GL2UseTrilinear = 1;
        GConfig->SetString(TEXT("Video"), TEXT("UseTrilinear"), TEXT("true"), TEXT("Option.ini"));
    }
    if (bOldTrilinear != GL2UseTrilinear)
        GConsole->Engine->GRenDev->SetUseTrilinear(GL2UseTrilinear);

    INT Texture = TextureCombo->Selected;
    if (Texture == 0)
        GL2TextureDetail = 0;
    else if (Texture == 1)
        GL2TextureDetail = 1;
    else
        GL2TextureDetail = 2;
    SaveInt(TEXT("Video"), TEXT("TextureDetail"), GL2TextureDetail);
    INT NewTextureDetail = GL2TextureDetail;
    if (GL2KeepMinFrameRate)
        NewTextureDetail = 2;
    if (OldTextureDetail != NewTextureDetail)
        GConsole->Engine->GRenDev->DeleteAllTexture();

    GL2UseReducePawnLOD = ModelingCombo->Selected != 0;
    SaveInt(TEXT("Video"), TEXT("ModelDetail"), GL2UseReducePawnLOD);
    GL2UseSkipAnim = AnimationCombo->Selected != 0;
    SaveInt(TEXT("Video"), TEXT("SkipAnim"), GL2UseSkipAnim);

    INT Widths[100], Heights[100], Bits[100], Rates[100];
    INT Mode = ResolutionCombo->Selected;
    INT ModeCount = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    if (Mode >= 0 && Mode < ModeCount)
    {
        INT RateIndex = RefreshCombo->Selected;
        INT NewX = Widths[Mode];
        INT NewY = Heights[Mode];
        INT NewBits = Bits[Mode];
        INT RateCount = GConsole->Engine->GRenDev->GetRefreshRate(100, NewX, NewY, NewBits, Rates);
        if (RateIndex >= 0 && RateIndex < RateCount)
        {
            INT Rate = Rates[RateIndex];
            if (GSEKScreenX != NewX || GSEKScreenY != NewY || GColorBits != NewBits || GRefreshRate != Rate)
            {
                if (GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), NewX, NewY, NewBits / 8,
                                                      Rate, bFullscreen, 1))
                {
                    GColorBits = NewBits;
                    GRefreshRate = Rate;
                }
                else if (GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), 1024, 768, 4, 60,
                                                           bFullscreen, 1))
                {
                    GColorBits = 32;
                    GRefreshRate = 60;
                    InitModes();
                }
            }
            SaveInt(TEXT("Video"), TEXT("GamePlayViewportX"), GSEKScreenX);
            SaveInt(TEXT("Video"), TEXT("GamePlayViewportY"), GSEKScreenY);
            SaveInt(TEXT("Video"), TEXT("ColorBits"), GColorBits);
            SaveInt(TEXT("Video"), TEXT("RefreshRate"), GRefreshRate);
        }
    }

    UClient* Client = GConsole->Engine->Client;
    if (Client)
    {
        FLOAT Gamma = Client->Gamma;
        switch (GammaCombo->Selected)
        {
        case 0: Gamma = 1.2f; break;
        case 1: Gamma = 1.0f; break;
        case 2: Gamma = 0.8f; break;
        case 3: Gamma = 0.6f; break;
        case 4: Gamma = 0.4f; break;
        }
        if (Gamma != Client->Gamma)
        {
            Client->Gamma = Gamma;
            SaveFloat(Text, TEXT("Video"), TEXT("Gamma"), Gamma);
            GConsole->Engine->UpdateGamma();
        }
    }

    switch (PawnRangeCombo->Selected)
    {
    case 0: GL2OriginPawnCR = 3.0f; break;
    case 1: GL2OriginPawnCR = 2.5f; break;
    case 2: GL2OriginPawnCR = 2.0f; break;
    case 3: GL2OriginPawnCR = 1.5f; break;
    case 4: GL2OriginPawnCR = 1.0f; break;
    }
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Pawn"), GL2OriginPawnCR);

    switch (TerrainRangeCombo->Selected)
    {
    case 0:
        GL2OriginTerrainCR = 8.0f;
        GL2OriginStaticMeshCR = 4.0f;
        GL2OriginStaticMeshLodCR = 6.0f;
        GL2ActorCR = 6.0f;
        break;
    case 1:
        GL2OriginTerrainCR = 7.0f;
        GL2OriginStaticMeshCR = 3.5f;
        GL2OriginStaticMeshLodCR = 5.5f;
        GL2ActorCR = 5.5f;
        break;
    case 2:
        GL2OriginTerrainCR = 6.0f;
        GL2OriginStaticMeshCR = 3.0f;
        GL2OriginStaticMeshLodCR = 5.0f;
        GL2ActorCR = 5.0f;
        break;
    case 3:
        GL2OriginTerrainCR = 5.0f;
        GL2OriginStaticMeshCR = 2.5f;
        GL2OriginStaticMeshLodCR = 4.5f;
        GL2ActorCR = 4.5f;
        break;
    case 4:
        GL2OriginTerrainCR = 4.0f;
        GL2OriginStaticMeshCR = 2.0f;
        GL2OriginStaticMeshLodCR = 4.0f;
        GL2ActorCR = 4.0f;
        break;
    }
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Terrain"), GL2OriginTerrainCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Actor"), GL2ActorCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("StaticMesh"), GL2OriginStaticMeshCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("StaticMeshLod"), GL2OriginStaticMeshLodCR);

    if (!GroundDetailBox->bChecked)
    {
        GL2RenderDeco = 0;
        GConfig->SetString(TEXT("Video"), TEXT("RenderDeco"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GL2RenderDeco = 1;
        GConfig->SetString(TEXT("Video"), TEXT("RenderDeco"), TEXT("true"), TEXT("Option.ini"));
    }
    if (!ShadowBox->bChecked)
    {
        GIsL2Projector = 0;
        GConfig->SetString(TEXT("Video"), TEXT("PawnShadow"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GIsL2Projector = 1;
        GConfig->SetString(TEXT("Video"), TEXT("PawnShadow"), TEXT("true"), TEXT("Option.ini"));
    }
    if (!CursorBox->bChecked)
    {
        GL2UseColorCursor = 0;
        GConfig->SetString(TEXT("Video"), TEXT("UseColorCursor"), TEXT("false"), TEXT("Option.ini"));
        SetCursor(0, NULL);
        LockCursor();
    }
    else
    {
        GL2UseColorCursor = 1;
        GConfig->SetString(TEXT("Video"), TEXT("UseColorCursor"), TEXT("true"), TEXT("Option.ini"));
    }

    INT Reflection = ReflectionCombo->Selected;
    if (!Reflection)
    {
        GIsL2WaterEffect = 0;
        GConfig->SetString(TEXT("L2WaterEffect"), TEXT("IsUseEffect"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GIsL2WaterEffect = 1;
        GConfig->SetString(TEXT("L2WaterEffect"), TEXT("IsUseEffect"), TEXT("true"), TEXT("Option.ini"));
    }
    GIsL2WaterEffectType = Reflection;
    SaveInt(TEXT("L2WaterEffect"), TEXT("EffectType"), Reflection);

    if (!LowerDetailBox->bChecked)
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("false"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 0;
    }
    else
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("true"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 1;
    }

    // the fog follows the terrain range; "Lower Detail" takes the nearest
    FLOAT FogStart = 1.5f;
    FLOAT FogEnd = 5.5f;
    if (GL2KeepMinFrameRate || GL2OriginTerrainCR < 5.0f)
        LoadFogRange(TEXT("StartRange1"), TEXT("EndRange1"), 0.5f, 3.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 5.0f && GL2OriginTerrainCR < 6.0f)
        LoadFogRange(TEXT("StartRange2"), TEXT("EndRange2"), 1.0f, 4.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 6.0f && GL2OriginTerrainCR < 7.0f)
        LoadFogRange(TEXT("StartRange3"), TEXT("EndRange3"), 1.5f, 5.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 7.0f && GL2OriginTerrainCR < 8.0f)
        LoadFogRange(TEXT("StartRange4"), TEXT("EndRange4"), 2.0f, 6.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 8.0f)
        LoadFogRange(TEXT("StartRange5"), TEXT("EndRange5"), 2.5f, 7.5f, FogStart, FogEnd);
    GL2FogStart = FogStart;
    GL2FogEnd = FogEnd;

    switch (ActorLimitCombo->Selected)
    {
    case 0: GL2RenderActorLimitOpt = 6; GL2RenderActorLimit = 10000; break;
    case 1: GL2RenderActorLimitOpt = 5; GL2RenderActorLimit = 400; break;
    case 2: GL2RenderActorLimitOpt = 4; GL2RenderActorLimit = 300; break;
    case 3: GL2RenderActorLimitOpt = 3; GL2RenderActorLimit = 200; break;
    case 4: GL2RenderActorLimitOpt = 2; GL2RenderActorLimit = 100; break;
    case 5: GL2RenderActorLimitOpt = 1; GL2RenderActorLimit = 50; break;
    }
    appSprintf(Text, TEXT("%d"), GL2RenderActorLimitOpt);
    GConfig->SetString(TEXT("Video"), TEXT("RenderActorLimited"), Text, TEXT("Option.ini"));

    // C3: the glow needs the box and not "Lower Detail"
    if (GlowBox->bChecked && !GL2KeepMinFrameRate)
        GL2GlowEffect = 1;
    else
        GL2GlowEffect = 0;
    GConfig->SetBool(TEXT("Video"), TEXT("Glow"), GL2GlowEffect, TEXT("Option.ini"));

    INT Level = AntialiasCombo->Selected;
    if (GL2Antialiasing != Level)
    {
        GL2Antialiasing = Level;
        GConfig->SetInt(TEXT("Video"), TEXT("AntiAliasing"), GL2Antialiasing, TEXT("Option.ini"));
        // the device takes the new level with the mode it has now (the 6-argument SetRes; the names are ours, by how
        // the stock hands the values over)
        INT RefreshRate, CurrentX, CurrentY, Unused, ColorBytes;
        GConsole->Engine->GRenDev->GetResParams(RefreshRate, CurrentX, CurrentY, Unused, ColorBytes);
        GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), CurrentX, CurrentY, ColorBytes,
                                          RefreshRate, 1);
    }
}
NW_HOOK_METHOD(0x100b8550, NCVideoOptionWnd, Apply)

// 0x100b7cd0 - every control back to empty / off (C3: the glow box and the anti-aliasing list too)
void NCVideoOptionWnd::ClearItems()
{
    ResolutionCombo->ClearItems();
    RefreshCombo->ClearItems();
    GammaCombo->ClearItems();
    TextureCombo->ClearItems();
    ModelingCombo->ClearItems();
    AnimationCombo->ClearItems();
    ShadowBox->bChecked = 0;
    GroundDetailBox->bChecked = 0;
    TerrainRangeCombo->ClearItems();
    PawnRangeCombo->ClearItems();
    ActorLimitCombo->ClearItems();
    CursorBox->bChecked = 0;
    ReflectionCombo->ClearItems();
    LowerDetailBox->bChecked = 0;
    TextureFilterBox->bChecked = 0;
    GlowBox->bChecked = 0;
    AntialiasCombo->ClearItems();
}
NW_HOOK_METHOD(0x100b7cd0, NCVideoOptionWnd, ClearItems)

// the gamma, range and limit boxes list "5" .. "1" (repeated inline in the stock InitItems)
static FORCEINLINE void AddLevels(NCComboBox* Combo)
{
    Combo->AddItem(TEXT("5"));
    Combo->AddItem(TEXT("4"));
    Combo->AddItem(TEXT("3"));
    Combo->AddItem(TEXT("2"));
    Combo->AddItem(TEXT("1"));
}

// 0x100bb440 - the lists; C3: the anti-aliasing combo lists None / 2x / 4x as far as the device multisamples (and is
// disabled without)
void NCVideoOptionWnd::InitItems()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    TCHAR Text[100];
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
    {
        appSprintf(Text, TEXT("%d*%d %dbit"), Widths[i], Heights[i], Bits[i]);
        ResolutionCombo->AddItem(Text);
    }
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
    {
        appSprintf(Text, TEXT("%dHz"), Rates[j]);
        RefreshCombo->AddItem(Text);
    }
    AddLevels(GammaCombo);
    TextureCombo->AddItem(0xE0);
    TextureCombo->AddItem(0xE1);
    TextureCombo->AddItem(0xE2);
    ModelingCombo->AddItem(0xE0);
    ModelingCombo->AddItem(0xE2);
    AnimationCombo->AddItem(0xE0);
    AnimationCombo->AddItem(0xE2);
    AddLevels(TerrainRangeCombo);
    AddLevels(PawnRangeCombo);
    ActorLimitCombo->AddItem(0x1B);
    AddLevels(ActorLimitCombo);
    ReflectionCombo->AddItem(0x1B);
    ReflectionCombo->AddItem(0x161);
    ReflectionCombo->AddItem(0x162);
    INT Samples = GConsole->Engine->GRenDev->GetMultiSample();
    if (Samples == 0)
    {
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->EnableWindow(0);
    }
    else if (Samples == 1)
    {
        AntialiasCombo->EnableWindow(1);
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->AddItem(0x366);
    }
    else if (Samples == 2)
    {
        AntialiasCombo->EnableWindow(1);
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->AddItem(0x366);
        AntialiasCombo->AddItem(0x367);
    }
    LoadSettings();
}
NW_HOOK_METHOD(0x100bb440, NCVideoOptionWnd, InitItems)

// 0x100b6a40 (vslot 44) - the options window (the parent) takes the backdrop of this page
void NCVideoOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    ((NCOptionWnd*)Entry->Parent->Wnd)->OptionBackTex = FindTexture(TEXT("L2ui_ch3.option1_back"));
}
NW_HOOK_METHOD(0x100b6a40, NCVideoOptionWnd, ShowWindow)

NW_CHECK_VTABLE(NCVideoOptionWnd, 0x10193018, 71)
