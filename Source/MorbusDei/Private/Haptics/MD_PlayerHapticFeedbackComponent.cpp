#include "Haptics/MD_PlayerHapticFeedbackComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Feedback/MD_FoleyEventRelayComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Haptics/MD_HapticDeveloperSettings.h"
#include "Haptics/MD_HapticSubsystem.h"
#include "Interaction/MD_PlayerInteractionComponent.h"

UMD_PlayerHapticFeedbackComponent::UMD_PlayerHapticFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMD_PlayerHapticFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());
	InteractionComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UMD_PlayerInteractionComponent>()
		: nullptr;
	FoleyEventRelayComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UMD_FoleyEventRelayComponent>()
		: nullptr;

	if (InteractionComponent)
	{
		InteractionComponent->OnInteractionExecuted.AddUniqueDynamic(
			this,
			&UMD_PlayerHapticFeedbackComponent::HandleInteractionExecuted);
		InteractionComponent->OnInteractionFocusChanged.AddUniqueDynamic(
			this,
			&UMD_PlayerHapticFeedbackComponent::HandleInteractionFocusChanged);
	}

	if (FoleyEventRelayComponent)
	{
		FoleyEventPlayedHandle = FoleyEventRelayComponent->OnFoleyEventPlayed().AddUObject(
			this,
			&UMD_PlayerHapticFeedbackComponent::HandleFoleyEventPlayed);
	}
	else if (IsDebugEnabled())
	{
		UE_LOG(LogMDHaptics, Warning,
			TEXT("Foley haptic relay missing: Pawn=%s"),
			*GetNameSafe(GetOwner()));
	}
}

void UMD_PlayerHapticFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionComponent)
	{
		InteractionComponent->OnInteractionExecuted.RemoveDynamic(
			this,
			&UMD_PlayerHapticFeedbackComponent::HandleInteractionExecuted);
		InteractionComponent->OnInteractionFocusChanged.RemoveDynamic(
			this,
			&UMD_PlayerHapticFeedbackComponent::HandleInteractionFocusChanged);
	}

	if (FoleyEventRelayComponent && FoleyEventPlayedHandle.IsValid())
	{
		FoleyEventRelayComponent->OnFoleyEventPlayed().Remove(FoleyEventPlayedHandle);
		FoleyEventPlayedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void UMD_PlayerHapticFeedbackComponent::HandleInteractionExecuted(AActor* InteractedActor)
{
	if (!OwningPawn)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwningPawn->GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
		: nullptr)
	{
		HapticSubsystem->PlayHapticEvent(EMDHapticEvent::Interaction);
	}
}

void UMD_PlayerHapticFeedbackComponent::HandleInteractionFocusChanged(
	AActor* PreviousInteractable,
	AActor* NewInteractable)
{
	if (!NewInteractable || !OwningPawn || !OwningPawn->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwningPawn->GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
		: nullptr)
	{
		HapticSubsystem->PlayHapticEvent(EMDHapticEvent::InteractionFocus);
	}
}

void UMD_PlayerHapticFeedbackComponent::HandleFoleyEventPlayed(const FGameplayTag EventTag)
{
	if (!OwningPawn || !OwningPawn->IsLocallyControlled())
	{
		return;
	}

	const UMD_HapticDeveloperSettings* Settings = GetDefault<UMD_HapticDeveloperSettings>();
	if (!Settings || !Settings->IsFootstepFoleyEvent(EventTag))
	{
		return;
	}

	const bool bPlaybackSucceeded = TryPlayFootstep();
	LogFoleyFootstep(EventTag, bPlaybackSucceeded);
}

bool UMD_PlayerHapticFeedbackComponent::TryPlayFootstep() const
{
	APlayerController* PlayerController = OwningPawn
		? Cast<APlayerController>(OwningPawn->GetController())
		: nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
		: nullptr;

	return HapticSubsystem && HapticSubsystem->PlayHapticEvent(EMDHapticEvent::Footstep);
}

bool UMD_PlayerHapticFeedbackComponent::IsDebugEnabled() const
{
	const IConsoleVariable* DebugVariable =
		IConsoleManager::Get().FindConsoleVariable(TEXT("md.Haptics.Debug"));
	return DebugVariable && DebugVariable->GetInt() != 0;
}

void UMD_PlayerHapticFeedbackComponent::LogFoleyFootstep(
	const FGameplayTag EventTag,
	const bool bPlaybackSucceeded) const
{
	if (!IsDebugEnabled())
	{
		return;
	}

	UE_LOG(LogMDHaptics, Display,
		TEXT("Foley footstep: Event=%s Pawn=%s Playback=%s RealTime=%.3f"),
		*EventTag.ToString(),
		*GetNameSafe(GetOwner()),
		bPlaybackSucceeded ? TEXT("Played") : TEXT("Rejected"),
		GetWorld() ? GetWorld()->GetRealTimeSeconds() : FPlatformTime::Seconds());
}
