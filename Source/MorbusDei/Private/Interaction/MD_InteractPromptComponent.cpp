#include "Interaction/MD_InteractPromptComponent.h"

#include "Components/WidgetComponent.h"

UMD_InteractPromptComponent::UMD_InteractPromptComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMD_InteractPromptComponent::SetPromptWidget(UWidgetComponent* NewPromptWidget)
{
	InteractPromptWidget = NewPromptWidget;
}

void UMD_InteractPromptComponent::SetPromptVisible(bool bVisible)
{
	if (InteractPromptWidget)
	{
		InteractPromptWidget->SetVisibility(bVisible);
	}
}