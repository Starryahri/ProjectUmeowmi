#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUPreparationBase.h"
#include "PUDishBlueprintLibrary.h"
#include "Engine/DataTable.h"

// Initialize the static counter
std::atomic<int32> FPUDishBase::GlobalInstanceCounter(0);

FPUDishBase::FPUDishBase()
    : DishName(NAME_None)
    , DisplayName(FText::GetEmpty())
    , CustomName(FText::GetEmpty())
    , IngredientDataTable(nullptr)
{
}

// realizing that get ingredient and logically the get all ingredient is not applying the preparations when called here.
bool FPUDishBase::GetIngredient(const FGameplayTag& IngredientTag, FPUIngredientBase& OutIngredient) const
{
    if (!IngredientDataTable.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUDishBase::GetIngredient - IngredientDataTable is not valid!"));
        return false;
    }
    
    UDataTable* LoadedIngredientDataTable = IngredientDataTable.LoadSynchronous();
    if (!LoadedIngredientDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUDishBase::GetIngredient - Failed to load IngredientDataTable!"));
        return false;
    }
    
    // Get the ingredient row name from the tag (removes "Ingredient." prefix, converts to lowercase, removes periods)
    FName RowName = UPUDishBlueprintLibrary::GetIngredientRowNameFromTag(IngredientTag);
    UE_LOG(LogTemp, Display, TEXT("🔍 FPUDishBase::GetIngredient - Looking for tag: %s, RowName: %s"), 
        *IngredientTag.ToString(), *RowName.ToString());
        
    if (FPUIngredientBase* FoundIngredient = LoadedIngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredient")))
    {
        OutIngredient = *FoundIngredient;
        
        // Load preparation data if available
        if (OutIngredient.PreparationDataTable.IsValid())
        {
            UDataTable* LoadedPreparationDataTable = OutIngredient.PreparationDataTable.LoadSynchronous();
            if (LoadedPreparationDataTable)
            {
                // Apply any active preparations to the ingredient
                for (const FGameplayTag& PrepTag : OutIngredient.ActivePreparations)
                {
                    // Get the preparation name from the tag (everything after the last period) and convert to lowercase
                    FString PrepFullTag = PrepTag.ToString();
                    int32 PrepLastPeriodIndex;
                    if (PrepFullTag.FindLastChar('.', PrepLastPeriodIndex))
                    {
                        FString PrepName = PrepFullTag.RightChop(PrepLastPeriodIndex + 1).ToLower();
                        FName PrepRowName = FName(*PrepName);
                        
                        if (FPUPreparationBase* Preparation = LoadedPreparationDataTable->FindRow<FPUPreparationBase>(PrepRowName, TEXT("GetIngredient")))
                        {
                            // Apply preparation modifiers
                            Preparation->ApplyModifiers(OutIngredient.FlavorAspects, OutIngredient.TextureAspects);
                        }
                    }
                }
            }
        }
        
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUDishBase::GetIngredient - Could not find ingredient row '%s' in data table!"), *RowName.ToString());
    return false;
}

bool FPUDishBase::GetIngredientForInstance(int32 InstanceIndex, FPUIngredientBase& OutIngredient) const
{
    if (!IngredientInstances.IsValidIndex(InstanceIndex))
    {
        return false;
    }

    // Simply return the ingredient data that's already stored in the instance
    OutIngredient = IngredientInstances[InstanceIndex].IngredientData;
    return true;
}

TArray<FPUIngredientBase> FPUDishBase::GetAllIngredients() const
{
    TArray<FPUIngredientBase> Ingredients;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        Ingredients.Add(Instance.IngredientData);
    }
    return Ingredients;
}

TArray<FIngredientInstance> FPUDishBase::GetAllIngredientInstances() const
{
    return IngredientInstances;
}

int32 FPUDishBase::GetTotalIngredientQuantity() const
{
    int32 TotalQuantity = 0;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        TotalQuantity += Instance.Quantity;
    }
    return TotalQuantity;
}

