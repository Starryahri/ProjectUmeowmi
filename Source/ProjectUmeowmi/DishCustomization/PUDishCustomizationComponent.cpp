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

    // Disable movement and show mouse cursor
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
    PlayerController->bShowMouseCursor = true;

    // Disable camera rotation input by ignoring the input
    if (UInputAction* RotateCameraAction = Character->GetRotateCameraAction())
    {
        PlayerController->SetIgnoreLookInput(true);
    }

    // Set input mode to Game and UI
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(nullptr);
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);

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

    // Setup input handling
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput);
            UE_LOG(LogTemp, Log, TEXT("Exit action bound"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No ExitCustomizationAction set"));
        }
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
        // Re-enable movement and hide mouse cursor
        PlayerController->SetIgnoreMoveInput(false);
        PlayerController->SetIgnoreLookInput(false);
        PlayerController->bShowMouseCursor = false;

        // Re-enable camera rotation input
        if (UInputAction* RotateCameraAction = CurrentCharacter->GetRotateCameraAction())
        {
            PlayerController->SetIgnoreLookInput(false);
        }

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

void UPUDishCustomizationComponent::HandleExitInput()
{
    UE_LOG(LogTemp, Log, TEXT("Exit input received"));
    EndCustomization();
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