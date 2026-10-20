#include "UIHUDWidget.h"
#include "UIManagerComponent.h"
#include "BuffsComponent.h"
#include "BuffCatalog.h"
#include "SkillsComponent.h"
#include "SkillCatalog.h"
#include "SkillSlotEntryWidget.h"
#include "BuffSlotEntryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "InventoryComponent.h"
#include "Kismet/KismetSystemLibrary.h"

// Buff 条目显示的文本：∞ / 剩余秒数 + 层数
static FString BuildBuffInfoString(const FActiveBuff& Buff)
{
    FString Info;
    if (Buff.TimeRemaining < 0.0f)
    {
        Info = TEXT("\x221E");
    }
    else
    {
        Info = FString::Printf(TEXT("%d"), FMath::CeilToInt(Buff.TimeRemaining));
    }
    if (Buff.Stack > 1)
    {
        Info += FString::Printf(TEXT(" x%d"), Buff.Stack);
    }
    return Info;
}

UUIHUDWidget::UUIHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 常驻 HUD：最底层、不暂停、不抢鼠标、Esc 不关闭、关闭后保留缓存
    Layer = EUILayer::HUD;
    bPauseGameWhenOpen = false;
    bUseUIInput = false;
    bCloseWithEsc = false;
    bCacheWhenClosed = true;
}

void UUIHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Inventory)
    {
        Btn_Inventory->OnClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleInventoryClicked);
    }
    if (Btn_Map)
    {
        Btn_Map->OnClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleMapClicked);
    }
    if (Btn_Settings)
    {
        Btn_Settings->OnClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleSettingsClicked);
    }
    if (Btn_Quit)
    {
        Btn_Quit->OnClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleQuitClicked);
    }
    if (Btn_Gacha)
    {
        Btn_Gacha->OnClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleGachaClicked);
    }

    // 监听金币变化
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UInventoryComponent* Inventory = PC->FindComponentByClass<UInventoryComponent>())
        {
            Inventory->OnCoinsChanged.AddUniqueDynamic(this, &UUIHUDWidget::HandleCoinsChanged);
        }

        // 监听 Buff 变化，刷新图标栏
        if (UBuffsComponent* Buffs = PC->FindComponentByClass<UBuffsComponent>())
        {
            Buffs->OnBuffsChanged.AddUniqueDynamic(this, &UUIHUDWidget::HandleBuffsChanged);
        }

        // 监听技能变化，刷新技能栏与能量条
        if (USkillsComponent* Skills = PC->FindComponentByClass<USkillsComponent>())
        {
            Skills->OnSkillsChanged.AddUniqueDynamic(this, &UUIHUDWidget::HandleSkillsChanged);
        }
    }
    RefreshCoins();
    RefreshBuffBar();
    RefreshSkillBar();
    RefreshEnergy();
}

void UUIHUDWidget::NativeDestruct()
{
    // 解绑跨对象委托：背包/Buff/技能组件挂在 PC 上，可能比本 HUD 活得久
    // （关卡切换时 PC 重生、旧 HUD 销毁），不主动解绑会导致组件广播时访问悬垂 Widget
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UInventoryComponent* Inventory = PC->FindComponentByClass<UInventoryComponent>())
        {
            Inventory->OnCoinsChanged.RemoveDynamic(this, &UUIHUDWidget::HandleCoinsChanged);
        }
        if (UBuffsComponent* Buffs = PC->FindComponentByClass<UBuffsComponent>())
        {
            Buffs->OnBuffsChanged.RemoveDynamic(this, &UUIHUDWidget::HandleBuffsChanged);
        }
        if (USkillsComponent* Skills = PC->FindComponentByClass<USkillsComponent>())
        {
            Skills->OnSkillsChanged.RemoveDynamic(this, &UUIHUDWidget::HandleSkillsChanged);
        }
    }

    Super::NativeDestruct();
}

void UUIHUDWidget::HandleInventoryClicked()
{
    if (OwnerUIManager)
    {
        OwnerUIManager->TogglePanel(EUIPanelType::Inventory);
    }
}

void UUIHUDWidget::HandleMapClicked()
{
    if (OwnerUIManager)
    {
        OwnerUIManager->TogglePanel(EUIPanelType::Map);
    }
}

void UUIHUDWidget::HandleSettingsClicked()
{
    if (OwnerUIManager)
    {
        OwnerUIManager->TogglePanel(EUIPanelType::Settings);
    }
}

void UUIHUDWidget::HandleQuitClicked()
{
    if (!OwnerUIManager)
    {
        return;
    }

    // 先弹确认框，确认后才真正退出
    UUIConfirmDialog* Dialog = OwnerUIManager->ShowConfirmDialog(
        FText::FromString(TEXT("提示")),
        FText::FromString(TEXT("确定要退出游戏吗？")),
        true,
        FText::FromString(TEXT("退出")),
        FText::FromString(TEXT("取消")));

    if (Dialog)
    {
        Dialog->OnDialogResult.AddDynamic(this, &UUIHUDWidget::HandleQuitDialogResult);
    }
}

void UUIHUDWidget::HandleQuitDialogResult(EDialogResult Result)
{
    if (Result != EDialogResult::Confirmed)
    {
        return;
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
    }
}

