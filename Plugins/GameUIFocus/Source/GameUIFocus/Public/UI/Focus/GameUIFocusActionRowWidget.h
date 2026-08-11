#pragma once

#include "CoreMinimal.h"
#include "UI/Focus/GameUIFocusItemWidgetBase.h"
#include "GameUIFocusActionRowWidget.generated.h"

class UBorder;
class USoundBase;
class UTextBlock;
class UWidget;
class UGameUIFocusActionRowWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameUIFocusActionTriggered, UGameUIFocusActionRowWidget*, ActionRow);

/**
 * A single, button-free focus target for actions displayed inside settings pages.
 * Input is committed on Accept release so keyboard, gamepad and pointer share one activation path.
 */
UCLASS(BlueprintType, Blueprintable)
class GAMEUIFOCUS_API UGameUIFocusActionRowWidget : public UGameUIFocusItemWidgetBase
{
	GENERATED_BODY()

public:
	UGameUIFocusActionRowWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus|Action Row")
	FGameUIFocusActionTriggered OnActionTriggered;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Action Row")
	void InitializeActionRow(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Action Row")
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Action Row")
	FText GetLabel() const { return Label; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void HandleFocusItemActivated_Implementation() override;
	virtual void OnInteractionHighlightChanged_Implementation(bool bHighlighted) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Action Row")
	void OnActionPressedStateChanged(bool bPressed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Action Row")
	void OnActionHighlightStateChanged(bool bHighlighted);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game UI|Focus|Action Row")
	FText Label;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Action Row|Feedback")
	TObjectPtr<USoundBase> ActivationSound = nullptr;

	/** Played once when keyboard, gamepad or pointer interaction highlights this row. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Action Row|Feedback")
	TObjectPtr<USoundBase> HighlightSound = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_FocusFrame = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Overlay_Pressed = nullptr;

	bool IsActionPressed() const { return bActionPressed; }

private:
	void RefreshLabel();
	void SetActionPressed(bool bPressed);
	void CancelPendingAction();
	static bool IsAcceptAction(const FKeyEvent& InKeyEvent);

	UPROPERTY(Transient)
	bool bActionPressed = false;

	UPROPERTY(Transient)
	bool bPointerPressStarted = false;
};
