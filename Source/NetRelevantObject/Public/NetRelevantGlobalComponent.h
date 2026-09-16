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
	
	template <class T>
	T* AddNetRelevantObject(APlayerController* OwnerController, const FName GroupName)
	{
		static_assert(TIsDerivedFrom<T, UNetRelevantObject>::Value, "T must be a subclass of UNetRelevantObject!");
		
		if (!GetOwner()->HasAuthority())
		{
			return nullptr;
		}
	
		// Create new object and set up.
		T* NetObject = NewObject<T>(this);
		AddNetRelevantObjectInternal(NetObject, OwnerController, GroupName);

		return NetObject;
	}
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(DeterminesOutputType="ObjectClass"))
	UNetRelevantObject* AddNetRelevantObject(TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController, FName GroupName);
	
	void AddNetRelevantObjectInternal(UNetRelevantObject* NetObject, APlayerController* OwnerController, FName GroupName);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void BeginObjectLogic(UNetRelevantObject* NetObject);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveNetRelevantObject(FGuid Id);
};
