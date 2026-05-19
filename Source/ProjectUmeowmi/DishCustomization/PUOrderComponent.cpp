#include "PUOrderComponent.h"
#include "PUOrderBlueprintLibrary.h"
#include "PUDishBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../Dialogue/PUDishGiver.h"

UPUOrderComponent::UPUOrderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    DefaultMinIngredients = 3;
    DefaultOrderDescription = FText::FromString(TEXT("Make me something with {0} ingredients. I want it {1}."));
    bHasActiveOrder = false;
}

void UPUOrderComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[OrderGen] UPUOrderComponent::BeginPlay - owner=%s"), GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"));
}

#if WITH_EDITOR
void UPUOrderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const FName PropName = PropertyChangedEvent.GetPropertyName();
    if (PropName == GET_MEMBER_NAME_CHECKED(UPUOrderComponent, DefaultTargetAspects))
    {
        for (FOrderAspectRequirement& Req : DefaultTargetAspects)
        {
            Req.AspectName = Req.GetAspectName();
        }
    }
}
#endif

void UPUOrderComponent::GenerateNewOrder()
{
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrder - Starting order generation"));

    // Validate the world first
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] GenerateNewOrder - No valid world!"));
        return;
    }

    // Check if player has a completed order - if so, don't generate a new one
    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
        }
    }

    if (PlayerChar && PlayerChar->IsCurrentOrderCompleted())
    {
        UE_LOG(LogTemp, Warning, TEXT("[OrderGen] GenerateNewOrder - Player has completed order, refusing to generate new order"));
        return;
    }

    // Clear any existing order (only if no completed order)
    if (bHasActiveOrder)
    {
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrder - Clearing existing order"));
        ClearCurrentOrder();
    }

    GenerateSimpleOrder(FGameplayTag()); // invalid = use pool or fallback
    bHasActiveOrder = true;

    UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrder - Order generated successfully"));
    CurrentOrder.LogOrderDetails();
    
    // Broadcast the event
    OnOrderGenerated.Broadcast(CurrentOrder);
}

void UPUOrderComponent::ClearCurrentOrder()
{
    //UE_LOG(LogTemp,Log, TEXT("UPUOrderComponent::ClearCurrentOrder - Clearing current order"));
    
    // Properly clean up UObject references before clearing
    //UE_LOG(LogTemp,Display, TEXT("UPUOrderComponent::ClearCurrentOrder - Cleaning up UObject references"));
    
    // Clear UObject references in the completed dish
    if (CurrentOrder.CompletedDish.PreviewTexture)
    {
        CurrentOrder.CompletedDish.PreviewTexture = nullptr;
    }
    if (CurrentOrder.CompletedDish.IngredientDataTable.IsValid())
    {
        CurrentOrder.CompletedDish.IngredientDataTable = nullptr;
    }
    
    // Clear UObject references in all ingredient instances
    for (FIngredientInstance& Instance : CurrentOrder.CompletedDish.IngredientInstances)
    {
        if (Instance.IngredientData.PreviewTexture)
        {
            Instance.IngredientData.PreviewTexture = nullptr;
        }
        if (Instance.IngredientData.MaterialInstance.IsValid())
        {
            Instance.IngredientData.MaterialInstance = nullptr;
        }
    }
    
    // Clear UObject references in the base dish
    if (CurrentOrder.BaseDish.PreviewTexture)
    {
        CurrentOrder.BaseDish.PreviewTexture = nullptr;
    }
    if (CurrentOrder.BaseDish.IngredientDataTable.IsValid())
    {
        CurrentOrder.BaseDish.IngredientDataTable = nullptr;
    }
    
    // Clear UObject references in base dish ingredient instances
    for (FIngredientInstance& Instance : CurrentOrder.BaseDish.IngredientInstances)
    {
        if (Instance.IngredientData.PreviewTexture)
        {
            Instance.IngredientData.PreviewTexture = nullptr;
        }
        if (Instance.IngredientData.MaterialInstance.IsValid())
        {
            Instance.IngredientData.MaterialInstance = nullptr;
        }
    }
    
    // Now safely clear the order data
    CurrentOrder = FPUOrderBase();
    bHasActiveOrder = false;
    
    //UE_LOG(LogTemp,Log, TEXT("UPUOrderComponent::ClearCurrentOrder - Order cleared"));
}

bool UPUOrderComponent::ValidateDish(const FPUDishBase& Dish) const
{
    if (!bHasActiveOrder)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUOrderComponent::ValidateDish - No active order to validate against"));
        return false;
    }
    
    //UE_LOG(LogTemp,Log, TEXT("UPUOrderComponent::ValidateDish - Validating dish against current order"));
    return UPUOrderBlueprintLibrary::ValidateDish(CurrentOrder, Dish);
}

float UPUOrderComponent::GetSatisfactionScore(const FPUDishBase& Dish) const
{
    if (!bHasActiveOrder)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUOrderComponent::GetSatisfactionScore - No active order to score against"));
        return 0.0f;
    }
    
    //UE_LOG(LogTemp,Log, TEXT("UPUOrderComponent::GetSatisfactionScore - Calculating satisfaction score"));
    return UPUOrderBlueprintLibrary::GetSatisfactionScore(CurrentOrder, Dish);
}

