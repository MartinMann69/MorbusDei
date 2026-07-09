#include "UI/Focus/GameUIFocusScreenWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
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

FKey GetGameUIFocusVirtualAcceptKey()
{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
	return EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
#else
	return EKeys::Virtual_Accept;
#endif
}

bool IsFocusTraceKey(const FKey& Key)
{
	return Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == GetGameUIFocusVirtualAcceptKey()
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

bool UGameUIFocusScreenWidgetBase::InitializeFocusScreen(bool bFocusNavigation)
{
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
		return;
	}

	if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget))
	{
		FocusItem->SetOwningNavigationScreen(this);
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
	NavigationEntries.Reset();
	for (UWidget* Widget : Widgets)
	{
		RegisterNavigationWidget(Widget);
	}
}

void UGameUIFocusScreenWidgetBase::SetNavigationEntries(const TArray<FGameUIFocusNavigationEntry>& Entries)
{
	NavigationEntries.Reset();

	for (const FGameUIFocusNavigationEntry& Entry : Entries)
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
		UE_LOG(LogGameUIFocus, Warning,
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

	const int32 NewIndex = (ActiveNavigationIndex + Direction + EntryCount) % EntryCount;
	const int32 PageIndex = GetPageIndexForNavigationIndex(NewIndex);
	if (bSwitchPageWithNavigationFocus && FocusWidgetSwitcher && PageIndex >= 0 && PageIndex < FocusWidgetSwitcher->GetNumWidgets())
	{
		const bool bSwitched = SwitchToPageIndex(PageIndex, false);
		return bSwitched && SetNavigationFocusByIndex(NewIndex);
	}

	return SetNavigationFocusByIndex(NewIndex);
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

	if (bApplyZone)
	{
		SetCurrentFocusZone(Zone);
	}
	if (NavigationIndexToApply != INDEX_NONE)
	{
		SetActiveNavigationIndex(NavigationIndexToApply);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Widget->SetUserFocus(GetOwningPlayer());
		Widget->SetKeyboardFocus();
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace FocusAppliedImmediate Screen=%s Serial=%llu Target=%s HasUserFocus=%d HasKeyboardFocus=%d"),
			*GetNameSafe(this),
			RequestSerial,
			*GetNameSafe(Widget),
			Widget->HasUserFocus(GetOwningPlayer()) ? 1 : 0,
			Widget->HasKeyboardFocus() ? 1 : 0);
		RememberFocusedWidget(Widget);
		if (bLeaveActivePageOnSuccess)
		{
			LeaveActivePageFocus();
		}
		return true;
	}

	Widget->SetUserFocus(GetOwningPlayer());
	Widget->SetKeyboardFocus();
	RememberFocusedWidget(Widget);
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace FocusAppliedNow Screen=%s Serial=%llu Target=%s Zone=%s HasUserFocus=%d HasKeyboardFocus=%d"),
		*GetNameSafe(this),
		RequestSerial,
		*GetNameSafe(Widget),
		*FocusZoneToString(CurrentFocusZone),
		Widget->HasUserFocus(GetOwningPlayer()) ? 1 : 0,
		Widget->HasKeyboardFocus() ? 1 : 0);
	if (bLeaveActivePageOnSuccess)
	{
		LeaveActivePageFocus();
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, FocusWidget, RequestSerial, bApplyZone, Zone, NavigationIndexToApply, bLeaveActivePageOnSuccess]()
	{
		UGameUIFocusScreenWidgetBase* FocusScreen = WeakThis.Get();
		UWidget* FocusTarget = FocusWidget.Get();
		if (FocusScreen && FocusScreen->FocusRequestSerial == RequestSerial && FocusScreen->IsUsableFocusTarget(FocusTarget))
		{
			FocusTarget->SetUserFocus(FocusScreen->GetOwningPlayer());
			FocusTarget->SetKeyboardFocus();
			UE_LOG(LogGameUIFocus, VeryVerbose,
				TEXT("GameUIFocusTrace FocusRefreshedNextTick Screen=%s Serial=%llu Target=%s Zone=%s HasUserFocus=%d HasKeyboardFocus=%d"),
				*GetNameSafe(FocusScreen),
				RequestSerial,
				*GetNameSafe(FocusTarget),
				*FocusZoneToString(FocusScreen->CurrentFocusZone),
				FocusTarget->HasUserFocus(FocusScreen->GetOwningPlayer()) ? 1 : 0,
				FocusTarget->HasKeyboardFocus() ? 1 : 0);
			FocusScreen->RememberFocusedWidget(FocusTarget);
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
	if (IsFocusTraceKey(Key))
	{
		UE_LOG(LogGameUIFocus, Warning,
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

		if (IsUpKey(Key))
		{
			if (CanProcessNavigationMove(InKeyEvent.IsRepeat()))
			{
				MoveNavigationFocus(-1);
			}
			return FReply::Handled();
		}

		if (IsDownKey(Key))
		{
			if (CanProcessNavigationMove(InKeyEvent.IsRepeat()))
			{
				MoveNavigationFocus(1);
			}
			return FReply::Handled();
		}

		if ((IsRightKey(Key) || IsAcceptKey(Key)) && ActivateCurrentNavigationEntry(true))
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

		if (IsBackKey(Key) && ReturnToNavigationZone())
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

		if (IsBackKey(Key) && ReturnFromModalZone())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UGameUIFocusScreenWidgetBase::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (CurrentFocusZone != EGameUIFocusZone::Navigation)
	{
		ResetAnalogNavigationMove();
		ResetAnalogContentEnter();
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	const FKey Key = InAnalogEvent.GetKey();
	const float AnalogValue = InAnalogEvent.GetAnalogValue();

	if (Key == EKeys::Gamepad_LeftY)
	{
		const int32 Direction = AnalogValue > 0.0f ? -1 : 1;
		return HandleAnalogNavigationMove(Direction, FMath::Abs(AnalogValue)) ? FReply::Handled() : FReply::Unhandled();
	}

	if (Key == EKeys::Gamepad_LeftX)
	{
		if (AnalogValue > 0.0f)
		{
			return HandleAnalogContentEnter(AnalogValue) ? FReply::Handled() : FReply::Unhandled();
		}

		ResetAnalogContentEnter();
		return FMath::Abs(AnalogValue) >= AnalogNavigationReleaseThreshold ? FReply::Handled() : FReply::Unhandled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
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

bool UGameUIFocusScreenWidgetBase::HandleAnalogNavigationMove(int32 Direction, float Magnitude)
{
	if (Direction == 0 || Magnitude < AnalogNavigationReleaseThreshold)
	{
		ResetAnalogNavigationMove();
		return false;
	}

	if (Magnitude < AnalogNavigationDeadZone)
	{
		return true;
	}

	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (!bAnalogNavigationHeld || LastAnalogNavigationDirection != Direction)
	{
		bAnalogNavigationHeld = true;
		bAnalogNavigationRepeatActive = false;
		LastAnalogNavigationDirection = Direction;
		LastAnalogNavigationMoveTimeSeconds = CurrentTimeSeconds;
		MoveNavigationFocus(Direction);
		return true;
	}

	const double RepeatDelay = bAnalogNavigationRepeatActive
		? static_cast<double>(AnalogRepeatInterval)
		: static_cast<double>(AnalogInitialRepeatDelay);

	if (CurrentTimeSeconds - LastAnalogNavigationMoveTimeSeconds >= RepeatDelay)
	{
		bAnalogNavigationRepeatActive = true;
		LastAnalogNavigationMoveTimeSeconds = CurrentTimeSeconds;
		MoveNavigationFocus(Direction);
	}

	return true;
}

bool UGameUIFocusScreenWidgetBase::HandleAnalogContentEnter(float Magnitude)
{
	if (Magnitude < AnalogNavigationReleaseThreshold)
	{
		ResetAnalogContentEnter();
		return false;
	}

	if (Magnitude < AnalogNavigationDeadZone)
	{
		return true;
	}

	if (!bAnalogContentEnterHeld)
	{
		bAnalogContentEnterHeld = true;
		EnterContentZone();
	}

	return true;
}

void UGameUIFocusScreenWidgetBase::ResetAnalogNavigationMove()
{
	bAnalogNavigationHeld = false;
	bAnalogNavigationRepeatActive = false;
	LastAnalogNavigationDirection = 0;
	LastAnalogNavigationMoveTimeSeconds = -1000.0;
}

void UGameUIFocusScreenWidgetBase::ResetAnalogContentEnter()
{
	bAnalogContentEnterHeld = false;
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

bool UGameUIFocusScreenWidgetBase::IsRightKey(const FKey& Key)
{
	return Key == EKeys::Right
		|| Key == EKeys::Gamepad_DPad_Right;
}

bool UGameUIFocusScreenWidgetBase::IsAcceptKey(const FKey& Key)
{
	return Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == GetGameUIFocusVirtualAcceptKey()
		|| Key == EKeys::Gamepad_FaceButton_Bottom;
}

bool UGameUIFocusScreenWidgetBase::IsUpKey(const FKey& Key)
{
	return Key == EKeys::Up
		|| Key == EKeys::Gamepad_DPad_Up;
}

bool UGameUIFocusScreenWidgetBase::IsDownKey(const FKey& Key)
{
	return Key == EKeys::Down
		|| Key == EKeys::Gamepad_DPad_Down;
}

bool UGameUIFocusScreenWidgetBase::IsBackKey(const FKey& Key)
{
	return Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Right;
}
