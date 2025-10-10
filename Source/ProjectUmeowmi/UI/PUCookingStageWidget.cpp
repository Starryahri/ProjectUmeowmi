#include "PUCookingStageWidget.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUIngredientButton.h"
#include "Components/Button.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Input/Events.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Player.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../DishCustomization/PUOrderBase.h"
#include "PUIngredientDragDropOperation.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"

FGameplayTagContainer UPUCookingStageWidget::GetPreparationTagsForImplement(int32 ImplementIndex) const
{
    if (ImplementPreparationTags.IsValidIndex(ImplementIndex))
    {
        return ImplementPreparationTags[ImplementIndex];
    }
    return FGameplayTagContainer();
}
UPUCookingStageWidget::UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Widgets tick by default, no need to enable explicitly
}

void UPUCookingStageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeConstruct - Cooking stage widget constructed"));
}

void UPUCookingStageWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeDestruct - Cooking stage widget destructing"));
    
    Super::NativeDestruct();
}

void UPUCookingStageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Update carousel rotation if needed
    if (bCarouselSpawned)
    {
        if (bIsRotating)
        {
            // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeTick - Carousel is rotating, updating..."));
        }
        UpdateCarouselRotation(InDeltaTime);
        
                    // Update hover detection only when not dragging
                    if (!bIsDragActive)
                    {
                        UpdateHoverDetection();
                    }
                    
                    // Disable click handling during drag
                    if (!bIsDragActive)
                    {
                        HandleMouseClick();
                    }
                    
                    // Update debug visualization
                    // DrawDebugHoverArea(); // Debug visualization disabled
                }
}

void UPUCookingStageWidget::InitializeCookingStage(const FPUDishBase& DishData, const FVector& CookingStationLocation)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Initializing cooking stage"));
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Station location passed: %s"), 
        *CookingStationLocation.ToString());
    
    // Initialize the current dish data
    CurrentDishData = DishData;
    
    // Set carousel center based on cooking station location
    if (CookingStationLocation != FVector::ZeroVector)
    {
        CarouselCenter = CookingStationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Carousel center set to: %s"), 
            *CarouselCenter.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::InitializeCookingStage - Station location is zero, using default carousel center"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Cooking stage initialized for dish: %s"), 
        *CurrentDishData.DisplayName.ToString());
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Selected ingredients: %d"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Spawn the cooking implement carousel
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - About to spawn carousel at center: %s"), 
        *CarouselCenter.ToString());
    SpawnCookingCarousel();
    
    // Create ingredient buttons from the dish data
    CreateIngredientButtonsFromDishData();
    
    // Create quantity controls for existing ingredients
    CreateQuantityControlsForSelectedIngredients();
    
    // Call Blueprint event
    OnCookingStageInitialized(CurrentDishData);
}

void UPUCookingStageWidget::CreateQuantityControlsForSelectedIngredients()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating quantity controls"));
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: CurrentDishData has %d ingredient instances"), CurrentDishData.IngredientInstances.Num());
    
    // Create quantity controls for each ingredient instance
    for (const FIngredientInstance& IngredientInstance : CurrentDishData.IngredientInstances)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating control for: %s"), 
            *IngredientInstance.IngredientData.DisplayName.ToString());
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: About to create widget for instance ID: %d"), IngredientInstance.InstanceID);
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: About to call OnQuantityControlCreated Blueprint event"));
        
        // Call Blueprint event to create quantity control widget (Blueprint handles everything)
        OnQuantityControlCreated(nullptr, IngredientInstance);
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: OnQuantityControlCreated Blueprint event called"));
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Blueprint event called for instance: %d"), 
            IngredientInstance.InstanceID);
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Created %d quantity controls"), 
        CurrentDishData.IngredientInstances.Num());
}

void UPUCookingStageWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
        IngredientInstance.InstanceID);
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Received quantity change - ID: %d, Qty: %d"), 
        IngredientInstance.InstanceID, IngredientInstance.Quantity);
    
    // Update the ingredient instance in the current dish data
    for (FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == IngredientInstance.InstanceID)
        {
            UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Found matching instance, updating from qty %d to %d"), 
                Instance.Quantity, IngredientInstance.Quantity);
            Instance = IngredientInstance;
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Updated instance in dish data"));
            break;
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: About to broadcast OnDishDataChanged"));
    
    // Broadcast dish data change to update flavor/texture/dish quantity profiles
    OnDishDataChanged(CurrentDishData);
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Broadcasted dish data change"));
}

void UPUCookingStageWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Debug: Log all instances before removal
    UE_LOG(LogTemp, Display, TEXT("🍳 BEFORE REMOVAL - Total instances: %d"), CurrentDishData.IngredientInstances.Num());
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍳   Instance %d: ID=%d, Ingredient=%s, Quantity=%d"), 
            i, Instance.InstanceID, *Instance.IngredientData.IngredientName.ToString(), Instance.Quantity);
    }
    
    // Remove ONLY the specific instance id; leave other instances of the same ingredient intact
    int32 Removed = CurrentDishData.IngredientInstances.RemoveAll([InstanceID](const FIngredientInstance& Instance) {
        return Instance.InstanceID == InstanceID;
    });
    UE_LOG(LogTemp, Display, TEXT("🔍 Removed instances with ID %d: %d"), InstanceID, Removed);
    
    // Debug: Log all instances after removal
    UE_LOG(LogTemp, Display, TEXT("🍳 AFTER REMOVAL - Total instances: %d"), CurrentDishData.IngredientInstances.Num());
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍳   Instance %d: ID=%d, Ingredient=%s, Quantity=%d"), 
            i, Instance.InstanceID, *Instance.IngredientData.IngredientName.ToString(), Instance.Quantity);
    }
    
    // Broadcast dish data change to update flavor/texture/dish quantity profiles
    OnDishDataChanged(CurrentDishData);
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Broadcasted dish data change"));
    
    // Note: Widget removal is handled by the quantity control widget itself
}

int32 UPUCookingStageWidget::GenerateUniqueIngredientInstanceID() const
{
    int32 MaxId = 0;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        MaxId = FMath::Max(MaxId, Instance.InstanceID);
    }
    // Next available id
    return MaxId + 1;
}

int32 UPUCookingStageWidget::GenerateGUIDBasedInstanceID()
{
    // Generate a GUID and convert it to a unique integer
    FGuid NewGUID = FGuid::NewGuid();
    
    // Convert GUID to a unique integer using hash
    int32 UniqueID = GetTypeHash(NewGUID);
    
    // Ensure it's positive (hash can be negative)
    UniqueID = FMath::Abs(UniqueID);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 Generated GUID-based InstanceID: %d from GUID: %s"), 
        UniqueID, *NewGUID.ToString());
    
    return UniqueID;
}

void UPUCookingStageWidget::FinishCookingAndStartPlating()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Finishing cooking and starting plating"));
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Final dish has %d ingredients"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Get the dish customization component to transition to plating mode
    if (DishCustomizationComponent)
    {
        // Transition to plating stage with the completed dish data
        DishCustomizationComponent->TransitionToPlatingStage(CurrentDishData);
        
        UE_LOG(LogTemp, Display, TEXT("✅ PUCookingStageWidget::FinishCookingAndStartPlating - Transitioned to plating stage"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FinishCookingAndStartPlating - No dish customization component reference"));
    }
    
    // Destroy carousel before finishing
    DestroyCookingCarousel();
    
    // Call Blueprint event
    OnCookingStageCompleted(CurrentDishData);
    
    // Broadcast the cooking completed event
    OnCookingCompleted.Broadcast(CurrentDishData);
    
    // Remove this widget from viewport
    RemoveFromParent();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Cooking stage completed, plating mode activated"));
}

void UPUCookingStageWidget::ExitCookingStage()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ExitCookingStage - Exiting cooking stage"));
    
    // Destroy carousel before exiting
    DestroyCookingCarousel();
    
    // Broadcast the cooking stage exited event (with current dish data)
    OnCookingStageExited.Broadcast(CurrentDishData);
    
    // Remove this widget from viewport
    RemoveFromParent();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ExitCookingStage - Cooking stage exited"));
}

void UPUCookingStageWidget::ExitCustomizationMode()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ExitCustomizationMode - Exiting customization mode completely"));
    
    // Destroy carousel before exiting
    DestroyCookingCarousel();
    
    // Switch back to player camera
    SwitchToPlayerCamera();
    
    // Use the existing dish customization component to exit
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->EndCustomization();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::ExitCustomizationMode - No dish customization component available"));
        // Fallback: remove this widget from viewport
        RemoveFromParent();
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ExitCustomizationMode - Customization mode exited"));
}