float FPUDishBase::GetTotalFlavorAspect(const FName& AspectName) const
{
    float TotalValue = 0.0f;
    
    // Sum up values from all ingredients
    // Use the aspects directly from IngredientData which already includes:
    // - Base aspects
    // - Preparation modifications
    // - Time/temperature modifications
    // - Quantity multiplication
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Use the aspects directly from IngredientData (already calculated with all modifiers and quantity)
        float AspectValue = Instance.IngredientData.GetFlavorAspect(AspectName);
        TotalValue += AspectValue * Instance.Quantity;
    }
    
    return TotalValue;
}

float FPUDishBase::GetTotalTextureAspect(const FName& AspectName) const
{
    float TotalValue = 0.0f;
    
    // Sum up values from all ingredients
    // Use the aspects directly from IngredientData which already includes:
    // - Base aspects
    // - Preparation modifications
    // - Time/temperature modifications
    // - Quantity multiplication
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Use the aspects directly from IngredientData (already calculated with all modifiers and quantity)
        float AspectValue = Instance.IngredientData.GetTextureAspect(AspectName);
        TotalValue += AspectValue * Instance.Quantity;
    }
    
    return TotalValue;
}

bool FPUDishBase::HasIngredient(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Check both the convenient field and the data field for compatibility
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (InstanceTag == IngredientTag)
        {
            return true;
        }
    }
    return false;
}

FText FPUDishBase::GetCurrentDisplayName() const
{
    // If we have a custom name, use it
    if (!CustomName.IsEmpty())
    {
        return CustomName;
    }
    
    // Count how many ingredients have 2 or more preparations (suspicious ingredients)
    int32 TotalIngredients = 0;
    int32 SuspiciousIngredients = 0;
    
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTagContainer Preparations = Instance.Preparations.Num() > 0 ? Instance.Preparations : Instance.IngredientData.ActivePreparations;
        
        TotalIngredients += Instance.Quantity;
        if (Preparations.Num() > 1)
        {
            SuspiciousIngredients += Instance.Quantity;
        }
    }
    
    // If more than half of the ingredients are suspicious, apply "Suspicious" to the dish
    if (TotalIngredients > 0 && SuspiciousIngredients > TotalIngredients / 2)
    {
        FString BaseDishName = DisplayName.ToString();
        return FText::FromString(TEXT("Suspicious ") + BaseDishName);
    }
    
    // Track quantities of each ingredient
    TMap<FGameplayTag, int32> IngredientQuantities;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        IngredientQuantities.FindOrAdd(InstanceTag) += Instance.Quantity;
    }

    // Find the ingredient with the highest quantity
    FGameplayTag MostCommonTag;
    int32 MaxQuantity = 0;
    for (const auto& Pair : IngredientQuantities)
    {
        if (Pair.Value > MaxQuantity)
        {
            MaxQuantity = Pair.Value;
            MostCommonTag = Pair.Key;
        }
    }

    // If we have an ingredient with the highest quantity, get its name and prefix it
    if (MaxQuantity > 0)
    {
        FPUIngredientBase MostCommonIngredient;
        if (GetIngredient(MostCommonTag, MostCommonIngredient))
        {
            // Find the first instance of the most common ingredient to get its preparations
            for (const FIngredientInstance& Instance : IngredientInstances)
            {
                // Use convenient field if available, fallback to data field
                FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
                if (InstanceTag == MostCommonTag)
                {
                    // Apply the preparations from this instance (prefer convenient field, fallback to data field)
                    MostCommonIngredient.ActivePreparations = Instance.Preparations.Num() > 0 ? Instance.Preparations : Instance.IngredientData.ActivePreparations;
                    break;
                }
            }
            
            FString IngredientName = MostCommonIngredient.GetCurrentDisplayName().ToString();
            FString BaseDishName = DisplayName.ToString();
            
            // If the dish name doesn't already start with the ingredient name
            if (!BaseDishName.StartsWith(IngredientName))
            {
                return FText::FromString(IngredientName + " " + BaseDishName);
            }
        }
    }
    
    // If no ingredients or no most common ingredient found, return base dish name
    return DisplayName;
}

