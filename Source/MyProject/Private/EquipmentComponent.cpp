#include "EquipmentComponent.h"

#include "AttributeComponent.h"
#include "InventoryComponent.h"
#include "ItemCatalog.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

const FString UEquipmentComponent::SaveSlotName = TEXT("EquipmentSlot");

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    LoadEquipment();
}

// ========== 宿主解析 ==========

ACharacter* UEquipmentComponent::GetCharacter() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    // PC 宿主 → 取当前 Pawn
    if (APlayerController* PC = Cast<APlayerController>(OwnerActor))
    {
        return Cast<ACharacter>(PC->GetPawn());
    }

    // 其他宿主（Character/Pawn/敌人）→ 宿主自身
    return Cast<ACharacter>(OwnerActor);
}

UAttributeComponent* UEquipmentComponent::GetAttribute() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    // 属性组件挂在角色/Pawn 上，不是 PC 上 → 从角色身上找
    if (ACharacter* Char = GetCharacter())
    {
        if (UAttributeComponent* Attr = Char->FindComponentByClass<UAttributeComponent>())
        {
            return Attr;
        }
    }

    // 兜底：宿主身上找（若直接挂在 Character 上）
    return OwnerActor->FindComponentByClass<UAttributeComponent>();
}

UInventoryComponent* UEquipmentComponent::GetInventory() const
{
    // 背包组件与装备组件同挂 PC（或 Character），从宿主身上找
    return GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

UItemCatalog* UEquipmentComponent::GetCatalog() const
{
    // Resolve 内部有静态缓存（GetDefaultCatalog），无需组件级缓存
    return UItemCatalog::Resolve(nullptr);
}

const FItemData* UEquipmentComponent::FindItem(FName ItemID) const
{
    const UItemCatalog* Catalog = GetCatalog();
    return Catalog ? Catalog->FindItem(ItemID) : nullptr;
}

// ========== 查询 ==========

FName UEquipmentComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
    const FName* Found = EquippedItems.Find(Slot);
    return Found ? *Found : NAME_None;
}

bool UEquipmentComponent::IsSlotEquipped(EEquipmentSlot Slot) const
{
    return !GetEquippedItem(Slot).IsNone();
}

const TMap<EEquipmentSlot, FName>& UEquipmentComponent::GetAllEquipped() const
{
    return EquippedItems;
}

FName UEquipmentComponent::GetEquippedWeaponItem() const
{
    return GetEquippedItem(EEquipmentSlot::Weapon);
}

bool UEquipmentComponent::IsWeaponEquipped() const
{
    return !GetEquippedWeaponItem().IsNone();
}

float UEquipmentComponent::GetTotalAttackBonus() const
{
    float Total = 0.0f;
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            Total += Item->AttackBonus;
        }
    }
    return Total;
}

float UEquipmentComponent::GetTotalDefenseBonus() const
{
    float Total = 0.0f;
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            Total += Item->DefenseBonus;
        }
    }
    return Total;
}

float UEquipmentComponent::GetTotalMaxHPBonus() const
{
    float Total = 0.0f;
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            Total += Item->MaxHPBonus;
        }
    }
    return Total;
}

float UEquipmentComponent::GetTotalMaxMPBonus() const
{
    float Total = 0.0f;
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            Total += Item->MaxMPBonus;
        }
    }
    return Total;
}

float UEquipmentComponent::GetTotalMoveSpeedBonus() const
{
    float Total = 0.0f;
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            Total += Item->MoveSpeedBonus;
        }
    }
    return Total;
}

// ========== 加成应用 ==========

void UEquipmentComponent::ApplyItemBonuses(const FItemData* Item, float Sign)
{
    if (!Item || Sign == 0.0f)
    {
        return;
    }

    if (UAttributeComponent* Attr = GetAttribute())
    {
        Attr->ModifyMaxHP(Item->MaxHPBonus * Sign);
        Attr->ModifyMaxMP(Item->MaxMPBonus * Sign);
        Attr->ModifyAttack(Item->AttackBonus * Sign);
        Attr->ModifyDefense(Item->DefenseBonus * Sign);
    }
}