void UPUCookingStageWidget::SwitchToPlayerCamera()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    // Get the player character
    AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SwitchToPlayerCamera - No player character found"));
        return;
    }
    
    // Switch back to the character's camera
    PC->SetViewTarget(Character);
    
    // Re-enable player input
    PC->SetIgnoreMoveInput(false);
    PC->SetIgnoreLookInput(false);
    
    // Hide mouse cursor
    PC->bShowMouseCursor = false;
    
    // Set input mode back to game
    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SwitchToPlayerCamera - Switched back to player camera"));
}

bool UPUCookingStageWidget::GetViewportMousePos(APlayerController* PC, FVector2D& OutViewportPos) const
{
    if (!PC)
    {
        return false;
    }

    float X = 0.0f;
    float Y = 0.0f;
    if (PC->GetMousePosition(X, Y))
    {
        OutViewportPos = FVector2D(X, Y);
        return true;
    }
    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::GetViewportMousePos - Failed to get mouse position"));
    return false;
}

int32 UPUCookingStageWidget::FindImplementUnderScreenPos(const FVector2D& ScreenPosViewport, FHitResult& OutHit) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return INDEX_NONE;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return INDEX_NONE;
    }

    // First try engine's screen hit test (uses Visibility channel)
    FHitResult Hit;
    if (PC->GetHitResultAtScreenPosition(ScreenPosViewport, ECollisionChannel::ECC_Visibility, true, Hit))
    {
        if (UPrimitiveComponent* Comp = Hit.GetComponent())
        {
            for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
            {
                const AStaticMeshActor* Actor = SpawnedCookingImplements[i];
                if (Actor)
                {
                    if (Actor->GetStaticMeshComponent() == Comp || Comp->GetOwner() == Actor)
                    {
                        OutHit = Hit;
                        // UE_LOG(LogTemp, Display, TEXT("🍳 HitTest (screen) implement %d at %s"), i, *ScreenPosViewport.ToString());
                        return i;
                    }
                }
            }
        }
    }

    // Fallback: manual line trace that works well with orthographic cameras
    FVector WorldOrigin;
    FVector WorldDirection;
    if (PC->DeprojectScreenPositionToWorld(ScreenPosViewport.X, ScreenPosViewport.Y, WorldOrigin, WorldDirection))
    {
        const float TraceDepth = 100000.0f;
        const FVector Start = WorldOrigin - WorldDirection * (TraceDepth * 0.5f);
        const FVector End = WorldOrigin + WorldDirection * (TraceDepth * 0.5f);

        TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
        ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
        ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

        TArray<AActor*> Ignore;

        FHitResult LocalHit;
        if (UKismetSystemLibrary::LineTraceSingleForObjects(World, Start, End, ObjTypes, false, Ignore, EDrawDebugTrace::None, LocalHit, true))
        {
            if (UPrimitiveComponent* Comp = LocalHit.GetComponent())
            {
                for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
                {
                    const AStaticMeshActor* Actor = SpawnedCookingImplements[i];
                    if (Actor)
                    {
                        if (Actor->GetStaticMeshComponent() == Comp || Comp->GetOwner() == Actor)
                        {
                            OutHit = LocalHit;
                            UE_LOG(LogTemp, Display, TEXT("🍳 HitTest (line) implement %d at %s"), i, *ScreenPosViewport.ToString());
                            return i;
                        }
                    }
                }
            }
        }
    }

    return INDEX_NONE;
}

FVector2D UPUCookingStageWidget::ConvertAbsoluteToViewport(const FVector2D& AbsoluteScreenPos) const
{
    // Convert absolute Slate coords to viewport-local using UWidgetLayoutLibrary geometry.
    if (UWorld* World = GetWorld())
    {
        const FGeometry ViewportGeo = UWidgetLayoutLibrary::GetViewportWidgetGeometry(World);
        return ViewportGeo.AbsoluteToLocal(AbsoluteScreenPos);
    }
    return AbsoluteScreenPos; // Fallback
}

void UPUCookingStageWidget::SetDishCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetDishCustomizationComponent - Setting dish customization component"));
    
    DishCustomizationComponent = Component;
    
    if (DishCustomizationComponent)
    {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetDishCustomizationComponent - Component reference set successfully: %s"), 
        *DishCustomizationComponent->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetDishCustomizationComponent - Component reference is NULL"));
    }
}

void UPUCookingStageWidget::SetQuantityControlContainer(UPanelWidget* Container)
{
    QuantityControlContainer = Container;
    if (QuantityControlContainer.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 QuantityControlContainer set to: %s"), *QuantityControlContainer->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ QuantityControlContainer set to null"));
    }
}

