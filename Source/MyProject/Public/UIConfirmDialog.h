#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "Delegates/DelegateCombinations.h"
#include "UIConfirmDialog.generated.h"

class UTextBlock;
class UButton;

/** 弹窗结束时的结果 */
UENUM(BlueprintType)
enum class EDialogResult : uint8
{
    Confirmed, // 点了“确认”
    Cancelled, // 点了“取消” / 遮罩 / Esc
};

/** 弹窗结果委托：弹窗关闭时广播一次，携带确认或取消 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogResult, EDialogResult, Result);

/**
 * 通用确认/取消弹窗。
 *   - 自动挂 Popup 层（ZOrder 20），盖在背包/设置等普通面板之上
 *   - 默认打开时暂停游戏、显示鼠标、Esc 等同取消、关闭即销毁（不缓存）
 *   - 蓝图 WBP 继承本类，控件名与下面 BindWidgetOptional 一致即可（均可缺省）：
 *       Text_Title(可选) Text_Message Btn_Confirm Btn_Cancel(可选) Btn_CloseMask(可选)
 *   - 推荐在弹窗根节点放一个全屏半透明 Button 当作 Btn_CloseMask，实现“点空白处取消”的模态遮罩。
 */
UCLASS()
class MYPROJECT_API UUIConfirmDialog : public UUIViewBase
{
    GENERATED_BODY()

public:
    UUIConfirmDialog(const FObjectInitializer& ObjectInitializer);

    // 弹窗结束时广播一次（确认/取消/Esc/遮罩都会触发）
    UPROPERTY(BlueprintAssignable, Category = "UI|Dialog")
    FOnDialogResult OnDialogResult;

    // 填充内容：标题（传空则隐藏标题）、正文、是否显示“取消”按钮（false 即单按钮提示框）、
    // 确认/取消按钮文字（留空则沿用蓝图里预设的文字）。一个弹窗蓝图靠这套参数复用到所有场景。
    UFUNCTION(BlueprintCallable, Category = "UI|Dialog", meta = (AdvancedDisplay = "ConfirmText,CancelText"))
    void SetupDialog(FText Title, FText Message, bool bShowCancel = true,
                     FText ConfirmText = FText(), FText CancelText = FText());

protected:
    virtual void NativeConstruct() override;

    // 由管理器 ClosePanel 触发（含 Esc）：若尚未结算，按“取消”兜底结算
    virtual void OnClose_Implementation() override;

    // ===== BindWidget 控件（名字需与蓝图一致，均可缺省）=====
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Title;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Message;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Confirm;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Cancel;

    // 全屏遮罩按钮：点击空白处等同取消
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_CloseMask;

    // 按钮上的文字（放在对应 Button 内部，可选；用于运行时改成“是/否/购买/删除”等）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Confirm;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Cancel;

    UFUNCTION()
    void HandleConfirmClicked();

    UFUNCTION()
    void HandleCancelClicked();

private:
    // 结算结果并关闭；bFinished 保证只广播一次
    void FinishDialog(EDialogResult Result);

    bool bFinished = false;
};
