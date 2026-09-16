// Fill out your copyright notice in the Description page of Project Settings.


#include "NetRelevantObject.h"

#include "NetRelevantGlobalComponent.h"
#include "NetRelevantObjectUtils.h"
#include "Net/UnrealNetwork.h"

UNetRelevantObject::UNetRelevantObject()
{
}

void UNetRelevantObject::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
	
	DOREPLIFETIME(ThisClass, Id);
}

void UNetRelevantObject::PreDestroyFromReplication()
{
	NetEndPlay();
	
	Super::PreDestroyFromReplication();
}

void UNetRelevantObject::OnControllerDestroyed(AActor* DestroyedActor)
{
	if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
	{
		Global->RemoveNetRelevantObject(Id);
	}
}

void UNetRelevantObject::NativeAuthorityBeginPlay()
{
	// Make sure the life cycle is sync with the owning player controller.
	if (PlayerController)
	{
		PlayerController->OnDestroyed.AddDynamic(this, &UNetRelevantObject::OnControllerDestroyed);
	}
	
	AuthorityBeginPlay();
}

void UNetRelevantObject::NativeAuthorityEndPlay()
{
	AuthorityEndPlay();
}

void UNetRelevantObject::NativeBeginPlay()
{
	BeginPlay();
}

void UNetRelevantObject::NativeEndPlay()
{
	EndPlay();
}

void UNetRelevantObject::NetBeginPlay()
{
	if (!bIsBegunPlay && Id.IsValid())
	{
		bIsBegunPlay = true;
		
		if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
		{
			Global->LocalNetRelevantObjects.Add(Id, this);
		}
		
		NativeBeginPlay();
	}
}

void UNetRelevantObject::NetEndPlay()
{
	if (bIsBegunPlay)
	{
		bIsBegunPlay = false;
	
		NativeEndPlay();
	
		if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
		{
			Global->LocalNetRelevantObjects.Remove(Id);
		}
	}
}

UNetRelevantGlobalComponent* UNetRelevantObject::GetOwnerComponent() const
{
	return Cast<UNetRelevantGlobalComponent>(GetOuter());
}

void UNetRelevantObject::AddNetGroup(const FName InGroupName)
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(this, InGroupName);
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(this);
}

void UNetRelevantObject::RemoveNetGroup(const FName InGroupName)
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::RemoveObjectNetGroup(this, InGroupName);
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(this);
}

void UNetRelevantObject::ClearNetGroups()
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::ClearNetGroups(this);
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(this);
}

void UNetRelevantObject::OnRep_Id()
{
	NetBeginPlay();
}
