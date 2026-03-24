#include "PUDishGiver.h"
#include "Engine/Engine.h"
#include "../ProjectUmeowmiCharacter.h"

APUDishGiver::APUDishGiver()
{
    // Dish giver emotes use screen space (match player / dialogue UX); TalkingObject defaults to World.
    EmoteWidgetSpace = EWidgetSpace::Screen;
    if (EmoteWidget)
    {
        EmoteWidget->SetWidgetSpace(EWidgetSpace::Screen);
    }

    // Set the object type to NPC since dish givers are characters
    ObjectType = ETalkingObjectType::NPC;
    
    // Create the order component
    OrderComponent = CreateDefaultSubobject<UPUOrderComponent>(TEXT("OrderComponent"));
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::APUDishGiver - Dish giver created with order component: %s"), 
    //    OrderComponent ? TEXT("SUCCESS") : TEXT("FAILED"));
}

void APUDishGiver::BeginPlay()
{
    EmoteWidgetSpace = EWidgetSpace::Screen;
    if (EmoteWidget)
    {
        EmoteWidget->SetWidgetSpace(EWidgetSpace::Screen);
    }

    Super::BeginPlay();
    
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::BeginPlay - Dish giver initialized: %s"), *GetTalkingObjectDisplayName().ToString());
}

void APUDishGiver::StartInteraction()
{
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::StartInteraction - Starting interaction with dish giver: %s"), *GetTalkingObjectDisplayName().ToString());
    
    // Find the player character first to check their current order status
    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }
    
    // Check if player has a completed order FROM THIS dish giver first
    if (PlayerChar && PlayerChar->IsCurrentOrderCompleted())
    {
        const FPUOrderBase& Order = PlayerChar->GetCurrentOrder();
        const FName MyParticipantName = GetTalkingObjectName();
        if (Order.OrderGiverParticipantName.IsNone() || Order.OrderGiverParticipantName == MyParticipantName)
        {
            HandleOrderCompletion(PlayerChar);
        }
    }
    // Check if player already has an active order
    else if (PlayerChar && PlayerChar->HasCurrentOrder())
    {
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::StartInteraction - Player already has an active order: %s"), 
            //*PlayerChar->GetCurrentOrder().OrderID.ToString());
    }
    
    // Don't generate orders automatically - let dialogue control this
    // Just start the dialogue system
    Super::StartInteraction();
}

void APUDishGiver::GenerateOrderForDialogue()
{
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateOrderForDialogue - %s"), *GetName());

    // Generate the order
    if (!IsValid(OrderComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateOrderForDialogue - Order component is not valid! (%s)"), *GetName());
        return;
    }

    OrderComponent->GenerateNewOrder();
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateOrderForDialogue - Done hasActive=%d (%s)"),
        OrderComponent->HasActiveOrder() ? 1 : 0, *GetName());
}

void APUDishGiver::GenerateAndGiveOrderToPlayer()
{
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Start (%s)"), *GetName());

    // Validate the order component first
    if (!IsValid(OrderComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Order component is not valid! (%s)"), *GetName());
        return;
    }

    // Find the player character
    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }

    if (!IsValid(PlayerChar))
    {
        UE_LOG(LogTemp, Warning, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - No valid player character (%s)"), *GetName());
        return;
    }

    // Check if player already has an active order
    if (PlayerChar->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Player already has order %s, skipping"),
            *PlayerChar->GetCurrentOrder().OrderID.ToString());
        return;
    }

    GenerateOrderForDialogue();

    // Validate that the order was generated successfully
    if (!OrderComponent->HasActiveOrder())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Generation failed, no active order (%s)"), *GetName());
        return;
    }

    // Get the order with validation
    FPUOrderBase Order = OrderComponent->GetCurrentOrder();
    if (!Order.OrderID.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Invalid order ID (%s)"), *GetName());
        return;
    }

    Order.OrderGiverParticipantName = GetTalkingObjectName();
    PlayerChar->SetCurrentOrder(Order);
    SetDialogueVariablesFromOrder(Order);

    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayer - Gave order %s to player (%s)"),
        *Order.OrderID.ToString(), *GetName());
}

