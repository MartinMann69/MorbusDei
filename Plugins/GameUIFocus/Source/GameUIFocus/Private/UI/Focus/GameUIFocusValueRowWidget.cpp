#include "UI/Focus/GameUIFocusValueRowWidget.h"

#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Internationalization/Text.h"
#include "UI/Focus/GameUIFocusInputKeys.h"
#include "UI/Focus/GameUIFocusPageWidgetBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameUIFocusValueRow, Log, All);

UGameUIFocusValueRowWidget::UGameUIFocusValueRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UGameUIFocusValueRowWidget::NativeDestruct()
{
	ResetHorizontalAnalogReleaseGate();
	Super::NativeDestruct();
}

void UGameUIFocusValueRowWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	ResetHorizontalAnalogInput();

	const UGameUIFocusPageWidgetBase* FocusPage = GetOwningFocusPage();
	HorizontalAnalogReleaseGate.Arm(
		FocusPage && FocusPage->IsHorizontalAnalogActuated(AnalogInputReleaseThreshold));

	if (HorizontalAnalogReleaseGate.IsAwaitingRelease())
	{
		UE_LOG(LogGameUIFocusValueRow, VeryVerbose,
			TEXT("Value row waiting for horizontal analog release. Row=%s Threshold=%.3f"),
			*GetNameSafe(this),
			AnalogInputReleaseThreshold);
	}
}

void UGameUIFocusValueRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	ResetHorizontalAnalogReleaseGate();
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
}

void UGameUIFocusValueRowWidget::InitializeValueRow(
	FGameplayTag InRowIdentifier,
	FText InLabel,
	const TArray<FText>& InOptions,
	int32 InCurrentIndex)
{
	InitializeValueRowWithControlType(
		InRowIdentifier,
		MoveTemp(InLabel),
		EGameUIFocusValueRowControlType::OptionList,
		InOptions,
		InCurrentIndex);
}

void UGameUIFocusValueRowWidget::InitializeValueRowWithControlType(
	FGameplayTag InRowIdentifier,
	FText InLabel,
	EGameUIFocusValueRowControlType InControlType,
	const TArray<FText>& InOptions,
	int32 InCurrentIndex,
	bool bInEnabled,
	FText InDisabledReason)
{
	RowIdentifier = InRowIdentifier;
	Label = MoveTemp(InLabel);
	Options = InOptions;
	ControlType = InControlType;
	bIsValueRowEnabled = bInEnabled;
	DisabledReason = MoveTemp(InDisabledReason);
	bHasNumericRange = false;
	CurrentIndex = FMath::Clamp(InCurrentIndex, 0, FMath::Max(Options.Num() - 1, 0));
	bIsOptionListExpanded = false;
	bUsesSelectionBeforeApply = ControlType == EGameUIFocusValueRowControlType::SelectionModal;
	bLastValueChangeWasUserInitiated = false;

	OnRowDataChanged();
	OnCurrentIndexChanged(CurrentIndex);
	OnOptionListExpandedChanged(bIsOptionListExpanded);
	RefreshBoundTextWidgets();
	InvalidateLayoutAndVolatility();
}

void UGameUIFocusValueRowWidget::InitializeNumericValueRow(
	FGameplayTag InRowIdentifier,
	FText InLabel,
	float MinValue,
	float MaxValue,
	float StepSize,
	float CurrentValue,
	int32 FractionalDigits,
	FText Suffix,
	bool bInEnabled,
	FText InDisabledReason)
{
	NumericMinValue = FMath::Min(MinValue, MaxValue);
	NumericMaxValue = FMath::Max(MinValue, MaxValue);
	NumericStepSize = FMath::Max(FMath::Abs(StepSize), UE_KINDA_SMALL_NUMBER);
	bHasNumericRange = true;

	BuildNumericOptions(FMath::Max(FractionalDigits, 0), MoveTemp(Suffix));

	RowIdentifier = InRowIdentifier;
	Label = MoveTemp(InLabel);
	ControlType = EGameUIFocusValueRowControlType::Slider;
	bIsValueRowEnabled = bInEnabled;
	DisabledReason = MoveTemp(InDisabledReason);
	CurrentIndex = GetOptionIndexForNumericValue(CurrentValue);
	bIsOptionListExpanded = false;
	bUsesSelectionBeforeApply = false;
	bLastValueChangeWasUserInitiated = false;
	bHasNumericRange = true;

	OnRowDataChanged();
	OnCurrentIndexChanged(CurrentIndex);
	OnOptionListExpandedChanged(bIsOptionListExpanded);
	RefreshBoundTextWidgets();
	InvalidateLayoutAndVolatility();
}

