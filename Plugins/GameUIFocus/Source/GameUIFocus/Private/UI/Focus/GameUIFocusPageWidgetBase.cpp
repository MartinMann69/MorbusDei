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

	if (bAutoRegisterFocusItemsOnConstruct)
	{
		RegisterFocusItemsInWidgetTree();
	}
}

void UGameUIFocusPageWidgetBase::RegisterFocusItem(UGameUIFocusItemWidgetBase* Item)
{
	if (Item)
	{
		Item->SetOwningFocusPage(this);
		RegisteredFocusItems.AddUnique(TWeakObjectPtr<UGameUIFocusItemWidgetBase>(Item));
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageRegisterItem Page=%s Item=%s RegisteredItems=%d"),
			*GetNameSafe(this),
			*GetNameSafe(Item),
			RegisteredFocusItems.Num());
	}
}

void UGameUIFocusPageWidgetBase::RegisterFocusItemsInWidgetTree()
{
	PruneInvalidRegisteredFocusItems();

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
	RegisteredFocusItems.Reset();
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
	if (IsUsableFocusTarget(Widget))
	{
		LastFocusWidget = Widget;
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageRememberFocus Page=%s Widget=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Widget));
		if (OwningFocusScreen)
		{
			OwningFocusScreen->RememberFocusedWidget(Widget);
		}
	}
}

void UGameUIFocusPageWidgetBase::NotifyFocusItemFocused(UWidget* Widget)
{
	if (!IsUsableFocusTarget(Widget))
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageNotifyFocusRejected Page=%s Widget=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Widget));
		return;
	}

	LastFocusWidget = Widget;
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageNotifyFocus Page=%s Widget=%s OwningScreen=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Widget),
		*GetNameSafe(OwningFocusScreen.Get()));
	HandleFocusItemFocused(Widget);

	if (OwningFocusScreen)
	{
		OwningFocusScreen->NotifyContentWidgetFocused(Widget);
	}
}

UWidget* UGameUIFocusPageWidgetBase::GetBestFocusTarget() const
{
	if (IsUsableFocusTarget(LastFocusWidget))
	{
		return LastFocusWidget;
	}

	if (IsUsableFocusTarget(DefaultFocusWidget))
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
		}
		Target->SetKeyboardFocus();
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
	if (Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	PruneInvalidRegisteredFocusItems();

	if (UGameUIFocusItemWidgetBase* GridTarget = FindBestGridTarget(CurrentItem, Direction))
	{
		return FocusItemInternal(GridTarget);
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

	if (UsableItems.Num() <= 0)
	{
		UE_LOG(LogGameUIFocus, VeryVerbose,
			TEXT("GameUIFocusTrace PageFocusAdjacentRejected Page=%s Current=%s Direction=%d Reason=NoUsableItems RegisteredItems=%d"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentItem),
			Direction.Y != 0 ? Direction.Y : Direction.X,
			RegisteredFocusItems.Num());
		return false;
	}

	const int32 LinearDirection = Direction.Y != 0 ? Direction.Y : Direction.X;
	const int32 TargetIndex = CurrentIndex == INDEX_NONE
		? (LinearDirection > 0 ? 0 : UsableItems.Num() - 1)
		: FMath::Clamp(CurrentIndex + LinearDirection, 0, UsableItems.Num() - 1);

	UGameUIFocusItemWidgetBase* Target = UsableItems[TargetIndex];
	UE_LOG(LogGameUIFocus, VeryVerbose,
		TEXT("GameUIFocusTrace PageFocusAdjacent Page=%s Current=%s CurrentIndex=%d Target=%s TargetIndex=%d Direction=%d UsableItems=%d OwningScreen=%s"),
		*GetNameSafe(this),
		*GetNameSafe(CurrentItem),
		CurrentIndex,
		*GetNameSafe(Target),
		TargetIndex,
		LinearDirection,
		UsableItems.Num(),
		*GetNameSafe(OwningFocusScreen.Get()));

	return FocusItemInternal(Target);
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
}

bool UGameUIFocusPageWidgetBase::RequestReturnToNavigationZone()
{
	return OwningFocusScreen ? OwningFocusScreen->ReturnToNavigationZone() : false;
}

void UGameUIFocusPageWidgetBase::SetFocusScrollBox(UScrollBox* InFocusScrollBox)
{
	FocusScrollBox = InFocusScrollBox;
}

void UGameUIFocusPageWidgetBase::OnPageActivated_Implementation()
{
}

void UGameUIFocusPageWidgetBase::OnPageDeactivated_Implementation()
{
}

bool UGameUIFocusPageWidgetBase::EnterPageFocus_Implementation()
{
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
		}
		Target->SetKeyboardFocus();
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

	RememberFocusedWidget(Target);
	if (OwningFocusScreen)
	{
		return OwningFocusScreen->RequestFocusNextTickForZone(Target, EGameUIFocusZone::Content);
	}

	ApplyDirectWidgetFocus(Target, GetOwningPlayer());
	return true;
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

	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed
		&& Visibility != ESlateVisibility::Hidden;
}
