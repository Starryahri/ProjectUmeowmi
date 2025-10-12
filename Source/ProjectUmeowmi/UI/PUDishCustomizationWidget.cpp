#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "Components/Button.h"

UPUDishCustomizationWidget::UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CustomizationComponent(nullptr)
{
}

void UPUDishCustomizationWidget::NativeConstruct()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - STARTING WIDGET CONSTRUCTION"));
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget class: %s"), *GetClass()->GetName());
    
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Super::NativeConstruct completed"));
    
    // Check if we're in the game world
    UWorld* World = GetWorld();
    if (World)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World valid: %s"), *World->GetName());
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World type: %s"), 
            World->IsGameWorld() ? TEXT("Game World") : TEXT("Editor World"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - No World available"));
    }
    
    // Check widget visibility
    if (IsVisible())
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is visible"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT visible"));
    }
    
    // Check if we're in viewport
    if (IsInViewport())
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is in viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT in viewport"));
    }
    
    // Note: Don't subscribe to events here - the component reference isn't set yet
    // Subscription will happen in SetCustomizationComponent()
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - WIDGET CONSTRUCTION COMPLETED"));
}

void UPUDishCustomizationWidget::NativeDestruct()
{
    // UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::NativeDestruct - Widget destructing"));
    
    // Unsubscribe from events
    UnsubscribeFromEvents();
    
    Super::NativeDestruct();
}

void UPUDishCustomizationWidget::OnInitialDishDataReceived(const FPUDishBase& InitialDishData)
{
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - RECEIVED INITIAL DISH DATA"));
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Received initial dish data: %s with %d ingredients"), 
        *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());
    
    // Log dish details
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish tag: %s"), *InitialDishData.DishTag.ToString());
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish display name: %s"), *InitialDishData.DisplayName.ToString());
    
    // Log ingredient details
    for (int32 i = 0; i < InitialDishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = InitialDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Ingredient %d: %s (Qty: %d, ID: %d)"), 
            i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity, Instance.InstanceID);
    }
    
    // Update current dish data
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Updating current dish data"));
    CurrentDishData = InitialDishData;
    
    // Call the Blueprint event
    UE_LOG(LogTemp, Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Calling Blueprint event OnDishDataReceived"));
    OnDishDataReceived(InitialDishData);
    
    UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::OnInitialDishDataReceived - INITIAL DISH DATA PROCESSED SUCCESSFULLY"));
}

void UPUDishCustomizationWidget::OnDishDataUpdated(const FPUDishBase& UpdatedDishData)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnDishDataUpdated - Received dish data update: %s with %d ingredients"), 
        *UpdatedDishData.DisplayName.ToString(), UpdatedDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = UpdatedDishData;
    
    // Call the Blueprint event
    OnDishDataChanged(UpdatedDishData);
}

void UPUDishCustomizationWidget::OnCustomizationEnded()
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::OnCustomizationEnded - Customization ended"));
    
    // Call the Blueprint event
    OnCustomizationModeEnded();
}

void UPUDishCustomizationWidget::SetCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - STARTING COMPONENT CONNECTION"));
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Widget name: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Component: %s"), 
        Component ? *Component->GetName() : TEXT("NULL"));
    
    // Unsubscribe from previous component if any
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Unsubscribing from previous component: %s"), *CustomizationComponent->GetName());
        UnsubscribeFromEvents();
    }
    
    // Set the new component reference
    CustomizationComponent = Component;
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Component reference set successfully"));
        
        // Subscribe to the new component's events
        UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Subscribing to component events"));
        SubscribeToEvents();
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Event subscription completed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::SetCustomizationComponent - Component reference is NULL"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - COMPONENT CONNECTION COMPLETED"));
}