void UUIHUDWidget::HandleGachaClicked()
{
    if (OwnerUIManager)
    {
        OwnerUIManager->TogglePanel(EUIPanelType::Gacha);
    }
}

void UUIHUDWidget::HandleCoinsChanged(int32 NewCoins)
{
    if (Text_Coins)
    {
        Text_Coins->SetText( FText::FromString( TEXT(" Coins = ") + FString::FormatAsNumber( NewCoins)));
    }
}

void UUIHUDWidget::HandleBuffsChanged()
{
    RefreshBuffBar();
}

void UUIHUDWidget::HandleSkillsChanged()
{
    RefreshSkillBar();
    RefreshEnergy();
}

void UUIHUDWidget::HandleSkillSlotClicked(FName SkillID)
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (USkillsComponent* Skills = PC->FindComponentByClass<USkillsComponent>())
        {
            Skills->CastSkill(SkillID);
        }
    }
}

void UUIHUDWidget::RefreshBuffBar()
{
    if (!Box_Buffs)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    UBuffsComponent* Buffs = PC ? PC->FindComponentByClass<UBuffsComponent>() : nullptr;
    if (!Buffs)
    {
        Box_Buffs->ClearChildren();
        return;
    }

    UBuffCatalog* Catalog = UBuffCatalog::Resolve(nullptr);
    if (!Catalog)
    {
        Box_Buffs->ClearChildren();
        return;
    }

    const TArray<FActiveBuff>& Active = Buffs->GetActiveBuffs();
    const int32 Existing = Box_Buffs->GetChildrenCount();

    // 数量变化（首次构建/增删 Buff）才重建；否则原位刷新文本，避免每秒 CreateWidget+GC
    if (Existing != Active.Num())
    {
        Box_Buffs->ClearChildren();

        UClass* BuffClass = BuffSlotClass ? BuffSlotClass.Get() : nullptr;
        for (const FActiveBuff& ActiveBuff : Active)
        {
            const FBuffData* Data = Catalog->FindBuff(ActiveBuff.BuffID);
            if (!Data)
            {
                continue;
            }

            if (BuffClass)
            {
                UBuffSlotEntryWidget* Entry = CreateWidget<UBuffSlotEntryWidget>(GetOwningPlayer(), BuffClass);
                if (Entry)
                {
                    Entry->SetupData(Data->Icon, BuildBuffInfoString(ActiveBuff));
                    Box_Buffs->AddChildToHorizontalBox(Entry);
                }
            }
        }
        return;
    }

    int32 Index = 0;
    for (const FActiveBuff& ActiveBuff : Active)
    {
        const FBuffData* Data = Catalog->FindBuff(ActiveBuff.BuffID);
        if (!Data)
        {
            continue;
        }
        if (UBuffSlotEntryWidget* Entry = Cast<UBuffSlotEntryWidget>(Box_Buffs->GetChildAt(Index)))
        {
            Entry->UpdateData(Data->Icon, BuildBuffInfoString(ActiveBuff));
        }
        ++Index;
    }
}

void UUIHUDWidget::RefreshSkillBar()
{
    if (!Box_Skills)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    USkillsComponent* Skills = PC ? PC->FindComponentByClass<USkillsComponent>() : nullptr;
    if (!Skills)
    {
        return;
    }

    USkillCatalog* Catalog = USkillCatalog::Resolve(nullptr);
    if (!Catalog)
    {
        return;
    }

    // 技能目录是静态数据资产：子控件数量一致说明无需重建。
    // 冷却/能量显示由各 SkillSlotEntryWidget 自刷新，这里不再每秒销毁重建整条技能栏。
    if (Box_Skills->GetChildrenCount() == Catalog->Skills.Num())
    {
        return;
    }

    Box_Skills->ClearChildren();

    UClass* SlotClass = SkillSlotClass ? SkillSlotClass.Get() : USkillSlotEntryWidget::StaticClass();
    for (const FSkillData& SkillData : Catalog->Skills)
    {
        USkillSlotEntryWidget* Entry = CreateWidget<USkillSlotEntryWidget>(PC, SlotClass);
        if (!Entry)
        {
            continue;
        }
        Entry->Setup(SkillData.SkillID);
        Entry->OnSlotClicked.AddUniqueDynamic(this, &UUIHUDWidget::HandleSkillSlotClicked);
        Box_Skills->AddChildToHorizontalBox(Entry);
    }
}

void UUIHUDWidget::RefreshEnergy()
{
    if (!EnergyBar)
    {
        return;
    }

    float Percent = 0.0f;
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (USkillsComponent* Skills = PC->FindComponentByClass<USkillsComponent>())
        {
            Percent = Skills->GetEnergyPercent();
        }
    }
    EnergyBar->SetPercent(Percent);
}

void UUIHUDWidget::RefreshCoins()
{
    if (!Text_Coins)
    {
        return;
    }

    int32 Coins = 0;
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UInventoryComponent* Inventory = PC->FindComponentByClass<UInventoryComponent>())
        {
            Coins = Inventory->GetCoins();
        }
    }
    HandleCoinsChanged(Coins);
}
