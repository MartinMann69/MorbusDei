#include "UI/Focus/GameUIFocusPageWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/Widget.h"
#include "UI/Focus/GameUIFocusItemWidgetBase.h"
#include "UI/Focus/GameUIFocusScreenWidgetBase.h"
#include "UI/Focus/GameUIFocusTypes.h"

UGameUIFocusPageWidgetBase::UGameUIFocusPageWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGameUIFocusPageWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	ResetAnalogNavigation();
	ResetHorizontalAnalogSample();

	if (bAutoRegisterFocusItemsOnConstruct)
	{
		RegisterFocusItemsInWidgetTree();
	}
}

void UGameUIFocusPageWidgetBase::NativeDestruct()
{
	ResetAnalogNavigation();
	ResetHorizontalAnalogSample();
	Super::NativeDestruct();
}

FReply UGameUIFocusPageWidgetBase::NativeOnAnalogValueChanged(
	const FGeometry& InGeometry,
	const FAnalogInputEvent& InAnalogEvent)
{
	// Usually the focused GameUIFocus item owns this event. A modal can, however,
	// focus a nested Slate button (or another child) directly. In that case the
	// unhandled event bubbles to the page and must be claimed here before it
	// reaches the menu screen behind the modal.
	UGameUIFocusItemWidgetBase* CurrentItem = Cast<UGameUIFocusItemWidgetBase>(LastFocusWidget.Get());
	if (!IsUsableFocusTarget(CurrentItem) || !IsRegisteredFocusItem(CurrentItem))
	{
		CurrentItem = Cast<UGameUIFocusItemWidgetBase>(GetBestFocusTarget());
	}

	if (CurrentItem
		&& HandleFocusItemAnalogInput(
			CurrentItem,
			InAnalogEvent.GetKey(),
			InAnalogEvent.GetAnalogValue()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

void UGameUIFocusPageWidgetBase::RegisterFocusItem(UGameUIFocusItemWidgetBase* Item)
{
	if (!Item)
	{
		return;
	}

	if (!Item->IsFocusable())
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogGameUIFocus, Warning,
			TEXT("GameUIFocus page rejected non-focusable item. Page=%s Item=%s."),
			*GetNameSafe(this),
			*GetNameSafe(Item));
#endif
		return;
	}

	// Equivalent to UUserWidgetFunctionLibrary::GetOuterUserWidget(), whose
	// UE 5.7 symbol is not exported for direct runtime-module linkage.
	UUserWidget* OuterUserWidget = Item->GetTypedOuter<UUserWidget>();
	if (OuterUserWidget != this)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogGameUIFocus, Warning,
			TEXT("GameUIFocus page rejected nested focus item. Page=%s Item=%s OuterUserWidget=%s."),
			*GetNameSafe(this),
			*GetNameSafe(Item),
			*GetNameSafe(OuterUserWidget));
#endif
		return;
	}

	TryMigrateLegacyAnalogConfig(Item);
	Item->SetOwningFocusPage(this);
	if (OwningFocusScreen)
	{
		Item->SetPointerInputActive(OwningFocusScreen->IsPointerInputActive());
	}
	RegisteredFocusItems.AddUnique(TWeakObjectPtr<UGameUIFocusItemWidgetBase>(Item));
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageRegisterItem Page=%s Item=%s RegisteredItems=%d"),
		*GetNameSafe(this),
		*GetNameSafe(Item),
		RegisteredFocusItems.Num());
}

void UGameUIFocusPageWidgetBase::RegisterFocusItemsInWidgetTree()
{
	ClearRegisteredFocusItems();

	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget))
		{
			RegisterFocusItem(FocusItem);
		}
	});
}

void UGameUIFocusPageWidgetBase::ClearRegisteredFocusItems()
{
	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		if (UGameUIFocusItemWidgetBase* Item = WeakItem.Get())
		{
			if (Item->GetOwningFocusPage() == this)
			{
				Item->SetOwningFocusPage(nullptr);
			}
		}
	}

	RegisteredFocusItems.Reset();
	LastFocusWidget = nullptr;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageClearRegisteredItems Page=%s"),
		*GetNameSafe(this));
}

TArray<UGameUIFocusItemWidgetBase*> UGameUIFocusPageWidgetBase::GetRegisteredFocusItems() const
{
	TArray<UGameUIFocusItemWidgetBase*> Items;
	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		if (UGameUIFocusItemWidgetBase* Item = WeakItem.Get())
		{
			Items.Add(Item);
		}
	}

	return Items;
}

