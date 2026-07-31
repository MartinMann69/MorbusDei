#include "UI/Focus/GameUIFocusItemWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
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
	SetIsFocusable(true);
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemConstruct Item=%s Class=%s FocusableForced=1"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()));
}

void UGameUIFocusItemWidgetBase::SetOwningFocusPage(UGameUIFocusPageWidgetBase* Page)
{
	OwningFocusPage = Page;
}

void UGameUIFocusItemWidgetBase::SetOwningNavigationScreen(UGameUIFocusScreenWidgetBase* Screen)
{
	OwningNavigationScreen = Screen;
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
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			SetUserFocus(PlayerController);
		}
		SetKeyboardFocus();
		NotifyFocused();
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

FReply UGameUIFocusItemWidgetBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ItemPreviewKey Item=%s Key=%s Page=%s"),
		*GetNameSafe(this),
		*InKeyEvent.GetKey().ToString(),
		*GetNameSafe(OwningFocusPage.Get()));

	if (HandleFocusItemPreviewKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}

	const FKey Key = InKeyEvent.GetKey();
	if (IsLeftKey(Key) && RequestMoveFocus2D(-1, 0))
	{
		return FReply::Handled();
	}
	if (IsRightKey(Key))
	{
		if (RightFocusOverrideIdentifier.IsValid() && OwningFocusPage
			&& OwningFocusPage->FocusItemByIdentifier(RightFocusOverrideIdentifier))
		{
			return FReply::Handled();
		}
		if (RequestMoveFocus2D(1, 0))
		{
			return FReply::Handled();
		}
	}
	if (IsUpKey(Key) && RequestMoveFocus2D(0, -1))
	{
		return FReply::Handled();
	}
	if (IsDownKey(Key))
	{
		if (DownFocusOverrideIdentifier.IsValid() && OwningFocusPage
			&& OwningFocusPage->FocusItemByIdentifier(DownFocusOverrideIdentifier))
		{
			return FReply::Handled();
		}
		if (RequestMoveFocus2D(0, 1))
		{
			return FReply::Handled();
		}
	}

	if (bReturnToNavigationOnBack && IsBackKey(Key) && RequestReturnToNavigationZone())
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusItemWidgetBase::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	const float AnalogValue = InAnalogEvent.GetAnalogValue();

	if (Key == EKeys::Gamepad_LeftX)
	{
		if (FMath::Abs(AnalogValue) < FocusItemAnalogNavigationReleaseThreshold)
		{
			ResetAnalogNavigationMove();
			return FReply::Unhandled();
		}

		const FIntPoint Direction(AnalogValue > 0.0f ? 1 : -1, 0);
		return HandleAnalogNavigationMove(Direction, FMath::Abs(AnalogValue)) ? FReply::Handled() : FReply::Unhandled();
	}

	if (Key == EKeys::Gamepad_LeftY)
	{
		if (FMath::Abs(AnalogValue) < FocusItemAnalogNavigationReleaseThreshold)
		{
			ResetAnalogNavigationMove();
			return FReply::Unhandled();
		}

		const FIntPoint Direction(0, AnalogValue > 0.0f ? -1 : 1);
		return HandleAnalogNavigationMove(Direction, FMath::Abs(AnalogValue)) ? FReply::Handled() : FReply::Unhandled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
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
	const bool bShouldHighlight = bHasFocusPath || bIsHovered;
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

bool UGameUIFocusItemWidgetBase::HandleAnalogNavigationMove(const FIntPoint Direction, const float Magnitude)
{
	if (Direction == FIntPoint::ZeroValue || Magnitude < FocusItemAnalogNavigationDeadZone)
	{
		return Magnitude >= FocusItemAnalogNavigationReleaseThreshold;
	}

	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (!bAnalogNavigationHeld || LastAnalogNavigationDirection != Direction)
	{
		bAnalogNavigationHeld = true;
		bAnalogNavigationRepeatActive = false;
		LastAnalogNavigationDirection = Direction;
		LastAnalogNavigationMoveTimeSeconds = CurrentTimeSeconds;
		RequestMoveFocus2D(Direction.X, Direction.Y);
		return true;
	}

	const double RepeatDelay = bAnalogNavigationRepeatActive
		? static_cast<double>(FocusItemAnalogRepeatInterval)
		: static_cast<double>(FocusItemAnalogInitialRepeatDelay);

	if (CurrentTimeSeconds - LastAnalogNavigationMoveTimeSeconds >= RepeatDelay)
	{
		bAnalogNavigationRepeatActive = true;
		LastAnalogNavigationMoveTimeSeconds = CurrentTimeSeconds;
		RequestMoveFocus2D(Direction.X, Direction.Y);
	}

	return true;
}

void UGameUIFocusItemWidgetBase::ResetAnalogNavigationMove()
{
	bAnalogNavigationHeld = false;
	bAnalogNavigationRepeatActive = false;
	LastAnalogNavigationDirection = FIntPoint::ZeroValue;
	LastAnalogNavigationMoveTimeSeconds = -1000.0;
}

bool UGameUIFocusItemWidgetBase::IsBackKey(const FKey& Key)
{
	return Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Right;
}

bool UGameUIFocusItemWidgetBase::IsLeftKey(const FKey& Key)
{
	return Key == EKeys::Left
		|| Key == EKeys::Gamepad_DPad_Left;
}

bool UGameUIFocusItemWidgetBase::IsRightKey(const FKey& Key)
{
	return Key == EKeys::Right
		|| Key == EKeys::Gamepad_DPad_Right;
}

bool UGameUIFocusItemWidgetBase::IsUpKey(const FKey& Key)
{
	return Key == EKeys::Up
		|| Key == EKeys::Gamepad_DPad_Up;
}

bool UGameUIFocusItemWidgetBase::IsDownKey(const FKey& Key)
{
	return Key == EKeys::Down
		|| Key == EKeys::Gamepad_DPad_Down;
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
