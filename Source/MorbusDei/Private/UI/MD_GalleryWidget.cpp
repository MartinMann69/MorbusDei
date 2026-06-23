#include "UI/MD_GalleryWidget.h"

#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tools/MD_MenuPreviewRig.h"

void UMD_GalleryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FindPreviewRig();
}

void UMD_GalleryWidget::NativeDestruct()
{
	if (bClearPreviewOnDestruct)
	{
		ClearPreview();
	}

	Super::NativeDestruct();
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