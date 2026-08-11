#include "Haptics/MD_GameUserSettings.h"

#include "Engine/Engine.h"

void UMD_GameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	ControllerVibrationStrength = FMath::Clamp(ControllerVibrationStrength, 0.0f, 1.0f);
}

void UMD_GameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	bControllerVibrationEnabled = true;
	ControllerVibrationStrength = 0.5f;
}

void UMD_GameUserSettings::SetControllerVibrationEnabled(const bool bEnabled)
{
	bControllerVibrationEnabled = bEnabled;
}

void UMD_GameUserSettings::SetControllerVibrationStrength(const float Strength)
{
	ControllerVibrationStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
}

UMD_GameUserSettings* UMD_GameUserSettings::Get()
{
	return GEngine ? Cast<UMD_GameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}
