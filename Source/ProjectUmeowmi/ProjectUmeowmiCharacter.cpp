// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DataTable.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Dialogue/TalkingObject.h"
#include "DishCustomization/PUDishCustomizationComponent.h"
#include "UI/PUDialogueBox.h"
#include "UI/PUEmoteData.h"
#include "UI/PUEmoteWidget.h"
#include "UI/PUJournalWidget.h"
#include "ProjectUmeowmi/UI/PUScorecardWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

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

	// Create emote widget (above character head) - do NOT override Space; user sets Screen in Blueprint
	EmoteWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("EmoteWidget"));
	EmoteWidget->SetupAttachment(RootComponent);
	EmoteWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // Above character head
	EmoteWidget->SetVisibility(false); // Hidden until ShowEmoteByTag

	// Create dish preview (above character head when carrying a dish)
	DishPreviewComponent = CreateDefaultSubobject<UPUDishPreviewComponent>(TEXT("DishPreview"));
	DishPreviewComponent->SetupAttachment(RootComponent);
	DishPreviewComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f)); // Above character head

	// Dish mesh created here (not in DishPreviewComponent) to avoid template/instance attachment mismatch in Blueprint subclasses (BP_Bao)
	DishPreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DishPreviewMesh"));
	DishPreviewMeshComponent->SetupAttachment(DishPreviewComponent);
	DishPreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DishPreviewMeshComponent->SetCastShadow(true);
	DishPreviewMeshComponent->SetVisibility(false);
	DishPreviewComponent->SetDishMeshComponent(DishPreviewMeshComponent);

	// Initialize target camera rotation
	TargetCameraRotation = FRotator(-15.0f, 45.0f, 0.0f);

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

	// Configure emote widget - set class; Draw Size comes from component in Blueprint
	// Must set bDrawAtDesiredSize=false or widget's desired size (e.g. 256) overrides component's Draw Size
	if (EmoteWidget && bEnableEmotes && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
	}

	//UE_LOG(LogTemp,Log, TEXT("Character BeginPlay - Camera initialized with position index: %d"), CameraPositionIndex);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProjectUmeowmiCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		//UE_LOG(LogTemp,Log, TEXT("Controller changed - PlayerController found"));
		
		// Always show mouse cursor
		PlayerController->bShowMouseCursor = true;
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
		
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			//UE_LOG(LogTemp,Log, TEXT("Enhanced Input Subsystem found"));
			
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				//UE_LOG(LogTemp,Log, TEXT("Default mapping context added successfully"));
			}
			else
			{
				//UE_LOG(LogTemp,Warning, TEXT("Default mapping context is null!"));
			}
		}
		else
		{
			//UE_LOG(LogTemp,Warning, TEXT("Enhanced Input Subsystem not found!"));
		}
	}
	else
	{
		//UE_LOG(LogTemp,Warning, TEXT("Controller changed - No PlayerController found!"));
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

		// Cycle between overlapping interact targets (Space bar)
		if (CycleInteractTargetAction)
		{
			EnhancedInputComponent->BindAction(CycleInteractTargetAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::CycleInteractTarget);
		}

		// Open/toggle journal (Start button, I key)
		if (OpenJournalAction)
		{
			EnhancedInputComponent->BindAction(OpenJournalAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ToggleJournal);
		}

		// Journal Recipes tab: cycle dishes with bumpers (only when journal open on Recipes)
		if (JournalCycleDishPrevAction)
		{
			EnhancedInputComponent->BindAction(JournalCycleDishPrevAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::OnJournalCycleDishPrev);
		}
		if (JournalCycleDishNextAction)
		{
			EnhancedInputComponent->BindAction(JournalCycleDishNextAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::OnJournalCycleDishNext);
		}

		// Hold to skip dialogue (fast typewriter, no sound, auto-advance)
		if (SkipDialogueAction)
		{
			EnhancedInputComponent->BindAction(SkipDialogueAction, ETriggerEvent::Started, this, &AProjectUmeowmiCharacter::OnSkipDialogueStarted);
			EnhancedInputComponent->BindAction(SkipDialogueAction, ETriggerEvent::Completed, this, &AProjectUmeowmiCharacter::OnSkipDialogueCompleted);
		}

		// Jump
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectUmeowmiCharacter::Move(const FInputActionValue& Value)
{
	// Block movement when journal is open
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		return;
	}

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
	// When journal is open, bumpers cycle dishes instead of rotating camera
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		return;
	}

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

	if (ATalkingObject* CurrentTalking = GetCurrentTalkingObject())
	{
		CurrentTalking->TickFacePlayerLerp(DeltaTime);
	}
	else
	{
		// Debug: uncomment to verify player Tick runs when no talking object
		// static int32 FrameCount = 0;
		// if (++FrameCount % 300 == 0) UE_LOG(LogTemp, Log, TEXT("[FacePlayerLerp] Player Tick, no CurrentTalkingObject"));
	}

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
	if (bLockOrthoWidth)
	{
		if (bShowOrthoWidthDebug)
		{
			UE_LOG(LogTemplateCharacter, Display, TEXT("[OrthoWidth] Locked at %.1f (zoom input ignored)"), FollowCamera->OrthoWidth);
		}
		return;
	}

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

	if (bShowOrthoWidthDebug)
	{
		UE_LOG(LogTemplateCharacter, Display, TEXT("[OrthoWidth] %.1f (range: %.1f - %.1f)"), NewOrthoWidth, MinOrthoWidth, MaxOrthoWidth);
	}
}

