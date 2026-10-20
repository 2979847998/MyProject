#pragma once

#include "CoreMinimal.h"

#include "UIViewBase.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "USettingsWidget.generated.h"

/**
 * 设置主界面（容器）。
 * 只负责：页签切换（WidgetSwitcher）+ 关闭/退出。
 * 各设置页是独立 UI 类，由本类在 NativeConstruct 里动态创建并绑定进
 * WidgetSwitcher_Sections 的每一页（无需蓝图手动嵌套）：
 *   页 0 = SettingsAudioPageClass    （默认 /Game/UserInterface/MySettingsAudioPage）
 *   页 1 = SettingsDisplayPageClass  （默认 /Game/UserInterface/MySettingsDisplayPage，显示+画质同页）
 *   页 2 = SettingsKeysPageClass     （默认 /Game/UserInterface/MySettingsKeysPage）
 */
UCLASS()
class MYPROJECT_API USettingsWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    // ========== 绑定控件（名字和蓝图控件一致） ==========

    // 分类切换：WidgetSwitcher 含 3 页（音频/显示/按键，按此顺序；显示页内含画质档位）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Sections;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Audio;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Display;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Keys;

    // 关闭 / 退出按钮
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Btn_Close;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Button_Exit;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Button_Save;

    // ===== 页面 UI 类（3 个设置页，C++ 动态创建并绑定进 WidgetSwitcher_Sections） =====
    // 在蓝图“类默认值”里配置自定义子类；留空则自动加载默认路径的蓝图类
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Pages")
    TSubclassOf<UUserWidget> SettingsAudioPageClass;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Pages")
    TSubclassOf<UUserWidget> SettingsDisplayPageClass;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Pages")
    TSubclassOf<UUserWidget> SettingsKeysPageClass;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    // ========== 分类页签 ==========
    UFUNCTION()
    void HandleTabAudioClicked();

    UFUNCTION()
    void HandleTabDisplayClicked();

    UFUNCTION()
    void HandleTabKeysClicked();

    // 当前设置页签（0=音频 1=显示 2=按键）
    int32 CurrentTabIndex = 0;

    // 切换页签：切 WidgetSwitcher + 高亮（页面实例由 BuildPageInstances 动态创建）
    void SetSettingsTab(int32 TabIndex);

    // 选中页签金色高亮，其余恢复默认
    void ApplyTabHighlight();

    // 动态创建 3 个设置页并绑定进 WidgetSwitcher_Sections（页 0~2 按顺序）
    void BuildPageInstances();

    // 取页面类：优先用配置的 TSubclassOf，为空则按默认路径加载蓝图类
    TSubclassOf<UUserWidget> ResolvePageClass(TSubclassOf<UUserWidget> Configured, const TCHAR* DefaultPath);

    UFUNCTION()
    void OnCloseClicked();

    UFUNCTION()
    void OnExitClicked();
};
