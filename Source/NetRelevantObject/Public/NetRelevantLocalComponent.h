// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NetRelevantFastArray.h"
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

	// Compatibility entry points. Update the owner's replicated visibility state on authority.
	UFUNCTION()
	void CallBeginReplicationEventClient(UObject* InObject);
	
	UFUNCTION()
	void CallEndReplicationEventClient(UObject* InObject);

	void RefreshObjectVisibility(UNetRelevantObject* Object);
	bool CanPresent(FGuid Id) const { return !bEndingPlay && VisibleIds.Contains(Id); }
	void RefreshLocalPresentation(FGuid Id) const;
	FName GetOrCreatePrivateGroup();
	FName GetPrivateGroup() const { return PrivateGroup; }
	
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void SetObjectVisible(UNetRelevantObject* Object, bool bVisible);

	UPROPERTY(Replicated)
	FNetRelevantVisibleIds VisibleIds;

	FName PrivateGroup;
	bool bEndingPlay = false;
};
