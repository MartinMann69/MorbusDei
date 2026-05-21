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

	FVector ViewLocation;
	FRotator ViewRotation;
	OwningPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector PivotLocation = ViewLocation + ViewRotation.Vector() * Item->GetInspectDistance();

	InspectPivot->SetWorldLocationAndRotation(PivotLocation, ViewRotation);

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
