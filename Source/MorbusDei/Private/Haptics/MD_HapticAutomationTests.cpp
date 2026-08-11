#if WITH_DEV_AUTOMATION_TESTS

#include "Feedback/MD_FoleyEventRelayComponent.h"
#include "Haptics/MD_GameUserSettings.h"
#include "Haptics/MD_HapticDeveloperSettings.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMDFoleyEventRelayTest,
	"Nautilus.Haptics.FoleyEventRelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMDFoleyEventRelayTest::RunTest(const FString& Parameters)
{
	UMD_FoleyEventRelayComponent* Relay = NewObject<UMD_FoleyEventRelayComponent>();
	int32 BroadcastCount = 0;
	FGameplayTag ReportedTag;
	Relay->OnFoleyEventPlayed().AddLambda(
		[&BroadcastCount, &ReportedTag](const FGameplayTag EventTag)
		{
			++BroadcastCount;
			ReportedTag = EventTag;
		});

	Relay->ReportFoleyEventPlayed(FGameplayTag());
	TestEqual(TEXT("Invalid Foley tags do not broadcast"), BroadcastCount, 0);

	const FGameplayTag WalkTag = FGameplayTag::RequestGameplayTag(TEXT("Foley.Event.Walk"));
	Relay->ReportFoleyEventPlayed(WalkTag);
	TestEqual(TEXT("A valid Foley event broadcasts exactly once"), BroadcastCount, 1);
	TestEqual(TEXT("The relay preserves the semantic Foley tag"), ReportedTag, WalkTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMDHapticConfigurationTest,
	"Nautilus.Haptics.Configuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMDHapticConfigurationTest::RunTest(const FString& Parameters)
{
	const UMD_HapticDeveloperSettings* Settings = GetDefault<UMD_HapticDeveloperSettings>();
	TestEqual(
		TEXT("Footsteps are low priority"),
		Settings->GetDefinition(EMDHapticEvent::Footstep).Priority,
		EMDHapticPriority::Low);
	TestTrue(
		TEXT("Footstep replay guard allows the observed running cadence"),
		FMath::IsNearlyEqual(
			Settings->GetDefinition(EMDHapticEvent::Footstep).MinimumReplayInterval,
			0.08f));

	const FMDHapticEventDefinition& InteractionFocus =
		Settings->GetDefinition(EMDHapticEvent::InteractionFocus);
	TestEqual(
		TEXT("Interaction focus is low priority"),
		InteractionFocus.Priority,
		EMDHapticPriority::Low);
	TestTrue(
		TEXT("Interaction focus uses a crisp generated small-motor pulse"),
		InteractionFocus.Effect.IsNull()
		&& FMath::IsNearlyEqual(InteractionFocus.PulseDuration, 0.06f)
		&& FMath::IsNearlyZero(InteractionFocus.LargeMotorStrength)
		&& FMath::IsNearlyEqual(InteractionFocus.SmallMotorStrength, 0.18f));
	TestTrue(
		TEXT("Interaction focus has a trace-jitter replay guard"),
		FMath::IsNearlyEqual(InteractionFocus.MinimumReplayInterval, 0.20f));
	TestNotEqual(
		TEXT("Interaction focus cannot replace the footstep effect"),
		InteractionFocus.PlaybackTag,
		Settings->GetDefinition(EMDHapticEvent::Footstep).PlaybackTag);

	const FName ExpectedFootstepTagNames[] =
	{
		TEXT("Foley.Event.Walk"),
		TEXT("Foley.Event.WalkBackwds"),
		TEXT("Foley.Event.Run"),
		TEXT("Foley.Event.RunBackwds"),
		TEXT("Foley.Event.RunStrafe")
	};
	for (const FName TagName : ExpectedFootstepTagNames)
	{
		TestTrue(
			*FString::Printf(TEXT("%s is routed as a footstep"), *TagName.ToString()),
			Settings->IsFootstepFoleyEvent(FGameplayTag::RequestGameplayTag(TagName)));
	}

	const FName ExcludedFoleyTagNames[] =
	{
		TEXT("Foley.Event.Jump"),
		TEXT("Foley.Event.Land"),
		TEXT("Foley.Event.Scuff"),
		TEXT("Foley.Event.Handplant")
	};
	for (const FName TagName : ExcludedFoleyTagNames)
	{
		TestFalse(
			*FString::Printf(TEXT("%s is not routed as a regular footstep"), *TagName.ToString()),
			Settings->IsFootstepFoleyEvent(FGameplayTag::RequestGameplayTag(TagName)));
	}
	TestEqual(
		TEXT("Interaction is normal priority"),
		Settings->GetDefinition(EMDHapticEvent::Interaction).Priority,
		EMDHapticPriority::Normal);
	TestEqual(
		TEXT("Menu selection is low priority"),
		Settings->GetDefinition(EMDHapticEvent::MenuSelection).Priority,
		EMDHapticPriority::Low);
	TestTrue(
		TEXT("Menu selection stays a subtle high-frequency tick"),
		Settings->GetDefinition(EMDHapticEvent::MenuSelection).PulseDuration <= 0.05f
		&& Settings->GetDefinition(EMDHapticEvent::MenuSelection).LargeMotorStrength <= 0.025f
		&& Settings->GetDefinition(EMDHapticEvent::MenuSelection).SmallMotorStrength <= 0.075f);
	TestTrue(
		TEXT("Menu Back stays a subtle pulse"),
		Settings->GetDefinition(EMDHapticEvent::MenuBack).PulseDuration <= 0.06f
		&& Settings->GetDefinition(EMDHapticEvent::MenuBack).LargeMotorStrength <= 0.04f
		&& Settings->GetDefinition(EMDHapticEvent::MenuBack).SmallMotorStrength <= 0.05f);
	TestEqual(
		TEXT("Heavy story feedback is high priority"),
		Settings->GetDefinition(EMDHapticEvent::StoryHeavy).Priority,
		EMDHapticPriority::High);
	TestEqual(
		TEXT("Story variants intentionally share their replacement tag"),
		Settings->GetDefinition(EMDHapticEvent::StoryLight).PlaybackTag,
		Settings->GetDefinition(EMDHapticEvent::StoryHeavy).PlaybackTag);

	UMD_GameUserSettings* UserSettings = NewObject<UMD_GameUserSettings>();
	UserSettings->SetControllerVibrationStrength(2.0f);
	TestEqual(TEXT("Strength clamps above one"), UserSettings->GetControllerVibrationStrength(), 1.0f);
	UserSettings->SetControllerVibrationStrength(-1.0f);
	TestEqual(TEXT("Strength clamps below zero"), UserSettings->GetControllerVibrationStrength(), 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMDHapticAssetsTest,
	"Nautilus.Haptics.Assets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMDHapticAssetsTest::RunTest(const FString& Parameters)
{
	const UMD_HapticDeveloperSettings* Settings = GetDefault<UMD_HapticDeveloperSettings>();

	struct FExpectedHapticAsset
	{
		EMDHapticEvent Event;
	};

	const FExpectedHapticAsset ExpectedAssets[] =
	{
		{ EMDHapticEvent::Footstep },
		{ EMDHapticEvent::Interaction },
		{ EMDHapticEvent::StoryLight },
		{ EMDHapticEvent::StoryHeavy }
	};

	for (const FExpectedHapticAsset& Expected : ExpectedAssets)
	{
		UForceFeedbackEffect* Effect = Settings->GetDefinition(Expected.Event).Effect.LoadSynchronous();
		if (!TestNotNull(TEXT("Configured force-feedback asset loads"), Effect))
		{
			continue;
		}

		TestTrue(TEXT("Asset has a non-zero one-shot duration"), Effect->Duration > 0.0f);
		if (!TestEqual(TEXT("Asset has separate large and small motor curves"), Effect->ChannelDetails.Num(), 2))
		{
			continue;
		}

		const FForceFeedbackChannelDetails& LargeChannel = Effect->ChannelDetails[0];
		const FForceFeedbackChannelDetails& SmallChannel = Effect->ChannelDetails[1];
		TestTrue(TEXT("Large curve targets both large motors"),
			LargeChannel.bAffectsLeftLarge && LargeChannel.bAffectsRightLarge
			&& !LargeChannel.bAffectsLeftSmall && !LargeChannel.bAffectsRightSmall);
		TestTrue(TEXT("Small curve targets both small motors"),
			SmallChannel.bAffectsLeftSmall && SmallChannel.bAffectsRightSmall
			&& !SmallChannel.bAffectsLeftLarge && !SmallChannel.bAffectsRightLarge);

		const TArray<FRichCurveKey>& LargeKeys =
			LargeChannel.Curve.GetRichCurveConst()->GetConstRefOfKeys();
		const TArray<FRichCurveKey>& SmallKeys =
			SmallChannel.Curve.GetRichCurveConst()->GetConstRefOfKeys();
		TestTrue(TEXT("Large motor curve has an authored envelope"), LargeKeys.Num() >= 2);
		TestTrue(TEXT("Small motor curve has an authored envelope"), SmallKeys.Num() >= 2);

		bool bHasPositiveLargeValue = false;
		for (const FRichCurveKey& Key : LargeKeys)
		{
			bHasPositiveLargeValue |= Key.Value > 0.0f;
		}
		bool bHasPositiveSmallValue = false;
		for (const FRichCurveKey& Key : SmallKeys)
		{
			bHasPositiveSmallValue |= Key.Value > 0.0f;
		}
		TestTrue(TEXT("At least one motor envelope contains feedback"),
			bHasPositiveLargeValue || bHasPositiveSmallValue);
	}

	return true;
}

#endif
