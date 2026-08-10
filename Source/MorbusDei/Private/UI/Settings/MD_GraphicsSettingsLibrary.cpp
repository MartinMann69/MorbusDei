#include "UI/Settings/MD_GraphicsSettingsLibrary.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace MDGraphicsSettings
{
	bool IsValidResolution(const FIntPoint Resolution)
	{
		return Resolution.X > 0 && Resolution.Y > 0;
	}

	bool IsSupportedResolution(const FIntPoint Resolution)
	{
		static const TSet<FIntPoint> SupportedResolutions =
		{
			// 16:9
			FIntPoint(1600, 900),
			FIntPoint(1920, 1080),
			FIntPoint(2560, 1440),
			FIntPoint(3840, 2160),

			// 16:10
			FIntPoint(1440, 900),
			FIntPoint(1680, 1050),
			FIntPoint(1920, 1200),
			FIntPoint(2560, 1600),
			FIntPoint(3840, 2400),

			// 21:9 ultrawide
			FIntPoint(2560, 1080),
			FIntPoint(3440, 1440),
			FIntPoint(3840, 1600),
			FIntPoint(5120, 2160)
		};

		return SupportedResolutions.Contains(Resolution);
	}

	void AddSupportedUniqueResolutions(
		const TArray<FIntPoint>& Source,
		TArray<FIntPoint>& Destination)
	{
		for (const FIntPoint Resolution : Source)
		{
			if (IsSupportedResolution(Resolution))
			{
				Destination.AddUnique(Resolution);
			}
		}
	}

	int64 GetPixelCount(const FIntPoint Resolution)
	{
		return static_cast<int64>(Resolution.X) * static_cast<int64>(Resolution.Y);
	}

	float CalculateResolutionDistance(const FIntPoint Candidate, const FIntPoint Reference)
	{
		const float WidthDifference =
			FMath::Abs(static_cast<float>(Candidate.X - Reference.X)) / static_cast<float>(Reference.X);
		const float HeightDifference =
			FMath::Abs(static_cast<float>(Candidate.Y - Reference.Y)) / static_cast<float>(Reference.Y);

		return WidthDifference + HeightDifference;
	}

	FIntPoint FindClosestSupportedResolution(
		const TArray<FIntPoint>& Resolutions,
		const FIntPoint Reference)
	{
		check(!Resolutions.IsEmpty());
		check(IsValidResolution(Reference));

		FIntPoint ClosestResolution = Resolutions[0];
		float ClosestDistance = CalculateResolutionDistance(ClosestResolution, Reference);

		for (int32 Index = 1; Index < Resolutions.Num(); ++Index)
		{
			const FIntPoint Candidate = Resolutions[Index];
			const float CandidateDistance = CalculateResolutionDistance(Candidate, Reference);
			const bool bIsCloser = CandidateDistance < ClosestDistance &&
				!FMath::IsNearlyEqual(CandidateDistance, ClosestDistance);
			const bool bIsEqualButCheaper = FMath::IsNearlyEqual(CandidateDistance, ClosestDistance) &&
				GetPixelCount(Candidate) < GetPixelCount(ClosestResolution);

			if (bIsCloser || bIsEqualButCheaper)
			{
				ClosestResolution = Candidate;
				ClosestDistance = CandidateDistance;
			}
		}

		return ClosestResolution;
	}

	FText FormatResolutionLabel(const FIntPoint Resolution)
	{
		FNumberFormattingOptions NumberFormat;
		NumberFormat.UseGrouping = false;

		return FText::Format(
			NSLOCTEXT("MDGraphicsSettings", "ResolutionLabel", "{0} x {1}"),
			FText::AsNumber(Resolution.X, &NumberFormat),
			FText::AsNumber(Resolution.Y, &NumberFormat));
	}

	FMDResolutionOptionSet BuildOptionSet(
		const TArray<FIntPoint>& CandidateResolutions,
		FIntPoint SelectedResolution)
	{
		FMDResolutionOptionSet Result;
		Result.SelectedResolution = SelectedResolution;
		AddSupportedUniqueResolutions(CandidateResolutions, Result.Resolutions);

		Result.Resolutions.Sort([](const FIntPoint A, const FIntPoint B)
		{
			return A.X == B.X ? A.Y < B.Y : A.X < B.X;
		});

		if (!Result.Resolutions.IsEmpty() && !Result.Resolutions.Contains(Result.SelectedResolution))
		{
			// Keep a valid legacy selection visually close to its previous dimensions.
			// A corrupt/first-run configuration uses the largest supported platform mode.
			Result.SelectedResolution = IsValidResolution(Result.SelectedResolution)
				? FindClosestSupportedResolution(Result.Resolutions, Result.SelectedResolution)
				: Result.Resolutions.Last();
		}

		Result.SelectedIndex = Result.Resolutions.IndexOfByKey(Result.SelectedResolution);
		Result.Labels.Reserve(Result.Resolutions.Num());

		for (const FIntPoint Resolution : Result.Resolutions)
		{
			Result.Labels.Add(FormatResolutionLabel(Resolution));
		}

		Result.bIsValid =
			Result.Resolutions.IsValidIndex(Result.SelectedIndex) &&
			Result.Labels.Num() == Result.Resolutions.Num();

		return Result;
	}
}