void AProjectUmeowmiCharacter::ToggleJournal(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		// Fallback: search for journal widget in the world (e.g. if it's a child of HUD)
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), /*bTopLevelOnly=*/ false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}

	if (Journal)
	{
		const bool bIsVisible = Journal->GetVisibility() == ESlateVisibility::Visible;
		if (bIsVisible)
		{
			Journal->CloseJournal();
		}
		else
		{
			Journal->OpenJournal();
		}
	}
}

void AProjectUmeowmiCharacter::OnJournalCycleDishPrev(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		Journal->CycleRecipesDish(-1);
	}
}

void AProjectUmeowmiCharacter::OnSkipDialogueStarted(const FInputActionValue& Value)
{
	if (DialogueBox && DialogueBox->GetVisibility() == ESlateVisibility::Visible)
	{
		DialogueBox->SetSkipMode(true);
	}
}

void AProjectUmeowmiCharacter::OnSkipDialogueCompleted(const FInputActionValue& Value)
{
	if (DialogueBox)
	{
		DialogueBox->SetSkipMode(false);
	}
}

void AProjectUmeowmiCharacter::OnJournalCycleDishNext(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		Journal->CycleRecipesDish(1);
	}
}

void AProjectUmeowmiCharacter::Interact(const FInputActionValue& Value)
{
	ATalkingObject* CurrentTalking = GetCurrentTalkingObject();

	// When in dialogue, Interact advances the dialogue (skip typewriter or next line)
	if (CurrentTalking && DialogueBox && DialogueBox->GetVisibility() == ESlateVisibility::Visible)
	{
		DialogueBox->AdvanceDialogue();
		return;
	}
		
	if (CurrentTalking)
	{
		CurrentTalking->StartInteraction();
	}
	else if (CurrentInteractable)
	{
		CurrentInteractable->StartInteraction();
	}
}

ATalkingObject* AProjectUmeowmiCharacter::GetCurrentTalkingObject() const
{
	if (OverlappingTalkingObjects.IsValidIndex(SelectedTalkingObjectIndex))
	{
		ATalkingObject* Obj = OverlappingTalkingObjects[SelectedTalkingObjectIndex];
		return (Obj && IsValid(Obj)) ? Obj : nullptr;
	}
	return nullptr;
}

void AProjectUmeowmiCharacter::RegisterTalkingObject(ATalkingObject* TalkingObject)
{
	if (!TalkingObject || !IsValid(TalkingObject)) return;

	// Add to list if not already present (avoid duplicates from overlap order)
	int32 ExistingIndex = OverlappingTalkingObjects.Find(TalkingObject);
	if (ExistingIndex == INDEX_NONE)
	{
		OverlappingTalkingObjects.Add(TalkingObject);
		if (OverlappingTalkingObjects.Num() == 1)
		{
			SelectedTalkingObjectIndex = 0;
		}
		else
		{
			// Most recent overlap becomes the selected target
			SelectedTalkingObjectIndex = OverlappingTalkingObjects.Num() - 1;
		}
		// Refresh all overlapping widgets so opacity updates immediately (e.g. first one fades when second overlaps)
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
		}
	}
}

