#include "Interaction/MD_PlayerInteractionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_InteractInterface.h"
#include "Interaction/MD_PlayerInspectComponent.h"

UMD_PlayerInteractionComponent::UMD_PlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMD_PlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());
	InspectComp = GetOwner()->FindComponentByClass<UMD_PlayerInspectComponent>();
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
		InspectComp->EndInspect();
		return;
	}

	if (!CurrentFocusedInteractable)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Interacted"));
	IMD_InteractInterface::Execute_Interact(CurrentFocusedInteractable, OwningPawn);
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
	if (!bCanInteract)
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
	
	UE_LOG(LogTemp, Warning, TEXT("Hit"));
	IMD_InteractInterface::Execute_SetInteractPromptVisible(CurrentFocusedInteractable, true);
	IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, true);
}