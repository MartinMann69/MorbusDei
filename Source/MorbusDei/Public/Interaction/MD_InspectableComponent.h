#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_InspectableComponent.generated.h"

class APawn;
class USceneComponent;
class UPrimitiveComponent;

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
	float GetMinInspectDistance() const { return MinInspectDistance; }
	float GetMaxInspectDistance() const { return MaxInspectDistance; }
	float GetZoomSpeed() const { return ZoomSpeed; }
	float GetRotationSpeed() const { return RotationSpeed; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Inspection")
	bool bCanInspect = false;

	UPROPERTY(EditAnywhere, Category="MD|Inspection")
	float InspectDistance = 100.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection")
	float RotationSpeed = 2.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float MinInspectDistance = 50.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float MaxInspectDistance = 180.f;

	UPROPERTY(EditAnywhere, Category="MD|Inspection|Zoom")
	float ZoomSpeed = 10.f;

	UPROPERTY()
	FTransform OriginalTransform;

	UPROPERTY()
	TArray<UPrimitiveComponent*> SimulatingPrimitiveComponents;

	bool bIsInspecting = false;
	bool bOriginalActorCollisionEnabled = true;

	FVector GetInspectableBoundsCenter() const;
	void DisableOwnerPhysics();
	void RestoreOwnerPhysics();
};