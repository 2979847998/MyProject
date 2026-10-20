#include "AttributeComponent.h"
#include "BuffsComponent.h"
#include "GameFramework/Actor.h"

UAttributeComponent::UAttributeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAttributeComponent::BeginPlay()
{
    Super::BeginPlay();

    // 开局按最大值初始化（若蓝图在关卡中改过 Max 值也能生效）
    HP = MaxHP;
    MP = MaxMP;


}

void UAttributeComponent::SetHP(float NewValue)
{
    const float Old = HP;
    HP = FMath::Clamp(NewValue, 0.0f, MaxHP);
    BroadcastHP();

    if (Old > 0.0f && HP <= 0.0f)
    {
        OnDied.Broadcast();
    }
}

void UAttributeComponent::SetMP(float NewValue)
{
    MP = FMath::Clamp(NewValue, 0.0f, MaxMP);
    BroadcastMP();
}

float UAttributeComponent::ModifyHP(float Delta)
{
    SetHP(HP + Delta);
    return HP;
}

float UAttributeComponent::ModifyMP(float Delta)
{
    SetMP(MP + Delta);
    return MP;
}

void UAttributeComponent::ModifyMaxHP(float Delta)
{
    const float OldHP = HP;
    MaxHP = FMath::Max(0.0f, MaxHP + Delta);
    // 当前 HP 同步增减并夹取，避免穿装备后"按满血比例掉血"的异常感
    HP = FMath::Clamp(HP + Delta, 0.0f, MaxHP);
    OnMaxHPChanged.Broadcast(MaxHP);
    BroadcastHP();

    // 卸下 +HP 装备导致当前 HP 归零时仍需触发死亡事件（防重复：仅在旧值>0 时广播）
    if (OldHP > 0.0f && HP <= 0.0f)
    {
        OnDied.Broadcast();
    }
}

void UAttributeComponent::ModifyMaxMP(float Delta)
{
    MaxMP = FMath::Max(0.0f, MaxMP + Delta);
    MP = FMath::Clamp(MP + Delta, 0.0f, MaxMP);
    OnMaxMPChanged.Broadcast(MaxMP);
    BroadcastMP();
}

void UAttributeComponent::ModifyAttack(float Delta)
{
    Attack = FMath::Max(0.0f, Attack + Delta);
    OnAttackChanged.Broadcast(Attack);
}

void UAttributeComponent::ModifyDefense(float Delta)
{
    Defense = FMath::Clamp(Defense + Delta, 0.0f, 0.9f);
    OnDefenseChanged.Broadcast(Defense);
}

float UAttributeComponent::ApplyDamage(float RawDamage)
{
    if (RawDamage <= 0.0f || HP <= 0.0f)
    {
        return 0.0f;
    }

    // 防御减伤：Defense=0 → 全额；Defense 越大减免越多（封顶 90%）
    const float Mitigation = FMath::Clamp(Defense, 0.0f, 0.9f);
    const float ActualDamage = RawDamage * (1.0f - Mitigation);

    const float Old = HP;
    HP = FMath::Max(HP - ActualDamage, 0.0f);
    BroadcastHP();

    if (Old > 0.0f && HP <= 0.0f)
    {
        OnDied.Broadcast();
    }
    return ActualDamage;
}

void UAttributeComponent::RestoreFully()
{
    HP = MaxHP;
    MP = MaxMP;
    BroadcastHP();
    BroadcastMP();
}

UBuffsComponent* UAttributeComponent::GetBuffsComponent() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UBuffsComponent>() : nullptr;
}

float UAttributeComponent::GetDamageMultiplier() const
{
    const UBuffsComponent* Buffs = GetBuffsComponent();
    return Buffs ? Buffs->GetStatModifier(EBuffStat::DamageMultiplier) : 1.0f;
}

float UAttributeComponent::GetCoinMultiplier() const
{
    const UBuffsComponent* Buffs = GetBuffsComponent();
    return Buffs ? Buffs->GetStatModifier(EBuffStat::CoinMultiplier) : 1.0f;
}

float UAttributeComponent::GetFinalAttack() const
{
    return Attack * GetDamageMultiplier();
}

void UAttributeComponent::BroadcastHP()
{
    OnHPChanged.Broadcast(HP);
}

void UAttributeComponent::BroadcastMP()
{
    OnMPChanged.Broadcast(MP);
}
