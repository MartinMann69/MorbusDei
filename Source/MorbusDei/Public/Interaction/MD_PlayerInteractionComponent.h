#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_PlayerInteractionComponent.generated.h"

class AActor;
class APawn;
class UMD_PlayerInspectComponent;

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
	void ClearInteractionFocus();
};