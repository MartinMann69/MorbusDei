#pragma once

#include "CoreMinimal.h"
#include "Haptics/MD_HapticTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MD_HapticBlueprintLibrary.generated.h"

/** Keeps Blueprint callers independent of controller and subsystem lookup details. */
UCLASS()
class MORBUSDEI_API UMD_HapticBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Haptics",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Play MD Haptic Event"))
	static bool PlayMDHapticEvent(const UObject* WorldContextObject, EMDHapticEvent Event);
};
