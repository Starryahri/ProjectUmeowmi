#include "PUPlatingStation.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBase.h"
#include "../DishCustomization/PUOrderBase.h"
#include "../UI/PUPlatingWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

APUPlatingStation::APUPlatingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create station mesh
    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    StationMesh->SetupAttachment(RootComponent);

    // Create interaction box
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));

    // Create plating component
    PlatingComponent = CreateDefaultSubobject<UPUDishCustomizationComponent>(TEXT("PlatingComponent"));
    PlatingComponent->SetupAttachment(RootComponent);

    // Create 3D scene components
    DishMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DishMesh"));
    DishMesh->SetupAttachment(RootComponent);
    DishMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

    IngredientSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("IngredientSpawnPoint"));
    IngredientSpawnPoint->SetupAttachment(RootComponent);
    IngredientSpawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

    // Set default values
    StationName = FText::FromString(TEXT("Plating Station"));
    StationDescription = FText::FromString(TEXT("Arrange your prepared ingredients on the dish"));
}

void APUPlatingStation::BeginPlay()
{
    Super::BeginPlay();

    // Set up the plating component with data tables
    if (PlatingComponent)
    {
        PlatingComponent->SetDataTables(DishDataTable, IngredientDataTable, PreparationDataTable);
        
        // Subscribe to plating ended event
        PlatingComponent->OnCustomizationEnded.AddDynamic(this, &APUPlatingStation::OnPlatingEnded);
    }
}

void APUPlatingStation::StartInteraction()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ APUPlatingStation::StartInteraction - Starting plating interaction"));

    // Get the character that's interacting
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ APUPlatingStation::StartInteraction - Failed to get player character"));
        return;
    }

    // Check if character has an active order
    if (!Character->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ APUPlatingStation::StartInteraction - No active order, starting dialogue"));
        StartNoOrderDialogue();
        return;
    }

    // Get the current order
    const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
    
    // Check if the order has a completed dish from cooking
    if (!CurrentOrder.CompletedDish.DishTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ APUPlatingStation::StartInteraction - No completed dish from cooking"));
        StartNoOrderDialogue();
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("✅ APUPlatingStation::StartInteraction - Starting plating with dish: %s"), 
        *CurrentOrder.CompletedDish.DisplayName.ToString());

    // Set the initial dish data for plating
    if (PlatingComponent)
    {
        PlatingComponent->SetInitialDishData(CurrentOrder.CompletedDish);
        
        // Set the plating widget class
        if (PlatingWidgetClass)
        {
            PlatingComponent->CustomizationWidgetClass = PlatingWidgetClass;
        }
    }

    // Start the plating customization
    PlatingComponent->StartCustomization(Character);
}

void APUPlatingStation::EndInteraction()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ APUPlatingStation::EndInteraction - Ending plating interaction"));

    if (PlatingComponent)
    {
        PlatingComponent->EndCustomization();
    }
}

void APUPlatingStation::OnPlatingEnded()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ APUPlatingStation::OnPlatingEnded - Plating session ended"));

    // Get the character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ APUPlatingStation::OnPlatingEnded - Failed to get player character"));
        return;
    }

    // Get the completed plating data
    if (PlatingComponent)
    {
        const FPUDishBase& PlatedDish = PlatingComponent->GetCurrentDishData();
        
        // Update the order with the plated dish data
        FPUOrderBase UpdatedOrder = Character->GetCurrentOrder();
        UpdatedOrder.CompletedDish = PlatedDish;
        
        // Set the updated order back to the character
        Character->SetCurrentOrder(UpdatedOrder);
        
        UE_LOG(LogTemp, Display, TEXT("✅ APUPlatingStation::OnPlatingEnded - Updated order with plated dish data"));
    }

    EndInteraction();
}

void APUPlatingStation::StartNoOrderDialogue()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ APUPlatingStation::StartNoOrderDialogue - Starting no order dialogue"));
    
    // Start the dialogue system with no order message
    // This will be handled by the TalkingObject base class
    Super::StartInteraction();
}

bool APUPlatingStation::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    // Add plating-specific conditions here if needed
    return Super::CheckCondition_Implementation(Context, ConditionName);
}

bool APUPlatingStation::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
    // Add plating-specific events here if needed
    return Super::OnDialogueEvent_Implementation(Context, EventName);
} 