void AProjectUmeowmiCharacter::UnregisterTalkingObject(ATalkingObject* TalkingObject)
{
	if (!TalkingObject) return;

	int32 RemovedIndex = OverlappingTalkingObjects.Find(TalkingObject);
	if (RemovedIndex != INDEX_NONE)
	{
		OverlappingTalkingObjects.RemoveAt(RemovedIndex);
		// Clamp selection index after removal
		if (OverlappingTalkingObjects.Num() == 0)
		{
			SelectedTalkingObjectIndex = 0;
		}
		else if (SelectedTalkingObjectIndex >= OverlappingTalkingObjects.Num())
		{
			SelectedTalkingObjectIndex = OverlappingTalkingObjects.Num() - 1;
		}
		else if (RemovedIndex < SelectedTalkingObjectIndex)
		{
			SelectedTalkingObjectIndex--;
		}
		// Refresh remaining widgets so the last one returns to full opacity
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
		}
	}
}

void AProjectUmeowmiCharacter::CycleInteractTarget(const FInputActionValue& Value)
{
	// Don't cycle during active dialogue
	if (DialogueBox && DialogueBox->GetVisibility() == ESlateVisibility::Visible) return;
	if (OverlappingTalkingObjects.Num() < 2) return;

	// Cycle forward (Space = next)
	SelectedTalkingObjectIndex = (SelectedTalkingObjectIndex + 1) % OverlappingTalkingObjects.Num();

		// Refresh all overlapping widgets so they can update selection state (e.g. highlight selected)
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
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
	//UE_LOG(LogTemp,Log, TEXT("Interaction started"));
}

void AProjectUmeowmiCharacter::OnInteractionEnded()
{
	// Handle interaction ended
	//UE_LOG(LogTemp,Log, TEXT("Interaction ended"));
}

void AProjectUmeowmiCharacter::OnInteractionFailed()
{
	// Handle interaction failed
	//UE_LOG(LogTemp,Log, TEXT("Interaction failed"));
}

//////////////////////////////////////////////////////////////////////////
// Emote

void AProjectUmeowmiCharacter::BeginFadeOutEmote()
{
	if (!EmoteWidget)
	{
		ClearEmote();
		return;
	}

	if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
	{
		EmoteUserWidget->PlayFadeOut();

		const float FadeDuration = EmoteUserWidget->GetFadeUpDuration();
		const float TimerDuration = (FadeDuration > 0.0f) ? (FadeDuration / 2.0f) : 0.25f;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
			World->GetTimerManager().SetTimer(EmoteFadeOutTimerHandle, this, &AProjectUmeowmiCharacter::ClearEmote, TimerDuration, false);
		}
		else
		{
			ClearEmote();
		}
	}
	else
	{
		ClearEmote();
	}
}

void AProjectUmeowmiCharacter::ShowEmoteByTag(FGameplayTag EmoteTag)
{
	if (!bEnableEmotes || !EmoteWidget)
	{
		return;
	}

	if (!EmoteTag.IsValid())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - Invalid emote tag on %s"), *GetName());
		return;
	}

	if (!EmoteDataTable)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - EmoteDataTable is not set on %s"), *GetName());
		return;
	}

	// DataTable rows use the tag's leaf name (part after last '.') lowercased, e.g. Emote.Happy -> "happy"
	FString TagStr = EmoteTag.ToString();
	int32 LastDot = INDEX_NONE;
	if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot >= 0)
	{
		TagStr = TagStr.Mid(LastDot + 1);
	}
	TagStr = TagStr.ToLower();
	const FName RowName = FName(*TagStr);

	const FPUEmoteData* EmoteRow = EmoteDataTable->FindRow<FPUEmoteData>(RowName, TEXT("ShowEmoteByTag"));
	if (!EmoteRow)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - No emote data row for tag %s on %s"), *EmoteTag.ToString(), *GetName());
		return;
	}

	if (!EmoteRow->Icon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - Emote row %s has no Icon on %s"), *RowName.ToString(), *GetName());
		return;
	}

	// Ensure widget is created - do NOT override Space or DrawSize (set on component in Blueprint)
	if (!EmoteWidget->GetWidget() && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
	}

	UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
	if (!EmoteUserWidget && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
		EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
	}

	if (!EmoteUserWidget)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - EmoteWidget is not of type UPUEmoteWidget on %s (set EmoteWidgetClass on this actor)"), *GetName());
		return;
	}

	EmoteUserWidget->SetEmoteIcon(EmoteRow->Icon);
	EmoteWidget->SetDrawAtDesiredSize(false); // Use component's Draw Size (128), not widget's desired size (256)
	EmoteWidget->SetVisibility(true);
	EmoteUserWidget->PlayFadeIn();
	ActiveEmoteTag = EmoteTag;

	if (EmoteRow->Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EmoteRow->Sound, GetActorLocation());
	}

	if (!EmoteRow->bLoop)
	{
		const float Duration = EmoteRow->Duration > 0.0f ? EmoteRow->Duration : 2.0f;
		if (UWorld* WorldPtr = GetWorld())
		{
			WorldPtr->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
			WorldPtr->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
			WorldPtr->GetTimerManager().SetTimer(EmoteHideTimerHandle, this, &AProjectUmeowmiCharacter::BeginFadeOutEmote, Duration, false);
		}
	}
}