void UEquipmentComponent::ApplyMoveSpeedBonuses()
{
    ACharacter* Char = GetCharacter();
    if (!Char)
    {
        return;
    }

    UCharacterMovementComponent* Move = Char->GetCharacterMovement();
    if (!Move)
    {
        return;
    }

    // 首次记录基础速度（此后为基准；>0 时使用蓝图配置值）
    if (BaseMoveSpeed <= 0.0f)
    {
        BaseMoveSpeed = Move->MaxWalkSpeed;
    }
    if (BaseMoveSpeed <= 0.0f)
    {
        return;
    }

    Move->MaxWalkSpeed = FMath::Max(0.0f, BaseMoveSpeed + GetTotalMoveSpeedBonus());
}

void UEquipmentComponent::ApplyAllBonuses()
{
    // 清空再按当前槽位整体应用，避免读档/换装时残留
    for (const auto& Pair : EquippedItems)
    {
        if (const FItemData* Item = FindItem(Pair.Value))
        {
            ApplyItemBonuses(Item, 1.0f);
        }
    }
    ApplyMoveSpeedBonuses();

    // 挂载当前武器（仅武器槽）
    if (const FItemData* Weapon = FindItem(GetEquippedItem(EEquipmentSlot::Weapon)))
    {
        ApplyAttachMesh(Weapon, true);
    }
}

// ========== 挂件 ==========

void UEquipmentComponent::ClearAttachedWeapon()
{
    if (AttachedWeapon)
    {
        AttachedWeapon->Destroy();
        AttachedWeapon = nullptr;
    }
}

void UEquipmentComponent::ApplyAttachMesh(const FItemData* Item, bool bAttach)
{
    ACharacter* Char = GetCharacter();
    if (!Char)
    {
        return;
    }

    USkeletalMeshComponent* Mesh = Char->GetMesh();
    if (!Mesh)
    {
        return;
    }

    ClearAttachedWeapon();
    if (!bAttach || !Item)
    {
        return;
    }

    const FName SocketName = Item->AttachSocketName.IsNone() ? TEXT("WeaponSocket") : Item->AttachSocketName;
    if (!Mesh->DoesSocketExist(SocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] AttachMesh: 网格上不存在 Socket=%s，装备 %s 挂载被跳过"),
            *SocketName.ToString(), *Item->ItemID.ToString());
        return;
    }

    // Spawn 武器 Actor
    if (Item->WeaponActorClass)
    {
        // 先获取 Socket 在世界中的位置，Spawn 时就放对位置
        FVector SocketLocation = FVector::ZeroVector;
        FRotator SocketRotation = FRotator::ZeroRotator;
        Mesh->GetSocketWorldLocationAndRotation(SocketName, SocketLocation, SocketRotation);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Char;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AttachedWeapon = GetWorld()->SpawnActor<AActor>(Item->WeaponActorClass, SocketLocation, SocketRotation, SpawnParams);
        
        if (AttachedWeapon)
        {
            // 确保 RootComponent 是 Movable
            if (USceneComponent* RootComp = AttachedWeapon->GetRootComponent())
            {
                RootComp->SetMobility(EComponentMobility::Movable);
            }
            
            // Attach 到骨骼 Socket，跟随动画
            AttachedWeapon->AttachToComponent(
                Mesh, 
                FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false), 
                SocketName
            );
            
            UE_LOG(LogTemp, Log, TEXT("[EquipmentComponent] 武器已挂载到 Socket=%s, Actor=%s"), 
                *SocketName.ToString(), *AttachedWeapon->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] AttachMesh: 装备 %s 无 WeaponActorClass"),
            *Item->ItemID.ToString());
    }
}

// ========== 穿戴/卸下 ==========

