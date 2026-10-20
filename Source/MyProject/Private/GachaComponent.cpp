#include "GachaComponent.h"
#include "InventoryComponent.h"

UGachaComponent::UGachaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const FGachaPoolConfig* UGachaComponent::GetActivePool() const
{
    UItemCatalog* CatalogResolved = UItemCatalog::Resolve(Catalog);
    if (!CatalogResolved)
    {
        return nullptr;
    }

    // 1. 运行时切换的池
    if (!CurrentPoolID.IsNone())
    {
        if (const FGachaPoolConfig* Pool = CatalogResolved->FindPool(CurrentPoolID))
        {
            return Pool;
        }
    }

    // 2. 配置的默认池
    if (!DefaultPoolID.IsNone())
    {
        if (const FGachaPoolConfig* Pool = CatalogResolved->FindPool(DefaultPoolID))
        {
            return Pool;
        }
    }

    // 3. 目录的第一个池
    if (CatalogResolved->Pools.Num() > 0)
    {
        return &CatalogResolved->Pools[0];
    }

    return nullptr;
}

UInventoryComponent* UGachaComponent::GetInventory() const
{
    const AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

int32 UGachaComponent::GetSingleCost() const
{
    const FGachaPoolConfig* Pool = GetActivePool();
    return Pool ? Pool->CostSingle : 0;
}

int32 UGachaComponent::GetTenCost() const
{
    const FGachaPoolConfig* Pool = GetActivePool();
    return Pool ? Pool->CostTen : 0;
}

TArray<FName> UGachaComponent::DrawSingle()
{
    return PerformDraw(1);
}

TArray<FName> UGachaComponent::DrawTen()
{
    return PerformDraw(10);
}

TArray<FName> UGachaComponent::PerformDraw(int32 Count)
{
    TArray<FName> Result;

    const FGachaPoolConfig* Pool = GetActivePool();
    UInventoryComponent* Inventory = GetInventory();
    if (!Pool || !Inventory || Count <= 0)
    {
        return Result;
    }

    // 先扣费，金币不足直接取消本次抽卡
    const int32 Cost = (Count == 1) ? Pool->CostSingle : Pool->CostTen;
    if (!Inventory->SpendCoins(Cost))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Gacha] 金币不足：需要 %d，当前 %d"), Cost, Inventory->GetCoins());
        return Result;
    }

    UItemCatalog* CatalogResolved = UItemCatalog::Resolve(Catalog);
    if (!CatalogResolved)
    {
        return Result;
    }

    for (int32 i = 0; i < Count; ++i)
    {
        // 达到保底阈值时，本抽强制在“史诗及以上”中随机
        const bool bForceRare =
            Pool->PityThreshold > 0 && (DrawsSinceRare + 1) >= Pool->PityThreshold;

        FName ItemID = CatalogResolved->RollItem(*Pool, EItemRarity::Epic, bForceRare);
        if (ItemID.IsNone())
        {
            // 卡池里没有满足保底稀有度的物品时，回退到全池普通随机
            ItemID = CatalogResolved->RollItem(*Pool, EItemRarity::Common, false);
        }
        if (ItemID.IsNone())
        {
            continue;
        }

        Result.Add(ItemID);

        const FItemData* Item = CatalogResolved->FindItem(ItemID);
        if (Item && static_cast<uint8>(Item->Rarity) >= static_cast<uint8>(EItemRarity::Epic))
        {
            DrawsSinceRare = 0;
        }
        else
        {
            ++DrawsSinceRare;
        }
    }

    if (Result.Num() > 0)
    {
        // 一次性写入背包（内部只刷新/存盘一次）
        Inventory->AddItems(Result);
        OnGachaDrawn.Broadcast(Result);
    }

    return Result;
}
