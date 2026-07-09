#pragma once

#include "InputCoreTypes.h"

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
}
