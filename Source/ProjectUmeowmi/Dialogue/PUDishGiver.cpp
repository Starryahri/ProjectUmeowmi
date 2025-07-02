#include "PUDishGiver.h"
#include "Engine/Engine.h"

APUDishGiver::APUDishGiver()
{
    // Set the object type to NPC since dish givers are characters
    ObjectType = ETalkingObjectType::NPC;
    
    // Create the order component
    OrderComponent = CreateDefaultSubobject<UPUOrderComponent>(TEXT("OrderComponent"));
    
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::APUDishGiver - Dish giver created with order component"));
}

void APUDishGiver::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::BeginPlay - Dish giver initialized: %s"), *GetTalkingObjectDisplayName().ToString());
}

void APUDishGiver::StartInteraction()
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::StartInteraction - Starting interaction with dish giver: %s"), *GetTalkingObjectDisplayName().ToString());
    
    // Generate order before starting dialogue
    GenerateOrderForDialogue();
    
    // Call parent implementation to start dialogue
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

bool APUDishGiver::CheckCondition(const UDlgContext* Context, FName ConditionName) const
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::CheckCondition - Checking condition: %s"), *ConditionName.ToString());
    
    // Add order-specific conditions here if needed
    // For now, just call the parent implementation
    return Super::CheckCondition(Context, ConditionName);
}

FText APUDishGiver::GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const
{
    UE_LOG(LogTemp, Log, TEXT("APUDishGiver::GetParticipantDisplayName_Implementation - Getting display name for speaker: %s"), *ActiveSpeaker.ToString());
    
    // For now, just return the base display name
    // In the future, this could be modified based on order state
    return Super::GetParticipantDisplayName_Implementation(ActiveSpeaker);
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