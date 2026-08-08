#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MD_FoleyEventBlueprintLibrary.generated.h"

class UAudioComponent;
class USkeletalMeshComponent;

/** Small Blueprint facade used by imported Foley notifies to report neutral semantic events. */
UCLASS()
class MORBUSDEI_API UMD_FoleyEventBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Reports a Foley event through the relay owned by MeshComp's actor. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Feedback|Foley")
	static bool ReportFoleyEventPlayed(
		USkeletalMeshComponent* MeshComp,
		FGameplayTag EventTag);

	/** Reports only when the normal Foley component returned a valid spawned audio component. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Feedback|Foley")
	static bool ReportFoleyEventPlayedFromAudioComponent(
		USkeletalMeshComponent* MeshComp,
		FGameplayTag EventTag,
		UAudioComponent* PlaybackComponent);
};
