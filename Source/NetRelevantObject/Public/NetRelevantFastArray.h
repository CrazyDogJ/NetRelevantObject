// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "NetRelevantFastArray.generated.h"

class UNetRelevantObject;
class UNetRelevantLocalComponent;

// Strong reference in the authority registry. This entry is never sent to clients.
USTRUCT()
struct NETRELEVANTOBJECT_API FNetRelevantObjectEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	FNetRelevantObjectEntry() {}
	FNetRelevantObjectEntry(UNetRelevantObject* InObject) : Object(InObject) {}
	
	UPROPERTY()
	UNetRelevantObject*	Object = nullptr;
};

// Preserve the registry layout used by Minimap.
USTRUCT()
struct NETRELEVANTOBJECT_API FNetRelevantObjectArray: public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FNetRelevantObjectEntry>	Objects;
	
	UPROPERTY(NotReplicated)
	TMap<FGuid, int32> IndicesMapping;
	
	void RebuildIndicesMapping();
	
	void AddObject(UNetRelevantObject* InObject);
	void RemoveObject(const FGuid Id);
	UNetRelevantObject* FindObject(const FGuid Id) const;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNetRelevantObjectEntry, FNetRelevantObjectArray>(Objects, DeltaParms, *this);
	}
};

USTRUCT()
struct FNetRelevantVisibleId : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;
};

USTRUCT()
struct FNetRelevantVisibleIds : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FNetRelevantVisibleId> Items;

	bool SetVisible(FGuid Id, bool bVisible);
	bool Contains(FGuid Id) const { return Indices.Contains(Id); }
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Params)
	{
		return FastArrayDeltaSerialize<FNetRelevantVisibleId, FNetRelevantVisibleIds>(Items, Params, *this);
	}
	void PreReplicatedRemove(TArrayView<int32> Removed, int32 FinalSize);
	void PostReplicatedAdd(TArrayView<int32> Added, int32 FinalSize);
	void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Params);

	TWeakObjectPtr<UNetRelevantLocalComponent> Owner;

private:
	// Indices are used for swap removal on authority. Clients only query membership.
	TMap<FGuid, int32> Indices;
	TArray<FGuid> ChangedIds;
};

template<>
struct TStructOpsTypeTraits<FNetRelevantVisibleIds> : TStructOpsTypeTraitsBase2<FNetRelevantVisibleIds>
{
	enum { WithNetDeltaSerializer = true };
};

template<>
struct TStructOpsTypeTraits<FNetRelevantObjectArray> : public TStructOpsTypeTraitsBase2<FNetRelevantObjectArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};
