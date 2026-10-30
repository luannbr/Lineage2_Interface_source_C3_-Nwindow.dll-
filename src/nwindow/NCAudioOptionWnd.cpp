/*=============================================================================
    NCAudioOptionWnd.cpp - the audio page of the options. Stock: 0x1007da00 .. 0x1007dd6a,
    0x10080210 .. 0x1008096f (WindowProc is NControl's body).
=============================================================================*/
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100bd470
NCAudioOptionWnd::NCAudioOptionWnd()
{
    Unused9C[0] = NULL;
    Unused9C[1] = NULL;
    Unused9C[2] = NULL;
}
NW_HOOK_CTOR(0x100bd470, NCAudioOptionWnd)

// 0x100bd4d0
NW_HOOK_DTOR(0x100bd4d0, NCAudioOptionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCAudioOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100bb800 (vslot 37) - C3: four volume boxes (sound, music, the two voice ones), 125 x 17 at x 107, 13 px apart
INT NCAudioOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    SoundCombo = new NCComboBox;
    SoundCombo->ID = NID_AUDIO_SOUND;
    SoundCombo->SetPosition(107, 23, 125, 17);
    CreateChild(SoundCombo, NWS_VISIBLE);
    MusicCombo = new NCComboBox;
    MusicCombo->ID = NID_AUDIO_MUSIC;
    MusicCombo->SetPosition(107, 42, 125, 17);
    CreateChild(MusicCombo, NWS_VISIBLE);
    WavVoiceCombo = new NCComboBox;
    WavVoiceCombo->ID = NID_AUDIO_WAVVOICE;
    WavVoiceCombo->SetPosition(107, 61, 125, 17);
    CreateChild(WavVoiceCombo, NWS_VISIBLE);
    OggVoiceCombo = new NCComboBox;
    OggVoiceCombo->ID = NID_AUDIO_OGGVOICE;
    OggVoiceCombo->SetPosition(107, 80, 125, 17);
    CreateChild(OggVoiceCombo, NWS_VISIBLE);
    InitVolumes();
    return 1;
}
NW_HOOK_METHOD(0x100bb800, NCAudioOptionWnd, OnCreate)

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

// 0x100b98a0 (vslot 38) - guarded; the four boxes and the three that were never made, then NCWnd's
INT NCAudioOptionWnd::OnDestroy()
{
    guard(NCAudioOptionWnd::OnDestroy);
    DestroyChild(this, SoundCombo);
    DestroyChild(this, MusicCombo);
    DestroyChild(this, WavVoiceCombo);
    DestroyChild(this, OggVoiceCombo);
    DestroyChild(this, Unused9C[0]);
    DestroyChild(this, Unused9C[1]);
    DestroyChild(this, Unused9C[2]);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b98a0, NCAudioOptionWnd, OnDestroy)

