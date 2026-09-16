// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetRelevantObjectUtils.generated.h"

#define GLOBAL_GROUP FName("Global")

class UNetworkSubsystem;
class UNetRelevantGlobalComponent;

UCLASS()
class NETRELEVANTOBJECT_API UNetRelevantObjectFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool CanPlayerReceiveSubobject(const TArrayView<const FName>& SubobjectGroups, const TArray<FName>& PlayerGroups);
	
	UFUNCTION(BlueprintPure, meta=(WorldContext = WorldContextObject))
	static UNetRelevantGlobalComponent* GetNetRelevantGlobalComponent(const UObject* WorldContextObject);
	
	static UNetworkSubsystem* GetNetworkSubsystem(const UObject* WorldContextObject);
	
	static UE::Net::FNetConditionGroupManager* GetNetConditionGroupManager(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	static TArray<FName> GetPlayerNetGroups(const APlayerController* InPlayerController);
	
	static void AddPlayerNetGroup(APlayerController* InPlayerController, FName InGroupName);
	
	static void RemovePlayerNetGroup(APlayerController* InPlayerController, FName InGroupName);
	
	static void ClearPlayerNetGroups(APlayerController* InPlayerController);
	
	static bool IsObjectHasAuthority(const UObject* Object);
	
	static TArrayView<const FName> GetObjectNetGroupsView(const UObject* Object);
	
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	static TArray<FName> GetObjectNetGroups(const UObject* Object);
	
	static void AddObjectNetGroup(UObject* Object, FName InGroupName);
	
	static void RemoveObjectNetGroup(UObject* Object, FName InGroupName);
	
	static void ClearNetGroups(UObject* Object);
	
	static void CopyNetGroups(UObject* Object, const APlayerController* InPlayerController);
	
	// For object.
	static void CallReplicationChangeClient(UObject* Object);
	// For player controller.
	static void CallReplicationChangeClient(const APlayerController* PC);
	
	static void CallStopReplicationClientInternal(const APlayerController* PC, UObject* Object);
	
	static void CallStartReplicationClientInternal(const APlayerController* PC, UObject* Object);
};
