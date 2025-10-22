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
#include "Dialogue/TalkingObject.h"
#include "DishCustomization/PUDishCustomizationComponent.h"

#include "Interfaces/PUInteractableInterface.h"

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

	// Always show mouse cursor
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AProjectUmeowmiCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the camera position based on the starting index
	InitializeCameraPosition();

	UE_LOG(LogTemp, Log, TEXT("Character BeginPlay - Camera initialized with position index: %d"), CameraPositionIndex);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProjectUmeowmiCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		UE_LOG(LogTemp, Log, TEXT("Controller changed - PlayerController found"));
		
		// Always show mouse cursor
		PlayerController->bShowMouseCursor = true;
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
		
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			UE_LOG(LogTemp, Log, TEXT("Enhanced Input Subsystem found"));
			
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("Default mapping context added successfully"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Default mapping context is null!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Enhanced Input Subsystem not found!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Controller changed - No PlayerController found!"));
	}
}

void AProjectUmeowmiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Look);

		// Rotating camera
		EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::GetCameraPositionIndex);

		// Zooming camera
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ZoomCamera);

		// Toggle grid movement
		EnhancedInputComponent->BindAction(ToggleGridMovementAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ToggleGridMovement);

		// Interact with talking objects
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Interact);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectUmeowmiCharacter::Move(const FInputActionValue& Value)
{
	// Get the input value
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Use camera angle directly for movement direction, independent of character rotation
		const FRotator YawRotation(0, CameraOffset, 0);

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

void AProjectUmeowmiCharacter::InitializeCameraPosition()
{
	// Calculate the angle for the current camera position
	float AngleStep = 360.0f / NumberOfCameraPositions;
	float CurrentAngle = BaseCameraAngle + (CameraPositionIndex * AngleStep);
	
	// Update the target camera rotation based on calculated angle
	TargetCameraRotation = FRotator(-25.0f, CurrentAngle, 0.0f);
	CameraOffset = CurrentAngle;
	
	// Immediately set the camera rotation to avoid any interpolation delay
	if (CameraBoom)
	{
		CameraBoom->SetRelativeRotation(TargetCameraRotation);
	}
	
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera initialized - Position Index: %d, Angle: %f"), CameraPositionIndex, CurrentAngle);
}

void AProjectUmeowmiCharacter::InitializeCameraPositionFromBlueprint()
{
	InitializeCameraPosition();
}

void AProjectUmeowmiCharacter::GetCameraPositionIndex(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %f"), InputValue);
	
	//If Value is negated, then we need to rotate the camera to the right	
	// Q or LB
	if (InputValue < 0)
	{
		CameraPositionIndex = (CameraPositionIndex + 1) % NumberOfCameraPositions;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}
	
	//If Value is positive, then we need to rotate the camera to the left
	// E or RB
	else if (InputValue > 0)
	{
		CameraPositionIndex = (CameraPositionIndex - 1 + NumberOfCameraPositions) % NumberOfCameraPositions;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}

	// Calculate the angle for the current camera position
	float AngleStep = 360.0f / NumberOfCameraPositions;
	float CurrentAngle = BaseCameraAngle + (CameraPositionIndex * AngleStep);
	
	// Update the target camera rotation based on calculated angle
	TargetCameraRotation = FRotator(-25.0f, CurrentAngle, 0.0f);
	CameraOffset = CurrentAngle;
	
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Angle: %f"), CurrentAngle);
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

void AProjectUmeowmiCharacter::ZoomCamera(const FInputActionValue& Value)
{
	// Get the zoom input value
	float ZoomValue = Value.Get<float>();
	
	// Determine if this is from a controller or mouse wheel
	// Controller input typically comes as a float between -1 and 1
	// Mouse wheel typically comes as a float with values like -1, 0, or 1
	
	// Calculate the zoom amount based on the input source
	float ZoomAmount = 0.0f;
	
	// If the absolute value is close to 1, it's likely from a controller
	if (FMath::IsNearlyEqual(FMath::Abs(ZoomValue), 1.0f, 0.1f))
	{
		// Controller input - use the controller zoom speed
		ZoomAmount = ZoomValue * ControllerZoomSpeed;
	}
	else
	{
		// Mouse wheel input - use the mouse wheel zoom speed
		ZoomAmount = ZoomValue * MouseWheelZoomSpeed;
	}
	
	// Get the current orthographic width
	float CurrentOrthoWidth = FollowCamera->OrthoWidth;
	
	// Calculate the new orthographic width
	// Note: For orthographic cameras, smaller width = more zoomed in
	float NewOrthoWidth = FMath::Clamp(CurrentOrthoWidth + ZoomAmount, MinOrthoWidth, MaxOrthoWidth);
	
	// Apply the new orthographic width
	FollowCamera->OrthoWidth = NewOrthoWidth;
}

void AProjectUmeowmiCharacter::Interact(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::Interact - CurrentTalkingObject: %s, CurrentInteractable: %s"), 
		CurrentTalkingObject ? *CurrentTalkingObject->GetName() : TEXT("NULL"),
		CurrentInteractable ? TEXT("Valid") : TEXT("NULL"));
		
	if (CurrentTalkingObject)
	{
		CurrentTalkingObject->StartInteraction();
	}
	else if (CurrentInteractable)
	{
		CurrentInteractable->StartInteraction();
	}
}

void AProjectUmeowmiCharacter::RegisterTalkingObject(ATalkingObject* TalkingObject)
{
	// Store the talking object reference
	CurrentTalkingObject = TalkingObject;
	
	// Log the registration
	UE_LOG(LogTemp, Log, TEXT("Registered talking object: %s"), *TalkingObject->GetTalkingObjectDisplayName().ToString());
}

void AProjectUmeowmiCharacter::UnregisterTalkingObject(ATalkingObject* TalkingObject)
{
	// Only unregister if this is the current talking object
	if (CurrentTalkingObject == TalkingObject)
	{
		// Clear the reference
		CurrentTalkingObject = nullptr;
		
		// Log the unregistration
		UE_LOG(LogTemp, Log, TEXT("Unregistered talking object: %s"), *TalkingObject->GetTalkingObjectDisplayName().ToString());
	}
}

void AProjectUmeowmiCharacter::RegisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable)
{
	if (Interactable)
	{
		CurrentInteractable = Interactable;
		
		// Bind to interaction events
		Interactable->OnInteractionStarted().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionStarted);
		Interactable->OnInteractionEnded().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionEnded);
		Interactable->OnInteractionFailed().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionFailed);
	}
}

