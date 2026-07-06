#include "Video/MD_VideoManager.h"

#include "MediaPlayer.h"
#include "MediaSource.h"
#include "AudioDevice.h"
#include "AudioDeviceHandle.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

AMD_VideoManager::AMD_VideoManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AMD_VideoManager::PlayVideo(
	UMediaSource* Source,
	FName VideoId,
	bool bCanSkip,
	bool bRestoreControl,
	bool bStopExistingSounds)
{
	if (bPlaying || !Source || !MediaPlayer || !VideoWidgetClass)
		return false;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
		return false;

	ActiveWidget = CreateWidget<UUserWidget>(PC, VideoWidgetClass);
	if (!ActiveWidget)
		return false;

	if (bStopExistingSounds)
	{
		FAudioDeviceHandle AudioDevice = GetWorld()->GetAudioDevice();

		if (AudioDevice.IsValid())
		{
			AudioDevice->StopAllSounds(true);
		}
	}
	
	bPlaying = true;
	bCanCurrentlySkip = bCanSkip;
	bRestoreControlAfterwards = bRestoreControl;
	ActiveVideoId = VideoId;
	ActivePlayerController = PC;

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
		Character->GetCharacterMovement()->StopMovementImmediately();

	ActiveWidget->AddToViewport(1000);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	ActiveWidget->SetKeyboardFocus();

	MediaPlayer->OnEndReached.AddDynamic(
		this, &AMD_VideoManager::HandleEndReached);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(
		this, &AMD_VideoManager::HandleOpenFailed);

	MediaPlayer->PlayOnOpen = true;
	MediaPlayer->SetLooping(false);

	if (!MediaPlayer->OpenSource(Source))
	{
		FinishVideo();
		return false;
	}

	return true;
}

void AMD_VideoManager::SkipVideo()
{
	if (bPlaying && bCanCurrentlySkip)
		FinishVideo();
}

void AMD_VideoManager::HandleEndReached()
{
	FinishVideo();
}

void AMD_VideoManager::HandleOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("Failed to open video: %s"), *FailedUrl);
	FinishVideo();
}

void AMD_VideoManager::FinishVideo()
{
	if (!bPlaying)
		return;

	const FName FinishedVideoId = ActiveVideoId;
	bPlaying = false;

	MediaPlayer->OnEndReached.RemoveDynamic(
		this, &AMD_VideoManager::HandleEndReached);
	MediaPlayer->OnMediaOpenFailed.RemoveDynamic(
		this, &AMD_VideoManager::HandleOpenFailed);
	MediaPlayer->Close();

	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}

	if (APlayerController* PC = ActivePlayerController.Get();
		PC && bRestoreControlAfterwards)
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	OnVideoFinished.Broadcast(FinishedVideoId);
}