#include "UI/Focus/GameUIFocusItemWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "UI/Focus/GameUIFocusInputKeys.h"
#include "UI/Focus/GameUIFocusInputDeviceTracker.h"
#include "UI/Focus/GameUIFocusPageWidgetBase.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"
#include "UI/Focus/GameUIFocusTypes.h"

DEFINE_LOG_CATEGORY(LogGameUIFocus);

UGameUIFocusItemWidgetBase::UGameUIFocusItemWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UGameUIFocusItemWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	PointerInputStateChangedHandle = GameUIFocusInputDeviceTracker::OnPointerInputStateChanged().AddUObject(
		this,
		&UGameUIFocusItemWidgetBase::SetPointerInputActive);
	SetPointerInputActive(GameUIFocusInputDeviceTracker::IsPointerInputActive());
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemConstruct Item=%s Class=%s Focusable=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()),
		IsFocusable() ? 1 : 0);
}

void UGameUIFocusItemWidgetBase::NativeDestruct()
{
	GameUIFocusInputDeviceTracker::OnPointerInputStateChanged().Remove(PointerInputStateChangedHandle);
	PointerInputStateChangedHandle.Reset();
	bAcceptPressed = false;
	bPointerPressed = false;
	Super::NativeDestruct();
}

void UGameUIFocusItemWidgetBase::SetOwningFocusPage(UGameUIFocusPageWidgetBase* Page)
{
	OwningFocusPage = Page;
	if (Page)
	{
		OwningNavigationScreen = nullptr;
	}
}

void UGameUIFocusItemWidgetBase::SetOwningNavigationScreen(UGameUIFocusScreenWidgetBase* Screen)
{
	OwningNavigationScreen = Screen;
	if (Screen)
	{
		OwningFocusPage = nullptr;
	}
}

void UGameUIFocusItemWidgetBase::NotifyFocused()
{
	if (OwningFocusPage)
	{
		OwningFocusPage->NotifyFocusItemFocused(this);
		return;
	}

	if (OwningNavigationScreen)
	{
		OwningNavigationScreen->NotifyNavigationWidgetFocused(this);
	}
}

void UGameUIFocusItemWidgetBase::ActivateFocusItem()
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemActivated Item=%s Page=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwningFocusPage.Get()));

	NotifyFocused();
	OnFocusItemActivated.Broadcast(this);

	if (bTriggerChildButtonClickOnActivation && TriggerFirstChildButtonClick())
	{
		return;
	}

	HandleFocusItemActivated();
}

bool UGameUIFocusItemWidgetBase::RequestReturnToNavigationZone()
{
	return OwningFocusPage ? OwningFocusPage->RequestReturnToNavigationZone() : false;
}

bool UGameUIFocusItemWidgetBase::RequestMoveFocus(int32 Direction)
{
	return OwningFocusPage ? OwningFocusPage->FocusAdjacentItem(this, Direction) : false;
}

bool UGameUIFocusItemWidgetBase::RequestMoveFocus2D(const int32 ColumnDirection, const int32 RowDirection)
{
	return OwningFocusPage ? OwningFocusPage->FocusAdjacentItem2D(this, FIntPoint(ColumnDirection, RowDirection)) : false;
}

void UGameUIFocusItemWidgetBase::SetFocusIdentifier(FGameplayTag InFocusIdentifier)
{
	FocusIdentifier = InFocusIdentifier;
}

void UGameUIFocusItemWidgetBase::SetFocusGridPosition(const int32 Column, const int32 Row)
{
	FocusGridPosition = FIntPoint(Column, Row);
	bHasFocusGridPosition = true;
}

void UGameUIFocusItemWidgetBase::ClearFocusGridPosition()
{
	FocusGridPosition = FIntPoint::ZeroValue;
	bHasFocusGridPosition = false;
}

void UGameUIFocusItemWidgetBase::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemAddedToFocusPath Item=%s Page=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwningFocusPage.Get()));
	NotifyFocused();
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	SetFocusPathActive(true);
}

void UGameUIFocusItemWidgetBase::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemRemovedFromFocusPath Item=%s Page=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwningFocusPage.Get()));
	bAcceptPressed = false;
	bPointerPressed = false;
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	SetFocusPathActive(false);
}

void UGameUIFocusItemWidgetBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHovered(true);
}

void UGameUIFocusItemWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bPointerPressed = false;
	SetHovered(false);
}

FReply UGameUIFocusItemWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPointerPressed = true;
		RequestInteractionFocus();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGameUIFocusItemWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPointerPressed)
	{
		bPointerPressed = false;
		ActivateFocusItem();
		return FReply::Handled();
	}

	bPointerPressed = false;
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

