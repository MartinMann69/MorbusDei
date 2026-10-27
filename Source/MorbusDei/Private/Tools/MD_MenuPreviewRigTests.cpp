#if WITH_DEV_AUTOMATION_TESTS

#include "Tools/MD_MenuPreviewRig.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMDGalleryPreviewTest, "Nautilus.Gallery.PreviewPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMDGalleryPreviewTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Preview test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();

	AMD_MenuPreviewRig* Rig = World->SpawnActor<AMD_MenuPreviewRig>(
		FVector(125.0f, -260.0f, 40.0f), FRotator(0.0f, 35.0f, 0.0f));
	Rig->SpawnPoint->SetRelativeLocation(FVector(60.0f, -45.0f, 85.0f));
	Rig->SpawnPoint->SetRelativeRotation(FRotator(0.0f, 25.0f, 0.0f));
	Rig->PreviewSpawnRotation = FRotator(0.0f, 90.0f, 0.0f);

	const TCHAR* PreviewPaths[] =
	{
		TEXT("/Game/MorbusDei/Blueprints/BP_Creature.BP_Creature_C"),
		TEXT("/Game/MorbusDei/Blueprints/World/GalleryBPs/BP_G_Statue.BP_G_Statue_C"),
		TEXT("/Game/MorbusDei/Blueprints/World/GalleryBPs/BP_G_Character.BP_G_Character_C")
	};
	for (const TCHAR* Path : PreviewPaths)
	{
		UClass* PreviewClass = LoadClass<AActor>(nullptr, Path);
		if (!TestNotNull(Path, PreviewClass))
		{
			continue;
		}
		const FRotator RotationOffset(0.0f, 40.0f, 0.0f);
		const FVector PivotOffset(10.0f, -5.0f, 15.0f);
		Rig->ShowPreviewWithPresentation(PreviewClass, RotationOffset, PivotOffset);
		TArray<AActor*> AttachedActors;
		Rig->GetAttachedActors(AttachedActors);
		if (!TestEqual(TEXT("Exactly one preview is attached"), AttachedActors.Num(), 1))
		{
			continue;
		}
		AActor* Preview = AttachedActors[0];
		const FTransform InitialTransform = Preview->GetActorTransform();
		const FQuat ExpectedRotation = Rig->SpawnPoint->GetComponentQuat()
			* Rig->PreviewSpawnRotation.Quaternion() * RotationOffset.Quaternion();
		TestTrue(TEXT("Start rotation follows the rig, spawn point and asset correction"),
			Preview->GetActorQuat().Equals(ExpectedRotation, 0.001f));

		FBox Bounds(ForceInit);
		TArray<UMeshComponent*> Meshes;
		Preview->GetComponents<UMeshComponent>(Meshes, true);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh->IsVisible() && !Mesh->bHiddenInGame && !Mesh->GetOwner()->IsHidden())
			{
				Bounds += Mesh->Bounds.GetBox();
			}
		}
		TestTrue(TEXT("Preview has visible mesh bounds"), Bounds.IsValid != 0);
		const FVector LocalCenter = InitialTransform.InverseTransformPosition(Bounds.GetCenter()) + PivotOffset;
		TestTrue(TEXT("Mesh center plus correction coincides with the rotation pivot"),
			InitialTransform.TransformPosition(LocalCenter).Equals(Rig->PreviewPivot->GetComponentLocation(), 0.1f));
		Rig->RotatePreview(410.0f, 170.0f);
		TestTrue(TEXT("Rotating an offset asset keeps the selected center stationary"),
			Preview->GetActorTransform().TransformPosition(LocalCenter).Equals(Rig->PreviewPivot->GetComponentLocation(), 0.1f));
		Rig->PanPreview(1.0f, 1.0f, 0.2f);
		Rig->ZoomPreview(2.0f);
		Rig->Tick(0.1f);
		Rig->ResetPreviewView();
		TestTrue(TEXT("Reset restores the authored initial view"), Preview->GetActorTransform().Equals(InitialTransform, 0.1f));

		if (APawn* Pawn = Cast<APawn>(Preview))
		{
			TestNull(TEXT("Gallery pawn has no gameplay controller"), Pawn->GetController());
			if (UMovementComponent* Movement = Pawn->GetMovementComponent())
			{
				TestFalse(TEXT("Gallery movement is inactive"), Movement->IsActive());
				TestFalse(TEXT("Gallery movement cannot tick and move the pivot"), Movement->IsComponentTickEnabled());
			}
		}
		for (int32 Frame = 0; Frame < 10; ++Frame)
		{
			World->Tick(LEVELTICK_All, 1.0f / 60.0f);
		}
		TestTrue(TEXT("Preview stays in place across gameplay ticks"), Preview->GetActorTransform().Equals(InitialTransform, 0.1f));
	}
	Rig->ClearPreview();
	TestFalse(TEXT("Closing clears the preview"), Rig->HasPreview());
	Rig->RotatePreview(100.0f, 100.0f);
	World->EndPlay(EEndPlayReason::Quit);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