void UPUDishCustomizationWidget::UpdateDishData(const FPUDishBase& NewDishData)
{
    UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::UpdateDishData - Updating dish data: %s with %d ingredients"), 
        *NewDishData.DisplayName.ToString(), NewDishData.IngredientInstances.Num());
    
    // Update local data
    CurrentDishData = NewDishData;
    
    // Sync back to the customization component
    if (CustomizationComponent)
    {
        CustomizationComponent->SyncDishDataFromUI(NewDishData);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PUDishCustomizationWidget::UpdateDishData - No customization component reference"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating ingredient buttons"));
    
    // Clear existing buttons
    IngredientButtonMap.Empty();
    
    if (CustomizationComponent)
    {
        TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Found %d available ingredients"), AvailableIngredients.Num());
        
        for (const FPUIngredientBase& IngredientData : AvailableIngredients)
        {
            if (IngredientButtonClass)
            {
                UPUIngredientButton* IngredientButton = CreateWidget<UPUIngredientButton>(this, IngredientButtonClass);
                if (IngredientButton)
                {
                    IngredientButton->SetIngredientData(IngredientData);
                    
                    // Store button reference in map for O(1) lookup
                    IngredientButtonMap.Add(IngredientData.IngredientTag, IngredientButton);
                    
                    // Bind the button click event
                    IngredientButton->OnIngredientButtonClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnIngredientButtonClicked);
                    
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Created button for: %s"), 
                        *IngredientData.DisplayName.ToString());
                    
                    // Hide text elements for planning stage (prep stage should hide text)
                    IngredientButton->HideAllText();
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Hidden text elements for planning stage"));
                    
                    // Call Blueprint event
                    OnIngredientButtonCreated(IngredientButton, IngredientData);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientButtons - No customization component available"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Created %d ingredient buttons"), IngredientButtonMap.Num());
}

void UPUDishCustomizationWidget::CreatePlatingIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Creating plating ingredient buttons"));

    if (!CustomizationComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - No customization component available"));
        return;
    }

    // Get the current dish data
    const FPUDishBase& DishData = CustomizationComponent->GetCurrentDishData();
    
    if (DishData.IngredientInstances.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - No ingredient instances in dish data"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Found %d ingredient instances"), 
        DishData.IngredientInstances.Num());

    // Debug: Log all ingredient instances
    for (int32 i = 0; i < DishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = DishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍽️ DEBUG: Instance %d - %s (ID: %d, Qty: %d, Preparations: %d)"), 
            i, *Instance.IngredientData.DisplayName.ToString(), Instance.InstanceID, Instance.Quantity, Instance.Preparations.Num());
        
        // Log preparation details
        TArray<FGameplayTag> PreparationTags;
        Instance.Preparations.GetGameplayTagArray(PreparationTags);
        for (const FGameplayTag& PrepTag : PreparationTags)
        {
            UE_LOG(LogTemp, Display, TEXT("🍽️ DEBUG:   - Preparation: %s"), *PrepTag.ToString());
        }
    }

    // Create buttons for each ingredient instance
    for (const FIngredientInstance& Instance : DishData.IngredientInstances)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ Creating plating button for: %s (ID: %d, Qty: %d)"), 
            *Instance.IngredientData.DisplayName.ToString(), Instance.InstanceID, Instance.Quantity);

        // Create ingredient button using the Blueprint class
        TSubclassOf<UPUIngredientButton> ButtonClass;
        if (IngredientButtonClass)
        {
            ButtonClass = IngredientButtonClass;
        }
        else
        {
            ButtonClass = UPUIngredientButton::StaticClass();
        }

        UPUIngredientButton* IngredientButton = CreateWidget<UPUIngredientButton>(this, ButtonClass);
        if (IngredientButton)
        {
            // Set the ingredient instance data (this uses our enhanced PUIngredientButton)
            IngredientButton->SetIngredientInstance(Instance);
            
            // Also set the ingredient data to ensure texture is set (like cooking stage)
            IngredientButton->SetIngredientData(Instance.IngredientData);
            
            // Keep the internal button enabled for plating stage
            UButton* InternalButton = IngredientButton->GetIngredientButton();
            if (InternalButton)
            {
                InternalButton->SetIsEnabled(true);
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Enabled internal button for: %s"), 
                    *Instance.IngredientData.DisplayName.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Internal button is null for: %s"), 
                    *Instance.IngredientData.DisplayName.ToString());
            }
            
            // Enable drag functionality for plating
            IngredientButton->SetDragEnabled(true);
            
            // Show text elements for plating stage
            IngredientButton->ShowAllText();
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Shown text elements for plating stage"));
            
            // Add to our plating button map using InstanceID as the key (since it's unique)
            PlatingIngredientButtonMap.Add(Instance.InstanceID, IngredientButton);
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Added button to plating map for InstanceID: %d"), Instance.InstanceID);
            
            // Add to container if available
            if (IngredientButtonContainer.IsValid())
            {
                IngredientButtonContainer->AddChild(IngredientButton);
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Added button to container for: %s"), 
                    *Instance.IngredientData.DisplayName.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - No ingredient button container set! Button will not be visible."));
                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Container is: %s"), 
                    IngredientButtonContainer.IsValid() ? TEXT("VALID") : TEXT("INVALID"));
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Created button for: %s"), 
                *Instance.IngredientData.DisplayName.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Failed to create button for: %s"), 
                *Instance.IngredientData.DisplayName.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Created %d plating ingredient buttons"), 
        PlatingIngredientButtonMap.Num());
    
    // Call the plating stage initialized event
    OnPlatingStageInitialized(DishData);
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Called OnPlatingStageInitialized event"));
}

