#include "PUDish.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "PUDishBlueprintLibrary.h"

APUDish::APUDish()
{
    PrimaryActorTick.bCanEverTick = false;
}

bool APUDish::AddIngredient(const FGameplayTag& IngredientTag)
{
    if (UPUDishBlueprintLibrary::AddIngredient(DishData, IngredientTag))
    {
        OnIngredientAdded.Broadcast(IngredientTag);
        return true;
    }
    return false;
}

bool APUDish::RemoveIngredient(const FGameplayTag& IngredientTag)
{
    if (UPUDishBlueprintLibrary::RemoveIngredient(DishData, IngredientTag))
    {
        OnIngredientRemoved.Broadcast(IngredientTag);
        return true;
    }
    return false;
}

float APUDish::GetTotalValueForProperty(const FName& PropertyName) const
{
    return UPUDishBlueprintLibrary::GetTotalValueForProperty(DishData, PropertyName);
}

TArray<FIngredientProperty> APUDish::GetPropertiesWithTag(const FGameplayTag& Tag) const
{
    return UPUDishBlueprintLibrary::GetPropertiesWithTag(DishData, Tag);
}

float APUDish::GetTotalValueForTag(const FGameplayTag& Tag) const
{
    return UPUDishBlueprintLibrary::GetTotalValueForTag(DishData, Tag);
}

bool APUDish::HasIngredient(const FGameplayTag& IngredientTag) const
{
    return UPUDishBlueprintLibrary::HasIngredient(DishData, IngredientTag);
}

FText APUDish::GetCurrentDisplayName() const
{
    return UPUDishBlueprintLibrary::GetCurrentDisplayName(DishData);
}

TArray<FPUIngredientBase> APUDish::GetAllIngredients() const
{
    return UPUDishBlueprintLibrary::GetAllIngredients(DishData);
}

bool APUDish::GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const
{
    return UPUDishBlueprintLibrary::GetIngredient(DishData, IngredientTag, OutIngredient);
} 