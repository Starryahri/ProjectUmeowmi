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

float UPUIngredientBlueprintLibrary::GetFlavorAspect(const FPUIngredientBase& Ingredient, const FName& AspectName)
{
    return Ingredient.GetFlavorAspect(AspectName);
}

float UPUIngredientBlueprintLibrary::GetTextureAspect(const FPUIngredientBase& Ingredient, const FName& AspectName)
{
    return Ingredient.GetTextureAspect(AspectName);
}

void UPUIngredientBlueprintLibrary::SetFlavorAspect(FPUIngredientBase& Ingredient, const FName& AspectName, float Value)
{
    Ingredient.SetFlavorAspect(AspectName, Value);
}

void UPUIngredientBlueprintLibrary::SetTextureAspect(FPUIngredientBase& Ingredient, const FName& AspectName, float Value)
{
    Ingredient.SetTextureAspect(AspectName, Value);
}

float UPUIngredientBlueprintLibrary::GetTotalFlavorValue(const FPUIngredientBase& Ingredient)
{
    return Ingredient.GetTotalFlavorValue();
}

float UPUIngredientBlueprintLibrary::GetTotalTextureValue(const FPUIngredientBase& Ingredient)
{
    return Ingredient.GetTotalTextureValue();
}

TArray<FGameplayTag> UPUIngredientBlueprintLibrary::GetEffectsAtQuantity(const FPUIngredientBase& Ingredient, int32 Quantity)
{
    return Ingredient.GetEffectsAtQuantity(Quantity);
} 