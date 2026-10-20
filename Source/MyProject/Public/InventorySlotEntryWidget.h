#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ItemCatalog.h"
#include "InventorySlotEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;
class UInventoryEntryObject;
struct FItemSlot;

/**
 * 单个物品格子控件。
 *   - 作为 TileView 的 Entry：实现 IUserObjectListEntry，系统塞入条目时自动刷新；
 *   - 也可被抽卡结果面板用 SetEntryData 手动填充（复用同一套外观）。
 *   - 鼠标交互：左键=拾取/取消/交换替换，右键=拆一半分堆（直接操作背包组件）。
 * 蓝图 WBP 继承本类，控件名（均可缺省）：Image_Icon、Text_Count、Border_Rarity。
 */
UCLASS()
class MYPROJECT_API UInventorySlotEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    // 手动填充（抽卡结果 WrapBox 用）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetEntryData(UInventoryEntryObject* Entry);

    // 稀有度 -> 边框配色（普通灰 / 稀有蓝 / 史诗紫 / 传说金）
    UFUNCTION(BlueprintPure, Category = "Inventory")
    static FLinearColor GetRarityColor(EItemRarity Rarity);

    // 物品目录（显示用）。未配置时自动加载 /Game/UserInterface/DA_Catalog
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UItemCatalog> ItemCatalog;

    // 拾取状态高亮（金色边框）；取消拾取后由刷新恢复稀有度色。
    // 刷新时自动读取背包组件的 PickedSlotIndex 判断是否高亮
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetPicked(bool bPicked);

protected:
    // IUserObjectListEntry：TileView 给本 Entry 绑定数据时调用
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    // 鼠标左/右键点击
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Icon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Count;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UBorder> Border_Rarity;

    // 持有当前条目数据，避免被 GC
    UPROPERTY(Transient)
    TObjectPtr<UInventoryEntryObject> CurrentEntry;

private:
    void RefreshFromSlot(const FItemSlot& ItemSlot);
};
