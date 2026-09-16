// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NetRelevantObject.generated.h"

class UNetRelevantGlobalComponent;
class UNetworkSubsystem;
class UNetRelevantLocalComponent;

// Net relevant object by channel.
UCLASS(Blueprintable, BlueprintType)
class NETRELEVANTOBJECT_API UNetRelevantObject : public UObject
{
	GENERATED_BODY()
	
public:
	UNetRelevantObject();
	
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreDestroyFromReplication() override;
#if WITH_EDITOR
	virtual bool ImplementsGetWorld() const override { return true; }
#endif

	UFUNCTION()
	void OnControllerDestroyed(AActor* DestroyedActor);
	
	virtual void NativeAuthorityBeginPlay();
	virtual void NativeAuthorityEndPlay();
	
	virtual void NativeBeginPlay();
	virtual void NativeEndPlay();
	
	void NetBeginPlay();
	void NetEndPlay();
	
	bool GetIsRunning() const { return bIsBegunPlay; }
	
	UFUNCTION(BlueprintPure)
	UNetRelevantGlobalComponent* GetOwnerComponent() const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddNetGroup(FName InGroupName);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveNetGroup(FName InGroupName);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ClearNetGroups();
	
	UFUNCTION(BlueprintImplementableEvent)
	void AuthorityBeginPlay();
	
	UFUNCTION(BlueprintImplementableEvent)
	void AuthorityEndPlay();
	
	UFUNCTION(BlueprintImplementableEvent)
	void BeginPlay();
	
	UFUNCTION(BlueprintImplementableEvent)
	void EndPlay();
	
	// Authority only player controller owner.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	APlayerController* PlayerController;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, ReplicatedUsing = OnRep_Id)
	FGuid Id;

	UFUNCTION()
	void OnRep_Id();
	
private:
	bool bIsBegunPlay = false;
};
