#include "UI/Focus/GameUIFocusScreenWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Focus/GameUIFocusInputKeys.h"
#include "UI/Focus/GameUIFocusInputDeviceTracker.h"
#include "UI/Focus/GameUIFocusItemWidgetBase.h"
#include "UI/Focus/GameUIFocusPageInterface.h"
#include "UI/Focus/GameUIFocusPageWidgetBase.h"
#include "TimerManager.h"

namespace
{
FString FocusZoneToString(const EGameUIFocusZone Zone)
{
	if (const UEnum* Enum = StaticEnum<EGameUIFocusZone>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(Zone));
	}

	return FString::FromInt(static_cast<int32>(Zone));
}

bool IsFocusTraceKey(const FKey& Key)
{
	return Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == GameUIFocusInputKeys::GetVirtualAcceptKey()
		|| Key == EKeys::Gamepad_FaceButton_Bottom
		|| Key == EKeys::Left
		|| Key == EKeys::Right
		|| Key == EKeys::Up
		|| Key == EKeys::Down
		|| Key == EKeys::Gamepad_DPad_Left
		|| Key == EKeys::Gamepad_DPad_Right
		|| Key == EKeys::Gamepad_DPad_Up
		|| Key == EKeys::Gamepad_DPad_Down
		|| Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Right;
}
}

UGameUIFocusScreenWidgetBase::UGameUIFocusScreenWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UGameUIFocusScreenWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildNavigationEntriesFromBindings();
	PointerInputStateChangedHandle = GameUIFocusInputDeviceTracker::OnPointerInputStateChanged().AddUObject(
		this,
		&UGameUIFocusScreenWidgetBase::HandleGlobalPointerInputStateChanged);
	SetPointerInputActive(GameUIFocusInputDeviceTracker::IsPointerInputActive());
}

void UGameUIFocusScreenWidgetBase::NativeDestruct()
{
	GameUIFocusInputDeviceTracker::OnPointerInputStateChanged().Remove(PointerInputStateChangedHandle);
	PointerInputStateChangedHandle.Reset();
	++PointerInputReapplySerial;
	++FocusScreenActivationSerial;
	++FocusRequestSerial;
	bIsFocusScreenActive = false;
	ResetAnalogNavigation();
	Super::NativeDestruct();
}

bool UGameUIFocusScreenWidgetBase::RequestFocusScreenActivation(const bool bFocusNavigation)
{
	// A layer activation is authoritative over focus work requested while the
	// widget was still being constructed or attached to its layer.
	++FocusRequestSerial;
	ResetAnalogNavigation();
	bIsFocusScreenActive = true;

	const uint64 ActivationSerial = ++FocusScreenActivationSerial;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ScreenActivationScheduled Screen=%s Serial=%llu FocusNavigation=%d"),
		*GetNameSafe(this),
		ActivationSerial,
		bFocusNavigation ? 1 : 0);

	UWorld* World = GetWorld();
	if (!World)
	{
		const bool bActivated = InitializeFocusScreen(bFocusNavigation);
		bIsFocusScreenActive = bActivated;
		return bActivated;
	}

	const TWeakObjectPtr<UGameUIFocusScreenWidgetBase> WeakThis = this;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
		[WeakThis, ActivationSerial, bFocusNavigation]()
		{
			UGameUIFocusScreenWidgetBase* FocusScreen = WeakThis.Get();
			if (!FocusScreen
				|| FocusScreen->FocusScreenActivationSerial != ActivationSerial
				|| !FocusScreen->bIsFocusScreenActive)
			{
				UE_LOG(LogGameUIFocus, VeryVerbose,
					TEXT("GameUIFocusTrace ScreenActivationSkipped Screen=%s Serial=%llu Reason=Stale"),
					*GetNameSafe(FocusScreen),
					ActivationSerial);
				return;
			}

			const ESlateVisibility Visibility = FocusScreen->GetVisibility();
			if (Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden)
			{
				FocusScreen->bIsFocusScreenActive = false;
				UE_LOG(LogGameUIFocus, VeryVerbose,
					TEXT("GameUIFocusTrace ScreenActivationSkipped Screen=%s Serial=%llu Reason=Hidden Visibility=%d"),
					*GetNameSafe(FocusScreen),
					ActivationSerial,
					static_cast<int32>(Visibility));
				return;
			}

			const bool bActivated = FocusScreen->InitializeFocusScreen(bFocusNavigation);
			FocusScreen->bIsFocusScreenActive = bActivated;
			UE_LOG(LogGameUIFocus, VeryVerbose,
				TEXT("GameUIFocusTrace ScreenActivationCompleted Screen=%s Serial=%llu Confirmed=%d"),
				*GetNameSafe(FocusScreen),
				ActivationSerial,
				bActivated ? 1 : 0);
		}));

	return true;
}

void UGameUIFocusScreenWidgetBase::DeactivateFocusScreen()
{
	const bool bWasActive = bIsFocusScreenActive;
	++FocusScreenActivationSerial;
	++FocusRequestSerial;
	bIsFocusScreenActive = false;
	ResetAnalogNavigation();

	if (bWasActive)
	{
		NotifyActivePageDeactivated();
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ScreenDeactivated Screen=%s WasActive=%d ActiveNav=%d Zone=%s"),
		*GetNameSafe(this),
		bWasActive ? 1 : 0,
		ActiveNavigationIndex,
		*FocusZoneToString(CurrentFocusZone));
}

bool UGameUIFocusScreenWidgetBase::InitializeFocusScreen(bool bFocusNavigation)
{
	TryMigrateLegacyAnalogConfig();
	ResetAnalogNavigation();
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace Initialize Screen=%s bFocusNavigation=%d ActiveNav=%d Switcher=%s ActivePage=%s"),
		*GetNameSafe(this),
		bFocusNavigation ? 1 : 0,
		ActiveNavigationIndex,
		*GetNameSafe(FocusWidgetSwitcher),
		*GetNameSafe(GetActivePageWidget()));

	SetCurrentFocusZone(EGameUIFocusZone::Navigation);

	if (UWidget* ActivePage = GetActiveFocusPageWidget())
	{
		if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
		{
			NativeFocusPage->SetOwningFocusScreen(this);
		}

		if (ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
		{
			IGameUIFocusPageInterface::Execute_OnPageActivated(ActivePage);
		}
		else if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
		{
			NativeFocusPage->OnPageActivated_Implementation();
		}
	}

	return !bFocusNavigation || SetNavigationFocusByIndex(ActiveNavigationIndex);
}

