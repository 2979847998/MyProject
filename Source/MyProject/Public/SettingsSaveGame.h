#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

/**
 * 设置存档对象：所有需要持久化的设置项都放这里。
 * 通过 UGameplayStatics::SaveGameToSlot / LoadGameFromSlot 读写。
 */
UCLASS()
class MYPROJECT_API USettingsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    // ========== 音频 ==========
    UPROPERTY()
    float MasterVolume = 1.0f;   // 0~1 线性值
    UPROPERTY()
    float MusicVolume = 1.0f;
    UPROPERTY()
    float SFXVolume = 1.0f;

    // ========== 显示 ==========
    UPROPERTY()
    bool bFullscreen = true;
    UPROPERTY()
    int32 ScreenWidth = 1920;
    UPROPERTY()
    int32 ScreenHeight = 1080;

    // ========== 画质 ==========
    UPROPERTY()
    int32 QualityLevel = 3;   // 0:低 1:中 2:高 3:史诗

    // ========== 鼠标 ==========
    UPROPERTY()
    float MouseSensitivity = 1.0f;
    UPROPERTY()
    bool bInvertMouse = false;

    // ========== 按键 ==========
    // 动作名 -> 玩家自定义按键；未出现的动作使用代码内默认键（I/M/G/Esc）
    UPROPERTY()
    TMap<FName, FKey> KeyBindings;

    // ========== 增强输入键位（IMC_Keys） ==========
    // Action 名 -> 自定义键；未出现的动作使用 IMC_Keys 资产里的默认键
    UPROPERTY()
    TMap<FName, FKey> EnhancedKeyOverrides;
};
