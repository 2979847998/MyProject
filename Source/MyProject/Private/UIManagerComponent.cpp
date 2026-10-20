#include "UIManagerComponent.h"
#include "UIViewBase.h"
#include "UIConfirmDialog.h"
#include "EquipmentWidget.h"
#include "USettingsSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"

UUIManagerComponent::UUIManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 装备面板是纯 C++ 构建（无需 WBP），这里给一个默认注册，开箱即可用 E 键打开。
    // 蓝图 PanelClasses 里若配置了 Equipment 项，以蓝图配置为准。
    PanelClasses.Add(EUIPanelType::Equipment, UEquipmentWidget::StaticClass());
}

void UUIManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 组件本身没有 InputComponent，直接用宿主 PlayerController 的 InputComponent 监听按键
    if (APlayerController* PC = GetOwnerPC())
    {
        // 订阅按键设置变更：玩家在“设置-按键”里改键后自动解绑重绑
        if (UGameInstance* GI = PC->GetGameInstance())
        {
            if (USettingsSubsystem* Settings = GI->GetSubsystem<USettingsSubsystem>())
            {
                Settings->OnKeyBindingsChanged.AddDynamic(this, &UUIManagerComponent::HandleKeyBindingsChanged);
            }
        }

        // 按设置里的当前按键绑定（默认 I/M/G/Esc，可在设置中修改）
        BindKeys();

        // 开局自动打开常驻主 HUD（未注册 HUD 时 OpenPanel 内部只会打一条 Warning）
        if (bAutoOpenHUDOnStart)
        {
            OpenPanel(EUIPanelType::HUD);
        }
    }
}

void UUIManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 1. 解绑设置子系统委托：子系统是 GameInstance 级（生命周期比 PlayerController 长），
    //    不主动解绑的话，PC 销毁后设置页改键广播会访问悬垂组件
    if (APlayerController* PC = GetOwnerPC())
    {
        if (UGameInstance* GI = PC->GetGameInstance())
        {
            if (USettingsSubsystem* Settings = GI->GetSubsystem<USettingsSubsystem>())
            {
                Settings->OnKeyBindingsChanged.RemoveDynamic(this, &UUIManagerComponent::HandleKeyBindingsChanged);
            }
        }
    }

    // 2. 从宿主 InputComponent 移除本组件注册的全部按键回调
    UnbindKeys();

    Super::EndPlay(EndPlayReason);
}

// ========== 对外接口 ==========

UUIViewBase* UUIManagerComponent::OpenPanel(EUIPanelType Type)
{
    if (Type == EUIPanelType::None) return nullptr;

    APlayerController* PC = GetOwnerPC();
    if (!PC) return nullptr;

    // 已缓存则直接复用，否则按注册表创建
    UUIViewBase* Panel = GetPanel(Type);
    if (!Panel)
    {
        TSubclassOf<UUIViewBase> PanelClass;
        if (TSubclassOf<UUIViewBase>* FoundClass = PanelClasses.Find(Type))
        {
            PanelClass = *FoundClass;
        }

        if (!PanelClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("[UI] OpenPanel: 未注册面板类型 %d"), static_cast<uint8>(Type));
            return nullptr;
        }

        Panel = CreateWidget<UUIViewBase>(PC, PanelClass);
        if (!Panel) return nullptr;

        Panel->PanelType = Type;
        Panel->OwnerUIManager = this;
        CachedPanels.Add(Type, Panel);
    }

    // 挂到对应分层（已在视口中则不重复添加）
    if (!Panel->IsInViewport())
    {
        Panel->AddToViewport(GetZOrderForLayer(Panel->Layer));
    }

    Panel->OnOpen();
    Panel->OnShow();

    // 面板打开后键盘焦点给面板：NativeOnKeyDown 兜底（按设置键开/关）不依赖输入组件焦点
    Panel->SetKeyboardFocus();

    // 提到栈顶
    PanelStack.Remove(Type);
    PanelStack.Add(Type);

    RefreshInputAndPause();
    UE_LOG(LogTemp, Warning, TEXT("[UI] OpenPanel: %d 打开, 栈=[%s]"),
        static_cast<uint8>(Type), *GetStackSummary());
    return Panel;
}

FString UUIManagerComponent::GetStackSummary() const
{
    TArray<FString> Parts;
    for (EUIPanelType T : PanelStack)
    {
        Parts.Add(FString::FromInt(static_cast<uint8>(T)));
    }
    return FString::Join(Parts, TEXT(","));
}

void UUIManagerComponent::ClosePanel(EUIPanelType Type)
{
    UUIViewBase* Panel = GetPanel(Type);
    if (!Panel) return;

    Panel->OnHide();
    Panel->OnClose();
    Panel->RemoveFromParent();

    PanelStack.Remove(Type);

    // 不缓存则释放引用，交由 GC 回收
    if (!Panel->bCacheWhenClosed)
    {
        CachedPanels.Remove(Type);
    }

    RefreshInputAndPause();
    UE_LOG(LogTemp, Warning, TEXT("[UI] ClosePanel: %d 关闭, 栈=[%s]"),
        static_cast<uint8>(Type), *GetStackSummary());
}

