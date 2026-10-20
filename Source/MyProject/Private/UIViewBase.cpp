// UIViewBase.cpp
// 所有 UI 面板基类的 C++ 实现：
// 1. 构造默认 bIsFocusable = true（面板打开后能接收键盘）
// 2. NativeOnKeyDown 兜底：面板打开时，按设置里实际绑定的键（开/关背包/地图/抽卡/Esc）
//    直接转发给 UIManager，不依赖输入组件焦点 —— 解决“面板打开后按键关不掉”
//    设置页豁免（PanelType == Settings），KeysPage 自己处理监听与穿透。

#include "UIViewBase.h"
#include "UIManagerComponent.h"
#include "USettingsSubsystem.h"

UUIViewBase::UUIViewBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 面板默认可聚焦：打开后 SetKeyboardFocus 让按键事件进入 UI 焦点链，
    // NativeOnKeyDown 兜底才能收到并转发给 UIManager。
    SetIsFocusable(true);
}

FReply UUIViewBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 设置页：KeysPage 处理监听态改键、Esc 取消监听后穿透，这里不做兜底
    if (PanelType == EUIPanelType::Settings)
    {
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }

    UUIManagerComponent* UI = OwnerUIManager;
    if (!UI)
    {
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }

    // 取设置里实际绑定的键（玩家改过用自定义键，没改过用默认键），保证与改键联动
    USettingsSubsystem* Settings = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UGameInstance* GI = PC->GetGameInstance())
        {
            Settings = GI->GetSubsystem<USettingsSubsystem>();
        }
    }

    const FKey K = InKeyEvent.GetKey();
    const FKey CloseKey  = Settings ? Settings->GetBoundKey(EBindableAction::ClosePanel)      : EKeys::Escape;
    const FKey InvKey    = Settings ? Settings->GetBoundKey(EBindableAction::ToggleInventory) : EKeys::I;
    const FKey MapKey    = Settings ? Settings->GetBoundKey(EBindableAction::ToggleMap)       : EKeys::M;
    const FKey GachaKey  = Settings ? Settings->GetBoundKey(EBindableAction::ToggleGacha)     : EKeys::G;
    const FKey EquipKey  = Settings ? Settings->GetBoundKey(EBindableAction::ToggleEquipment) : EKeys::E;

    if (K == CloseKey)
    {
        UI->CloseTopmost();
        return FReply::Handled();
    }
    if (K == InvKey)
    {
        UI->TogglePanel(EUIPanelType::Inventory);
        return FReply::Handled();
    }
    if (K == MapKey)
    {
        UI->TogglePanel(EUIPanelType::Map);
        return FReply::Handled();
    }
    if (K == GachaKey)
    {
        UI->TogglePanel(EUIPanelType::Gacha);
        return FReply::Handled();
    }
    if (K == EquipKey)
    {
        UI->TogglePanel(EUIPanelType::Equipment);
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
