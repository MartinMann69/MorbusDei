#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_InspectableComponent.generated.h"

class APawn;
class USceneComponent;
class UPrimitiveComponent;
class USoundBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_InspectableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_InspectableComponent();

	bool CanInspect() const;
	bool StartInspect(APawn* Interactor, USceneComponent* InspectPivot);
	void EndInspect();

	bool IsInspecting() const;

	float GetInspectDistance() const { return InspectDistance; }
	float GetDesiredInspectDistance() const;
	float GetMinInspectDistance() const { return MinInspectDistance; }
	float GetMaxInspectDistance() const { return MaxInspectDistance; }
	float GetZoomSpeed() const { return ZoomSpeed; }
	float GetRotationSpeed() const { return RotationSpeed; }
	float GetEnterDuration() const { return EnterDuration; }
	float GetExitDuration() const { return ExitDuration; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Inspection")
	bool bCanInspect = false;
	
	UPROPERTY(EditAnywhere, Category="MD|Inspection|Distance")
	bool bUseAutomaticDistance = true;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Distance", meta=(EditCondition="!bUseAutomaticDistance"))
	float InspectDistance = 100.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Distance", meta=(EditCondition="bUseAutomaticDistance", ClampMin="0.1"))
	float AutomaticDistanceMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Distance", meta=(EditCondition="bUseAutomaticDistance", ClampMin="0.0"))
	float AutomaticDistancePadding = 35.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection")
	float RotationSpeed = 0.3f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float MinInspectDistance = 50.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float MaxInspectDistance = 180.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float ZoomSpeed = 10.f;
	
	UPROPERTY(EditAnywhere, Category="MD|Inspection|Transition", meta=(ClampMin="0.0"))
	float EnterDuration = 0.3f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Transition", meta=(ClampMin="0.0"))
	float ExitDuration = 0.25f;

	UPROPERTY()
	FTransform OriginalTransform;

	UPROPERTY()
	TArray<UPrimitiveComponent*> SimulatingPrimitiveComponents;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio|Inspection")
	USoundBase* InspectSound = nullptr;

	bool bIsInspecting = false;
	bool bOriginalActorCollisionEnabled = true;

	FVector GetInspectableBoundsCenter() const;
	void DisableOwnerPhysics();
	void RestoreOwnerPhysics();
	float GetInspectableBoundsRadius() const;
};