void UGameUIFocusPageWidgetBase::RememberFocusedWidget(UWidget* Widget)
{
	UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
	if (IsUsableFocusTarget(FocusItem) && IsRegisteredFocusItem(FocusItem))
	{
		LastFocusWidget = FocusItem;
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageRememberFocus Page=%s Widget=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Widget));
		if (OwningFocusScreen)
		{
			OwningFocusScreen->RememberFocusedWidget(FocusItem);
		}
		return;
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageRememberFocusRejected Page=%s Widget=%s Registered=%d"),
		*GetNameSafe(this),
		*GetNameSafe(Widget),
		IsRegisteredFocusItem(FocusItem) ? 1 : 0);
}

void UGameUIFocusPageWidgetBase::NotifyFocusItemFocused(UWidget* Widget)
{
	UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
	if (!IsUsableFocusTarget(FocusItem) || !IsRegisteredFocusItem(FocusItem))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageNotifyFocusRejected Page=%s Widget=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Widget));
		return;
	}

	LastFocusWidget = FocusItem;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageNotifyFocus Page=%s Widget=%s OwningScreen=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Widget),
		*GetNameSafe(OwningFocusScreen.Get()));
	HandleFocusItemFocused(FocusItem);

	if (OwningFocusScreen)
	{
		OwningFocusScreen->NotifyContentWidgetFocused(FocusItem);
	}
}

UWidget* UGameUIFocusPageWidgetBase::GetBestFocusTarget() const
{
	const UGameUIFocusItemWidgetBase* LastFocusItem = Cast<UGameUIFocusItemWidgetBase>(LastFocusWidget.Get());
	if (IsUsableFocusTarget(LastFocusItem) && IsRegisteredFocusItem(LastFocusItem))
	{
		return LastFocusWidget;
	}

	const UGameUIFocusItemWidgetBase* DefaultFocusItem = Cast<UGameUIFocusItemWidgetBase>(DefaultFocusWidget.Get());
	if (IsUsableFocusTarget(DefaultFocusItem) && IsRegisteredFocusItem(DefaultFocusItem))
	{
		return DefaultFocusWidget.Get();
	}

	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		if (UGameUIFocusItemWidgetBase* Item = WeakItem.Get(); IsUsableFocusTarget(Item))
		{
			return Item;
		}
	}

	return nullptr;
}

bool UGameUIFocusPageWidgetBase::FocusBestTarget()
{
	if (UWidget* Target = GetBestFocusTarget())
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageFocusBestTarget Page=%s Target=%s OwningScreen=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Target),
			*GetNameSafe(OwningFocusScreen.Get()));
		if (OwningFocusScreen)
		{
			return OwningFocusScreen->RequestFocusNextTick(Target);
		}

		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			Target->SetUserFocus(PlayerController);
			if (!Target->HasUserFocus(PlayerController))
			{
				Target->SetKeyboardFocus();
			}
		}
		else
		{
			Target->SetKeyboardFocus();
		}
		return true;
	}

	return false;
}

bool UGameUIFocusPageWidgetBase::FocusAdjacentItem(UGameUIFocusItemWidgetBase* CurrentItem, int32 Direction)
{
	return FocusAdjacentItem2D(CurrentItem, FIntPoint(0, Direction));
}

bool UGameUIFocusPageWidgetBase::FocusAdjacentItem2D(UGameUIFocusItemWidgetBase* CurrentItem, FIntPoint Direction)
{
	return NavigateFromItem(CurrentItem, Direction) == EGameUIFocusNavigationResult::Moved;
}

bool UGameUIFocusPageWidgetBase::FocusItemByIdentifier(FGameplayTag FocusIdentifier)
{
	if (!FocusIdentifier.IsValid())
	{
		return false;
	}

	PruneInvalidRegisteredFocusItems();
	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		UGameUIFocusItemWidgetBase* Item = WeakItem.Get();
		if (IsUsableFocusTarget(Item) && Item->GetFocusIdentifier() == FocusIdentifier)
		{
			return FocusItemInternal(Item);
		}
	}

	return false;
}

void UGameUIFocusPageWidgetBase::SetOwningFocusScreen(UGameUIFocusScreenWidgetBase* Screen)
{
	OwningFocusScreen = Screen;
	if (!OwningFocusScreen)
	{
		return;
	}

	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		if (UGameUIFocusItemWidgetBase* Item = WeakItem.Get())
		{
			Item->SetPointerInputActive(OwningFocusScreen->IsPointerInputActive());
		}
	}
}

bool UGameUIFocusPageWidgetBase::RequestReturnToNavigationZone()
{
	return OwningFocusScreen ? OwningFocusScreen->ReturnToNavigationZone() : false;
}

