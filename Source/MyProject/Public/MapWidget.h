#pragma once

#include "CoreMinimal.h"
#include "UIViewBase.h"
#include "MapWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;

/**
 * 大地图面板。
 *   - 位于 Panel 层（ZOrder 10），用 M 键或 HUD 上的地图按钮开关
 *   - 打开时暂停游戏并显示鼠标，Esc / 关闭按钮可关，关闭后缓存（再开保留状态）
 *
 * 蓝图 WBP 继承本类，控件名与下列 BindWidgetOptional 一致即可（均可缺省）：
 *   Text_Title(可选)  Image_Map(可选，放地图图片)  Btn_Close
 */
UCLASS()
class MYPROJECT_API UMapWidget : public UUIViewBase
{
    GENERATED_BODY()

public:
    UMapWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

    // ===== 绑定控件（名字需与蓝图一致，均可缺省）=====
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Title;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Map;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Close;

    UFUNCTION()
    void HandleCloseClicked();
};
