#include "USettingsDisplayPage.h"
#include "USettingsSubsystem.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"

void USettingsDisplayPage::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        SettingsMgr = GI->GetSubsystem<USettingsSubsystem>();
    }
    if (!SettingsMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsDisplayPage] 获取 USettingsSubsystem 失败"));
        return;
    }

    if (CheckBox_Fullscreen) CheckBox_Fullscreen->OnCheckStateChanged.AddDynamic(this, &USettingsDisplayPage::OnFullscreenChanged);
    if (ComboBox_Resolution) ComboBox_Resolution->OnSelectionChanged.AddDynamic(this, &USettingsDisplayPage::OnResolutionChanged);
    if (ComboBox_Quality) ComboBox_Quality->OnSelectionChanged.AddDynamic(this, &USettingsDisplayPage::OnQualityChanged);

    // 填充分辨率列表
    if (ComboBox_Resolution)
    {
        const TArray<FString> ResList = SettingsMgr->GetAllSupportedResolutions();
        for (const FString& Res : ResList)
        {
            ComboBox_Resolution->AddOption(Res);
        }
    }

    // 填充画质档位
    if (ComboBox_Quality)
    {
        ComboBox_Quality->AddOption(TEXT("低"));
        ComboBox_Quality->AddOption(TEXT("中"));
        ComboBox_Quality->AddOption(TEXT("高"));
        ComboBox_Quality->AddOption(TEXT("史诗"));
    }

    InitUIFromSettings();
}

void USettingsDisplayPage::NativeDestruct()
{
    if (CheckBox_Fullscreen) CheckBox_Fullscreen->OnCheckStateChanged.RemoveDynamic(this, &USettingsDisplayPage::OnFullscreenChanged);
    if (ComboBox_Resolution) ComboBox_Resolution->OnSelectionChanged.RemoveDynamic(this, &USettingsDisplayPage::OnResolutionChanged);
    if (ComboBox_Quality) ComboBox_Quality->OnSelectionChanged.RemoveDynamic(this, &USettingsDisplayPage::OnQualityChanged);

    Super::NativeDestruct();
}

void USettingsDisplayPage::InitUIFromSettings()
{
    if (!SettingsMgr) return;

    if (CheckBox_Fullscreen) CheckBox_Fullscreen->SetIsChecked(SettingsMgr->IsFullscreen());
    if (ComboBox_Resolution)
    {
        const FIntPoint CurrentRes = SettingsMgr->GetCurrentResolution();
        const FString ResString = FString::Printf(TEXT("%dx%d"), CurrentRes.X, CurrentRes.Y);
        ComboBox_Resolution->SetSelectedOption(ResString);
    }
    if (ComboBox_Quality)
    {
        const int32 QualityIndex = SettingsMgr->GetQualityLevel();
        if (QualityIndex >= 0 && QualityIndex < ComboBox_Quality->GetOptionCount())
        {
            ComboBox_Quality->SetSelectedIndex(QualityIndex);
        }
    }
}

void USettingsDisplayPage::OnFullscreenChanged(bool bIsChecked)
{
    if (SettingsMgr) SettingsMgr->SetFullscreen(bIsChecked);
}

void USettingsDisplayPage::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!SettingsMgr || SelectedItem.IsEmpty()) return;

    TArray<FString> Parts;
    SelectedItem.ParseIntoArray(Parts, TEXT("x"));
    if (Parts.Num() != 2) return;

    const int32 Width  = FCString::Atoi(*Parts[0]);
    const int32 Height = FCString::Atoi(*Parts[1]);
    SettingsMgr->SetResolution(Width, Height);
}

void USettingsDisplayPage::OnQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!SettingsMgr || !ComboBox_Quality) return;
    SettingsMgr->SetQualityLevel(ComboBox_Quality->GetSelectedIndex());
}
