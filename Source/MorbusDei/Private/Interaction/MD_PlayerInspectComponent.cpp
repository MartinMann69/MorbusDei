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
	PrimaryComponentTick.bStartWithTickEnabled = false;

}

void UMD_PlayerInspectComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());
	EnsureInspectPivot();
}

void UMD_PlayerInspectComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (InspectState)
	{
	case EMD_InspectState::Entering:
		UpdateEnterTransition(DeltaTime);
		break;

	case EMD_InspectState::Active:
		UpdateSmoothZoom(DeltaTime);
		UpdateSmoothRotation(DeltaTime);
		break;

	case EMD_InspectState::Exiting:
		UpdateExitTransition(DeltaTime);
		break;

	case EMD_InspectState::Inactive:
	default:
		break;
	}
}

bool UMD_PlayerInspectComponent::StartInspect(UMD_InspectableComponent* Inspectable)
{
	if (InspectState != EMD_InspectState::Inactive)
	{
		return false;
	}
	
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
	
	OwningPawn->GetController()->GetPlayerViewPoint(InspectViewLocation, InspectViewRotation);

	const float MinDistance = FMath::Min(Inspectable->GetMinInspectDistance(), Inspectable->GetMaxInspectDistance());
	const float MaxDistance = FMath::Max(Inspectable->GetMinInspectDistance(), Inspectable->GetMaxInspectDistance());

	CurrentInspectDistance = FMath::Clamp(Inspectable->GetInspectDistance(), MinDistance, MaxDistance);
	
	TargetInspectDistance = CurrentInspectDistance;
	
	RotationVelocity = FVector2D::ZeroVector;
	CurrentInspectPitch = 0.f;
	
	const FVector PivotLocation = InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	const FTransform DesiredPivotTransform(InspectViewRotation, PivotLocation, FVector::OneVector);

	CurrentInspectable = Inspectable;

	if (!CurrentInspectable->StartInspect(OwningPawn, InspectPivot))
	{
		CurrentInspectable = nullptr;
		return false;
	}

	OriginalPivotTransform = InspectPivot->GetComponentTransform();

	TransitionStartTransform = OriginalPivotTransform;
	TransitionTargetTransform = DesiredPivotTransform;
	TransitionElapsed = 0.f;

	SetInspectState(EMD_InspectState::Entering);
	return true;
}

void UMD_PlayerInspectComponent::EndInspect()
{
	if (InspectState == EMD_InspectState::Inactive ||InspectState == EMD_InspectState::Exiting ||!CurrentInspectable ||!InspectPivot)
	{
		return;
	}

	TransitionStartTransform = InspectPivot->GetComponentTransform();
	TransitionTargetTransform = OriginalPivotTransform;
	TransitionElapsed = 0.f;
	RotationVelocity = FVector2D::ZeroVector;

	SetInspectState(EMD_InspectState::Exiting);
}

void UMD_PlayerInspectComponent::SetInspectState(EMD_InspectState NewState)
{
	InspectState = NewState;

	SetComponentTickEnabled(InspectState != EMD_InspectState::Inactive);
}

void UMD_PlayerInspectComponent::UpdateEnterTransition(float DeltaTime)
{
	if (!CurrentInspectable || !InspectPivot)
	{
		return;
	}

	TransitionElapsed += DeltaTime;

	const float Duration = CurrentInspectable->GetEnterDuration();

	const float Alpha = Duration <= KINDA_SMALL_NUMBER ? 1.f : FMath::Clamp(TransitionElapsed / Duration, 0.f, 1.f);

	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	FTransform NewTransform;
	NewTransform.Blend(TransitionStartTransform,TransitionTargetTransform,SmoothAlpha);

	InspectPivot->SetWorldTransform(NewTransform);

	if (Alpha >= 1.f)
	{
		InspectPivot->SetWorldTransform(TransitionTargetTransform);
		SetInspectState(EMD_InspectState::Active);
	}
}

