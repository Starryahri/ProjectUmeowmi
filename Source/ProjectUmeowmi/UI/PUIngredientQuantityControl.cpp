#include "PUIngredientQuantityControl.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/CheckBox.h"
#include "PUPreparationCheckbox.h"
#include "../DishCustomization/PUPreparationBase.h"


UPUIngredientQuantityControl::UPUIngredientQuantityControl(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUIngredientQuantityControl::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Bind button events
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnDecreaseQuantityClicked);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Decrease button event bound"));
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnIncreaseQuantityClicked);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Increase button event bound"));
    }
    
    if (RemoveButton)
    {
        RemoveButton->OnClicked.AddDynamic(this, &UPUIngredientQuantityControl::OnRemoveButtonClicked);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Remove button event bound"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::NativeConstruct - Widget setup complete"));
}

void UPUIngredientQuantityControl::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientQuantityControl::NativeDestruct - Widget destructing"));
    
    // Unbind button events
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnDecreaseQuantityClicked);
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnIncreaseQuantityClicked);
    }
    
    if (RemoveButton)
    {
        RemoveButton->OnClicked.RemoveDynamic(this, &UPUIngredientQuantityControl::OnRemoveButtonClicked);
    }
    
    // Clear preparation checkboxes
    ClearPreparationCheckboxes();
    
    Super::NativeDestruct();
}

void UPUIngredientQuantityControl::SetIngredientInstance(const FIngredientInstance& InIngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetIngredientInstance - Setting ingredient instance: %s (ID: %d)"), 
        *InIngredientInstance.IngredientData.DisplayName.ToString(), InIngredientInstance.InstanceID);
    
    // Update ingredient instance data
    IngredientInstance = InIngredientInstance;
    
    // Update UI components
    UpdateIngredientDisplay();
    
    if (IngredientIcon && IngredientInstance.IngredientData.PreviewTexture)
    {
        IngredientIcon->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetIngredientInstance - Updated ingredient icon"));
    }
    
    // Update quantity controls
    UpdateQuantityControls();
    
    // Update preparation checkboxes
    UpdatePreparationCheckboxes();
    
    // Call Blueprint event
    OnIngredientInstanceSet(IngredientInstance);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetIngredientInstance - Ingredient instance set successfully"));
}

void UPUIngredientQuantityControl::SetPreparationCheckboxClass(TSubclassOf<UPUPreparationCheckbox> InPreparationCheckboxClass)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetPreparationCheckboxClass - Setting preparation checkbox class"));
    
    PreparationCheckboxClass = InPreparationCheckboxClass;
    
    // If we already have an ingredient instance, update the preparation checkboxes
    if (IngredientInstance.InstanceID != 0)
    {
        UpdatePreparationCheckboxes();
    }
}

void UPUIngredientQuantityControl::SetQuantity(int32 NewQuantity)
{
    if (NewQuantity != IngredientInstance.Quantity)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetQuantity - Changing quantity from %d to %d"), 
            IngredientInstance.Quantity, NewQuantity);
        
        IngredientInstance.Quantity = NewQuantity;
        UpdateQuantityControls();
        BroadcastChange();
        
        // Call Blueprint event
        OnQuantityChanged(NewQuantity);
    }
}

void UPUIngredientQuantityControl::AddPreparation(const FGameplayTag& PreparationTag)
{
    if (!IngredientInstance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::AddPreparation - Adding preparation: %s"), 
            *PreparationTag.ToString());
        
        IngredientInstance.Preparations.AddTag(PreparationTag);
        
        // Update the ingredient data with the new preparation
        IngredientInstance.IngredientData.ActivePreparations = IngredientInstance.Preparations;
        
        // Log the current preparation state
        TArray<FGameplayTag> CurrentPreparations;
        IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::AddPreparation - Current preparations for instance %d: %d total"), 
            IngredientInstance.InstanceID, CurrentPreparations.Num());
        for (const FGameplayTag& Prep : CurrentPreparations)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::AddPreparation -   - %s"), *Prep.ToString());
        }
        
        // Update the UI to reflect the new name
        UpdateIngredientDisplay();
        
        UpdatePreparationCheckboxes();
        BroadcastChange();
        
        // Call Blueprint event
        OnPreparationChanged(PreparationTag, true);
    }
}

