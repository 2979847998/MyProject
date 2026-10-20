#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemCatalog.h"
#include "ItemSlot.h"
#include "InventoryData.generated.h"

// 背包内容变化（增删物品）时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryDataChanged);

// 金币变化时广播，携带新金币数
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinsDataChanged, int32, NewCoins);

/**
 * 背包数据类：把“物品堆数组”封装成独立可复用的 UObject。
 *
 * 物品以 FName ItemID 引用集中目录 UItemCatalog（一个资产管理所有物品定义），
 * 格子数组 TArray<FItemSlot> Slots 只存 ID + 数量；金币即 ItemID == "Coin" 的格子。
 *
 * 职责：
 *  - 持有格子数组与堆叠/占位/移除/查询/清空逻辑；
 *  - 金币物品化接口 AddCoins / SpendCoins / GetCoins（货币槽不按 MaxStack 拆堆）；
 *  - 内容变化时广播 OnInventoryChanged / OnCoinsChanged。
 *
 * 不负责：存档读写（由 UInventoryComponent 负责，经 SetSlots / GetSlots 搬运数据）。
 * 任何 Actor 都可以 NewObject 一个独立使用（如 NPC / 商店 / 箱子），或由组件持有。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class MYPROJECT_API UInventoryData : public UObject
{
    GENERATED_BODY()

public:
    UInventoryData();

    // 最多容纳多少“堆”（一种物品堆满后另起一堆，每堆最多占一格）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
    int32 MaxSlots = 60;

    // 物品目录（所有物品定义集中在这里）。未配置时自动加载 /Game/UserInterface/DA_Catalog
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Catalog", meta = (ToolTip = "集中物品目录（ItemCatalog）。未设置时自动尝试加载 /Game/UserInterface/DA_Catalog"))
    TObjectPtr<UItemCatalog> Catalog;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryDataChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnCoinsDataChanged OnCoinsChanged;

    // ===== 物品（按目录中的 ItemID 操作） =====

    // 加入物品（自动堆叠），全部放下返回 true，背包满/目录无此物品返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(FName ItemID, int32 Count = 1);

    // 批量加入（抽卡用）：内部只广播一次；返回未能放入的数量（0 = 全部成功）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 AddItems(const TArray<FName>& ItemIDs);

    // 移除物品，数量不足时不动并返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName ItemID, int32 Count = 1);

    // 清空指定槽位（整格删除，玩家手动删除用）。金币槽不可删除；越界/空槽返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetUsedSlots() const { return Slots.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    // 交换两个槽位（整理背包/拾取替换）。两个下标都有效且不同才执行
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SwapSlots(int32 IndexA, int32 IndexB);

    // 从 Index 槽拆出 Count 个到新槽（手动分堆）。要求 Count>0 且小于原堆数量、背包未满
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SplitStack(int32 Index, int32 Count);

    // 背包排序：金币槽固定排最后；其余按稀有度（传说→史诗→稀有→普通）降序，
    // 同稀有度按显示名升序，仍相同再按 ItemID 字典序。
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortInventory();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FItemSlot>& GetSlots() const { return Slots; }

    // ===== 金币（ItemID == "Coin" 的背包物品） =====

    UFUNCTION(BlueprintCallable, Category = "Inventory|Coin")
    void AddCoins(int32 Amount);

    // 金币足够才扣除并返回 true，否则不动返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory|Coin")
    bool SpendCoins(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Inventory|Coin")
    int32 GetCoins() const;

    // ===== 存档桥接（供 UInventoryComponent 搬运数据，不触发广播） =====

    void SetSlots(const TArray<FItemSlot>& InSlots) { Slots = InSlots; }

protected:
    // 当前物品堆（只存有物品的格子；金币也在其中，ItemID == "Coin"）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FItemSlot> Slots;

private:
    // 金币物品在目录中的 ItemID
    static const FName CoinItemID;

    // 真正的堆叠/占位逻辑；OutRemaining 返回没放下的数量
    bool AddItemInternal(FName ItemID, int32 Count, int32& OutRemaining);

    // 查目录拿物品定义；目录未配置时兜底加载默认目录
    const FItemData* GetItemData(FName ItemID) const;
};
