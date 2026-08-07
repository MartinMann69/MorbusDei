#pragma once

#include "CoreMinimal.h"

enum class EGameUIFocusInputMode : uint8
{
	Pointer,
	KeyboardNavigation,
	GamepadNavigation
};

namespace GameUIFocusInputDeviceTracker
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FInputModeChanged, EGameUIFocusInputMode);
	DECLARE_MULTICAST_DELEGATE_OneParam(FPointerInputStateChanged, bool);

	GAMEUIFOCUS_API EGameUIFocusInputMode GetInputMode();
	GAMEUIFOCUS_API void SetInputMode(EGameUIFocusInputMode NewMode);
	GAMEUIFOCUS_API FInputModeChanged& OnInputModeChanged();

	/** Compatibility helpers for consumers that only distinguish pointer from navigation. */
	GAMEUIFOCUS_API bool IsPointerInputActive();
	GAMEUIFOCUS_API void SetPointerInputActive(bool bActive);
	GAMEUIFOCUS_API FPointerInputStateChanged& OnPointerInputStateChanged();
}
