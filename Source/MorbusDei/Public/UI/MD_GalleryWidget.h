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
	UFUNCTION(BlueprintCallable, Category="MD|Gallery")
	void ShowPreviewItem(TSubclassOf<AActor> PreviewClass);

	UFUNCTION(BlueprintCallable, Category="MD|Gallery")
	void ClearPreview();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UWidget> PreviewInputArea;

	UPROPERTY(BlueprintReadOnly, Category="MD|Gallery")
	TObjectPtr<AMD_MenuPreviewRig> PreviewRigRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Gallery")
	bool bClearPreviewOnDestruct = true;

private:
	bool bDraggingPreview = false;
	FVector2D LastMouseScreenPosition = FVector2D::ZeroVector;

	void FindPreviewRig();
	bool IsPointerOverPreviewArea(const FPointerEvent& MouseEvent) const;
};
