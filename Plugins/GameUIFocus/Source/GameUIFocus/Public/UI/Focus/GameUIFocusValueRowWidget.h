#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Focus/GameUIFocusItemWidgetBase.h"
#include "GameUIFocusValueRowWidget.generated.h"

class UTextBlock;

UENUM(BlueprintType)
enum class EGameUIFocusValueRowControlType : uint8
{
	OptionList UMETA(DisplayName = "Option List"),
	Slider UMETA(DisplayName = "Slider"),
	Toggle UMETA(DisplayName = "Toggle"),
	SelectionModal UMETA(DisplayName = "Selection Modal")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameUIFocusValueRowChanged, FGameplayTag, RowIdentifier, int32, OptionIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGameUIFocusNumericValueRowChanged, FGameplayTag, RowIdentifier, int32, OptionIndex, float, NumericValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameUIFocusValueRowSelectionRequested, FGameplayTag, RowIdentifier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameUIFocusValueRowDisabledInteractionRequested, FGameplayTag, RowIdentifier);

UCLASS(BlueprintType, Blueprintable)
class GAMEUIFOCUS_API UGameUIFocusValueRowWidget : public UGameUIFocusItemWidgetBase
{
	GENERATED_BODY()

public:
	UGameUIFocusValueRowWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus|Value Row")
	FGameUIFocusValueRowChanged OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus|Value Row")
	FGameUIFocusNumericValueRowChanged OnNumericValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus|Value Row")
	FGameUIFocusValueRowSelectionRequested OnSelectionRequested;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus|Value Row")
	FGameUIFocusValueRowDisabledInteractionRequested OnDisabledInteractionRequested;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void InitializeValueRow(FGameplayTag InRowIdentifier, FText InLabel, const TArray<FText>& InOptions, int32 InCurrentIndex);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void InitializeValueRowWithControlType(
		FGameplayTag InRowIdentifier,
		FText InLabel,
		EGameUIFocusValueRowControlType InControlType,
		const TArray<FText>& InOptions,
		int32 InCurrentIndex,
		bool bInEnabled = true,
		FText InDisabledReason = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void InitializeNumericValueRow(
		FGameplayTag InRowIdentifier,
		FText InLabel,
		float MinValue,
		float MaxValue,
		float StepSize,
		float CurrentValue,
		int32 FractionalDigits = 1,
		FText Suffix = FText::GetEmpty(),
		bool bInEnabled = true,
		FText InDisabledReason = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void SetCurrentIndex(int32 InCurrentIndex, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void SelectOptionIndex(int32 OptionIndex);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void RequestSelection();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void SelectNextOption();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void SelectPreviousOption();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void OpenExpandedOptionList();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void CloseExpandedOptionList();

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	FGameplayTag GetRowIdentifier() const { return RowIdentifier; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	FText GetLabel() const { return Label; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	const TArray<FText>& GetOptions() const { return Options; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	EGameUIFocusValueRowControlType GetControlType() const { return ControlType; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool IsOptionListExpanded() const { return bIsOptionListExpanded; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool UsesSelectionBeforeApply() const { return bUsesSelectionBeforeApply; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool IsRowEnabled() const { return bIsValueRowEnabled; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	FText GetDisabledReason() const { return DisabledReason; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool WasLastValueChangeUserInitiated() const { return bLastValueChangeWasUserInitiated; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool IsInteractionFocused() const { return bHasInteractionFocus; }

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus|Value Row")
	void SetInteractionFocused(bool bInHasInteractionFocus);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool ShouldDrawSlider() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool ShouldDrawToggle() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool ShouldUseCompactValueText() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	float GetSliderPercent() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	FText GetCurrentOptionLabel() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool IsToggleOn() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	bool HasNumericRange() const { return bHasNumericRange; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	float GetCurrentNumericValue() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	float GetNumericValueForIndex(int32 OptionIndex) const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Value Row")
	int32 GetOptionIndexForNumericValue(float Value) const;

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual void OnInteractionHighlightChanged_Implementation(bool bHighlighted) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Value Row")
	void OnRowDataChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Value Row")
	void OnCurrentIndexChanged(int32 CurrentOptionIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Value Row")
	void OnSelectionRequestedEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Value Row")
	void OnOptionListExpandedChanged(bool bExpanded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Focus|Value Row")
	void OnInteractionFocusChanged(bool bFocused);

private:
	FReply HandleDirectionalInput(const FKeyEvent& KeyEvent);
	bool CanProcessDirectionalInput(bool bIsRepeat);
	void StepOptionIndex(int32 Direction);
	bool CanInteractWithRow() const;
	void RequestDisabledInteraction();
	bool HandleAnalogOptionInput(int32 Direction, float Magnitude, bool bSelectionOnly, double& LastInputTimeSeconds, int32& LastDirection, bool& bInputHeld, bool& bRepeatActive);
	void ResetAnalogOptionInput(double& LastInputTimeSeconds, int32& LastDirection, bool& bInputHeld, bool& bRepeatActive);
	void SetOptionListExpanded(bool bExpanded);
	void RefreshBoundTextWidgets();
	void BuildNumericOptions(int32 FractionalDigits, FText Suffix);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "Game UI|Focus|Value Row|Widgets")
	TObjectPtr<UTextBlock> Text_Label = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "Game UI|Focus|Value Row|Widgets")
	TObjectPtr<UTextBlock> Text_Value = nullptr;

	UPROPERTY(Transient)
	FGameplayTag RowIdentifier;

	UPROPERTY(Transient)
	FText Label;

	UPROPERTY(Transient)
	TArray<FText> Options;

	UPROPERTY(Transient)
	int32 CurrentIndex = 0;

	UPROPERTY(Transient)
	bool bIsOptionListExpanded = false;

	UPROPERTY(Transient)
	bool bHasInteractionFocus = false;

	UPROPERTY(Transient)
	bool bLastValueChangeWasUserInitiated = false;

	UPROPERTY(Transient)
	EGameUIFocusValueRowControlType ControlType = EGameUIFocusValueRowControlType::OptionList;

	UPROPERTY(Transient)
	bool bUsesSelectionBeforeApply = false;

	UPROPERTY(Transient)
	bool bIsValueRowEnabled = true;

	UPROPERTY(Transient)
	FText DisabledReason;

	UPROPERTY(Transient)
	bool bHasNumericRange = false;

	UPROPERTY(Transient)
	float NumericMinValue = 0.0f;

	UPROPERTY(Transient)
	float NumericMaxValue = 0.0f;

	UPROPERTY(Transient)
	float NumericStepSize = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Game UI|Focus|Value Row|Input")
	float DirectionalRepeatDelay = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Game UI|Focus|Value Row|Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogInputDeadZone = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Game UI|Focus|Value Row|Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogInputReleaseThreshold = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Game UI|Focus|Value Row|Input", meta = (ClampMin = "0.0"))
	float AnalogInitialRepeatDelay = 0.30f;

	UPROPERTY(EditAnywhere, Category = "Game UI|Focus|Value Row|Input", meta = (ClampMin = "0.0"))
	float AnalogRepeatInterval = 0.11f;

	double LastDirectionalInputTimeSeconds = -1000.0;
	double LastHorizontalAnalogInputTimeSeconds = -1000.0;
	int32 LastHorizontalAnalogDirection = 0;
	bool bHorizontalAnalogInputHeld = false;
	bool bHorizontalAnalogRepeatActive = false;
	double LastVerticalAnalogInputTimeSeconds = -1000.0;
	int32 LastVerticalAnalogDirection = 0;
	bool bVerticalAnalogInputHeld = false;
	bool bVerticalAnalogRepeatActive = false;
};
