#include "PUDishCustomizationComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../ProjectUmeowmiCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "../UI/PUPlatingWidget.h"
#include "../UI/PUIngredientSlot.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "PUIngredientMesh.h"
#include "Camera/CameraActor.h"

UPUDishCustomizationComponent::UPUDishCustomizationComponent()
{
    PrimaryComponentTick.bCanEverTick = true; // Enable tick for camera transitions
}

void UPUDishCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPUDishCustomizationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsTransitioningCamera && CurrentCharacter)
    {
        UpdateCameraTransition(DeltaTime);
    }

    // Update plating camera transition if active
    if (bPlatingCameraTransitioning)
    {
        UpdatePlatingCameraTransition(DeltaTime);
    }

    // Update mouse dragging if active
    if (bIsDragging)
    {
        UpdateMouseDrag();
    }
}

void UPUDishCustomizationComponent::StartCustomization(AProjectUmeowmiCharacter* Character)
{
    UE_LOG(LogTemp, Display, TEXT("🚀 UPUDishCustomizationComponent::StartCustomization - STARTING CUSTOMIZATION"));
    
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Character"));
        return;
    }

    // Store the original dish container mesh before any changes
    StoreOriginalDishContainerMesh();

    UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Character valid: %s"), *Character->GetName());
    CurrentCharacter = Character;

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Player Controller"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Player Controller valid: %s"), *PlayerController->GetName());

    // Set input mode first to ensure the input system is ready
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(nullptr);
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);

    // Enable mouse cursor and input
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
    PlayerController->bShowMouseCursor = true;
    PlayerController->CurrentMouseCursor = EMouseCursor::Default;
    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;

    // Center the mouse cursor
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    PlayerController->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);

    UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Input mode set, viewport size: %dx%d"), ViewportSizeX, ViewportSizeY);

    // Handle mapping contexts
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
        UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Found Enhanced Input Subsystem"));
        
        // Store the original mapping context
        OriginalMappingContext = Character->GetDefaultMappingContext();
        UE_LOG(LogTemp, Display, TEXT("📋 UPUDishCustomizationComponent::StartCustomization - Original mapping context: %s"), 
            OriginalMappingContext ? *OriginalMappingContext->GetName() : TEXT("None"));
        
        // Remove the original mapping context
        if (OriginalMappingContext)
        {
            Subsystem->RemoveMappingContext(OriginalMappingContext);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed original mapping context"));
        }

        // Add the customization mapping context
        if (CustomizationMappingContext)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Adding customization mapping context: %s"), *CustomizationMappingContext->GetName());
            Subsystem->AddMappingContext(CustomizationMappingContext, 0);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Added customization mapping context"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No CustomizationMappingContext set"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Subsystem"));
    }

    // Setup input handling for exit action and controller mouse
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Found Enhanced Input Component"));
        
        // First unbind any existing bindings to ensure clean state
        if (ControllerMouseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing controller mouse binding"));
        }
        
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing exit action binding"));
        }

        // Now set up new bindings
        if (ExitCustomizationAction)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding exit action: %s"), *ExitCustomizationAction->GetName());
            ExitActionBindingHandle = EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput).GetHandle();
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Exit action bound with handle: %d"), ExitActionBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ExitCustomizationAction set"));
        }

        if (ControllerMouseAction)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding controller mouse action: %s"), *ControllerMouseAction->GetName());
            // Bind to both Ongoing and Triggered events to ensure we catch all input
            ControllerMouseBindingHandle = EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Ongoing, this, &UPUDishCustomizationComponent::HandleControllerMouse).GetHandle();
            EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleControllerMouse);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Controller mouse action bound with handle: %d"), ControllerMouseBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ControllerMouseAction set"));
        }

        if (MouseClickAction)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding mouse click action: %s"), *MouseClickAction->GetName());
            MouseClickBindingHandle = EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Started, this, &UPUDishCustomizationComponent::HandleMouseClick).GetHandle();
            EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Completed, this, &UPUDishCustomizationComponent::HandleMouseRelease);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Mouse click action bound with handle: %d"), MouseClickBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No MouseClickAction set"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Component"));
    }

    // Create and show the customization widget
    UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to create widget"));
    UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - CustomizationWidgetClass: %s"), 
        CustomizationWidgetClass ? *CustomizationWidgetClass->GetName() : TEXT("NULL"));
    
    if (CustomizationWidgetClass)
    {
        UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Creating widget with class: %s"), *CustomizationWidgetClass->GetName());
        
        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No World available for widget creation"));
            return;
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - World valid: %s"), *World->GetName());
        
        CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
        if (CustomizationWidget)
        {
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget created successfully: %s"), *CustomizationWidget->GetName());
            
            // Try to cast to our custom widget class and set up the connection
            if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
            {
                UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is PUDishCustomizationWidget, connecting to component"));
                DishWidget->SetCustomizationComponent(this);
                
                UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Component connection completed"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is not a PUDishCustomizationWidget, manual connection may be needed"));
                UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidget->GetClass()->GetName());
            }
            
            // Pass the initial dish data to the widget during creation
            // The widget can access this data in its construction script or BeginPlay
            UE_LOG(LogTemp, Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to add widget to viewport"));
            
            // Add to viewport with a lower Z-Order so it doesn't override the recipe book widget
            CustomizationWidget->AddToViewport(250);
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget added to viewport successfully with Z-Order -100"));
            
            // Check if widget is visible
            if (CustomizationWidget->IsVisible())
            {
                UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is visible"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is NOT visible"));
            }
            
            // Broadcast the initial dish data now that the widget is created and subscribed
            if (CurrentDishData.DishTag.IsValid())
            {
                UE_LOG(LogTemp, Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Broadcasting initial dish data to newly created widget"));
                UE_LOG(LogTemp, Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Dish data: %s"), *CurrentDishData.DisplayName.ToString());
                BroadcastInitialDishData(CurrentDishData);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No valid dish data to broadcast"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to create Customization UI Widget"));
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidgetClass->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No CustomizationWidgetClass set"));
    }

    // Start camera transition to customization view
    UE_LOG(LogTemp, Display, TEXT("🎬 UPUDishCustomizationComponent::StartCustomization - Starting camera transition"));
    StartCameraTransition(true);
    
    UE_LOG(LogTemp, Display, TEXT("🎉 UPUDishCustomizationComponent::StartCustomization - CUSTOMIZATION STARTED SUCCESSFULLY"));
}

void UPUDishCustomizationComponent::EndCustomization()
{
    if (!CurrentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("No current character in EndCustomization"));
        return;
    }

    // End plating stage if we're in plating mode
    if (bPlatingMode)
    {
        EndPlatingStage();
    }

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (PlayerController)
    {
        // Re-enable physical mouse input
        PlayerController->bEnableMouseOverEvents = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->CurrentMouseCursor = EMouseCursor::Default;

        // Unbind the controller mouse action first
        if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
        {
            if (ControllerMouseAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
                UE_LOG(LogTemp, Log, TEXT("Unbound controller mouse action"));
            }
            if (ExitCustomizationAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
                UE_LOG(LogTemp, Log, TEXT("Unbound exit action"));
            }
        }

        // Get the enhanced input subsystem
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // Remove the customization mapping context
            if (CustomizationMappingContext)
            {
                Subsystem->RemoveMappingContext(CustomizationMappingContext);
                UE_LOG(LogTemp, Log, TEXT("Removed customization mapping context"));
            }

            // Restore the original mapping context
            if (OriginalMappingContext)
            {
                Subsystem->AddMappingContext(OriginalMappingContext, 0);
                UE_LOG(LogTemp, Log, TEXT("Restored original mapping context"));
            }
        }

        // Re-enable movement and hide mouse cursor
        PlayerController->SetIgnoreMoveInput(false);
        PlayerController->SetIgnoreLookInput(false);
        PlayerController->bShowMouseCursor = true;

        // Set input mode back to game and UI to keep mouse visible
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(nullptr);
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(InputMode);
    }

    // Clean up the customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Customization UI Widget Removed"));
    }

    // Clean up the cooking stage widget
    if (CookingStageWidget)
    {
        CookingStageWidget->RemoveFromParent();
        CookingStageWidget = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Cooking Stage Widget Removed"));
    }

    // Switch back to character camera
    SwitchToCharacterCamera();
    
    // Start camera transition back to original view
    StartCameraTransition(false);
}

