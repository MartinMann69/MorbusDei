// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/MD_PlayerInteractionComponent.h"
#include "MD_PlayerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;

class UCameraComponent;
class USpringArmComponent;
class UMD_PlayerInteractionComponent;
class UMD_PlayerInspectComponent;

class UUserWidget;

struct FInputActionValue;



UCLASS()
class MORBUSDEI_API AMD_PlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMD_PlayerCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY()
	UUserWidget* PauseMenuWidget;

protected:

	UPROPERTY()
	APlayerController* CachedPlayerController = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComponent;
	
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UMD_PlayerInteractionComponent* InteractionComp;

	UPROPERTY(VisibleAnywhere)
	UMD_PlayerInspectComponent* InspectComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MenuAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category="Interaction")
	float InteractDistance = 500.0f;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleEscapeMenu();
	
};