void UPUCookingStageWidget::EnableQuantityControlDrag(bool bEnabled)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::EnableQuantityControlDrag - Setting drag enabled to %s for all quantity controls"), 
        bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
    
    if (!QuantityControlContainer.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::EnableQuantityControlDrag - QuantityControlContainer is not valid"));
        return;
    }
    
    // Get all child widgets in the container
    TArray<UWidget*> ChildWidgets = QuantityControlContainer->GetAllChildren();
    int32 QuantityControlsFound = 0;
    
    for (UWidget* ChildWidget : ChildWidgets)
    {
        if (UPUIngredientQuantityControl* QuantityControl = Cast<UPUIngredientQuantityControl>(ChildWidget))
        {
            QuantityControl->SetDragEnabled(bEnabled);
            QuantityControlsFound++;
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::EnableQuantityControlDrag - Set drag enabled for quantity control: %s"), 
                *QuantityControl->GetIngredientInstance().IngredientData.DisplayName.ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::EnableQuantityControlDrag - Updated %d quantity controls"), QuantityControlsFound);
}

bool UPUCookingStageWidget::UpdateExistingQuantityControl(int32 InstanceID, const FGameplayTagContainer& NewPreparations)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Looking for quantity control with InstanceID: %d"), InstanceID);
    
    // Try to use the bound QuantityScrollBox first, then fall back to QuantityControlContainer
    UPanelWidget* ContainerToUse = nullptr;
    
    if (QuantityScrollBox)
    {
        ContainerToUse = QuantityScrollBox;
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Using bound QuantityScrollBox: %s"), *QuantityScrollBox->GetName());
    }
    else if (QuantityControlContainer.IsValid())
    {
        ContainerToUse = QuantityControlContainer.Get();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Using QuantityControlContainer: %s"), *QuantityControlContainer->GetName());
    }
    
    if (!ContainerToUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::UpdateExistingQuantityControl - No container found, trying to find quantity controls automatically"));
        
        // Try to find the container automatically by searching for quantity controls in the widget hierarchy
        TArray<UPUIngredientQuantityControl*> FoundQuantityControls;
        FindQuantityControlsInHierarchy(FoundQuantityControls);
        
        if (FoundQuantityControls.Num() > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Found %d quantity controls in hierarchy"), FoundQuantityControls.Num());
            
            // Check if any of the found quantity controls match our InstanceID
            for (UPUIngredientQuantityControl* QuantityControl : FoundQuantityControls)
            {
                if (QuantityControl && QuantityControl->GetInstanceID() == InstanceID)
                {
                    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Found existing quantity control for InstanceID: %d"), InstanceID);
                    
                    // Apply new preparations to the existing quantity control
                    TArray<FGameplayTag> PreparationTags;
                    NewPreparations.GetGameplayTagArray(PreparationTags);
                    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Applying %d new preparations"), PreparationTags.Num());
                    
                    for (const FGameplayTag& PreparationTag : PreparationTags)
                    {
                        if (!QuantityControl->GetIngredientInstance().Preparations.HasTag(PreparationTag))
                        {
                            QuantityControl->AddPreparation(PreparationTag);
                            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Added preparation: %s"), *PreparationTag.ToString());
                        }
                        else
                        {
                            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Preparation already exists: %s"), *PreparationTag.ToString());
                        }
                    }
                    
                    UE_LOG(LogTemp, Display, TEXT("✅ PUCookingStageWidget::UpdateExistingQuantityControl - Successfully updated existing quantity control"));
                    return true;
                }
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::UpdateExistingQuantityControl - No quantity controls found in hierarchy"));
        return false;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Container is valid: %s"), *ContainerToUse->GetName());
    
    // Get all child widgets in the container
    TArray<UWidget*> ChildWidgets = ContainerToUse->GetAllChildren();
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Found %d child widgets in container"), ChildWidgets.Num());
    
    for (int32 i = 0; i < ChildWidgets.Num(); i++)
    {
        UWidget* ChildWidget = ChildWidgets[i];
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Checking child widget %d: %s"), i, ChildWidget ? *ChildWidget->GetName() : TEXT("NULL"));
        
        if (UPUIngredientQuantityControl* QuantityControl = Cast<UPUIngredientQuantityControl>(ChildWidget))
        {
            int32 ControlInstanceID = QuantityControl->GetInstanceID();
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Found quantity control with InstanceID: %d (looking for: %d)"), ControlInstanceID, InstanceID);
            
            // Check if this is the quantity control we're looking for
            if (ControlInstanceID == InstanceID)
            {
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Found existing quantity control for InstanceID: %d"), InstanceID);
                
                // Apply new preparations to the existing quantity control
                TArray<FGameplayTag> PreparationTags;
                NewPreparations.GetGameplayTagArray(PreparationTags);
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Applying %d new preparations"), PreparationTags.Num());
                
                for (const FGameplayTag& PreparationTag : PreparationTags)
                {
                    if (!QuantityControl->GetIngredientInstance().Preparations.HasTag(PreparationTag))
                    {
                        QuantityControl->AddPreparation(PreparationTag);
                        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Added preparation: %s"), *PreparationTag.ToString());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Preparation already exists: %s"), *PreparationTag.ToString());
                    }
                }
                
                UE_LOG(LogTemp, Display, TEXT("✅ PUCookingStageWidget::UpdateExistingQuantityControl - Successfully updated existing quantity control"));
                return true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateExistingQuantityControl - Child widget %d is not a quantity control"), i);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("⚠️ PUCookingStageWidget::UpdateExistingQuantityControl - No existing quantity control found for InstanceID: %d"), InstanceID);
    return false;
}

void UPUCookingStageWidget::FindQuantityControlsInHierarchy(TArray<UPUIngredientQuantityControl*>& OutQuantityControls)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindQuantityControlsInHierarchy - Searching for quantity controls in widget hierarchy"));
    
    // Clear the output array
    OutQuantityControls.Empty();
    
    // Recursively search through all child widgets
    FindQuantityControlsRecursive(this, OutQuantityControls);
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindQuantityControlsInHierarchy - Found %d quantity controls"), OutQuantityControls.Num());
}

void UPUCookingStageWidget::FindQuantityControlsRecursive(UWidget* ParentWidget, TArray<UPUIngredientQuantityControl*>& OutQuantityControls)
{
    if (!ParentWidget)
    {
        return;
    }
    
    // Check if this widget is a quantity control
    if (UPUIngredientQuantityControl* QuantityControl = Cast<UPUIngredientQuantityControl>(ParentWidget))
    {
        OutQuantityControls.Add(QuantityControl);
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindQuantityControlsRecursive - Found quantity control: %s (InstanceID: %d)"), 
            *QuantityControl->GetName(), QuantityControl->GetInstanceID());
    }
    
    // Recursively search child widgets
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(ParentWidget))
    {
        TArray<UWidget*> ChildWidgets = PanelWidget->GetAllChildren();
        for (UWidget* ChildWidget : ChildWidgets)
        {
            FindQuantityControlsRecursive(ChildWidget, OutQuantityControls);
        }
    }
}

void UPUCookingStageWidget::OnIngredientDroppedOnImplement(int32 ImplementIndex, const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnIngredientDroppedOnImplement - Ingredient %s dropped on implement %d (ID: %d, Qty: %d)"), 
        *IngredientTag.ToString(), ImplementIndex, InstanceID, Quantity);
    
    // Validate implement index
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::OnIngredientDroppedOnImplement - Invalid implement index: %d"), ImplementIndex);
        return;
    }
    
    // Get the cooking implement
    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (!ImplementActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::OnIngredientDroppedOnImplement - No implement actor at index %d"), ImplementIndex);
        return;
    }
    
    // Get implement-specific preparation tags
    const FGameplayTagContainer AllowedTags = GetPreparationTagsForImplement(ImplementIndex);
    
    // First, try to update existing quantity control if it exists
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: About to call UpdateExistingQuantityControl for InstanceID: %d"), InstanceID);
    bool bFoundExisting = UpdateExistingQuantityControl(InstanceID, AllowedTags);
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: UpdateExistingQuantityControl returned: %s"), bFoundExisting ? TEXT("TRUE") : TEXT("FALSE"));
    
    if (bFoundExisting)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnIngredientDroppedOnImplement - Updated existing quantity control for InstanceID: %d"), InstanceID);
        
        // Update the dish data to reflect the new preparations
        for (FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
        {
            if (Instance.InstanceID == InstanceID)
            {
                // Apply the new preparations to the dish data as well
                TArray<FGameplayTag> PreparationTags;
                AllowedTags.GetGameplayTagArray(PreparationTags);
                
                for (const FGameplayTag& PreparationTag : PreparationTags)
                {
                    if (!Instance.Preparations.HasTag(PreparationTag))
                    {
                        Instance.Preparations.AddTag(PreparationTag);
                        UE_LOG(LogTemp, Display, TEXT("🍳 Updated dish data with preparation: %s"), *PreparationTag.ToString());
                    }
                }
                
                // Update the ingredient data active preparations
                Instance.IngredientData.ActivePreparations = Instance.Preparations;
                break;
            }
        }
        
        // Broadcast dish data change to update radar graphs
        OnDishDataChanged(CurrentDishData);
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnIngredientDroppedOnImplement - Updated dish data and broadcasted changes"));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: No existing quantity control found, creating new one"));
        
        // Create quantity control from dropped ingredient
        CreateQuantityControlFromDroppedIngredient(IngredientData, InstanceID, Quantity);
        
        UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: CreateQuantityControlFromDroppedIngredient completed"));
    }
    
    // Rotate carousel to bring selected implement to front
    RotateCarouselToSelection(ImplementIndex);
    
    const FString ImplementName = CookingImplementNames.IsValidIndex(ImplementIndex)
        ? CookingImplementNames[ImplementIndex]
        : FString::Printf(TEXT("Implement %d"), ImplementIndex);
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnIngredientDroppedOnImplement - Applied %s to %s"), 
        *IngredientData.DisplayName.ToString(), *ImplementName);
    
    // TODO: Implement additional cooking logic here
    // This is where you would:
    // 1. Apply the ingredient to the cooking implement
    // 2. Update the dish data
    // 3. Show cooking effects/animations
    // 4. Update the cooking progress
}

bool UPUCookingStageWidget::IsDragOverImplement(int32 ImplementIndex, const FVector2D& ScreenPosition) const
{
    // Validate implement index
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        return false;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }
    
    // Get the cooking implement
    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (!ImplementActor)
    {
        return false;
    }
    
    UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
    if (!MeshComponent)
    {
        return false;
    }
    
    // Get the center of the mesh bounds
    FVector MeshCenter = MeshComponent->Bounds.Origin;
    
    // Project the mesh center to screen space
    FVector2D ScreenLocation;
    if (PC->ProjectWorldLocationToScreen(MeshCenter, ScreenLocation))
    {
        // Calculate distance from drag position to mesh center on screen
        float Distance = FVector2D::Distance(ScreenPosition, ScreenLocation);
        
        // Use the same radius as hover detection for consistency
        return Distance < HoverDetectionRadius;
    }
    
    return false;
}