void UGameUIFocusScreenWidgetBase::RegisterNavigationWidget(UWidget* Widget)
{
	RegisterNavigationEntry(Widget, NavigationEntries.Num());
}

void UGameUIFocusScreenWidgetBase::RegisterNavigationEntry(UWidget* Widget, int32 PageIndex)
{
	if (!Widget)
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace RegisterNavigationRejected Screen=%s PageIndex=%d Reason=NullWidget"),
			*GetNameSafe(this),
			PageIndex);
		return;
	}

	if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget))
	{
		if (!FocusItem->IsFocusable())
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			UE_LOG(LogGameUIFocus, Warning,
				TEXT("GameUIFocus screen rejected non-focusable navigation item. Screen=%s Item=%s."),
				*GetNameSafe(this),
				*GetNameSafe(FocusItem));
#endif
			return;
		}

		FocusItem->SetOwningNavigationScreen(this);
		FocusItem->SetPointerInputActive(bPointerInputActive);
	}

	for (FGameUIFocusNavigationEntry& ExistingEntry : NavigationEntries)
	{
		if (ExistingEntry.NavigationWidget == Widget)
		{
			ExistingEntry.PageIndex = PageIndex;
			return;
		}
	}

	FGameUIFocusNavigationEntry Entry;
	Entry.NavigationWidget = Widget;
	Entry.PageIndex = PageIndex;
	NavigationEntries.Add(Entry);
}

void UGameUIFocusScreenWidgetBase::SetNavigationWidgets(const TArray<UWidget*>& Widgets)
{
	ClearNavigationEntries();
	for (UWidget* Widget : Widgets)
	{
		RegisterNavigationWidget(Widget);
	}
}

void UGameUIFocusScreenWidgetBase::SetNavigationEntries(const TArray<FGameUIFocusNavigationEntry>& Entries)
{
	const TArray<FGameUIFocusNavigationEntry> EntriesCopy = Entries;
	ClearNavigationEntries();

	for (const FGameUIFocusNavigationEntry& Entry : EntriesCopy)
	{
		if (Entry.NavigationWidget)
		{
			RegisterNavigationEntry(Entry.NavigationWidget, Entry.PageIndex);
		}
	}
}

void UGameUIFocusScreenWidgetBase::SetFocusWidgetSwitcher(UWidgetSwitcher* InWidgetSwitcher)
{
	FocusWidgetSwitcher = InWidgetSwitcher;
}

bool UGameUIFocusScreenWidgetBase::SwitchToPageIndex(int32 PageIndex, bool bFocusNavigation)
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace SwitchToPage Screen=%s Page=%d bFocusNavigation=%d Zone=%s ActiveBefore=%d"),
		*GetNameSafe(this),
		PageIndex,
		bFocusNavigation ? 1 : 0,
		*FocusZoneToString(CurrentFocusZone),
		FocusWidgetSwitcher ? FocusWidgetSwitcher->GetActiveWidgetIndex() : INDEX_NONE);

	if (!FocusWidgetSwitcher || PageIndex < 0 || PageIndex >= FocusWidgetSwitcher->GetNumWidgets())
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace SwitchToPageRejected Screen=%s Page=%d Switcher=%s Num=%d"),
			*GetNameSafe(this),
			PageIndex,
			*GetNameSafe(FocusWidgetSwitcher),
			FocusWidgetSwitcher ? FocusWidgetSwitcher->GetNumWidgets() : 0);
		return false;
	}

	if (FocusWidgetSwitcher->GetActiveWidgetIndex() == PageIndex)
	{
		if (bFocusNavigation)
		{
			if (CurrentFocusZone == EGameUIFocusZone::Content)
			{
				UE_LOG(LogGameUIFocus, VeryVerbose,
					TEXT("GameUIFocusTrace SwitchToPageIgnoredNavigationRefocus Screen=%s Page=%d"),
					*GetNameSafe(this),
					PageIndex);
				return true;
			}
			return SetNavigationFocusByIndex(FindNavigationIndexForPageIndex(PageIndex));
		}
		return true;
	}

	if (UWidget* OldPage = GetActiveFocusPageWidget())
	{
		if (OldPage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
		{
			IGameUIFocusPageInterface::Execute_OnPageDeactivated(OldPage);
		}
		else if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(OldPage))
		{
			NativeFocusPage->OnPageDeactivated_Implementation();
		}
	}

	FocusWidgetSwitcher->SetActiveWidgetIndex(PageIndex);
	const int32 NewNavigationIndex = FindNavigationIndexForPageIndex(PageIndex);
	ModalFocusStack.Reset();
	LastFocusedWidget.Reset();

	if (!bFocusNavigation)
	{
		SetActiveNavigationIndex(NewNavigationIndex);
		SetCurrentFocusZone(EGameUIFocusZone::Navigation);
	}

	if (UWidget* NewPage = GetActiveFocusPageWidget())
	{
		if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(NewPage))
		{
			NativeFocusPage->SetOwningFocusScreen(this);
		}

		if (NewPage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
		{
			IGameUIFocusPageInterface::Execute_OnPageActivated(NewPage);
		}
		else if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(NewPage))
		{
			NativeFocusPage->OnPageActivated_Implementation();
		}
	}

	if (bFocusNavigation)
	{
		SetNavigationFocusByIndex(NewNavigationIndex);
	}

	return true;
}

bool UGameUIFocusScreenWidgetBase::ActivateNavigationEntryByIndex(int32 NavigationIndex, bool bEnterContent)
{
	if (!NavigationEntries.IsValidIndex(NavigationIndex))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace ActivateNavigationRejected Screen=%s NavIndex=%d Reason=InvalidIndex Entries=%d"),
			*GetNameSafe(this),
			NavigationIndex,
			NavigationEntries.Num());
		return false;
	}

	UWidget* NavigationWidget = GetNavigationWidgetByIndex(NavigationIndex);
	if (!IsUsableFocusTarget(NavigationWidget))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace ActivateNavigationRejected Screen=%s NavIndex=%d Widget=%s Reason=Unusable"),
			*GetNameSafe(this),
			NavigationIndex,
			*GetNameSafe(NavigationWidget));
		return false;
	}

	const int32 PageIndex = GetPageIndexForNavigationIndex(NavigationIndex);
	const bool bHasValidPage = FocusWidgetSwitcher
		&& PageIndex >= 0
		&& PageIndex < FocusWidgetSwitcher->GetNumWidgets();

	if (bHasValidPage && !SwitchToPageIndex(PageIndex, false))
	{
		return false;
	}

	if (!SetNavigationFocusByIndex(NavigationIndex))
	{
		return false;
	}

	if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(NavigationWidget))
	{
		FocusItem->ActivateFocusItem();
	}

	if (bEnterContent && bHasValidPage)
	{
		return EnterContentZone();
	}

	return true;
}

