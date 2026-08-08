#include "Feedback/MD_FoleyEventBlueprintLibrary.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Feedback/MD_FoleyEventRelayComponent.h"

bool UMD_FoleyEventBlueprintLibrary::ReportFoleyEventPlayed(
	USkeletalMeshComponent* MeshComp,
	const FGameplayTag EventTag)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UMD_FoleyEventRelayComponent* Relay = Owner
		? Owner->FindComponentByClass<UMD_FoleyEventRelayComponent>()
		: nullptr;
	if (!Relay || !EventTag.IsValid())
	{
		return false;
	}

	Relay->ReportFoleyEventPlayed(EventTag);
	return true;
}

bool UMD_FoleyEventBlueprintLibrary::ReportFoleyEventPlayedFromAudioComponent(
	USkeletalMeshComponent* MeshComp,
	const FGameplayTag EventTag,
	UAudioComponent* PlaybackComponent)
{
	return IsValid(PlaybackComponent) && ReportFoleyEventPlayed(MeshComp, EventTag);
}
