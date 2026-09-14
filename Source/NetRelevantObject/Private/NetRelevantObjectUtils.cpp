// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantObjectUtils.h"

#include "NetRelevantGlobalComponent.h"
#include "NetRelevantLocalComponent.h"
#include "NetRelevantObject.h"
#include "GameFramework/GameStateBase.h"
#include "Net/Subsystems/NetworkSubsystem.h"

bool UNetRelevantObjectFunctionLibrary::CanPlayerReceiveSubobject(const TArrayView<const FName>& SubobjectGroups,
                                                                  const TArray<FName>& PlayerGroups)
{
	for (const FName& PlayerGroup : PlayerGroups)
	{
		if (SubobjectGroups.Contains(PlayerGroup))
		{
			return true;
		}
	}
	
	return false;
}

UNetRelevantGlobalComponent* UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(
	const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (const auto World = WorldContextObject->GetWorld())
	{
		if (const auto GS = World->GetGameState())
		{
			return GS->GetComponentByClass<UNetRelevantGlobalComponent>();
		}
	}
	
	return nullptr;
}

UNetworkSubsystem* UNetRelevantObjectFunctionLibrary::GetNetworkSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (const auto World = WorldContextObject->GetWorld())
	{
		return World->GetSubsystem<UNetworkSubsystem>();
	}
	
	return nullptr;
}

UE::Net::FNetConditionGroupManager* UNetRelevantObjectFunctionLibrary::GetNetConditionGroupManager(
	const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (const auto NetSub = GetNetworkSubsystem(WorldContextObject))
	{
		return &NetSub->GetNetConditionGroupManager();
	}
	
	return nullptr;
}

TArray<FName> UNetRelevantObjectFunctionLibrary::GetPlayerNetGroups(const APlayerController* InPlayerController)
{
	if (!InPlayerController)
	{
		return TArray<FName>();
	}
	
	if (InPlayerController->HasAuthority())
	{
		return InPlayerController->GetNetConditionGroups();
	}
	
	return TArray<FName>();
}

void UNetRelevantObjectFunctionLibrary::AddPlayerNetGroup(APlayerController* InPlayerController,
	const FName InGroupName)
{
	if (!InPlayerController)
	{
		return;
	}

	if (InGroupName.IsNone())
	{
		return;
	}
	
	if (InPlayerController->HasAuthority())
	{
		InPlayerController->IncludeInNetConditionGroup(InGroupName);
	}
}

void UNetRelevantObjectFunctionLibrary::RemovePlayerNetGroup(APlayerController* InPlayerController,
	const FName InGroupName)
{
	if (!InPlayerController)
	{
		return;
	}
	
	if (InGroupName.IsNone())
	{
		return;
	}
	
	if (InPlayerController->HasAuthority())
	{
		InPlayerController->RemoveFromNetConditionGroup(InGroupName);
	}
}

void UNetRelevantObjectFunctionLibrary::ClearPlayerNetGroups(APlayerController* InPlayerController)
{
	if (!InPlayerController)
	{
		return;
	}
	
	if (InPlayerController->HasAuthority())
	{
		const auto Groups = InPlayerController->GetNetConditionGroups();
		for (const auto Group : Groups)
		{
			InPlayerController->RemoveFromNetConditionGroup(Group);
		}
	}
}

bool UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(const UObject* Object)
{
	if (!Object)
	{
		return false;
	}

	if (const auto World = Object->GetWorld())
	{
		if (World->GetAuthGameMode())
		{
			return true;
		}
	}
	
	return false;
}

TArrayView<const FName> UNetRelevantObjectFunctionLibrary::GetObjectNetGroupsView(const UObject* Object)
{
	if (!Object)
	{
		return TArrayView<const FName>();
	}

	if (!IsObjectHasAuthority(Object))
	{
		return TArrayView<const FName>();
	}
	
	if (const auto Manager = GetNetConditionGroupManager(Object))
	{
		return Manager->GetSubObjectNetConditionGroups(Object);
	}
	
	return TArrayView<const FName>();
}

TArray<FName> UNetRelevantObjectFunctionLibrary::GetObjectNetGroups(const UObject* Object)
{
	if (!Object)
	{
		return TArray<FName>();
	}
	
	if (!IsObjectHasAuthority(Object))
	{
		return TArray<FName>();
	}
	
	const auto View = GetObjectNetGroupsView(Object);
	return TArray<FName>(View);
}

void UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(UObject* Object, FName InGroupName)
{
	if (!Object)
	{
		return;
	}
	
	if (InGroupName.IsNone())
	{
		return;
	}
	
	if (!IsObjectHasAuthority(Object))
	{
		return;
	}
	
	if (const auto Manager = GetNetConditionGroupManager(Object))
	{
		Manager->RegisterSubObjectInGroup(Object, InGroupName);
	}
}

void UNetRelevantObjectFunctionLibrary::RemoveObjectNetGroup(UObject* Object, FName InGroupName)
{
	if (!Object)
	{
		return;
	}
	
	if (InGroupName.IsNone())
	{
		return;
	}
	
	if (!IsObjectHasAuthority(Object))
	{
		return;
	}
	
	if (const auto Manager = GetNetConditionGroupManager(Object))
	{
		Manager->UnregisterSubObjectFromGroup(Object, InGroupName);
	}
}

void UNetRelevantObjectFunctionLibrary::ClearNetGroups(UObject* Object)
{
	if (!Object)
	{
		return;
	}
	
	if (!IsObjectHasAuthority(Object))
	{
		return;
	}
	
	if (const auto Manager = GetNetConditionGroupManager(Object))
	{
		Manager->UnregisterSubObjectFromAllGroups(Object);
	}
}

void UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(UObject* Object)
{
	if (!Object)
	{
		return;
	}
	
	if (!IsObjectHasAuthority(Object))
	{
		return;
	}
	
	const auto World = Object->GetWorld();
	if (!World)
	{
		return;
	}
	
	const auto SubObjectGroups = GetObjectNetGroupsView(Object);
	
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerActor = Iterator->Get())
		{
			if (!CanPlayerReceiveSubobject(SubObjectGroups, GetPlayerNetGroups(PlayerActor)))
			{
				CallStopReplicationClientInternal(PlayerActor, Object);
			}
			else
			{
				CallStartReplicationClientInternal(PlayerActor, Object);
			}
		}
	}
}

void UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(const APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	
	const auto Global = GetNetRelevantGlobalComponent(PC);
	const auto PCGroups = PC->GetNetConditionGroups();
	if (Global)
	{
		for (const auto Object : Global->NetRelevantObjects.Objects)
		{
			if (Object.Object)
			{
				const auto Groups = GetObjectNetGroupsView(Object.Object);
				if (!CanPlayerReceiveSubobject(Groups, PCGroups))
				{
					CallStopReplicationClientInternal(PC, Object.Object);
				}
				else
				{
					CallStartReplicationClientInternal(PC, Object.Object);
				}
			}
		}
	}
}

void UNetRelevantObjectFunctionLibrary::CallStopReplicationClientInternal(const APlayerController* PC, UObject* Object)
{
	if (PC && Object)
	{
		if (const auto Local = PC->GetComponentByClass<UNetRelevantLocalComponent>())
		{
			Local->CallEndReplicationEventClient(Object);
		}
	}
}

void UNetRelevantObjectFunctionLibrary::CallStartReplicationClientInternal(const APlayerController* PC, UObject* Object)
{
	if (PC && Object)
	{
		if (const auto Local = PC->GetComponentByClass<UNetRelevantLocalComponent>())
		{
			Local->CallBeginReplicationEventClient(Object);
		}
	}
}