void APUDishGiver::GenerateAndGiveOrderToPlayerWithDish(FGameplayTag DishTag)
{
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - dish=%s (%s)"),
        *DishTag.ToString(), *GetName());

    if (!IsValid(OrderComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - No order component (%s)"), *GetName());
        return;
    }

    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }
    if (!IsValid(PlayerChar))
    {
        UE_LOG(LogTemp, Warning, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - No player (%s)"), *GetName());
        return;
    }
    if (PlayerChar->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - Player already has order, skipping"));
        return;
    }

    OrderComponent->GenerateNewOrderWithDish(DishTag);
    if (!OrderComponent->HasActiveOrder())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - No active order after generate (%s)"), *GetName());
        return;
    }

    FPUOrderBase Order = OrderComponent->GetCurrentOrder();
    if (!Order.OrderID.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - Invalid order ID (%s)"), *GetName());
        return;
    }

    Order.OrderGiverParticipantName = GetTalkingObjectName();
    PlayerChar->SetCurrentOrder(Order);
    SetDialogueVariablesFromOrder(Order);

    UE_LOG(LogTemp, Display, TEXT("[OrderGen] APUDishGiver::GenerateAndGiveOrderToPlayerWithDish - Gave order %s"), *Order.OrderID.ToString());
}

void APUDishGiver::RevealHintToPlayer(FName AspectName)
{
    UE_LOG(LogTemp, Display, TEXT("[Hint] RevealHintToPlayer(%s) called on %s"), *AspectName.ToString(), *GetName());

    if (!GetWorld() || AspectName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintToPlayer aborted - no world or invalid aspect"));
        return;
    }

    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
    }

    if (!IsValid(PlayerChar) || !PlayerChar->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintToPlayer aborted - player has no active order"));
        return;
    }

    const FPUOrderBase& Order = PlayerChar->GetCurrentOrder();
    const FName MyParticipantName = GetTalkingObjectName();
    if (!Order.OrderGiverParticipantName.IsNone() && Order.OrderGiverParticipantName != MyParticipantName)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintToPlayer aborted - order is from %s, not %s"), *Order.OrderGiverParticipantName.ToString(), *MyParticipantName.ToString());
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[Hint] Passing hint %s to player character"), *AspectName.ToString());
    PlayerChar->RevealHintOnCurrentOrder(AspectName);
}