// 0x100b9670 (vslot 56) - C3: the four titles (sysstrings 216, 217, 977, 978: GL2GameData +0x664ac / +0x664b8 / +0x68858
// / +0x68864) right-aligned on x 100, at y 26, 45, 64, 83, in 0xFFDCDCDC
enum { SYSSTR_WavVoice = 977, SYSSTR_OggVoice = 978 };   // names ours
INT NCAudioOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *SysString(216), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(100 - Size.cx, 26, 0xFFDCDCDC, *SysString(216), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(217), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(100 - Size.cx, 45, 0xFFDCDCDC, *SysString(217), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(SYSSTR_WavVoice), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(100 - Size.cx, 64, 0xFFDCDCDC, *SysString(SYSSTR_WavVoice), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(SYSSTR_OggVoice), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(100 - Size.cx, 83, 0xFFDCDCDC, *SysString(SYSSTR_OggVoice), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b9670, NCAudioOptionWnd, OnPaint)

// 0x100b6a70 - C3: the four boxes
void NCAudioOptionWnd::ClearItems()
{
    SoundCombo->ClearItems();
    MusicCombo->ClearItems();
    WavVoiceCombo->ClearItems();
    OggVoiceCombo->ClearItems();
}
NW_HOOK_METHOD(0x100b6a70, NCAudioOptionWnd, ClearItems)

// 0x100b6fe0 - kept out of line: OnCreate calls it, as in the stock code. C3: four boxes
__declspec(noinline) void NCAudioOptionWnd::InitVolumes()
{
    SoundCombo->AddItem(TEXT("5"));
    SoundCombo->AddItem(TEXT("4"));
    SoundCombo->AddItem(TEXT("3"));
    SoundCombo->AddItem(TEXT("2"));
    SoundCombo->AddItem(TEXT("1"));
    SoundCombo->AddItem(0xE4);   // "Off"
    MusicCombo->AddItem(TEXT("5"));
    MusicCombo->AddItem(TEXT("4"));
    MusicCombo->AddItem(TEXT("3"));
    MusicCombo->AddItem(TEXT("2"));
    MusicCombo->AddItem(TEXT("1"));
    MusicCombo->AddItem(0xE4);
    WavVoiceCombo->AddItem(TEXT("5"));
    WavVoiceCombo->AddItem(TEXT("4"));
    WavVoiceCombo->AddItem(TEXT("3"));
    WavVoiceCombo->AddItem(TEXT("2"));
    WavVoiceCombo->AddItem(TEXT("1"));
    WavVoiceCombo->AddItem(0xE4);
    OggVoiceCombo->AddItem(TEXT("5"));
    OggVoiceCombo->AddItem(TEXT("4"));
    OggVoiceCombo->AddItem(TEXT("3"));
    OggVoiceCombo->AddItem(TEXT("2"));
    OggVoiceCombo->AddItem(TEXT("1"));
    OggVoiceCombo->AddItem(0xE4);
    SelectVolumes();
}
NW_HOOK_METHOD(0x100b6fe0, NCAudioOptionWnd, InitVolumes)

// the level of a volume: [1, ...) 0, [0.8, 1) 1 ... [0, 0.2) 5 (repeated inline in the
// stock code); a negative volume selects nothing
static FORCEINLINE void SelectLevel(NCComboBox* Combo, FLOAT Volume)
{
    if (0.0f <= Volume && Volume < 0.2f)
        Combo->SetSelected(5);
    else if (0.2f <= Volume && Volume < 0.4f)
        Combo->SetSelected(4);
    else if (0.4f <= Volume && Volume < 0.6f)
        Combo->SetSelected(3);
    else if (0.6f <= Volume && Volume < 0.8f)
        Combo->SetSelected(2);
    else if (0.8f <= Volume && Volume < 1.0f)
        Combo->SetSelected(1);
    else if (1.0f <= Volume)
        Combo->SetSelected(0);
}

// 0x100b6aa0 - kept out of line: InitVolumes tail-calls it, as in the stock code. C3: the voices too (the last
// combo is selected without the else branches of the others: a negative volume selects nothing, as above)
__declspec(noinline) void NCAudioOptionWnd::SelectVolumes()
{
    if (GConsole->Engine->Audio)
        SelectLevel(SoundCombo, GConsole->Engine->Audio->GetSoundVolume());
    else
        SoundCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(MusicCombo, GConsole->Engine->Audio->GetMusicVolume());
    else
        MusicCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(WavVoiceCombo, GConsole->Engine->Audio->GetWavVoiceVolume());
    else
        WavVoiceCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(OggVoiceCombo, GConsole->Engine->Audio->GetOggVoiceVolume());
    else
        OggVoiceCombo->EnableWindow(0);
}
NW_HOOK_METHOD(0x100b6aa0, NCAudioOptionWnd, SelectVolumes)

// 0x100b9a50 - Off also silences what plays (StopSound, StopAllMusic, StopAllVoice). Music back on from Off restarts
// it: the player controller's ResetMusicControll (reached through its pawn), then the lobby music is restored.
// C3: the two voice volumes (Option.ini WavVoiceVolume, OggVoiceVolume)
void NCAudioOptionWnd::Apply()
{
    TCHAR Buffer[256];
    if (GConsole->Engine->Audio)
    {
        switch (SoundCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetSoundVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetSoundVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetSoundVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetSoundVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetSoundVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetSoundVolume(0.0f);
            GConsole->Engine->Audio->StopSound((AActor*)NULL, (USound*)NULL);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetSoundVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("SoundVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        INT Level = MusicCombo->Selected;
        FLOAT OldVolume = GConsole->Engine->Audio->GetMusicVolume();
        switch (Level)
        {
        case 0:
            GConsole->Engine->Audio->SetMusicVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetMusicVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetMusicVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetMusicVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetMusicVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetMusicVolume(0.0f);
            GConsole->Engine->Audio->StopAllMusic(0.0f, 0);
            break;
        }
        if (GConsole->Engine->Audio->GetMusicVolume() > 0.0f && OldVolume == 0.0f)
        {
            APlayerController* Controller = GConsole->Engine->Client->Viewports(0)->Actor;
            if (Controller)
            {
                APawn* Pawn = Controller->Pawn;
                if (Pawn)
                {
                    AController* PawnController = Pawn->Controller;
                    if (PawnController)
                        PawnController->ResetMusicControll();
                }
            }
            GConsole->RestoreLobbyMusic();
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetMusicVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("MusicVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        switch (WavVoiceCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetWavVoiceVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.0f);
            GConsole->Engine->Audio->StopAllVoice(0.0f, 0);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetWavVoiceVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("WavVoiceVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        switch (OggVoiceCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetOggVoiceVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.0f);
            GConsole->Engine->Audio->StopAllVoice(0.0f, 1);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetOggVoiceVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("OggVoiceVolume"), Buffer, TEXT("Option.ini"));
    }
}
NW_HOOK_METHOD(0x100b9a50, NCAudioOptionWnd, Apply)

// 0x100b7170 (vslot 44) - the options window (the parent) takes the backdrop of this page
void NCAudioOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    ((NCOptionWnd*)Entry->Parent->Wnd)->OptionBackTex = FindTexture(TEXT("L2ui_ch3.option2_back"));
}
NW_HOOK_METHOD(0x100b7170, NCAudioOptionWnd, ShowWindow)

NW_CHECK_VTABLE(NCAudioOptionWnd, 0x10193138, 68)
