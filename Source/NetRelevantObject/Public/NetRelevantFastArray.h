// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "NetRelevantFastArray.generated.h"

class UNetRelevantObject;

// Just avoid object gc.
USTRUCT()
struct NETRELEVANTOBJECT_API FNetRelevantObjectEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	FNetRelevantObjectEntry() {}
	FNetRelevantObjectEntry(UNetRelevantObject* InObject) : Object(InObject) {}
	
	UPROPERTY()
	UNetRelevantObject*	Object = nullptr;
};

// Just avoid object gc.
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

template<>
struct TStructOpsTypeTraits<FNetRelevantObjectArray> : public TStructOpsTypeTraitsBase2<FNetRelevantObjectArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};