FText APUDishGiver::GetOrderDialogueText() const
{
    if (OrderComponent && OrderComponent->HasActiveOrder())
    {
        const FPUOrderBase& CurrentOrder = OrderComponent->GetCurrentOrder();
        //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::GetOrderDialogueText - Returning order dialogue text"));
        return CurrentOrder.OrderDialogueText;
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::GetOrderDialogueText - No active order available"));
    return FText::FromString(TEXT("I don't have any orders right now."));
}

bool APUDishGiver::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    //UE_LOG(LogTemp,Display, TEXT("=== APUDishGiver::CheckCondition CALLED ==="));
    //UE_LOG(LogTemp,Display, TEXT("Condition Name: %s"), *ConditionName.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("This Object: %s"), *GetName());
    
    // Helper function to get player character using weak reference first
    auto GetPlayerCharacter = [this]() -> AProjectUmeowmiCharacter*
    {
        // Try cached weak reference first
        AProjectUmeowmiCharacter* PlayerChar = CachedPlayerCharacter.Get();
        if (IsValid(PlayerChar))
        {
            return PlayerChar;
        }
        
        // Fallback: Find the player character
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                return Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
            }
        }
        return nullptr;
    };
    
    // Use switch statement for better performance and maintainability
    const FString ConditionNameStr = ConditionName.ToString();
    
    // Convert string to integer for switch statement
    int32 ConditionType = 0;
    if (ConditionNameStr == TEXT("HasActiveOrder"))
    {
        ConditionType = 1;
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::CheckCondition - Matched HasActiveOrder condition"));
    }
    else if (ConditionNameStr == TEXT("OrderCompleted"))
    {
        ConditionType = 2;
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::CheckCondition - Matched OrderCompleted condition"));
    }
    else if (ConditionNameStr == TEXT("NoActiveOrder"))
    {
        ConditionType = 3;
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::CheckCondition - Matched NoActiveOrder condition"));
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::CheckCondition - No match for condition: '%s'"), *ConditionNameStr);
    }
    
    switch (ConditionType)
    {
        case 1: // HasActiveOrder - true only if player has an active order FROM THIS dish giver
        {
            if (AProjectUmeowmiCharacter* PlayerChar = GetPlayerCharacter())
            {
                if (!PlayerChar->HasCurrentOrder()) return false;
                const FPUOrderBase& Order = PlayerChar->GetCurrentOrder();
                const FName MyParticipantName = GetTalkingObjectName();
                bool bOrderIsFromMe = Order.OrderGiverParticipantName.IsNone() || Order.OrderGiverParticipantName == MyParticipantName;
                return bOrderIsFromMe;
            }
            return false;
        }
        
        case 2: // OrderCompleted - true only if player has a completed order FROM THIS dish giver
        {
            if (AProjectUmeowmiCharacter* PlayerChar = GetPlayerCharacter())
            {
                if (!PlayerChar->IsCurrentOrderCompleted()) return false;
                const FPUOrderBase& Order = PlayerChar->GetCurrentOrder();
                const FName MyParticipantName = GetTalkingObjectName();
                bool bOrderIsFromMe = Order.OrderGiverParticipantName.IsNone() || Order.OrderGiverParticipantName == MyParticipantName;
                return bOrderIsFromMe;
            }
            return false;
        }
        
        case 3: // NoActiveOrder
        {
            if (AProjectUmeowmiCharacter* PlayerChar = GetPlayerCharacter())
            {
                bool bNoOrder = !PlayerChar->HasCurrentOrder();
                //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::CheckCondition - NoActiveOrder: %s"), bNoOrder ? TEXT("TRUE") : TEXT("FALSE"));
                return bNoOrder;
            }
            //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::CheckCondition - NoActiveOrder: Could not find player character, returning TRUE"));
            return true; // Default to true if we can't find the player
        }
        
        default:
            //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::CheckCondition - Unknown condition: %s, calling parent"), *ConditionName.ToString());
            return Super::CheckCondition_Implementation(Context, ConditionName);
    }
}

FText APUDishGiver::GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const
{
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::GetParticipantDisplayName_Implementation - Getting display name for speaker: %s"), *ActiveSpeaker.ToString());
    
    // For now, just return the base display name
    // In the future, this could be modified based on order state
    return Super::GetParticipantDisplayName_Implementation(ActiveSpeaker);
}

