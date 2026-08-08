#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MD_PlayerHapticFeedbackComponent.generated.h"

class AActor;
class APawn;
class UMD_FoleyEventRelayComponent;
class UMD_PlayerInteractionComponent;

/** Adapts player-owned gameplay and Foley signals to semantic haptic events. */
UCLASS(ClassGroup = (MD), meta = (BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_PlayerHapticFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_PlayerHapticFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleInteractionExecuted(AActor* InteractedActor);

	UFUNCTION()
	void HandleInteractionFocusChanged(AActor* PreviousInteractable, AActor* NewInteractable);

	void HandleFoleyEventPlayed(FGameplayTag EventTag);
	bool TryPlayFootstep() const;
	bool IsDebugEnabled() const;
	void LogFoleyFootstep(FGameplayTag EventTag, bool bPlaybackSucceeded) const;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OwningPawn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMD_PlayerInteractionComponent> InteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMD_FoleyEventRelayComponent> FoleyEventRelayComponent = nullptr;

	FDelegateHandle FoleyEventPlayedHandle;
};
