#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "EquipmentTypes.h"
#include "ItemCatalog.generated.h"

class USkeletalMesh;
class UStaticMesh;

/** 物品稀有度（同时决定背包格子的边框配色） */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common    UMETA(DisplayName = "普通"),
    Rare      UMETA(DisplayName = "稀有"),
    Epic      UMETA(DisplayName = "史诗"),
    Legendary UMETA(DisplayName = "传说"),
};

/** 物品大类 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Misc       UMETA(DisplayName = "杂物"),
    Material   UMETA(DisplayName = "材料"),
    Consumable UMETA(DisplayName = "消耗品"),
    Equipment  UMETA(DisplayName = "装备"),
    Quest      UMETA(DisplayName = "任务道具"),
};

/**
 * 物品定义（目录中的一行，轻量值类型）。
 * 不再是"一个物品一个资产文件"，而是集中放在 UItemCatalog::AllItems 数组里，
 * 加物品 = 在目录里加一行。背包槽/卡池只存 FName ItemID，运行时查目录取定义。
 */
USTRUCT(BlueprintType)
struct FItemData
{
    GENERATED_BODY()

    // 唯一标识（如 "Coin"、"Potion_S"），背包槽 / 卡池条目用它引用本行
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemType ItemType = EItemType::Misc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemRarity Rarity = EItemRarity::Common;

    // 单格最大堆叠数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
    int32 MaxStack = 99;

    // 使用后触发的 Buff（为空 = 不可使用）。关联 /Game/UserInterface/DA_Buffs 中的 BuffID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Buff")
    FName BuffID;

    // ===== 装备专属字段（ItemType == Equipment 时生效）=====
    // 装备槽位（None = 不可装备）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment")
    EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

    // 五项属性加成：穿戴时加到宿主 UAttributeComponent / 移动速度上，卸下时扣除
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (ClampMin = "0"))
    float AttackBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (ClampMin = "0"))
    float DefenseBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (ClampMin = "0"))
    float MaxHPBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (ClampMin = "0"))
    float MaxMPBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (ClampMin = "0"))
    float MoveSpeedBonus = 0.0f;

    // 武器 Actor 类（装备时 Spawn 这个 Actor，Attach 到角色 Socket）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment")
    TSubclassOf<AActor> WeaponActorClass;

    // 挂点名称（默认 WeaponSocket；蓝图可改）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment")
    FName AttachSocketName = NAME_None;
};

/** 卡池中的一条掉落项：物品 ID + 权重 */
USTRUCT(BlueprintType)
struct FGachaPoolEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha")
    FName ItemID;

    // 相对权重，越大越容易出（同卡池内按比例分配概率）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha", meta = (ClampMin = "1"))
    int32 Weight = 1;
};

/** 一个卡池的完整配置（目录中的一行） */
USTRUCT(BlueprintType)
struct FGachaPoolConfig
{
    GENERATED_BODY()

    // 卡池标识（如 "DefaultPool"），GachaComponent 用它选择当前池
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha")
    FName PoolID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha")
    FText PoolName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha")
    TArray<FGachaPoolEntry> Entries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha", meta = (ClampMin = "0"))
    int32 CostSingle = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha", meta = (ClampMin = "0"))
    int32 CostTen = 900;

    // 保底：连续多少抽没出“史诗及以上”，下一抽必出史诗及以上；0 = 关闭保底
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gacha", meta = (ClampMin = "0"))
    int32 PityThreshold = 10;
};

/**
 * 集中式物品/卡池目录（一个资产管理所有物品与卡池）。
 * 内容浏览器创建方式：右键 → Miscellaneous → Data Asset → 选 ItemCatalog，
 * 推荐资产路径 /Game/UserInterface/DA_Catalog（代码按此路径兜底加载）。
 *
 * 职责：
 *  - AllItems：全部物品定义（加物品 = 数组加一行）；
 *  - Pools：全部卡池配置（加卡池 = 数组加一行）；
 *  - 查询：按 ItemID 找物品 / 按 PoolID 找卡池；
 *  - 抽卡：按权重随机掉落（含史诗保底过滤）。
 */
UCLASS(BlueprintType)
class MYPROJECT_API UItemCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
    TArray<FItemData> AllItems;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
    TArray<FGachaPoolConfig> Pools;

    // ===== 查询 =====

    // 按 ID 找物品定义（C++ 用；找不到返回 nullptr）
    const FItemData* FindItem(FName ItemID) const;

    // 按 ID 找卡池（C++ 用；找不到返回 nullptr）
    const FGachaPoolConfig* FindPool(FName PoolID) const;

    // 以下供蓝图直接取显示信息（找不到返回空/默认；文本优先走 String Table，表里没有回退内联文本）
    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDisplayName(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDescription(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    UTexture2D* GetIcon(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    int32 GetMaxStack(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    bool HasItem(FName ItemID) const;

    // ===== 抽卡 =====

    /**
     * 按权重随机一个掉落物品的 ID。
     * @param Pool           卡池配置
     * @param MinRarity      最低稀有度（保底时传 Epic）
     * @param bUseMinRarity  是否只在 MinRarity 及以上的物品中随机
     * @return 随机到的物品 ID；卡池为空（或没有满足稀有度的物品）时返回 NAME_None
     */
    FName RollItem(const FGachaPoolConfig& Pool, EItemRarity MinRarity, bool bUseMinRarity) const;

    // ===== 默认目录 =====

    // 优先返回传入的目录，否则加载约定路径的默认目录 /Game/UserInterface/DA_Catalog
    static UItemCatalog* Resolve(UItemCatalog* Preferred);

    // 加载约定路径的默认目录（static 缓存）
    static UItemCatalog* GetDefaultCatalog();

private:
    // 本地化文本：从 Config/Localization/ST_Data.csv 的内存字符串表取 Key，未命中回退 Fallback（内联文本）
    FText GetLocalizedText(FName ItemID, const FText& Fallback, const TCHAR* Field) const;
};
