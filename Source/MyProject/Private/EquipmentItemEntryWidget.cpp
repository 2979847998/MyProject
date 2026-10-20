#include "EquipmentItemEntryWidget.h"
#include "EquipmentComponent.h"
#include "UIManagerComponent.h"
#include "UIConfirmDialog.h"
#include "InventorySlotEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

void UEquipmentItemEntryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 必须在 Slate 构建（TakeWidget/RebuildWidget）之前设置 WidgetTree->RootWidget，
    // 否则面板拿到空根节点渲染为空白
    EnsureBuilt();
}

void UEquipmentItemEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureBuilt();

    // 建树可能发生在 SetItem 之后，补一次填充
    ApplyData();
}

void UEquipmentItemEntryWidget::EnsureBuilt()
{
    if (bUIBuilt || !WidgetTree)
    {
        return;
    }
    bUIBuilt = true;

    // 根 = 整行按钮，点击即穿戴
    Btn_Root = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    WidgetTree->RootWidget = Btn_Root;
    Btn_Root->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.06f));
    Btn_Root->OnClicked.AddUniqueDynamic(this, &UEquipmentItemEntryWidget::HandleClicked);

    // 稀有度边框（与背包格子同一套配色）
    Border_Rarity = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border_Rarity->SetPadding(FMargin(2.f, 2.f, 2.f, 2.f));
    Btn_Root->AddChild(Border_Rarity);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border_Rarity->SetContent(Row);

    // 图标
    Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Image_Icon->SetDesiredSizeOverride(FVector2D(44.f, 44.f));
    if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Image_Icon))
    {
        IconSlot->SetPadding(FMargin(4.f, 4.f, 8.f, 4.f));
        IconSlot->SetVerticalAlignment(VAlign_Center);
    }

    // 名称 + 槽位/加成摘要
    UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextBox))
    {
        TextSlot->SetPadding(FMargin(0.f, 4.f, 8.f, 4.f));
        TextSlot->SetVerticalAlignment(VAlign_Center);
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    Text_Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (UVerticalBoxSlot* NameSlot = TextBox->AddChildToVerticalBox(Text_Name))
    {
        NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
    }

    // 字体族沿用控件默认（含中文回退），只调字号与颜色，避免换成不含中文字形的字体
    Text_Info = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo InfoFont = Text_Name->GetFont();
    InfoFont.Size = FMath::Max(InfoFont.Size - 2, 8);
    Text_Info->SetFont(InfoFont);
    Text_Info->SetColorAndOpacity(FLinearColor(0.75f, 0.78f, 0.82f, 1.0f));
    TextBox->AddChildToVerticalBox(Text_Info);

    // 持有数量（>1 才显示）
    Text_Count = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FSlateFontInfo CountFont = Text_Name->GetFont();
    CountFont.Size = FMath::Max(CountFont.Size - 1, 8);
    Text_Count->SetFont(CountFont);
    if (UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(Text_Count))
    {
        CountSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        CountSlot->SetVerticalAlignment(VAlign_Center);
        CountSlot->SetHorizontalAlignment(HAlign_Right);
    }
}

UEquipmentComponent* UEquipmentItemEntryWidget::GetEquipment() const
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UEquipmentComponent>();
    }
    return nullptr;
}

void UEquipmentItemEntryWidget::ShowAlert(const FText& Message) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }
    if (UUIManagerComponent* UI = PC->FindComponentByClass<UUIManagerComponent>())
    {
        // bShowCancel=false：退化为单按钮提示框
        UI->ShowConfirmDialog(FText::FromString(TEXT("装备")), Message, false);
    }
}

void UEquipmentItemEntryWidget::SetItem(FName InItemID, int32 InCount)
{
    ItemID = InItemID;
    Count = InCount;

    // 树还没建（SetItem 早于 NativeConstruct）时只存数据，建树后由 NativeConstruct 补填充
    if (bUIBuilt)
    {
        ApplyData();
    }
}

void UEquipmentItemEntryWidget::ApplyData()
{
    if (!bUIBuilt || ItemID.IsNone())
    {
        return;
    }

    const UItemCatalog* Catalog = UItemCatalog::Resolve(ItemCatalog);
    const FItemData* Item = Catalog ? Catalog->FindItem(ItemID) : nullptr;
    if (!Item)
    {
        Text_Name->SetText(FText::FromName(ItemID));
        Text_Info->SetText(FText::FromString(TEXT("物品目录中找不到该物品")));
        Text_Count->SetVisibility(ESlateVisibility::Collapsed);
        Border_Rarity->SetBrushColor(UInventorySlotEntryWidget::GetRarityColor(EItemRarity::Common));
        Btn_Root->SetIsEnabled(false);
        return;
    }

    Btn_Root->SetIsEnabled(true);

    if (Image_Icon && Item->Icon)
    {
        Image_Icon->SetBrushFromTexture(Item->Icon);
    }

    Text_Name->SetText(Item->DisplayName);
    Border_Rarity->SetBrushColor(UInventorySlotEntryWidget::GetRarityColor(Item->Rarity));

    // 槽位显示名（取枚举 DisplayName，如"武器"）+ 非零加成摘要
    FString Info;
    if (const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>())
    {
        Info = FString::Printf(TEXT("[%s]"),
            *SlotEnum->GetDisplayNameTextByValue(static_cast<int64>(Item->EquipmentSlot)).ToString());
    }

    struct FBonusLabel { const TCHAR* Label; float Value; };
    const FBonusLabel Bonuses[] = {
        { TEXT("攻击"), Item->AttackBonus },
        { TEXT("防御"), Item->DefenseBonus },
        { TEXT("生命"), Item->MaxHPBonus },
        { TEXT("法力"), Item->MaxMPBonus },
        { TEXT("移速"), Item->MoveSpeedBonus },
    };
    for (const FBonusLabel& B : Bonuses)
    {
        if (B.Value <= 0.0f)
        {
            continue;
        }
        Info += FString::Printf(TEXT("  %s+%g"), B.Label, B.Value);
    }
    Text_Info->SetText(FText::FromString(Info));

    if (Count > 1)
    {
        Text_Count->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
        Text_Count->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        Text_Count->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UEquipmentItemEntryWidget::HandleClicked()
{
    if (ItemID.IsNone())
    {
        return;
    }

    UEquipmentComponent* Equipment = GetEquipment();
    if (!Equipment)
    {
        ShowAlert(FText::FromString(TEXT("找不到装备组件，无法穿戴。")));
        return;
    }

    // 失败原因组件内已写日志；背包满是最常见的一种，这里给用户可见提示
    if (!Equipment->EquipItem(ItemID))
    {
        ShowAlert(FText::FromString(TEXT("穿戴失败：背包已满或该物品不可装备。")));
    }
    // 成功后的刷新由 OnEquipmentChanged / OnInventoryChanged 广播驱动面板统一处理
}
