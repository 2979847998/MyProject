#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "EquipmentTypes.h"
#include "EquipmentWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UPanelWidget;
class UTextBlock;
class UUserWidget;
class UVerticalBox;
class UEquipmentComponent;
class UInventoryComponent;
class UItemCatalog;

class UEquipmentWidget;

/**
 * 槽位行"卸下"按钮的点击代理。
 * UButton::OnClicked 是动态多播委托，不能直接绑 lambda；
 * 用这个轻量 UObject 携带"本行是哪个槽位"的上下文（与 UKeyBindingRowProxy 同一套做法）。
 */
UCLASS()
class UEquipmentSlotRowProxy : public UObject
{
    GENERATED_BODY()

public:
    EEquipmentSlot Slot = EEquipmentSlot::None;

    // 所属装备面板（弱引用，避免循环持有）
    TWeakObjectPtr<UEquipmentWidget> Owner;

    UFUNCTION()
    void OnUnequipClicked();
};

/** 装备面板里一个槽位行的控件集合（纯 C++ 构建，运行时持有防 GC） */
USTRUCT()
struct FEquipmentSlotRow
{
    GENERATED_BODY()

    EEquipmentSlot Slot = EEquipmentSlot::None;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Border = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> Icon = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Name = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Info = nullptr;

    // 点击 = 卸下该槽位装备；空槽时禁用
    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnUnequip = nullptr;
};

/**
 * 装备面板：左侧五个槽位（武器/头盔/护甲/鞋子/饰品）显示当前穿戴，右侧列出背包里
 * 全部可装备物品，点一下即穿上、点槽位上的"卸下"即放回背包。
 *
 * 纯 C++ 动态构建控件树，无需 WBP：蓝图里只要把 Equipment 面板类直接指向本 C++ 类
 * （UIManagerComponent::PanelClasses → Equipment → UEquipmentWidget）即可使用。
 *
 * 与后端的分工（穿戴/卸下/存档/属性加成逻辑都在 UEquipmentComponent，本面板只做展示与转发）：
 *   - 穿：UEquipmentComponent::EquipItem(ItemID) —— 从背包取 1 个穿上，目标槽已有装备自动换下
 *   - 卸：UEquipmentComponent::UnequipItem(Slot) —— 放回背包
 *   - 刷新：订阅 OnEquipmentChanged（穿/卸/读档）与 OnInventoryChanged（背包增删），事件驱动
 *   - 失败提示：背包满时弹单按钮提示框，不静默失败
 */
UCLASS()
class MYPROJECT_API UEquipmentWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    UEquipmentWidget(const FObjectInitializer& ObjectInitializer);

    // 物品目录（显示名/图标/加成来源）。未配置时自动加载 /Game/UserInterface/DA_Catalog
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    TObjectPtr<UItemCatalog> ItemCatalog;

protected:
    // 建树必须早于 TakeWidget/RebuildWidget（那时才会读 WidgetTree->RootWidget），
    // 放在 NativeConstruct 里会导致首帧渲染空白，故用 NativeOnInitialized
    virtual void NativeOnInitialized() override;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 每次打开都重新拉取数据（背包/装备被其它面板改动后再打开也能同步）
    virtual void OnOpen_Implementation() override;

    // 关闭时若背包正处于"拾取中"，取消它，避免抓取层残留在屏幕上
    virtual void OnClose_Implementation() override;

    UPROPERTY(Transient)
    TObjectPtr<UPanelWidget> RootBox;

    UPROPERTY(Transient)
    TObjectPtr<UButton> Btn_Close;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Box_Slots;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Box_Available;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Text_Summary;

    // 五行槽位（按 EEquipmentSlot 顺序：武器/头盔/护甲/鞋子/饰品）
    UPROPERTY(Transient)
    TArray<FEquipmentSlotRow> SlotRows;

    // 右侧列表里的物品行（持有引用防 GC）
    UPROPERTY(Transient)
    TArray<TObjectPtr<UUserWidget>> AvailableEntries;

    // 五行槽位的点击代理（Outer 指向本 Widget，一并持有防 GC）
    UPROPERTY()
    TArray<TObjectPtr<UEquipmentSlotRowProxy>> RowProxies;

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleEquipmentChanged();

    UFUNCTION()
    void HandleInventoryChanged();

    UEquipmentComponent* GetEquipment() const;
    UInventoryComponent* GetInventory() const;

    void BuildUI();

    // 重新填充五行槽位与右侧可装备列表（统一入口，两个事件都走这里）
    void RefreshAll();

    void RefreshSlotRows();
    void RefreshAvailableList();
    void RefreshSummary();

    // 卸下某槽位（行按钮回调；失败弹提示框）
    void HandleUnequipSlot(EEquipmentSlot InSlot);

    void ShowAlert(const FText& Message) const;

    // 槽位显示名（取枚举 DisplayName，如"武器"）
    static FText GetSlotDisplayName(EEquipmentSlot Slot);

    // 是否已订阅装备/背包广播；NativeConstruct 订阅、NativeDestruct 解绑，避免重复绑定
    bool bBound = false;

    // 控件树是否已构建（BuildUI 只跑一次）
    bool bUIBuilt = false;

    // 槽位行代理需要触发 HandleUnequipSlot
    friend class UEquipmentSlotRowProxy;
};