bool UGameUIFocusScreenWidgetBase::ActivateNavigationWidget(UWidget* NavigationWidget, bool bEnterContent)
{
	const int32 NavigationIndex = FindNavigationIndexForWidget(NavigationWidget);
	return NavigationIndex != INDEX_NONE
		&& ActivateNavigationEntryByIndex(NavigationIndex, bEnterContent);
}

bool UGameUIFocusScreenWidgetBase::ActivateCurrentNavigationEntry(bool bEnterContent)
{
	return ActivateNavigationEntryByIndex(ActiveNavigationIndex, bEnterContent);
}

bool UGameUIFocusScreenWidgetBase::EnterContentZone()
{
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace EnterContentStart Screen=%s Zone=%s ActiveNav=%d ActiveSwitcherWidget=%s ActiveFocusPage=%s"),
		*GetNameSafe(this),
		*FocusZoneToString(CurrentFocusZone),
		ActiveNavigationIndex,
		*GetNameSafe(GetActivePageWidget()),
		*GetNameSafe(GetActiveFocusPageWidget()));

	if (!FocusWidgetSwitcher)
	{
		LogContentFocusFailure(TEXT("missing FocusWidgetSwitcher"));
		return false;
	}

	UWidget* ActiveSwitcherWidget = GetActivePageWidget();
	if (!ActiveSwitcherWidget)
	{
		LogContentFocusFailure(TEXT("FocusWidgetSwitcher has no active widget"));
		return false;
	}

	UWidget* ActivePage = GetActiveFocusPageWidget();
	if (!ActivePage)
	{
		LogContentFocusFailure(TEXT("active switcher widget does not contain a focus page"), ActiveSwitcherWidget);
		return false;
	}

	bool bEnteredPageFocus = false;
	if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
	{
		NativeFocusPage->SetOwningFocusScreen(this);
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace EnterContentNativePage Screen=%s Page=%s BestTarget=%s RegisteredItems=%d"),
			*GetNameSafe(this),
			*GetNameSafe(ActivePage),
			*GetNameSafe(NativeFocusPage->GetBestFocusTarget()),
			NativeFocusPage->GetRegisteredFocusItems().Num());
		if (!ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
		{
			bEnteredPageFocus = NativeFocusPage->EnterPageFocus_Implementation();
		}
	}

	if (!bEnteredPageFocus && ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
	{
		bEnteredPageFocus = IGameUIFocusPageInterface::Execute_EnterPageFocus(ActivePage);
	}

	if (!bEnteredPageFocus)
	{
		if (const UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
		{
			UWidget* DefaultFocusWidget = nullptr;
			UWidget* LastFocusWidget = nullptr;
			if (ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
			{
				DefaultFocusWidget = IGameUIFocusPageInterface::Execute_GetDefaultFocusWidget(ActivePage);
				LastFocusWidget = IGameUIFocusPageInterface::Execute_GetLastFocusWidget(ActivePage);
			}
			else
			{
				DefaultFocusWidget = NativeFocusPage->GetDefaultFocusWidget_Implementation();
				LastFocusWidget = NativeFocusPage->GetLastFocusWidget_Implementation();
			}

			UE_LOG(LogGameUIFocus, Warning,
				TEXT("GameUIFocusScreen '%s' could not enter content focus. Page='%s' Default='%s' Last='%s' RegisteredItems=%d."),
				*GetNameSafe(this),
				*GetNameSafe(ActivePage),
				*GetNameSafe(DefaultFocusWidget),
				*GetNameSafe(LastFocusWidget),
				NativeFocusPage->GetRegisteredFocusItems().Num());
		}
		else
		{
			LogContentFocusFailure(TEXT("focus page rejected EnterPageFocus"), ActivePage);
		}
	}

	return bEnteredPageFocus;
}

bool UGameUIFocusScreenWidgetBase::ReturnToNavigationZone()
{
	UWidget* NavigationWidget = GetNavigationWidgetByIndex(ActiveNavigationIndex);
	if (!NavigationWidget)
	{
		return false;
	}

	return RequestFocusNextTickInternal(NavigationWidget, true, EGameUIFocusZone::Navigation, ActiveNavigationIndex, true);
}

bool UGameUIFocusScreenWidgetBase::EnterModalZone(UWidget* ModalFocusWidget)
{
	return EnterModalZoneWithReturnFocus(ModalFocusWidget, LastFocusedWidget.Get());
}

bool UGameUIFocusScreenWidgetBase::EnterModalZoneWithReturnFocus(UWidget* ModalFocusWidget, UWidget* ReturnFocusWidget)
{
	if (!IsUsableFocusTarget(ModalFocusWidget))
	{
		return false;
	}

	FGameUIFocusStateSnapshot Snapshot;
	Snapshot.Zone = CurrentFocusZone;
	Snapshot.FocusWidget = IsUsableFocusTarget(ReturnFocusWidget) ? ReturnFocusWidget : LastFocusedWidget.Get();
	ModalFocusStack.Add(Snapshot);

	return RequestFocusNextTickForZone(ModalFocusWidget, EGameUIFocusZone::Modal);
}

bool UGameUIFocusScreenWidgetBase::ReturnFromModalZone()
{
	if (CurrentFocusZone != EGameUIFocusZone::Modal || ModalFocusStack.Num() <= 0)
	{
		return false;
	}

	const FGameUIFocusStateSnapshot Snapshot = ModalFocusStack.Pop(EAllowShrinking::No);

	if (IsUsableFocusTarget(Snapshot.FocusWidget.Get()))
	{
		return RequestFocusNextTickForZone(Snapshot.FocusWidget.Get(), Snapshot.Zone);
	}

	if (Snapshot.Zone == EGameUIFocusZone::Content)
	{
		return EnterContentZone();
	}

	return SetNavigationFocusByIndex(ActiveNavigationIndex);
}

bool UGameUIFocusScreenWidgetBase::SetNavigationFocusByIndex(int32 NavigationIndex)
{
	UWidget* NavigationWidget = GetNavigationWidgetByIndex(NavigationIndex);
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace SetNavigationFocus Screen=%s Index=%d Widget=%s Zone=%s"),
		*GetNameSafe(this),
		NavigationIndex,
		*GetNameSafe(NavigationWidget),
		*FocusZoneToString(CurrentFocusZone));
	if (!NavigationWidget)
	{
		return false;
	}

	return RequestFocusNextTickInternal(NavigationWidget, true, EGameUIFocusZone::Navigation, NavigationIndex);
}

bool UGameUIFocusScreenWidgetBase::MoveNavigationFocus(int32 Direction)
{
	const int32 EntryCount = GetNavigationEntryCount();
	if (EntryCount <= 0 || Direction == 0)
	{
		return false;
	}

	const int32 NewIndex = FMath::Clamp(ActiveNavigationIndex + FMath::Sign(Direction), 0, EntryCount - 1);
	if (NewIndex == ActiveNavigationIndex)
	{
		return false;
	}

	const int32 PageIndex = GetPageIndexForNavigationIndex(NewIndex);
	if (bSwitchPageWithNavigationFocus && FocusWidgetSwitcher && PageIndex >= 0 && PageIndex < FocusWidgetSwitcher->GetNumWidgets())
	{
		const bool bSwitched = SwitchToPageIndex(PageIndex, false);
		return bSwitched && SetNavigationFocusByIndex(NewIndex);
	}

	return SetNavigationFocusByIndex(NewIndex);
}

bool UGameUIFocusScreenWidgetBase::HandleNavigationWidgetAnalogInput(
	UWidget* NavigationWidget,
	const FKey Key,
	const float Value)
{
	const int32 NavigationIndex = FindNavigationIndexForWidget(NavigationWidget);
	if (CurrentFocusZone != EGameUIFocusZone::Navigation
		|| NavigationIndex == INDEX_NONE
		|| NavigationIndex != ActiveNavigationIndex)
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace NavigationItemAnalogRejected Screen=%s Widget=%s Key=%s Value=%.3f Zone=%s ItemIndex=%d ActiveIndex=%d"),
			*GetNameSafe(this),
			*GetNameSafe(NavigationWidget),
			*Key.ToString(),
			Value,
			*FocusZoneToString(CurrentFocusZone),
			NavigationIndex,
			ActiveNavigationIndex);
		return false;
	}

	return ProcessNavigationAnalogInput(Key, Value);
}

