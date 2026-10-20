#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SettingsSaveGame.h"
#include "USettingsSubsystem.generated.h"

// 可在设置里重新绑定的动作（与 UIManagerComponent 的按键处理一一对应）
UENUM(BlueprintType)
enum class EBindableAction : uint8
{
    ToggleInventory UMETA(DisplayName = "打开/关闭背包"),
    ToggleMap       UMETA(DisplayName = "打开/关闭地图"),
    ToggleGacha     UMETA(DisplayName = "打开/关闭抽卡"),
    ClosePanel      UMETA(DisplayName = "关闭当前面板"),
    // 追加在末尾，避免打乱蓝图/存档已序列化的枚举值
    ToggleEquipment UMETA(DisplayName = "打开/关闭装备"),
};

// 设置变更多播委托，任何设置被修改时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsChanged);

// 按键绑定被修改时广播（UI 管理器据此重新绑定输入）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyBindingsChanged);

/**
 * 全局设置子系统：负责音量/显示/画质/按键鼠标等设置的读写、应用与持久化。
 * 通过 GameInstance 获取，蓝图可直接调用。
 */
UCLASS()
class MYPROJECT_API USettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 设置变更事件
    UPROPERTY(BlueprintAssignable, Category = "Settings")
    FOnSettingsChanged OnSettingsChanged;

    // ========== 音频设置 ==========
    UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
    void SetMasterVolume(float InVolume);

    UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
    void SetMusicVolume(float InVolume);

    UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
    void SetSFXVolume(float InVolume);

    UFUNCTION(BlueprintPure, Category = "Settings|Audio")
    float GetMasterVolume() const;

    UFUNCTION(BlueprintPure, Category = "Settings|Audio")
    float GetMusicVolume() const;

    UFUNCTION(BlueprintPure, Category = "Settings|Audio")
    float GetSFXVolume() const;

    // ========== 显示设置 ==========
    UFUNCTION(BlueprintCallable, Category = "Settings|Display")
    void SetFullscreen(bool bInFullscreen);

    UFUNCTION(BlueprintCallable, Category = "Settings|Display")
    void SetResolution(int32 Width, int32 Height);

    UFUNCTION(BlueprintPure, Category = "Settings|Display")
    bool IsFullscreen() const;

    UFUNCTION(BlueprintPure, Category = "Settings|Display")
    FIntPoint GetCurrentResolution() const;

    UFUNCTION(BlueprintCallable, Category = "Settings|Display")
    TArray<FString> GetAllSupportedResolutions() const;

    // ========== 画质设置 ==========
    UFUNCTION(BlueprintCallable, Category = "Settings|Quality")
    void SetQualityLevel(int32 Level);

    UFUNCTION(BlueprintPure, Category = "Settings|Quality")
    int32 GetQualityLevel() const;

    // ========== 按键设置 ==========
    UPROPERTY(BlueprintAssignable, Category = "Settings|Keys")
    FOnKeyBindingsChanged OnKeyBindingsChanged;

    // 取某动作当前生效的键（玩家改过用自定义，没改过用默认）
    UFUNCTION(BlueprintPure, Category = "Settings|Keys")
    FKey GetBoundKey(EBindableAction Action) const;

    // 取某动作的默认键
    UFUNCTION(BlueprintPure, Category = "Settings|Keys")
    FKey GetDefaultKey(EBindableAction Action) const;

    // 是否玩家自定义过该动作的键（区分“自定义键”与“默认键回落”）
    UFUNCTION(BlueprintPure, Category = "Settings|Keys")
    bool HasCustomKey(EBindableAction Action) const;

    // 修改某动作的按键并立即生效、存盘
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys")
    void SetBoundKey(EBindableAction Action, FKey NewKey);

    // 单个动作恢复默认
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys")
    void ResetKey(EBindableAction Action);

    // 全部动作恢复默认
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys")
    void ResetAllKeys();

    // 全部可重绑动作（设置 UI 据此逐行生成）
    UFUNCTION(BlueprintPure, Category = "Settings|Keys")
    TArray<EBindableAction> GetRemappableActions() const;

    // 动作的中文显示名（取自枚举 DisplayName）
    UFUNCTION(BlueprintPure, Category = "Settings|Keys")
    FText GetActionDisplayName(EBindableAction Action) const;

    // ========== 增强输入键位（IMC_Keys 的 Action） ==========
    // 取某 Action 当前生效的键（玩家自定义优先，否则 IMC 里的默认键）
    UFUNCTION(BlueprintPure, Category = "Settings|Keys|Enhanced")
    FKey GetEnhancedKey(FName ActionName, FKey DefaultKey) const;

    // 修改某 Action 的按键：存盘 + 广播（PlayerController 据此重绑 IMC）
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys|Enhanced")
    void SetEnhancedKey(FName ActionName, FKey NewKey);

    // 恢复某 Action 为 IMC 资产里的默认键
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys|Enhanced")
    void ResetEnhancedKey(FName ActionName);

    // 全部增强输入 Action 恢复默认
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys|Enhanced")
    void ResetAllEnhancedKeys();

    // 把当前自定义键覆盖到指定 IMC 的 Mappings（改后需 RequestRebuildControlMappings 生效）
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys|Enhanced")
    void ApplyEnhancedKeyOverrides(class UInputMappingContext* IMC) const;

    // ========== 存档读写 ==========
    UFUNCTION(BlueprintCallable, Category = "Settings|Save")
    void SaveSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings|Save")
    void LoadSettings();

private:
    // 当前运行时设置对象（USaveGame 必须以指针持有）
    UPROPERTY()
    TObjectPtr<USettingsSaveGame> CurrentSettings;

    // 音量控制用的 SoundMix
    UPROPERTY()
    TObjectPtr<class USoundMix> SettingsMix;

    // 存档槽位名（类内声明，类外定义）
    static const FString SaveSlotName;

    void ApplyAudioSettings();
    void ApplyDisplaySettings();
    void ApplyQualitySettings();

    // 动作枚举 -> 存档 Map 使用的稳定键名
    static FName ActionToName(EBindableAction Action);
};
