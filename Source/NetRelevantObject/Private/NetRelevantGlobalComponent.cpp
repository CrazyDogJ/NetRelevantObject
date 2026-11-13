// Fill out your copyright notice in the Description page of Project Settings.

#include "NetRelevantGlobalComponent.h"

#include "NetRelevantObject.h"
#include "NetRelevantObjectUtils.h"
#include "NetRelevantLocalComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StrongObjectPtr.h"
#if UE_WITH_IRIS
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#endif

UNetRelevantGlobalComponent::UNetRelevantGlobalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
void UNetRelevantGlobalComponent::OnReplicationStartedForIris(const FOnReplicationStartedParams& Params)
{
	Super::OnReplicationStartedForIris(Params);
	for (const auto& Entry : NetRelevantObjects.Objects)
		UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(Entry.Object, GetWorld());
}
#endif

void UNetRelevantGlobalComponent::RegisterReplica(UNetRelevantObject* Object)
{
	Replicas.Add(Object->Id, Object);
	RefreshLocalVisibility(Object->Id);
}

void UNetRelevantGlobalComponent::ForgetReplica(UNetRelevantObject* Object)
{
	if (Replicas.FindRef(Object->Id).Get() == Object) Replicas.Remove(Object->Id);
}

void UNetRelevantGlobalComponent::RefreshLocalVisibility(FGuid Id)
{
	UNetRelevantObject* Object = GetOwner()->HasAuthority() ? NetRelevantObjects.FindObject(Id) : Replicas.FindRef(Id).Get();
	if (!IsValid(Object)) return;

	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalPlayerController()) continue;
		const auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>();
		if (Local && Local->CanPresent(Id))
		{
			Object->NetBeginPlay();
			return;
		}
	}
	Object->NetEndPlay();
}

UNetRelevantObject* UNetRelevantGlobalComponent::FindNetRelevantObject(FGuid Id) const
{
	return GetOwner()->HasAuthority() ? NetRelevantObjects.FindObject(Id) : LocalNetRelevantObjects.FindRef(Id);
}

TArray<UNetRelevantObject*> UNetRelevantGlobalComponent::GetNetRelevantObjects() const
{
	TArray<UNetRelevantObject*> Out;
	LocalNetRelevantObjects.GenerateValueArray(Out);
	Out.RemoveAll([](const UNetRelevantObject* Object) { return !IsValid(Object) || !Object->GetIsRunning(); });
	return Out;
}

TArray<UNetRelevantObject*> UNetRelevantGlobalComponent::GetPureNetRelevantObjects() const
{
	TArray<UNetRelevantObject*> Out;
	if (!GetOwner()->HasAuthority()) return GetNetRelevantObjects();
	Out.Reserve(NetRelevantObjects.Objects.Num());
	for (const auto& Entry : NetRelevantObjects.Objects)
	{
		Out.Add(Entry.Object);
	}
	return Out;
}

UNetRelevantObject* UNetRelevantGlobalComponent::FindNetRelevantObject_BP(FGuid Id) const
{
	return FindNetRelevantObject(Id);
}

UNetRelevantObject* UNetRelevantGlobalComponent::AddNetRelevantObject(const TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController,
                                                                      const FName GroupName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	
	// 修复：未指定类或传入抽象类会导致 NewObject 断言。
	if (!ObjectClass || ObjectClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;

	// Create new object and set up.
	const auto NetObject = NewObject<UNetRelevantObject>(this, ObjectClass);
	AddNetRelevantObjectInternal(NetObject, OwnerController, GroupName);

	return NetObject;
}

UNetRelevantObject* UNetRelevantGlobalComponent::AddNetRelevantObjectFollowingOwnerGroups(
	TSubclassOf<UNetRelevantObject> ObjectClass, APlayerController* OwnerController)
{
	if (!IsValid(OwnerController) || !GetOwner() || !GetOwner()->HasAuthority() ||
		!ObjectClass || ObjectClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	auto* Object = NewObject<UNetRelevantObject>(this, ObjectClass);
	Object->bFollowOwnerGroups = true;
	AddNetRelevantObjectInternal(Object, OwnerController, NAME_None);
	return Object;
}

void UNetRelevantGlobalComponent::AddNetRelevantObjectInternal(UNetRelevantObject* NetObject, APlayerController* OwnerController, const FName GroupName)
{
	if (!IsValid(NetObject) || NetObject->GetOuter() != this || NetObject->Id.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}
	NetObject->PlayerController = OwnerController;
	NetObject->Id = FGuid::NewGuid();
	if (NetObject->FollowsOwnerGroups())
	{
		UNetRelevantObjectFunctionLibrary::CopyNetGroups(NetObject, NetObject->PlayerController);
	}
	else if (OwnerController && GroupName.IsNone())
	{
		// Without a local component there is no private audience. Leave the object ungrouped.
		if (auto* Local = OwnerController->GetComponentByClass<UNetRelevantLocalComponent>())
		{
			if (auto* Manager = UNetRelevantObjectFunctionLibrary::GetNetConditionGroupManager(this))
				Manager->RegisterSubObjectInGroup(NetObject, Local->GetOrCreatePrivateGroup());
		}
	}
	else
	{
		const FName Name = GroupName.IsNone() ? GLOBAL_GROUP : GroupName;
		UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(NetObject, Name);
	}
	
	// Add object to replicating list.
	NetRelevantObjects.AddObject(NetObject);
	AddReplicatedSubObject(NetObject, COND_NetGroup);
#if UE_WITH_IRIS
	UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(NetObject, GetWorld());
#endif
}

void UNetRelevantGlobalComponent::BeginObjectLogic(UNetRelevantObject* NetObject)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 修复：只启动当前组件中仍然注册的对象。
	if (!IsValid(NetObject) || NetObject->bAuthorityLogicStarted || FindNetRelevantObject(NetObject->Id) != NetObject) return;

	// Call functions on authority server.
	NetObject->bAuthorityLogicStarted = true;
	NetObject->NativeAuthorityBeginPlay();
	// Begin play check.
	if (FindNetRelevantObject(NetObject->Id) == NetObject)
	{
		UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(NetObject);
	}
}

void UNetRelevantGlobalComponent::RemoveNetRelevantObject(FGuid Id)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// Find object pointer.
	if (const auto Found = NetRelevantObjects.FindObject(Id))
	{
		// 修复：先移出查找表，再执行可重入的蓝图结束事件，避免重复销毁。
		TStrongObjectPtr<UNetRelevantObject> KeepAlive(Found);
		NetRelevantObjects.RemoveObject(Id);
		Found->bAuthorityLogicStarted = false;
		for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (auto* PC = It->Get())
			{
				if (auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>()) Local->CallEndReplicationEventClient(Found);
			}
		}
		Found->NetEndPlay();
		Found->NativeAuthorityEndPlay();
		DestroyReplicatedSubObjectOnRemotePeers(Found);
		UNetRelevantObjectFunctionLibrary::ClearNetGroups(Found);
	}
}
