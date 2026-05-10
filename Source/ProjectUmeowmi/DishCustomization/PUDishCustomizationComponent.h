#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "PUPreparationBase.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "PUDishCustomizationComponent.generated.h"

// Forward declarations
class APUIngredientMesh;
class UUserWidget;
class UInputAction;
class UEnhancedInputComponent;
class UInputMappingContext;
class UNiagaraComponent;
struct FInputActionValue;

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
    virtual void BeginDestroy() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Activation/Deactivation
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void StartCustomization(AProjectUmeowmiCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void EndCustomization();

    // Check if currently customizing
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    bool IsCustomizing() const { return CurrentCharacter != nullptr; }

    /** True during EndCustomization teardown — stations must not call EndCustomization again from nested EndInteraction (focus/input chains). */
    UFUNCTION(BlueprintPure, Category = "Dish Customization")
    bool IsTearingDownCustomization() const { return bInEndCustomization; }

    /** UMG virtual pointer is active — OS hardware cursor must stay hidden (Slate capture / other code can re-enable it). */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Virtual Cursor")
    bool ShouldSuppressHardwareMouseCursor() const;

    /** After SetUserFocus/SetKeyboardFocus on dish UI, Slate may call UsePlatformCursorForCursorUser(true) — call this to restore faux cursor + hover sync. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Virtual Cursor")
    void ReassertVirtualCursorAfterUMGFocus(APlayerController* PC);

	/** Player currently in customization (null if not customizing). */
	AProjectUmeowmiCharacter* GetCurrentCharacter() const { return CurrentCharacter; }

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

    // Get spawn position above the cooking station/pan (for reliable ingredient placement)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    FVector GetSpawnPositionAboveStation() const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void SetPlatingMode(bool bInPlatingMode);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    bool IsPlatingMode() const;

    // True when 3D ingredient spawning is allowed (both cooking and plating stages)
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    bool CanSpawnIngredientsIn3D() const;

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void TransitionToPlatingStage(const FPUDishBase& DishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void EndPlatingStage();

	/** Scorecard RT: capture dish on station while primitives still exist. Used by EndPlatingStage and GoToStage (EndCustomization skips EndPlatingStage if bPlatingMode was cleared early). */
	void CaptureScorecardSnapshotFromPlatingStation();

    /** Capture ingredient positions/rotations from live meshes into CurrentDishData.PlatingEntries. Call before leaving plating (e.g. in GoToStage). */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void CapturePlatingTransformsFromMeshes();

    /** Dish container + plated ingredient primitives for scorecard snapshot (while still in plating). */
    void GatherDishSnapshotPrimitives(TArray<class UPrimitiveComponent*>& OutPrimitives) const;

    /** Plating camera used for framing; scorecard snapshot can match this view. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Camera")
    class UCameraComponent* GetPlatingStationCamera() const { return PlatingStationCamera; }

    /** Destroy all spawned ingredient meshes and liquid components. Call before RestoreOriginalDishContainerMesh to avoid physics/collision issues. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void ClearAll3DIngredientMeshes();

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

    // Stage navigation input actions
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* NextStageAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* PreviousStageAction;

    /** Increase/decrease quantity on the focused prep/active ingredient slot (bind in IMC_DishCustomization). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* QuantityIncreaseAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* QuantityDecreaseAction;

    // Input Mapping Context for customization mode (layered on top of the character's DefaultMappingContext; does not remove it).
    // Map IA_OpenJournal here too if it shares a key with another binding (e.g. Exit); higher-priority context wins for that key.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputMappingContext* CustomizationMappingContext;

    /** Priority for CustomizationMappingContext. Default 1 — keep character JournalMappingContext at a higher priority (e.g. 2) so journal overlays customization. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization", meta = (ClampMin = "1", UIMin = "1"))
    int32 CustomizationMappingContextPriority = 1;

    // Controller Mouse Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Virtual Cursor")
    float ControllerMouseSensitivity = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Virtual Cursor")
    float ControllerMouseDeadzone = 0.2f;

    /** UMG widget for the on-screen pointer (e.g. Image with your texture). Optionally reparent to UPUVirtualCursorUserWidget for press/release visuals. Assign on the component / BP defaults. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Virtual Cursor")
    TSubclassOf<UUserWidget> VirtualCursorWidgetClass;

    /** Draw above dish UMG. Runtime uses max(this, PUDishVirtualCursorViewportZOrder) so the cursor stays above the scoring stack (~50152). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Virtual Cursor", meta = (ClampMin = "0"))
    int32 VirtualCursorZOrder = 10000;

    /**
     * Hotspot offset in viewport pixels: distance from the widget's top-left to the point that should sit on the logical cursor (clicks, hover, traces).
     * Example: 32×32 crosshair — use (16, 16) to center. Arrow with tip 8px from left, 4px from top — use (8, 4). Default (0,0) = widget top-left = hotspot.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Virtual Cursor")
    FVector2D VirtualCursorHotspotOffset = FVector2D::ZeroVector;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> VirtualCursorWidgetInstance = nullptr;

    /**
     * Default ON: UI-first flow — no spring-arm customization framing on enter/exit, no world ingredient spawn/drag,
     * no plating bowl mesh swap. Set false only if you restore legacy 3D station framing (SwitchToCookingCamera / plating cameras).
     * OnCustomizationEnded fires next tick after EndCustomization completes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|2D Mode")
    bool bUse2DCustomizationMode = true;

    UFUNCTION(BlueprintPure, Category = "Dish Customization|2D Mode")
    bool IsUsing2DCustomizationMode() const { return bUse2DCustomizationMode; }

    // Cooking Stage Camera Management (legacy 3D station view — skipped when bUse2DCustomizationMode)
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

    // Height above dish container to spawn ingredients (avoids collision with rim/platform)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Plating", meta = (ClampMin = "10.0", UIMin = "10.0"))
    float IngredientSpawnHeightOffset = 30.0f;

    // Blueprint class for spawned 3D ingredient meshes (set DefaultMaterial, HoverMaterial, GrabbedMaterial here)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Plating")
    TSubclassOf<class APUIngredientMesh> IngredientMeshClass;

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

    /** Blocks re-entrant EndCustomization (Slate focus / interaction code can call EndInteraction → EndCustomization mid-teardown). */
    bool bInEndCustomization = false;

    // Input binding handles
    uint32 ExitActionBindingHandle;
    uint32 ControllerMouseBindingHandle;
    uint32 MouseClickBindingHandle;
    FDelegateHandle PreInputMouseDownHandle;  // Slate pre-input listener (bypasses widget consumption)
    uint32 NextStageBindingHandle;
    uint32 PreviousStageBindingHandle;
    uint32 QuantityIncreaseBindingHandle;
    uint32 QuantityDecreaseBindingHandle;

    // Mouse interaction state
    bool bIsDragging = false;
    bool bWasMouseDown = false;  // For Tick-based click fallback when widget blocks Enhanced Input

    /** DefaultViewportMouseCaptureMode is often CapturePermanently_* — that requires a click before the OS cursor tracks; controller virtual cursor needs NoCapture. */
    bool bHasSavedViewportMouseCaptureForCustomization = false;
    EMouseCaptureMode SavedViewportMouseCaptureModeForCustomization = EMouseCaptureMode::NoCapture;
    class APUIngredientMesh* CurrentlyDraggedIngredient = nullptr;
    FVector DragStartPosition;
    FVector DragStartMousePosition;
    FVector DragOffset; // Offset between mouse and ingredient when grabbed

    // Cooking stage camera component
    UPROPERTY()
    UCameraComponent* CookingStationCamera = nullptr;

    // Plating stage camera component
    UPROPERTY()
    UCameraComponent* PlatingStationCamera = nullptr;

private:
	/** Unlock any ingredients in the dish that aren't already in the pantry (so they appear when customization starts). */
	void EnsureDishIngredientsInPantry(const FPUDishBase& Dish);

    // Spawn visual 3D mesh for ingredient
    void SpawnVisualIngredientMesh(const FIngredientInstance& IngredientInstance, const FVector& WorldPosition);

    // Plating mode state
    bool bPlatingMode = false;

    // Plating placement tracking
    TMap<int32, int32> PlacedIngredientQuantities; // InstanceID -> Placed Quantity

    // Track spawned 3D ingredient meshes for cleanup (weak: actors can self-Destroy e.g. GroundDestroyZThreshold in Tick)
    TArray<TWeakObjectPtr<APUIngredientMesh>> SpawnedIngredientMeshes;

    // Track spawned liquid Niagara components for cleanup (allows multiple per InstanceID when Quantity > 1)
    TArray<TPair<int32, TObjectPtr<UNiagaraComponent>>> SpawnedLiquidComponents;

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

    /** Right-stick cursor in viewport pixels; sole source of truth during customization (not GetMousePosition / hardware mouse). */
    FVector2D VirtualCursorViewport = FVector2D::ZeroVector;
    bool bVirtualCursorInitialized = false;

    /** Last MouseClickAction (Started) was routed to Slate as a synthetic LMB down (UMG under virtual cursor); release must send synthetic LMB up. */
    bool bVirtualClickConsumedBySlateUI = false;

    /** True while we're inside ProcessMouseButtonDownEvent for a synthetic virtual-cursor click — that call re-fires pre-input listeners; without this, OnPreInputMouseButtonDown -> HandleMouseClick recurses until stack overflow. */
    bool bInsideSyntheticSlateMouseDispatch = false;

    /** Virtual cursor in Slate "virtual desktop" pixels (from SceneViewport::ViewportToVirtualDesktopPixel). FSlateUser::GetCursorPosition can be invalid when OS cursor is hidden. */
    FVector2D LastVirtualCursorDesktopAbs = FVector2D::ZeroVector;
    bool bLastVirtualCursorDesktopValid = false;

    /** Last extents used for virtual cursor clamp (see TryGetVirtualCursorViewportPixelExtents); detect resize without stick input. */
    int32 CachedVirtualCursorViewportExtentsX = 0;
    int32 CachedVirtualCursorViewportExtentsY = 0;

    bool TryComputeVirtualCursorDesktopAbsolute(APlayerController* PC, FVector2D& OutDesktopAbs) const;

    /** Use FSceneViewport size when available — matches ViewportToVirtualDesktopPixel and stays consistent across PIE / selected viewport / resize. Falls back to GetViewportSize. */
    bool TryGetVirtualCursorViewportPixelExtents(APlayerController* PC, int32& OutW, int32& OutH) const;

    void ApplyVirtualCursorVisual(APlayerController* PC);
    /** Release Slate pointer capture and clear a stuck synthetic LMB-down before/after virtual-cursor sessions (fixes UMG hover/clicks on re-open). */
    void FlushSlateVirtualCursorPointerState(APlayerController* PC);
    /** Slate/UI capture can re-show the hardware cursor; call when using VirtualCursorWidgetClass. */
    void ApplyVirtualCursorHardwareCursorLock(APlayerController* PC) const;
    /** Re-apply lock on the next frame — some Slate paths toggle cursor after we return. */
    void ScheduleVirtualCursorHardwareCursorLockNextFrame(APlayerController* PC);
    /** Pair with ApplyVirtualCursorHardwareCursorLock when leaving customization (restore real OS cursor for menus/desktop). */
    static void RestoreSlatePlatformCursorForUser();
    /** If VirtualCursorWidgetInstance is a UPUVirtualCursorUserWidget, forwards press/release for BP visuals. */
    void NotifyVirtualCursorInteractVisual(bool bPressed);
    FVector2D GetVirtualCursorScreenPosition(APlayerController* PC) const;
    void OnCustomizationViewportDeferredSetup();
    /** Runs OnCustomizationEnded next tick so Blueprint ReceiveEndInteraction / delegate graphs cannot re-enter EndCustomization during synchronous teardown. */
    void BroadcastOnCustomizationEndedNextTick();
    void OnPreInputMouseButtonDown(const struct FPointerEvent& MouseEvent);  // Slate pre-input (before widgets consume)
    void HandleMouseClick(const FInputActionValue& Value);
    void HandleMouseRelease(const FInputActionValue& Value);
    void HandleNextStage();
    void HandlePreviousStage();
    void HandleQuantityIncrease(const FInputActionValue& Value);
    void HandleQuantityDecrease(const FInputActionValue& Value);
    void UpdateMouseDrag();

    // Cooking / plating station cameras (legacy 3D paths)
    void SetPlatingCameraPositionOffset(const FVector& NewOffset);
    void StartPlatingCameraTransition(const FVector* ExplicitStartLocation = nullptr, const FRotator* ExplicitStartRotation = nullptr, float ExplicitStartOrthoWidth = -1.0f);
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

    // Store original dish container mesh
    void StoreOriginalDishContainerMesh();

    // Get plate/dish surface height for drag projection (matches GetSpawnPositionAboveStation surface)
    bool GetPlateSurfaceHeight(float& OutSurfaceHeight) const;

    // Get plate surface height and center point (for view-independent drag projection)
    bool GetPlateSurfaceInfo(float& OutSurfaceHeight, FVector& OutSurfaceCenter) const;
}; 