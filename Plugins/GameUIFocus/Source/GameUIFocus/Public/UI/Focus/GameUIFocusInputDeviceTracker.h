#pragma once

#include "CoreMinimal.h"

namespace GameUIFocusInputDeviceTracker
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FPointerInputStateChanged, bool);

	GAMEUIFOCUS_API bool IsPointerInputActive();
	GAMEUIFOCUS_API void SetPointerInputActive(bool bActive);
	GAMEUIFOCUS_API FPointerInputStateChanged& OnPointerInputStateChanged();
}
