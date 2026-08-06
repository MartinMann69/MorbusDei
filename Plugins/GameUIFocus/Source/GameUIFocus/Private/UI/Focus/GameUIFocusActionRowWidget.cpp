#include "UI/Focus/GameUIFocusActionRowWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/Focus/GameUIFocusInputKeys.h"

UGameUIFocusActionRowWidget::UGameUIFocusActionRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UUserWidget defaults to SelfHitTestInvisible. The action row has no child button,
	// so the outer focus item itself must participate in pointer hit testing.
	SetVisibility(ESlateVisibility::Visible);
	bTriggerChildButtonClickOnActivation = false;
}

void UGameUIFocusActionRowWidget::InitializeActionRow(FText InLabel)
{
	SetLabel(MoveTemp(InLabel));
}

void UGameUIFocusActionRowWidget::SetLabel(FText InLabel)
{
	Label = MoveTemp(InLabel);
	RefreshLabel();
}

void UGameUIFocusActionRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshLabel();

	if (Border_FocusFrame)
	{
		Border_FocusFrame->SetRenderOpacity(IsInteractionHighlighted() ? 1.0f : 0.0f);
	}
	if (Overlay_Pressed)
	{
		Overlay_Pressed->SetRenderOpacity(bActionPressed ? 1.0f : 0.0f);
	}
}

void UGameUIFocusActionRowWidget::NativeDestruct()
{
	CancelPendingAction();
	Super::NativeDestruct();
}

void UGameUIFocusActionRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	CancelPendingAction();
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
}

FReply UGameUIFocusActionRowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!IsAcceptAction(InKeyEvent))
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	if (!InKeyEvent.IsRepeat())
	{
		SetActionPressed(true);
	}
	return FReply::Handled();
}

FReply UGameUIFocusActionRowWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!IsAcceptAction(InKeyEvent))
	{
		return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
	}

	const bool bShouldActivate = bActionPressed;
	SetActionPressed(false);
	if (bShouldActivate)
	{
		ActivateFocusItem();
	}
	return FReply::Handled();
}

void UGameUIFocusActionRowWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// Keep pointer and controller selection in sync, matching the existing
	// WBP_BaseButton hover contract without adding a focusable child button.
	RequestInteractionFocus();
}

FReply UGameUIFocusActionRowWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const TSharedRef<SWidget> MouseCaptor = TakeWidget();
	RequestInteractionFocus();
	bPointerPressStarted = true;
	SetActionPressed(true);
	return FReply::Handled().CaptureMouse(MouseCaptor);
}

FReply UGameUIFocusActionRowWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	const bool bShouldActivate = bPointerPressStarted && bActionPressed;
	bPointerPressStarted = false;
	SetActionPressed(false);
	if (bShouldActivate)
	{
		ActivateFocusItem();
	}
	return FReply::Handled().ReleaseMouseCapture();
}

void UGameUIFocusActionRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	CancelPendingAction();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void UGameUIFocusActionRowWidget::HandleFocusItemActivated_Implementation()
{
	Super::HandleFocusItemActivated_Implementation();
	if (ActivationSound)
	{
		UGameplayStatics::PlaySound2D(this, ActivationSound);
	}
	OnActionTriggered.Broadcast(this);
}

void UGameUIFocusActionRowWidget::OnInteractionHighlightChanged_Implementation(const bool bHighlighted)
{
	Super::OnInteractionHighlightChanged_Implementation(bHighlighted);
	if (bHighlighted && HighlightSound)
	{
		UGameplayStatics::PlaySound2D(this, HighlightSound);
	}
	if (Border_FocusFrame)
	{
		Border_FocusFrame->SetRenderOpacity(bHighlighted ? 1.0f : 0.0f);
	}
	OnActionHighlightStateChanged(bHighlighted);
}

void UGameUIFocusActionRowWidget::RefreshLabel()
{
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
}

void UGameUIFocusActionRowWidget::SetActionPressed(const bool bPressed)
{
	if (bActionPressed == bPressed)
	{
		return;
	}

	bActionPressed = bPressed;
	if (Overlay_Pressed)
	{
		Overlay_Pressed->SetRenderOpacity(bPressed ? 1.0f : 0.0f);
	}
	SetRenderScale(bPressed ? FVector2D(0.985f) : FVector2D(1.0f));
	OnActionPressedStateChanged(bPressed);
}

void UGameUIFocusActionRowWidget::CancelPendingAction()
{
	bPointerPressStarted = false;
	SetActionPressed(false);
}

bool UGameUIFocusActionRowWidget::IsAcceptAction(const FKeyEvent& InKeyEvent)
{
	return GameUIFocusInputKeys::IsAcceptAction(InKeyEvent);
}
