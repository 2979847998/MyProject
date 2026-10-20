#include "SkillsComponent.h"

#include "BuffsComponent.h"
#include "GameFramework/PlayerController.h"

USkillsComponent::USkillsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USkillsComponent::BeginPlay()
{
    Super::BeginPlay();
    Energy = MaxEnergy;
}

void USkillsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 冷却递减
    bool bCooldownChanged = false;
    for (int32 i = ActiveCooldowns.Num() - 1; i >= 0; --i)
    {
        FSkillState& State = ActiveCooldowns[i];
        if (State.CooldownRemaining > 0.0f)
        {
            State.CooldownRemaining = FMath::Max(0.0f, State.CooldownRemaining - DeltaTime);
            if (State.CooldownRemaining <= 0.0f)
            {
                ActiveCooldowns.RemoveAt(i);
                bCooldownChanged = true;
            }
        }
    }

    // 能量自然回复（每秒一次，仅实际回复时广播）
    bool bEnergyChanged = false;
    TimeSinceLastBroadcast += DeltaTime;
    if (TimeSinceLastBroadcast >= 1.0f)
    {
        TimeSinceLastBroadcast = 0.0f;
        if (Energy < MaxEnergy)
        {
            Energy = FMath::Min(MaxEnergy, Energy + EnergyRegenPerSecond);
            bEnergyChanged = true;
        }
    }

    // 只在确有变化时广播（冷却倒计时由技能格子自刷新，不需要每秒广播驱动重建）
    if (bCooldownChanged || bEnergyChanged)
    {
        OnSkillsChanged.Broadcast();
    }

    // 无冷却且能量已满：无需每帧 Tick（CastSkill 会重新开启）
    if (ActiveCooldowns.Num() == 0 && Energy >= MaxEnergy)
    {
        SetComponentTickEnabled(false);
    }
}

float USkillsComponent::GetCooldownRemaining(FName SkillID) const
{
    const FSkillState* State = ActiveCooldowns.FindByPredicate([SkillID](const FSkillState& S) { return S.SkillID == SkillID; });
    return State ? State->CooldownRemaining : 0.0f;
}

bool USkillsComponent::CastSkillBySlot(int32 SlotIndex)
{
    USkillCatalog* Catalog = USkillCatalog::Resolve(SkillCatalog);
    if (!Catalog || !Catalog->Skills.IsValidIndex(SlotIndex))
    {
        return false;
    }
    return CastSkill(Catalog->Skills[SlotIndex].SkillID);
}

bool USkillsComponent::CastSkill(FName SkillID)
{
    USkillCatalog* Catalog = USkillCatalog::Resolve(SkillCatalog);
    const FSkillData* Data = Catalog ? Catalog->FindSkill(SkillID) : nullptr;
    if (!Data)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillsComponent] CastSkill: 目录查不到 SkillID=%s"), *SkillID.ToString());
        return false;
    }

    // 冷却检查
    if (GetCooldownRemaining(SkillID) > 0.0f)
    {
        return false;
    }

    // 能量检查
    if (Energy < Data->EnergyCost)
    {
        return false;
    }

    // 扣能量 + 进入冷却
    Energy = FMath::Max(0.0f, Energy - Data->EnergyCost);
    if (Data->Cooldown > 0.0f)
    {
        FSkillState NewState;
        NewState.SkillID = SkillID;
        NewState.CooldownRemaining = Data->Cooldown;
        ActiveCooldowns.Add(NewState);
    }

    // 有冷却或能量待回复时需要 Tick
    SetComponentTickEnabled(true);

    // 执行效果（失败也认为已释放：消耗和冷却成立，效果本身可能被蓝图处理）
    ExecuteEffect(*Data);

    OnSkillCasted.Broadcast(SkillID);
    OnSkillsChanged.Broadcast();
    return true;
}

void USkillsComponent::AddEnergy(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }
    Energy = FMath::Min(MaxEnergy, Energy + Amount);

    // 未满则需要继续回复（Tick 开）；已满且无冷却则无需 Tick
    SetComponentTickEnabled(Energy < MaxEnergy || ActiveCooldowns.Num() > 0);

    OnSkillsChanged.Broadcast();
}

bool USkillsComponent::ExecuteEffect(const FSkillData& Data)
{
    switch (Data.EffectType)
    {
    case ESkillEffectType::ApplyBuff:
    {
        if (Data.BuffID.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[SkillsComponent] %s: ApplyBuff 但 BuffID 为空"), *Data.SkillID.ToString());
            return false;
        }
        if (UBuffsComponent* Buffs = GetHostBuffs())
        {
            return Buffs->AddBuff(Data.BuffID, Data.BuffStack);
        }
        return false;
    }

    case ESkillEffectType::Instant:
        ApplyInstantEffect(Data.SkillID, Data.InstantValue);
        return true;

    case ESkillEffectType::None:
    default:
        // 无内置效果：释放钩子（OnSkillCasted）已广播，效果由蓝图接
        return true;
    }
}

UBuffsComponent* USkillsComponent::GetHostBuffs() const
{
    // 宿主 = PlayerController；玩家身上的 BuffsComponent 挂在同一宿主上
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        return PC->FindComponentByClass<UBuffsComponent>();
    }
    return nullptr;
}

// 蓝图实现钩子默认实现：不做事（Instant 效果的落地由蓝图覆写）
void USkillsComponent::ApplyInstantEffect_Implementation(FName SkillID, float InstantValue)
{
    UE_LOG(LogTemp, Warning, TEXT("[SkillsComponent] %s: Instant 效果未实现（请覆写 ApplyInstantEffect 蓝图事件）"), *SkillID.ToString());
}
