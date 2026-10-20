#include "InventoryData.h"

const FName UInventoryData::CoinItemID = TEXT("Coin");

UInventoryData::UInventoryData()
{
    
}

// ========== 目录查询 ==========

const FItemData* UInventoryData::GetItemData(FName ItemID) const
{
    UItemCatalog* CatalogResolved = UItemCatalog::Resolve(Catalog);
    return CatalogResolved ? CatalogResolved->FindItem(ItemID) : nullptr;
}

// ========== 物品 ==========

bool UInventoryData::AddItemInternal(FName ItemID, int32 Count, int32& OutRemaining)
{
    OutRemaining = Count;

    const FItemData* Item = GetItemData(ItemID);
    if (!Item || Count <= 0)
    {   
        UE_LOG(LogTemp, Warning, TEXT("[InventoryData] AddItemInternal 拒绝: ItemID=%s Count=%d Catalog=%s"),
            *ItemID.ToString(), Count, Catalog ? *Catalog->GetPathName() : TEXT("NULL(走默认目录)"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[InventoryData] AddItemInternal: ItemID=%s Count=%d MaxStack=%d 开始入包"),
        *ItemID.ToString(), Count, Item->MaxStack);

    const int32 MaxStack = Item->MaxStack;
     
    // 1. 优先堆叠到已有的、没满的同类堆
    for (FItemSlot& Slot : Slots)
    {
        if (OutRemaining <= 0)
        {
            break;
        }
        if (Slot.ItemID == ItemID && Slot.Count < MaxStack)
        {
            const int32 CanAdd = FMath::Min(OutRemaining, MaxStack - Slot.Count);
            Slot.Count += CanAdd;
            OutRemaining -= CanAdd;
        }
    }

    // 2. 剩余的放进新格子
    while (OutRemaining > 0)
    {
        if (Slots.Num() >= MaxSlots)
        {
            return false; // 背包已满，OutRemaining 仍 > 0
        }

        FItemSlot NewSlot;
        NewSlot.ItemID = ItemID;
        const int32 Take = FMath::Min(OutRemaining, MaxStack);
        NewSlot.Count = Take;
        Slots.Add(NewSlot);
        OutRemaining -= Take;
    }

    UE_LOG(LogTemp, Warning, TEXT("[InventoryData] AddItemInternal 完成: ItemID=%s 剩余未入包=%d 当前槽数=%d"),
        *ItemID.ToString(), OutRemaining, Slots.Num());
    return true;
}

bool UInventoryData::AddItem(FName ItemID, int32 Count)
{
    int32 Remaining = 0;
    const bool bAllAdded = AddItemInternal(ItemID, Count, Remaining);

    OnInventoryChanged.Broadcast();
    return bAllAdded;
}

int32 UInventoryData::AddItems(const TArray<FName>& ItemIDs)
{
    int32 TotalRemaining = 0;

    for (const FName& ItemID : ItemIDs)
    {
        int32 Remaining = 0;
        AddItemInternal(ItemID, 1, Remaining);
        TotalRemaining += Remaining;
    }

    OnInventoryChanged.Broadcast();
    return TotalRemaining;
}

bool UInventoryData::RemoveItem(FName ItemID, int32 Count)
{
    if (ItemID.IsNone() || Count <= 0)
    {
        return false;
    }

    // 数量不足则不做任何改动
    if (GetItemCount(ItemID) < Count)
    {
        return false;
    }

    int32 Remaining = Count;
    for (int32 i = Slots.Num() - 1; i >= 0 && Remaining > 0; --i)
    {
        if (Slots[i].ItemID != ItemID)
        {
            continue;
        }

        const int32 Taken = FMath::Min(Remaining, Slots[i].Count);
        Slots[i].Count -= Taken;
        Remaining -= Taken;

        if (Slots[i].Count <= 0)
        {
            Slots.RemoveAt(i);
        }
    }

    OnInventoryChanged.Broadcast();
    return true;
}

bool UInventoryData::RemoveSlot(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    // 金币是货币槽，不允许玩家手动删除
    if (Slots[SlotIndex].ItemID == CoinItemID)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryData] RemoveSlot: 拒绝删除金币槽"));
        return false;
    }

    Slots.RemoveAt(SlotIndex);
    OnInventoryChanged.Broadcast();
    return true;
}

int32 UInventoryData::GetItemCount(FName ItemID) const
{
    if (ItemID.IsNone())
    {
        return 0;
    }

    int32 Total = 0;
    for (const FItemSlot& Slot : Slots)
    {
        if (Slot.ItemID == ItemID)
        {
            Total += Slot.Count;
        }
    }
    return Total;
}

void UInventoryData::Clear()
{
    if (Slots.Num() == 0)
    {
        return;
    }

    Slots.Reset();
    OnInventoryChanged.Broadcast();
}