void UGameUIFocusPageWidgetBase::SetFocusScrollBox(UScrollBox* InFocusScrollBox)
{
	FocusScrollBox = InFocusScrollBox;
}

bool UGameUIFocusPageWidgetBase::HandleFocusItemAnalogInput(
	UGameUIFocusItemWidgetBase* CurrentItem,
	const FKey Key,
	const float Value)
{
	if (!IsUsableFocusTarget(CurrentItem) || CurrentItem->GetOwningFocusPage() != this)
	{
		return false;
	}

	if (Key == EKeys::Gamepad_LeftX)
	{
		UpdateHorizontalAnalogSample(Value);
	}

	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	const FGameUIAnalogNavigationResult Result = AnalogNavigationState.ProcessAxis(
		Key,
		Value,
		CurrentTimeSeconds,
		AnalogNavigationConfig,
		AnalogNavigationMode);

	if (!Result.bHandled)
	{
		return false;
	}

	if (!Result.bShouldNavigate)
	{
		return true;
	}

	const EGameUIFocusNavigationResult NavigationResult = NavigateFromItem(CurrentItem, Result.Direction);
	if (NavigationResult == EGameUIFocusNavigationResult::Moved)
	{
		AnalogNavigationState.NotifyNavigationSucceeded();
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace AnalogAccepted Owner=Page Page=%s Item=%s Direction=(%d,%d) Magnitude=%.3f Repeat=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentItem),
			Result.Direction.X,
			Result.Direction.Y,
			Result.Magnitude,
			Result.bIsRepeat ? 1 : 0);
	}
	else if (NavigationResult == EGameUIFocusNavigationResult::Blocked)
	{
		const bool bShouldBroadcastBlocked = AnalogNavigationState.NotifyNavigationBlocked();
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace AnalogBlocked Owner=Page Page=%s Item=%s Direction=(%d,%d) Magnitude=%.3f Repeat=%d Feedback=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentItem),
			Result.Direction.X,
			Result.Direction.Y,
			Result.Magnitude,
			Result.bIsRepeat ? 1 : 0,
			bShouldBroadcastBlocked ? 1 : 0);

		if (bShouldBroadcastBlocked)
		{
			OnNavigationBlocked.Broadcast(CurrentItem, Result.Direction);
		}
	}

	return true;
}

bool UGameUIFocusPageWidgetBase::HandleFocusItemDigitalInput(
	UGameUIFocusItemWidgetBase* CurrentItem,
	const FIntPoint Direction)
{
	const EGameUIFocusNavigationResult Result = NavigateFromItem(CurrentItem, Direction);
	if (Result == EGameUIFocusNavigationResult::Blocked)
	{
		OnNavigationBlocked.Broadcast(CurrentItem, Direction);
	}

	return Result != EGameUIFocusNavigationResult::Unhandled;
}

void UGameUIFocusPageWidgetBase::OnPageActivated_Implementation()
{
	ResetAnalogNavigation();
	ResetHorizontalAnalogSample();
}

void UGameUIFocusPageWidgetBase::OnPageDeactivated_Implementation()
{
	ResetAnalogNavigation();
	ResetHorizontalAnalogSample();
}

bool UGameUIFocusPageWidgetBase::EnterPageFocus_Implementation()
{
	ResetAnalogNavigation();

	if (UWidget* Target = GetBestFocusTarget())
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageEnterFocus Page=%s Target=%s OwningScreen=%s RegisteredItems=%d"),
			*GetNameSafe(this),
			*GetNameSafe(Target),
			*GetNameSafe(OwningFocusScreen.Get()),
			RegisteredFocusItems.Num());
		if (OwningFocusScreen)
		{
			return OwningFocusScreen->RequestFocusNextTickForZone(Target, EGameUIFocusZone::Content);
		}

		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			Target->SetUserFocus(PlayerController);
			if (!Target->HasUserFocus(PlayerController))
			{
				Target->SetKeyboardFocus();
			}
		}
		else
		{
			Target->SetKeyboardFocus();
		}
		return true;
	}

	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageEnterFocusRejected Page=%s Default=%s Last=%s RegisteredItems=%d"),
		*GetNameSafe(this),
		*GetNameSafe(DefaultFocusWidget.Get()),
		*GetNameSafe(LastFocusWidget.Get()),
		RegisteredFocusItems.Num());
	return false;
}

void UGameUIFocusPageWidgetBase::LeavePageFocus_Implementation()
{
	ResetAnalogNavigation();
	ResetHorizontalAnalogSample();
}