bool UGameUIFocusScreenWidgetBase::HandleNavigationWidgetDigitalInput(
	UWidget* NavigationWidget,
	FIntPoint Direction,
	const bool bIsRepeat)
{
	Direction.X = FMath::Clamp(Direction.X, -1, 1);
	Direction.Y = FMath::Clamp(Direction.Y, -1, 1);
	const int32 NavigationIndex = FindNavigationIndexForWidget(NavigationWidget);
	if (CurrentFocusZone != EGameUIFocusZone::Navigation
		|| NavigationIndex == INDEX_NONE
		|| NavigationIndex != ActiveNavigationIndex
		|| Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	if (Direction.Y != 0)
	{
		if (CanProcessNavigationMove(bIsRepeat))
		{
			MoveNavigationFocus(Direction.Y);
		}
		return true;
	}

	if (Direction.X > 0)
	{
		ActivateCurrentNavigationEntry(true);
		return true;
	}

	return false;
}

bool UGameUIFocusScreenWidgetBase::RequestFocusNextTick(UWidget* Widget)
{
	return RequestFocusNextTickInternal(Widget, false, CurrentFocusZone, INDEX_NONE);
}

bool UGameUIFocusScreenWidgetBase::RequestFocusNextTickForZone(UWidget* Widget, EGameUIFocusZone Zone)
{
	return RequestFocusNextTickInternal(Widget, true, Zone, INDEX_NONE);
}

void UGameUIFocusScreenWidgetBase::RememberFocusedWidget(UWidget* Widget)
{
	if (IsUsableFocusTarget(Widget))
	{
		LastFocusedWidget = Widget;
	}
}

void UGameUIFocusScreenWidgetBase::NotifyContentWidgetFocused(UWidget* Widget)
{
	if (!IsUsableFocusTarget(Widget))
	{
		return;
	}

	LastFocusedWidget = Widget;
	SetCurrentFocusZone(EGameUIFocusZone::Content);
}

void UGameUIFocusScreenWidgetBase::NotifyNavigationWidgetFocused(UWidget* Widget)
{
	if (!IsUsableFocusTarget(Widget))
	{
		return;
	}

	const int32 NavigationIndex = FindNavigationIndexForWidget(Widget);
	if (NavigationIndex == INDEX_NONE)
	{
		return;
	}

	LastFocusedWidget = Widget;
	SetActiveNavigationIndex(NavigationIndex);
	SetCurrentFocusZone(EGameUIFocusZone::Navigation);

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace NavigationWidgetFocused Screen=%s Widget=%s NavIndex=%d"),
		*GetNameSafe(this),
		*GetNameSafe(Widget),
		NavigationIndex);
}

