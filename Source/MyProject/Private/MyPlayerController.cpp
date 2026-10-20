// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "UIManagerComponent.h"
#include "UIViewBase.h"
#include "USettingsSubsystem.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"

AMyPlayerController::AMyPlayerController()
{
    // 点击移动需要可见鼠标（具体显示逻辑仍由 UIManagerComponent 按面板状态控制）
    bShowMouseCursor = true;
}

void AMyPlayerController::SetAttackState(bool bNewState)
{
    if (bIsAttacking != bNewState)
    {
        bIsAttacking = bNewState;
        OnAttackStateChanged.Broadcast(bIsAttacking);
    }
   
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // 动态创建增强输入资源（纯 C++，无需蓝图资产）
    if (!ClickMoveAction)
    {
        ClickMoveAction = NewObject<UInputAction>(this);
        ClickMoveAction->ValueType = EInputActionValueType::Boolean;
    }
    if (!ClickMoveContext)
    {
        ClickMoveContext = NewObject<UInputMappingContext>(this);
        ClickMoveContext->MapKey(ClickMoveAction, EKeys::LeftMouseButton);
    }

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInput->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &AMyPlayerController::OnClickMovePressed);
    }

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        // 点击移动优先级 10（保住核心功能）；IMC_Keys 用低优先级 0
        Subsystem->AddMappingContext(ClickMoveContext, 10);
    }

    // 接入 IMC_Keys（克隆版，应用存档自定义键），低优先级不抢占点击移动
    SetupEnhancedKeys();
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 设置里改键（legacy 或增强输入）→ 重绑
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USettingsSubsystem* Settings = GI->GetSubsystem<USettingsSubsystem>())
        {
            Settings->OnKeyBindingsChanged.AddDynamic(this, &AMyPlayerController::HandleKeyBindingsChanged);
        }
    }
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 解绑设置子系统委托：子系统是 GameInstance 级，比 PC 活得久，不解绑会导致悬垂回调
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USettingsSubsystem* Settings = GI->GetSubsystem<USettingsSubsystem>())
        {
            Settings->OnKeyBindingsChanged.RemoveDynamic(this, &AMyPlayerController::HandleKeyBindingsChanged);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::HandleKeyBindingsChanged()
{
    RebuildEnhancedKeys();
}

// 取蓝图配置的键位映射上下文；未配置（默认空）时不绑定任何 IMC
UInputMappingContext* AMyPlayerController::GetEnhancedKeysContext() const
{
    return EnhancedKeysContext;
}

// 取设置页可重绑的增强 Action 清单：
// 蓝图配置 EnhancedKeyActions 时用配置；否则自动收集 IMC 里已映射的 Action（去重），
// 并补齐工程默认四个 IA（IA_Attack / IA_Chilk / IA_Horizontal / IA_Vertical）——
// 这样即使资产里没有键位映射，设置页也能列出它们供重绑
TArray<UInputAction*> AMyPlayerController::GetRebindableEnhancedActions() const
{
    TArray<UInputAction*> Out;

    if (EnhancedKeyActions.Num() > 0)
    {
        for (const TObjectPtr<UInputAction>& A : EnhancedKeyActions)
        {
            if (A)
            {
                Out.AddUnique(A.Get());
            }
        }
        return Out;
    }

    // 自动收集：IMC 里已映射的 Action（含键无效的占位映射）
    if (UInputMappingContext* IMC = EnhancedKeysContext)
    {
        for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
        {
            if (M.Action)
            {
                Out.AddUnique(const_cast<UInputAction*>(M.Action.Get()));
            }
        }
    }

    // 补齐默认四 IA（资产里没有映射也能在设置页绑定；绑定的键写存档并动态建映射）
    static const TCHAR* DefaultActionPaths[] = {
        TEXT("/Game/Blueprints_Old/InputKey/IA_Attack.IA_Attack"),
        TEXT("/Game/Blueprints_Old/InputKey/IA_Chilk.IA_Chilk"),
        TEXT("/Game/Blueprints_Old/InputKey/IA_Horizontal.IA_Horizontal"),
        TEXT("/Game/Blueprints_Old/InputKey/IA_Vertical.IA_Vertical"),
    };
    for (const TCHAR* Path : DefaultActionPaths)
    {
        if (UInputAction* IA = LoadObject<UInputAction>(nullptr, Path))
        {
            Out.AddUnique(IA);
        }
    }
    return Out;
}

UInputMappingContext* AMyPlayerController::GetRuntimeKeysIMC() const
{
    return RuntimeIMCKeys;
}

// 首次接入：加载 IMC -> 克隆 -> 应用存档键位 -> 挂载
void AMyPlayerController::SetupEnhancedKeys()
{
    if (RuntimeIMCKeys)
    {
        return; // 已初始化
    }

    // 即使未配置 IMC 也挂一个空运行时 IMC：设置页为 IA 绑定键后动态 MapKey 即可生效
    RuntimeIMCKeys = NewObject<UInputMappingContext>(this);
    if (UInputMappingContext* SourceIMC = GetEnhancedKeysContext())
    {
        RuntimeIMCKeys->ContextDescription = SourceIMC->ContextDescription;
    }

    RebuildRuntimeIMCFromSources();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(RuntimeIMCKeys, 0);
        Subsystem->RequestRebuildControlMappings();
    }

    UE_LOG(LogTemp, Warning, TEXT("[MyPlayerController] 增强键位 IMC 已接入（克隆版，%d 个有效映射）"),
        RuntimeIMCKeys->GetMappings().Num());

    // 打印运行时映射明细：确认每个 Action 实际绑定的键（定位"IA_Attack 无法监听 LMB"）
    for (const FEnhancedActionKeyMapping& M : RuntimeIMCKeys->GetMappings())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MyPlayerController] 运行时映射: Action=%s Key=%s Trigger数=%d"),
            M.Action ? *M.Action->GetName() : TEXT("null"), *M.Key.ToString(), M.Triggers.Num());
    }
}

