#include "EquipmentWidget.h"
#include "EquipmentComponent.h"
#include "EquipmentItemEntryWidget.h"
#include "InventoryComponent.h"
#include "InventorySlotEntryWidget.h"
#include "ItemSlot.h"
#include "UIManagerComponent.h"
#include "UIConfirmDialog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

// 槽位显示顺序（None 不参与）
static const EEquipmentSlot SlotOrder[] = {
    EEquipmentSlot::Weapon,
    EEquipmentSlot::Helmet,
    EEquipmentSlot::Armor,
    EEquipmentSlot::Boots,
    EEquipmentSlot::Accessory,
};

// ===== 槽位行点击代理 =====

void UEquipmentSlotRowProxy::OnUnequipClicked()
{
    if (UEquipmentWidget* Panel = Owner.Get())
    {
        Panel->HandleUnequipSlot(Slot);
    }
}

// ===== 面板 =====

UEquipmentWidget::UEquipmentWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 装备：普通面板层、打开暂停、显示鼠标、Esc 可关、关闭后缓存
    Layer = EUILayer::Panel;
    bPauseGameWhenOpen = true;
    bUseUIInput = true;
    bCloseWithEsc = true;
    bCacheWhenClosed = true;
}

UEquipmentComponent* UEquipmentWidget::GetEquipment() const
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UEquipmentComponent>();
    }
    return nullptr;
}

UInventoryComponent* UEquipmentWidget::GetInventory() const
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UInventoryComponent>();
    }
    return nullptr;
}

FText UEquipmentWidget::GetSlotDisplayName(EEquipmentSlot Slot)
{
    if (const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>())
    {
        return SlotEnum->GetDisplayNameTextByValue(static_cast<int64>(Slot));
    }
    return FText::FromString(TEXT("槽位"));
}

void UEquipmentWidget::ShowAlert(const FText& Message) const
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UUIManagerComponent* UI = PC->FindComponentByClass<UUIManagerComponent>())
        {
            UI->ShowConfirmDialog(FText::FromString(TEXT("装备")), Message, false);
        }
    }
}

// 建一个带文字的按钮（按钮文字用控件默认字体，含中文回退）
static UButton* MakeLabeledButton(UWidgetTree* Tree, const TCHAR* Label, const FLinearColor& BgColor)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
    Btn->SetBackgroundColor(BgColor);
    UTextBlock* Txt = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Txt->SetText(FText::FromString(Label));
    Btn->AddChild(Txt);
    return Btn;
}

