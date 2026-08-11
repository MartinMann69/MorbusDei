#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "MD_GameUserSettings.generated.h"

/** Persistent accessibility settings owned by Unreal's standard user-settings flow. */
UCLASS(Config = GameUserSettings)
class MORBUSDEI_API UMD_GameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	virtual void ValidateSettings() override;
	virtual void SetToDefaults() override;

	void SetControllerVibrationEnabled(bool bEnabled);
	void SetControllerVibrationStrength(float Strength);

	bool IsControllerVibrationEnabled() const { return bControllerVibrationEnabled; }
	float GetControllerVibrationStrength() const { return ControllerVibrationStrength; }

	static UMD_GameUserSettings* Get();

private:
	UPROPERTY(Config)
	bool bControllerVibrationEnabled = true;

	UPROPERTY(Config)
	float ControllerVibrationStrength = 0.5f;
};
