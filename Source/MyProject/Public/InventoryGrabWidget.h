#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGrabWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UImage;
class UTextBlock;
class UInventoryComponent;
struct FItemSlot;

/**
 * 背包“抓取”层：拾起物品后图标跟随鼠标移动，点击目标格子交换/放下。
 * 纯 C++ 动态构建（无需蓝图），由 UInventoryWidget 创建并驱动刷新：
 *   - 显示：背包组件 PickedSlotIndex >= 0 时，显示该槽位图标 + 数量 + 稀有度边框
 *   - 跟随：NativeTick 里取鼠标位置并 SetPositionInViewport
 *   - 隐藏：PickedSlotIndex == -1 时 Collapsed（取消拾取/交换完成后由刷新触发）
 */
UCLASS()
class MYPROJECT_API UInventoryGrabWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 依据背包组件当前拾取槽刷新抓取层（显示/隐藏 + 内容）
    void RefreshFromInventory();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UInventoryComponent* GetInventory();

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> BorderFrame;

    UPROPERTY(Transient)
    TObjectPtr<UImage> ImageIcon;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TextCount;

    // 图标跟随时略透明，避免完全遮挡格子
    static constexpr float GrabOpacity = 0.85f;
};
