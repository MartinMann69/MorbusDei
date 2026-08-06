#include "Input/MD_InputDeviceSubsystem.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Focus/GameUIFocusInputDeviceTracker.h"
#include "Widgets/SViewport.h"

class FMDInputDevicePreProcessor final : public IInputProcessor
{
public:
	explicit FMDInputDevicePreProcessor(UMD_InputDeviceSubsystem* InSubsystem)
		: Subsystem(InSubsystem)
	{
	}

	virtual void Tick(
		const float DeltaTime,
		FSlateApplication& SlateApp,
		TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(
		FSlateApplication& SlateApp,
		const FKeyEvent& InKeyEvent) override
	{
		NotifyKey(InKeyEvent.GetKey());
		return false;
	}

	virtual bool HandleAnalogInputEvent(
		FSlateApplication& SlateApp,
		const FAnalogInputEvent& InAnalogInputEvent) override
	{
		UMD_InputDeviceSubsystem* InputDeviceSubsystem = Subsystem.Get();
		if (InputDeviceSubsystem
			&& InAnalogInputEvent.GetKey().IsGamepadKey()
			&& FMath::Abs(InAnalogInputEvent.GetAnalogValue())
				>= InputDeviceSubsystem->GamepadAnalogActivationThreshold)
		{
			InputDeviceSubsystem->NotifyInputDevice(EMDInputDeviceType::Gamepad);
		}
		return false;
	}

	virtual bool HandleMouseMoveEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override
	{
		UMD_InputDeviceSubsystem* InputDeviceSubsystem = Subsystem.Get();
		if (InputDeviceSubsystem
			&& IsPointerInsideGameViewport(SlateApp, MouseEvent)
			&& MouseEvent.GetCursorDelta().SizeSquared()
				>= FMath::Square(InputDeviceSubsystem->MouseMoveActivationThreshold))
		{
			InputDeviceSubsystem->NotifyInputDevice(EMDInputDeviceType::KeyboardMouse);
		}
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override
	{
		if (IsPointerInsideGameViewport(SlateApp, MouseEvent))
		{
			NotifyKeyboardMouse();
		}
		return false;
	}

	virtual bool HandleMouseButtonDoubleClickEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override
	{
		if (IsPointerInsideGameViewport(SlateApp, MouseEvent))
		{
			NotifyKeyboardMouse();
		}
		return false;
	}

	virtual bool HandleMouseWheelOrGestureEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& InWheelEvent,
		const FPointerEvent* InGestureEvent) override
	{
		if (IsPointerInsideGameViewport(SlateApp, InWheelEvent)
			&& (!FMath::IsNearlyZero(InWheelEvent.GetWheelDelta()) || InGestureEvent))
		{
			NotifyKeyboardMouse();
		}
		return false;
	}

	virtual const TCHAR* GetDebugName() const override
	{
		return TEXT("MD Input Device Detector");
	}

private:
	static bool IsPointerInsideGameViewport(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
	{
		const TSharedPtr<SViewport> GameViewport = SlateApp.GetGameViewport();
		return GameViewport.IsValid()
			&& GameViewport->GetCachedGeometry().IsUnderLocation(MouseEvent.GetScreenSpacePosition());
	}

	void NotifyKey(const FKey& Key) const
	{
		if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = Subsystem.Get())
		{
			InputDeviceSubsystem->NotifyInputDevice(
				Key.IsGamepadKey()
					? EMDInputDeviceType::Gamepad
					: EMDInputDeviceType::KeyboardMouse);
		}
	}

	void NotifyKeyboardMouse() const
	{
		if (UMD_InputDeviceSubsystem* InputDeviceSubsystem = Subsystem.Get())
		{
			InputDeviceSubsystem->NotifyInputDevice(EMDInputDeviceType::KeyboardMouse);
		}
	}

	TWeakObjectPtr<UMD_InputDeviceSubsystem> Subsystem;
};

void UMD_InputDeviceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveInputDevice = EMDInputDeviceType::KeyboardMouse;
	GameUIFocusInputDeviceTracker::SetPointerInputActive(true);

	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	InputPreProcessor = MakeShared<FMDInputDevicePreProcessor>(this);
	FSlateApplication::Get().RegisterInputPreProcessor(InputPreProcessor);
}

void UMD_InputDeviceSubsystem::Deinitialize()
{
	if (InputPreProcessor && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputPreProcessor);
	}
	InputPreProcessor.Reset();
	GameUIFocusInputDeviceTracker::SetPointerInputActive(true);

	Super::Deinitialize();
}

void UMD_InputDeviceSubsystem::SetGamepadAnalogActivationThreshold(const float NewThreshold)
{
	GamepadAnalogActivationThreshold = FMath::Clamp(NewThreshold, 0.0f, 1.0f);
}

void UMD_InputDeviceSubsystem::SetMouseMoveActivationThreshold(const float NewThreshold)
{
	MouseMoveActivationThreshold = FMath::Max(0.0f, NewThreshold);
}

void UMD_InputDeviceSubsystem::NotifyInputDevice(const EMDInputDeviceType NewDevice)
{
	GameUIFocusInputDeviceTracker::SetPointerInputActive(NewDevice != EMDInputDeviceType::Gamepad);

	if (ActiveInputDevice == NewDevice)
	{
		return;
	}

	const EMDInputDeviceType PreviousDevice = ActiveInputDevice;
	ActiveInputDevice = NewDevice;
	OnInputDeviceChanged.Broadcast(PreviousDevice, ActiveInputDevice);
}
