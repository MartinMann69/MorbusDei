#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameUIFocusTypes.generated.h"

class UWidget;

GAMEUIFOCUS_API DECLARE_LOG_CATEGORY_EXTERN(LogGameUIFocus, Log, All);

UENUM(BlueprintType)
enum class EGameUIFocusZone : uint8
{
	Navigation UMETA(DisplayName = "Navigation"),
	Content UMETA(DisplayName = "Content"),
	Modal UMETA(DisplayName = "Modal")
};

UENUM(BlueprintType)
enum class EGameUIAnalogNavigationMode : uint8
{
	Vertical UMETA(DisplayName = "Vertical"),
	Horizontal UMETA(DisplayName = "Horizontal"),
	TwoDimensional UMETA(DisplayName = "Two Dimensional")
};

/** Designer-facing tuning for deliberate, controller-friendly analog focus navigation. */
USTRUCT(BlueprintType)
struct GAMEUIFOCUS_API FGameUIAnalogNavigationConfig
{
	GENERATED_BODY()

	/** Stick magnitude required to start a navigation gesture. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeadZone = 0.55f;

	/** Lower threshold used to release a held gesture without dead-zone chatter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReleaseThreshold = 0.30f;

	/** Delay before a held stick starts repeating. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0"))
	float InitialRepeatDelay = 0.40f;

	/** Delay between navigation steps once repeat is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.01"))
	float RepeatInterval = 0.17f;

	/** Disable for short menus that should require one deliberate stick gesture per step. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog")
	bool bAllowRepeat = true;
};

/** Internal result returned by the frame-rate-independent analog navigation state. */
struct GAMEUIFOCUS_API FGameUIAnalogNavigationResult
{
	bool bHandled = false;
	bool bShouldNavigate = false;
	bool bIsRepeat = false;
	FIntPoint Direction = FIntPoint::ZeroValue;
	float Magnitude = 0.0f;
};

/**
 * Stable analog gesture state. The owner must outlive individual focus targets so a
 * focus transfer cannot turn a held stick into a fresh press.
 */
struct GAMEUIFOCUS_API FGameUIAnalogNavigationState
{
	FGameUIAnalogNavigationResult ProcessAxis(
		FKey Key,
		float Value,
		double CurrentTimeSeconds,
		const FGameUIAnalogNavigationConfig& Config,
		EGameUIAnalogNavigationMode Mode);

	void Reset();
	void NotifyNavigationSucceeded();
	bool NotifyNavigationBlocked();

	bool IsHeld() const { return bHeld; }
	FIntPoint GetLatchedDirection() const { return LatchedDirection; }

private:
	FIntPoint ResolveDirection(EGameUIAnalogNavigationMode Mode) const;
	float GetRelevantMagnitude(EGameUIAnalogNavigationMode Mode) const;

	FVector2D StickValue = FVector2D::ZeroVector;
	FIntPoint LatchedDirection = FIntPoint::ZeroValue;
	double LastNavigationTimeSeconds = -1000.0;
	bool bHeld = false;
	bool bRepeatActive = false;
	bool bBlockedFeedbackSent = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGameUIFocusNavigationBlocked,
	UWidget*, CurrentWidget,
	FIntPoint, Direction);
