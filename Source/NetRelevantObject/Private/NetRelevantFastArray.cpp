// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantFastArray.h"

#include "NetRelevantObject.h"

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
		MarkItemDirty(Objects[Index]);
	}
}

void FNetRelevantObjectArray::RemoveObject(const FGuid Id)
{
	if (IndicesMapping.Contains(Id))
	{
		if (const auto FoundIndex = IndicesMapping.Find(Id))
		{
			Objects.RemoveAt(*FoundIndex);
			MarkArrayDirty();
		}
	}
	
	RebuildIndicesMapping();
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
