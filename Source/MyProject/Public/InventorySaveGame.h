#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ItemSlot.h"
#include "InventorySaveGame.generated.h"

/**
 * 背包存档对象：持久化玩家拥有的物品。
 * 槽位名 InventorySlot，由 UInventoryComponent 在开局加载、变更后保存。
 *
 * 新格式：金币也是背包中的一个物品，存在 SavedSlots 里（ItemData 指向金币资产）。
 * SavedCoins 仅为旧版本（金币是独立字段）的存档迁移保留：加载时并入金币物品槽，
 * 保存时始终写 0。
 */
UCLASS()
class MYPROJECT_API UInventorySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TArray<FItemSlot> SavedSlots;

    // 仅用于旧存档迁移（老版本金币独立字段）；新存档保存时恒为 0
    UPROPERTY(BlueprintReadWrite)
    int32 SavedCoins = 0;
};
