#include "InventorySlotEntryWidget.h"
#include "InventoryEntryObject.h"
#include "InventoryComponent.h"

#include "GameFramework/PlayerController.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

FLinearColor UInventorySlotEntryWidget::GetRarityColor(EItemRarity Rarity)
{
    switch (Rarity)
    {
    case EItemRarity::Common:
        return FLinearColor(0.65f, 0.65f, 0.68f, 1.0f);
    case EItemRarity::Rare:
        return FLinearColor(0.25f, 0.50f, 1.00f, 1.0f);
    case EItemRarity::Epic:
        return FLinearColor(0.65f, 0.25f, 1.00f, 1.0f);
    case EItemRarity::Legendary:
        return FLinearColor(1.00f, 0.78f, 0.15f, 1.0f);
    default:
        return FLinearColor::White;
    }
}

void UInventorySlotEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    SetEntryData(Cast<UInventoryEntryObject>(ListItemObject));
}

void UInventorySlotEntryWidget::SetEntryData(UInventoryEntryObject* Entry)
{
    CurrentEntry = Entry;
    if (Entry)
    {
        RefreshFromSlot(Entry->SlotData);
    }
}

FReply UInventorySlotEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!CurrentEntry)
    {
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    UInventoryComponent* Inventory = GetOwningPlayer()
        ? GetOwningPlayer()->FindComponentByClass<UInventoryComponent>()
        : nullptr;
    if (!Inventory)
    {
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 单击 = 拾取 / 取消 / 交换（替换）
        Inventory->TogglePick(CurrentEntry->SlotIndex);
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (InMouseEvent.IsShiftDown())
        {
            // Shift+右键 = 拆出一半到新格（分堆，数量>1 才有效）
            Inventory->SplitHalf(CurrentEntry->SlotIndex);
        }
        else
        {
            // 右键 = 使用物品（消耗 1 个并触发该物品的 Buff）
            if (Inventory->PickedSlotIndex >= 0)
            {
                Inventory->PickedSlotIndex = -1;
                Inventory->OnInventoryChanged.Broadcast();
            }
            Inventory->UseItem(CurrentEntry->SlotIndex);
        }
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotEntryWidget::SetPicked(bool bPicked)
{
    if (!Border_Rarity)
    {
        return;
    }
    if (bPicked)
    {
        Border_Rarity->SetBrushColor(FLinearColor(1.00f, 0.85f, 0.20f, 1.0f));
        return;
    }
    // 取消拾取：恢复该物品的稀有度边框色
    EItemRarity Rarity = EItemRarity::Common;
    if (CurrentEntry)
    {
        if (const UItemCatalog* Catalog = UItemCatalog::Resolve(ItemCatalog))
        {
            if (const FItemData* Item = Catalog->FindItem(CurrentEntry->SlotData.ItemID))
            {
                Rarity = Item->Rarity;
            }
        }
    }
    Border_Rarity->SetBrushColor(GetRarityColor(Rarity));
}

void UInventorySlotEntryWidget::RefreshFromSlot(const FItemSlot& ItemSlot)
{
    if (ItemSlot.IsEmpty())
    {
        return;
    }

    // 目录查询：按 ItemID 取物品定义（优先用配置的目录，否则加载默认目录）
    const UItemCatalog* Catalog = UItemCatalog::Resolve(ItemCatalog);
    const FItemData* Item = Catalog ? Catalog->FindItem(ItemSlot.ItemID) : nullptr;
    if (!Item)
    {
        return;
    }

    if (Image_Icon)
    {
        if (Item->Icon)
        {
            Image_Icon->SetBrushFromTexture(Item->Icon);
        }
    }

    if (Text_Count)
    {
        if (ItemSlot.Count > 1)
        {
            Text_Count->SetVisibility(ESlateVisibility::Visible);
            Text_Count->SetText(FText::AsNumber(ItemSlot.Count));
        }
        else
        {
            Text_Count->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Border_Rarity)
    {
        Border_Rarity->SetBrushColor(GetRarityColor(Item->Rarity));
    }

    // 拾取高亮：若背包组件当前拾取的就是本格，显示金色边框
    const UInventoryComponent* Inventory = GetOwningPlayer()
        ? GetOwningPlayer()->FindComponentByClass<UInventoryComponent>()
        : nullptr;
    SetPicked(Inventory && Inventory->PickedSlotIndex == CurrentEntry->SlotIndex);
}