FMDResolutionOptionSet UMDGraphicsSettingsLibrary::BuildCurrentResolutionOptions()
{
	FIntPoint SelectedResolution = FIntPoint::ZeroValue;
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (UserSettings)
	{
		SelectedResolution = UserSettings->GetLastConfirmedScreenResolution();

		if (!MDGraphicsSettings::IsValidResolution(SelectedResolution))
		{
			SelectedResolution = UserSettings->GetScreenResolution();
		}

		if (!MDGraphicsSettings::IsValidResolution(SelectedResolution))
		{
			SelectedResolution = UserSettings->GetDesktopResolution();
		}
	}

	TArray<FIntPoint> PlatformResolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(PlatformResolutions);

	// Some platforms or remote sessions do not report fullscreen modes. Windowed
	// recommendations are a better fallback than inventing a hard-coded resolution.
	if (PlatformResolutions.IsEmpty())
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(PlatformResolutions);
	}

	return MDGraphicsSettings::BuildOptionSet(PlatformResolutions, SelectedResolution);
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMDResolutionOptionSetTest,
	"Nautilus.Settings.Graphics.ResolutionOptionSet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMDResolutionOptionSetTest::RunTest(const FString& Parameters)
{
	const TArray<FIntPoint> ReportedResolutions =
	{
		FIntPoint(1600, 900),
		FIntPoint(1920, 1080),
		FIntPoint(2560, 1440),
		FIntPoint(3840, 2160),
		FIntPoint(1440, 900),
		FIntPoint(1680, 1050),
		FIntPoint(1920, 1200),
		FIntPoint(2560, 1600),
		FIntPoint(3840, 2400),
		FIntPoint(2560, 1080),
		FIntPoint(3440, 1440),
		FIntPoint(3840, 1600),
		FIntPoint(5120, 2160),
		FIntPoint(1920, 1200),
		FIntPoint(1280, 720),
		FIntPoint(1280, 800),
		FIntPoint(1360, 768),
		FIntPoint(1366, 768),
		FIntPoint(1280, 1024),
		FIntPoint(2048, 1152),
		FIntPoint(5120, 1440),
		FIntPoint::ZeroValue
	};

	TestTrue(TEXT("1600x900 16:9 is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1600, 900)));
	TestTrue(TEXT("1920x1080 16:9 is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1920, 1080)));
	TestTrue(TEXT("2560x1440 16:9 is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(2560, 1440)));
	TestTrue(TEXT("1440x900 16:10 is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1440, 900)));
	TestTrue(TEXT("1920x1200 16:10 is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1920, 1200)));
	TestTrue(TEXT("2560x1080 ultrawide is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(2560, 1080)));
	TestTrue(TEXT("3440x1440 ultrawide is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(3440, 1440)));
	TestTrue(TEXT("5120x2160 ultrawide is supported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(5120, 2160)));

	TestFalse(TEXT("1280x720 is outside the curated catalog"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1280, 720)));
	TestFalse(TEXT("1280x800 is outside the curated catalog"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1280, 800)));
	TestFalse(TEXT("1360x768 is outside the curated catalog"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1360, 768)));
	TestFalse(TEXT("1366x768 is outside the curated catalog"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(1366, 768)));
	TestFalse(TEXT("2048x1152 is not fabricated from aspect ratio alone"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(2048, 1152)));
	TestFalse(TEXT("5120x1440 32:9 is unsupported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint(5120, 1440)));
	TestFalse(TEXT("Zero resolution is unsupported"), MDGraphicsSettings::IsSupportedResolution(FIntPoint::ZeroValue));

	const FMDResolutionOptionSet ExistingSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint(1920, 1200));
	TestTrue(TEXT("Existing confirmed resolution produces a valid option set"), ExistingSelection.bIsValid);
	TestEqual(TEXT("Duplicate and uncatalogued modes are removed"), ExistingSelection.Resolutions.Num(), 13);
	TestEqual(TEXT("Existing confirmed resolution keeps its sorted index"), ExistingSelection.SelectedIndex, 4);
	TestEqual(TEXT("Labels remain aligned with resolution values"), ExistingSelection.Labels.Num(), ExistingSelection.Resolutions.Num());
	TestEqual(TEXT("Selected label remains aligned"), ExistingSelection.Labels[ExistingSelection.SelectedIndex].ToString(), FString(TEXT("1920 x 1200")));
	TestEqual(TEXT("Supported modes remain sorted from lowest width"), ExistingSelection.Resolutions[0], FIntPoint(1440, 900));
	TestEqual(TEXT("Supported modes remain sorted through highest width"), ExistingSelection.Resolutions.Last(), FIntPoint(5120, 2160));

	TArray<FIntPoint> ReportedWithoutFullHd = ReportedResolutions;
	ReportedWithoutFullHd.Remove(FIntPoint(1920, 1080));
	const FMDResolutionOptionSet UnavailableSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedWithoutFullHd,
		FIntPoint(1920, 1080));
	TestTrue(TEXT("An unavailable supported resolution gets a valid fallback"), UnavailableSelection.bIsValid);
	TestFalse(TEXT("An unavailable supported resolution is not fabricated"), UnavailableSelection.Resolutions.Contains(FIntPoint(1920, 1080)));
	TestEqual(TEXT("An unavailable supported resolution uses the closest reported mode"), UnavailableSelection.SelectedResolution, FIntPoint(1920, 1200));

	const FMDResolutionOptionSet UnsupportedSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint(5120, 1440));
	TestTrue(TEXT("An unsupported confirmed resolution gets a valid fallback"), UnsupportedSelection.bIsValid);
	TestFalse(TEXT("An unsupported confirmed resolution is not reinserted"), UnsupportedSelection.Resolutions.Contains(FIntPoint(5120, 1440)));
	TestEqual(TEXT("Super-ultrawide falls back to the closest supported ultrawide"), UnsupportedSelection.SelectedResolution, FIntPoint(3440, 1440));
	TestEqual(TEXT("Closest fallback index is explicitly resolved"), UnsupportedSelection.SelectedIndex, 8);

	const TArray<FIntPoint> EqualDistanceResolutions =
	{
		FIntPoint(1600, 900),
		FIntPoint(2560, 1440)
	};
	const FMDResolutionOptionSet EqualDistanceSelection = MDGraphicsSettings::BuildOptionSet(
		EqualDistanceResolutions,
		FIntPoint(2080, 1170));
	TestEqual(TEXT("Equal-distance fallback prefers the lower pixel count"), EqualDistanceSelection.SelectedResolution, FIntPoint(1600, 900));

	const FMDResolutionOptionSet InvalidSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint::ZeroValue);
	TestTrue(TEXT("Invalid first-run selection uses a reported platform mode"), InvalidSelection.bIsValid);
	TestEqual(TEXT("Invalid first-run selection prefers the largest supported reported mode"), InvalidSelection.SelectedResolution, FIntPoint(5120, 2160));
	TestEqual(TEXT("First-run fallback index is explicitly resolved"), InvalidSelection.SelectedIndex, 12);

	const FMDResolutionOptionSet EmptySelection = MDGraphicsSettings::BuildOptionSet(
		TArray<FIntPoint>(),
		FIntPoint::ZeroValue);
	TestFalse(TEXT("No fabricated resolution is returned when the platform reports none"), EmptySelection.bIsValid);
	TestEqual(TEXT("An empty option set preserves INDEX_NONE"), EmptySelection.SelectedIndex, INDEX_NONE);

	return true;
}

#endif
