#include "UI/MD_ActionHintWidget.h"

#include "Components/TextBlock.h"
#include "Input/MD_InputDeviceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UMD_ActionHintWidget::UMD_ActionHintWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	KeyboardKeyText = NSLOCTEXT("MDActionHint", "DefaultKeyboardKey", "Esc");
	GamepadKeyText = NSLOCTEXT("MDActionHint", "DefaultGamepadKey", "B");
	ActionText = NSLOCTEXT("MDActionHint", "DefaultBackAction", "Zurück");

	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/MorbusDei/Audio/MetaSounds/SFX/MSS_MenuButtonClick.MSS_MenuButtonClick"));
	if (SoundFinder.Succeeded())
	{
		HandledSound = SoundFinder.Object;
	}
}

void UMD_ActionHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr)
	{
		InputDeviceSubsystem->OnInputDeviceChanged.AddUniqueDynamic(
			this,
			&UMD_ActionHintWidget::HandleInputDeviceChanged);
	}
	RefreshPresentation();
}

void UMD_ActionHintWidget::NativeDestruct()
{
	if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr)
	{
		InputDeviceSubsystem->OnInputDeviceChanged.RemoveDynamic(
			this,
			&UMD_ActionHintWidget::HandleInputDeviceChanged);
	}
	StopPulse(true);
	Super::NativeDestruct();
}

void UMD_ActionHintWidget::ConfigureHint(
	const FText& InKeyboardKeyText,
	const FText& InGamepadKeyText,
	const FText& InActionText)
{
	KeyboardKeyText = InKeyboardKeyText;
	GamepadKeyText = InGamepadKeyText;
	ActionText = InActionText;
	RefreshPresentation();
}

void UMD_ActionHintWidget::SetActionText(const FText& InActionText)
{
	ActionText = InActionText;
	RefreshPresentation();
}

void UMD_ActionHintWidget::PlayHandledFeedback()
{
	if (HandledSound)
	{
		UGameplayStatics::PlaySound2D(this, HandledSound, HandledSoundVolume, HandledSoundPitch);
	}

	StopPulse(false);
	PulseElapsed = 0.0f;
	SetRenderOpacity(PulseStartOpacity);
	if (PulseDuration <= UE_SMALL_NUMBER)
	{
		SetRenderOpacity(1.0f);
		return;
	}

	PulseTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UMD_ActionHintWidget::TickPulse));
}

bool UMD_ActionHintWidget::IsUsingGamepad() const
{
	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr;
	return InputDeviceSubsystem && InputDeviceSubsystem->IsUsingGamepad();
}

void UMD_ActionHintWidget::HandleInputDeviceChanged(
	const EMDInputDeviceType PreviousDevice,
	const EMDInputDeviceType NewDevice)
{
	RefreshPresentation();
}

void UMD_ActionHintWidget::RefreshPresentation()
{
	const bool bUsingGamepad = IsUsingGamepad();
	if (BackKeyText)
	{
		BackKeyText->SetText(bUsingGamepad ? GamepadKeyText : KeyboardKeyText);
	}
	if (BackActionText)
	{
		BackActionText->SetText(ActionText);
	}
	OnPresentationChanged.Broadcast(bUsingGamepad);
}

bool UMD_ActionHintWidget::TickPulse(const float DeltaTime)
{
	PulseElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(PulseElapsed / PulseDuration, 0.0f, 1.0f);
	SetRenderOpacity(FMath::Lerp(PulseStartOpacity, 1.0f, Alpha));
	if (Alpha < 1.0f)
	{
		return true;
	}

	PulseTickerHandle.Reset();
	return false;
}

void UMD_ActionHintWidget::StopPulse(const bool bRestoreOpacity)
{
	if (PulseTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PulseTickerHandle);
		PulseTickerHandle.Reset();
	}
	if (bRestoreOpacity)
	{
		SetRenderOpacity(1.0f);
	}
}
