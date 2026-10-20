#include "MapWidget.h"
#include "UIManagerComponent.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

UMapWidget::UMapWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 大地图：普通面板层、打开暂停、显示鼠标、Esc 可关、关闭后缓存
    Layer = EUILayer::Panel;
    bPauseGameWhenOpen = true;
    bUseUIInput = true;
    bCloseWithEsc = true;
    bCacheWhenClosed = true;
}

void UMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Text_Title)
    {
        Text_Title->SetText(FText::FromString(TEXT("地图")));
    }

    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddUniqueDynamic(this, &UMapWidget::HandleCloseClicked);
    }
}

void UMapWidget::HandleCloseClicked()
{
    // 通过管理器关闭（会走 OnHide/OnClose 并恢复输入/暂停）；兜底直接移除
    if (OwnerUIManager)
    {
        OwnerUIManager->ClosePanel(PanelType);
    }
    else
    {
        RemoveFromParent();
    }
}
