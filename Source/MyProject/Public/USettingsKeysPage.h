#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USettingsSubsystem.h"
#include "USettingsKeysPage.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class USettingsSubsystem;

class USettingsKeysPage;

/**
 * 动态生成的每一行“改键按钮”的点击代理。
 * UButton::OnClicked 是动态多播委托，不能直接绑定 lambda；
 * 用这个轻量 UObject 携带“本行对应哪个动作”的上下文，再以 UFUNCTION 接收点击。
 */
UCLASS()
class UKeyBindingRowProxy : public UObject
{
    GENERATED_BODY()

public:
    // 本行对应的可重绑动作（legacy 输入用）
    EBindableAction Action = EBindableAction::ClosePanel;

    // 本行对应的增强输入 Action 名（IMC_Keys 行用）；为空则走上面的 EBindableAction 分支
    FName EnhancedAction;

    // 所属按键页（弱引用，避免循环持有）
    TWeakObjectPtr<USettingsKeysPage> Owner;

    UFUNCTION()
    void OnChooseClicked();
};


/**
 * 设置 - 按键页（独立 UI 场景）。
 * 蓝图只需放一个 VerticalBox 容器（行由 C++ 动态生成）和一个“恢复默认”按钮。
 */
UCLASS()
class MYPROJECT_API USettingsKeysPage : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> Box_KeyBindings;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_ResetKeys;

    // 供动态行代理 UKeyBindingRowProxy 调用：开始为指定动作监听新按键
    void RequestListeningForKey(EBindableAction Action);

    // 供动态行代理 UKeyBindingRowProxy 调用：开始为增强输入 Action 监听新按键
    void RequestListeningForKeyEnhanced(FName ActionName);

    // 强制退出改键监听（关闭设置页时调用，防止监听状态残留吞掉键盘输入）
    UFUNCTION(BlueprintCallable, Category = "Settings|Keys")
    void ForceCancelListening();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 改键监听期间捕获按键（Esc 取消，修饰键忽略）
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // 改键监听期间也捕获鼠标按键（LMB/RMB 等可绑定；滚轮忽略）
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    UPROPERTY()
    TObjectPtr<USettingsSubsystem> SettingsMgr;

    // 依据可重绑动作列表，动态生成“动作名 + 当前键按钮”的行
    void BuildKeyBindingRows();
    // 依据 IMC_Keys 资产，为每个 Action 生成一行（增强输入键位）
    void BuildEnhancedKeyRows();
    // 用当前生效按键刷新每一行按钮文字（legacy + 增强输入）
    void RefreshAllKeyRows();
    // 用当前生效按键刷新增强输入行按钮文字
    void RefreshEnhancedKeyRows();
    // 进入“等待按下新键”状态（legacy 动作）
    void StartListening(EBindableAction Action, UButton* RowButton, UTextBlock* RowLabel);
    // 进入“等待按下新键”状态（增强输入 Action）
    void StartListeningEnhanced(FName ActionName, UButton* RowButton, UTextBlock* RowLabel);
    void CancelListening();
    void CommitListeningKey(FKey NewKey);

    UFUNCTION()
    void OnResetKeysClicked();

    // 动作 -> 该行显示按键的文本
    TMap<EBindableAction, UTextBlock*> KeyRowLabels;
    // 行按钮 -> 它对应的动作
    TMap<UButton*, EBindableAction> KeyRowButtons;

    // 增强输入 Action 名 -> 该行显示按键的文本
    TMap<FName, UTextBlock*> EnhancedRowLabels;
    // 行按钮 -> 它对应的增强输入 Action 名
    TMap<UButton*, FName> EnhancedRowButtons;

    // 持有动态行点击代理，防止被 GC（其 Outer 也指向本 Widget）
    UPROPERTY()
    TArray<TObjectPtr<UKeyBindingRowProxy>> RowProxies;

    bool bListeningForKey = false;
    // 正在监听的是否为增强输入行（legacy 行时为 false）
    bool bListeningEnhanced = false;
    EBindableAction ListeningAction = EBindableAction::ClosePanel;
    FName ListeningEnhancedAction;
    TObjectPtr<UButton> ListeningButton;
    TObjectPtr<UTextBlock> ListeningLabel;
};
