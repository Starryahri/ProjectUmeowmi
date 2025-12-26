#include "PUDish.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "PUDishBlueprintLibrary.h"

APUDish::APUDish()
{
    PrimaryActorTick.bCanEverTick = false;
}

FIngredientInstance APUDish::AddIngredient(const FGameplayTag& IngredientTag)
{
    FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(DishData, IngredientTag);
    OnIngredientAdded.Broadcast(IngredientTag);
    return NewInstance;
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

float APUDish::GetTotalFlavorAspect(const FName& AspectName) const
{
    return UPUDishBlueprintLibrary::GetTotalFlavorAspect(DishData, AspectName);
}

float APUDish::GetTotalTextureAspect(const FName& AspectName) const
{
    return UPUDishBlueprintLibrary::GetTotalTextureAspect(DishData, AspectName);
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

TArray<FIngredientInstance> APUDish::GetAllIngredientInstances() const
{
    return UPUDishBlueprintLibrary::GetAllIngredientInstances(DishData);
}

bool APUDish::GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const
{
    return UPUDishBlueprintLibrary::GetIngredient(DishData, IngredientTag, OutIngredient);
} 