int32 FPUDishBase::FindInstanceIndexByID(int32 InstanceID) const
{
    for (int32 i = 0; i < IngredientInstances.Num(); ++i)
    {
        if (IngredientInstances[i].InstanceID == InstanceID)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 FPUDishBase::GenerateNewInstanceID() const
{
    int32 MaxID = 0;
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        MaxID = FMath::Max(MaxID, Instance.InstanceID);
    }
    return MaxID + 1;
}

int32 FPUDishBase::GenerateUniqueInstanceID()
{
    return ++GlobalInstanceCounter;
}

bool FPUDishBase::GetIngredientForInstanceID(int32 InstanceID, FPUIngredientBase& OutIngredient) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        OutIngredient = IngredientInstances[InstanceIndex].IngredientData;
        return true;
    }
    return false;
}

bool FPUDishBase::GetIngredientInstanceByID(int32 InstanceID, FIngredientInstance& OutInstance) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        OutInstance = IngredientInstances[InstanceIndex];
        return true;
    }
    return false;
}

FGameplayTag FPUDishBase::GetIngredientTag(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        // Use convenient field if available, fallback to data field
        return Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
    }
    return FGameplayTag();
}

FGameplayTagContainer FPUDishBase::GetPreparations(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        // Use convenient field if available, fallback to data field
        return Instance.Preparations.Num() > 0 ? Instance.Preparations : Instance.IngredientData.ActivePreparations;
    }
    return FGameplayTagContainer();
}

int32 FPUDishBase::GetQuantity(int32 InstanceID) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        return IngredientInstances[InstanceIndex].Quantity;
    }
    return 0;
}

// Plating-related functions (internal use only)
bool FPUDishBase::HasPlatingData() const
{
    for (const FIngredientInstance& Instance : IngredientInstances)
    {
        if (Instance.bIsPlated)
        {
            return true;
        }
    }
    return false;
}

void FPUDishBase::SetIngredientPlating(int32 InstanceID, const FVector& Position, const FRotator& Rotation, const FVector& Scale)
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        Instance.PlatingPosition = Position;
        Instance.PlatingRotation = Rotation;
        Instance.PlatingScale = Scale;
        Instance.bIsPlated = true;
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ FPUDishBase::SetIngredientPlating - Set plating for instance %d: Pos(%.2f,%.2f,%.2f) Rot(%.2f,%.2f,%.2f) Scale(%.2f,%.2f,%.2f)"), 
            InstanceID, Position.X, Position.Y, Position.Z, Rotation.Pitch, Rotation.Yaw, Rotation.Roll, Scale.X, Scale.Y, Scale.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUDishBase::SetIngredientPlating - Instance %d not found"), InstanceID);
    }
}

void FPUDishBase::ClearIngredientPlating(int32 InstanceID)
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        Instance.bIsPlated = false;
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ FPUDishBase::ClearIngredientPlating - Cleared plating for instance %d"), InstanceID);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ FPUDishBase::ClearIngredientPlating - Instance %d not found"), InstanceID);
    }
}

bool FPUDishBase::GetIngredientPlating(int32 InstanceID, FVector& OutPosition, FRotator& OutRotation, FVector& OutScale) const
{
    int32 InstanceIndex = FindInstanceIndexByID(InstanceID);
    if (InstanceIndex != INDEX_NONE)
    {
        const FIngredientInstance& Instance = IngredientInstances[InstanceIndex];
        if (Instance.bIsPlated)
        {
            OutPosition = Instance.PlatingPosition;
            OutRotation = Instance.PlatingRotation;
            OutScale = Instance.PlatingScale;
            return true;
        }
    }
    return false;
}
 