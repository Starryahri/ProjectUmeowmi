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

    // Unsubscribe from old component if it exists
    UnsubscribeFromEvents();

    CustomizationComponent = Component;

    // Subscribe to new component if it exists
    SubscribeToEvents();
}

void UPUPlatingWidget::EndCustomizationFromUI()
{
    UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::EndCustomizationFromUI - Ending customization from UI"));

    if (CustomizationComponent)
    {
        CustomizationComponent->EndCustomization();
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

    // This will be implemented in Blueprint to create the actual UI buttons
    // For now, we just log the ingredients that should be available for plating
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍽️ UPUPlatingWidget::CreateIngredientButtons - Ingredient %d: %s (Qty: %d, ID: %d)"), 
            i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity, Instance.InstanceID);
    }
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