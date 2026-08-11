#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MD_GraphicsSettingsLibrary.generated.h"

/**
 * A display-ready resolution list whose labels and values always share the same index.
 */
USTRUCT(BlueprintType)
struct MORBUSDEI_API FMDResolutionOptionSet
{
	GENERATED_BODY()

	/** Platform-reported resolutions that satisfy Nautilus' supported display policy. */
	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	TArray<FIntPoint> Resolutions;

	/** Display labels matching Resolutions one-to-one. */
	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	TArray<FText> Labels;

	/** Index of SelectedResolution in both arrays. INDEX_NONE only when no valid option exists. */
	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	int32 SelectedIndex = INDEX_NONE;

	/** Currently confirmed supported resolution, or the closest supported platform fallback. */
	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	FIntPoint SelectedResolution = FIntPoint::ZeroValue;

	/** True when the arrays and SelectedIndex form a usable option set. */
	UPROPERTY(BlueprintReadOnly, Category = "Nautilus|Settings|Graphics")
	bool bIsValid = false;
};

/**
 * Stateless helpers for presenting graphics settings in UI.
 * UGameUserSettings remains the owner of loading, applying, confirming, and saving.
 */
UCLASS()
class MORBUSDEI_API UMDGraphicsSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Builds a stable, de-duplicated list of supported platform resolutions and selects
	 * the confirmed video mode. Unsupported or unavailable selections use the closest
	 * reported fallback without applying it until the user confirms the settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nautilus|Settings|Graphics")
	static FMDResolutionOptionSet BuildCurrentResolutionOptions();
};
