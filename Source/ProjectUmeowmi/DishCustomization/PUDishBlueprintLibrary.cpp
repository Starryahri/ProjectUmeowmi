#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"

bool UPUDishBlueprintLibrary::AddIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTagContainer& Preparations)
{
    // Check if we already have an instance with this exact ingredient tag and preparations
    for (FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag && Instance.Preparations == Preparations)
        {
            // If we already have this exact combination, increment its quantity
            Instance.Quantity++;
            return true;
        }
    }

    // If we don't have this combination, create a new instance
    FIngredientInstance NewInstance;
    NewInstance.IngredientTag = IngredientTag;
    NewInstance.Quantity = 1;
    NewInstance.Preparations = Preparations;
    Dish.IngredientInstances.Add(NewInstance);
    return true;
}

bool UPUDishBlueprintLibrary::RemoveIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.IngredientInstances.RemoveAll([&](const FIngredientInstance& Instance) {
        return Instance.IngredientTag == IngredientTag;
    }) > 0;
}

bool UPUDishBlueprintLibrary::RemoveIngredientInstance(FPUDishBase& Dish, int32 InstanceIndex)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstance - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    // Remove the instance
    Dish.IngredientInstances.RemoveAt(InstanceIndex);
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstance - Removed instance at index %d"), InstanceIndex);
    
    return true;
}

bool UPUDishBlueprintLibrary::RemoveIngredientQuantity(FPUDishBase& Dish, int32 InstanceIndex, int32 Quantity)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if we have enough quantity to remove
    if (Instance.Quantity < Quantity)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Not enough quantity. Have: %d, Requested: %d"), 
            Instance.Quantity, Quantity);
        return false;
    }

    // Remove the quantity
    Instance.Quantity -= Quantity;
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Removed %d from instance %d. Remaining: %d"), 
        Quantity, InstanceIndex, Instance.Quantity);

    // Auto-cleanup: Remove instance if quantity reaches 0
    if (Instance.Quantity <= 0)
    {
        Dish.IngredientInstances.RemoveAt(InstanceIndex);
        UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Auto-removed empty instance at index %d"), InstanceIndex);
    }
    
    return true;
}

bool UPUDishBlueprintLibrary::IncrementIngredientAmount(FPUDishBase& Dish, const FGameplayTag& IngredientTag, int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    // Find the first instance of this ingredient (preferably one without preparations for simplicity)
    for (int32 i = 0; i < Dish.IngredientInstances.Num(); ++i)
    {
        FIngredientInstance& Instance = Dish.IngredientInstances[i];
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
                    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::IncrementIngredientAmount - Added %d to instance %d. New quantity: %d"), 
                        Amount, i, Instance.Quantity);
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

    // Find the first instance of this ingredient
    for (int32 i = 0; i < Dish.IngredientInstances.Num(); ++i)
    {
        FIngredientInstance& Instance = Dish.IngredientInstances[i];
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
                    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientAmount - Removed %d from instance %d. New quantity: %d"), 
                        Amount, i, Instance.Quantity);

                    // Auto-cleanup: Remove instance if quantity reaches 0
                    if (Instance.Quantity <= 0)
                    {
                        Dish.IngredientInstances.RemoveAt(i);
                        UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientAmount - Auto-removed empty instance at index %d"), i);
                    }
                    
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
    int32 TotalQuantity = 0;
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            TotalQuantity += Instance.Quantity;
        }
    }
    return TotalQuantity;
}

int32 UPUDishBlueprintLibrary::GetIngredientInstanceCount(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    int32 Count = 0;
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientTag == IngredientTag)
        {
            Count++;
        }
    }
    return Count;
}

TArray<int32> UPUDishBlueprintLibrary::GetInstanceIndicesForIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    TArray<int32> Indices;
    for (int32 i = 0; i < Dish.IngredientInstances.Num(); ++i)
    {
        if (Dish.IngredientInstances[i].IngredientTag == IngredientTag)
        {
            Indices.Add(i);
        }
    }
    return Indices;
}

bool UPUDishBlueprintLibrary::ApplyPreparation(FPUDishBase& Dish, int32 InstanceIndex, const FGameplayTag& PreparationTag)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if this preparation is already applied
    if (Instance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Preparation %s already applied to instance %d"), 
            *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    // Apply the preparation
    Instance.Preparations.AddTag(PreparationTag);
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Applied %s to instance %d"), 
        *PreparationTag.ToString(), InstanceIndex);
    
    return true;
}

bool UPUDishBlueprintLibrary::RemovePreparation(FPUDishBase& Dish, int32 InstanceIndex, const FGameplayTag& PreparationTag)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if this preparation is actually applied
    if (!Instance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Preparation %s not applied to instance %d"), 
            *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    // Remove the preparation
    Instance.Preparations.RemoveTag(PreparationTag);
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Removed %s from instance %d"), 
        *PreparationTag.ToString(), InstanceIndex);
    
    return true;
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

int32 UPUDishBlueprintLibrary::GetTotalIngredientQuantity(const FPUDishBase& Dish)
{
    return Dish.GetTotalIngredientQuantity();
} 