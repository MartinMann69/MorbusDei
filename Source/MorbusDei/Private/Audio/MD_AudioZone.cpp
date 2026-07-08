// Fill out your copyright notice in the Description page of Project Settings.


#include "Audio/MD_AudioZone.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "GameFramework/Pawn.h"

AMD_AudioZone::AMD_AudioZone()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetHiddenInGame(true);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(Root);
	AudioComponent->bAutoActivate = false;
}

void AMD_AudioZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateTriggerState();
	AudioComponent->SetAttenuationSettings(AttenuationSettings);
}

void AMD_AudioZone::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateTriggerState();

	if (UsesTrigger())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMD_AudioZone::HandleBeginOverlap);

		Trigger->OnComponentEndOverlap.AddDynamic(this, &AMD_AudioZone::HandleEndOverlap);
	}

	AudioComponent->OnAudioFinished.AddDynamic(this, &AMD_AudioZone::HandleAudioFinished);

	ConfigureAudioComponent();

	if (bPlayOnBeginPlay)
	{
		PlayZoneSound();
	}
}

void AMD_AudioZone::PlayZoneSound()
{
	if (!SoundToPlay || AudioComponent->IsPlaying())
	{
		return;
	}
	
	if (AudioComponent->IsPlaying())
	{
		return;
	}
	
	bStopRequested = false;
	bPlaybackLimitReached = false;
	
	ConfigureAudioComponent();

	if (!LoopParameterName.IsNone())
	{
		AudioComponent->SetBoolParameter(LoopParameterName, bLoop);
	}

	if (FadeInDuration > 0.0f)
	{
		AudioComponent->FadeIn(FadeInDuration, 1.0f);
	}
	else
	{
		AudioComponent->Play();
	}
	
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	if (MaxPlaybackDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			PlaybackLimitTimer,
			this,
			&AMD_AudioZone::HandlePlaybackLimitReached,
			MaxPlaybackDuration,
			false
		);
	}
}

void AMD_AudioZone::StopZoneSound()
{
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);
	
	if (!AudioComponent->IsPlaying())
	{
		return;
	}

	bStopRequested = true;

	if (FadeOutDuration > 0.0f)
	{
		AudioComponent->FadeOut(FadeOutDuration, 0.0f);
	}
	else
	{
		AudioComponent->Stop();
	}
}

void AMD_AudioZone::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bPlayWhenPlayerEnters && IsPlayerActor(OtherActor))
	{
		PlayZoneSound();
	}
}

void AMD_AudioZone::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (bStopWhenPlayerLeaves && IsPlayerActor(OtherActor))
	{
		StopZoneSound();
	}
}

void AMD_AudioZone::HandleAudioFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("Destroying audio component"));
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	const bool bFinishedNaturally = !bStopRequested;
	const bool bShouldDestroy = bDestroyAfterPlayback && !bLoop && (bFinishedNaturally || bPlaybackLimitReached);

	bStopRequested = false;
	bPlaybackLimitReached = false;

	if (bShouldDestroy)
	{
		Destroy();
	}
}

void AMD_AudioZone::ConfigureAudioComponent()
{
	AudioComponent->SetSound(SoundToPlay);
	AudioComponent->SetAttenuationSettings(AttenuationSettings);
	AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
	AudioComponent->SetPitchMultiplier(PitchMultiplier);
}

bool AMD_AudioZone::IsPlayerActor(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);

	return Pawn && Pawn->IsPlayerControlled();
}

void AMD_AudioZone::HandlePlaybackLimitReached()
{
	bPlaybackLimitReached = true;
	StopZoneSound();
}

bool AMD_AudioZone::UsesTrigger() const
{
	return bPlayWhenPlayerEnters || bStopWhenPlayerLeaves;
}

void AMD_AudioZone::UpdateTriggerState()
{
	const bool bUsesTrigger = UsesTrigger();

	Trigger->SetBoxExtent(TriggerExtent);
	Trigger->SetVisibility(bUsesTrigger);
	Trigger->SetHiddenInGame(true);
	Trigger->SetGenerateOverlapEvents(bUsesTrigger);
	Trigger->SetCollisionEnabled(bUsesTrigger ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool AMD_AudioZone::IsZoneSoundPlaying() const
{
	return AudioComponent && AudioComponent->IsPlaying();
}