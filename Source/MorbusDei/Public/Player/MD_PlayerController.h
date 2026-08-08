// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MD_PlayerController.generated.h"

class UMD_PauseMenuWidget;
class UMD_MenuLayerScreenWidget;

/** Owns local pause-menu lifetime, input mode and guarded close transitions. */
UCLASS()
class MORBUSDEI_API AMD_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Opens the pause menu through the existing UI layer bridge. */
	UFUNCTION(BlueprintCallable, Category = "MD|UI|Pause Menu")
	bool OpenPauseMenu();

	/** Requests one guarded close transition. Duplicate requests are rejected. */
	UFUNCTION(BlueprintCallable, Category = "MD|UI|Pause Menu")
	bool RequestClosePauseMenu();

	UFUNCTION(BlueprintPure, Category = "MD|UI|Pause Menu")
	bool IsPauseMenuOpen() const;

private:
	friend class UMD_PauseMenuWidget;
	friend class UMD_MenuLayerScreenWidget;

	enum class EPauseMenuState : uint8
	{
		Closed,
		Open,
		Closing
	};

	void RegisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget);
	void UnregisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget);
	void HandleCloseTransitionFinished(UMD_PauseMenuWidget* PauseMenuWidget);
	void FinalizePauseMenuClose();
	bool ExecutePauseMenuLayerToggle();
	bool ApplyPauseMenuInput(UMD_PauseMenuWidget* PauseMenuWidget);
	bool RestorePauseMenuFocus();
	void RestoreGameplayInput();
	void EnablePauseMenuFullTick();
	void RestorePauseMenuFullTick();

	UPROPERTY(Transient)
	TWeakObjectPtr<UMD_PauseMenuWidget> ActivePauseMenuWidget;

	FDelegateHandle CloseTransitionFinishedHandle;
	EPauseMenuState PauseMenuState = EPauseMenuState::Closed;
	bool bPauseMenuOverridesFullTick = false;
	bool bPreviousFullTickWhenPaused = false;
};
