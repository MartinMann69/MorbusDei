#include "Haptics/MD_HapticSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "GameFramework/ForceFeedbackParameters.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Haptics/MD_GameUserSettings.h"
#include "Haptics/MD_HapticDeveloperSettings.h"
#include "Input/MD_InputDeviceSubsystem.h"
#include "UI/Focus/GameUIFocusFeedback.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"

DEFINE_LOG_CATEGORY(LogMDHaptics);

namespace MDHaptics
{
	static TAutoConsoleVariable<int32> CVarDebug(
		TEXT("md.Haptics.Debug"),
		0,
		TEXT("Logs semantic haptic playback and blocker reasons. 0: off, 1: on."),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarEnabled(
		TEXT("md.Haptics.Enabled"),
		1,
		TEXT("Global development override for Morbus Dei haptics. 0: off, 1: on."),
		ECVF_Default);

	const TCHAR* GetEventName(const EMDHapticEvent Event)
	{
		switch (Event)
		{
		case EMDHapticEvent::Footstep:
			return TEXT("Footstep");
		case EMDHapticEvent::InteractionFocus:
			return TEXT("InteractionFocus");
		case EMDHapticEvent::Interaction:
			return TEXT("Interaction");
		case EMDHapticEvent::MenuSelection:
			return TEXT("MenuSelection");
		case EMDHapticEvent::MenuBack:
			return TEXT("MenuBack");
		case EMDHapticEvent::StoryLight:
			return TEXT("StoryLight");
		case EMDHapticEvent::StoryHeavy:
			return TEXT("StoryHeavy");
		default:
			return TEXT("Unknown");
		}
	}

	EMDHapticEvent ParseEvent(const FString& Value, bool& bOutValid)
	{
		bOutValid = true;
		if (Value.Equals(TEXT("Footstep"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::Footstep;
		}
		if (Value.Equals(TEXT("InteractionFocus"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::InteractionFocus;
		}
		if (Value.Equals(TEXT("Interaction"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::Interaction;
		}
		if (Value.Equals(TEXT("MenuSelection"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::MenuSelection;
		}
		if (Value.Equals(TEXT("MenuBack"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::MenuBack;
		}
		if (Value.Equals(TEXT("StoryLight"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::StoryLight;
		}
		if (Value.Equals(TEXT("StoryHeavy"), ESearchCase::IgnoreCase))
		{
			return EMDHapticEvent::StoryHeavy;
		}

		bOutValid = false;
		return EMDHapticEvent::Footstep;
	}

	void TestHapticEvent(const TArray<FString>& Arguments, UWorld* World)
	{
		if (!World || Arguments.IsEmpty())
		{
			UE_LOG(LogMDHaptics, Display,
				TEXT("Usage: md.Haptics.Test Footstep|InteractionFocus|Interaction|MenuSelection|MenuBack|StoryLight|StoryHeavy"));
			return;
		}

		bool bValidEvent = false;
		const EMDHapticEvent Event = ParseEvent(Arguments[0], bValidEvent);
		APlayerController* PlayerController = World->GetFirstPlayerController();
		ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
		UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
			? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
			: nullptr;

		if (!bValidEvent || !HapticSubsystem)
		{
			UE_LOG(LogMDHaptics, Display,
				TEXT("Haptic test failed: invalid event or no local player."));
			return;
		}

		HapticSubsystem->PlayHapticEvent(Event);
	}

	static FAutoConsoleCommandWithWorldAndArgs TestCommand(
		TEXT("md.Haptics.Test"),
		TEXT("Plays a semantic haptic event on the first local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestHapticEvent));
}

void UMD_HapticSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PreloadEffects();

	if (const UMD_GameUserSettings* UserSettings = UMD_GameUserSettings::Get())
	{
		bControllerVibrationEnabled = UserSettings->IsControllerVibrationEnabled();
		ControllerVibrationStrength = UserSettings->GetControllerVibrationStrength();
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		PlayerControllerChangedHandle = LocalPlayer->OnPlayerControllerChanged().AddUObject(
			this,
			&UMD_HapticSubsystem::HandlePlayerControllerChanged);

		if (UGameInstance* GameInstance = LocalPlayer->GetGameInstance())
		{
			InputDeviceSubsystem = GameInstance->GetSubsystem<UMD_InputDeviceSubsystem>();
		}
	}

	if (InputDeviceSubsystem.IsValid())
	{
		InputDeviceSubsystem->OnInputDeviceChanged.AddUniqueDynamic(
			this,
			&UMD_HapticSubsystem::HandleInputDeviceChanged);
	}

	MenuSelectionChangedHandle = GameUIFocusFeedback::OnSelectionChanged().AddUObject(
		this,
		&UMD_HapticSubsystem::HandleMenuSelectionChanged);
	MenuBackHandledHandle = GameUIFocusFeedback::OnBackHandled().AddUObject(
		this,
		&UMD_HapticSubsystem::HandleMenuBackHandled);

	ApplyControllerSettings();
}

void UMD_HapticSubsystem::Deinitialize()
{
	StopAllHaptics();
	ResolvedEffects.Reset();

	GameUIFocusFeedback::OnSelectionChanged().Remove(MenuSelectionChangedHandle);
	GameUIFocusFeedback::OnBackHandled().Remove(MenuBackHandledHandle);
	MenuSelectionChangedHandle.Reset();
	MenuBackHandledHandle.Reset();

	if (InputDeviceSubsystem.IsValid())
	{
		InputDeviceSubsystem->OnInputDeviceChanged.RemoveDynamic(
			this,
			&UMD_HapticSubsystem::HandleInputDeviceChanged);
	}
	InputDeviceSubsystem.Reset();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer(); PlayerControllerChangedHandle.IsValid())
	{
		LocalPlayer->OnPlayerControllerChanged().Remove(PlayerControllerChangedHandle);
		PlayerControllerChangedHandle.Reset();
	}

	Super::Deinitialize();
}

bool UMD_HapticSubsystem::PlayHapticEvent(const EMDHapticEvent Event)
{
	if (MDHaptics::CVarEnabled.GetValueOnGameThread() == 0)
	{
		LogBlocked(Event, TEXT("global override disabled"));
		return false;
	}
	if (!bControllerVibrationEnabled || ControllerVibrationStrength <= UE_SMALL_NUMBER)
	{
		LogBlocked(Event, TEXT("user setting disabled or strength is zero"));
		return false;
	}
	if (!InputDeviceSubsystem.IsValid() || !InputDeviceSubsystem->IsUsingGamepad())
	{
		LogBlocked(Event, TEXT("active input device is keyboard/mouse"));
		return false;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		LogBlocked(Event, TEXT("no local player controller"));
		return false;
	}

	const UMD_HapticDeveloperSettings* Settings = GetDefault<UMD_HapticDeveloperSettings>();
	const FMDHapticEventDefinition& Definition = Settings->GetDefinition(Event);
	UForceFeedbackEffect* Effect = ResolvedEffects.FindRef(Event);
	if (!Effect)
	{
		if (!MissingEffectWarnings.Contains(Event))
		{
			MissingEffectWarnings.Add(Event);
			UE_LOG(LogMDHaptics, Warning,
				TEXT("Haptic event %s has no loaded ForceFeedbackEffect (%s)."),
				MDHaptics::GetEventName(Event),
				*Definition.Effect.ToSoftObjectPath().ToString());
		}
		LogBlocked(Event, TEXT("missing effect"));
		return false;
	}

	const double CurrentRealTime = GetCurrentRealTime();
	CleanupExpiredPlayback(CurrentRealTime);

	if (Definition.Priority < EMDHapticPriority::High
		&& CurrentRealTime < ActiveHighPriorityEndTime)
	{
		LogBlocked(Event, TEXT("high-priority story feedback is active"));
		return false;
	}

	if (const double* LastPlaybackTime = LastPlaybackTimes.Find(Event);
		LastPlaybackTime && CurrentRealTime - *LastPlaybackTime < Definition.MinimumReplayInterval)
	{
		LogBlocked(Event, TEXT("minimum replay interval"));
		return false;
	}

	if (Definition.Priority == EMDHapticPriority::High)
	{
		StopLowerPriorityPlayback(Definition.Priority);
	}

	FForceFeedbackParameters Parameters;
	Parameters.Tag = Definition.PlaybackTag;
	Parameters.bLooping = false;
	Parameters.bIgnoreTimeDilation = true;
	Parameters.bPlayWhilePaused = true;
	PlayerController->ClientPlayForceFeedback(Effect, Parameters);

	const double EffectEndTime = CurrentRealTime + FMath::Max(Effect->Duration, 0.0f);
	ActiveEffects.Add(Definition.PlaybackTag, Effect);
	ActiveEffectEndTimes.Add(Definition.PlaybackTag, EffectEndTime);
	ActiveEffectPriorities.Add(Definition.PlaybackTag, Definition.Priority);
	LastPlaybackTimes.Add(Event, CurrentRealTime);

	if (Definition.Priority == EMDHapticPriority::High)
	{
		ActiveHighPriorityEndTime = EffectEndTime;
	}

	if (MDHaptics::CVarDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogMDHaptics, Display,
			TEXT("Played %s: Effect=%s Tag=%s Input=Gamepad Strength=%.2f Duration=%.3fs"),
			MDHaptics::GetEventName(Event),
			*GetNameSafe(Effect),
			*Definition.PlaybackTag.ToString(),
			ControllerVibrationStrength,
			Effect->Duration);
	}

	return true;
}

void UMD_HapticSubsystem::StopHapticEvent(const EMDHapticEvent Event)
{
	const FMDHapticEventDefinition& Definition =
		GetDefault<UMD_HapticDeveloperSettings>()->GetDefinition(Event);
	const TObjectPtr<UForceFeedbackEffect>* ActiveEffect = ActiveEffects.Find(Definition.PlaybackTag);
	APlayerController* PlayerController = GetLocalPlayerController();
	if (ActiveEffect && PlayerController)
	{
		PlayerController->ClientStopForceFeedback(ActiveEffect->Get(), Definition.PlaybackTag);
	}

	ActiveEffects.Remove(Definition.PlaybackTag);
	ActiveEffectEndTimes.Remove(Definition.PlaybackTag);
	ActiveEffectPriorities.Remove(Definition.PlaybackTag);
	if (Definition.Priority == EMDHapticPriority::High)
	{
		ActiveHighPriorityEndTime = 0.0;
	}
}

void UMD_HapticSubsystem::StopAllHaptics()
{
	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		for (const TPair<FName, TObjectPtr<UForceFeedbackEffect>>& Playback : ActiveEffects)
		{
			PlayerController->ClientStopForceFeedback(Playback.Value, Playback.Key);
		}
	}

	ActiveEffects.Reset();
	ActiveEffectEndTimes.Reset();
	ActiveEffectPriorities.Reset();
	ActiveHighPriorityEndTime = 0.0;
}

void UMD_HapticSubsystem::SetControllerVibrationEnabled(const bool bEnabled)
{
	bControllerVibrationEnabled = bEnabled;
	if (UMD_GameUserSettings* UserSettings = UMD_GameUserSettings::Get())
	{
		UserSettings->SetControllerVibrationEnabled(bEnabled);
		UserSettings->SaveSettings();
	}

	if (!bEnabled)
	{
		StopAllHaptics();
	}
	ApplyControllerSettings();
}

void UMD_HapticSubsystem::SetControllerVibrationStrength(const float Strength)
{
	ControllerVibrationStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	if (UMD_GameUserSettings* UserSettings = UMD_GameUserSettings::Get())
	{
		UserSettings->SetControllerVibrationStrength(ControllerVibrationStrength);
		UserSettings->SaveSettings();
	}

	if (ControllerVibrationStrength <= UE_SMALL_NUMBER)
	{
		StopAllHaptics();
	}
	ApplyControllerSettings();
}

void UMD_HapticSubsystem::HandleInputDeviceChanged(
	const EMDInputDeviceType PreviousDevice,
	const EMDInputDeviceType NewDevice)
{
	if (NewDevice != EMDInputDeviceType::Gamepad)
	{
		StopAllHaptics();
	}
}

void UMD_HapticSubsystem::HandlePlayerControllerChanged(APlayerController* NewPlayerController)
{
	ActiveEffects.Reset();
	ActiveEffectEndTimes.Reset();
	ActiveEffectPriorities.Reset();
	ActiveHighPriorityEndTime = 0.0;
	ApplyControllerSettings(NewPlayerController);
}

void UMD_HapticSubsystem::HandleMenuSelectionChanged(UGameUIFocusScreenWidgetBase* Screen)
{
	if (Screen && Screen->GetOwningLocalPlayer() == GetLocalPlayer())
	{
		PlayHapticEvent(EMDHapticEvent::MenuSelection);
	}
}

void UMD_HapticSubsystem::HandleMenuBackHandled(UGameUIFocusScreenWidgetBase* Screen)
{
	if (Screen && Screen->GetOwningLocalPlayer() == GetLocalPlayer())
	{
		PlayHapticEvent(EMDHapticEvent::MenuBack);
	}
}

void UMD_HapticSubsystem::PreloadEffects()
{
	const UMD_HapticDeveloperSettings* Settings = GetDefault<UMD_HapticDeveloperSettings>();
	for (const EMDHapticEvent Event :
		{ EMDHapticEvent::Footstep, EMDHapticEvent::InteractionFocus,
		  EMDHapticEvent::Interaction,
		  EMDHapticEvent::MenuSelection, EMDHapticEvent::MenuBack,
		  EMDHapticEvent::StoryLight, EMDHapticEvent::StoryHeavy })
	{
		const FMDHapticEventDefinition& Definition = Settings->GetDefinition(Event);
		UForceFeedbackEffect* Effect = Definition.Effect.IsNull()
			? CreateGeneratedPulse(Event, Definition)
			: Definition.Effect.LoadSynchronous();
		if (Effect)
		{
			ResolvedEffects.Add(Event, Effect);
			continue;
		}

		MissingEffectWarnings.Add(Event);
		UE_LOG(LogMDHaptics, Warning,
			TEXT("Unable to resolve haptic event %s from %s or generated pulse settings."),
			MDHaptics::GetEventName(Event),
			*Definition.Effect.ToSoftObjectPath().ToString());
	}
}

UForceFeedbackEffect* UMD_HapticSubsystem::CreateGeneratedPulse(
	const EMDHapticEvent Event,
	const FMDHapticEventDefinition& Definition)
{
	if (Definition.PulseDuration <= UE_SMALL_NUMBER
		|| (Definition.LargeMotorStrength <= UE_SMALL_NUMBER
			&& Definition.SmallMotorStrength <= UE_SMALL_NUMBER))
	{
		return nullptr;
	}

	UForceFeedbackEffect* Effect = NewObject<UForceFeedbackEffect>(
		this,
		*FString::Printf(TEXT("FF_Generated_%s"), MDHaptics::GetEventName(Event)),
		RF_Transient);
	if (!Effect)
	{
		return nullptr;
	}

	auto AddPulseChannel = [Effect, &Definition](
		const float Strength,
		const bool bLargeMotor)
	{
		if (Strength <= UE_SMALL_NUMBER)
		{
			return;
		}

		FForceFeedbackChannelDetails& Channel = Effect->ChannelDetails.AddDefaulted_GetRef();
		Channel.bAffectsLeftLarge = bLargeMotor;
		Channel.bAffectsRightLarge = bLargeMotor;
		Channel.bAffectsLeftSmall = !bLargeMotor;
		Channel.bAffectsRightSmall = !bLargeMotor;

		FRichCurve* Curve = Channel.Curve.GetRichCurve();
		const float AttackTime = FMath::Min(0.004f, Definition.PulseDuration * 0.25f);
		Curve->AddKey(0.0f, 0.0f);
		Curve->AddKey(AttackTime, Strength);
		Curve->AddKey(Definition.PulseDuration, 0.0f);
	};

	AddPulseChannel(Definition.LargeMotorStrength, true);
	AddPulseChannel(Definition.SmallMotorStrength, false);
	Effect->Duration = Definition.PulseDuration;
	return Effect;
}

void UMD_HapticSubsystem::ApplyControllerSettings(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		PlayerController = GetLocalPlayerController();
	}
	if (PlayerController)
	{
		PlayerController->ForceFeedbackScale = bControllerVibrationEnabled
			? ControllerVibrationStrength
			: 0.0f;
	}
}

void UMD_HapticSubsystem::CleanupExpiredPlayback(const double CurrentRealTime)
{
	TArray<FName> ExpiredTags;
	for (const TPair<FName, double>& Playback : ActiveEffectEndTimes)
	{
		if (Playback.Value <= CurrentRealTime)
		{
			ExpiredTags.Add(Playback.Key);
		}
	}

	for (const FName Tag : ExpiredTags)
	{
		ActiveEffects.Remove(Tag);
		ActiveEffectEndTimes.Remove(Tag);
		ActiveEffectPriorities.Remove(Tag);
	}
	if (ActiveHighPriorityEndTime <= CurrentRealTime)
	{
		ActiveHighPriorityEndTime = 0.0;
	}
}

void UMD_HapticSubsystem::StopLowerPriorityPlayback(const EMDHapticPriority Priority)
{
	TArray<FName> LowerPriorityTags;
	for (const TPair<FName, EMDHapticPriority>& Playback : ActiveEffectPriorities)
	{
		if (Playback.Value < Priority)
		{
			LowerPriorityTags.Add(Playback.Key);
		}
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	for (const FName Tag : LowerPriorityTags)
	{
		if (const TObjectPtr<UForceFeedbackEffect>* Effect = ActiveEffects.Find(Tag);
			Effect && PlayerController)
		{
			PlayerController->ClientStopForceFeedback(Effect->Get(), Tag);
		}
		ActiveEffects.Remove(Tag);
		ActiveEffectEndTimes.Remove(Tag);
		ActiveEffectPriorities.Remove(Tag);
	}
}

void UMD_HapticSubsystem::LogBlocked(const EMDHapticEvent Event, const TCHAR* Reason) const
{
	if (MDHaptics::CVarDebug.GetValueOnGameThread() == 0)
	{
		return;
	}

	const TCHAR* InputName = InputDeviceSubsystem.IsValid() && InputDeviceSubsystem->IsUsingGamepad()
		? TEXT("Gamepad")
		: TEXT("KeyboardMouse");
	UE_LOG(LogMDHaptics, Display,
		TEXT("Blocked %s: Reason=%s Input=%s"),
		MDHaptics::GetEventName(Event), Reason, InputName);
}

APlayerController* UMD_HapticSubsystem::GetLocalPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}

double UMD_HapticSubsystem::GetCurrentRealTime() const
{
	return GetWorld() ? GetWorld()->GetRealTimeSeconds() : FPlatformTime::Seconds();
}
