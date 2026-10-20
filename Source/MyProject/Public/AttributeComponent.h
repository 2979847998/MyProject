#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

class UBuffsComponent;

// 属性数值变化时广播（参数 = 新值）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChanged, float, NewValue);

// HP 归零死亡时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDied);

/**
 * 玩家/敌人通用属性组件：当前 HP/MP、攻击、防御等战斗数值。
 *
 * 与 Buff 系统打通：
 *  - GetDamageMultiplier / GetCoinMultiplier 读宿主 UBuffsComponent::GetStatModifier
 *    （MoveSpeed 仍由 BuffsComponent 直接写 DefaultPawn.MaxSpeed，本组件不管）；
 *  - GetFinalAttack() = Attack × 伤害倍率，供战斗计算使用。
 *
 * 挂载方式：在角色/敌人蓝图（Pawn/Character）上添加本组件，
 * 在蓝图类默认值里配置 MaxHP/MaxMP/Attack/Defense。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UAttributeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAttributeComponent();

    // ===== 基础值（蓝图类默认值中配置）=====

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
    float MaxHP = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
    float MaxMP = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
    float Attack = 10.0f;

    // 防御：ApplyDamage 中按减伤比例生效（0~0.9 夹取）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
    float Defense = 0.0f;

    // ===== 当前值（运行期，蓝图只读）=====

    UPROPERTY(BlueprintReadOnly, Category = "Attribute")
    float HP = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Attribute")
    float MP = 0.0f;

    // ===== 事件 =====

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnHPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnMPChanged;

    // 基础值变化广播（装备穿戴/卸下等修改时触发，UI 属性面板刷新用）
    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnMaxHPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnMaxMPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnAttackChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnAttributeChanged OnDefenseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attribute")
    FOnDied OnDied;

    // ===== 查询 =====

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetMaxHP() const { return MaxHP; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetMaxMP() const { return MaxMP; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetHP() const { return HP; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetMP() const { return MP; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    bool IsDead() const { return HP <= 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetAttack() const { return Attack; }

    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetDefense() const { return Defense; }

    // ===== 数值修改 =====

    // 直接设置 HP（自动夹取 0~MaxHP 并广播、触发死亡事件）
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void SetHP(float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void SetMP(float NewValue);

    // 增量修改 HP（正=回血，负=掉血），返回修改后的 HP
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    float ModifyHP(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attribute")
    float ModifyMP(float Delta);

    // 增量修改基础值（装备穿戴/卸下等）。Max 变化时当前值同步增减，保持"穿装备不掉血"的直觉
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void ModifyMaxHP(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void ModifyMaxMP(float Delta);

    // 攻击/防御基础值修改（防御夹取 0~0.9，与 ApplyDamage 减伤上限一致）
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void ModifyAttack(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void ModifyDefense(float Delta);

    // 承受伤害：RawDamage 先按防御减伤，再扣 HP，返回实际造成的伤害
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    float ApplyDamage(float RawDamage);

    // 满血 / 满蓝
    UFUNCTION(BlueprintCallable, Category = "Attribute")
    void RestoreFully();

    // ===== 与 Buff 打通 =====

    // 宿主上的 Buff 组件（未挂载返回 nullptr）
    UFUNCTION(BlueprintPure, Category = "Attribute")
    UBuffsComponent* GetBuffsComponent() const;

    // 伤害倍率（Buff 汇总；无 Buff 为 1.0）
    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetDamageMultiplier() const;

    // 金币倍率（Buff 汇总；无 Buff 为 1.0）
    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetCoinMultiplier() const;

    // 最终攻击力 = Attack × 伤害倍率（战斗计算用）
    UFUNCTION(BlueprintPure, Category = "Attribute")
    float GetFinalAttack() const;

protected:
    virtual void BeginPlay() override;

private:
    void BroadcastHP();
    void BroadcastMP();
};
