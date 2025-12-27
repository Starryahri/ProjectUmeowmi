#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUIngredientButton.h"
#include "GameplayTagContainer.h"
#include "Components/ScrollBox.h"
#include "PUCookingStageWidget.generated.h"

class UPUIngredientQuantityControl;
class UPUIngredientButton;
class AStaticMeshActor;
class UScrollBox;
class UPURadarChart;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingCompleted, const FPUDishBase&, FinalDishData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingImplementSelected, int32, ImplementIndex);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUCookingStageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Initialize the cooking stage with dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void InitializeCookingStage(const FPUDishBase& DishData, const FVector& CookingStationLocation = FVector::ZeroVector);

    // Get the current dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // Update the current dish data (for ingredient slots to update preparations, etc.)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void UpdateCurrentDishData(const FPUDishBase& NewDishData);

    // GUID-based unique ID generation
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    static int32 GenerateGUIDBasedInstanceID();

    // Get the planning data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUPlanningData& GetPlanningData() const { return PlanningData; }

    // Finish cooking and transition to plating
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void FinishCookingAndStartPlating();

    // Exit cooking stage and return to customization
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void ExitCookingStage();

    // Exit customization mode completely and return to main game
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void ExitCustomizationMode();

    // Carousel Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SpawnCookingCarousel();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void DestroyCookingCarousel();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SelectCookingImplement(int32 ImplementIndex);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    int32 GetSelectedImplementIndex() const { return SelectedImplementIndex; }

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SetCarouselCenter(const FVector& NewCenter);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SetCookingStationLocation(const FVector& StationLocation);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void FindAndSetNearestCookingStation();

    // Implement Preparation Tags
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Preparations")
    FGameplayTagContainer GetPreparationTagsForImplement(int32 ImplementIndex) const;

    // Update radar chart with current dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Radar Chart")
    void UpdateRadarChart(class UPURadarChart* InRadarChartWidget);

    // Remove ingredient instance from cooking stage (for use by ingredient slots)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void RemoveIngredientInstanceFromCookingStage(int32 InstanceID);

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageInitialized(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageCompleted(const FPUDishBase& FinalDishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnQuantityControlCreated(class UPUIngredientQuantityControl* QuantityControl, const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget|Pantry")
    void OnPantryOpened();

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget|Pantry")
    void OnPantryClosed();

    // Called when pantry is closed due to drag operation (play animation in reverse)
    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget|Pantry")
    void OnPantryClosedFromDrag();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingCompleted OnCookingCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingImplementSelected OnCookingImplementSelected;

    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingCompleted OnCookingStageExited;

    // Reference to the dish customization component for exit functionality
    UPROPERTY(BlueprintReadWrite, Category = "Cooking Stage Widget")
    class UPUDishCustomizationComponent* DishCustomizationComponent;

    // Reference to the dish customization widget (for ingredient slot access)
    UPROPERTY(BlueprintReadWrite, Category = "Cooking Stage Widget")
    class UPUDishCustomizationWidget* DishCustomizationWidget;

    // Optional reference to radar chart widget (will be automatically updated when dish data changes)
    UPROPERTY(BlueprintReadWrite, Category = "Cooking Stage Widget|Radar Chart")
    class UPURadarChart* RadarChartWidget;

    // Set the customization component reference
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void SetDishCustomizationComponent(UPUDishCustomizationComponent* Component);

    // Set the dish customization widget reference
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void SetDishCustomizationWidget(class UPUDishCustomizationWidget* Widget);

    // Drag and Drop Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Drag Drop")
    void OnIngredientDroppedOnImplement(int32 ImplementIndex, const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Drag Drop")
    bool IsDragOverImplement(int32 ImplementIndex, const FVector2D& ScreenPosition) const;

    // Get created ingredient slots (for drag and drop functionality)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    const TArray<class UPUIngredientSlot*>& GetCreatedIngredientSlots() const { return CreatedIngredientSlots; }

    // Native drag and drop events
    virtual bool NativeOnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // Ingredient Button Management Functions (DEPRECATED - use slots instead)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void CreateIngredientButtonsFromDishData();

    // Ingredient Slot Management Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void CreateIngredientSlotsFromDishData();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void CreateQuantityControlFromDroppedIngredient(const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity);

    // Set the container widget for ingredient buttons
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void SetIngredientButtonContainer(UPanelWidget* Container);

    // Set the container widget for quantity controls (e.g., a ScrollBox)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void SetQuantityControlContainer(UPanelWidget* Container);

    // Enable/disable drag functionality on all quantity controls
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void EnableQuantityControlDrag(bool bEnabled);

    // Find and update existing quantity control by InstanceID
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    bool UpdateExistingQuantityControl(int32 InstanceID, const FGameplayTagContainer& NewPreparations);

    // Find quantity controls in the widget hierarchy
    void FindQuantityControlsInHierarchy(TArray<UPUIngredientQuantityControl*>& OutQuantityControls);

    // Recursive helper function to find quantity controls
    void FindQuantityControlsRecursive(UWidget* ParentWidget, TArray<UPUIngredientQuantityControl*>& OutQuantityControls);

    // Pantry Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void PopulatePantrySlots();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void OpenPantry();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void ClosePantry();

    // Close pantry from drag operation (triggers reverse animation event)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void ClosePantryFromDrag();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    bool IsPantryOpen() const { return bPantryOpen; }

    // Handle pantry button click
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void OnPantryButtonClicked();

    // Handle pantry slot click (when selecting ingredient from pantry)
    UFUNCTION()
    void OnPantrySlotClicked(class UPUIngredientSlot* IngredientSlot);

    // Handle empty slot click (opens pantry)
    UFUNCTION()
    void OnEmptySlotClicked(class UPUIngredientSlot* IngredientSlot);

    // Helper function to get or create a current shelving widget
    UUserWidget* GetOrCreateCurrentPantryShelvingWidget(UPanelWidget* ContainerToUse);
    
    // Helper function to add a slot to the current shelving widget
    bool AddSlotToCurrentPantryShelvingWidget(class UPUIngredientSlot* IngredientSlot);

    // Set the pantry container widget (by reference)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void SetPantryContainer(UPanelWidget* Container);

    // Set the pantry container widget (by name - searches widget hierarchy)
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Pantry")
    void SetPantryContainerByName(const FName& ContainerName);


protected:
    // Current dish data (being built during cooking)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Data")
    FPUDishBase CurrentDishData;

    // Planning data from the planning stage
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    FPUPlanningData PlanningData;

    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientButton> IngredientButtonClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<class UPUIngredientSlot> IngredientSlotClass;

    // Shelving widget class for organizing pantry slots (holds up to 3 slots)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes|Pantry")
    TSubclassOf<UUserWidget> ShelvingWidgetClass;

    // Name of the HorizontalBox widget inside WBP_Shelving (default: "SlotContainer" or "HorizontalBox")
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes|Pantry", meta = (ToolTip = "Name of the HorizontalBox widget inside WBP_Shelving where slots will be added"))
    FName ShelvingHorizontalBoxName = TEXT("SlotContainer");

    // Ingredient Button Management Properties (DEPRECATED - use slots instead)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    TArray<class UPUIngredientButton*> CreatedIngredientButtons;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    bool bIngredientButtonsCreated = false;

    // Widget reference for ingredient button container (set in Blueprint) (DEPRECATED - use slots instead)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> IngredientButtonContainer;

    // Ingredient Slot Management Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    TArray<class UPUIngredientSlot*> CreatedIngredientSlots;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    bool bIngredientSlotsCreated = false;

    // Widget reference for ingredient slot container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> IngredientSlotContainer;

    // Optional direct reference to the ScrollBox that should host quantity controls
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "Cooking Stage Widget|Ingredients")
    UScrollBox* QuantityScrollBox = nullptr;

    // Widget reference for quantity control container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> QuantityControlContainer;

    // Pantry Management Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    TArray<class UPUIngredientSlot*> CreatedPantrySlots;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    bool bPantrySlotsCreated = false;

    // Flag to track if pantry is open
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    bool bPantryOpen = false;

    // Widget reference for pantry container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Pantry")
    TWeakObjectPtr<class UPanelWidget> PantryContainer;

    // Store references to pantry slots by ingredient tag (for quick lookup)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    TMap<FGameplayTag, class UPUIngredientSlot*> PantrySlotMap;

    // Reference to the empty slot that triggered pantry open (for populating after selection)
    UPROPERTY()
    TWeakObjectPtr<class UPUIngredientSlot> PendingEmptySlot;

    // Shelving widget management (for pantry slots)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    TArray<UUserWidget*> CreatedPantryShelvingWidgets;

    // Current shelving widget being filled (nullptr if we need to create a new one)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    TWeakObjectPtr<UUserWidget> CurrentPantryShelvingWidget;

    // Number of slots in the current shelving widget (0-3)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Pantry")
    int32 CurrentPantryShelvingWidgetSlotCount = 0;

    // Carousel Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    TArray<class UStaticMesh*> CookingImplementMeshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    TArray<FString> CookingImplementNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    FVector CarouselCenter = FVector(0.0f, 0.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float CarouselRadius = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float CarouselHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float RotationSpeed = 2.0f;

    // One entry per cooking implement (same order as CookingImplementMeshes)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking Stage Widget|Preparations", meta=(EditFixedSize=true, Categories="Prep", DisplayName="Implement Preparation Tags", ToolTip="Per-implement allowed preparation tags; must match CookingImplementMeshes order/length and only uses Prep.* tags"))
    TArray<FGameplayTagContainer> ImplementPreparationTags;

    // Hover Detection Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    TSubclassOf<UUserWidget> HoverTextWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float HoverTextOffset = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    FLinearColor HoverGlowColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float HoverGlowIntensity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    FVector HoverDetectionOffset = FVector(0.0f, 0.0f, 20.0f); // Adjustable offset for hover detection

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float HoverDetectionRadius = 100.0f; // Radius in pixels for hover detection

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float ClickDetectionRadius = 120.0f; // Radius in pixels for click detection

    // Front indicator properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float FrontItemScale = 1.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float FrontItemHeight = 50.0f;

private:
    // Create quantity controls for selected ingredients
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void CreateQuantityControlsForSelectedIngredients();

    // Handle quantity control changes
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    // Handle quantity control removal
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);

    // Carousel private functions
    void PositionCookingImplements();
    void RotateCarouselToSelection(int32 TargetIndex);
    FVector CalculateImplementPosition(int32 Index, int32 TotalCount) const;
    FRotator CalculateImplementRotation(int32 Index, int32 TotalCount) const;

    // Hover detection functions
    void SetupHoverDetection();
    UFUNCTION()
    void OnImplementHoverBegin(UPrimitiveComponent* TouchedComponent);
    UFUNCTION()
    void OnImplementHoverEnd(UPrimitiveComponent* TouchedComponent);
    void ShowHoverText(int32 ImplementIndex, bool bShow);
    void ApplyHoverVisualEffect(int32 ImplementIndex, bool bApply);
    int32 GetImplementIndexFromComponent(UPrimitiveComponent* Component) const;

    // Click detection functions
    void SetupClickDetection();
    UFUNCTION()
    void OnImplementClicked(UPrimitiveComponent* ClickedComponent, FKey ButtonPressed);
    void UpdateCarouselRotation(float DeltaTime);
    void ApplySelectionVisualEffect(int32 ImplementIndex, bool bApply);
    void UpdateFrontIndicator();
    int32 GetFrontImplementIndex() const;
    void DebugCollisionDetection();
    void SetupMultiHitCollisionDetection();
    void OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
    void UpdateHoverDetection();
    bool IsPointInCarouselBounds(const FVector2D& ScreenPoint) const;
    void HandleMouseClick();
    bool IsMouseOverImplement(int32 ImplementIndex) const;
    void ShowDebugSpheres();
    void HideDebugSpheres();
    void ToggleDebugVisualization();
    void DrawDebugHoverArea();
    void SwitchToPlayerCamera();
    bool GetViewportMousePos(class APlayerController* PC, FVector2D& OutViewportPos) const;
    int32 FindImplementUnderScreenPos(const FVector2D& ScreenPosViewport, struct FHitResult& OutHit) const;
    FVector2D ConvertAbsoluteToViewport(const FVector2D& AbsoluteScreenPos) const;
    int32 GenerateUniqueIngredientInstanceID() const;
    
    // Helper function to update radar chart and broadcast dish data changed event
    void UpdateRadarChartAndBroadcast();

    // Carousel state
    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedCookingImplements;

    UPROPERTY()
    TArray<UWidgetComponent*> HoverTextComponents;

    UPROPERTY()
    int32 SelectedImplementIndex = 0;

    UPROPERTY()
    int32 HoveredImplementIndex = -1;

    UPROPERTY()
    bool bCarouselSpawned = false;

    // Carousel rotation state
    UPROPERTY()
    bool bIsRotating = false;

    UPROPERTY()
    float CurrentRotationAngle = 0.0f;

    UPROPERTY()
    float TargetRotationAngle = 0.0f;

    UPROPERTY()
    float RotationProgress = 0.0f;

    UPROPERTY()
    bool bIsDragActive = false;
}; 