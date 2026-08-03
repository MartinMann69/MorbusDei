#include "MD_MirrorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AMD_MirrorActor::AMD_MirrorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	MirrorPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MirrorPlane"));
	MirrorPlane->SetupAttachment(RootComponent);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
}

void AMD_MirrorActor::BeginPlay()
{
	Super::BeginPlay();

	if (SceneCapture)
	{
		SceneCapture->HiddenActors.Add(this);

		for (const TSoftObjectPtr<AActor>& SoftActor : EnvToIgnore)
		{
			if (AActor* ResolvedActor = SoftActor.LoadSynchronous())
			{
				SceneCapture->HiddenActors.Add(ResolvedActor);
			}
		}

		SceneCapture->bEnableClipPlane = true;
	}
}

void AMD_MirrorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();

		if (MirrorPlane && SceneCapture)
		{
			FTransform MirrorTransform = FTransform(MirrorPlane->GetComponentRotation(), MirrorPlane->GetComponentLocation(), FVector(1.0f));
			FVector LocalCameraLocation = MirrorTransform.InverseTransformPosition(CameraLocation);

			if (bMirrorLocalZ)
			{
				LocalCameraLocation.X *= 0.3f;
				LocalCameraLocation.Y *= 0.1f;
				LocalCameraLocation.Z *= -0.5f;
			}
			else if (bMirrorLocalY)
			{
				LocalCameraLocation.X *= 0.3f;
				LocalCameraLocation.Z *= 0.1f;
				LocalCameraLocation.Y *= -0.5f;
			}
			else if (bMirrorLocalX)
			{
				LocalCameraLocation.Y *= 0.3f;
				LocalCameraLocation.Z *= 0.1f;
				LocalCameraLocation.X *= -0.5f;
			}

			FVector MirroredWorldLocation = MirrorTransform.TransformPosition(LocalCameraLocation);
			SceneCapture->SetWorldLocation(MirroredWorldLocation);

			FVector MirrorNormal = FVector::ZeroVector;
			if (bMirrorLocalX) MirrorNormal = MirrorTransform.GetUnitAxis(EAxis::X);
			else if (bMirrorLocalY) MirrorNormal = MirrorTransform.GetUnitAxis(EAxis::Y);
			else if (bMirrorLocalZ) MirrorNormal = MirrorTransform.GetUnitAxis(EAxis::Z);

			if (!MirrorNormal.IsNearlyZero())
			{
				SceneCapture->SetWorldRotation(MirrorNormal.Rotation());
			}

			FVector MirrorWorldLocation = MirrorPlane->GetComponentLocation();
			SceneCapture->ClipPlaneBase = MirrorWorldLocation;
			SceneCapture->ClipPlaneNormal = MirrorNormal;
		}
	}
}