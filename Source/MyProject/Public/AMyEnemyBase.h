#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AMyEnemyBase.generated.h"

class UAttributeComponent;
class UBuffsComponent;

/**
 * 敌人基类（第一版：静态靶子 + 受击/死亡闭环，AI 追击后续添加）。
 *
 * 自动挂载：
 *  - UAttributeComponent（HP/受击/死亡；MaxHP/Attack/Defense 在蓝图类默认值配置）
 *  - UBuffsComponent（敌人 Buff，与玩家共用一套目录 DA_Buffs）
 *
 * 伤害入口：
 *  - 引擎 TakeDamage（被武器 Sweep / 投射物命中时由引擎调用）
 *  - ReceiveDamage（蓝图可直接调用）
 * 两者统一走 AttributeComponent::ApplyDamage → 蓝图事件 OnEnemyDamaged / OnEnemyDied。
 *
 * 死亡流程：禁用碰撞 + 停止移动 → OnEnemyDied（蓝图实现 Die 动画/掉落）
 *          → 延迟 Destroy（DestroyDelayAfterDeath，<=0 不销毁）。
 */
UCLASS()
class MYPROJECT_API AMyEnemyBase : public ACharacter
{
    GENERATED_BODY()

public:
    AMyEnemyBase();

    // ===== 伤害入口 =====

    // 引擎受击入口：被任何 ApplyDamage 来源命中时由引擎调用
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    // 蓝图可直接调用的受击入口（返回实际造成的伤害）
    UFUNCTION(BlueprintCallable, Category = "Enemy")
    float ReceiveDamage(float RawDamage, AActor* DamageCauser);

    // ===== 组件访问 =====

    UFUNCTION(BlueprintPure, Category = "Enemy")
    UAttributeComponent* GetAttribute() const { return AttributeComp; }

    UFUNCTION(BlueprintPure, Category = "Enemy")
    UBuffsComponent* GetBuffs() const { return BuffsComp; }

    // 死亡后延迟销毁秒数（<=0 表示不销毁）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
    float DestroyDelayAfterDeath = 5.0f;

protected:
    virtual void BeginPlay() override;

    // 蓝图事件：受击（ActualDamage = 实际伤害，用于飘字/GetHit 动画）
    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void OnEnemyDamaged(float ActualDamage, AActor* DamageCauser);

    // 蓝图事件：死亡（播放 Die 动画 / 掉落物品 / 经验 都在这里实现）
    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void OnEnemyDied(AController* EventInstigator, AActor* DamageCauser);

    // 死亡处理（防重入）：禁用碰撞 → 停止移动 → 蓝图事件 → 延迟销毁
    UFUNCTION()
    void HandleDeath();

private:
    void SelfDestruct();

    // ===== 组件 =====

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAttributeComponent> AttributeComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBuffsComponent> BuffsComp;

    // 死亡已处理标志（防 OnDied 重复触发导致二次流程）
    bool bDeathHandled = false;

    // 最后伤害来源（供死亡事件使用）
    TWeakObjectPtr<AActor> LastDamageCauser;
    TWeakObjectPtr<AController> LastEventInstigator;

    FTimerHandle DeathTimerHandle;
};
