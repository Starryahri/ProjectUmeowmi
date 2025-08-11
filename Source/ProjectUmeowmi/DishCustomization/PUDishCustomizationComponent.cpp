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
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "../UI/PUDishCustomizationWidget.h"
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

    // Raycast to find ingredient under mouse
    FHitResult HitResult;
    if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 Hit something: %s"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
        
        APUIngredientMesh* HitIngredient = Cast<APUIngredientMesh>(HitResult.GetActor());
        if (HitIngredient)
        {
            // Test mouse interaction for this ingredient
            HitIngredient->TestMouseInteraction();
            
            bIsDragging = true;
            CurrentlyDraggedIngredient = HitIngredient;
            DragStartPosition = HitIngredient->GetActorLocation();
            
            // Calculate offset between mouse and ingredient
            FVector MouseWorldPosition = HitResult.Location;
            DragOffset = DragStartPosition - MouseWorldPosition;
            
            UE_LOG(LogTemp, Display, TEXT("🎯 Started dragging ingredient: %s with offset: (%.2f,%.2f,%.2f)"), 
                *HitIngredient->GetName(), DragOffset.X, DragOffset.Y, DragOffset.Z);
            
            // Call the ingredient's grab function
            HitIngredient->OnMouseGrab();
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("🔍 Hit actor is not an ingredient mesh"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 No hit under cursor"));
    }
}

void UPUDishCustomizationComponent::HandleMouseRelease(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Display, TEXT("🔍 Mouse release event received - Value: %s"), *Value.ToString());
    
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        // Call the ingredient's release function
        CurrentlyDraggedIngredient->OnMouseRelease();
        
        UE_LOG(LogTemp, Display, TEXT("🎯 Stopped dragging ingredient: %s"), *CurrentlyDraggedIngredient->GetName());
        
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
    }
}

void UPUDishCustomizationComponent::UpdateMouseDrag()
{
    if (!bIsDragging || !CurrentlyDraggedIngredient || !CurrentCharacter)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
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
            float T = (StationHeight - WorldLocation.Z) / WorldDirection.Z;
            FVector MouseWorldPosition = WorldLocation + (WorldDirection * T);
            
            // Apply the stored offset to maintain the grab point
            FVector NewPosition = MouseWorldPosition + DragOffset;
            
            // Update ingredient position
            CurrentlyDraggedIngredient->UpdatePosition(NewPosition);
            
            UE_LOG(LogTemp, Display, TEXT("🎯 Dragging ingredient to: (%.2f,%.2f,%.2f)"), 
                NewPosition.X, NewPosition.Y, NewPosition.Z);
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
    
    // Remove the current customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }
    
    // Create and show the cooking stage widget
    if (CurrentCharacter && CurrentCharacter->GetWorld())
    {
        // Create cooking stage widget
        if (UPUCookingStageWidget* CookingWidget = CreateWidget<UPUCookingStageWidget>(CurrentCharacter->GetWorld(), CookingStageWidgetClass))
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
            CookingWidget->SetDishCustomizationComponent(this);
            
            // Initialize the cooking stage with dish data and station location
            CookingWidget->InitializeCookingStage(DishData, CookingStationLocation);
            
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

    // TEMPORARY: Force plating mode to true for testing
    bPlatingMode = true;
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - FORCED plating mode to TRUE for testing"));

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
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Found matching ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(Instance.InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
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

void UPUDishCustomizationComponent::SetPlatingMode(bool bInPlatingMode)
{
    bPlatingMode = bInPlatingMode;
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUDishCustomizationComponent::SetPlatingMode - Plating mode set to: %s"), 
        bPlatingMode ? TEXT("TRUE") : TEXT("FALSE"));
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
    FVector SpawnPosition = WorldPosition + FVector(0, 0, 50); // 50 units above the converted position

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
        
        UE_LOG(LogTemp, Display, TEXT("✅ Spawned interactive ingredient: %s"), *IngredientInstance.IngredientData.IngredientTag.ToString());
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