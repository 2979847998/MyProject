#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "ItemCatalog.h"
#include "EquipmentTypes.h"
#include "EquipmentComponent.generated.h"

class UAttributeComponent;
class UInventoryComponent;
class UItemCatalog;
class UCharacterMovementComponent;
class ACharacter;

// 装备变化（穿戴/卸下/读档应用）时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

/**
 * 装备存档对象：持久化"槽位 → 物品 ID"映射。
 * 独立槽位名（EquipmentSlot），与背包存档互不影响。
 */
UCLASS()
class MYPROJECT_API UEquipmentSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TMap<EEquipmentSlot, FName> SavedEquipment;
};

/**
 * 装备组件：五槽（武器/头盔/护甲/鞋子/饰品）穿戴与属性加成。
 *
 * 与背包/属性系统的关系：
 *  - 挂在 PlayerController 上（与 UInventoryComponent 同宿主，UI 好找）；敌人若需装备也可挂到 Character
 *  - 物品定义来自 UItemCatalog（ItemType == Equipment 的行才可装备）
 *  - 穿戴 = 从背包移除 1 个 → 写入槽位 → 属性加成（加法，走 AttributeComponent::Modify*）
 *  - 移动速度加成：直接写入 CharacterMovement::MaxWalkSpeed（基础速 + 装备加成；
 *    与 Buff 系统同样写 MaxWalkSpeed，为已知交互点——二者同时生效时最后写入者覆盖，如需精确叠加再统一入口）
 *  - 武器显示：把 AttachMesh 挂到角色 Mesh 的 AttachSocketName（默认 WeaponSocket）
 *  - 存档独立槽位名 "EquipmentSlot"，Load 时若 Pawn 未就绪会延迟重试应用加成
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEquipmentComponent();

    virtual void BeginPlay() override;

    UPROPERTY(BlueprintAssignable, Category = "Equipment")
    FOnEquipmentChanged OnEquipmentChanged;

    // 槽位 → 装备物品 ID（None = 空槽）。SaveGame 标记：随存档序列化
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Equipment")
    TMap<EEquipmentSlot, FName> EquippedItems;

    // 基础移速（0 = 首次应用时自动从 CharacterMovement 记录；>0 = 蓝图强制指定）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0"))
    float BaseMoveSpeed = 0.0f;

    // ===== 查询 =====

    UFUNCTION(BlueprintPure, Category = "Equipment")
    FName GetEquippedItem(EEquipmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsSlotEquipped(EEquipmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    const TMap<EEquipmentSlot, FName>& GetAllEquipped() const;

    // 获取当前 Spawn 的武器 Actor（攻击命中检测用，返回 null = 未装备武器）
    UFUNCTION(BlueprintPure, Category = "Equipment")
    AActor* GetAttachedWeapon() const { return AttachedWeapon; }

    // 快捷：获取武器槽装备的物品 ID（None = 空）
    UFUNCTION(BlueprintPure, Category = "Equipment")
    FName GetEquippedWeaponItem() const;

    // 快捷：获取武器槽是否已装备
    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsWeaponEquipped() const;

    // 五项加成汇总（UI 属性面板 / 战斗计算可直接读取）
    UFUNCTION(BlueprintPure, Category = "Equipment")
    float GetTotalAttackBonus() const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    float GetTotalDefenseBonus() const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    float GetTotalMaxHPBonus() const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    float GetTotalMaxMPBonus() const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    float GetTotalMoveSpeedBonus() const;

    // ===== 操作 =====

    // 从背包取 1 个物品穿上。目标槽已有装备时先自动换下回背包；背包满或非法物品返回 false
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipItem(FName ItemID);

    // 卸下指定槽位装备（回背包）。背包满返回 false
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipItem(EEquipmentSlot Slot);

    // ===== 存档 =====

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void SaveEquipment();

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void LoadEquipment();

private:
    static const FString SaveSlotName;

    // 宿主角色（PC 宿主取 Pawn；Pawn 宿主取自身），取不到返回 nullptr
    ACharacter* GetCharacter() const;

    UAttributeComponent* GetAttribute() const;
    UInventoryComponent* GetInventory() const;
    UItemCatalog* GetCatalog() const;
    const FItemData* FindItem(FName ItemID) const;

    // 整体应用/撤销当前全部装备的属性加成（读档后调用）
    void ApplyAllBonuses();
    // 应用/撤销单件装备的属性加成（Sign=+1 穿 / -1 卸）
    void ApplyItemBonuses(const FItemData* Item, float Sign);
    // 重算并写入移动速度（基础速 + 全部装备移速加成）
    void ApplyMoveSpeedBonuses();
    // 挂载（bAttach=true）或卸载（false）指定装备的武器网格
    void ApplyAttachMesh(const FItemData* Item, bool bAttach);
    // 销毁当前挂件（换装/卸下/读档时先清）
    void ClearAttachedWeapon();

    // Pawn 未就绪时的延迟应用（PC 宿主在 BeginPlay 时可能尚未 Possess）
    void RetryApplyPending();

    // 动态 Spawn 的武器 Actor（持有防 GC）
    UPROPERTY(Transient)
    TObjectPtr<AActor> AttachedWeapon;

    // 读档后是否已把加成应用到宿主（Pawn 就绪前保持 false）
    bool bAppliedPending = false;

    int32 PendingRetryCount = 0;
    static constexpr int32 MaxPendingRetryCount = 50;
};