void UPUIngredientQuantityControl::RemovePreparation(const FGameplayTag& PreparationTag)
{
    if (IngredientInstance.Preparations.HasTag(PreparationTag))
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::RemovePreparation - Removing preparation: %s"), 
            *PreparationTag.ToString());
        
        IngredientInstance.Preparations.RemoveTag(PreparationTag);
        
        // Update the ingredient data with the new preparation state
        IngredientInstance.IngredientData.ActivePreparations = IngredientInstance.Preparations;
        
        // Log the current preparation state
        TArray<FGameplayTag> CurrentPreparations;
        IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::RemovePreparation - Current preparations for instance %d: %d total"), 
            IngredientInstance.InstanceID, CurrentPreparations.Num());
        for (const FGameplayTag& Prep : CurrentPreparations)
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::RemovePreparation -   - %s"), *Prep.ToString());
        }
        
        // Update the UI to reflect the new name
        UpdateIngredientDisplay();
        
        UpdatePreparationCheckboxes();
        BroadcastChange();
        
        // Call Blueprint event
        OnPreparationChanged(PreparationTag, false);
    }
}

bool UPUIngredientQuantityControl::HasPreparation(const FGameplayTag& PreparationTag) const
{
    return IngredientInstance.Preparations.HasTag(PreparationTag);
}

void UPUIngredientQuantityControl::RemoveIngredientInstance()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::RemoveIngredientInstance - Removing ingredient instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Broadcast removal event with widget reference
    OnQuantityControlRemoved.Broadcast(IngredientInstance.InstanceID, this);
    
    // Call Blueprint event
    OnIngredientRemoved();
}

void UPUIngredientQuantityControl::OnDecreaseQuantityClicked()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::OnDecreaseQuantityClicked - Decrease button clicked"));
    
    int32 NewQuantity = FMath::Max(IngredientInstance.Quantity - 1, IngredientInstance.IngredientData.MinQuantity);
    SetQuantity(NewQuantity);
}

void UPUIngredientQuantityControl::OnIncreaseQuantityClicked()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::OnIncreaseQuantityClicked - Increase button clicked"));
    
    int32 NewQuantity = FMath::Min(IngredientInstance.Quantity + 1, IngredientInstance.IngredientData.MaxQuantity);
    SetQuantity(NewQuantity);
}

void UPUIngredientQuantityControl::OnRemoveButtonClicked()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::OnRemoveButtonClicked - Remove button clicked"));
    
    RemoveIngredientInstance();
}

void UPUIngredientQuantityControl::OnPreparationCheckboxChanged(const FGameplayTag& PreparationTag, bool bIsChecked)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::OnPreparationCheckboxChanged - Preparation %s %s"), 
        *PreparationTag.ToString(), bIsChecked ? TEXT("added") : TEXT("removed"));
    
    if (bIsChecked)
    {
        AddPreparation(PreparationTag);
    }
    else
    {
        RemovePreparation(PreparationTag);
    }
}



