#include "Modules/ModuleManager.h"

#include "UI/Focus/GameUIFocusInputDeviceTracker.h"

namespace GameUIFocusInputDeviceTracker
{
	bool bPointerInputActive = true;
	FPointerInputStateChanged PointerInputStateChanged;

	bool IsPointerInputActive()
	{
		return bPointerInputActive;
	}

	void SetPointerInputActive(const bool bActive)
	{
		if (bPointerInputActive == bActive)
		{
			return;
		}

		bPointerInputActive = bActive;
		PointerInputStateChanged.Broadcast(bPointerInputActive);
	}

	FPointerInputStateChanged& OnPointerInputStateChanged()
	{
		return PointerInputStateChanged;
	}
}

IMPLEMENT_MODULE(FDefaultModuleImpl, GameUIFocus);
