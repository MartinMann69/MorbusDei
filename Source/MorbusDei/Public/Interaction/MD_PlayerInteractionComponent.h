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
	UPROPERTY(EditAnywhere, Category="MD|Interaction")
	float InteractDistance = 500.0f;

	UPROPERTY()
	AActor* CurrentFocusedInteractable = nullptr;

	UPROPERTY()
	APawn* OwningPawn = nullptr;

	UPROPERTY()
	UMD_PlayerInspectComponent* InspectComp = nullptr;

	void UpdateInteractionFocus();
	void SetInteractionFocus(AActor* NewInteractable);
	void ClearInteractionFocus();
};
