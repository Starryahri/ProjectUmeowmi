#include "PUDishBlueprintLibrary.h"
#include "PUDishBase.h"
#include "PUDishCustomizationComponent.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "PUOrderBase.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "../UI/PUScorecardTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"

// Debug output toggles (kept in code, but disabled by default to avoid startup/on-screen spam).
namespace
{
    // Enables extra UE_LOG lines while loading dishes/ingredients from data tables.
    constexpr bool bPU_LogDishDataTableDebug = false;

    // Enables on-screen debug messages while loading dishes/ingredients from data tables.
    constexpr bool bPU_ShowDishDataTableOnScreenDebug = false;

    // Enables tag-heavy logs (dish/ingredient gameplay tags) which can be noisy during customization startup.
    constexpr bool bPU_LogDishTagSpam = false;

    void ApplyPrepTagModifiers(UDataTable* PrepTable, const FGameplayTag& PrepTag, FPUIngredientBase& IngredientData, const TCHAR* Context)
    {
        if (!PrepTable || !PrepTag.IsValid())
        {
            return;
        }

        FString PrepFullTag = PrepTag.ToString();
        int32 PrepLastPeriodIndex = INDEX_NONE;
        if (!PrepFullTag.FindLastChar(TEXT('.'), PrepLastPeriodIndex))
        {
            return;
        }

        const FName PrepRowName(*PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower());
        if (FPUPreparationBase* Preparation = PrepTable->FindRow<FPUPreparationBase>(PrepRowName, Context))
        {
            Preparation->ApplyModifiers(IngredientData.FlavorAspects, IngredientData.TextureAspects);
        }
    }

    void RemovePrepTagModifiers(UDataTable* PrepTable, const FGameplayTag& PrepTag, FPUIngredientBase& IngredientData, const TCHAR* Context)
    {
        if (!PrepTable || !PrepTag.IsValid())
        {
            return;
        }

        FString PrepFullTag = PrepTag.ToString();
        int32 PrepLastPeriodIndex = INDEX_NONE;
        if (!PrepFullTag.FindLastChar(TEXT('.'), PrepLastPeriodIndex))
        {
            return;
        }

        const FName PrepRowName(*PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower());
        if (FPUPreparationBase* Preparation = PrepTable->FindRow<FPUPreparationBase>(PrepRowName, Context))
        {
            Preparation->RemoveModifiers(IngredientData.FlavorAspects, IngredientData.TextureAspects);
        }
    }
}

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

FIngredientInstance UPUDishBlueprintLibrary::AddIngredient(
    FPUDishBase& Dish,
    const FGameplayTag& IngredientTag,
    const FGameplayTagContainer& Preparations,
    UDataTable* PreparationDataTable)
{
    // Validate the dish has an ingredient data table
    if (!Dish.IngredientDataTable.IsValid())
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Dish has no ingredient data table"));
        return FIngredientInstance();
    }
    
    UDataTable* LoadedIngredientDataTable = Dish.IngredientDataTable.LoadSynchronous();
    if (!LoadedIngredientDataTable)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Failed to load ingredient data table"));
        return FIngredientInstance();
    }
    
    // Get the ingredient row name from the tag (removes "Ingredient." prefix, converts to lowercase, removes periods)
    FName RowName = GetIngredientRowNameFromTag(IngredientTag);
        
    if (FPUIngredientBase* FoundIngredient = LoadedIngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("AddIngredient")))
    {
        // Create a new ingredient instance
        FIngredientInstance NewInstance;
        // Use GUID-based instance ID generation (same as ingredient buttons)
        NewInstance.InstanceID = UPUDishCustomizationWidget::GenerateGUIDBasedInstanceID();
        NewInstance.Quantity = 1;
        NewInstance.IngredientData = *FoundIngredient;
        NewInstance.IngredientTag = IngredientTag;
        NewInstance.Preparations = Preparations;
        
        if (PreparationDataTable)
        {
            TArray<FGameplayTag> PreparationTags;
            Preparations.GetGameplayTagArray(PreparationTags);
            for (const FGameplayTag& PrepTag : PreparationTags)
            {
                ApplyPrepTagModifiers(PreparationDataTable, PrepTag, NewInstance.IngredientData, TEXT("AddIngredient"));
            }
        }
        
        // Add the instance to the dish
        Dish.IngredientInstances.Add(NewInstance);
        
        if (bPU_LogDishTagSpam)
        {
            //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::AddIngredient - Added ingredient %s with %d preparations"),
            //    *IngredientTag.ToString(), Preparations.Num());
        }
        
        return NewInstance;
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::AddIngredient - Ingredient %s not found in data table"), *IngredientTag.ToString());
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
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstance - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    // Remove the instance
    Dish.IngredientInstances.RemoveAt(InstanceIndex);
    //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstance - Removed instance at index %d"), InstanceIndex);
    
    return true;
}