void UGameUIFocusValueRowWidget::SetCurrentIndex(int32 InCurrentIndex, bool bBroadcast)
{
	const int32 ClampedIndex = FMath::Clamp(InCurrentIndex, 0, FMath::Max(Options.Num() - 1, 0));
	if (CurrentIndex == ClampedIndex)
	{
		return;
	}

	CurrentIndex = ClampedIndex;
	bLastValueChangeWasUserInitiated = bBroadcast;
	OnCurrentIndexChanged(CurrentIndex);
	RefreshBoundTextWidgets();
	InvalidateLayoutAndVolatility();

	if (bBroadcast)
	{
		OnValueChanged.Broadcast(RowIdentifier, CurrentIndex);
		if (bHasNumericRange)
		{
			OnNumericValueChanged.Broadcast(RowIdentifier, CurrentIndex, GetCurrentNumericValue());
		}
	}
}

void UGameUIFocusValueRowWidget::SelectOptionIndex(int32 OptionIndex)
{
	if (!CanInteractWithRow())
	{
		RequestDisabledInteraction();
		return;
	}

	if (!Options.IsValidIndex(OptionIndex))
	{
		return;
	}

	SetCurrentIndex(OptionIndex, true);
	SetOptionListExpanded(false);
}

void UGameUIFocusValueRowWidget::RequestSelection()
{
	if (!CanInteractWithRow())
	{
		RequestDisabledInteraction();
		return;
	}

	if (!bUsesSelectionBeforeApply || Options.Num() <= 0)
	{
		return;
	}

	OnSelectionRequested.Broadcast(RowIdentifier);
	OnSelectionRequestedEvent();
}

void UGameUIFocusValueRowWidget::SelectNextOption()
{
	StepOptionIndex(1);
}

void UGameUIFocusValueRowWidget::SelectPreviousOption()
{
	StepOptionIndex(-1);
}

void UGameUIFocusValueRowWidget::OpenExpandedOptionList()
{
	if (!CanInteractWithRow())
	{
		RequestDisabledInteraction();
		return;
	}

	SetOptionListExpanded(true);
}

void UGameUIFocusValueRowWidget::CloseExpandedOptionList()
{
	SetOptionListExpanded(false);
}

FReply UGameUIFocusValueRowWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = HandleDirectionalInput(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusValueRowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = HandleDirectionalInput(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusValueRowWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	const float AnalogValue = InAnalogEvent.GetAnalogValue();

	if (Key == EKeys::Gamepad_LeftX)
	{
		if (UGameUIFocusPageWidgetBase* FocusPage = GetOwningFocusPage())
		{
			FocusPage->UpdateHorizontalAnalogSample(AnalogValue);
		}

		const bool bWasAwaitingRelease = HorizontalAnalogReleaseGate.IsAwaitingRelease();
		if (HorizontalAnalogReleaseGate.Process(AnalogValue, AnalogInputReleaseThreshold))
		{
			ResetHorizontalAnalogInput();
			if (bWasAwaitingRelease && !HorizontalAnalogReleaseGate.IsAwaitingRelease())
			{
				UE_LOG(LogGameUIFocusValueRow, VeryVerbose,
					TEXT("Value row horizontal analog release confirmed. Row=%s Value=%.3f"),
					*GetNameSafe(this),
					AnalogValue);
			}

			// The release sample only unlocks the row. A later, deliberate stick
			// gesture is required to change its value.
			return FReply::Handled();
		}

		const int32 Direction = AnalogValue > 0.0f ? 1 : -1;
		return HandleAnalogOptionInput(
			Direction,
			FMath::Abs(AnalogValue),
			bUsesSelectionBeforeApply,
			LastHorizontalAnalogInputTimeSeconds,
			LastHorizontalAnalogDirection,
			bHorizontalAnalogInputHeld,
			bHorizontalAnalogRepeatActive)
			? FReply::Handled()
			: FReply::Unhandled();
	}

	if (bIsOptionListExpanded && Key == EKeys::Gamepad_LeftY)
	{
		const int32 Direction = AnalogValue > 0.0f ? -1 : 1;
		return HandleAnalogOptionInput(
			Direction,
			FMath::Abs(AnalogValue),
			false,
			LastVerticalAnalogInputTimeSeconds,
			LastVerticalAnalogDirection,
			bVerticalAnalogInputHeld,
			bVerticalAnalogRepeatActive)
			? FReply::Handled()
			: FReply::Unhandled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UGameUIFocusValueRowWidget::HandleDirectionalInput(const FKeyEvent& KeyEvent)
{
	const EUINavigation NavigationDirection = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetNavigationDirectionFromKey(KeyEvent)
		: EUINavigation::Invalid;

	if (NavigationDirection == EUINavigation::Left)
	{
		if (!CanInteractWithRow())
		{
			if (!KeyEvent.IsRepeat())
			{
				RequestDisabledInteraction();
			}
		}
		else if (bUsesSelectionBeforeApply)
		{
			RequestSelection();
		}
		else if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
		{
			StepOptionIndex(-1);
		}
		return FReply::Handled();
	}

	if (NavigationDirection == EUINavigation::Right)
	{
		if (!CanInteractWithRow())
		{
			if (!KeyEvent.IsRepeat())
			{
				RequestDisabledInteraction();
			}
		}
		else if (bUsesSelectionBeforeApply)
		{
			RequestSelection();
		}
		else if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
		{
			StepOptionIndex(1);
		}
		return FReply::Handled();
	}

	const bool bAcceptAction = GameUIFocusInputKeys::IsAcceptAction(KeyEvent);
	if (bAcceptAction)
	{
		if (!CanInteractWithRow())
		{
			if (!KeyEvent.IsRepeat())
			{
				RequestDisabledInteraction();
			}
		}
		else if (bIsOptionListExpanded)
		{
			SelectOptionIndex(CurrentIndex);
		}
		else if (bUsesSelectionBeforeApply)
		{
			RequestSelection();
		}
		else if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
		{
			StepOptionIndex(1);
		}
		return FReply::Handled();
	}

	const bool bMoveUp = NavigationDirection == EUINavigation::Up;
	const bool bMoveDown = NavigationDirection == EUINavigation::Down;

	if (bMoveUp)
	{
		if (bIsOptionListExpanded)
		{
			if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
			{
				StepOptionIndex(-1);
			}
		}
		else if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
		{
			RequestMoveFocus(-1);
		}
		return FReply::Handled();
	}

	if (bMoveDown)
	{
		if (bIsOptionListExpanded)
		{
			if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
			{
				StepOptionIndex(1);
			}
		}
		else if (CanProcessDirectionalInput(KeyEvent.IsRepeat()))
		{
			RequestMoveFocus(1);
		}
		return FReply::Handled();
	}

	const bool bBackAction = GameUIFocusInputKeys::IsBackAction(KeyEvent);
	if (bIsOptionListExpanded && bBackAction)
	{
		SetOptionListExpanded(false);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

bool UGameUIFocusValueRowWidget::ShouldDrawSlider() const
{
	return ControlType == EGameUIFocusValueRowControlType::Slider && Options.Num() > 1;
}

bool UGameUIFocusValueRowWidget::ShouldDrawToggle() const
{
	return ControlType == EGameUIFocusValueRowControlType::Toggle && Options.Num() > 1;
}

bool UGameUIFocusValueRowWidget::ShouldUseCompactValueText() const
{
	return ShouldDrawSlider();
}

float UGameUIFocusValueRowWidget::GetSliderPercent() const
{
	if (Options.Num() <= 1)
	{
		return 0.0f;
	}

	return static_cast<float>(FMath::Clamp(CurrentIndex, 0, Options.Num() - 1)) / static_cast<float>(Options.Num() - 1);
}

FText UGameUIFocusValueRowWidget::GetCurrentOptionLabel() const
{
	return Options.IsValidIndex(CurrentIndex) ? Options[CurrentIndex] : FText::GetEmpty();
}

bool UGameUIFocusValueRowWidget::IsToggleOn() const
{
	return ControlType == EGameUIFocusValueRowControlType::Toggle && CurrentIndex > 0;
}

float UGameUIFocusValueRowWidget::GetCurrentNumericValue() const
{
	return GetNumericValueForIndex(CurrentIndex);
}

float UGameUIFocusValueRowWidget::GetNumericValueForIndex(int32 OptionIndex) const
{
	if (!bHasNumericRange)
	{
		return static_cast<float>(OptionIndex);
	}

	const int32 ClampedIndex = FMath::Max(OptionIndex, 0);
	return FMath::Clamp(NumericMinValue + static_cast<float>(ClampedIndex) * NumericStepSize, NumericMinValue, NumericMaxValue);
}

int32 UGameUIFocusValueRowWidget::GetOptionIndexForNumericValue(float Value) const
{
	if (!bHasNumericRange || NumericStepSize <= UE_KINDA_SMALL_NUMBER)
	{
		return FMath::Clamp(FMath::RoundToInt(Value), 0, FMath::Max(Options.Num() - 1, 0));
	}

	const float ClampedValue = FMath::Clamp(Value, NumericMinValue, NumericMaxValue);
	const int32 Index = FMath::RoundToInt((ClampedValue - NumericMinValue) / NumericStepSize);
	return FMath::Clamp(Index, 0, FMath::Max(Options.Num() - 1, 0));
}

void UGameUIFocusValueRowWidget::OnInteractionHighlightChanged_Implementation(bool bHighlighted)
{
	Super::OnInteractionHighlightChanged_Implementation(bHighlighted);
	SetInteractionFocused(bHighlighted);
}

bool UGameUIFocusValueRowWidget::CanProcessDirectionalInput(bool bIsRepeat)
{
	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (bIsRepeat && CurrentTimeSeconds - LastDirectionalInputTimeSeconds < static_cast<double>(DirectionalRepeatDelay))
	{
		return false;
	}

	LastDirectionalInputTimeSeconds = CurrentTimeSeconds;
	return true;
}

bool UGameUIFocusValueRowWidget::CanInteractWithRow() const
{
	return bIsValueRowEnabled && Options.Num() > 0;
}

void UGameUIFocusValueRowWidget::RequestDisabledInteraction()
{
	UE_LOG(LogGameUIFocusValueRow, Verbose,
		TEXT("Value row disabled interaction. Row=%s Identifier=%s Reason=%s"),
		*GetNameSafe(this),
		*RowIdentifier.ToString(),
		*DisabledReason.ToString());
	OnDisabledInteractionRequested.Broadcast(RowIdentifier);
}

bool UGameUIFocusValueRowWidget::HandleAnalogOptionInput(
	int32 Direction,
	float Magnitude,
	bool bSelectionOnly,
	double& LastInputTimeSeconds,
	int32& LastDirection,
	bool& bInputHeld,
	bool& bRepeatActive)
{
	if (Direction == 0 || Magnitude < AnalogInputReleaseThreshold)
	{
		ResetAnalogOptionInput(LastInputTimeSeconds, LastDirection, bInputHeld, bRepeatActive);
		return false;
	}

	if (Magnitude < AnalogInputDeadZone)
	{
		return true;
	}

	if (!CanInteractWithRow())
	{
		if (!bInputHeld || LastDirection != Direction)
		{
			bInputHeld = true;
			bRepeatActive = false;
			LastDirection = Direction;
			LastInputTimeSeconds = FPlatformTime::Seconds();
			RequestDisabledInteraction();
		}
		return true;
	}

	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (!bInputHeld || LastDirection != Direction)
	{
		bInputHeld = true;
		bRepeatActive = false;
		LastDirection = Direction;
		LastInputTimeSeconds = CurrentTimeSeconds;

		if (bSelectionOnly)
		{
			RequestSelection();
		}
		else
		{
			StepOptionIndex(Direction);
		}
		return true;
	}

	if (bSelectionOnly)
	{
		return true;
	}

	const double RepeatDelay = bRepeatActive
		? static_cast<double>(AnalogRepeatInterval)
		: static_cast<double>(AnalogInitialRepeatDelay);

	if (CurrentTimeSeconds - LastInputTimeSeconds >= RepeatDelay)
	{
		bRepeatActive = true;
		LastInputTimeSeconds = CurrentTimeSeconds;
		StepOptionIndex(Direction);
	}

	return true;
}

void UGameUIFocusValueRowWidget::ResetAnalogOptionInput(double& LastInputTimeSeconds, int32& LastDirection, bool& bInputHeld, bool& bRepeatActive)
{
	bInputHeld = false;
	bRepeatActive = false;
	LastDirection = 0;
	LastInputTimeSeconds = -1000.0;
}

void UGameUIFocusValueRowWidget::ResetHorizontalAnalogInput()
{
	ResetAnalogOptionInput(
		LastHorizontalAnalogInputTimeSeconds,
		LastHorizontalAnalogDirection,
		bHorizontalAnalogInputHeld,
		bHorizontalAnalogRepeatActive);
}

void UGameUIFocusValueRowWidget::ResetHorizontalAnalogReleaseGate()
{
	HorizontalAnalogReleaseGate.Reset();
	ResetHorizontalAnalogInput();
}

void UGameUIFocusValueRowWidget::StepOptionIndex(int32 Direction)
{
	if (!CanInteractWithRow())
	{
		RequestDisabledInteraction();
		return;
	}

	if (Options.Num() <= 0)
	{
		return;
	}

	const int32 NewIndex = (CurrentIndex + Direction + Options.Num()) % Options.Num();
	SetCurrentIndex(NewIndex, true);
}

void UGameUIFocusValueRowWidget::SetInteractionFocused(bool bInHasInteractionFocus)
{
	if (bHasInteractionFocus == bInHasInteractionFocus)
	{
		return;
	}

	bHasInteractionFocus = bInHasInteractionFocus;
	OnInteractionFocusChanged(bHasInteractionFocus);
}

void UGameUIFocusValueRowWidget::SetOptionListExpanded(bool bExpanded)
{
	if (bIsOptionListExpanded == bExpanded)
	{
		return;
	}

	bIsOptionListExpanded = bExpanded;
	OnOptionListExpandedChanged(bIsOptionListExpanded);
}

void UGameUIFocusValueRowWidget::RefreshBoundTextWidgets()
{
	if (Text_Label)
	{
		Text_Label->SetText(Label);
		Text_Label->SetIsEnabled(bIsValueRowEnabled);
	}

	if (Text_Value)
	{
		Text_Value->SetText(GetCurrentOptionLabel());
		Text_Value->SetIsEnabled(bIsValueRowEnabled);
	}
}

void UGameUIFocusValueRowWidget::BuildNumericOptions(int32 FractionalDigits, FText Suffix)
{
	Options.Reset();

	const int32 StepCount = FMath::Max(0, FMath::RoundToInt((NumericMaxValue - NumericMinValue) / NumericStepSize));
	for (int32 Index = 0; Index <= StepCount; ++Index)
	{
		const float Value = GetNumericValueForIndex(Index);
		FNumberFormattingOptions FormatOptions;
		FormatOptions.MinimumFractionalDigits = FractionalDigits;
		FormatOptions.MaximumFractionalDigits = FractionalDigits;

		FText ValueText = FText::AsNumber(Value, &FormatOptions);
		if (!Suffix.IsEmpty())
		{
			ValueText = FText::Format(NSLOCTEXT("GameUIFocus", "NumericValueWithSuffix", "{0}{1}"), ValueText, Suffix);
		}

		Options.Add(ValueText);
	}
}
