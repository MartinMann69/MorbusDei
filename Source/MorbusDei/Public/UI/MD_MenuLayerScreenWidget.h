#pragma once

#include "CoreMinimal.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"
#include "MD_MenuLayerScreenWidget.generated.h"

/** Base for menu-layer sub-screens that return to the screen below on root Back. */
UCLASS(Abstract, Blueprintable)
class MORBUSDEI_API UMD_MenuLayerScreenWidget : public UGameUIFocusScreenWidgetBase
{
	GENERATED_BODY()

public:
	/** Pops this screen and restores focus to the active pause menu below it. */
	UFUNCTION(BlueprintCallable, Category = "MD|UI|Navigation")
	bool CloseMenuLayerScreen();

protected:
	virtual bool HandleRootBackAction_Implementation() override;

	/** Blueprint keeps ownership of the existing layer-stack mutation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "MD|UI|Navigation")
	bool RequestPopMenuLayer();
};
