#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "UIConfirmDialog.h" // EDialogResult（UFUNCTION 回调参数需要完整 UENUM）
#include "UIHUDWidget.generated.h"

class UButton;
class UTextBlock;
class UHorizontalBox;
class UProgressBar;
class USkillSlotEntryWidget;
class UBuffSlotEntryWidget;

/**
 * 常驻主 HUD / 主管理界面。
 *   - 位于 HUD 层（ZOrder 0），进游戏时由 UIManagerComponent 自动打开
 *   - 不暂停游戏、不抢鼠标（游戏中保持 GameOnly 输入）、Esc 不会关闭它、常驻缓存
 *   - 提供打开 背包 / 地图 / 设置 的入口，以及“退出游戏”（带确认弹窗）
 *   - Buff 图标栏：蓝图放一个横向排列容器命名 Box_Buffs（可缺省），自动显示生效中的 Buff
 *
 * 蓝图 WBP 继承本类，控件名与下列 BindWidgetOptional 一致即可（均可缺省，缺哪个哪个按钮无效）：
 *   Btn_Inventory  Btn_Map  Btn_Settings  Btn_Quit  Box_Buffs
 */
UCLASS()
class MYPROJECT_API UUIHUDWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    UUIHUDWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ===== 入口按钮（名字需与蓝图一致，均可缺省）=====
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Inventory;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Map;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Settings;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Quit;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Gacha;

    // 金币显示（监听背包组件的 OnCoinsChanged 自动刷新）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Coins;

    // Buff 图标栏（横向排列容器，蓝图命名 Box_Buffs，可缺省）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> Box_Buffs;

    // 技能栏（横向排列容器，蓝图命名 Box_Skills，可缺省；条目由 C++ 动态生成）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> Box_Skills;

    // 技能条目蓝图类（在 MyUIHUDWidget 蓝图里指定 WBP_技能槽）
    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    TSubclassOf<USkillSlotEntryWidget> SkillSlotClass;

    // Buff 条目蓝图类（在 MyUIHUDWidget 蓝图里指定 WBP_Buff槽）
    UPROPERTY(EditDefaultsOnly, Category = "Buff")
    TSubclassOf<UBuffSlotEntryWidget> BuffSlotClass;

    // 能量条（蓝图命名 EnergyBar，可缺省）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> EnergyBar;

    UFUNCTION()
    void HandleInventoryClicked();

    UFUNCTION()
    void HandleMapClicked();

    UFUNCTION()
    void HandleSettingsClicked();

    UFUNCTION()
    void HandleQuitClicked();

    // 退出确认弹窗的结果回调：仅在“确认”时退出游戏
    UFUNCTION()
    void HandleQuitDialogResult(EDialogResult Result);

    UFUNCTION()
    void HandleGachaClicked();

    UFUNCTION()
    void HandleCoinsChanged(int32 NewCoins);

    UFUNCTION()
    void HandleBuffsChanged();

    UFUNCTION()
    void HandleSkillsChanged();

    // 技能条目点击（图标点击释放）
    UFUNCTION()
    void HandleSkillSlotClicked(FName SkillID);

private:
    void RefreshCoins();
    // 重建 Buff 图标栏（图标 + 剩余秒数 + 叠层）
    void RefreshBuffBar();
    // 重建技能栏（目录数组顺序 = 槽位，条目点击释放）
    void RefreshSkillBar();
    // 刷新能量条
    void RefreshEnergy();
};
