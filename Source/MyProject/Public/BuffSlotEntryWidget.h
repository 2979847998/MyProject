#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuffSlotEntryWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

/**
 * Buff 图标槽（HUD 上显示当前生效的 buff）。
 * 蓝图 WBP 继承本类，控件名需匹配：
 *   - Border_Base   ：外框（可选）
 *   - Image_Icon    ：Buff 图标
 *   - Text_Count    ：剩余秒数 / 层数
 */
UCLASS()
class MYPROJECT_API UBuffSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UBuffSlotEntryWidget(const FObjectInitializer& ObjectInitializer);

    // 填充数据（首次构建时调用）
    UFUNCTION(BlueprintCallable, Category = "Buff")
    void SetupData(UTexture2D* Icon, const FString& Text);

    // 原位刷新（每秒更新倒计时文本，复用已建控件避免 CreateWidget/GC 抖动）
    void UpdateData(UTexture2D* Icon, const FString& Text);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UBorder> Border_Base;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Icon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Count;
};
