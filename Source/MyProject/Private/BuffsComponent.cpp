#include "BuffsComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffsComponent::UBuffsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBuffsComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyStatEffects();
}

void UBuffsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 每秒检查一次到期/广播（图标栏显示整数秒即可）
    TimeSinceLastBroadcast += DeltaTime;
    const bool bBroadcastNow = TimeSinceLastBroadcast >= 1.0f;

    bool bChanged = false;
    for (int32 i = ActiveBuffs.Num() - 1; i >= 0; --i)
    {
        FActiveBuff& Buff = ActiveBuffs[i];
        if (Buff.TimeRemaining >= 0.0f)
        {
            Buff.TimeRemaining -= DeltaTime;
            if (Buff.TimeRemaining <= 0.0f)
            {
                ActiveBuffs.RemoveAt(i);
                bChanged = true;
            }
        }
    }

    if (bChanged)
    {
        ApplyStatEffects();
    }
    if (bBroadcastNow && (bChanged || ActiveBuffs.Num() > 0))
    {
        TimeSinceLastBroadcast = 0.0f;
        OnBuffsChanged.Broadcast();
    }

    // 没有定时 Buff 时无需每帧 Tick（AddBuff 添加定时 Buff 会重新开启）
    if (!HasTimedBuffs())
    {
        SetComponentTickEnabled(false);
    }
}

bool UBuffsComponent::HasTimedBuffs() const
{
    for (const FActiveBuff& Buff : ActiveBuffs)
    {
        if (Buff.TimeRemaining >= 0.0f)
        {
            return true;
        }
    }
    return false;
}

bool UBuffsComponent::HasBuff(FName BuffID) const
{
    return ActiveBuffs.ContainsByPredicate([BuffID](const FActiveBuff& Buff) { return Buff.BuffID == BuffID; });
}

float UBuffsComponent::GetStatModifier(EBuffStat Stat) const
{
    // Multiplicative：连乘（默认 1.0 = 无加成）
    UBuffCatalog* Catalog = UBuffCatalog::Resolve(BuffCatalog);
    if (!Catalog)
    {
        return 1.0f;
    }

    float Multiplier = 1.0f;
    for (const FActiveBuff& Buff : ActiveBuffs)
    {
        const FBuffData* Data = Catalog->FindBuff(Buff.BuffID);
        if (Data && Data->Stat == Stat && Data->ModifierType == EBuffModifierType::Multiplicative)
        {
            Multiplier *= FMath::Pow(Data->ModifierValue, static_cast<float>(Buff.Stack));
        }
    }
    return Multiplier;
}

bool UBuffsComponent::AddBuff(FName BuffID, int32 Stack)
{
    if (Stack <= 0)
    {
        return false;
    }
    UBuffCatalog* Catalog = UBuffCatalog::Resolve(BuffCatalog);
    const FBuffData* Data = Catalog ? Catalog->FindBuff(BuffID) : nullptr;
    if (!Data)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BuffsComponent] AddBuff: 目录查不到 BuffID=%s"), *BuffID.ToString());
        return false;
    }

    FActiveBuff* Existing = ActiveBuffs.FindByPredicate([BuffID](const FActiveBuff& Buff) { return Buff.BuffID == BuffID; });
    switch (Data->StackRule)
    {
    case EBuffStackRule::Refresh:
        if (Existing)
        {
            Existing->TimeRemaining = Data->Duration > 0.0f ? Data->Duration : -1.0f;
            Existing->Stack = 1;
        }
        else
        {
            FActiveBuff NewBuff;
            NewBuff.BuffID = BuffID;
            NewBuff.TimeRemaining = Data->Duration > 0.0f ? Data->Duration : -1.0f;
            NewBuff.Stack = 1;
            ActiveBuffs.Add(NewBuff);
        }
        break;

    case EBuffStackRule::Stack:
        if (Existing)
        {
            Existing->Stack = FMath::Min(Existing->Stack + Stack, Data->MaxStack);
            Existing->TimeRemaining = Data->Duration > 0.0f ? Data->Duration : -1.0f;
        }
        else
        {
            FActiveBuff NewBuff;
            NewBuff.BuffID = BuffID;
            NewBuff.TimeRemaining = Data->Duration > 0.0f ? Data->Duration : -1.0f;
            NewBuff.Stack = Stack;
            ActiveBuffs.Add(NewBuff);
        }
        break;

    case EBuffStackRule::Override:
    default:
        ActiveBuffs.RemoveAll([BuffID](const FActiveBuff& Buff) { return Buff.BuffID == BuffID; });
        {
            FActiveBuff NewBuff;
            NewBuff.BuffID = BuffID;
            NewBuff.TimeRemaining = Data->Duration > 0.0f ? Data->Duration : -1.0f;
            NewBuff.Stack = Stack;
            ActiveBuffs.Add(NewBuff);
        }
        break;
    }

    ApplyStatEffects();
    OnBuffsChanged.Broadcast();

    // 定时 Buff 需要 Tick 计时
    if (Data->Duration > 0.0f)
    {
        SetComponentTickEnabled(true);
    }
    return true;
}

void UBuffsComponent::RemoveBuff(FName BuffID)
{
    const int32 Removed = ActiveBuffs.RemoveAll([BuffID](const FActiveBuff& Buff) { return Buff.BuffID == BuffID; });
    if (Removed > 0)
    {
        ApplyStatEffects();
        OnBuffsChanged.Broadcast();
    }
}

void UBuffsComponent::ClearBuffs()
{
    if (ActiveBuffs.Num() > 0)
    {
        ActiveBuffs.Reset();
        ApplyStatEffects();
        OnBuffsChanged.Broadcast();
    }
}

void UBuffsComponent::ApplyStatEffects()
{
    ApplyMoveSpeed();
}

void UBuffsComponent::ApplyMoveSpeed()
{
    // 通用挂接：宿主为 PlayerController 时取 Pawn，否则宿主本身就是 Pawn/Character（敌人等）
    APawn* Pawn = nullptr;
    if (AActor* OwnerActor = GetOwner())
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerActor))
        {
            Pawn = PC->GetPawn();
        }
        else
        {
            Pawn = Cast<APawn>(OwnerActor);
        }
    }
    if (!Pawn)
    {
        return;
    }

    const float Modifier = GetStatModifier(EBuffStat::MoveSpeed);

    // 首次记录基础速度（之后恢复用）
    if (BaseMaxSpeed <= 0.0f)
    {
        BaseMaxSpeed = Pawn->GetMovementComponent() ? Pawn->GetMovementComponent()->GetMaxSpeed() : 0.0f;
    }
    if (BaseMaxSpeed <= 0.0f)
    {
        return;
    }

    if (UFloatingPawnMovement* FloatComp = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent()))
    {
        // 玩家 DefaultPawn / 浮空移动敌人
        FloatComp->MaxSpeed = BaseMaxSpeed * Modifier;
    }
    else if (UCharacterMovementComponent* CharComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent()))
    {
        // 角色类敌人（CharacterMovementComponent）
        CharComp->MaxWalkSpeed = BaseMaxSpeed * Modifier;
    }
}
