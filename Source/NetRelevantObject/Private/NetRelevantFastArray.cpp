// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantFastArray.h"

#include "NetRelevantObject.h"
#include "NetRelevantLocalComponent.h"

void FNetRelevantObjectArray::RebuildIndicesMapping()
{
	IndicesMapping.Empty();

	for (int i = 0; i < Objects.Num(); ++i)
	{
		const auto Entry = Objects[i];
		if (Entry.Object)
		{
			IndicesMapping.Add(Entry.Object->Id, i);
		}
	}
}

void FNetRelevantObjectArray::AddObject(UNetRelevantObject* InObject)
{
	if (!InObject)
	{
		return;
	}
	
	if (!IndicesMapping.Contains(InObject->Id))
	{
		auto NewEntry = FNetRelevantObjectEntry(InObject);
		const auto Index = Objects.Add(NewEntry);
		IndicesMapping.Add(InObject->Id, Index);
	}
}

void FNetRelevantObjectArray::RemoveObject(const FGuid Id)
{
	int32 Index;
	if (IndicesMapping.RemoveAndCopyValue(Id, Index))
	{
		Objects.RemoveAtSwap(Index, EAllowShrinking::No);
		if (Objects.IsValidIndex(Index))
		{
			IndicesMapping.FindChecked(Objects[Index].Object->Id) = Index;
		}
	}
}

UNetRelevantObject* FNetRelevantObjectArray::FindObject(const FGuid Id) const
{
	if (const auto Found = IndicesMapping.Find(Id))
	{
		if (Objects.IsValidIndex(*Found))
		{
			return Objects[*Found].Object;
		}
	}
	
	return nullptr;
}

bool FNetRelevantVisibleIds::SetVisible(FGuid Id, bool bVisible)
{
	if (bVisible)
	{
		if (Indices.Contains(Id)) return false;
		const int32 Index = Items.AddDefaulted();
		Items[Index].Id = Id;
		Indices.Add(Id, Index);
		MarkItemDirty(Items[Index]);
	}
	else
	{
		int32 Index;
		if (!Indices.RemoveAndCopyValue(Id, Index)) return false;
		Items.RemoveAtSwap(Index, EAllowShrinking::No);
		if (Items.IsValidIndex(Index)) Indices.FindChecked(Items[Index].Id) = Index;
		MarkArrayDirty();
	}
	return true;
}

void FNetRelevantVisibleIds::PreReplicatedRemove(TArrayView<int32> Removed, int32 FinalSize)
{
	for (int32 Index : Removed)
	{
		Indices.Remove(Items[Index].Id);
		ChangedIds.Add(Items[Index].Id);
	}
}

void FNetRelevantVisibleIds::PostReplicatedAdd(TArrayView<int32> Added, int32 FinalSize)
{
	for (int32 Index : Added)
	{
		Indices.Add(Items[Index].Id, Index);
		ChangedIds.Add(Items[Index].Id);
	}
}

void FNetRelevantVisibleIds::PostReplicatedReceive(const FPostReplicatedReceiveParameters& Params)
{
	const TArray<FGuid> Changes = MoveTemp(ChangedIds);
	for (const FGuid& Id : Changes)
	{
		if (UNetRelevantLocalComponent* Local = Owner.Get()) Local->RefreshLocalPresentation(Id);
	}
}