// Carousel Functions
void UPUCookingStageWidget::SpawnCookingCarousel()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Spawning cooking implement carousel"));
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Carousel center: %s, Radius: %.2f, Height: %.2f"), 
        *CarouselCenter.ToString(), CarouselRadius, CarouselHeight);
    
    // Clear any existing carousel
    DestroyCookingCarousel();
    
    if (CookingImplementMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SpawnCookingCarousel - No cooking implement meshes defined"));
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::SpawnCookingCarousel - No world available"));
        return;
    }
    
    // Spawn cooking implement actors
    for (int32 i = 0; i < CookingImplementMeshes.Num(); ++i)
    {
        if (UStaticMesh* Mesh = CookingImplementMeshes[i])
        {
            // Calculate position for this implement
            FVector Position = CalculateImplementPosition(i, CookingImplementMeshes.Num());
            FRotator Rotation = CalculateImplementRotation(i, CookingImplementMeshes.Num());
            
            // Spawn the actor
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            AStaticMeshActor* ImplementActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Position, Rotation, SpawnParams);
            
            if (ImplementActor)
            {
                // Set the static mesh
                UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
                if (MeshComponent)
                {
                    // Set mobility to movable so we can change the mesh at runtime
                    MeshComponent->SetMobility(EComponentMobility::Movable);
                    
                    MeshComponent->SetStaticMesh(Mesh);
                    
                    // Make the mesh 2 times bigger
                    MeshComponent->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f));
                    
                    // Set up collision for interaction
                    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                    MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                    MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
                }
                
                // Add to our array
                SpawnedCookingImplements.Add(ImplementActor);
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Spawned implement %d at position %s"), 
                    i, *Position.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::SpawnCookingCarousel - Failed to spawn implement %d"), i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SpawnCookingCarousel - Invalid mesh at index %d"), i);
        }
    }
    
    bCarouselSpawned = true;
    SelectedImplementIndex = 0;
    
    // Set up hover detection for the carousel
    SetupHoverDetection();
    
    // Set up click detection for the carousel
    SetupClickDetection();
    
    // Set up initial front indicator
    UpdateFrontIndicator();
    
    // Debug collision detection
    // DebugCollisionDetection(); // Debug visualization disabled
    
    // Set up multi-hit collision detection for orthographic cameras
    // SetupMultiHitCollisionDetection(); // Debug visualization disabled
    
    // Show debug spheres to visualize hover detection areas
    // ShowDebugSpheres(); // Debug visualization disabled
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Carousel spawned with %d implements"), 
        SpawnedCookingImplements.Num());
}

void UPUCookingStageWidget::DestroyCookingCarousel()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DestroyCookingCarousel - Destroying cooking implement carousel"));
    
    // Destroy all spawned actors
    for (AStaticMeshActor* Actor : SpawnedCookingImplements)
    {
        if (Actor && IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    
    SpawnedCookingImplements.Empty();
    bCarouselSpawned = false;
    SelectedImplementIndex = 0;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DestroyCookingCarousel - Carousel destroyed"));
}

void UPUCookingStageWidget::SelectCookingImplement(int32 ImplementIndex)
{
    if (!bCarouselSpawned || SpawnedCookingImplements.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SelectCookingImplement - Carousel not spawned or no implements available"));
        return;
    }
    
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SelectCookingImplement - Invalid implement index: %d"), ImplementIndex);
        return;
    }
    
    if (SelectedImplementIndex == ImplementIndex)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SelectCookingImplement - Implement %d already selected"), ImplementIndex);
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SelectCookingImplement - Selecting implement %d (was %d)"), 
        ImplementIndex, SelectedImplementIndex);
    
    SelectedImplementIndex = ImplementIndex;
    
    // Rotate carousel to bring selected item to front
    RotateCarouselToSelection(ImplementIndex);
    
    // Broadcast event
    OnCookingImplementSelected.Broadcast(ImplementIndex);
}

void UPUCookingStageWidget::PositionCookingImplements()
{
    if (!bCarouselSpawned || SpawnedCookingImplements.Num() == 0)
    {
        return;
    }
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        if (AStaticMeshActor* Actor = SpawnedCookingImplements[i])
        {
            FVector Position = CalculateImplementPosition(i, SpawnedCookingImplements.Num());
            FRotator Rotation = CalculateImplementRotation(i, SpawnedCookingImplements.Num());
            
            Actor->SetActorLocation(Position);
            Actor->SetActorRotation(Rotation);
        }
    }
}

void UPUCookingStageWidget::RotateCarouselToSelection(int32 TargetIndex)
{
    if (TargetIndex < 0 || TargetIndex >= SpawnedCookingImplements.Num())
    {
        return;
    }

    // Calculate the target rotation angle
    // We want the selected item to be at the front (0 degrees)
    float AngleStep = 360.0f / SpawnedCookingImplements.Num();
    float TargetAngle = -(TargetIndex * AngleStep); // Negative because we want to rotate the carousel, not the item
    
    // Calculate the shortest rotation path
    float CurrentAngle = FMath::Fmod(CurrentRotationAngle, 360.0f);
    float TargetAngleNormalized = FMath::Fmod(TargetAngle, 360.0f);
    
    // Debug: Log the angle calculations
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Angle calculation: CurrentAngle=%.2f, TargetAngle=%.2f, TargetAngleNormalized=%.2f"), 
        CurrentAngle, TargetAngle, TargetAngleNormalized);
    
    // Calculate the shortest path difference
    float AngleDifference = TargetAngleNormalized - CurrentAngle;
    
    // Normalize to get the shortest path (can be positive or negative)
    if (AngleDifference > 180.0f)
    {
        // If difference is more than 180°, go the other way (shorter path)
        AngleDifference -= 360.0f;
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Shortest path: AngleDifference=%.2f (adjusted for shorter path)"), AngleDifference);
    }
    else if (AngleDifference < -180.0f)
    {
        // If difference is less than -180°, go the other way (shorter path)
        AngleDifference += 360.0f;
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Shortest path: AngleDifference=%.2f (adjusted for shorter path)"), AngleDifference);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Shortest path: AngleDifference=%.2f (no adjustment needed)"), AngleDifference);
    }
    
    // Set the target rotation
    TargetRotationAngle = CurrentRotationAngle + AngleDifference;

    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotating carousel to bring implement %d to front (current: %.2f, target: %.2f, difference: %.2f)"), 
        TargetIndex, CurrentRotationAngle, TargetRotationAngle, TargetRotationAngle - CurrentRotationAngle);
    
    // Debug: Log the rotation path details
    float RotationDistance = TargetRotationAngle - CurrentRotationAngle;
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotation distance: %.2f degrees (%.2f steps)"), 
        RotationDistance, RotationDistance / (360.0f / SpawnedCookingImplements.Num()));

    // Start rotation
    bIsRotating = true;
    RotationProgress = 0.0f;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotation started: bIsRotating=%s, Progress=%.3f"), 
        bIsRotating ? TEXT("TRUE") : TEXT("FALSE"), RotationProgress);
}

FVector UPUCookingStageWidget::CalculateImplementPosition(int32 Index, int32 TotalCount) const
{
    if (TotalCount <= 0)
    {
        return CarouselCenter;
    }
    
    // Calculate angle for this implement
    float AngleInRadians = FMath::DegreesToRadians((360.0f / TotalCount) * Index);
    
    // Calculate position on circle
    float X = CarouselCenter.X + (CarouselRadius * FMath::Cos(AngleInRadians));
    float Y = CarouselCenter.Y + (CarouselRadius * FMath::Sin(AngleInRadians));
    float Z = CarouselCenter.Z + CarouselHeight;
    
    FVector Position = FVector(X, Y, Z);
    
    return Position;
}

FRotator UPUCookingStageWidget::CalculateImplementRotation(int32 Index, int32 TotalCount) const
{
    if (TotalCount <= 0)
    {
        return FRotator::ZeroRotator;
    }
    
    // Calculate angle for this implement
    float AngleInDegrees = (360.0f / TotalCount) * Index;
    
    // Make implements face toward the center of the carousel
    // Add 180 degrees so they face inward
    float Yaw = AngleInDegrees + 180.0f;
    
    return FRotator(0.0f, Yaw, 0.0f);
}

void UPUCookingStageWidget::SetCarouselCenter(const FVector& NewCenter)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCarouselCenter - Setting carousel center to: %s"), 
        *NewCenter.ToString());
    
    CarouselCenter = NewCenter;
    
    // If carousel is already spawned, reposition it
    if (bCarouselSpawned)
    {
        PositionCookingImplements();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCarouselCenter - Repositioned existing carousel"));
    }
}

void UPUCookingStageWidget::SetCookingStationLocation(const FVector& StationLocation)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Setting station location: %s"), 
        *StationLocation.ToString());
    
    // Set carousel center based on cooking station location
    CarouselCenter = StationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Carousel center set to: %s"), 
        *CarouselCenter.ToString());
    
    // If carousel is already spawned, reposition it
    if (bCarouselSpawned)
    {
        PositionCookingImplements();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Repositioned existing carousel"));
    }
}

