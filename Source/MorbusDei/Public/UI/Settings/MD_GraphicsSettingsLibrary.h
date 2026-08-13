#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MD_GraphicsSettingsLibrary.generated.h"

USTRUCT(BlueprintType)
struct MORBUSDEI_API FMDResolutionOptionSet
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	TArray<FIntPoint> Resolutions;

	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	TArray<FText> Labels;

	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	FIntPoint SelectedResolution = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	bool bIsValid = false;
};

UCLASS()
class MORBUSDEI_API UMDGraphicsSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Nautilus|Settings|Graphics")
	static FMDResolutionOptionSet BuildCurrentResolutionOptions();
};
