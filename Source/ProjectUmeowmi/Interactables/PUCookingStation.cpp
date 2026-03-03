#include "PUCookingStation.h"
#include "../ProjectUmeowmiCharacter.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    constexpr bool bPU_LogCookingStationDishDebug = false;
}

APUCookingStation::APUCookingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // Set TalkingObject defaults first
    ParticipantName = TEXT("CookingStation");
    DisplayName = FText::FromString(TEXT("Cooking Station"));
    ObjectType = ETalkingObjectType::NPC; // Use NPC type to get proper dialogue handling
    InteractionRange = 200.0f;

    // Set cooking station specific defaults
    StationName = FText::FromString(TEXT("Cooking Station"));
    StationDescription = FText::FromString(TEXT("A station for customizing dishes"));

    // Create and setup cooking station specific components
    // Note: RootComponent is created by TalkingObject constructor
    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    if (StationMesh)
    {
        StationMesh->SetupAttachment(GetRootComponent());
    }

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    if (InteractionBox)
    {
        InteractionBox->SetupAttachment(GetRootComponent());
        // DO NOT set collision profile during CDO construction - will be set in PostInitializeComponents
        InteractionBox->SetBoxExtent(FVector(200.0f)); // Use fixed range for now
    }

    DishCustomizationComponent = CreateDefaultSubobject<UPUDishCustomizationComponent>(TEXT("DishCustomizationComponent"));
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->SetupAttachment(GetRootComponent());
    }
}

void APUCookingStation::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Set collision profile after GEngine is initialized (safe from CDO construction)
    // Double-check: ensure we're not in CDO construction AND GEngine is available AND component is not CDO
    if (InteractionBox && !HasAnyFlags(RF_ClassDefaultObject) && !InteractionBox->HasAnyFlags(RF_ClassDefaultObject) && GEngine)
    {
        InteractionBox->SetCollisionProfileName(TEXT("Trigger"));
    }
}

void APUCookingStation::BeginPlay()
{
    Super::BeginPlay();

    // Bind to dish customization events
    if (IsValid(DishCustomizationComponent))
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::BeginPlay - Binding to OnCustomizationEnded event"));
        }
        DishCustomizationComponent->OnCustomizationEnded.AddDynamic(this, &APUCookingStation::OnCustomizationEnded);
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::BeginPlay - Event binding completed"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("CookingStation::BeginPlay - DishCustomizationComponent is null, cannot bind events"));
    }
}

void APUCookingStation::StartCustomizationFromCurrentOrder()
{
    // Get the player character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (!Character || !IsValid(DishCustomizationComponent))
    {
        return;
    }

    // Don't try to start if we're already in customization mode
    if (DishCustomizationComponent->IsCustomizing())
    {
        return;
    }

    // Require an active order
    if (!Character->HasCurrentOrder())
    {
        return;
    }

    const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
    if (bPU_LogCookingStationDishDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartCustomizationFromCurrentOrder - Player has active order: %s"),
        //    *CurrentOrder.OrderID.ToString());
    }

    // Use event-driven data passing
    if (CurrentOrder.BaseDish.DishTag.IsValid())
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartCustomizationFromCurrentOrder - Broadcasting initial dish data from order: %s"),
            //    *CurrentOrder.BaseDish.DisplayName.ToString());
        }

        // Broadcast initial dish data into the customization pipeline
        DishCustomizationComponent->BroadcastInitialDishData(CurrentOrder.BaseDish);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("CookingStation::StartCustomizationFromCurrentOrder - Order has no base dish"));
    }

    // Set the data tables on the dish customization component
    if (IngredientDataTable && PreparationDataTable)
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartCustomizationFromCurrentOrder - Setting data tables on dish customization component"));
        }
        DishCustomizationComponent->SetDataTables(DishDataTable, IngredientDataTable, PreparationDataTable);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("CookingStation::StartCustomizationFromCurrentOrder - Data tables not set on cooking station"));
    }

    // Start dish customization with the character reference
    DishCustomizationComponent->StartCustomization(Character);

    // Start planning mode by default
    DishCustomizationComponent->StartPlanningMode();

    // Hide the interaction widget since we're in dish customization mode
    if (InteractionWidget)
    {
        InteractionWidget->SetVisibility(false);
    }
}

