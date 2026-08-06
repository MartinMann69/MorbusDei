#pragma once

#include "CoreMinimal.h"
#include "Interaction/MD_Interactable.h"
#include "MD_MirrorActor.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;

UCLASS()
class MORBUSDEI_API AMD_MirrorActor : public AMD_Interactable
{
	GENERATED_BODY()

public:
	AMD_MirrorActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	TObjectPtr<UStaticMeshComponent> MirrorPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	bool bMirrorLocalX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	bool bMirrorLocalY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	bool bMirrorLocalZ = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Mirror")
	TArray<TSoftObjectPtr<AActor>> EnvToIgnore;
};