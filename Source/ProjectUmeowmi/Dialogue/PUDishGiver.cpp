#include "PUDishGiver.h"
#include "Engine/Engine.h"
#include "../ProjectUmeowmiCharacter.h"

APUDishGiver::APUDishGiver()
{
    // Set the object type to NPC since dish givers are characters
    ObjectType = ETalkingObjectType::NPC;
    
    // Create the order component
    OrderComponent = CreateDefaultSubobject<UPUOrderComponent>(TEXT("OrderComponent"));
    
    UE_LOG(LogTemp, Display, TEXT("APUDishGiver::APUDishGiver - Dish giver created with order component: %s"), 
        OrderComponent ? TEXT("SUCCESS") : TEXT("FAILED"));
}

void APUDishGiver::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::BeginPlay - Dish giver initialized: %s"), *GetTalkingObjectDisplayName().ToString());
}

void APUDishGiver::StartInteraction()
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::StartInteraction - Starting interaction with dish giver: %s"), *GetTalkingObjectDisplayName().ToString());
    
    // Find the player character first to check their current order status
    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }
    
    // Check if player already has an active order
    if (PlayerChar && PlayerChar->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::StartInteraction - Player already has an active order: %s"), 
            *PlayerChar->GetCurrentOrder().OrderID.ToString());
        
        // Check if the order is completed
        if (PlayerChar->IsCurrentOrderCompleted())
        {
            UE_LOG(LogTemp, Display, TEXT("APUDishGiver::StartInteraction - Player has completed order, handling completion"));
            HandleOrderCompletion(PlayerChar);
        }
    }
    
    // Don't generate orders automatically - let dialogue control this
    // Just start the dialogue system
    Super::StartInteraction();
}

void APUDishGiver::GenerateOrderForDialogue()
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GenerateOrderForDialogue - Generating order for dialogue"));
    
    if (OrderComponent)
    {
        OrderComponent->GenerateNewOrder();
        UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GenerateOrderForDialogue - Order generated successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("APUDishGiver::GenerateOrderForDialogue - Order component is null!"));
    }
}

void APUDishGiver::GenerateAndGiveOrderToPlayer()
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GenerateAndGiveOrderToPlayer - Generating and giving order to player"));
    
    // Find the player character
    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }
    
    if (!PlayerChar)
    {
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::GenerateAndGiveOrderToPlayer - Could not find player character"));
        return;
    }
    
    // Check if player already has an active order
    if (PlayerChar->HasCurrentOrder())
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::GenerateAndGiveOrderToPlayer - Player already has an active order, not generating new one"));
        return;
    }
    
    // Generate the order
    GenerateOrderForDialogue();
    
    // Pass the order to the player character
    if (OrderComponent && OrderComponent->HasActiveOrder())
    {
        const FPUOrderBase& Order = OrderComponent->GetCurrentOrder();
        PlayerChar->SetCurrentOrder(Order);
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::GenerateAndGiveOrderToPlayer - Order passed to player character: %s"), *Order.OrderID.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::GenerateAndGiveOrderToPlayer - Failed to generate order"));
    }
}

FText APUDishGiver::GetOrderDialogueText() const
{
    if (OrderComponent && OrderComponent->HasActiveOrder())
    {
        const FPUOrderBase& CurrentOrder = OrderComponent->GetCurrentOrder();
        UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GetOrderDialogueText - Returning order dialogue text"));
        return CurrentOrder.OrderDialogueText;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::GetOrderDialogueText - No active order available"));
    return FText::FromString(TEXT("I don't have any orders right now."));
}

bool APUDishGiver::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    UE_LOG(LogTemp, Display, TEXT("=== APUDishGiver::CheckCondition CALLED ==="));
    UE_LOG(LogTemp, Display, TEXT("Condition Name: %s"), *ConditionName.ToString());
    UE_LOG(LogTemp, Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("This Object: %s"), *GetName());
    
    // Check for order-related conditions
    if (ConditionName == TEXT("HasActiveOrder"))
    {
        // Find the player character to check their order status
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (AProjectUmeowmiCharacter* PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
                {
                    bool bHasOrder = PlayerChar->HasCurrentOrder();
                    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::CheckCondition - HasActiveOrder: %s"), bHasOrder ? TEXT("TRUE") : TEXT("FALSE"));
                    return bHasOrder;
                }
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::CheckCondition - HasActiveOrder: Could not find player character, returning FALSE"));
        return false;
    }
    else if (ConditionName == TEXT("OrderCompleted"))
    {
        // Find the player character to check if their order is completed
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (AProjectUmeowmiCharacter* PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
                {
                    bool bOrderCompleted = PlayerChar->HasCurrentOrder() && PlayerChar->IsCurrentOrderCompleted();
                    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::CheckCondition - OrderCompleted: %s"), bOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
                    return bOrderCompleted;
                }
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::CheckCondition - OrderCompleted: Could not find player character, returning FALSE"));
        return false;
    }
    else if (ConditionName == TEXT("NoActiveOrder"))
    {
        // Find the player character to check if they have no active order
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (AProjectUmeowmiCharacter* PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
                {
                    bool bNoOrder = !PlayerChar->HasCurrentOrder();
                    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::CheckCondition - NoActiveOrder: %s"), bNoOrder ? TEXT("TRUE") : TEXT("FALSE"));
                    return bNoOrder;
                }
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::CheckCondition - NoActiveOrder: Could not find player character, returning TRUE"));
        return true; // Default to true if we can't find the player
    }
    
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::CheckCondition - Unknown condition: %s, calling parent"), *ConditionName.ToString());
    // For other conditions, call the parent implementation
    return Super::CheckCondition_Implementation(Context, ConditionName);
}

FText APUDishGiver::GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GetParticipantDisplayName_Implementation - Getting display name for speaker: %s"), *ActiveSpeaker.ToString());
    
    // For now, just return the base display name
    // In the future, this could be modified based on order state
    return Super::GetParticipantDisplayName_Implementation(ActiveSpeaker);
}

// Add a simple test condition that always returns true to verify conditions are being called
bool APUDishGiver::GetBoolValue_Implementation(FName ValueName) const
{
    UE_LOG(LogTemp, Display, TEXT("=== APUDishGiver::GetBoolValue CALLED ==="));
    UE_LOG(LogTemp, Display, TEXT("Value Name: %s"), *ValueName.ToString());
    
    if (ValueName == TEXT("TestCondition"))
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::GetBoolValue - TestCondition returning TRUE"));
        return true;
    }
    else if (ValueName == TEXT("bTestCondition"))
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::GetBoolValue - bTestCondition returning: %s"), bTestCondition ? TEXT("TRUE") : TEXT("FALSE"));
        return bTestCondition;
    }
    
    UE_LOG(LogTemp, Display, TEXT("APUDishGiver::GetBoolValue - Unknown value, calling parent"));
    return Super::GetBoolValue_Implementation(ValueName);
}