void UPUDishCustomizationWidget::EnablePlatingButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabling all plating buttons"));

    for (auto& ButtonPair : PlatingIngredientButtonMap)
    {
        if (ButtonPair.Value)
        {
            // Enable the internal button (unlike cooking stage)
            UButton* InternalButton = ButtonPair.Value->GetIngredientButton();
            if (InternalButton)
            {
                InternalButton->SetIsEnabled(true);
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabled internal button for: %s"), 
                    *ButtonPair.Value->GetIngredientData().DisplayName.ToString());
            }
            
            // Update the button display (this should update icons and text)
            ButtonPair.Value->UpdatePlatingDisplay();
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabled button for: %s"), 
                *ButtonPair.Value->GetIngredientData().DisplayName.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabled %d plating buttons"), PlatingIngredientButtonMap.Num());
}


void UPUDishCustomizationWidget::SetIngredientButtonContainer(UPanelWidget* Container)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientButtonContainer - Setting ingredient button container"));
    
    if (!Container)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::SetIngredientButtonContainer - Container is null"));
        return;
    }
    
    IngredientButtonContainer = Container;
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientButtonContainer - Container set successfully"));
    
    // Add any existing buttons to the new container
    // Check both regular ingredient buttons and plating ingredient buttons
    int32 TotalButtons = IngredientButtonMap.Num() + PlatingIngredientButtonMap.Num();
    
    if (TotalButtons > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientButtonContainer - Adding %d existing buttons to container"), TotalButtons);
        
        // Add regular ingredient buttons
        for (auto& ButtonPair : IngredientButtonMap)
        {
            if (ButtonPair.Value)
            {
                Container->AddChild(ButtonPair.Value);
            }
        }
        
        // Add plating ingredient buttons
        for (auto& ButtonPair : PlatingIngredientButtonMap)
        {
            if (ButtonPair.Value)
            {
                Container->AddChild(ButtonPair.Value);
            }
        }
    }
}

UPUIngredientButton* UPUDishCustomizationWidget::FindIngredientButton(const FPUIngredientBase& IngredientData) const
{
    // O(1) lookup using map
    const UPUIngredientButton* const* FoundButton = IngredientButtonMap.Find(IngredientData.IngredientTag);
    if (FoundButton)
    {
        return const_cast<UPUIngredientButton*>(*FoundButton);
    }
    return nullptr;
}

UPUIngredientButton* UPUDishCustomizationWidget::GetIngredientButtonByTag(const FGameplayTag& IngredientTag) const
{
    // O(1) lookup using map
    const UPUIngredientButton* const* FoundButton = IngredientButtonMap.Find(IngredientTag);
    if (FoundButton)
    {
        return const_cast<UPUIngredientButton*>(*FoundButton);
    }
    return nullptr;
}