void UPUOrderComponent::GenerateNewOrderWithDish(FGameplayTag DishTag)
{
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrderWithDish - Starting (dish tag: %s)"),
        DishTag.IsValid() ? *DishTag.ToString() : TEXT("(invalid — will use pool/fallback)"));

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[OrderGen] GenerateNewOrderWithDish - No valid world!"));
        return;
    }

    AProjectUmeowmiCharacter* PlayerChar = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
    }
    if (PlayerChar && PlayerChar->IsCurrentOrderCompleted())
    {
        UE_LOG(LogTemp, Warning, TEXT("[OrderGen] GenerateNewOrderWithDish - Player has completed order, refusing"));
        return;
    }
    if (bHasActiveOrder)
    {
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrderWithDish - Clearing existing order"));
        ClearCurrentOrder();
    }

    GenerateSimpleOrder(DishTag);
    bHasActiveOrder = true;

    UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateNewOrderWithDish - Order generated successfully"));
    CurrentOrder.LogOrderDetails();
    OnOrderGenerated.Broadcast(CurrentOrder);
}

void UPUOrderComponent::GenerateSimpleOrder(FGameplayTag OptionalDishTag)
{
    FGameplayTag DishTag;
    if (OptionalDishTag.IsValid())
    {
        DishTag = OptionalDishTag;
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateSimpleOrder - Using override dish tag: %s"), *DishTag.ToString());
    }
    else if (AvailableDishTags.Num() > 0)
    {
        TArray<FGameplayTag> Valid;
        for (const FGameplayTag& Tag : AvailableDishTags)
        {
            if (Tag.IsValid()) Valid.Add(Tag);
        }
        if (Valid.Num() > 0)
        {
            DishTag = Valid[FMath::RandRange(0, Valid.Num() - 1)];
            UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateSimpleOrder - Picked from pool (%d tags): %s"), Valid.Num(), *DishTag.ToString());
        }
    }
    if (!DishTag.IsValid())
    {
        DishTag = UPUDishBlueprintLibrary::GetRandomDishTag();
        if (DishTag.IsValid())
        {
            UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateSimpleOrder - From GetRandomDishTag: %s"), *DishTag.ToString());
        }
    }
    if (!DishTag.IsValid())
    {
        DishTag = FGameplayTag::RequestGameplayTag(TEXT("Dish.Congee"));
        UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateSimpleOrder - Fallback dish: Dish.Congee"));
    }

    FPUDishBase BaseDish;
    bool bGotBaseDish = false;
    if (DishDataTable && IsValid(DishDataTable))
    {
        bGotBaseDish = UPUDishBlueprintLibrary::GetDishFromDataTable(DishDataTable, IngredientDataTable, DishTag, BaseDish, PreparationDataTable);
    }
    UE_LOG(LogTemp, Display, TEXT("[OrderGen] GenerateSimpleOrder - Base dish from data table: %s (ingredients: %d)"), bGotBaseDish ? TEXT("yes") : TEXT("no"), BaseDish.IngredientInstances.Num());
    if (!bGotBaseDish)
    {
        BaseDish.DishTag = DishTag;
        BaseDish.DisplayName = FText::FromString(DishTag.ToString());
        BaseDish.IngredientDataTable = nullptr;
        BaseDish.IngredientInstances.Empty();
    }

    FName OrderID = FName(*FString::Printf(TEXT("Order_%d"), FMath::RandRange(1000, 9999)));
    FString AspectSummary = (DefaultTargetAspects.Num() > 0)
        ? DefaultTargetAspects[0].GetAspectName().ToString()
        : FString(TEXT("flavorful"));
    FText DialogueText = FText::Format(
        DefaultOrderDescription,
        FText::AsNumber(DefaultMinIngredients),
        FText::FromString(AspectSummary)
    );

    TArray<FOrderAspectRequirement> Aspects = DefaultTargetAspects;
    for (FOrderAspectRequirement& Req : Aspects)
    {
        Req.AspectName = Req.GetAspectName();
    }
    if (Aspects.Num() == 0)
    {
        FOrderAspectRequirement DefaultReq;
        DefaultReq.AspectType = EOrderAspectType::Flavor;
        DefaultReq.FlavorAspect = EPUFlavorAspect::Salt;
        DefaultReq.TargetValue = 5.0f;
        DefaultReq.AspectName = DefaultReq.GetAspectName();
        Aspects.Add(DefaultReq);
    }

    CurrentOrder = UPUOrderBlueprintLibrary::CreateSimpleOrder(
        OrderID,
        FText::FromString(FString::Printf(TEXT("Simple %s order"), *DishTag.ToString())),
        DefaultMinIngredients,
        Aspects,
        DialogueText
    );
    CurrentOrder.BaseDish = BaseDish;

    // Set dish giver from owner if this component is on a dish giver
    if (APUDishGiver* DishGiver = Cast<APUDishGiver>(GetOwner()))
    {
        CurrentOrder.OrderGiverParticipantName = DishGiver->GetTalkingObjectName();
    }
} 