void UPUDishCustomizationComponent::StartCameraTransition(bool bToCustomization)
{
    if (!CurrentCharacter)
    {
        return;
    }

    // Store current camera settings
    USpringArmComponent* CameraBoom = CurrentCharacter->GetCameraBoom();
    UCameraComponent* FollowCamera = CurrentCharacter->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    // Store original values if transitioning to customization
    if (bToCustomization)
    {
        OriginalCameraDistance = CameraBoom->TargetArmLength;
        OriginalCameraPitch = CameraBoom->GetRelativeRotation().Pitch;
        OriginalCameraYaw = CameraBoom->GetRelativeRotation().Yaw;
        OriginalOrthoWidth = FollowCamera->OrthoWidth;
        OriginalCameraOffset = CurrentCharacter->GetCameraOffset();
        OriginalCameraPositionIndex = CurrentCharacter->GetCameraPositionIndex();

        // Set target values for customization view
        TargetCameraDistance = CustomizationCameraDistance;
        TargetCameraPitch = CustomizationCameraPitch;
        TargetCameraYaw = OriginalCameraYaw; // Keep the same yaw
        TargetOrthoWidth = CustomizationOrthoWidth;
        TargetCameraOffset = OriginalCameraOffset; // Keep the same offset
        TargetCameraPositionIndex = OriginalCameraPositionIndex; // Keep the same position index
    }
    else
    {
        // Set target values back to original
        TargetCameraDistance = OriginalCameraDistance;
        TargetCameraPitch = OriginalCameraPitch;
        TargetCameraYaw = OriginalCameraYaw;
        TargetOrthoWidth = OriginalOrthoWidth;
        TargetCameraOffset = OriginalCameraOffset;
        TargetCameraPositionIndex = OriginalCameraPositionIndex;
    }

    bIsTransitioningCamera = true;
}

void UPUDishCustomizationComponent::SwitchToCookingCamera()
{
    if (!CurrentCharacter || !CurrentCharacter->GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCookingCamera - No character or world available"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Switching to cooking camera"));

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCookingCamera - No player controller found"));
        return;
    }

    // Find the cooking station camera component
    if (!CookingStationCamera)
    {
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            CookingStationCamera = Cast<UCameraComponent>(OwnerActor->GetComponentByClass(UCameraComponent::StaticClass()));
            
            if (!CookingStationCamera)
            {
                // Try to find by name if the default class search didn't work
                TArray<UCameraComponent*> CameraComponents;
                OwnerActor->GetComponents<UCameraComponent>(CameraComponents);
                
                for (UCameraComponent* CameraComp : CameraComponents)
                {
                    if (CameraComp && CameraComp->GetName() == CookingStationCameraComponentName.ToString())
                    {
                        CookingStationCamera = CameraComp;
                        break;
                    }
                }
            }
            
            if (CookingStationCamera)
            {
                // CRITICAL: Set the CookingCamera as the active camera component on the actor FIRST
                AActor* StationActor = CookingStationCamera->GetOwner();
                if (StationActor)
                {
                    // Disable the plating camera first
                    TArray<UCameraComponent*> AllCameras;
                    StationActor->GetComponents<UCameraComponent>(AllCameras);
                    
                    for (UCameraComponent* Camera : AllCameras)
                    {
                        if (Camera && Camera->GetName() == TEXT("PlatingCamera"))
                        {
                            Camera->SetActive(false);
                            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Disabled PlatingCamera"));
                        }
                    }
                    
                    // Enable the cooking camera
                    CookingStationCamera->SetActive(true);
                    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Activated CookingCamera"));
                }

                // Configure the camera for orthographic projection
                CookingStationCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
                CookingStationCamera->OrthoWidth = CookingOrthoWidth;
                
                // Position the camera properly for smooth transition
                FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + CookingCameraPositionOffset;
                FRotator CameraRotation = FRotator(CookingCameraPitch, CookingCameraYaw, 0.0f);
                
                CookingStationCamera->SetWorldLocation(CameraLocation);
                CookingStationCamera->SetWorldRotation(CameraRotation);
                
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Found and configured cooking camera component, width: %.2f, position: %s"), 
                    CookingOrthoWidth, *CameraLocation.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToCookingCamera - No camera component found on cooking station"));
                return;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToCookingCamera - No owner actor found"));
            return;
        }
    }

    // Switch to the cooking camera
    PlayerController->SetViewTargetWithBlend(CookingStationCamera->GetOwner(), 0.5f);
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Switched to cooking camera"));
}

void UPUDishCustomizationComponent::SetCookingCameraPositionOffset(const FVector& NewOffset)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetCookingCameraPositionOffset - Setting camera offset to: %s"), 
        *NewOffset.ToString());
    
    CookingCameraPositionOffset = NewOffset;
    
    // If the cooking camera component is found, update its position
    if (CookingStationCamera)
    {
        FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + NewOffset;
        FRotator CameraRotation = FRotator(CookingCameraPitch, CookingCameraYaw, 0.0f);
        
        CookingStationCamera->SetWorldLocation(CameraLocation);
        CookingStationCamera->SetWorldRotation(CameraRotation);
        
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetCookingCameraPositionOffset - Updated existing camera position to: %s"), 
            *CameraLocation.ToString());
    }
}

void UPUDishCustomizationComponent::SwitchToCharacterCamera()
{
    if (!CurrentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCharacterCamera - No character available"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Switching to character camera"));

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCharacterCamera - No player controller found"));
        return;
    }

    // Switch back to the character camera
    PlayerController->SetViewTargetWithBlend(CurrentCharacter, 0.5f);
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Switched to character camera"));

    // Reset the cooking camera component reference
    CookingStationCamera = nullptr;
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Reset cooking camera reference"));
}

void UPUDishCustomizationComponent::UpdateCameraTransition(float DeltaTime)
{
    if (!CurrentCharacter)
    {
        return;
    }

    USpringArmComponent* CameraBoom = CurrentCharacter->GetCameraBoom();
    UCameraComponent* FollowCamera = CurrentCharacter->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    // Get current values
    float CurrentDistance = CameraBoom->TargetArmLength;
    float CurrentPitch = CameraBoom->GetRelativeRotation().Pitch;
    float CurrentYaw = CameraBoom->GetRelativeRotation().Yaw;
    float CurrentOrthoWidth = FollowCamera->OrthoWidth;
    float CurrentCameraOffset = CurrentCharacter->GetCameraOffset();
    int32 CurrentCameraPositionIndex = CurrentCharacter->GetCameraPositionIndex();

    // Interpolate values
    float NewDistance = FMath::FInterpTo(CurrentDistance, TargetCameraDistance, DeltaTime, CameraTransitionSpeed);
    float NewPitch = FMath::FInterpTo(CurrentPitch, TargetCameraPitch, DeltaTime, CameraTransitionSpeed);
    float NewYaw = FMath::FInterpTo(CurrentYaw, TargetCameraYaw, DeltaTime, CameraTransitionSpeed);
    float NewOrthoWidth = FMath::FInterpTo(CurrentOrthoWidth, TargetOrthoWidth, DeltaTime, CameraTransitionSpeed);
    float NewCameraOffset = FMath::FInterpTo(CurrentCameraOffset, TargetCameraOffset, DeltaTime, CameraTransitionSpeed);

    // Apply new values
    CameraBoom->TargetArmLength = NewDistance;
    CameraBoom->SetRelativeRotation(FRotator(NewPitch, NewYaw, 0.0f));
    FollowCamera->OrthoWidth = NewOrthoWidth;
    CurrentCharacter->SetCameraOffset(NewCameraOffset);
    CurrentCharacter->SetCameraPositionIndex(TargetCameraPositionIndex);

    // Check if we've reached the target
    if (FMath::IsNearlyEqual(NewDistance, TargetCameraDistance, 1.0f) &&
        FMath::IsNearlyEqual(NewPitch, TargetCameraPitch, 1.0f) &&
        FMath::IsNearlyEqual(NewYaw, TargetCameraYaw, 1.0f) &&
        FMath::IsNearlyEqual(NewOrthoWidth, TargetOrthoWidth, 1.0f) &&
        FMath::IsNearlyEqual(NewCameraOffset, TargetCameraOffset, 1.0f))
    {
        bIsTransitioningCamera = false;

        // If we're exiting customization (returning to original camera settings), re-enable collision detection
        if (TargetOrthoWidth == OriginalOrthoWidth && CurrentCharacter)
        {
            if (CameraBoom)
            {
                CameraBoom->bDoCollisionTest = true;
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::UpdateCameraTransition - Re-enabled spring arm collision detection"));
            }
        }

        // If we're exiting customization, clear the character reference and broadcast the end event
        if (TargetOrthoWidth == OriginalOrthoWidth)
        {
            // Clear all 3D ingredient meshes before ending customization
            ClearAll3DIngredientMeshes();
            
            // Restore the original dish container mesh
            RestoreOriginalDishContainerMesh();
            
            AProjectUmeowmiCharacter* TempCharacter = CurrentCharacter;
            CurrentCharacter = nullptr;
            OnCustomizationEnded.Broadcast();
            UE_LOG(LogTemp, Log, TEXT("Customization fully ended"));
        }
    }
}

