#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/Focus/GameUIFocusPageInterface.h"
#include "UI/Focus/GameUIFocusTypes.h"
#include "GameUIFocusPageWidgetBase.generated.h"

class UGameUIFocusItemWidgetBase;
class UGameUIFocusScreenWidgetBase;
class UScrollBox;
class UWidget;

enum class EGameUIFocusNavigationResult : uint8
{
	Unhandled,
	Blocked,
	Moved
};

UCLASS(BlueprintType, Blueprintable)
class GAMEUIFOCUS_API UGameUIFocusPageWidgetBase : public UUserWidget, public IGameUIFocusPageInterface
{
	GENERATED_BODY()

public:
	UGameUIFocusPageWidgetBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RegisterFocusItem(UGameUIFocusItemWidgetBase* Item);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RegisterFocusItemsInWidgetTree();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void ClearRegisteredFocusItems();

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	TArray<UGameUIFocusItemWidgetBase*> GetRegisteredFocusItems() const;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RememberFocusedWidget(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	virtual void NotifyFocusItemFocused(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	UWidget* GetBestFocusTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool FocusBestTarget();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool FocusAdjacentItem(UGameUIFocusItemWidgetBase* CurrentItem, int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool FocusAdjacentItem2D(UGameUIFocusItemWidgetBase* CurrentItem, FIntPoint Direction);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool FocusItemByIdentifier(FGameplayTag FocusIdentifier);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetOwningFocusScreen(UGameUIFocusScreenWidgetBase* Screen);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	UGameUIFocusScreenWidgetBase* GetOwningFocusScreen() const { return OwningFocusScreen.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestReturnToNavigationZone();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetFocusScrollBox(UScrollBox* InFocusScrollBox);

	bool HandleFocusItemAnalogInput(UGameUIFocusItemWidgetBase* CurrentItem, FKey Key, float Value);
	bool HandleFocusItemDigitalInput(UGameUIFocusItemWidgetBase* CurrentItem, FIntPoint Direction);
	void ResetAnalogNavigation();
	void UpdateHorizontalAnalogSample(float Value);
	void ResetHorizontalAnalogSample();
	bool IsHorizontalAnalogActuated(float ReleaseThreshold) const;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUIFocusNavigationBlocked OnNavigationBlocked;

	virtual void OnPageActivated_Implementation() override;
	virtual void OnPageDeactivated_Implementation() override;
	virtual bool EnterPageFocus_Implementation() override;
	virtual void LeavePageFocus_Implementation() override;
	virtual UWidget* GetDefaultFocusWidget_Implementation() const override;
	virtual UWidget* GetLastFocusWidget_Implementation() const override;
	virtual void SetLastFocusWidget_Implementation(UWidget* Widget) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleFocusItemFocused(UWidget* Widget);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	TObjectPtr<UWidget> DefaultFocusWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Game UI|Focus")
	TObjectPtr<UWidget> LastFocusWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Game UI|Focus")
	TObjectPtr<UGameUIFocusScreenWidgetBase> OwningFocusScreen = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Game UI|Focus")
	TObjectPtr<UScrollBox> FocusScrollBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	bool bAutoRegisterFocusItemsOnConstruct = true;

	/** Page-level analog navigation settings shared by every registered focus item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ShowOnlyInnerProperties))
	FGameUIAnalogNavigationConfig AnalogNavigationConfig;

	/** Restrict ordinary lists to Vertical to reject horizontal stick drift explicitly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog")
	EGameUIAnalogNavigationMode AnalogNavigationMode = EGameUIAnalogNavigationMode::TwoDimensional;

private:
	void PruneInvalidRegisteredFocusItems();
	bool IsRegisteredFocusItem(const UGameUIFocusItemWidgetBase* Item) const;
	EGameUIFocusNavigationResult NavigateFromItem(UGameUIFocusItemWidgetBase* CurrentItem, FIntPoint Direction);
	bool FocusItemInternal(UGameUIFocusItemWidgetBase* Target);
	UGameUIFocusItemWidgetBase* FindBestGridTarget(UGameUIFocusItemWidgetBase* CurrentItem, FIntPoint Direction) const;
	static bool IsUsableFocusTarget(const UWidget* Widget);
	void TryMigrateLegacyAnalogConfig(const UGameUIFocusItemWidgetBase* Item);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UGameUIFocusItemWidgetBase>> RegisteredFocusItems;

	FGameUIAnalogNavigationState AnalogNavigationState;
	float HorizontalAnalogSample = 0.0f;
	bool bMigratedLegacyAnalogConfig = false;
};