bool UInventoryData::SwapSlots(int32 IndexA, int32 IndexB)
{
    if (IndexA == IndexB)
    {
        return false;
    }
    if (!Slots.IsValidIndex(IndexA) || !Slots.IsValidIndex(IndexB))
    {
        return false;
    }

    ::Swap(Slots[IndexA], Slots[IndexB]);
    OnInventoryChanged.Broadcast();
    return true;
}

bool UInventoryData::SplitStack(int32 Index, int32 Count)
{
    if (Count <= 0)
    {
        return false;
    }
    if (!Slots.IsValidIndex(Index))
    {
        return false;
    }
    // 必须留至少 1 个在原堆，否则用不上分堆
    if (Slots[Index].Count <= Count)
    {
        return false;
    }
    if (Slots.Num() >= MaxSlots)
    {
        return false;
    }

    FItemSlot NewSlot;
    NewSlot.ItemID = Slots[Index].ItemID;
    NewSlot.Count = Count;
    Slots[Index].Count -= Count;
    Slots.Add(NewSlot);

    OnInventoryChanged.Broadcast();
    return true;
}

void UInventoryData::SortInventory()
{
    if (Slots.Num() <= 1)
    {
        return;
    }

    Slots.Sort([this](const FItemSlot& A, const FItemSlot& B)
    {
        // 金币槽固定排最后（货币不参与物品排序）
        const bool bACoin = A.ItemID == CoinItemID;
        const bool bBCoin = B.ItemID == CoinItemID;
        if (bACoin != bBCoin)
        {
            return !bACoin;
        }
        if (A.ItemID == B.ItemID)
        {
            return false;
        }

        // 稀有度：传说 > 史诗 > 稀有 > 普通
        const FItemData* DA = GetItemData(A.ItemID);
        const FItemData* DB = GetItemData(B.ItemID);
        const int32 RA = DA ? static_cast<int32>(DA->Rarity) : 0;
        const int32 RB = DB ? static_cast<int32>(DB->Rarity) : 0;
        if (RA != RB)
        {
            return RA > RB;
        }

        // 同稀有度按显示名升序（走本地化查询；目录没配名字则退回用 ItemID）
        const FString NA = DA ? UItemCatalog::Resolve(Catalog)->GetDisplayName(A.ItemID).ToString() : A.ItemID.ToString();
        const FString NB = DB ? UItemCatalog::Resolve(Catalog)->GetDisplayName(B.ItemID).ToString() : B.ItemID.ToString();
        if (NA != NB)
        {
            return NA < NB;
        }

        // 完全同名按 ItemID 字典序
        return A.ItemID.LexicalLess(B.ItemID);
    });

    OnInventoryChanged.Broadcast();
}

// ========== 金币（ItemID == "Coin" 的背包物品） ==========

void UInventoryData::AddCoins(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    // 目录里没有金币行则无法操作
    if (!GetItemData(CoinItemID))
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryData] 目录（DA_Catalog）中没有金币物品行（ItemID=Coin），无法添加金币 %d。"), Amount);
        return;
    }

    // 金币作为背包物品：优先累加到已有的金币槽（货币不按 MaxStack 拆堆）
    for (FItemSlot& Slot : Slots)
    {
        if (Slot.ItemID == CoinItemID)
        {
            Slot.Count += Amount;
            OnInventoryChanged.Broadcast();
            OnCoinsChanged.Broadcast(Slot.Count);
            return;
        }
    }

    // 背包里还没有金币槽：新建一个（占用一格）
    if (Slots.Num() >= MaxSlots)
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryData] 背包已满，无法放入金币。"));
        return;
    }

    FItemSlot CoinSlot;
    CoinSlot.ItemID = CoinItemID;
    CoinSlot.Count = Amount;
    Slots.Add(CoinSlot);

    OnInventoryChanged.Broadcast();
    OnCoinsChanged.Broadcast(Amount);
}

bool UInventoryData::SpendCoins(int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    if (!GetItemData(CoinItemID) || GetCoins() < Amount)
    {
        return false;
    }

    // 从金币槽扣除（从后往前，避免移除元素时越界）
    int32 Remaining = Amount;
    for (int32 i = Slots.Num() - 1; i >= 0 && Remaining > 0; --i)
    {
        if (Slots[i].ItemID != CoinItemID)
        {
            continue;
        }

        const int32 Taken = FMath::Min(Remaining, Slots[i].Count);
        Slots[i].Count -= Taken;
        Remaining -= Taken;

        if (Slots[i].Count <= 0)
        {
            Slots.RemoveAt(i);
        }
    }

    OnInventoryChanged.Broadcast();
    OnCoinsChanged.Broadcast(GetCoins());
    return true;
}

int32 UInventoryData::GetCoins() const
{
    if (!GetItemData(CoinItemID))
    {
        return 0;
    }
    return GetItemCount(CoinItemID);
}