void UPUDishCustomizationComponent::HandleExitInput()
{
    UE_LOG(LogTemp, Log, TEXT("Exit input received"));
    EndCustomization();
}

void UPUDishCustomizationComponent::HandleControllerMouse(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse called"));
    
    if (!CurrentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleControllerMouse - No current character"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleControllerMouse - No player controller"));
        return;
    }

    // Get the input value (should be a Vector2D for the right stick)
    FVector2D StickInput = Value.Get<FVector2D>();
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - Raw stick input: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);
    
    // Apply deadzone
    if (FMath::Abs(StickInput.X) < ControllerMouseDeadzone)
    {
        StickInput.X = 0.0f;
    }
    if (FMath::Abs(StickInput.Y) < ControllerMouseDeadzone)
    {
        StickInput.Y = 0.0f;
    }
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - After deadzone: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);

    // Get viewport size
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - Viewport size: %dx%d"), ViewportSizeX, ViewportSizeY);

    // Get current mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - Current mouse position: X=%.2f, Y=%.2f"), MouseX, MouseY);

    // Calculate movement based on stick input and sensitivity
    float DeltaX = StickInput.X * ControllerMouseSensitivity;
    float DeltaY = StickInput.Y * ControllerMouseSensitivity;
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - Calculated delta: X=%.2f, Y=%.2f"), DeltaX, DeltaY);

    // Calculate new position
    float NewMouseX = MouseX + DeltaX;
    float NewMouseY = MouseY + DeltaY;

    // Clamp to viewport bounds
    NewMouseX = FMath::Clamp(NewMouseX, 0.0f, static_cast<float>(ViewportSizeX));
    NewMouseY = FMath::Clamp(NewMouseY, 0.0f, static_cast<float>(ViewportSizeY));
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - New mouse position: X=%.2f, Y=%.2f"), NewMouseX, NewMouseY);

    // Set new mouse position
    int32 NewX = static_cast<int32>(NewMouseX);
    int32 NewY = static_cast<int32>(NewMouseY);
    PlayerController->SetMouseLocation(NewX, NewY);
    
    // Verify the position was set
    float VerifyX, VerifyY;
    PlayerController->GetMousePosition(VerifyX, VerifyY);
    UE_LOG(LogTemp, Log, TEXT("HandleControllerMouse - Verified mouse position: X=%.2f, Y=%.2f"), VerifyX, VerifyY);
}

void UPUDishCustomizationComponent::HandleMouseClick(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Display, TEXT("🔍 Mouse click event received - Value: %s"), *Value.ToString());
    
    if (!CurrentCharacter || !bPlatingMode)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 Mouse click - Character: %s, Plating mode: %s"), 
            CurrentCharacter ? TEXT("Valid") : TEXT("NULL"), bPlatingMode ? TEXT("True") : TEXT("False"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 Mouse click - No player controller"));
        return;
    }

    // Check if widget is blocking mouse events
    if (CustomizationWidget && CustomizationWidget->IsVisible())
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 Customization widget is visible - might be blocking mouse events"));
    }

    // Get mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    DragStartMousePosition = FVector(MouseX, MouseY, 0);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 Mouse click at screen position: (%.0f, %.0f)"), MouseX, MouseY);

    // Convert screen position to world space for custom trace
    FVector WorldLocation;
    FVector WorldDirection;
    if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
    {
        UE_LOG(LogTemp, Warning, TEXT("🔍 Failed to deproject screen position"));
        return;
    }

    // Find the station/bowl actor to ignore in the trace
    AActor* StationActor = GetOwner();
    TArray<AActor*> ActorsToIgnore;
    if (StationActor)
    {
        ActorsToIgnore.Add(StationActor);
        // Also find all child components of the station that might be the bowl
        TArray<AActor*> ChildActors;
        StationActor->GetAllChildActors(ChildActors);
        ActorsToIgnore.Append(ChildActors);
    }

    // Use a multi-line trace to find all hits, then prioritize ingredient meshes
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActors(ActorsToIgnore);
    QueryParams.bTraceComplex = true; // Use complex collision for more accurate hits
    
    FVector TraceStart = WorldLocation;
    FVector TraceEnd = WorldLocation + (WorldDirection * 10000.0f); // Long trace distance
    
    bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 Line trace found %d hits"), HitResults.Num());
    
    // Look for ingredient meshes first (they should be on top/closest)
    APUIngredientMesh* HitIngredient = nullptr;
    FHitResult IngredientHitResult;
    
    for (const FHitResult& Hit : HitResults)
    {
        if (Hit.bBlockingHit)
        {
            UE_LOG(LogTemp, Display, TEXT("🔍 Hit: %s"), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("NULL"));
            
            APUIngredientMesh* TestIngredient = Cast<APUIngredientMesh>(Hit.GetActor());
            if (TestIngredient)
            {
                HitIngredient = TestIngredient;
                IngredientHitResult = Hit;
                UE_LOG(LogTemp, Display, TEXT("🔍 Found ingredient mesh: %s"), *HitIngredient->GetName());
                break; // Found an ingredient, use it
            }
        }
    }
    
    if (HitIngredient)
    {
        // Test mouse interaction for this ingredient
        HitIngredient->TestMouseInteraction();
        
        bIsDragging = true;
        CurrentlyDraggedIngredient = HitIngredient;
        DragStartPosition = HitIngredient->GetActorLocation();
        
        // Calculate offset between mouse and ingredient
        FVector MouseWorldPosition = IngredientHitResult.Location;
        DragOffset = DragStartPosition - MouseWorldPosition;
        
        UE_LOG(LogTemp, Display, TEXT("🎯 Started dragging ingredient: %s with offset: (%.2f,%.2f,%.2f)"), 
            *HitIngredient->GetName(), DragOffset.X, DragOffset.Y, DragOffset.Z);
        
        // Call the ingredient's grab function
        HitIngredient->OnMouseGrab();
    }
    else if (HitResults.Num() > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 Hit %d actors but none were ingredient meshes"), HitResults.Num());
        for (const FHitResult& Hit : HitResults)
        {
            if (Hit.bBlockingHit && Hit.GetActor())
            {
                UE_LOG(LogTemp, Display, TEXT("🔍   - %s (%s)"), *Hit.GetActor()->GetName(), *Hit.GetActor()->GetClass()->GetName());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 No hit under cursor (ignoring station)"));
    }
}

void UPUDishCustomizationComponent::HandleMouseRelease(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Display, TEXT("🔍 [DRAG] Mouse release event received - Value: %s"), *Value.ToString());
    
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        FString IngredientName = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetName() : TEXT("INVALID");
        FVector FinalPosition = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetActorLocation() : FVector::ZeroVector;
        
        UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] Stopping drag for %s at final position (%.2f,%.2f,%.2f)"), 
            *IngredientName, FinalPosition.X, FinalPosition.Y, FinalPosition.Z);
        
        // Call the ingredient's release function
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
            
            FVector PositionAfterRelease = CurrentlyDraggedIngredient->GetActorLocation();
            UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] After OnMouseRelease - %s at position (%.2f,%.2f,%.2f)"), 
                *CurrentlyDraggedIngredient->GetName(), 
                PositionAfterRelease.X, PositionAfterRelease.Y, PositionAfterRelease.Z);
        }
        
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
    }
}