// 按可重绑清单重建克隆 IMC：设置 override 优先，其次资产默认键，两者都无则跳过（未绑定）
void AMyPlayerController::RebuildRuntimeIMCFromSources()
{
    if (!RuntimeIMCKeys)
    {
        return;
    }

    RuntimeIMCKeys->UnmapAll();

    UInputMappingContext* SourceIMC = GetEnhancedKeysContext();
    USettingsSubsystem* Settings = nullptr;
    if (UGameInstance* GI = GetGameInstance())
    {
        Settings = GI->GetSubsystem<USettingsSubsystem>();
    }

    for (UInputAction* Action : GetRebindableEnhancedActions())
    {
        if (!Action)
        {
            continue;
        }

        const FName ActionName = Action->GetFName();

        // 1. 设置里绑过自定义键 → 用它建映射
        FKey OverrideKey = EKeys::Invalid;
        if (Settings)
        {
            OverrideKey = Settings->GetEnhancedKey(ActionName, EKeys::Invalid);
        }
        if (OverrideKey.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MyPlayerController] Rebuild: %s override=%s"),
                *ActionName.ToString(), *OverrideKey.ToString());
            FEnhancedActionKeyMapping& NewMap = RuntimeIMCKeys->MapKey(Action, OverrideKey);
            // 若资产里有该 Action 的映射，顺带复制 Triggers/Modifiers
            if (SourceIMC)
            {
                for (const FEnhancedActionKeyMapping& M : SourceIMC->GetMappings())
                {
                    if (M.Action == Action)
                    {
                        NewMap.Triggers = M.Triggers;
                        NewMap.Modifiers = M.Modifiers;
                        break;
                    }
                }
            }
            continue;
        }

        // 2. 无自定义键 → 用资产默认键（没有映射的 Action 保持“未绑定”，不建映射）
        if (SourceIMC)
        {
            for (const FEnhancedActionKeyMapping& M : SourceIMC->GetMappings())
            {
                if (M.Action == Action && M.Key.IsValid())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MyPlayerController] Rebuild: %s 资产默认键=%s"),
                        *ActionName.ToString(), *M.Key.ToString());
                    FEnhancedActionKeyMapping& NewMap = RuntimeIMCKeys->MapKey(Action, M.Key);
                    NewMap.Triggers = M.Triggers;
                    NewMap.Modifiers = M.Modifiers;
                    break;
                }
            }
        }
    }
}

// 设置改键后：从磁盘默认键重置克隆版（避免叠加），再应用 override 并重建映射
void AMyPlayerController::RebuildEnhancedKeys()
{
    if (!RuntimeIMCKeys)
    {
        SetupEnhancedKeys();
        return;
    }

    RebuildRuntimeIMCFromSources();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->RequestRebuildControlMappings();
    }
}

bool AMyPlayerController::IsAnyUIOpen() const
{
    // HUD 是常驻层，不拦截点击；其余面板打开时点击归 UI 使用
    if (const UUIManagerComponent* UI = GetComponentByClass<UUIManagerComponent>())
    {
        for (uint8 i = static_cast<uint8>(EUIPanelType::Inventory); i <= static_cast<uint8>(EUIPanelType::Gacha); ++i)
        {
            if (i == static_cast<uint8>(EUIPanelType::HUD))
            {
                continue;
            }
            if (UI->IsPanelOpen(static_cast<EUIPanelType>(i)))
            {
                return true;
            }
        }
    }
    return false;
}

void AMyPlayerController::OnClickMovePressed()
{
    // 有面板打开时，点击交给 UI（背包格子、设置控件等）
    if (IsAnyUIOpen())
    {
        return;
    }

    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit)
    {
        MoveTarget = Hit.Location;
        bMoveToTarget = true;
    }
}

void AMyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!bMoveToTarget)
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        bMoveToTarget = false;
        return;
    }

    // 只取水平距离（Z 忽略），到达判定
    const FVector ToTarget = MoveTarget - ControlledPawn->GetActorLocation();
    const FVector Horizontal = FVector(ToTarget.X, ToTarget.Y, 0.f);
    if (Horizontal.SizeSquared() <= StopDistance * StopDistance)
    {
        bMoveToTarget = false;
        return;
    }

    const FVector MoveDir = Horizontal.GetSafeNormal2D();

    // 角色：走 CharacterMovement 输入（保留动画/寻路相关行为）
    if (ACharacter* MoveCharacter = Cast<ACharacter>(ControlledPawn))
    {
        MoveCharacter->AddMovementInput(MoveDir, 1.f);
        return;
    }

    // 其它 Pawn：有移动组件则直接喂输入，否则直线平移
    if (UCharacterMovementComponent* MoveComp = ControlledPawn->FindComponentByClass<UCharacterMovementComponent>())
    {
        MoveComp->AddInputVector(MoveDir);
    }
    else
    {
        const float MoveSpeed = 600.f;
        ControlledPawn->SetActorLocation(ControlledPawn->GetActorLocation() + MoveDir * MoveSpeed * DeltaTime);
    }
}
