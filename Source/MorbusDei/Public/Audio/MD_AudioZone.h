// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#include "MD_AudioZone.generated.h"

class UAudioComponent;
class UBoxComponent;
class USceneComponent;
class USoundAttenuation;
class USoundBase;
class UPrimitiveComponent;

UCLASS()
class MORBUSDEI_API AMD_AudioZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AMD_AudioZone();
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="MD|Audio")
	void PlayZoneSound();

	UFUNCTION(BlueprintCallable, Category="MD|Audio")
	void StopZoneSound();
	
	UFUNCTION(BlueprintPure, Category="MD|Audio")
	bool IsZoneSoundPlaying() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Audio|Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Audio|Components")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MD|Audio|Components")
	UAudioComponent* AudioComponent;

	// Accepts MetaSound Sources, Sound Waves and Sound Cues.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	USoundBase* SoundToPlay = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	USoundAttenuation* AttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	FVector TriggerExtent = FVector(200.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	bool bPlayOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	bool bPlayWhenPlayerEnters = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	bool bStopWhenPlayerLeaves = true;

	// Requires a matching Boolean input in the MetaSound. Named "Loop"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(EditCondition="bLoop"))
	FName LoopParameterName = TEXT("Loop");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(EditCondition="!bLoop"))
	bool bDestroyAfterPlayback = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(ClampMin="0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(ClampMin="0.01"))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(ClampMin="0.0"))
	float FadeInDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(ClampMin="0.0"))
	float FadeOutDuration = 0.5f;
	
	// Zero means no playback limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio", meta=(ClampMin="0.0"))
	float MaxPlaybackDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MD|Audio|Voice")
	bool bStopOtherVoiceLinesOnPlay = true;

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleAudioFinished();
	void HandlePlaybackLimitReached();
	void ConfigureAudioComponent();
	void UpdateTriggerState();
	void StopCompetingVoiceLine();
	void StopVoiceLineImmediately();
	void ClearActiveVoiceLineIfNeeded();
	
	bool IsPlayerActor(const AActor* Actor) const;
	bool UsesTrigger() const;

	bool bStopRequested = false;
	bool bPlaybackLimitReached = false;
	
	FTimerHandle PlaybackLimitTimer;

	static TWeakObjectPtr<AMD_AudioZone> ActiveVoiceLineZone;

};
