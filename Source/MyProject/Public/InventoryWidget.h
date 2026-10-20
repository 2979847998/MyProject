#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "ItemCatalog.h"
#include "ItemSlot.h"
#include "InventoryWidget.generated.h"

class UTileView;
class UButton;
class UWidgetSwitcher;
class UInventoryComponent;
class UInventoryEntryObject;
class UInventoryGrabWidget;

/**
 * 背包面板：页签 + WidgetSwitcher 切换分类列表。
 * 蓝图 WBP 继承本类：
 *   - 放一个 WidgetSwitcher 命名 WidgetSwitcher_Sections，内含 6 个页面（全部/武器/材料/消耗品/杂物/任务道具），
 *     每页放一个 TileView（命名 TileView_All / TileView_Weapon / TileView_Material /
 *     TileView_Consumable / TileView_Misc / TileView_Quest），把各自的 Entry Widget Class 设为物品格子 WBP
 *   - 放分类页签按钮（可缺省）：Btn_Tab_All / Btn_Tab_Weapon / Btn_Tab_Material /
 *     Btn_Tab_Consumable / Btn_Tab_Misc / Btn_Tab_Quest（按钮文字在蓝图里写）
 *   - 放关闭按钮 Btn_Close（可缺省）
 *
 * 格子交互（由物品格子 WBP 直接驱动组件，本类只负责数据刷新）：
 *   - 左键点击 = 拾取（金色高亮）/ 点另一格交换替换 / 点回原格取消
 *   - 右键点击 = 拆出一半到新格（分堆）
 */
UCLASS()
class MYPROJECT_API UInventoryWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    UInventoryWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 空白处点击：抓取中则取消抓取（放回原格）
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,\
        const FPointerEvent& InMouseEvent) override;

    // 每次打开都拉取最新数据（抽卡/其它面板改动后再打开也能同步）
    virtual void OnOpen_Implementation() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Sections;

    // 六个分类列表页（0=全部 1=武器 2=材料 3=消耗品 4=杂物 5=任务道具）
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_All;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_Weapon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_Material;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_Consumable;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_Misc;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTileView> TileView_Quest;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Close;

    // 删除当前选中（拾取中）物品的按钮
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Delete;

    // 抓取层：拾起物品后图标跟随鼠标（独立 UI 类，纯 C++ 构建）
    UPROPERTY(Transient)
    TObjectPtr<UInventoryGrabWidget> GrabWidget;

    // ===== 分类页签（0=全部 1=武器 2=材料 3=消耗品 4=杂物 5=任务道具） =====

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_All;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Weapon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Material;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Consumable;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Misc;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Tab_Quest;

    UFUNCTION()
    void HandleTabAllClicked();

    UFUNCTION()
    void HandleTabWeaponClicked();

    UFUNCTION()
    void HandleTabMaterialClicked();

    UFUNCTION()
    void HandleTabConsumableClicked();

    UFUNCTION()
    void HandleTabMiscClicked();

    UFUNCTION()
    void HandleTabQuestClicked();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleDeleteClicked();

    // Delete 键快捷删除选中物品（背包面板内生效）
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    UFUNCTION()
    void HandleInventoryChanged();

    // 每个分类页的条目缓存（各自持有，防止 GC；全部页缓存所有非空槽）
    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsAll;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsWeapon;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsConsumable;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsMisc;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInventoryEntryObject>> EntryObjectsQuest;

    UInventoryComponent* GetInventory();

    // 刷新当前页签对应的列表（切换页签/内容变化/打开面板时调用）
    void RefreshCurrentPage();

    // 刷新指定页签的列表
    void RefreshPage(int32 TabIndex);

    // 页签下标 -> 物品类型（1=武器 2=材料 3=消耗品 4=杂物 5=任务道具；0 不参与）
    static EItemType TabToType(int32 TabIndex);

    // 取某页的 TileView 与条目缓存（TabIndex 越界返回 nullptr / 空引用）
    UTileView* GetPageTileView(int32 TabIndex, \
        TArray<TObjectPtr<UInventoryEntryObject>>& OutEntries);

    // 当前页签（0=全部），切换后同步 WidgetSwitcher 与高亮
    int32 CurrentTabIndex = 0;

    // 切换分类页签并刷新
    void SetFilterTab(int32 TabIndex);

    // 遍历页签按钮，把当前 Tab 设为金色高亮，其余恢复默认
    void ApplyTabHighlight();
};
