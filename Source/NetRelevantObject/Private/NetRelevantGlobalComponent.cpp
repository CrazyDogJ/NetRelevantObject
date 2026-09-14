// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantGlobalComponent.h"

#include "NetRelevantObject.h"
#include "NetRelevantObjectUtils.h"
#include "Net/UnrealNetwork.h"

UNetRelevantGlobalComponent::UNetRelevantGlobalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UNetRelevantGlobalComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, NetRelevantObjects)
}

UNetRelevantObject* UNetRelevantGlobalComponent::FindNetRelevantObject(FGuid Id) const
{
	if (const auto Found = NetRelevantObjects.FindObject(Id))
	{
		return Found;
	}
	
	return nullptr;
}

TArray<UNetRelevantObject*> UNetRelevantGlobalComponent::GetNetRelevantObjects() const
{
	TArray<UNetRelevantObject*> Out;
	for (const auto Entry : NetRelevantObjects.Objects)
	{
		if (Entry.Object && Entry.Object->GetIsRunning())
		{
			Out.Add(Entry.Object);
		}
	}
	return Out;
}

TArray<UNetRelevantObject*> UNetRelevantGlobalComponent::GetPureNetRelevantObjects() const
{
	TArray<UNetRelevantObject*> Out;
	for (const auto Entry : NetRelevantObjects.Objects)
	{
		Out.Add(Entry.Object);
	}
	return Out;
}

UNetRelevantObject* UNetRelevantGlobalComponent::FindNetRelevantObject_BP(FGuid Id) const
{
	if (const auto Found = FindNetRelevantObject(Id))
	{
		return Found;
	}
	
	return nullptr;
}

UNetRelevantObject* UNetRelevantGlobalComponent::AddNetRelevantObject(const TSubclassOf<UNetRelevantObject> ObjectClass,
                                                                      const FName GroupName)
{
	if (!GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	
	// Create new object and set up.
	const auto NetObject = NewObject<UNetRelevantObject>(this, ObjectClass);
	NetObject->Id = FGuid::NewGuid();
	const FName Name = GroupName.IsNone() ? "Global" : GroupName;
	
	// Add object to replicating list.
	NetRelevantObjects.AddObject(NetObject);
	AddReplicatedSubObject(NetObject, COND_NetGroup);
	UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(NetObject, Name);
	
	// Call functions on authority server.
	NetObject->NativeAuthorityBeginPlay();
	// Begin play check.
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(NetObject);

	return NetObject;
}

void UNetRelevantGlobalComponent::RemoveNetRelevantObject(FGuid Id)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	// Find object pointer.
	if (const auto Found = NetRelevantObjects.FindObject(Id))
	{
		Found->NetEndPlay();
		Found->NativeAuthorityEndPlay();
		DestroyReplicatedSubObjectOnRemotePeers(Found);
	}
	
	NetRelevantObjects.RemoveObject(Id);
}