bool UGameUIFocusScreenWidgetBase::RequestFocusNextTickInternal(UWidget* Widget, bool bApplyZone, EGameUIFocusZone Zone, int32 NavigationIndexToApply, bool bLeaveActivePageOnSuccess)
{
	if (!IsUsableFocusTarget(Widget))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace FocusRequestRejected Screen=%s Target=%s ApplyZone=%d Zone=%s Reason=Unusable Enabled=%d Visibility=%d"),
			*GetNameSafe(this),
			*GetNameSafe(Widget),
			bApplyZone ? 1 : 0,
			*FocusZoneToString(Zone),
			Widget ? (Widget->GetIsEnabled() ? 1 : 0) : 0,
			Widget ? static_cast<int32>(Widget->GetVisibility()) : -1);
		return false;
	}

	WarnIfWeakFocusTarget(Widget, Zone);

	const TWeakObjectPtr<UWidget> FocusWidget = Widget;
	const TWeakObjectPtr<UGameUIFocusScreenWidgetBase> WeakThis = this;
	const uint64 RequestSerial = ++FocusRequestSerial;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace FocusRequestScheduled Screen=%s Serial=%llu Target=%s ApplyZone=%d Zone=%s NavIndex=%d LeavePage=%d"),
		*GetNameSafe(this),
		RequestSerial,
		*GetNameSafe(Widget),
		bApplyZone ? 1 : 0,
		*FocusZoneToString(Zone),
		NavigationIndexToApply,
		bLeaveActivePageOnSuccess ? 1 : 0);

	if (TryApplyPlayerFocus(Widget))
	{
		FinalizeFocusRequest(Widget, bApplyZone, Zone, NavigationIndexToApply, bLeaveActivePageOnSuccess);
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace FocusConfirmedImmediate Screen=%s Serial=%llu Target=%s Zone=%s"),
			*GetNameSafe(this),
			RequestSerial,
			*GetNameSafe(Widget),
			*FocusZoneToString(Zone));
		return true;
	}

	UWorld* World = GetWorld();
	if (!GetOwningPlayer() || !World)
	{
		const bool bKeyboardFocused = TryApplyKeyboardFocus(Widget);
		if (bKeyboardFocused)
		{
			FinalizeFocusRequest(Widget, bApplyZone, Zone, NavigationIndexToApply, bLeaveActivePageOnSuccess);
		}
		return bKeyboardFocused;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, FocusWidget, RequestSerial, bApplyZone, Zone, NavigationIndexToApply, bLeaveActivePageOnSuccess]()
	{
		UGameUIFocusScreenWidgetBase* FocusScreen = WeakThis.Get();
		UWidget* FocusTarget = FocusWidget.Get();
		if (FocusScreen
			&& FocusScreen->FocusRequestSerial == RequestSerial
			&& FocusScreen->IsRetryTargetValid(FocusTarget, Zone))
		{
			const bool bFocused = FocusScreen->TryApplyPlayerFocus(FocusTarget)
				|| FocusScreen->TryApplyKeyboardFocus(FocusTarget);
			if (bFocused)
			{
				FocusScreen->FinalizeFocusRequest(
					FocusTarget,
					bApplyZone,
					Zone,
					NavigationIndexToApply,
					bLeaveActivePageOnSuccess);
			}

			UE_LOG(LogGameUIFocus, VeryVerbose,
				TEXT("GameUIFocusTrace FocusRetryCompleted Screen=%s Serial=%llu Target=%s Confirmed=%d"),
				*GetNameSafe(FocusScreen),
				RequestSerial,
				*GetNameSafe(FocusTarget),
				bFocused ? 1 : 0);
		}
		else
		{
			UE_LOG(LogGameUIFocus, VeryVerbose,
				TEXT("GameUIFocusTrace FocusApplySkipped Screen=%s Serial=%llu CurrentSerial=%llu Target=%s ValidScreen=%d ValidTarget=%d"),
				*GetNameSafe(FocusScreen),
				RequestSerial,
				FocusScreen ? FocusScreen->FocusRequestSerial : 0,
				*GetNameSafe(FocusTarget),
				FocusScreen ? 1 : 0,
				FocusTarget ? 1 : 0);
		}
	}));

	return true;
}

bool UGameUIFocusScreenWidgetBase::TryApplyPlayerFocus(UWidget* Widget) const
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!Widget || !PlayerController)
	{
		return false;
	}

	Widget->SetUserFocus(PlayerController);
	return Widget->HasUserFocus(PlayerController);
}

bool UGameUIFocusScreenWidgetBase::TryApplyKeyboardFocus(UWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	Widget->SetKeyboardFocus();
	return Widget->HasKeyboardFocus();
}

void UGameUIFocusScreenWidgetBase::FinalizeFocusRequest(
	UWidget* Widget,
	const bool bApplyZone,
	const EGameUIFocusZone Zone,
	const int32 NavigationIndexToApply,
	const bool bLeaveActivePageOnSuccess)
{
	if (bApplyZone)
	{
		SetCurrentFocusZone(Zone);
	}
	if (NavigationIndexToApply != INDEX_NONE)
	{
		SetActiveNavigationIndex(NavigationIndexToApply);
	}

	RememberFocusedWidget(Widget);
	if (bLeaveActivePageOnSuccess)
	{
		LeaveActivePageFocus();
	}
}

void UGameUIFocusScreenWidgetBase::ClearNavigationEntries()
{
	for (const FGameUIFocusNavigationEntry& Entry : NavigationEntries)
	{
		if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Entry.NavigationWidget))
		{
			if (FocusItem->GetOwningNavigationScreen() == this)
			{
				FocusItem->SetOwningNavigationScreen(nullptr);
			}
		}
	}

	NavigationEntries.Reset();
}

void UGameUIFocusScreenWidgetBase::RebuildNavigationEntriesFromBindings()
{
	if (NavigationBindings.IsEmpty())
	{
		return;
	}

	ClearNavigationEntries();
	for (const FGameUIFocusNavigationBinding& Binding : NavigationBindings)
	{
		if (Binding.WidgetName.IsNone())
		{
			continue;
		}

		UWidget* NavigationWidget = FindWidgetByNameRecursive(this, Binding.WidgetName);
		if (!NavigationWidget)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			UE_LOG(LogGameUIFocus, Warning,
				TEXT("GameUIFocus screen could not resolve navigation binding. Screen=%s WidgetName=%s PageIndex=%d."),
				*GetNameSafe(this),
				*Binding.WidgetName.ToString(),
				Binding.PageIndex);
#endif
			continue;
		}

		RegisterNavigationEntry(NavigationWidget, Binding.PageIndex);
	}
}

void UGameUIFocusScreenWidgetBase::NotifyActivePageDeactivated()
{
	UWidget* ActivePage = GetActiveFocusPageWidget();
	if (!ActivePage)
	{
		return;
	}

	if (ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
	{
		IGameUIFocusPageInterface::Execute_OnPageDeactivated(ActivePage);
		return;
	}

	if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
	{
		NativeFocusPage->OnPageDeactivated_Implementation();
	}
}

bool UGameUIFocusScreenWidgetBase::IsRetryTargetValid(UWidget* Widget, const EGameUIFocusZone Zone) const
{
	if (!IsUsableFocusTarget(Widget))
	{
		return false;
	}

	if (Zone == EGameUIFocusZone::Content)
	{
		const UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
		const UGameUIFocusPageWidgetBase* ActivePage = Cast<UGameUIFocusPageWidgetBase>(GetActiveFocusPageWidget());
		return FocusItem && ActivePage && FocusItem->GetOwningFocusPage() == ActivePage;
	}

	if (Zone == EGameUIFocusZone::Navigation)
	{
		return FindNavigationIndexForWidget(Widget) != INDEX_NONE;
	}

	return true;
}

void UGameUIFocusScreenWidgetBase::LeaveActivePageFocus()
{
	if (UWidget* ActivePage = GetActiveFocusPageWidget())
	{
		if (ActivePage->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()))
		{
			IGameUIFocusPageInterface::Execute_LeavePageFocus(ActivePage);
		}
		else if (UGameUIFocusPageWidgetBase* NativeFocusPage = Cast<UGameUIFocusPageWidgetBase>(ActivePage))
		{
			NativeFocusPage->LeavePageFocus_Implementation();
		}
	}
}

