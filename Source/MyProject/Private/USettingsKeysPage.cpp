#include "USettingsKeysPage.h"
#include "USettingsSubsystem.h"
#include "MyPlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"

// 增强输入键位资产路径（默认 IMC_Keys；PC 蓝图配置了 IMC 时优先用配置的）
static const TCHAR* EnhancedKeysIMCPath = TEXT("/Game/Blueprints_Old/IMC_Keys.IMC_Keys");

// 解析当前要展示的键位映射上下文：只取 PC 蓝图配置的 IMC，未配置则不显示增强行
static UInputMappingContext* ResolveEnhancedKeysIMC(const UUserWidget* Widget)
{
    if (const AMyPlayerController* PC = Widget ? Widget->GetOwningPlayer<AMyPlayerController>() : nullptr)
    {
        return PC->GetEnhancedKeysContext();
    }
    return nullptr;
}

void USettingsKeysPage::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        SettingsMgr = GI->GetSubsystem<USettingsSubsystem>();
    }
    if (!SettingsMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsKeysPage] 获取 USettingsSubsystem 失败"));
        return;
    }

    BuildKeyBindingRows();
    if (Btn_ResetKeys) Btn_ResetKeys->OnClicked.AddDynamic(this, &USettingsKeysPage::OnResetKeysClicked);
}

void USettingsKeysPage::NativeDestruct()
{
    if (Btn_ResetKeys) Btn_ResetKeys->OnClicked.RemoveDynamic(this, &USettingsKeysPage::OnResetKeysClicked);

    Super::NativeDestruct();
}

void UKeyBindingRowProxy::OnChooseClicked()
{
    if (USettingsKeysPage* Page = Owner.Get())
    {
        if (!EnhancedAction.IsNone())
        {
            Page->RequestListeningForKeyEnhanced(EnhancedAction);
        }
        else
        {
            Page->RequestListeningForKey(Action);
        }
    }
}

void USettingsKeysPage::RequestListeningForKeyEnhanced(FName ActionName)
{
    // 由动作反查它所在行的按钮
    UButton* RowButton = nullptr;
    for (const TPair<UButton*, FName>& Pair : EnhancedRowButtons)
    {
        if (Pair.Value == ActionName)
        {
            RowButton = Pair.Key;
            break;
        }
    }

    // 由动作取该行显示按键的文本
    UTextBlock* RowLabel = nullptr;
    if (UTextBlock* const* Found = EnhancedRowLabels.Find(ActionName))
    {
        RowLabel = *Found;
    }

    StartListeningEnhanced(ActionName, RowButton, RowLabel);
}

void USettingsKeysPage::RequestListeningForKey(EBindableAction Action)
{
    // 由动作反查它所在行的按钮
    UButton* RowButton = nullptr;
    for (const TPair<UButton*, EBindableAction>& Pair : KeyRowButtons)
    {
        if (Pair.Value == Action)
        {
            RowButton = Pair.Key;
            break;
        }
    }

    // 由动作取该行显示按键的文本
    UTextBlock* RowLabel = nullptr;
    if (UTextBlock* const* Found = KeyRowLabels.Find(Action))
    {
        RowLabel = *Found;
    }

    StartListening(Action, RowButton, RowLabel);
}

void USettingsKeysPage::BuildKeyBindingRows()
{
    if (!Box_KeyBindings || !SettingsMgr) return;

    Box_KeyBindings->ClearChildren();
    KeyRowLabels.Reset();
    KeyRowButtons.Reset();
    RowProxies.Reset();

    for (EBindableAction Action : SettingsMgr->GetRemappableActions())
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        // 左侧：动作名
        UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
        NameText->SetText(SettingsMgr->GetActionDisplayName(Action));

        // 右侧：显示当前键的按钮，点击进入改键监听
        UButton* KeyButton = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* KeyLabel = WidgetTree->ConstructWidget<UTextBlock>();
        KeyButton->AddChild(KeyLabel);

        if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
        {
            NameSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 6.f));
            NameSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(KeyButton))
        {
            ButtonSlot->SetVerticalAlignment(VAlign_Center);
        }

        Box_KeyBindings->AddChild(Row);

        KeyRowLabels.Add(Action, KeyLabel);
        KeyRowButtons.Add(KeyButton, Action);

        // 动态多播不能绑 lambda：为每行建一个携带动作上下文的代理 UObject
        UKeyBindingRowProxy* Proxy = NewObject<UKeyBindingRowProxy>(this);
        Proxy->Action = Action;
        Proxy->Owner = this;
        RowProxies.Add(Proxy);
        KeyButton->OnClicked.AddDynamic(Proxy, &UKeyBindingRowProxy::OnChooseClicked);
    }

    // 追加增强输入（IMC_Keys）的行
    BuildEnhancedKeyRows();

    RefreshAllKeyRows();
}

