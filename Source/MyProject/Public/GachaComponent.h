#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "ItemCatalog.h"
#include "GachaComponent.generated.h"

class UInventoryComponent;

// 抽卡完成时广播，携带本次抽到的全部物品 ID（单抽长度 1，十连长度 10）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGachaDrawn, TArray<FName>, DrawnItemIDs);

/**
 * 抽卡组件。挂在 PlayerController 上（与背包组件同宿主）。
 * 负责：从集中目录 UItemCatalog 读取卡池配置、扣金币、按权重随机（含史诗保底）、
 *       把产物写入背包并广播结果。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UGachaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGachaComponent();

    // 物品/卡池目录（所有卡池集中在这里）。未配置时自动加载 /Game/UserInterface/DA_Catalog
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gacha")
    TObjectPtr<UItemCatalog> Catalog;

    // 默认卡池 ID（在目录的 Pools 数组里选一个；留空则用第一个卡池）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gacha")
    FName DefaultPoolID;

    UPROPERTY(BlueprintAssignable, Category = "Gacha")
    FOnGachaDrawn OnGachaDrawn;

    // 运行时切换卡池（不传则一直用默认池）
    UFUNCTION(BlueprintCallable, Category = "Gacha")
    void SetPool(FName PoolID) { CurrentPoolID = PoolID; }

    UFUNCTION(BlueprintCallable, Category = "Gacha")
    TArray<FName> DrawSingle();

    UFUNCTION(BlueprintCallable, Category = "Gacha")
    TArray<FName> DrawTen();

    UFUNCTION(BlueprintPure, Category = "Gacha")
    int32 GetSingleCost() const;

    UFUNCTION(BlueprintPure, Category = "Gacha")
    int32 GetTenCost() const;

protected:
    // 当前生效卡池 ID；为空时回退 DefaultPoolID，再为空时用目录第一个池
    UPROPERTY(Transient)
    FName CurrentPoolID;

    // 距离上次“史诗及以上”已经抽了多少次（保底计数）
    int32 DrawsSinceRare = 0;

    UInventoryComponent* GetInventory() const;
    const FGachaPoolConfig* GetActivePool() const;

    // Count=1 单抽，Count=10 十连（十连按卡池 CostTen 一次性扣费）
    TArray<FName> PerformDraw(int32 Count);
};