bool UEquipmentComponent::EquipItem(FName ItemID)
{
    const FItemData* Item = FindItem(ItemID);
    if (!Item || Item->ItemType != EItemType::Equipment || Item->EquipmentSlot == EEquipmentSlot::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem: %s 不是可装备物品"), *ItemID.ToString());
        return false;
    }

    UInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem: 未找到背包组件"));
        return false;
    }

    // 背包里必须有至少 1 个
    if (Inventory->GetItemCount(ItemID) < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem: 背包中没有 %s"), *ItemID.ToString());
        return false;
    }

    const EEquipmentSlot Slot = Item->EquipmentSlot;
    const FName OldItemID = GetEquippedItem(Slot);

    // 先从背包移除新装备（此后任何失败都要回滚）
    if (!Inventory->RemoveItem(ItemID, 1))
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem: 从背包移除 %s 失败"), *ItemID.ToString());
        return false;
    }

    // 目标槽已有装备：先换下回背包；背包满则整体回滚
    if (!OldItemID.IsNone())
    {
        if (!Inventory->AddItem(OldItemID, 1))
        {
            // 回滚：新装备放回背包
            Inventory->AddItem(ItemID, 1);
            UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem: 背包满，无法换下 %s"), *OldItemID.ToString());
            return false;
        }

        // 撤销旧装备加成 + 卸载旧挂件
        if (const FItemData* OldItem = FindItem(OldItemID))
        {
            ApplyItemBonuses(OldItem, -1.0f);
        }
    }

    // 写入槽位
    EquippedItems.Add(Slot, ItemID);

    // 应用新装备加成 + 重算移速 + 挂载新武器（仅武器槽需要换挂件）
    ApplyItemBonuses(Item, 1.0f);
    ApplyMoveSpeedBonuses();
    if (Slot == EEquipmentSlot::Weapon)
    {
        ApplyAttachMesh(Item, true);
    }

    OnEquipmentChanged.Broadcast();
    SaveEquipment();

    UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] EquipItem 成功: %s → 槽位 %d"), *ItemID.ToString(), static_cast<int32>(Slot));
    return true;
}

bool UEquipmentComponent::UnequipItem(EEquipmentSlot Slot)
{
    const FName ItemID = GetEquippedItem(Slot);
    if (ItemID.IsNone())
    {
        return false;
    }

    UInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        return false;
    }

    // 回背包（背包满则失败，保持穿戴）
    if (!Inventory->AddItem(ItemID, 1))
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] UnequipItem: 背包满，%s 无法卸下"), *ItemID.ToString());
        return false;
    }

    EquippedItems.Remove(Slot);

    if (const FItemData* Item = FindItem(ItemID))
    {
        ApplyItemBonuses(Item, -1.0f);
    }
    ApplyMoveSpeedBonuses();
    if (Slot == EEquipmentSlot::Weapon)
    {
        ClearAttachedWeapon();
    }

    OnEquipmentChanged.Broadcast();
    SaveEquipment();

    UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] UnequipItem 成功: 槽位 %d 卸下 %s"), static_cast<int32>(Slot), *ItemID.ToString());
    return true;
}

// ========== 存档 ==========

void UEquipmentComponent::SaveEquipment()
{
    // 优化：所有字段都会被整体覆盖，无需先读旧档（省一次磁盘读取+反序列化）
    UEquipmentSaveGame* Save = Cast<UEquipmentSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UEquipmentSaveGame::StaticClass()));
    if (!Save)
    {
        return;
    }

    Save->SavedEquipment = EquippedItems;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void UEquipmentComponent::LoadEquipment()
{
    UEquipmentSaveGame* Save = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        Save = Cast<UEquipmentSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    }

    EquippedItems.Reset();
    if (Save)
    {
        EquippedItems = Save->SavedEquipment;
    }

    // Pawn 就绪则立即应用加成/挂件；否则延迟重试（PC 宿主常见）
    if (GetCharacter())
    {
        ApplyAllBonuses();
        bAppliedPending = true;
        OnEquipmentChanged.Broadcast();
    }
    else
    {
        bAppliedPending = false;
        PendingRetryCount = 0;
        RetryApplyPending();
    }
}

void UEquipmentComponent::RetryApplyPending()
{
    if (bAppliedPending)
    {
        return;
    }

    if (GetCharacter())
    {
        ApplyAllBonuses();
        bAppliedPending = true;
        OnEquipmentChanged.Broadcast();
        return;
    }

    if (++PendingRetryCount >= MaxPendingRetryCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] LoadEquipment: 重试 %d 次后 Pawn 仍未就绪，装备加成延迟到首次穿戴时应用"),
            PendingRetryCount);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(this, &UEquipmentComponent::RetryApplyPending);
    }
}