void UEquipmentWidget::BuildUI()
{
    if (bUIBuilt || !WidgetTree)
    {
        return;
    }
    bUIBuilt = true;

    // 根：全屏半透明遮罩，内容居中
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Dim->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.65f));
    Dim->SetHorizontalAlignment(HAlign_Center);
    Dim->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Dim;

    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SizeBox->SetWidthOverride(760.f);
    SizeBox->SetHeightOverride(500.f);
    Dim->SetContent(SizeBox);

    // 面板主体
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.10f, 0.11f, 0.14f, 0.97f));
    Panel->SetPadding(FMargin(18.f, 16.f, 18.f, 16.f));
    SizeBox->AddChild(Panel);

    UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->SetContent(Main);
    RootBox = Main;

    // ===== 标题行 =====
    UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Main->AddChildToVerticalBox(TitleRow);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Title->SetText(FText::FromString(TEXT("装备")));
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.Size = FMath::Max(TitleFont.Size + 8, 18);
    Title->SetFont(TitleFont);
    if (UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(Title))
    {
        TitleSlot->SetVerticalAlignment(VAlign_Center);
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    Btn_Close = MakeLabeledButton(WidgetTree, TEXT("关闭"), FLinearColor(0.55f, 0.18f, 0.18f, 1.0f));
    if (UHorizontalBoxSlot* CloseSlot = TitleRow->AddChildToHorizontalBox(Btn_Close))
    {
        CloseSlot->SetVerticalAlignment(VAlign_Center);
        CloseSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    }

    // ===== 内容行：左槽位 / 右可装备列表 =====
    UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (UVerticalBoxSlot* ContentSlot = Main->AddChildToVerticalBox(Content))
    {
        ContentSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
        ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    // ---- 左列：五个槽位 ----
    UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* LeftSlot = Content->AddChildToHorizontalBox(LeftCol))
    {
        LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LeftSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    }

    UTextBlock* LeftHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LeftHeader->SetText(FText::FromString(TEXT("已装备")));
    LeftHeader->SetColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.9f, 1.0f));
    if (UVerticalBoxSlot* LHSlot = LeftCol->AddChildToVerticalBox(LeftHeader))
    {
        LHSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    // 逐槽位建行 + 代理
    SlotRows.Reset();
    RowProxies.Reset();
    for (EEquipmentSlot EqSlot : SlotOrder)
    {
        FEquipmentSlotRow Row;
        Row.Slot = EqSlot;

        Row.Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Row.Border->SetPadding(FMargin(6.f, 6.f, 6.f, 6.f));
        Row.Border->SetBrushColor(FLinearColor(0.16f, 0.17f, 0.21f, 1.0f));
        if (UVerticalBoxSlot* RowSlot = LeftCol->AddChildToVerticalBox(Row.Border))
        {
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
        }

        UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        Row.Border->SetContent(RowBox);

        Row.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Row.Icon->SetDesiredSizeOverride(FVector2D(40.f, 40.f));
        if (UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(Row.Icon))
        {
            IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
            IconSlot->SetVerticalAlignment(VAlign_Center);
        }

        UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        if (UHorizontalBoxSlot* TBSlot = RowBox->AddChildToHorizontalBox(TextBox))
        {
            TBSlot->SetVerticalAlignment(VAlign_Center);
            TBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        Row.Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        FSlateFontInfo NameFont = Row.Name->GetFont();
        NameFont.Size = FMath::Max(NameFont.Size + 1, 12);
        Row.Name->SetFont(NameFont);
        if (UVerticalBoxSlot* NameSlot = TextBox->AddChildToVerticalBox(Row.Name))
        {
            NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }

        Row.Info = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        FSlateFontInfo InfoFont = Row.Info->GetFont();
        InfoFont.Size = FMath::Max(InfoFont.Size - 2, 8);
        Row.Info->SetFont(InfoFont);
        Row.Info->SetColorAndOpacity(FLinearColor(0.7f, 0.73f, 0.78f, 1.0f));
        TextBox->AddChildToVerticalBox(Row.Info);

        Row.BtnUnequip = MakeLabeledButton(WidgetTree, TEXT("卸下"), FLinearColor(0.3f, 0.33f, 0.42f, 1.0f));
        if (UHorizontalBoxSlot* BtnSlot = RowBox->AddChildToHorizontalBox(Row.BtnUnequip))
        {
            BtnSlot->SetVerticalAlignment(VAlign_Center);
            BtnSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
        }

        // 点击代理：携带本行槽位
        UEquipmentSlotRowProxy* Proxy = NewObject<UEquipmentSlotRowProxy>(this);
        Proxy->Slot = EqSlot;
        Proxy->Owner = this;
        Row.BtnUnequip->OnClicked.AddUniqueDynamic(Proxy, &UEquipmentSlotRowProxy::OnUnequipClicked);
        RowProxies.Add(Proxy);

        SlotRows.Add(Row);
    }

    // ---- 右列：背包可装备物品 ----
    UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* RightSlot = Content->AddChildToHorizontalBox(RightCol))
    {
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        RightSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
    }

    UTextBlock* RightHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RightHeader->SetText(FText::FromString(TEXT("背包装备（点击穿戴）")));
    RightHeader->SetColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.9f, 1.0f));
    if (UVerticalBoxSlot* RHSlot = RightCol->AddChildToVerticalBox(RightHeader))
    {
        RHSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    UBorder* ListBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    ListBg->SetBrushColor(FLinearColor(0.07f, 0.075f, 0.09f, 1.0f));
    ListBg->SetPadding(FMargin(4.f, 4.f, 4.f, 4.f));
    if (UVerticalBoxSlot* LBSlot = RightCol->AddChildToVerticalBox(ListBg))
    {
        LBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    ListBg->SetContent(Scroll);

    Box_Available = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(Box_Available);

    // ===== 底部：属性加成汇总 =====
    Text_Summary = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text_Summary->SetColorAndOpacity(FLinearColor(0.75f, 0.82f, 0.7f, 1.0f));
    if (UVerticalBoxSlot* SumSlot = Main->AddChildToVerticalBox(Text_Summary))
    {
        SumSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    }
}

void UEquipmentWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildUI();
}

void UEquipmentWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildUI();

    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddUniqueDynamic(this, &UEquipmentWidget::HandleCloseClicked);
    }

    // 订阅装备/背包变化：穿、卸、抽卡、拾取后都自动刷新
    if (UEquipmentComponent* Equipment = GetEquipment())
    {
        Equipment->OnEquipmentChanged.AddUniqueDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
    }
    if (UInventoryComponent* Inventory = GetInventory())
    {
        Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UEquipmentWidget::HandleInventoryChanged);
    }
    bBound = true;

    RefreshAll();
}

void UEquipmentWidget::NativeDestruct()
{
    // 解绑跨对象委托：组件是 PC 级，可能比本 Widget 活得久；不解绑会在广播时访问悬垂 Widget
    if (bBound)
    {
        if (UEquipmentComponent* Equipment = GetEquipment())
        {
            Equipment->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
        }
        if (UInventoryComponent* Inventory = GetInventory())
        {
            Inventory->OnInventoryChanged.RemoveDynamic(this, &UEquipmentWidget::HandleInventoryChanged);
        }
        bBound = false;
    }

    Super::NativeDestruct();
}

void UEquipmentWidget::OnOpen_Implementation()
{
    RefreshAll();
}

void UEquipmentWidget::OnClose_Implementation()
{
    // 关闭时取消背包拾取，避免抓取层残留在屏幕上
    if (UInventoryComponent* Inventory = GetInventory())
    {
        if (Inventory->PickedSlotIndex >= 0)
        {
            Inventory->PickedSlotIndex = -1;
            Inventory->OnInventoryChanged.Broadcast();
        }
    }
}

void UEquipmentWidget::HandleCloseClicked()
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

void UEquipmentWidget::HandleEquipmentChanged()
{
    RefreshAll();
}

void UEquipmentWidget::HandleInventoryChanged()
{
    RefreshAll();
}

void UEquipmentWidget::RefreshAll()
{
    RefreshSlotRows();
    RefreshAvailableList();
    RefreshSummary();
}

