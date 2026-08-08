#include "Player/MD_PlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Input/MD_InputDeviceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MD_PauseMenuWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMDPlayerController, Log, All);

void AMD_PlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!IsLocalController() || !World || !World->IsPaused() || ShouldPerformFullTickWhenPaused())
	{
		return;
	}

	// APlayerController's minimal pause tick returns before Unreal processes force
	// feedback. Process only that missing path here; bPlayWhilePaused still decides
	// which effects are allowed to reach the controller while gameplay is frozen.
	ProcessForceFeedbackAndHaptics(DeltaSeconds, true);
}

bool AMD_PlayerController::OpenPauseMenu()
{
	if (PauseMenuState != EPauseMenuState::Closed)
	{
		return false;
	}

	if (!ExecutePauseMenuLayerToggle())
	{
		RestoreGameplayInput();
		return false;
	}

	if (!ActivePauseMenuWidget.IsValid())
	{
		UE_LOG(LogMDPlayerController, Error,
			TEXT("Pause layer toggle did not construct a UMD_PauseMenuWidget. Reparent WBP_PauseMenu to the native pause widget class."));

		// Roll the Blueprint layer operation back before restoring gameplay. This
		// prevents a wrong widget class or failed construction from trapping input.
		PauseMenuState = EPauseMenuState::Open;
		ExecutePauseMenuLayerToggle();
		PauseMenuState = EPauseMenuState::Closed;
		RestoreGameplayInput();
		return false;
	}

	if (!ApplyPauseMenuInput(ActivePauseMenuWidget.Get()))
	{
		UE_LOG(LogMDPlayerController, Error, TEXT("Failed to apply pause-menu input state; rolling the layer push back."));
		ExecutePauseMenuLayerToggle();
		ActivePauseMenuWidget.Reset();
		PauseMenuState = EPauseMenuState::Closed;
		RestoreGameplayInput();
		return false;
	}

	return true;
}

bool AMD_PlayerController::RequestClosePauseMenu()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuState != EPauseMenuState::Open || !PauseMenuWidget)
	{
		return false;
	}

	PauseMenuState = EPauseMenuState::Closing;
	if (!PauseMenuWidget->BeginCloseTransition())
	{
		PauseMenuState = EPauseMenuState::Open;
		return false;
	}

	return true;
}

bool AMD_PlayerController::IsPauseMenuOpen() const
{
	return PauseMenuState != EPauseMenuState::Closed;
}

void AMD_PlayerController::RegisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (!IsValid(PauseMenuWidget))
	{
		return;
	}

	if (UMD_PauseMenuWidget* PreviousWidget = ActivePauseMenuWidget.Get())
	{
		if (PreviousWidget == PauseMenuWidget)
		{
			return;
		}

		PreviousWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
		UE_LOG(LogMDPlayerController, Warning,
			TEXT("Replacing an active pause widget. Previous=%s New=%s"),
			*GetNameSafe(PreviousWidget),
			*GetNameSafe(PauseMenuWidget));
	}

	ActivePauseMenuWidget = PauseMenuWidget;
	CloseTransitionFinishedHandle = PauseMenuWidget->OnCloseTransitionFinished().AddUObject(
		this,
		&AMD_PlayerController::HandleCloseTransitionFinished);
	PauseMenuState = EPauseMenuState::Open;
}

void AMD_PlayerController::UnregisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (ActivePauseMenuWidget.Get() != PauseMenuWidget)
	{
		return;
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
	}
	CloseTransitionFinishedHandle.Reset();
	ActivePauseMenuWidget.Reset();

	if (PauseMenuState != EPauseMenuState::Closing)
	{
		PauseMenuState = EPauseMenuState::Closed;
	}
}

void AMD_PlayerController::HandleCloseTransitionFinished(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (PauseMenuState != EPauseMenuState::Closing || ActivePauseMenuWidget.Get() != PauseMenuWidget)
	{
		return;
	}

	FinalizePauseMenuClose();
}

void AMD_PlayerController::FinalizePauseMenuClose()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuWidget)
	{
		PauseMenuWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
	}
	CloseTransitionFinishedHandle.Reset();

	const bool bLayerToggleExecuted = ExecutePauseMenuLayerToggle();
	if (!bLayerToggleExecuted && PauseMenuWidget)
	{
		// Fail safe: never leave the player trapped if the legacy layer bridge is unavailable.
		PauseMenuWidget->RemoveFromParent();
	}

	ActivePauseMenuWidget.Reset();
	PauseMenuState = EPauseMenuState::Closed;
	RestoreGameplayInput();
}

bool AMD_PlayerController::ExecutePauseMenuLayerToggle()
{
	// The layer stack is still Blueprint-authored. Keep this compatibility bridge
	// private so all gameplay callers must pass through the guarded native API.
	static const FName ToggleFunctionName(TEXT("TogglePauseMenu"));
	UFunction* ToggleFunction = FindFunction(ToggleFunctionName);
	if (!ToggleFunction)
	{
		UE_LOG(LogMDPlayerController, Error,
			TEXT("BP_MainPlayerController must provide the existing TogglePauseMenu layer bridge."));
		return false;
	}

	ProcessEvent(ToggleFunction, nullptr);
	return true;
}

bool AMD_PlayerController::ApplyPauseMenuInput(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (!IsValid(PauseMenuWidget) || !UGameplayStatics::SetGamePaused(this, true))
	{
		return false;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr;
	bShowMouseCursor = !InputDeviceSubsystem || !InputDeviceSubsystem->IsUsingGamepad();
	return true;
}

bool AMD_PlayerController::RestorePauseMenuFocus()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuState != EPauseMenuState::Open || !IsValid(PauseMenuWidget))
	{
		return false;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>()
		: nullptr;
	bShowMouseCursor = !InputDeviceSubsystem || !InputDeviceSubsystem->IsUsingGamepad();

	// The world remains paused, so a world-timer next-tick request may never run.
	// Defer through the core ticker and initialize directly after the layer pop.
	const TWeakObjectPtr<AMD_PlayerController> WeakPlayerController(this);
	const TWeakObjectPtr<UMD_PauseMenuWidget> WeakPauseMenuWidget(PauseMenuWidget);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakPlayerController, WeakPauseMenuWidget](const float DeltaTime)
		{
			AMD_PlayerController* PlayerController = WeakPlayerController.Get();
			UMD_PauseMenuWidget* DeferredPauseMenuWidget = WeakPauseMenuWidget.Get();
			if (PlayerController
				&& DeferredPauseMenuWidget
				&& PlayerController->PauseMenuState == EPauseMenuState::Open
				&& PlayerController->ActivePauseMenuWidget.Get() == DeferredPauseMenuWidget)
			{
				DeferredPauseMenuWidget->InitializeFocusScreen(true);
			}
			return false;
		}));
	return true;
}

void AMD_PlayerController::RestoreGameplayInput()
{
	UGameplayStatics::SetGamePaused(this, false);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	FlushPressedKeys();
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

