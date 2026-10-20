#include "USettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Sound/AudioSettings.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"

// 静态成员类外定义
const FString USettingsSubsystem::SaveSlotName = TEXT("SettingsSlot");

namespace
{
    // 通过 CVar 设置浮点音量，不依赖额外 SoundClass 资产
    void SetCVarFloat(const TCHAR* Name, float Value)
    {
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            CVar->Set(Value, ECVF_SetByConsole);
        }
    }
}

void USettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 创建音量控制用的 SoundMix
    SettingsMix = NewObject<USoundMix>();
    SettingsMix->AddToRoot();

    LoadSettings();
}

void USettingsSubsystem::Deinitialize()
{
    SaveSettings();

    // Initialize 里对 SettingsMix 调用了 AddToRoot() 手动保活；
    // 子系统销毁时必须 RemoveFromRoot，否则该 UObject 永久泄漏（永远不被 GC）
    if (SettingsMix)
    {
        SettingsMix->RemoveFromRoot();
        SettingsMix = nullptr;
    }

    Super::Deinitialize();
}

// ========== 音频 ==========
void USettingsSubsystem::SetMasterVolume(float InVolume)
{
    if (!CurrentSettings) return;
    CurrentSettings->MasterVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
    ApplyAudioSettings();
    OnSettingsChanged.Broadcast();
}

void USettingsSubsystem::SetMusicVolume(float InVolume)
{
    if (!CurrentSettings) return;
    CurrentSettings->MusicVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
    ApplyAudioSettings();
    OnSettingsChanged.Broadcast();
}

void USettingsSubsystem::SetSFXVolume(float InVolume)
{
    if (!CurrentSettings) return;
    CurrentSettings->SFXVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
    ApplyAudioSettings();
    OnSettingsChanged.Broadcast();
}

float USettingsSubsystem::GetMasterVolume() const { return CurrentSettings ? CurrentSettings->MasterVolume : 1.0f; }
float USettingsSubsystem::GetMusicVolume() const  { return CurrentSettings ? CurrentSettings->MusicVolume  : 1.0f; }
float USettingsSubsystem::GetSFXVolume() const    { return CurrentSettings ? CurrentSettings->SFXVolume    : 1.0f; }

// ========== 显示 ==========
void USettingsSubsystem::SetFullscreen(bool bInFullscreen)
{
    if (!CurrentSettings) return;
    CurrentSettings->bFullscreen = bInFullscreen;
    ApplyDisplaySettings();
    OnSettingsChanged.Broadcast();
}

void USettingsSubsystem::SetResolution(int32 Width, int32 Height)
{
    if (!CurrentSettings) return;
    CurrentSettings->ScreenWidth  = FMath::Max(Width, 640);
    CurrentSettings->ScreenHeight = FMath::Max(Height, 480);
    ApplyDisplaySettings();
    OnSettingsChanged.Broadcast();
}

bool USettingsSubsystem::IsFullscreen() const
{
    return CurrentSettings ? CurrentSettings->bFullscreen : true;
}

FIntPoint USettingsSubsystem::GetCurrentResolution() const
{
    return CurrentSettings
        ? FIntPoint(CurrentSettings->ScreenWidth, CurrentSettings->ScreenHeight)
        : FIntPoint(1920, 1080);
}

TArray<FString> USettingsSubsystem::GetAllSupportedResolutions() const
{
    // UGameUserSettings 没有暴露"枚举全部支持分辨率"的 API，这里用标准档位 + 桌面分辨率组合：
    // 1. 列出常见标准分辨率
    // 2. 只保留不超过桌面分辨率的项
    // 3. 最后补上当前桌面分辨率本身
    TArray<FString> Result;
    TSet<FIntPoint> Seen;

    FIntPoint Desktop(1920, 1080);
    if (GEngine && GEngine->GameUserSettings)
    {
        Desktop = GEngine->GameUserSettings->GetDesktopResolution();
    }

    const TArray<FIntPoint> Presets = {
        FIntPoint(1280,  720),
        FIntPoint(1600,  900),
        FIntPoint(1920, 1080),
        FIntPoint(2560, 1440),
        FIntPoint(3840, 2160),
    };

    for (const FIntPoint& R : Presets)
    {
        // 只列出 <= 桌面分辨率的档位，避免给一个 1080p 屏列 4K
        if ((R.X <= Desktop.X && R.Y <= Desktop.Y) && !Seen.Contains(R))
        {
            Seen.Add(R);
            Result.Add(FString::Printf(TEXT("%dx%d"), R.X, R.Y));
        }
    }

    // 桌面分辨率一定出现（即使用户的屏幕不是标准档位）
    if (!Seen.Contains(Desktop))
    {
        Seen.Add(Desktop);
        Result.Add(FString::Printf(TEXT("%dx%d"), Desktop.X, Desktop.Y));
    }

    return Result;
}

