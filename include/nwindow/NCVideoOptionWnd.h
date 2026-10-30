#pragma once
// NCVideoOptionWnd - the video page of the options window (UClass "CVideoOptionWnd", 0xDC):
// display mode and refresh rate (from the render device), gamma, texture / model / animation
// detail, shadows, ground detail, terrain and PC/NPC ranges, PC/NPC limit, reflections, color
// cursor, texture filter and "Lower Detail" (which fixes the detail controls low). Apply sets
// the globals and the render device and saves Option.ini ([Video], [ClippingRange],
// [L2WaterEffect]).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCComboBox;
class NCCheckBox;

// NWM_COMMAND ids of the controls
enum
{
    NID_VIDEO_RESOLUTION = 0x1F08,
    NID_VIDEO_GAMMA = 0x1F09,
    NID_VIDEO_TEXTURE = 0x1F0A,
    NID_VIDEO_TERRAINRANGE = 0x1F0D,
    NID_VIDEO_PAWNRANGE = 0x1F0E,
    NID_VIDEO_CURSOR = 0x1F0F,
    NID_VIDEO_REFLECTION = 0x1F11,
    NID_VIDEO_REFRESH = 0x1F14,
    NID_VIDEO_ACTORLIMIT = 0x1F17,
    NID_VIDEO_MODELING = 0x1F18,
    NID_VIDEO_ANIMATION = 0x1F19,
    NID_VIDEO_ANTIALIAS = 0x1F20,   // C3
    NID_VIDEO_LOWERDETAIL = (INT)0xF0F01465,
    NID_VIDEO_SHADOW = (INT)0xF0F01467,
    NID_VIDEO_GROUNDDETAIL = (INT)0xF0F01468,
    NID_VIDEO_TEXTUREFILTER = (INT)0xF0F01469,
    NID_VIDEO_GLOW = (INT)0xF0F01471,   // C3
    NID_VIDEO_FULLSCREEN = (INT)0xF0F0146A,   // phase 6 (NW_FEATURES): not in the stock page
};

class NCVideoOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCVideoOptionWnd, NCWnd, 0, 0x10294CD0)

    NCComboBox* ResolutionCombo;           // +0xC8 "%d*%d %dbit", the device's display modes
    NCComboBox* RefreshCombo;              // +0xCC "%dHz", the rates of the selected mode
    NCComboBox* GammaCombo;                // +0xD0 "5" .. "1": gamma 1.2 .. 0.4
    NCComboBox* TextureCombo;              // +0xD4 High / Medium / Low (GL2TextureDetail)
    NCComboBox* ModelingCombo;             // +0xD8 High / Low (GL2UseReducePawnLOD)
    NCComboBox* AnimationCombo;            // +0xDC High / Low (GL2UseSkipAnim)
    NCCheckBox* ShadowBox;                 // +0xE0 GIsL2Projector
    NCCheckBox* GroundDetailBox;           // +0xE4 GL2RenderDeco
    NCComboBox* TerrainRangeCombo;         // +0xE8 "5" .. "1": terrain clipping 8 .. 4
    NCComboBox* PawnRangeCombo;            // +0xEC "5" .. "1": PC/NPC clipping 3 .. 1
    NCCheckBox* CursorBox;                 // +0xF0 GL2UseColorCursor
    NCComboBox* ReflectionCombo;           // +0xF4 None / All / Sky (water effect type)
    NCCheckBox* LowerDetailBox;            // +0xF8 GL2KeepMinFrameRate
    NCCheckBox* TextureFilterBox;          // +0xFC GL2UseTrilinear
    NCComboBox* ActorLimitCombo;           // +0x100 None / "5" .. "1" (GL2RenderActorLimitOpt)
    NCCheckBox* GlowBox;                   // +0x104 (new in C3) GL2GlowEffect; free only with pixel shaders 2.0
    NCComboBox* AntialiasCombo;            // +0x108 (new in C3) GL2Antialiasing; None / 2x / 4x by GetMultiSample
    NCWnd* UnusedD0[3];                    // +0x10C destroyed, never created (phase 6, NW_FEATURES:
                                           //   [0] is the "Full Screen" box, an NCCheckBox)

    NCVideoOptionWnd();                                              // 0x100bd3c0
    // ~NCVideoOptionWnd                                             // 0x100a08a0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b6830
    virtual INT OnCreate();                                          // 37 0x100bbd60
    virtual INT OnDestroy();                                         // 38 0x100b78e0
    virtual void ShowWindow();                                       // 44 0x100b6a40
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b7340
    // names ours
    virtual INT OnResolutionChanged(INT Index);                      // 68 0x100b7da0
    virtual INT OnRefreshChanged(INT Index) { return 1; }            // 69 0x10003360 (folded)
    virtual INT OnLowerDetailClicked();                              // 70 0x100b9490

    // names ours
    void InitModes();                                                // 0x100b68d0 (Apply)
    void LoadSettings();                                             // 0x100b7ef0 current values
    void Apply();                                                    // 0x100b8550
    void ClearItems();                                               // 0x100b7cd0 (C2 only; before InitItems)
    void InitItems();                                                // 0x100bb440

    // phase 6 (defined only with NW_FEATURES): the "Full Screen" box shows the mode the viewport
    // is in now (LoadSettings, and every time the options window opens)
    void SyncModeBox();
};
NW_ASSERT_OFFSET(NCVideoOptionWnd, ResolutionCombo, 0xC8);
NW_ASSERT_OFFSET(NCVideoOptionWnd, ActorLimitCombo, 0x100);
NW_ASSERT_OFFSET(NCVideoOptionWnd, GlowBox, 0x104);
NW_ASSERT_OFFSET(NCVideoOptionWnd, AntialiasCombo, 0x108);
NW_ASSERT_OFFSET(NCVideoOptionWnd, UnusedD0, 0x10C);
NW_ASSERT_SIZE(NCVideoOptionWnd, 0x118);

#pragma pack(pop)