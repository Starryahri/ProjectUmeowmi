#include "PUDishCustomizationWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"

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

int32 UPUDishCustomizationWidget::GenerateGUIDBasedInstanceID()
{
    // Generate a GUID and convert it to a unique integer
    FGuid NewGUID = FGuid::NewGuid();
    
    // Convert GUID to a unique integer using hash
    int32 UniqueID = GetTypeHash(NewGUID);
    
    // Ensure it's positive (hash can be negative)
    UniqueID = FMath::Abs(UniqueID);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 PUDishCustomizationWidget::GenerateGUIDBasedInstanceID - Generated GUID-based InstanceID: %d from GUID: %s"), 
        UniqueID, *NewGUID.ToString());
    
    return UniqueID;
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

void UPUDishCustomizationWidget::CreateIngredientSlots()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Creating ingredient slots from available ingredients"));
    
    // Clear existing slots and shelving widgets
    CreatedIngredientSlots.Empty();
    IngredientSlotMap.Empty();
    bIngredientSlotsCreated = false;
    
    // Clear shelving widgets
    CreatedShelvingWidgets.Empty();
    CurrentShelvingWidget.Reset();
    CurrentShelvingWidgetSlotCount = 0;
    
    // Check if we have a valid world context
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreateIngredientSlots - No world context available"));
        return;
    }
    
    if (CustomizationComponent)
    {
        TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Found %d available ingredients"), AvailableIngredients.Num());
        
        // Get the container to use (use slot container first, fallback to button container if they're the same)
        UPanelWidget* ContainerToUse = nullptr;
        if (IngredientSlotContainer.IsValid())
        {
            ContainerToUse = IngredientSlotContainer.Get();
        }
        else if (IngredientButtonContainer.IsValid())
        {
            ContainerToUse = IngredientButtonContainer.Get();
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Using IngredientButtonContainer as fallback"));
        }
        
        if (!ContainerToUse)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - No ingredient container set (neither slot nor button container)! Slots cannot be added."));
            UE_LOG(LogTemp, Warning, TEXT("⚠️   Slots will be added when SetIngredientSlotContainer() is called."));
        }
        
        for (const FPUIngredientBase& IngredientData : AvailableIngredients)
        {
            // Create ingredient slot using the Blueprint class
            TSubclassOf<UPUIngredientSlot> SlotClass;
            if (IngredientSlotClass)
            {
                SlotClass = IngredientSlotClass;
            }
            else
            {
                SlotClass = UPUIngredientSlot::StaticClass();
            }
            
            UPUIngredientSlot* IngredientSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
            if (IngredientSlot)
            {
                // Set the dish widget reference for easy access
                IngredientSlot->SetDishCustomizationWidget(this);
                
                // Set the location to Prep (for prep stage ingredient selection)
                IngredientSlot->SetLocation(EPUIngredientSlotLocation::Prep);
                
                // Create a minimal ingredient instance with just the ingredient data (quantity 0)
                // This allows the slot to display the pantry texture while remaining "empty"
                FIngredientInstance PantryInstance;
                PantryInstance.IngredientData = IngredientData;
                PantryInstance.Quantity = 0; // Empty slot, but has ingredient data for display
                PantryInstance.InstanceID = 0; // Not a real instance, just for display
                
                // Set the ingredient instance (slot will handle displaying pantry texture)
                IngredientSlot->SetIngredientInstance(PantryInstance);
                
                // Update the display
                IngredientSlot->UpdateDisplay();
                
                // Set the preparation data table if we have access to it
                if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
                {
                    IngredientSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
                }
                
                // Store slot reference in map for O(1) lookup (similar to buttons)
                IngredientSlotMap.Add(IngredientData.IngredientTag, IngredientSlot);
                
                // Also add to array
                CreatedIngredientSlots.Add(IngredientSlot);
                
                // Bind empty slot click event to handle ingredient selection
                IngredientSlot->OnEmptySlotClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);
                
                // Check if this ingredient is already selected (from existing dish ingredients)
                // This ensures ingredients that are already in the dish show as selected visually
                bool bIsSelected = IsIngredientSelected(IngredientData);
                IngredientSlot->SetSelected(bIsSelected);
                if (bIsSelected)
                {
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Marked ingredient as selected: %s"), 
                        *IngredientData.DisplayName.ToString());
                }
                
                // Call Blueprint event for slot creation (pass empty instance with ingredient data for reference)
                OnIngredientSlotCreated(IngredientSlot, PantryInstance);
                
                // Add to shelving widget (which will be added to container)
                if (ContainerToUse)
                {
                    // Get or create a current shelving widget
                    UUserWidget* ShelvingWidget = GetOrCreateCurrentShelvingWidget(ContainerToUse);
                    if (ShelvingWidget)
                    {
                        // Add slot to the shelving widget
                        if (AddSlotToCurrentShelvingWidget(IngredientSlot))
                        {
                            // UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Added slot to shelving widget for: %s"), 
                            //     *IngredientData.DisplayName.ToString());
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - Failed to add slot to shelving widget"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - Failed to get or create shelving widget"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Slot created but not added to container (container not set yet)"));
                }
                
                // UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Created slot for: %s"), 
                //     *IngredientData.DisplayName.ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - No customization component available"));
    }
    
    // Mark that slots have been created
    bIngredientSlotsCreated = true;
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Created %d ingredient slots"), CreatedIngredientSlots.Num());
}

