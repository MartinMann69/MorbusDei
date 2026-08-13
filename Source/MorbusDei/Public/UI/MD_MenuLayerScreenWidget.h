#pragma once

#include "CoreMinimal.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"
#include "MD_MenuLayerScreenWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class MORBUSDEI_API UMD_MenuLayerScreenWidget : public UGameUIFocusScreenWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MD|UI|Navigation")
	bool CloseMenuLayerScreen();

protected:
	virtual bool HandleRootBackAction_Implementation() override;

	/** Implemented by the Blueprint-owned layer stack. */
	UFUNCTION(BlueprintImplementableEvent, Category = "MD|UI|Navigation")
	bool RequestPopMenuLayer();
};
