#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameUIFocusPageInterface.generated.h"

class UWidget;

UINTERFACE(BlueprintType)
class GAMEUIFOCUS_API UGameUIFocusPageInterface : public UInterface
{
	GENERATED_BODY()
};

class GAMEUIFOCUS_API IGameUIFocusPageInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	void OnPageActivated();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	void OnPageDeactivated();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	bool EnterPageFocus();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	void LeavePageFocus();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	UWidget* GetDefaultFocusWidget() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	UWidget* GetLastFocusWidget() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game UI|Focus")
	void SetLastFocusWidget(UWidget* Widget);
};
