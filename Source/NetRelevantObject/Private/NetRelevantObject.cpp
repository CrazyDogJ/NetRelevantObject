// Fill out your copyright notice in the Description page of Project Settings.


#include "NetRelevantObject.h"

#include "NetRelevantGlobalComponent.h"
#include "NetRelevantObjectUtils.h"
#include "Net/UnrealNetwork.h"

UNetRelevantObject::UNetRelevantObject()
{
}

void UNetRelevantObject::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
	
	DOREPLIFETIME(ThisClass, Id);
}

void UNetRelevantObject::PreDestroyFromReplication()
{
	if (auto* Global = GetOwnerComponent()) Global->ForgetReplica(this);
	NetEndPlay();
	
	Super::PreDestroyFromReplication();
}

void UNetRelevantObject::OnControllerDestroyed(AActor* DestroyedActor)
{
	if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
	{
		Global->RemoveNetRelevantObject(Id);
	}
}

void UNetRelevantObject::NativeAuthorityBeginPlay()
{
	// Make sure the life cycle is sync with the owning player controller.
	if (PlayerController)
	{
		PlayerController->OnDestroyed.AddUniqueDynamic(this, &UNetRelevantObject::OnControllerDestroyed);
	}
	
	AuthorityBeginPlay();
}

void UNetRelevantObject::NativeAuthorityEndPlay()
{
	// 修复：提前移除对象时同步解除控制器销毁回调。
	if (PlayerController)
	{
		PlayerController->OnDestroyed.RemoveDynamic(this, &UNetRelevantObject::OnControllerDestroyed);
	}
	AuthorityEndPlay();
}

void UNetRelevantObject::NativeBeginPlay()
{
	BeginPlay();
}

void UNetRelevantObject::NativeEndPlay()
{
	EndPlay();
}

void UNetRelevantObject::NetBeginPlay()
{
	if (!bIsBegunPlay && Id.IsValid())
	{
		bIsBegunPlay = true;
		
		if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
		{
			Global->LocalNetRelevantObjects.Add(Id, this);
		}
		
		NativeBeginPlay();
	}
}

void UNetRelevantObject::NetEndPlay()
{
	if (bIsBegunPlay)
	{
		bIsBegunPlay = false;
	
		NativeEndPlay();
	
		if (const auto Global = Cast<UNetRelevantGlobalComponent>(GetOuter()))
		{
			// 修复：结束事件可能重入并重新启动对象，不抹去新的登记。
			if (!bIsBegunPlay && Global->LocalNetRelevantObjects.FindRef(Id) == this)
			{
				Global->LocalNetRelevantObjects.Remove(Id);
			}
		}
	}
}

UNetRelevantGlobalComponent* UNetRelevantObject::GetOwnerComponent() const
{
	return Cast<UNetRelevantGlobalComponent>(GetOuter());
}

void UNetRelevantObject::AddNetGroup(const FName InGroupName)
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::AddObjectNetGroup(this, InGroupName);
}

void UNetRelevantObject::RemoveNetGroup(const FName InGroupName)
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::RemoveObjectNetGroup(this, InGroupName);
}

void UNetRelevantObject::ClearNetGroups()
{
	if (!UNetRelevantObjectFunctionLibrary::IsObjectHasAuthority(this))
	{
		return;
	}
	
	UNetRelevantObjectFunctionLibrary::ClearNetGroups(this);
}

void UNetRelevantObject::OnRep_Id()
{
	if (Id.IsValid())
	{
		if (auto* Global = GetOwnerComponent()) Global->RegisterReplica(this);
	}
}
