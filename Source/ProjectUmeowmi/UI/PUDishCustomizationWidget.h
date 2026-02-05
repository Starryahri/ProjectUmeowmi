#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUIngredientSlot.h"
#include "GameplayTagContainer.h"
#include "Components/ScrollBox.h"
#include "PUDishCustomizationWidget.generated.h"

class UPUDishCustomizationComponent;
class UScrollBox;

// Stage type enum for dish customization stages
UENUM(BlueprintType)
enum class EDishCustomizationStageType : uint8
{
    Planning     UMETA(DisplayName = "Planning"),
    Cooking      UMETA(DisplayName = "Cooking"),
    Plating      UMETA(DisplayName = "Plating")
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUDishCustomizationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Event handlers for dish data
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnInitialDishDataReceived(const FPUDishBase& InitialDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnDishDataUpdated(const FPUDishBase& UpdatedDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void OnCustomizationEnded();

    // Set the customization component reference (for event subscription only)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void SetCustomizationComponent(UPUDishCustomizationComponent* Component);

    // Get the customization component reference (for accessing data)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    UPUDishCustomizationComponent* GetCustomizationComponent() const { return CustomizationComponent; }

    // Get current dish data
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // GUID-based unique ID generation for ingredient instances
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    static int32 GenerateGUIDBasedInstanceID();

    // End customization function for UI buttons
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void EndCustomizationFromUI();

    // Update dish data and sync back to component
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget")
    void UpdateDishData(const FPUDishBase& NewDishData);

    // Stage navigation functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    void GoToStage(class UPUDishCustomizationWidget* TargetStage);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    void GoToNextStage();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    void GoToPreviousStage();

    // Stage reference setters (for Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    void SetPreviousStage(class UPUDishCustomizationWidget* Stage);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    void SetNextStage(class UPUDishCustomizationWidget* Stage);

    // Stage reference getters
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    TSubclassOf<class UPUDishCustomizationWidget> GetPreviousStage() const { return PreviousStage; }

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    TSubclassOf<class UPUDishCustomizationWidget> GetNextStage() const { return NextStage; }

    // Stage type getter
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Stages")
    EDishCustomizationStageType GetStageType() const { return StageType; }

    // Ingredient management functions
    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientSlots();

    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    void OnIngredientButtonClicked(const FPUIngredientBase& IngredientData);

    // Unified Ingredient Slot Creation Function
    // This is the main function for creating slots - all other creation functions use this internally
    // Parameters:
    //   - Container: The container widget to add slots to (can be null if bUseShelvingWidgets is true, container will be used for shelving widgets)
    //   - Location: The location type for the slots (Pantry, ActiveIngredientArea, Prep, Plating, Prepped)
    //   - MaxSlots: Maximum number of slots to create (default 12, clamped to 1-12)
    //   - bUseShelvingWidgets: If true, slots will be organized into shelving widgets (3 slots per shelf). If false, slots added directly to container
    //   - bCreateEmptySlots: If true, creates empty slots up to MaxSlots. If false, only creates slots for existing ingredients
    //   - bEnableDrag: Whether to enable drag functionality on the slots
    //   - IngredientSource: Array of ingredient instances to use. If empty, uses CurrentDishData.IngredientInstances
    //   - FirstSlotLeftPadding: Left padding to apply to the first slot when added directly to container (not using shelving widgets). Useful for aligning with skewed backgrounds.
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateSlots(UPanelWidget* Container, EPUIngredientSlotLocation Location, int32 MaxSlots, bool bUseShelvingWidgets, bool bCreateEmptySlots, bool bEnableDrag, const TArray<FIngredientInstance>& IngredientSource, float FirstSlotLeftPadding = 0.0f);
    
    // Convenience function that uses CurrentDishData.IngredientInstances (no IngredientSource parameter needed)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateSlotsFromDishData(UPanelWidget* Container, EPUIngredientSlotLocation Location, int32 MaxSlots = 12, bool bUseShelvingWidgets = false, bool bCreateEmptySlots = true, bool bEnableDrag = true, float FirstSlotLeftPadding = 0.0f);
    
    // Helper function to convert ingredient data table to ingredient instances array
    // Takes a data table containing FPUIngredientBase rows and converts them to FIngredientInstance array
    // All instances will have quantity 0 and instance ID 0 (suitable for pantry/prep slots)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    static TArray<FIngredientInstance> GetIngredientInstancesFromDataTable(UDataTable* IngredientDataTable);

    // Ingredient Slot Management Functions
    // Create ingredient slots in a specified container (max 12 slots)
    // This is a convenience function for blueprints to easily create slots in a container
    // DEPRECATED: Use CreateSlots() instead
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients", meta = (CallInEditor = "true", DeprecatedFunction, DeprecationMessage = "Use CreateSlots() instead"))
    void CreateIngredientSlotsInContainer(UPanelWidget* Container, int32 MaxSlots = 12, EPUIngredientSlotLocation SlotLocation = EPUIngredientSlotLocation::ActiveIngredientArea);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);

    // Plating stage functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Plating")
    void CreatePlatingIngredientSlots();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Plating")
    void SetIngredientSlotContainer(UPanelWidget* Container, EPUIngredientSlotLocation SlotLocation = EPUIngredientSlotLocation::Prep);

    // Prepped ingredient management functions (for prep stage)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Prepped")
    void CreateOrUpdatePreppedSlot(const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Prepped")
    void RemovePreppedSlot(const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Prepped")
    void SetPreppedIngredientContainer(UPanelWidget* Container);

    // Planning stage functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Planning")
    void ToggleIngredientSelection(const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Planning")
    bool IsIngredientSelected(const FPUIngredientBase& IngredientData) const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Planning")
    void StartPlanningMode();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Planning")
    void FinishPlanningAndStartCooking();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Planning")
    const FPUPlanningData& GetPlanningData() const { return PlanningData; }

    // Find ingredient button by ingredient data
    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    class UPUIngredientButton* FindIngredientButton(const FPUIngredientBase& IngredientData) const;

    // Find ingredient button by tag (simpler alternative)
    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    class UPUIngredientButton* GetIngredientButtonByTag(const FGameplayTag& IngredientTag) const;

    // Remove ingredient instance by tag
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void RemoveIngredientInstanceByTag(const FGameplayTag& IngredientTag);

    // Remove ingredient instance by ID
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void RemoveIngredientInstance(int32 InstanceID);

    // Check if we can add more ingredients
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    bool CanAddMoreIngredients() const;

    // Get the maximum number of ingredients allowed
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    int32 GetMaxIngredients() const { return MaxIngredients; }

    // Set the maximum number of ingredients allowed
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void SetMaxIngredients(int32 NewMaxIngredients);

    // Blueprint events for planning mode
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Planning")
    void OnPlanningModeStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Planning")
    void OnIngredientSelectionChanged(const FPUIngredientBase& IngredientData, bool bIsSelected);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Planning")
    void OnPlanningCompleted(const FPUPlanningData& InPlanningData);
    

    // Get created ingredient slots (for reset functionality)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    const TArray<class UPUIngredientSlot*>& GetCreatedIngredientSlots() const { return CreatedIngredientSlots; }

    // Quantity Control Management Functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void SetQuantityControlContainer(UPanelWidget* Container);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void EnableQuantityControlDrag(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    bool UpdateExistingQuantityControl(int32 InstanceID, const FGameplayTagContainer& NewPreparations);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void FindQuantityControlsInHierarchy(TArray<UPUIngredientQuantityControl*>& OutQuantityControls);

    // Pantry Functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void PopulatePantrySlots();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void OpenPantry();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void ClosePantry();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void ClosePantryFromDrag();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    bool IsPantryOpen() const { return bPantryOpen; }

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void OnPantryButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void SetPantryContainer(UPanelWidget* Container);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Pantry")
    void SetPantryContainerByName(const FName& ContainerName);

    // Handle empty slot click (opens pantry)
    UFUNCTION()
    void OnEmptySlotClicked(class UPUIngredientSlot* IngredientSlot);

    // Implement Preparation Tags (simplified - no carousel dependency)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Preparations")
    FGameplayTagContainer GetPreparationTagsForImplement(int32 ImplementIndex) const;

    // Controller navigation setup for prep stage
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Controller")
    void SetupPrepSlotNavigation();

    // Set initial focus for prep stage
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Controller")
    void SetInitialFocusForPrepStage();
    
    // Set up navigation for pantry slots (for controller support)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Controller")
    void SetupPantrySlotNavigation();
    
    // Set initial focus for pantry (for controller support)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Controller")
    void SetInitialFocusForPantry();

protected:
    // Current dish data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Data")
    FPUDishBase CurrentDishData;

    // Planning data for multi-stage cooking
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    FPUPlanningData PlanningData;

    // Planning mode flag
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    bool bInPlanningMode = false;

    // Stage type - determines what transition logic to apply
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Stages")
    EDishCustomizationStageType StageType = EDishCustomizationStageType::Cooking;

    // Stage navigation - previous and next stage widgets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Stages")
    TSubclassOf<class UPUDishCustomizationWidget> PreviousStage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Stages")
    TSubclassOf<class UPUDishCustomizationWidget> NextStage;

    // Maximum number of ingredients that can be selected
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Settings", meta = (ClampMin = "1", ClampMax = "20"))
    int32 MaxIngredients = 10;

    // Store references to ingredient buttons for O(1) lookup
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Buttons")
    TMap<FGameplayTag, class UPUIngredientButton*> IngredientButtonMap;

    // Store references to ingredient slots by ingredient tag (for pantry slots)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slots")
    TMap<FGameplayTag, class UPUIngredientSlot*> IngredientSlotMap;


    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientButton> IngredientButtonClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<class UPUIngredientSlot> IngredientSlotClass;

    // Shelving widget class for organizing slots (holds up to 3 slots)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UUserWidget> ShelvingWidgetClass;

    // Name of the HorizontalBox widget inside WBP_Shelving (default: "SlotContainer" or "HorizontalBox")
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes", meta = (ToolTip = "Name of the HorizontalBox widget inside WBP_Shelving where slots will be added"))
    FName ShelvingHorizontalBoxName = TEXT("SlotContainer");

    // Container for ingredient buttons
    UPROPERTY(BlueprintReadOnly, Category = "Dish Customization Widget|Plating")
    TWeakObjectPtr<UPanelWidget> IngredientButtonContainer;

    // Ingredient Slot Management Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Ingredients")
    TArray<class UPUIngredientSlot*> CreatedIngredientSlots;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Ingredients")
    bool bIngredientSlotsCreated = false;

    // Widget reference for ingredient slot container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> IngredientSlotContainer;

    // Shelving widget management (for pantry and prep slots)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Ingredients")
    TArray<UUserWidget*> CreatedShelvingWidgets;

    // Current shelving widget being filled (nullptr if we need to create a new one)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Ingredients")
    TWeakObjectPtr<UUserWidget> CurrentShelvingWidget;

    // Number of slots in the current shelving widget (0-3)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Ingredients")
    int32 CurrentShelvingWidgetSlotCount = 0;

    // Optional direct reference to the ScrollBox that should host quantity controls
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "Dish Customization Widget|Ingredients")
    UScrollBox* QuantityScrollBox = nullptr;

    // Widget reference for quantity control container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> QuantityControlContainer;

    // Pantry Management Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    TArray<class UPUIngredientSlot*> CreatedPantrySlots;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    bool bPantrySlotsCreated = false;

    // Flag to track if pantry is open
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    bool bPantryOpen = false;

    // Prepped Ingredient Management Properties (for prep stage)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Prepped")
    TArray<class UPUIngredientSlot*> CreatedPreppedSlots;

    // Widget reference for prepped ingredient container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Prepped")
    TWeakObjectPtr<class UPanelWidget> PreppedIngredientContainer;

    // Map to track prepped slots by ingredient instance ID (for quick lookup)
    // One prepped slot per unique ingredient instance, so duplicates of the same ingredient
    // in different prep slots each get their own bowl in the prepped area.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Prepped")
    TMap<int32, class UPUIngredientSlot*> PreppedSlotMap;

    // Widget reference for pantry container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Pantry")
    TWeakObjectPtr<class UPanelWidget> PantryContainer;

    // Store references to pantry slots by ingredient tag (for quick lookup)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    TMap<FGameplayTag, class UPUIngredientSlot*> PantrySlotMap;

    // Reference to the empty slot that triggered pantry open (for populating after selection)
    UPROPERTY()
    TWeakObjectPtr<class UPUIngredientSlot> PendingEmptySlot;

    // Shelving widget management (for pantry slots)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    TArray<UUserWidget*> CreatedPantryShelvingWidgets;

    // Current shelving widget being filled (nullptr if we need to create a new one)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    TWeakObjectPtr<UUserWidget> CurrentPantryShelvingWidget;

    // Number of slots in the current pantry shelving widget (0-3)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Customization Widget|Pantry")
    int32 CurrentPantryShelvingWidgetSlotCount = 0;

    // Implement Preparation Tags (simplified - no carousel, just data structure)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization Widget|Preparations", meta=(EditFixedSize=true, Categories="Prep", DisplayName="Implement Preparation Tags", ToolTip="Per-implement allowed preparation tags; uses Prep.* tags"))
    TArray<FGameplayTagContainer> ImplementPreparationTags;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataReceived(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnCustomizationModeEnded();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnIngredientButtonCreated(class UPUIngredientButton* IngredientButton, const FPUIngredientBase& IngredientData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnIngredientSlotCreated(class UPUIngredientSlot* IngredientSlot, const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget")
    void OnQuantityControlCreated(class UPUIngredientQuantityControl* QuantityControl, const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Plating")
    void OnPlatingStageInitialized(const FPUDishBase& DishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Plating")
    void EnablePlatingSlots();

    // Blueprint events for pantry
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Pantry")
    void OnPantryOpened();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Pantry")
    void OnPantryClosed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization Widget|Pantry")
    void OnPantryClosedFromDrag();


private:
    // Internal component reference for event subscription only
    UPROPERTY()
    UPUDishCustomizationComponent* CustomizationComponent;

    // Timer handle for delayed focus setting
    UPROPERTY()
    FTimerHandle InitialFocusTimerHandle;

    void SubscribeToEvents();
    void UnsubscribeFromEvents();
    
    // Helper function to update radar chart from planning data
    void UpdateRadarChartFromPlanningData();

    // Helper functions
    void CreateIngredientInstance(const FPUIngredientBase& IngredientData);
    void UpdateIngredientInstance(const FIngredientInstance& IngredientInstance);
    void RefreshQuantityControls();
    
    // Handle pantry slot clicks
    UFUNCTION()
    void OnPantrySlotClicked(class UPUIngredientSlot* IngredientSlot);

    // Helper function to get or create a current shelving widget
    UUserWidget* GetOrCreateCurrentShelvingWidget(UPanelWidget* ContainerToUse);
    
    // Helper function to add a slot to the current shelving widget
    bool AddSlotToCurrentShelvingWidget(class UPUIngredientSlot* IngredientSlot);

    // Recursive helper function to find quantity controls
    void FindQuantityControlsRecursive(UWidget* ParentWidget, TArray<UPUIngredientQuantityControl*>& OutQuantityControls);

    // Helper function to get or create a current pantry shelving widget
    UUserWidget* GetOrCreateCurrentPantryShelvingWidget(UPanelWidget* ContainerToUse);
    
    // Helper function to add a slot to the current pantry shelving widget
    bool AddSlotToCurrentPantryShelvingWidget(class UPUIngredientSlot* IngredientSlot);
}; 