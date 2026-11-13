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
	
	// Authority registry. Minimap iterates Objects when updating pin transforms.
	UPROPERTY(Transient)
	FNetRelevantObjectArray NetRelevantObjects;

	void RegisterReplica(UNetRelevantObject* Object);
	void ForgetReplica(UNetRelevantObject* Object);
	void RefreshLocalVisibility(FGuid Id);
#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	virtual void OnReplicationStartedForIris(const FOnReplicationStartedParams& Params) override;
#endif
	
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
		
		return Cast<T>(AddNetRelevantObject(T::StaticClass(), OwnerController, GroupName));
	}
	
	// A named group is explicit. With None, an owned object is private and an unowned object uses Global.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(DeterminesOutputType="ObjectClass"))
	UNetRelevantObject* AddNetRelevantObject(TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController, FName GroupName);

	// For objects that should follow changes to the controller's gameplay groups.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(DeterminesOutputType="ObjectClass"))
	UNetRelevantObject* AddNetRelevantObjectFollowingOwnerGroups(TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController);
	
	void AddNetRelevantObjectInternal(UNetRelevantObject* NetObject, APlayerController* OwnerController, FName GroupName);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void BeginObjectLogic(UNetRelevantObject* NetObject);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveNetRelevantObject(FGuid Id);

private:
	TMap<FGuid, TWeakObjectPtr<UNetRelevantObject>> Replicas;
};