// Add a simple test condition that always returns true to verify conditions are being called
bool APUDishGiver::GetBoolValue_Implementation(FName ValueName) const
{
    //UE_LOG(LogTemp,Display, TEXT("=== APUDishGiver::GetBoolValue CALLED ==="));
    //UE_LOG(LogTemp,Display, TEXT("Value Name: %s"), *ValueName.ToString());
    
    if (ValueName == TEXT("TestCondition"))
    {
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::GetBoolValue - TestCondition returning TRUE"));
        return true;
    }
    else if (ValueName == TEXT("bTestCondition"))
    {
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::GetBoolValue - bTestCondition returning: %s"), bTestCondition ? TEXT("TRUE") : TEXT("FALSE"));
        return bTestCondition;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::GetBoolValue - Unknown value, calling parent"));
    return Super::GetBoolValue_Implementation(ValueName);
}

const FPUOrderBase& APUDishGiver::GetCurrentOrder() const
{
    if (OrderComponent && OrderComponent->HasActiveOrder())
    {
        //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::GetCurrentOrder - Returning current order"));
        return OrderComponent->GetCurrentOrder();
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::GetCurrentOrder - No active order available"));
    static FPUOrderBase EmptyOrder;
    return EmptyOrder;
}

bool APUDishGiver::HasActiveOrder() const
{
    bool bHasOrder = OrderComponent ? OrderComponent->HasActiveOrder() : false;
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::HasActiveOrder - Has active order: %s"), bHasOrder ? TEXT("YES") : TEXT("NO"));
    return bHasOrder;
}

bool APUDishGiver::ValidateDish(const FPUDishBase& Dish) const
{
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::ValidateDish - Validating dish against current order"));
    
    if (OrderComponent)
    {
        return OrderComponent->ValidateDish(Dish);
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::ValidateDish - Order component is null!"));
    return false;
}

float APUDishGiver::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::GetSatisfactionScore - Calculating satisfaction score"));
    
    if (OrderComponent)
    {
        return OrderComponent->GetSatisfactionScore(Dish);
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::GetSatisfactionScore - Order component is null!"));
    return 0.0f;
}

void APUDishGiver::HandleOrderCompletion(AProjectUmeowmiCharacter* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::HandleOrderCompletion - Player character is null"));
        return;
    }

    if (!PlayerCharacter->IsCurrentOrderCompleted())
    {
        //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::HandleOrderCompletion - No completed order to handle"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::HandleOrderCompletion - Handling order completion"));
    
    // Cache the player character using weak reference
    CachedPlayerCharacter = PlayerCharacter;
    
    // Get the completed order with all the dish data
    const FPUOrderBase& CompletedOrder = PlayerCharacter->GetCurrentOrder();
    
    // Analyze the completed dish and set dialogue variables
    AnalyzeCompletedDish(CompletedOrder);
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::HandleOrderCompletion - Order analysis complete, dialogue variables set"));
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::HandleOrderCompletion - Satisfaction: %.1f%%"), CompletedOrder.FinalSatisfactionScore * 100.0f);
    
    // Note: Order clearing is now controlled by dialogue system
    // Use ClearCompletedOrderFromPlayer dialogue event to clear when ready
}

FText APUDishGiver::GetOrderCompletionFeedback(AProjectUmeowmiCharacter* PlayerCharacter) const
{
    if (!PlayerCharacter || !PlayerCharacter->IsCurrentOrderCompleted())
    {
        return FText::FromString(TEXT("I don't see any completed orders."));
    }
    
    float SatisfactionScore = PlayerCharacter->GetOrderSatisfaction();
    
    // 4-grade system: Perfect (1.0), Great (0.75), Okay (0.5), Needs Improvement (0.25)
    FString LocalFeedbackText;
    if (SatisfactionScore >= 0.875f)
    {
        LocalFeedbackText = TEXT("Wow! This is absolutely perfect! You've exceeded my expectations!");
    }
    else if (SatisfactionScore >= 0.625f)
    {
        LocalFeedbackText = TEXT("Excellent work! This is exactly what I was looking for!");
    }
    else if (SatisfactionScore >= 0.375f)
    {
        LocalFeedbackText = TEXT("Good job! This meets my requirements nicely.");
    }
    else
    {
        LocalFeedbackText = TEXT("This isn't quite right. You're missing key ingredients or the balance is off.");
    }
    
    // Store the feedback in the class variable (const cast needed since this is a const function)
    const_cast<APUDishGiver*>(this)->SatisfactionFeedbackText = FText::FromString(LocalFeedbackText);
    
    return FText::FromString(LocalFeedbackText);
}

void APUDishGiver::ClearCompletedOrderFromPlayer()
{
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::ClearCompletedOrderFromPlayer - Marking order for delayed clearing"));
    
    // Instead of clearing immediately, mark it for delayed clearing
    MarkOrderForClearing();
}

void APUDishGiver::MarkOrderForClearing()
{
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::MarkOrderForClearing - Marking order for delayed clearing"));
    
    // Mark the order for clearing
    bOrderMarkedForClearing = true;
    
    // Set a timer to clear the order after a short delay (after dialogue processing is complete)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            DelayedClearingTimerHandle,
            this,
            &APUDishGiver::ExecuteDelayedOrderClearing,
            0.1f, // 100ms delay
            false  // Don't repeat
        );
    }
}

void APUDishGiver::ExecuteDelayedOrderClearing()
{
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::ExecuteDelayedOrderClearing - Executing delayed order clearing"));
    
    // Use the cached weak reference first, then fall back to finding the player
    AProjectUmeowmiCharacter* PlayerChar = CachedPlayerCharacter.Get();
    
    if (!PlayerChar)
    {
        // Fallback: Find the player character
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
            }
        }
    }
    
    if (!IsValid(PlayerChar))
    {
        //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::ExecuteDelayedOrderClearing - Could not find valid player character"));
        bOrderMarkedForClearing = false;
        return;
    }
    
    if (!PlayerChar->IsCurrentOrderCompleted())
    {
        //UE_LOG(LogTemp,Warning, TEXT("APUDishGiver::ExecuteDelayedOrderClearing - Player has no completed order to clear"));
        bOrderMarkedForClearing = false;
        return;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::ExecuteDelayedOrderClearing - About to clear completed order"));
    PlayerChar->ClearCompletedOrder();
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::ExecuteDelayedOrderClearing - Completed order cleared successfully"));
    
    // Clear the cached reference after successful clearing
    CachedPlayerCharacter = nullptr;
    bOrderMarkedForClearing = false;
}