void UPUDishCustomizationWidget::CreatePlatingIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Creating plating ingredient slots (replacing buttons)"));

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

    // Check if we have a valid world context
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreatePlatingIngredientButtons - No world context available"));
        return;
    }

    // Clear any existing slots
    CreatedIngredientSlots.Empty();
    bIngredientSlotsCreated = false;
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Cleared existing slots"));

    // Get the container to use (use slot container first, fallback to button container if they're the same)
    UPanelWidget* ContainerToUse = nullptr;
    if (IngredientSlotContainer.IsValid())
    {
        ContainerToUse = IngredientSlotContainer.Get();
    }
    else if (IngredientButtonContainer.IsValid())
    {
        ContainerToUse = IngredientButtonContainer.Get();
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Using IngredientButtonContainer as fallback"));
    }
    
    if (!ContainerToUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - No ingredient container set (neither slot nor button container)! Slots cannot be added."));
        UE_LOG(LogTemp, Warning, TEXT("⚠️   Slots will be added when SetIngredientSlotContainer() is called."));
    }

    // Create slots for each ingredient instance
    for (const FIngredientInstance& Instance : DishData.IngredientInstances)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ Creating plating slot for: %s (ID: %d, Qty: %d)"), 
            *Instance.IngredientData.DisplayName.ToString(), Instance.InstanceID, Instance.Quantity);

        // Create ingredient slot using the Blueprint class
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }

        UPUIngredientSlot* IngredientSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (IngredientSlot)
        {
            // Set the dish widget reference for easy access
            IngredientSlot->SetDishCustomizationWidget(this);
            
            // Set the location to ActiveIngredientArea (same as cooking stage)
            IngredientSlot->SetLocation(EPUIngredientSlotLocation::ActiveIngredientArea);
            
            // Set the ingredient instance
            IngredientSlot->SetIngredientInstance(Instance);
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Created slot with ingredient: %s (ID: %d, Qty: %d)"), 
                *Instance.IngredientData.DisplayName.ToString(), Instance.InstanceID, Instance.Quantity);
            
            // Set the preparation data table if we have access to it
            if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
            {
                IngredientSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
            }
            
            // Bind to slot's ingredient changed event so we can update dish data
            IngredientSlot->OnSlotIngredientChanged.AddDynamic(this, &UPUDishCustomizationWidget::OnQuantityControlChanged);
            
            // Enable drag functionality for plating stage
            IngredientSlot->SetDragEnabled(true);
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Enabled drag for slot: %s"), 
                *Instance.IngredientData.DisplayName.ToString());
            
            // Add to our array
            CreatedIngredientSlots.Add(IngredientSlot);
            
            // Call Blueprint event for slot creation (equivalent to OnIngredientButtonCreated)
            OnIngredientSlotCreated(IngredientSlot, Instance);
            
            // Add to the UI container if available
            if (ContainerToUse)
            {
                ContainerToUse->AddChild(IngredientSlot);
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Added slot to container for: %s"), 
                    *Instance.IngredientData.DisplayName.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Slot created but not added to container (container not set yet)"));
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Created slot for: %s"), 
                *Instance.IngredientData.DisplayName.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Failed to create slot for: %s"), 
                *Instance.IngredientData.DisplayName.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Created %d plating ingredient slots"), 
        CreatedIngredientSlots.Num());
    
    // Mark that slots have been created
    bIngredientSlotsCreated = true;
    
    // Call the plating stage initialized event
    OnPlatingStageInitialized(DishData);
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientButtons - Called OnPlatingStageInitialized event"));
}

