#pragma once

#include "CoreMinimal.h"
#include "GameUIFocusTypes.generated.h"

GAMEUIFOCUS_API DECLARE_LOG_CATEGORY_EXTERN(LogGameUIFocus, Log, All);

UENUM(BlueprintType)
enum class EGameUIFocusZone : uint8
{
	Navigation UMETA(DisplayName = "Navigation"),
	Content UMETA(DisplayName = "Content"),
	Modal UMETA(DisplayName = "Modal")
};
