#include "PUCookingStation.h"
#include "../ProjectUmeowmiCharacter.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

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
        InteractionBox->SetCollisionProfileName(TEXT("Trigger"));
        InteractionBox->SetBoxExtent(FVector(200.0f)); // Use fixed range for now
    }

    DishCustomizationComponent = CreateDefaultSubobject<UPUDishCustomizationComponent>(TEXT("DishCustomizationComponent"));
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->SetupAttachment(GetRootComponent());
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

    // Check if player has a current order
    if (Character->HasCurrentOrder())
    {
        const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Player has active order: %s"), 
            //    *CurrentOrder.OrderID.ToString());
        }
        
        // Use event-driven data passing
        if (CurrentOrder.BaseDish.DishTag.IsValid())
        {
            if (bPU_LogCookingStationDishDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Broadcasting initial dish data from order: %s"), 
                //    *CurrentOrder.BaseDish.DisplayName.ToString());
                
                // Debug: Log the base dish details
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Base dish details:"));
                //UE_LOG(LogTemp,Display, TEXT("  - Dish Tag: %s"), *CurrentOrder.BaseDish.DishTag.ToString());
                //UE_LOG(LogTemp,Display, TEXT("  - Display Name: %s"), *CurrentOrder.BaseDish.DisplayName.ToString());
                //UE_LOG(LogTemp,Display, TEXT("  - Ingredient Data Table: %s"), CurrentOrder.BaseDish.IngredientDataTable.IsValid() ? TEXT("Valid") : TEXT("NULL"));
                //UE_LOG(LogTemp,Display, TEXT("  - Ingredient Instances: %d"), CurrentOrder.BaseDish.IngredientInstances.Num());
                
                for (int32 i = 0; i < CurrentOrder.BaseDish.IngredientInstances.Num(); i++)
                {
                    const FIngredientInstance& Instance = CurrentOrder.BaseDish.IngredientInstances[i];
                    // Use convenient field if available, fallback to data field
                    FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
                    //UE_LOG(LogTemp,Display, TEXT("    - Instance %d: %s (Qty: %d)"), 
                    //    i, *InstanceTag.ToString(), Instance.Quantity);
                }
            }
            
            // Use the event-driven approach
            DishCustomizationComponent->BroadcastInitialDishData(CurrentOrder.BaseDish);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("CookingStation::StartInteraction - Order has no base dish"));
        }

        // Set the data tables on the dish customization component
        if (IngredientDataTable && PreparationDataTable)
        {
            if (bPU_LogCookingStationDishDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("CookingStation::StartInteraction - Setting data tables on dish customization component"));
            }
            DishCustomizationComponent->SetDataTables(DishDataTable, IngredientDataTable, PreparationDataTable);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("CookingStation::StartInteraction - Data tables not set on cooking station"));
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
        
        // Don't call Super::StartInteraction() to avoid triggering dialogue
        // The dish customization component will handle its own state
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
    
    // Check flavor value if there's a target flavor
    bool bMeetsFlavorRequirement = true;
    if (!Order.TargetFlavorProperty.IsNone())
    {
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Looking for flavor property: %s"), *Order.TargetFlavorProperty.ToString());
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish has %d ingredients"), Dish.IngredientInstances.Num());
        }
        
        // Debug each ingredient's flavor contribution
        for (int32 i = 0; i < Dish.IngredientInstances.Num(); ++i)
        {
            const FIngredientInstance& Instance = Dish.IngredientInstances[i];
            FPUIngredientBase Ingredient;
            if (Dish.GetIngredientForInstance(i, Ingredient))
            {
                float IngredientFlavor = Ingredient.GetFlavorAspect(Order.TargetFlavorProperty);
                if (bPU_LogCookingStationDishDebug)
                {
                    //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Ingredient %d (%s) has flavor value: %.2f"), 
                    //    i, *Instance.IngredientData.IngredientTag.ToString(), IngredientFlavor);
                }
                
                // Log preparations for this instance
                if (Instance.IngredientData.ActivePreparations.Num() > 0)
                {
                    TArray<FGameplayTag> PreparationTags;
                    Instance.IngredientData.ActivePreparations.GetGameplayTagArray(PreparationTags);
                    FString PrepString = TEXT("Preparations: ");
                    for (const FGameplayTag& PrepTag : PreparationTags)
                    {
                        PrepString += PrepTag.ToString() + TEXT(", ");
                    }
                    if (bPU_LogCookingStationDishDebug)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - %s"), *PrepString);
                    }
                }
                else
                {
                    if (bPU_LogCookingStationDishDebug)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - No preparations applied"));
                    }
                }
                
                // Debug the ingredient's aspects
                if (bPU_LogCookingStationDishDebug)
                {
                    //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Ingredient %s flavor aspects: Umami=%.2f, Sweet=%.2f, Salt=%.2f, Sour=%.2f, Bitter=%.2f, Spicy=%.2f"), 
                    //    *Instance.IngredientData.IngredientTag.ToString(), 
                    //    Ingredient.FlavorAspects.Umami, Ingredient.FlavorAspects.Sweet, Ingredient.FlavorAspects.Salt,
                    //    Ingredient.FlavorAspects.Sour, Ingredient.FlavorAspects.Bitter, Ingredient.FlavorAspects.Spicy);
                }
            }
        }
        
        float FlavorValue = Dish.GetTotalFlavorAspect(Order.TargetFlavorProperty);
        bMeetsFlavorRequirement = FlavorValue >= Order.MinFlavorValue;
        
        if (bPU_LogCookingStationDishDebug)
        {
            //UE_LOG(LogTemp,Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish flavor value: %.2f, minimum required: %.2f"), 
            //    FlavorValue, Order.MinFlavorValue);
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
    // Calculate satisfaction score (weighted average of ingredient count and flavor value)
    float IngredientScore = FMath::Min(1.0f, (float)Dish.IngredientInstances.Num() / (float)Order.MinIngredientCount);
    float FlavorScore = 1.0f; // Default to perfect if no flavor requirement
    
    if (!Order.TargetFlavorProperty.IsNone())
    {
        float FlavorValue = Dish.GetTotalFlavorAspect(Order.TargetFlavorProperty);
        FlavorScore = FMath::Min(1.0f, FlavorValue / Order.MinFlavorValue);
    }
    
    // Weighted average: 60% ingredients, 40% flavor
    float SatisfactionScore = (IngredientScore * 0.6f) + (FlavorScore * 0.4f);
    
    //UE_LOG(LogTemp,Display, TEXT("CookingStation::CalculateSatisfactionScore - Ingredient Score: %.2f, Flavor Score: %.2f, Final Score: %.2f"), 
    //    IngredientScore, FlavorScore, SatisfactionScore);
    
    return SatisfactionScore;
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
    
    // Handle cooking station specific dialogue events
    if (EventName == TEXT("EndDialogue"))
    {
        //UE_LOG(LogTemp,Display, TEXT("CookingStation::OnDialogueEvent - Ending dialogue"));
        EndInteraction();
        return true;
    }
    
    return bParentResult;
}