UWidget* UGameUIFocusScreenWidgetBase::GetActivePageWidget() const
{
	return FocusWidgetSwitcher ? FocusWidgetSwitcher->GetActiveWidget() : nullptr;
}

UWidget* UGameUIFocusScreenWidgetBase::GetActiveFocusPageWidget() const
{
	return FindFocusPageWidget(GetActivePageWidget());
}

FReply UGameUIFocusScreenWidgetBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key.IsGamepadKey())
	{
		NotifyGamepadInput();
	}

	const EUINavigation NavigationDirection = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent)
		: EUINavigation::Invalid;
	const bool bAcceptAction = GameUIFocusInputKeys::IsAcceptAction(InKeyEvent);
	const bool bBackAction = GameUIFocusInputKeys::IsBackAction(InKeyEvent);
	if (IsFocusTraceKey(Key))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PreviewKey Screen=%s Key=%s Zone=%s ActiveNav=%d ActiveSwitcherWidget=%s ActiveFocusPage=%s Last=%s"),
			*GetNameSafe(this),
			*Key.ToString(),
			*FocusZoneToString(CurrentFocusZone),
			ActiveNavigationIndex,
			*GetNameSafe(GetActivePageWidget()),
			*GetNameSafe(GetActiveFocusPageWidget()),
			*GetNameSafe(LastFocusedWidget.Get()));
	}

	if (CurrentFocusZone == EGameUIFocusZone::Navigation)
	{
		if (HandleNavigationZoneKey(Key))
		{
			return FReply::Handled();
		}

		if (NavigationDirection == EUINavigation::Up)
		{
			if (CanProcessNavigationMove(InKeyEvent.IsRepeat()))
			{
				MoveNavigationFocus(-1);
			}
			return FReply::Handled();
		}

		if (NavigationDirection == EUINavigation::Down)
		{
			if (CanProcessNavigationMove(InKeyEvent.IsRepeat()))
			{
				MoveNavigationFocus(1);
			}
			return FReply::Handled();
		}

		if ((NavigationDirection == EUINavigation::Right || bAcceptAction)
			&& ActivateCurrentNavigationEntry(true))
		{
			return FReply::Handled();
		}
	}
	else if (CurrentFocusZone == EGameUIFocusZone::Content)
	{
		if (HandleContentZoneKey(Key))
		{
			return FReply::Handled();
		}

		if (bBackAction && ReturnToNavigationZone())
		{
			return FReply::Handled();
		}
	}
	else if (CurrentFocusZone == EGameUIFocusZone::Modal)
	{
		if (HandleModalZoneKey(Key))
		{
			return FReply::Handled();
		}

		if (bBackAction && ReturnFromModalZone())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusScreenWidgetBase::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey().IsGamepadKey()
		&& FMath::Abs(InAnalogEvent.GetAnalogValue()) >= GamepadActivationThreshold)
	{
		NotifyGamepadInput(InAnalogEvent.GetAnalogValue());
	}

	return ProcessNavigationAnalogInput(InAnalogEvent.GetKey(), InAnalogEvent.GetAnalogValue())
		? FReply::Handled()
		: Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UGameUIFocusScreenWidgetBase::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	NotifyPointerInput();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGameUIFocusScreenWidgetBase::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetCursorDelta().SizeSquared()
		>= FMath::Square(MouseMoveActivationThreshold))
	{
		NotifyPointerInput();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UGameUIFocusScreenWidgetBase::NativeOnMouseWheel(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!FMath::IsNearlyZero(InMouseEvent.GetWheelDelta()))
	{
		NotifyPointerInput();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UGameUIFocusScreenWidgetBase::NotifyGamepadInput(const float InputStrength)
{
	if (FMath::Abs(InputStrength) < GamepadActivationThreshold)
	{
		return;
	}

	GameUIFocusInputDeviceTracker::SetPointerInputActive(false);
	SetPointerInputActive(false);
}

void UGameUIFocusScreenWidgetBase::NotifyPointerInput()
{
	GameUIFocusInputDeviceTracker::SetPointerInputActive(true);
	SetPointerInputActive(true);
}

void UGameUIFocusScreenWidgetBase::SetPointerInputActive(const bool bActive)
{
	const bool bInputDeviceChanged = bPointerInputActive != bActive;
	bPointerInputActive = bActive;
	ApplyMouseCursorVisibility();
	SchedulePointerInputStateReapply();
	if (!bInputDeviceChanged)
	{
		return;
	}

	RefreshPointerInteractionState();
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace InputDeviceChanged Screen=%s PointerActive=%d CursorManaged=%d"),
		*GetNameSafe(this),
		bPointerInputActive ? 1 : 0,
		bManageMouseCursorForInputDevice ? 1 : 0);
}

void UGameUIFocusScreenWidgetBase::HandleGlobalPointerInputStateChanged(const bool bActive)
{
	SetPointerInputActive(bActive);
}

void UGameUIFocusScreenWidgetBase::ApplyMouseCursorVisibility() const
{
	if (!bManageMouseCursorForInputDevice)
	{
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->bShowMouseCursor = bPointerInputActive;
	}
}

void UGameUIFocusScreenWidgetBase::SchedulePointerInputStateReapply()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const uint64 ReapplySerial = ++PointerInputReapplySerial;
	const TWeakObjectPtr<UGameUIFocusScreenWidgetBase> WeakThis = this;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
		[WeakThis, ReapplySerial]()
		{
			UGameUIFocusScreenWidgetBase* FocusScreen = WeakThis.Get();
			if (!FocusScreen || FocusScreen->PointerInputReapplySerial != ReapplySerial)
			{
				return;
			}

			FocusScreen->ApplyMouseCursorVisibility();
			FocusScreen->RefreshPointerInteractionState();
		}));
}

void UGameUIFocusScreenWidgetBase::RefreshPointerInteractionState()
{
	TSet<UWidget*> VisitedWidgets;
	TFunction<void(UWidget*)> VisitWidget;
	VisitWidget = [this, &VisitedWidgets, &VisitWidget](UWidget* Widget)
	{
		if (!Widget || VisitedWidgets.Contains(Widget))
		{
			return;
		}

		VisitedWidgets.Add(Widget);
		if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget))
		{
			FocusItem->SetPointerInputActive(bPointerInputActive);
		}

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
			{
				VisitWidget(Panel->GetChildAt(ChildIndex));
			}
		}

		if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
		{
			if (UWidgetTree* Tree = UserWidget->WidgetTree)
			{
				Tree->ForEachWidget([&VisitWidget](UWidget* ChildWidget)
				{
					VisitWidget(ChildWidget);
				});
			}
		}
	};

	VisitWidget(this);
}

