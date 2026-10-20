#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USettingsSubsystem.h"
#include "UIViewBase.h"
#include "UIManagerComponent.generated.h"

class UUserWidget;
class UUIConfirmDialog;

/**
 * UI 管理器组件。挂在 PlayerController 上，统一负责：
 *   1. 面板的懒加载创建与缓存
 *   2. 按分层 ZOrder 挂到视口
 *   3. 输入模式 / 鼠标显隐 / 暂停 的集中切换
 *   4. Esc 关闭栈（从最上层往下关）
 *   5. Tooltip 的统一显示
 *
 * 使用：在 PlayerController 蓝图上添加本组件，在 PanelClasses 里配置
 *       “面板类型 -> Widget 蓝图类（必须继承 UIViewBase）”，然后调用 OpenPanel 等接口。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UUIManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUIManagerComponent();

    virtual void BeginPlay() override;

    // 销毁前解绑外部委托与本组件注册到 InputComponent 的按键回调，防止悬垂崩溃
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ===== 面板注册表：类型 -> Widget 蓝图类（在细节面板配置）=====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TMap<EUIPanelType, TSubclassOf<UUIViewBase>> PanelClasses;

    // 开局是否自动打开常驻主 HUD（需在 PanelClasses 注册 HUD）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    bool bAutoOpenHUDOnStart = true;

    // ===== 对外接口 =====

    // 打开（或把已打开的）面板提到最前，返回面板实例
    UFUNCTION(BlueprintCallable, Category = "UI")
    UUIViewBase* OpenPanel(EUIPanelType Type);

    // 关闭指定面板
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClosePanel(EUIPanelType Type);

    // 关闭栈顶（最前）且允许 Esc 关闭的面板
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CloseTopmost();

    // 打开/关闭切换（背包开关键用）
    UFUNCTION(BlueprintCallable, Category = "UI")
    void TogglePanel(EUIPanelType Type);

    // 关闭全部面板
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CloseAllPanels();

    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsPanelOpen(EUIPanelType Type) const;

    UFUNCTION(BlueprintPure, Category = "UI")
    UUIViewBase* GetPanel(EUIPanelType Type) const;

    // ===== 通用确认弹窗 =====
    // 弹出确认框并返回其实例；调用方在返回值上 Bind OnDialogResult 接收确认/取消结果。
    // bShowCancel=false 时退化为单按钮“提示框”。需先在 PanelClasses 注册 ConfirmDialog。
    // bShowCancel=false 时退化为单按钮“提示框”；ConfirmText/CancelText 留空则用蓝图预设按钮文字。
    // 需先在 PanelClasses 注册 ConfirmDialog。后两个按钮文字参数在节点“高级”里。
    UFUNCTION(BlueprintCallable, Category = "UI|Dialog", meta = (AdvancedDisplay = "ConfirmText,CancelText"))
    UUIConfirmDialog* ShowConfirmDialog(FText Title, FText Message, bool bShowCancel = true,
                                        FText ConfirmText = FText(), FText CancelText = FText());

    // ===== Tooltip（挂 Tooltip 层，跟随鼠标位置）=====
    UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
    void ShowTooltip(UUserWidget* TooltipWidget, FVector2D ScreenPosition);

    UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
    void HideTooltip();

protected:
    // 已创建并缓存的面板（关闭后是否保留取决于面板的 bCacheWhenClosed）
    UPROPERTY(Transient)
    TMap<EUIPanelType, TObjectPtr<UUIViewBase>> CachedPanels;

    // 面板打开顺序栈，数组末尾为最前的面板
    UPROPERTY(Transient)
    TArray<EUIPanelType> PanelStack;

    // 当前显示的 Tooltip
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ActiveTooltip;

    APlayerController* GetOwnerPC() const;

    // 分层 -> ZOrder
    int32 GetZOrderForLayer(EUILayer Layer) const;

    // 根据当前打开的面板，统一刷新输入模式、鼠标、暂停状态
    void RefreshInputAndPause();

    // Esc 按下回调
    void HandleEscapePressed();

    // I 键开关背包面板（具体按键可在“设置-按键”里改）
    void HandleToggleInventory();

    // M 键开关大地图面板（具体按键可在“设置-按键”里改）
    void HandleToggleMap();

    // G 键开关抽卡面板（具体按键可在“设置-按键”里改）
    void HandleToggleGacha();

    // E 键开关装备面板（具体按键可在“设置-按键”里改）
    void HandleToggleEquipment();

    // 设置里改键后的回调：解绑旧键再按最新设置绑定
    UFUNCTION()
    void HandleKeyBindingsChanged();

    // 按设置子系统里的当前按键绑定全部输入
    void BindKeys();

    // 绑定单个动作到指定键（BindKeys 内部按“自定义优先”调用）
    void BindActionKey(class UInputComponent* IC, EBindableAction Action, FKey Key);

    // 面板栈摘要（诊断日志用）
    FString GetStackSummary() const;

    // 解绑此前注册到本组件的全部按键（按对象清除，无需保存句柄）
    void UnbindKeys();
};
