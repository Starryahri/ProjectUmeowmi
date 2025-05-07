#include "PUDishCustomizationComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

UPUDishCustomizationComponent::UPUDishCustomizationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UPUDishCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("Dish Customization Component Initialized"));
}

void UPUDishCustomizationComponent::StartCustomization()
{
    // Check if customization is already active
    if (CustomizationWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Dish customization is already active"));
        return;
    }

    // Get player controller from the character
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Character in StartCustomization"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Successfully got Character: %s"), *OwnerCharacter->GetName());

    // Try to get the controller, with a fallback to getting it from the game instance
    APlayerController* PC = nullptr;
    
    // First try: Get from character's controller
    PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (PC)
    {
        UE_LOG(LogTemp, Log, TEXT("Got Player Controller from Character's controller: %s"), *PC->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Failed to get Player Controller from Character's controller, trying Game Instance..."));
    }
    
    // Second try: Get from game instance if first attempt failed
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            UE_LOG(LogTemp, Log, TEXT("Got Player Controller from Game Instance: %s"), *PC->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Failed to get Player Controller from Game Instance"));
        }
    }

    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Player Controller in StartCustomization"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Starting Dish Customization with Player Controller: %s"), *PC->GetName());

    // Disable movement
    PC->SetIgnoreMoveInput(true);
    UE_LOG(LogTemp, Log, TEXT("Player movement disabled"));

    // Show mouse cursor and set input mode to Game and UI
    PC->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(nullptr);
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    UE_LOG(LogTemp, Log, TEXT("Mouse cursor shown and input mode set to Game and UI"));

    // Setup camera
    SetupCustomizationCamera();

    // Create and show UI
    if (CustomizationWidgetClass)
    {
        CustomizationWidget = CreateWidget<UUserWidget>(PC, CustomizationWidgetClass);
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

    // Setup input
    if (PC->InputComponent)
    {
        SetupPlayerInputComponent(PC->InputComponent);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No InputComponent found on Player Controller"));
    }
}

void UPUDishCustomizationComponent::EndCustomization()
{
    // Get player controller from the character
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Character in EndCustomization"));
        return;
    }

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Player Controller in EndCustomization"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Ending Dish Customization"));

    // Re-enable movement
    PC->SetIgnoreMoveInput(false);
    UE_LOG(LogTemp, Log, TEXT("Player movement re-enabled"));

    // Hide mouse cursor and set input mode back to game
    PC->bShowMouseCursor = false;
    PC->SetInputMode(FInputModeGameOnly());
    UE_LOG(LogTemp, Log, TEXT("Mouse cursor hidden and input mode set to game"));

    // Remove UI
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Customization UI Widget Removed"));
    }

    // Reset camera
    // (Implement camera reset)
}

void UPUDishCustomizationComponent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (!PlayerInputComponent) 
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is null in SetupPlayerInputComponent"));
        return;
    }

    // Bind exit input
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput);
            UE_LOG(LogTemp, Log, TEXT("Successfully bound ExitCustomizationAction"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ExitCustomizationAction not set in SetupPlayerInputComponent"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to cast to EnhancedInputComponent in SetupPlayerInputComponent"));
    }
}

void UPUDishCustomizationComponent::HandleExitInput()
{
    UE_LOG(LogTemp, Log, TEXT("Exit input received"));
    EndCustomization();
} 