void UPUDishCustomizationComponent::StartDraggingIngredient(APUIngredientMesh* Ingredient)
{
    if (!Ingredient || !bPlatingMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - Invalid ingredient or not in plating mode"));
        return;
    }
    
    if (!IsValid(Ingredient))
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - Ingredient is not valid"));
        return;
    }
    
    if (!CurrentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - No character"));
        return;
    }
    
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - No player controller"));
        return;
    }
    
    // Get current ingredient position before any changes
    FVector IngredientStartPos = Ingredient->GetActorLocation();
    UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] StartDraggingIngredient - %s at position (%.2f,%.2f,%.2f)"), 
        *Ingredient->GetName(), IngredientStartPos.X, IngredientStartPos.Y, IngredientStartPos.Z);
    
    // Get mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    
    // Convert screen position to world space
    FVector WorldLocation;
    FVector WorldDirection;
    if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
    {
        // Find where the mouse ray intersects with the ingredient's Z plane (same height as ingredient)
        // This gives us the mouse position on the same plane as the ingredient
        float IngredientZ = IngredientStartPos.Z;
        
        if (FMath::Abs(WorldDirection.Z) > SMALL_NUMBER)
        {
            // Calculate where the ray intersects the ingredient's Z plane
            float T = (IngredientZ - WorldLocation.Z) / WorldDirection.Z;
            if (T > 0.0f)
            {
                FVector MouseWorldPosition = WorldLocation + (WorldDirection * T);
                // Calculate offset from mouse hit point to ingredient center
                DragOffset = IngredientStartPos - MouseWorldPosition;
                
                UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] Mouse ray hit plane at (%.2f,%.2f,%.2f), offset: (%.2f,%.2f,%.2f)"), 
                    MouseWorldPosition.X, MouseWorldPosition.Y, MouseWorldPosition.Z,
                    DragOffset.X, DragOffset.Y, DragOffset.Z);
            }
            else
            {
                // Ray is going away from the plane, use zero offset
                DragOffset = FVector::ZeroVector;
                UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] Mouse ray going away from ingredient plane, using zero offset"));
            }
        }
        else
        {
            // Ray is parallel to Z plane, use zero offset
            DragOffset = FVector::ZeroVector;
            UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] Mouse ray parallel to ingredient plane, using zero offset"));
        }
    }
    else
    {
        DragOffset = FVector::ZeroVector;
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] Failed to deproject mouse position, using zero offset"));
    }
    
    // Call the ingredient's grab function first to set up its state
    Ingredient->OnMouseGrab();
    
    // Verify ingredient is still valid after grab
    if (!IsValid(Ingredient))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [DRAG] StartDraggingIngredient - Ingredient became invalid after OnMouseGrab!"));
        return;
    }
    
    FVector IngredientAfterGrabPos = Ingredient->GetActorLocation();
    UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] After OnMouseGrab - %s at position (%.2f,%.2f,%.2f)"), 
        *Ingredient->GetName(), IngredientAfterGrabPos.X, IngredientAfterGrabPos.Y, IngredientAfterGrabPos.Z);
    
    bIsDragging = true;
    CurrentlyDraggedIngredient = Ingredient;
    DragStartPosition = IngredientAfterGrabPos;
    DragStartMousePosition = FVector(MouseX, MouseY, 0);
    
    UE_LOG(LogTemp, Display, TEXT("✅ [DRAG] Started dragging ingredient: %s with offset: (%.2f,%.2f,%.2f)"), 
        *Ingredient->GetName(), DragOffset.X, DragOffset.Y, DragOffset.Z);
}

void UPUDishCustomizationComponent::UpdateMouseDrag()
{
    if (!bIsDragging || !CurrentCharacter)
    {
        return;
    }
    
    // Check if the ingredient is still valid (not destroyed)
    if (!IsValid(CurrentlyDraggedIngredient))
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] UpdateMouseDrag - Ingredient is no longer valid, stopping drag"));
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        return;
    }

    // Check if mouse button is still held down
    bool bLeftMouseDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
    if (!bLeftMouseDown)
    {
        // Mouse button released, stop dragging
        UE_LOG(LogTemp, Display, TEXT("🖱️ [DRAG] UpdateMouseDrag - Mouse button released, stopping drag for %s"), 
            *CurrentlyDraggedIngredient->GetName());
        HandleMouseRelease(FInputActionValue());
        return;
    }

    // Get current mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);

    // Convert mouse screen position to world position on the station surface
    FVector WorldLocation;
    FVector WorldDirection;
    
    if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
    {
        // Find the dish customization station to get the surface height
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
        
        AActor* DishStation = nullptr;
        for (AActor* Actor : FoundActors)
        {
            if (Actor && (Actor->GetName().Contains(TEXT("CookingStation")) || Actor->GetName().Contains(TEXT("DishCustomization"))))
            {
                DishStation = Actor;
                break;
            }
        }
        
        if (DishStation)
        {
            // Calculate world position on the station surface
            FVector StationLocation = DishStation->GetActorLocation();
            FVector StationBounds = DishStation->GetComponentsBoundingBox().GetSize();
            
            // Calculate where the mouse ray intersects the station surface
            float StationHeight = StationLocation.Z + (StationBounds.Z * 0.2f);
            
            // Calculate intersection point
            if (FMath::Abs(WorldDirection.Z) > SMALL_NUMBER)
            {
                float T = (StationHeight - WorldLocation.Z) / WorldDirection.Z;
                if (T > 0.0f && T < 10000.0f) // Reasonable range
                {
                    FVector MouseWorldPosition = WorldLocation + (WorldDirection * T);
                    
                    // Apply the stored offset to maintain the grab point
                    FVector NewPosition = MouseWorldPosition + DragOffset;
                    
                    // Validate the new position
                    if (!NewPosition.ContainsNaN() && FVector::Dist(NewPosition, StationLocation) < 5000.0f)
                    {
                        // Update ingredient position
                        FVector OldPosition = CurrentlyDraggedIngredient->GetActorLocation();
                        CurrentlyDraggedIngredient->UpdatePosition(NewPosition);
                        
                        // Verify the ingredient is still visible and at the expected position
                        FVector VerifyPosition = CurrentlyDraggedIngredient->GetActorLocation();
                        bool bIsVisible = CurrentlyDraggedIngredient->IsHidden() == false;
                        
                        UE_LOG(LogTemp, Verbose, TEXT("🎯 [DRAG] %s: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f), Hidden: %s"), 
                            *CurrentlyDraggedIngredient->GetName(),
                            OldPosition.X, OldPosition.Y, OldPosition.Z,
                            VerifyPosition.X, VerifyPosition.Y, VerifyPosition.Z,
                            CurrentlyDraggedIngredient->IsHidden() ? TEXT("Yes") : TEXT("No"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] Invalid position calculated for %s: (%.2f,%.2f,%.2f)"), 
                            *CurrentlyDraggedIngredient->GetName(), NewPosition.X, NewPosition.Y, NewPosition.Z);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ [DRAG] Invalid T value: %.2f for %s"), T, *CurrentlyDraggedIngredient->GetName());
                }
            }
        }
    }
}

void UPUDishCustomizationComponent::UpdateCurrentDishData(const FPUDishBase& NewDishData)
{
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Updating dish data with %d ingredients"), 
        NewDishData.IngredientInstances.Num());
    
    CurrentDishData = NewDishData;
    
    // Log the ingredients for debugging
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Ingredient %d: %s (Qty: %d)"), 
            i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
    }
}

void UPUDishCustomizationComponent::SyncDishDataFromUI(const FPUDishBase& DishDataFromUI)
{
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Syncing dish data from UI with %d ingredients"), 
        DishDataFromUI.IngredientInstances.Num());
    
    // Update the current dish data with the data from the UI
    UpdateCurrentDishData(DishDataFromUI);
    
    // Broadcast the updated dish data to all subscribers
    OnDishDataUpdated.Broadcast(DishDataFromUI);
    
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Dish data synced and broadcasted successfully"));
}

// This function is no longer needed - we'll use a different approach
void UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget(UUserWidget* Widget)
{
    // Removed to avoid crashes - using alternative data passing methods
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget - Function disabled, using alternative approach"));
}

void UPUDishCustomizationComponent::SetWidgetComponentReference(UPUDishCustomizationWidget* Widget)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Setting widget component reference"));
    
    if (Widget)
    {
        // Set the component reference on the widget
        Widget->SetCustomizationComponent(this);
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Widget component reference set successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetWidgetComponentReference - Widget is null"));
    }
}

void UPUDishCustomizationComponent::SetActiveCustomizationWidget(UPUDishCustomizationWidget* ActiveWidget)
{
    if (ActiveWidget)
    {
        // Update the CustomizationWidget reference to track the currently active widget
        // This ensures EndCustomization() can properly clean up the active widget
        CustomizationWidget = ActiveWidget;
        UE_LOG(LogTemp, Display, TEXT("🔄 UPUDishCustomizationComponent::SetActiveCustomizationWidget - Updated active widget to: %s"), 
            *ActiveWidget->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetActiveCustomizationWidget - Active widget is null"));
    }
}

void UPUDishCustomizationComponent::SetInitialDishData(const FPUDishBase& InitialDishData)
{
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Setting initial dish data: %s with %d ingredients"), 
        *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());
    
    // Set the initial dish data
    UpdateCurrentDishData(InitialDishData);
    
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Initial dish data set successfully"));
}

