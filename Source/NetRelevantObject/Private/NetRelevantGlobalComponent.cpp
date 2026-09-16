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

UNetRelevantObject* UNetRelevantGlobalComponent::AddNetRelevantObject(const TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController,
                                                                      const FName GroupName)
{
	if (!GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	
	// Create new object and set up.
	const auto NetObject = NewObject<UNetRelevantObject>(this, ObjectClass);
	AddNetRelevantObjectInternal(NetObject, OwnerController, GroupName);

	return NetObject;
}

void UNetRelevantGlobalComponent::AddNetRelevantObjectInternal(UNetRelevantObject* NetObject, APlayerController* OwnerController, const FName GroupName)
{
	if (!NetObject)
	{
		return;
	}
	NetObject->PlayerController = OwnerController;
	NetObject->Id = FGuid::NewGuid();
	if (NetObject->PlayerController)
	{
		UNetRelevantObjectFunctionLibrary::CopyNetGroups(NetObject, NetObject->PlayerController);
	}
	else
	{
		const FName Name = GroupName.IsNone() ? GLOBAL_GROUP : GroupName;
		UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(NetObject, Name);
	}
	
	// Add object to replicating list.
	NetRelevantObjects.AddObject(NetObject);
	AddReplicatedSubObject(NetObject, COND_NetGroup);
}

void UNetRelevantGlobalComponent::BeginObjectLogic(UNetRelevantObject* NetObject)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	// Call functions on authority server.
	NetObject->NativeAuthorityBeginPlay();
	// Begin play check.
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(NetObject);
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