void AProjectUmeowmiCharacter::UnregisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable)
{
	if (CurrentInteractable == Interactable)
	{
		// Unbind from interaction events
		if (Interactable)
		{
			Interactable->OnInteractionStarted().RemoveAll(this);
			Interactable->OnInteractionEnded().RemoveAll(this);
			Interactable->OnInteractionFailed().RemoveAll(this);
		}
		
		CurrentInteractable = nullptr;
	}
}

void AProjectUmeowmiCharacter::OnInteractionStarted()
{
	// Handle interaction started
	UE_LOG(LogTemp, Log, TEXT("Interaction started"));
}

void AProjectUmeowmiCharacter::OnInteractionEnded()
{
	// Handle interaction ended
	UE_LOG(LogTemp, Log, TEXT("Interaction ended"));
}

void AProjectUmeowmiCharacter::OnInteractionFailed()
{
	// Handle interaction failed
	UE_LOG(LogTemp, Log, TEXT("Interaction failed"));
}

// Order System Integration
void AProjectUmeowmiCharacter::SetCurrentOrder(const FPUOrderBase& Order)
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Setting current order: %s"), *Order.OrderID.ToString());
	
	// Clean up any existing UObject references before setting new order
	if (bHasCurrentOrder)
	{
		UE_LOG(LogTemp, Display, TEXT("SetCurrentOrder - Cleaning up existing UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
	}
	
	CurrentOrder = Order;
	bHasCurrentOrder = true;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;
	
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Order set successfully"));
}

void AProjectUmeowmiCharacter::ClearCurrentOrder()
{
	UE_LOG(LogTemp, Display, TEXT("=== CLEARING CURRENT ORDER ==="));
	UE_LOG(LogTemp, Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	UE_LOG(LogTemp, Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	UE_LOG(LogTemp, Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Properly clean up UObject references before clearing
	UE_LOG(LogTemp, Display, TEXT("ClearCurrentOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;
	
	UE_LOG(LogTemp, Display, TEXT("=== ORDER CLEARED ==="));
	UE_LOG(LogTemp, Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	UE_LOG(LogTemp, Display, TEXT("========================="));
}

void AProjectUmeowmiCharacter::SetOrderResult(bool bCompleted, float SatisfactionScore)
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::SetOrderResult - Order completed: YES, Satisfaction: %.2f"), 
		SatisfactionScore);
	
	// Orders are always completed when submitted - satisfaction score indicates quality
	bHasCurrentOrder = false;      // Clear active flag
	bCurrentOrderCompleted = true; // Set completed flag
	CurrentOrderSatisfaction = SatisfactionScore;
	
	// Display the order result immediately
	DisplayOrderResult();
}

void AProjectUmeowmiCharacter::DisplayOrderResult()
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::DisplayOrderResult - Displaying order result"));
	
	// Orders are always completed - satisfaction score indicates quality
	// Determine satisfaction level
	FString SatisfactionLevel;
	if (CurrentOrderSatisfaction >= 0.9f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.7f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.5f)
	{
		SatisfactionLevel = TEXT("Good!");
	}
	else
	{
		SatisfactionLevel = TEXT("Okay.");
	}
	
	UE_LOG(LogTemp, Display, TEXT("=== ORDER COMPLETED ==="));
	UE_LOG(LogTemp, Display, TEXT("Order: %s"), *CurrentOrder.OrderDescription.ToString());
	UE_LOG(LogTemp, Display, TEXT("Satisfaction: %s (%.1f%%)"), *SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	UE_LOG(LogTemp, Display, TEXT("====================="));
	
	// Call the order completed event
	OnOrderCompleted();
}

void AProjectUmeowmiCharacter::ClearCompletedOrder()
{
	UE_LOG(LogTemp, Display, TEXT("=== CLEARING COMPLETED ORDER ==="));
	UE_LOG(LogTemp, Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	UE_LOG(LogTemp, Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	UE_LOG(LogTemp, Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Validate that we can safely clear the order
	if (!bCurrentOrderCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClearCompletedOrder - Cannot clear: order not completed"));
		return;
	}
	
	// Properly clean up UObject references before clearing
	UE_LOG(LogTemp, Display, TEXT("ClearCompletedOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;
	
	UE_LOG(LogTemp, Display, TEXT("=== COMPLETED ORDER CLEARED ==="));
	UE_LOG(LogTemp, Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	UE_LOG(LogTemp, Display, TEXT("================================="));
}

FText AProjectUmeowmiCharacter::GetOrderResultText() const
{
	if (!bCurrentOrderCompleted)
	{
		return FText::FromString(TEXT("No order completed yet."));
	}
	
	// Orders are always completed - satisfaction score indicates quality
	// Determine satisfaction level
	FString SatisfactionLevel;
	if (CurrentOrderSatisfaction >= 0.9f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.7f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.5f)
	{
		SatisfactionLevel = TEXT("Good!");
	}
	else
	{
		SatisfactionLevel = TEXT("Okay.");
	}
	
	FString ResultText = FString::Printf(TEXT("Order Completed!\nSatisfaction: %s (%.1f%%)"), 
		*SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	
	return FText::FromString(ResultText);
}

void AProjectUmeowmiCharacter::OnOrderCompleted()
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::OnOrderCompleted - Order completed successfully!"));
	
	// This function can be overridden in Blueprints to add visual/audio feedback
	// For now, just log the completion
	UE_LOG(LogTemp, Display, TEXT("🎉 ORDER COMPLETED! 🎉"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction: %.1f%%"), CurrentOrderSatisfaction * 100.0f);
}

void AProjectUmeowmiCharacter::OnOrderFailed()
{
	UE_LOG(LogTemp, Display, TEXT("ProjectUmeowmiCharacter::OnOrderFailed - Order completed with low satisfaction"));
	
	// This function can be overridden in Blueprints to add visual/audio feedback
	// Note: Orders are now always completed - this event is for low satisfaction scenarios
	UE_LOG(LogTemp, Display, TEXT("⚠️ ORDER COMPLETED WITH LOW SATISFACTION ⚠️"));
	UE_LOG(LogTemp, Display, TEXT("Satisfaction: %.1f%% - Try again for better results!"), CurrentOrderSatisfaction * 100.0f);
}

void AProjectUmeowmiCharacter::CleanupOrderUObjectReferences(FPUOrderBase& Order)
{
	// Clear UObject references in the completed dish
	if (Order.CompletedDish.PreviewTexture)
	{
		Order.CompletedDish.PreviewTexture = nullptr;
	}
	if (Order.CompletedDish.IngredientDataTable.IsValid())
	{
		Order.CompletedDish.IngredientDataTable = nullptr;
	}
	
	// Clear UObject references in all ingredient instances
	for (FIngredientInstance& Instance : Order.CompletedDish.IngredientInstances)
	{
		if (Instance.IngredientData.PreviewTexture)
		{
			Instance.IngredientData.PreviewTexture = nullptr;
		}
		if (Instance.IngredientData.MaterialInstance.IsValid())
		{
			Instance.IngredientData.MaterialInstance = nullptr;
		}
		if (Instance.IngredientData.IngredientMesh.IsValid())
		{
			Instance.IngredientData.IngredientMesh = nullptr;
		}
		if (Instance.IngredientData.PreparationDataTable.IsValid())
		{
			Instance.IngredientData.PreparationDataTable = nullptr;
		}
	}
	
	// Clear UObject references in the base dish
	if (Order.BaseDish.PreviewTexture)
	{
		Order.BaseDish.PreviewTexture = nullptr;
	}
	if (Order.BaseDish.IngredientDataTable.IsValid())
	{
		Order.BaseDish.IngredientDataTable = nullptr;
	}
	
	// Clear UObject references in base dish ingredient instances
	for (FIngredientInstance& Instance : Order.BaseDish.IngredientInstances)
	{
		if (Instance.IngredientData.PreviewTexture)
		{
			Instance.IngredientData.PreviewTexture = nullptr;
		}
		if (Instance.IngredientData.MaterialInstance.IsValid())
		{
			Instance.IngredientData.MaterialInstance = nullptr;
		}
		if (Instance.IngredientData.IngredientMesh.IsValid())
		{
			Instance.IngredientData.IngredientMesh = nullptr;
		}
		if (Instance.IngredientData.PreparationDataTable.IsValid())
		{
			Instance.IngredientData.PreparationDataTable = nullptr;
		}
	}
}

void AProjectUmeowmiCharacter::ShowMouseCursor()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}
}

void AProjectUmeowmiCharacter::HideMouseCursor()
{
	// Do nothing - we want the cursor to always be visible
}

void AProjectUmeowmiCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up character: %s"), *GetName());
	
	// Clear order UObject references to prevent garbage collection issues
	if (bHasCurrentOrder || bCurrentOrderCompleted)
	{
		UE_LOG(LogTemp, Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up order UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
		
		// Clear the order data
		CurrentOrder = FPUOrderBase();
		bHasCurrentOrder = false;
		bCurrentOrderCompleted = false;
		CurrentOrderSatisfaction = 0.0f;
	}
	
	// Clear talking object reference to prevent dangling references
	if (CurrentTalkingObject)
	{
		UE_LOG(LogTemp, Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing talking object reference"));
		CurrentTalkingObject = nullptr;
	}
	
	// Clear interactable reference
	if (CurrentInteractable)
	{
		UE_LOG(LogTemp, Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing interactable reference"));
		CurrentInteractable = nullptr;
	}
	
	// Clear dialogue box reference
	if (DialogueBox)
	{
		UE_LOG(LogTemp, Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing dialogue box reference"));
		DialogueBox = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void AProjectUmeowmiCharacter::SetMousePosition(int32 X, int32 Y)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetMouseLocation(X, Y);
	}
}

void AProjectUmeowmiCharacter::CenterMouseCursor()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		PC->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
	}
}


