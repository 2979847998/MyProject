#include "AMyEnemyBase.h"
#include "AttributeComponent.h"
#include "BuffsComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AMyEnemyBase::AMyEnemyBase()
{
    // 优化：本类无 Tick 逻辑，关闭每帧 Tick 减少开销。
    // 注意：若某个敌人蓝图子类需要 Event Tick，在该蓝图里重新打开即可。
    PrimaryActorTick.bCanEverTick = false;

    // 自动挂载属性与 Buff 组件（蓝图继承后可在类默认值配置 MaxHP/Attack/Defense 等）
    AttributeComp = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComp"));
    BuffsComp = CreateDefaultSubobject<UBuffsComponent>(TEXT("BuffsComp"));
}

void AMyEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    // 无论从哪个入口致死（ApplyDamage / SetHP），都走同一死亡流程
    if (AttributeComp)
    {
        AttributeComp->OnDied.AddDynamic(this, &AMyEnemyBase::HandleDeath);
    }
}

float AMyEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    LastEventInstigator = EventInstigator;
    LastDamageCauser = DamageCauser;
    return ReceiveDamage(DamageAmount, DamageCauser);
}

float AMyEnemyBase::ReceiveDamage(float RawDamage, AActor* DamageCauser)
{
    if (!AttributeComp || AttributeComp->IsDead())
    {
        return 0.0f;
    }

    LastDamageCauser = DamageCauser;

    // 防御减伤与死亡广播都在 ApplyDamage 内部处理
    const float ActualDamage = AttributeComp->ApplyDamage(RawDamage);
    if (ActualDamage > 0.0f)
    {
        OnEnemyDamaged(ActualDamage, DamageCauser);
    }

    // ApplyDamage 内部已在 HP 归零时广播 OnDied → HandleDeath（此处为双保险）
    if (AttributeComp->IsDead())
    {
        HandleDeath();
    }

    return ActualDamage;
}

void AMyEnemyBase::HandleDeath()
{
    if (bDeathHandled)
    {
        return;
    }
    bDeathHandled = true;

    // 停止行为：禁用碰撞 + 停止移动
    SetActorEnableCollision(false);
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
    }

    // 死亡蓝图事件：播放 Die 动画 / 掉落物品 / 经验 在这里实现
    OnEnemyDied(LastEventInstigator.Get(), LastDamageCauser.Get());

    // 延迟销毁（可选，<=0 不销毁）
    if (DestroyDelayAfterDeath > 0.0f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, this, &AMyEnemyBase::SelfDestruct, DestroyDelayAfterDeath, false);
    }
}

void AMyEnemyBase::SelfDestruct()
{
    Destroy();
}
