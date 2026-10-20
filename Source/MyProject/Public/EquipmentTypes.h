#pragma once

#include "CoreMinimal.h"
#include "EquipmentTypes.generated.h"

/**
 * 装备槽位类型（五槽方案）。
 * 独立头文件存放，避免 ItemCatalog（目录）与 EquipmentComponent（组件）互相包含。
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    Weapon    UMETA(DisplayName = "武器"),
    Helmet    UMETA(DisplayName = "头盔"),
    Armor     UMETA(DisplayName = "护甲"),
    Boots     UMETA(DisplayName = "鞋子"),
    Accessory UMETA(DisplayName = "饰品"),
    None      UMETA(DisplayName = "无"),
};
