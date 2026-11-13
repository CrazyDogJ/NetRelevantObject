#include "NetRelevantObjectUtils.h"

#include "NetRelevantGlobalComponent.h"
#include "NetRelevantLocalComponent.h"
#include "NetRelevantObject.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Net/Subsystems/NetworkSubsystem.h"

#if UE_WITH_IRIS
#include "Engine/Engine.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#endif

namespace
{
bool IsGameplayGroup(FName Name)
{
	return !Name.IsNone() && !UE::Net::IsSpecialNetConditionGroup(Name) &&
		!Name.ToString().StartsWith(TEXT("NetRelevantPrivate_"));
}

void SetObjectGroup(UObject* Object, FName Group, bool bMember)
{
	auto* Manager = UNetRelevantObjectFunctionLibrary::GetNetConditionGroupManager(Object);
	if (!Manager || Manager->GetSubObjectNetConditionGroups(Object).Contains(Group) == bMember) return;
	if (bMember) Manager->RegisterSubObjectInGroup(Object, Group);
	else Manager->UnregisterSubObjectFromGroup(Object, Group);
#if UE_WITH_IRIS
	using namespace UE::Net;
	FReplicationSystemUtil::ForEachReplicationSystem(GEngine, Object->GetWorld(), [&](UReplicationSystem* System)
	{
		if (!System->IsServer() || !System->GetReplicationBridge()) return;
		const auto Handle = System->GetReplicationBridge()->GetReplicatedRefHandle(Object);
		if (!Handle.IsValid()) return;
		if (bMember)
		{
			System->AddToGroup(System->GetOrCreateSubObjectFilter(Group), Handle);
		}
		else
		{
			const auto Filter = System->GetSubObjectFilterGroupHandle(Group);
			if (Filter.IsValid()) System->RemoveFromGroup(Filter, Handle);
		}
	});
#endif
}
}

bool UNetRelevantObjectFunctionLibrary::CanPlayerReceiveSubobject(const TArrayView<const FName>& SubobjectGroups,
	const TArray<FName>& PlayerGroups)
{
	for (const FName Group : SubobjectGroups)
	{
		if (PlayerGroups.Contains(Group)) return true;
	}
	return false;
}

UNetRelevantGlobalComponent* UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return GS ? GS->GetComponentByClass<UNetRelevantGlobalComponent>() : nullptr;
}

UNetworkSubsystem* UNetRelevantObjectFunctionLibrary::GetNetworkSubsystem(const UObject* Context)
{
	return Context ? UWorld::GetSubsystem<UNetworkSubsystem>(Context->GetWorld()) : nullptr;
}

UE::Net::FNetConditionGroupManager* UNetRelevantObjectFunctionLibrary::GetNetConditionGroupManager(const UObject* Context)
{
	auto* Subsystem = GetNetworkSubsystem(Context);
	return Subsystem ? &Subsystem->GetNetConditionGroupManager() : nullptr;
}

TArray<FName> UNetRelevantObjectFunctionLibrary::GetPlayerNetGroups(const APlayerController* PC)
{
	if (!PC || !PC->HasAuthority()) return {};
	TArray<FName> Groups = PC->GetNetConditionGroups();
	if (const auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>())
		Groups.RemoveSingleSwap(Local->GetPrivateGroup());
	return Groups;
}

void UNetRelevantObjectFunctionLibrary::AddPlayerNetGroup(APlayerController* PC, FName Group)
{
	if (!PC || !PC->HasAuthority() || !IsGameplayGroup(Group) || PC->IsMemberOfNetConditionGroup(Group)) return;
	PC->IncludeInNetConditionGroup(Group);
	CallReplicationChangeClient(PC);
}

void UNetRelevantObjectFunctionLibrary::RemovePlayerNetGroup(APlayerController* PC, FName Group)
{
	if (!PC || !PC->HasAuthority() || !IsGameplayGroup(Group) || !PC->IsMemberOfNetConditionGroup(Group)) return;
	PC->RemoveFromNetConditionGroup(Group);
	CallReplicationChangeClient(PC);
}

void UNetRelevantObjectFunctionLibrary::ClearPlayerNetGroups(APlayerController* PC)
{
	if (!PC || !PC->HasAuthority()) return;
	const auto Groups = GetPlayerNetGroups(PC);
	if (Groups.IsEmpty()) return;
	for (FName Group : Groups) PC->RemoveFromNetConditionGroup(Group);
	CallReplicationChangeClient(PC);
}