void UPUDishCustomizationWidget::OnIngredientButtonClicked(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Ingredient button clicked: %s"), 
        *IngredientData.DisplayName.ToString());
    
    if (bInPlanningMode)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - In planning mode, handling toggle selection"));
        
        // In planning mode, toggle ingredient selection by adding/removing from dish data
        if (IsIngredientSelected(IngredientData))
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Ingredient is selected, removing it"));
            
            // Remove ingredient instance
            RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Ingredient is not selected, checking if can add"));
            
            // Check if we can add more ingredients
            if (CanAddMoreIngredients())
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Can add more ingredients, adding ingredient"));
                
                // Add ingredient instance with default quantity
                CreateIngredientInstance(IngredientData);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Cannot add more ingredients, max reached (%d)"), MaxIngredients);
                // TODO: Show UI feedback that max ingredients reached
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Not in planning mode, using legacy behavior"));
        
        // Legacy behavior - create a new ingredient instance
        CreateIngredientInstance(IngredientData);
    }
}

bool UPUDishCustomizationWidget::CanAddMoreIngredients() const
{
    // Count unique ingredients (not instances, since one ingredient can have multiple instances)
    TSet<FGameplayTag> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        UniqueIngredients.Add(Instance.IngredientTag);
    }
    
    bool bCanAdd = UniqueIngredients.Num() < MaxIngredients;
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CanAddMoreIngredients - Current: %d, Max: %d, CanAdd: %s"), 
        UniqueIngredients.Num(), MaxIngredients, bCanAdd ? TEXT("Yes") : TEXT("No"));
    
    return bCanAdd;
}

void UPUDishCustomizationWidget::SetMaxIngredients(int32 NewMaxIngredients)
{
    // Clamp the value to reasonable bounds
    MaxIngredients = FMath::Clamp(NewMaxIngredients, 1, 20);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::SetMaxIngredients - Set max ingredients to %d"), MaxIngredients);
}

void UPUDishCustomizationWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Log the preparations in the received ingredient instance
    TArray<FGameplayTag> CurrentPreparations;
    IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Received instance %d with %d preparations:"), 
        IngredientInstance.InstanceID, CurrentPreparations.Num());
    for (const FGameplayTag& Prep : CurrentPreparations)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged -   - %s"), *Prep.ToString());
    }
    
    // Update the ingredient instance in the dish data
    UpdateIngredientInstance(IngredientInstance);
}

void UPUDishCustomizationWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Remove the ingredient instance from the dish data
    RemoveIngredientInstance(InstanceID);
    
    // Remove the widget from viewport
    if (QuantityControlWidget)
    {
        QuantityControlWidget->RemoveFromParent();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Widget removed from viewport"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientInstance(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Creating ingredient instance: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Create a new ingredient instance using the blueprint library
    FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(CurrentDishData, IngredientData.IngredientTag);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Created instance with ID: %d"), NewInstance.InstanceID);
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
    
    // Call Blueprint event to create quantity control with ingredient instance data
    OnQuantityControlCreated(nullptr, NewInstance); // Pass the ingredient instance data
}

