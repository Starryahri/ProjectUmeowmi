#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUDishCustomizationWidget.generated.h"

class UPUDishCustomizationComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUDishCustomizationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

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

    // Ingredient management functions
    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientSlots();

    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    void OnIngredientButtonClicked(const FPUIngredientBase& IngredientData);

    // Ingredient Slot Management Functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void CreateIngredientSlotsFromDishData();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);

    // Plating stage functions
    UFUNCTION(Category = "Dish Customization Widget|Plating")
    void CreatePlatingIngredientButtons();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Plating")
    void SetIngredientSlotContainer(UPanelWidget* Container);

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

    // Get plating ingredient button map
    UFUNCTION(Category = "Dish Customization Widget|Ingredients")
    const TMap<int32, class UPUIngredientButton*>& GetPlatingIngredientButtonMap() const { return PlatingIngredientButtonMap; }

    // Remove ingredient instance by tag
    UFUNCTION(BlueprintCallable, Category = "Dish Customization Widget|Ingredients")
    void RemoveIngredientInstanceByTag(const FGameplayTag& IngredientTag);

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

    // Maximum number of ingredients that can be selected
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization Widget|Settings", meta = (ClampMin = "1", ClampMax = "20"))
    int32 MaxIngredients = 10;

    // Store references to ingredient buttons for O(1) lookup
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Buttons")
    TMap<FGameplayTag, class UPUIngredientButton*> IngredientButtonMap;

    // Store references to ingredient slots by ingredient tag (for pantry slots)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Slots")
    TMap<FGameplayTag, class UPUIngredientSlot*> IngredientSlotMap;

    // Store references to plating ingredient buttons by InstanceID (for multiple instances of same ingredient)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Buttons")
    TMap<int32, class UPUIngredientButton*> PlatingIngredientButtonMap;

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
    void EnablePlatingButtons();


private:
    // Internal component reference for event subscription only
    UPROPERTY()
    UPUDishCustomizationComponent* CustomizationComponent;

    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    // Helper functions
    void CreateIngredientInstance(const FPUIngredientBase& IngredientData);
    void UpdateIngredientInstance(const FIngredientInstance& IngredientInstance);
    void RemoveIngredientInstance(int32 InstanceID);
    void RefreshQuantityControls();
    
    // Handle pantry slot clicks
    UFUNCTION()
    void OnPantrySlotClicked(class UPUIngredientSlot* IngredientSlot);

    // Helper function to get or create a current shelving widget
    UUserWidget* GetOrCreateCurrentShelvingWidget(UPanelWidget* ContainerToUse);
    
    // Helper function to add a slot to the current shelving widget
    bool AddSlotToCurrentShelvingWidget(class UPUIngredientSlot* IngredientSlot);
}; 