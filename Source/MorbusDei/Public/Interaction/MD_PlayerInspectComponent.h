// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_PlayerInspectComponent.generated.h"

class APawn;
class AMD_Inspectable;
class USceneComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_PlayerInspectComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UMD_PlayerInspectComponent();

	virtual void BeginPlay() override;

	bool StartInspect(AMD_Inspectable* Item);
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
	AMD_Inspectable* CurrentInspectedItem = nullptr;

	UPROPERTY()
	USceneComponent* InspectPivot = nullptr;

	void EnsureInspectPivot();
	void UpdateInspectPivotLocation();
};
