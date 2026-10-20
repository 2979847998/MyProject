#include "GachaWidget.h"
#include "UIManagerComponent.h"
#include "GachaComponent.h"
#include "InventoryComponent.h"
#include "InventoryEntryObject.h"
#include "InventorySlotEntryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"

UGachaWidget::UGachaWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 抽卡：普通面板层、打开暂停、显示鼠标、Esc 可关、关闭后缓存
    Layer = EUILayer::Panel;
    bPauseGameWhenOpen = true;
    bUseUIInput = true;
    bCloseWithEsc = true;
    bCacheWhenClosed = true;
}

UInventoryComponent* UGachaWidget::GetInventory()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UInventoryComponent>();
    }
    return nullptr;
}

UGachaComponent* UGachaWidget::GetGacha()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UGachaComponent>();
    }
    return nullptr;
}

void UGachaWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_DrawSingle)
    {
        Btn_DrawSingle->OnClicked.AddUniqueDynamic(this, &UGachaWidget::HandleDrawSingleClicked);
    }
    if (Btn_DrawTen)
    {
        Btn_DrawTen->OnClicked.AddUniqueDynamic(this, &UGachaWidget::HandleDrawTenClicked);
    }
    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddUniqueDynamic(this, &UGachaWidget::HandleCloseClicked);
    }

    if (UInventoryComponent* Inventory = GetInventory())
    {
        Inventory->OnCoinsChanged.AddUniqueDynamic(this, &UGachaWidget::HandleCoinsChanged);
    }
    if (UGachaComponent* Gacha = GetGacha())
    {
        Gacha->OnGachaDrawn.AddUniqueDynamic(this, &UGachaWidget::HandleGachaDrawn);
    }

    RefreshCoins();
}

void UGachaWidget::NativeDestruct()
{
    // 解绑跨对象委托：背包/抽卡组件是 PC 级，可能比本 Widget 活得久；不主动解绑，
    // 组件广播（金币变化/抽卡完成）时会访问悬垂 Widget
    if (UInventoryComponent* Inventory = GetInventory())
    {
        Inventory->OnCoinsChanged.RemoveDynamic(this, &UGachaWidget::HandleCoinsChanged);
    }
    if (UGachaComponent* Gacha = GetGacha())
    {
        Gacha->OnGachaDrawn.RemoveDynamic(this, &UGachaWidget::HandleGachaDrawn);
    }

    Super::NativeDestruct();
}

void UGachaWidget::OnOpen_Implementation()
{
    RefreshCoins();
}

void UGachaWidget::RefreshCoins()
{
    if (Text_Coins)
    {
        if (UInventoryComponent* Inventory = GetInventory())
        {
            Text_Coins->SetText(FText::AsNumber(Inventory->GetCoins()));
        }
        else
        {
            Text_Coins->SetText(FText::AsNumber(0));
        }
    }
}

void UGachaWidget::ShowResults(const TArray<FName>& DrawnItemIDs)
{
    if (!WrapBox_Results || !ResultEntryClass)
    {
        return;
    }

    WrapBox_Results->ClearChildren();

    APlayerController* PC = GetOwningPlayer();

    for (const FName& ItemID : DrawnItemIDs)
    {
        if (ItemID.IsNone())
        {
            continue;
        }

        UInventorySlotEntryWidget* EntryWidget =
            CreateWidget<UInventorySlotEntryWidget>(PC, ResultEntryClass);
        if (!EntryWidget)
        {
            continue;
        }

        // 条目对象以格子控件为 Outer，控件销毁时一并回收
        UInventoryEntryObject* EntryObject = NewObject<UInventoryEntryObject>(EntryWidget);
        EntryObject->Init(ItemID, 1);
        EntryWidget->SetEntryData(EntryObject);

        WrapBox_Results->AddChildToWrapBox(EntryWidget);
    }
}

void UGachaWidget::HandleDrawSingleClicked()
{
    if (UGachaComponent* Gacha = GetGacha())
    {
        Gacha->DrawSingle(); // 结果由 OnGachaDrawn 广播到 HandleGachaDrawn，金币由 OnCoinsChanged 刷新
    }
}

void UGachaWidget::HandleDrawTenClicked()
{
    if (UGachaComponent* Gacha = GetGacha())
    {
        Gacha->DrawTen();
    }
}

void UGachaWidget::HandleCloseClicked()
{
    if (OwnerUIManager)
    {
        OwnerUIManager->ClosePanel(PanelType);
    }
    else
    {
        RemoveFromParent();
    }
}

void UGachaWidget::HandleCoinsChanged(int32 NewCoins)
{
    if (Text_Coins)
    {
        Text_Coins->SetText(FText::AsNumber(NewCoins));
    }
}

void UGachaWidget::HandleGachaDrawn(TArray<FName> DrawnItemIDs)
{
    ShowResults(DrawnItemIDs);
}
