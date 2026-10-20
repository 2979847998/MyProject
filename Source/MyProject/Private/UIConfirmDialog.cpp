#include "UIConfirmDialog.h"
#include "UIManagerComponent.h"

#include "Delegates/DelegateCombinations.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"

UUIConfirmDialog::UUIConfirmDialog(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 弹窗默认行为：Popup 层、暂停、显示鼠标、Esc 可关、用完即销毁
    Layer = EUILayer::Popup;
    bPauseGameWhenOpen = true;
    bCloseWithEsc = true;
    bCacheWhenClosed = false;
    bUseUIInput = true;
}

void UUIConfirmDialog::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Confirm)
    {
        Btn_Confirm->OnClicked.AddDynamic(this, &UUIConfirmDialog::HandleConfirmClicked);
    }
    if (Btn_Cancel)
    {
        Btn_Cancel->OnClicked.AddDynamic(this, &UUIConfirmDialog::HandleCancelClicked);
    }
    if (Btn_CloseMask)
    {
        // 点击遮罩空白处等同取消
        Btn_CloseMask->OnClicked.AddDynamic(this, &UUIConfirmDialog::HandleCancelClicked);
    }
}

void UUIConfirmDialog::SetupDialog(FText Title, FText Message, \
    bool bShowCancel, FText ConfirmText, FText CancelText)
{
    if (Text_Title)
    {
        const bool bHasTitle = !Title.IsEmpty();
        Text_Title->SetVisibility(bHasTitle ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bHasTitle)
        {
            Text_Title->SetText(Title);
        }
    }

    if (Text_Message)
    {
        Text_Message->SetText(Message);
    }

    if (Btn_Cancel)
    {
        Btn_Cancel->SetVisibility(bShowCancel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 仅当调用方传了文字时才覆盖，否则保留蓝图里预设的按钮文字
    if (!ConfirmText.IsEmpty() && Text_Confirm)
    {
        Text_Confirm->SetText(ConfirmText);
    }
    if (!CancelText.IsEmpty() && Text_Cancel)
    {
        Text_Cancel->SetText(CancelText);
    }
}

void UUIConfirmDialog::HandleConfirmClicked()
{
    FinishDialog(EDialogResult::Confirmed);
}

void UUIConfirmDialog::HandleCancelClicked()
{
    FinishDialog(EDialogResult::Cancelled);
}

void UUIConfirmDialog::FinishDialog(EDialogResult Result)
{
    if (bFinished)
    {
        return;
    }
    bFinished = true;

    OnDialogResult.Broadcast(Result);

    // 通过管理器关闭（会走 OnHide/OnClose 并刷新输入/暂停）；兜底直接移除
    if (OwnerUIManager)
    {
        OwnerUIManager->ClosePanel(PanelType);
    }
    else
    {
        RemoveFromParent();
    }
}

void UUIConfirmDialog::OnClose_Implementation()
{
    // Esc 或被管理器直接关闭时（没点任何按钮），按“取消”结算一次
    if (!bFinished)
    {
        bFinished = true;
        OnDialogResult.Broadcast(EDialogResult::Cancelled);
    }
}