void APUCookingStation::StartInteraction()
{
    //UE_LOG(LogTemp,Log, TEXT("CookingStation::StartInteraction - Attempting to start interaction"));
    
    // Get the character that triggered the interaction
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (!Character)
    {
        //UE_LOG(LogTemp,Error, TEXT("CookingStation::StartInteraction - Failed to get Character reference"));
        return;
    }

    //UE_LOG(LogTemp,Log, TEXT("CookingStation::StartInteraction - Got character reference"));
    
    // Check if we're already in customization mode
    if (IsValid(DishCustomizationComponent) && DishCustomizationComponent->IsCustomizing())
    {
        //UE_LOG(LogTemp,Log, TEXT("CookingStation::StartInteraction - Already in customization mode, ignoring interaction"));
        return;
    }

    // If the player has a current order, either start customization immediately
    // or go through dialogue depending on configuration.
    if (Character->HasCurrentOrder())
    {
        if (bStartCustomizationImmediatelyWhenHasOrder)
        {
            // Preserve existing behavior: jump straight into customization.
            StartCustomizationFromCurrentOrder();
        }
        else
        {
            if (bPU_LogCookingStationDishDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Player has active order, but using dialogue-based flow"));
            }

            // Let parent handle dialogue creation and interaction state.
            Super::StartInteraction();
        }
    }
    else
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Player has no active order, starting dialogue"));
        }

        // Let parent handle dialogue creation and interaction state
        Super::StartInteraction();
    }
}

void APUCookingStation::EndInteraction()
{
    // End dish customization if active
    if (IsValid(DishCustomizationComponent))
    {
        DishCustomizationComponent->EndCustomization();
    }

    // Call parent to handle interaction state and dialogue cleanup
    Super::EndInteraction();
}

void APUCookingStation::EndDialogueOnly()
{
    // Intentionally do NOT touch DishCustomizationComponent here.
    // This lets us close dialogue while keeping the customization
    // UI, camera, and input context active.
    Super::EndInteraction();
}





void APUCookingStation::OnCustomizationEnded()
{
    if (bPU_LogCookingStationDishDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnCustomizationEnded - FUNCTION CALLED! Dish customization completed"));
    }
    
    // Get the player character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (Character && Character->HasCurrentOrder())
    {
            // Get the completed dish data
    if (IsValid(DishCustomizationComponent))
    {
        const FPUDishBase& CompletedDish = DishCustomizationComponent->GetCurrentDishData();
            const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
            
            if (bPU_LogCookingStationDishDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnCustomizationEnded - Validating dish against order: %s"), 
                //    *CurrentOrder.OrderID.ToString());
                
                // Debug: Log the dish data
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnCustomizationEnded - Dish data: %d ingredients"), 
                //    CompletedDish.IngredientInstances.Num());
                
                for (int32 i = 0; i < CompletedDish.IngredientInstances.Num(); i++)
                {
                    const FIngredientInstance& Instance = CompletedDish.IngredientInstances[i];
                    //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnCustomizationEnded - Ingredient %d: %s (Qty: %d)"), 
                    //    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
                }
            }
            
            // Validate the dish against the current order using helper function
            float SatisfactionScore = 0.0f;
            bool bOrderCompleted = ValidateDishAgainstOrder(CompletedDish, CurrentOrder, SatisfactionScore);
            
            // Store the completed dish data in the order
            FPUOrderBase UpdatedOrder = CurrentOrder;
            UpdatedOrder.CompletedDish = CompletedDish;
            UpdatedOrder.FinalSatisfactionScore = SatisfactionScore;
            
            // Update the order with completion data
            Character->SetCurrentOrder(UpdatedOrder);
            
            // Set the order result on the player character
            Character->SetOrderResult(bOrderCompleted, SatisfactionScore);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("CookingStation::OnCustomizationEnded - DishCustomizationComponent is null"));
        }
    }
    else
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnCustomizationEnded - No active order to validate against"));
        }
    }
    
    EndInteraction();
}

