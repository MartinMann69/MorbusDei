// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MD_Interactable.h"
#include "MD_Inspectable.generated.h"

UCLASS()
class MORBUSDEI_API AMD_Inspectable : public AMD_Interactable
{
	GENERATED_BODY()
	
public:	
	virtual void Interact_Implementation(APawn* Interactor) override;

	bool StartInspect(APawn* Interactor, USceneComponent* InspectPivot);
	void EndInspect();
	
	bool IsInspecting() const;
	float GetInspectDistance() const { return InspectDistance; }
	float GetRotationSpeed() const { return RotationSpeed; }


protected:
	UPROPERTY(EditAnywhere, Category="Inspection")
	float InspectDistance = 100.f;

	UPROPERTY(EditAnywhere, Category="Inspection")
	float RotationSpeed = 2.f;

	UPROPERTY()
	FTransform OriginalTransform;

	bool bIsInspecting = false;
	bool bOriginalCanInteract = true;
	bool bOriginalActorCollisionEnabled = true;
	bool bOriginalSimulatePhysics = false;

	FVector GetInspectableBoundsCenter() const;
};
