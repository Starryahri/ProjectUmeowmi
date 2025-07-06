#include "PUIngredientButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

UPUIngredientButton::UPUIngredientButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUIngredientButton::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Bind button events
    if (IngredientButton)
    {
        IngredientButton->OnClicked.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonClickedInternal);
        IngredientButton->OnHovered.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonHoveredInternal);
        IngredientButton->OnUnhovered.AddDynamic(this, &UPUIngredientButton::OnIngredientButtonUnhoveredInternal);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Button events bound"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientButton::NativeConstruct - IngredientButton component not found"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::NativeConstruct - Widget setup complete"));
}

void UPUIngredientButton::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButton::NativeDestruct - Widget destructing"));
    
    // Unbind button events
    if (IngredientButton)
    {
        IngredientButton->OnClicked.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonClickedInternal);
        IngredientButton->OnHovered.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonHoveredInternal);
        IngredientButton->OnUnhovered.RemoveDynamic(this, &UPUIngredientButton::OnIngredientButtonUnhoveredInternal);
    }
    
    Super::NativeDestruct();
}

void UPUIngredientButton::SetIngredientData(const FPUIngredientBase& InIngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Setting ingredient data: %s"), 
        *InIngredientData.DisplayName.ToString());
    
    // Update ingredient data
    IngredientData = InIngredientData;
    
    // Update UI components
    if (IngredientNameText)
    {
        IngredientNameText->SetText(IngredientData.DisplayName);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Updated ingredient name text"));
    }
    
    if (IngredientIcon && IngredientData.PreviewTexture)
    {
        IngredientIcon->SetBrushFromTexture(IngredientData.PreviewTexture);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Updated ingredient icon"));
    }
    
    // Call Blueprint event
    OnIngredientDataSet(InIngredientData);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::SetIngredientData - Ingredient data set successfully"));
}

void UPUIngredientButton::OnIngredientButtonClickedInternal()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonClickedInternal - Button clicked for ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Broadcast the click event with ingredient data
    OnIngredientButtonClicked.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonClicked();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonClickedInternal - Click event broadcasted"));
}

void UPUIngredientButton::OnIngredientButtonHoveredInternal()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonHoveredInternal - Button hovered for ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Broadcast the hover event with ingredient data
    OnIngredientButtonHovered.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonHovered();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonHoveredInternal - Hover event broadcasted"));
}

void UPUIngredientButton::OnIngredientButtonUnhoveredInternal()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonUnhoveredInternal - Button unhovered for ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Broadcast the unhover event with ingredient data
    OnIngredientButtonUnhovered.Broadcast(IngredientData);
    
    // Call Blueprint event
    OnButtonUnhovered();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButton::OnIngredientButtonUnhoveredInternal - Unhover event broadcasted"));
} 