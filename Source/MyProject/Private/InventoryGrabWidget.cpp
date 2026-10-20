#include "InventoryGrabWidget.h"
#include "InventoryComponent.h"
#include "ItemCatalog.h"
#include "InventorySlotEntryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UInventoryComponent* UInventoryGrabWidget::GetInventory()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UInventoryComponent>();
    }
    return nullptr;
}

void UInventoryGrabWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree) return;

    // 根 = CanvasPanel（跟随鼠标需要视口绝对定位）
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = RootCanvas;

    BorderFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    ImageIcon   = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    TextCount   = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

    BorderFrame->SetContent(ImageIcon);

    if (UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BorderFrame))
    {
        BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        BorderSlot->SetSize(FVector2D(64.f, 64.f));
    }
    if (UCanvasPanelSlot* CountSlot = RootCanvas->AddChildToCanvas(TextCount))
    {
        CountSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
        CountSlot->SetAlignment(FVector2D(1.f, 1.f));
        CountSlot->SetAutoSize(true);
    }
    if (BorderFrame)
    {
        BorderFrame->SetBrushColor(FLinearColor::White);
    }
    if (TextCount)
    {
        TextCount->SetColorAndOpacity(FLinearColor::White);
        TextCount->SetShadowColorAndOpacity(FLinearColor::Black);
        TextCount->SetShadowOffset(FVector2D(1.f, 1.f));
    }

    SetVisibility(ESlateVisibility::Collapsed);
}

void UInventoryGrabWidget::RefreshFromInventory()
{
    UInventoryComponent* Inventory = GetInventory();
    const bool bHasPicked = Inventory
        && Inventory->PickedSlotIndex >= 0
        && Inventory->PickedSlotIndex < Inventory->GetSlots().Num()
        && !Inventory->GetSlots()[Inventory->PickedSlotIndex].IsEmpty();

    if (!bHasPicked)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FItemSlot& ItemSlot = Inventory->GetSlots()[Inventory->PickedSlotIndex];
    const UItemCatalog* Catalog = UItemCatalog::GetDefaultCatalog();
    const FItemData* Item = Catalog ? Catalog->FindItem(ItemSlot.ItemID) : nullptr;
    if (!Item)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (ImageIcon && Item->Icon)
    {
        ImageIcon->SetBrushFromTexture(Item->Icon);
    }
    if (TextCount)
    {
        if (ItemSlot.Count > 1)
        {
            TextCount->SetText(FText::AsNumber(ItemSlot.Count));
            TextCount->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            TextCount->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (BorderFrame)
    {
        BorderFrame->SetBrushColor(UInventorySlotEntryWidget::GetRarityColor(Item->Rarity));
    }

    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(GrabOpacity);
}

void UInventoryGrabWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() != ESlateVisibility::Visible) return;

    if (APlayerController* PC = GetOwningPlayer())
    {
        double MouseX = 0.0, MouseY = 0.0;
        if (PC->GetMousePosition(MouseX, MouseY))
        {
            SetPositionInViewport(FVector2D(static_cast<float>(MouseX), static_cast<float>(MouseY)), true);
        }
    }
}
