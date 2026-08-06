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

	void AddValidUniqueResolutions(
		const TArray<FIntPoint>& Source,
		TArray<FIntPoint>& Destination)
	{
		for (const FIntPoint Resolution : Source)
		{
			if (IsValidResolution(Resolution))
			{
				Destination.AddUnique(Resolution);
			}
		}
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
		AddValidUniqueResolutions(CandidateResolutions, Result.Resolutions);

		// Windowed/custom resolutions are valid even when a platform only reports
		// exclusive-fullscreen modes. Keep the active value visible in the UI.
		if (IsValidResolution(Result.SelectedResolution))
		{
			Result.Resolutions.AddUnique(Result.SelectedResolution);
		}

		Result.Resolutions.Sort([](const FIntPoint A, const FIntPoint B)
		{
			return A.X == B.X ? A.Y < B.Y : A.X < B.X;
		});

		if (!IsValidResolution(Result.SelectedResolution) && !Result.Resolutions.IsEmpty())
		{
			// Prefer the largest reported mode for a corrupt/first-run configuration.
			Result.SelectedResolution = Result.Resolutions.Last();
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
		FIntPoint(1920, 1080),
		FIntPoint(1280, 720),
		FIntPoint(1920, 1080),
		FIntPoint::ZeroValue
	};

	const FMDResolutionOptionSet ExistingSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint(1920, 1080));
	TestTrue(TEXT("Existing confirmed resolution produces a valid option set"), ExistingSelection.bIsValid);
	TestEqual(TEXT("Duplicate and invalid modes are removed"), ExistingSelection.Resolutions.Num(), 2);
	TestEqual(TEXT("Existing confirmed resolution keeps its sorted index"), ExistingSelection.SelectedIndex, 1);
	TestEqual(TEXT("Labels remain aligned with resolution values"), ExistingSelection.Labels.Num(), ExistingSelection.Resolutions.Num());

	const FMDResolutionOptionSet MissingSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint(2560, 1440));
	TestTrue(TEXT("A confirmed custom resolution is retained"), MissingSelection.bIsValid);
	TestEqual(TEXT("Confirmed custom resolution is included"), MissingSelection.Resolutions.Num(), 3);
	TestEqual(TEXT("Confirmed custom resolution never falls back to index zero"), MissingSelection.SelectedIndex, 2);
	TestEqual(TEXT("Confirmed custom resolution label is correct"), MissingSelection.Labels[2].ToString(), FString(TEXT("2560 x 1440")));

	const FMDResolutionOptionSet InvalidSelection = MDGraphicsSettings::BuildOptionSet(
		ReportedResolutions,
		FIntPoint::ZeroValue);
	TestTrue(TEXT("Invalid first-run selection uses a reported platform mode"), InvalidSelection.bIsValid);
	TestEqual(TEXT("Invalid first-run selection prefers the largest reported mode"), InvalidSelection.SelectedResolution, FIntPoint(1920, 1080));
	TestEqual(TEXT("First-run fallback index is explicitly resolved"), InvalidSelection.SelectedIndex, 1);

	const FMDResolutionOptionSet EmptySelection = MDGraphicsSettings::BuildOptionSet(
		TArray<FIntPoint>(),
		FIntPoint::ZeroValue);
	TestFalse(TEXT("No fabricated resolution is returned when the platform reports none"), EmptySelection.bIsValid);
	TestEqual(TEXT("An empty option set preserves INDEX_NONE"), EmptySelection.SelectedIndex, INDEX_NONE);

	return true;
}

#endif
