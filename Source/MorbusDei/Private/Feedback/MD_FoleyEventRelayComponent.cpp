#include "Feedback/MD_FoleyEventRelayComponent.h"

UMD_FoleyEventRelayComponent::UMD_FoleyEventRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMD_FoleyEventRelayComponent::ReportFoleyEventPlayed(const FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	FoleyEventPlayed.Broadcast(EventTag);
}