void USettingsKeysPage::BuildEnhancedKeyRows()
{
    if (!Box_KeyBindings) return;

    const AMyPlayerController* PC = GetOwningPlayer<AMyPlayerController>();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SettingsKeysPage] 未找到 MyPlayerController，增强输入键位行不显示"));
        return;
    }

    // 可重绑清单：蓝图配置优先；空则自动收集 IMC 映射 + 默认四 IA（含资产里没有键位映射的）
    TArray<UInputAction*> Actions = PC->GetRebindableEnhancedActions();
    if (Actions.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SettingsKeysPage] 增强输入 Action 清单为空，增强输入键位行不显示"));
        return;
    }

    EnhancedRowLabels.Reset();
    EnhancedRowButtons.Reset();

    for (UInputAction* Action : Actions)
    {
        if (!Action) continue;
        const FName ActionName = Action->GetFName();

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
        const FText DisplayName = Action->ActionDescription;
        NameText->SetText(DisplayName.IsEmpty() ? FText::FromName(ActionName) : DisplayName);

        UButton* KeyButton = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* KeyLabel = WidgetTree->ConstructWidget<UTextBlock>();
        KeyButton->AddChild(KeyLabel);

        if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
        {
            NameSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 6.f));
            NameSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(KeyButton))
        {
            ButtonSlot->SetVerticalAlignment(VAlign_Center);
        }

        Box_KeyBindings->AddChild(Row);

        EnhancedRowLabels.Add(ActionName, KeyLabel);
        EnhancedRowButtons.Add(KeyButton, ActionName);

        UKeyBindingRowProxy* Proxy = NewObject<UKeyBindingRowProxy>(this);
        Proxy->EnhancedAction = ActionName;
        Proxy->Owner = this;
        RowProxies.Add(Proxy);
        KeyButton->OnClicked.AddDynamic(Proxy, &UKeyBindingRowProxy::OnChooseClicked);
    }

    RefreshEnhancedKeyRows();
}

void USettingsKeysPage::RefreshEnhancedKeyRows()
{
    if (!SettingsMgr) return;

    UInputMappingContext* IMC = ResolveEnhancedKeysIMC(this);

    for (const TPair<FName, UTextBlock*>& Pair : EnhancedRowLabels)
    {
        if (UTextBlock* Label = Pair.Value)
        {
            // 设置里绑过自定义键 → 显示自定义键
            const FKey OverrideKey = SettingsMgr->GetEnhancedKey(Pair.Key, EKeys::Invalid);
            if (OverrideKey.IsValid())
            {
                Label->SetText(OverrideKey.GetDisplayName(false));
                continue;
            }

            // 否则显示资产里的默认键；资产也没有 → “未绑定”（可在设置里重绑）
            FKey DefaultKey = EKeys::Invalid;
            if (IMC)
            {
                for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
                {
                    if (M.Action && M.Action->GetFName() == Pair.Key && M.Key.IsValid())
                    {
                        DefaultKey = M.Key;
                        break;
                    }
                }
            }

            if (DefaultKey.IsValid())
            {
                Label->SetText(DefaultKey.GetDisplayName(false));
            }
            else
            {
                Label->SetText(FText::FromString(TEXT("未绑定")));
            }
        }
    }
}

void USettingsKeysPage::RefreshAllKeyRows()
{
    if (!SettingsMgr) return;

    for (const TPair<EBindableAction, UTextBlock*>& Pair : KeyRowLabels)
    {
        if (UTextBlock* Label = Pair.Value)
        {
            const FKey Key = SettingsMgr->GetBoundKey(Pair.Key);
            Label->SetText(Key.GetDisplayName(false));
        }
    }

    RefreshEnhancedKeyRows();
}

void USettingsKeysPage::StartListeningEnhanced(FName ActionName, UButton* RowButton, UTextBlock* RowLabel)
{
    // 若此前正在监听另一个动作（legacy 或增强），先把它还原
    if (bListeningForKey || bListeningEnhanced)
    {
        CancelListening();
    }

    bListeningEnhanced = true;
    ListeningEnhancedAction = ActionName;
    ListeningButton = RowButton;
    ListeningLabel = RowLabel;

    if (RowLabel) RowLabel->SetText(FText::FromString(TEXT("按新键…(Esc取消)")));
    if (RowButton) RowButton->SetKeyboardFocus();
}

