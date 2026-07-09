#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "UI/Focus/GameUIFocusTypes.h"
#include "GameUIFocusScreenWidgetBase.generated.h"

class UWidget;
class UWidgetSwitcher;

USTRUCT(BlueprintType)
struct FGameUIFocusNavigationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	TObjectPtr<UWidget> NavigationWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
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

	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool InitializeFocusScreen(bool bFocusNavigation = true);

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
	bool MoveNavigationFocus(int32 Direction);

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

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleNavigationZoneKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleContentZoneKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	bool HandleModalZoneKey(FKey Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleFocusZoneChanged(EGameUIFocusZone PreviousZone, EGameUIFocusZone NewZone);

	UFUNCTION(BlueprintNativeEvent, Category = "Game UI|Focus")
	void HandleNavigationIndexChanged(int32 PreviousIndex, int32 NewIndex);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Game UI|Focus")
	TObjectPtr<UWidgetSwitcher> FocusWidgetSwitcher = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	TArray<FGameUIFocusNavigationEntry> NavigationEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	EGameUIFocusZone CurrentFocusZone = EGameUIFocusZone::Navigation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	int32 ActiveNavigationIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	bool bSwitchPageWithNavigationFocus = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus")
	float NavigationRepeatDelay = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogNavigationDeadZone = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogNavigationReleaseThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0"))
	float AnalogInitialRepeatDelay = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI|Focus|Analog", meta = (ClampMin = "0.0"))
	float AnalogRepeatInterval = 0.11f;

public:
	UFUNCTION(BlueprintCallable, Category = "Game UI|Focus")
	bool RequestFocusNextTickForZone(UWidget* Widget, EGameUIFocusZone Zone);

private:
	bool RequestFocusNextTickInternal(UWidget* Widget, bool bApplyZone, EGameUIFocusZone Zone, int32 NavigationIndexToApply = INDEX_NONE, bool bLeaveActivePageOnSuccess = false);
	void LeaveActivePageFocus();
	UWidget* GetActiveFocusPageWidget() const;
	void SetCurrentFocusZone(EGameUIFocusZone NewZone);
	void SetActiveNavigationIndex(int32 NewIndex);
	int32 FindNavigationIndexForWidget(const UWidget* Widget) const;
	int32 FindNavigationIndexForPageIndex(int32 PageIndex) const;
	int32 GetPageIndexForNavigationIndex(int32 NavigationIndex) const;
	UWidget* GetNavigationWidgetByIndex(int32 NavigationIndex) const;
	int32 GetNavigationEntryCount() const;
	bool CanProcessNavigationMove(bool bIsRepeat);
	bool HandleAnalogNavigationMove(int32 Direction, float Magnitude);
	bool HandleAnalogContentEnter(float Magnitude);
	void ResetAnalogNavigationMove();
	void ResetAnalogContentEnter();
	void WarnIfWeakFocusTarget(const UWidget* Widget, EGameUIFocusZone Zone) const;
	void LogContentFocusFailure(const TCHAR* Reason, const UWidget* ContextWidget = nullptr) const;
	static bool IsUsableFocusTarget(const UWidget* Widget);
	static bool IsFocusPageWidget(const UWidget* Widget);
	static UWidget* FindFocusPageWidget(UWidget* RootWidget);
	static bool IsRightKey(const FKey& Key);
	static bool IsAcceptKey(const FKey& Key);
	static bool IsUpKey(const FKey& Key);
	static bool IsDownKey(const FKey& Key);
	static bool IsBackKey(const FKey& Key);

	UPROPERTY(Transient)
	TWeakObjectPtr<UWidget> LastFocusedWidget;

	UPROPERTY(Transient)
	TArray<FGameUIFocusStateSnapshot> ModalFocusStack;

	uint64 FocusRequestSerial = 0;

	double LastNavigationMoveTimeSeconds = -1000.0;
	double LastAnalogNavigationMoveTimeSeconds = -1000.0;
	int32 LastAnalogNavigationDirection = 0;
	bool bAnalogNavigationHeld = false;
	bool bAnalogNavigationRepeatActive = false;
	bool bAnalogContentEnterHeld = false;
};
