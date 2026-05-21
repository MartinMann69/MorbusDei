// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/MD_Inspectable.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_PlayerInspectComponent.h"


void AMD_Inspectable::Interact_Implementation(APawn* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	UMD_PlayerInspectComponent* InspectComp = Interactor->FindComponentByClass<UMD_PlayerInspectComponent>();
	if (!InspectComp)
	{
		return;
	}

	InspectComp->StartInspect(this);
}

bool AMD_Inspectable::StartInspect(APawn* Interactor, USceneComponent* InspectPivot)
{
	if (bIsInspecting || !Interactor || !InspectPivot)
	{
		return false;
	}

	bIsInspecting = true;

	OriginalTransform = GetActorTransform();
	bOriginalCanInteract = bCanInteract;
	bOriginalActorCollisionEnabled = GetActorEnableCollision();

	if (Root)
	{
		bOriginalSimulatePhysics = Root->IsSimulatingPhysics();
		Root->SetSimulatePhysics(false);
	}

	bCanInteract = false;
	SetActorEnableCollision(false);

	SetActorRotation(
		InspectPivot->GetComponentRotation(),
		ETeleportType::TeleportPhysics
	);

	const FVector BoundsCenter = GetInspectableBoundsCenter();
	const FVector CenterOffset = BoundsCenter - GetActorLocation();
	const FVector NewActorLocation = InspectPivot->GetComponentLocation() - CenterOffset;

	SetActorLocation(
		NewActorLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	AttachToComponent(
		InspectPivot,
		FAttachmentTransformRules::KeepWorldTransform
	);

	return true;
}

void AMD_Inspectable::EndInspect()
{
	if (!bIsInspecting)
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetActorTransform(
		OriginalTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (Root)
	{
		Root->SetSimulatePhysics(bOriginalSimulatePhysics);
	}

	SetActorEnableCollision(bOriginalActorCollisionEnabled);
	bCanInteract = bOriginalCanInteract;

	bIsInspecting = false;
}

bool AMD_Inspectable::IsInspecting() const
{
	return bIsInspecting;
}

FVector AMD_Inspectable::GetInspectableBoundsCenter() const
{
	FBox Bounds(EForceInit::ForceInit);

	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (MeshComp && MeshComp->GetStaticMesh())
		{
			Bounds += MeshComp->Bounds.GetBox();
		}
	}

	if (Bounds.IsValid)
	{
		return Bounds.GetCenter();
	}

	return GetActorLocation();
}