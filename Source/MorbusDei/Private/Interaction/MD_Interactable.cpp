#include "Interaction/MD_Interactable.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Components/WidgetComponent.h"
#include "Interaction/MD_HighlightComponent.h"
#include "Interaction/MD_InteractPromptComponent.h"

AMD_Interactable::AMD_Interactable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Object"));
	RootComponent = Root;
	Root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Root->SetCollisionResponseToAllChannels(ECR_Block);

	HighlightedObjects = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightedObjects"));
	HighlightedObjects->SetupAttachment(Root);
	HighlightedObjects->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HighlightedObjects->SetGenerateOverlapEvents(false);

	ToggleableObjects = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToggleableObjects"));
	ToggleableObjects->SetupAttachment(Root);
	ToggleableObjects->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToggleableObjects->SetGenerateOverlapEvents(false);

	InteractPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidget"));
	InteractPromptWidget->SetupAttachment(RootComponent);
	InteractPromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	InteractPromptWidget->SetWidgetSpace(EWidgetSpace::Screen); // or World
	InteractPromptWidget->SetDrawSize(FVector2D(200.f, 200.f));
	InteractPromptWidget->SetVisibility(false);
	InteractPromptWidget->SetPivot(FVector2D(0, 0));

	HighlightComponent = CreateDefaultSubobject<UMD_HighlightComponent>(TEXT("HighlightComponent"));
	HighlightComponent->SetHighlightRoot(HighlightedObjects);

	InteractPromptComponent = CreateDefaultSubobject<UMD_InteractPromptComponent>(TEXT("InteractPromptComponent"));
	InteractPromptComponent->SetPromptWidget(InteractPromptWidget);
}

void AMD_Interactable::BeginPlay()
{
	Super::BeginPlay();
}

void AMD_Interactable::Interact_Implementation(APawn* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("%s interacted with %s"),*GetNameSafe(Interactor),*GetNameSafe(this));
	GEngine->AddOnScreenDebugMessage(-1,2.0f,FColor::Green,FString::Printf(TEXT("Interacted with %s"), *GetNameSafe(this)));

	if (!ToggleableObjects)
	{
		return;
	}
	
	TArray<USceneComponent*> ToggleChildComponents;
	ToggleableObjects->GetChildrenComponents(true, ToggleChildComponents);

	for (USceneComponent* Child : ToggleChildComponents)
	{
		Child->ToggleVisibility();
	}
}

void AMD_Interactable::SetInteractPromptVisible_Implementation(bool bVisible)
{
	if (InteractPromptComponent)
	{
		InteractPromptComponent->SetPromptVisible(bVisible);
	}
}

bool AMD_Interactable::CanInteract_Implementation() const
{
	return bCanInteract;
}

void AMD_Interactable::Highlight_Implementation(bool bHighlight)
{
	if (HighlightComponent)
	{
		HighlightComponent->SetHighlighted(bHighlight);
	}
}