// New event-driven data passing methods
void UPUDishCustomizationComponent::BroadcastDishDataUpdate(const FPUDishBase& NewDishData)
{
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::BroadcastDishDataUpdate - Broadcasting dish data update with %d ingredients"), 
        NewDishData.IngredientInstances.Num());
    
    // Update internal data
    UpdateCurrentDishData(NewDishData);
    
    // Broadcast the update to all subscribers
    OnDishDataUpdated.Broadcast(NewDishData);
}

void UPUDishCustomizationComponent::BroadcastInitialDishData(const FPUDishBase& InitialDishData)
{
    UE_LOG(LogTemp, Display, TEXT("📡 UPUDishCustomizationComponent::BroadcastInitialDishData - Broadcasting initial dish data: %s"), 
        *InitialDishData.DisplayName.ToString());
    
    CurrentDishData = InitialDishData;
    OnInitialDishDataReceived.Broadcast(InitialDishData);
}

void UPUDishCustomizationComponent::StartPlanningMode()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Initialize planning data with current dish
    CurrentPlanningData.TargetDish = CurrentDishData;
    CurrentPlanningData.SelectedIngredients.Empty();
    CurrentPlanningData.bPlanningCompleted = false;
    
    // Populate SelectedIngredients from existing dish ingredients
    // Extract unique ingredients from IngredientInstances (planning mode uses ingredients without quantities)
    TMap<FGameplayTag, FPUIngredientBase> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        // Only add if we haven't seen this ingredient tag before
        if (InstanceTag.IsValid() && !UniqueIngredients.Contains(InstanceTag))
        {
            // Use the ingredient data from the instance (which already has preparations applied)
            UniqueIngredients.Add(InstanceTag, Instance.IngredientData);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Added existing ingredient to SelectedIngredients: %s"), 
                *InstanceTag.ToString());
        }
    }
    
    // Add all unique ingredients to SelectedIngredients
    UniqueIngredients.GenerateValueArray(CurrentPlanningData.SelectedIngredients);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Populated %d existing ingredients into SelectedIngredients"), 
        CurrentPlanningData.SelectedIngredients.Num());
    
    // Notify the widget to start planning mode
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            DishWidget->StartPlanningMode();
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Planning mode started for dish: %s"), 
        *CurrentPlanningData.TargetDish.DisplayName.ToString());
}

void UPUDishCustomizationComponent::TransitionToCookingStage(const FPUDishBase& DishData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Transitioning to cooking stage"));
    
    // Store the dish data
    CurrentDishData = DishData;
    
    // Switch to cooking stage camera
    SwitchToCookingCamera();
    
    // Save reference to dish widget BEFORE removing it (needed for ingredient slots)
    UPUDishCustomizationWidget* SavedDishWidget = nullptr;
    if (CustomizationWidget)
    {
        SavedDishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget);
    }
    
    // Remove the current customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }
    
    // Create and show the cooking stage widget (now uses PUDishCustomizationWidget or subclass)
    if (CurrentCharacter && CurrentCharacter->GetWorld())
    {
        // Create cooking stage widget (should be a subclass of PUDishCustomizationWidget)
        if (UPUDishCustomizationWidget* CookingWidget = CreateWidget<UPUDishCustomizationWidget>(CurrentCharacter->GetWorld(), CookingStageWidgetClass))
        {
            // Store reference to cooking stage widget
            CookingStageWidget = CookingWidget;
            
            // Add to viewport first
            CookingWidget->AddToViewport(250); // Same Z-order as customization widget
            
            // Get the cooking station location (this component's owner location)
            FVector CookingStationLocation = GetOwner()->GetActorLocation();
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking station location: %s"), 
                *CookingStationLocation.ToString());
            
            // Set the dish customization component reference
            CookingWidget->SetCustomizationComponent(this);
            
            // Broadcast initial dish data to the new widget
            BroadcastInitialDishData(DishData);
            
            // Note: The new cooking stage widget should handle initialization in its Blueprint
            // or override OnInitialDishDataReceived to set up ingredient slots, etc.
            // The old InitializeCookingStage method is no longer used - initialization happens
            // through the standard dish data flow
            
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking stage widget created and added to viewport"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToCookingStage - Failed to create cooking stage widget"));
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Planning completed with %d selected ingredients"), 
        DishData.IngredientInstances.Num());
}

void UPUDishCustomizationComponent::SetDataTables(UDataTable* DishTable, UDataTable* IngredientTable, UDataTable* PreparationTable)
{
    UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::SetDataTables - Setting data table references"));
    
    IngredientDataTable = IngredientTable;
    PreparationDataTable = PreparationTable;
}

TArray<FPUIngredientBase> UPUDishCustomizationComponent::GetIngredientData() const
{
    TArray<FPUIngredientBase> IngredientData;
    
    if (IngredientDataTable)
    {
        UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Getting ingredient data from table: %s"), *IngredientDataTable->GetName());
        
        TArray<FName> RowNames = IngredientDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUIngredientBase* Ingredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredientData")))
            {
                IngredientData.Add(*Ingredient);
            }
        }
        
        UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Retrieved %d ingredients"), IngredientData.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishCustomizationComponent::GetIngredientData - No ingredient data table available"));
    }
    
    return IngredientData;
}

TArray<FPUPreparationBase> UPUDishCustomizationComponent::GetPreparationData() const
{
    TArray<FPUPreparationBase> PreparationData;
    
    if (PreparationDataTable)
    {
        UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Getting preparation data from table: %s"), *PreparationDataTable->GetName());
        
        TArray<FName> RowNames = PreparationDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUPreparationBase* Preparation = PreparationDataTable->FindRow<FPUPreparationBase>(RowName, TEXT("GetPreparationData")))
            {
                PreparationData.Add(*Preparation);
            }
        }
        
        UE_LOG(LogTemp, Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Retrieved %d preparations"), PreparationData.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishCustomizationComponent::GetPreparationData - No preparation data table available"));
    }
    
    return PreparationData;
}

// Plating-specific functions
void UPUDishCustomizationComponent::SpawnIngredientIn3D(const FGameplayTag& IngredientTag, const FVector& WorldPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - START - Ingredient %s at position (%.2f,%.2f,%.2f)"), 
        *IngredientTag.ToString(), WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!bPlatingMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Not in plating mode"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - In plating mode, checking %d ingredient instances"), 
        CurrentDishData.IngredientInstances.Num());

    // Find the ingredient in the current dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Checking instance %d: %s vs %s"), 
            i, *Instance.IngredientData.IngredientTag.ToString(), *IngredientTag.ToString());
        
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(Instance.InstanceID))
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Cannot place ingredient %s - quantity limit reached"), 
                    *IngredientTag.ToString());
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed (quantity limit)"));
                return;
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Found matching ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(Instance.InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(Instance.InstanceID);
            
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3D - Set plating for instance %d"), Instance.InstanceID);
            
            // Spawn visual 3D mesh
            SpawnVisualIngredientMesh(Instance, WorldPosition);
            
            // Broadcast the updated dish data
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Success"));
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Ingredient %s not found in current dish"), *IngredientTag.ToString());
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed"));
}

void UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID(int32 InstanceID, const FVector& WorldPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - START - InstanceID %d at position (%.2f,%.2f,%.2f)"), 
        InstanceID, WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!bPlatingMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Not in plating mode"));
        return;
    }

    // Find the specific ingredient instance by InstanceID
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Found instance %d: %s"), 
                InstanceID, *Instance.IngredientData.DisplayName.ToString());
            
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(InstanceID))
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Cannot place ingredient %s (InstanceID: %d) - quantity limit reached"), 
                    *Instance.IngredientData.DisplayName.ToString(), InstanceID);
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed (quantity limit)"));
                return;
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Can place ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(InstanceID);
            
            // Update the ingredient slot's quantity display (NOT buttons - we use slots in plating mode)
            UpdateIngredientSlotQuantity(InstanceID);
            
            UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Set plating for instance %d"), InstanceID);
            
            // Spawn visual 3D mesh
            SpawnVisualIngredientMesh(Instance, WorldPosition);
            
            // Broadcast the updated dish data
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Success"));
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - InstanceID %d not found in current dish"), InstanceID);
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed"));
}

