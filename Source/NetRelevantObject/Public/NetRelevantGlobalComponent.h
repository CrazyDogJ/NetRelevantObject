// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetRelevantFastArray.h"
#include "Components/ActorComponent.h"
#include "NetRelevantGlobalComponent.generated.h"

class UNetworkSubsystem;
class UNetRelevantLocalComponent;
class UNetRelevantObject;

// Component on game state.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NETRELEVANTOBJECT_API UNetRelevantGlobalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNetRelevantGlobalComponent();
	
	UPROPERTY(BlueprintReadOnly)
	TMap<FGuid, UNetRelevantObject*> LocalNetRelevantObjects;
	
	UPROPERTY(Replicated)
	FNetRelevantObjectArray NetRelevantObjects;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UNetRelevantObject* FindNetRelevantObject(FGuid Id) const;
	
	UFUNCTION(BlueprintPure)
	TArray<UNetRelevantObject*> GetNetRelevantObjects() const;
	
	UFUNCTION(BlueprintPure)
	TArray<UNetRelevantObject*> GetPureNetRelevantObjects() const;
	
	UFUNCTION(BlueprintCallable, DisplayName = "FindNetRelevantObject")
	UNetRelevantObject* FindNetRelevantObject_BP(FGuid Id) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(DeterminesOutputType="ObjectClass"))
	UNetRelevantObject* AddNetRelevantObject(TSubclassOf<UNetRelevantObject> ObjectClass, FName GroupName);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveNetRelevantObject(FGuid Id);
};
