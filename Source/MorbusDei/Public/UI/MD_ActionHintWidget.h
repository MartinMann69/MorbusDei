#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Blueprint/UserWidget.h"
#include "MD_ActionHintWidget.generated.h"

class USoundBase;
class UTextBlock;
enum class EMDInputDeviceType : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMDActionHintPresentationChanged, bool, bUsingGamepad);

/** Reusable, device-aware action hint presentation with lightweight feedback. */
UCLASS(Abstract, Blueprintable)
class MORBUSDEI_API UMD_ActionHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMD_ActionHintWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "MD|UI|Action Hint")
	void ConfigureHint(const FText& InKeyboardKeyText, const FText& InGamepadKeyText, const FText& InActionText);

	UFUNCTION(BlueprintCallable, Category = "MD|UI|Action Hint")
	void SetActionText(const FText& InActionText);

	UFUNCTION(BlueprintCallable, Category = "MD|UI|Action Hint")
	void PlayHandledFeedback();

	UFUNCTION(BlueprintPure, Category = "MD|UI|Action Hint")
	bool IsUsingGamepad() const;

	UPROPERTY(BlueprintAssignable, Category = "MD|UI|Action Hint")
	FMDActionHintPresentationChanged OnPresentationChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Kept compatible with the names already used by WBP_ActionHint. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|UI|Action Hint")
	TObjectPtr<UTextBlock> BackKeyText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|UI|Action Hint")
	TObjectPtr<UTextBlock> BackActionText = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Text")
	FText KeyboardKeyText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Text")
	FText GamepadKeyText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Text")
	FText ActionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Feedback")
	TObjectPtr<USoundBase> HandledSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Feedback", meta = (ClampMin = "0.0"))
	float HandledSoundVolume = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Feedback", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float HandledSoundPitch = 0.92f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Feedback", meta = (ClampMin = "0.0"))
	float PulseDuration = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|UI|Action Hint|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PulseStartOpacity = 0.55f;

private:
	UFUNCTION()
	void HandleInputDeviceChanged(EMDInputDeviceType PreviousDevice, EMDInputDeviceType NewDevice);

	void RefreshPresentation();
	bool TickPulse(float DeltaTime);
	void StopPulse(bool bRestoreOpacity);

	FTSTicker::FDelegateHandle PulseTickerHandle;
	float PulseElapsed = 0.0f;
};