void UPUDishCustomizationWidget::EnablePlatingButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabling all plating slots (replacing buttons)"));

    // Update all slots to ensure they're displaying correctly and enable drag
    for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
    {
        if (IngredientSlot)
        {
            // Enable drag functionality for plating stage
            IngredientSlot->SetDragEnabled(true);
            
            // Update the slot display (this will refresh icons, quantity control, etc.)
            IngredientSlot->UpdateDisplay();
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Enabled drag and updated slot display for: %s"), 
                IngredientSlot->IsEmpty() ? TEXT("Empty Slot") : *IngredientSlot->GetIngredientInstance().IngredientData.DisplayName.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingButtons - Updated %d plating slots"), CreatedIngredientSlots.Num());
}


void UPUDishCustomizationWidget::SetIngredientSlotContainer(UPanelWidget* Container, EPUIngredientSlotLocation SlotLocation)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Setting ingredient slot container (Location: %d)"), (int32)SlotLocation);
    
    if (!Container)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::SetIngredientSlotContainer - Container is null"));
        return;
    }
    
    IngredientButtonContainer = Container;
    // Also set the slot container to the same container since they're the same
    IngredientSlotContainer = Container;
    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Container set successfully (also set IngredientSlotContainer)"));
    
    // Add any existing buttons to the new container
    // Note: Plating now uses slots instead of buttons, so we only add regular ingredient buttons
    int32 TotalButtons = IngredientButtonMap.Num();
    
    if (TotalButtons > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Adding %d existing buttons to container"), TotalButtons);
        
        // Add regular ingredient buttons (for planning stage)
        for (auto& ButtonPair : IngredientButtonMap)
        {
            if (ButtonPair.Value)
            {
                Container->AddChild(ButtonPair.Value);
            }
        }
    }
    
    // If we already have created slots, add them to the new container
    if (CreatedIngredientSlots.Num() > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Found %d existing slots to add to container"), CreatedIngredientSlots.Num());
        int32 SlotsAdded = 0;
        for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
        {
            if (IngredientSlot)
            {
                // Set location to the specified location for all slots
                IngredientSlot->SetLocation(SlotLocation);
                
                if (!IngredientSlot->GetParent())
                {
                    Container->AddChild(IngredientSlot);
                    SlotsAdded++;
                    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Added slot to container (Slot: %s, Location: %d)"), 
                        *IngredientSlot->GetName(), (int32)SlotLocation);
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Slot already has a parent, skipping (Slot: %s)"), *IngredientSlot->GetName());
                }
            }
        }
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Successfully added %d slots to container"), SlotsAdded);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - No existing slots to add (slots will be created when ingredients are added)"));
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
        
        // In planning mode, use ToggleIngredientSelection which properly handles both SelectedIngredients and IngredientInstances
        ToggleIngredientSelection(IngredientData);
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
    
    // Update the ingredient instance in the dish data (or add if new)
    bool bFound = false;
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            bFound = true;
            break;
        }
    }
    
    if (bFound)
    {
        // Update existing instance
        UpdateIngredientInstance(IngredientInstance);
    }
    else
    {
        // Add new instance (e.g., when ingredient is dropped on empty slot)
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Instance not found in dish data, adding new instance (ID: %d, Qty: %d)"), 
            IngredientInstance.InstanceID, IngredientInstance.Quantity);
        CurrentDishData.IngredientInstances.Add(IngredientInstance);
        UpdateDishData(CurrentDishData);
    }
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

    for (int32 i = CurrentDishData.IngredientInstances.Num() - 1; i >= 0; i--)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        // Check both convenient field and data field (same logic as IsIngredientSelected)
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        if (InstanceTag == IngredientTag)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstanceByTag - Instance removed successfully"));
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