void UUIManagerComponent::CloseTopmost()
{
    for (int32 i = PanelStack.Num() - 1; i >= 0; --i)
    {
        UUIViewBase* Panel = GetPanel(PanelStack[i]);
        if (Panel && Panel->bCloseWithEsc)
        {
            ClosePanel(PanelStack[i]);
            return;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[UI] CloseTopmost: 无可关闭面板, 栈=[%s]"), *GetStackSummary());
}

void UUIManagerComponent::TogglePanel(EUIPanelType Type)
{
    if (IsPanelOpen(Type))
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] TogglePanel: %d 已开 -> 关闭"), static_cast<uint8>(Type));
        ClosePanel(Type);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] TogglePanel: %d 未开 -> 打开"), static_cast<uint8>(Type));
        OpenPanel(Type);
    }
}

void UUIManagerComponent::CloseAllPanels()
{
    TArray<EUIPanelType> ToClose = PanelStack;
    for (EUIPanelType Type : ToClose)
    {
        ClosePanel(Type);
    }
}

bool UUIManagerComponent::IsPanelOpen(EUIPanelType Type) const
{
    return PanelStack.Contains(Type);
}

UUIViewBase* UUIManagerComponent::GetPanel(EUIPanelType Type) const
{
    const TObjectPtr<UUIViewBase>* Found = CachedPanels.Find(Type);
    return Found ? Found->Get() : nullptr;
}

// ========== 通用确认弹窗 ==========

UUIConfirmDialog* UUIManagerComponent::ShowConfirmDialog(FText Title, FText Message, bool bShowCancel, FText ConfirmText, FText CancelText)
{
    UUIViewBase* View = OpenPanel(EUIPanelType::ConfirmDialog);
    UUIConfirmDialog* Dialog = Cast<UUIConfirmDialog>(View);
    if (Dialog)
    {
        Dialog->SetupDialog(Title, Message, bShowCancel, ConfirmText, CancelText);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] ShowConfirmDialog: ConfirmDialog 未注册或不是 UIConfirmDialog 子类"));
    }
    return Dialog;
}

// ========== Tooltip ==========

void UUIManagerComponent::ShowTooltip(UUserWidget* TooltipWidget, FVector2D ScreenPosition)
{
    if (!TooltipWidget) return;

    if (ActiveTooltip && ActiveTooltip != TooltipWidget)
    {
        ActiveTooltip->RemoveFromParent();
    }

    ActiveTooltip = TooltipWidget;

    if (!TooltipWidget->IsInViewport())
    {
        TooltipWidget->AddToViewport(GetZOrderForLayer(EUILayer::Tooltip));
    }

    TooltipWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
    TooltipWidget->SetPositionInViewport(ScreenPosition, false);
}

void UUIManagerComponent::HideTooltip()
{
    if (ActiveTooltip)
    {
        ActiveTooltip->RemoveFromParent();
        ActiveTooltip = nullptr;
    }
}

// ========== 内部 ==========

APlayerController* UUIManagerComponent::GetOwnerPC() const
{
    return Cast<APlayerController>(GetOwner());
}

int32 UUIManagerComponent::GetZOrderForLayer(EUILayer Layer) const
{
    switch (Layer)
    {
    case EUILayer::HUD:     return 0;
    case EUILayer::Panel:   return 10;
    case EUILayer::Popup:   return 20;
    case EUILayer::Tooltip: return 30;
    case EUILayer::Top:     return 100;
    default:                return 0;
    }
}

