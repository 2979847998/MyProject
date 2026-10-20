#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "SkillCatalog.generated.h"

/** 技能类型（当前框架以主动技能为主，被动预留） */
UENUM(BlueprintType)
enum class ESkillType : uint8
{
    Active  UMETA(DisplayName = "主动"),
    Passive UMETA(DisplayName = "被动"),
};

/** 技能效果类型（通用效果框架：数据配置效果，效果执行在 C++ / 蓝图钩子） */
UENUM(BlueprintType)
enum class ESkillEffectType : uint8
{
    // 只消耗并进入冷却，不做事（效果完全由蓝图 OnSkillCasted 接）
    None UMETA(DisplayName = "无效果（纯释放钩子）"),

    // 对宿主施加一个 Buff（关联 /Game/UserInterface/DA_Buffs 中的 BuffID）
    ApplyBuff UMETA(DisplayName = "施加 Buff"),

    // 触发蓝图事件 ApplyInstantEffect(SkillID, InstantValue)，由蓝图实现瞬时效果（回血/伤害/位移等）
    Instant UMETA(DisplayName = "瞬时效果（蓝图实现）"),
};

/**
 * 技能定义（目录中的一行，轻量值类型）。
 * 集中放在 USkillCatalog::Skills 数组里，加技能 = 数组加一行。
 * 玩家技能列表 = 目录数组顺序，槽位 = 数组下标（数字键 1/2/3... 对应槽位 0/1/2...）。
 */
USTRUCT(BlueprintType)
struct FSkillData
{
    GENERATED_BODY()

    // 唯一标识（如 "SpeedDash"），组件 / 蓝图用 SkillID 引用本行
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillType SkillType = ESkillType::Active;

    // 冷却时间（秒）；0 = 无冷却
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0"))
    float Cooldown = 0.0f;

    // 能量消耗（0 = 不耗能）；能量不足时无法释放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0"))
    float EnergyCost = 0.0f;

    // ===== 效果配置（通用效果框架）=====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    ESkillEffectType EffectType = ESkillEffectType::None;

    // EffectType = ApplyBuff 时：施加的 BuffID（关联 DA_Buffs）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    FName BuffID;

    // EffectType = ApplyBuff 时：施加的叠层数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect", meta = (ClampMin = "1"))
    int32 BuffStack = 1;

    // EffectType = Instant 时：传给蓝图事件 ApplyInstantEffect 的数值（回血量 / 伤害量等，语义由蓝图决定）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    float InstantValue = 0.0f;
};

/**
 * 集中式技能目录（一个资产管理所有技能）。
 * 内容浏览器创建方式：右键 → Miscellaneous → Data Asset → 选 SkillCatalog，
 * 推荐资产路径 /Game/UserInterface/DA_Skills（代码按此路径兜底加载）。
 *
 * 职责：
 *  - Skills：全部技能定义（加技能 = 数组加一行）；
 *  - 查询：按 SkillID 找技能定义；
 *  - 玩家技能栏 = Skills 数组顺序，槽位 = 下标。
 */
UCLASS(BlueprintType)
class MYPROJECT_API USkillCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
    TArray<FSkillData> Skills;

    // ===== 查询 =====

    // 按 ID 找技能定义（C++ 用；找不到返回 nullptr）
    const FSkillData* FindSkill(FName SkillID) const;

    // 以下供蓝图直接取显示信息（找不到返回空/默认；文本优先走 Config/Localization/ST_Data.csv，未命中回退内联文本）
    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDisplayName(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDescription(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    UTexture2D* GetIcon(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    float GetCooldown(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    float GetEnergyCost(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    ESkillEffectType GetEffectType(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    bool HasSkill(FName SkillID) const;

    // ===== 默认目录 =====

    // 优先返回传入的目录，否则加载约定路径的默认目录 /Game/UserInterface/DA_Skills
    static USkillCatalog* Resolve(USkillCatalog* Preferred);

    // 加载约定路径的默认目录（static 缓存）
    static USkillCatalog* GetDefaultCatalog();

private:
    // 本地化文本：从 Config/Localization/ST_Data.csv 的内存字符串表取 Key，未命中回退 Fallback（内联文本）
    FText GetLocalizedText(FName SkillID, const FText& Fallback, const TCHAR* Field) const;
};