// ========== 画质 ==========
void USettingsSubsystem::SetQualityLevel(int32 Level)
{
    if (!CurrentSettings) return;
    CurrentSettings->QualityLevel = FMath::Clamp(Level, 0, 3);
    ApplyQualitySettings();
    OnSettingsChanged.Broadcast();
}

int32 USettingsSubsystem::GetQualityLevel() const
{
    return CurrentSettings ? CurrentSettings->QualityLevel : 3;
}

// ========== 按键设置 ==========
FName USettingsSubsystem::ActionToName(EBindableAction Action)
{
    switch (Action)
    {
    case EBindableAction::ToggleInventory: return TEXT("ToggleInventory");
    case EBindableAction::ToggleMap:       return TEXT("ToggleMap");
    case EBindableAction::ToggleGacha:     return TEXT("ToggleGacha");
    case EBindableAction::ClosePanel:      return TEXT("ClosePanel");
    case EBindableAction::ToggleEquipment: return TEXT("ToggleEquipment");
    default:                               return NAME_None;
    }
}

FKey USettingsSubsystem::GetDefaultKey(EBindableAction Action) const
{
    switch (Action)
    {
    case EBindableAction::ToggleInventory: return EKeys::I;
    case EBindableAction::ToggleMap:       return EKeys::M;
    case EBindableAction::ToggleGacha:     return EKeys::G;
    case EBindableAction::ClosePanel:      return EKeys::Escape;
    case EBindableAction::ToggleEquipment: return EKeys::E;
    default:                               return EKeys::Invalid;
    }
}

FKey USettingsSubsystem::GetBoundKey(EBindableAction Action) const
{
    if (CurrentSettings)
    {
        if (const FKey* Custom = CurrentSettings->KeyBindings.Find(ActionToName(Action)))
        {
            if (Custom->IsValid())
            {
                return *Custom;
            }
        }
    }
    return GetDefaultKey(Action);
}

bool USettingsSubsystem::HasCustomKey(EBindableAction Action) const
{
    if (CurrentSettings)
    {
        if (const FKey* Custom = CurrentSettings->KeyBindings.Find(ActionToName(Action)))
        {
            return Custom->IsValid();
        }
    }
    return false;
}

void USettingsSubsystem::SetBoundKey(EBindableAction Action, FKey NewKey)
{
    if (!CurrentSettings || !NewKey.IsValid()) return;

    // 不允许两个动作绑同一个键：先清掉占用该键的其它动作
    for (TPair<FName, FKey>& Pair : CurrentSettings->KeyBindings)
    {
        if (Pair.Value == NewKey)
        {
            Pair.Value = EKeys::Invalid;
        }
    }

    CurrentSettings->KeyBindings.Add(ActionToName(Action), NewKey);

    OnKeyBindingsChanged.Broadcast();
    SaveSettings();
}

void USettingsSubsystem::ResetKey(EBindableAction Action)
{
    if (!CurrentSettings) return;
    CurrentSettings->KeyBindings.Remove(ActionToName(Action));
    OnKeyBindingsChanged.Broadcast();
    SaveSettings();
}

void USettingsSubsystem::ResetAllKeys()
{
    if (!CurrentSettings) return;
    CurrentSettings->KeyBindings.Reset();
    OnKeyBindingsChanged.Broadcast();
    SaveSettings();
}

TArray<EBindableAction> USettingsSubsystem::GetRemappableActions() const
{
    return {
        EBindableAction::ToggleInventory,
        EBindableAction::ToggleMap,
        EBindableAction::ToggleGacha,
        EBindableAction::ClosePanel,
        EBindableAction::ToggleEquipment,
    };
}

FText USettingsSubsystem::GetActionDisplayName(EBindableAction Action) const
{
    if (const UEnum* Enum = StaticEnum<EBindableAction>())
    {
        return Enum->GetDisplayNameTextByValue(static_cast<int64>(Action));
    }
    return FText::FromName(ActionToName(Action));
}

// ========== 增强输入键位（IMC_Keys） ==========

FKey USettingsSubsystem::GetEnhancedKey(FName ActionName, FKey DefaultKey) const
{
    if (CurrentSettings && !ActionName.IsNone())
    {
        if (const FKey* Custom = CurrentSettings->EnhancedKeyOverrides.Find(ActionName))
        {
            if (Custom->IsValid())
            {
                return *Custom;
            }
        }
    }
    return DefaultKey;
}

