#include "InventoryWidget.h"
#include "UIManagerComponent.h"
#include "InventoryComponent.h"
#include "InventoryEntryObject.h"
#include "InventoryGrabWidget.h"
#include "ItemCatalog.h"

#include "GameFramework/PlayerController.h"
#include "Components/TileView.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 背包：普通面板层、打开暂停、显示鼠标、Esc 可关、关闭后缓存
    Layer = EUILayer::Panel;
    bPauseGameWhenOpen = true;
    bUseUIInput = true;
    bCloseWithEsc = true;
    bCacheWhenClosed = true;
}

UInventoryComponent* UInventoryWidget::GetInventory()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        return PC->FindComponentByClass<UInventoryComponent>();
    }
    return nullptr;
}

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleCloseClicked);
    }

    if (Btn_Delete)
    {
        Btn_Delete->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleDeleteClicked);
    }

    // 绑定分类页签（蓝图没放对应按钮时跳过）。
    // 注意：AddUniqueDynamic 是宏（按编译期函数名文本处理），不能用循环变量传函数指针，
    // 必须逐个显式写出，否则运行时会断言 "xxx does not look like a member function"。
    if (Btn_Tab_All)        { Btn_Tab_All->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabAllClicked); }
    if (Btn_Tab_Weapon)     { Btn_Tab_Weapon->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabWeaponClicked); }
    if (Btn_Tab_Material)   { Btn_Tab_Material->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabMaterialClicked); }
    if (Btn_Tab_Consumable) { Btn_Tab_Consumable->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabConsumableClicked); }
    if (Btn_Tab_Misc)       { Btn_Tab_Misc->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabMiscClicked); }
    if (Btn_Tab_Quest)      { Btn_Tab_Quest->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::HandleTabQuestClicked); }

    if (UInventoryComponent* Inventory = GetInventory())
    {
        // 背包内容变化时自动刷新当前页（AddUniqueDynamic 防止缓存面板重复绑定）
        Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UInventoryWidget::HandleInventoryChanged);
    }

    // 创建抓取层（拾起物品后图标跟随鼠标），置于最高 Z 序
    if (!GrabWidget)
    {
        GrabWidget = CreateWidget<UInventoryGrabWidget>(this, UInventoryGrabWidget::StaticClass());
        if (GrabWidget)
        {
            GrabWidget->AddToViewport(100);
            GrabWidget->RefreshFromInventory();
        }
    }

    ApplyTabHighlight();
    RefreshCurrentPage();
}

void UInventoryWidget::NativeDestruct()
{
    if (GrabWidget)
    {
        GrabWidget->RemoveFromParent();
        GrabWidget = nullptr;
    }

    // 解绑跨对象委托：背包组件可能比本 Widget 活得久（PC 重生/关卡切换时 Widget 先销毁），
    // 不主动解绑会导致组件广播时访问悬垂 Widget
    if (UInventoryComponent* Inventory = GetInventory())
    {
        Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
    }

    Super::NativeDestruct();
}

