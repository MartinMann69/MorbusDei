#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MD_InputDeviceSubsystem.generated.h"

class FMDInputDevicePreProcessor;
class IInputProcessor;

UENUM(BlueprintType)
enum class EMDInputDeviceType : uint8
{
	KeyboardMouse UMETA(DisplayName = "Keyboard / Mouse"),
	Gamepad UMETA(DisplayName = "Gamepad")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FMDInputDeviceTypeChanged,
	EMDInputDeviceType, PreviousDevice,
	EMDInputDeviceType, NewDevice);

/**
 * Blueprint-facing source for the last meaningful input device.
 *
 * The subsystem observes Slate input without consuming it. UI widgets can bind to
 * OnInputDeviceChanged and choose their own icon, animation and visibility behavior.
 */
UCLASS()
class MORBUSDEI_API UMD_InputDeviceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "MD|Input Device")
	FMDInputDeviceTypeChanged OnInputDeviceChanged;

	UFUNCTION(BlueprintPure, Category = "MD|Input Device")
	EMDInputDeviceType GetActiveInputDevice() const { return ActiveInputDevice; }

	UFUNCTION(BlueprintPure, Category = "MD|Input Device")
	bool IsUsingGamepad() const { return ActiveInputDevice == EMDInputDeviceType::Gamepad; }

	/** Ignores resting-stick and trigger noise below this value. */
	UFUNCTION(BlueprintCallable, Category = "MD|Input Device")
	void SetGamepadAnalogActivationThreshold(float NewThreshold);

	/** Ignores tiny cursor deltas caused by platform or viewport jitter. */
	UFUNCTION(BlueprintCallable, Category = "MD|Input Device")
	void SetMouseMoveActivationThreshold(float NewThreshold);

private:
	friend class FMDInputDevicePreProcessor;

	void NotifyInputDevice(EMDInputDeviceType NewDevice);

	TSharedPtr<IInputProcessor> InputPreProcessor;

	UPROPERTY(Transient)
	EMDInputDeviceType ActiveInputDevice = EMDInputDeviceType::KeyboardMouse;

	float GamepadAnalogActivationThreshold = 0.2f;
	float MouseMoveActivationThreshold = 0.5f;
};
