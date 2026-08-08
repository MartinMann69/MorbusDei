#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MD_FoleyEventRelayComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FMDFoleyEventPlayed, FGameplayTag);

/**
 * Neutral, player-owned event bridge for Foley playback.
 * Audio reports semantic events here; feedback systems may subscribe without coupling to Foley assets.
 */
UCLASS(ClassGroup = (MD), meta = (BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_FoleyEventRelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_FoleyEventRelayComponent();

	/** Reports a Foley event after the audio playback path has successfully executed. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "MD|Feedback|Foley")
	void ReportFoleyEventPlayed(FGameplayTag EventTag);

	FMDFoleyEventPlayed& OnFoleyEventPlayed() { return FoleyEventPlayed; }

private:
	FMDFoleyEventPlayed FoleyEventPlayed;
};
