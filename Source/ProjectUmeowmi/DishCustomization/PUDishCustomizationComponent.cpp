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
}

void UPUDishCustomizationComponent::StartCustomization(AProjectUmeowmiCharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Character in StartCustomization"));
        return;
    }

    CurrentCharacter = Character;

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Player Controller in StartCustomization"));
        return;
    }

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

    // Handle mapping contexts
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
        UE_LOG(LogTemp, Log, TEXT("Found Enhanced Input Subsystem"));
        
        // Store the original mapping context
        OriginalMappingContext = Character->GetDefaultMappingContext();
        UE_LOG(LogTemp, Log, TEXT("Original mapping context: %s"), OriginalMappingContext ? *OriginalMappingContext->GetName() : TEXT("None"));
        
        // Remove the original mapping context
        if (OriginalMappingContext)
        {
            Subsystem->RemoveMappingContext(OriginalMappingContext);
            UE_LOG(LogTemp, Log, TEXT("Removed original mapping context"));
        }

        // Add the customization mapping context
        if (CustomizationMappingContext)
        {
            UE_LOG(LogTemp, Log, TEXT("Adding customization mapping context: %s"), *CustomizationMappingContext->GetName());
            Subsystem->AddMappingContext(CustomizationMappingContext, 0);
            UE_LOG(LogTemp, Log, TEXT("Added customization mapping context"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No CustomizationMappingContext set"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Enhanced Input Subsystem"));
    }

    // Setup input handling for exit action and controller mouse
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        UE_LOG(LogTemp, Log, TEXT("Found Enhanced Input Component"));
        
        // First unbind any existing bindings to ensure clean state
        if (ControllerMouseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
            UE_LOG(LogTemp, Log, TEXT("Removed existing controller mouse binding"));
        }
        
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
            UE_LOG(LogTemp, Log, TEXT("Removed existing exit action binding"));
        }

        // Now set up new bindings
        if (ExitCustomizationAction)
        {
            UE_LOG(LogTemp, Log, TEXT("Binding exit action: %s"), *ExitCustomizationAction->GetName());
            ExitActionBindingHandle = EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput).GetHandle();
            UE_LOG(LogTemp, Log, TEXT("Exit action bound with handle: %d"), ExitActionBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No ExitCustomizationAction set"));
        }

        if (ControllerMouseAction)
        {
            UE_LOG(LogTemp, Log, TEXT("Binding controller mouse action: %s"), *ControllerMouseAction->GetName());
            // Bind to both Ongoing and Triggered events to ensure we catch all input
            ControllerMouseBindingHandle = EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Ongoing, this, &UPUDishCustomizationComponent::HandleControllerMouse).GetHandle();
            EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleControllerMouse);
            UE_LOG(LogTemp, Log, TEXT("Controller mouse action bound with handle: %d"), ControllerMouseBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No ControllerMouseAction set"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Enhanced Input Component"));
    }

    // Create and show the customization widget
    if (CustomizationWidgetClass)
    {
        CustomizationWidget = CreateWidget<UUserWidget>(GetWorld(), CustomizationWidgetClass);
        if (CustomizationWidget)
        {
            CustomizationWidget->AddToViewport();
            UE_LOG(LogTemp, Log, TEXT("Customization UI Widget Added to Viewport"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create Customization UI Widget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No CustomizationWidgetClass set"));
    }

    // Start camera transition to customization view
    StartCameraTransition(true);
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
        PlayerController->bShowMouseCursor = false;

        // Set input mode back to game only
        PlayerController->SetInputMode(FInputModeGameOnly());
    }

    // Clean up the widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Customization UI Widget Removed"));
    }

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

        UE_LOG(LogTemp, Log, TEXT("Storing original ortho width: %f"), OriginalOrthoWidth);
        UE_LOG(LogTemp, Log, TEXT("Storing original camera position index: %d"), OriginalCameraPositionIndex);

        // Set target values for customization view
        TargetCameraDistance = CustomizationCameraDistance;
        TargetCameraPitch = CustomizationCameraPitch;
        TargetCameraYaw = OriginalCameraYaw; // Keep the same yaw
        TargetOrthoWidth = CustomizationOrthoWidth;
        TargetCameraOffset = OriginalCameraOffset; // Keep the same offset
        TargetCameraPositionIndex = OriginalCameraPositionIndex; // Keep the same position index

        UE_LOG(LogTemp, Log, TEXT("Setting target ortho width to: %f"), TargetOrthoWidth);
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

        UE_LOG(LogTemp, Log, TEXT("Restoring to original ortho width: %f"), TargetOrthoWidth);
        UE_LOG(LogTemp, Log, TEXT("Restoring to original camera position index: %d"), TargetCameraPositionIndex);
    }

    bIsTransitioningCamera = true;
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

    UE_LOG(LogTemp, Log, TEXT("Current ortho width: %f, Target: %f"), NewOrthoWidth, TargetOrthoWidth);
    UE_LOG(LogTemp, Log, TEXT("Current camera position index: %d, Target: %d"), CurrentCameraPositionIndex, TargetCameraPositionIndex);

    // Check if we've reached the target
    if (FMath::IsNearlyEqual(NewDistance, TargetCameraDistance, 1.0f) &&
        FMath::IsNearlyEqual(NewPitch, TargetCameraPitch, 1.0f) &&
        FMath::IsNearlyEqual(NewYaw, TargetCameraYaw, 1.0f) &&
        FMath::IsNearlyEqual(NewOrthoWidth, TargetOrthoWidth, 1.0f) &&
        FMath::IsNearlyEqual(NewCameraOffset, TargetCameraOffset, 1.0f))
    {
        bIsTransitioningCamera = false;
        UE_LOG(LogTemp, Log, TEXT("Camera transition complete. Final ortho width: %f"), NewOrthoWidth);

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