bool UPUDishBlueprintLibrary::RemoveIngredientQuantity(FPUDishBase& Dish, int32 InstanceIndex, int32 Quantity)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if we have enough quantity to remove
    if (Instance.Quantity < Quantity)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Not enough quantity. Have: %d, Requested: %d"), 
        //    Instance.Quantity, Quantity);
        return false;
    }

    // Remove the quantity
    Instance.Quantity -= Quantity;
    //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Removed %d from instance %d. Remaining: %d"), 
    //    Quantity, InstanceIndex, Instance.Quantity);

    // Auto-cleanup: Remove instance if quantity reaches 0
    if (Instance.Quantity <= 0)
    {
        Dish.IngredientInstances.RemoveAt(InstanceIndex);
        //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantity - Auto-removed empty instance at index %d"), InstanceIndex);
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
                    //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::IncrementIngredientAmount - Added %d to instance %d. New quantity: %d"), 
                    //    Amount, i, Instance.Quantity);
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
                    //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientAmount - Removed %d from instance %d. New quantity: %d"), 
                    //    Amount, i, Instance.Quantity);

                    // Auto-cleanup: Remove instance if quantity reaches 0
                    if (Instance.Quantity <= 0)
                    {
                        Dish.IngredientInstances.RemoveAt(i);
                        //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientAmount - Auto-removed empty instance at index %d"), i);
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

bool UPUDishBlueprintLibrary::ApplyPreparation(
    FPUDishBase& Dish,
    int32 InstanceIndex,
    const FGameplayTag& PreparationTag,
    UDataTable* PreparationDataTable)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if this preparation is already applied (check both Preparations and ActivePreparations)
    if (Instance.IngredientData.ActivePreparations.HasTag(PreparationTag) || Instance.Preparations.HasTag(PreparationTag))
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparation - Preparation %s already applied to instance %d"), 
        //    *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    // Apply the preparation to both fields to keep them in sync
    Instance.IngredientData.ActivePreparations.AddTag(PreparationTag);
    Instance.Preparations.AddTag(PreparationTag);
    
    if (PreparationDataTable)
    {
        ApplyPrepTagModifiers(PreparationDataTable, PreparationTag, Instance.IngredientData, TEXT("ApplyPreparation"));
    }
    
    return true;
}

bool UPUDishBlueprintLibrary::RemovePreparation(
    FPUDishBase& Dish,
    int32 InstanceIndex,
    const FGameplayTag& PreparationTag,
    UDataTable* PreparationDataTable)
{
    // Check if the instance index is valid
    if (!Dish.IngredientInstances.IsValidIndex(InstanceIndex))
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Invalid instance index: %d"), InstanceIndex);
        return false;
    }

    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if this preparation is actually applied (check both Preparations and ActivePreparations)
    if (!Instance.IngredientData.ActivePreparations.HasTag(PreparationTag) && !Instance.Preparations.HasTag(PreparationTag))
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparation - Preparation %s not applied to instance %d"), 
        //    *PreparationTag.ToString(), InstanceIndex);
        return false;
    }

    if (PreparationDataTable)
    {
        RemovePrepTagModifiers(PreparationDataTable, PreparationTag, Instance.IngredientData, TEXT("RemovePreparation"));
    }

    // Remove the preparation from both fields to keep them in sync
    Instance.IngredientData.ActivePreparations.RemoveTag(PreparationTag);
    Instance.Preparations.RemoveTag(PreparationTag);
    
    return true;
}

bool UPUDishBlueprintLibrary::ApplyPreparationByID(
    FPUDishBase& Dish,
    int32 InstanceID,
    const FGameplayTag& PreparationTag,
    UDataTable* PreparationDataTable)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::ApplyPreparationByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return ApplyPreparation(Dish, InstanceIndex, PreparationTag, PreparationDataTable);
}