void UGameUIFocusPageWidgetBase::HandleFocusItemFocused_Implementation(UWidget* Widget)
{
	if (!FocusScrollBox || !IsUsableFocusTarget(Widget))
	{
		return;
	}

	for (const UWidget* Current = Widget; Current; Current = Current->GetParent())
	{
		if (Current == FocusScrollBox)
		{
			FocusScrollBox->ScrollWidgetIntoView(Widget, true, EDescendantScrollDestination::IntoView);
			return;
		}
	}
}

UWidget* UGameUIFocusPageWidgetBase::GetDefaultFocusWidget_Implementation() const
{
	return DefaultFocusWidget.Get();
}

UWidget* UGameUIFocusPageWidgetBase::GetLastFocusWidget_Implementation() const
{
	return LastFocusWidget.Get();
}

void UGameUIFocusPageWidgetBase::SetLastFocusWidget_Implementation(UWidget* Widget)
{
	RememberFocusedWidget(Widget);
}

void UGameUIFocusPageWidgetBase::PruneInvalidRegisteredFocusItems()
{
	RegisteredFocusItems.RemoveAll([](const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem)
	{
		return !WeakItem.IsValid();
	});
}

bool UGameUIFocusPageWidgetBase::IsRegisteredFocusItem(const UGameUIFocusItemWidgetBase* Item) const
{
	if (!Item)
	{
		return false;
	}

	return RegisteredFocusItems.ContainsByPredicate([Item](const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem)
	{
		return WeakItem.Get() == Item;
	});
}

EGameUIFocusNavigationResult UGameUIFocusPageWidgetBase::NavigateFromItem(
	UGameUIFocusItemWidgetBase* CurrentItem,
	FIntPoint Direction)
{
	Direction.X = FMath::Clamp(Direction.X, -1, 1);
	Direction.Y = FMath::Clamp(Direction.Y, -1, 1);
	if (Direction == FIntPoint::ZeroValue
		|| !IsUsableFocusTarget(CurrentItem)
		|| !IsRegisteredFocusItem(CurrentItem))
	{
		return EGameUIFocusNavigationResult::Unhandled;
	}

	PruneInvalidRegisteredFocusItems();

	const FGameplayTag OverrideIdentifier = CurrentItem->GetFocusOverrideIdentifier(Direction);
	if (OverrideIdentifier.IsValid())
	{
		for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
		{
			UGameUIFocusItemWidgetBase* Candidate = WeakItem.Get();
			if (IsUsableFocusTarget(Candidate) && Candidate->GetFocusIdentifier() == OverrideIdentifier)
			{
				return FocusItemInternal(Candidate)
					? EGameUIFocusNavigationResult::Moved
					: EGameUIFocusNavigationResult::Blocked;
			}
		}

		return EGameUIFocusNavigationResult::Blocked;
	}

	if (UGameUIFocusItemWidgetBase* GridTarget = FindBestGridTarget(CurrentItem, Direction))
	{
		return FocusItemInternal(GridTarget)
			? EGameUIFocusNavigationResult::Moved
			: EGameUIFocusNavigationResult::Blocked;
	}

	TArray<UGameUIFocusItemWidgetBase*> UsableItems;
	int32 CurrentIndex = INDEX_NONE;
	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		UGameUIFocusItemWidgetBase* Item = WeakItem.Get();
		if (!IsUsableFocusTarget(Item))
		{
			continue;
		}

		const int32 ItemIndex = UsableItems.Add(Item);
		if (Item == CurrentItem)
		{
			CurrentIndex = ItemIndex;
		}
	}

	if (CurrentIndex == INDEX_NONE || UsableItems.IsEmpty())
	{
		return EGameUIFocusNavigationResult::Unhandled;
	}

	const int32 LinearDirection = FMath::Sign(Direction.Y != 0 ? Direction.Y : Direction.X);
	const int32 TargetIndex = CurrentIndex + LinearDirection;
	if (!UsableItems.IsValidIndex(TargetIndex))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageNavigationBlocked Page=%s Current=%s Direction=(%d,%d) Index=%d Items=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentItem),
			Direction.X,
			Direction.Y,
			CurrentIndex,
			UsableItems.Num());
		return EGameUIFocusNavigationResult::Blocked;
	}

	UGameUIFocusItemWidgetBase* Target = UsableItems[TargetIndex];
	return Target != CurrentItem && FocusItemInternal(Target)
		? EGameUIFocusNavigationResult::Moved
		: EGameUIFocusNavigationResult::Blocked;
}

