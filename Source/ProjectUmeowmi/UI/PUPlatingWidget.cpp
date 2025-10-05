#include "PUPlatingWidget.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUIngredientDragDropOperation.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"

UPUPlatingWidget::UPUPlatingWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CustomizationComponent = nullptr;
}

void UPUPlatingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::NativeConstruct - Plating widget constructed"));

    // Subscribe to events
    SubscribeToEvents();

    // Set plating mode on the component
    if (CustomizationComponent)
    {
        CustomizationComponent->SetPlatingMode(true);
    }
}

void UPUPlatingWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::NativeDestruct - Plating widget destructing"));

    // Unsubscribe from events
    UnsubscribeFromEvents();

    // Clear plating mode on the component
    if (CustomizationComponent)
    {
        CustomizationComponent->SetPlatingMode(false);
    }

    Super::NativeDestruct();
}

void UPUPlatingWidget::OnInitialDishDataReceived(const FPUDishBase& InitialDishData)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::OnInitialDishDataReceived - Received initial dish data with %d ingredients"), 
        InitialDishData.IngredientInstances.Num());

    CurrentDishData = InitialDishData;

    // Create ingredient buttons for plating
    CreateIngredientButtons();

    // Call blueprint event
    OnDishDataReceived(InitialDishData);
}

void UPUPlatingWidget::OnDishDataUpdated(const FPUDishBase& UpdatedDishData)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::OnDishDataUpdated - Received updated dish data with %d ingredients"), 
        UpdatedDishData.IngredientInstances.Num());

    CurrentDishData = UpdatedDishData;

    // Call blueprint event
    OnDishDataChanged(UpdatedDishData);
}

void UPUPlatingWidget::OnCustomizationEnded()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::OnCustomizationEnded - Customization ended"));

    // Call blueprint event
    OnCustomizationModeEnded();
}

void UPUPlatingWidget::SetCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SetCustomizationComponent - Setting customization component"));
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SetCustomizationComponent - Component: %s"), 
        Component ? TEXT("Valid") : TEXT("NULL"));

    // Unsubscribe from old component if it exists
    UnsubscribeFromEvents();

    CustomizationComponent = Component;
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SetCustomizationComponent - CustomizationComponent set to: %s"), 
        CustomizationComponent ? TEXT("Valid") : TEXT("NULL"));

    // Subscribe to new component if it exists
    SubscribeToEvents();
}

void UPUPlatingWidget::EndCustomizationFromUI()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::EndCustomizationFromUI - Ending customization from UI"));
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::EndCustomizationFromUI - CustomizationComponent: %s"), 
        CustomizationComponent ? TEXT("Valid") : TEXT("NULL"));

    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::EndCustomizationFromUI - Calling EndCustomization"));
        CustomizationComponent->EndCustomization();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUPlatingWidget::EndCustomizationFromUI - CustomizationComponent is NULL!"));
    }
}

void UPUPlatingWidget::UpdateDishData(const FPUDishBase& NewDishData)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::UpdateDishData - Updating dish data"));

    CurrentDishData = NewDishData;

    if (CustomizationComponent)
    {
        CustomizationComponent->SyncDishDataFromUI(NewDishData);
    }
}