bool UPUDishBlueprintLibrary::RemovePreparationByID(
    FPUDishBase& Dish,
    int32 InstanceID,
    const FGameplayTag& PreparationTag,
    UDataTable* PreparationDataTable)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemovePreparationByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemovePreparation(Dish, InstanceIndex, PreparationTag, PreparationDataTable);
}

bool UPUDishBlueprintLibrary::RemoveIngredientInstanceByID(FPUDishBase& Dish, int32 InstanceID)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientInstanceByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemoveIngredientInstance(Dish, InstanceIndex);
}

bool UPUDishBlueprintLibrary::RemoveIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Quantity)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::RemoveIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    return RemoveIngredientQuantity(Dish, InstanceIndex, Quantity);
}

bool UPUDishBlueprintLibrary::IncrementIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Amount)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::IncrementIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    
    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    Instance.Quantity += Amount;
    
    //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::IncrementIngredientQuantityByID - Incremented instance %d quantity by %d (new total: %d)"), 
    //    InstanceID, Amount, Instance.Quantity);
    
    return true;
}

bool UPUDishBlueprintLibrary::DecrementIngredientQuantityByID(FPUDishBase& Dish, int32 InstanceID, int32 Amount)
{
    int32 InstanceIndex = Dish.FindInstanceIndexByID(InstanceID);
    if (InstanceIndex == INDEX_NONE)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Instance ID %d not found"), InstanceID);
        return false;
    }
    
    FIngredientInstance& Instance = Dish.IngredientInstances[InstanceIndex];
    
    // Check if we have enough quantity to remove
    if (Instance.Quantity < Amount)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Not enough quantity. Current: %d, Requested: %d"), 
        //    Instance.Quantity, Amount);
        return false;
    }
    
    Instance.Quantity -= Amount;
    
    // If quantity reaches zero, remove the instance
    if (Instance.Quantity <= 0)
    {
        //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Quantity reached zero, removing instance %d"), InstanceID);
        Dish.IngredientInstances.RemoveAt(InstanceIndex);
    }
    else
    {
        //UE_LOG(LogTemp,Log, TEXT("UPUDishBlueprintLibrary::DecrementIngredientQuantityByID - Decremented instance %d quantity by %d (new total: %d)"), 
        //    InstanceID, Amount, Instance.Quantity);
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

bool UPUDishBlueprintLibrary::GetDishFromDataTable(
    UDataTable* DishDataTable,
    UDataTable* IngredientDataTable,
    const FGameplayTag& DishTag,
    FPUDishBase& OutDish,
    UDataTable* PreparationDataTable)
{
    if (!DishDataTable)
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - DishDataTable is null"));
        return false;
    }

    if (!DishTag.IsValid())
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - DishTag is invalid"));
        return false;
    }

    // Get the dish row name from the tag: strip "Dish." prefix, lowercase, remove all periods.
    // Example: "Dish.Cookies.EggYolk" -> "cookieseggyolk" (same convention as GetIngredientRowNameFromTag)
    FString FullTag = DishTag.ToString();
    FName RowName = NAME_None;
    if (FullTag.StartsWith(TEXT("Dish.")))
    {
        FullTag = FullTag.RightChop(5); // Remove "Dish." (5 characters)
    }
    FullTag = FullTag.ToLower();
    FullTag.ReplaceInline(TEXT("."), TEXT(""));
    if (!FullTag.IsEmpty())
    {
        RowName = FName(*FullTag);
    }
    if (RowName != NAME_None && bPU_LogDishTagSpam)
    {
        //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Looking for dish: %s (RowName: %s)"),
        //    *DishTag.ToString(), *RowName.ToString());
    }

    if (RowName != NAME_None)
    {
        if (FPUDishBase* FoundDish = DishDataTable->FindRow<FPUDishBase>(RowName, TEXT("GetDishFromDataTable")))
        {
            OutDish = *FoundDish;
            if (bPU_LogDishDataTableDebug)
            {
                //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Found dish: %s with %d default ingredients"),
                //    *OutDish.DisplayName.ToString(), OutDish.IngredientInstances.Num());
            }
            
            // DEBUG: Print dish data table path and ingredient data table status
            // (Disabled globally to avoid log/on-screen debug spam. Re-enable locally as needed.)
            /*
            if (bPU_LogDishDataTableDebug || bPU_ShowDishDataTableOnScreenDebug)
            {
                const FString DishDataTablePath = DishDataTable->GetPathName();

                if (bPU_LogDishDataTableDebug)
                {
                    //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Dish Data Table Path: %s"), *DishDataTablePath);
                    //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Ingredient Data Table Provided: %s"), IngredientDataTable ? TEXT("TRUE") : TEXT("FALSE"));
                }

                // Print to screen as well
                if (bPU_ShowDishDataTableOnScreenDebug && GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Dish Data Table: %s"), *DishDataTablePath));
                    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Ingredient Data Table Provided: %s"), IngredientDataTable ? TEXT("TRUE") : TEXT("FALSE")));
                }
            }
            */
            
            // Populate IngredientData for each instance using the convenient fields
            for (FIngredientInstance& Instance : OutDish.IngredientInstances)
            {
                if (Instance.IngredientTag.IsValid() && IngredientDataTable)
                {
                    if (bPU_LogDishTagSpam)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Populating ingredient data for tag: %s"),
                        //    *Instance.IngredientTag.ToString());
                    }
                    
                    // DEBUG: Print ingredient data table status
                    // DEBUG: Ingredient data table logging/on-screen messages
                    // (Disabled globally to avoid log/on-screen debug spam. Re-enable locally as needed.)
                    /*
                    if (bPU_LogDishDataTableDebug)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Using provided ingredient data table"));
                    }
                    
                    // Print to screen as well
                    if (bPU_ShowDishDataTableOnScreenDebug && GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, FString::Printf(TEXT("Using provided ingredient data table")));
                    }
                    */
                    
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
                                if (bPU_LogDishDataTableDebug)
                                {
                                    //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Copied %d ActivePreparations from data table to Instance.Preparations"),
                                    //    Instance.IngredientData.ActivePreparations.Num());
                                }
                            }
                            
                            // Sync both ways: if Instance.Preparations has values, use those; otherwise use ActivePreparations from data table
                            if (Instance.Preparations.Num() > 0)
                            {
                                Instance.IngredientData.ActivePreparations = Instance.Preparations;
                            }
                            // If Instance.Preparations is empty but ActivePreparations has values, they're already synced above
                            
                            if (PreparationDataTable)
                            {
                                TArray<FGameplayTag> PreparationTags;
                                Instance.Preparations.GetGameplayTagArray(PreparationTags);
                                for (const FGameplayTag& PrepTag : PreparationTags)
                                {
                                    ApplyPrepTagModifiers(PreparationDataTable, PrepTag, Instance.IngredientData, TEXT("GetDishFromDataTable"));
                                }
                            }
                            
                            if (bPU_LogDishDataTableDebug)
                            {
                                //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Successfully populated ingredient data for: %s"),
                                //    *Instance.IngredientData.DisplayName.ToString());
                            }
                        }
                        else
                        {
                            //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Failed to find ingredient in data table: %s"), *IngredientRowName.ToString());
                        }
                    }
                    else
                    {
                        // DEBUG: Missing ingredient data table logging/on-screen messages
                        // (Disabled globally to avoid log/on-screen debug spam. Re-enable locally as needed.)
                        /*
                        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - No ingredient data table provided"));
                        
                        // Print to screen as well
                        if (bPU_ShowDishDataTableOnScreenDebug && GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("No ingredient data table provided")));
                        }
                        */
                    }
                }
            }
            
            return true;
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Dish not found in data table: %s (RowName: %s)"),
            //    *DishTag.ToString(), *RowName.ToString());
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetDishFromDataTable - Invalid or empty dish tag (expected e.g. Dish.Cookies.EggYolk -> row cookieseggyolk): %s"), *DishTag.ToString());
    }

    return false;
}