void UPUDishCustomizationComponent::SetPlatingMode(bool bInPlatingMode)
{
    bPlatingMode = bInPlatingMode;
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SetPlatingMode - Plating mode set to: %s"), 
        bPlatingMode ? TEXT("TRUE") : TEXT("FALSE"));
}

bool UPUDishCustomizationComponent::IsPlatingMode() const
{
    return bPlatingMode;
}

void UPUDishCustomizationComponent::TransitionToPlatingStage(const FPUDishBase& DishData)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Transitioning to plating stage"));
    
    // Set plating mode
    SetPlatingMode(true);
    
    // Reset placement tracking for new plating session
    ResetPlatingPlacements();
    
    // Update the current dish data
    CurrentDishData = DishData;
    
    // Switch to plating widget class if available
    if (PlatingWidgetClass)
    {
        // Store the original widget class before switching
        OriginalWidgetClass = CustomizationWidgetClass;
        CustomizationWidgetClass = PlatingWidgetClass;
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Stored original widget class and switched to plating widget class"));
        
        // Remove the current widget if it exists
        if (CustomizationWidget)
        {
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Removing current widget"));
            CustomizationWidget->RemoveFromParent();
            CustomizationWidget = nullptr;
        }
        
        // Create the new plating widget
        UWorld* World = GetWorld();
        if (World)
        {
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating new plating widget"));
            CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
            
            if (CustomizationWidget)
            {
                UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget created successfully"));
                
                // Try to cast to dish customization widget and set up the connection
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is UPUDishCustomizationWidget, connecting to component"));
                    DishWidget->SetCustomizationComponent(this);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is not a UPUDishCustomizationWidget"));
                }
                
                // Add the widget to viewport
                CustomizationWidget->AddToViewport();
                UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget added to viewport"));
                
                // Create plating ingredient buttons
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating plating ingredient slots"));
                    DishWidget->CreatePlatingIngredientSlots();
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - Failed to create plating widget"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - No world available"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - No plating widget class set"));
    }
    
    // Broadcast the dish data to the new widget
    BroadcastInitialDishData(DishData);
    
    // Switch to plating camera
    SwitchToPlatingCamera();
    
    // Swap to plating dish mesh
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Checking PlatingDishMesh..."));
    UE_LOG(LogTemp, Display, TEXT("🍽️ PlatingDishMesh.IsValid(): %s"), PlatingDishMesh.IsValid() ? TEXT("TRUE") : TEXT("FALSE"));
    UE_LOG(LogTemp, Display, TEXT("🍽️ PlatingDishMesh.ToString(): %s"), *PlatingDishMesh.ToString());
    
    if (PlatingDishMesh.IsValid())
    {
        UStaticMesh* LoadedMesh = PlatingDishMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            SwapDishContainerMesh(LoadedMesh);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Swapped to plating dish mesh"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - Failed to load plating dish mesh"));
        }
    }
    else
    {
        // Try to load the mesh directly by path since IsValid() is false but path exists
        UE_LOG(LogTemp, Display, TEXT("🍽️ Trying to load mesh directly by path..."));
        UStaticMesh* DirectLoadedMesh = LoadObject<UStaticMesh>(nullptr, *PlatingDishMesh.ToString());
        if (DirectLoadedMesh)
        {
            SwapDishContainerMesh(DirectLoadedMesh);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Loaded mesh directly and swapped"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - Failed to load mesh directly by path"));
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating stage transition complete"));
}

void UPUDishCustomizationComponent::EndPlatingStage()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Ending plating stage"));

    // Set plating mode to false
    SetPlatingMode(false);

    // Remove the plating widget from viewport
    if (CustomizationWidget)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Removing plating widget from viewport"));
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }

    // Restore the original widget class
    if (OriginalWidgetClass)
    {
        CustomizationWidgetClass = OriginalWidgetClass;
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Restored original widget class"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::EndPlatingStage - No original widget class stored"));
    }

    // Clear the original widget class reference
    OriginalWidgetClass = nullptr;

    // Switch back to cooking camera for the next cooking session
    SwitchToCookingCamera();
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Switched back to cooking camera"));

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Plating stage ended"));
}

void UPUDishCustomizationComponent::SpawnVisualIngredientMesh(const FIngredientInstance& IngredientInstance, const FVector& WorldPosition)
{
    // Get the owner actor (should be the plating station or dish)
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    // Get the world
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Check if the ingredient has a mesh
    UStaticMesh* IngredientMesh = IngredientInstance.IngredientData.IngredientMesh.LoadSynchronous();
    
    if (!IngredientMesh)
    {
        // TEMPORARY: Use a default mesh for testing
        IngredientMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
        
        if (!IngredientMesh)
        {
            return;
        }
    }

    // Use the WorldPosition that was already converted from screen coordinates
    // Add an offset above the surface to ensure ingredients are visible and clickable
    // Since ingredients have physics, they'll settle naturally on the surface
    FVector SpawnPosition = WorldPosition + FVector(0, 0, 20); // Offset above the surface for visibility and clickability

    // Spawn the interactive ingredient mesh actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerActor;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APUIngredientMesh* SpawnedIngredient = World->SpawnActor<APUIngredientMesh>(APUIngredientMesh::StaticClass(), SpawnPosition, FRotator::ZeroRotator, SpawnParams);
    
    if (SpawnedIngredient)
    {
        // Initialize the ingredient with its data
        SpawnedIngredient->InitializeWithIngredient(IngredientInstance.IngredientData);
        
        // Set the mesh manually if needed
        UStaticMeshComponent* MeshComponent = SpawnedIngredient->FindComponentByClass<UStaticMeshComponent>();
        if (MeshComponent && IngredientMesh)
        {
            MeshComponent->SetStaticMesh(IngredientMesh);
        }
        
        // Scale the ingredient using the configurable scale
        SpawnedIngredient->SetActorScale3D(IngredientMeshScale);
        
        // Track the spawned mesh for cleanup
        SpawnedIngredientMeshes.Add(SpawnedIngredient);
        
        UE_LOG(LogTemp, Display, TEXT("✅ Spawned interactive ingredient: %s (Total spawned: %d) - Scaled to (%.2f,%.2f,%.2f)"), 
            *IngredientInstance.IngredientData.IngredientTag.ToString(), SpawnedIngredientMeshes.Num(),
            IngredientMeshScale.X, IngredientMeshScale.Y, IngredientMeshScale.Z);
    }
}

void UPUDishCustomizationComponent::StartCookingStageCameraTransition()
{
    if (!CurrentCharacter)
    {
        return;
    }

    USpringArmComponent* CameraBoom = CurrentCharacter->GetCameraBoom();
    UCameraComponent* FollowCamera = CurrentCharacter->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartCookingStageCameraTransition - Starting cooking stage camera transition"));

    // Disable collision detection on the spring arm to prevent jittering
    CameraBoom->bDoCollisionTest = false;
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::StartCookingStageCameraTransition - Disabled spring arm collision detection"));

    // Set target values for cooking stage view
    TargetCameraDistance = CookingCameraDistance;
    TargetCameraPitch = CookingCameraPitch;
    TargetCameraYaw = CookingCameraYaw;
    TargetOrthoWidth = CookingOrthoWidth;
    TargetCameraOffset = OriginalCameraOffset; // Keep the same offset
    TargetCameraPositionIndex = OriginalCameraPositionIndex; // Keep the same position index

    bIsTransitioningCamera = true;
}


