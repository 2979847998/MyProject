#include "USettingsWidget.h"
#include "USettingsKeysPage.h"
#include "UIManagerComponent.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/UserWidget.h"
#include "Delegates/DelegateCombinations.h"
#include "Engine/Engine.h"
#include "Engine/AssetManager.h"

void USettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 动态创建 3 个设置页并绑定进 WidgetSwitcher_Sections（页 0~2 = 音频/显示/按键）
    BuildPageInstances();

    // 绑定分类页签（宏要求编译期函数名，逐个显式写出；蓝图没放按钮时跳过）
    if (Btn_Tab_Audio)    { Btn_Tab_Audio->OnClicked.AddDynamic(this, &USettingsWidget::HandleTabAudioClicked); }
    if (Btn_Tab_Display)  { Btn_Tab_Display->OnClicked.AddDynamic(this, &USettingsWidget::HandleTabDisplayClicked); }
    if (Btn_Tab_Keys)     { Btn_Tab_Keys->OnClicked.AddDynamic(this, &USettingsWidget::HandleTabKeysClicked); }

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &USettingsWidget::OnCloseClicked);
    if (Button_Exit) Button_Exit->OnClicked.AddDynamic(this, &USettingsWidget::OnExitClicked);

    // 默认停在音频页
    if (WidgetSwitcher_Sections)
    {
        WidgetSwitcher_Sections->SetActiveWidgetIndex(0); 
    }
    ApplyTabHighlight();
}

// ========== 页面 UI 动态绑定 ==========

TSubclassOf<UUserWidget> USettingsWidget::ResolvePageClass(TSubclassOf<UUserWidget> Configured, const TCHAR* DefaultPath)
{
    if (Configured)
    {
        return Configured;
    }
    return LoadClass<UUserWidget>(nullptr, DefaultPath);
}

void USettingsWidget::BuildPageInstances()
{
    if (!WidgetSwitcher_Sections)
    {
        return;
    }

    struct FPageDesc
    {
        TSubclassOf<UUserWidget>& Class;
        const TCHAR* DefaultPath;
    };

    // 按页顺序收集（0=音频 1=显示 2=按键；显示页内含画质档位）
    FPageDesc Pages[] = {
        { SettingsAudioPageClass,   TEXT("/Game/UserInterface/MySettingsAudioPage.MySettingsAudioPage_C") },
        { SettingsDisplayPageClass, TEXT("/Game/UserInterface/MySettingsDisplayPage.MySettingsDisplayPage_C") },
        { SettingsKeysPageClass,    TEXT("/Game/UserInterface/MySettingsKeysPage.MySettingsKeysPage_C") },
    };

    // 清空蓝图预置的页面，统一由 C++ 按顺序重建，保证页 0~2 固定
    WidgetSwitcher_Sections->ClearChildren();

    for (const FPageDesc& Desc : Pages)
    {
        const TSubclassOf<UUserWidget> PageClass = ResolvePageClass(Desc.Class, Desc.DefaultPath);
        if (!PageClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[SettingsWidget] 页面类加载失败: %s"), Desc.DefaultPath);
            continue;
        }

        UUserWidget* Page = CreateWidget<UUserWidget>(this, PageClass);
        if (!Page)
        {
            UE_LOG(LogTemp, Error, TEXT("[SettingsWidget] 页面创建失败: %s"), *PageClass->GetName());
            continue;
        }
        WidgetSwitcher_Sections->AddChild(Page);
    }
}

void USettingsWidget::NativeDestruct()
{
    if (Btn_Tab_Audio)    { Btn_Tab_Audio->OnClicked.RemoveDynamic(this, &USettingsWidget::HandleTabAudioClicked); }
    if (Btn_Tab_Display)  { Btn_Tab_Display->OnClicked.RemoveDynamic(this, &USettingsWidget::HandleTabDisplayClicked); }
    if (Btn_Tab_Keys)     { Btn_Tab_Keys->OnClicked.RemoveDynamic(this, &USettingsWidget::HandleTabKeysClicked); }

    if (Btn_Close) Btn_Close->OnClicked.RemoveDynamic(this, &USettingsWidget::OnCloseClicked);
    if (Button_Exit) Button_Exit->OnClicked.RemoveDynamic(this, &USettingsWidget::OnExitClicked);

    Super::NativeDestruct();
}

// ========== 分类页签（WidgetSwitcher 切换：0=音频 1=显示 2=按键） ==========

void USettingsWidget::SetSettingsTab(int32 TabIndex)
{
    CurrentTabIndex = TabIndex;
    if (WidgetSwitcher_Sections && TabIndex >= 0 && TabIndex < WidgetSwitcher_Sections->GetNumWidgets())
    {
        WidgetSwitcher_Sections->SetActiveWidgetIndex(TabIndex);
    }
    ApplyTabHighlight();
}

void USettingsWidget::ApplyTabHighlight()
{
    // 选中页签金色高亮，其余恢复默认
    const FLinearColor Selected(1.0f, 0.84f, 0.0f, 1.0f);   // 金
    const FLinearColor Normal(1.0f, 1.0f, 1.0f, 1.0f);     // 白

    TArray<TObjectPtr<UButton>> Buttons = {
        Btn_Tab_Audio, Btn_Tab_Display, Btn_Tab_Keys
    };
    for (int32 i = 0; i < Buttons.Num(); ++i)
    {
        if (Buttons[i])
        {
            Buttons[i]->SetColorAndOpacity(i == CurrentTabIndex ? Selected : Normal);
        }
    }
}

void USettingsWidget::HandleTabAudioClicked()   { SetSettingsTab(0); }
void USettingsWidget::HandleTabDisplayClicked() { SetSettingsTab(1); }
void USettingsWidget::HandleTabKeysClicked()    { SetSettingsTab(2); }

// ========== 关闭按钮 ==========

void USettingsWidget::OnCloseClicked()
{
    // 1. 复位按键页的改键监听状态：残留的监听会吞掉键盘输入，导致设置页按 Esc/其它键都关不掉
    if (WidgetSwitcher_Sections)
    {
        for (int32 i = 0; i < WidgetSwitcher_Sections->GetNumWidgets(); ++i)
        {
            if (USettingsKeysPage* KeysPage = Cast<USettingsKeysPage>(WidgetSwitcher_Sections->GetWidgetAtIndex(i)))
            {
                KeysPage->ForceCancelListening();
            }
        }
    }

    // 2. 走管理器关闭：清 PanelStack + 恢复输入与暂停。
    //    直接 RemoveFromParent 会绕过 ClosePanel：面板消失但 PanelStack 里还在，
    //    导致暂停残留（关闭 UI 后游戏仍暂停）且下次 Toggle 判断错误
    if (OwnerUIManager)
    {
        OwnerUIManager->ClosePanel(PanelType);
    }
    else
    {
        RemoveFromParent();
    }
}

void USettingsWidget::OnExitClicked()
{
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
        FString::Printf(TEXT("[NativeConstruct] GameInstance=%p, Button_Exit=%p"),
            GetGameInstance(), Button_Exit.Get()) );
}
