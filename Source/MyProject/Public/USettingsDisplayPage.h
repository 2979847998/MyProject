#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USettingsDisplayPage.generated.h"

class UCheckBox;
class UComboBoxString;
class USettingsSubsystem;

/**
 * 设置 - 显示页（独立 UI 场景）。
 * 显示与画质为同一分类：全屏 / 分辨率 / 画质档位。
 */
UCLASS()
class MYPROJECT_API USettingsDisplayPage : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> CheckBox_Fullscreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ComboBox_Resolution;

    // 画质档位（低/中/高/史诗，C++ 自动填充；与显示同属一页）
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ComboBox_Quality;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<USettingsSubsystem> SettingsMgr;

    UFUNCTION()
    void OnFullscreenChanged(bool bIsChecked);

    UFUNCTION()
    void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    void InitUIFromSettings();
};
