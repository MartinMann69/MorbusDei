#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_PlayerInteractionComponent.generated.h"

class AActor;
class APawn;
class UMD_PlayerInspectComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMDInteractionExecuted,
	AActor*, InteractedActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FMDInteractionFocusChanged,
	AActor*, PreviousInteractable,
	AActor*, NewInteractable);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_PlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_PlayerInteractionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	void Interact();
	void Inspect();

	/** Fired only after an interactable accepted and executed the interaction. */
	UPROPERTY(BlueprintAssignable, Category = "MD|Interaction")
	FMDInteractionExecuted OnInteractionExecuted;

	/** Fired once when the valid interaction focus changes. NewInteractable is null when focus is lost. */
	UPROPERTY(BlueprintAssignable, Category = "MD|Interaction")
	FMDInteractionFocusChanged OnInteractionFocusChanged;

protected:
	UPROPERTY(EditAnywhere, Category="MD|Interaction", meta=(ClampMin="0.0", Units="cm"))
	float InteractDistance = 500.0f;

	/** Maximum angle from camera center at which a new target can be acquired. */
	UPROPERTY(EditAnywhere, Category="MD|Interaction|Aim Assist", meta=(ClampMin="0.0", ClampMax="89.0", Units="Degrees"))
	float FocusAcquireAngleDegrees = 4.0f;

	/** Wider angle used for the current target to prevent focus flicker during small stick movements. */
	UPROPERTY(EditAnywhere, Category="MD|Interaction|Aim Assist", meta=(ClampMin="0.0", ClampMax="89.0", Units="Degrees"))
	float FocusReleaseAngleDegrees = 6.0f;

	UPROPERTY(EditAnywhere, Category="MD|Interaction|Aim Assist", meta=(ClampMin="0.0"))
	float AimAlignmentWeight = 0.95f;

	UPROPERTY(EditAnywhere, Category="MD|Interaction|Aim Assist", meta=(ClampMin="0.0"))
	float DistanceWeight = 0.05f;

	UPROPERTY(EditAnywhere, Category="MD|Interaction|Aim Assist", meta=(ClampMin="0.0"))
	float CurrentFocusScoreBonus = 0.03f;

	UPROPERTY()
	AActor* CurrentFocusedInteractable = nullptr;

	UPROPERTY()
	APawn* OwningPawn = nullptr;

	UPROPERTY()
	UMD_PlayerInspectComponent* InspectComp = nullptr;

	void UpdateInteractionFocus();
	void SetInteractionFocus(AActor* NewInteractable);
	void ClearInteractionFocus();

	AActor* FindDirectInteractionCandidate(const FVector& ViewLocation, const FVector& ViewDirection) const;
	AActor* FindBestInteractionCandidate(const FVector& ViewLocation, const FVector& ViewDirection) const;
	bool IsInteractionCandidateAvailable(AActor* Candidate) const;
	FVector GetInteractionFocusLocation(const AActor* Candidate) const;
	bool HasLineOfSightToCandidate(const AActor* Candidate, const FVector& ViewLocation, const FVector& FocusLocation) const;
};