void UPUDishCustomizationWidget::UpdateIngredientInstance(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Updating ingredient instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Log the preparations before updating
    TArray<FGameplayTag> PreparationsBefore;
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i].Preparations.GetGameplayTagArray(PreparationsBefore);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d had %d preparations before update:"), 
                IngredientInstance.InstanceID, PreparationsBefore.Num());
            for (const FGameplayTag& Prep : PreparationsBefore)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            break;
        }
    }
    
    // Find and update the ingredient instance in the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i] = IngredientInstance;
            
            // Log the preparations after updating
            TArray<FGameplayTag> PreparationsAfter;
            IngredientInstance.Preparations.GetGameplayTagArray(PreparationsAfter);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d now has %d preparations after update:"), 
                IngredientInstance.InstanceID, PreparationsAfter.Num());
            for (const FGameplayTag& Prep : PreparationsAfter)
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance updated successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RemoveIngredientInstance(int32 InstanceID)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Removing ingredient instance: %d"), InstanceID);
    
    // Find and remove the ingredient instance from the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == InstanceID)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Instance removed successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RemoveIngredientInstanceByTag(const FGameplayTag& IngredientTag)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstanceByTag - Removing ingredient instance by tag: %s"), *IngredientTag.ToString());

    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].IngredientTag == IngredientTag)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstanceByTag - Instance removed successfully"));
            break;
        }
    }
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RefreshQuantityControls()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RefreshQuantityControls - Refreshing quantity controls"));
    
    // This will be called when dish data is updated to refresh all quantity controls
    // Blueprint can override this to handle the UI updates
}

void UPUDishCustomizationWidget::SubscribeToEvents()
{
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - STARTING EVENT SUBSCRIPTION"));
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Widget name: %s"), *GetName());
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - Customization component valid: %s"), *CustomizationComponent->GetName());
        
        // Subscribe to the component's events
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnInitialDishDataReceived"));
        CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnDishDataUpdated"));
        CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        
        UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnCustomizationEnded"));
        CustomizationComponent->OnCustomizationEnded.AddDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
        
        UE_LOG(LogTemp, Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - All events subscribed successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::SubscribeToEvents - No customization component to subscribe to"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - EVENT SUBSCRIPTION COMPLETED"));
}

void UPUDishCustomizationWidget::UnsubscribeFromEvents()
{
    if (CustomizationComponent)
    {
        // UE_LOG(LogTemp, Display, TEXT("PUDishCustomizationWidget::UnsubscribeFromEvents - Unsubscribing from customization component events"));
        
        // Unsubscribe from the component's events
        CustomizationComponent->OnInitialDishDataReceived.RemoveDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.RemoveDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.RemoveDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
    }
}

void UPUDishCustomizationWidget::EndCustomizationFromUI()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - UI button pressed to end customization"));
    
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - Calling EndCustomization on component"));
        CustomizationComponent->EndCustomization();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::EndCustomizationFromUI - No customization component available"));
    }
} 

void UPUDishCustomizationWidget::ToggleIngredientSelection(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Toggling ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Check if ingredient is already selected
    bool bWasSelected = IsIngredientSelected(IngredientData);
    
    if (bWasSelected)
    {
        // Remove from selected ingredients
        RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Removed ingredient from planning"));
    }
    else
    {
        // Add to selected ingredients
        CreateIngredientInstance(IngredientData);
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Added ingredient to planning"));
    }
    
    // Call Blueprint event
    OnIngredientSelectionChanged(IngredientData, !bWasSelected);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Planning now has %d selected ingredients"), 
        CurrentDishData.IngredientInstances.Num());
}

bool UPUDishCustomizationWidget::IsIngredientSelected(const FPUIngredientBase& IngredientData) const
{
    return CurrentDishData.IngredientInstances.ContainsByPredicate([&](const FIngredientInstance& Instance) {
        return Instance.IngredientTag == IngredientData.IngredientTag;
    });
}

void UPUDishCustomizationWidget::StartPlanningMode()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Clear existing ingredient instances for planning
    CurrentDishData.IngredientInstances.Empty();
    
    // Call Blueprint event
    OnPlanningModeStarted();
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Planning mode started for dish: %s"), 
        *CurrentDishData.DisplayName.ToString());
}

void UPUDishCustomizationWidget::FinishPlanningAndStartCooking()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Finishing planning and starting cooking"));
    
    if (!bInPlanningMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::FinishPlanningAndStartCooking - Not in planning mode"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Planning completed with %d selected ingredients"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Transition to cooking stage through the component
    if (CustomizationComponent)
    {
        CustomizationComponent->TransitionToCookingStage(CurrentDishData);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::FinishPlanningAndStartCooking - No customization component available"));
    }
} 