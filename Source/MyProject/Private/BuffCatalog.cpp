#include "BuffCatalog.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/AssetManager.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextKey.h"
#include "Misc/Paths.h"

const FBuffData* UBuffCatalog::FindBuff(FName BuffID) const
{
    if (BuffID.IsNone())
    {
        return nullptr; 
    }
    return Buffs.FindByPredicate([BuffID](const FBuffData& Buff) { return Buff.BuffID == BuffID; });
}

FText UBuffCatalog::GetDisplayName(FName BuffID) const
{
    const FBuffData* Buff = FindBuff(BuffID);
    return Buff ? GetLocalizedText(BuffID, Buff->DisplayName, TEXT("DisplayName")) : FText::GetEmpty();
}

FText UBuffCatalog::GetDescription(FName BuffID) const
{
    const FBuffData* Buff = FindBuff(BuffID);
    return Buff ? GetLocalizedText(BuffID, Buff->Description, TEXT("Description")) : FText::GetEmpty();
}

UTexture2D* UBuffCatalog::GetIcon(FName BuffID) const
{
    const FBuffData* Buff = FindBuff(BuffID);
    return Buff ? Buff->Icon : nullptr;
}

bool UBuffCatalog::HasBuff(FName BuffID) const
{
    return FindBuff(BuffID) != nullptr;
}

UBuffCatalog* UBuffCatalog::Resolve(UBuffCatalog* Preferred)
{
    if (Preferred)
    {
        return Preferred;
    }
    return GetDefaultCatalog();
}

UBuffCatalog* UBuffCatalog::GetDefaultCatalog()
{
    static TWeakObjectPtr<UBuffCatalog> Cached;
    if (Cached.IsValid())
    {
        return Cached.Get();
    }

    UBuffCatalog* Catalog = LoadObject<UBuffCatalog>(nullptr, TEXT("/Game/UserInterface/DA_Buffs.DA_Buffs"));
    if (Catalog)
    {
        Cached = Catalog;
    }
    return Catalog;
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
            UE_LOG(LogTemp, Warning, TEXT("[BuffCatalog] 本地化 CSV 加载失败: %s"), *CsvPath);
        }
    }
    return *Table;
}

FText UBuffCatalog::GetLocalizedText(FName BuffID, const FText& Fallback, const TCHAR* Field) const
{
    if (!BuffID.IsNone())
    {
        const FString KeyStr = FString::Printf(TEXT("Buff_%s_%s"), *BuffID.ToString(), Field);
        FString Source;
        if (GetDataStringTable().GetSourceString(FTextKey(KeyStr), Source))
        {
            return FText::FromString(Source);
        }
    }
    return Fallback;
}