bool UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(const UObject* Object)
{
	if (const auto* Owner = Object ? Object->GetTypedOuter<AActor>() : nullptr) return Owner->HasAuthority();
	const UWorld* World = Object ? Object->GetWorld() : nullptr;
	return World && World->GetAuthGameMode();
}

TArrayView<const FName> UNetRelevantObjectFunctionLibrary::GetObjectNetGroupsView(const UObject* Object)
{
	if (IsObjectHasAuthority(Object))
	{
		if (const auto* Manager = GetNetConditionGroupManager(Object))
			return Manager->GetSubObjectNetConditionGroups(Object);
	}
	return {};
}

TArray<FName> UNetRelevantObjectFunctionLibrary::GetObjectNetGroups(const UObject* Object)
{
	return TArray<FName>(GetObjectNetGroupsView(Object));
}

void UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(UObject* Object, FName Group)
{
	if (!IsObjectHasAuthority(Object) || !IsGameplayGroup(Group) || GetObjectNetGroupsView(Object).Contains(Group)) return;
	SetObjectGroup(Object, Group, true);
	CallReplicationChangeClient(Object);
}

void UNetRelevantObjectFunctionLibrary::RemoveObjectNetGroup(UObject* Object, FName Group)
{
	if (!IsObjectHasAuthority(Object) || !GetObjectNetGroupsView(Object).Contains(Group)) return;
	SetObjectGroup(Object, Group, false);
	CallReplicationChangeClient(Object);
}

void UNetRelevantObjectFunctionLibrary::ClearNetGroups(UObject* Object)
{
	const auto Groups = GetObjectNetGroups(Object);
	if (Groups.IsEmpty()) return;
	for (FName Group : Groups) SetObjectGroup(Object, Group, false);
	CallReplicationChangeClient(Object);
}

void UNetRelevantObjectFunctionLibrary::CopyNetGroups(UObject* Object, const APlayerController* PC)
{
	if (!PC || !PC->HasAuthority() || !IsObjectHasAuthority(Object)) return;
	const auto Old = GetObjectNetGroups(Object);
	const auto Desired = GetPlayerNetGroups(PC);
	bool bChanged = false;
	for (FName Group : Old)
	{
		if (Desired.Contains(Group)) continue;
		SetObjectGroup(Object, Group, false);
		bChanged = true;
	}
	for (FName Group : Desired)
	{
		if (Old.Contains(Group)) continue;
		SetObjectGroup(Object, Group, true);
		bChanged = true;
	}
	if (bChanged) CallReplicationChangeClient(Object);
}

void UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(UObject* Object)
{
	auto* NetObject = Cast<UNetRelevantObject>(Object);
	if (!NetObject || !IsObjectHasAuthority(Object)) return;
	// AddObject configures groups before registering the object. Only published IDs can need an update.
	if (!NetObject->HasAuthorityLogicStarted()) return;
	// A listen-server callback can remove an object or a player during reconciliation.
	TArray<TWeakObjectPtr<UNetRelevantLocalComponent>> Locals;
	for (auto It = Object->GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (auto* PC = It->Get())
		{
			if (auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>()) Locals.Add(Local);
		}
	}
	for (const auto& WeakLocal : Locals)
	{
		if (auto* Local = WeakLocal.Get()) Local->RefreshObjectVisibility(NetObject);
	}
}

void UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(const APlayerController* PC)
{
	if (!PC || !PC->HasAuthority()) return;
	const auto* Global = GetNetRelevantGlobalComponent(PC);
	auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>();
	if (!Global) return;
	for (auto* Object : Global->GetPureNetRelevantObjects())
	{
		if (!IsValid(Object) || Global->FindNetRelevantObject(Object->Id) != Object) continue;
		if (Object->FollowsOwnerGroups() && Object->PlayerController == PC) CopyNetGroups(Object, PC);
		if (IsValid(Local)) Local->RefreshObjectVisibility(Object);
	}
}

void UNetRelevantObjectFunctionLibrary::CallStopReplicationClientInternal(const APlayerController* PC, UObject* Object)
{
	if (PC)
	{
		if (auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>()) Local->CallEndReplicationEventClient(Object);
	}
}

void UNetRelevantObjectFunctionLibrary::CallStartReplicationClientInternal(const APlayerController* PC, UObject* Object)
{
	if (PC)
	{
		if (auto* Local = PC->GetComponentByClass<UNetRelevantLocalComponent>()) Local->CallBeginReplicationEventClient(Object);
	}
}