const FPUOrderBase& APUDishGiver::GetCurrentOrder() const
{
    if (OrderComponent && OrderComponent->HasActiveOrder())
    {
        UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GetCurrentOrder - Returning current order"));
        return OrderComponent->GetCurrentOrder();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::GetCurrentOrder - No active order available"));
    static FPUOrderBase EmptyOrder;
    return EmptyOrder;
}

bool APUDishGiver::HasActiveOrder() const
{
    bool bHasOrder = OrderComponent ? OrderComponent->HasActiveOrder() : false;
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::HasActiveOrder - Has active order: %s"), bHasOrder ? TEXT("YES") : TEXT("NO"));
    return bHasOrder;
}

bool APUDishGiver::ValidateDish(const FPUDishBase& Dish) const
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::ValidateDish - Validating dish against current order"));
    
    if (OrderComponent)
    {
        return OrderComponent->ValidateDish(Dish);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::ValidateDish - Order component is null!"));
    return false;
}

float APUDishGiver::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GetSatisfactionScore - Calculating satisfaction score"));
    
    if (OrderComponent)
    {
        return OrderComponent->GetSatisfactionScore(Dish);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::GetSatisfactionScore - Order component is null!"));
    return 0.0f;
}

void APUDishGiver::HandleOrderCompletion(AProjectUmeowmiCharacter* PlayerCharacter)
{
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("APUDishGiver::HandleOrderCompletion - Player character is null"));
        return;
    }

    if (!PlayerCharacter->HasCurrentOrder() || !PlayerCharacter->IsCurrentOrderCompleted())
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::HandleOrderCompletion - No completed order to handle"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("APUDishGiver::HandleOrderCompletion - Handling order completion"));
    
    // Get the order result
    float SatisfactionScore = PlayerCharacter->GetOrderSatisfaction();
    bool bOrderCompleted = PlayerCharacter->IsCurrentOrderCompleted();
    
    if (bOrderCompleted)
    {
        // Determine feedback based on satisfaction
        FString FeedbackText;
        if (SatisfactionScore >= 0.9f)
        {
            FeedbackText = TEXT("Wow! This is absolutely perfect! You've exceeded my expectations!");
        }
        else if (SatisfactionScore >= 0.7f)
        {
            FeedbackText = TEXT("Excellent work! This is exactly what I was looking for!");
        }
        else if (SatisfactionScore >= 0.5f)
        {
            FeedbackText = TEXT("Good job! This meets my requirements nicely.");
        }
        else
        {
            FeedbackText = TEXT("Well, it's acceptable. Could be better, but I'll take it.");
        }
        
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::HandleOrderCompletion - Customer feedback: %s"), *FeedbackText);
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::HandleOrderCompletion - Satisfaction: %.1f%%"), SatisfactionScore * 100.0f);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("APUDishGiver::HandleOrderCompletion - Order was not completed successfully"));
    }
    
    // Clear the completed order from the player
    PlayerCharacter->ClearCompletedOrder();
}

FText APUDishGiver::GetOrderCompletionFeedback(AProjectUmeowmiCharacter* PlayerCharacter) const
{
    if (!PlayerCharacter || !PlayerCharacter->HasCurrentOrder() || !PlayerCharacter->IsCurrentOrderCompleted())
    {
        return FText::FromString(TEXT("I don't see any completed orders."));
    }
    
    float SatisfactionScore = PlayerCharacter->GetOrderSatisfaction();
    
    FString FeedbackText;
    if (SatisfactionScore >= 0.9f)
    {
        FeedbackText = TEXT("Wow! This is absolutely perfect! You've exceeded my expectations!");
    }
    else if (SatisfactionScore >= 0.7f)
    {
        FeedbackText = TEXT("Excellent work! This is exactly what I was looking for!");
    }
    else if (SatisfactionScore >= 0.5f)
    {
        FeedbackText = TEXT("Good job! This meets my requirements nicely.");
    }
    else
    {
        FeedbackText = TEXT("Well, it's acceptable. Could be better, but I'll take it.");
    }
    
    return FText::FromString(FeedbackText);
}