void APUDishGiver::SetDialogueVariablesFromOrder(const FPUOrderBase& Order)
{
    bHasOrderReady = true;
    OrderDescription = Order.OrderDescription;
    MinIngredientCount = Order.MinIngredientCount;
    OrderDialogueText = Order.OrderDialogueText;
    // First aspect for backward-compat dialogue vars (TargetFlavorProperty / MinFlavorValue)
    if (Order.TargetAspects.Num() > 0)
    {
        const FOrderAspectRequirement& First = Order.TargetAspects[0];
        TargetFlavorProperty = FText::FromString(First.GetAspectName().ToString());
        MinFlavorValue = First.TargetValue;
    }
    else
    {
        TargetFlavorProperty = FText::FromString(TEXT(""));
        MinFlavorValue = 0.0f;
    }
}

void APUDishGiver::AnalyzeCompletedDish(const FPUOrderBase& CompletedOrder)
{
    const FPUDishBase& CompletedDish = CompletedOrder.CompletedDish;
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::AnalyzeCompletedDish - Analyzing completed dish for order: %s"), *CompletedOrder.OrderID.ToString());
    
    // Set basic completion data
    bHasCompletedDish = true;
    CompletedDishSatisfaction = CompletedOrder.FinalSatisfactionScore;
    CompletedDishIngredientCount = CompletedDish.IngredientInstances.Num();
    
    // First target aspect for dialogue (flavor/texture value and requirement)
    if (CompletedOrder.TargetAspects.Num() > 0)
    {
        const FOrderAspectRequirement& First = CompletedOrder.TargetAspects[0];
        const FName FirstAspectName = First.GetAspectName();
        float Val = (First.AspectType == EOrderAspectType::Flavor)
            ? CompletedDish.GetTotalFlavorAspect(FirstAspectName)
            : CompletedDish.GetTotalTextureAspect(FirstAspectName);
        CompletedDishFlavorValue = FText::FromString(FString::Printf(TEXT("%.1f"), Val));
        CompletedDishTargetFlavor = FText::FromString(FirstAspectName.ToString());
        CompletedDishMinFlavorValue = FText::FromString(FString::Printf(TEXT("%.1f"), First.TargetValue));
    }
    else
    {
        CompletedDishFlavorValue = FText::FromString(TEXT("0"));
        CompletedDishTargetFlavor = FText::FromString(TEXT(""));
        CompletedDishMinFlavorValue = FText::FromString(TEXT("0"));
    }
    
    // Find most used ingredient
    TMap<FGameplayTag, int32> IngredientQuantities;
    for (const FIngredientInstance& Instance : CompletedDish.IngredientInstances)
    {
        IngredientQuantities.FindOrAdd(Instance.IngredientData.IngredientTag) += Instance.Quantity;
    }
    
    // Find the ingredient with highest quantity
    FGameplayTag MostUsedTag;
    int32 MaxQuantity = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        if (Pair.Value > MaxQuantity)
        {
            MaxQuantity = Pair.Value;
            MostUsedTag = Pair.Key;
        }
    }
    MostUsedIngredient = FText::FromString(MostUsedTag.ToString());
    
    // Count preparations
    PreparationCount = 0;
    for (const FIngredientInstance& Instance : CompletedDish.IngredientInstances)
    {
        if (Instance.IngredientData.ActivePreparations.Num() > 0)
        {
            PreparationCount++;
        }
    }
    
    // Determine quality level (4-grade system: Perfect, Great, Okay, Needs Improvement)
    if (CompletedDishSatisfaction >= 0.875f) QualityLevel = FText::FromString(TEXT("Perfect"));
    else if (CompletedDishSatisfaction >= 0.625f) QualityLevel = FText::FromString(TEXT("Great"));
    else if (CompletedDishSatisfaction >= 0.375f) QualityLevel = FText::FromString(TEXT("Okay"));
    else QualityLevel = FText::FromString(TEXT("Needs Improvement"));
    
    // Generate basic feedback text
    FeedbackText = FText::FromString(FString::Printf(TEXT("Your %s level of %s vs the required %s - %s!"), 
        *CompletedDishTargetFlavor.ToString(),
        *CompletedDishFlavorValue.ToString(),
        *CompletedDishMinFlavorValue.ToString(),
        *QualityLevel.ToString()));
    
    // Generate satisfaction-based feedback text
    if (CompletedDishSatisfaction >= 0.875f)
    {
        SatisfactionFeedbackText = FText::FromString(TEXT("Wow! This is absolutely perfect! You've exceeded my expectations!"));
    }
    else if (CompletedDishSatisfaction >= 0.625f)
    {
        SatisfactionFeedbackText = FText::FromString(TEXT("Excellent work! This is exactly what I was looking for!"));
    }
    else if (CompletedDishSatisfaction >= 0.375f)
    {
        SatisfactionFeedbackText = FText::FromString(TEXT("Good job! This meets my requirements nicely."));
    }
    else
    {
        SatisfactionFeedbackText = FText::FromString(TEXT("This isn't quite right. You're missing key ingredients or the balance is off."));
    }
    
    //UE_LOG(LogTemp,Display, TEXT("APUDishGiver::AnalyzeCompletedDish - Analysis complete:"));
    //UE_LOG(LogTemp,Display, TEXT("  Satisfaction: %.2f"), CompletedDishSatisfaction);
    //UE_LOG(LogTemp,Display, TEXT("  Ingredient Count: %d"), CompletedDishIngredientCount);
    //UE_LOG(LogTemp,Display, TEXT("  Flavor: %s = %s (required: %s)"), 
        //*CompletedDishTargetFlavor.ToString(), 
        //*CompletedDishFlavorValue.ToString(), 
        //*CompletedDishMinFlavorValue.ToString());
    //UE_LOG(LogTemp,Display, TEXT("  Most Used: %s"), *MostUsedIngredient.ToString());
    //UE_LOG(LogTemp,Display, TEXT("  Preparations: %d"), PreparationCount);
    //UE_LOG(LogTemp,Display, TEXT("  Quality: %s"), *QualityLevel.ToString());
}

void APUDishGiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    //UE_LOG(LogTemp,Log, TEXT("APUDishGiver::EndPlay - Cleaning up dish giver: %s"), *GetName());
    
    // Clear any pending timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DelayedClearingTimerHandle);
    }
    
    // Clear the cached player reference
    CachedPlayerCharacter = nullptr;
    bOrderMarkedForClearing = false;
    
    Super::EndPlay(EndPlayReason);
}