bool APUCookingStation::ValidateDishAgainstOrder(const FPUDishBase& Dish, const FPUOrderBase& Order, float& OutSatisfactionScore) const
{
    if (bPU_LogCookingStationDishDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Validating dish against order: %s"), *Order.OrderID.ToString());
    }
    
    // Check minimum ingredient count
    int32 IngredientCount = Dish.IngredientInstances.Num();
    bool bMeetsMinIngredients = IngredientCount >= Order.MinIngredientCount;
    
    if (bPU_LogCookingStationDishDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish has %d ingredients, minimum required: %d"), 
        //    IngredientCount, Order.MinIngredientCount);
    }
    
    // Check all target aspects (flavor and texture)
    bool bMeetsAspectRequirements = true;
    for (const FOrderAspectRequirement& Req : Order.TargetAspects)
    {
        float CurrentValue = (Req.AspectType == EOrderAspectType::Flavor)
            ? Dish.GetTotalFlavorAspect(Req.AspectName)
            : Dish.GetTotalTextureAspect(Req.AspectName);
        if (CurrentValue < Req.MinValue)
        {
            bMeetsAspectRequirements = false;
            break;
        }
    }

    // Calculate satisfaction score
    OutSatisfactionScore = CalculateSatisfactionScore(Dish, Order);
    
    // Order is always completed when submitted - satisfaction score indicates quality
    bool bOrderCompleted = true;
    
    if (bPU_LogCookingStationDishDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Order completed: YES, Satisfaction: %.2f"), 
        //    OutSatisfactionScore);
    }
    
    return bOrderCompleted;
}

float APUCookingStation::CalculateSatisfactionScore(const FPUDishBase& Dish, const FPUOrderBase& Order) const
{
    return Order.GetSatisfactionScore(Dish);
}



void APUCookingStation::StartNoOrderDialogue()
{
    //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartNoOrderDialogue - Starting no order dialogue"));
    
    // Use TalkingObject's built-in dialogue system
    if (AvailableDialogues.Num() > 0)
    {
        StartRandomDialogue();
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("CookingStation::StartNoOrderDialogue - No dialogue available"));
    }
}

// Override dialogue participant methods for cooking station specific logic
bool APUCookingStation::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    //UE_LOG(LogTemp,Display, TEXT("CookingStation::CheckCondition - Condition: %s"), *ConditionName.ToString());
    
    // Call parent implementation first
    bool bParentResult = Super::CheckCondition_Implementation(Context, ConditionName);
    
    // Add cooking station specific conditions
    if (ConditionName == TEXT("HasCompletedOrder"))
    {
        AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Character)
        {
            return Character->GetOrderCompleted();
        }
    }
    else if (ConditionName == TEXT("NoOrder"))
    {
        AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Character)
        {
            return !Character->HasCurrentOrder();
        }
    }
    
    return bParentResult;
}

bool APUCookingStation::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
    //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnDialogueEvent - Event: %s"), *EventName.ToString());
    
    // Call parent implementation first
    bool bParentResult = Super::OnDialogueEvent_Implementation(Context, EventName);

    // Optional dialogue event hook to start dish customization from the current order.
    // This allows dialogue nodes to control WHEN customization begins, instead of it
    // always auto-starting when the player has an order.
    if (EventName == TEXT("StartCustomizationFromCurrentOrder") ||
        EventName == TEXT("StartDishCustomizationFromOrder"))
    {
        StartCustomizationFromCurrentOrder();
        return true;
    }
    
    return bParentResult;
}