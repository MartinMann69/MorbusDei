#include "Modules/ModuleManager.h"

#include "UI/Focus/GameUIFocusInputDeviceTracker.h"

namespace GameUIFocusInputDeviceTracker
{
	EGameUIFocusInputMode InputMode = EGameUIFocusInputMode::Pointer;
	FInputModeChanged InputModeChanged;
	FPointerInputStateChanged PointerInputStateChanged;

	EGameUIFocusInputMode GetInputMode()
	{
		return InputMode;
	}

	void SetInputMode(const EGameUIFocusInputMode NewMode)
	{
		if (InputMode == NewMode)
		{
			return;
		}

		const bool bWasPointerInputActive = InputMode == EGameUIFocusInputMode::Pointer;
		InputMode = NewMode;
		InputModeChanged.Broadcast(InputMode);

		const bool bIsPointerInputActive = InputMode == EGameUIFocusInputMode::Pointer;
		if (bWasPointerInputActive != bIsPointerInputActive)
		{
			PointerInputStateChanged.Broadcast(bIsPointerInputActive);
		}
	}

	FInputModeChanged& OnInputModeChanged()
	{
		return InputModeChanged;
	}

	bool IsPointerInputActive()
	{
		return InputMode == EGameUIFocusInputMode::Pointer;
	}

	void SetPointerInputActive(const bool bActive)
	{
		SetInputMode(bActive
			? EGameUIFocusInputMode::Pointer
			: EGameUIFocusInputMode::GamepadNavigation);
	}

	FPointerInputStateChanged& OnPointerInputStateChanged()
	{
		return PointerInputStateChanged;
	}
}

IMPLEMENT_MODULE(FDefaultModuleImpl, GameUIFocus);
