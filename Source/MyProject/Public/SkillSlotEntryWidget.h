#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotEntryWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class USkillsComponent;

// 条目被点击时广播（HUD 转发给技能组件释放）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillSlotClicked, FName, SkillID);

/**
 * 技能栏中的一个技能条目。
 * 派生蓝图模式：C++ 只声明 BindWidgetOptional 控件，布局在蓝图 Designer 里拖。
 * 蓝图 WBP 继承本类，控件名需匹配：
 *   - Border_Base   ：外框（可选，用于高亮/状态色）
 *   - Image_Icon    ：技能图标
 *   - Text_Cooldown ：冷却剩余秒数 / 层数
 */
UCLASS()
class MYPROJECT_API USkillSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    USkillSlotEntryWidget(const FObjectInitializer& ObjectInitializer);

    // 绑定一个技能（显示信息取自目录）
    void Setup(FName InSkillID);

    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillSlotClicked OnSlotClicked;

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 蓝图 Designer 里拖的控件（名字必须匹配）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UBorder> Border_Base;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Icon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Cooldown;

private:
    void RefreshDisplay();

    FName SkillID;

    // 性能缓存：仅在显示值变化时才触碰控件（避免每帧 SetText/SetBrushColor 无效重绘）
    TWeakObjectPtr<USkillsComponent> CachedSkills;
    int32 LastDisplayedSeconds = -1;   // 上次显示的冷却秒数
    int8 LastEnoughEnergy = -1;        // 上次能量是否充足（-1=未初始化，0/1=缓存值）
    float CachedEnergyCost = -1.0f;    // 该技能能量消耗缓存（-1=未初始化）
};