void AProjectUmeowmiCharacter::ClearEmote()
{
	if (EmoteWidget)
	{
		if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
		{
			EmoteUserWidget->ClearEmoteIcon();
		}
		EmoteWidget->SetVisibility(false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
		World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
	}

	ActiveEmoteTag = FGameplayTag();
}

bool AProjectUmeowmiCharacter::IsEmoteActive() const
{
	return EmoteWidget && EmoteWidget->IsVisible();
}

// Order System Integration
void AProjectUmeowmiCharacter::SetCurrentOrder(const FPUOrderBase& Order)
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Setting current order: %s"), *Order.OrderID.ToString());
	
	// Clean up any existing UObject references before setting new order
	if (bHasCurrentOrder)
	{
		//UE_LOG(LogTemp,Display, TEXT("SetCurrentOrder - Cleaning up existing UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
	}
	
	CurrentOrder = Order;
	bHasCurrentOrder = true;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;
	
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Order set successfully"));
}

void AProjectUmeowmiCharacter::RevealHintOnCurrentOrder(FName AspectName)
{
	UE_LOG(LogTemp, Display, TEXT("[Hint] RevealHintOnCurrentOrder(%s) - hasOrder=%d completed=%d"), *AspectName.ToString(), bHasCurrentOrder, bCurrentOrderCompleted);

	if (!bHasCurrentOrder || bCurrentOrderCompleted || AspectName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintOnCurrentOrder aborted - no active order or aspect invalid"));
		return;
	}

	for (const FOrderAspectRequirement& Req : CurrentOrder.TargetAspects)
	{
		if (Req.GetAspectName() == AspectName)
		{
			// Check if already discovered
			for (const FOrderAspectRequirement& Discovered : CurrentOrder.DiscoveredHints)
			{
				if (Discovered.GetAspectName() == AspectName)
				{
					UE_LOG(LogTemp, Display, TEXT("[Hint] Hint %s already revealed, skipping"), *AspectName.ToString());
					return;
				}
			}
			CurrentOrder.DiscoveredHints.Add(Req);
			UE_LOG(LogTemp, Display, TEXT("[Hint] SUCCESS: Revealed hint %s (target %.1f) - total discovered: %d"), *AspectName.ToString(), Req.TargetValue, CurrentOrder.DiscoveredHints.Num());
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintOnCurrentOrder - aspect %s not in order's TargetAspects"), *AspectName.ToString());
}

void AProjectUmeowmiCharacter::ClearCurrentOrder()
{
	//UE_LOG(LogTemp,Display, TEXT("=== CLEARING CURRENT ORDER ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Properly clean up UObject references before clearing
	//UE_LOG(LogTemp,Display, TEXT("ClearCurrentOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;

	// Clear the dish preview above the character's head
	if (DishPreviewComponent)
	{
		DishPreviewComponent->ClearPreview();
	}
	
	//UE_LOG(LogTemp,Display, TEXT("=== ORDER CLEARED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	//UE_LOG(LogTemp,Display, TEXT("========================="));
}

void AProjectUmeowmiCharacter::SetOrderResult(bool bCompleted, float SatisfactionScore)
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetOrderResult - Order completed: YES, Satisfaction: %.2f"), 
	//	SatisfactionScore);
	
	// Orders are always completed when submitted - satisfaction score indicates quality
	bHasCurrentOrder = false;      // Clear active flag
	bCurrentOrderCompleted = true; // Set completed flag
	CurrentOrderSatisfaction = SatisfactionScore;
	
	// Display the order result immediately
	DisplayOrderResult();
}

void AProjectUmeowmiCharacter::DisplayOrderResult()
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::DisplayOrderResult - Displaying order result"));
	
	// Orders are always completed - satisfaction score indicates quality
	// Determine satisfaction level
	FString SatisfactionLevel;
	if (CurrentOrderSatisfaction >= 0.875f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.625f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.375f)
	{
		SatisfactionLevel = TEXT("Okay!");
	}
	else
	{
		SatisfactionLevel = TEXT("Needs Improvement.");
	}
	
	//UE_LOG(LogTemp,Display, TEXT("=== ORDER COMPLETED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction: %s (%.1f%%)"), *SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	//UE_LOG(LogTemp,Display, TEXT("====================="));
	
	// Call the order completed event
	OnOrderCompleted();
}

