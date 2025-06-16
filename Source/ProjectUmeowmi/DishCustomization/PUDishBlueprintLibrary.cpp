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