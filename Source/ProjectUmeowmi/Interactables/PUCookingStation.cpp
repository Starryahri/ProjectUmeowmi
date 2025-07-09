#include "PUCookingStation.h"
#include "../ProjectUmeowmiCharacter.h"
#include "GameplayTagContainer.h"

APUCookingStation::APUCookingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create and setup the root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create and setup components
    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    StationMesh->SetupAttachment(RootComponent);

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetCollisionProfileName(TEXT("Trigger"));
    InteractionBox->SetBoxExtent(FVector(InteractionRange));

    DishCustomizationComponent = CreateDefaultSubobject<UPUDishCustomizationComponent>(TEXT("DishCustomizationComponent"));
    DishCustomizationComponent->SetupAttachment(RootComponent);

    // Create and setup the interaction widget
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);
    InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
    InteractionWidget->SetVisibility(false);

    // Set default values
    StationName = FText::FromString(TEXT("Cooking Station"));
    StationDescription = FText::FromString(TEXT("A station for customizing dishes"));
    InteractionRange = 200.0f;
}

void APUCookingStation::BeginPlay()
{
    Super::BeginPlay();

    // Bind to dish customization events
    if (DishCustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("CookingStation::BeginPlay - Binding to OnCustomizationEnded event"));
        DishCustomizationComponent->OnCustomizationEnded.AddDynamic(this, &APUCookingStation::OnCustomizationEnded);
        UE_LOG(LogTemp, Display, TEXT("CookingStation::BeginPlay - Event binding completed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CookingStation::BeginPlay - DishCustomizationComponent is null, cannot bind events"));
    }

    // Bind to interaction box events
    if (InteractionBox)
    {
        InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &APUCookingStation::OnInteractionBoxBeginOverlap);
        InteractionBox->OnComponentEndOverlap.AddDynamic(this, &APUCookingStation::OnInteractionBoxEndOverlap);
    }
}

void APUCookingStation::StartInteraction()
{
    UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Attempting to start interaction"));
    if (CanInteract() && DishCustomizationComponent)
    {
        // Get the character that triggered the interaction
        AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Character)
        {
            UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Got character reference"));
            
            // Check if we're already in customization mode
            if (DishCustomizationComponent->IsCustomizing())
            {
                UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Already in customization mode, ignoring interaction"));
                return;
            }

            // Check if player has a current order
            if (Character->HasCurrentOrder())
            {
                const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
                UE_LOG(LogTemp, Display, TEXT("CookingStation::StartInteraction - Player has active order: %s"), 
                    *CurrentOrder.OrderID.ToString());
                
                // Use event-driven data passing
                if (CurrentOrder.BaseDish.DishTag.IsValid())
                {
                    UE_LOG(LogTemp, Display, TEXT("CookingStation::StartInteraction - Broadcasting initial dish data from order: %s"), 
                        *CurrentOrder.BaseDish.DisplayName.ToString());
                    
                    // Debug: Log the base dish details
                    UE_LOG(LogTemp, Display, TEXT("CookingStation::StartInteraction - Base dish details:"));
                    UE_LOG(LogTemp, Display, TEXT("  - Dish Tag: %s"), *CurrentOrder.BaseDish.DishTag.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  - Display Name: %s"), *CurrentOrder.BaseDish.DisplayName.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  - Ingredient Data Table: %s"), CurrentOrder.BaseDish.IngredientDataTable ? TEXT("Valid") : TEXT("NULL"));
                    UE_LOG(LogTemp, Display, TEXT("  - Ingredient Instances: %d"), CurrentOrder.BaseDish.IngredientInstances.Num());
                    
                    for (int32 i = 0; i < CurrentOrder.BaseDish.IngredientInstances.Num(); i++)
                    {
                        const FIngredientInstance& Instance = CurrentOrder.BaseDish.IngredientInstances[i];
                        // Use convenient field if available, fallback to data field
                        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
                        UE_LOG(LogTemp, Display, TEXT("    - Instance %d: %s (Qty: %d)"), 
                            i, *InstanceTag.ToString(), Instance.Quantity);
                    }
                    
                    // Use the event-driven approach
                    DishCustomizationComponent->BroadcastInitialDishData(CurrentOrder.BaseDish);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("CookingStation::StartInteraction - Order has no base dish"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("CookingStation::StartInteraction - Player has no active order"));
            }

            // Hide the interaction widget
            if (InteractionWidget)
            {
                InteractionWidget->SetVisibility(false);
            }

            // Start dish customization with the character reference
            DishCustomizationComponent->StartCustomization(Character);
            BroadcastInteractionStarted();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CookingStation::StartInteraction - Failed to get Character reference"));
            BroadcastInteractionFailed();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CookingStation::StartInteraction - Cannot interact: CanInteract=%d, HasDishComponent=%d"), 
            CanInteract(), DishCustomizationComponent != nullptr);
        BroadcastInteractionFailed();
    }
}

void APUCookingStation::EndInteraction()
{
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->EndCustomization();
    }

    // Show the interaction widget again
    if (InteractionWidget)
    {
        InteractionWidget->SetVisibility(true);
    }

    BroadcastInteractionEnded();
}

FText APUCookingStation::GetInteractionText() const
{
    return StationName;
}

FText APUCookingStation::GetInteractionDescription() const
{
    return StationDescription;
}

