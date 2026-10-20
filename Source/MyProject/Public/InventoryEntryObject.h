#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemSlot.h"
#include "InventoryEntryObject.generated.h"

/**
 * TileView/ListView 的条目数据必须是 UObject。
 * 本类就是对一个 FItemSlot 的 UObject 包装：背包刷新时为每个格子创建一个，喂给 TileView。
 */
UCLASS(BlueprintType)
class MYPROJECT_API UInventoryEntryObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FItemSlot SlotData;

    // 该条目在 Slots 数组中的下标（拾取/替换/分堆用）
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    int32 SlotIndex = -1;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Init(FName InItemID, int32 InCount, int32 InSlotIndex = -1)
    {
        SlotData.ItemID = InItemID;
        SlotData.Count = InCount;
        SlotIndex = InSlotIndex;
    }
};