bool UGameUIFocusScreenWidgetBase::ProcessNavigationAnalogInput(const FKey Key, const float Value)
{
	if (CurrentFocusZone != EGameUIFocusZone::Navigation)
	{
		ResetAnalogNavigation();
		return false;
	}

	TryMigrateLegacyAnalogConfig();
	const FGameUIAnalogNavigationResult Result = AnalogNavigationState.ProcessAxis(
		Key,
		Value,
		FPlatformTime::Seconds(),
		AnalogNavigationConfig,
		EGameUIAnalogNavigationMode::TwoDimensional);
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace ScreenAnalogResult Screen=%s Key=%s Value=%.3f Handled=%d Navigate=%d Direction=(%d,%d) Magnitude=%.3f Held=%d DeadZone=%.3f Release=%.3f"),
		*GetNameSafe(this),
		*Key.ToString(),
		Value,
		Result.bHandled ? 1 : 0,
		Result.bShouldNavigate ? 1 : 0,
		Result.Direction.X,
		Result.Direction.Y,
		Result.Magnitude,
		AnalogNavigationState.IsHeld() ? 1 : 0,
		AnalogNavigationConfig.DeadZone,
		AnalogNavigationConfig.ReleaseThreshold);
	if (!Result.bHandled)
	{
		return false;
	}

	if (!Result.bShouldNavigate)
	{
		return true;
	}

	bool bMoved = false;
	if (Result.Direction.Y != 0)
	{
		bMoved = MoveNavigationFocus(Result.Direction.Y);
	}
	else if (Result.Direction.X > 0)
	{
		UGameUIFocusPageWidgetBase* ActiveFocusPage = Cast<UGameUIFocusPageWidgetBase>(GetActiveFocusPageWidget());
		if (ActiveFocusPage)
		{
			// Seed the page before focus can transfer immediately or on the next tick.
			// A focused value row will then require this same stick gesture to release
			// before accepting horizontal value changes.
			ActiveFocusPage->UpdateHorizontalAnalogSample(AnalogNavigationState.GetStickValue().X);
		}

		bMoved = EnterContentZone();
		if (!bMoved && ActiveFocusPage)
		{
			ActiveFocusPage->ResetHorizontalAnalogSample();
		}
	}

	UWidget* CurrentWidget = GetNavigationWidgetByIndex(ActiveNavigationIndex);
	if (bMoved)
	{
		AnalogNavigationState.NotifyNavigationSucceeded();
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace AnalogAccepted Owner=Screen Screen=%s Widget=%s Direction=(%d,%d) Magnitude=%.3f Repeat=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentWidget),
			Result.Direction.X,
			Result.Direction.Y,
			Result.Magnitude,
			Result.bIsRepeat ? 1 : 0);
	}
	else
	{
		const bool bShouldBroadcastBlocked = AnalogNavigationState.NotifyNavigationBlocked();
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace AnalogBlocked Owner=Screen Screen=%s Widget=%s Direction=(%d,%d) Magnitude=%.3f Repeat=%d Feedback=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentWidget),
			Result.Direction.X,
			Result.Direction.Y,
			Result.Magnitude,
			Result.bIsRepeat ? 1 : 0,
			bShouldBroadcastBlocked ? 1 : 0);
		if (bShouldBroadcastBlocked)
		{
			OnNavigationBlocked.Broadcast(CurrentWidget, Result.Direction);
		}
	}

	return true;
}

bool UGameUIFocusScreenWidgetBase::HandleNavigationZoneKey_Implementation(FKey Key)
{
	return false;
}

bool UGameUIFocusScreenWidgetBase::HandleContentZoneKey_Implementation(FKey Key)
{
	return false;
}

bool UGameUIFocusScreenWidgetBase::HandleModalZoneKey_Implementation(FKey Key)
{
	return false;
}

void UGameUIFocusScreenWidgetBase::HandleFocusZoneChanged_Implementation(EGameUIFocusZone PreviousZone, EGameUIFocusZone NewZone)
{
}

void UGameUIFocusScreenWidgetBase::HandleNavigationIndexChanged_Implementation(int32 PreviousIndex, int32 NewIndex)
{
}

void UGameUIFocusScreenWidgetBase::SetCurrentFocusZone(EGameUIFocusZone NewZone)
{
	if (CurrentFocusZone == NewZone)
	{
		return;
	}

	const EGameUIFocusZone PreviousZone = CurrentFocusZone;
	if (UGameUIFocusPageWidgetBase* ActivePage = Cast<UGameUIFocusPageWidgetBase>(GetActiveFocusPageWidget()))
	{
		// Blueprint interface overrides are not required to call their native parent, so
		// reset the stable page state at the owning screen's authoritative zone boundary.
		ActivePage->ResetAnalogNavigation();
	}
	ResetAnalogNavigation();
	CurrentFocusZone = NewZone;
	OnFocusZoneChanged.Broadcast(PreviousZone, NewZone);
	HandleFocusZoneChanged(PreviousZone, NewZone);
}

void UGameUIFocusScreenWidgetBase::SetActiveNavigationIndex(int32 NewIndex)
{
	if (ActiveNavigationIndex == NewIndex)
	{
		return;
	}

	const int32 PreviousIndex = ActiveNavigationIndex;
	ActiveNavigationIndex = NewIndex;
	OnNavigationIndexChanged.Broadcast(PreviousIndex, NewIndex);
	HandleNavigationIndexChanged(PreviousIndex, NewIndex);
}

int32 UGameUIFocusScreenWidgetBase::GetPageIndexForNavigationIndex(int32 NavigationIndex) const
{
	if (NavigationEntries.IsValidIndex(NavigationIndex))
	{
		return NavigationEntries[NavigationIndex].PageIndex;
	}

	return NavigationIndex;
}