FGameplayTag UPUDishBlueprintLibrary::GetRandomDishTag()
{
    // Define available dish tags
    TArray<FGameplayTag> AvailableDishTags = {
        FGameplayTag::RequestGameplayTag(TEXT("Dish.Congee")),
        //FGameplayTag::RequestGameplayTag(TEXT("Dish.GourmetToast")),
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
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishBlueprintLibrary::GetRandomDishTag - No valid dish tags found"));
        return FGameplayTag::EmptyTag;
    }
    
    // Return a random dish tag
    FGameplayTag RandomTag = ValidDishTags[FMath::RandRange(0, ValidDishTags.Num() - 1)];
    if (bPU_LogDishTagSpam)
    {
        //UE_LOG(LogTemp,Display, TEXT("UPUDishBlueprintLibrary::GetRandomDishTag - Selected dish: %s"), *RandomTag.ToString());
    }
    
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

UTexture2D* UPUDishBlueprintLibrary::GetLoadedJournalTexture(const FPUDishBase& Dish)
{
    if (Dish.JournalTexture.IsNull()) return nullptr;
    return Dish.JournalTexture.LoadSynchronous();
}

UTexture2D* UPUDishBlueprintLibrary::GetLoadedPreviewTexture(const FPUDishBase& Dish)
{
    if (Dish.PreviewTexture.IsNull()) return nullptr;
    return Dish.PreviewTexture.LoadSynchronous();
}

bool UPUDishBlueprintLibrary::IsDishSuspicious(const FPUDishBase& CompletedDish, const FPUDishBase& BaseRecipeDish)
{
    for (const FIngredientInstance& BaseInstance : BaseRecipeDish.IngredientInstances)
    {
        FGameplayTag BaseTag = BaseInstance.IngredientTag.IsValid() ? BaseInstance.IngredientTag : BaseInstance.IngredientData.IngredientTag;
        if (!BaseTag.IsValid()) continue;
        if (!HasIngredient(CompletedDish, BaseTag))
        {
            return true; // Missing a base ingredient -> suspicious
        }
    }
    return false;
}

FText UPUDishBlueprintLibrary::GetEndingStageText(const FPUDishBase& CompletedDish, const FPUDishBase& BaseRecipeDish)
{
    const FString DishName = BaseRecipeDish.DisplayName.ToString();
    const bool bSuspicious = IsDishSuspicious(CompletedDish, BaseRecipeDish);
    if (bSuspicious)
    {
        return FText::FromString(FString::Printf(TEXT("You created Suspicious %s"), *DishName));
    }
    return FText::FromString(FString::Printf(TEXT("You created %s"), *DishName));
}

namespace
{
    const TArray<FName> FlavorAspectNames = {
        TEXT("Umami"), TEXT("Salt"), TEXT("Sweet"), TEXT("Sour"), TEXT("Bitter"), TEXT("Spicy")
    };
    const TArray<FName> TextureAspectNames = {
        TEXT("Rich"), TEXT("Juicy"), TEXT("Tender"), TEXT("Chewy"), TEXT("Crispy"), TEXT("Crumbly")
    };

    /** Map performance ratio (PlayerValue/TargetValue) to 0-5 stars using same bands as scoring. */
    int32 GetStarsFromRatio(float Ratio)
    {
        if (Ratio >= 0.80f && Ratio <= 1.20f) return 5;  // Perfect
        if (Ratio >= 0.70f && Ratio <= 1.30f) return 4;  // Great
        if (Ratio >= 0.60f && Ratio <= 1.40f) return 3;  // Okay
        return 2;  // Needs Improvement
    }

    FPUAspectRanking BuildAspectRanking(const FPUDishBase& Dish, const FName& AspectName, bool bFlavor)
    {
        FPUAspectRanking Ranking;
        Ranking.AspectName = AspectName;

        // Sum contribution per ingredient (group by IngredientTag), store entry for display
        TMap<FGameplayTag, float> ContributionByIngredient;
        TMap<FGameplayTag, FPUBaseIngredientEntry> EntryByIngredient;

        for (const FIngredientInstance& Instance : Dish.IngredientInstances)
        {
            FGameplayTag Tag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
            if (!Tag.IsValid()) continue;

            float Contribution = (bFlavor ? Instance.IngredientData.GetFlavorAspect(AspectName) : Instance.IngredientData.GetTextureAspect(AspectName))
                * static_cast<float>(Instance.Quantity);

            float* Existing = ContributionByIngredient.Find(Tag);
            if (Existing)
            {
                *Existing += Contribution;
            }
            else
            {
                ContributionByIngredient.Add(Tag, Contribution);
                FPUBaseIngredientEntry Entry;
                Entry.DisplayName = Instance.IngredientData.DisplayName;
                Entry.PreviewTexture = Instance.IngredientData.PantryTexture ? Instance.IngredientData.PantryTexture : Instance.IngredientData.PreviewTexture;
                EntryByIngredient.Add(Tag, Entry);
            }
        }

        // Sort by contribution descending, take top 3
        TArray<TPair<FGameplayTag, float>> Sorted;
        for (const auto& Pair : ContributionByIngredient)
        {
            if (Pair.Value > 0.0f)
            {
                Sorted.Add(TPair<FGameplayTag, float>(Pair.Key, Pair.Value));
            }
        }
        Sorted.Sort([](const TPair<FGameplayTag, float>& A, const TPair<FGameplayTag, float>& B) { return A.Value > B.Value; });

        for (int32 i = 0; i < FMath::Min(3, Sorted.Num()); ++i)
        {
            if (FPUBaseIngredientEntry* Entry = EntryByIngredient.Find(Sorted[i].Key))
            {
                Ranking.TopContributingIngredients.Add(*Entry);
            }
        }

        Ranking.TotalValue = bFlavor ? Dish.GetTotalFlavorAspect(AspectName) : Dish.GetTotalTextureAspect(AspectName);
        // Integer star rating 0-5: clamp and round (aspect values 0-5)
        Ranking.StarRating = FMath::Clamp(FMath::RoundToInt(Ranking.TotalValue), 0, 5);

        return Ranking;
    }

    FPUAspectProfileData BuildAspectProfile(const FPUDishBase& Dish, const TArray<FName>& AspectNames, bool bFlavor)
    {
        FPUAspectProfileData Profile;

        TArray<TPair<FName, float>> AspectTotals;
        for (const FName& Name : AspectNames)
        {
            float Total = bFlavor ? Dish.GetTotalFlavorAspect(Name) : Dish.GetTotalTextureAspect(Name);
            AspectTotals.Add(TPair<FName, float>(Name, Total));
        }
        AspectTotals.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B) { return A.Value > B.Value; });

        float SumForStars = 0.0f;
        int32 CountForStars = 0;
        for (int32 i = 0; i < FMath::Min(2, AspectTotals.Num()); ++i)
        {
            if (AspectTotals[i].Value > 0.0f)
            {
                Profile.TopAspects.Add(BuildAspectRanking(Dish, AspectTotals[i].Key, bFlavor));
                SumForStars += AspectTotals[i].Value;
                CountForStars++;
            }
        }
        // When dish has no ingredients, always show at least one aspect: "None", no icon, zero stars
        if (Profile.TopAspects.Num() == 0)
        {
            FPUAspectRanking NoneRanking;
            NoneRanking.AspectName = FName(TEXT("None"));
            NoneRanking.StarRating = 0;
            Profile.TopAspects.Add(NoneRanking);
        }
        // Overall star rating: average of top 2 aspect totals (0-5 scale), rounded to integer
        if (CountForStars > 0)
        {
            Profile.StarRating = FMath::Clamp(FMath::RoundToInt(SumForStars / static_cast<float>(CountForStars)), 0, 5);
        }

        return Profile;
    }

    /** Build profile from order's TargetAspects with performance-based stars (Option 5). Falls back to top 2 by value if no order aspects. */
    FPUAspectProfileData BuildAspectProfileFromOrder(const FPUOrderBase& Order, const FPUDishBase& CompletedDish, bool bFlavor)
    {
        const EOrderAspectType RequiredType = bFlavor ? EOrderAspectType::Flavor : EOrderAspectType::Texture;
        const TArray<FName>& AspectNames = bFlavor ? FlavorAspectNames : TextureAspectNames;

        TArray<FOrderAspectRequirement> OrderAspects;
        for (const FOrderAspectRequirement& Req : Order.TargetAspects)
        {
            if (Req.AspectType == RequiredType)
            {
                OrderAspects.Add(Req);
            }
        }

        if (OrderAspects.Num() == 0)
        {
            return BuildAspectProfile(CompletedDish, AspectNames, bFlavor);
        }

        FPUAspectProfileData Profile;
        int32 StarSum = 0;

        for (const FOrderAspectRequirement& Req : OrderAspects)
        {
            FPUAspectRanking Ranking = BuildAspectRanking(CompletedDish, Req.GetAspectName(), bFlavor);

            float PlayerValue = Ranking.TotalValue;
            float TargetValue = FMath::Max(0.01f, Req.TargetValue);
            float Ratio = PlayerValue / TargetValue;
            Ranking.StarRating = GetStarsFromRatio(Ratio);

            Profile.TopAspects.Add(Ranking);
            StarSum += Ranking.StarRating;
        }

        if (Profile.TopAspects.Num() > 0)
        {
            Profile.StarRating = FMath::Clamp(FMath::RoundToInt(static_cast<float>(StarSum) / Profile.TopAspects.Num()), 0, 5);
        }

        return Profile;
    }
}