bool UGameUIFocusItemWidgetBase::RequestInteractionFocus()
{
	if (OwningFocusPage)
	{
		if (UGameUIFocusScreenWidgetBase* FocusScreen = OwningFocusPage->GetOwningFocusScreen())
		{
			return FocusScreen->RequestFocusNextTickForZone(this, EGameUIFocusZone::Content);
		}
	}
	else if (OwningNavigationScreen)
	{
		return OwningNavigationScreen->RequestFocusNextTickForZone(this, EGameUIFocusZone::Navigation);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		SetUserFocus(PlayerController);
		if (HasUserFocus(PlayerController))
		{
			return true;
		}
	}

	SetKeyboardFocus();
	return HasKeyboardFocus();
}

FGameplayTag UGameUIFocusItemWidgetBase::GetFocusOverrideIdentifier(const FIntPoint Direction) const
{
	if (Direction.X > 0)
	{
		return RightFocusOverrideIdentifier;
	}
	if (Direction.Y > 0)
	{
		return DownFocusOverrideIdentifier;
	}

	return FGameplayTag();
}

void UGameUIFocusItemWidgetBase::SetPointerInputActive(const bool bActive)
{
	if (bPointerInputActive == bActive)
	{
		return;
	}

	bPointerInputActive = bActive;
	if (!bPointerInputActive)
	{
		bPointerPressed = false;
	}
	UpdateInteractionHighlight();
}

FReply UGameUIFocusItemWidgetBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey().IsGamepadKey())
	{
		UGameUIFocusScreenWidgetBase* FocusScreen = OwningNavigationScreen.Get();
		if (!FocusScreen && OwningFocusPage)
		{
			FocusScreen = OwningFocusPage->GetOwningFocusScreen();
		}
		if (FocusScreen)
		{
			FocusScreen->NotifyGamepadInput();
		}
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemPreviewKey Item=%s Key=%s Page=%s"),
		*GetNameSafe(this),
		*InKeyEvent.GetKey().ToString(),
		*GetNameSafe(OwningFocusPage.Get()));

	if (HandleFocusItemPreviewKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}

	if (FSlateApplication::IsInitialized())
	{
		const EUINavigation NavigationDirection = FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent);
		FIntPoint Direction = FIntPoint::ZeroValue;
		switch (NavigationDirection)
		{
		case EUINavigation::Left: Direction = FIntPoint(-1, 0); break;
		case EUINavigation::Right: Direction = FIntPoint(1, 0); break;
		case EUINavigation::Up: Direction = FIntPoint(0, -1); break;
		case EUINavigation::Down: Direction = FIntPoint(0, 1); break;
		default: break;
		}

		if (Direction != FIntPoint::ZeroValue)
		{
			if (OwningFocusPage && OwningFocusPage->HandleFocusItemDigitalInput(this, Direction))
			{
				return FReply::Handled();
			}

			if (OwningNavigationScreen
				&& OwningNavigationScreen->HandleNavigationWidgetDigitalInput(this, Direction, InKeyEvent.IsRepeat()))
			{
				return FReply::Handled();
			}
		}
	}

	const bool bBackAction = GameUIFocusInputKeys::IsBackAction(InKeyEvent);
	if (bReturnToNavigationOnBack && bBackAction && RequestReturnToNavigationZone())
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusItemWidgetBase::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (!IsAcceptAction(InKeyEvent))
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	if (!InKeyEvent.IsRepeat())
	{
		bAcceptPressed = true;
	}

	return FReply::Handled();
}

FReply UGameUIFocusItemWidgetBase::NativeOnKeyUp(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (!IsAcceptAction(InKeyEvent))
	{
		return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
	}

	const bool bShouldActivate = bAcceptPressed;
	bAcceptPressed = false;
	if (bShouldActivate)
	{
		ActivateFocusItem();
	}

	return FReply::Handled();
}

