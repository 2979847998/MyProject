#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USettingsAudioPage.generated.h"

class USlider;
class UTextBlock;
class USettingsSubsystem;

/**
 * 设置 - 音频页（独立 UI 场景）。
 * BindWidget 名字必须和对应 WBP 蓝图控件名一致。
 */
UCLASS()
class MYPROJECT_API USettingsAudioPage : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> Slider_Master;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Master;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> Slider_Music;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Music;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> Slider_SFX;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_SFX;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<USettingsSubsystem> SettingsMgr;

    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnMusicVolumeChanged(float Value);

    UFUNCTION()
    void OnSFXVolumeChanged(float Value);

    // 用当前设置值刷新本页 UI
    void InitUIFromSettings();

    void UpdateVolumeText(UTextBlock* TextBlock, float Value);
};
