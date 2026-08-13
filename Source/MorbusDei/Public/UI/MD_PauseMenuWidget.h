#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"
#include "MD_PauseMenuWidget.generated.h"

class USoundBase;
class UTextBlock;
class UWidgetAnimation;
class UMD_ActionHintWidget;
enum class EMDInputDeviceType : uint8;

class UMD_PauseMenuWidget;
DECLARE_MULTICAST_DELEGATE_OneParam(FMDPauseMenuCloseTransitionFinished, UMD_PauseMenuWidget*);

UCLASS(Abstract, Blueprintable)
class MORBUSDEI_API UMD_PauseMenuWidget : public UGameUIFocusScreenWidgetBase
{
	GENERATED_BODY()

public:
	UMD_PauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	bool BeginCloseTransition();

	FMDPauseMenuCloseTransitionFinished& OnCloseTransitionFinished() { return CloseTransitionFinished; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool HandleRootBackAction_Implementation() override;

	/** Optional animation; native easing is the fallback. */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PauseTransition = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	TObjectPtr<UTextBlock> BackKeyText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	TObjectPtr<UTextBlock> BackActionText = nullptr;

	/** Preferred binding; raw text widgets are the legacy fallback. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	TObjectPtr<UMD_ActionHintWidget> ActionHint = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Transition", meta = (ClampMin = "0.0"))
	float OpenTransitionDuration = 0.14f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Transition", meta = (ClampMin = "0.0"))
	float CloseTransitionDuration = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Transition", meta = (ClampMin = "0.0"))
	float TransitionOffset = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Feedback")
	TObjectPtr<USoundBase> BackSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Feedback", meta = (ClampMin = "0.0"))
	float BackSoundVolume = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Feedback", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float BackSoundPitch = 0.92f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	FText KeyboardBackKeyText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	FText GamepadBackKeyText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	FText BackLabelText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Pause Menu|Hint")
	FText ResumeLabelText;

private:
	UFUNCTION()
	void HandlePauseTransitionFinished();

	UFUNCTION()
	void HandleInputDeviceChanged(EMDInputDeviceType PreviousDevice, EMDInputDeviceType NewDevice);

	UFUNCTION()
	void HandlePauseFocusZoneChanged(EGameUIFocusZone PreviousZone, EGameUIFocusZone NewZone);

	UFUNCTION()
	void HandleBackActionFeedback(EGameUIFocusZone SourceZone);

	void UpdateBackHint();
	void StartNativeTransition(bool bClosing);
	bool TickNativeTransition(float DeltaTime);
	void StopNativeTransition();
	void StartHintPulse();
	bool TickHintPulse(float DeltaTime);
	void StopHintPulse();
	void FinishCloseTransition();
	bool IsOpeningInputGuardActive() const;

	FMDPauseMenuCloseTransitionFinished CloseTransitionFinished;
	FTSTicker::FDelegateHandle NativeTransitionTickerHandle;
	FTSTicker::FDelegateHandle HintPulseTickerHandle;
	FVector2D BaseRenderTranslation = FVector2D::ZeroVector;
	FVector2D TransitionStartTranslation = FVector2D::ZeroVector;
	float TransitionStartOpacity = 1.0f;
	float TransitionElapsed = 0.0f;
	float HintPulseElapsed = 0.0f;
	double InputUnlockTimeSeconds = 0.0;
	bool bCloseRequested = false;
	bool bNativeTransitionClosing = false;
};