void AProjectUmeowmiCharacter::ClearCompletedOrder()
{
	//UE_LOG(LogTemp,Display, TEXT("=== CLEARING COMPLETED ORDER ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Validate that we can safely clear the order
	if (!bCurrentOrderCompleted)
	{
		//UE_LOG(LogTemp,Warning, TEXT("ClearCompletedOrder - Cannot clear: order not completed"));
		return;
	}
	
	// Properly clean up UObject references before clearing
	//UE_LOG(LogTemp,Display, TEXT("ClearCompletedOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;

	// Clear the dish preview above the character's head
	if (DishPreviewComponent)
	{
		DishPreviewComponent->ClearPreview();
	}
	
	//UE_LOG(LogTemp,Display, TEXT("=== COMPLETED ORDER CLEARED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	//UE_LOG(LogTemp,Display, TEXT("================================="));
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
	if (CurrentOrderSatisfaction >= 0.875f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.625f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.375f)
	{
		SatisfactionLevel = TEXT("Okay!");
	}
	else
	{
		SatisfactionLevel = TEXT("Needs Improvement.");
	}
	
	FString ResultText = FString::Printf(TEXT("Order Completed!\nSatisfaction: %s (%.1f%%)"), 
		*SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	
	return FText::FromString(ResultText);
}

void AProjectUmeowmiCharacter::OnOrderCompleted()
{
}

UPUScorecardWidget* AProjectUmeowmiCharacter::ShowScorecard(TSubclassOf<UPUScorecardWidget> ScorecardWidgetClass)
{
	UE_LOG(LogTemp, Display, TEXT("[Scorecard] ShowScorecard called: bCurrentOrderCompleted=%d, Class=%s"), bCurrentOrderCompleted ? 1 : 0, ScorecardWidgetClass ? *ScorecardWidgetClass->GetName() : TEXT("NULL"));
	if (!bCurrentOrderCompleted || !ScorecardWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] ShowScorecard aborted: order not completed or class null"));
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] ShowScorecard aborted: no PlayerController"));
		return nullptr;
	}

	UPUScorecardWidget* Widget = CreateWidget<UPUScorecardWidget>(PC, ScorecardWidgetClass);
	if (!Widget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] ShowScorecard aborted: CreateWidget failed"));
		return nullptr;
	}

	UE_LOG(LogTemp, Display, TEXT("[Scorecard] AddToViewport + ShowFromOrder (Order has %d base ingredients, %d completed ingredients)"), CurrentOrder.BaseDish.IngredientInstances.Num(), CurrentOrder.GetCompletedDish().IngredientInstances.Num());
	Widget->AddToViewport();
	Widget->ShowFromOrder(CurrentOrder, nullptr);
	return Widget;
}

void AProjectUmeowmiCharacter::OnOrderFailed()
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::OnOrderFailed - Order completed with low satisfaction"));
	
	// This function can be overridden in Blueprints to add visual/audio feedback
	// Note: Orders are now always completed - this event is for low satisfaction scenarios
	//UE_LOG(LogTemp,Display, TEXT("⚠️ ORDER COMPLETED WITH LOW SATISFACTION ⚠️"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction: %.1f%% - Try again for better results!"), CurrentOrderSatisfaction * 100.0f);
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
	//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up character: %s"), *GetName());
	
	// Clear order UObject references to prevent garbage collection issues
	if (bHasCurrentOrder || bCurrentOrderCompleted)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up order UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
		
		// Clear the order data
		CurrentOrder = FPUOrderBase();
		bHasCurrentOrder = false;
		bCurrentOrderCompleted = false;
		CurrentOrderSatisfaction = 0.0f;
	}
	
	// Clear overlapping talking objects to prevent dangling references
	OverlappingTalkingObjects.Empty();
	SelectedTalkingObjectIndex = 0;
	
	// Clear interactable reference
	if (CurrentInteractable)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing interactable reference"));
		CurrentInteractable = nullptr;
	}
	
	// Clear dialogue box reference
	if (DialogueBox)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing dialogue box reference"));
		DialogueBox = nullptr;
	}

	// Clear journal widget reference
	if (JournalWidget)
	{
		JournalWidget = nullptr;
	}

	// Clear any pending emote timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
		World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
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


