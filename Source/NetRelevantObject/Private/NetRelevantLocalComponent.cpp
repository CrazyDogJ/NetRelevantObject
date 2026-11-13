#include "NetRelevantLocalComponent.h"

#include "GameFramework/PlayerController.h"
#include "NetRelevantGlobalComponent.h"
#include "NetRelevantObject.h"
#include "NetRelevantObjectUtils.h"
#include "Net/UnrealNetwork.h"

UNetRelevantLocalComponent::UNetRelevantLocalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNetRelevantLocalComponent::OnRegister()
{
	Super::OnRegister();
	VisibleIds.Owner = this;
}

void UNetRelevantLocalComponent::BeginPlay()
{
	Super::BeginPlay();
	bEndingPlay = false;
	if (auto* PC = Cast<APlayerController>(GetOwner()); PC && PC->HasAuthority())
	{
		PC->IncludeInNetConditionGroup(GLOBAL_GROUP);
		UNetRelevantObjectFunctionLibrary::CallReplicationChangeClient(PC);
	}
	// The initial owner-only delta can arrive before BeginPlay.
	TArray<FGuid> Ids;
	for (const auto& Item : VisibleIds.Items) Ids.Add(Item.Id);
	for (const FGuid& Id : Ids) RefreshLocalPresentation(Id);
}

void UNetRelevantLocalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	TArray<FGuid> Ids;
	for (const auto& Item : VisibleIds.Items) Ids.Add(Item.Id);
	for (const FGuid& Id : Ids) RefreshLocalPresentation(Id);
	Super::EndPlay(EndPlayReason);
}

FName UNetRelevantLocalComponent::GetOrCreatePrivateGroup()
{
	auto* PC = CastChecked<APlayerController>(GetOwner());
	check(PC->HasAuthority());
	if (PrivateGroup.IsNone())
	{
		PrivateGroup = FName(*(TEXT("NetRelevantPrivate_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		PC->IncludeInNetConditionGroup(PrivateGroup);
	}
	return PrivateGroup;
}

void UNetRelevantLocalComponent::AddNetConditionGroup(FName GroupName) const
{
	UNetRelevantObjectFunctionLibrary::AddPlayerNetGroup(Cast<APlayerController>(GetOwner()), GroupName);
}

void UNetRelevantLocalComponent::RemoveNetConditionGroup(FName GroupName) const
{
	UNetRelevantObjectFunctionLibrary::RemovePlayerNetGroup(Cast<APlayerController>(GetOwner()), GroupName);
}

void UNetRelevantLocalComponent::ClearNetConditionGroups() const
{
	UNetRelevantObjectFunctionLibrary::ClearPlayerNetGroups(Cast<APlayerController>(GetOwner()));
}

void UNetRelevantLocalComponent::RefreshObjectVisibility(UNetRelevantObject* Object)
{
	auto* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->HasAuthority() || !IsValid(Object)) return;
	const bool bVisible = Object->HasAuthorityLogicStarted() &&
		UNetRelevantObjectFunctionLibrary::CanPlayerReceiveSubobject(
			UNetRelevantObjectFunctionLibrary::GetObjectNetGroupsView(Object), PC->GetNetConditionGroups());
	SetObjectVisible(Object, bVisible);
}

void UNetRelevantLocalComponent::SetObjectVisible(UNetRelevantObject* Object, bool bVisible)
{
	auto* PC = Cast<APlayerController>(GetOwner());
	if (PC && PC->HasAuthority() && IsValid(Object) && Object->GetOwnerComponent() &&
		VisibleIds.SetVisible(Object->Id, bVisible))
	{
		PC->ForceNetUpdate();
		if (PC->IsLocalPlayerController()) Object->GetOwnerComponent()->RefreshLocalVisibility(Object->Id);
	}
}

void UNetRelevantLocalComponent::RefreshLocalPresentation(FGuid Id) const
{
	const auto* PC = Cast<APlayerController>(GetOwner());
	if (PC && PC->IsLocalPlayerController())
	{
		if (auto* Global = UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(this))
			Global->RefreshLocalVisibility(Id);
	}
}

void UNetRelevantLocalComponent::CallBeginReplicationEventClient(UObject* InObject)
{
	RefreshObjectVisibility(Cast<UNetRelevantObject>(InObject));
}

void UNetRelevantLocalComponent::CallEndReplicationEventClient(UObject* InObject)
{
	SetObjectVisible(Cast<UNetRelevantObject>(InObject), false);
}

void UNetRelevantLocalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, VisibleIds, COND_OwnerOnly);
}
