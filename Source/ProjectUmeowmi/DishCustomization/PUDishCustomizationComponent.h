#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "PUPreparationBase.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "PUDishCustomizationComponent.generated.h"

// Forward declarations
class UUserWidget;
class UInputAction;
class UEnhancedInputComponent;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCustomizationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDishDataUpdated, const FPUDishBase&, NewDishData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInitialDishDataReceived, const FPUDishBase&, InitialDishData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlanningCompleted, const FPUPlanningData&, InPlanningData);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUDishCustomizationComponent : public USceneComponent
{
    GENERATED_BODY()

public:    
    UPUDishCustomizationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Activation/Deactivation
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void StartCustomization(AProjectUmeowmiCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void EndCustomization();

    // Check if currently customizing
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    bool IsCustomizing() const { return CurrentCharacter != nullptr; }

    // Dish data management
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void UpdateCurrentDishData(const FPUDishBase& NewDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // Blueprint-callable function for UI to sync dish data
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|UI")
    void SyncDishDataFromUI(const FPUDishBase& DishDataFromUI);

    // Function to set the dish customization component reference on the widget
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|UI")
    void SetWidgetComponentReference(UPUDishCustomizationWidget* Widget);

    // Function to set the dish customization component reference on any widget (legacy)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|UI")
    void SetDishCustomizationComponentOnWidget(UUserWidget* Widget);

    // Function to set the currently active customization widget (for stage navigation)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|UI")
    void SetActiveCustomizationWidget(UPUDishCustomizationWidget* ActiveWidget);

    // Function to set the initial dish data from an order
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Orders")
    void SetInitialDishData(const FPUDishBase& InitialDishData);

    // Function to set the data table references
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    void SetDataTables(UDataTable* DishTable, UDataTable* IngredientTable, UDataTable* PreparationTable);

    // Function to get ingredient data for the widget
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    TArray<FPUIngredientBase> GetIngredientData() const;

    // Function to get preparation data for the widget
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    TArray<FPUPreparationBase> GetPreparationData() const;

    // Plating-specific functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void SpawnIngredientIn3D(const FGameplayTag& IngredientTag, const FVector& WorldPosition);

    // Spawn ingredient in 3D world by InstanceID (for plating stage)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void SpawnIngredientIn3DByInstanceID(int32 InstanceID, const FVector& WorldPosition);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void SetPlatingMode(bool bInPlatingMode);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    bool IsPlatingMode() const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void TransitionToPlatingStage(const FPUDishBase& DishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void EndPlatingStage();

    // Ingredient dragging (called from ingredient mesh)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void StartDraggingIngredient(class APUIngredientMesh* Ingredient);

    // Camera switching functions (for stage navigation)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Camera")
    void SwitchToCookingCamera();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Camera")
    void SwitchToPlatingCamera();

    // Plating placement management (for stage navigation)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void ResetPlatingPlacements();

    // Dish mesh management (for stage navigation)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Cooking")
    void SwapDishContainerMesh(UStaticMesh* NewDishMesh);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Cooking")
    void RestoreOriginalDishContainerMesh();

    // Planning mode functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    void StartPlanningMode();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    void TransitionToCookingStage(const FPUDishBase& DishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    bool IsInPlanningMode() const { return bInPlanningMode; }

    // Cooking Camera Position Control
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Cooking Camera")
    void SetCookingCameraPositionOffset(const FVector& NewOffset);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Dish Customization|Events")
    FOnCustomizationEnded OnCustomizationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Dish Customization|Events")
    FOnDishDataUpdated OnDishDataUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Dish Customization|Events")
    FOnInitialDishDataReceived OnInitialDishDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Dish Customization|Events")
    FOnPlanningCompleted OnPlanningCompleted;

    // Alternative data passing methods
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data")
    void BroadcastDishDataUpdate(const FPUDishBase& NewDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data")
    void BroadcastInitialDishData(const FPUDishBase& InitialDishData);

    // UI Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    TSubclassOf<UUserWidget> CustomizationWidgetClass;

    // Original widget class (stored before switching to plating)
    UPROPERTY()
    TSubclassOf<UUserWidget> OriginalWidgetClass;

    // Widget class to spawn for cooking stage (should inherit from PUDishCustomizationWidget)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|UI")
    TSubclassOf<class UPUDishCustomizationWidget> CookingStageWidgetClass;

    // Widget class to spawn for plating stage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|UI")
    TSubclassOf<UUserWidget> PlatingWidgetClass;

    // Input Actions
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* ExitCustomizationAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* ControllerMouseAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* MouseClickAction;

    // Input Mapping Context for customization mode
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputMappingContext* CustomizationMappingContext;

    // Controller Mouse Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Controller Mouse")
    float ControllerMouseSensitivity = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Controller Mouse")
    float ControllerMouseDeadzone = 0.2f;

    // Camera Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationCameraDistance = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationCameraPitch = -25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CameraTransitionSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationOrthoWidth = 500.0f;

    // Cooking Stage Camera Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    float CookingCameraDistance = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    float CookingCameraPitch = -15.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    float CookingCameraYaw = 180.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    float CookingOrthoWidth = 600.0f;

    // Cooking Stage Camera Position Offsets
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    FVector CookingCameraPositionOffset = FVector(0.0f, 0.0f, 0.0f); // X=Left/Right, Y=Forward/Back, Z=Up/Down

    // Cooking Stage Camera Component Reference
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Cooking Camera")
    FName CookingStationCameraComponentName = TEXT("CookingCamera");

    // Plating Stage Camera Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    float PlatingCameraDistance = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    float PlatingCameraPitch = -15.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    float PlatingCameraYaw = 180.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    float PlatingOrthoWidth = 600.0f;

    // Plating Stage Camera Position Offsets
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    FVector PlatingCameraPositionOffset = FVector(0.0f, 0.0f, 0.0f); // X=Left/Right, Y=Forward/Back, Z=Up/Down

    // Plating Stage Camera Component Reference
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Plating Camera")
    FName PlatingStationCameraComponentName = TEXT("PlatingCamera");

    // Current dish being customized
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Data")
    FPUDishBase CurrentDishData;

    // Planning data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    FPUPlanningData CurrentPlanningData;

    // Planning mode flag
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    bool bInPlanningMode = false;

    // Data table references (for accessing ingredient and preparation data)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* IngredientDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* PreparationDataTable;

    // Plating dish mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Plating")
    TSoftObjectPtr<UStaticMesh> PlatingDishMesh;

    // Ingredient mesh scale for plating stage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Plating")
    FVector IngredientMeshScale = FVector(1.0f, 1.0f, 1.0f);

    // Original dish container mesh (stored when customization starts)
    UPROPERTY()
    UStaticMesh* OriginalDishContainerMesh = nullptr;

    // Original dish container children meshes (stored when customization starts)
    UPROPERTY()
    TArray<UStaticMesh*> OriginalDishContainerChildren;

protected:
    // Internal state management
    UPROPERTY()
    UUserWidget* CustomizationWidget;

    UPROPERTY()
    UPUDishCustomizationWidget* CookingStageWidget;

    UPROPERTY()
    AProjectUmeowmiCharacter* CurrentCharacter;

    // Input context management
    UPROPERTY()
    UInputMappingContext* OriginalMappingContext;

    // Input binding handles
    uint32 ExitActionBindingHandle;
    uint32 ControllerMouseBindingHandle;
    uint32 MouseClickBindingHandle;

    // Mouse interaction state
    bool bIsDragging = false;
    class APUIngredientMesh* CurrentlyDraggedIngredient = nullptr;
    FVector DragStartPosition;
    FVector DragStartMousePosition;
    FVector DragOffset; // Offset between mouse and ingredient when grabbed

    // Camera transition state
    bool bIsTransitioningCamera = false;
    float OriginalCameraDistance = 0.0f;
    float OriginalCameraPitch = 0.0f;
    float OriginalCameraYaw = 0.0f;
    float OriginalOrthoWidth = 0.0f;
    float OriginalCameraOffset = 0.0f;
    int32 OriginalCameraPositionIndex = 0;

    // Cooking stage camera component
    UPROPERTY()
    UCameraComponent* CookingStationCamera = nullptr;

    // Plating stage camera component
    UPROPERTY()
    UCameraComponent* PlatingStationCamera = nullptr;

private:
    // Spawn visual 3D mesh for ingredient
    void SpawnVisualIngredientMesh(const FIngredientInstance& IngredientInstance, const FVector& WorldPosition);
    float TargetCameraDistance = 0.0f;
    float TargetCameraPitch = 0.0f;
    float TargetCameraYaw = 0.0f;
    float TargetOrthoWidth = 0.0f;
    float TargetCameraOffset = 0.0f;
    int32 TargetCameraPositionIndex = 0;

    // Plating mode state
    bool bPlatingMode = false;

    // Plating placement tracking
    TMap<int32, int32> PlacedIngredientQuantities; // InstanceID -> Placed Quantity

    // Track spawned 3D ingredient meshes for cleanup
    TArray<class APUIngredientMesh*> SpawnedIngredientMeshes;

    // Plating camera transition state
    bool bPlatingCameraTransitioning = false;
    float PlatingCameraTransitionTime = 0.0f;
    float PlatingCameraTransitionDuration = 1.0f;
    FVector PlatingCameraStartLocation;
    FRotator PlatingCameraStartRotation;
    FVector PlatingCameraTargetLocation;
    FRotator PlatingCameraTargetRotation;
    float PlatingCameraStartOrthoWidth = 0.0f;
    float PlatingCameraTargetOrthoWidth = 0.0f;

    // Input handling
    void HandleExitInput();
    void HandleControllerMouse(const FInputActionValue& Value);
    void HandleMouseClick(const FInputActionValue& Value);
    void HandleMouseRelease(const FInputActionValue& Value);
    void UpdateMouseDrag();

    // Camera handling
    void StartCameraTransition(bool bToCustomization);
    void UpdateCameraTransition(float DeltaTime);

    // Cooking stage camera handling
    void StartCookingStageCameraTransition();
    void SwitchToCharacterCamera();

    // Plating stage camera handling
    void SetPlatingCameraPositionOffset(const FVector& NewOffset);
    void StartPlatingCameraTransition();
    void UpdatePlatingCameraTransition(float DeltaTime);

    // Plating placement limits
    bool CanPlaceIngredient(int32 InstanceID) const;
    int32 GetRemainingQuantity(int32 InstanceID) const;
    int32 GetPlacedQuantity(int32 InstanceID) const;
    void PlaceIngredient(int32 InstanceID);
    void RemoveIngredient(int32 InstanceID);

    // Blueprint-callable plating limits
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    bool CanPlaceIngredientByTag(const FGameplayTag& IngredientTag) const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    int32 GetRemainingQuantityByTag(const FGameplayTag& IngredientTag) const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    int32 GetPlacedQuantityByTag(const FGameplayTag& IngredientTag) const;

    // Update ingredient slot quantity display (for plating mode - uses slots, not buttons)
    void UpdateIngredientSlotQuantity(int32 InstanceID);

    // Reset all plating (restore original quantities and clear placed ingredients)
    UFUNCTION(BlueprintCallable, Category = "Plating")
    void ResetPlating();

    // Clear all 3D ingredient meshes
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void ClearAll3DIngredientMeshes();

    // Store original dish container mesh
    void StoreOriginalDishContainerMesh();
}; 