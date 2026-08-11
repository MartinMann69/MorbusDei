#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Haptics/MD_HapticTypes.h"
#include "MD_HapticDeveloperSettings.generated.h"

/** Project-wide semantic haptic definitions. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Haptics"))
class MORBUSDEI_API UMD_HapticDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMD_HapticDeveloperSettings();

	virtual FName GetCategoryName() const override { return TEXT("Morbus Dei"); }
	virtual FName GetSectionName() const override { return TEXT("Haptics"); }

	const FMDHapticEventDefinition& GetDefinition(EMDHapticEvent Event) const;
	bool IsFootstepFoleyEvent(FGameplayTag EventTag) const;

	/** Foley events that produce a regular locomotion footstep impulse. Exact matches only. */
	UPROPERTY(EditAnywhere, Config, Category = "Footsteps")
	FGameplayTagContainer FootstepFoleyEventTags;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition Footstep;

	/** Crisp target-acquired pulse, intentionally distinct from the low-frequency footstep. */
	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition InteractionFocus;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition Interaction;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition MenuSelection;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition MenuBack;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition StoryLight;

	UPROPERTY(EditAnywhere, Config, Category = "Events")
	FMDHapticEventDefinition StoryHeavy;
};