void UPUCookingStageWidget::FindAndSetNearestCookingStation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No world available"));
        return;
    }
    
    // Get player character location
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No player controller found"));
        return;
    }
    
    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No player pawn found"));
        return;
    }
    
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    
    // Find all actors with "CookingStation" tag
    TArray<AActor*> CookingStations;
    UGameplayStatics::GetAllActorsWithTag(World, FName("CookingStation"), CookingStations);
    
    if (CookingStations.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No cooking stations found with 'CookingStation' tag"));
        return;
    }
    
    // Find the nearest cooking station
    AActor* NearestStation = nullptr;
    float NearestDistance = MAX_FLT;
    
    for (AActor* Station : CookingStations)
    {
        if (Station)
        {
            float Distance = FVector::Dist(PlayerLocation, Station->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestStation = Station;
            }
        }
    }
    
    if (NearestStation)
    {
        FVector StationLocation = NearestStation->GetActorLocation();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Found nearest cooking station at: %s"), 
            *StationLocation.ToString());
        
        // Set carousel center based on cooking station location
        CarouselCenter = StationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
        
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Carousel center set to: %s"), 
            *CarouselCenter.ToString());
        
        // If carousel is already spawned, reposition it
        if (bCarouselSpawned)
        {
            PositionCookingImplements();
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Repositioned existing carousel"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No valid cooking station found"));
    }
}

// Hover Detection Functions
void UPUCookingStageWidget::SetupHoverDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Setting up hover detection for %d implements"), 
        SpawnedCookingImplements.Num());

    // Clear existing hover text components
    if (HoverTextComponents.Num() > 0)
    {
        for (UWidgetComponent* TextComponent : HoverTextComponents)
        {
            if (TextComponent)
            {
                TextComponent->DestroyComponent();
            }
        }
    }
    HoverTextComponents.Empty();

    // Set up hover detection for each implement
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Enable mouse over events with better collision for orthographic cameras
                MeshComponent->SetNotifyRigidBodyCollision(false);
                MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
                
                // Make sure the collision bounds are large enough for mouse detection
                MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
                
                // For orthographic cameras, we need to ensure collision bounds are large enough
                // and that the collision doesn't block other objects behind
                MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

                // Bind hover events
                MeshComponent->OnBeginCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverBegin);
                MeshComponent->OnEndCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverEnd);

                // Bind click events
                MeshComponent->OnClicked.AddDynamic(this, &UPUCookingStageWidget::OnImplementClicked);

                // Create hover text component
                if (HoverTextWidgetClass)
                {
                    UWidgetComponent* TextComponent = NewObject<UWidgetComponent>(ImplementActor);
                    if (TextComponent)
                    {
                        TextComponent->SetupAttachment(MeshComponent);
                        TextComponent->SetWidgetClass(HoverTextWidgetClass);
                        
                        // For orthographic cameras, use larger draw size
                        TextComponent->SetDrawSize(FVector2D(300.0f, 100.0f));
                        TextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HoverTextOffset));
                        TextComponent->SetVisibility(false);
                        
                        // Important for orthographic cameras - make it always face the camera
                        TextComponent->SetWidgetSpace(EWidgetSpace::World);
                        TextComponent->SetPivot(FVector2D(0.5f, 0.5f));
                        
                        TextComponent->RegisterComponent();

                        HoverTextComponents.Add(TextComponent);
                        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Created hover text component for implement %d (orthographic optimized)"), i);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetupHoverDetection - Failed to create text component for implement %d"), i);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetupHoverDetection - No HoverTextWidgetClass set for implement %d"), i);
                }
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Hover detection setup complete"));
}

void UPUCookingStageWidget::SetupClickDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupClickDetection - Setting up click detection for %d implements"), 
        SpawnedCookingImplements.Num());

    // Click detection is already set up in SetupHoverDetection
    // This function is here for future expansion if needed
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupClickDetection - Click detection setup complete"));
}

void UPUCookingStageWidget::OnImplementHoverBegin(UPrimitiveComponent* TouchedComponent)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(TouchedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementHoverBegin - Implement %d hover begin"), ImplementIndex);
        
        HoveredImplementIndex = ImplementIndex;
        ShowHoverText(ImplementIndex, true);
        ApplyHoverVisualEffect(ImplementIndex, true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::OnImplementHoverBegin - Could not find implement index for component: %s"), 
            TouchedComponent ? *TouchedComponent->GetName() : TEXT("NULL"));
    }
}

void UPUCookingStageWidget::OnImplementHoverEnd(UPrimitiveComponent* TouchedComponent)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(TouchedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementHoverEnd - Implement %d hover end"), ImplementIndex);
        
        if (HoveredImplementIndex == ImplementIndex)
        {
            HoveredImplementIndex = -1;
        }
        ShowHoverText(ImplementIndex, false);
        ApplyHoverVisualEffect(ImplementIndex, false);
    }
}

int32 UPUCookingStageWidget::GetImplementIndexFromComponent(UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return -1;
    }

    // Find which implement this component belongs to
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor && ImplementActor->GetStaticMeshComponent() == Component)
        {
            return i;
        }
    }

    return -1;
}

void UPUCookingStageWidget::OnImplementClicked(UPrimitiveComponent* ClickedComponent, FKey ButtonPressed)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(ClickedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementClicked - Implement %d clicked with button: %s"), 
            ImplementIndex, *ButtonPressed.ToString());
        
        SelectCookingImplement(ImplementIndex);
    }
}



void UPUCookingStageWidget::UpdateCarouselRotation(float DeltaTime)
{
    if (!bIsRotating)
    {
        return;
    }

    // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Rotating: Progress=%.3f, CurrentAngle=%.2f, TargetAngle=%.2f"),
    //     RotationProgress, CurrentRotationAngle, TargetRotationAngle);

    // Update rotation progress
    float ProgressIncrement = DeltaTime * RotationSpeed;
    RotationProgress += ProgressIncrement;
    
    // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Progress increment: %.4f, Total progress: %.3f"),
    //     ProgressIncrement, RotationProgress);
    
    if (RotationProgress >= 1.0f)
    {
        // Rotation complete
        RotationProgress = 1.0f;
        bIsRotating = false;
        CurrentRotationAngle = TargetRotationAngle;
        
        // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Carousel rotation complete"));
    }
    else
    {
        // Interpolate rotation
        float Alpha = FMath::SmoothStep(0.0f, 1.0f, RotationProgress);
        CurrentRotationAngle = FMath::Lerp(CurrentRotationAngle, TargetRotationAngle, Alpha);
    }

    // Apply rotation to all implements
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            // Calculate the base position for this implement
            FVector BasePosition = CalculateImplementPosition(i, SpawnedCookingImplements.Num());
            
            // Apply carousel rotation
            FVector RotatedPosition = CarouselCenter + FQuat(FVector::UpVector, FMath::DegreesToRadians(CurrentRotationAngle))
                .RotateVector(BasePosition - CarouselCenter);
            
            // Set the new position
            ImplementActor->SetActorLocation(RotatedPosition);
            
            // Update rotation to face outward
            FRotator BaseRotation = CalculateImplementRotation(i, SpawnedCookingImplements.Num());
            FRotator RotatedRotation = BaseRotation + FRotator(0.0f, CurrentRotationAngle, 0.0f);
            ImplementActor->SetActorRotation(RotatedRotation);
            
            // Debug logging removed to reduce spam
        }
    }

    // Update front indicator after rotation
    UpdateFrontIndicator();
}

void UPUCookingStageWidget::ApplySelectionVisualEffect(int32 ImplementIndex, bool bApply)
{
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        return;
    }

    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (ImplementActor)
    {
        UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
        if (MeshComponent)
        {
            if (bApply)
            {
                // Apply selection effect - gentle up/down bobbing
                // We'll implement this with a simple scale effect for now
                FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                MeshComponent->SetRelativeScale3D(CurrentScale * 1.2f); // Make selected item 20% bigger
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplySelectionVisualEffect - Applied selection effect to implement %d"), ImplementIndex);
            }
            else
            {
                // Remove selection effect
                FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                MeshComponent->SetRelativeScale3D(CurrentScale / 1.2f); // Restore original scale
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplySelectionVisualEffect - Removed selection effect from implement %d"), ImplementIndex);
            }
        }
    }
}

void UPUCookingStageWidget::ShowHoverText(int32 ImplementIndex, bool bShow)
{
    // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ShowHoverText - Attempting to %s hover text for implement %d (Total components: %d)"), 
    //     bShow ? TEXT("show") : TEXT("hide"), ImplementIndex, HoverTextComponents.Num());
    
    if (ImplementIndex >= 0 && ImplementIndex < HoverTextComponents.Num())
    {
        UWidgetComponent* TextComponent = HoverTextComponents[ImplementIndex];
        if (TextComponent)
        {
            TextComponent->SetVisibility(bShow);
            // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ShowHoverText - %s hover text for implement %d"), 
            //     bShow ? TEXT("Showing") : TEXT("Hiding"), ImplementIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::ShowHoverText - Text component is null for implement %d"), ImplementIndex);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::ShowHoverText - Invalid implement index %d (valid range: 0-%d)"), 
            ImplementIndex, HoverTextComponents.Num() - 1);
    }
}

