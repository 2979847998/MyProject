// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * 玩家控制器基类：在原有基础上增加“点击移动到鼠标位置”。
 * - 左键点击地面（无 UI 面板打开时）→ 角色自动走向点击点
 * - 到达后自动停止
 * 蓝图 PlayerController 继承本类即可获得该功能；也可以在细节面板调 StopDistance。
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStateChanged, bool, bNewAttackState);

UCLASS()
class MYPROJECT_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMyPlayerController();

    UPROPERTY(BlueprintReadWrite, Category = "Attack|State")
    bool bIsAttacking = false;
    UPROPERTY(BlueprintAssignable, Category = "Attack|State")
    FOnAttackStateChanged OnAttackStateChanged;
    UFUNCTION(BlueprintCallable, Category = "Attack|State")
    void SetAttackState(bool bNewState);
 
    // ===== 增强输入键位（蓝图可配置，默认不绑定任何 IMC）=====
    // 在蓝图/细节面板指定要在设置里重绑键位的 IMC；默认为空（空 = 不接入增强键位）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|EnhancedKeys", meta = (DisplayName = "键位映射上下文"))
    TObjectPtr<class UInputMappingContext> EnhancedKeysContext;

    // 可在设置页重绑键位的增强输入 Action 清单（蓝图可配置）。
    // 默认为空 = 自动收集 IMC_Keys 里已映射的 Action，并补齐工程默认四个 IA
    // （IA_Attack / IA_Chilk / IA_Horizontal / IA_Vertical）——即使资产里没有键位映射，
    // 设置页也会列出它们供绑定，绑定的键只写存档，不改动磁盘资产
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|EnhancedKeys")
    TArray<TObjectPtr<UInputAction>> EnhancedKeyActions;

    // 取蓝图配置的键位映射上下文（默认为空，不绑定任何 IMC）
    UFUNCTION(BlueprintPure, Category = "Input|EnhancedKeys")
    UInputMappingContext* GetEnhancedKeysContext() const;

    // 取设置页可重绑的增强 Action 清单（蓝图配置优先，空则自动收集 IMC 映射 + 默认四 IA）
    UFUNCTION(BlueprintPure, Category = "Input|EnhancedKeys")
    TArray<UInputAction*> GetRebindableEnhancedActions() const;

    // 取运行时克隆版（改键后的实际生效 IMC）
    UFUNCTION(BlueprintPure, Category = "Input|EnhancedKeys")
    UInputMappingContext* GetRuntimeKeysIMC() const;

protected:
    virtual void SetupInputComponent() override;

    // 每帧驱动角色朝目标点移动
    virtual void PlayerTick(float DeltaTime) override;

    // 点击移动：左键点击地面（无 UI 面板打开时）→ 角色自动走向点击点
    void OnClickMovePressed();

    // 判断是否有会拦截点击的 UI 面板开着（背包/设置/地图/确认框/抽卡）
    bool IsAnyUIOpen() const;

    // ===== 增强输入键位（内部实现）=====
    // 运行时持有的 IMC 克隆（改键只改克隆版，磁盘资产保持干净；UPROPERTY 防 GC）
    UPROPERTY(Transient)
    TObjectPtr<class UInputMappingContext> RuntimeIMCKeys;

    // 首次接入：加载 IMC 并克隆、应用存档自定义键、挂到增强输入子系统
    void SetupEnhancedKeys();

    // 设置改键后重绑克隆版 IMC 并请求重建映射
    void RebuildEnhancedKeys();

    // 按可重绑清单重建克隆 IMC：设置 override 优先，其次资产默认键，两者都无则跳过（未绑定）
    void RebuildRuntimeIMCFromSources();

    // 设置子系统广播（改键）时的回调
    UFUNCTION()
    void HandleKeyBindingsChanged();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 动态创建的增强输入资源（持有防 GC）
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ClickMoveAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ClickMoveContext;

    // 当前移动目标点
    UPROPERTY(Transient)
    FVector MoveTarget = FVector::ZeroVector;

    // 是否正在朝目标点移动
    UPROPERTY(Transient)
    bool bMoveToTarget = false;

    // 距目标点多远算到达（单位 cm）
    UPROPERTY(EditDefaultsOnly, Category = "ClickToMove")
    float StopDistance = 40.0f;


    
};
