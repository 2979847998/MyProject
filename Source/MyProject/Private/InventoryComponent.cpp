#include "InventoryComponent.h"
#include "InventoryData.h"
#include "InventorySaveGame.h"
#include "BuffsComponent.h"

#include "Kismet/GameplayStatics.h"

const FString UInventoryComponent::SaveSlotName = TEXT("InventorySlot");

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 创建背包数据实例（Instanced 子对象，可在细节面板展开配置 MaxSlots / Catalog）
    InventoryData = CreateDefaultSubobject<UInventoryData>(TEXT("InventoryData"));
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // 监听数据类的变更广播，转发为组件的同名委派（蓝图绑定不变）
    if (UInventoryData* Data = GetData())
    {
        Data->OnInventoryChanged.AddDynamic(this, &UInventoryComponent::HandleInventoryChanged);
        Data->OnCoinsChanged.AddDynamic(this, &UInventoryComponent::HandleCoinsChanged);
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] BeginPlay: 数据类就绪，LoadInventory 开始"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[InventoryComponent] BeginPlay: 数据类为空! 背包将无法工作"));
    }

    LoadInventory();
    UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] BeginPlay: LoadInventory 后槽数=%d 金币=%d"),
        GetSlots().Num(), GetCoins());
}

UInventoryData* UInventoryComponent::GetData() const
{
    return InventoryData;
}

// ========== 委派转发 ==========

void UInventoryComponent::HandleInventoryChanged()
{
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::HandleCoinsChanged(int32 NewCoins)
{
    OnCoinsChanged.Broadcast(NewCoins);
}

// ========== 物品（转发数据类 + 存盘） ==========

bool UInventoryComponent::AddItem(FName ItemID, int32 Count)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] AddItem: 数据类为空! ItemID=%s"), *ItemID.ToString());
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] AddItem 调用: ItemID=%s Count=%d"),
        *ItemID.ToString(), Count);
    const bool bAllAdded = Data->AddItem(ItemID, Count);
    SaveInventory();
    return bAllAdded;
}

int32 UInventoryComponent::AddItems(const TArray<FName>& ItemIDs)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return ItemIDs.Num();
    }

    const int32 TotalRemaining = Data->AddItems(ItemIDs);
    SaveInventory();
    return TotalRemaining;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Count)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const bool bRemoved = Data->RemoveItem(ItemID, Count);
    SaveInventory();
    return bRemoved;
}

bool UInventoryComponent::RemoveSlot(int32 SlotIndex)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const bool bRemoved = Data->RemoveSlot(SlotIndex);
    if (bRemoved)
    {
        // 删除后校正拾取状态：删的是拾取中的槽 → 取消；删的是拾取槽之后的槽 → 下标前移
        if (PickedSlotIndex == SlotIndex)
        {
            PickedSlotIndex = -1;
        }
        else if (PickedSlotIndex > SlotIndex)
        {
            --PickedSlotIndex;
        }
        SaveInventory();
    }
    return bRemoved;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
    const UInventoryData* Data = GetData();
    return Data ? Data->GetItemCount(ItemID) : 0;
}

int32 UInventoryComponent::GetUsedSlots() const
{
    const UInventoryData* Data = GetData();
    return Data ? Data->GetUsedSlots() : 0;
}

void UInventoryComponent::ClearInventory()
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    Data->Clear();
    SaveInventory();
}

bool UInventoryComponent::SwapSlots(int32 IndexA, int32 IndexB)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const bool bResult = Data->SwapSlots(IndexA, IndexB);
    if (bResult)
    {
        SaveInventory();
    }
    return bResult;
}

bool UInventoryComponent::SplitStack(int32 Index, int32 Count)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const bool bResult = Data->SplitStack(Index, Count);
    if (bResult)
    {
        SaveInventory();
    }
    return bResult;
}

void UInventoryComponent::TogglePick(int32 SlotIndex)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    const TArray<FItemSlot>& Slots = Data->GetSlots();
    if (!Slots.IsValidIndex(SlotIndex))
    {
        return;
    }

    if (PickedSlotIndex < 0)
    {
        // 无拾取 → 拾取当前格
        PickedSlotIndex = SlotIndex;
        OnInventoryChanged.Broadcast(); // 触发 UI 刷新显示高亮
    }
    else if (PickedSlotIndex == SlotIndex)
    {
        // 点回同一格 → 取消拾取
        PickedSlotIndex = -1;
        OnInventoryChanged.Broadcast();
    }
    else
    {
        // 点击另一格 → 交换（替换）；数据广播会自动刷新
        Data->SwapSlots(PickedSlotIndex, SlotIndex);
        PickedSlotIndex = -1;
        SaveInventory();
    }
}