void UUIManagerComponent::RefreshInputAndPause()
{
    APlayerController* PC = GetOwnerPC();
    if (!PC) return;

    bool bHasUIOpen = false;
    bool bNeedMouse = false;
    bool bNeedPause = false;

    for (EUIPanelType Type : PanelStack)
    {
        UUIViewBase* Panel = GetPanel(Type);
        if (!Panel) continue;

        bHasUIOpen = true;
        if (Panel->bUseUIInput) bNeedMouse = true;
        if (Panel->bPauseGameWhenOpen) bNeedPause = true;
    }

    if (bHasUIOpen && bNeedMouse)
    {
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
    /*else
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = true;
    }*/

    // 有任意面板要求暂停则暂停；全部关闭后恢复
    // 无条件执行：SetPause 本身安全，Pawn 为空时也必须能解暂停，否则会永久卡在暂停
    PC->SetPause(bNeedPause);
    UE_LOG(LogTemp, Warning, TEXT("[UI] RefreshInputAndPause: 开UI=%d 鼠标=%d 暂停=%d, 栈=[%s]"),
        bHasUIOpen, bNeedMouse, bNeedPause, *GetStackSummary());
}

void UUIManagerComponent::HandleEscapePressed()
{
    UE_LOG(LogTemp, Warning, TEXT("[UI] 收到关闭键(ClosePanel 动作)"));
    CloseTopmost();
}

void UUIManagerComponent::HandleToggleInventory()
{
    UE_LOG(LogTemp, Warning, TEXT("[UI] 收到背包开关键(ToggleInventory)"));
    TogglePanel(EUIPanelType::Inventory);
}

void UUIManagerComponent::HandleToggleMap()
{
    UE_LOG(LogTemp, Warning, TEXT("[UI] 收到地图开关键(ToggleMap)"));
    TogglePanel(EUIPanelType::Map);
}

void UUIManagerComponent::HandleToggleGacha()
{
    UE_LOG(LogTemp, Warning, TEXT("[UI] 收到抽卡开关键(ToggleGacha)"));
    TogglePanel(EUIPanelType::Gacha);
}

void UUIManagerComponent::HandleToggleEquipment()
{
    UE_LOG(LogTemp, Warning, TEXT("[UI] 收到装备开关键(ToggleEquipment)"));
    TogglePanel(EUIPanelType::Equipment);
}

// ========== 按键绑定（读设置，可运行时重绑）==========

void UUIManagerComponent::BindKeys()
{
    APlayerController* PC = GetOwnerPC();
    if (!PC) return;
    UInputComponent* IC = PC->InputComponent;
    if (!IC) return;

    USettingsSubsystem* Settings = nullptr;
    if (UGameInstance* GI = PC->GetGameInstance())
    {
        Settings = GI->GetSubsystem<USettingsSubsystem>();
    }
    if (!Settings) return;

    // 同一键只允许一个动作：防止存档异常（例如两个动作被绑到同一个键）导致"一按双开"
    // 绑定策略：玩家自定义过的键优先（用户显式新绑的键必须生效），
    // 自定义键占用了默认键时，默认键让位（跳过绑定），保证“设置页新绑的键一定可以开关”。
    TSet<FKey> UsedKeys;
    TArray<EBindableAction> Actions = Settings->GetRemappableActions();

    // 第一遍：只绑定玩家自定义过的动作（SetBoundKey 内部已做同键互斥清理，自定义键之间不会冲突）
    for (EBindableAction Action : Actions)
    {
        if (!Settings->HasCustomKey(Action))
        {
            continue;
        }
        const FKey Key = Settings->GetBoundKey(Action);
        if (!Key.IsValid() || UsedKeys.Contains(Key))
        {
            UE_LOG(LogTemp, Warning, TEXT("[UI] BindKeys: 自定义键 %s 无效/冲突，%s 跳过"),
                *Key.ToString(), *Settings->GetActionDisplayName(Action).ToString());
            continue;
        }
        UsedKeys.Add(Key);
        BindActionKey(IC, Action, Key);
    }

    // 第二遍：未自定义的动作绑定默认键；默认键被自定义占用则让位（不绑，避免一按双开）
    for (EBindableAction Action : Actions)
    {
        if (Settings->HasCustomKey(Action))
        {
            continue;
        }
        const FKey Key = Settings->GetDefaultKey(Action);
        if (!Key.IsValid())
        {
            continue;
        }
        if (UsedKeys.Contains(Key))
        {
            UE_LOG(LogTemp, Warning, TEXT("[UI] BindKeys: 默认键 %s 已被自定义占用，%s 让位不绑定"),
                *Key.ToString(), *Settings->GetActionDisplayName(Action).ToString());
            continue;
        }
        UsedKeys.Add(Key);
        BindActionKey(IC, Action, Key);
    }
}

void UUIManagerComponent::BindActionKey(UInputComponent* IC, EBindableAction Action, FKey Key)
{
    USettingsSubsystem* Settings = nullptr;
    if (APlayerController* PC = GetOwnerPC())
    {
        if (UGameInstance* GI = PC->GetGameInstance())
        {
            Settings = GI->GetSubsystem<USettingsSubsystem>();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[UI] BindKeys: %s -> %s"),
        Settings ? *Settings->GetActionDisplayName(Action).ToString() : TEXT("?"), *Key.ToString());

    switch (Action)
    {
    case EBindableAction::ClosePanel:
        IC->BindKey(Key, IE_Pressed, this, &UUIManagerComponent::HandleEscapePressed);
        break;
    case EBindableAction::ToggleInventory:
        IC->BindKey(Key, IE_Pressed, this, &UUIManagerComponent::HandleToggleInventory);
        break;
    case EBindableAction::ToggleMap:
        IC->BindKey(Key, IE_Pressed, this, &UUIManagerComponent::HandleToggleMap);
        break;
    case EBindableAction::ToggleGacha:
        IC->BindKey(Key, IE_Pressed, this, &UUIManagerComponent::HandleToggleGacha);
        break;
    case EBindableAction::ToggleEquipment:
        IC->BindKey(Key, IE_Pressed, this, &UUIManagerComponent::HandleToggleEquipment);
        break;
    default:
        break;
    }
}

void UUIManagerComponent::UnbindKeys()
{
    APlayerController* PC = GetOwnerPC();
    UInputComponent* IC = PC ? PC->InputComponent : nullptr;
    if (IC)
    {
        // 移除所有绑定到本组件对象的按键回调（即上面 BindKeys 注册的那几个）
        IC->ClearBindingsForObject(this);
    }
}

void UUIManagerComponent::HandleKeyBindingsChanged()
{
    UnbindKeys();
    BindKeys();
}
