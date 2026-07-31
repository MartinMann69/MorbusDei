#include "Interaction/MD_Interactable.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Components/WidgetComponent.h"
#include "Interaction/MD_HighlightComponent.h"
#include "Interaction/MD_InteractPromptComponent.h"
#include "Interaction/MD_InspectableComponent.h"
#include "Audio/MD_AudioZone.h"
#include "Kismet/GameplayStatics.h"

AMD_Interactable::AMD_Interactable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	HighlightedObjects = CreateDefaultSubobject<USceneComponent>(TEXT("HighlightedObjects"));
	HighlightedObjects->SetupAttachment(Root);

	ToggleableObjects = CreateDefaultSubobject<USceneComponent>(TEXT("ToggleableObjects"));
	ToggleableObjects->SetupAttachment(Root);

	InteractPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidget"));
	InteractPromptWidget->SetupAttachment(RootComponent);
	InteractPromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	InteractPromptWidget->SetWidgetSpace(EWidgetSpace::Screen); // or World
	InteractPromptWidget->SetDrawSize(FVector2D(200.f, 200.f));
	InteractPromptWidget->SetVisibility(false);
	InteractPromptWidget->SetPivot(FVector2D(0, 0));

	HighlightComponent = CreateDefaultSubobject<UMD_HighlightComponent>(TEXT("HighlightComponent"));
	HighlightComponent->SetHighlightRoot(HighlightedObjects);

	InteractPromptComponent = CreateDefaultSubobject<UMD_InteractPromptComponent>(TEXT("InteractPromptComponent"));
	InteractPromptComponent->SetPromptWidget(InteractPromptWidget);

	InspectableComponent = CreateDefaultSubobject<UMD_InspectableComponent>(TEXT("InspectableComponent"));
}

void AMD_Interactable::BeginPlay()
{
	Super::BeginPlay();

	if (InspectableComponent)
	{
		InspectableComponent->SetInspectSound(InspectSound);
	}
}

void AMD_Interactable::Interact_Implementation(APawn* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("%s interacted with %s"),*GetNameSafe(Interactor),*GetNameSafe(this));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Interacted with %s"), *GetNameSafe(this)));
	}
	
	const bool bVoiceoverBlocked = IsVoiceoverAudioBlocked();
	
	if (!bVoiceoverBlocked)
	{
		PlayOneShotAtSelf(InteractionSound);

		if (AudioZoneClass)
		{
			if (VoiceoverDelayAfterInteractionSound > 0.0f)
			{
				bVoiceoverStartPending = true;

				FTimerDelegate VoiceoverDelegate;
				VoiceoverDelegate.BindUObject(this, &AMD_Interactable::PlayAssignedAudioZone, Interactor);

				GetWorldTimerManager().SetTimer(VoiceoverDelayTimer, VoiceoverDelegate, VoiceoverDelayAfterInteractionSound, false);
			}
			else
			{
				PlayAssignedAudioZone(Interactor);
			}
		}
	}
	
	if (!ToggleableObjects)
	{
		return;
	}
	
	TArray<USceneComponent*> ToggleChildComponents;
	ToggleableObjects->GetChildrenComponents(true, ToggleChildComponents);

	for (USceneComponent* Child : ToggleChildComponents)
	{
		Child->ToggleVisibility();
	}
}

void AMD_Interactable::SetInteractPromptVisible_Implementation(bool bVisible)
{
	if (InteractPromptComponent)
	{
		InteractPromptComponent->SetPromptVisible(bVisible);
	}
}

bool AMD_Interactable::CanInteract_Implementation() const
{
	return bCanInteract;
}

void AMD_Interactable::Highlight_Implementation(bool bHighlight)
{
	if (HighlightComponent)
	{
		HighlightComponent->SetHighlighted(bHighlight);
	}
}

void AMD_Interactable::PlayAssignedAudioZone(APawn* Interactor)
{
	bVoiceoverStartPending = false;

	if (!AudioZoneClass)
	{
		return;
	}

	if (IsValid(SpawnedAudioZone) && SpawnedAudioZone->IsZoneSoundPlaying())
	{
		return;
	}

	const bool bShouldSpawnAudioZone =
		!IsValid(SpawnedAudioZone) && (!bSpawnAudioZoneOnlyOnce || SpawnedAudioZone == nullptr);

	if (bShouldSpawnAudioZone)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Interactor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedAudioZone = GetWorld()->SpawnActor<AMD_AudioZone>(AudioZoneClass, GetActorTransform(), SpawnParams);
	}

	if (IsValid(SpawnedAudioZone))
	{
		SpawnedAudioZone->PlayZoneSound();
	}
}

void AMD_Interactable::PlayOneShotAtSelf(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

bool AMD_Interactable::IsVoiceoverAudioBlocked() const
{
	return bVoiceoverStartPending || (IsValid(SpawnedAudioZone) && SpawnedAudioZone->IsZoneSoundPlaying());
}

