#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "UI/Focus/GameUIFocusTypes.h"
#include "GameUIFocusScreenWidgetBase.generated.h"

class UWidget;
class UWidgetSwitcher;
enum class EGameUIFocusInputMode : uint8;

USTRUCT(BlueprintType)
struct FGameUIFocusNavigationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	TObjectPtr<UWidget> NavigationWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	int32 PageIndex = INDEX_NONE;
};

/** Designer-authored navigation entry resolved from the runtime widget hierarchy. */
USTRUCT(BlueprintType)
struct FGameUIFocusNavigationBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game UI|Focus")
	FName WidgetName = NAME_None;

	/** INDEX_NONE creates an action-only entry such as Back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game UI|Focus")
	int32 PageIndex = INDEX_NONE;
};

USTRUCT()
struct FGameUIFocusStateSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	EGameUIFocusZone Zone = EGameUIFocusZone::Navigation;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWidget> FocusWidget;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameUIFocusZoneChanged, EGameUIFocusZone, PreviousZone, EGameUIFocusZone, NewZone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameUINavigationIndexChanged, int32, PreviousIndex, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameUIBackActionHandled, EGameUIFocusZone, SourceZone);

UCLASS(BlueprintType, Blueprintable)
class GAMEUIFOCUS_API UGameUIFocusScreenWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameUIFocusScreenWidgetBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUIFocusZoneChanged OnFocusZoneChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUINavigationIndexChanged OnNavigationIndexChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUIFocusNavigationBlocked OnNavigationBlocked;

	/** Fired once after Back was consumed by this screen. Use it for audio and visual feedback. */
	UPROPERTY(BlueprintAssignable, Category = "Game UI|Focus")
	FGameUIBackActionHandled OnBackActionHandled;

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool InitializeFocusScreen(bool bFocusNavigation = true);

	/**
	 * Marks this screen as the active top entry and restores its focus after the
	 * current layer-stack mutation has completed. Repeated requests are
	 * serialized; only the newest request may apply focus.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestFocusScreenActivation(bool bFocusNavigation = true);

	/**
	 * Invalidates pending activation/focus work when this screen is covered or
	 * removed from its layer. Remembered navigation and content focus are kept.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void DeactivateFocusScreen();

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	bool IsFocusScreenActive() const { return bIsFocusScreenActive; }

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RegisterNavigationWidget(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RegisterNavigationEntry(UWidget* Widget, int32 PageIndex);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetNavigationWidgets(const TArray<UWidget*>& Widgets);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetNavigationEntries(const TArray<FGameUIFocusNavigationEntry>& Entries);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void SetFocusWidgetSwitcher(UWidgetSwitcher* InWidgetSwitcher);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool SwitchToPageIndex(int32 PageIndex, bool bFocusNavigation = true);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool ActivateNavigationEntryByIndex(int32 NavigationIndex, bool bEnterContent = false);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool ActivateNavigationWidget(UWidget* NavigationWidget, bool bEnterContent = false);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool ActivateCurrentNavigationEntry(bool bEnterContent = false);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool EnterContentZone();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool ReturnToNavigationZone();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool EnterModalZone(UWidget* ModalFocusWidget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool EnterModalZoneWithReturnFocus(UWidget* ModalFocusWidget, UWidget* ReturnFocusWidget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool ReturnFromModalZone();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool SetNavigationFocusByIndex(int32 NavigationIndex);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	virtual bool MoveNavigationFocus(int32 Direction);

	/** Routes analog input from a registered navigation item through the screen-owned gesture state. */
	virtual bool HandleNavigationWidgetAnalogInput(UWidget* NavigationWidget, FKey Key, float Value);

	/** Routes D-pad/keyboard navigation directly from the focused registered entry. */
	virtual bool HandleNavigationWidgetDigitalInput(UWidget* NavigationWidget, FIntPoint Direction, bool bIsRepeat);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestFocusNextTick(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void RememberFocusedWidget(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void NotifyContentWidgetFocused(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	void NotifyNavigationWidgetFocused(UWidget* Widget);

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	EGameUIFocusZone GetCurrentFocusZone() const { return CurrentFocusZone; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	int32 GetActiveNavigationIndex() const { return ActiveNavigationIndex; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus")
	UWidget* GetActivePageWidget() const;

	/** Switches UI feedback to gamepad navigation and hides stale pointer hover. */
	void NotifyGamepadInput(float InputStrength = 1.0f);

	/** Switches UI feedback to keyboard navigation without hiding the cursor. */
	void NotifyNavigationInput();

	/** Switches UI feedback back to pointer interaction. */
	void NotifyPointerInput();

	UFUNCTION(BlueprintPure, Category = "Game UI|Focus|Input")
	bool IsPointerInputActive() const { return bPointerInputActive; }

	/** Broadcasts local and global semantic feedback after Back was successfully consumed. */
	void BroadcastBackActionHandled(EGameUIFocusZone SourceZone);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleNavigationZoneKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleContentZoneKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleModalZoneKey(FKey Key);

	/** Optional top-level Back behavior. Other screens remain unchanged unless they override this. */
	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleRootBackAction();

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleFocusZoneChanged(EGameUIFocusZone PreviousZone, EGameUIFocusZone NewZone);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleNavigationIndexChanged(int32 PreviousIndex, int32 NewIndex);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Game UI|Focus")
	TObjectPtr<UWidgetSwitcher> FocusWidgetSwitcher = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	TArray<FGameUIFocusNavigationEntry> NavigationEntries;

	/** Optional deterministic setup; names may resolve inside composed child widgets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus")
	TArray<FGameUIFocusNavigationBinding> NavigationBindings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	EGameUIFocusZone CurrentFocusZone = EGameUIFocusZone::Navigation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	int32 ActiveNavigationIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	bool bSwitchPageWithNavigationFocus = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	float NavigationRepeatDelay = 0.18f;

	/** Hides the hardware cursor while gamepad input is the active UI device. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Input")
	bool bManageMouseCursorForInputDevice = true;

	/** Ignores tiny mouse deltas caused by cursor initialization or platform jitter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Input", meta = (ClampMin = "0.0"))
	float MouseMoveActivationThreshold = 0.5f;

	/** Ignores resting-stick noise when deciding that the gamepad became active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GamepadActivationThreshold = 0.15f;

	/** Shared tuning for navigation entries and the rightward left-stick gesture entering content. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game UI|Focus|Analog", meta = (ShowOnlyInnerProperties))
	FGameUIAnalogNavigationConfig AnalogNavigationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", ClampMax = "1.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig instead."))
	float AnalogNavigationDeadZone = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", ClampMax = "1.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig instead."))
	float AnalogNavigationReleaseThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig instead."))
	float AnalogInitialRepeatDelay = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog|Deprecated", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Configure AnalogNavigationConfig instead."))
	float AnalogRepeatInterval = 0.11f;

public:
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestFocusNextTickForZone(UWidget* Widget, EGameUIFocusZone Zone);

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FGameUIFocusSettingsAssetContractTest;
	friend class FGameUIFocusRuntimeAssetEventRoutingTest;
#endif

	void ClearNavigationEntries();
	void RebuildNavigationEntriesFromBindings();
	void NotifyActivePageDeactivated();
	bool RequestFocusNextTickInternal(UWidget* Widget, bool bApplyZone, EGameUIFocusZone Zone, int32 NavigationIndexToApply = INDEX_NONE, bool bLeaveActivePageOnSuccess = false);
	bool TryApplyPlayerFocus(UWidget* Widget) const;
	bool TryApplyKeyboardFocus(UWidget* Widget) const;
	void FinalizeFocusRequest(UWidget* Widget, bool bApplyZone, EGameUIFocusZone Zone, int32 NavigationIndexToApply, bool bLeaveActivePageOnSuccess);
	bool IsRetryTargetValid(UWidget* Widget, EGameUIFocusZone Zone) const;
	void LeaveActivePageFocus();
	UWidget* GetActiveFocusPageWidget() const;
	void SetCurrentFocusZone(EGameUIFocusZone NewZone);
	void SetActiveNavigationIndex(int32 NewIndex);
	int32 FindNavigationIndexForWidget(const UWidget* Widget) const;
	int32 FindNavigationIndexForPageIndex(int32 PageIndex) const;
	bool ProcessNavigationAnalogInput(FKey Key, float Value);
	int32 GetPageIndexForNavigationIndex(int32 NavigationIndex) const;
	UWidget* GetNavigationWidgetByIndex(int32 NavigationIndex) const;
	int32 GetNavigationEntryCount() const;
	bool CanProcessNavigationMove(bool bIsRepeat);
	void ResetAnalogNavigation();
	void TryMigrateLegacyAnalogConfig();
	void SetInputMode(EGameUIFocusInputMode NewMode);
	void RefreshPointerInteractionState();
	void HandleGlobalInputModeChanged(EGameUIFocusInputMode NewMode);
	void ApplyMouseCursorVisibility() const;
	void SchedulePointerInputStateReapply();
	void WarnIfWeakFocusTarget(const UWidget* Widget, EGameUIFocusZone Zone) const;
	void LogContentFocusFailure(const TCHAR* Reason, const UWidget* ContextWidget = nullptr) const;
	static bool IsUsableFocusTarget(const UWidget* Widget);
	static bool IsFocusPageWidget(const UWidget* Widget);
	static UWidget* FindFocusPageWidget(UWidget* RootWidget);
	static UWidget* FindWidgetByNameRecursive(UWidget* RootWidget, FName WidgetName);
	UPROPERTY(Transient)
	TWeakObjectPtr<UWidget> LastFocusedWidget;

	UPROPERTY(Transient)
	TArray<FGameUIFocusStateSnapshot> ModalFocusStack;

	uint64 FocusRequestSerial = 0;
	uint64 FocusScreenActivationSerial = 0;
	bool bIsFocusScreenActive = false;

	double LastNavigationMoveTimeSeconds = -1000.0;
	FGameUIAnalogNavigationState AnalogNavigationState;
	bool bMigratedLegacyAnalogConfig = false;
	bool bPointerInputActive = true;
	EGameUIFocusInputMode InputMode;
	FDelegateHandle InputModeChangedHandle;
	uint64 PointerInputReapplySerial = 0;
};
