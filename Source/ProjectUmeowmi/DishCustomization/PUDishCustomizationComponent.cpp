#include "PUDishCustomizationComponent.h"
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
    if (!PlayerInputComponent) return;

    // Bind exit input
    PlayerInputComponent->BindAction("ExitCustomization", IE_Pressed, this, &UPUDishCustomizationComponent::HandleExitInput);
    UE_LOG(LogTemp, Log, TEXT("Exit input bound"));
}

void UPUDishCustomizationComponent::HandleExitInput()
{
    UE_LOG(LogTemp, Log, TEXT("Exit input received"));
    EndCustomization();
} 