#include "SkillCatalog.h"

#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextKey.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StrongObjectPtr.h"

USkillCatalog* USkillCatalog::GetDefaultCatalog()
{
    // TStrongObjectPtr 保活：避免资产被 GC 后 static 裸指针悬垂
    static TStrongObjectPtr<USkillCatalog> Cached =
        TStrongObjectPtr<USkillCatalog>(LoadObject<USkillCatalog>(nullptr, TEXT("/Game/UserInterface/DA_Skills")));
    return Cached.Get();
}

USkillCatalog* USkillCatalog::Resolve(USkillCatalog* Preferred)
{
    return Preferred ? Preferred : GetDefaultCatalog();
}

const FSkillData* USkillCatalog::FindSkill(FName SkillID) const
{
    return Skills.FindByPredicate([SkillID](const FSkillData& Skill) { return Skill.SkillID == SkillID; });
}

FText USkillCatalog::GetDisplayName(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? GetLocalizedText(SkillID, Data->DisplayName, TEXT("DisplayName")) : FText::GetEmpty();
}

FText USkillCatalog::GetDescription(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? GetLocalizedText(SkillID, Data->Description, TEXT("Description")) : FText::GetEmpty();
}

UTexture2D* USkillCatalog::GetIcon(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? Data->Icon : nullptr;
}

float USkillCatalog::GetCooldown(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? Data->Cooldown : 0.0f;
}

float USkillCatalog::GetEnergyCost(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? Data->EnergyCost : 0.0f;
}

ESkillEffectType USkillCatalog::GetEffectType(FName SkillID) const
{
    const FSkillData* Data = FindSkill(SkillID);
    return Data ? Data->EffectType : ESkillEffectType::None;
}

bool USkillCatalog::HasSkill(FName SkillID) const
{
    return FindSkill(SkillID) != nullptr;
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
            UE_LOG(LogTemp, Warning, TEXT("[SkillCatalog] 本地化 CSV 加载失败: %s"), *CsvPath);
        }
    }
    return *Table;
}

FText USkillCatalog::GetLocalizedText(FName SkillID, const FText& Fallback, const TCHAR* Field) const
{
    if (!SkillID.IsNone())
    {
        const FString KeyStr = FString::Printf(TEXT("Skill_%s_%s"), *SkillID.ToString(), Field);
        FString Source;
        if (GetDataStringTable().GetSourceString(FTextKey(KeyStr), Source))
        {
            return FText::FromString(Source);
        }
    }
    return Fallback;
}
