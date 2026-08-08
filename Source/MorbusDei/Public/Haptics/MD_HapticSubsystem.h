#pragma once

#include "CoreMinimal.h"
#include "Haptics/MD_HapticTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "MD_HapticSubsystem.generated.h"

class APlayerController;
class UForceFeedbackEffect;
class UMD_InputDeviceSubsystem;
class UGameUIFocusScreenWidgetBase;
enum class EMDInputDeviceType : uint8;

/** Local, cosmetic haptic playback with semantic priorities and accessibility settings. */
UCLASS()
class MORBUSDEI_API UMD_HapticSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Haptics")
	bool PlayHapticEvent(EMDHapticEvent Event);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Haptics")
	void StopHapticEvent(EMDHapticEvent Event);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Haptics")
	void StopAllHaptics();

	UFUNCTION(BlueprintCallable, Category = "MD|Haptics|Settings")
	void SetControllerVibrationEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "MD|Haptics|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	void SetControllerVibrationStrength(float Strength);

	UFUNCTION(BlueprintPure, Category = "MD|Haptics|Settings")
	bool IsControllerVibrationEnabled() const { return bControllerVibrationEnabled; }

	UFUNCTION(BlueprintPure, Category = "MD|Haptics|Settings")
	float GetControllerVibrationStrength() const { return ControllerVibrationStrength; }

private:
	UFUNCTION()
	void HandleInputDeviceChanged(EMDInputDeviceType PreviousDevice, EMDInputDeviceType NewDevice);

	void HandlePlayerControllerChanged(APlayerController* NewPlayerController);
	void HandleMenuSelectionChanged(UGameUIFocusScreenWidgetBase* Screen);
	void HandleMenuBackHandled(UGameUIFocusScreenWidgetBase* Screen);
	void PreloadEffects();
	UForceFeedbackEffect* CreateGeneratedPulse(EMDHapticEvent Event, const FMDHapticEventDefinition& Definition);
	void ApplyControllerSettings(APlayerController* PlayerController = nullptr) const;
	void CleanupExpiredPlayback(double CurrentRealTime);
	void StopLowerPriorityPlayback(EMDHapticPriority Priority);
	void LogBlocked(EMDHapticEvent Event, const TCHAR* Reason) const;

	APlayerController* GetLocalPlayerController() const;
	double GetCurrentRealTime() const;

	TWeakObjectPtr<UMD_InputDeviceSubsystem> InputDeviceSubsystem;
	FDelegateHandle PlayerControllerChangedHandle;
	FDelegateHandle MenuSelectionChangedHandle;
	FDelegateHandle MenuBackHandledHandle;

	UPROPERTY(Transient)
	TMap<EMDHapticEvent, TObjectPtr<UForceFeedbackEffect>> ResolvedEffects;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UForceFeedbackEffect>> ActiveEffects;

	TMap<FName, double> ActiveEffectEndTimes;
	TMap<FName, EMDHapticPriority> ActiveEffectPriorities;
	TMap<EMDHapticEvent, double> LastPlaybackTimes;
	TSet<EMDHapticEvent> MissingEffectWarnings;

	double ActiveHighPriorityEndTime = 0.0;
	bool bControllerVibrationEnabled = true;
	float ControllerVibrationStrength = 1.0f;
};