void UPUCookingStageWidget::ApplyHoverVisualEffect(int32 ImplementIndex, bool bApply)
{
    if (ImplementIndex >= 0 && ImplementIndex < SpawnedCookingImplements.Num())
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                if (bApply)
                {
                    // For orthographic cameras, use a more visible approach
                    // Scale up the mesh slightly to create a "glow" effect
                    FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                    MeshComponent->SetRelativeScale3D(CurrentScale * 1.1f);
                    
                    // Also try custom depth (may work better with orthographic)
                    MeshComponent->SetRenderCustomDepth(true);
                    MeshComponent->SetCustomDepthStencilValue(1);
                    
                    // Set material parameter for glow
                    MeshComponent->SetScalarParameterValueOnMaterials(TEXT("GlowIntensity"), HoverGlowIntensity);
                    
                    // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplyHoverVisualEffect - Applied hover effect to implement %d (scaled up)"), ImplementIndex);
                }
                else
                {
                    // Restore original scale
                    FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                    MeshComponent->SetRelativeScale3D(CurrentScale / 1.1f);
                    
                    // Remove custom depth
                    MeshComponent->SetRenderCustomDepth(false);
                    MeshComponent->SetScalarParameterValueOnMaterials(TEXT("GlowIntensity"), 0.0f);
                    
                    // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplyHoverVisualEffect - Removed hover effect from implement %d (scaled down)"), ImplementIndex);
                }
            }
        }
    }
}

void UPUCookingStageWidget::UpdateFrontIndicator()
{
    int32 FrontIndex = GetFrontImplementIndex();
    
    // Reset all implements to normal scale and height
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Reset to base scale (2.0f as set in SpawnCookingCarousel)
                MeshComponent->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
                
                // Reset to base height
                FVector CurrentLocation = ImplementActor->GetActorLocation();
                CurrentLocation.Z = CarouselCenter.Z + CarouselHeight;
                ImplementActor->SetActorLocation(CurrentLocation);
            }
        }
    }
    
    // Apply front indicator to the front item
    if (FrontIndex >= 0 && FrontIndex < SpawnedCookingImplements.Num())
    {
        AStaticMeshActor* FrontActor = SpawnedCookingImplements[FrontIndex];
        if (FrontActor)
        {
            UStaticMeshComponent* MeshComponent = FrontActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Scale up the front item
                MeshComponent->SetRelativeScale3D(FVector(2.0f * FrontItemScale, 2.0f * FrontItemScale, 2.0f * FrontItemScale));
                
                // Raise the front item
                FVector CurrentLocation = FrontActor->GetActorLocation();
                CurrentLocation.Z += FrontItemHeight;
                FrontActor->SetActorLocation(CurrentLocation);
                
                // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateFrontIndicator - Front item is implement %d (scale: %.2f, height: %.2f)"), 
                //     FrontIndex, FrontItemScale, FrontItemHeight);
            }
        }
    }
}

int32 UPUCookingStageWidget::GetFrontImplementIndex() const
{
    if (SpawnedCookingImplements.Num() == 0)
    {
        return -1;
    }
    
    // The front item is the one at 0 degrees (facing the camera)
    // We need to calculate which implement is currently at the front based on the carousel rotation
    
    // Calculate the angle of each implement relative to the front
    float AngleStep = 360.0f / SpawnedCookingImplements.Num();
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        // Calculate the current angle of this implement
        float ImplementAngle = (i * AngleStep) + CurrentRotationAngle;
        
        // Normalize to 0-360 range
        ImplementAngle = FMath::Fmod(ImplementAngle + 360.0f, 360.0f);
        
        // Check if this implement is at the front (within a small tolerance)
        if (FMath::Abs(ImplementAngle) < 10.0f || FMath::Abs(ImplementAngle - 360.0f) < 10.0f)
        {
            return i;
        }
    }
    
    // If no exact match, return the closest one
    float ClosestAngle = 360.0f;
    int32 ClosestIndex = 0;
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        float ImplementAngle = (i * AngleStep) + CurrentRotationAngle;
        ImplementAngle = FMath::Fmod(ImplementAngle + 360.0f, 360.0f);
        
        float DistanceToFront = FMath::Min(FMath::Abs(ImplementAngle), FMath::Abs(ImplementAngle - 360.0f));
        if (DistanceToFront < ClosestAngle)
        {
            ClosestAngle = DistanceToFront;
            ClosestIndex = i;
        }
    }
    
    return ClosestIndex;
}

void UPUCookingStageWidget::DebugCollisionDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DebugCollisionDetection - Checking collision setup for %d implements"), 
        SpawnedCookingImplements.Num());

    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                FVector Location = ImplementActor->GetActorLocation();
                FVector Bounds = MeshComponent->Bounds.BoxExtent;
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DebugCollisionDetection - Implement %d: Location=%s, Bounds=%s, CollisionEnabled=%s"), 
                    i, *Location.ToString(), *Bounds.ToString(), 
                    MeshComponent->GetCollisionEnabled() == ECollisionEnabled::QueryOnly ? TEXT("QueryOnly") : TEXT("Other"));
            }
        }
    }
} 

void UPUCookingStageWidget::SetupMultiHitCollisionDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupMultiHitCollisionDetection - Setting up multi-hit collision detection"));
    
    // For orthographic cameras, we need to disable the automatic collision detection
    // and implement our own raycast system that can detect all objects
    for (AStaticMeshActor* ImplementActor : SpawnedCookingImplements)
    {
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Disable automatic cursor over events for orthographic cameras
                MeshComponent->OnBeginCursorOver.RemoveAll(this);
                MeshComponent->OnEndCursorOver.RemoveAll(this);
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupMultiHitCollisionDetection - Disabled automatic collision detection for orthographic camera"));
            }
        }
    }
}

void UPUCookingStageWidget::OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // For orthographic cameras, we need to implement custom hover detection
    UpdateHoverDetection();
}

void UPUCookingStageWidget::UpdateHoverDetection()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    // Get mouse position in screen space
    FVector2D MousePosition;
    PC->GetMousePosition(MousePosition.X, MousePosition.Y);
    
    // Find which implement is closest to the mouse using screen-space distance
    float ClosestDistance = HoverDetectionRadius; // Use configurable hover radius
    int32 NewHoveredIndex = -1;
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (!ImplementActor)
        {
            continue;
        }
        
        // Get the mesh component's bounds center
        UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
        if (!MeshComponent)
        {
            continue;
        }
        
        // Get the center of the mesh bounds
        FVector MeshCenter = MeshComponent->Bounds.Origin;
        
        // Project the mesh center to screen space
        FVector2D ScreenLocation;
        if (PC->ProjectWorldLocationToScreen(MeshCenter, ScreenLocation))
        {
            // Calculate distance from mouse to mesh center on screen
            float Distance = FVector2D::Distance(MousePosition, ScreenLocation);
            
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                NewHoveredIndex = i;
            }
        }
    }
    
    // Update hover state
    if (NewHoveredIndex != HoveredImplementIndex)
    {
        // Clear previous hover
        if (HoveredImplementIndex >= 0)
        {
            ShowHoverText(HoveredImplementIndex, false);
            ApplyHoverVisualEffect(HoveredImplementIndex, false);
        }
        
        // Set new hover
        if (NewHoveredIndex >= 0)
        {
            HoveredImplementIndex = NewHoveredIndex;
            ShowHoverText(HoveredImplementIndex, true);
            ApplyHoverVisualEffect(HoveredImplementIndex, true);
            
            // UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateHoverDetection - Hovering over implement %d (distance: %.1f)"), HoveredImplementIndex, ClosestDistance);
        }
        else
        {
            HoveredImplementIndex = -1;
        }
    }
}

bool UPUCookingStageWidget::IsPointInCarouselBounds(const FVector2D& ScreenPoint) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }
    
    // Convert carousel center to screen position
    FVector2D CarouselScreenLocation;
    if (PC->ProjectWorldLocationToScreen(CarouselCenter, CarouselScreenLocation))
    {
        // Check if mouse is within carousel radius (converted to screen space)
        float Distance = FVector2D::Distance(ScreenPoint, CarouselScreenLocation);
        return Distance < 300.0f; // Approximate carousel radius in screen space
    }
    
    return false;
}

