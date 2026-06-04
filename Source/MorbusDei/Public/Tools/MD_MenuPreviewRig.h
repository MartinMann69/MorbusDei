#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MD_MenuPreviewRig.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class URectLightComponent;

UCLASS()
class MORBUSDEI_API AMD_MenuPreviewRig : public AActor
{
	GENERATED_BODY()
	
public:
	AMD_MenuPreviewRig();

	UFUNCTION(BlueprintCallable, Category="MD|Menu Preview")
	void ShowPreview(TSubclassOf<AActor> PreviewClass);

	UFUNCTION(BlueprintCallable, Category="MD|Menu Preview")
	void ClearPreview();

	UFUNCTION(BlueprintCallable, Category="MD|Menu Preview")
	void RotatePreview(float DeltaX, float DeltaY);

	UFUNCTION(BlueprintCallable, Category="MD|Menu Preview")
	void ZoomPreview(float WheelDelta);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview")
	TObjectPtr<USceneComponent> PreviewPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview")
	TObjectPtr<USceneComponent> SpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview")
	TObjectPtr<UCameraComponent> PreviewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview")
	TObjectPtr<USceneComponent> PreviewLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Menu Preview")
	float RotationSpeed = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Menu Preview")
	float ZoomSpeed = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Menu Preview")
	float MinZoom = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Menu Preview")
	float MaxZoom = 800.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Menu Preview")
	FRotator PreviewSpawnRotation = FRotator::ZeroRotator;

private:
	UPROPERTY()
	TObjectPtr<AActor> CurrentPreviewActor;

	float PreviewYaw = 0.0f;
	float PreviewPitch = 0.0f;

	bool GetStaticMeshBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent) const;
};
