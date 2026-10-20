#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "InventorySlotEntryWidget.h"
#include "GachaWidget.generated.h"

class UTextBlock;
class UButton;
class UWrapBox;
class UInventorySlotEntryWidget;
class UInventoryComponent;
class UGachaComponent;

/**
 * 抽卡面板：显示当前金币、单抽/十连按钮，结果在 WrapBox_Results 里用物品格子控件铺开。
 * 蓝图 WBP 继承本类，控件名（除 Result Entry Class 外均可缺省）：
 *   Text_Coins  Btn_DrawSingle  Btn_DrawTen  Btn_Close  WrapBox_Results
 * 并在类默认值把 Result Entry Class 设为物品格子 WBP（与背包 TileView 用同一个）。
 */
UCLASS()
class MYPROJECT_API UGachaWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    UGachaWidget(const FObjectInitializer& ObjectInitializer);

    // 抽卡结果里每个物品使用的格子控件类（指定 WBP_InventorySlotEntry）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gacha")
    TSubclassOf<UInventorySlotEntryWidget> ResultEntryClass;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void OnOpen_Implementation() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Coins;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_DrawSingle;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_DrawTen;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Close;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWrapBox> WrapBox_Results;

    UFUNCTION()
    void HandleDrawSingleClicked();

    UFUNCTION()
    void HandleDrawTenClicked();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleCoinsChanged(int32 NewCoins);

    UFUNCTION()
    void HandleGachaDrawn(TArray<FName> DrawnItemIDs);

    UInventoryComponent* GetInventory();
    UGachaComponent* GetGacha();

    void RefreshCoins();
    void ShowResults(const TArray<FName>& DrawnItemIDs);
};