FPUScorecardData UPUDishBlueprintLibrary::GetScorecardData(const FPUOrderBase& Order)
{
    FPUScorecardData Data;

    // Display name: dish name from completed dish
    Data.DisplayName = GetCurrentDisplayName(Order.GetCompletedDish());

    // Seal tier: 4 grades (Perfect A, Great B, Okay C, NeedsImprovement F)
    // Score mapping: 1.0=Perfect, 0.75=Great, 0.5=Okay, 0.25=NeedsImprovement
    const float Score = Order.GetFinalSatisfactionScore();
    if (Score >= 0.875f)
    {
        Data.SealTier = EPUScorecardSealTier::Perfect;
    }
    else if (Score >= 0.625f)
    {
        Data.SealTier = EPUScorecardSealTier::Great;
    }
    else if (Score >= 0.375f)
    {
        Data.SealTier = EPUScorecardSealTier::Okay;
    }
    else
    {
        Data.SealTier = EPUScorecardSealTier::NeedsImprovement;
    }

    // Base ingredients: prefer BaseDish (recipe), fallback to CompletedDish when recipe has none
    const FPUDishBase& BaseDish = Order.BaseDish;
    const FPUDishBase& CompletedDish = Order.GetCompletedDish();
    const bool bUsingRecipe = BaseDish.IngredientInstances.Num() > 0;
    const TArray<FIngredientInstance>& IngredientSource = bUsingRecipe
        ? BaseDish.IngredientInstances
        : CompletedDish.IngredientInstances;
    TSet<FGameplayTag> SeenTags;
    for (const FIngredientInstance& Instance : IngredientSource)
    {
        FGameplayTag Tag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (Tag.IsValid() && !SeenTags.Contains(Tag))
        {
            SeenTags.Add(Tag);
            FPUBaseIngredientEntry Entry;
            Entry.DisplayName = Instance.IngredientData.DisplayName;
            Entry.PreviewTexture = Instance.IngredientData.PantryTexture ? Instance.IngredientData.PantryTexture : Instance.IngredientData.PreviewTexture;
            Entry.bObtained = bUsingRecipe ? HasIngredient(CompletedDish, Tag) : true;
            Data.BaseIngredients.Add(Entry);
        }
    }

    // Flavor and texture profiles: order's TargetAspects with performance-based stars (Option 5)
    Data.FlavorProfile = BuildAspectProfileFromOrder(Order, CompletedDish, true);
    Data.TextureProfile = BuildAspectProfileFromOrder(Order, CompletedDish, false);

    UE_LOG(LogTemp, Display, TEXT("[Scorecard] GetScorecardData: Score=%.2f, SealTier=%d, BaseIngredients=%d, FlavorTopAspects=%d, TextureTopAspects=%d"),
        Score, (int32)Data.SealTier, Data.BaseIngredients.Num(), Data.FlavorProfile.TopAspects.Num(), Data.TextureProfile.TopAspects.Num());
    return Data;
}

