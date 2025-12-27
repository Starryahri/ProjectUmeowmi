#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "../UI/PUCookingStageWidget.h"

FName UPUDishBlueprintLibrary::GetIngredientRowNameFromTag(const FGameplayTag& IngredientTag)
{
    // Remove "Ingredient." prefix, convert to lowercase, and remove all periods
    // Example: "Ingredient.Noodle.Bihon" -> "noodlebihon"
    FString FullTag = IngredientTag.ToString();
    
    // Remove "Ingredient." prefix if present
    if (FullTag.StartsWith(TEXT("Ingredient.")))
    {
        FullTag = FullTag.RightChop(11); // Remove "Ingredient." (11 characters)
    }
    
    // Convert to lowercase
    FullTag = FullTag.ToLower();
    
    // Remove all periods
    FullTag.ReplaceInline(TEXT("."), TEXT(""));
    
    return FName(*FullTag);
}

FIngredientInstance UPUDishBlueprintLibrary::AddIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag, const FGameplayTagContainer& Preparations)
{
    // Validate the dish has an ingredient data table
    if (!Dish.IngredientDataTable.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Dish has no ingredient data table"));
        return FIngredientInstance();
    }
    
    UDataTable* LoadedIngredientDataTable = Dish.IngredientDataTable.LoadSynchronous();
    if (!LoadedIngredientDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Failed to load ingredient data table"));
        return FIngredientInstance();
    }
    
    // Get the ingredient row name from the tag (removes "Ingredient." prefix, converts to lowercase, removes periods)
    FName RowName = GetIngredientRowNameFromTag(IngredientTag);
        
    if (FPUIngredientBase* FoundIngredient = LoadedIngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("AddIngredient")))
    {
        // Create a new ingredient instance
        FIngredientInstance NewInstance;
        // Use GUID-based instance ID generation (same as ingredient buttons)
        NewInstance.InstanceID = UPUCookingStageWidget::GenerateGUIDBasedInstanceID();
        NewInstance.Quantity = 1;
        NewInstance.IngredientData = *FoundIngredient;
        NewInstance.IngredientTag = IngredientTag;
        NewInstance.Preparations = Preparations;
        
        // Apply preparations to the ingredient data
        if (NewInstance.IngredientData.PreparationDataTable.IsValid())
        {
            UDataTable* LoadedPreparationDataTable = NewInstance.IngredientData.PreparationDataTable.LoadSynchronous();
            if (LoadedPreparationDataTable)
            {
                TArray<FGameplayTag> PreparationTags;
                Preparations.GetGameplayTagArray(PreparationTags);
                
                for (const FGameplayTag& PrepTag : PreparationTags)
                {
                    // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                    FString PrepFullTag = PrepTag.ToString();
                    int32 PrepLastPeriodIndex;
                    if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
                    {
                        FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
                        FName PrepRowName = FName(*PrepName);
                        
                        if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("AddIngredient")))
                        {
                            // Apply preparation modifiers
                            Preparation->ApplyModifiers(NewInstance.IngredientData.FlavorAspects, NewInstance.IngredientData.TextureAspects);
                        }
                    }
                }
            }
        }
        
        // Add the instance to the dish
        Dish.IngredientInstances.Add(NewInstance);
        
        UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::AddIngredient - Added ingredient %s with %d preparations"), 
            *IngredientTag.ToString(), Preparations.Num());
        
        return NewInstance;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Ingredient %s not found in data table"), *IngredientTag.ToString());
    return FIngredientInstance();
}