void UPUCookingStageWidget::HandleMouseClick()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    // Check if left mouse button is pressed
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        // Get mouse position in screen space
        FVector2D MousePosition;
        PC->GetMousePosition(MousePosition.X, MousePosition.Y);
        
        // Find which implement is closest to the mouse using screen-space distance
        float ClosestDistance = ClickDetectionRadius; // Use configurable click radius
        int32 ClosestIndex = -1;
        
        for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
        {
            AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
            if (!ImplementActor)
            {
                continue;
            }
            
            // Get the mesh component's bounds center
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (!MeshComponent)
            {
                continue;
            }
            
            // Get the center of the mesh bounds
            FVector MeshCenter = MeshComponent->Bounds.Origin;
            
            // Project the mesh center to screen space
            FVector2D ScreenLocation;
            if (PC->ProjectWorldLocationToScreen(MeshCenter, ScreenLocation))
            {
                // Calculate distance from mouse to mesh center on screen
                float Distance = FVector2D::Distance(MousePosition, ScreenLocation);
                
                if (Distance < ClosestDistance)
                {
                    ClosestDistance = Distance;
                    ClosestIndex = i;
                }
            }
        }
        
        if (ClosestIndex >= 0)
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::HandleMouseClick - Clicked on implement %d (distance: %.1f)"), ClosestIndex, ClosestDistance);
            SelectCookingImplement(ClosestIndex);
        }
    }
}

bool UPUCookingStageWidget::IsMouseOverImplement(int32 ImplementIndex) const
{
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        return false;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }
    
    // Get mouse position in screen space
    FVector2D MousePosition;
    PC->GetMousePosition(MousePosition.X, MousePosition.Y);
    
    // Get the specific implement we're checking
    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (!ImplementActor)
    {
        return false;
    }
    
    UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
    if (!MeshComponent)
    {
        return false;
    }
    
    // Get the center of the mesh bounds
    FVector MeshCenter = MeshComponent->Bounds.Origin;
    
    // Project the mesh center to screen space
    FVector2D ScreenLocation;
    if (PC->ProjectWorldLocationToScreen(MeshCenter, ScreenLocation))
    {
        // Calculate distance from mouse to mesh center on screen
        float Distance = FVector2D::Distance(MousePosition, ScreenLocation);
        
        // Check if mouse is within hover radius
        return Distance < HoverDetectionRadius; // Use configurable hover radius
    }
    
    return false;
} 

void UPUCookingStageWidget::ShowDebugSpheres()
{
    // Use Unreal's built-in debug drawing instead
    DrawDebugHoverArea();
}

void UPUCookingStageWidget::HideDebugSpheres()
{
    // Clear debug drawing
    UWorld* World = GetWorld();
    if (World)
    {
        FlushDebugStrings(World);
        FlushPersistentDebugLines(World);
    }
}

void UPUCookingStageWidget::ToggleDebugVisualization()
{
    static bool bDebugEnabled = false;
    bDebugEnabled = !bDebugEnabled;
    
    if (bDebugEnabled)
    {
        ShowDebugSpheres();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ToggleDebugVisualization - Debug visualization ENABLED"));
    }
    else
    {
        HideDebugSpheres();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ToggleDebugVisualization - Debug visualization DISABLED"));
    }
}

void UPUCookingStageWidget::DrawDebugHoverArea()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // Draw debug circles around each implement to show hover detection area
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (!ImplementActor)
        {
            continue;
        }
        
        // Use the actor location with a simple offset
        FVector WorldLocation = ImplementActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
        
        // Draw a debug circle to represent the hover detection area
        // Convert 150 pixels to world space (approximate)
        float WorldRadius = 75.0f; // Increased to match the larger hover radius
        
        // Draw a circle around the implement
        DrawDebugCircle(World, WorldLocation, WorldRadius, 32, FColor::Red, false, -1.0f, 0, 2.0f);
        
        // Draw a line from the implement to show its center
        DrawDebugLine(World, WorldLocation, WorldLocation + FVector(0, 0, 50), FColor::Yellow, false, -1.0f, 0, 2.0f);
        
        // Add debug text
        FString DebugText = FString::Printf(TEXT("Implement %d"), i);
        DrawDebugString(World, WorldLocation + FVector(0, 0, 60), DebugText, nullptr, FColor::White, 0.0f, true);
    }
}

