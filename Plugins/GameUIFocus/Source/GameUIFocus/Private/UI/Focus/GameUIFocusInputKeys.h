#pragma once

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "Runtime/Launch/Resources/Version.h"

namespace GameUIFocusInputKeys
{
inline FKey GetVirtualAcceptKey()
{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
	return EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
#else
	return EKeys::Virtual_Accept;
#endif
}

inline FKey GetVirtualBackKey()
{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
	return EKeys::Virtual_Gamepad_Back.GetVirtualKey();
#else
	return EKeys::Virtual_Back;
#endif
}

/**
 * Respect the active Slate navigation configuration first. Some project input
 * paths still deliver the physical gamepad key instead of Unreal's virtual
 * Accept key, so use the conventional keys only when Slate has no rule.
 */
inline bool IsAcceptAction(const FKeyEvent& KeyEvent)
{
	const EUINavigationAction NavigationAction = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetNavigationActionFromKey(KeyEvent)
		: EUINavigationAction::Invalid;
	if (NavigationAction != EUINavigationAction::Invalid)
	{
		return NavigationAction == EUINavigationAction::Accept;
	}

	const FKey Key = KeyEvent.GetKey();
	return Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == GetVirtualAcceptKey()
		|| Key == EKeys::Gamepad_FaceButton_Bottom;
}

/** Same policy as IsAcceptAction, applied to Back/Cancel. */
inline bool IsBackAction(const FKeyEvent& KeyEvent)
{
	const EUINavigationAction NavigationAction = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetNavigationActionFromKey(KeyEvent)
		: EUINavigationAction::Invalid;
	if (NavigationAction != EUINavigationAction::Invalid)
	{
		return NavigationAction == EUINavigationAction::Back;
	}

	const FKey Key = KeyEvent.GetKey();
	return Key == EKeys::Escape
		|| Key == GetVirtualBackKey()
		|| Key == EKeys::Gamepad_FaceButton_Right;
}
}