void UInventoryComponent::SplitHalf(int32 SlotIndex)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    const TArray<FItemSlot>& Slots = Data->GetSlots();
    if (!Slots.IsValidIndex(SlotIndex))
    {
        return;
    }

    const int32 Count = Slots[SlotIndex].Count;
    if (Count > 1 && Data->SplitStack(SlotIndex, Count / 2))
    {
        SaveInventory();
    }
}

void UInventoryComponent::SortInventory()
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    Data->SortInventory();
    SaveInventory();
}

const TArray<FItemSlot>& UInventoryComponent::GetSlots() const
{
    static const TArray<FItemSlot> EmptySlots;

    const UInventoryData* Data = GetData();
    return Data ? Data->GetSlots() : EmptySlots;
}

// ========== 金币（转发数据类 + 存盘） ==========

void UInventoryComponent::AddCoins(int32 Amount)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    // 示例挂接：CoinMultiplier 类 Buff 加成金币获取（2.0 = 双倍）
    float Multiplier = 1.0f;
    if (UBuffsComponent* Buffs = GetOwner() ? GetOwner()->FindComponentByClass<UBuffsComponent>() : nullptr)
    {
        Multiplier = Buffs->GetStatModifier(EBuffStat::CoinMultiplier);
    }

    Data->AddCoins(FMath::Max(1, FMath::RoundToInt(static_cast<float>(Amount) * Multiplier)));
    SaveInventory();
}

bool UInventoryComponent::UseItem(int32 SlotIndex)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const TArray<FItemSlot>& Slots = Data->GetSlots();
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || Slots[SlotIndex].Count <= 0)
    {
        return false;
    }

    // 查目录拿该物品配置的 BuffID（空 = 不可使用）
    const UItemCatalog* Catalog = UItemCatalog::Resolve(Data->Catalog);
    const FItemData* Item = Catalog ? Catalog->FindItem(Slots[SlotIndex].ItemID) : nullptr;
    if (!Item || Item->BuffID.IsNone())
    {
        return false;
    }

    // 消耗 1 个，然后触发 Buff
    if (!Data->RemoveItem(Item->ItemID, 1))
    {
        return false;
    }
    SaveInventory();

    if (UBuffsComponent* Buffs = GetOwner() ? GetOwner()->FindComponentByClass<UBuffsComponent>() : nullptr)
    {
        Buffs->AddBuff(Item->BuffID);
    }
    return true;
}

bool UInventoryComponent::SpendCoins(int32 Amount)
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return false;
    }

    const bool bSpent = Data->SpendCoins(Amount);
    SaveInventory();
    return bSpent;
}

int32 UInventoryComponent::GetCoins() const
{
    const UInventoryData* Data = GetData();
    return Data ? Data->GetCoins() : 0;
}

// ========== 存档 ==========

void UInventoryComponent::SaveInventory()
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    // 优化：所有字段都会被整体覆盖，无需先读旧档（省一次磁盘读取+反序列化）
    UInventorySaveGame* Save = Cast<UInventorySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UInventorySaveGame::StaticClass()));
    if (!Save)
    {
        return;
    }

    Save->SavedSlots = Data->GetSlots();
    // 新格式：金币已作为物品保存在 SavedSlots 中；SavedCoins 仅用于旧档迁移，保存时清零
    Save->SavedCoins = 0;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void UInventoryComponent::LoadInventory()
{
    UInventoryData* Data = GetData();
    if (!Data)
    {
        return;
    }

    UInventorySaveGame* Save = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        Save = Cast<UInventorySaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    }

    bool bMigrated = false;

    if (Save)
    {
        Data->SetSlots(Save->SavedSlots);

        // 旧存档迁移：老版本金币是独立字段（SavedCoins），并入金币物品槽
        if (Save->SavedCoins > 0)
        {
            if (Data->GetCoins() <= 0)
            {
                Data->AddCoins(Save->SavedCoins);
                bMigrated = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[Inventory] 旧存档含 %d 金币，但目录中没有金币物品（ItemID=Coin），无法迁移。"), Save->SavedCoins);
            }
        }
    }
    else
    {
        Data->SetSlots(TArray<FItemSlot>());
    }

    OnInventoryChanged.Broadcast();
    OnCoinsChanged.Broadcast(Data->GetCoins());

    // 若发生了旧档迁移，立即存盘固化（此后 SavedCoins 归零，金币以物品形式持久化）
    if (bMigrated)
    {
        SaveInventory();
    }
}
