#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffCatalog.h"
#include "BuffsComponent.generated.h"

/** 激活中的一条 Buff（轻量值类型，由组件持有） */
USTRUCT(BlueprintType)
struct FActiveBuff
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    FName BuffID;

    // 剩余秒数；< 0 表示永久
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    float TimeRemaining = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 Stack = 1;

    bool operator==(const FActiveBuff& Other) const { return BuffID == Other.BuffID; }
};

// Buff 列表变化（添加/移除/刷新/到期）时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuffsChanged);

/**
 * Buff 组件：管理激活中的 Buff 列表、计时与数值汇总。
 * 挂在 PlayerControllerMy 上（和背包/UI 组件同宿主，方便互查）。
 *
 * 数值框架：每种属性是一类 EBuffStat；GetStatModifier(Stat) 返回当前所有
 * 同类 Buff 汇总后的乘数（Multiplicative 相乘 / Additive 相加），供各系统
 * 读取生效（示例：金币倍率挂接在背包 AddCoins，移动速度挂接在 DefaultPawn.MaxSpeed）。
 *
 * 添加方式：UBuffsComponent::AddBuff(BuffID)（蓝图可调）；物品"使用"由
 * 背包组件转发到这里。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UBuffsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBuffsComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ===== 数据 =====

    // 激活中的 Buff 列表（只读）
    UFUNCTION(BlueprintPure, Category = "Buff")
    const TArray<FActiveBuff>& GetActiveBuffs() const { return ActiveBuffs; }

    UFUNCTION(BlueprintPure, Category = "Buff")
    bool HasBuff(FName BuffID) const;

    // 某属性的当前汇总乘数/加成（见 EBuffStat 注释）。无 Buff 返回 1.0（加成倍率）或 0（加值）
    UFUNCTION(BlueprintPure, Category = "Buff")
    float GetStatModifier(EBuffStat Stat) const;

    UPROPERTY(BlueprintAssignable, Category = "Buff")
    FOnBuffsChanged OnBuffsChanged;

    // ===== 操作 =====

    // 添加 Buff（按目录定义处理时长刷新/叠加/覆盖），成功返回 true
    UFUNCTION(BlueprintCallable, Category = "Buff")
    bool AddBuff(FName BuffID, int32 Stack = 1);

    // 立即移除全部同名 Buff
    UFUNCTION(BlueprintCallable, Category = "Buff")
    void RemoveBuff(FName BuffID);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ClearBuffs();

protected:
    virtual void BeginPlay() override;

private:
    // 应用所有 Buff 到真实属性（当前示例：DefaultPawn.MaxSpeed 移动速度）
    void ApplyStatEffects();
    // 从宿主 Pawn 读基础速度并写入加成后的值
    void ApplyMoveSpeed();
    // 是否存在需要按秒计时的 Buff（永久 Buff 不需要 Tick）
    bool HasTimedBuffs() const;

    // 目录（未配置时兜底 /Game/UserInterface/DA_Buffs）
    UPROPERTY(EditDefaultsOnly, Category = "Buff")
    TObjectPtr<UBuffCatalog> BuffCatalog;

    // 激活的 Buff 栈
    UPROPERTY(Transient)
    TArray<FActiveBuff> ActiveBuffs;

    // 记录 Pawn 基础速度，恢复用
    float BaseMaxSpeed = 0.0f;

    // 距上次广播的累计时间（图标栏整数秒刷新用）
    float TimeSinceLastBroadcast = 0.0f;
};
