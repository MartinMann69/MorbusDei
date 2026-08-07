#include "UI/MD_PauseMenuWidget.h"

#include "Components/TextBlock.h"
#include "Containers/Ticker.h"
#include "Input/MD_InputDeviceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MD_PlayerController.h"
#include "Sound/SoundBase.h"
#include "UI/MD_ActionHintWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/WidgetAnimation.h"

UMD_PauseMenuWidget::UMD_PauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	KeyboardBackKeyText = NSLOCTEXT("MDPauseMenu", "KeyboardBackKey", "Esc");
	GamepadBackKeyText = NSLOCTEXT("MDPauseMenu", "GamepadBackKey", "B");
	BackLabelText = NSLOCTEXT("MDPauseMenu", "BackLabel", "Zurück");
	ResumeLabelText = NSLOCTEXT("MDPauseMenu", "ResumeLabel", "Fortsetzen");

	static ConstructorHelpers::FObjectFinder<USoundBase> BackSoundFinder(
		TEXT("/Game/MorbusDei/Audio/MetaSounds/SFX/MSS_MenuButtonClick.MSS_MenuButtonClick"));
	if (BackSoundFinder.Succeeded())
	{
		BackSound = BackSoundFinder.Object;
	}
}

void UMD_PauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AMD_PlayerController* PlayerController = Cast<AMD_PlayerController>(GetOwningPlayer()))
	{
		PlayerController->RegisterPauseMenuWidget(this);
	}

	bCloseRequested = false;
	BaseRenderTranslation = GetRenderTransform().Translation;
	InputUnlockTimeSeconds = FPlatformTime::Seconds() + static_cast<double>(OpenTransitionDuration);

	OnFocusZoneChanged.AddUniqueDynamic(this, &UMD_PauseMenuWidget::HandlePauseFocusZoneChanged);
	OnBackActionHandled.AddUniqueDynamic(this, &UMD_PauseMenuWidget::HandleBackActionFeedback);
	if (!ActionHint)
	{
		if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
			: nullptr)
		{
			InputDeviceSubsystem->OnInputDeviceChanged.AddUniqueDynamic(
				this,
				&UMD_PauseMenuWidget::HandleInputDeviceChanged);
		}
	}

	UpdateBackHint();

	if (PauseTransition)
	{
		FWidgetAnimationDynamicEvent TransitionFinishedEvent;
		TransitionFinishedEvent.BindDynamic(this, &UMD_PauseMenuWidget::HandlePauseTransitionFinished);
		BindToAnimationFinished(PauseTransition, TransitionFinishedEvent);
		PlayAnimation(PauseTransition, 0.0f, 1, EUMGSequencePlayMode::Forward);
	}
	else
	{
		StartNativeTransition(false);
	}
}

void UMD_PauseMenuWidget::NativeDestruct()
{
	if (AMD_PlayerController* PlayerController = Cast<AMD_PlayerController>(GetOwningPlayer()))
	{
		PlayerController->UnregisterPauseMenuWidget(this);
	}

	StopNativeTransition();
	StopHintPulse();
	if (PauseTransition)
	{
		UnbindAllFromAnimationFinished(PauseTransition);
	}

	OnFocusZoneChanged.RemoveDynamic(this, &UMD_PauseMenuWidget::HandlePauseFocusZoneChanged);
	OnBackActionHandled.RemoveDynamic(this, &UMD_PauseMenuWidget::HandleBackActionFeedback);
	if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr)
	{
		InputDeviceSubsystem->OnInputDeviceChanged.RemoveDynamic(
			this,
			&UMD_PauseMenuWidget::HandleInputDeviceChanged);
	}

	CloseTransitionFinished.Clear();
	Super::NativeDestruct();
}