void UPUDishCustomizationComponent::SwitchToPlatingCamera()
{
    if (!CurrentCharacter || !GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToPlatingCamera - No character or world available"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Switching to plating camera"));

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToPlatingCamera - No player controller found"));
        return;
    }

    // Find the plating station camera component
    if (!PlatingStationCamera)
    {
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Owner actor: %s"), *OwnerActor->GetName());
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Looking for camera component named: %s"), *PlatingStationCameraComponentName.ToString());
            
            // Search for camera component with specific name (since there are multiple cameras)
            TArray<UCameraComponent*> CameraComponents;
            OwnerActor->GetComponents<UCameraComponent>(CameraComponents);
            
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Found %d camera components"), CameraComponents.Num());
            
            for (UCameraComponent* CameraComp : CameraComponents)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Camera component: %s"), *CameraComp->GetName());
                if (CameraComp && CameraComp->GetName() == PlatingStationCameraComponentName.ToString())
                {
                    PlatingStationCamera = CameraComp;
                    UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::SwitchToPlatingCamera - Found matching camera component: %s"), *CameraComp->GetName());
                    break;
                }
            }
            
            if (!PlatingStationCamera)
            {
                UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No camera component found with name: %s"), *PlatingStationCameraComponentName.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No owner actor found"));
            return;
        }
    }

    if (PlatingStationCamera)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Configuring plating camera: %s"), *PlatingStationCamera->GetName());
        
        // CRITICAL: Set the PlatingCamera as the active camera component on the actor FIRST
        AActor* StationActor = PlatingStationCamera->GetOwner();
        if (StationActor)
        {
            // Disable the cooking camera first
            TArray<UCameraComponent*> AllCameras;
            StationActor->GetComponents<UCameraComponent>(AllCameras);
            
            for (UCameraComponent* Camera : AllCameras)
            {
                if (Camera && Camera->GetName() == TEXT("CookingCamera"))
                {
                    Camera->SetActive(false);
                    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Disabled CookingCamera"));
                }
            }
            
            // Enable the plating camera
            PlatingStationCamera->SetActive(true);
            UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Activated PlatingCamera"));
        }

        // Configure the plating camera BEFORE switching to it
        PlatingStationCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
        PlatingStationCamera->OrthoWidth = PlatingOrthoWidth;

        // Set camera position and rotation BEFORE switching
        FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + PlatingCameraPositionOffset;
        FRotator CameraRotation = FRotator(PlatingCameraPitch, PlatingCameraYaw, 0.0f);

        PlatingStationCamera->SetWorldLocation(CameraLocation);
        PlatingStationCamera->SetWorldRotation(CameraRotation);

        UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Configured camera - Width: %.2f, Location: %s, Rotation: %s"),
            PlatingOrthoWidth, *CameraLocation.ToString(), *CameraRotation.ToString());

        // Start smooth transition for camera properties
        StartPlatingCameraTransition();
        UE_LOG(LogTemp, Display, TEXT("✅ UPUDishCustomizationComponent::SwitchToPlatingCamera - Started smooth plating camera transition"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No camera component found on plating station"));
    }
}

void UPUDishCustomizationComponent::StartPlatingCameraTransition()
{
    if (!PlatingStationCamera || !CurrentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartPlatingCameraTransition - No camera or character available"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🎬 UPUDishCustomizationComponent::StartPlatingCameraTransition - Starting smooth camera transition"));

    // Get current camera position and properties (from cooking camera)
    FVector CurrentLocation = PlatingStationCamera->GetComponentLocation();
    FRotator CurrentRotation = PlatingStationCamera->GetComponentRotation();
    float CurrentOrthoWidth = PlatingStationCamera->OrthoWidth;
    
    // Try to get the cooking camera properties for smoother transition
    AActor* StationActor = PlatingStationCamera->GetOwner();
    if (StationActor)
    {
        TArray<UCameraComponent*> AllCameras;
        StationActor->GetComponents<UCameraComponent>(AllCameras);
        
        for (UCameraComponent* Camera : AllCameras)
        {
            if (Camera && Camera->GetName() == TEXT("CookingCamera"))
            {
                CurrentLocation = Camera->GetComponentLocation();
                CurrentRotation = Camera->GetComponentRotation();
                CurrentOrthoWidth = Camera->OrthoWidth;
                UE_LOG(LogTemp, Display, TEXT("🎬 UPUDishCustomizationComponent::StartPlatingCameraTransition - Using cooking camera properties"));
                break;
            }
        }
    }

    // Set up transition state
    bPlatingCameraTransitioning = true;
    PlatingCameraTransitionTime = 0.0f;
    PlatingCameraStartLocation = CurrentLocation;
    PlatingCameraStartRotation = CurrentRotation;

    // Target position and properties (already set in SwitchToPlatingCamera)
    PlatingCameraTargetLocation = PlatingStationCamera->GetComponentLocation();
    PlatingCameraTargetRotation = PlatingStationCamera->GetComponentRotation();

    // Store start and target ortho widths for smooth transition
    PlatingCameraStartOrthoWidth = CurrentOrthoWidth;
    PlatingCameraTargetOrthoWidth = PlatingOrthoWidth;

    UE_LOG(LogTemp, Display, TEXT("🎬 UPUDishCustomizationComponent::StartPlatingCameraTransition - Start: %s (Ortho: %.2f), Target: %s (Ortho: %.2f)"), 
        *PlatingCameraStartLocation.ToString(), PlatingCameraStartOrthoWidth,
        *PlatingCameraTargetLocation.ToString(), PlatingCameraTargetOrthoWidth);
}

void UPUDishCustomizationComponent::UpdatePlatingCameraTransition(float DeltaTime)
{
    if (!bPlatingCameraTransitioning || !PlatingStationCamera)
    {
        return;
    }

    PlatingCameraTransitionTime += DeltaTime;
    float Alpha = FMath::Clamp(PlatingCameraTransitionTime / PlatingCameraTransitionDuration, 0.0f, 1.0f);

    // Use smooth interpolation
    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

    // Interpolate position and rotation
    FVector NewLocation = FMath::Lerp(PlatingCameraStartLocation, PlatingCameraTargetLocation, SmoothAlpha);
    FRotator NewRotation = FMath::Lerp(PlatingCameraStartRotation, PlatingCameraTargetRotation, SmoothAlpha);

    // Interpolate ortho width smoothly
    float NewOrthoWidth = FMath::Lerp(PlatingCameraStartOrthoWidth, PlatingCameraTargetOrthoWidth, SmoothAlpha);

    // Update camera position, rotation, and ortho width
    PlatingStationCamera->SetWorldLocation(NewLocation);
    PlatingStationCamera->SetWorldRotation(NewRotation);
    PlatingStationCamera->OrthoWidth = NewOrthoWidth;

    // Check if transition is complete
    if (Alpha >= 1.0f)
    {
        bPlatingCameraTransitioning = false;
        UE_LOG(LogTemp, Display, TEXT("🎬 UPUDishCustomizationComponent::UpdatePlatingCameraTransition - Transition complete"));
    }
}

void UPUDishCustomizationComponent::SetPlatingCameraPositionOffset(const FVector& NewOffset)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetPlatingCameraPositionOffset - Setting camera offset to: %s"),
        *NewOffset.ToString());

    PlatingCameraPositionOffset = NewOffset;

    // If the plating camera component is found, update its position
    if (PlatingStationCamera)
    {
        FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + PlatingCameraPositionOffset;
        FRotator CameraRotation = FRotator(PlatingCameraPitch, PlatingCameraYaw, 0.0f);

        PlatingStationCamera->SetWorldLocation(CameraLocation);
        PlatingStationCamera->SetWorldRotation(CameraRotation);

        UE_LOG(LogTemp, Display, TEXT("🎯 UPUDishCustomizationComponent::SetPlatingCameraPositionOffset - Updated existing camera position to: %s"),
            *CameraLocation.ToString());
    }
}

// Plating placement limits implementation
bool UPUDishCustomizationComponent::CanPlaceIngredient(int32 InstanceID) const
{
    // Find the ingredient instance
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            bool bCanPlace = PlacedQuantity < Instance.Quantity;
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d: Placed %d/%d, Can place: %s"), 
                InstanceID, PlacedQuantity, Instance.Quantity, bCanPlace ? TEXT("Yes") : TEXT("No"));
            
            return bCanPlace;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d not found"), InstanceID);
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantity(int32 InstanceID) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            int32 Remaining = Instance.Quantity - PlacedQuantity;
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d: %d remaining"), 
                InstanceID, Remaining);
            
            return Remaining;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d not found"), InstanceID);
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantity(int32 InstanceID) const
{
    if (const int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        return *PlacedQuantity;
    }
    return 0;
}

void UPUDishCustomizationComponent::PlaceIngredient(int32 InstanceID)
{
    int32 CurrentPlaced = GetPlacedQuantity(InstanceID);
    PlacedIngredientQuantities.Add(InstanceID, CurrentPlaced + 1);
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::PlaceIngredient - Instance %d: Placed %d"), 
        InstanceID, CurrentPlaced + 1);
}

void UPUDishCustomizationComponent::RemoveIngredient(int32 InstanceID)
{
    if (int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        if (*PlacedQuantity > 0)
        {
            (*PlacedQuantity)--;
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RemoveIngredient - Instance %d: Now placed %d"), 
                InstanceID, *PlacedQuantity);
        }
    }
}

void UPUDishCustomizationComponent::ResetPlatingPlacements()
{
    PlacedIngredientQuantities.Empty();
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlatingPlacements - Reset all placement tracking"));
}