bool UPUDishBlueprintLibrary::RemoveIngredient(FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.IngredientInstances.RemoveAll([&](const FIngredientInstance& Instance) {
        return Instance.IngredientData.IngredientTag == IngredientTag;
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
        if (Instance.IngredientData.IngredientTag == IngredientTag)
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
        if (Instance.IngredientData.IngredientTag == IngredientTag)
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
        if (Instance.IngredientData.IngredientTag == IngredientTag)
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
        if (Instance.IngredientData.IngredientTag == IngredientTag)
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
        if (Dish.IngredientInstances[i].IngredientData.IngredientTag == IngredientTag)
        {
            Indices.Add(i);
        }
    }
    return Indices;
}

TArray<int32> UPUDishBlueprintLibrary::GetInstanceIDsForIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    TArray<int32> IDs;
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            IDs.Add(Instance.InstanceID);
        }
    }
    return IDs;
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
    
    // Check if this preparation is already applied (check both Preparations and ActivePreparations)
    if (Instance.IngredientData.ActivePreparations.HasTag(PreparationTag) || Instance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Preparation %s already applied to instance %d"), 
            *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    // Apply the preparation to both fields to keep them in sync
    Instance.IngredientData.ActivePreparations.AddTag(PreparationTag);
    Instance.Preparations.AddTag(PreparationTag);
    
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Applied %s to instance %d (now has %d preparations)"), 
        *PreparationTag.ToString(), InstanceIndex, Instance.Preparations.Num());
    
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
    
    // Check if this preparation is actually applied (check both Preparations and ActivePreparations)
    if (!Instance.IngredientData.ActivePreparations.HasTag(PreparationTag) && !Instance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Preparation %s not applied to instance %d"), 
            *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    // Remove the preparation from both fields to keep them in sync
    Instance.IngredientData.ActivePreparations.RemoveTag(PreparationTag);
    Instance.Preparations.RemoveTag(PreparationTag);
    
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Removed %s from instance %d (now has %d preparations)"), 
        *PreparationTag.ToString(), InstanceIndex, Instance.Preparations.Num());
    
    return true;
}

bool UPUDishBlueprintLibrary::ApplyPreparationByID(FPUDishBase& Dish, int32 InstanceID, const FGameplayTag& PreparationTag)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparationByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return ApplyPreparation(Dish, InstanceIndex, PreparationTag);
}

bool UPUDishBlueprintLibrary::RemovePreparationByID(FPUDishBase& Dish, int32 InstanceID, const FGameplayTag& PreparationTag)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparationByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemovePreparation(Dish, InstanceIndex, PreparationTag);
}

bool UPUDishBlueprintLibrary::RemoveIngredientInstanceByID(FPUDishBase& Dish, int32 InstanceID)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstanceByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemoveIngredientInstance(Dish, InstanceIndex);
}

bool UPUDishBlueprintLibrary::RemoveIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Quantity)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemoveIngredientQuantity(Dish, InstanceIndex, Quantity);
}

bool UPUDishBlueprintLibrary::IncrementIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Amount)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::IncrementIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    
    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    Instance.Quantity += Amount;
    
    UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::IncrementIngredientQuantityByID - Incremented instance %d quantity by %d (new total: %d)"), 
        InstanceID, Amount, Instance.Quantity);
    
    return true;
}

bool UPUDishBlueprintLibrary::DecrementIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Amount)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    
    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if we have enough quantity to remove
    if (Instance.Quantity < Amount)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Not enough quantity. Current: %d, Requested: %d"), 
            Instance.Quantity, Amount);
        return false;
    }
    
    Instance.Quantity -= Amount;
    
    // If quantity reaches zero, remove the instance
    if (Instance.Quantity <= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Quantity reached zero, removing instance %d"), InstanceID);
        Dish.IngredientInstances.RemoveAt(InstanceIndex);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Decremented instance %d quantity by %d (new total: %d)"), 
            InstanceID, Amount, Instance.Quantity);
    }
    
    return true;
}

float UPUDishBlueprintLibrary::GetTotalFlavorAspect(const FPUDishBase& Dish, const FName& AspectName)
{
    return Dish.GetTotalFlavorAspect(AspectName);
}