FReply UMD_PauseMenuWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (bCloseRequested || IsOpeningInputGuardActive())
	{
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right)
	{
		NotifyGamepadInput();
		const EGameUIFocusZone SourceZone = GetCurrentFocusZone();
		if (SourceZone != EGameUIFocusZone::Modal)
		{
			if (AMD_PlayerController* PlayerController = Cast<AMD_PlayerController>(GetOwningPlayer());
				PlayerController && PlayerController->RequestClosePauseMenu())
			{
				BroadcastBackActionHandled(SourceZone);
			}
		}

		// Start never bypasses a confirmation modal or reaches gameplay while UI-only input is active.
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMD_PauseMenuWidget::NativeOnAnalogValueChanged(
	const FGeometry& InGeometry,
	const FAnalogInputEvent& InAnalogEvent)
{
	return bCloseRequested || IsOpeningInputGuardActive()
		? FReply::Handled()
		: Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UMD_PauseMenuWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	return bCloseRequested || IsOpeningInputGuardActive()
		? FReply::Handled()
		: Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

bool UMD_PauseMenuWidget::HandleRootBackAction_Implementation()
{
	if (AMD_PlayerController* PlayerController = Cast<AMD_PlayerController>(GetOwningPlayer()))
	{
		return PlayerController->RequestClosePauseMenu();
	}

	return false;
}

bool UMD_PauseMenuWidget::BeginCloseTransition()
{
	if (bCloseRequested)
	{
		return false;
	}

	bCloseRequested = true;
	DeactivateFocusScreen();

	if (PauseTransition)
	{
		StopAnimation(PauseTransition);
		const float ClosePlayRate = CloseTransitionDuration > UE_SMALL_NUMBER
			? FMath::Max(OpenTransitionDuration / CloseTransitionDuration, 0.01f)
			: 1.0f;
		PlayAnimation(PauseTransition, 0.0f, 1, EUMGSequencePlayMode::Reverse, ClosePlayRate);
	}
	else if (CloseTransitionDuration > UE_SMALL_NUMBER)
	{
		StartNativeTransition(true);
	}
	else
	{
		FinishCloseTransition();
	}

	return true;
}

void UMD_PauseMenuWidget::HandlePauseTransitionFinished()
{
	if (bCloseRequested)
	{
		FinishCloseTransition();
	}
}

void UMD_PauseMenuWidget::HandleInputDeviceChanged(
	const EMDInputDeviceType PreviousDevice,
	const EMDInputDeviceType NewDevice)
{
	UpdateBackHint();
}

void UMD_PauseMenuWidget::HandlePauseFocusZoneChanged(
	const EGameUIFocusZone PreviousZone,
	const EGameUIFocusZone NewZone)
{
	UpdateBackHint();
}

void UMD_PauseMenuWidget::HandleBackActionFeedback(const EGameUIFocusZone SourceZone)
{
	if (ActionHint)
	{
		ActionHint->PlayHandledFeedback();
		return;
	}

	if (BackSound)
	{
		UGameplayStatics::PlaySound2D(this, BackSound, BackSoundVolume, BackSoundPitch);
	}
	StartHintPulse();
}

void UMD_PauseMenuWidget::UpdateBackHint()
{
	const FText CurrentActionText = GetCurrentFocusZone() == EGameUIFocusZone::Navigation
		? ResumeLabelText
		: BackLabelText;
	if (ActionHint)
	{
		ActionHint->ConfigureHint(KeyboardBackKeyText, GamepadBackKeyText, CurrentActionText);
		return;
	}

	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr;
	const bool bUsingGamepad = InputDeviceSubsystem && InputDeviceSubsystem->IsUsingGamepad();

	if (BackKeyText)
	{
		BackKeyText->SetText(bUsingGamepad ? GamepadBackKeyText : KeyboardBackKeyText);
	}
	if (BackActionText)
	{
		BackActionText->SetText(CurrentActionText);
	}
}

void UMD_PauseMenuWidget::StartNativeTransition(const bool bClosing)
{
	StopNativeTransition();
	bNativeTransitionClosing = bClosing;
	TransitionElapsed = 0.0f;
	TransitionStartOpacity = bClosing ? GetRenderOpacity() : 0.0f;
	TransitionStartTranslation = bClosing
		? GetRenderTransform().Translation
		: BaseRenderTranslation - FVector2D(TransitionOffset, 0.0f);
	SetRenderOpacity(TransitionStartOpacity);
	SetRenderTranslation(TransitionStartTranslation);

	NativeTransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UMD_PauseMenuWidget::TickNativeTransition));
}

bool UMD_PauseMenuWidget::TickNativeTransition(const float DeltaTime)
{
	TransitionElapsed += DeltaTime;
	const float Duration = bNativeTransitionClosing ? CloseTransitionDuration : OpenTransitionDuration;
	const float Alpha = Duration > UE_SMALL_NUMBER
		? FMath::Clamp(TransitionElapsed / Duration, 0.0f, 1.0f)
		: 1.0f;
	const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 3.0f);
	const float TargetOpacity = bNativeTransitionClosing ? 0.0f : 1.0f;
	const FVector2D TargetTranslation = bNativeTransitionClosing
		? BaseRenderTranslation - FVector2D(TransitionOffset, 0.0f)
		: BaseRenderTranslation;

	SetRenderOpacity(FMath::Lerp(TransitionStartOpacity, TargetOpacity, EasedAlpha));
	SetRenderTranslation(FMath::Lerp(TransitionStartTranslation, TargetTranslation, EasedAlpha));

	if (Alpha < 1.0f)
	{
		return true;
	}

	NativeTransitionTickerHandle.Reset();
	if (bNativeTransitionClosing)
	{
		FinishCloseTransition();
	}
	return false;
}

void UMD_PauseMenuWidget::StopNativeTransition()
{
	if (NativeTransitionTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(NativeTransitionTickerHandle);
		NativeTransitionTickerHandle.Reset();
	}
}

void UMD_PauseMenuWidget::StartHintPulse()
{
	if (!BackKeyText && !BackActionText)
	{
		return;
	}

	StopHintPulse();
	HintPulseElapsed = 0.0f;
	if (BackKeyText)
	{
		BackKeyText->SetRenderOpacity(0.55f);
	}
	if (BackActionText)
	{
		BackActionText->SetRenderOpacity(0.55f);
	}

	HintPulseTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UMD_PauseMenuWidget::TickHintPulse));
}

bool UMD_PauseMenuWidget::TickHintPulse(const float DeltaTime)
{
	constexpr float HintPulseDuration = 0.08f;
	HintPulseElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(HintPulseElapsed / HintPulseDuration, 0.0f, 1.0f);
	const float Opacity = FMath::Lerp(0.55f, 1.0f, Alpha);
	if (BackKeyText)
	{
		BackKeyText->SetRenderOpacity(Opacity);
	}
	if (BackActionText)
	{
		BackActionText->SetRenderOpacity(Opacity);
	}

	if (Alpha < 1.0f)
	{
		return true;
	}

	HintPulseTickerHandle.Reset();
	return false;
}

void UMD_PauseMenuWidget::StopHintPulse()
{
	if (HintPulseTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HintPulseTickerHandle);
		HintPulseTickerHandle.Reset();
	}
}

void UMD_PauseMenuWidget::FinishCloseTransition()
{
	StopNativeTransition();
	CloseTransitionFinished.Broadcast(this);
}

bool UMD_PauseMenuWidget::IsOpeningInputGuardActive() const
{
	return FPlatformTime::Seconds() < InputUnlockTimeSeconds;
}
