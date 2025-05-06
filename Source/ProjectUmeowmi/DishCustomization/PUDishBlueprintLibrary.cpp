#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"

bool UPUDishBlueprintLibrary::AddIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    if (!Dish.HasIngredient(IngredientTag))
    {
        Dish.IngredientTags.Add(IngredientTag);
        return true;
    }
    return false;
}

bool UPUDishBlueprintLibrary::RemoveIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.IngredientTags.Remove(IngredientTag) > 0;
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