bool UPUDishBlueprintLibrary::DishHasCustomizationPipeline(const FPUDishBase& Dish)
{
    return Dish.HasCustomizationPipeline();
}

TArray<FGameplayTag> UPUDishBlueprintLibrary::GetCustomizationPipelineStageIds(const FPUDishBase& Dish)
{
    TArray<FGameplayTag> OutIds;
    OutIds.Reserve(Dish.CustomizationStages.Num());
    for (const FPUDishCustomizationStageDescriptor& Row : Dish.CustomizationStages)
    {
        if (Row.StageId.IsValid())
        {
            OutIds.Add(Row.StageId);
        }
    }
    return OutIds;
}

int32 UPUDishBlueprintLibrary::GetCustomizationPipelineStageCount(const FPUDishBase& Dish)
{
    return Dish.CustomizationStages.Num();
}

bool UPUDishBlueprintLibrary::TryGetCustomizationPipelineStage(const FPUDishBase& Dish, int32 StageIndex, FPUDishCustomizationStageDescriptor& OutStage)
{
    if (!Dish.CustomizationStages.IsValidIndex(StageIndex))
    {
        return false;
    }
    OutStage = Dish.CustomizationStages[StageIndex];
    return true;
}

int32 UPUDishBlueprintLibrary::FindCustomizationPipelineStageIndex(const FPUDishBase& Dish, const FGameplayTag StageId)
{
    if (!StageId.IsValid())
    {
        return INDEX_NONE;
    }
    return Dish.CustomizationStages.IndexOfByPredicate([&StageId](const FPUDishCustomizationStageDescriptor& Row) {
        return Row.StageId == StageId;
    });
}

