#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCatalog.h"
#include "EquipmentItemEntryWidget.generated.h"

class UButton;
class UBorder;
class UImage;
class UTextBlock;
class UEquipmentComponent;

/**
 * 装备面板里"背包可装备物品"列表的一行。
 *
 * 纯 C++ 动态构建（无需 WBP），由 UEquipmentWidget 创建后调用 SetItem 填充：
 *   - 显示：图标 + 物品名 + 目标槽位与加成摘要 + 持有数量，边框按稀有度着色
 *   - 交互：左键点击 = 穿上这件装备（目标槽已有装备时由 EquipmentComponent 自动换下回背包）
 *   - 失败（背包满 / 非法物品）时弹出提示框，不会静默失败
 */
UCLASS()
class MYPROJECT_API UEquipmentItemEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 填充本行数据（ItemID 必须是 UItemCatalog 中 ItemType == Equipment 的行）
    void SetItem(FName InItemID, int32 InCount);

    // 物品目录（显示用）。未配置时自动加载 /Game/UserInterface/DA_Catalog
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    TObjectPtr<UItemCatalog> ItemCatalog;

protected:
    // 建树必须早于 TakeWidget（否则 Slate 拿到空 RootWidget，面板渲染为空白），
    // 因此放在 NativeOnInitialized（CreateWidget 期间就会调用）
    virtual void NativeOnInitialized() override;

    virtual void NativeConstruct() override;

    // 整行按钮：点击即穿戴
    UPROPERTY(Transient)
    TObjectPtr<UButton> Btn_Root;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Border_Rarity;

    UPROPERTY(Transient)
    TObjectPtr<UImage> Image_Icon;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Text_Name;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Text_Info;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Text_Count;

    UFUNCTION()
    void HandleClicked();

    UEquipmentComponent* GetEquipment() const;

    // 弹出单按钮提示框（背包满等失败场景）
    void ShowAlert(const FText& Message) const;

private:
    // 构建行内控件树。SetItem 与 NativeConstruct 都会调用，bUIBuilt 保证只建一次
    // （二者先后顺序取决于 Widget 何时进入活跃树，不能只依赖其中一个）
    void EnsureBuilt();

    // 用已存的 ItemID/Count 填充控件（需先 EnsureBuilt）
    void ApplyData();

    bool bUIBuilt = false;

    FName ItemID;
    int32 Count = 0;
};
