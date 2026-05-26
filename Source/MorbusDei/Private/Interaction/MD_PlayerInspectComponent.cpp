// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/MD_PlayerInspectComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_InspectableComponent.h"

// Sets default values
UMD_PlayerInspectComponent::UMD_PlayerInspectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMD_PlayerInspectComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());
	EnsureInspectPivot();
}

bool UMD_PlayerInspectComponent::StartInspect(UMD_InspectableComponent* Inspectable)
{
	if (!Inspectable || !Inspectable->CanInspect())
	{
		return false;
	}

	if (!OwningPawn)
	{
		OwningPawn = Cast<APawn>(GetOwner());
	}

	if (!OwningPawn || !OwningPawn->GetController())
	{
		return false;
	}

	EnsureInspectPivot();
	
	if (!InspectPivot)
	{
		return false;
	}
	
	if (CurrentInspectable)
	{
		EndInspect();
	}
	
	OwningPawn->GetController()->GetPlayerViewPoint(InspectViewLocation, InspectViewRotation);

	const float MinDistance = FMath::Min(Inspectable->GetMinInspectDistance(), Inspectable->GetMaxInspectDistance());
	const float MaxDistance = FMath::Max(Inspectable->GetMinInspectDistance(), Inspectable->GetMaxInspectDistance());

	CurrentInspectDistance = FMath::Clamp(Inspectable->GetInspectDistance(), MinDistance, MaxDistance);
	
	const FVector PivotLocation = InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	InspectPivot->SetWorldLocationAndRotation(PivotLocation, InspectViewRotation);

	CurrentInspectable = Inspectable;

	if (!CurrentInspectable->StartInspect(OwningPawn, InspectPivot))
	{
		CurrentInspectable = nullptr;
		return false;
	}

	return true;
}

void UMD_PlayerInspectComponent::EndInspect()
{
	if (!CurrentInspectable)
	{
		return;
	}

	UMD_InspectableComponent* InspectableToEnd = CurrentInspectable;
	CurrentInspectable = nullptr;

	InspectableToEnd->EndInspect();
}

void UMD_PlayerInspectComponent::RotateInspectedItem(const FVector2D& LookInput)
{
	if (!CurrentInspectable || !InspectPivot)
	{
		return;
	}

	const float YawAmount = LookInput.X * CurrentInspectable->GetRotationSpeed();
	const float PitchAmount = LookInput.Y * CurrentInspectable->GetRotationSpeed();

	InspectPivot->AddWorldRotation(FRotator(0.f, YawAmount, 0.f));
	InspectPivot->AddLocalRotation(FRotator(PitchAmount, 0.f, 0.f));
}

void UMD_PlayerInspectComponent::ZoomInspectedItem(float ZoomInput)
{
	if (!CurrentInspectable || !InspectPivot)
	{
		return;
	}

	const float MinDistance = FMath::Min(
		CurrentInspectable->GetMinInspectDistance(),
		CurrentInspectable->GetMaxInspectDistance()
	);

	const float MaxDistance = FMath::Max(
		CurrentInspectable->GetMinInspectDistance(),
		CurrentInspectable->GetMaxInspectDistance()
	);

	CurrentInspectDistance = FMath::Clamp(
		CurrentInspectDistance + ZoomInput * CurrentInspectable->GetZoomSpeed(),
		MinDistance,
		MaxDistance
	);

	UpdateInspectPivotLocation();
}

bool UMD_PlayerInspectComponent::IsInspecting() const
{
	return CurrentInspectable != nullptr;
}

void UMD_PlayerInspectComponent::EnsureInspectPivot()
{
	if (InspectPivot || !GetOwner())
	{
		return;
	}

	InspectPivot = NewObject<USceneComponent>(GetOwner(), TEXT("InspectPivot"));
	InspectPivot->SetMobility(EComponentMobility::Movable);

	GetOwner()->AddInstanceComponent(InspectPivot);
	InspectPivot->RegisterComponent();
}

void UMD_PlayerInspectComponent::UpdateInspectPivotLocation()
{
	if (!InspectPivot)
	{
		return;
	}

	const FVector NewLocation = InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	InspectPivot->SetWorldLocation(NewLocation);
}