void UPUDishCustomizationWidget::OnPantrySlotClicked(UPUIngredientSlot* IngredientSlot)
{
    // Handle pantry/prep slot click - find the ingredient data from the slot map
    if (!IngredientSlot)
    {
        return;
    }
    
    // Find the ingredient data by looking up the slot in our map
    for (auto& SlotPair : IngredientSlotMap)
    {
        if (SlotPair.Value == IngredientSlot)
        {
            // Found the slot, get the ingredient data from the component
            if (CustomizationComponent)
            {
                TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
                const FPUIngredientBase* FoundIngredient = AvailableIngredients.FindByPredicate([&SlotPair](const FPUIngredientBase& Ingredient) {
                    return Ingredient.IngredientTag == SlotPair.Key;
                });
                
                if (FoundIngredient)
                {
                    // For prep slots, handle selection with max limit enforcement
                    if (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::Prep)
                    {
                        bool bCurrentlySelected = IngredientSlot->IsSelected();
                        bool bWantToSelect = !bCurrentlySelected;
                        
                        // Check if ingredient is already in dish data (actually selected)
                        bool bIngredientInDishData = IsIngredientSelected(*FoundIngredient);
                        
                        // If trying to select, check max limit
                        if (bWantToSelect && !bIngredientInDishData)
                        {
                            // Count unique selected ingredients
                            int32 CurrentSelectedCount = CurrentDishData.IngredientInstances.Num();
                            
                            if (CurrentSelectedCount >= MaxIngredients)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::OnPantrySlotClicked - Max ingredients reached (%d). Cannot select more."), MaxIngredients);
                                // Don't toggle selection, don't add ingredient - keep visual state as is
                                return;
                            }
                        }
                        
                        // DON'T set visual state here - wait until after we confirm the ingredient was actually added
                    }
                    
                    // Call the same handler as button click (this handles ingredient selection logic)
                    // In planning mode, this calls ToggleIngredientSelection which already updates the slot's visual state
                    // So we don't need to update it again here
                    OnIngredientButtonClicked(*FoundIngredient);
                    
                    // Only sync visual state if NOT in planning mode (planning mode is handled by ToggleIngredientSelection)
                    if (!bInPlanningMode && IngredientSlot->GetLocation() == EPUIngredientSlotLocation::Prep)
                    {
                        bool bActuallySelected = IsIngredientSelected(*FoundIngredient);
                        IngredientSlot->SetSelected(bActuallySelected);
                        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnPantrySlotClicked - Synced slot selection state to: %s (actual: %s)"), 
                            bActuallySelected ? TEXT("SELECTED") : TEXT("UNSELECTED"),
                            bActuallySelected ? TEXT("YES") : TEXT("NO"));
                    }
                    
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::OnPantrySlotClicked - Slot clicked for: %s"), 
                        *FoundIngredient->DisplayName.ToString());
                    return;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::OnPantrySlotClicked - Could not find ingredient data for clicked slot"));
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
    
    if (bInPlanningMode)
    {
        // In planning mode, work with SelectedIngredients
        if (bWasSelected)
        {
            // Remove from selected ingredients
            PlanningData.SelectedIngredients.RemoveAll([&](const FPUIngredientBase& SelectedIngredient) {
                return SelectedIngredient.IngredientTag == IngredientData.IngredientTag;
            });
            
            // Also remove from IngredientInstances when unselecting in planning mode
            // This ensures the ingredient is fully unselected and won't appear in cooking stage
            RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
            
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Removed ingredient from planning"));
        }
        else
        {
            // Add to selected ingredients
            PlanningData.SelectedIngredients.Add(IngredientData);
            
            // In planning mode, we don't create IngredientInstances yet (quantities will be set in cooking stage)
            // But we mark it as selected in SelectedIngredients
            
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Added ingredient to planning"));
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Planning now has %d selected ingredients"), 
            PlanningData.SelectedIngredients.Num());
        
        // Update the slot's visual state to reflect the new selection state
        if (UPUIngredientSlot** FoundSlot = IngredientSlotMap.Find(IngredientData.IngredientTag))
        {
            if (*FoundSlot)
            {
                bool bIsNowSelected = IsIngredientSelected(IngredientData);
                (*FoundSlot)->SetSelected(bIsNowSelected);
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Updated slot visual state to: %s"), 
                    bIsNowSelected ? TEXT("SELECTED") : TEXT("UNSELECTED"));
            }
        }
        
        // Update radar chart in planning mode by creating a temporary dish from SelectedIngredients
        UpdateRadarChartFromPlanningData();
    }
    else
    {
        // In cooking/prep mode, work with IngredientInstances
        if (bWasSelected)
        {
            // Remove from selected ingredients
            RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Removed ingredient from dish"));
        }
        else
        {
            // Add to selected ingredients
            CreateIngredientInstance(IngredientData);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Added ingredient to dish"));
        }
        
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Dish now has %d ingredient instances"), 
            CurrentDishData.IngredientInstances.Num());
    }
    
    // Call Blueprint event
    OnIngredientSelectionChanged(IngredientData, !bWasSelected);
}