FReply UGameUIFocusItemWidgetBase::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey().IsGamepadKey())
	{
		UGameUIFocusScreenWidgetBase* FocusScreen = OwningNavigationScreen.Get();
		if (!FocusScreen && OwningFocusPage)
		{
			FocusScreen = OwningFocusPage->GetOwningFocusScreen();
		}
		if (FocusScreen)
		{
			FocusScreen->NotifyGamepadInput(InAnalogEvent.GetAnalogValue());
		}
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemAnalog Item=%s Key=%s Value=%.3f Page=%s Screen=%s"),
		*GetNameSafe(this),
		*InAnalogEvent.GetKey().ToString(),
		InAnalogEvent.GetAnalogValue(),
		*GetNameSafe(OwningFocusPage.Get()),
		*GetNameSafe(OwningNavigationScreen.Get()));

	if (OwningFocusPage)
	{
		return OwningFocusPage->HandleFocusItemAnalogInput(
			this,
			InAnalogEvent.GetKey(),
			InAnalogEvent.GetAnalogValue())
			? FReply::Handled()
			: Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	if (OwningNavigationScreen
		&& OwningNavigationScreen->HandleNavigationWidgetAnalogInput(
			this,
			InAnalogEvent.GetKey(),
			InAnalogEvent.GetAnalogValue()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

bool UGameUIFocusItemWidgetBase::TryGetCustomizedLegacyAnalogNavigationConfig(
	FGameUIAnalogNavigationConfig& OutConfig) const
{
	constexpr float LegacyDeadZone = 0.55f;
	constexpr float LegacyReleaseThreshold = 0.35f;
	constexpr float LegacyInitialRepeatDelay = 0.30f;
	constexpr float LegacyRepeatInterval = 0.11f;

	const bool bCustomized = !FMath::IsNearlyEqual(FocusItemAnalogNavigationDeadZone, LegacyDeadZone)
		|| !FMath::IsNearlyEqual(FocusItemAnalogNavigationReleaseThreshold, LegacyReleaseThreshold)
		|| !FMath::IsNearlyEqual(FocusItemAnalogInitialRepeatDelay, LegacyInitialRepeatDelay)
		|| !FMath::IsNearlyEqual(FocusItemAnalogRepeatInterval, LegacyRepeatInterval);
	if (!bCustomized)
	{
		return false;
	}

	OutConfig.DeadZone = FocusItemAnalogNavigationDeadZone;
	OutConfig.ReleaseThreshold = FocusItemAnalogNavigationReleaseThreshold;
	OutConfig.InitialRepeatDelay = FocusItemAnalogInitialRepeatDelay;
	OutConfig.RepeatInterval = FocusItemAnalogRepeatInterval;
	return true;
}

bool UGameUIFocusItemWidgetBase::HandleFocusItemPreviewKey_Implementation(FKey Key)
{
	return false;
}

void UGameUIFocusItemWidgetBase::HandleFocusItemActivated_Implementation()
{
}

void UGameUIFocusItemWidgetBase::OnInteractionHighlightChanged_Implementation(bool bHighlighted)
{
}

void UGameUIFocusItemWidgetBase::SetFocusPathActive(bool bInHasFocusPath)
{
	if (bHasFocusPath == bInHasFocusPath)
	{
		return;
	}

	bHasFocusPath = bInHasFocusPath;
	UpdateInteractionHighlight();
}

void UGameUIFocusItemWidgetBase::SetHovered(bool bInIsHovered)
{
	if (bIsHovered == bInIsHovered)
	{
		return;
	}

	bIsHovered = bInIsHovered;
	UpdateInteractionHighlight();
}

void UGameUIFocusItemWidgetBase::UpdateInteractionHighlight()
{
	const bool bShouldHighlight = bHasFocusPath || (bPointerInputActive && bIsHovered);
	if (bIsInteractionHighlighted == bShouldHighlight)
	{
		return;
	}

	bIsInteractionHighlighted = bShouldHighlight;
	OnInteractionHighlightChanged(bIsInteractionHighlighted);
}

bool UGameUIFocusItemWidgetBase::TriggerFirstChildButtonClick()
{
	if (!WidgetTree)
	{
		return false;
	}

	UButton* ButtonToClick = nullptr;
	WidgetTree->ForEachWidget([&ButtonToClick](UWidget* Widget)
	{
		if (ButtonToClick)
		{
			return;
		}

		UButton* Button = Cast<UButton>(Widget);
		if (IsUsableFocusTarget(Button))
		{
			ButtonToClick = Button;
		}
	});

	if (!ButtonToClick)
	{
		return false;
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemTriggerChildButtonClick Item=%s Button=%s"),
		*GetNameSafe(this),
		*GetNameSafe(ButtonToClick));

	ButtonToClick->OnClicked.Broadcast();
	return true;
}

bool UGameUIFocusItemWidgetBase::IsAcceptAction(const FKeyEvent& KeyEvent)
{
	return GameUIFocusInputKeys::IsAcceptAction(KeyEvent);
}

bool UGameUIFocusItemWidgetBase::IsBackKey(const FKey& Key)
{
	return Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Right;
}

bool UGameUIFocusItemWidgetBase::IsUsableFocusTarget(const UWidget* Widget)
{
	if (!IsValid(Widget) || !Widget->GetIsEnabled())
	{
		return false;
	}

	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed
		&& Visibility != ESlateVisibility::Hidden;
}