float UPUDishBlueprintLibrary::GetTotalTextureAspect(const FPUDishBase& Dish, const FName& AspectName)
{
    return Dish.GetTotalTextureAspect(AspectName);
}

bool UPUDishBlueprintLibrary::HasIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag)
{
    return Dish.HasIngredient(IngredientTag);
}

bool UPUDishBlueprintLibrary::GetDishFromDataTable(UDataTable* DishDataTable, UDataTable* IngredientDataTable, const FGameplayTag& DishTag, FPUDishBase& OutDish)
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
    FName RowName = NAME_None;
    if (FullTag.FindLastChar('.', LastPeriodIndex))
    {
        FString DishName = FullTag.RightChop(LastPeriodIndex + 1).ToLower();
        RowName = FName(*DishName);
        
        UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Looking for dish: %s (RowName: %s)"), 
            *DishTag.ToString(), *RowName.ToString());
        
        if (FPUDishBase* FoundDish = DishDataTable->FindRow<FPUDishBase>(RowName, TEXT("GetDishFromDataTable")))
        {
            OutDish = *FoundDish;
            UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Found dish: %s with %d default ingredients"), 
                *OutDish.DisplayName.ToString(), OutDish.IngredientInstances.Num());
            
            // DEBUG: Print dish data table path and ingredient data table status
            FString DishDataTablePath = DishDataTable->GetPathName();
            UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Dish Data Table Path: %s"), *DishDataTablePath);
            UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Ingredient Data Table Provided: %s"), IngredientDataTable ? TEXT("TRUE") : TEXT("FALSE"));
            
            // Print to screen as well
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Dish Data Table: %s"), *DishDataTablePath));
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Ingredient Data Table Provided: %s"), IngredientDataTable ? TEXT("TRUE") : TEXT("FALSE")));
            }
            
            // Populate IngredientData for each instance using the convenient fields
            for (FIngredientInstance& Instance : OutDish.IngredientInstances)
            {
                if (Instance.IngredientTag.IsValid() && IngredientDataTable)
                {
                    UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Populating ingredient data for tag: %s"), 
                        *Instance.IngredientTag.ToString());
                    
                    // DEBUG: Print ingredient data table status
                    UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Using provided ingredient data table"));
                    
                    // Print to screen as well
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, FString::Printf(TEXT("Using provided ingredient data table")));
                    }
                    
                    UDataTable* LoadedIngredientDataTable = IngredientDataTable;
                    if (LoadedIngredientDataTable)
                    {
                        // Get the ingredient row name from the tag (removes "Ingredient." prefix, converts to lowercase, removes periods)
                        FName IngredientRowName = GetIngredientRowNameFromTag(Instance.IngredientTag);
                            
                        if (FPUIngredientBase* FoundIngredient = LoadedIngredientDataTable->FindRow<FPUIngredientBase>(IngredientRowName, TEXT("GetDishFromDataTable")))
                        {
                            // Copy the base ingredient data
                            Instance.IngredientData = *FoundIngredient;
                            
                            // IMPORTANT: If the ingredient data table row has ActivePreparations set (like Prep.Char in bbqduck),
                            // copy them to Instance.Preparations so they're preserved
                            if (Instance.Preparations.Num() == 0 && Instance.IngredientData.ActivePreparations.Num() > 0)
                            {
                                Instance.Preparations = Instance.IngredientData.ActivePreparations;
                                UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Copied %d ActivePreparations from data table to Instance.Preparations"), 
                                    Instance.IngredientData.ActivePreparations.Num());
                            }
                            
                            // Sync both ways: if Instance.Preparations has values, use those; otherwise use ActivePreparations from data table
                            if (Instance.Preparations.Num() > 0)
                            {
                                Instance.IngredientData.ActivePreparations = Instance.Preparations;
                            }
                            // If Instance.Preparations is empty but ActivePreparations has values, they're already synced above
                            
                            // Apply each preparation's modifiers
                            if (Instance.IngredientData.PreparationDataTable.IsValid())
                            {
                                UDataTable* LoadedPreparationDataTable = Instance.IngredientData.PreparationDataTable.LoadSynchronous();
                                if (LoadedPreparationDataTable)
                                {
                                    TArray<FGameplayTag> PreparationTags;
                                    Instance.Preparations.GetGameplayTagArray(PreparationTags);

                                    for (const FGameplayTag& PrepTag : PreparationTags)
                                    {
                                        // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                                        FString PrepTagName = PrepTag.ToString();
                                        int32 PrepLastPeriodIndex;
                                        if (PrepTagName.FindLastChar('.', PrepLastPeriodIndex))
                                        {
                                            FString PrepName = PrepTagName.RightChop(PrepLastPeriodIndex + 1).ToLower();
                                            FName PrepRowName = FName(*PrepName);
                                            
                                            if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetDishFromDataTable")))
                                            {
                                                // Apply preparation modifiers
                                                Preparation->ApplyModifiers(Instance.IngredientData.FlavorAspects, Instance.IngredientData.TextureAspects);
                                            }
                                        }
                                    }
                                }
                            }
                            
                            UE_LOG(LogTemp, Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Successfully populated ingredient data for: %s"), 
                                *Instance.IngredientData.DisplayName.ToString());
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Failed to find ingredient in data table: %s"), *IngredientRowName.ToString());
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - No ingredient data table provided"));
                        
                        // Print to screen as well
                        if (GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("No ingredient data table provided")));
                        }
                    }
                }
            }
            
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Dish not found in data table: %s (RowName: %s)"), 
                *DishTag.ToString(), *RowName.ToString());
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
        //FGameplayTag::RequestGameplayTag(TEXT("Dish.ChiFan")),
        //FGameplayTag::RequestGameplayTag(TEXT("Dish.HaloHalo"))
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

