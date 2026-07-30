#include "UI/MD_GalleryWidget.h"

#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tools/MD_MenuPreviewRig.h"

UMD_GalleryWidget::UMD_GalleryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UMD_GalleryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FindPreviewRig();
}

void UMD_GalleryWidget::NativeDestruct()
{
	ClearKeyboardPanInput();

	if (bClearPreviewOnDestruct)
	{
		ClearPreview();
	}

	Super::NativeDestruct();
}

void UMD_GalleryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D PanInput = GetKeyboardPanInput();
	if (PanInput.IsNearlyZero())
	{
		return;
	}

	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->PanPreview(PanInput.X, PanInput.Y, InDeltaTime);
	}
}

void UMD_GalleryWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	ClearKeyboardPanInput();
	Super::NativeOnFocusLost(InFocusEvent);
}

void UMD_GalleryWidget::FindPreviewRig()
{
	if (!GetWorld())
	{
		return;
	}

	PreviewRigRef = Cast<AMD_MenuPreviewRig>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AMD_MenuPreviewRig::StaticClass())
	);
}

void UMD_GalleryWidget::ShowPreviewItem(TSubclassOf<AActor> PreviewClass)
{
	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->ShowPreview(PreviewClass);
	}

	SetKeyboardFocus();
}

void UMD_GalleryWidget::ClearPreview()
{
	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->ClearPreview();
	}
}

FReply UMD_GalleryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsPointerOverPreviewArea(InMouseEvent))
	{
		bDraggingPreview = true;
		LastMouseScreenPosition = InMouseEvent.GetScreenSpacePosition();
		SetKeyboardFocus();

		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingPreview)
	{
		bDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDraggingPreview)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	const FVector2D CurrentMousePosition = InMouseEvent.GetScreenSpacePosition();
	const FVector2D MouseDelta = CurrentMousePosition - LastMouseScreenPosition;
	LastMouseScreenPosition = CurrentMousePosition;

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->RotatePreview(MouseDelta.X, MouseDelta.Y);
	}

	return FReply::Handled();
}

FReply UMD_GalleryWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsPointerOverPreviewArea(InMouseEvent))
	{
		if (!IsValid(PreviewRigRef))
		{
			FindPreviewRig();
		}

		if (IsValid(PreviewRigRef))
		{
			PreviewRigRef->ZoomPreview(InMouseEvent.GetWheelDelta());
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = HandleKeyboardPanKeyDown(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMD_GalleryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = HandleKeyboardPanKeyDown(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UMD_GalleryWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = HandleKeyboardPanKeyUp(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

bool UMD_GalleryWidget::IsPointerOverPreviewArea(const FPointerEvent& MouseEvent) const
{
	if (!PreviewInputArea)
	{
		return false;
	}

	const FGeometry AreaGeometry = PreviewInputArea->GetCachedGeometry();
	const FVector2D LocalPosition = AreaGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D LocalSize = AreaGeometry.GetLocalSize();

	return LocalPosition.X >= 0.0f
		&& LocalPosition.Y >= 0.0f
		&& LocalPosition.X <= LocalSize.X
		&& LocalPosition.Y <= LocalSize.Y;
}

FReply UMD_GalleryWidget::HandleKeyboardPanKeyDown(const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::A)
	{
		bPanLeftHeld = true;
	}
	else if (Key == EKeys::D)
	{
		bPanRightHeld = true;
	}
	else if (Key == EKeys::W)
	{
		bPanUpHeld = true;
	}
	else if (Key == EKeys::S)
	{
		bPanDownHeld = true;
	}
	else
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply UMD_GalleryWidget::HandleKeyboardPanKeyUp(const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::A)
	{
		bPanLeftHeld = false;
	}
	else if (Key == EKeys::D)
	{
		bPanRightHeld = false;
	}
	else if (Key == EKeys::W)
	{
		bPanUpHeld = false;
	}
	else if (Key == EKeys::S)
	{
		bPanDownHeld = false;
	}
	else
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

void UMD_GalleryWidget::ClearKeyboardPanInput()
{
	bPanUpHeld = false;
	bPanDownHeld = false;
	bPanLeftHeld = false;
	bPanRightHeld = false;
}

FVector2D UMD_GalleryWidget::GetKeyboardPanInput() const
{
	const float HorizontalDirection = (bPanRightHeld ? 1.0f : 0.0f) - (bPanLeftHeld ? 1.0f : 0.0f);
	const float VerticalDirection = (bPanUpHeld ? 1.0f : 0.0f) - (bPanDownHeld ? 1.0f : 0.0f);

	return FVector2D(HorizontalDirection, VerticalDirection).GetSafeNormal();
}