namespace
{
static FText ResolveCustomizationStageDisplayTitle(const FPUDishCustomizationStageDescriptor& Stage)
{
    if (!Stage.StageDisplayName.IsEmpty())
    {
        return Stage.StageDisplayName;
    }
    if (Stage.StageId.IsValid())
    {
        return FText::FromName(Stage.StageId.GetTagName());
    }
    return FText::GetEmpty();
}
} // namespace

FText UPUDishBlueprintLibrary::GetCustomizationPipelineStageDisplayName(const FPUDishBase& Dish, const int32 StageIndex)
{
    FPUDishCustomizationStageDescriptor Stage;
    if (!TryGetCustomizationPipelineStage(Dish, StageIndex, Stage))
    {
        return FText::GetEmpty();
    }
    return ResolveCustomizationStageDisplayTitle(Stage);
}

TArray<FText> UPUDishBlueprintLibrary::GetCustomizationPipelineStageDisplayNames(const FPUDishBase& Dish)
{
    TArray<FText> Names;
    Names.Reserve(Dish.CustomizationStages.Num());
    for (const FPUDishCustomizationStageDescriptor& Row : Dish.CustomizationStages)
    {
        Names.Add(ResolveCustomizationStageDisplayTitle(Row));
    }
    return Names;
}

int32 UPUDishBlueprintLibrary::GetCurrentCustomizationStageIndex(const UPUDishCustomizationComponent* CustomizationComponent)
{
    return IsValid(CustomizationComponent)
        ? CustomizationComponent->GetActiveCustomizationPipelineIndex()
        : INDEX_NONE;
}

