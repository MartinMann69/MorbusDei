// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MD_InteractInterface.h"
#include "MD_Interactable.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class UMD_HighlightComponent;
class UMD_InteractPromptComponent;

UCLASS()
class MORBUSDEI_API AMD_Interactable : public AActor, public IMD_InteractInterface
{
	GENERATED_BODY()

public:
	AMD_Interactable();
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* HighlightedObjects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* ToggleableObjects;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	UWidgetComponent* InteractPromptWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UMD_HighlightComponent* HighlightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UMD_InteractPromptComponent* InteractPromptComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bCanInteract = true;


public:
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual void SetInteractPromptVisible_Implementation(bool bVisible) override;
	virtual bool CanInteract_Implementation() const override;
	virtual void Highlight_Implementation(bool bHighlight) override;
};
