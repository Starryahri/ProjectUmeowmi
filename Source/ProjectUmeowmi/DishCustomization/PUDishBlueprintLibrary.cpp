#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"

bool UPUDishBlueprintLibrary::AddIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    FIngredientInstance NewInstance;
    NewInstance.IngredientTag = IngredientTag;
    Dish.IngredientInstances.Add(NewInstance);
    return true;
}

bool UPUDishBlueprintLibrary::RemoveIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.IngredientInstances.RemoveAll([&](const FIngredientInstance& Instance) {
        return Instance.IngredientTag == IngredientTag;
    }) > 0;
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