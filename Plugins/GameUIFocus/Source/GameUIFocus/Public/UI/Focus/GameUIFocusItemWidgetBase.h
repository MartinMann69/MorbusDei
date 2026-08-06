#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "UI/Focus/GameUIFocusTypes.h"
#include "GameUIFocusItemWidgetBase.generated.h"

class UGameUIFocusPageWidgetBase;
class UGameUIFocusScreenWidgetBase;
class UGameUIFocusItemWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameUIFocusItemActivated, UGameUIFocusItemWidgetBase*, Item);

UCLASS(BlueprintType, Blueprintable)
class GAMEUIFOCUS_API UGameUIFocusItemWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameUIFocusItemWidgetBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUIFocusItemActivated OnFocusItemActivated;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetOwningFocusPage(UGameUIFocusPageWidgetBase* Page);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetOwningNavigationScreen(UGameUIFocusScreenWidgetBase* Screen);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	UGameUIFocusPageWidgetBase* GetOwningFocusPage() const { return OwningFocusPage.Get(); }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	UGameUIFocusScreenWidgetBase* GetOwningNavigationScreen() const { return OwningNavigationScreen.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void NotifyFocused();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void ActivateFocusItem();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestReturnToNavigationZone();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestMoveFocus(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestMoveFocus2D(int32 ColumnDirection, int32 RowDirection);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetFocusIdentifier(FGameplayTag InFocusIdentifier);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	FGameplayTag GetFocusIdentifier() const { return FocusIdentifier; }

	/** See RightFocusOverrideIdentifier. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetRightFocusOverrideIdentifier(FGameplayTag InIdentifier) { RightFocusOverrideIdentifier = InIdentifier; }

	/** See DownFocusOverrideIdentifier. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetDownFocusOverrideIdentifier(FGameplayTag InIdentifier) { DownFocusOverrideIdentifier = InIdentifier; }

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetFocusGridPosition(int32 Column, int32 Row);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void ClearFocusGridPosition();

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	bool HasFocusGridPosition() const { return bHasFocusGridPosition; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	FIntPoint GetFocusGridPosition() const { return FocusGridPosition; }

	/** Transitional C++ migration hook for Blueprints that customized the former item-level tuning. */
	bool TryGetCustomizedLegacyAnalogNavigationConfig(FGameUIAnalogNavigationConfig& OutConfig) const;
	FGameplayTag GetFocusOverrideIdentifier(FIntPoint Direction) const;

	/** Called by the owning screen to suppress stale hover while a gamepad is active. */
	void SetPointerInputActive(bool bActive);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleFocusItemPreviewKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleFocusItemActivated();

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void OnInteractionHighlightChanged(bool bHighlighted);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	bool IsInteractionHighlighted() const { return bIsInteractionHighlighted; }

	bool RequestInteractionFocus();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	bool bReturnToNavigationOnBack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	bool bTriggerChildButtonClickOnActivation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	FGameplayTag FocusIdentifier;

	/**
	 * When set, pressing Right jumps straight to the focus item registered with this
	 * identifier (via FocusItemByIdentifier) instead of the nearest item found by 2D
	 * grid geometry. Use this for category-boundary transitions where geometric
	 * nearest-neighbor would be ambiguous or layout-fragile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	FGameplayTag RightFocusOverrideIdentifier;

	/** Same as RightFocusOverrideIdentifier, but for the Down direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	FGameplayTag DownFocusOverrideIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", ClampMax = "1.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig on the owning focus page."))
	float FocusItemAnalogNavigationDeadZone = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", ClampMax = "1.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig on the owning focus page."))
	float FocusItemAnalogNavigationReleaseThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig on the owning focus page."))
	float FocusItemAnalogInitialRepeatDelay = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig on the owning focus page."))
	float FocusItemAnalogRepeatInterval = 0.11f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Game UI|Focus")
	TObjectPtr<UGameUIFocusPageWidgetBase> OwningFocusPage = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Game UI|Focus")
	TObjectPtr<UGameUIFocusScreenWidgetBase> OwningNavigationScreen = nullptr;

private:
	void SetFocusPathActive(bool bInHasFocusPath);
	void SetHovered(bool bInIsHovered);
	void UpdateInteractionHighlight();
	bool TriggerFirstChildButtonClick();
	static bool IsAcceptAction(const FKeyEvent& KeyEvent);
	static bool IsBackKey(const FKey& Key);
	static bool IsUsableFocusTarget(const UWidget* Widget);

	UPROPERTY(Transient)
	bool bHasFocusPath = false;

	UPROPERTY(Transient)
	bool bIsHovered = false;

	UPROPERTY(Transient)
	bool bPointerInputActive = true;

	UPROPERTY(Transient)
	bool bIsInteractionHighlighted = false;

	UPROPERTY(Transient)
	bool bPointerPressed = false;

	UPROPERTY(Transient)
	bool bAcceptPressed = false;

	UPROPERTY(Transient)
	FIntPoint FocusGridPosition = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	bool bHasFocusGridPosition = false;

	FDelegateHandle PointerInputStateChangedHandle;

};
