#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "PUPreparationBase.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Layout/WidgetPath.h"
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

    /** Locate the UMG widget path under the virtual cursor (for slot activation / synthetic clicks). */
    bool TryLocateVirtualCursorWidgetPath(APlayerController* PC, FWidgetPath& OutPath) const;

	/** Player currently in customization (null if not customizing). */
	AProjectUmeowmiCharacter* GetCurrentCharacter() const { return CurrentCharacter; }

    // Dish data management
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void UpdateCurrentDishData(const FPUDishBase& NewDishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    /** True when current dish row defines CustomizationStages (Phase 2 pipeline). */
    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    bool HasActiveCustomizationPipeline() const;

    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    int32 GetCustomizationPipelineStageCount() const;

    /** INDEX_NONE when no pipeline; otherwise active stage index in CustomizationStages. */
    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    int32 GetActiveCustomizationPipelineIndex() const { return ActiveCustomizationPipelineIndex; }

    /** Alias for GetActiveCustomizationPipelineIndex — 0-based pipeline row during customization. */
    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline", meta = (DisplayName = "Get Current Stage Index"))
    int32 GetCurrentStageIndex() const { return GetActiveCustomizationPipelineIndex(); }

    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    bool TryGetActivePipelineStage(FPUDishCustomizationStageDescriptor& OutStage) const;

    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    bool TryGetPipelineStageByIndex(int32 Index, FPUDishCustomizationStageDescriptor& OutStage) const;

    /** Convenience: active row's StageDisplayName, or readable StageId fallback. Empty when no active pipeline stage. */
    UFUNCTION(BlueprintPure, Category = "Dish Customization|Pipeline")
    FText GetActiveCustomizationPipelineStageDisplayName() const;

    /** Reset to stage 0 when the dish has a pipeline; otherwise INDEX_NONE. Call when entering customization. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Pipeline")
    void ResetCustomizationPipelineProgress();

    /** Move to next stage; returns false if no pipeline or already past last stage. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Pipeline")
    bool AdvanceCustomizationPipeline();

    /** Clamp Index into pipeline range or noop when invalid / no pipeline. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Pipeline")
    void SetActiveCustomizationPipelineIndex(int32 Index);

    /** Refreshes rail + mounted stage module on the active customization widget after pipeline index changes. */
    void RefreshActiveWidgetPipelinePresentation();

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

    /** Clears dead widget pointers held on this component (shutdown). */
    void SanitizeStaleWidgetReferences();

    // Function to set the initial dish data from an order
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Orders")
    void SetInitialDishData(const FPUDishBase& InitialDishData);

    // Function to set the data table references
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    void SetDataTables(UDataTable* DishTable, UDataTable* IngredientTable, UDataTable* PreparationTable);

    // Function to get ingredient data for the widget
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    TArray<FPUIngredientBase> GetIngredientData() const;

    /** Unlocked ingredients filtered for pantry display. Slot type filter supersedes stage parent tags when non-empty. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Data Tables")
    TArray<FPUIngredientBase> GetPantryEligibleIngredients(
        const FGameplayTagContainer& SlotRequiredTypes,
        const FGameplayTagContainer& StageParentTags,
        const FGameplayTag& TutorialAllowedIngredientTag) const;

    UFUNCTION(BlueprintPure, Category = "Dish Customization|Data Tables")
    UDataTable* GetIngredientTypeDataTable() const { return IngredientTypeDataTable; }

    /** Cover art for pipeline stage triptych transitions (`FPUDishCustomizationTriptychRow`). */
    UFUNCTION(BlueprintPure, Category = "Dish Customization|Data Tables")
    UDataTable* GetTriptychDataTable() const { return TriptychDataTable; }

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

    /** Station snapshot pipeline removed — always null (scorecard uses preview / other capture paths). */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Camera")
    class UCameraComponent* GetPlatingStationCamera() const { return nullptr; }

    /** Destroy any leftover spawned mesh actors / Niagara (typically unused in UI-only customization). */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void ClearAll3DIngredientMeshes();

    /** Legacy hook for world ingredient actors — UI-only customization; no-op. */
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void StartDraggingIngredient(APUIngredientMesh* Ingredient);

    // Ingredient dragging was used with world mesh actors (removed).
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Plating")
    void ResetPlatingPlacements();

    // Planning mode functions
    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    void StartPlanningMode();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    void TransitionToCookingStage(const FPUDishBase& DishData);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization|Planning")
    bool IsInPlanningMode() const { return bInPlanningMode; }

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

    /** Icons/labels for Ingredient.Type.* tags (optional; also configurable on Game Instance). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* IngredientTypeDataTable;

    /** Three-panel cover art per pipeline stage (`FPUDishCustomizationTriptychRow`). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* TriptychDataTable = nullptr;

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

    /** Index into CurrentDishData.CustomizationStages while session uses data pipeline (Phase 2). */
    int32 ActiveCustomizationPipelineIndex = INDEX_NONE;

    // Input binding handles
    uint32 ExitActionBindingHandle;
    uint32 ControllerMouseBindingHandle;
    uint32 MouseClickBindingHandle;
    FDelegateHandle PreInputMouseDownHandle;  // Slate pre-input listener (bypasses widget consumption)
    uint32 NextStageBindingHandle;
    uint32 PreviousStageBindingHandle;
    uint32 QuantityIncreaseBindingHandle;
    uint32 QuantityDecreaseBindingHandle;
    bool bCustomizationControllerFaceButtonsBound = false;

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

private:
	/** Unlock any ingredients in the dish that aren't already in the pantry (so they appear when customization starts). */
	void EnsureDishIngredientsInPantry(const FPUDishBase& Dish);

    // Plating mode state
    bool bPlatingMode = false;

    // Plating placement tracking
    TMap<int32, int32> PlacedIngredientQuantities; // InstanceID -> Placed Quantity

    // Track spawned 3D ingredient meshes for cleanup (weak: actors can self-Destroy e.g. GroundDestroyZThreshold in Tick)
    TArray<TWeakObjectPtr<APUIngredientMesh>> SpawnedIngredientMeshes;

    // Track spawned liquid Niagara components for cleanup (allows multiple per InstanceID when Quantity > 1)
    TArray<TPair<int32, TObjectPtr<UNiagaraComponent>>> SpawnedLiquidComponents;

    // Input handling
    void HandleExitInput();
    void HandleControllerMouse(const FInputActionValue& Value);
    void HandleControllerSlotActivate();
    void HandleControllerMinigameToggle();
    void BindCustomizationControllerFaceButtons(APlayerController* PlayerController);
    void UnbindCustomizationControllerFaceButtons(APlayerController* PlayerController);
    void PollIngredientRailControllerNavigation(APlayerController* PlayerController);

    UPUDishCustomizationWidget* GetActiveDishCustomizationWidget() const;

    bool TryActivateIngredientSlotUnderVirtualCursor(APlayerController* PC);
    bool TryToggleStageMinigameUnderVirtualCursorOrFocus(APlayerController* PC);

    bool DispatchVirtualCursorPointerDown(APlayerController* PC, const FWidgetPath& Path);
    bool DispatchVirtualCursorPointerUp(APlayerController* PC, const FWidgetPath& Path);

    /** Right-stick cursor in viewport pixels; sole source of truth during customization (not GetMousePosition / hardware mouse). */
    FVector2D VirtualCursorViewport = FVector2D::ZeroVector;
    bool bVirtualCursorInitialized = false;

    /** Last MouseClickAction (Started) was routed to Slate as a synthetic LMB down (UMG under virtual cursor); release must send synthetic LMB up. */
    bool bVirtualClickConsumedBySlateUI = false;

    /** Widget path hit on the last synthetic virtual-cursor pointer down (paired with RoutePointerUpEvent on release). */
    FWidgetPath LastVirtualClickWidgetPath;
    bool bLastVirtualClickWidgetPathValid = false;

    /** True while we're inside ProcessMouseButtonDownEvent for a synthetic virtual-cursor click — that call re-fires pre-input listeners; without this, OnPreInputMouseButtonDown -> HandleMouseClick recurses until stack overflow. */
    bool bInsideSyntheticSlateMouseDispatch = false;

    /** Virtual cursor in Slate "virtual desktop" pixels (from SceneViewport::ViewportToVirtualDesktopPixel). FSlateUser::GetCursorPosition can be invalid when OS cursor is hidden. */
    FVector2D LastVirtualCursorDesktopAbs = FVector2D::ZeroVector;
    bool bLastVirtualCursorDesktopValid = false;

    /** Last extents used for virtual cursor clamp (see TryGetVirtualCursorViewportPixelExtents); detect resize without stick input. */
    int32 CachedVirtualCursorViewportExtentsX = 0;
    int32 CachedVirtualCursorViewportExtentsY = 0;

    float CustomizationUIGCSanitizeAccumulator = 0.f;

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

    // Get plate/dish surface height for drag projection (matches GetSpawnPositionAboveStation surface)
    bool GetPlateSurfaceHeight(float& OutSurfaceHeight) const;

    // Get plate surface height and center point (for view-independent drag projection)
    bool GetPlateSurfaceInfo(float& OutSurfaceHeight, FVector& OutSurfaceCenter) const;
}; 