#include "Tools/MD_MenuPreviewRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/RectLightComponent.h"

AMD_MenuPreviewRig::AMD_MenuPreviewRig()
{
	PrimaryActorTick.bCanEverTick = false;
	bFindCameraComponentWhenViewTarget = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PreviewLight = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewLight"));
	PreviewLight->SetupAttachment(RootComponent);
	
	PreviewPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewPivot"));
	PreviewPivot->SetupAttachment(RootComponent);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(PreviewPivot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 700.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(SpringArm);
	PreviewCamera->SetAutoActivate(true);
	PreviewCamera->SetRelativeLocation(FVector(0.0f, 65.0f, 0.0f));
	PreviewCamera->FieldOfView = 35.0f;

	RectLight = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight"));
	RectLight->SetupAttachment(PreviewLight);
	RectLight->SetRelativeLocation(FVector(-200.0f, -200.0f, 0.0f));
	RectLight->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
	RectLight->SetIntensity(5000.0f);
	RectLight->SetSourceWidth(550.0f);
	RectLight->SetSourceHeight(550.0f);

	RectLight1 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight1"));
	RectLight1->SetupAttachment(PreviewLight);
	RectLight1->SetRelativeLocation(FVector(-200.0f, 200.0f, 0.0f));
	RectLight1->SetRelativeRotation(FRotator(0.0f, -45.0f, 0.0f));
	RectLight1->SetIntensity(5000.0f);
	RectLight1->SetSourceWidth(550.0f);
	RectLight1->SetSourceHeight(550.0f);

	RectLight2 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight2"));
	RectLight2->SetupAttachment(PreviewLight);
	RectLight2->SetRelativeLocation(FVector(-200.0f, 0.0f, 200.0f));
	RectLight2->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	RectLight2->SetIntensity(5000.0f);
	RectLight2->SetSourceWidth(550.0f);
	RectLight2->SetSourceHeight(550.0f);

	RectLight3 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight3"));
	RectLight3->SetupAttachment(PreviewLight);
	RectLight3->SetRelativeLocation(FVector(-200.0f, 0.0f, -200.0f));
	RectLight3->SetRelativeRotation(FRotator(45.0f, 0.0f, 0.0f));
	RectLight3->SetIntensity(5000.0f);
	RectLight3->SetSourceWidth(550.0f);
	RectLight3->SetSourceHeight(550.0f);
}

void AMD_MenuPreviewRig::ShowPreview(TSubclassOf<AActor> PreviewClass)
{
	if (!PreviewClass || !GetWorld())
	{
		return;
	}

	ClearPreview();

	PreviewYaw = 0.0f;
	PreviewPitch = 0.0f;
	PreviewPivot->SetRelativeRotation(FRotator::ZeroRotator);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform(
	PreviewSpawnRotation,
	SpawnPoint->GetComponentLocation(),
	SpawnPoint->GetComponentScale()
	);

	CurrentPreviewActor = GetWorld()->SpawnActor<AActor>(
		PreviewClass,
		SpawnTransform,
		SpawnParams
	);

	if (!CurrentPreviewActor)
	{
		return;
	}

	CurrentPreviewActor->SetActorEnableCollision(false);

	FVector MeshCenter;
	FVector MeshExtent;
	if (GetStaticMeshBounds(CurrentPreviewActor, MeshCenter, MeshExtent))
	{
		const FVector Offset = SpawnPoint->GetComponentLocation() - MeshCenter;
		CurrentPreviewActor->SetActorLocation(
			CurrentPreviewActor->GetActorLocation() + Offset,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
	}

	CurrentPreviewActor->AttachToComponent(
		PreviewPivot,
		FAttachmentTransformRules::KeepWorldTransform
	);
}

void AMD_MenuPreviewRig::ClearPreview()
{
	if (IsValid(CurrentPreviewActor))
	{
		CurrentPreviewActor->Destroy();
	}

	CurrentPreviewActor = nullptr;
}

void AMD_MenuPreviewRig::RotatePreview(float DeltaX, float DeltaY)
{
	PreviewYaw += DeltaX * RotationSpeed;
	PreviewPitch = FMath::Clamp(PreviewPitch + DeltaY * RotationSpeed, -45.0f, 45.0f);

	PreviewPivot->SetRelativeRotation(FRotator(PreviewPitch, PreviewYaw, 0.0f));
}

void AMD_MenuPreviewRig::ZoomPreview(float WheelDelta)
{
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - WheelDelta * ZoomSpeed,
		MinZoom,
		MaxZoom
	);
}

bool AMD_MenuPreviewRig::GetStaticMeshBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent) const
{
	if (!Actor)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

	FBox CombinedBox(ForceInit);

	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || !MeshComponent->GetStaticMesh())
		{
			continue;
		}

		CombinedBox += MeshComponent->Bounds.GetBox();
	}

	if (!CombinedBox.IsValid)
	{
		return false;
	}

	OutCenter = CombinedBox.GetCenter();
	OutExtent = CombinedBox.GetExtent();
	return true;
}