void UPUPlatingWidget::CreateIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientButtons - Creating ingredient buttons for %d ingredients"), 
        CurrentDishData.IngredientInstances.Num());

    // Group ingredients by base ingredient tag
    TMap<FGameplayTag, TArray<FIngredientInstance>> GroupedIngredients;
    
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        FGameplayTag BaseTag = Instance.IngredientTag;
        if (!GroupedIngredients.Contains(BaseTag))
        {
            GroupedIngredients.Add(BaseTag, TArray<FIngredientInstance>());
        }
        GroupedIngredients[BaseTag].Add(Instance);
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientButtons - Grouped into %d ingredient categories"), 
        GroupedIngredients.Num());
    
    // Log the grouped ingredients for debugging
    for (const auto& Pair : GroupedIngredients)
    {
        FGameplayTag BaseTag = Pair.Key;
        const TArray<FIngredientInstance>& Instances = Pair.Value;
        
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientButtons - Category: %s (%d variants)"), 
            *BaseTag.ToString(), Instances.Num());
        
        for (const FIngredientInstance& Instance : Instances)
        {
            FString PrepText = TEXT("Raw");
            if (Instance.Preparations.Num() > 0)
            {
                FString PrepNames;
                for (const FGameplayTag& Prep : Instance.Preparations)
                {
                    if (!PrepNames.IsEmpty()) PrepNames += TEXT(", ");
                    PrepNames += Prep.ToString().Replace(TEXT("Prep."), TEXT(""));
                }
                PrepText = PrepNames;
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientButtons -   - %s (%s) x%d [ID: %d]"), 
                *Instance.IngredientData.DisplayName.ToString(), *PrepText, Instance.Quantity, Instance.InstanceID);
        }
    }
    
    // This will be implemented in Blueprint to create the actual UI buttons
    // The Blueprint should create grouped collapsible sections for each base ingredient
    // and populate them with enhanced ingredient buttons for each preparation variant
}

void UPUPlatingWidget::OnIngredientButtonClicked(const FPUIngredientBase& IngredientData)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::OnIngredientButtonClicked - Ingredient clicked: %s"), 
        *IngredientData.IngredientTag.ToString());

    // This will be handled in Blueprint to start Unreal's drag and drop
}

void UPUPlatingWidget::SpawnIngredientAtPosition(const FGameplayTag& IngredientTag, const FVector2D& ScreenPosition)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - START - Ingredient %s at screen position (%.2f,%.2f)"), 
        *IngredientTag.ToString(), ScreenPosition.X, ScreenPosition.Y);

    // Convert screen position to world position using raycast

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUPlatingWidget::SpawnIngredientAtPosition - No player controller"));
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - Got player controller: %s"), 
        *PlayerController->GetName());

    // Get camera location and rotation
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
    
    // Get viewport size
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - Viewport: %dx%d, Mouse: (%.0f,%.0f)"), 
        ViewportSizeX, ViewportSizeY, ScreenPosition.X, ScreenPosition.Y);
    
    // Find the dish customization station in the world
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
    
    AActor* DishStation = nullptr;
    for (AActor* Actor : FoundActors)
    {
        if (Actor && (Actor->GetName().Contains(TEXT("CookingStation")) || Actor->GetName().Contains(TEXT("DishCustomization"))))
        {
            DishStation = Actor;
            break;
        }
    }
    
    // Declare spawn position at function level
    FVector SpawnPosition;
    
    if (DishStation)
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Found dish customization station: %s"), *DishStation->GetName());
        
        // Get the station's location and bounds
        FVector StationLocation = DishStation->GetActorLocation();
        FVector StationBounds = DishStation->GetComponentsBoundingBox().GetSize();
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Station location: (%.2f,%.2f,%.2f), bounds: (%.2f,%.2f,%.2f)"), 
            StationLocation.X, StationLocation.Y, StationLocation.Z, StationBounds.X, StationBounds.Y, StationBounds.Z);
        
        // Calculate spawn position on the station surface
        // Use a small random offset to avoid stacking ingredients exactly on top of each other
        float RandomOffsetX = FMath::RandRange(-50.0f, 50.0f);
        float RandomOffsetY = FMath::RandRange(-50.0f, 50.0f);
        
        SpawnPosition = StationLocation + FVector(RandomOffsetX, RandomOffsetY, StationBounds.Z * 0.2f);
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Spawning on dish customization station at: (%.2f,%.2f,%.2f)"), 
            SpawnPosition.X, SpawnPosition.Y, SpawnPosition.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ No dish customization station found! Spawning at default position."));
        
        // Fallback: spawn near the player
        SpawnPosition = CameraLocation + (CameraRotation.Vector() * 300.0f);
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Fallback spawn position: (%.2f,%.2f,%.2f)"), 
            SpawnPosition.X, SpawnPosition.Y, SpawnPosition.Z);
    }
    
    // Spawn the ingredient in 3D
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - Calling SpawnIngredientIn3D on customization component"));
        CustomizationComponent->SpawnIngredientIn3D(IngredientTag, SpawnPosition);
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - SpawnIngredientIn3D call completed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ UPUPlatingWidget::SpawnIngredientAtPosition - CustomizationComponent is NULL!"));
    }

    // Call blueprint event
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - Calling OnIngredientSpawned Blueprint event"));
    OnIngredientSpawned(IngredientTag, ScreenPosition);
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SpawnIngredientAtPosition - END"));
}