void UMD_PlayerInspectComponent::UpdateExitTransition(float DeltaTime)
{
	if (!CurrentInspectable || !InspectPivot)
	{
		return;
	}

	TransitionElapsed += DeltaTime;

	const float Duration = CurrentInspectable->GetExitDuration();

	const float Alpha = Duration <= KINDA_SMALL_NUMBER ? 1.f : FMath::Clamp(TransitionElapsed / Duration, 0.f, 1.f);

	const float SmoothAlpha =FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	FTransform NewTransform;
	NewTransform.Blend(TransitionStartTransform,TransitionTargetTransform,SmoothAlpha);

	InspectPivot->SetWorldTransform(NewTransform);

	if (Alpha >= 1.f)
	{
		InspectPivot->SetWorldTransform(TransitionTargetTransform);

		UMD_InspectableComponent* FinishedInspectable = CurrentInspectable;

		CurrentInspectable = nullptr;
		FinishedInspectable->EndInspect();

		SetInspectState(EMD_InspectState::Inactive);
	}
}

void UMD_PlayerInspectComponent::RotateInspectedItem(const FVector2D& LookInput)
{
	if (InspectState != EMD_InspectState::Active || !CurrentInspectable || !InspectPivot)
	{
		return;
	}

	RotationVelocity += LookInput * CurrentInspectable->GetRotationSpeed();

	if (MaxRotationVelocity > 0.f)
	{
		RotationVelocity = RotationVelocity.GetClampedToMaxSize(MaxRotationVelocity);
	}
}

void UMD_PlayerInspectComponent::UpdateSmoothRotation(float DeltaTime)
{
	if (!CurrentInspectable || !InspectPivot || RotationVelocity.IsNearlyZero())
	{
		return;
	}

	const float YawAmount = RotationVelocity.X;
	float PitchAmount = RotationVelocity.Y;

	if (bLimitInspectPitch)
	{
		const float NewPitch = FMath::Clamp(CurrentInspectPitch + PitchAmount, MinInspectPitch, MaxInspectPitch);

		PitchAmount = NewPitch - CurrentInspectPitch;
		CurrentInspectPitch = NewPitch;

		if (FMath::IsNearlyZero(PitchAmount))
		{
			RotationVelocity.Y = 0.f;
		}
	}
	else
	{
		CurrentInspectPitch += PitchAmount;
	}

	InspectPivot->AddWorldRotation(FRotator(0.f, YawAmount, 0.f));
	InspectPivot->AddLocalRotation(FRotator(PitchAmount, 0.f, 0.f));

	RotationVelocity.X = FMath::FInterpTo(RotationVelocity.X, 0.f, DeltaTime, RotationDamping);

	RotationVelocity.Y = FMath::FInterpTo(RotationVelocity.Y, 0.f, DeltaTime, RotationDamping);
}

void UMD_PlayerInspectComponent::ZoomInspectedItem(float ZoomInput)
{
	if (InspectState != EMD_InspectState::Active || !CurrentInspectable || !InspectPivot)
	{
		return;
	}

	const float MinDistance = FMath::Min(CurrentInspectable->GetMinInspectDistance(), CurrentInspectable->GetMaxInspectDistance());

	const float MaxDistance = FMath::Max(CurrentInspectable->GetMinInspectDistance(), CurrentInspectable->GetMaxInspectDistance());

	TargetInspectDistance = FMath::Clamp(TargetInspectDistance + ZoomInput * CurrentInspectable->GetZoomSpeed(), MinDistance, MaxDistance);
}

void UMD_PlayerInspectComponent::UpdateSmoothZoom(float DeltaTime)
{
	if (!CurrentInspectable || !InspectPivot)
	{
		return;
	}

	const float NewDistance = FMath::FInterpTo(CurrentInspectDistance, TargetInspectDistance, DeltaTime, ZoomInterpSpeed);

	if (FMath::IsNearlyEqual(CurrentInspectDistance, NewDistance, 0.01f))
	{
		return;
	}

	CurrentInspectDistance = NewDistance;
	UpdateInspectPivotLocation();
}

bool UMD_PlayerInspectComponent::IsInspecting() const
{
	return InspectState != EMD_InspectState::Inactive;
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