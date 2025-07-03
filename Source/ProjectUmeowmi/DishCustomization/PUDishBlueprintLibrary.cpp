#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"

bool UPUDishBlueprintLibrary::AddIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    // Check if we already have this ingredient
    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            // If we already have this ingredient, increment its quantity
            Instance.Quantity++;
            return true;
        }
    }

    // If we don't have this ingredient, create a new instance
    FIngredientInstance NewInstance;
    NewInstance.IngredientTag = IngredientTag;
    NewInstance.Quantity = 1;
    Dish.IngredientInstances.Add(NewInstance);
    return true;
}

bool UPUDishBlueprintLibrary::RemoveIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.IngredientInstances.RemoveAll([&](const FIngredientInstance& Instance) {
        return Instance.IngredientTag == IngredientTag;
    }) > 0;
}

bool UPUDishBlueprintLibrary::IncrementIngredientAmount(FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            // Get the base ingredient to check max quantity
            FPUIngredientBase BaseIngredient;
            if (Dish.GetIngredient(IngredientTag, BaseIngredient))
            {
                // Check if we're within the max quantity
                if (Instance.Quantity + Amount <= BaseIngredient.MaxQuantity)
                {
                    Instance.Quantity += Amount;
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

bool UPUDishBlueprintLibrary::DecrementIngredientAmount(FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            // Get the base ingredient to check min quantity
            FPUIngredientBase BaseIngredient;
            if (Dish.GetIngredient(IngredientTag, BaseIngredient))
            {
                // Check if we're within the min quantity
                if (Instance.Quantity - Amount >= BaseIngredient.MinQuantity)
                {
                    Instance.Quantity -= Amount;
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

int32 UPUDishBlueprintLibrary::GetIngredientQuantity(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            return Instance.Quantity;
        }
    }
    return 0;
}

bool UPUDishBlueprintLibrary::ApplyPreparation(FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTag& PreparationTag)
{
    bool bApplied = false;
    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            Instance.Preparations.AddTag(PreparationTag);
            bApplied = true;
        }
    }
    return bApplied;
}

bool UPUDishBlueprintLibrary::RemovePreparation(FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTag& PreparationTag)
{
    bool bRemoved = false;
    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            Instance.Preparations.RemoveTag(PreparationTag);
            bRemoved = true;
        }
    }
    return bRemoved;
}

float UPUDishBlueprintLibrary::GetTotalValueForProperty(const FPUDishBase& Dish, const FName& PropertyName)
{
    return Dish.GetTotalValueForProperty(PropertyName);
}

TArray<FIngredientProperty> UPUDishBlueprintLibrary::GetPropertiesWithTag(const FPUDishBase& Dish, const FGameplayTag& Tag)
{
    return Dish.GetPropertiesWithTag(Tag);
}

float UPUDishBlueprintLibrary::GetTotalValueForTag(const FPUDishBase& Dish, const FGameplayTag& Tag)
{
    return Dish.GetTotalValueForTag(Tag);
}

bool UPUDishBlueprintLibrary::HasIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.HasIngredient(IngredientTag);
}

bool UPUDishBlueprintLibrary::GetDishFromDataTable(UDataTable* DishDataTable, const FGameplayTag& DishTag, FPUDishBase& OutDish)
{
    if (!DishDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - DishDataTable is null"));
        return false;
    }

    if (!DishTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - DishTag is invalid"));
        return false;
    }

    // Get the dish name from the tag (everything after the last period) and convert to lowercase
    FString FullTag = DishTag.ToString();
    int32 LastPeriodIndex;
    if (FullTag.FindLastChar('.', LastPeriodIndex))
    {
        FString DishName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
        FName RowName = FName(*DishName);
        
        UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Looking for dish: %s (RowName: %s)"), 
            *DishTag.ToString(), *RowName.ToString());
        
        if (FPUDishBase* FoundDish = DishDataTable->FindRow<FPUDishBase>(RowName, TEXT("GetDishFromDataTable")))
        {
            OutDish = *FoundDish;
            UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Found dish: %s with %d default ingredients"), 
                *OutDish.DisplayName.ToString(), OutDish.IngredientInstances.Num());
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Dish not found in data table: %s"), *RowName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Invalid dish tag format: %s"), *DishTag.ToString());
    }
    
    return false;
}

FGameplayTag UPUDishBlueprintLibrary::GetRandomDishTag()
{
    // Define available dish tags
    TArray<FGameplayTag> AvailableDishTags = {
        FGameplayTag::RequestGameplayTag(TEXT("Dish.Congee")),
        FGameplayTag::RequestGameplayTag(TEXT("Dish.ChiFan")),
        FGameplayTag::RequestGameplayTag(TEXT("Dish.HaloHalo"))
    };
    
    // Filter out invalid tags
    TArray<FGameplayTag> ValidDishTags;
    for (const FGameplayTag& Tag : AvailableDishTags)
    {
        if (Tag.IsValid())
        {
            ValidDishTags.Add(Tag);
        }
    }
    
    if (ValidDishTags.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetRandomDishTag - No valid dish tags found"));
        return FGameplayTag::EmptyTag;
    }
    
    // Return a random dish tag
    FGameplayTag RandomTag = ValidDishTags[FMath::RandRange(0, ValidDishTags.Num() - 1)];
    UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetRandomDishTag - Selected dish: %s"), *RandomTag.ToString());
    
    return RandomTag;
}

FText UPUDishBlueprintLibrary::GetCurrentDisplayName(const FPUDishBase& Dish)
{
    return Dish.GetCurrentDisplayName();
}

TArray<FPUIngredientBase> UPUDishBlueprintLibrary::GetAllIngredients(const FPUDishBase& Dish)
{
    return Dish.GetAllIngredients();
}

bool UPUDishBlueprintLibrary::GetIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient)
{
    return Dish.GetIngredient(IngredientTag, OutIngredient);
} 