UPUIngredientDragDropOperation* UPUPlatingWidget::CreateIngredientDragDropOperation(const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity)
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientDragDropOperation - Creating drag operation for ingredient %s (ID: %d, Qty: %d)"), 
        *IngredientTag.ToString(), InstanceID, Quantity);

    // Create the drag drop operation
    UPUIngredientDragDropOperation* DragOperation = nullptr;
    
    if (IngredientDragDropOperationClass)
    {
        DragOperation = NewObject<UPUIngredientDragDropOperation>(this, IngredientDragDropOperationClass);
    }
    else
    {
        // Fallback to creating the default class
        DragOperation = NewObject<UPUIngredientDragDropOperation>(this, UPUIngredientDragDropOperation::StaticClass());
    }

    if (DragOperation)
    {
        // Set up the drag operation with ingredient data
        DragOperation->SetupIngredientDrag(IngredientTag, IngredientData, InstanceID, Quantity);
        
        UE_LOG(LogTemp, Display, TEXT("✅ UPUPlatingWidget::CreateIngredientDragDropOperation - Successfully created drag operation"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ UPUPlatingWidget::CreateIngredientDragDropOperation - Failed to create drag operation"));
    }

    return DragOperation;
}

void UPUPlatingWidget::SubscribeToEvents()
{
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::SubscribeToEvents - Subscribing to customization component events"));

        CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &UPUPlatingWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &UPUPlatingWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.AddDynamic(this, &UPUPlatingWidget::OnCustomizationEnded);
    }
}

void UPUPlatingWidget::UnsubscribeFromEvents()
{
    if (CustomizationComponent)
    {
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::UnsubscribeFromEvents - Unsubscribing from customization component events"));

        CustomizationComponent->OnInitialDishDataReceived.RemoveDynamic(this, &UPUPlatingWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.RemoveDynamic(this, &UPUPlatingWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.RemoveDynamic(this, &UPUPlatingWidget::OnCustomizationEnded);
    }
}



void UPUPlatingWidget::RefreshIngredientButtons()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::RefreshIngredientButtons - Refreshing ingredient buttons"));

    // This will be implemented in Blueprint to refresh the UI
}

TArray<FIngredientInstance> UPUPlatingWidget::GetIngredientInstancesForBase(const FGameplayTag& BaseIngredientTag) const
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::GetIngredientInstancesForBase - Getting instances for base ingredient: %s"), 
        *BaseIngredientTag.ToString());

    TArray<FIngredientInstance> Instances;
    
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientTag == BaseIngredientTag)
        {
            Instances.Add(Instance);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::GetIngredientInstancesForBase - Found %d instances"), Instances.Num());
    
    return Instances;
}

TArray<FGameplayTag> UPUPlatingWidget::GetBaseIngredientTags() const
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::GetBaseIngredientTags - Getting all base ingredient tags"));

    TArray<FGameplayTag> BaseTags;
    
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (!BaseTags.Contains(Instance.IngredientTag))
        {
            BaseTags.Add(Instance.IngredientTag);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::GetBaseIngredientTags - Found %d unique base ingredients"), BaseTags.Num());
    
    return BaseTags;
}

void UPUPlatingWidget::ResetAllIngredientQuantities()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::ResetAllIngredientQuantities - Resetting all ingredient quantities"));

    // This will be implemented in Blueprint to reset all ingredient button quantities
    // The Blueprint should call ResetQuantity() on all ingredient buttons
} 