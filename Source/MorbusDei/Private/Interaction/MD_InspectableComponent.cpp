#include "Interaction/MD_InspectableComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"

UMD_InspectableComponent::UMD_InspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMD_InspectableComponent::CanInspect() const
{
	return bCanInspect && !bIsInspecting;
}

bool UMD_InspectableComponent::StartInspect(APawn* Interactor, USceneComponent* InspectPivot)
{
	AActor* Owner = GetOwner();

	if (!CanInspect() || !Owner || !Interactor || !InspectPivot)
	{
		return false;
	}

	bIsInspecting = true;
	OriginalTransform = Owner->GetActorTransform();
	bOriginalActorCollisionEnabled = Owner->GetActorEnableCollision();

	DisableOwnerPhysics();
	Owner->SetActorEnableCollision(false);

	Owner->SetActorRotation(
		InspectPivot->GetComponentRotation(),
		ETeleportType::TeleportPhysics
	);

	const FVector BoundsCenter = GetInspectableBoundsCenter();
	const FVector CenterOffset = BoundsCenter - Owner->GetActorLocation();
	const FVector NewActorLocation = InspectPivot->GetComponentLocation() - CenterOffset;

	Owner->SetActorLocation(
		NewActorLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	Owner->AttachToComponent(
		InspectPivot,
		FAttachmentTransformRules::KeepWorldTransform
	);

	return true;
}

void UMD_InspectableComponent::EndInspect()
{
	AActor* Owner = GetOwner();

	if (!bIsInspecting || !Owner)
	{
		return;
	}

	Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	Owner->SetActorTransform(
		OriginalTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	RestoreOwnerPhysics();
	Owner->SetActorEnableCollision(bOriginalActorCollisionEnabled);

	bIsInspecting = false;
}

bool UMD_InspectableComponent::IsInspecting() const
{
	return bIsInspecting;
}

FVector UMD_InspectableComponent::GetInspectableBoundsCenter() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	FBox Bounds(EForceInit::ForceInit);

	TArray<UStaticMeshComponent*> MeshComponents;
	Owner->GetComponents<UStaticMeshComponent>(MeshComponents);

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

	return Owner->GetActorLocation();
}

void UMD_InspectableComponent::DisableOwnerPhysics()
{
	SimulatingPrimitiveComponents.Reset();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive && Primitive->IsSimulatingPhysics())
		{
			SimulatingPrimitiveComponents.Add(Primitive);
			Primitive->SetSimulatePhysics(false);
		}
	}
}

void UMD_InspectableComponent::RestoreOwnerPhysics()
{
	for (UPrimitiveComponent* Primitive : SimulatingPrimitiveComponents)
	{
		if (Primitive)
		{
			Primitive->SetSimulatePhysics(true);
		}
	}

	SimulatingPrimitiveComponents.Reset();
}