#include "PUIngredientQuantityControl.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/CheckBox.h"


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
    

    
    Super::NativeDestruct();
}

void UPUIngredientQuantityControl::SetIngredientInstance(const FIngredientInstance& InIngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetIngredientInstance - Setting ingredient instance: %s (ID: %d)"), 
        *InIngredientInstance.IngredientData.DisplayName.ToString(), InIngredientInstance.InstanceID);
    
    // Update ingredient instance data
    IngredientInstance = InIngredientInstance;
    
    // Update UI components
    if (IngredientNameText)
    {
        IngredientNameText->SetText(IngredientInstance.IngredientData.DisplayName);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::SetIngredientInstance - Updated ingredient name text"));
    }
    
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
    // This will be implemented when we add preparation checkboxes
    // For now, just log that preparations were updated
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::UpdatePreparationCheckboxes - Preparations updated (count: %d)"), 
        IngredientInstance.Preparations.Num());
}

void UPUIngredientQuantityControl::BroadcastChange()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientQuantityControl::BroadcastChange - Broadcasting ingredient instance change"));
    
    // Broadcast the change event with updated ingredient instance
    OnQuantityControlChanged.Broadcast(IngredientInstance);
} 