#pragma once

#include "CoreMinimal.h"

class UGameUIFocusScreenWidgetBase;

/** Project-agnostic semantic events for audio, haptics, and other UI feedback systems. */
namespace GameUIFocusFeedback
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FSelectionChanged, UGameUIFocusScreenWidgetBase*);
	DECLARE_MULTICAST_DELEGATE_OneParam(FBackHandled, UGameUIFocusScreenWidgetBase*);

	GAMEUIFOCUS_API FSelectionChanged& OnSelectionChanged();
	GAMEUIFOCUS_API FBackHandled& OnBackHandled();
}
