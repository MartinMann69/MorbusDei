#include "Interaction/MD_PlayerInteractionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_InteractInterface.h"
#include "Interaction/MD_InspectableComponent.h"
#include "Interaction/MD_PlayerInspectComponent.h"

UMD_PlayerInteractionComponent::UMD_PlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMD_PlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());

	if (GetOwner())
	{
		InspectComp = GetOwner()->FindComponentByClass<UMD_PlayerInspectComponent>();
	}
}

void UMD_PlayerInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInteractionFocus();
}

void UMD_PlayerInteractionComponent::Interact()
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		return;
	}

	if (!CurrentFocusedInteractable)
	{
		return;
	}

	if (!IMD_InteractInterface::Execute_CanInteract(CurrentFocusedInteractable))
	{
		return;
	}

	IMD_InteractInterface::Execute_Interact(CurrentFocusedInteractable, OwningPawn);
}

void UMD_PlayerInteractionComponent::Inspect()
{
	if (!InspectComp)
	{
		return;
	}

	if (InspectComp->IsInspecting())
	{
		InspectComp->EndInspect();
		return;
	}

	if (!CurrentFocusedInteractable)
	{
		return;
	}

	UMD_InspectableComponent* Inspectable =
		CurrentFocusedInteractable->FindComponentByClass<UMD_InspectableComponent>();

	if (!Inspectable || !Inspectable->CanInspect())
	{
		return;
	}

	ClearInteractionFocus();
	InspectComp->StartInspect(Inspectable);
}

void UMD_PlayerInteractionComponent::ClearInteractionFocus()
{
	if (CurrentFocusedInteractable &&
		CurrentFocusedInteractable->Implements<UMD_InteractInterface>())
	{
		IMD_InteractInterface::Execute_SetInteractPromptVisible(CurrentFocusedInteractable, false);
		IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, false);
	}

	CurrentFocusedInteractable = nullptr;
}

void UMD_PlayerInteractionComponent::UpdateInteractionFocus()
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		ClearInteractionFocus();
		return;
	}
	
	if (!OwningPawn || !OwningPawn->GetController() || !GetWorld())
	{
		ClearInteractionFocus();
		return;
	}
	
	FVector Start;
	FRotator ViewRotation;
	OwningPawn->GetController()->GetPlayerViewPoint(Start, ViewRotation);

	const FVector End = Start + (ViewRotation.Vector() * InteractDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwningPawn);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (!bHit || !Hit.GetActor())
	{
		ClearInteractionFocus();
		return;
	}

	AActor* HitActor = Hit.GetActor();

	if (!HitActor->Implements<UMD_InteractInterface>())
	{
		ClearInteractionFocus();
		return;
	}

	const bool bCanInteract = IMD_InteractInterface::Execute_CanInteract(HitActor);

	const UMD_InspectableComponent* Inspectable = HitActor->FindComponentByClass<UMD_InspectableComponent>();

	const bool bCanInspect = Inspectable && Inspectable->CanInspect();
	
	if (!bCanInteract && !bCanInspect)
	{
		ClearInteractionFocus();
		return;
	}

	if (CurrentFocusedInteractable == HitActor)
	{
		return;
	}

	ClearInteractionFocus();

	CurrentFocusedInteractable = HitActor;
	
	IMD_InteractInterface::Execute_SetInteractPromptVisible(CurrentFocusedInteractable, true);
	IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, true);
}