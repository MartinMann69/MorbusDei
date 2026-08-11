#pragma once

#include "CoreMinimal.h"
#include "MD_HapticTypes.generated.h"

class UForceFeedbackEffect;

DECLARE_LOG_CATEGORY_EXTERN(LogMDHaptics, Log, All);

UENUM(BlueprintType)
enum class EMDHapticEvent : uint8
{
	Footstep UMETA(DisplayName = "Footstep"),
	Interaction UMETA(DisplayName = "Interaction"),
	StoryLight UMETA(DisplayName = "Story Light"),
	StoryHeavy UMETA(DisplayName = "Story Heavy"),
	MenuSelection UMETA(DisplayName = "Menu Selection"),
	MenuBack UMETA(DisplayName = "Menu Back"),
	InteractionFocus UMETA(DisplayName = "Interaction Focus")
};

UENUM(BlueprintType)
enum class EMDHapticPriority : uint8
{
	Low,
	Normal,
	High
};

USTRUCT(BlueprintType)
struct FMDHapticEventDefinition
{
	GENERATED_BODY()

	/** One-shot force-feedback pattern used for this semantic event. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics")
	TSoftObjectPtr<UForceFeedbackEffect> Effect;

	/** Replaces an already playing effect with the same tag. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics")
	FName PlaybackTag;

	/** Higher priorities suppress lower-priority feedback while active. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics")
	EMDHapticPriority Priority = EMDHapticPriority::Normal;

	/** Real-time guard against animation blends or repeated gameplay events. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumReplayInterval = 0.0f;

	/** Duration of a generated pulse used when no ForceFeedbackEffect is assigned. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics|Generated Pulse", meta = (ClampMin = "0.0", Units = "s"))
	float PulseDuration = 0.0f;

	/** Low-frequency motor strength for a generated pulse. Keep menu feedback deliberately subtle. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics|Generated Pulse", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LargeMotorStrength = 0.0f;

	/** High-frequency motor strength for a generated pulse. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Haptics|Generated Pulse", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmallMotorStrength = 0.0f;
};
