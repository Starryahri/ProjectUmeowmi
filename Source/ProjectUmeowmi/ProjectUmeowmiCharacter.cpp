// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AProjectUmeowmiCharacter

AProjectUmeowmiCharacter::AProjectUmeowmiCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Initialize target camera rotation
	TargetCameraRotation = FRotator(-25.0f, 45.0f, 0.0f);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProjectUmeowmiCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AProjectUmeowmiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Look);

		// Rotate Camera
		EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::GetCameraPositionIndex);

		// Toggle Grid Movement
		EnhancedInputComponent->BindAction(ToggleGridMovementAction, ETriggerEvent::Started, this, &AProjectUmeowmiCharacter::ToggleGridMovement);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectUmeowmiCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw + CameraOffset, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		if (bUseGridMovement)
		{
			// Only process new movement input if we're not already moving to a grid position
			if (!bIsMovingToGrid)
			{
				// For grid movement, we only move in cardinal directions
				// Determine which direction has the larger input
				if (FMath::Abs(MovementVector.X) > FMath::Abs(MovementVector.Y))
				{
					// Move horizontally
					FVector CurrentLocation = GetActorLocation();
					TargetGridPosition = CurrentLocation + RightDirection * GridSize * FMath::Sign(MovementVector.X);
					TargetGridPosition = SnapToGrid(TargetGridPosition);
					
					// Set target rotation based on movement direction
					TargetRotation = FRotator(0.0f, YawRotation.Yaw + (MovementVector.X > 0 ? 90.0f : -90.0f), 0.0f);
					
					bIsMovingToGrid = true;
				}
				else if (MovementVector.Y != 0)
				{
					// Move vertically
					FVector CurrentLocation = GetActorLocation();
					TargetGridPosition = CurrentLocation + ForwardDirection * GridSize * FMath::Sign(MovementVector.Y);
					TargetGridPosition = SnapToGrid(TargetGridPosition);
					
					// Set target rotation based on movement direction
					TargetRotation = FRotator(0.0f, YawRotation.Yaw + (MovementVector.Y > 0 ? 0.0f : 180.0f), 0.0f);
					
					bIsMovingToGrid = true;
				}
			}
		}
		else
		{
			// Normal movement
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
	}
}

void AProjectUmeowmiCharacter::GetCameraPositionIndex(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %f"), InputValue);
	
	//If Value is negated, then we need to rotate the camera to the right	
	// Q or LB
	if (InputValue < 0)
	{
		if (CameraPositionIndex == 3)
		{
			CameraPositionIndex = 0;
		}
		else
		{
			CameraPositionIndex++;
		}
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}
	
	//If Value is positive, then we need to rotate the camera to the left
	// E or RB
	else if (InputValue > 0)
	{
		if (CameraPositionIndex == 0)
		{
			CameraPositionIndex = 3;
		}
		else
		{
			CameraPositionIndex--;
		}
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}

	// Update the target camera rotation based on position index
	switch (CameraPositionIndex)
	{
		case 0:
			TargetCameraRotation = FRotator(-25.0f, -45.0f, 0.0f);
			CameraOffset = -45.0f;
			break;
		case 1:
			TargetCameraRotation = FRotator(-25.0f, -135.0f, 0.0f);
			CameraOffset = -135.0f;
			break;
		case 2:
			TargetCameraRotation = FRotator(-25.0f, 135.0f, 0.0f);
			CameraOffset = 135.0f;
			break;
		case 3:
			TargetCameraRotation = FRotator(-25.0f, 45.0f, 0.0f);
			CameraOffset = 45.0f;
			break;
	}
}

void AProjectUmeowmiCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		 //AddControllerYawInput(LookAxisVector.X);
		 //AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AProjectUmeowmiCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smoothly interpolate the camera rotation
	FRotator CurrentRotation = CameraBoom->GetRelativeRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetCameraRotation, DeltaTime, CameraTransitionSpeed);
	CameraBoom->SetRelativeRotation(NewRotation);

	// Handle grid movement
	if (bUseGridMovement && bIsMovingToGrid)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector Direction = (TargetGridPosition - CurrentLocation).GetSafeNormal();
		float DistanceToTarget = FVector::Distance(CurrentLocation, TargetGridPosition);

		if (DistanceToTarget > 1.0f) // If we're not close enough to the target
		{
			// Move towards the target grid position
			AddMovementInput(Direction, 1.0f);
			
			// Smoothly rotate towards the target rotation
			FRotator CurrentActorRotation = GetActorRotation();
			FRotator NewActorRotation = FMath::RInterpTo(CurrentActorRotation, TargetRotation, DeltaTime, 15.0f);
			SetActorRotation(NewActorRotation);
		}
		else
		{
			// We've reached the target grid position
			SetActorLocation(TargetGridPosition);
			SetActorRotation(TargetRotation);
			bIsMovingToGrid = false;
		}
	}
}

void AProjectUmeowmiCharacter::ToggleGridMovement(const FInputActionValue& Value)
{
	bUseGridMovement = !bUseGridMovement;
	
	// Update movement speed based on grid mode
	if (bUseGridMovement)
	{
		GetCharacterMovement()->MaxWalkSpeed = GridMovementSpeed;
		// Initialize target position and rotation to current values when enabling grid movement
		TargetGridPosition = GetActorLocation();
		TargetRotation = GetActorRotation();
		bIsMovingToGrid = false;
		
		// Ensure character movement component settings are correct for grid movement
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 500.0f; // Reset to default speed
		bIsMovingToGrid = false;
		
		// Reset character movement component settings
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	UE_LOG(LogTemplateCharacter, Log, TEXT("Grid Movement %s"), bUseGridMovement ? TEXT("Enabled") : TEXT("Disabled"));
}

FVector AProjectUmeowmiCharacter::SnapToGrid(const FVector& Location) const
{
	// Snap X and Y coordinates to the nearest grid point
	float SnappedX = FMath::RoundToFloat(Location.X / GridSize) * GridSize;
	float SnappedY = FMath::RoundToFloat(Location.Y / GridSize) * GridSize;
	
	// Keep Z coordinate unchanged
	return FVector(SnappedX, SnappedY, Location.Z);
}