// 空白处点击：正在抓取物品则取消（放回原格）。条目/按钮等子控件已 Handled，不会走到这里
FReply UInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (UInventoryComponent* Inventory = GetInventory())
        {
            if (Inventory->PickedSlotIndex >= 0)
            {
                Inventory->PickedSlotIndex = -1;
                Inventory->OnInventoryChanged.Broadcast();
                return FReply::Handled();
            }
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryWidget::OnOpen_Implementation()
{
    // 打开背包时自动排序（金币最后，稀有度降序，名字升序）
    if (UInventoryComponent* Inventory = GetInventory())
    {
        Inventory->SortInventory();
    }
    RefreshCurrentPage();
    if (GrabWidget)
    {
        GrabWidget->RefreshFromInventory();
    }
}

EItemType UInventoryWidget::TabToType(int32 TabIndex)
{
    switch (TabIndex)
    {
        case 1: return EItemType::Equipment;
        case 2: return EItemType::Material;
        case 3: return EItemType::Consumable;
        case 4: return EItemType::Misc;
        case 5: return EItemType::Quest;
        default: return EItemType::Misc;
    }
}

UTileView* UInventoryWidget::GetPageTileView(int32 TabIndex, TArray<TObjectPtr<UInventoryEntryObject>>& OutEntries)
{
    switch (TabIndex)
    {
        case 1:  OutEntries = EntryObjectsWeapon;     return TileView_Weapon;
        case 2:  OutEntries = EntryObjectsMaterial;   return TileView_Material;
        case 3:  OutEntries = EntryObjectsConsumable; return TileView_Consumable;
        case 4:  OutEntries = EntryObjectsMisc;       return TileView_Misc;
        case 5:  OutEntries = EntryObjectsQuest;      return TileView_Quest;
        default: OutEntries = EntryObjectsAll;        return TileView_All;
    }
}

void UInventoryWidget::SetFilterTab(int32 TabIndex)
{
    CurrentTabIndex = TabIndex;
    if (TabIndex >= 0 && TabIndex < WidgetSwitcher_Sections->GetNumWidgets())
    {
        WidgetSwitcher_Sections->SetActiveWidgetIndex(TabIndex);
    }
    ApplyTabHighlight();
    RefreshCurrentPage();
}

void UInventoryWidget::ApplyTabHighlight()
{
    // 选中页签金色高亮，其余恢复默认
    const FLinearColor Selected(1.0f, 0.84f, 0.0f, 1.0f);   // 金
    const FLinearColor Normal(1.0f, 1.0f, 1.0f, 1.0f);     // 白

    TArray<TObjectPtr<UButton>> Buttons = {
        Btn_Tab_All, Btn_Tab_Weapon, Btn_Tab_Material,
        Btn_Tab_Consumable, Btn_Tab_Misc, Btn_Tab_Quest
    };
    for (int32 i = 0; i < Buttons.Num(); ++i)
    {
        if (Buttons[i])
        {
            Buttons[i]->SetColorAndOpacity(i == CurrentTabIndex ? Selected : Normal);
        }
    }
}

void UInventoryWidget::HandleTabAllClicked()        { SetFilterTab(0); }
void UInventoryWidget::HandleTabWeaponClicked()    { SetFilterTab(1); }
void UInventoryWidget::HandleTabMaterialClicked()  { SetFilterTab(2); }
void UInventoryWidget::HandleTabConsumableClicked(){ SetFilterTab(3); }
void UInventoryWidget::HandleTabMiscClicked()      { SetFilterTab(4); }
void UInventoryWidget::HandleTabQuestClicked()     { SetFilterTab(5); }

void UInventoryWidget::RefreshCurrentPage()
{
    RefreshPage(CurrentTabIndex);
}

void UInventoryWidget::RefreshPage(int32 TabIndex)
{
    UInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        UE_LOG(LogTemp, Error, TEXT("[InventoryWidget] RefreshPage: 找不到 InventoryComponent!"));
        return;
    }

    TArray<TObjectPtr<UInventoryEntryObject>> Entries;
    UTileView* TileView = GetPageTileView(TabIndex, Entries);
    if (!TileView)
    {
        UE_LOG(LogTemp, Error, TEXT("[InventoryWidget] RefreshPage: 页签 %d 的 TileView 绑定为空(蓝图 Is Variable 或命名不对)!"), TabIndex);
        return;
    }

    const EItemType PageType = TabToType(TabIndex);
    Entries.Reset();

    int32 SlotIndex = 0;
    for (const FItemSlot& ItemSlot : Inventory->GetSlots())
    {
        if (ItemSlot.IsEmpty())
        {
            ++SlotIndex;
            continue;
        }
        if (TabIndex > 0)
        {
            const UItemCatalog* Catalog = UItemCatalog::GetDefaultCatalog();
            const FItemData* Data = Catalog ? Catalog->FindItem(ItemSlot.ItemID) : nullptr;
            if (!Data || Data->ItemType != PageType)
            {
                ++SlotIndex;
                continue;
            }
        }

        UInventoryEntryObject* Entry = NewObject<UInventoryEntryObject>(this);
        Entry->Init(ItemSlot.ItemID, ItemSlot.Count, SlotIndex);
        Entries.Add(Entry);
        ++SlotIndex;
    }

    // 写回对应页的缓存（防 GC）
    switch (TabIndex)
    {
        case 1:  EntryObjectsWeapon     = Entries; break;
        case 2:  EntryObjectsMaterial   = Entries; break;
        case 3:  EntryObjectsConsumable = Entries; break;
        case 4:  EntryObjectsMisc       = Entries; break;
        case 5:  EntryObjectsQuest      = Entries; break;
        default: EntryObjectsAll        = Entries; break;
    }

    TArray<UObject*> ListItems;
    ListItems.Reserve(Entries.Num());
    for (UInventoryEntryObject* Entry : Entries)
    {
        ListItems.Add(Entry);
    }

    TileView->SetListItems(ListItems);
    UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] RefreshPage: 页签=%d 条目=%d 槽数=%d"),
        TabIndex, ListItems.Num(), Inventory->GetSlots().Num());
}

void UInventoryWidget::HandleInventoryChanged()
{
    RefreshCurrentPage();
    if (GrabWidget)
    {
        GrabWidget->RefreshFromInventory();
    }
}

void UInventoryWidget::HandleCloseClicked()
{
    // 关闭前取消正在抓取的物品（放回原格），避免下次打开还带着抓取状态
    if (UInventoryComponent* Inventory = GetInventory())
    {
        if (Inventory->PickedSlotIndex >= 0)
        {
            Inventory->PickedSlotIndex = -1;
            Inventory->OnInventoryChanged.Broadcast();
        }
    }
    if (OwnerUIManager)
    {
        OwnerUIManager->ClosePanel(PanelType);
    }
    else
    {
        RemoveFromParent();
    }
}

// 删除当前选中（拾取中）的物品：整格删除，金币槽不可删
void UInventoryWidget::HandleDeleteClicked()
{
    UInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        return;
    }
    if (Inventory->PickedSlotIndex < 0)
    {
        return; // 没有选中任何物品
    }
    if (Inventory->RemoveSlot(Inventory->PickedSlotIndex))
    {
        // RemoveSlot 内部已校正 PickedSlotIndex；数据广播会自动触发 HandleInventoryChanged 刷新列表
        UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 已删除选中槽位 %d"), Inventory->PickedSlotIndex);
    }
}

// Delete 键 = 删除选中物品（等价于点击删除按钮）
FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Delete)
    {
        HandleDeleteClicked();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
