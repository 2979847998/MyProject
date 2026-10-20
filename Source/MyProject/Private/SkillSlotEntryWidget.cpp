#include "SkillSlotEntryWidget.h"
#include "SkillCatalog.h"
#include "SkillsComponent.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

USkillSlotEntryWidget::USkillSlotEntryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USkillSlotEntryWidget::Setup(FName InSkillID)
{
    SkillID = InSkillID;

    if (USkillCatalog* Catalog = USkillCatalog::GetDefaultCatalog())
    {
        if (UTexture2D* Icon = Catalog->GetIcon(SkillID))
        {
            if (Image_Icon)
            {
                Image_Icon->SetBrushFromTexture(Icon);
            }
        }
        else
        {
            // 无图标时占位
            if (Text_Cooldown)
            {
                FText Name = Catalog->GetDisplayName(SkillID);
                Text_Cooldown->SetText(Name);
            }
        }

        FText Desc = Catalog->GetDescription(SkillID);
        if (!Desc.IsEmpty())
        {
            SetToolTipText(Desc);
        }
    }

    RefreshDisplay();
}

void USkillSlotEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshDisplay();
}

FReply USkillSlotEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OnSlotClicked.Broadcast(SkillID);
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USkillSlotEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshDisplay();
}

void USkillSlotEntryWidget::RefreshDisplay()
{
    if (SkillID.IsNone())
    {
        return;
    }

    // 缓存宿主组件（Tick 里不做 FindComponentByClass）
    if (!CachedSkills.IsValid())
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            CachedSkills = PC->FindComponentByClass<USkillsComponent>();
        }
    }
    USkillsComponent* Skills = CachedSkills.Get();
    if (!Skills)
    {
        return;
    }

    // 冷却中：显示剩余秒数（只在整数秒变化时更新控件，避免每帧 SetText 触发重绘）
    const float Remaining = Skills->GetCooldownRemaining(SkillID);
    const int32 DisplaySeconds = FMath::CeilToInt(FMath::Max(0.0f, Remaining));
    if (DisplaySeconds != LastDisplayedSeconds)
    {
        LastDisplayedSeconds = DisplaySeconds;
        if (Text_Cooldown)
        {
            if (DisplaySeconds > 0)
            {
                Text_Cooldown->SetVisibility(ESlateVisibility::Visible);
                Text_Cooldown->SetText(FText::FromString(FString::Printf(TEXT("%d"), DisplaySeconds)));
                if (Image_Icon)
                {
                    Image_Icon->SetRenderOpacity(0.5f);
                }
            }
            else
            {
                Text_Cooldown->SetVisibility(ESlateVisibility::Hidden);
                if (Image_Icon)
                {
                    Image_Icon->SetRenderOpacity(1.0f);
                }
            }
        }
    }

    // 能量不足：暗红色边框（能量消耗缓存后仅状态翻转时更新颜色）
    if (Border_Base)
    {
        if (CachedEnergyCost < 0.0f)
        {
            if (USkillCatalog* Catalog = USkillCatalog::GetDefaultCatalog())
            {
                if (const FSkillData* Data = Catalog->FindSkill(SkillID))
                {
                    CachedEnergyCost = Data->EnergyCost;
                }
            }
            if (CachedEnergyCost < 0.0f)
            {
                CachedEnergyCost = 0.0f;
            }
        }

        const bool bEnough = Skills->GetEnergy() >= CachedEnergyCost;
        const int8 EnoughState = bEnough ? 1 : 0;
        if (EnoughState != LastEnoughEnergy)
        {
            LastEnoughEnergy = EnoughState;
            Border_Base->SetBrushColor(bEnough
                ? FLinearColor(0.10f, 0.12f, 0.16f, 0.95f)
                : FLinearColor(0.45f, 0.12f, 0.12f, 0.95f));
        }
    }
}
