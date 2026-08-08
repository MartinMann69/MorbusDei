#include "Haptics/MD_HapticDeveloperSettings.h"

#include "GameFramework/ForceFeedbackEffect.h"

namespace
{
	TSoftObjectPtr<UForceFeedbackEffect> MakeHapticEffectReference(const TCHAR* AssetPath)
	{
		return TSoftObjectPtr<UForceFeedbackEffect>(FSoftObjectPath(AssetPath));
	}

	void AddFoleyEventTag(FGameplayTagContainer& Container, const TCHAR* TagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (Tag.IsValid())
		{
			Container.AddTag(Tag);
		}
	}
}

UMD_HapticDeveloperSettings::UMD_HapticDeveloperSettings()
{
	Footstep.Effect = MakeHapticEffectReference(
		TEXT("/Game/MorbusDei/Feedback/Haptics/FF_MD_Footstep.FF_MD_Footstep"));
	Footstep.PlaybackTag = TEXT("Haptic.Footstep");
	Footstep.Priority = EMDHapticPriority::Low;
	Footstep.MinimumReplayInterval = 0.08f;

	AddFoleyEventTag(FootstepFoleyEventTags, TEXT("Foley.Event.Walk"));
	AddFoleyEventTag(FootstepFoleyEventTags, TEXT("Foley.Event.WalkBackwds"));
	AddFoleyEventTag(FootstepFoleyEventTags, TEXT("Foley.Event.Run"));
	AddFoleyEventTag(FootstepFoleyEventTags, TEXT("Foley.Event.RunBackwds"));
	AddFoleyEventTag(FootstepFoleyEventTags, TEXT("Foley.Event.RunStrafe"));

	InteractionFocus.PlaybackTag = TEXT("Haptic.Interaction.Focus");
	InteractionFocus.Priority = EMDHapticPriority::Low;
	InteractionFocus.MinimumReplayInterval = 0.20f;
	InteractionFocus.PulseDuration = 0.06f;
	InteractionFocus.LargeMotorStrength = 0.0f;
	InteractionFocus.SmallMotorStrength = 0.18f;

	Interaction.Effect = MakeHapticEffectReference(
		TEXT("/Game/MorbusDei/Feedback/Haptics/FF_MD_Interaction.FF_MD_Interaction"));
	Interaction.PlaybackTag = TEXT("Haptic.Interaction");
	Interaction.Priority = EMDHapticPriority::Normal;
	Interaction.MinimumReplayInterval = 0.12f;

	MenuSelection.PlaybackTag = TEXT("Haptic.Menu.Selection");
	MenuSelection.Priority = EMDHapticPriority::Low;
	MenuSelection.MinimumReplayInterval = 0.035f;
	MenuSelection.PulseDuration = 0.035f;
	MenuSelection.LargeMotorStrength = 0.015f;
	MenuSelection.SmallMotorStrength = 0.055f;

	MenuBack.PlaybackTag = TEXT("Haptic.Menu.Back");
	MenuBack.Priority = EMDHapticPriority::Low;
	MenuBack.MinimumReplayInterval = 0.08f;
	MenuBack.PulseDuration = 0.045f;
	MenuBack.LargeMotorStrength = 0.025f;
	MenuBack.SmallMotorStrength = 0.035f;

	StoryLight.Effect = MakeHapticEffectReference(
		TEXT("/Game/MorbusDei/Feedback/Haptics/FF_MD_Event_Light.FF_MD_Event_Light"));
	StoryLight.PlaybackTag = TEXT("Haptic.Story");
	StoryLight.Priority = EMDHapticPriority::High;
	StoryLight.MinimumReplayInterval = 0.25f;

	StoryHeavy.Effect = MakeHapticEffectReference(
		TEXT("/Game/MorbusDei/Feedback/Haptics/FF_MD_Event_Heavy.FF_MD_Event_Heavy"));
	StoryHeavy.PlaybackTag = TEXT("Haptic.Story");
	StoryHeavy.Priority = EMDHapticPriority::High;
	StoryHeavy.MinimumReplayInterval = 0.50f;
}

bool UMD_HapticDeveloperSettings::IsFootstepFoleyEvent(const FGameplayTag EventTag) const
{
	return EventTag.IsValid() && FootstepFoleyEventTags.HasTagExact(EventTag);
}

const FMDHapticEventDefinition& UMD_HapticDeveloperSettings::GetDefinition(
	const EMDHapticEvent Event) const
{
	switch (Event)
	{
	case EMDHapticEvent::InteractionFocus:
		return InteractionFocus;
	case EMDHapticEvent::MenuSelection:
		return MenuSelection;
	case EMDHapticEvent::MenuBack:
		return MenuBack;
	case EMDHapticEvent::Interaction:
		return Interaction;
	case EMDHapticEvent::StoryLight:
		return StoryLight;
	case EMDHapticEvent::StoryHeavy:
		return StoryHeavy;
	case EMDHapticEvent::Footstep:
	default:
		return Footstep;
	}
}
