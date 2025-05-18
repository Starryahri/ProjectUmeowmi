#include "PUIngredientBlueprintLibrary.h"

bool UPUIngredientBlueprintLibrary::ApplyPreparation(FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation)
{
    return Ingredient.ApplyPreparation(Preparation);
}

bool UPUIngredientBlueprintLibrary::RemovePreparation(FPUIngredientBase& Ingredient, const FPUPreparationBase& Preparation)
{
    return Ingredient.RemovePreparation(Preparation);
}

bool UPUIngredientBlueprintLibrary::HasPreparation(const FPUIngredientBase& Ingredient, const FGameplayTag& PreparationTag)
{
    return Ingredient.HasPreparation(PreparationTag);
}

FText UPUIngredientBlueprintLibrary::GetCurrentDisplayName(const FPUIngredientBase& Ingredient)
{
    return Ingredient.GetCurrentDisplayName();
}

float UPUIngredientBlueprintLibrary::GetPropertyValue(const FPUIngredientBase& Ingredient, const FName& PropertyName)
{
    return Ingredient.GetPropertyValue(PropertyName);
}

void UPUIngredientBlueprintLibrary::SetPropertyValue(FPUIngredientBase& Ingredient, const FName& PropertyName, float Value)
{
    Ingredient.SetPropertyValue(PropertyName, Value);
}

bool UPUIngredientBlueprintLibrary::HasProperty(const FPUIngredientBase& Ingredient, const FName& PropertyName)
{
    return Ingredient.HasProperty(PropertyName);
}

TArray<FIngredientProperty> UPUIngredientBlueprintLibrary::GetPropertiesByTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag)
{
    return Ingredient.GetPropertiesByTag(Tag);
}

bool UPUIngredientBlueprintLibrary::HasPropertiesWithTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag)
{
    return Ingredient.HasPropertiesWithTag(Tag);
}

float UPUIngredientBlueprintLibrary::GetTotalValueForTag(const FPUIngredientBase& Ingredient, const FGameplayTag& Tag)
{
    return Ingredient.GetTotalValueForTag(Tag);
}

TArray<FGameplayTag> UPUIngredientBlueprintLibrary::GetEffectsAtQuantity(const FPUIngredientBase& Ingredient, int32 Quantity)
{
    return Ingredient.GetEffectsAtQuantity(Quantity);
} 