void UPUDishCustomizationWidget::UpdateRadarChartFromPlanningData()
{
    if (!bInPlanningMode)
    {
        return;
    }
    
    // Create a temporary dish from SelectedIngredients for the radar chart
    FPUDishBase TempDish = CurrentDishData;
    TempDish.IngredientInstances.Empty();
    
    // Convert SelectedIngredients to IngredientInstances (with quantity 1 for each)
    for (const FPUIngredientBase& SelectedIngredient : PlanningData.SelectedIngredients)
    {
        // Create a temporary instance with quantity 1
        FIngredientInstance TempInstance;
        TempInstance.InstanceID = FPUDishBase::GenerateUniqueInstanceID();
        TempInstance.Quantity = 1;
        TempInstance.IngredientData = SelectedIngredient;
        TempInstance.IngredientTag = SelectedIngredient.IngredientTag;
        TempInstance.Preparations = SelectedIngredient.ActivePreparations;
        
        TempDish.IngredientInstances.Add(TempInstance);
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::UpdateRadarChartFromPlanningData - Created temp dish with %d ingredients for radar chart"), 
        TempDish.IngredientInstances.Num());
    
    // Update the radar chart by calling the existing Blueprint event that already handles it
    OnDishDataChanged(TempDish);
}

bool UPUDishCustomizationWidget::IsIngredientSelected(const FPUIngredientBase& IngredientData) const
{
    // In planning mode, check both SelectedIngredients and IngredientInstances
    // (we keep IngredientInstances visible so players can see what's already in the dish)
    if (bInPlanningMode)
    {
        // First check SelectedIngredients
        bool bInSelectedIngredients = PlanningData.SelectedIngredients.ContainsByPredicate([&](const FPUIngredientBase& SelectedIngredient) {
            return SelectedIngredient.IngredientTag == IngredientData.IngredientTag;
        });
        
        // Also check IngredientInstances (in case something is there but not in SelectedIngredients yet)
        bool bInIngredientInstances = CurrentDishData.IngredientInstances.ContainsByPredicate([&](const FIngredientInstance& Instance) {
            FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
            return InstanceTag == IngredientData.IngredientTag;
        });
        
        return bInSelectedIngredients || bInIngredientInstances;
    }
    
    // In cooking/prep mode, check IngredientInstances
    return CurrentDishData.IngredientInstances.ContainsByPredicate([&](const FIngredientInstance& Instance) {
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        return InstanceTag == IngredientData.IngredientTag;
    });
}