void UPUCookingStageWidget::CreateIngredientButtonsFromDishData()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Creating ingredient buttons from dish data"));
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Current button count before clearing: %d"), CreatedIngredientButtons.Num());
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Buttons already created: %s"), bIngredientButtonsCreated ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Check if buttons were already created
    if (bIngredientButtonsCreated)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateIngredientButtonsFromDishData - Buttons already created, skipping to prevent duplicates"));
        return;
    }
    
    // Check if we have a valid world context
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::CreateIngredientButtonsFromDishData - No world context available"));
        return;
    }
    
    // Clear any existing buttons
    CreatedIngredientButtons.Empty();
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Cleared existing buttons"));
    
    // Check if we have dish data
    if (CurrentDishData.IngredientInstances.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateIngredientButtonsFromDishData - No ingredient instances in dish data"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Found %d ingredient instances"), CurrentDishData.IngredientInstances.Num());
    
    // Debug: Log each ingredient instance
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        UE_LOG(LogTemp, Display, TEXT("🍳 Ingredient %d: Tag=%s, Name=%s, ID=%d, Qty=%d"), 
            i, 
            *Instance.IngredientData.IngredientTag.ToString(),
            *Instance.IngredientData.DisplayName.ToString(),
            Instance.InstanceID,
            Instance.Quantity);
    }
    
    // Create buttons for up to 10 ingredient instances
    int32 MaxButtons = FMath::Min(10, CurrentDishData.IngredientInstances.Num());
    for (int32 i = 0; i < MaxButtons; ++i)
    {
        const FIngredientInstance& IngredientInstance = CurrentDishData.IngredientInstances[i];
        
        // Validate ingredient instance
        if (!IngredientInstance.IngredientData.IngredientTag.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateIngredientButtonsFromDishData - Invalid ingredient instance at index %d"), i);
            continue;
        }
        
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
        
        UE_LOG(LogTemp, Display, TEXT("🍳 Creating ingredient button using class: %s"), 
            ButtonClass ? *ButtonClass->GetName() : TEXT("NULL"));
        
        UPUIngredientButton* IngredientButton = CreateWidget<UPUIngredientButton>(this, ButtonClass);
        if (IngredientButton)
        {
            // Set the ingredient data
            IngredientButton->SetIngredientData(IngredientInstance.IngredientData);
            
            // Try to disable click functionality (only drag should work)
            UButton* InternalButton = IngredientButton->GetIngredientButton();
            if (InternalButton)
            {
                InternalButton->SetIsEnabled(false);
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Disabled click functionality for ingredient: %s"), 
                    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateIngredientButtonsFromDishData - Internal button is null for ingredient: %s (this is expected for programmatically created widgets)"), 
                    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
            
            // Add to our array
            CreatedIngredientButtons.Add(IngredientButton);
            
            // Enable drag functionality for cooking stage
            IngredientButton->SetDragEnabled(true);
            
            // Add to the UI container if available
            if (IngredientButtonContainer.IsValid())
            {
                IngredientButtonContainer->AddChild(IngredientButton);
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Added button to container for ingredient: %s"), 
                    *IngredientInstance.IngredientData.DisplayName.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateIngredientButtonsFromDishData - No ingredient button container set! Button will not be visible."));
            }
            
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Created button for ingredient: %s (ID: %d, Qty: %d)"), 
                *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::CreateIngredientButtonsFromDishData - Failed to create ingredient button for: %s"), 
                *IngredientInstance.IngredientData.DisplayName.ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateIngredientButtonsFromDishData - Successfully created %d ingredient buttons"), CreatedIngredientButtons.Num());
    
    // Mark that buttons have been created
    bIngredientButtonsCreated = true;
}

void UPUCookingStageWidget::SetIngredientButtonContainer(UPanelWidget* Container)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetIngredientButtonContainer - Setting ingredient button container"));
    
    if (Container)
    {
        IngredientButtonContainer = Container;
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetIngredientButtonContainer - Container set successfully"));
        
        // If we already have created buttons, add them to the new container
        if (CreatedIngredientButtons.Num() > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetIngredientButtonContainer - Adding %d existing buttons to container"), CreatedIngredientButtons.Num());
            for (UPUIngredientButton* Button : CreatedIngredientButtons)
            {
                if (Button && !Button->GetParent())
                {
                    Container->AddChild(Button);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetIngredientButtonContainer - Container is null"));
    }
}

void UPUCookingStageWidget::CreateQuantityControlFromDroppedIngredient(const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlFromDroppedIngredient - Creating quantity control for dropped ingredient: %s"), 
        *IngredientData.DisplayName.ToString());
    
    // Print the message as requested
    UE_LOG(LogTemp, Display, TEXT("🎯 INGREDIENT DROPPED: %s (ID: %d, Qty: %d)"), 
        *IngredientData.DisplayName.ToString(), InstanceID, Quantity);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Creating ingredient instance data"));
    
    // Create ingredient instance for Blueprint event
    FIngredientInstance IngredientInstance;
    IngredientInstance.InstanceID = InstanceID;
    IngredientInstance.Quantity = Quantity;
    IngredientInstance.IngredientData = IngredientData;
    IngredientInstance.IngredientTag = IngredientData.IngredientTag;
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Ingredient instance created - ID: %d, Qty: %d, Tag: %s"), 
        IngredientInstance.InstanceID, IngredientInstance.Quantity, *IngredientInstance.IngredientTag.ToString());
    
    // Apply implement-specific preparation tags on drop
    {
        const FGameplayTagContainer AllowedTags = GetPreparationTagsForImplement(SelectedImplementIndex);
        if (AllowedTags.Num() > 0)
        {
            TArray<FGameplayTag> TagsArray;
            AllowedTags.GetGameplayTagArray(TagsArray);
            for (const FGameplayTag& Tag : TagsArray)
            {
                if (!IngredientInstance.Preparations.HasTag(Tag))
                {
                    IngredientInstance.Preparations.AddTag(Tag);
                    UE_LOG(LogTemp, Display, TEXT("🎯 Applied preparation tag on drop: %s"), *Tag.ToString());
                }
            }
            // Mirror to ingredient data active preps for UI/name logic
            IngredientInstance.IngredientData.ActivePreparations = IngredientInstance.Preparations;
            UE_LOG(LogTemp, Display, TEXT("🎯 Total applied preparation tags: %d"), IngredientInstance.Preparations.Num());
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("ℹ️ No implement-specific preparation tags configured for implement %d"), SelectedImplementIndex);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Adding ingredient to CurrentDishData with InstanceID: %d"), IngredientInstance.InstanceID);
    
    // Add the ingredient instance to the current dish data
    CurrentDishData.IngredientInstances.Add(IngredientInstance);
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Added ingredient to dish data. Total ingredients: %d"), CurrentDishData.IngredientInstances.Num());
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: About to call OnQuantityControlCreated Blueprint event"));
    
    // Call Blueprint event to create quantity control widget
    OnQuantityControlCreated(nullptr, IngredientInstance);
    
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: OnQuantityControlCreated Blueprint event called"));
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: After Blueprint event, CurrentDishData has %d ingredients"), CurrentDishData.IngredientInstances.Num());
    
    // Broadcast dish data change to update radar graphs
    OnDishDataChanged(CurrentDishData);
    UE_LOG(LogTemp, Display, TEXT("🔍 DEBUG: Broadcasted OnDishDataChanged for radar graph update"));
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlFromDroppedIngredient - Blueprint event called for quantity control creation"));
}

bool UPUCookingStageWidget::NativeOnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UPUIngredientDragDropOperation>(InOperation))
    {
        bIsDragActive = true;
        UWorld* World = GetWorld();
        if (!World)
        {
            return false;
        }

        // Use the drag event's absolute position; convert to viewport pixel coords
        const FVector2D Absolute = InDragDropEvent.GetScreenSpacePosition();
        const FVector2D ViewportPos = ConvertAbsoluteToViewport(Absolute);

        FHitResult Hit;
		const int32 ImplementIdx = FindImplementUnderScreenPos(ViewportPos, Hit);
		if (ImplementIdx != INDEX_NONE)
        {
			// Only allow hover on the currently selected implement
			if (ImplementIdx == SelectedImplementIndex)
            {
				if (HoveredImplementIndex != ImplementIdx)
				{
					if (HoveredImplementIndex != INDEX_NONE)
					{
						ApplyHoverVisualEffect(HoveredImplementIndex, false);
					}
					HoveredImplementIndex = ImplementIdx;
					ApplyHoverVisualEffect(HoveredImplementIndex, true);
				}
				UE_LOG(LogTemp, Verbose, TEXT("🎯 DragOver hovering active implement %d"), ImplementIdx);
				return true;
            }
			else
			{
				if (HoveredImplementIndex != INDEX_NONE)
				{
					ApplyHoverVisualEffect(HoveredImplementIndex, false);
					HoveredImplementIndex = INDEX_NONE;
				}
				UE_LOG(LogTemp, Verbose, TEXT("🛑 DragOver rejected: implement %d is not active (active=%d)"), ImplementIdx, SelectedImplementIndex);
				return false;
			}
        }

        if (HoveredImplementIndex != INDEX_NONE)
        {
            ApplyHoverVisualEffect(HoveredImplementIndex, false);
            HoveredImplementIndex = INDEX_NONE;
            UE_LOG(LogTemp, Verbose, TEXT("🎯 DragOver left hover"));
        }
    }

    return false;
}

bool UPUCookingStageWidget::NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (UPUIngredientDragDropOperation* IngredientOp = Cast<UPUIngredientDragDropOperation>(InOperation))
    {
        bIsDragActive = false;
        UWorld* World = GetWorld();
        if (!World)
        {
            return false;
        }
        const FVector2D Absolute = InDragDropEvent.GetScreenSpacePosition();
        const FVector2D ViewportPos = ConvertAbsoluteToViewport(Absolute);

        FHitResult Hit;
		const int32 ImplementIdx = FindImplementUnderScreenPos(ViewportPos, Hit);
		if (ImplementIdx != INDEX_NONE)
        {
			if (ImplementIdx == SelectedImplementIndex)
			{
				UE_LOG(LogTemp, Display, TEXT("🍳 NativeOnDrop - Dropping on active implement %d"), ImplementIdx);
				OnIngredientDroppedOnImplement(ImplementIdx, IngredientOp->IngredientTag, IngredientOp->IngredientData, IngredientOp->InstanceID, IngredientOp->Quantity);
				RotateCarouselToSelection(ImplementIdx);
				return true;
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("🛑 NativeOnDrop - Rejected drop on non-active implement %d (active=%d)"), ImplementIdx, SelectedImplementIndex);
			}
        }
    }

    return false;
}

void UPUCookingStageWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // Handle drop onto world when no widget accepted the drop
    if (UPUIngredientDragDropOperation* IngredientOp = Cast<UPUIngredientDragDropOperation>(InOperation))
    {
        bIsDragActive = false;
        UE_LOG(LogTemp, Display, TEXT("🍳 NativeOnDragCancelled - Attempting world drop"));
        UWorld* World = GetWorld();
        if (!World)
        {
            return;
        }
        const FVector2D Absolute = InDragDropEvent.GetScreenSpacePosition();
        const FVector2D ViewportPos = ConvertAbsoluteToViewport(Absolute);

        FHitResult Hit;
		const int32 ImplementIdx = FindImplementUnderScreenPos(ViewportPos, Hit);
		if (ImplementIdx != INDEX_NONE)
        {
			if (ImplementIdx == SelectedImplementIndex)
			{
				UE_LOG(LogTemp, Display, TEXT("🍳 NativeOnDragCancelled - Dropped on active implement %d"), ImplementIdx);
				OnIngredientDroppedOnImplement(ImplementIdx, IngredientOp->IngredientTag, IngredientOp->IngredientData, IngredientOp->InstanceID, IngredientOp->Quantity);
				RotateCarouselToSelection(ImplementIdx);
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("🛑 NativeOnDragCancelled - Rejected drop on non-active implement %d (active=%d)"), ImplementIdx, SelectedImplementIndex);
			}
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 NativeOnDragCancelled - No implement under cursor"));
        }
    }
}

void UPUCookingStageWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UPUIngredientDragDropOperation>(InOperation))
    {
        bIsDragActive = true;
    }
}

void UPUCookingStageWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UPUIngredientDragDropOperation>(InOperation))
    {
        bIsDragActive = false;
        if (HoveredImplementIndex != INDEX_NONE)
        {
            ApplyHoverVisualEffect(HoveredImplementIndex, false);
            HoveredImplementIndex = INDEX_NONE;
        }
    }
}
