#include "UI/MD_MenuLayerScreenWidget.h"

#include "Player/MD_PlayerController.h"

bool UMD_MenuLayerScreenWidget::CloseMenuLayerScreen()
{
	if (!RequestPopMenuLayer())
	{
		return false;
	}

	if (AMD_PlayerController* PlayerController = Cast<AMD_PlayerController>(GetOwningPlayer()))
	{
		PlayerController->RestorePauseMenuFocus();
	}
	return true;
}

bool UMD_MenuLayerScreenWidget::HandleRootBackAction_Implementation()
{
	return CloseMenuLayerScreen();
}