void UPUDishCustomizationWidget::StartPlanningMode()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Initialize planning data with current dish
    PlanningData.TargetDish = CurrentDishData;
    PlanningData.SelectedIngredients.Empty();
    PlanningData.bPlanningCompleted = false;
    
    // Populate SelectedIngredients from existing dish ingredients
    // Extract unique ingredients from IngredientInstances (planning mode uses ingredients without quantities)
    // IMPORTANT: We do NOT clear IngredientInstances - they should remain visible to the player
    // The ingredients that are already in the dish should be shown as selected
    TMap<FGameplayTag, FPUIngredientBase> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        // Only add if we haven't seen this ingredient tag before
        if (InstanceTag.IsValid() && !UniqueIngredients.Contains(InstanceTag))
        {
            // Use the ingredient data from the instance (which already has preparations applied)
            UniqueIngredients.Add(InstanceTag, Instance.IngredientData);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Added existing ingredient to SelectedIngredients: %s"), 
                *InstanceTag.ToString());
        }
    }
    
    // Add all unique ingredients to SelectedIngredients
    UniqueIngredients.GenerateValueArray(PlanningData.SelectedIngredients);
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Populated %d existing ingredients into SelectedIngredients"), 
        PlanningData.SelectedIngredients.Num());
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Keeping %d ingredient instances visible (not clearing)"), 
        CurrentDishData.IngredientInstances.Num());
    
    // DO NOT clear IngredientInstances - they should remain visible and selected
    // The player should see what ingredients are already in the dish
    
    // Update all existing ingredient slots to show their correct selection state
    // This ensures ingredients that are already in the dish show as selected visually
    for (auto& SlotPair : IngredientSlotMap)
    {
        if (SlotPair.Value)
        {
            // Find the ingredient data for this slot
            if (CustomizationComponent)
            {
                TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
                const FPUIngredientBase* FoundIngredient = AvailableIngredients.FindByPredicate([&SlotPair](const FPUIngredientBase& Ingredient) {
                    return Ingredient.IngredientTag == SlotPair.Key;
                });
                
                if (FoundIngredient)
                {
                    bool bIsSelected = IsIngredientSelected(*FoundIngredient);
                    SlotPair.Value->SetSelected(bIsSelected);
                    if (bIsSelected)
                    {
                        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Updated slot selection state for: %s (SELECTED)"), 
                            *FoundIngredient->DisplayName.ToString());
                    }
                }
            }
        }
    }
    
    // Update radar chart with initial planning data
    UpdateRadarChartFromPlanningData();
    
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
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Planning has %d selected ingredients in SelectedIngredients"), 
        PlanningData.SelectedIngredients.Num());
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Current dish has %d ingredient instances"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Convert SelectedIngredients to IngredientInstances for cooking stage
    // First, clear existing instances (we'll rebuild from SelectedIngredients)
    // But preserve any that are already there with quantities (like pre-loaded ingredients)
    FPUDishBase CookingDishData = CurrentDishData;
    
    // Create a map of existing instances by tag to preserve quantities
    TMap<FGameplayTag, FIngredientInstance> ExistingInstances;
    for (const FIngredientInstance& Instance : CookingDishData.IngredientInstances)
    {
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (InstanceTag.IsValid() && Instance.Quantity > 0)
        {
            ExistingInstances.Add(InstanceTag, Instance);
        }
    }
    
    // Clear and rebuild IngredientInstances from SelectedIngredients
    CookingDishData.IngredientInstances.Empty();
    
    for (const FPUIngredientBase& SelectedIngredient : PlanningData.SelectedIngredients)
    {
        // Check if we already have an instance for this ingredient (preserve quantity)
        if (FIngredientInstance* ExistingInstance = ExistingInstances.Find(SelectedIngredient.IngredientTag))
        {
            // Use existing instance (preserves quantity and preparations)
            CookingDishData.IngredientInstances.Add(*ExistingInstance);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Preserved existing instance for: %s (Qty: %d)"), 
                *SelectedIngredient.DisplayName.ToString(), ExistingInstance->Quantity);
        }
        else
        {
            // Create new instance with default quantity of 1
            FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(CookingDishData, SelectedIngredient.IngredientTag);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Created new instance for: %s (Qty: %d)"), 
                *SelectedIngredient.DisplayName.ToString(), NewInstance.Quantity);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Cooking dish now has %d ingredient instances"), 
        CookingDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = CookingDishData;
    UpdateDishData(CookingDishData);
    
    // Mark planning as completed
    PlanningData.bPlanningCompleted = true;
    
    // Transition to cooking stage through the component
    if (CustomizationComponent)
    {
        CustomizationComponent->TransitionToCookingStage(CookingDishData);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::FinishPlanningAndStartCooking - No customization component available"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientSlotsFromDishData()
{
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Creating ingredient slots from dish data"));
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Current slot count before clearing: %d"), CreatedIngredientSlots.Num());
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Slots already created: %s"), bIngredientSlotsCreated ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Check if slots were already created
    if (bIngredientSlotsCreated)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Slots already created, skipping to prevent duplicates"));
        return;
    }
    
    // Check if we have a valid world context
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - No world context available"));
        return;
    }
    
    // Clear any existing slots and shelving widgets
    CreatedIngredientSlots.Empty();
    CreatedShelvingWidgets.Empty();
    CurrentShelvingWidget.Reset();
    CurrentShelvingWidgetSlotCount = 0;
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Cleared existing slots and shelving widgets"));
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Found %d ingredient instances in dish data"), CurrentDishData.IngredientInstances.Num());
    
    // Debug: Log each ingredient instance
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🎯 Ingredient %d: Tag=%s, Name=%s, ID=%d, Qty=%d"), 
            i, 
            *Instance.IngredientData.IngredientTag.ToString(),
            *Instance.IngredientData.DisplayName.ToString(),
            Instance.InstanceID,
            Instance.Quantity);
    }
    
    // Get the container to use (use slot container first, fallback to button container if they're the same)
    UPanelWidget* ContainerToUse = nullptr;
    if (IngredientSlotContainer.IsValid())
    {
        ContainerToUse = IngredientSlotContainer.Get();
    }
    else if (IngredientButtonContainer.IsValid())
    {
        ContainerToUse = IngredientButtonContainer.Get();
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Using IngredientButtonContainer as fallback"));
    }
    
    if (!ContainerToUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - No ingredient container set (neither slot nor button container)! Slots cannot be added."));
        UE_LOG(LogTemp, Warning, TEXT("⚠️   Slots will be added when SetIngredientSlotContainer() is called."));
    }
    
    // Create slots for up to 12 ingredient instances (max slots in active ingredient area)
    // Create slots for existing ingredients, or create empty slots if no ingredients yet
    int32 NumSlotsToCreate = FMath::Max(CurrentDishData.IngredientInstances.Num(), 12); // Always create 12 slots (empty or filled)
    NumSlotsToCreate = FMath::Min(12, NumSlotsToCreate); // Cap at 12
    
    for (int32 i = 0; i < NumSlotsToCreate; ++i)
    {
        // Create ingredient slot using the Blueprint class
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }
        
        UPUIngredientSlot* IngredientSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (IngredientSlot)
        {
            // Set the dish widget reference for easy access
            IngredientSlot->SetDishCustomizationWidget(this);
            
            // Set the location to ActiveIngredientArea
            IngredientSlot->SetLocation(EPUIngredientSlotLocation::ActiveIngredientArea);
            
            // If we have an ingredient instance for this slot, set it
            if (i < CurrentDishData.IngredientInstances.Num())
            {
                const FIngredientInstance& IngredientInstance = CurrentDishData.IngredientInstances[i];
                
                // Validate ingredient instance
                if (IngredientInstance.IngredientData.IngredientTag.IsValid())
                {
                    // Set the ingredient instance
                    IngredientSlot->SetIngredientInstance(IngredientInstance);
                    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Created slot with ingredient: %s (ID: %d, Qty: %d)"), 
                        *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Invalid ingredient instance at index %d, creating empty slot"), i);
                }
            }
            else
            {
                // Create empty slot - ensure display is cleared (removes quantity control)
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Created empty slot %d"), i);
                // Call UpdateDisplay to ensure empty slot clears quantity control and other UI elements
                IngredientSlot->UpdateDisplay();
            }
            
            // Set the preparation data table if we have access to it
            if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
            {
                IngredientSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
            }
            
            // Bind to slot's ingredient changed event so we can update dish data and flavor graphs
            IngredientSlot->OnSlotIngredientChanged.AddDynamic(this, &UPUDishCustomizationWidget::OnQuantityControlChanged);
            
            // Enable drag functionality for all slots (for testing - can be disabled per slot if needed)
            if (IngredientSlot->IsEmpty() == false)
            {
                IngredientSlot->SetDragEnabled(true);
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Enabled drag for slot with ingredient"));
            }
            
            // Add to our array
            CreatedIngredientSlots.Add(IngredientSlot);
            
            // Call Blueprint event for slot creation (equivalent to OnIngredientButtonCreated)
            // Only call if slot has an ingredient (empty slots don't need the event)
            if (i < CurrentDishData.IngredientInstances.Num())
            {
                const FIngredientInstance& IngredientInstance = CurrentDishData.IngredientInstances[i];
                if (IngredientInstance.IngredientData.IngredientTag.IsValid())
                {
                    OnIngredientSlotCreated(IngredientSlot, IngredientInstance);
                }
            }
            
            // Add to shelving widget (which will be added to container)
            if (ContainerToUse)
            {
                // Get or create a current shelving widget
                UUserWidget* ShelvingWidget = GetOrCreateCurrentShelvingWidget(ContainerToUse);
                if (ShelvingWidget)
                {
                    // Add slot to the shelving widget
                    if (AddSlotToCurrentShelvingWidget(IngredientSlot))
                    {
                        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Added slot to shelving widget"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Failed to add slot to shelving widget"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Failed to get or create shelving widget"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Slot created but not added to container (container not set yet)"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Failed to create ingredient slot at index %d"), i);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsFromDishData - Successfully created %d ingredient slots in %d shelving widgets"), 
        CreatedIngredientSlots.Num(), CreatedShelvingWidgets.Num());
    
    // Mark that slots have been created
    bIngredientSlotsCreated = true;
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget(UPanelWidget* ContainerToUse)
{
    // Check if we need a new shelving widget
    // Need a new one if: no current widget, or current widget has 3 slots
    if (!CurrentShelvingWidget.IsValid() || CurrentShelvingWidgetSlotCount >= 3)
    {
        // Create a new shelving widget
        if (!ShelvingWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - ShelvingWidgetClass not set!"));
            return nullptr;
        }
        
        if (!GetWorld())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - No world context available"));
            return nullptr;
        }
        
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - Failed to create shelving widget"));
            return nullptr;
        }
        
        // Add the shelving widget to the container
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
            UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - Created and added new shelving widget (Total: %d)"), 
                CreatedShelvingWidgets.Num() + 1);
        }
        
        // Track the new shelving widget
        CreatedShelvingWidgets.Add(NewShelvingWidget);
        CurrentShelvingWidget = NewShelvingWidget;
        CurrentShelvingWidgetSlotCount = 0;
        
        return NewShelvingWidget;
    }
    
    // Return the current shelving widget
    return CurrentShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - IngredientSlot is null"));
        return false;
    }
    
    if (!CurrentShelvingWidget.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - CurrentShelvingWidget is not valid"));
        return false;
    }
    
    // Find the HorizontalBox inside the shelving widget
    // First, try to get it by name
    UWidget* FoundWidget = CurrentShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        // If not found by name, try common names
        FoundWidget = CurrentShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }
    
    // Try to cast to HorizontalBox
    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentShelvingWidgetSlotCount++;
        // UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Added slot to HorizontalBox (Slot count: %d/3)"), 
        //     CurrentShelvingWidgetSlotCount);
        return true;
    }
    
    // Try casting to any panel widget
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentShelvingWidgetSlotCount++;
        UE_LOG(LogTemp, Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Added slot to panel widget (Slot count: %d/3)"), 
            CurrentShelvingWidgetSlotCount);
        return true;
    }
    
    // If still not found, log error with helpful message
    UE_LOG(LogTemp, Error, TEXT("❌ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Could not find HorizontalBox or panel widget in WBP_Shelving!"));
    UE_LOG(LogTemp, Error, TEXT("   Searched for widget named: '%s', 'HorizontalBox', 'SlotContainer'"), *ShelvingHorizontalBoxName.ToString());
    UE_LOG(LogTemp, Error, TEXT("   Please ensure WBP_Shelving contains a HorizontalBox (or other panel widget) with one of these names."));
    return false;
}
