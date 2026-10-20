#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "BuffCatalog.generated.h"

/** 数值属性类型（通用框架：加新属性 = 在此加枚举项，然后在对应系统里读 GetStatModifier） */
UENUM(BlueprintType)
enum class EBuffStat : uint8
{
    MoveSpeed        UMETA(DisplayName = "移动速度"),
    JumpVelocity     UMETA(DisplayName = "跳跃力度"),
    CoinMultiplier   UMETA(DisplayName = "金币倍率"),
    DamageMultiplier UMETA(DisplayName = "伤害倍率"),
};

/** 数值生效方式 */
UENUM(BlueprintType)
enum class EBuffModifierType : uint8
{
    Additive       UMETA(DisplayName = "加减值（速度 +100）"),
    Multiplicative UMETA(DisplayName = "百分比乘数（1.5 = +50%）"),
};

/** 重复获得同一 Buff 时的处理规则 */
UENUM(BlueprintType)
enum class EBuffStackRule : uint8
{
    Refresh  UMETA(DisplayName = "刷新时长（不叠数值）"),
    Stack    UMETA(DisplayName = "叠加层数（每层数值生效）"),
    Override UMETA(DisplayName = "覆盖旧的"),
};

/** Buff 定义（目录中的一行） */
USTRUCT(BlueprintType)
struct FBuffData
{
    GENERATED_BODY()

    // 唯一标识（如 "SpeedBuff"、"CoinBuff"），物品目录按它引用
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FName BuffID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    TObjectPtr<UTexture2D> Icon;

    // 影响的属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    EBuffStat Stat = EBuffStat::MoveSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    EBuffModifierType ModifierType = EBuffModifierType::Multiplicative;

    // 数值：Additive = 加减原始值；Multiplicative = 乘数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    float ModifierValue = 1.5f;

    // 持续秒数；<= 0 表示永久
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0"))
    float Duration = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    EBuffStackRule StackRule = EBuffStackRule::Refresh;

    // Stack 规则下的最大层数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "1"))
    int32 MaxStack = 3;
};

/**
 * 集中式 Buff 目录（一个资产管理所有 Buff 定义）。
 * 创建方式：内容浏览器 → 右键 → Miscellaneous → Data Asset → 选 BuffCatalog，
 * 推荐资产路径 /Game/UserInterface/DA_Buffs（代码按此路径兜底加载）。
 *
 * 职责：
 *  - Buffs：全部 Buff 定义（加 Buff = 数组加一行）；
 *  - 按 BuffID 查询定义；
 *  - 供 UBuffsComponent 添加 Buff 时查定义、供 UI 取图标/名字/剩余时间说明。
 */
UCLASS(BlueprintType)
class MYPROJECT_API UBuffCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
    TArray<FBuffData> Buffs;

    // 按 ID 找 Buff 定义（C++ 用；找不到返回 nullptr）
    const FBuffData* FindBuff(FName BuffID) const;

    // 蓝图查询（找不到返回空/默认；文本优先走 Config/Localization/ST_Data.csv，未命中回退内联文本）
    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDisplayName(FName BuffID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    FText GetDescription(FName BuffID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    UTexture2D* GetIcon(FName BuffID) const;

    UFUNCTION(BlueprintPure, Category = "Catalog")
    bool HasBuff(FName BuffID) const;

    // 优先返回传入的目录，否则加载约定路径 /Game/UserInterface/DA_Buffs
    static UBuffCatalog* Resolve(UBuffCatalog* Preferred);

    static UBuffCatalog* GetDefaultCatalog();

private:
    // 本地化文本：从 Config/Localization/ST_Data.csv 的内存字符串表取 Key，未命中回退 Fallback（内联文本）
    FText GetLocalizedText(FName BuffID, const FText& Fallback, const TCHAR* Field) const;
};
