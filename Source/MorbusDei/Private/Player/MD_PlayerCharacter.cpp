// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/MD_PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Camera/CameraComponent.h"

#include "Interaction/MD_PlayerInspectComponent.h"
#include "Interaction/MD_PlayerInteractionComponent.h"

// Sets default values
AMD_PlayerCharacter::AMD_PlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);
	
	SpringArmComp->TargetArmLength = 210.f;
	SpringArmComp->SocketOffset = FVector(0.f, 50.f, 55.f);
	SpringArmComp->bDoCollisionTest = true;

	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 9.f;
	SpringArmComp->CameraLagMaxDistance = 35.f;

	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->CameraRotationLagSpeed = 12.f;

	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->FieldOfView = 66.f;

	InteractionComp = CreateDefaultSubobject<UMD_PlayerInteractionComponent>("InteractionComponent");
	InspectComp = CreateDefaultSubobject<UMD_PlayerInspectComponent>("InspectComponent");
}

// Called when the game starts or when spawned
void AMD_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CachedPlayerController = Cast<APlayerController>(GetController());

	ULocalPlayer* LocalPlayer = CachedPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

// Called every frame
void AMD_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMD_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Look);
	EnhancedInput->BindAction(MenuAction, ETriggerEvent::Started, this, &AMD_PlayerCharacter::ToggleEscapeMenu);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, InteractionComp, &UMD_PlayerInteractionComponent::Interact);
}

void AMD_PlayerCharacter::Move(const FInputActionValue& Value)
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		return;
	}
	
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	FRotator ControlRot = Controller->GetControlRotation();
	FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}

void AMD_PlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (InspectComp && InspectComp->IsInspecting())
	{
		InspectComp->RotateInspectedItem(LookAxisVector);
		return;
	}
	
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AMD_PlayerCharacter::ToggleEscapeMenu() //! Should later be moved in to "PlayerController"
{
	// if (InspectComp && InspectComp->IsInspecting())
	// {
	// 	InspectComp->EndInspect();
	// 	return;
	// }
	
	UE_LOG(LogTemp, Warning, TEXT("Open Menu"));

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PauseMenuWidgetClass && !PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}