void UEquipmentWidget::RefreshSlotRows()
{
    UEquipmentComponent* Equipment = GetEquipment();
    const UItemCatalog* Catalog = UItemCatalog::Resolve(ItemCatalog);

    for (FEquipmentSlotRow& Row : SlotRows)
    {
        if (!Row.Border)
        {
            continue;
        }

        const FName ItemID = Equipment ? Equipment->GetEquippedItem(Row.Slot) : NAME_None;
        const FItemData* Item = (!ItemID.IsNone() && Catalog) ? Catalog->FindItem(ItemID) : nullptr;

        if (!Item)
        {
            // 空槽：显示槽位名 + 占位，禁用卸下
            Row.Border->SetBrushColor(FLinearColor(0.16f, 0.17f, 0.21f, 1.0f));
            if (Row.Icon) Row.Icon->SetBrushFromTexture(nullptr);
            Row.Name->SetText(GetSlotDisplayName(Row.Slot));
            Row.Name->SetColorAndOpacity(FLinearColor(0.5f, 0.52f, 0.56f, 1.0f));
            Row.Info->SetText(FText::FromString(TEXT("（空）")));
            if (Row.BtnUnequip) Row.BtnUnequip->SetIsEnabled(false);
            continue;
        }

        // 已装备：稀有度边框 + 图标 + 名称 + 加成摘要，启用卸下
        Row.Border->SetBrushColor(UInventorySlotEntryWidget::GetRarityColor(Item->Rarity));
        if (Row.Icon && Item->Icon) Row.Icon->SetBrushFromTexture(Item->Icon);
        Row.Name->SetText(Item->DisplayName);
        Row.Name->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

        FString Info;
        struct FBonusLabel { const TCHAR* Label; float Value; };
        const FBonusLabel Bonuses[] = {
            { TEXT("攻击"), Item->AttackBonus },
            { TEXT("防御"), Item->DefenseBonus },
            { TEXT("生命"), Item->MaxHPBonus },
            { TEXT("法力"), Item->MaxMPBonus },
            { TEXT("移速"), Item->MoveSpeedBonus },
        };
        bool bFirst = true;
        for (const FBonusLabel& B : Bonuses)
        {
            if (B.Value <= 0.0f) continue;
            if (!bFirst) Info += TEXT("  ");
            Info += FString::Printf(TEXT("%s+%g"), B.Label, B.Value);
            bFirst = false;
        }
        Row.Info->SetText(FText::FromString(Info.IsEmpty() ? TEXT("无属性加成") : Info));
        if (Row.BtnUnequip) Row.BtnUnequip->SetIsEnabled(true);
    }
}

void UEquipmentWidget::RefreshAvailableList()
{
    if (!Box_Available)
    {
        return;
    }

    Box_Available->ClearChildren();
    AvailableEntries.Reset();

    UInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        return;
    }

    const UItemCatalog* Catalog = UItemCatalog::Resolve(ItemCatalog);

    // 遍历背包，列出全部可装备物品（ItemType==Equipment 且槽位有效）
    for (const FItemSlot& ItemSlot : Inventory->GetSlots())
    {
        if (ItemSlot.IsEmpty())
        {
            continue;
        }
        const FItemData* Item = Catalog ? Catalog->FindItem(ItemSlot.ItemID) : nullptr;
        if (!Item || Item->ItemType != EItemType::Equipment || Item->EquipmentSlot == EEquipmentSlot::None)
        {
            continue;
        }

        UEquipmentItemEntryWidget* Entry = CreateWidget<UEquipmentItemEntryWidget>(
            GetOwningPlayer(), UEquipmentItemEntryWidget::StaticClass());
        if (!Entry)
        {
            continue;
        }
        Entry->ItemCatalog = ItemCatalog;
        Entry->SetItem(ItemSlot.ItemID, ItemSlot.Count);
        Box_Available->AddChild(Entry);
        AvailableEntries.Add(Entry);
    }
}

void UEquipmentWidget::RefreshSummary()
{
    if (!Text_Summary)
    {
        return;
    }

    UEquipmentComponent* Equipment = GetEquipment();
    if (!Equipment)
    {
        Text_Summary->SetText(FText::FromString(TEXT("装备加成：—")));
        return;
    }

    const FString Summary = FString::Printf(
        TEXT("装备加成   攻击 +%g   防御 +%g   生命 +%g   法力 +%g   移速 +%g"),
        Equipment->GetTotalAttackBonus(),
        Equipment->GetTotalDefenseBonus(),
        Equipment->GetTotalMaxHPBonus(),
        Equipment->GetTotalMaxMPBonus(),
        Equipment->GetTotalMoveSpeedBonus());
    Text_Summary->SetText(FText::FromString(Summary));
}

void UEquipmentWidget::HandleUnequipSlot(EEquipmentSlot InSlot)
{
    UEquipmentComponent* Equipment = GetEquipment();
    if (!Equipment)
    {
        ShowAlert(FText::FromString(TEXT("找不到装备组件，无法卸下。")));
        return;
    }

    // 背包满会失败（装备保持穿戴），给用户可见提示；成功刷新由广播驱动
    if (!Equipment->UnequipItem(InSlot))
    {
        ShowAlert(FText::FromString(TEXT("卸下失败：背包已满。")));
    }
}
