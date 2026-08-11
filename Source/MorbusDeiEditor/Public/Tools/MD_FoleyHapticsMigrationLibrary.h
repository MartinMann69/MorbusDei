#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MD_FoleyHapticsMigrationLibrary.generated.h"

/** Reapplies the Morbus Dei Foley relay seam after imported Foley package updates. */
UCLASS()
class MORBUSDEIEDITOR_API UMD_FoleyHapticsMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MD|Tools|Foley")
	static bool IntegrateFoleyNotify();

	UFUNCTION(BlueprintPure, Category = "MD|Tools|Foley")
	static bool IsFoleyNotifyIntegrated();
};
