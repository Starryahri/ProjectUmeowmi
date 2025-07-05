#include "PUIngredientButtonWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "PUDishCustomizationWidget.h"
#include "PUIngredientQuantityControl.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

UPUIngredientButtonWidget::UPUIngredientButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ParentDishWidget(nullptr)
    , SpawnedQuantityWidget(nullptr)
{
}

void UPUIngredientButtonWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButtonWidget::NativeConstruct - Widget constructed: %s"), *GetName());
    
    // Bind button click event
    if (IngredientButton)
    {
        IngredientButton->OnClicked.AddDynamic(this, &UPUIngredientButtonWidget::OnButtonClicked);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButtonWidget::NativeConstruct - Button click event bound"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientButtonWidget::NativeConstruct - No ingredient button found"));
    }
    
    // Update display if we have instance data
    if (IngredientInstance.InstanceID != 0)
    {
        UpdateDisplayText();
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUIngredientButtonWidget::NativeConstruct - Widget setup complete"));
}

void UPUIngredientButtonWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::NativeDestruct - Widget destructing"));
    
    // Unbind button click event
    if (IngredientButton)
    {
        IngredientButton->OnClicked.RemoveDynamic(this, &UPUIngredientButtonWidget::OnButtonClicked);
    }
    
    // Clean up spawned quantity widget
    DestroyQuantityControlWidget();
    
    Super::NativeDestruct();
}

void UPUIngredientButtonWidget::InitializeWithInstance(const FIngredientInstance& Instance)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::InitializeWithInstance - Initializing with instance ID: %d, ingredient: %s"), 
        Instance.InstanceID, *Instance.IngredientData.DisplayName.ToString());
    
    IngredientInstance = Instance;
    UpdateDisplayText();
}

void UPUIngredientButtonWidget::UpdateDisplay(const FIngredientInstance& Instance)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::UpdateDisplay - Updating display for instance ID: %d"), Instance.InstanceID);
    
    IngredientInstance = Instance;
    UpdateDisplayText();
}

void UPUIngredientButtonWidget::SetParentDishWidget(UPUDishCustomizationWidget* DishWidget)
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::SetParentDishWidget - Setting parent dish widget"));
    
    ParentDishWidget = DishWidget;
}

void UPUIngredientButtonWidget::OnIngredientButtonClicked()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::OnIngredientButtonClicked - Button clicked for instance ID: %d"), 
        IngredientInstance.InstanceID);
    
    OnButtonClicked();
}

void UPUIngredientButtonWidget::CloseQuantityWidget()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::CloseQuantityWidget - Closing quantity widget"));
    
    DestroyQuantityControlWidget();
}

bool UPUIngredientButtonWidget::IsQuantityWidgetOpen() const
{
    return SpawnedQuantityWidget != nullptr;
}

void UPUIngredientButtonWidget::OnButtonClicked()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::OnButtonClicked - Button clicked for ingredient: %s (Instance ID: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID);
    
    // If quantity widget is already open, close it
    if (SpawnedQuantityWidget)
    {
        DestroyQuantityControlWidget();
    }
    else
    {
        // Spawn new quantity control widget
        SpawnQuantityControlWidget();
    }
}

void UPUIngredientButtonWidget::UpdateDisplayText()
{
    // Update ingredient name
    if (IngredientNameText)
    {
        FText DisplayName = IngredientInstance.IngredientData.DisplayName;
        if (DisplayName.IsEmpty())
        {
            // Fallback to ingredient tag if display name is empty
            DisplayName = FText::FromString(IngredientInstance.IngredientData.IngredientTag.ToString());
        }
        IngredientNameText->SetText(DisplayName);
    }
    
    // Update quantity text
    if (QuantityText)
    {
        FText QuantityDisplay = FText::Format(FText::FromString(TEXT("Qty: {0}")), FText::AsNumber(IngredientInstance.Quantity));
        QuantityText->SetText(QuantityDisplay);
    }
    
    // Update preparation text
    UpdatePreparationText();
    
    // Update ingredient image
    UpdateIngredientImage();
    
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::UpdateDisplayText - Updated display for: %s (Qty: %d)"), 
        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.Quantity);
}

void UPUIngredientButtonWidget::UpdatePreparationText()
{
    if (!PreparationText)
    {
        return;
    }
    
    // Get preparations from the convenient field or fallback to data field
    FGameplayTagContainer Preparations = IngredientInstance.Preparations.Num() > 0 ? 
        IngredientInstance.Preparations : IngredientInstance.IngredientData.ActivePreparations;
    
    if (Preparations.Num() > 0)
    {
        // Convert preparations to display text
        TArray<FString> PreparationNames;
        TArray<FGameplayTag> PreparationTags;
        Preparations.GetGameplayTagArray(PreparationTags);
        
        for (const FGameplayTag& PrepTag : PreparationTags)
        {
            FString PrepName = PrepTag.ToString();
            // Extract just the preparation name (after the last period)
            int32 LastPeriodIndex;
            if (PrepName.FindLastChar('.', LastPeriodIndex))
            {
                PrepName = PrepName.RightChop(LastPeriodIndex + 1);
            }
            PreparationNames.Add(PrepName);
        }
        
        FString PreparationsString = FString::Join(PreparationNames, TEXT(", "));
        PreparationText->SetText(FText::FromString(PreparationsString));
    }
    else
    {
        PreparationText->SetText(FText::FromString(TEXT("No preparations")));
    }
}

void UPUIngredientButtonWidget::UpdateIngredientImage()
{
    if (IngredientImage && IngredientInstance.IngredientData.PreviewTexture)
    {
        IngredientImage->SetBrushFromTexture(IngredientInstance.IngredientData.PreviewTexture);
    }
}

void UPUIngredientButtonWidget::SpawnQuantityControlWidget()
{
    UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::SpawnQuantityControlWidget - Spawning quantity control widget"));
    
    // Check if we have a parent dish widget
    if (!ParentDishWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientButtonWidget::SpawnQuantityControlWidget - No parent dish widget set"));
        return;
    }
    
    // Create the quantity control widget using the parent dish widget
    SpawnedQuantityWidget = ParentDishWidget->CreateQuantityControlWidget(IngredientInstance);
    if (SpawnedQuantityWidget)
    {
        // Set the parent button widget reference
        SpawnedQuantityWidget->SetParentButtonWidget(this);
        
        // Add to the scroll box in the parent dish widget
        ParentDishWidget->AddQuantityControlToScrollBox(SpawnedQuantityWidget);
        
        UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::SpawnQuantityControlWidget - Quantity control widget spawned successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUIngredientButtonWidget::SpawnQuantityControlWidget - Failed to create quantity control widget"));
    }
}

void UPUIngredientButtonWidget::DestroyQuantityControlWidget()
{
    if (SpawnedQuantityWidget)
    {
        UE_LOG(LogTemp, Display, TEXT("PUIngredientButtonWidget::DestroyQuantityControlWidget - Destroying quantity control widget"));
        
        // Remove from parent dish widget's scroll box
        if (ParentDishWidget)
        {
            ParentDishWidget->RemoveQuantityControlFromScrollBox(SpawnedQuantityWidget);
        }
        
        SpawnedQuantityWidget = nullptr;
    }
} 