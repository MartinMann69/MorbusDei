#include "Haptics/MD_HapticTriggerVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Haptics/MD_HapticSubsystem.h"

AMD_HapticTriggerVolume::AMD_HapticTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->InitBoxExtent(FVector(100.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetVisibility(true);
	Trigger->SetHiddenInGame(true);
}

void AMD_HapticTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&AMD_HapticTriggerVolume::HandleBeginOverlap);
}

void AMD_HapticTriggerVolume::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	const double CurrentRealTime = GetWorld()
		? GetWorld()->GetRealTimeSeconds()
		: FPlatformTime::Seconds();
	if (!bTriggerOnce && CurrentRealTime - LastSuccessfulTriggerRealTime < RetriggerCooldown)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
		: nullptr;
	if (!HapticSubsystem || !HapticSubsystem->PlayHapticEvent(Event))
	{
		return;
	}

	LastSuccessfulTriggerRealTime = CurrentRealTime;
	if (bTriggerOnce)
	{
		Trigger->SetGenerateOverlapEvents(false);
		Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
