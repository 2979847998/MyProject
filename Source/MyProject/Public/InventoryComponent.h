#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "ItemSlot.h"
#include "InventoryComponent.generated.h"

class UInventoryData;

// 背包内容变化（增删物品）时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// 金币变化时广播，携带新金币数
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinsChanged, int32, NewCoins);

/**
 * 背包组件。挂在 PlayerController 上（和 UIManagerComponent 同一宿主，UI 好找）。
 *
 * 数据结构与物品/金币逻辑已抽到独立数据类 UInventoryData（详见 InventoryData.h）：
 * 组件持有其实例并转发全部接口，负责生命周期与存档读写（槽位 InventorySlot）。
 * 物品以 FName ItemID 引用集中目录（UItemCatalog），金币即 ItemID == "Coin" 的格子。
 *
 * 所有对外接口签名保持不变（仅参数从资产指针改为 FName ItemID），调用方按新签名适配。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void BeginPlay() override;

    // 背包数据实例（独立类）：Slots / 堆叠 / 金币逻辑都在其中。
    // 可在组件细节面板展开配置（MaxSlots、Catalog 等）。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Inventory|Data")
    TObjectPtr<UInventoryData> InventoryData;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnCoinsChanged OnCoinsChanged;

    // ===== 物品（按目录中的 ItemID） =====

    // 加入物品（自动堆叠），全部放下返回 true，背包满返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(FName ItemID, int32 Count = 1);

    // 批量加入（抽卡用）：内部只刷新/存盘一次；返回未能放入的数量（0 = 全部成功）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 AddItems(const TArray<FName>& ItemIDs);

    // 移除物品，数量不足时不动并返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName ItemID, int32 Count = 1);

    // 清空指定槽位（整格删除）。金币槽不可删除；删除后自动取消/校正拾取状态并存盘
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetUsedSlots() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ClearInventory();

    // 交换两个槽位（拾取替换），成功自动存盘
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SwapSlots(int32 IndexA, int32 IndexB);

    // 从 Index 拆出 Count 到新槽（手动分堆），成功自动存盘
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SplitStack(int32 Index, int32 Count);

    // ===== 拾取/替换（背包格子交互） =====

    // 当前拾取中的槽下标，-1 = 没在拾取（格子高亮/替换逻辑用）
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory")
    int32 PickedSlotIndex = -1;

    // 左键格子：无拾取→拾取；点同格→取消；点他格→交换（替换）。操作后广播刷新
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void TogglePick(int32 SlotIndex);

    // 右键格子：拆出一半到新格（分堆，数量>1 才有效）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SplitHalf(int32 SlotIndex);

    // 使用格子里的物品：消耗 1 个并触发该物品配置的 Buff（无 BuffID 的物品不可用）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItem(int32 SlotIndex);

    // 背包排序（规则见 UInventoryData::SortInventory）：排序后自动存盘
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortInventory();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FItemSlot>& GetSlots() const;

    // ===== 金币 =====

    UFUNCTION(BlueprintCallable, Category = "Inventory|Coin")
    void AddCoins(int32 Amount);

    // 金币足够才扣除并返回 true，否则不动返回 false
    UFUNCTION(BlueprintCallable, Category = "Inventory|Coin")
    bool SpendCoins(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Inventory|Coin")
    int32 GetCoins() const;

    // ===== 存档 =====

    UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
    void SaveInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
    void LoadInventory();

private:
    static const FString SaveSlotName;

    // 返回数据实例（正常情况下由构造函数创建，此处仅做防御性兜底）
    UInventoryData* GetData() const;

    // 转发数据类的广播到组件的同名委派（保持调用方兼容）
    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void HandleCoinsChanged(int32 NewCoins);
};
