// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MD_GalleryWidget.generated.h"

class UWidget;
class AMD_MenuPreviewRig;

UCLASS()
class MORBUSDEI_API UMD_GalleryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UMD_GalleryWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="MD|Gallery")
	void ShowPreviewItem(TSubclassOf<AActor> PreviewClass);

	UFUNCTION(BlueprintCallable, Category="MD|Gallery")
	void ClearPreview();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UWidget> PreviewInputArea;

	UPROPERTY(BlueprintReadOnly, Category="MD|Gallery")
	TObjectPtr<AMD_MenuPreviewRig> PreviewRigRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Gallery")
	bool bClearPreviewOnDestruct = true;

private:
	bool bDraggingPreview = false;
	bool bPanUpHeld = false;
	bool bPanDownHeld = false;
	bool bPanLeftHeld = false;
	bool bPanRightHeld = false;
	FVector2D LastMouseScreenPosition = FVector2D::ZeroVector;

	void FindPreviewRig();
	bool IsPointerOverPreviewArea(const FPointerEvent& MouseEvent) const;
	FReply HandleKeyboardPanKeyDown(const FKeyEvent& KeyEvent);
	FReply HandleKeyboardPanKeyUp(const FKeyEvent& KeyEvent);
	void ClearKeyboardPanInput();
	FVector2D GetKeyboardPanInput() const;
};