void APUCookingStation::OnCustomizationEnded()
{
    UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - FUNCTION CALLED! Dish customization completed"));
    
    // Get the player character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (Character && Character->HasCurrentOrder())
    {
        // Get the completed dish data
        if (DishCustomizationComponent)
        {
            const FPUDishBase& CompletedDish = DishCustomizationComponent->GetCurrentDishData();
            const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
            
            UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - Validating dish against order: %s"), 
                *CurrentOrder.OrderID.ToString());
            
            // Debug: Log the dish data
            UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - Dish data: %d ingredients"), 
                CompletedDish.IngredientInstances.Num());
            
            for (int32 i = 0; i < CompletedDish.IngredientInstances.Num(); i++)
            {
                const FIngredientInstance& Instance = CompletedDish.IngredientInstances[i];
                UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - Ingredient %d: %s (Qty: %d)"), 
                    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
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
            UE_LOG(LogTemp, Warning, TEXT("CookingStation::OnCustomizationEnded - DishCustomizationComponent is null"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - No active order to validate against"));
    }
    
    EndInteraction();
}

bool APUCookingStation::ValidateDishAgainstOrder(const FPUDishBase& Dish, const FPUOrderBase& Order, float& OutSatisfactionScore) const
{
    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Validating dish against order: %s"), *Order.OrderID.ToString());
    
    // Check minimum ingredient count
    int32 IngredientCount = Dish.IngredientInstances.Num();
    bool bMeetsMinIngredients = IngredientCount >= Order.MinIngredientCount;
    
    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish has %d ingredients, minimum required: %d"), 
        IngredientCount, Order.MinIngredientCount);
    
    // Check flavor value if there's a target flavor
    bool bMeetsFlavorRequirement = true;
    if (!Order.TargetFlavorProperty.IsNone())
    {
        UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Looking for flavor property: %s"), *Order.TargetFlavorProperty.ToString());
        UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish has %d ingredients"), Dish.IngredientInstances.Num());
        
        // Debug each ingredient's flavor contribution
        for (int32 i = 0; i < Dish.IngredientInstances.Num(); ++i)
        {
            const FIngredientInstance& Instance = Dish.IngredientInstances[i];
            FPUIngredientBase Ingredient;
            if (Dish.GetIngredientForInstance(i, Ingredient))
            {
                float IngredientFlavor = Ingredient.GetPropertyValue(Order.TargetFlavorProperty);
                UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Ingredient %d (%s) has flavor value: %.2f"), 
                    i, *Instance.IngredientData.IngredientTag.ToString(), IngredientFlavor);
                
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
                    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - %s"), *PrepString);
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - No preparations applied"));
                }
                
                // Debug the ingredient's properties
                UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Ingredient %s has %d natural properties"), 
                    *Instance.IngredientData.IngredientTag.ToString(), Ingredient.NaturalProperties.Num());
                
                for (int32 j = 0; j < Ingredient.NaturalProperties.Num(); j++)
                {
                    const FIngredientProperty& Property = Ingredient.NaturalProperties[j];
                    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Property %d: %s = %.2f"), 
                        j, *Property.GetPropertyName().ToString(), Property.Value);
                }
            }
        }
        
        float FlavorValue = Dish.GetTotalValueForProperty(Order.TargetFlavorProperty);
        bMeetsFlavorRequirement = FlavorValue >= Order.MinFlavorValue;
        
        UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish flavor value: %.2f, minimum required: %.2f"), 
            FlavorValue, Order.MinFlavorValue);
    }
    
    // Calculate satisfaction score
    OutSatisfactionScore = CalculateSatisfactionScore(Dish, Order);
    
    // Order is always completed when submitted - satisfaction score indicates quality
    bool bOrderCompleted = true;
    
    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Order completed: YES, Satisfaction: %.2f"), 
        OutSatisfactionScore);
    
    return bOrderCompleted;
}

float APUCookingStation::CalculateSatisfactionScore(const FPUDishBase& Dish, const FPUOrderBase& Order) const
{
    // Calculate satisfaction score (weighted average of ingredient count and flavor value)
    float IngredientScore = FMath::Min(1.0f, (float)Dish.IngredientInstances.Num() / (float)Order.MinIngredientCount);
    float FlavorScore = 1.0f; // Default to perfect if no flavor requirement
    
    if (!Order.TargetFlavorProperty.IsNone())
    {
        float FlavorValue = Dish.GetTotalValueForProperty(Order.TargetFlavorProperty);
        FlavorScore = FMath::Min(1.0f, FlavorValue / Order.MinFlavorValue);
    }
    
    // Weighted average: 60% ingredients, 40% flavor
    float SatisfactionScore = (IngredientScore * 0.6f) + (FlavorScore * 0.4f);
    
    UE_LOG(LogTemp, Display, TEXT("CookingStation::CalculateSatisfactionScore - Ingredient Score: %.2f, Flavor Score: %.2f, Final Score: %.2f"), 
        IngredientScore, FlavorScore, SatisfactionScore);
    
    return SatisfactionScore;
}

void APUCookingStation::OnInteractionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("CookingStation::OnInteractionBoxBeginOverlap - Player entered range"));
        Character->RegisterInteractable(this);
        BroadcastInteractionRangeEntered();

        // Show the interaction widget
        if (InteractionWidget)
        {
            InteractionWidget->SetVisibility(true);
        }
    }
}

void APUCookingStation::OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("CookingStation::OnInteractionBoxEndOverlap - Player exited range"));
        Character->UnregisterInteractable(this);
        BroadcastInteractionRangeExited();

        // Hide the interaction widget
        if (InteractionWidget)
        {
            InteractionWidget->SetVisibility(false);
        }
    }
} 