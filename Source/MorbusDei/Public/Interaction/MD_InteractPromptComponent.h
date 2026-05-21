#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_InteractPromptComponent.generated.h"

class UWidgetComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_InteractPromptComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_InteractPromptComponent();

	void SetPromptWidget(UWidgetComponent* NewPromptWidget);
	void SetPromptVisible(bool bVisible);

protected:
	UPROPERTY()
	UWidgetComponent* InteractPromptWidget = nullptr;
};