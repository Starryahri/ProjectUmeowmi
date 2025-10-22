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

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageInitialized(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageCompleted(const FPUDishBase& FinalDishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnQuantityControlCreated(class UPUIngredientQuantityControl* QuantityControl, const FIngredientInstance& IngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnDishDataChanged(const FPUDishBase& DishData);

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

    // Set the customization component reference
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void SetDishCustomizationComponent(UPUDishCustomizationComponent* Component);

    // Drag and Drop Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Drag Drop")
    void OnIngredientDroppedOnImplement(int32 ImplementIndex, const FGameplayTag& IngredientTag, const FPUIngredientBase& IngredientData, int32 InstanceID, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Drag Drop")
    bool IsDragOverImplement(int32 ImplementIndex, const FVector2D& ScreenPosition) const;

    // Native drag and drop events
    virtual bool NativeOnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& MyGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // Ingredient Button Management Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Ingredients")
    void CreateIngredientButtonsFromDishData();

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

    // Ingredient Button Management Properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    TArray<class UPUIngredientButton*> CreatedIngredientButtons;

    // Flag to prevent double creation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Stage Widget|Ingredients")
    bool bIngredientButtonsCreated = false;

    // Widget reference for ingredient button container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> IngredientButtonContainer;

    // Optional direct reference to the ScrollBox that should host quantity controls
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "Cooking Stage Widget|Ingredients")
    UScrollBox* QuantityScrollBox = nullptr;

    // Widget reference for quantity control container (set in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Ingredients")
    TWeakObjectPtr<class UPanelWidget> QuantityControlContainer;

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