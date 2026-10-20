#pragma once

#include "CoreMinimal.h"
#include "ItemSlot.generated.h"

/**
 * 背包里的一个格子（一堆同种物品）。
 * 是轻量值类型（USTRUCT，不是 UObject）：由背包组件持有、可直接序列化进存档。
 * 只存 FName ItemID（指向 UItemCatalog 中的物品定义行），不直接持有资产指针，
 * 这样加物品/改物品都只需编辑目录，存档也更轻量。
 */
USTRUCT(BlueprintType)
struct FItemSlot
{
    GENERATED_BODY()

    // 物品定义 ID（在 UItemCatalog::AllItems 中的 ItemID）；ItemID == "Coin" 的格子即金币槽
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName ItemID;

    // 当前堆叠数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Count = 0;

    bool IsEmpty() const
    {
        return ItemID.IsNone() || Count <= 0;
    }
};
