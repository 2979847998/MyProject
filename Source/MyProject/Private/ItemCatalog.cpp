#include "ItemCatalog.h"

#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextKey.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

const FItemData* UItemCatalog::FindItem(FName ItemID) const
{
    for (const FItemData& Item : AllItems)
    {
        if (Item.ItemID == ItemID)
        {
            return &Item;
        }
    }
    return nullptr;
}

const FGachaPoolConfig* UItemCatalog::FindPool(FName PoolID) const
{
    for (const FGachaPoolConfig& Pool : Pools)
    {
        if (Pool.PoolID == PoolID)
        {
            return &Pool;
        }
    }
    return nullptr;
}

FText UItemCatalog::GetDisplayName(FName ItemID) const
{
    const FItemData* Item = FindItem(ItemID);
    return Item ? GetLocalizedText(ItemID, Item->DisplayName, TEXT("DisplayName")) : FText::GetEmpty();
}

FText UItemCatalog::GetDescription(FName ItemID) const
{
    const FItemData* Item = FindItem(ItemID);
    return Item ? GetLocalizedText(ItemID, Item->Description, TEXT("Description")) : FText::GetEmpty();
}

UTexture2D* UItemCatalog::GetIcon(FName ItemID) const
{
    const FItemData* Item = FindItem(ItemID);
    return Item ? Item->Icon : nullptr;
}
int32 UItemCatalog::GetMaxStack(FName ItemID) const
{
    const FItemData* Item = FindItem(ItemID);
    return Item ? Item->MaxStack : 0;
}

bool UItemCatalog::HasItem(FName ItemID) const
{
    return FindItem(ItemID) != nullptr;
}

FName UItemCatalog::RollItem(const FGachaPoolConfig& Pool, EItemRarity MinRarity, bool bUseMinRarity) const
{
    // 收集本次参与随机的候选条目并累计权重（按目录里的 Rarity 过滤）
    TArray<const FGachaPoolEntry*> Candidates;
    int32 TotalWeight = 0;

    for (const FGachaPoolEntry& Entry : Pool.Entries)
    {
        if (Entry.ItemID.IsNone() || Entry.Weight <= 0)
        {
            continue;
        }

        const FItemData* Item = FindItem(Entry.ItemID);
        if (!Item)
        {
            continue;
        }

        if (bUseMinRarity &&
            static_cast<uint8>(Item->Rarity) < static_cast<uint8>(MinRarity))
        {
            continue;
        }

        Candidates.Add(&Entry);
        TotalWeight += Entry.Weight;
    }

    if (Candidates.Num() == 0 || TotalWeight <= 0)
    {
        return NAME_None;
    }

    // 在 [1, TotalWeight] 上取一个落点，按权重区间命中
    const int32 Roll = FMath::RandRange(1, TotalWeight);
    int32 Accumulated = 0;

    for (const FGachaPoolEntry* Entry : Candidates)
    {
        Accumulated += Entry->Weight;
        if (Roll <= Accumulated)
        {
            return Entry->ItemID;
        }
    }

    return Candidates.Last()->ItemID;
}

UItemCatalog* UItemCatalog::Resolve(UItemCatalog* Preferred)
{
    return Preferred ? Preferred : GetDefaultCatalog();
}

UItemCatalog* UItemCatalog::GetDefaultCatalog()
{
    // TStrongObjectPtr 保活：避免资产被 GC 后 static 裸指针悬垂
    static TStrongObjectPtr<UItemCatalog> DefaultCatalog =
        TStrongObjectPtr<UItemCatalog>(
            LoadObject<UItemCatalog>(nullptr, TEXT("/Game/UserInterface/DA_Catalog.DA_Catalog")));
    return DefaultCatalog.Get();
}

// 内存字符串表：从 Config/Localization/ST_Data.csv 加载（首次调用时），Key 规则与资产脚本一致
static FStringTable& GetDataStringTable()
{
    static FStringTableRef Table = MakeShared<FStringTable>();
    static bool bLoaded = false;
    if (!bLoaded)
    {
        const FString CsvPath = FPaths::ProjectConfigDir() + TEXT("Localization/ST_Data.csv");
        bLoaded = Table->ImportStrings(CsvPath);
        if (!bLoaded)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ItemCatalog] 本地化 CSV 加载失败: %s"), *CsvPath);
        }
    }
    return *Table;
}

FText UItemCatalog::GetLocalizedText(FName ItemID, const FText& Fallback, const TCHAR* Field) const
{
    if (!ItemID.IsNone())
    {
        const FString KeyStr = FString::Printf(TEXT("Item_%s_%s"), *ItemID.ToString(), Field);
        FString Source;
        if (GetDataStringTable().GetSourceString(FTextKey(KeyStr), Source))
        {
            return FText::FromString(Source);
        }
    }
    return Fallback;
}
