#include "UI/Focus/GameUIFocusFeedback.h"

namespace GameUIFocusFeedback
{
	FSelectionChanged& OnSelectionChanged()
	{
		static FSelectionChanged Delegate;
		return Delegate;
	}

	FBackHandled& OnBackHandled()
	{
		static FBackHandled Delegate;
		return Delegate;
	}
}
