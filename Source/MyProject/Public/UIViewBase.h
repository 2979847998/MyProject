#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIViewBase.generated.h"

class UUIManagerComponent;

/**
 * UI 面板类型枚举。
 * 新增一个面板时：在这里加一个枚举值，并在 UIManagerComponent 的 PanelClasses 里配置对应蓝图类。
 */
UENUM(BlueprintType)
enum class EUIPanelType : uint8
{
    None       UMETA(Hidden),
    Inventory,     // 背包
    Settings,      // 设置
    Map,           // 大地图
    ConfirmDialog, // 通用确认/取消弹窗（Popup 层）
    // 追加在末尾，避免打乱蓝图 PanelClasses 已序列化的枚举值
    HUD,           // 常驻主 HUD / 主管理界面（HUD 层，进游戏自动打开）
    Gacha,         // 抽卡面板
    Equipment,     // 装备面板（穿/卸装备）
};

/**
 * 屏幕分层。决定面板 AddToViewport 时的 ZOrder，数值越大越靠前。
 */
UENUM(BlueprintType)
enum class EUILayer : uint8
{
    HUD,      // 常驻 HUD：血条、准星、金币栏
    Panel,    // 菜单面板：背包、设置、地图
    Popup,    // 弹窗：确认框、数量输入
    Tooltip,  // 悬浮提示：物品 Tooltip（始终盖在面板之上）
    Top,      // 最顶层：Loading、黑屏、全局飘字
};

/**
 * 所有 UI 面板的基类。
 * 蓝图面板继承本类后，在“类默认值”里配置分层、是否暂停、是否 Esc 关闭等元数据，
 * 并按需重写 OnOpen / OnClose / OnShow / OnHide 事件。
 */
UCLASS()
class MYPROJECT_API UUIViewBase : public UUserWidget
{
    GENERATED_BODY()

public:
    // ===== 面板元数据（在蓝图默认值里配置）=====

    // 构造：默认可聚焦，便于面板打开后接收键盘做按键兜底
    UUIViewBase(const FObjectInitializer& ObjectInitializer);

    // 按键兜底：按设置里的实际绑定键处理开/关（设置页豁免，KeysPage 自处理监听）
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // 所属分层，决定 ZOrder
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|View")
    EUILayer Layer = EUILayer::Panel;

    // 打开时是否暂停游戏
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|View")
    bool bPauseGameWhenOpen = false;

    // 关闭后是否缓存对象（true：再开秒开并保留状态；false：关闭即销毁）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|View")
    bool bCacheWhenClosed = true;

    // 是否响应 Esc 关闭
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|View")
    bool bCloseWithEsc = true;

    // 打开时是否显示鼠标并切换到“游戏 + UI”输入模式
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|View")
    bool bUseUIInput = true;

    // ===== 生命周期事件（蓝图可重写）=====

    // 每次打开时调用（适合做数据刷新）
    UFUNCTION(BlueprintNativeEvent, Category = "UI|View")
    void OnOpen();
    virtual void OnOpen_Implementation() {}

    // 每次关闭时调用
    UFUNCTION(BlueprintNativeEvent, Category = "UI|View")
    void OnClose();
    virtual void OnClose_Implementation() {}

    // 显示到屏幕时调用
    UFUNCTION(BlueprintNativeEvent, Category = "UI|View")
    void OnShow();
    virtual void OnShow_Implementation() {}

    // 从屏幕移除/隐藏时调用
    UFUNCTION(BlueprintNativeEvent, Category = "UI|View")
    void OnHide();
    virtual void OnHide_Implementation() {}

    UFUNCTION(BlueprintPure, Category = "UI|View")
    EUIPanelType GetPanelType() const { return PanelType; }

    UFUNCTION(BlueprintPure, Category = "UI|View")
    UUIManagerComponent* GetUIManager() const { return OwnerUIManager; }

protected:
    // 所属面板类型，由 UIManagerComponent 创建时注入
    UPROPERTY(BlueprintReadOnly, Category = "UI|View")
    EUIPanelType PanelType = EUIPanelType::None;

    // 所属管理器，由 UIManagerComponent 创建时注入
    UPROPERTY(BlueprintReadOnly, Category = "UI|View")
    TObjectPtr<UUIManagerComponent> OwnerUIManager;

    friend class UUIManagerComponent;
};