void UPUIngredientQuantityControl::UpdateQuantityControls()
{

    
    if (DecreaseQuantityButton)
    {
        bool bCanDecrease = IngredientInstance.Quantity > IngredientInstance.IngredientData.MinQuantity;
        DecreaseQuantityButton->SetIsEnabled(bCanDecrease);
    }
    
    if (IncreaseQuantityButton)
    {
        bool bCanIncrease = IngredientInstance.Quantity < IngredientInstance.IngredientData.MaxQuantity;
        IncreaseQuantityButton->SetIsEnabled(bCanIncrease);
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdateQuantityControls - Quantity controls updated"));
}

void UPUIngredientQuantityControl::UpdatePreparationCheckboxes()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - Updating preparation checkboxes"));
    
    // Clear existing checkboxes
    ClearPreparationCheckboxes();
    
    // Check if we have a preparation data table
    if (!IngredientInstance.IngredientData.PreparationDataTable.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - No preparation data table available"));
        return;
    }
    
    UDataTable* LoadedPreparationDataTable = IngredientInstance.IngredientData.PreparationDataTable.LoadSynchronous();
    if (!LoadedPreparationDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - Failed to load preparation data table"));
        return;
    }
    
    TArray<FPUPreparationBase*> PreparationRows;
    LoadedPreparationDataTable->GetAllRows<FPUPreparationBase>(TEXT("UpdatePreparationCheckboxes"), PreparationRows);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - Found %d preparation options"), PreparationRows.Num());
    
    // Create checkboxes for each available preparation
    for (FPUPreparationBase* PreparationData : PreparationRows)
    {
        if (PreparationData && PreparationData->PreparationTag.IsValid())
        {
            // Check if this preparation is currently applied
            bool bIsCurrentlyApplied = IngredientInstance.Preparations.HasTag(PreparationData->PreparationTag);
            
            // Create the checkbox
            CreatePreparationCheckbox(*PreparationData, bIsCurrentlyApplied);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - Preparation checkboxes updated successfully"));
}

void UPUIngredientQuantityControl::UpdateIngredientDisplay()
{
    // Get the current display name (which includes preparation modifications)
    FText CurrentDisplayName = IngredientInstance.IngredientData.GetCurrentDisplayName();
    
    // Update the ingredient name text
    if (IngredientNameText)
    {
        IngredientNameText->SetText(CurrentDisplayName);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdateIngredientDisplay - Updated ingredient name to: %s"), 
            *CurrentDisplayName.ToString());
    }
}

void UPUIngredientQuantityControl::BroadcastChange()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::BroadcastChange - Broadcasting ingredient instance change for instance %d"), 
        IngredientInstance.InstanceID);
    
    // Log the current state of the ingredient instance being broadcast
    TArray<FGameplayTag> CurrentPreparations;
    IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::BroadcastChange - Instance %d has %d preparations:"), 
        IngredientInstance.InstanceID, CurrentPreparations.Num());
    for (const FGameplayTag& Prep : CurrentPreparations)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::BroadcastChange -   - %s"), *Prep.ToString());
    }
    
    // Broadcast the change event with updated ingredient instance
    OnQuantityControlChanged.Broadcast(IngredientInstance);
}

void UPUIngredientQuantityControl::ClearPreparationCheckboxes()
{
    if (PreparationsScrollBox)
    {
        PreparationsScrollBox->ClearChildren();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::ClearPreparationCheckboxes - Cleared preparation checkboxes"));
    }
}

void UPUIngredientQuantityControl::CreatePreparationCheckbox(const FPUPreparationBase& PreparationData, bool bIsCurrentlyApplied)
{
    if (!PreparationCheckboxClass || !PreparationsScrollBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 PUIngredientQuantityControl::CreatePreparationCheckbox - Missing checkbox class or scroll box"));
        return;
    }
    
    // Create the preparation checkbox widget
    UPUPreparationCheckbox* PreparationCheckbox = CreateWidget<UPUPreparationCheckbox>(this, PreparationCheckboxClass);
    if (!PreparationCheckbox)
    {
        UE_LOG(LogTemp, Error, TEXT("🎯 PUIngredientQuantityControl::CreatePreparationCheckbox - Failed to create preparation checkbox widget"));
        return;
    }
    
    // Get the preview texture
    UTexture2D* PreviewTexture = PreparationData.PreviewTexture;
    
    // Set the preparation data
    PreparationCheckbox->SetPreparationData(
        PreparationData.PreparationTag,
        PreparationData.DisplayName,
        PreparationData.Description,
        PreviewTexture
    );
    
    // Set the initial checked state
    PreparationCheckbox->SetChecked(bIsCurrentlyApplied);
    
    // Bind the checkbox change event
    PreparationCheckbox->OnPreparationCheckboxChanged.AddDynamic(this, &UPUIngredientQuantityControl::OnPreparationCheckboxChanged);
    
    // Add the checkbox to the scroll box
    PreparationsScrollBox->AddChild(PreparationCheckbox);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::CreatePreparationCheckbox - Created checkbox for preparation: %s (applied: %s)"), 
        *PreparationData.DisplayName.ToString(), bIsCurrentlyApplied ? TEXT("true") : TEXT("false"));
} 