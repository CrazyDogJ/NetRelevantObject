// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NetRelevantLocalComponent.generated.h"

class UNetRelevantObject;
class UNetworkSubsystem;
class UNetRelevantGlobalComponent;

// Component on player controller.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NETRELEVANTOBJECT_API UNetRelevantLocalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNetRelevantLocalComponent();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddNetConditionGroup(FName GroupName) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveNetConditionGroup(FName Name) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ClearNetConditionGroups() const;

	UFUNCTION(Client, Reliable)
	void CallBeginReplicationEventClient(UObject* InObject);
	
	UFUNCTION(Client, Reliable)
	void CallEndReplicationEventClient(UObject* InObject);
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
