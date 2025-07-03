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
    bIsInteractable = true;
}

void APUCookingStation::BeginPlay()
{
    Super::BeginPlay();

    // Bind to dish customization events
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->OnCustomizationEnded.AddDynamic(this, &APUCookingStation::OnCustomizationEnded);
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
                UE_LOG(LogTemp, Display, TEXT("CookingStation::StartInteraction - Player has active order: %s"), 
                    *Character->GetCurrentOrder().OrderID.ToString());
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
    UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - Dish customization completed"));
    
    // Get the player character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (Character && Character->HasCurrentOrder())
    {
        // Get the completed dish data
        if (DishCustomizationComponent)
        {
            const FPUDishBase& CompletedDish = DishCustomizationComponent->CurrentDishData;
            const FPUOrderBase& CurrentOrder = Character->GetCurrentOrder();
            
            UE_LOG(LogTemp, Display, TEXT("CookingStation::OnCustomizationEnded - Validating dish against order: %s"), 
                *CurrentOrder.OrderID.ToString());
            
            // Validate the dish against the current order using helper function
            float SatisfactionScore = 0.0f;
            bool bOrderCompleted = ValidateDishAgainstOrder(CompletedDish, CurrentOrder, SatisfactionScore);
            
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
        // Convert FName to FGameplayTag for the dish validation
        FGameplayTag FlavorTag = FGameplayTag::RequestGameplayTag(FName(*Order.TargetFlavorProperty.ToString()));
        float FlavorValue = Dish.GetTotalValueForTag(FlavorTag);
        bMeetsFlavorRequirement = FlavorValue >= Order.MinFlavorValue;
        
        UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Dish flavor value: %.2f, minimum required: %.2f"), 
            FlavorValue, Order.MinFlavorValue);
    }
    
    // Determine if order is completed
    bool bOrderCompleted = bMeetsMinIngredients && bMeetsFlavorRequirement;
    
    // Calculate satisfaction score
    OutSatisfactionScore = CalculateSatisfactionScore(Dish, Order);
    
    UE_LOG(LogTemp, Display, TEXT("CookingStation::ValidateDishAgainstOrder - Order completed: %s, Satisfaction: %.2f"), 
        bOrderCompleted ? TEXT("YES") : TEXT("NO"), OutSatisfactionScore);
    
    return bOrderCompleted;
}

float APUCookingStation::CalculateSatisfactionScore(const FPUDishBase& Dish, const FPUOrderBase& Order) const
{
    // Calculate satisfaction score (weighted average of ingredient count and flavor value)
    float IngredientScore = FMath::Min(1.0f, (float)Dish.IngredientInstances.Num() / (float)Order.MinIngredientCount);
    float FlavorScore = 1.0f; // Default to perfect if no flavor requirement
    
    if (!Order.TargetFlavorProperty.IsNone())
    {
        // Convert FName to FGameplayTag for the dish validation
        FGameplayTag FlavorTag = FGameplayTag::RequestGameplayTag(FName(*Order.TargetFlavorProperty.ToString()));
        float FlavorValue = Dish.GetTotalValueForTag(FlavorTag);
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