#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillCatalog.h"
#include "SkillsComponent.generated.h"

/** 冷却中的一条技能（轻量值类型，由组件持有） */
USTRUCT(BlueprintType)
struct FSkillState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FName SkillID;

    // 剩余冷却秒数；<= 0 表示可用（组件 Tick 递减后移除）
    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    float CooldownRemaining = 0.0f;

    bool operator==(const FSkillState& Other) const { return SkillID == Other.SkillID; }
};

// 技能列表变化（CD 开始/结束、能量变化）时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillsChanged);

// 技能释放成功时广播（蓝图可接播动画/音效/弹文字）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCasted, FName, SkillID);

/**
 * 技能组件：管理能量、冷却与技能释放。
 * 挂在 PlayerControllerMy 上（和背包/Buff 组件同宿主）。
 *
 * 通用效果框架：
 *  - 技能数据在 DA_Skills 定义（加行即新技能，槽位 = 数组下标）；
 *  - CastSkill(SkillID) / CastSkillBySlot(SlotIndex) 检查 冷却 + 能量；
 *  - EffectType = ApplyBuff → 对宿主 BuffsComponent 施加 Buff；
 *  - EffectType = Instant → 触发蓝图事件 ApplyInstantEffect（回血/伤害等由蓝图实现）；
 *  - EffectType = None → 只消耗 + 冷却，效果由 OnSkillCasted 蓝图接。
 *
 * 能量：每秒回复 EnergyRegenPerSecond，最大 MaxEnergy。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API USkillsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillsComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ===== 数据 =====

    // 当前能量（0~MaxEnergy）
    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetEnergy() const { return Energy; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetMaxEnergy() const { return MaxEnergy; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetEnergyPercent() const { return MaxEnergy > 0.0f ? Energy / MaxEnergy : 0.0f; }

    // 某技能的剩余冷却秒数（0 = 可用）
    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetCooldownRemaining(FName SkillID) const;

    // 冷却中的技能列表（只读）
    UFUNCTION(BlueprintPure, Category = "Skill")
    const TArray<FSkillState>& GetActiveCooldowns() const { return ActiveCooldowns; }

    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillsChanged OnSkillsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillCasted OnSkillCasted;

    // ===== 释放 =====

    // 按槽位释放（数字键 1/2/3... 对应槽位 0/1/2...，图标点击同样走这里）
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool CastSkillBySlot(int32 SlotIndex);

    // 按 SkillID 释放（任意蓝图/敌人逻辑可用）
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool CastSkill(FName SkillID);

    // 直接增加能量（捡能量球/喝药等），上限 MaxEnergy
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void AddEnergy(float Amount);

protected:
    virtual void BeginPlay() override;

    // Instant 类型效果的蓝图实现钩子：覆写实现回血/伤害/位移等瞬时效果
    UFUNCTION(BlueprintNativeEvent, Category = "Skill")
    void ApplyInstantEffect(FName SkillID, float InstantValue);

private:
    // 执行技能效果（按 EffectType 分发），失败返回 false
    bool ExecuteEffect(const FSkillData& Data);

    // 从宿主 PlayerController 拿 BuffsComponent（ApplyBuff 用）
    class UBuffsComponent* GetHostBuffs() const;

    // ===== 配置 =====

    // 目录（未配置时兜底 /Game/UserInterface/DA_Skills）
    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    TObjectPtr<USkillCatalog> SkillCatalog;

    // 能量上限
    UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "1"))
    float MaxEnergy = 100.0f;

    // 每秒自然回复能量
    UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "0"))
    float EnergyRegenPerSecond = 5.0f;

    // ===== 运行时状态 =====

    // 当前能量
    UPROPERTY(Transient)
    float Energy = 100.0f;

    // 冷却中的技能
    UPROPERTY(Transient)
    TArray<FSkillState> ActiveCooldowns;

    // 距上次广播的累计时间（HUD 每秒刷新）
    float TimeSinceLastBroadcast = 0.0f;
};