// Blueprint-callable plating limits
bool UPUDishCustomizationComponent::CanPlaceIngredientByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return CanPlaceIngredient(Instance.InstanceID);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredientByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetRemainingQuantity(Instance.InstanceID);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetPlacedQuantity(Instance.InstanceID);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetPlacedQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

void UPUDishCustomizationComponent::UpdateIngredientSlotQuantity(int32 InstanceID)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Updating slot quantity for InstanceID: %d"), InstanceID);
    
    // Find the ingredient slot and update its quantity
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        // Find the slot with matching InstanceID
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID == InstanceID)
            {
                // Decrease the slot's remaining quantity
                IngredientSlot->DecreaseQuantity();
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Decreased quantity for slot (InstanceID: %d)"), InstanceID);
                return;
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No slot found for InstanceID: %d"), InstanceID);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No customization widget found"));
    }
}

void UPUDishCustomizationComponent::ResetPlating()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Resetting all plating"));
    
    // Clear all placed ingredient quantities
    PlacedIngredientQuantities.Empty();
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Cleared placed ingredient quantities"));
    
    // Reset all ingredient slot quantities (for plating stage)
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Found %d ingredient slots to reset"), 
            IngredientSlots.Num());
        
        // Reset each slot's quantity from the dish data
        // In plating mode, slots are created with ActiveIngredientArea location (not Plating)
        // So we reset all slots that have ingredients, regardless of location
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID != 0)
            {
                IngredientSlot->ResetQuantityFromDishData();
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Reset slot for InstanceID: %d (Location: %d)"), 
                    IngredientSlot->GetIngredientInstance().InstanceID, (int32)IngredientSlot->GetLocation());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::ResetPlating - No customization widget found"));
    }
    
    // Clear all 3D ingredient meshes
    ClearAll3DIngredientMeshes();
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Plating reset complete"));
}

void UPUDishCustomizationComponent::ClearAll3DIngredientMeshes()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ [CLEANUP] ClearAll3DIngredientMeshes - Clearing %d 3D ingredient meshes"), 
        SpawnedIngredientMeshes.Num());
    
    // Stop any active dragging before clearing
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ [CLEANUP] ClearAll3DIngredientMeshes - Stopping active drag"));
        bIsDragging = false;
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
        }
        CurrentlyDraggedIngredient = nullptr;
    }
    
    // Destroy all tracked ingredient meshes
    int32 ValidCount = 0;
    int32 InvalidCount = 0;
    
    for (APUIngredientMesh* IngredientMesh : SpawnedIngredientMeshes)
    {
        if (IngredientMesh != nullptr && IsValid(IngredientMesh))
        {
            // Only try to get the name if the object is in a safe state
            FString MeshName = TEXT("IngredientMesh");
            if (IngredientMesh->IsValidLowLevel() && !IngredientMesh->IsUnreachable())
            {
                // Safe to get name
                MeshName = IngredientMesh->GetName();
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ [CLEANUP] ClearAll3DIngredientMeshes - Destroying ingredient mesh: %s"), 
                *MeshName);
            
            IngredientMesh->Destroy();
            ValidCount++;
        }
        else
        {
            InvalidCount++;
        }
    }
    
    if (InvalidCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [CLEANUP] ClearAll3DIngredientMeshes - Found %d invalid ingredient mesh pointers"), InvalidCount);
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ [CLEANUP] ClearAll3DIngredientMeshes - Destroyed %d valid meshes"), ValidCount);
    
    // Clear the tracking array
    SpawnedIngredientMeshes.Empty();
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ [CLEANUP] ClearAll3DIngredientMeshes - All 3D ingredient meshes cleared"));
}

void UPUDishCustomizationComponent::SwapDishContainerMesh(UStaticMesh* NewDishMesh)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Swapping dish container mesh"));
    
    if (!NewDishMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is null"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is valid: %s"), 
        *NewDishMesh->GetName());
    
    // Find the cooking station actor by searching all actors
    AActor* DishStation = nullptr;
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        if (ActorItr->GetName().Contains(TEXT("BP_CookingStation")))
        {
            DishStation = *ActorItr;
            break;
        }
    }
    
    if (!DishStation)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - DishStation not found"));
        return;
    }
    
    // Find the DishContainer child component (nested in StationMesh)
    UStaticMeshComponent* DishContainer = nullptr;
    
    // First, find the StationMesh component
    UStaticMeshComponent* StationMesh = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents)
    {
        if (MeshComp->GetName().Contains(TEXT("StationMesh")))
        {
            StationMesh = MeshComp;
            break;
        }
    }
    
    if (!StationMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - StationMesh component not found"));
        return;
    }
    
    // Now find DishContainer as a child of StationMesh using a simpler approach
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Clear any child static mesh components under DishContainer
    // Get all components from the DishContainer's owner actor and clear only child meshes
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only clear meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh != StationMesh && 
            ChildMesh->GetAttachParent() == DishContainer)
        {
            ChildMesh->SetStaticMesh(nullptr);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Cleared child mesh: %s"), 
                *ChildMesh->GetName());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Cleared child meshes only"));
    
    // Add the new dish mesh under DishContainer
    if (NewDishMesh)
    {
        // Set the mesh on the main DishContainer component
        DishContainer->SetStaticMesh(NewDishMesh);
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Set mesh on DishContainer: %s"), 
            *NewDishMesh->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is null!"));
    }
}

void UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restoring original dish container mesh"));
    
    // Find the cooking station actor by searching all actors
    AActor* DishStation = nullptr;
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        if (ActorItr->GetName().Contains(TEXT("BP_CookingStation")))
        {
            DishStation = *ActorItr;
            break;
        }
    }
    
    if (!DishStation)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - BP_CookingStation not found"));
        return;
    }
    
    // Find the DishContainer child component
    UStaticMeshComponent* DishContainer = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Clear any existing children first
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only clear meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh->GetAttachParent() == DishContainer)
        {
            ChildMesh->SetStaticMesh(nullptr);
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Cleared child mesh: %s"), 
                *ChildMesh->GetName());
        }
    }

    // Restore the original mesh
    if (OriginalDishContainerMesh)
    {
        DishContainer->SetStaticMesh(OriginalDishContainerMesh);
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored original mesh: %s"), 
            *OriginalDishContainerMesh->GetName());
    }
    else
    {
        // Clear the mesh if no original was stored
        DishContainer->SetStaticMesh(nullptr);
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Cleared mesh (no original stored)"));
    }

    // Restore the original children meshes
    for (UStaticMesh* ChildMesh : OriginalDishContainerChildren)
    {
        if (ChildMesh)
        {
            // Create a new static mesh component for the child
            UStaticMeshComponent* NewChildComponent = NewObject<UStaticMeshComponent>(DishContainer->GetOwner());
            NewChildComponent->SetStaticMesh(ChildMesh);
            NewChildComponent->SetupAttachment(DishContainer);
            NewChildComponent->RegisterComponent();
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored child mesh: %s"), 
                *ChildMesh->GetName());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored %d child meshes"), 
        OriginalDishContainerChildren.Num());
}

void UPUDishCustomizationComponent::StoreOriginalDishContainerMesh()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Storing original dish container mesh"));
    
    // Check if we have a valid world
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - No world available, cannot store mesh"));
        return;
    }
    
    // Find the cooking station actor by searching all actors
    AActor* DishStation = nullptr;
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        if (ActorItr->GetName().Contains(TEXT("BP_CookingStation")))
        {
            DishStation = *ActorItr;
            break;
        }
    }
    
    if (!DishStation)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - BP_CookingStation not found"));
        return;
    }
    
    // Find the DishContainer child component
    UStaticMeshComponent* DishContainer = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Store the original mesh
    OriginalDishContainerMesh = DishContainer->GetStaticMesh();
    if (OriginalDishContainerMesh)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored original mesh: %s"), 
            *OriginalDishContainerMesh->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - No original mesh found (DishContainer was empty)"));
    }

    // Store the original children meshes
    OriginalDishContainerChildren.Empty();
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only store meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh->GetAttachParent() == DishContainer)
        {
            UStaticMesh* ChildStaticMesh = ChildMesh->GetStaticMesh();
            if (ChildStaticMesh)
            {
                OriginalDishContainerChildren.Add(ChildStaticMesh);
                UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored child mesh: %s"), 
                    *ChildStaticMesh->GetName());
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored %d child meshes"), 
        OriginalDishContainerChildren.Num());
}