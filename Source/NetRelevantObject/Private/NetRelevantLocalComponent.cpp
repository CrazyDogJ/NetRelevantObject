// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantLocalComponent.h"

#include "NetRelevantObject.h"
#include "NetRelevantObjectUtils.h"

UNetRelevantLocalComponent::UNetRelevantLocalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNetRelevantLocalComponent::AddNetConditionGroup(const FName GroupName) const
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::AddPlayerNetGroup(Cast<APlayerController>(GetOwner()), GroupName);
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(Cast<APlayerController>(GetOwner()));
}

void UNetRelevantLocalComponent::RemoveNetConditionGroup(const FName Name) const
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::RemovePlayerNetGroup(Cast<APlayerController>(GetOwner()), Name);
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(Cast<APlayerController>(GetOwner()));
}

void UNetRelevantLocalComponent::ClearNetConditionGroups() const
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::ClearPlayerNetGroups(Cast<APlayerController>(GetOwner()));
	UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(Cast<APlayerController>(GetOwner()));
}

void UNetRelevantLocalComponent::CallBeginReplicationEventClient_Implementation(UObject* InObject)
{
	if (InObject)
	{
		// Authority will call end play when has a controller.
		if (const auto Object = Cast<UNetRelevantObject>(InObject))
		{
			Object->NetBeginPlay();
		}
	}
}

void UNetRelevantLocalComponent::CallEndReplicationEventClient_Implementation(UObject* InObject)
{
	if (InObject)
	{
		// Authority will call end play when has a controller.
		if (const auto Object = Cast<UNetRelevantObject>(InObject))
		{
			Object->NetEndPlay();
		}
	}
}

void UNetRelevantLocalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	AddNetConditionGroup(GLOBAL_GROUP);
}

void UNetRelevantLocalComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
