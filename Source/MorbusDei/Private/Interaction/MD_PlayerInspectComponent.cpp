// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/MD_PlayerInspectComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_Inspectable.h"

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

bool UMD_PlayerInspectComponent::StartInspect(AMD_Inspectable* Item)
{
	if (!Item)
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
	
	if (CurrentInspectedItem)
	{
		EndInspect();
	}
	
	OwningPawn->GetController()->GetPlayerViewPoint(InspectViewLocation, InspectViewRotation);

	const float MinDistance = FMath::Min(Item->GetMinInspectDistance(), Item->GetMaxInspectDistance());
	const float MaxDistance = FMath::Max(Item->GetMinInspectDistance(), Item->GetMaxInspectDistance());

	CurrentInspectDistance = FMath::Clamp(Item->GetInspectDistance(), MinDistance, MaxDistance);
	
	const FVector PivotLocation = InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	InspectPivot->SetWorldLocationAndRotation(PivotLocation, InspectViewRotation);

	CurrentInspectedItem = Item;

	if (!CurrentInspectedItem->StartInspect(OwningPawn, InspectPivot))
	{
		CurrentInspectedItem = nullptr;
		return false;
	}

	return true;
}

void UMD_PlayerInspectComponent::EndInspect()
{
	if (!CurrentInspectedItem)
	{
		return;
	}

	AMD_Inspectable* ItemToEnd = CurrentInspectedItem;
	CurrentInspectedItem = nullptr;

	ItemToEnd->EndInspect();
}

void UMD_PlayerInspectComponent::RotateInspectedItem(const FVector2D& LookInput)
{
	if (!CurrentInspectedItem || !InspectPivot)
	{
		return;
	}

	const float RotationSpeed = CurrentInspectedItem->GetRotationSpeed();

	const float YawAmount = LookInput.X * RotationSpeed;
	const float PitchAmount = LookInput.Y * RotationSpeed;

	InspectPivot->AddWorldRotation(FRotator(0.f, YawAmount, 0.f));
	InspectPivot->AddLocalRotation(FRotator(PitchAmount, 0.f, 0.f));
}

void UMD_PlayerInspectComponent::ZoomInspectedItem(float ZoomInput)
{
	if (!CurrentInspectedItem || !InspectPivot)
	{
		return;
	}

	const float MinDistance = FMath::Min(
		CurrentInspectedItem->GetMinInspectDistance(),
		CurrentInspectedItem->GetMaxInspectDistance()
	);

	const float MaxDistance = FMath::Max(
		CurrentInspectedItem->GetMinInspectDistance(),
		CurrentInspectedItem->GetMaxInspectDistance()
	);

	CurrentInspectDistance = FMath::Clamp(
		CurrentInspectDistance + ZoomInput * CurrentInspectedItem->GetZoomSpeed(),
		MinDistance,
		MaxDistance
	);

	UpdateInspectPivotLocation();
}

void UMD_PlayerInspectComponent::UpdateInspectPivotLocation()
{
	if (!InspectPivot)
	{
		return;
	}

	const FVector NewLocation =
		InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	InspectPivot->SetWorldLocation(NewLocation);
}

bool UMD_PlayerInspectComponent::IsInspecting() const
{
	return CurrentInspectedItem != nullptr;
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
