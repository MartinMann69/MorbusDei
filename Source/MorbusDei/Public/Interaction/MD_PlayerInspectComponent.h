#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_PlayerInspectComponent.generated.h"

class APawn;
class USceneComponent;
class UMD_InspectableComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_PlayerInspectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_PlayerInspectComponent();

	virtual void BeginPlay() override;

	bool StartInspect(UMD_InspectableComponent* Inspectable);
	void EndInspect();
	void RotateInspectedItem(const FVector2D& LookInput);
	void ZoomInspectedItem(float ZoomInput);

	bool IsInspecting() const;

protected:
	float CurrentInspectDistance = 0.f;

	FVector InspectViewLocation = FVector::ZeroVector;
	FRotator InspectViewRotation = FRotator::ZeroRotator;

	UPROPERTY()
	APawn* OwningPawn = nullptr;

	UPROPERTY()
	UMD_InspectableComponent* CurrentInspectable = nullptr;

	UPROPERTY()
	USceneComponent* InspectPivot = nullptr;

	void EnsureInspectPivot();
	void UpdateInspectPivotLocation();
};