namespace
{
	void ApplyDirectWidgetFocus(UWidget* Target, APlayerController* OwningPlayer)
	{
		if (!Target)
		{
			return;
		}

		if (OwningPlayer)
		{
			Target->SetUserFocus(OwningPlayer);
			if (Target->HasUserFocus(OwningPlayer))
			{
				return;
			}
		}
		Target->SetKeyboardFocus();
	}
}

bool UGameUIFocusPageWidgetBase::FocusItemInternal(UGameUIFocusItemWidgetBase* Target)
{
	if (!IsUsableFocusTarget(Target))
	{
		return false;
	}

	if (OwningFocusScreen)
	{
		return OwningFocusScreen->RequestFocusNextTickForZone(Target, EGameUIFocusZone::Content);
	}

	RememberFocusedWidget(Target);
	ApplyDirectWidgetFocus(Target, GetOwningPlayer());
	return true;
}

void UGameUIFocusPageWidgetBase::ResetAnalogNavigation()
{
	AnalogNavigationState.Reset();
}

void UGameUIFocusPageWidgetBase::UpdateHorizontalAnalogSample(const float Value)
{
	HorizontalAnalogSample = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UGameUIFocusPageWidgetBase::ResetHorizontalAnalogSample()
{
	HorizontalAnalogSample = 0.0f;
}

bool UGameUIFocusPageWidgetBase::IsHorizontalAnalogActuated(const float ReleaseThreshold) const
{
	return FMath::Abs(HorizontalAnalogSample) > FMath::Clamp(ReleaseThreshold, 0.0f, 1.0f);
}

void UGameUIFocusPageWidgetBase::TryMigrateLegacyAnalogConfig(const UGameUIFocusItemWidgetBase* Item)
{
	if (bMigratedLegacyAnalogConfig || !Item)
	{
		return;
	}

	FGameUIAnalogNavigationConfig LegacyConfig;
	if (!Item->TryGetCustomizedLegacyAnalogNavigationConfig(LegacyConfig))
	{
		return;
	}

	AnalogNavigationConfig = LegacyConfig;
	bMigratedLegacyAnalogConfig = true;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("Migrated customized item-level analog navigation values at runtime. Page=%s SourceItem=%s. Move these values to the page AnalogNavigationConfig."),
		*GetNameSafe(this),
		*GetNameSafe(Item));
}

UGameUIFocusItemWidgetBase* UGameUIFocusPageWidgetBase::FindBestGridTarget(
	UGameUIFocusItemWidgetBase* CurrentItem,
	FIntPoint Direction) const
{
	if (!CurrentItem || !CurrentItem->HasFocusGridPosition())
	{
		return nullptr;
	}

	Direction.X = FMath::Clamp(Direction.X, -1, 1);
	Direction.Y = FMath::Clamp(Direction.Y, -1, 1);
	if (Direction == FIntPoint::ZeroValue)
	{
		return nullptr;
	}

	const FIntPoint CurrentPosition = CurrentItem->GetFocusGridPosition();
	UGameUIFocusItemWidgetBase* BestTarget = nullptr;
	int32 BestPrimaryDistance = MAX_int32;
	int32 BestSecondaryDistance = MAX_int32;

	for (const TWeakObjectPtr<UGameUIFocusItemWidgetBase>& WeakItem : RegisteredFocusItems)
	{
		UGameUIFocusItemWidgetBase* Candidate = WeakItem.Get();
		if (Candidate == CurrentItem || !IsUsableFocusTarget(Candidate) || !Candidate->HasFocusGridPosition())
		{
			continue;
		}

		const FIntPoint Delta = Candidate->GetFocusGridPosition() - CurrentPosition;
		const int32 PrimaryDistance = Direction.X != 0 ? Delta.X * Direction.X : Delta.Y * Direction.Y;
		if (PrimaryDistance <= 0)
		{
			continue;
		}

		const int32 SecondaryDistance = Direction.X != 0 ? FMath::Abs(Delta.Y) : FMath::Abs(Delta.X);
		if (PrimaryDistance < BestPrimaryDistance
			|| (PrimaryDistance == BestPrimaryDistance && SecondaryDistance < BestSecondaryDistance))
		{
			BestTarget = Candidate;
			BestPrimaryDistance = PrimaryDistance;
			BestSecondaryDistance = SecondaryDistance;
		}
	}

	return BestTarget;
}

bool UGameUIFocusPageWidgetBase::IsUsableFocusTarget(const UWidget* Widget)
{
	if (!IsValid(Widget) || !Widget->GetIsEnabled())
	{
		return false;
	}

	const UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
	if (!FocusItem || !FocusItem->IsFocusable())
	{
		return false;
	}

	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed
		&& Visibility != ESlateVisibility::Hidden;
}