int32 UGameUIFocusScreenWidgetBase::FindNavigationIndexForWidget(const UWidget* Widget) const
{
	if (!Widget)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < NavigationEntries.Num(); ++Index)
	{
		if (NavigationEntries[Index].NavigationWidget == Widget)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

int32 UGameUIFocusScreenWidgetBase::FindNavigationIndexForPageIndex(int32 PageIndex) const
{
	for (int32 Index = 0; Index < NavigationEntries.Num(); ++Index)
	{
		if (NavigationEntries[Index].PageIndex == PageIndex)
		{
			return Index;
		}
	}

	return PageIndex;
}

UWidget* UGameUIFocusScreenWidgetBase::GetNavigationWidgetByIndex(int32 NavigationIndex) const
{
	if (NavigationEntries.IsValidIndex(NavigationIndex))
	{
		return NavigationEntries[NavigationIndex].NavigationWidget.Get();
	}

	return nullptr;
}

int32 UGameUIFocusScreenWidgetBase::GetNavigationEntryCount() const
{
	return NavigationEntries.Num();
}

bool UGameUIFocusScreenWidgetBase::IsUsableFocusTarget(const UWidget* Widget)
{
	if (!IsValid(Widget) || !Widget->GetIsEnabled())
	{
		return false;
	}

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget); UserWidget && !UserWidget->IsFocusable())
	{
		return false;
	}

	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed
		&& Visibility != ESlateVisibility::Hidden;
}

bool UGameUIFocusScreenWidgetBase::IsFocusPageWidget(const UWidget* Widget)
{
	return Widget
		&& (Widget->IsA<UGameUIFocusPageWidgetBase>()
			|| Widget->GetClass()->ImplementsInterface(UGameUIFocusPageInterface::StaticClass()));
}

UWidget* UGameUIFocusScreenWidgetBase::FindFocusPageWidget(UWidget* RootWidget)
{
	if (!RootWidget)
	{
		return nullptr;
	}

	if (IsFocusPageWidget(RootWidget))
	{
		return RootWidget;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (UWidget* FoundPage = FindFocusPageWidget(Panel->GetChildAt(ChildIndex)))
			{
				return FoundPage;
			}
		}
	}

	if (UUserWidget* UserWidget = Cast<UUserWidget>(RootWidget))
	{
		if (UWidgetTree* Tree = UserWidget->WidgetTree)
		{
			UWidget* FoundPage = nullptr;
			Tree->ForEachWidget([&FoundPage](UWidget* Widget)
			{
				if (!FoundPage)
				{
					FoundPage = FindFocusPageWidget(Widget);
				}
			});

			return FoundPage;
		}
	}

	return nullptr;
}

UWidget* UGameUIFocusScreenWidgetBase::FindWidgetByNameRecursive(UWidget* RootWidget, const FName WidgetName)
{
	if (!RootWidget || WidgetName.IsNone())
	{
		return nullptr;
	}

	if (RootWidget->GetFName() == WidgetName)
	{
		return RootWidget;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (UWidget* Match = FindWidgetByNameRecursive(Panel->GetChildAt(ChildIndex), WidgetName))
			{
				return Match;
			}
		}
	}

	if (UUserWidget* UserWidget = Cast<UUserWidget>(RootWidget))
	{
		if (UWidget* NamedWidget = UserWidget->GetWidgetFromName(WidgetName))
		{
			return NamedWidget;
		}

		if (UWidgetTree* Tree = UserWidget->WidgetTree)
		{
			UWidget* Match = nullptr;
			Tree->ForEachWidget([&Match, WidgetName](UWidget* Widget)
			{
				if (!Match)
				{
					Match = FindWidgetByNameRecursive(Widget, WidgetName);
				}
			});
			return Match;
		}
	}

	return nullptr;
}

bool UGameUIFocusScreenWidgetBase::CanProcessNavigationMove(bool bIsRepeat)
{
	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (bIsRepeat && CurrentTimeSeconds - LastNavigationMoveTimeSeconds < static_cast<double>(NavigationRepeatDelay))
	{
		return false;
	}

	LastNavigationMoveTimeSeconds = CurrentTimeSeconds;
	return true;
}

void UGameUIFocusScreenWidgetBase::ResetAnalogNavigation()
{
	AnalogNavigationState.Reset();
}

void UGameUIFocusScreenWidgetBase::TryMigrateLegacyAnalogConfig()
{
	if (bMigratedLegacyAnalogConfig)
	{
		return;
	}

	constexpr float LegacyDeadZone = 0.55f;
	constexpr float LegacyReleaseThreshold = 0.35f;
	constexpr float LegacyInitialRepeatDelay = 0.30f;
	constexpr float LegacyRepeatInterval = 0.11f;
	const bool bCustomized = !FMath::IsNearlyEqual(AnalogNavigationDeadZone, LegacyDeadZone)
		|| !FMath::IsNearlyEqual(AnalogNavigationReleaseThreshold, LegacyReleaseThreshold)
		|| !FMath::IsNearlyEqual(AnalogInitialRepeatDelay, LegacyInitialRepeatDelay)
		|| !FMath::IsNearlyEqual(AnalogRepeatInterval, LegacyRepeatInterval);

	if (bCustomized)
	{
		AnalogNavigationConfig.DeadZone = AnalogNavigationDeadZone;
		AnalogNavigationConfig.ReleaseThreshold = AnalogNavigationReleaseThreshold;
		AnalogNavigationConfig.InitialRepeatDelay = AnalogInitialRepeatDelay;
		AnalogNavigationConfig.RepeatInterval = AnalogRepeatInterval;
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("Migrated customized screen-level analog navigation values at runtime. Screen=%s. Move these values to AnalogNavigationConfig."),
			*GetNameSafe(this));
	}

	bMigratedLegacyAnalogConfig = true;
}

void UGameUIFocusScreenWidgetBase::WarnIfWeakFocusTarget(const UWidget* Widget, EGameUIFocusZone Zone) const
{
	if (Zone != EGameUIFocusZone::Content || !Widget)
	{
		return;
	}

	if (!Widget->IsA<UGameUIFocusItemWidgetBase>())
	{
		UE_LOG(LogGameUIFocus, Warning, TEXT("Game UI content focus target '%s' is not a UGameUIFocusItemWidgetBase. Prefer focus rows/items to avoid silent controller focus failures."),
			*GetNameSafe(Widget));
	}
}

void UGameUIFocusScreenWidgetBase::LogContentFocusFailure(const TCHAR* Reason, const UWidget* ContextWidget) const
{
	UE_LOG(LogGameUIFocus, Warning,
		TEXT("GameUIFocusScreen '%s' could not enter content focus: %s. FocusSwitcher='%s' ActiveSwitcherWidget='%s' Context='%s'."),
		*GetNameSafe(this),
		Reason ? Reason : TEXT("unknown reason"),
		*GetNameSafe(FocusWidgetSwitcher),
		*GetNameSafe(GetActivePageWidget()),
		*GetNameSafe(ContextWidget));
}
