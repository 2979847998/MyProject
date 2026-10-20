#include "BuffSlotEntryWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UBuffSlotEntryWidget::UBuffSlotEntryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UBuffSlotEntryWidget::SetupData(UTexture2D* Icon, const FString& Text)
{
    if (Image_Icon)
    {
        if (Icon)
        {
            Image_Icon->SetBrushFromTexture(Icon);
        }
        Image_Icon->SetDesiredSizeOverride(FVector2D(48.0f, 48.0f));
    }

    if (Text_Count)
    {
        Text_Count->SetText(FText::FromString(Text));
    }

    if (Border_Base)
    {
        Border_Base->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.95f));
    }
}

void UBuffSlotEntryWidget::UpdateData(UTexture2D* Icon, const FString& Text)
{
    // 原位刷新：只更新会变化的内容（图标/倒计时文本），布局与边框保持初始状态
    if (Image_Icon && Icon)
    {
        Image_Icon->SetBrushFromTexture(Icon);
    }
    if (Text_Count)
    {
        Text_Count->SetText(FText::FromString(Text));
    }
}
