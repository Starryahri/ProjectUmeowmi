#include "PUDishCustomizationGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

APUDishCustomizationGameMode::APUDishCustomizationGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void APUDishCustomizationGameMode::BeginPlay()
{
    Super::BeginPlay();
    SetupCustomizationCamera();
}

void APUDishCustomizationGameMode::TransitionToDishCustomizationMode()
{
    // Get the current world
    UWorld* World = GEngine->GameViewport->GetWorld();
    if (!World) return;

    // Get the current game instance
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
    if (!GameInstance) return;

    // Get the current game mode class
    TSubclassOf<AGameModeBase> CurrentGameModeClass = World->GetAuthGameMode()->GetClass();

    // Set the new game mode
    World->ServerTravel(TEXT("?game=/Game/ProjectUmeowmi/Core/Blueprints/BP_DishCustomizationGameMode.BP_DishCustomizationGameMode_C"), true);
}

void APUDishCustomizationGameMode::ExitDishCustomizationMode()
{
    // Here you would implement the logic to return to the previous game mode
    // This might involve saving the current customization state
    // and transitioning back to the main game mode
} 