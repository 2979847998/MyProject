#include "USettingsAudioPage.h"
#include "USettingsSubsystem.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void USettingsAudioPage::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        SettingsMgr = GI->GetSubsystem<USettingsSubsystem>();
    }
    if (!SettingsMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsAudioPage] 获取 USettingsSubsystem 失败"));
        return;
    }

    if (Slider_Master) Slider_Master->OnValueChanged.AddDynamic(this, &USettingsAudioPage::OnMasterVolumeChanged);
    if (Slider_Music)  Slider_Music->OnValueChanged.AddDynamic(this, &USettingsAudioPage::OnMusicVolumeChanged);
    if (Slider_SFX)    Slider_SFX->OnValueChanged.AddDynamic(this, &USettingsAudioPage::OnSFXVolumeChanged);

    InitUIFromSettings();
}

void USettingsAudioPage::NativeDestruct()
{
    if (Slider_Master) Slider_Master->OnValueChanged.RemoveDynamic(this, &USettingsAudioPage::OnMasterVolumeChanged);
    if (Slider_Music)  Slider_Music->OnValueChanged.RemoveDynamic(this, &USettingsAudioPage::OnMusicVolumeChanged);
    if (Slider_SFX)    Slider_SFX->OnValueChanged.RemoveDynamic(this, &USettingsAudioPage::OnSFXVolumeChanged);

    Super::NativeDestruct();
}

void USettingsAudioPage::InitUIFromSettings()
{
    if (!SettingsMgr) return;

    if (Slider_Master) Slider_Master->SetValue(SettingsMgr->GetMasterVolume());
    if (Slider_Music)  Slider_Music->SetValue(SettingsMgr->GetMusicVolume());
    if (Slider_SFX)    Slider_SFX->SetValue(SettingsMgr->GetSFXVolume());

    UpdateVolumeText(Text_Master, SettingsMgr->GetMasterVolume());
    UpdateVolumeText(Text_Music,  SettingsMgr->GetMusicVolume());
    UpdateVolumeText(Text_SFX,    SettingsMgr->GetSFXVolume());
}

void USettingsAudioPage::OnMasterVolumeChanged(float Value)
{
    if (SettingsMgr) SettingsMgr->SetMasterVolume(Value);
    UpdateVolumeText(Text_Master, Value);
}

void USettingsAudioPage::OnMusicVolumeChanged(float Value)
{
    if (SettingsMgr) SettingsMgr->SetMusicVolume(Value);
    UpdateVolumeText(Text_Music, Value);
}

void USettingsAudioPage::OnSFXVolumeChanged(float Value)
{
    if (SettingsMgr) SettingsMgr->SetSFXVolume(Value);
    UpdateVolumeText(Text_SFX, Value);
}

void USettingsAudioPage::UpdateVolumeText(UTextBlock* TextBlock, float Value)
{
    if (!TextBlock) return;
    const int32 Percent = FMath::RoundToInt(Value * 100.0f);
    TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s %d%%"), *TextBlock->GetName(), Percent)));
}