TArray<FIngredientInstance> UPUDishBlueprintLibrary::GetAllIngredientInstances(const FPUDishBase& Dish)
{
    return Dish.GetAllIngredientInstances();
}

bool UPUDishBlueprintLibrary::GetIngredient(const FPUDishBase& Dish, const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient)
{
    return Dish.GetIngredient(IngredientTag, OutIngredient);
}

int32 UPUDishBlueprintLibrary::GetTotalIngredientQuantity(const FPUDishBase& Dish)
{
    return Dish.GetTotalIngredientQuantity();
}

bool UPUDishBlueprintLibrary::GetIngredientForInstance(const FPUDishBase& Dish, int32 InstanceIndex, FPUIngredientBase& OutIngredient)
{
    return Dish.GetIngredientForInstance(InstanceIndex, OutIngredient);
}

bool UPUDishBlueprintLibrary::GetIngredientForInstanceID(const FPUDishBase& Dish, int32 InstanceID, FPUIngredientBase& OutIngredient)
{
    return Dish.GetIngredientForInstanceID(InstanceID, OutIngredient);
}

// Plating-related functions
bool UPUDishBlueprintLibrary::HasPlatingData(const FPUDishBase& Dish)
{
    return Dish.HasPlatingData();
}

void UPUDishBlueprintLibrary::SetIngredientPlating(FPUDishBase& Dish, int32 InstanceID, const FVector& Position, const FRotator& Rotation, const FVector& Scale)
{
    Dish.SetIngredientPlating(InstanceID, Position, Rotation, Scale);
}

void UPUDishBlueprintLibrary::ClearIngredientPlating(FPUDishBase& Dish, int32 InstanceID)
{
    Dish.ClearIngredientPlating(InstanceID);
}

bool UPUDishBlueprintLibrary::GetIngredientPlating(const FPUDishBase& Dish, int32 InstanceID, FVector& OutPosition, FRotator& OutRotation, FVector& OutScale)
{
    return Dish.GetIngredientPlating(InstanceID, OutPosition, OutRotation, OutScale);
} 