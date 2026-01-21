#include "PUPreparationCheckbox.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

UPUPreparationCheckbox::UPUPreparationCheckbox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , IconTexture(nullptr)
{
}

void UPUPreparationCheckbox::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Bind checkbox event
    if (PreparationCheckBox)
    {
        PreparationCheckBox->OnCheckStateChanged.AddDynamic(this, &UPUPreparationCheckbox::OnCheckBoxChanged);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::NativeConstruct - Checkbox event bound"));
    }
    
    // Update UI if we already have data
    UpdateUI();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::NativeConstruct - Widget setup complete"));
}

void UPUPreparationCheckbox::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUPreparationCheckbox::NativeDestruct - Widget destructing"));
    
    // Unbind checkbox event
    if (PreparationCheckBox)
    {
        PreparationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPUPreparationCheckbox::OnCheckBoxChanged);
    }
    
    Super::NativeDestruct();
}

void UPUPreparationCheckbox::SetPreparationData(const FGameplayTag& InPreparationTag, const FText& InDisplayName, const FText& InDescription, UTexture2D* InIconTexture)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::SetPreparationData - Setting preparation data: %s"), 
        *InPreparationTag.ToString());
    
    PreparationTag = InPreparationTag;
    DisplayName = InDisplayName;
    Description = InDescription;
    IconTexture = InIconTexture;
    
    UpdateUI();
    
    // Call Blueprint event
    OnPreparationDataSet(InPreparationTag, InDisplayName, InDescription);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::SetPreparationData - Preparation data set successfully"));
}

void UPUPreparationCheckbox::SetChecked(bool bIsChecked)
{
    if (PreparationCheckBox && PreparationCheckBox->IsChecked() != bIsChecked)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::SetChecked - Setting checked state to: %s"), 
            bIsChecked ? TEXT("true") : TEXT("false"));
        
        PreparationCheckBox->SetIsChecked(bIsChecked);
    }
}

bool UPUPreparationCheckbox::IsChecked() const
{
    return PreparationCheckBox ? PreparationCheckBox->IsChecked() : false;
}

void UPUPreparationCheckbox::OnCheckBoxChanged(bool bIsChecked)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::OnCheckBoxChanged - Checkbox changed to: %s for preparation: %s"), 
        bIsChecked ? TEXT("checked") : TEXT("unchecked"), *PreparationTag.ToString());
    
    // Broadcast the change event
    OnPreparationCheckboxChanged.Broadcast(PreparationTag, bIsChecked);
    
    // Call Blueprint event
    OnCheckedStateChanged(bIsChecked);
}

void UPUPreparationCheckbox::UpdateUI()
{
    // Update preparation name text
    if (PreparationNameText)
    {
        PreparationNameText->SetText(DisplayName);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::UpdateUI - Updated preparation name text"));
    }
    
    // Update preparation icon
    if (PreparationIcon && IconTexture)
    {
        PreparationIcon->SetBrushFromTexture(IconTexture);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::UpdateUI - Updated preparation icon"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUPreparationCheckbox::UpdateUI - UI updated successfully"));
} 