bool UPUDishBlueprintLibrary::TryGetTriptychRowForStage(
    UDataTable* TriptychDataTable,
    const FPUDishCustomizationStageDescriptor& Stage,
    FPUDishCustomizationTriptychRow& OutTriptychRow)
{
    if (!TriptychDataTable)
    {
        return false;
    }

    static const FString Context(TEXT("TryGetTriptychRowForStage"));

    if (!Stage.TriptychRowName.IsNone())
    {
        if (const FPUDishCustomizationTriptychRow* Row =
                TriptychDataTable->FindRow<FPUDishCustomizationTriptychRow>(Stage.TriptychRowName, Context, false))
        {
            OutTriptychRow = *Row;
            return true;
        }
    }

    if (Stage.StageId.IsValid())
    {
        const FName StageRowName = Stage.StageId.GetTagName();
        if (const FPUDishCustomizationTriptychRow* Row =
                TriptychDataTable->FindRow<FPUDishCustomizationTriptychRow>(StageRowName, Context, false))
        {
            OutTriptychRow = *Row;
            return true;
        }

        for (const FName& RowName : TriptychDataTable->GetRowNames())
        {
            const FPUDishCustomizationTriptychRow* Row =
                TriptychDataTable->FindRow<FPUDishCustomizationTriptychRow>(RowName, Context, false);
            if (Row && Row->StageId == Stage.StageId)
            {
                OutTriptychRow = *Row;
                return true;
            }
        }
    }

    return false;
}

bool UPUDishBlueprintLibrary::StageHasTriptychData(
    UDataTable* TriptychDataTable,
    const FPUDishCustomizationStageDescriptor& Stage)
{
    FPUDishCustomizationTriptychRow UnusedRow;
    return TryGetTriptychRowForStage(TriptychDataTable, Stage, UnusedRow);
} 