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
    // Get the current game instance
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GEngine->GameViewport->GetWorld());
    if (!GameInstance) return;

    // Get the current world
    UWorld* World = GEngine->GameViewport->GetWorld();
    if (!World) return;

    // Create and set the new game mode
    APUDishCustomizationGameMode* NewGameMode = World->SpawnActor<APUDishCustomizationGameMode>();
    if (NewGameMode)
    {
        World->GetAuthGameMode()->AuthorityGameMode = NewGameMode;
    }
}

void APUDishCustomizationGameMode::ExitDishCustomizationMode()
{
    // Here you would implement the logic to return to the previous game mode
    // This might involve saving the current customization state
    // and transitioning back to the main game mode
} 