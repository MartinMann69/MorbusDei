#include "UI/Focus/GameUIFocusTypes.h"

#include "InputCoreTypes.h"

void FGameUIAnalogReleaseGate::Arm(const bool bShouldWaitForRelease)
{
	bAwaitingRelease = bShouldWaitForRelease;
}

bool FGameUIAnalogReleaseGate::Process(const float AnalogValue, const float ReleaseThreshold)
{
	if (!bAwaitingRelease)
	{
		return false;
	}

	if (FMath::Abs(AnalogValue) <= FMath::Clamp(ReleaseThreshold, 0.0f, 1.0f))
	{
		bAwaitingRelease = false;
	}

	return true;
}

void FGameUIAnalogReleaseGate::Reset()
{
	bAwaitingRelease = false;
}

FGameUIAnalogNavigationResult FGameUIAnalogNavigationState::ProcessAxis(
	const FKey Key,
	const float Value,
	const double CurrentTimeSeconds,
	const FGameUIAnalogNavigationConfig& Config,
	const EGameUIAnalogNavigationMode Mode)
{
	FGameUIAnalogNavigationResult Result;
	if (Key == EKeys::Gamepad_LeftX)
	{
		StickValue.X = FMath::Clamp(Value, -1.0f, 1.0f);
		Result.bHandled = true;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		StickValue.Y = FMath::Clamp(Value, -1.0f, 1.0f);
		Result.bHandled = true;
	}
	else
	{
		return Result;
	}

	const float ReleaseThreshold = FMath::Clamp(Config.ReleaseThreshold, 0.0f, 1.0f);
	const float DeadZone = FMath::Clamp(Config.DeadZone, ReleaseThreshold, 1.0f);
	Result.Magnitude = GetRelevantMagnitude(Mode);

	if (Result.Magnitude <= ReleaseThreshold)
	{
		Reset();
		return Result;
	}

	if (Result.Magnitude < DeadZone)
	{
		return Result;
	}

	if (!bHeld)
	{
		LatchedDirection = ResolveDirection(Mode);
		if (LatchedDirection == FIntPoint::ZeroValue)
		{
			return Result;
		}

		bHeld = true;
		bRepeatActive = false;
		bBlockedFeedbackSent = false;
		LastNavigationTimeSeconds = CurrentTimeSeconds;
		Result.bShouldNavigate = true;
		Result.Direction = LatchedDirection;
		return Result;
	}

	// A fast physical reversal can cross the neutral zone between two controller
	// samples. Treat an exact opposite direction as a new deliberate
	// gesture so the previous latch cannot trap focus at a list boundary.
	const FIntPoint CurrentDirection = ResolveDirection(Mode);
	const FIntPoint OppositeLatchedDirection(-LatchedDirection.X, -LatchedDirection.Y);
	if (CurrentDirection == OppositeLatchedDirection)
	{
		LatchedDirection = CurrentDirection;
		bRepeatActive = false;
		bBlockedFeedbackSent = false;
		LastNavigationTimeSeconds = CurrentTimeSeconds;
		Result.bShouldNavigate = true;
		Result.Direction = LatchedDirection;
		return Result;
	}

	Result.Direction = LatchedDirection;
	if (!Config.bAllowRepeat)
	{
		return Result;
	}

	const double RepeatDelay = bRepeatActive
		? FMath::Max(0.01, static_cast<double>(Config.RepeatInterval))
		: FMath::Max(0.0, static_cast<double>(Config.InitialRepeatDelay));

	constexpr double TimingToleranceSeconds = 0.0001;
	if (CurrentTimeSeconds - LastNavigationTimeSeconds + TimingToleranceSeconds >= RepeatDelay)
	{
		bRepeatActive = true;
		// Advance along the configured timeline instead of anchoring to the event frame.
		// This prevents 30/60/120 Hz input sampling from changing the repeat cadence.
		LastNavigationTimeSeconds += RepeatDelay;
		Result.bShouldNavigate = true;
		Result.bIsRepeat = true;
	}

	return Result;
}

void FGameUIAnalogNavigationState::Reset()
{
	StickValue = FVector2D::ZeroVector;
	LatchedDirection = FIntPoint::ZeroValue;
	LastNavigationTimeSeconds = -1000.0;
	bHeld = false;
	bRepeatActive = false;
	bBlockedFeedbackSent = false;
}

void FGameUIAnalogNavigationState::NotifyNavigationSucceeded()
{
	bBlockedFeedbackSent = false;
}

bool FGameUIAnalogNavigationState::NotifyNavigationBlocked()
{
	if (bBlockedFeedbackSent)
	{
		return false;
	}

	bBlockedFeedbackSent = true;
	return true;
}

FIntPoint FGameUIAnalogNavigationState::ResolveDirection(const EGameUIAnalogNavigationMode Mode) const
{
	const int32 HorizontalDirection = StickValue.X > 0.0f ? 1 : -1;
	const int32 VerticalDirection = StickValue.Y > 0.0f ? -1 : 1;

	if (Mode == EGameUIAnalogNavigationMode::Vertical)
	{
		return FMath::IsNearlyZero(StickValue.Y) ? FIntPoint::ZeroValue : FIntPoint(0, VerticalDirection);
	}

	if (Mode == EGameUIAnalogNavigationMode::Horizontal)
	{
		return FMath::IsNearlyZero(StickValue.X) ? FIntPoint::ZeroValue : FIntPoint(HorizontalDirection, 0);
	}

	// Prefer vertical on an exact tie: most menu layouts are read top-to-bottom.
	if (FMath::Abs(StickValue.Y) >= FMath::Abs(StickValue.X))
	{
		return FMath::IsNearlyZero(StickValue.Y) ? FIntPoint::ZeroValue : FIntPoint(0, VerticalDirection);
	}

	return FMath::IsNearlyZero(StickValue.X) ? FIntPoint::ZeroValue : FIntPoint(HorizontalDirection, 0);
}

float FGameUIAnalogNavigationState::GetRelevantMagnitude(const EGameUIAnalogNavigationMode Mode) const
{
	if (Mode == EGameUIAnalogNavigationMode::Vertical)
	{
		return FMath::Abs(StickValue.Y);
	}

	if (Mode == EGameUIAnalogNavigationMode::Horizontal)
	{
		return FMath::Abs(StickValue.X);
	}

	return FMath::Max(FMath::Abs(StickValue.X), FMath::Abs(StickValue.Y));
}