void USettingsSubsystem::SetEnhancedKey(FName ActionName, FKey NewKey)
{
    if (!CurrentSettings || ActionName.IsNone() || !NewKey.IsValid())
    {
        return;
    }

    // 同一 IMC 内不允许两个 Action 绑同一个键：先清掉占用该键的其它 Action
    for (TPair<FName, FKey>& Pair : CurrentSettings->EnhancedKeyOverrides)
    {
        if (Pair.Value == NewKey)
        {
            Pair.Value = EKeys::Invalid;
        }
    }

    CurrentSettings->EnhancedKeyOverrides.Add(ActionName, NewKey);

    // 广播后 PlayerController 重绑克隆版 IMC；UIManager 也会顺带重绑 legacy 键（幂等）
    OnKeyBindingsChanged.Broadcast();
    SaveSettings();
}

void USettingsSubsystem::ResetEnhancedKey(FName ActionName)
{
    if (!CurrentSettings) return;
    CurrentSettings->EnhancedKeyOverrides.Remove(ActionName);
    OnKeyBindingsChanged.Broadcast();
    SaveSettings();
}

void USettingsSubsystem::ResetAllEnhancedKeys()
{
    if (!CurrentSettings) return;
    if (CurrentSettings->EnhancedKeyOverrides.Num() > 0)
    {
        CurrentSettings->EnhancedKeyOverrides.Reset();
        OnKeyBindingsChanged.Broadcast();
        SaveSettings();
    }
}

void USettingsSubsystem::ApplyEnhancedKeyOverrides(UInputMappingContext* IMC) const
{
    if (!IMC || !CurrentSettings || CurrentSettings->EnhancedKeyOverrides.Num() == 0)
    {
        return;
    }

    // GetMappings() 是只读访问；需要写 Key 时用非 const 的 GetMapping(Index)
    const int32 Num = IMC->GetMappings().Num();
    for (int32 Idx = 0; Idx < Num; ++Idx)
    {
        const FEnhancedActionKeyMapping& Src = IMC->GetMappings()[Idx];
        if (!Src.Action)
        {
            continue;
        }
        if (const FKey* Custom = CurrentSettings->EnhancedKeyOverrides.Find(Src.Action->GetFName()))
        {
            if (Custom->IsValid())
            {
                IMC->GetMapping(Idx).Key = *Custom;
            }
        }
    }
}

// ========== 存档 ==========
void USettingsSubsystem::SaveSettings()
{
    if (!CurrentSettings) return;
    UGameplayStatics::SaveGameToSlot(CurrentSettings, SaveSlotName, 0);
}

void USettingsSubsystem::LoadSettings()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        CurrentSettings = Cast<USettingsSaveGame>(
            UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    }
    if (!CurrentSettings)
    {
        CurrentSettings = NewObject<USettingsSaveGame>();
    }

    ApplyAudioSettings();
    ApplyDisplaySettings();
    ApplyQualitySettings();
}

// ========== 内部应用 ==========
void USettingsSubsystem::ApplyAudioSettings()
{
    if (!CurrentSettings) return;

    UWorld* World = GetWorld();
    if (!World || !SettingsMix) return;

    // 从引擎音频设置获取默认 Master SoundClass
    static USoundClass* MasterClass = nullptr;
    if (!MasterClass)
    {
        UAudioSettings* AudioSettings = GetMutableDefault<UAudioSettings>();
        if (AudioSettings)
        {
            MasterClass = AudioSettings->GetDefaultSoundClass();
        }
    }

    if (MasterClass)
    {
        // 每次都确保 Mix 已激活（重复 Push 同一个 Mix 是安全的）
        UGameplayStatics::PushSoundMixModifier(World, SettingsMix);

        UGameplayStatics::SetSoundMixClassOverride(
            World,
            SettingsMix,
            MasterClass,
            CurrentSettings->MasterVolume,
            1.0f,
            0.0f,
            true);
    }
}

void USettingsSubsystem::ApplyDisplaySettings()
{
    if (!GEngine || !GEngine->GameUserSettings || !CurrentSettings) return;

    UGameUserSettings* const Settings = GEngine->GameUserSettings;

    // 分辨率：窗口模式下即窗口客户区大小；窗口全屏模式下会自动使用桌面分辨率
    Settings->SetScreenResolution(
        FIntPoint(CurrentSettings->ScreenWidth, CurrentSettings->ScreenHeight));

    if (CurrentSettings->bFullscreen)
    {
        // 无边框窗口全屏：Alt+Tab 无黑屏、多显示器兼容好、切换稳定
        Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
    }
    else
    {
        Settings->SetFullscreenMode(EWindowMode::Windowed);
    }

    // 只应用分辨率与窗口模式，避免连带触发画质等其它设置
    Settings->ApplyResolutionSettings(false);
}

void USettingsSubsystem::ApplyQualitySettings()
{
    if (!GEngine || !GEngine->GameUserSettings || !CurrentSettings) return;

    GEngine->GameUserSettings->SetOverallScalabilityLevel(CurrentSettings->QualityLevel);
    GEngine->GameUserSettings->ApplySettings(false);
}