void USettingsKeysPage::StartListening(EBindableAction Action, UButton* RowButton, UTextBlock* RowLabel)
{
    // 若此前正在监听另一个动作（legacy 或增强），先把它还原
    if (bListeningForKey || bListeningEnhanced)
    {
        CancelListening();
    }

    bListeningForKey = true;
    ListeningAction = Action;
    ListeningButton = RowButton;
    ListeningLabel = RowLabel;

    if (RowLabel) RowLabel->SetText(FText::FromString(TEXT("按新键…(Esc取消)")));
    if (RowButton) RowButton->SetKeyboardFocus();
}

void USettingsKeysPage::CancelListening()
{
    bListeningForKey = false;
    bListeningEnhanced = false;
    ListeningEnhancedAction = NAME_None;
    ListeningButton = nullptr;
    ListeningLabel = nullptr;
    RefreshAllKeyRows();
}

void USettingsKeysPage::CommitListeningKey(FKey NewKey)
{
    const bool bWasEnhanced = bListeningEnhanced;
    const FName EnhancedActionName = ListeningEnhancedAction;
    const EBindableAction LegacyAction = ListeningAction;

    bListeningForKey = false;
    bListeningEnhanced = false;
    ListeningEnhancedAction = NAME_None;
    ListeningButton = nullptr;
    ListeningLabel = nullptr;

    if (bWasEnhanced)
    {
        // 增强输入行：写存档 + 广播（PC 重绑 IMC）
        if (SettingsMgr && NewKey.IsValid() && !EnhancedActionName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[KeysPage] 增强键改绑: %s -> %s"), *EnhancedActionName.ToString(), *NewKey.ToString());
            SettingsMgr->SetEnhancedKey(EnhancedActionName, NewKey);
        }
    }
    else if (SettingsMgr && NewKey.IsValid())
    {
        // legacy 行
        UE_LOG(LogTemp, Warning, TEXT("[KeysPage] 键改绑: %d -> %s"), static_cast<uint8>(LegacyAction), *NewKey.ToString());
        SettingsMgr->SetBoundKey(LegacyAction, NewKey);
    }
    RefreshAllKeyRows();
}

void USettingsKeysPage::OnResetKeysClicked()
{
    if (!SettingsMgr) return;

    bListeningForKey = false;
    bListeningEnhanced = false;
    ListeningEnhancedAction = NAME_None;

    SettingsMgr->ResetAllKeys();        // legacy 动作恢复默认（广播后 UI 管理器重绑）
    SettingsMgr->ResetAllEnhancedKeys(); // 增强输入 Action 恢复默认（广播后 PC 重绑 IMC）
    RefreshAllKeyRows();
}

void USettingsKeysPage::ForceCancelListening()
{
    if (bListeningForKey || bListeningEnhanced)
    {
        bListeningForKey = false;
        bListeningEnhanced = false;
        ListeningEnhancedAction = NAME_None;
        ListeningButton = nullptr;
        ListeningLabel = nullptr;
        RefreshAllKeyRows();
    }
}

FReply USettingsKeysPage::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // legacy 与增强输入行的监听都占用键盘；残留状态会吞掉所有按键（含 Esc）
    if (bListeningForKey || bListeningEnhanced)
    {
        const FKey PressedKey = InKeyEvent.GetKey();

        // 单独按 Ctrl/Shift/Alt 不提交，等待主键
        if (PressedKey.IsModifierKey())
        {
            return FReply::Handled();
        }

        if (PressedKey == EKeys::Escape)
        {
            CancelListening();
            // 不 Handled：让 Esc 继续传到上层（PC 的关闭面板绑定），一次按 Esc 即退出监听并关闭设置
            return FReply::Unhandled();
        }

        CommitListeningKey(PressedKey);
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USettingsKeysPage::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 监听模式下也支持绑定鼠标按键（LMB/RMB/侧键等）
    if (bListeningForKey || bListeningEnhanced)
    {
        const FKey PressedMouseKey = InMouseEvent.GetEffectingButton();
        // 滚轮/移动不产生按钮，忽略；修饰键（如 Shift 配合点击）不单独提交
        if (PressedMouseKey.IsValid() && PressedMouseKey.IsMouseButton() && !PressedMouseKey.IsModifierKey())
        {
            CommitListeningKey(PressedMouseKey);
            return FReply::Handled();
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
