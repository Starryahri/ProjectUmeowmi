#include "PUDishCustomizationWidget.h"
#include "PURadarChart.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../ProjectUmeowmiCharacter.h"
#include "PUDialogueBox.h"
#include "PUJournalWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "PUPopupData.h"
#include "PUIngredientButton.h"
#include "PUIngredientQuantityControl.h"
#include "PUIngredientSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "UObject/GarbageCollection.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SObjectWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    // Enables verbose logging for dish/ingredient data reception and slot population.
    constexpr bool bPU_LogDishDataReceiveDebug = false;

    static UWidget* GetWidgetObjectFromSlate(const TSharedPtr<SWidget>& SlateWidget)
    {
        for (TSharedPtr<SWidget> Current = SlateWidget; Current.IsValid(); Current = Current->GetParentWidget())
        {
            if (Current->GetType() == FName(TEXT("SObjectWidget")))
            {
                return static_cast<SObjectWidget*>(Current.Get())->GetWidgetObject();
            }
        }
        return nullptr;
    }

    static bool IsWidgetDescendantOf(UWidget* Widget, UWidget* PotentialAncestor)
    {
        for (UWidget* W = Widget; W; W = W->GetParent())
        {
            if (W == PotentialAncestor)
            {
                return true;
            }
        }
        return false;
    }

    static bool IngredientInstanceHasAnyPreparation(const FIngredientInstance& Inst)
    {
        return Inst.Preparations.Num() > 0 || Inst.IngredientData.ActivePreparations.Num() > 0;
    }

    static uint32 HashGameplayTagContainerStable(const FGameplayTagContainer& Container)
    {
        TArray<FGameplayTag> Tags;
        Container.GetGameplayTagArray(Tags);
        Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B) {
            return A.GetTagName().FastLess(B.GetTagName());
        });
        uint32 Hash = 0;
        for (const FGameplayTag& Tag : Tags)
        {
            Hash = HashCombine(Hash, GetTypeHash(Tag));
        }
        return Hash;
    }

    static uint32 ComputeRecipeLogBaseContentSignature(const TArray<FIngredientInstance>& BaseOrdered)
    {
        uint32 Hash = 0;
        for (const FIngredientInstance& I : BaseOrdered)
        {
            Hash = HashCombine(Hash, GetTypeHash(I.InstanceID));
            Hash = HashCombine(Hash, GetTypeHash(I.IngredientTag));
        }
        return Hash;
    }

    static uint32 ComputeRecipeLogPreppedContentSignature(const TArray<FIngredientInstance>& PreppedOrdered)
    {
        uint32 Hash = 0;
        for (const FIngredientInstance& I : PreppedOrdered)
        {
            Hash = HashCombine(Hash, GetTypeHash(I.InstanceID));
            Hash = HashCombine(Hash, GetTypeHash(I.IngredientTag));
            Hash = HashCombine(Hash, HashGameplayTagContainerStable(I.Preparations));
            Hash = HashCombine(Hash, HashGameplayTagContainerStable(I.IngredientData.ActivePreparations));
            Hash = HashCombine(Hash, GetTypeHash(I.TimeValue));
            Hash = HashCombine(Hash, GetTypeHash(I.TemperatureValue));
        }
        return Hash;
    }

    static bool AreRecipeLogSlotsValid(const TArray<UPUIngredientSlot*>& Slots)
    {
        for (UPUIngredientSlot* SlotWidget : Slots)
        {
            if (!IsValid(SlotWidget))
            {
                return false;
            }
        }
        return true;
    }

    static UPanelWidget* FindRecipeLogPanelByNames(UUserWidget* Owner, std::initializer_list<FName> Names)
    {
        if (!Owner)
        {
            return nullptr;
        }
        for (FName WidgetName : Names)
        {
            UWidget* Found = Owner->WidgetTree ? Owner->WidgetTree->FindWidget(WidgetName) : nullptr;
            if (!Found)
            {
                Found = Owner->GetWidgetFromName(WidgetName);
            }
            if (UPanelWidget* Panel = Cast<UPanelWidget>(Found))
            {
                return Panel;
            }
        }
        return nullptr;
    }

    /** Editor UMG designer/preview + incremental GC: avoid dynamic spawn there (PIE/game use teardown + normal GC lifecycle). */
    static bool PU_ShouldSpawnDishPantryLikeDynamicWidgets(const UWorld* World, const UWidget* OwnerWidget)
    {
        if (!World || !OwnerWidget)
        {
            return false;
        }
#if WITH_EDITOR
        if (OwnerWidget->IsDesignTime() || OwnerWidget->IsPreviewTime())
        {
            return false;
        }
#endif
        switch (World->WorldType)
        {
        case EWorldType::Game:
        case EWorldType::PIE:
        case EWorldType::GamePreview:
            return true;
        default:
            return false;
        }
    }

    static void ReassertVirtualCursorAfterSlotFocus(UPUDishCustomizationWidget* DishWidget, APlayerController* PC)
    {
        if (!DishWidget || !PC)
        {
            return;
        }
        if (UPUDishCustomizationComponent* Comp = DishWidget->GetCustomizationComponent())
        {
            Comp->ReassertVirtualCursorAfterUMGFocus(PC);
        }
    }

    /** Walks ancestors for a standalone quantity widget (slots no longer embed PUIngredientQuantityControl). */
    static UPUIngredientQuantityControl* FindQuantityControlFromLeaf(UWidget* Leaf)
    {
        for (UWidget* W = Leaf; W; W = W->GetParent())
        {
            if (UPUIngredientQuantityControl* QC = Cast<UPUIngredientQuantityControl>(W))
            {
                return QC;
            }
        }
        return nullptr;
    }

    /** Gamepad UI navigation sets user focus; keyboard sets keyboard focus — use both. */
    static TSharedPtr<SWidget> GetBestFocusedSlateForOwner(const UWidget* OwnerWidget)
    {
        if (!FSlateApplication::IsInitialized())
        {
            return nullptr;
        }
        uint32 UserIndex = 0;
        if (OwnerWidget)
        {
            if (const ULocalPlayer* LP = OwnerWidget->GetOwningLocalPlayer())
            {
                UserIndex = static_cast<uint32>(FMath::Max(0, LP->GetLocalPlayerIndex()));
            }
        }
        TSharedPtr<SWidget> W = FSlateApplication::Get().GetUserFocusedWidget(UserIndex);
        if (W.IsValid())
        {
            return W;
        }
        return FSlateApplication::Get().GetKeyboardFocusedWidget();
    }

    static void SanitizeRadarChartsInWidgetTree(UWidgetTree* InWidgetTree)
    {
        if (!InWidgetTree)
        {
            return;
        }
        TArray<UWidget*> AllWidgets;
        InWidgetTree->GetAllWidgets(AllWidgets);
        for (UWidget* W : AllWidgets)
        {
            if (URadarChart* Radar = Cast<URadarChart>(W))
            {
                UPURadarChart::SanitizeObjectReferencesOnAnyRadar(Radar);
            }
        }
    }
}

UPUDishCustomizationWidget::UPUDishCustomizationWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CustomizationComponent(nullptr)
{
}

void UPUDishCustomizationWidget::NativeConstruct()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - STARTING WIDGET CONSTRUCTION"));
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget name: %s"), *GetName());
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget class: %s"), *GetClass()->GetName());
    
    Super::NativeConstruct();

    SanitizeRadarChartsInWidgetTree(WidgetTree);

    TryResolveRecipeLogPanelsFromHierarchy();

    // Recipe log: no timers/ticks — one synchronous build when allowed (PIE/game; never UMG design/preview).
    if (PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        RefreshRecipeLog();
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Super::NativeConstruct completed"));
    
    // Check if we're in the game world
    UWorld* World = GetWorld();
    if (World)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World valid: %s"), *World->GetName());
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - World type: %s"), 
        //    World->IsGameWorld() ? TEXT("Game World") : TEXT("Editor World"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - No World available"));
    }
    
    // Check widget visibility
    if (IsVisible())
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is visible"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT visible"));
    }
    
    // Check if we're in viewport
    if (IsInViewport())
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - Widget is in viewport"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::NativeConstruct - Widget is NOT in viewport"));
    }
    
    // Note: Don't subscribe to events here - the component reference isn't set yet
    // Subscription will happen in SetCustomizationComponent()
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::NativeConstruct - WIDGET CONSTRUCTION COMPLETED"));
}

void UPUDishCustomizationWidget::NativeDestruct()
{
    // //UE_LOG(LogTemp,Display, TEXT("PUDishCustomizationWidget::NativeDestruct - Widget destructing"));

    // Clear any pending timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialFocusTimerHandle);
    }

    UnsubscribeFromEvents();

    ReleaseProgrammaticCustomizationSlots();

    Super::NativeDestruct();
}

void UPUDishCustomizationWidget::ReleaseProgrammaticCustomizationSlots()
{
    TeardownRecipeLogDynamicWidgets(false);

    TArray<UPUIngredientButton*> ButtonsToRelease;
    IngredientButtonMap.GenerateValueArray(ButtonsToRelease);
    IngredientButtonMap.Empty();
    for (UPUIngredientButton* Btn : ButtonsToRelease)
    {
        if (IsValid(Btn))
        {
            Btn->OnIngredientButtonClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnIngredientButtonClicked);
            Btn->RemoveFromParent();
            Btn->ReleaseSlateResources(true);
        }
    }

    auto UnbindIngredientSlotDelegates = [&](UPUIngredientSlot* S)
    {
        if (!IsValid(S))
        {
            return;
        }
        S->OnEmptySlotClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnEmptySlotClicked);
        S->OnEmptySlotClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);
        S->OnSlotIngredientChanged.RemoveDynamic(this, &UPUDishCustomizationWidget::OnQuantityControlChanged);
        S->OnIngredientDroppedOnSlot.RemoveDynamic(this, &UPUDishCustomizationWidget::OnPlatingIngredientDropped);
        S->RemoveFromParent();
        S->ReleaseSlateResources(true);
    };

    for (UPUIngredientSlot* S : CreatedIngredientSlots)
    {
        UnbindIngredientSlotDelegates(S);
    }
    CreatedIngredientSlots.Empty();

    auto TearDownShelving = [&](TArray<UUserWidget*>& ShelvingArray, TWeakObjectPtr<UUserWidget>& CurrentShelving, int32& CurrentCount)
    {
        for (UUserWidget* W : ShelvingArray)
        {
            if (IsValid(W))
            {
                W->RemoveFromParent();
                W->ReleaseSlateResources(true);
            }
        }
        ShelvingArray.Empty();
        CurrentShelving.Reset();
        CurrentCount = 0;
    };

    TearDownShelving(CreatedShelvingWidgets, CurrentShelvingWidget, CurrentShelvingWidgetSlotCount);

    for (UPUIngredientSlot* S : CreatedPantrySlots)
    {
        UnbindIngredientSlotDelegates(S);
    }
    CreatedPantrySlots.Empty();
    TearDownShelving(CreatedPantryShelvingWidgets, CurrentPantryShelvingWidget, CurrentPantryShelvingWidgetSlotCount);
    bPantrySlotsCreated = false;

    for (UPUIngredientSlot* S : CreatedPreppedSlots)
    {
        UnbindIngredientSlotDelegates(S);
    }
    CreatedPreppedSlots.Empty();

    for (UPUIngredientSlot* S : CreatedPreppedPantrySlots)
    {
        UnbindIngredientSlotDelegates(S);
    }
    CreatedPreppedPantrySlots.Empty();
    TearDownShelving(CreatedPreppedPantryShelvingWidgets, CurrentPreppedPantryShelvingWidget, CurrentPreppedPantryShelvingWidgetSlotCount);

    CachedPreppedPantrySlotsContentSignature = 0;
    bPreppedPantrySlotsHierarchyBuilt = false;

    IngredientSlotMap.Empty();
    PantrySlotMap.Empty();
    PreppedSlotMap.Empty();
    PendingEmptySlot.Reset();
    bIngredientSlotsCreated = false;

    CachedRecipeLogBaseSignature = 0;
    CachedRecipeLogPreppedSignature = 0;
    CachedRecipeLogSlotsPerRowForRecipeLog = -1;
    CachedRecipeLogMaxBaseForRecipeLog = -1;
    bRecipeLogBaseHierarchyBuilt = false;
    bRecipeLogPreppedHierarchyBuilt = false;
}

void UPUDishCustomizationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // NativeTick is available for any real-time updates needed
    // Currently used for potential future features
}

void UPUDishCustomizationWidget::OnInitialDishDataReceived(const FPUDishBase& InitialDishData)
{
    if (bPU_LogDishDataReceiveDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - RECEIVED INITIAL DISH DATA"));
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Widget name: %s"), *GetName());
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Received initial dish data: %s with %d ingredients"),
        //    *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());

        // Log dish details
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish tag: %s"), *InitialDishData.DishTag.ToString());
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Dish display name: %s"), *InitialDishData.DisplayName.ToString());

        // Log ingredient details
        for (int32 i = 0; i < InitialDishData.IngredientInstances.Num(); i++)
        {
            const FIngredientInstance& Instance = InitialDishData.IngredientInstances[i];
            //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Ingredient %d: %s (Qty: %d, ID: %d)"),
            //    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity, Instance.InstanceID);
        }
    }
    
    // Update current dish data
    if (bPU_LogDishDataReceiveDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Updating current dish data"));
    }
    CurrentDishData = InitialDishData;
    
    // Planning/cooking entry starts with blank IngredientInstances (player gathers from pantry).
    // Keep DishTag, DisplayName, etc. for the ending stage, but clear ingredients so prepped area is empty.
    CurrentDishData.IngredientInstances.Empty();
    // Clear any existing prepped slots (e.g. from SetPreppedIngredientContainer called before this)
    for (UPUIngredientSlot* PreppedSlot : CreatedPreppedSlots)
    {
        if (PreppedSlot && PreppedSlot->IsValidLowLevel() && PreppedSlot->GetParent())
        {
            PreppedSlot->RemoveFromParent();
        }
    }
    CreatedPreppedSlots.Empty();
    PreppedSlotMap.Empty();
    // Sync cleared dish data back to component
    if (CustomizationComponent)
    {
        CustomizationComponent->UpdateCurrentDishData(CurrentDishData);
    }
    
    RefreshPreppedPantrySlots();
    RefreshRecipeLog();

    RefreshRadarChartsFromDishData(CurrentDishData);

    // Cooking stage: set up controller navigation and focus on first slot (same pattern as prep stage)
    if (StageType == EDishCustomizationStageType::Cooking && CreatedPreppedSlots.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 PUDishCustomizationWidget::OnInitialDishDataReceived - Cooking stage: setting up navigation and focus on first slot"));
        SetupCookingSlotNavigation();
        SetInitialFocusForCookingStage();
    }
    
    // Call the Blueprint event
    if (bPU_LogDishDataReceiveDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("📥 PUDishCustomizationWidget::OnInitialDishDataReceived - Calling Blueprint event OnDishDataReceived"));
    }
    OnDishDataReceived(InitialDishData);
    
    if (bPU_LogDishDataReceiveDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::OnInitialDishDataReceived - INITIAL DISH DATA PROCESSED SUCCESSFULLY"));
    }
}

void UPUDishCustomizationWidget::OnDishDataUpdated(const FPUDishBase& UpdatedDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("PUDishCustomizationWidget::OnDishDataUpdated - Received dish data update: %s with %d ingredients"), 
    //    *UpdatedDishData.DisplayName.ToString(), UpdatedDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = UpdatedDishData;
    
    // Update radar charts (flavor/texture) when dish changes - e.g. when preparations are applied
    RefreshRadarChartsFromDishData(UpdatedDishData);
    
    // Call the Blueprint event
    OnDishDataChanged(UpdatedDishData);

    RefreshPreppedPantrySlots();
    RefreshRecipeLog();
}

void UPUDishCustomizationWidget::OnCustomizationEnded()
{
    //UE_LOG(LogTemp,Display, TEXT("PUDishCustomizationWidget::OnCustomizationEnded - Customization ended"));
    
    // Call the Blueprint event
    OnCustomizationModeEnded();
}

void UPUDishCustomizationWidget::SetCustomizationComponent(UPUDishCustomizationComponent* Component)
{
    //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - STARTING COMPONENT CONNECTION"));
    //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Widget name: %s"), *GetName());
    //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Component: %s"), 
    //    Component ? *Component->GetName() : TEXT("NULL"));
    
    // Unsubscribe from previous component if any
    if (CustomizationComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Unsubscribing from previous component: %s"), *CustomizationComponent->GetName());
        UnsubscribeFromEvents();
    }
    
    // Set the new component reference
    CustomizationComponent = Component;
    
    if (CustomizationComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Component reference set successfully"));
        
        // Subscribe to the new component's events
        //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - Subscribing to component events"));
        SubscribeToEvents();
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::SetCustomizationComponent - Event subscription completed"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::SetCustomizationComponent - Component reference is NULL"));
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🔗 PUDishCustomizationWidget::SetCustomizationComponent - COMPONENT CONNECTION COMPLETED"));
}

void UPUDishCustomizationWidget::UpdateDishData(const FPUDishBase& NewDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("PUDishCustomizationWidget::UpdateDishData - Updating dish data: %s with %d ingredients"), 
    //    *NewDishData.DisplayName.ToString(), NewDishData.IngredientInstances.Num());
    
    // Update local data
    CurrentDishData = NewDishData;
    
    // Sync back to the customization component (broadcasts OnDishDataUpdated -> radar charts + Blueprint OnDishDataChanged)
    if (CustomizationComponent)
    {
        CustomizationComponent->SyncDishDataFromUI(NewDishData);
    }
    else
    {
        // No component - still trigger radar chart update so SetValuesFromOrder* in Blueprint gets the new dish
        RefreshRadarChartsFromDishData(NewDishData);
        OnDishDataChanged(NewDishData);
    }

    RefreshPreppedPantrySlots();
    RefreshRecipeLog();
}

FText UPUDishCustomizationWidget::GetEndingStageTextForCurrentDish() const
{
    UWorld* World = GetWorld();
    UPUProjectUmeowmiGameInstance* GI = World ? World->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
    if (!GI || !CurrentDishData.DishTag.IsValid())
    {
        return FText::GetEmpty();
    }
    FPUDishBase BaseRecipeDish;
    if (!GI->GetDishDataForTag(CurrentDishData.DishTag, BaseRecipeDish))
    {
        return FText::GetEmpty();
    }
    return UPUDishBlueprintLibrary::GetEndingStageText(CurrentDishData, BaseRecipeDish);
}

void UPUDishCustomizationWidget::GoToStage(UPUDishCustomizationWidget* TargetStage)
{
    if (!TargetStage)
    {
        //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToStage - Target stage is null"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Navigating from %s (Stage: %d) to %s (Stage: %d)"), 
    //    *GetName(), (int32)StageType, *TargetStage->GetName(), (int32)TargetStage->StageType);

    // Handle cleanup when leaving current stage (capture plating transforms FIRST when leaving plating)
    if (CustomizationComponent)
    {
        switch (StageType)
        {
            case EDishCustomizationStageType::Plating:
                // CRITICAL: Capture ingredient positions/rotations BEFORE any cleanup - otherwise PlatingEntries stays empty
                CustomizationComponent->CapturePlatingTransformsFromMeshes();
                // Scorecard RT: must run while plating meshes still exist. GoToStage clears bPlatingMode before EndCustomization,
                // so EndPlatingStage() never runs later — same path as EndPlatingStage (before ClearAll3DIngredientMeshes).
                CustomizationComponent->CaptureScorecardSnapshotFromPlatingStation();
                // Clear ingredient meshes BEFORE restoring dish mesh - otherwise physics/collision blows them apart
                CustomizationComponent->ClearAll3DIngredientMeshes();
                CustomizationComponent->SetPlatingMode(false);
                break;
            
            case EDishCustomizationStageType::Cooking:
                // No specific cleanup needed for cooking stage
                break;
            
            case EDishCustomizationStageType::Planning:
                // No specific cleanup needed for planning stage
                break;
            
            case EDishCustomizationStageType::Ending:
                // No specific cleanup needed for ending stage
                break;
        }
    }

    // Get dish data to pass - use component's data (has PlatingEntries after capture when leaving plating)
    const FPUDishBase& CurrentData = CustomizationComponent ? CustomizationComponent->GetCurrentDishData() : GetCurrentDishData();

    // Hide/remove current widget from viewport
    if (IsInViewport())
    {
        RemoveFromParent();
        //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Removed current widget from viewport"));
    }

    // Ensure target widget has the component reference
    if (CustomizationComponent)
    {
        if (TargetStage->GetCustomizationComponent() != CustomizationComponent)
        {
            TargetStage->SetCustomizationComponent(CustomizationComponent);
            //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Set component reference on target widget"));
        }
        
        // Always notify component about the active widget change, even if component reference already matches
        // This ensures CustomizationWidget is updated when navigating between stages
        CustomizationComponent->SetActiveCustomizationWidget(TargetStage);
        //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Updated component's active widget reference"));
    }

    // Handle setup when entering target stage
    if (CustomizationComponent)
    {
        switch (TargetStage->StageType)
        {
            case EDishCustomizationStageType::Plating:
            {
                CustomizationComponent->SetPlatingMode(true);
                CustomizationComponent->ResetPlatingPlacements();
                break;
            }
            
            case EDishCustomizationStageType::Cooking:
                CustomizationComponent->SetPlatingMode(false);
                break;
            
            case EDishCustomizationStageType::Planning:
                // Setup planning stage (if needed)
                //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Setting up planning stage"));
                // Planning stage might not need specific component setup
                break;
            
            case EDishCustomizationStageType::Ending:
                // Setup ending stage (if needed)
                break;
        }
    }

    // Add target widget to viewport if not already there
    if (!TargetStage->IsInViewport())
    {
        TargetStage->AddToViewport();
        //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::GoToStage - Added target widget to viewport"));
    }

    // Update target widget's dish data
    TargetStage->CurrentDishData = CurrentData;

    // Call OnDishDataReceived on target widget to initialize it
    TargetStage->OnDishDataReceived(CurrentData);
    //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::GoToStage - Navigation complete"));
}

void UPUDishCustomizationWidget::GoToNextStage()
{
    if (NextStage)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            UPUDishCustomizationWidget* NextStageWidget = CreateWidget<UPUDishCustomizationWidget>(World, NextStage);
            if (NextStageWidget)
            {
                // Ensure the new widget has the component reference
                if (CustomizationComponent)
                {
                    NextStageWidget->SetCustomizationComponent(CustomizationComponent);
                }
                GoToStage(NextStageWidget);
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToNextStage - Failed to create widget from class"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToNextStage - No world available"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::GoToNextStage - No next stage, ending customization"));
        
        // Remove current widget from viewport
        if (IsInViewport())
        {
            RemoveFromParent();
            //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::GoToNextStage - Removed widget from viewport"));
        }
        
        // End customization through the component
        if (CustomizationComponent)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::GoToNextStage - Calling EndCustomization on component"));
            CustomizationComponent->EndCustomization();
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::GoToNextStage - No customization component available to end customization"));
        }
    }
}

void UPUDishCustomizationWidget::GoToPreviousStage()
{
    if (PreviousStage)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            UPUDishCustomizationWidget* PreviousStageWidget = CreateWidget<UPUDishCustomizationWidget>(World, PreviousStage);
            if (PreviousStageWidget)
            {
                // Ensure the new widget has the component reference
                if (CustomizationComponent)
                {
                    PreviousStageWidget->SetCustomizationComponent(CustomizationComponent);
                }
                GoToStage(PreviousStageWidget);
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToPreviousStage - Failed to create widget from class"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToPreviousStage - No world available"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("🚫 PUDishCustomizationWidget::GoToPreviousStage - Previous stage class is not set"));
    }
}

// Removed NativeOnKeyDown - now handled through Enhanced Input Actions in PUDishCustomizationComponent

void UPUDishCustomizationWidget::SetPreviousStage(UPUDishCustomizationWidget* Stage)
{
    if (Stage)
    {
        PreviousStage = Stage->GetClass();
    }
    else
    {
        PreviousStage = nullptr;
    }
}

void UPUDishCustomizationWidget::SetNextStage(UPUDishCustomizationWidget* Stage)
{
    if (Stage)
    {
        NextStage = Stage->GetClass();
    }
    else
    {
        NextStage = nullptr;
    }
}

int32 UPUDishCustomizationWidget::GenerateGUIDBasedInstanceID()
{
    // Generate a GUID and convert it to a unique integer
    FGuid NewGUID = FGuid::NewGuid();
    
    // Convert GUID to a unique integer using hash
    int32 UniqueID = GetTypeHash(NewGUID);
    
    // Ensure it's positive (hash can be negative)
    UniqueID = FMath::Abs(UniqueID);
    
    //UE_LOG(LogTemp,Display, TEXT("🔍 PUDishCustomizationWidget::GenerateGUIDBasedInstanceID - Generated GUID-based InstanceID: %d from GUID: %s"), 
    //    UniqueID, *NewGUID.ToString());
    
    return UniqueID;
}

void UPUDishCustomizationWidget::CreateIngredientButtons()
{
    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Creating ingredient buttons"));
    
    // Clear existing buttons
    IngredientButtonMap.Empty();
    
    if (CustomizationComponent)
    {
        TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Found %d available ingredients"), AvailableIngredients.Num());
        
        for (const FPUIngredientBase& IngredientData : AvailableIngredients)
        {
            if (IngredientButtonClass)
            {
                UPUIngredientButton* IngredientButton = CreateWidget<UPUIngredientButton>(this, IngredientButtonClass);
                if (IngredientButton)
                {
                    IngredientButton->SetIngredientData(IngredientData);
                    
                    // Store button reference in map for O(1) lookup
                    IngredientButtonMap.Add(IngredientData.IngredientTag, IngredientButton);
                    
                    // Bind the button click event
                    IngredientButton->OnIngredientButtonClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnIngredientButtonClicked);
                    
                    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Created button for: %s"), 
                    //    *IngredientData.DisplayName.ToString());
                    
                    // Hide text elements for planning stage (prep stage should hide text)
                    IngredientButton->HideAllText();
                    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Hidden text elements for planning stage"));
                    
                    // Call Blueprint event
                    OnIngredientButtonCreated(IngredientButton, IngredientData);
                }
            }
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientButtons - No customization component available"));
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientButtons - Created %d ingredient buttons"), IngredientButtonMap.Num());
}

void UPUDishCustomizationWidget::CreateIngredientSlots()
{
    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🎯🎯🎯 PUDishCustomizationWidget::CreateIngredientSlots - FUNCTION CALLED! Creating ingredient slots from available ingredients"));
    
    // Clear existing slots and shelving widgets
    CreatedIngredientSlots.Empty();
    IngredientSlotMap.Empty();
    bIngredientSlotsCreated = false;
    
    // Clear shelving widgets
    CreatedShelvingWidgets.Empty();
    CurrentShelvingWidget.Reset();
    CurrentShelvingWidgetSlotCount = 0;
    
    // Check if we have a valid world context
    if (!GetWorld())
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreateIngredientSlots - No world context available"));
        return;
    }
    
    if (!CustomizationComponent)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - No customization component available"));
        return;
    }
    
    TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Found %d available ingredients"), AvailableIngredients.Num());
    
    // Get the container to use (use slot container first, fallback to button container if they're the same)
    UPanelWidget* ContainerToUse = nullptr;
    if (IngredientSlotContainer.IsValid())
    {
        ContainerToUse = IngredientSlotContainer.Get();
    }
    else if (IngredientButtonContainer.IsValid())
    {
        ContainerToUse = IngredientButtonContainer.Get();
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Using IngredientButtonContainer as fallback"));
    }
    
    if (!ContainerToUse)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateIngredientSlots - No ingredient container set (neither slot nor button container)! Slots cannot be added."));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   Slots will be added when SetIngredientSlotContainer() is called."));
    }
    
    // Convert available ingredients to ingredient instances (with quantity 0 for display)
    TArray<FIngredientInstance> PantryInstances;
    for (const FPUIngredientBase& IngredientData : AvailableIngredients)
    {
        FIngredientInstance PantryInstance;
        PantryInstance.IngredientData = IngredientData;
        PantryInstance.IngredientTag = IngredientData.IngredientTag;
        PantryInstance.Quantity = 0; // Empty slot, but has ingredient data for display
        PantryInstance.InstanceID = 0; // Not a real instance, just for display
        PantryInstances.Add(PantryInstance);
    }
    
    // Use unified CreateSlots function with shelving widgets enabled
    CreateSlots(ContainerToUse, EPUIngredientSlotLocation::ActiveIngredientArea, AvailableIngredients.Num(), true, false, false, PantryInstances, 0.0f);
    
    // Post-process: Set up special logic for prep stage slots (selection state, pantry click handler)
    for (int32 i = 0; i < CreatedIngredientSlots.Num() && i < AvailableIngredients.Num(); ++i)
    {
        UPUIngredientSlot* IngredientSlot = CreatedIngredientSlots[i];
        const FPUIngredientBase& IngredientData = AvailableIngredients[i];
        
        if (IngredientSlot)
        {
            // Store slot reference in map for O(1) lookup (similar to buttons)
            IngredientSlotMap.Add(IngredientData.IngredientTag, IngredientSlot);
            
            // Unbind the default empty slot click and bind to pantry slot click instead
            IngredientSlot->OnEmptySlotClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnEmptySlotClicked);
            IngredientSlot->OnEmptySlotClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);
            
            // Check if this ingredient is already selected (from existing dish ingredients)
            // This ensures ingredients that are already in the dish show as selected visually
            bool bIsSelected = IsIngredientSelected(IngredientData);
            IngredientSlot->SetSelected(bIsSelected);
            if (bIsSelected)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Marked ingredient as selected: %s"), 
                //    *IngredientData.DisplayName.ToString());
            }
        }
    }
    
    // Set up navigation for prep slots (for controller support)
    SetupPrepSlotNavigation();
    
    // Set initial focus for controller navigation
    SetInitialFocusForPrepStage();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlots - Created %d ingredient slots"), CreatedIngredientSlots.Num());
}

void UPUDishCustomizationWidget::CreatePlatingIngredientSlots()
{
    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - Creating plating ingredient slots"));

    if (!CustomizationComponent)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - No customization component available"));
        return;
    }

    // Get the current dish data
    const FPUDishBase& DishData = CustomizationComponent->GetCurrentDishData();
    
    if (DishData.IngredientInstances.Num() == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - No ingredient instances in dish data"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - Found %d ingredient instances"), 
    //    DishData.IngredientInstances.Num());

    // Debug: Log all ingredient instances
    for (int32 i = 0; i < DishData.IngredientInstances.Num(); i++)
    {
        const FIngredientInstance& Instance = DishData.IngredientInstances[i];
        //UE_LOG(LogTemp,Display, TEXT("🍽️ DEBUG: Instance %d - %s (ID: %d, Qty: %d, Preparations: %d)"), 
        //    i, *Instance.IngredientData.DisplayName.ToString(), Instance.InstanceID, Instance.Quantity, Instance.Preparations.Num());
        
        // Log preparation details
        TArray<FGameplayTag> PreparationTags;
        Instance.Preparations.GetGameplayTagArray(PreparationTags);
        for (const FGameplayTag& PrepTag : PreparationTags)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ DEBUG:   - Preparation: %s"), *PrepTag.ToString());
        }
    }

    // Check if we have a valid world context
    if (!GetWorld())
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreatePlatingIngredientSlots - No world context available"));
        return;
    }

    // Get the container to use (use slot container first, fallback to button container if they're the same)
    UPanelWidget* ContainerToUse = nullptr;
    if (IngredientSlotContainer.IsValid())
    {
        ContainerToUse = IngredientSlotContainer.Get();
    }
    else if (IngredientButtonContainer.IsValid())
    {
        ContainerToUse = IngredientButtonContainer.Get();
        //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - Using IngredientButtonContainer as fallback"));
    }
    
    if (!ContainerToUse)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - No ingredient container set! Slots cannot be added."));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   Slots will be added when SetIngredientSlotContainer() is called."));
    }

    // Use unified CreateSlots function
    // For plating, we want to create slots for each ingredient instance (no empty slots)
    // Use Plating location (not ActiveIngredientArea)
    CreateSlots(ContainerToUse, EPUIngredientSlotLocation::Plating, 12, false, false, true, DishData.IngredientInstances, 0.0f);
    
    // Tutorial: advance step 3 to 4 when plating stage is shown. User handles BAO "I'm thinking we put the gochujang..." in Blueprint.
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        if (GI->IsTutorialModeEnabled() && GI->GetTutorialStep() == 3)
        {
            GI->AdvanceTutorialStep();
        }
    }
    
    // Call the plating stage initialized event
    OnPlatingStageInitialized(DishData);
    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::CreatePlatingIngredientSlots - Called OnPlatingStageInitialized event"));
}

void UPUDishCustomizationWidget::EnablePlatingSlots()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingSlots - Enabling all plating slots"));

    // Update all slots to ensure they're displaying correctly and enable drag
    for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
    {
        if (IngredientSlot)
        {
            // Enable drag functionality for plating stage
            IngredientSlot->SetDragEnabled(true);
            
            // Update the slot display (this will refresh icons, quantity control, etc.)
            IngredientSlot->UpdateDisplay();
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingSlots - Enabled drag and updated slot display for: %s"), 
            //    IngredientSlot->IsEmpty() ? TEXT("Empty Slot") : *IngredientSlot->GetIngredientInstance().IngredientData.DisplayName.ToString());
        }
    }

    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::EnablePlatingSlots - Updated %d plating slots"), CreatedIngredientSlots.Num());
}


void UPUDishCustomizationWidget::SetIngredientSlotContainer(UPanelWidget* Container, EPUIngredientSlotLocation SlotLocation)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Setting ingredient slot container (Location: %d)"), (int32)SlotLocation);
    
    if (!Container)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::SetIngredientSlotContainer - Container is null"));
        return;
    }
    
    IngredientButtonContainer = Container;
    // Also set the slot container to the same container since they're the same
    IngredientSlotContainer = Container;
    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Container set successfully (also set IngredientSlotContainer)"));
    
    // Add any existing buttons to the new container
    // Note: Plating now uses slots instead of buttons, so we only add regular ingredient buttons
    int32 TotalButtons = IngredientButtonMap.Num();
    
    if (TotalButtons > 0)
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Adding %d existing buttons to container"), TotalButtons);
        
        // Add regular ingredient buttons (for planning stage)
        for (auto& ButtonPair : IngredientButtonMap)
        {
            if (ButtonPair.Value)
            {
                Container->AddChild(ButtonPair.Value);
            }
        }
    }
    
    // If we already have created slots, add them to the new container
    if (CreatedIngredientSlots.Num() > 0)
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Found %d existing slots to add to container"), CreatedIngredientSlots.Num());
        int32 SlotsAdded = 0;
        for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
        {
            if (IngredientSlot)
            {
                // Set location to the specified location for all slots
                IngredientSlot->SetLocation(SlotLocation);
                
                if (!IngredientSlot->GetParent())
                {
                    Container->AddChild(IngredientSlot);
                    SlotsAdded++;
                    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Added slot to container (Slot: %s, Location: %d)"), 
                    //    *IngredientSlot->GetName(), (int32)SlotLocation);
                }
                else
                {
                    //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Slot already has a parent, skipping (Slot: %s)"), *IngredientSlot->GetName());
                }
            }
        }
        //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - Successfully added %d slots to container"), SlotsAdded);
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ PUDishCustomizationWidget::SetIngredientSlotContainer - No existing slots to add (slots will be created when ingredients are added)"));
    }
}

UPUIngredientButton* UPUDishCustomizationWidget::FindIngredientButton(const FPUIngredientBase& IngredientData) const
{
    // O(1) lookup using map
    const UPUIngredientButton* const* FoundButton = IngredientButtonMap.Find(IngredientData.IngredientTag);
    if (FoundButton)
    {
        return const_cast<UPUIngredientButton*>(*FoundButton);
    }
    return nullptr;
}

UPUIngredientButton* UPUDishCustomizationWidget::GetIngredientButtonByTag(const FGameplayTag& IngredientTag) const
{
    // O(1) lookup using map
    const UPUIngredientButton* const* FoundButton = IngredientButtonMap.Find(IngredientTag);
    if (FoundButton)
    {
        return const_cast<UPUIngredientButton*>(*FoundButton);
    }
    return nullptr;
}

void UPUDishCustomizationWidget::OnIngredientButtonClicked(const FPUIngredientBase& IngredientData)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Ingredient button clicked: %s"), 
    //    *IngredientData.DisplayName.ToString());
    
    if (bInPlanningMode)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - In planning mode, handling toggle selection"));
        
        // In planning mode, use ToggleIngredientSelection which properly handles both SelectedIngredients and IngredientInstances
        ToggleIngredientSelection(IngredientData);
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnIngredientButtonClicked - Not in planning mode, using legacy behavior"));
        
        // Legacy behavior - create a new ingredient instance
        CreateIngredientInstance(IngredientData);
    }
}

bool UPUDishCustomizationWidget::CanAddMoreIngredients() const
{
    // Count unique ingredients (not instances, since one ingredient can have multiple instances)
    TSet<FGameplayTag> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        UniqueIngredients.Add(Instance.IngredientTag);
    }
    
    bool bCanAdd = UniqueIngredients.Num() < MaxIngredients;
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CanAddMoreIngredients - Current: %d, Max: %d, CanAdd: %s"), 
    //    UniqueIngredients.Num(), MaxIngredients, bCanAdd ? TEXT("Yes") : TEXT("No"));
    
    return bCanAdd;
}

void UPUDishCustomizationWidget::SetMaxIngredients(int32 NewMaxIngredients)
{
    // Clamp the value to reasonable bounds
    MaxIngredients = FMath::Clamp(NewMaxIngredients, 1, 20);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetMaxIngredients - Set max ingredients to %d"), MaxIngredients);
}

void UPUDishCustomizationWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
    //    IngredientInstance.InstanceID);
    
    // Log the preparations in the received ingredient instance
    TArray<FGameplayTag> CurrentPreparations;
    IngredientInstance.Preparations.GetGameplayTagArray(CurrentPreparations);
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged - Received instance %d with %d preparations:"), 
    //    IngredientInstance.InstanceID, CurrentPreparations.Num());
    for (const FGameplayTag& Prep : CurrentPreparations)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlChanged -   - %s"), *Prep.ToString());
    }
    
    // Update the ingredient instance in the dish data (or add if new)
    bool bFound = false;
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            bFound = true;
            break;
        }
    }
    
    if (bFound)
    {
        // Update existing instance
        UpdateIngredientInstance(IngredientInstance);
    }
    else
    {
        // Add new instance (e.g., when ingredient is dropped on empty slot)
        //UE_LOG(LogTemp,Display, TEXT("🔍 DEBUG: Instance not found in dish data, adding new instance (ID: %d, Qty: %d)"), 
        //    IngredientInstance.InstanceID, IngredientInstance.Quantity);
        CurrentDishData.IngredientInstances.Add(IngredientInstance);
        UpdateDishData(CurrentDishData);
    }
}

void UPUDishCustomizationWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Remove the ingredient instance from the dish data
    RemoveIngredientInstance(InstanceID);
    
    // Remove the widget from viewport
    if (QuantityControlWidget)
    {
        QuantityControlWidget->RemoveFromParent();
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnQuantityControlRemoved - Widget removed from viewport"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientInstance(const FPUIngredientBase& IngredientData)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Creating ingredient instance: %s"), 
    //    *IngredientData.DisplayName.ToString());
    
    // Create a new ingredient instance using the blueprint library
    FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(CurrentDishData, IngredientData.IngredientTag);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientInstance - Created instance with ID: %d"), NewInstance.InstanceID);
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
    
    // Call Blueprint event to create quantity control with ingredient instance data
    OnQuantityControlCreated(nullptr, NewInstance); // Pass the ingredient instance data
}

void UPUDishCustomizationWidget::UpdateIngredientInstance(const FIngredientInstance& IngredientInstance)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Updating ingredient instance: %d"), 
    //    IngredientInstance.InstanceID);
    
    // Log the preparations before updating
    TArray<FGameplayTag> PreparationsBefore;
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i].Preparations.GetGameplayTagArray(PreparationsBefore);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d had %d preparations before update:"), 
            //    IngredientInstance.InstanceID, PreparationsBefore.Num());
            for (const FGameplayTag& Prep : PreparationsBefore)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            break;
        }
    }
    
    // Find and update the ingredient instance in the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == IngredientInstance.InstanceID)
        {
            CurrentDishData.IngredientInstances[i] = IngredientInstance;
            
            // Log the preparations after updating
            TArray<FGameplayTag> PreparationsAfter;
            IngredientInstance.Preparations.GetGameplayTagArray(PreparationsAfter);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance %d now has %d preparations after update:"), 
            //    IngredientInstance.InstanceID, PreparationsAfter.Num());
            for (const FGameplayTag& Prep : PreparationsAfter)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance -   - %s"), *Prep.ToString());
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateIngredientInstance - Instance updated successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RemoveIngredientInstance(int32 InstanceID)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Removing ingredient instance: %d"), InstanceID);
    
    // Find and remove the ingredient instance from the dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
    {
        if (CurrentDishData.IngredientInstances[i].InstanceID == InstanceID)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstance - Instance removed successfully"));
            break;
        }
    }
    
    // Update the dish data
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RemoveIngredientInstanceByTag(const FGameplayTag& IngredientTag)
{
    if (bPU_LogDishDataReceiveDebug)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstanceByTag - Removing ingredient instance by tag: %s"), *IngredientTag.ToString());
    }

    for (int32 i = CurrentDishData.IngredientInstances.Num() - 1; i >= 0; i--)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        // Check both convenient field and data field (same logic as IsIngredientSelected)
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        if (InstanceTag == IngredientTag)
        {
            CurrentDishData.IngredientInstances.RemoveAt(i);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::RemoveIngredientInstanceByTag - Instance removed successfully"));
        }
    }
    UpdateDishData(CurrentDishData);
}

void UPUDishCustomizationWidget::RefreshQuantityControls()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::RefreshQuantityControls - Refreshing quantity controls"));
    
    // This will be called when dish data is updated to refresh all quantity controls
    // Blueprint can override this to handle the UI updates
}

void UPUDishCustomizationWidget::CompletePendingStripFillAndClosePantry(const FIngredientInstance& NewInstance)
{
    if (!PendingEmptySlot.IsValid())
    {
        return;
    }

    UPUIngredientSlot* EmptySlot = PendingEmptySlot.Get();
    TWeakObjectPtr<UPUIngredientSlot> PrepSlotToFocus = EmptySlot;

    if (!EmptySlot->IsEmpty())
    {
        const FIngredientInstance OldInstance = EmptySlot->GetIngredientInstance();
        RemovePreppedSlot(OldInstance);
        if (OldInstance.InstanceID != 0)
        {
            RemoveIngredientInstance(OldInstance.InstanceID);
        }
    }

    CurrentDishData.IngredientInstances.Add(NewInstance);

    EmptySlot->OnSlotIngredientChanged.AddUniqueDynamic(this, &UPUDishCustomizationWidget::OnQuantityControlChanged);

    EmptySlot->SetIngredientInstance(NewInstance);

    UpdateDishData(CurrentDishData);

    PendingEmptySlot.Reset();

    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        if (GI->IsTutorialModeEnabled())
        {
            const int32 CurrentStep = GI->GetTutorialStep();
            if (CurrentStep == 1)
            {
                GI->AdvanceTutorialStep();
                FPopupData PopupData;
                PopupData.PopupType = EPopupType::Tutorial;
                PopupData.Title = FText::FromString(TEXT("TUTORIAL"));
                PopupData.Message = FText::FromString(TEXT("Great! Now do the same for the gochujang. Select the ingredient from the Pantry to add it to a Prep Plate."));
                PopupData.bModal = true;
                PopupData.bShowCloseButton = true;
                GI->ShowPopup(PopupData);
            }
            else if (CurrentStep == 2)
            {
                GI->AdvanceTutorialStep();
            }
        }
    }

    ClosePantry();

    if (UWorld* World = GetWorld())
    {
        FTimerHandle FocusRestoreTimerHandle;
        World->GetTimerManager().SetTimer(FocusRestoreTimerHandle, [PrepSlotToFocus, this]()
        {
            if (PrepSlotToFocus.IsValid() && !IsDialogueVisible())
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::CompletePendingStripFillAndClosePantry - Restoring focus to prep slot: %s"),
                    *PrepSlotToFocus->GetName());

                PrepSlotToFocus->SetIsFocusable(true);
                PrepSlotToFocus->SetKeyboardFocus();
                FSlateApplication::Get().SetUserFocus(0, PrepSlotToFocus->TakeWidget());

                PrepSlotToFocus->ShowFocusVisuals();
                ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());

                if (!PrepSlotToFocus->HasKeyboardFocus())
                {
                    FTimerHandle RetryTimerHandle;
                    GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, [PrepSlotToFocus, this]()
                    {
                        if (PrepSlotToFocus.IsValid())
                        {
                            PrepSlotToFocus->SetKeyboardFocus();
                            PrepSlotToFocus->ShowFocusVisuals();
                            ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::CompletePendingStripFillAndClosePantry - Retry: Focus restored to %s (HasFocus: %s)"),
                                *PrepSlotToFocus->GetName(), PrepSlotToFocus->HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
                        }
                    }, 0.1f, false);
                }
            }
        }, 0.3f, false);
    }
}

void UPUDishCustomizationWidget::OnPantrySlotClicked(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot)
    {
        return;
    }

    if (IngredientSlot->IsRecipeLogSlot())
    {
        return;
    }

    auto PassesTutorialIngredientGate = [this](const FGameplayTag& IngredientTagToCheck) -> bool
    {
        UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
        if (!GI || !GI->IsTutorialModeEnabled())
        {
            return true;
        }
        const FGameplayTag AllowedTag = GI->GetTutorialAllowedIngredientTag();
        if (!AllowedTag.IsValid() || IngredientTagToCheck == AllowedTag)
        {
            return true;
        }
        FPopupData PopupData;
        PopupData.PopupType = EPopupType::Tutorial;
        PopupData.Title = FText::FromString(TEXT("TUTORIAL"));
        PopupData.Message = (GI->GetTutorialStep() == 1)
            ? FText::FromString(TEXT("Let's add the Egg Yolk Cookies to a Prep Plate. Find the cookies in your Pantry window and select the ingredient to add it to a Prep Plate."))
            : FText::FromString(TEXT("Great! Now do the same for the gochujang. Select the ingredient from the Pantry to add it to a Prep Plate."));
        PopupData.bModal = true;
        PopupData.bShowCloseButton = true;
        GI->ShowPopup(PopupData);
        return false;
    };

    if (IngredientSlot->IsPreppedPantryPickerSlot())
    {
        const FIngredientInstance& Pick = IngredientSlot->GetIngredientInstance();
        if (!Pick.IngredientData.IngredientTag.IsValid())
        {
            return;
        }
        if (!PassesTutorialIngredientGate(Pick.IngredientData.IngredientTag))
        {
            return;
        }

        if (PendingEmptySlot.IsValid())
        {
            FIngredientInstance NewInstance = Pick;
            NewInstance.InstanceID = GenerateGUIDBasedInstanceID();
            NewInstance.Quantity = 1;
            if (!NewInstance.IngredientTag.IsValid())
            {
                NewInstance.IngredientTag = NewInstance.IngredientData.IngredientTag;
            }
            NewInstance.IngredientData.ActivePreparations = NewInstance.Preparations;
            CompletePendingStripFillAndClosePantry(NewInstance);
        }
        else
        {
            ClosePantry();
        }
        return;
    }

    if (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::Pantry)
    {
        const FIngredientInstance& PantryInstance = IngredientSlot->GetIngredientInstance();

        if (!PantryInstance.IngredientData.IngredientTag.IsValid())
        {
            return;
        }

        if (!PassesTutorialIngredientGate(PantryInstance.IngredientData.IngredientTag))
        {
            return;
        }

        if (PendingEmptySlot.IsValid())
        {
            FIngredientInstance NewInstance;
            NewInstance.IngredientData = PantryInstance.IngredientData;
            NewInstance.InstanceID = GenerateGUIDBasedInstanceID();
            NewInstance.Quantity = 1;
            NewInstance.IngredientTag = PantryInstance.IngredientData.IngredientTag;
            CompletePendingStripFillAndClosePantry(NewInstance);
        }
        else
        {
            ClosePantry();
        }
        return;
    }

    for (auto& SlotPair : IngredientSlotMap)
    {
        if (SlotPair.Value == IngredientSlot)
        {
            if (CustomizationComponent)
            {
                TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
                const FPUIngredientBase* FoundIngredient = AvailableIngredients.FindByPredicate([&SlotPair](const FPUIngredientBase& Ingredient) {
                    return Ingredient.IngredientTag == SlotPair.Key;
                });

                if (FoundIngredient)
                {
                    if (IngredientSlot->IsPlanningGatherPlateSlot())
                    {
                        bool bCurrentlySelected = IngredientSlot->IsSelected();
                        bool bWantToSelect = !bCurrentlySelected;

                        bool bIngredientInDishData = IsIngredientSelected(*FoundIngredient);

                        if (bWantToSelect && !bIngredientInDishData)
                        {
                            int32 CurrentSelectedCount = CurrentDishData.IngredientInstances.Num();

                            if (CurrentSelectedCount >= MaxIngredients)
                            {
                                return;
                            }
                        }
                    }

                    OnIngredientButtonClicked(*FoundIngredient);

                    if (!bInPlanningMode && IngredientSlot->IsPlanningGatherPlateSlot())
                    {
                        bool bActuallySelected = IsIngredientSelected(*FoundIngredient);
                        IngredientSlot->SetSelected(bActuallySelected);
                    }

                    return;
                }
            }
        }
    }
}

void UPUDishCustomizationWidget::OnPlatingIngredientDropped(UPUIngredientSlot* DroppedSlot)
{
    // Tutorial: advance step 4 (first drop on plating) to step 5. Call Blueprint event for BAO "Look out belowww!" dialogue.
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        if (GI->IsTutorialModeEnabled() && GI->GetTutorialStep() == 4)
        {
            GI->AdvanceTutorialStep();
            OnTutorialPlatingDrop();
        }
    }
}

void UPUDishCustomizationWidget::SubscribeToEvents()
{
    //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - STARTING EVENT SUBSCRIPTION"));
    //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Widget name: %s"), *GetName());
    
    if (CustomizationComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - Customization component valid: %s"), *CustomizationComponent->GetName());
        
        // Subscribe to the component's events
        //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnInitialDishDataReceived"));
        CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        
        //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnDishDataUpdated"));
        CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        
        //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - Subscribing to OnCustomizationEnded"));
        CustomizationComponent->OnCustomizationEnded.AddDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
        if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
        {
            GI->OnPopupClosedEvent.AddDynamic(this, &UPUDishCustomizationWidget::OnPopupClosedForFocusRestore);
            GI->OnDialogueClosedEvent.AddDynamic(this, &UPUDishCustomizationWidget::OnDialogueClosedForFocusRestore);
        }
        
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::SubscribeToEvents - All events subscribed successfully"));
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::SubscribeToEvents - No customization component to subscribe to"));
    }
    
    //UE_LOG(LogTemp,Display, TEXT("📡 PUDishCustomizationWidget::SubscribeToEvents - EVENT SUBSCRIPTION COMPLETED"));
}

void UPUDishCustomizationWidget::UnsubscribeFromEvents()
{
    if (CustomizationComponent)
    {
        // //UE_LOG(LogTemp,Display, TEXT("PUDishCustomizationWidget::UnsubscribeFromEvents - Unsubscribing from customization component events"));
        
        // Unsubscribe from the component's events
        CustomizationComponent->OnInitialDishDataReceived.RemoveDynamic(this, &UPUDishCustomizationWidget::OnInitialDishDataReceived);
        CustomizationComponent->OnDishDataUpdated.RemoveDynamic(this, &UPUDishCustomizationWidget::OnDishDataUpdated);
        CustomizationComponent->OnCustomizationEnded.RemoveDynamic(this, &UPUDishCustomizationWidget::OnCustomizationEnded);
    }
    
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        GI->OnPopupClosedEvent.RemoveDynamic(this, &UPUDishCustomizationWidget::OnPopupClosedForFocusRestore);
        GI->OnDialogueClosedEvent.RemoveDynamic(this, &UPUDishCustomizationWidget::OnDialogueClosedForFocusRestore);
    }
}

void UPUDishCustomizationWidget::OnDialogueClosedForFocusRestore()
{
    // When dialogue closes and we're customizing, restore focus to dish customization (controller support).
    if (!CustomizationComponent || !CustomizationComponent->IsCustomizing())
    {
        return;
    }
    if (bPantryOpen)
    {
        SetInitialFocusForPantry();
    }
    else if (StageType == EDishCustomizationStageType::Cooking || StageType == EDishCustomizationStageType::Plating)
    {
        SetInitialFocusForCookingStage();
    }
    else
    {
        SetInitialFocusForPrepStage();
    }
}

void UPUDishCustomizationWidget::OnPopupClosedForFocusRestore(FName ButtonID)
{
    // When a popup closes (e.g. tutorial) and the pantry is open, restore focus to the pantry.
    // Skip if dialogue is visible - popup-close handler already restored focus to dialogue (dialogue has priority).
    if (bPantryOpen)
    {
        if (ACharacter* PC = UGameplayStatics::GetPlayerCharacter(this, 0))
        {
            if (AProjectUmeowmiCharacter* Char = Cast<AProjectUmeowmiCharacter>(PC))
            {
                if (UPUDialogueBox* DialogueBox = Char->GetDialogueBox())
                {
                    if (DialogueBox->GetVisibility() == ESlateVisibility::Visible)
                    {
                        return; // Dialogue has focus, don't steal it
                    }
                }
            }
        }
        SetInitialFocusForPantry();
    }
}

void UPUDishCustomizationWidget::EndCustomizationFromUI()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - UI button pressed to end customization"));
    
    if (CustomizationComponent)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::EndCustomizationFromUI - Calling EndCustomization on component"));
        CustomizationComponent->EndCustomization();
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::EndCustomizationFromUI - No customization component available"));
    }
} 

void UPUDishCustomizationWidget::ToggleIngredientSelection(const FPUIngredientBase& IngredientData)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Toggling ingredient: %s"), 
    //    *IngredientData.DisplayName.ToString());
    
    // Check if ingredient is already selected
    bool bWasSelected = IsIngredientSelected(IngredientData);
    
    if (bInPlanningMode)
    {
        // In planning mode, work with SelectedIngredients
        if (bWasSelected)
        {
            // Remove from selected ingredients
            PlanningData.SelectedIngredients.RemoveAll([&](const FPUIngredientBase& SelectedIngredient) {
                return SelectedIngredient.IngredientTag == IngredientData.IngredientTag;
            });
            
            // Also remove from IngredientInstances when unselecting in planning mode
            // This ensures the ingredient is fully unselected and won't appear in cooking stage
            RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Removed ingredient from planning"));
        }
        else
        {
            // Add to selected ingredients
            PlanningData.SelectedIngredients.Add(IngredientData);
            
            // In planning mode, we don't create IngredientInstances yet (quantities will be set in cooking stage)
            // But we mark it as selected in SelectedIngredients
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Added ingredient to planning"));
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Planning now has %d selected ingredients"), 
        //    PlanningData.SelectedIngredients.Num());
        
        // Update the slot's visual state to reflect the new selection state
        if (UPUIngredientSlot** FoundSlot = IngredientSlotMap.Find(IngredientData.IngredientTag))
        {
            if (*FoundSlot)
            {
                bool bIsNowSelected = IsIngredientSelected(IngredientData);
                (*FoundSlot)->SetSelected(bIsNowSelected);
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Updated slot visual state to: %s"), 
                //    bIsNowSelected ? TEXT("SELECTED") : TEXT("UNSELECTED"));
            }
        }
        
        // Update radar chart in planning mode by creating a temporary dish from SelectedIngredients
        UpdateRadarChartFromPlanningData();
    }
    else
    {
        // In cooking/prep mode, work with IngredientInstances
        if (bWasSelected)
        {
            // Remove from selected ingredients
            RemoveIngredientInstanceByTag(IngredientData.IngredientTag);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Removed ingredient from dish"));
        }
        else
        {
            // Add to selected ingredients
            CreateIngredientInstance(IngredientData);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Added ingredient to dish"));
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ToggleIngredientSelection - Dish now has %d ingredient instances"), 
        //    CurrentDishData.IngredientInstances.Num());
    }
    
    // Call Blueprint event
    OnIngredientSelectionChanged(IngredientData, !bWasSelected);
}

void UPUDishCustomizationWidget::RefreshRadarChartsFromDishData(const FPUDishBase& Dish)
{
    SanitizeRadarChartsInWidgetTree(WidgetTree);

    // Update assigned radar charts (assign in Blueprint Details under "Radar Chart" category)
    if (FlavorRadarChart)
    {
        FlavorRadarChart->SetValuesFromDishFlavorProfile(Dish);
    }
    if (TextureRadarChart)
    {
        TextureRadarChart->SetValuesFromDishTextureProfile(Dish);
    }
    // Fallback: if no charts assigned, search widget tree for PURadarChart and update with texture (Crumbly)
    if (!FlavorRadarChart && !TextureRadarChart && WidgetTree)
    {
        TArray<UWidget*> AllWidgets;
        WidgetTree->GetAllWidgets(AllWidgets);
        for (UWidget* W : AllWidgets)
        {
            if (UPURadarChart* Chart = Cast<UPURadarChart>(W))
            {
                Chart->SetValuesFromDishTextureProfile(Dish);
            }
        }
    }
}

void UPUDishCustomizationWidget::UpdateRadarChartFromPlanningData()
{
    if (!bInPlanningMode)
    {
        return;
    }
    
    // Create a temporary dish from SelectedIngredients for the radar chart
    FPUDishBase TempDish = CurrentDishData;
    TempDish.IngredientInstances.Empty();
    
    // Convert SelectedIngredients to IngredientInstances (with quantity 1 for each)
    for (const FPUIngredientBase& SelectedIngredient : PlanningData.SelectedIngredients)
    {
        // Create a temporary instance with quantity 1
        FIngredientInstance TempInstance;
        TempInstance.InstanceID = FPUDishBase::GenerateUniqueInstanceID();
        TempInstance.Quantity = 1;
        TempInstance.IngredientData = SelectedIngredient;
        TempInstance.IngredientTag = SelectedIngredient.IngredientTag;
        TempInstance.Preparations = SelectedIngredient.ActivePreparations;
        
        TempDish.IngredientInstances.Add(TempInstance);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateRadarChartFromPlanningData - Created temp dish with %d ingredients for radar chart"), 
    //    TempDish.IngredientInstances.Num());
    
    // Update the radar chart by calling the existing Blueprint event that already handles it
    OnDishDataChanged(TempDish);
}

bool UPUDishCustomizationWidget::IsIngredientSelected(const FPUIngredientBase& IngredientData) const
{
    // In planning mode, check both SelectedIngredients and IngredientInstances
    // (we keep IngredientInstances visible so players can see what's already in the dish)
    if (bInPlanningMode)
    {
        // First check SelectedIngredients
        bool bInSelectedIngredients = PlanningData.SelectedIngredients.ContainsByPredicate([&](const FPUIngredientBase& SelectedIngredient) {
            return SelectedIngredient.IngredientTag == IngredientData.IngredientTag;
        });
        
        // Also check IngredientInstances (in case something is there but not in SelectedIngredients yet)
        bool bInIngredientInstances = CurrentDishData.IngredientInstances.ContainsByPredicate([&](const FIngredientInstance& Instance) {
            FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
            return InstanceTag == IngredientData.IngredientTag;
        });
        
        return bInSelectedIngredients || bInIngredientInstances;
    }
    
    // In cooking/prep mode, check IngredientInstances
    return CurrentDishData.IngredientInstances.ContainsByPredicate([&](const FIngredientInstance& Instance) {
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        return InstanceTag == IngredientData.IngredientTag;
    });
}

void UPUDishCustomizationWidget::StartPlanningMode()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Initialize planning data with current dish
    PlanningData.TargetDish = CurrentDishData;
    PlanningData.SelectedIngredients.Empty();
    PlanningData.bPlanningCompleted = false;
    
    // Populate SelectedIngredients from existing dish ingredients
    // Extract unique ingredients from IngredientInstances (planning mode uses ingredients without quantities)
    // IMPORTANT: We do NOT clear IngredientInstances - they should remain visible to the player
    // The ingredients that are already in the dish should be shown as selected
    TMap<FGameplayTag, FPUIngredientBase> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        // Only add if we haven't seen this ingredient tag before
        if (InstanceTag.IsValid() && !UniqueIngredients.Contains(InstanceTag))
        {
            // Use the ingredient data from the instance (which already has preparations applied)
            UniqueIngredients.Add(InstanceTag, Instance.IngredientData);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Added existing ingredient to SelectedIngredients: %s"), 
            //    *InstanceTag.ToString());
        }
    }
    
    // Add all unique ingredients to SelectedIngredients
    UniqueIngredients.GenerateValueArray(PlanningData.SelectedIngredients);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Populated %d existing ingredients into SelectedIngredients"), 
    //    PlanningData.SelectedIngredients.Num());
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Keeping %d ingredient instances visible (not clearing)"), 
    //    CurrentDishData.IngredientInstances.Num());
    
    // DO NOT clear IngredientInstances - they should remain visible and selected
    // The player should see what ingredients are already in the dish
    
    // Update all existing ingredient slots to show their correct selection state
    // This ensures ingredients that are already in the dish show as selected visually
    for (auto& SlotPair : IngredientSlotMap)
    {
        if (SlotPair.Value)
        {
            // Find the ingredient data for this slot
            if (CustomizationComponent)
            {
                TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
                const FPUIngredientBase* FoundIngredient = AvailableIngredients.FindByPredicate([&SlotPair](const FPUIngredientBase& Ingredient) {
                    return Ingredient.IngredientTag == SlotPair.Key;
                });
                
                if (FoundIngredient)
                {
                    bool bIsSelected = IsIngredientSelected(*FoundIngredient);
                    SlotPair.Value->SetSelected(bIsSelected);
                    if (bIsSelected)
                    {
                        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Updated slot selection state for: %s (SELECTED)"), 
                        //    *FoundIngredient->DisplayName.ToString());
                    }
                }
            }
        }
    }
    
    // Update radar chart with initial planning data
    UpdateRadarChartFromPlanningData();
    
    // Call Blueprint event
    OnPlanningModeStarted();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::StartPlanningMode - Planning mode started for dish: %s"), 
    //    *CurrentDishData.DisplayName.ToString());
}

void UPUDishCustomizationWidget::FinishPlanningAndStartCooking()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Finishing planning and starting cooking"));
    
    if (!bInPlanningMode)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::FinishPlanningAndStartCooking - Not in planning mode"));
        return;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Planning has %d selected ingredients in SelectedIngredients"), 
    //    PlanningData.SelectedIngredients.Num());
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Current dish has %d ingredient instances"), 
    //    CurrentDishData.IngredientInstances.Num());
    
    // Convert SelectedIngredients to IngredientInstances for cooking stage
    // First, clear existing instances (we'll rebuild from SelectedIngredients)
    // But preserve any that are already there with quantities (like pre-loaded ingredients)
    FPUDishBase CookingDishData = CurrentDishData;
    
    // Create a map of existing instances by tag to preserve quantities
    TMap<FGameplayTag, FIngredientInstance> ExistingInstances;
    for (const FIngredientInstance& Instance : CookingDishData.IngredientInstances)
    {
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (InstanceTag.IsValid() && Instance.Quantity > 0)
        {
            ExistingInstances.Add(InstanceTag, Instance);
        }
    }
    
    // Clear and rebuild IngredientInstances from SelectedIngredients
    CookingDishData.IngredientInstances.Empty();
    
    for (const FPUIngredientBase& SelectedIngredient : PlanningData.SelectedIngredients)
    {
        // Check if we already have an instance for this ingredient (preserve quantity)
        if (FIngredientInstance* ExistingInstance = ExistingInstances.Find(SelectedIngredient.IngredientTag))
        {
            // Use existing instance (preserves quantity and preparations)
            CookingDishData.IngredientInstances.Add(*ExistingInstance);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Preserved existing instance for: %s (Qty: %d)"), 
            //    *SelectedIngredient.DisplayName.ToString(), ExistingInstance->Quantity);
        }
        else
        {
            // Create new instance with default quantity of 1
            FIngredientInstance NewInstance = UPUDishBlueprintLibrary::AddIngredient(CookingDishData, SelectedIngredient.IngredientTag);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Created new instance for: %s (Qty: %d)"), 
            //    *SelectedIngredient.DisplayName.ToString(), NewInstance.Quantity);
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FinishPlanningAndStartCooking - Cooking dish now has %d ingredient instances"), 
    //    CookingDishData.IngredientInstances.Num());
    
    // Update current dish data
    CurrentDishData = CookingDishData;
    UpdateDishData(CookingDishData);
    
    // Mark planning as completed
    PlanningData.bPlanningCompleted = true;
    
    // Transition to cooking stage through the component
    if (CustomizationComponent)
    {
        CustomizationComponent->TransitionToCookingStage(CookingDishData);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::FinishPlanningAndStartCooking - No customization component available"));
    }
}

void UPUDishCustomizationWidget::CreateIngredientSlotsInContainer(UPanelWidget* Container, int32 MaxSlots, EPUIngredientSlotLocation SlotLocation)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateIngredientSlotsInContainer - DEPRECATED: Use CreateSlots() instead"));
    
    // Use unified CreateSlotsFromDishData function (uses CurrentDishData.IngredientInstances)
    CreateSlotsFromDishData(Container, SlotLocation, MaxSlots, false, true, true, 0.0f);
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget(UPanelWidget* ContainerToUse)
{
    // Check if we need a new shelving widget
    // Need a new one if: no current widget, or current widget has 3 slots
    if (!CurrentShelvingWidget.IsValid() || CurrentShelvingWidgetSlotCount >= 3)
    {
        // Create a new shelving widget
        if (!ShelvingWidgetClass)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - ShelvingWidgetClass not set!"));
            return nullptr;
        }
        
        if (!GetWorld())
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - No world context available"));
            return nullptr;
        }
        
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - Failed to create shelving widget"));
            return nullptr;
        }
        
        // Add the shelving widget to the container
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::GetOrCreateCurrentShelvingWidget - Created and added new shelving widget (Total: %d)"), 
            //    CreatedShelvingWidgets.Num() + 1);
        }
        
        // Track the new shelving widget
        CreatedShelvingWidgets.Add(NewShelvingWidget);
        CurrentShelvingWidget = NewShelvingWidget;
        CurrentShelvingWidgetSlotCount = 0;
        
        return NewShelvingWidget;
    }
    
    // Return the current shelving widget
    return CurrentShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - IngredientSlot is null"));
        return false;
    }
    
    if (!CurrentShelvingWidget.IsValid())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - CurrentShelvingWidget is not valid"));
        return false;
    }
    
    // Find the HorizontalBox inside the shelving widget
    // First, try to get it by name
    UWidget* FoundWidget = CurrentShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        // If not found by name, try common names
        FoundWidget = CurrentShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }
    
    // Try to cast to HorizontalBox
    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentShelvingWidgetSlotCount++;
        // //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Added slot to HorizontalBox (Slot count: %d/3)"), 
        //     CurrentShelvingWidgetSlotCount);
        return true;
    }
    
    // Try casting to any panel widget
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentShelvingWidgetSlotCount++;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Added slot to panel widget (Slot count: %d/3)"), 
        //    CurrentShelvingWidgetSlotCount);
        return true;
    }
    
    // If still not found, log error with helpful message
    //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::AddSlotToCurrentShelvingWidget - Could not find HorizontalBox or panel widget in WBP_Shelving!"));
    //UE_LOG(LogTemp,Error, TEXT("   Searched for widget named: '%s', 'HorizontalBox', 'SlotContainer'"), *ShelvingHorizontalBoxName.ToString());
    //UE_LOG(LogTemp,Error, TEXT("   Please ensure WBP_Shelving contains a HorizontalBox (or other panel widget) with one of these names."));
    return false;
}

void UPUDishCustomizationWidget::CreateSlots(UPanelWidget* Container, EPUIngredientSlotLocation Location, int32 MaxSlots, bool bUseShelvingWidgets, bool bCreateEmptySlots, bool bEnableDrag, const TArray<FIngredientInstance>& IngredientSource, float FirstSlotLeftPadding)
{
    UE_LOG(LogTemp, Warning, TEXT("🎯🎯🎯 PUDishCustomizationWidget::CreateSlots - FUNCTION CALLED! Location: %d, MaxSlots: %d"),
        (int32)Location, MaxSlots);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Creating slots (Location: %d, MaxSlots: %d, UseShelving: %s, CreateEmpty: %s, EnableDrag: %s)"), 
    //    (int32)Location, MaxSlots, bUseShelvingWidgets ? TEXT("YES") : TEXT("NO"), bCreateEmptySlots ? TEXT("YES") : TEXT("NO"), bEnableDrag ? TEXT("YES") : TEXT("NO"));
    
    if (!Container && !bUseShelvingWidgets)
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreateSlots - Container is NULL and bUseShelvingWidgets is false!"));
        return;
    }
    
    if (!GetWorld())
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreateSlots - No world context available"));
        return;
    }

    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }
    
    // Clamp MaxSlots to a reasonable range (1-12)
    MaxSlots = FMath::Clamp(MaxSlots, 1, 12);
    
    // Determine ingredient source
    const TArray<FIngredientInstance>* IngredientInstancesToUse = nullptr;
    if (IngredientSource.Num() > 0)
    {
        IngredientInstancesToUse = &IngredientSource;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Using provided ingredient source (%d instances)"), IngredientSource.Num());
    }
    else
    {
        IngredientInstancesToUse = &CurrentDishData.IngredientInstances;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Using CurrentDishData.IngredientInstances (%d instances)"), CurrentDishData.IngredientInstances.Num());
    }
    
    // Calculate how many slots to create
    int32 NumSlotsToCreate = 0;
    if (bCreateEmptySlots)
    {
        // Create slots for existing ingredients, or create empty slots up to MaxSlots
        NumSlotsToCreate = FMath::Max(IngredientInstancesToUse->Num(), MaxSlots);
        NumSlotsToCreate = FMath::Min(MaxSlots, NumSlotsToCreate); // Cap at MaxSlots
    }
    else
    {
        // Only create slots for existing ingredients (up to MaxSlots)
        NumSlotsToCreate = FMath::Min(IngredientInstancesToUse->Num(), MaxSlots);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Creating %d slots (source has %d ingredients, max is %d)"), 
    //    NumSlotsToCreate, IngredientInstancesToUse->Num(), MaxSlots);
    
    // Clear existing slots if this is the first time creating them
    if (!bIngredientSlotsCreated)
    {
        CreatedIngredientSlots.Empty();
        if (bUseShelvingWidgets)
        {
            CreatedShelvingWidgets.Empty();
            CurrentShelvingWidget.Reset();
            CurrentShelvingWidgetSlotCount = 0;
        }
    }
    
    // Get slot class
    TSubclassOf<UPUIngredientSlot> SlotClass;
    if (IngredientSlotClass)
    {
        SlotClass = IngredientSlotClass;
    }
    else
    {
        SlotClass = UPUIngredientSlot::StaticClass();
    }
    
    // Create slots
    for (int32 i = 0; i < NumSlotsToCreate; ++i)
    {
        UPUIngredientSlot* IngredientSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (!IngredientSlot)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreateSlots - Failed to create ingredient slot at index %d"), i);
            continue;
        }
        
        // Common setup for all slots
        IngredientSlot->SetDishCustomizationWidget(this);
        IngredientSlot->SetLocation(Location);
        IngredientSlot->SetDragEnabled(bEnableDrag);
        
        // Set the preparation data table if available
        if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
        {
            IngredientSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
        }
        
        // Bind common events
        IngredientSlot->OnSlotIngredientChanged.AddDynamic(this, &UPUDishCustomizationWidget::OnQuantityControlChanged);
        IngredientSlot->OnEmptySlotClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnEmptySlotClicked);
        if (Location == EPUIngredientSlotLocation::Plating)
        {
            IngredientSlot->OnIngredientDroppedOnSlot.AddDynamic(this, &UPUDishCustomizationWidget::OnPlatingIngredientDropped);
        }
        
        // Set ingredient instance if we have one
        if (i < IngredientInstancesToUse->Num())
        {
            const FIngredientInstance& IngredientInstance = (*IngredientInstancesToUse)[i];
            
            // Validate ingredient instance
            if (IngredientInstance.IngredientData.IngredientTag.IsValid())
            {
                IngredientSlot->SetIngredientInstance(IngredientInstance);
                IngredientSlot->UpdateDisplay();
                
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Created slot %d with ingredient: %s (ID: %d, Qty: %d)"), 
                //    i, *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID, IngredientInstance.Quantity);
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - Invalid ingredient instance at index %d, creating empty slot"), i);
                IngredientSlot->UpdateDisplay();
            }
        }
        else
        {
            // Create empty slot
            IngredientSlot->UpdateDisplay();
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Created empty slot %d"), i);
        }
        
        // Add to our array
        CreatedIngredientSlots.Add(IngredientSlot);
        
        // Call Blueprint event for slot creation
        if (i < IngredientInstancesToUse->Num())
        {
            OnIngredientSlotCreated(IngredientSlot, (*IngredientInstancesToUse)[i]);
        }
        else
        {
            FIngredientInstance EmptyInstance;
            OnIngredientSlotCreated(IngredientSlot, EmptyInstance);
        }
        
        // Add to container or shelving widget
        if (bUseShelvingWidgets)
        {
            if (Container)
            {
                // Get or create a current shelving widget
                UUserWidget* ShelvingWidget = GetOrCreateCurrentShelvingWidget(Container);
                if (ShelvingWidget)
                {
                    // Add slot to the shelving widget
                    if (AddSlotToCurrentShelvingWidget(IngredientSlot))
                    {
                        // Successfully added to shelving widget
                    }
                    else
                    {
                        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - Failed to add slot to shelving widget"));
                    }
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - Failed to get or create shelving widget"));
                }
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - bUseShelvingWidgets is true but Container is NULL!"));
            }
        }
        else
        {
            // Add directly to the container
            if (Container)
            {
                Container->AddChild(IngredientSlot);
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Added slot %d directly to container"), i);
                
                // Apply left padding to first planning gather slot on the plate grid when requested
                if (i == 0 && Location == EPUIngredientSlotLocation::ActiveIngredientArea &&
                    StageType == EDishCustomizationStageType::Planning && FirstSlotLeftPadding > 0.0f)
                {
                    // Try to get the slot and apply padding based on container type
                    if (UWrapBox* WrapBox = Cast<UWrapBox>(Container))
                    {
                        if (UWrapBoxSlot* WrapBoxSlot = Cast<UWrapBoxSlot>(IngredientSlot->Slot))
                        {
                            FMargin CurrentPadding = WrapBoxSlot->GetPadding();
                            WrapBoxSlot->SetPadding(FMargin(FirstSlotLeftPadding, CurrentPadding.Top, CurrentPadding.Right, CurrentPadding.Bottom));
                            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Applied left padding %.2f to first prep slot"), FirstSlotLeftPadding);
                        }
                    }
                    else if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(Container))
                    {
                        if (UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>(IngredientSlot->Slot))
                        {
                            FMargin CurrentPadding = HorizontalBoxSlot->GetPadding();
                            HorizontalBoxSlot->SetPadding(FMargin(FirstSlotLeftPadding, CurrentPadding.Top, CurrentPadding.Right, CurrentPadding.Bottom));
                            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Applied left padding %.2f to first prep slot"), FirstSlotLeftPadding);
                        }
                    }
                    else if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(Container))
                    {
                        // For CanvasPanel, we'd need to adjust position instead of padding
                        // This would require getting the slot position and adding the offset
                        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - CanvasPanel padding not yet implemented for prep area"));
                    }
                    // Add other container types as needed (VerticalBox, etc.)
                }
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateSlots - Container is NULL, slot not added to UI"));
            }
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Successfully created %d ingredient slots"), CreatedIngredientSlots.Num());
    
    // Mark that slots have been created
    bIngredientSlotsCreated = true;
    
    // Planning gather plate vs cooking strip both use ActiveIngredientArea — disambiguate with StageType
    if (Location == EPUIngredientSlotLocation::ActiveIngredientArea && StageType == EDishCustomizationStageType::Planning)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯🎯🎯 PUDishCustomizationWidget::CreateSlots - Planning gather slots created! Setting up navigation..."));
        SetupPrepSlotNavigation();
        SetInitialFocusForPrepStage();
    }
    else if (Location == EPUIngredientSlotLocation::Prepped ||
             (Location == EPUIngredientSlotLocation::ActiveIngredientArea && StageType == EDishCustomizationStageType::Cooking))
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 PUDishCustomizationWidget::CreateSlots - Cooking stage slots created! Setting up navigation and focus..."));
        SetupCookingSlotNavigation();
        SetInitialFocusForCookingStage();
    }
}

void UPUDishCustomizationWidget::CreateSlotsFromDishData(UPanelWidget* Container, EPUIngredientSlotLocation Location, int32 MaxSlots, bool bUseShelvingWidgets, bool bCreateEmptySlots, bool bEnableDrag, float FirstSlotLeftPadding)
{
    // CreateSlots with empty IngredientSource selects CurrentDishData.IngredientInstances.
    TArray<FIngredientInstance> EmptyArray;
    CreateSlots(Container, Location, MaxSlots, bUseShelvingWidgets, bCreateEmptySlots, bEnableDrag, EmptyArray, FirstSlotLeftPadding);
}

TArray<FIngredientInstance> UPUDishCustomizationWidget::GetIngredientInstancesFromDataTable(UDataTable* IngredientDataTable)
{
    TArray<FIngredientInstance> IngredientInstances;
    
    if (!IngredientDataTable)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationWidget::GetIngredientInstancesFromDataTable - IngredientDataTable is NULL"));
        return IngredientInstances;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationWidget::GetIngredientInstancesFromDataTable - Getting ingredient instances from table: %s"), 
    //    *IngredientDataTable->GetName());
    
    // Get all row names from the data table
    TArray<FName> RowNames = IngredientDataTable->GetRowNames();
    
    // Convert each ingredient base to an ingredient instance
    for (const FName& RowName : RowNames)
    {
        if (FPUIngredientBase* Ingredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredientInstancesFromDataTable")))
        {
            // Create an ingredient instance with quantity 0 and instance ID 0
            // This is suitable for pantry/prep slots that display ingredients but aren't "active" instances
            FIngredientInstance Instance;
            Instance.IngredientData = *Ingredient;
            Instance.IngredientTag = Ingredient->IngredientTag;
            Instance.Quantity = 0; // Empty slot, but has ingredient data for display
            Instance.InstanceID = 0; // Not a real instance, just for display
            Instance.Preparations = FGameplayTagContainer(); // No preparations initially
            
            IngredientInstances.Add(Instance);
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationWidget::GetIngredientInstancesFromDataTable - Converted %d ingredients to instances"), 
    //    IngredientInstances.Num());
    
    return IngredientInstances;
}

// ============================================================================
// Quantity Control Management Functions
// ============================================================================

void UPUDishCustomizationWidget::EnableQuantityControlDrag(bool bEnabled)
{
    TArray<UPUIngredientQuantityControl*> Found;
    FindQuantityControlsInHierarchy(Found);
    for (UPUIngredientQuantityControl* QuantityControl : Found)
    {
        if (QuantityControl)
        {
            QuantityControl->SetDragEnabled(bEnabled);
        }
    }
}

bool UPUDishCustomizationWidget::UpdateExistingQuantityControl(int32 InstanceID, const FGameplayTagContainer& NewPreparations)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Looking for quantity control with InstanceID: %d"), InstanceID);

    UPanelWidget* ContainerToUse = QuantityScrollBox;

    if (!ContainerToUse)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::UpdateExistingQuantityControl - No QuantityScrollBox; searching widget hierarchy for quantity controls"));
        
        // Try to find the container automatically by searching for quantity controls in the widget hierarchy
        TArray<UPUIngredientQuantityControl*> FoundQuantityControls;
        FindQuantityControlsInHierarchy(FoundQuantityControls);
        
        if (FoundQuantityControls.Num() > 0)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Found %d quantity controls in hierarchy"), FoundQuantityControls.Num());
            
            // Check if any of the found quantity controls match our InstanceID
            for (UPUIngredientQuantityControl* QuantityControl : FoundQuantityControls)
            {
                if (QuantityControl && QuantityControl->GetInstanceID() == InstanceID)
                {
                    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Found existing quantity control for InstanceID: %d"), InstanceID);
                    
                    // Apply new preparations to the existing quantity control
                    TArray<FGameplayTag> PreparationTags;
                    NewPreparations.GetGameplayTagArray(PreparationTags);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Applying %d new preparations"), PreparationTags.Num());
                    
                    for (const FGameplayTag& PreparationTag : PreparationTags)
                    {
                        if (!QuantityControl->GetIngredientInstance().Preparations.HasTag(PreparationTag))
                        {
                            QuantityControl->AddPreparation(PreparationTag);
                            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Added preparation: %s"), *PreparationTag.ToString());
                        }
                        else
                        {
                            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Preparation already exists: %s"), *PreparationTag.ToString());
                        }
                    }
                    
                    //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::UpdateExistingQuantityControl - Successfully updated existing quantity control"));
                    return true;
                }
            }
        }
        
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::UpdateExistingQuantityControl - No quantity controls found in hierarchy"));
        return false;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Container is valid: %s"), *ContainerToUse->GetName());
    
    // Get all child widgets in the container
    TArray<UWidget*> ChildWidgets = ContainerToUse->GetAllChildren();
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Found %d child widgets in container"), ChildWidgets.Num());
    
    for (int32 i = 0; i < ChildWidgets.Num(); i++)
    {
        UWidget* ChildWidget = ChildWidgets[i];
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Checking child widget %d: %s"), i, ChildWidget ? *ChildWidget->GetName() : TEXT("NULL"));
        
        if (UPUIngredientQuantityControl* QuantityControl = Cast<UPUIngredientQuantityControl>(ChildWidget))
        {
            int32 ControlInstanceID = QuantityControl->GetInstanceID();
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Found quantity control with InstanceID: %d (looking for: %d)"), ControlInstanceID, InstanceID);
            
            // Check if this is the quantity control we're looking for
            if (ControlInstanceID == InstanceID)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Found existing quantity control for InstanceID: %d"), InstanceID);
                
                // Apply new preparations to the existing quantity control
                TArray<FGameplayTag> PreparationTags;
                NewPreparations.GetGameplayTagArray(PreparationTags);
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Applying %d new preparations"), PreparationTags.Num());
                
                for (const FGameplayTag& PreparationTag : PreparationTags)
                {
                    if (!QuantityControl->GetIngredientInstance().Preparations.HasTag(PreparationTag))
                    {
                        QuantityControl->AddPreparation(PreparationTag);
                        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Added preparation: %s"), *PreparationTag.ToString());
                    }
                    else
                    {
                        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Preparation already exists: %s"), *PreparationTag.ToString());
                    }
                }
                
                //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::UpdateExistingQuantityControl - Successfully updated existing quantity control"));
                return true;
            }
        }
        else
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::UpdateExistingQuantityControl - Child widget %d is not a quantity control"), i);
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("⚠️ PUDishCustomizationWidget::UpdateExistingQuantityControl - No existing quantity control found for InstanceID: %d"), InstanceID);
    return false;
}

void UPUDishCustomizationWidget::FindQuantityControlsInHierarchy(TArray<UPUIngredientQuantityControl*>& OutQuantityControls)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FindQuantityControlsInHierarchy - Searching for quantity controls in widget hierarchy"));
    
    // Clear the output array
    OutQuantityControls.Empty();
    
    // Recursively search through all child widgets
    FindQuantityControlsRecursive(this, OutQuantityControls);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FindQuantityControlsInHierarchy - Found %d quantity controls"), OutQuantityControls.Num());
}

void UPUDishCustomizationWidget::FindQuantityControlsRecursive(UWidget* ParentWidget, TArray<UPUIngredientQuantityControl*>& OutQuantityControls)
{
    if (!ParentWidget)
    {
        return;
    }
    
    // Check if this widget is a quantity control
    if (UPUIngredientQuantityControl* QuantityControl = Cast<UPUIngredientQuantityControl>(ParentWidget))
    {
        OutQuantityControls.Add(QuantityControl);
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::FindQuantityControlsRecursive - Found quantity control: %s (InstanceID: %d)"), 
        //    *QuantityControl->GetName(), QuantityControl->GetInstanceID());
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

bool UPUDishCustomizationWidget::TryApplyQuantityInputFromEnhancedInput(int32 Delta)
{
    if (Delta == 0)
    {
        return false;
    }
    if (bInPlanningMode)
    {
        return false;
    }
    if (IsDialogueVisible())
    {
        return false;
    }
    if (UPUDishCustomizationComponent* Comp = GetCustomizationComponent())
    {
        if (Comp->IsPlatingMode())
        {
            return false;
        }
    }
    if (!FSlateApplication::IsInitialized())
    {
        return false;
    }

    UPUIngredientQuantityControl* QC = nullptr;

    const TSharedPtr<SWidget> Focused = GetBestFocusedSlateForOwner(this);
    if (Focused.IsValid())
    {
        if (UWidget* LeafWidget = GetWidgetObjectFromSlate(Focused))
        {
            if (IsWidgetDescendantOf(LeafWidget, this))
            {
                QC = FindQuantityControlFromLeaf(LeafWidget);
            }
        }
    }

    if (QC && QC->IsVisible())
    {
        if (Delta > 0)
        {
            QC->IncreaseQuantity();
        }
        else
        {
            QC->DecreaseQuantity();
        }
        return true;
    }

    return false;
}

FGameplayTagContainer UPUDishCustomizationWidget::GetPreparationTagsForImplement(int32 ImplementIndex) const
{
    if (ImplementPreparationTags.IsValidIndex(ImplementIndex))
    {
        return ImplementPreparationTags[ImplementIndex];
    }
    return FGameplayTagContainer();
}

// ============================================================================
// Pantry Functions
// ============================================================================

void UPUDishCustomizationWidget::SetPantryContainer(UPanelWidget* Container)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainer - Setting pantry container"));
    
    if (Container)
    {
        PantryContainer = Container;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainer - Container set successfully"));
        
        // If we already have created pantry shelving widgets, add them to the new container
        if (CreatedPantryShelvingWidgets.Num() > 0)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainer - Adding %d existing pantry shelving widgets to container"), CreatedPantryShelvingWidgets.Num());
            for (UUserWidget* ShelvingWidget : CreatedPantryShelvingWidgets)
            {
                if (ShelvingWidget && !ShelvingWidget->GetParent())
                {
                    Container->AddChild(ShelvingWidget);
                }
            }
        }
        else if (!bPantrySlotsCreated)
        {
            // If container is set but slots haven't been created yet, populate them now
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainer - Container set but slots not created, populating now"));
            PopulatePantrySlots();
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::SetPantryContainer - Container is null"));
    }
}

void UPUDishCustomizationWidget::SetPantryContainerByName(const FName& ContainerName)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainerByName - Searching for container named: %s"), *ContainerName.ToString());
    
    // Search for the widget by name in the widget hierarchy
    UWidget* FoundWidget = GetWidgetFromName(ContainerName);
    
    if (!FoundWidget)
    {
        // Try searching recursively through the widget tree
        FoundWidget = WidgetTree ? WidgetTree->FindWidget(ContainerName) : nullptr;
    }
    
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        SetPantryContainer(PanelWidget);
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPantryContainerByName - Found and set container: %s"), *ContainerName.ToString());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::SetPantryContainerByName - Could not find container widget named: %s"), *ContainerName.ToString());
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   Make sure the widget exists and is a UPanelWidget (HorizontalBox, VerticalBox, etc.)"));
    }
}

void UPUDishCustomizationWidget::TeardownRecipeLogBaseDynamicWidgets(bool bFinishDestroyInstances)
{
    (void)bFinishDestroyInstances;
    for (UPUIngredientSlot* SlotWidget : CreatedRecipeLogBaseSlots)
    {
        if (!IsValid(SlotWidget))
        {
            continue;
        }
        SlotWidget->RemoveFromParent();
        SlotWidget->ReleaseSlateResources(true);
    }
    CreatedRecipeLogBaseSlots.Empty();

    CurrentRecipeLogBaseShelvingWidget.Reset();
    CurrentRecipeLogBaseShelvingWidgetSlotCount = 0;

    for (UUserWidget* Shelving : CreatedRecipeLogBaseShelvingWidgets)
    {
        if (!IsValid(Shelving))
        {
            continue;
        }
        Shelving->RemoveFromParent();
        Shelving->ReleaseSlateResources(true);
    }
    CreatedRecipeLogBaseShelvingWidgets.Empty();
}

void UPUDishCustomizationWidget::TeardownRecipeLogPreppedDynamicWidgets(bool bFinishDestroyInstances)
{
    (void)bFinishDestroyInstances;
    for (UPUIngredientSlot* SlotWidget : CreatedRecipeLogPreppedSlots)
    {
        if (!IsValid(SlotWidget))
        {
            continue;
        }
        SlotWidget->RemoveFromParent();
        SlotWidget->ReleaseSlateResources(true);
    }
    CreatedRecipeLogPreppedSlots.Empty();

    CurrentRecipeLogPreppedShelvingWidget.Reset();
    CurrentRecipeLogPreppedShelvingWidgetSlotCount = 0;

    for (UUserWidget* Shelving : CreatedRecipeLogPreppedShelvingWidgets)
    {
        if (!IsValid(Shelving))
        {
            continue;
        }
        Shelving->RemoveFromParent();
        Shelving->ReleaseSlateResources(true);
    }
    CreatedRecipeLogPreppedShelvingWidgets.Empty();
}

void UPUDishCustomizationWidget::TeardownRecipeLogDynamicWidgets(bool bFinishDestroyInstances)
{
    TeardownRecipeLogBaseDynamicWidgets(bFinishDestroyInstances);
    TeardownRecipeLogPreppedDynamicWidgets(bFinishDestroyInstances);
}

void UPUDishCustomizationWidget::TryResolveRecipeLogPanelsFromHierarchy()
{
    if (RecipeLogBaseScrollBox)
    {
        RecipeLogBaseContainer = RecipeLogBaseScrollBox.Get();
    }
    else if (!RecipeLogBaseContainer.IsValid())
    {
        if (UPanelWidget* Found = FindRecipeLogPanelByNames(
                this,
                {FName(TEXT("RecipeLogBaseScrollBox")), FName(TEXT("RecipeLogBaseContainer"))}))
        {
            RecipeLogBaseContainer = Found;
        }
    }

    if (RecipeLogPreppedScrollBox)
    {
        RecipeLogPreppedContainer = RecipeLogPreppedScrollBox.Get();
    }
    else if (!RecipeLogPreppedContainer.IsValid())
    {
        if (UPanelWidget* Found = FindRecipeLogPanelByNames(
                this,
                {FName(TEXT("RecipeLogPreppedScrollBox")), FName(TEXT("RecipeLogPreppedContainer"))}))
        {
            RecipeLogPreppedContainer = Found;
        }
    }
}

void UPUDishCustomizationWidget::SetRecipeLogBaseContainer(UPanelWidget* Container)
{
    UPanelWidget* Previous = RecipeLogBaseContainer.IsValid() ? RecipeLogBaseContainer.Get() : nullptr;
    if (Previous != Container)
    {
        TeardownRecipeLogBaseDynamicWidgets(true);
        bRecipeLogBaseHierarchyBuilt = false;
        CachedRecipeLogBaseSignature = 0;
    }
    RecipeLogBaseContainer = Container;
    RefreshRecipeLog();
}

void UPUDishCustomizationWidget::SetRecipeLogBaseContainerByName(const FName& ContainerName)
{
    UWidget* FoundWidget = GetWidgetFromName(ContainerName);
    if (!FoundWidget)
    {
        FoundWidget = WidgetTree ? WidgetTree->FindWidget(ContainerName) : nullptr;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        SetRecipeLogBaseContainer(PanelWidget);
    }
}

void UPUDishCustomizationWidget::SetRecipeLogPreppedContainer(UPanelWidget* Container)
{
    UPanelWidget* Previous = RecipeLogPreppedContainer.IsValid() ? RecipeLogPreppedContainer.Get() : nullptr;
    if (Previous != Container)
    {
        TeardownRecipeLogPreppedDynamicWidgets(true);
        bRecipeLogPreppedHierarchyBuilt = false;
        CachedRecipeLogPreppedSignature = 0;
    }
    RecipeLogPreppedContainer = Container;
    RefreshRecipeLog();
}

void UPUDishCustomizationWidget::SetRecipeLogPreppedContainerByName(const FName& ContainerName)
{
    UWidget* FoundWidget = GetWidgetFromName(ContainerName);
    if (!FoundWidget)
    {
        FoundWidget = WidgetTree ? WidgetTree->FindWidget(ContainerName) : nullptr;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        SetRecipeLogPreppedContainer(PanelWidget);
    }
}

void UPUDishCustomizationWidget::NavigateJournalToIngredientInInventory_Implementation(FGameplayTag IngredientTag)
{
    if (!IngredientTag.IsValid())
    {
        return;
    }
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    UPUJournalWidget* Journal = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (AProjectUmeowmiCharacter* Char = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
        {
            Journal = Char->GetJournalWidget();
        }
    }
    if (!Journal)
    {
        TArray<UUserWidget*> FoundWidgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UPUJournalWidget::StaticClass(), false);
        for (UUserWidget* W : FoundWidgets)
        {
            if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
            {
                Journal = J;
                break;
            }
        }
    }
    if (Journal)
    {
        Journal->OpenJournalToIngredient(IngredientTag);
    }
}

void UPUDishCustomizationWidget::RefreshRecipeLog()
{
    UWorld* World = GetWorld();
    if (!IsValid(this) || HasAnyFlags(RF_BeginDestroyed) || !World)
    {
        return;
    }

    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(World, this))
    {
        return;
    }

    if (IsGarbageCollecting())
    {
        return;
    }

    const int32 SlotsPerRow = FMath::Max(1, RecipeLogSlotsPerRow);
    const int32 MaxBase = FMath::Max(0, RecipeLogMaxBaseIngredients);

    UPanelWidget* BaseContainer = RecipeLogBaseContainer.IsValid() ? RecipeLogBaseContainer.Get() : nullptr;
    if (BaseContainer && !IsValid(BaseContainer))
    {
        BaseContainer = nullptr;
    }
    UPanelWidget* PreppedContainer = RecipeLogPreppedContainer.IsValid() ? RecipeLogPreppedContainer.Get() : nullptr;
    if (PreppedContainer && !IsValid(PreppedContainer))
    {
        PreppedContainer = nullptr;
    }

    TArray<FIngredientInstance> BaseInstances;
    for (const FIngredientInstance& Inst : CurrentDishData.IngredientInstances)
    {
        if (!Inst.IngredientData.IngredientTag.IsValid())
        {
            continue;
        }
        if (IngredientInstanceHasAnyPreparation(Inst))
        {
            continue;
        }
        BaseInstances.Add(Inst);
    }
    while (BaseInstances.Num() > MaxBase)
    {
        BaseInstances.Pop();
    }

    TArray<FIngredientInstance> PreppedInstances;
    for (const FIngredientInstance& Inst : CurrentDishData.IngredientInstances)
    {
        if (IngredientInstanceHasAnyPreparation(Inst))
        {
            PreppedInstances.Add(Inst);
        }
    }

    const uint32 NewBaseSig = ComputeRecipeLogBaseContentSignature(BaseInstances);
    const uint32 NewPreppedSig = ComputeRecipeLogPreppedContentSignature(PreppedInstances);

    const int32 NumBase = BaseInstances.Num();
    const int32 TotalBaseSlots =
        BaseContainer ? FMath::Max(SlotsPerRow, FMath::DivideAndRoundUp(NumBase, SlotsPerRow) * SlotsPerRow) : 0;

    const int32 NumPrepped = PreppedInstances.Num();
    const int32 TotalPreppedSlots =
        PreppedContainer ? FMath::Max(SlotsPerRow, FMath::DivideAndRoundUp(NumPrepped, SlotsPerRow) * SlotsPerRow) : 0;

    const bool bSkipBaseRebuild = BaseContainer != nullptr && bRecipeLogBaseHierarchyBuilt && NewBaseSig == CachedRecipeLogBaseSignature
                                  && SlotsPerRow == CachedRecipeLogSlotsPerRowForRecipeLog && MaxBase == CachedRecipeLogMaxBaseForRecipeLog
                                  && CreatedRecipeLogBaseSlots.Num() == TotalBaseSlots && AreRecipeLogSlotsValid(CreatedRecipeLogBaseSlots);

    const bool bSkipPreppedRebuild =
        PreppedContainer != nullptr && bRecipeLogPreppedHierarchyBuilt && NewPreppedSig == CachedRecipeLogPreppedSignature
        && SlotsPerRow == CachedRecipeLogSlotsPerRowForRecipeLog && CreatedRecipeLogPreppedSlots.Num() == TotalPreppedSlots
        && AreRecipeLogSlotsValid(CreatedRecipeLogPreppedSlots);

    if (!BaseContainer)
    {
        if (bRecipeLogBaseHierarchyBuilt || CreatedRecipeLogBaseSlots.Num() > 0 || CreatedRecipeLogBaseShelvingWidgets.Num() > 0)
        {
            TeardownRecipeLogBaseDynamicWidgets(true);
        }
        bRecipeLogBaseHierarchyBuilt = false;
        CachedRecipeLogBaseSignature = 0;
    }

    if (!PreppedContainer)
    {
        if (bRecipeLogPreppedHierarchyBuilt || CreatedRecipeLogPreppedSlots.Num() > 0 || CreatedRecipeLogPreppedShelvingWidgets.Num() > 0)
        {
            TeardownRecipeLogPreppedDynamicWidgets(true);
        }
        bRecipeLogPreppedHierarchyBuilt = false;
        CachedRecipeLogPreppedSignature = 0;
    }

    auto CreatePaddingSlot = [&](EPUIngredientSlotLocation SlotLocation) -> UPUIngredientSlot* {
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }
        UPUIngredientSlot* RLSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (!RLSlot)
        {
            return nullptr;
        }
        RLSlot->SetDishCustomizationWidget(this);
        RLSlot->SetRecipeLogSlot(false);
        RLSlot->SetPreppedPantryPickerSlot(false);
        RLSlot->SetPantryShelfPaddingCell(true);
        RLSlot->SetLocation(SlotLocation);
        RLSlot->SetDragEnabled(false);
        RLSlot->SetIsEnabled(false);
        RLSlot->SetIsFocusable(false);
        FIngredientInstance EmptyInst;
        RLSlot->SetIngredientInstance(EmptyInst);
        if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
        {
            RLSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
        }
        return RLSlot;
    };

    auto CreateFilledRecipeLogSlot = [&](EPUIngredientSlotLocation SlotLocation, const FIngredientInstance& SourceInst) -> UPUIngredientSlot* {
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }
        UPUIngredientSlot* RLSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (!RLSlot)
        {
            return nullptr;
        }
        RLSlot->SetDishCustomizationWidget(this);
        RLSlot->SetRecipeLogSlot(true);
        RLSlot->SetPreppedPantryPickerSlot(false);
        RLSlot->SetPantryShelfPaddingCell(false);
        RLSlot->SetLocation(SlotLocation);
        RLSlot->SetDragEnabled(false);
        RLSlot->SetIsEnabled(true);
        RLSlot->SetIsFocusable(true);
        FIngredientInstance DisplayInst = SourceInst;
        DisplayInst.Quantity = 0;
        RLSlot->SetIngredientInstance(DisplayInst);
        if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
        {
            RLSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
        }
        return RLSlot;
    };

    if (!bSkipBaseRebuild && BaseContainer)
    {
        TeardownRecipeLogBaseDynamicWidgets(true);

        for (int32 SlotIndex = 0; SlotIndex < TotalBaseSlots; ++SlotIndex)
        {
            const bool bIsPadding = SlotIndex >= NumBase;
            UPUIngredientSlot* RLSlot = bIsPadding ? CreatePaddingSlot(EPUIngredientSlotLocation::Pantry)
                                                   : CreateFilledRecipeLogSlot(EPUIngredientSlotLocation::Pantry, BaseInstances[SlotIndex]);
            if (!RLSlot)
            {
                continue;
            }
            CreatedRecipeLogBaseSlots.Add(RLSlot);
            if (UUserWidget* ShelvingWidget = GetOrCreateCurrentRecipeLogBaseShelvingWidget(BaseContainer))
            {
                AddSlotToCurrentRecipeLogBaseShelvingWidget(RLSlot);
            }
        }
        CachedRecipeLogBaseSignature = NewBaseSig;
        bRecipeLogBaseHierarchyBuilt = true;
    }

    if (!bSkipPreppedRebuild && PreppedContainer)
    {
        TeardownRecipeLogPreppedDynamicWidgets(true);

        for (int32 SlotIndex = 0; SlotIndex < TotalPreppedSlots; ++SlotIndex)
        {
            const bool bIsPadding = SlotIndex >= NumPrepped;
            UPUIngredientSlot* RLSlot = bIsPadding ? CreatePaddingSlot(EPUIngredientSlotLocation::Prepped)
                                                   : CreateFilledRecipeLogSlot(EPUIngredientSlotLocation::Prepped, PreppedInstances[SlotIndex]);
            if (!RLSlot)
            {
                continue;
            }
            CreatedRecipeLogPreppedSlots.Add(RLSlot);
            if (UUserWidget* ShelvingWidget = GetOrCreateCurrentRecipeLogPreppedShelvingWidget(PreppedContainer))
            {
                AddSlotToCurrentRecipeLogPreppedShelvingWidget(RLSlot);
            }
        }
        CachedRecipeLogPreppedSignature = NewPreppedSig;
        bRecipeLogPreppedHierarchyBuilt = true;
    }

    CachedRecipeLogSlotsPerRowForRecipeLog = SlotsPerRow;
    CachedRecipeLogMaxBaseForRecipeLog = MaxBase;
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentRecipeLogBaseShelvingWidget(UPanelWidget* ContainerToUse)
{
    const int32 RowSlots = FMath::Max(1, RecipeLogSlotsPerRow);
    if (!CurrentRecipeLogBaseShelvingWidget.IsValid() || CurrentRecipeLogBaseShelvingWidgetSlotCount >= RowSlots)
    {
        if (!ShelvingWidgetClass)
        {
            return nullptr;
        }
        if (!GetWorld())
        {
            return nullptr;
        }
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            return nullptr;
        }
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
        }
        CreatedRecipeLogBaseShelvingWidgets.Add(NewShelvingWidget);
        CurrentRecipeLogBaseShelvingWidget = NewShelvingWidget;
        CurrentRecipeLogBaseShelvingWidgetSlotCount = 0;
        return NewShelvingWidget;
    }
    return CurrentRecipeLogBaseShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentRecipeLogBaseShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot || !CurrentRecipeLogBaseShelvingWidget.IsValid())
    {
        return false;
    }

    UWidget* FoundWidget = CurrentRecipeLogBaseShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        FoundWidget = CurrentRecipeLogBaseShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentRecipeLogBaseShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }

    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentRecipeLogBaseShelvingWidgetSlotCount++;
        return true;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentRecipeLogBaseShelvingWidgetSlotCount++;
        return true;
    }
    return false;
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentRecipeLogPreppedShelvingWidget(UPanelWidget* ContainerToUse)
{
    const int32 RowSlots = FMath::Max(1, RecipeLogSlotsPerRow);
    if (!CurrentRecipeLogPreppedShelvingWidget.IsValid() || CurrentRecipeLogPreppedShelvingWidgetSlotCount >= RowSlots)
    {
        if (!ShelvingWidgetClass)
        {
            return nullptr;
        }
        if (!GetWorld())
        {
            return nullptr;
        }
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            return nullptr;
        }
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
        }
        CreatedRecipeLogPreppedShelvingWidgets.Add(NewShelvingWidget);
        CurrentRecipeLogPreppedShelvingWidget = NewShelvingWidget;
        CurrentRecipeLogPreppedShelvingWidgetSlotCount = 0;
        return NewShelvingWidget;
    }
    return CurrentRecipeLogPreppedShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentRecipeLogPreppedShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot || !CurrentRecipeLogPreppedShelvingWidget.IsValid())
    {
        return false;
    }

    UWidget* FoundWidget = CurrentRecipeLogPreppedShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        FoundWidget = CurrentRecipeLogPreppedShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentRecipeLogPreppedShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }

    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentRecipeLogPreppedShelvingWidgetSlotCount++;
        return true;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentRecipeLogPreppedShelvingWidgetSlotCount++;
        return true;
    }
    return false;
}

void UPUDishCustomizationWidget::SetPreppedPantryContainer(UPanelWidget* Container)
{
    if (!Container)
    {
        PreppedPantryContainer = nullptr;
        RefreshPreppedPantrySlots();
        return;
    }

    UPanelWidget* Previous = PreppedPantryContainer.IsValid() ? PreppedPantryContainer.Get() : nullptr;
    if (Previous != Container)
    {
        bPreppedPantrySlotsHierarchyBuilt = false;
        CachedPreppedPantrySlotsContentSignature = 0;
    }

    PreppedPantryContainer = Container;
    RefreshPreppedPantrySlots();
}

void UPUDishCustomizationWidget::SetPreppedPantryContainerByName(const FName& ContainerName)
{
    UWidget* FoundWidget = GetWidgetFromName(ContainerName);
    if (!FoundWidget)
    {
        FoundWidget = WidgetTree ? WidgetTree->FindWidget(ContainerName) : nullptr;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        SetPreppedPantryContainer(PanelWidget);
    }
}

void UPUDishCustomizationWidget::RefreshPreppedPantrySlots()
{
    if (!IsValid(this) || !GetWorld())
    {
        return;
    }

    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }

    if (IsGarbageCollecting())
    {
        return;
    }

    auto ApplyPreppedPantryTutorialLocks = [&]() {
        if (UPUProjectUmeowmiGameInstance* GI = GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>())
        {
            if (GI->IsTutorialModeEnabled())
            {
                const FGameplayTag AllowedTag = GI->GetTutorialAllowedIngredientTag();
                for (UPUIngredientSlot* PSlot : CreatedPreppedPantrySlots)
                {
                    if (!PSlot)
                    {
                        continue;
                    }
                    const FIngredientInstance& Inst = PSlot->GetIngredientInstance();
                    if (!Inst.IngredientData.IngredientTag.IsValid())
                    {
                        PSlot->SetIsEnabled(false);
                        PSlot->SetDragEnabled(false);
                        continue;
                    }
                    const bool bAllowed = !AllowedTag.IsValid() || (Inst.IngredientData.IngredientTag == AllowedTag);
                    PSlot->SetIsEnabled(bAllowed);
                    PSlot->SetDragEnabled(bAllowed);
                }
            }
        }
    };

    UPanelWidget* ContainerToUse = PreppedPantryContainer.IsValid() ? PreppedPantryContainer.Get() : nullptr;

    TArray<FIngredientInstance> PreppedInstances;
    for (const FIngredientInstance& Inst : CurrentDishData.IngredientInstances)
    {
        if (IngredientInstanceHasAnyPreparation(Inst))
        {
            PreppedInstances.Add(Inst);
        }
    }

    const uint32 NewPreppedPantrySig = ComputeRecipeLogPreppedContentSignature(PreppedInstances);
    const int32 NumPrepped = PreppedInstances.Num();
    const int32 TotalSlots = FMath::Max(3, (NumPrepped + 2) / 3 * 3);

    if (!ContainerToUse)
    {
        for (UPUIngredientSlot* PreppedPantrySlotWidget : CreatedPreppedPantrySlots)
        {
            if (!IsValid(PreppedPantrySlotWidget))
            {
                continue;
            }
            if (PreppedPantrySlotWidget->IsPreppedPantryPickerSlot())
            {
                PreppedPantrySlotWidget->OnEmptySlotClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);
            }
            if (PreppedPantrySlotWidget->GetParent())
            {
                PreppedPantrySlotWidget->RemoveFromParent();
            }
            PreppedPantrySlotWidget->ReleaseSlateResources(true);
        }
        CreatedPreppedPantrySlots.Empty();

        CurrentPreppedPantryShelvingWidget.Reset();

        for (UUserWidget* Shelving : CreatedPreppedPantryShelvingWidgets)
        {
            if (!IsValid(Shelving))
            {
                continue;
            }
            if (Shelving->GetParent())
            {
                Shelving->RemoveFromParent();
            }
            Shelving->ReleaseSlateResources(true);
        }
        CreatedPreppedPantryShelvingWidgets.Empty();
        CurrentPreppedPantryShelvingWidgetSlotCount = 0;

        bPreppedPantrySlotsHierarchyBuilt = false;
        CachedPreppedPantrySlotsContentSignature = 0;
        return;
    }

    const bool bSkipPreppedPantryRebuild = bPreppedPantrySlotsHierarchyBuilt && NewPreppedPantrySig == CachedPreppedPantrySlotsContentSignature
                                           && CreatedPreppedPantrySlots.Num() == TotalSlots && AreRecipeLogSlotsValid(CreatedPreppedPantrySlots);

    if (bSkipPreppedPantryRebuild)
    {
        ApplyPreppedPantryTutorialLocks();
        return;
    }

    for (UPUIngredientSlot* PreppedPantrySlotWidget : CreatedPreppedPantrySlots)
    {
        if (!IsValid(PreppedPantrySlotWidget))
        {
            continue;
        }
        if (PreppedPantrySlotWidget->IsPreppedPantryPickerSlot())
        {
            PreppedPantrySlotWidget->OnEmptySlotClicked.RemoveDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);
        }
        if (PreppedPantrySlotWidget->GetParent())
        {
            PreppedPantrySlotWidget->RemoveFromParent();
        }
        PreppedPantrySlotWidget->ReleaseSlateResources(true);
    }
    CreatedPreppedPantrySlots.Empty();

    CurrentPreppedPantryShelvingWidget.Reset();

    for (UUserWidget* Shelving : CreatedPreppedPantryShelvingWidgets)
    {
        if (!IsValid(Shelving))
        {
            continue;
        }
        if (Shelving->GetParent())
        {
            Shelving->RemoveFromParent();
        }
        Shelving->ReleaseSlateResources(true);
    }
    CreatedPreppedPantryShelvingWidgets.Empty();
    CurrentPreppedPantryShelvingWidgetSlotCount = 0;

    for (int32 SlotIndex = 0; SlotIndex < TotalSlots; ++SlotIndex)
    {
        const bool bIsPadding = SlotIndex >= NumPrepped;

        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }

        UPUIngredientSlot* PantrySlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (!PantrySlot)
        {
            continue;
        }

        PantrySlot->SetDishCustomizationWidget(this);

        if (bIsPadding)
        {
            PantrySlot->SetPreppedPantryPickerSlot(false);
            PantrySlot->SetPantryShelfPaddingCell(true);
            PantrySlot->SetLocation(EPUIngredientSlotLocation::Prepped);
            PantrySlot->SetDragEnabled(false);
            PantrySlot->SetIsEnabled(false);
            PantrySlot->SetIsFocusable(false);

            FIngredientInstance EmptyInst;
            PantrySlot->SetIngredientInstance(EmptyInst);

            if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
            {
                PantrySlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
            }

            CreatedPreppedPantrySlots.Add(PantrySlot);

            if (UUserWidget* ShelvingWidget = GetOrCreateCurrentPreppedPantryShelvingWidget(ContainerToUse))
            {
                AddSlotToCurrentPreppedPantryShelvingWidget(PantrySlot);
            }
            continue;
        }

        PantrySlot->SetPreppedPantryPickerSlot(true);
        PantrySlot->SetPantryShelfPaddingCell(false);
        PantrySlot->SetLocation(EPUIngredientSlotLocation::Prepped);
        PantrySlot->SetDragEnabled(true);
        PantrySlot->SetIsEnabled(true);
        PantrySlot->SetIsFocusable(true);

        FIngredientInstance DisplayInst = PreppedInstances[SlotIndex];
        DisplayInst.Quantity = 0;

        PantrySlot->SetIngredientInstance(DisplayInst);

        if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
        {
            PantrySlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
        }

        PantrySlot->OnEmptySlotClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);

        CreatedPreppedPantrySlots.Add(PantrySlot);

        if (UUserWidget* ShelvingWidget = GetOrCreateCurrentPreppedPantryShelvingWidget(ContainerToUse))
        {
            AddSlotToCurrentPreppedPantryShelvingWidget(PantrySlot);
        }
    }

    CachedPreppedPantrySlotsContentSignature = NewPreppedPantrySig;
    bPreppedPantrySlotsHierarchyBuilt = true;

    ApplyPreppedPantryTutorialLocks();
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentPreppedPantryShelvingWidget(UPanelWidget* ContainerToUse)
{
    if (!CurrentPreppedPantryShelvingWidget.IsValid() || CurrentPreppedPantryShelvingWidgetSlotCount >= 3)
    {
        if (!ShelvingWidgetClass)
        {
            return nullptr;
        }
        if (!GetWorld())
        {
            return nullptr;
        }
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            return nullptr;
        }
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
        }
        CreatedPreppedPantryShelvingWidgets.Add(NewShelvingWidget);
        CurrentPreppedPantryShelvingWidget = NewShelvingWidget;
        CurrentPreppedPantryShelvingWidgetSlotCount = 0;
        return NewShelvingWidget;
    }
    return CurrentPreppedPantryShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentPreppedPantryShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot || !CurrentPreppedPantryShelvingWidget.IsValid())
    {
        return false;
    }

    UWidget* FoundWidget = CurrentPreppedPantryShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        FoundWidget = CurrentPreppedPantryShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentPreppedPantryShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }

    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentPreppedPantryShelvingWidgetSlotCount++;
        return true;
    }
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentPreppedPantryShelvingWidgetSlotCount++;
        return true;
    }
    return false;
}

void UPUDishCustomizationWidget::PopulatePantrySlots()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::PopulatePantrySlots - Populating pantry slots"));
    
    // Check if slots were already created
    if (bPantrySlotsCreated)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::PopulatePantrySlots - Pantry slots already created, skipping"));
        return;
    }

    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }
    
    // Clear any existing shelving widgets and reset state
    CreatedPantryShelvingWidgets.Empty();
    CurrentPantryShelvingWidget.Reset();
    CurrentPantryShelvingWidgetSlotCount = 0;
    
    // Check if we have a valid world context
    if (!GetWorld())
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::PopulatePantrySlots - No world context available"));
        return;
    }
    
    // Check if we have a customization component to get ingredient data
    if (!CustomizationComponent)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::PopulatePantrySlots - No customization component available"));
        return;
    }
    
    // Get all available ingredients from the component
    TArray<FPUIngredientBase> AvailableIngredients = CustomizationComponent->GetIngredientData();
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::PopulatePantrySlots - Found %d available ingredients"), AvailableIngredients.Num());
    
    // Get the container to use
    UPanelWidget* ContainerToUse = nullptr;
    if (PantryContainer.IsValid())
    {
        ContainerToUse = PantryContainer.Get();
    }
    
    if (!ContainerToUse)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::PopulatePantrySlots - No pantry container set! Slots cannot be added."));
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   Slots will be added when SetPantryContainer() is called."));
        // Don't mark as created if we don't have a container - we'll try again when container is set
        return;
    }
    
    // Pad pantry shelves to multiples of three slots (minimum one row of three).
    const int32 NumIngredients = AvailableIngredients.Num();
    const int32 TotalSlots = FMath::Max(3, (NumIngredients + 2) / 3 * 3);

    for (int32 SlotIndex = 0; SlotIndex < TotalSlots; ++SlotIndex)
    {
        const bool bIsShelfPadding = SlotIndex >= NumIngredients;

        // Create ingredient slot using the Blueprint class
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }

        UPUIngredientSlot* PantrySlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (!PantrySlot)
        {
            continue;
        }

        PantrySlot->SetDishCustomizationWidget(this);

        if (bIsShelfPadding)
        {
            PantrySlot->SetPantryShelfPaddingCell(true);
            PantrySlot->SetLocation(EPUIngredientSlotLocation::Pantry);
            PantrySlot->SetDragEnabled(false);
            PantrySlot->SetIsEnabled(false);
            PantrySlot->SetIsFocusable(false);

            FIngredientInstance EmptyPantryInstance;
            PantrySlot->SetIngredientInstance(EmptyPantryInstance);

            if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
            {
                PantrySlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
            }

            CreatedPantrySlots.Add(PantrySlot);

            if (ContainerToUse)
            {
                UUserWidget* ShelvingWidget = GetOrCreateCurrentPantryShelvingWidget(ContainerToUse);
                if (ShelvingWidget)
                {
                    AddSlotToCurrentPantryShelvingWidget(PantrySlot);
                }
            }
            continue;
        }

        const FPUIngredientBase& IngredientData = AvailableIngredients[SlotIndex];

        PantrySlot->SetPantryShelfPaddingCell(false);
        PantrySlot->SetLocation(EPUIngredientSlotLocation::Pantry);
        PantrySlot->SetDragEnabled(true);

        // Create a minimal ingredient instance with just the ingredient data (quantity 0)
        FIngredientInstance PantryInstance;
        PantryInstance.IngredientData = IngredientData;
        PantryInstance.IngredientTag = IngredientData.IngredientTag;
        PantryInstance.Quantity = 0;
        PantryInstance.InstanceID = 0;

        PantrySlot->SetIngredientInstance(PantryInstance);

        if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
        {
            PantrySlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
        }

        PantrySlotMap.Add(IngredientData.IngredientTag, PantrySlot);
        CreatedPantrySlots.Add(PantrySlot);
        PantrySlot->OnEmptySlotClicked.AddDynamic(this, &UPUDishCustomizationWidget::OnPantrySlotClicked);

        if (ContainerToUse)
        {
            UUserWidget* ShelvingWidget = GetOrCreateCurrentPantryShelvingWidget(ContainerToUse);
            if (ShelvingWidget)
            {
                AddSlotToCurrentPantryShelvingWidget(PantrySlot);
            }
        }
    }

    if (CreatedPantrySlots.Num() > 0 && ContainerToUse)
    {
        bPantrySlotsCreated = true;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::PopulatePantrySlots - Successfully created %d pantry slots in %d shelving widgets"), 
        //    CreatedPantrySlots.Num(), CreatedPantryShelvingWidgets.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::PopulatePantrySlots - Created %d slots but container not set, not marking as created"), 
        //    CreatedPantrySlots.Num());
    }
}

void UPUDishCustomizationWidget::OpenPantry()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OpenPantry - Opening pantry"));
    
    // Populate pantry slots if not already populated
    if (!bPantrySlotsCreated)
    {
        PopulatePantrySlots();
    }

    RefreshPreppedPantrySlots();
    RefreshRecipeLog();
    
    // Tutorial: initialize step 1 when first opening pantry in tutorial mode
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        if (GI->IsTutorialModeEnabled() && GI->GetTutorialStep() == 0)
        {
            GI->SetTutorialStep(1);
            GI->SaveGame();
        }
        
        // Apply pantry slot restrictions (enable only the allowed ingredient in tutorial mode)
        const FGameplayTag AllowedTag = GI->GetTutorialAllowedIngredientTag();
        for (UPUIngredientSlot* PantrySlot : CreatedPantrySlots)
        {
            if (PantrySlot)
            {
                const FIngredientInstance& Instance = PantrySlot->GetIngredientInstance();
                if (!Instance.IngredientData.IngredientTag.IsValid())
                {
                    PantrySlot->SetIsEnabled(false);
                    PantrySlot->SetDragEnabled(false);
                    continue;
                }
                const bool bIsAllowed = !AllowedTag.IsValid() || (Instance.IngredientData.IngredientTag == AllowedTag);
                PantrySlot->SetIsEnabled(bIsAllowed);
                PantrySlot->SetDragEnabled(bIsAllowed);
            }
        }
        
        // Show tutorial popup for step 1 when pantry opens
        if (GI->IsTutorialModeEnabled() && GI->GetTutorialStep() == 1)
        {
            FPopupData PopupData;
            PopupData.PopupType = EPopupType::Tutorial;
            PopupData.Title = FText::FromString(TEXT("TUTORIAL"));
            PopupData.Message = FText::FromString(TEXT("Let's add the Egg Yolk Cookies to a Prep Plate. Find the cookies in your Pantry window and select the ingredient to add it to a Prep Plate."));
            PopupData.bModal = true;
            PopupData.bShowCloseButton = true;
            GI->ShowPopup(PopupData);
        }
    }
    
    // Set up navigation for pantry slots (for controller support)
    SetupPantrySlotNavigation();
    SetupPreppedPantrySlotNavigation();
    
    // Set pantry open flag
    bPantryOpen = true;
    
    // Call Blueprint event to trigger UMG animation
    OnPantryOpened();
    
    // Set initial focus for pantry - but NOT when a popup or dialogue is showing.
    // Otherwise we steal focus and the user can't dismiss/advance with controller.
    UPUProjectUmeowmiGameInstance* GICheck = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
    if ((!GICheck || !GICheck->IsPopupShowing()) && !IsDialogueVisible())
    {
        SetInitialFocusForPantry();
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OpenPantry - Pantry opened (Blueprint will handle animation)"));
}

void UPUDishCustomizationWidget::ClosePantry()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ClosePantry - Closing pantry"));
    
    // Set pantry open flag
    bPantryOpen = false;
    
    // Clear pending empty slot
    PendingEmptySlot.Reset();
    
    // Call Blueprint event to trigger UMG animation (normal close - play forward or hide)
    OnPantryClosed();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ClosePantry - Pantry closed (Blueprint will handle animation)"));
}

void UPUDishCustomizationWidget::ClosePantryFromDrag()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ClosePantryFromDrag - Closing pantry from drag operation"));
    
    // Set pantry open flag
    bPantryOpen = false;
    
    // Clear pending empty slot
    PendingEmptySlot.Reset();
    
    // Call Blueprint event to trigger UMG animation in reverse
    OnPantryClosedFromDrag();
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::ClosePantryFromDrag - Pantry closed from drag (Blueprint will play animation in reverse)"));
}

void UPUDishCustomizationWidget::OnPantryButtonClicked()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnPantryButtonClicked - Pantry button clicked"));
    
    if (bPantryOpen)
    {
        ClosePantry();
    }
    else
    {
        OpenPantry();
    }
}

void UPUDishCustomizationWidget::OnEmptySlotClicked(UPUIngredientSlot* IngredientSlot)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnEmptySlotClicked - Empty slot clicked (Location: %d)"), 
    //    IngredientSlot ? (int32)IngredientSlot->GetLocation() : -1);
    
    if (!IngredientSlot)
    {
        return;
    }
    
    if (IngredientSlot->GetLocation() != EPUIngredientSlotLocation::ActiveIngredientArea)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::OnEmptySlotClicked - Ignoring slot click (Location: %d)"),
        //    (int32)IngredientSlot->GetLocation());
        return;
    }

    // Strip slot click toggles pantry (same expectation as the pantry button).
    if (bPantryOpen)
    {
        ClosePantry();
        return;
    }

    PendingEmptySlot = IngredientSlot;
    OpenPantry();
}

UUserWidget* UPUDishCustomizationWidget::GetOrCreateCurrentPantryShelvingWidget(UPanelWidget* ContainerToUse)
{
    // Check if we need a new shelving widget
    // Need a new one if: no current widget, or current widget has 3 slots
    if (!CurrentPantryShelvingWidget.IsValid() || CurrentPantryShelvingWidgetSlotCount >= 3)
    {
        // Create a new shelving widget
        if (!ShelvingWidgetClass)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentPantryShelvingWidget - ShelvingWidgetClass not set!"));
            return nullptr;
        }
        
        if (!GetWorld())
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentPantryShelvingWidget - No world context available"));
            return nullptr;
        }
        
        UUserWidget* NewShelvingWidget = CreateWidget<UUserWidget>(this, ShelvingWidgetClass);
        if (!NewShelvingWidget)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::GetOrCreateCurrentPantryShelvingWidget - Failed to create shelving widget"));
            return nullptr;
        }
        
        // Add the shelving widget to the container
        if (ContainerToUse)
        {
            ContainerToUse->AddChild(NewShelvingWidget);
            //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::GetOrCreateCurrentPantryShelvingWidget - Created and added new shelving widget (Total: %d)"), 
            //    CreatedPantryShelvingWidgets.Num() + 1);
        }
        
        // Track the new shelving widget
        CreatedPantryShelvingWidgets.Add(NewShelvingWidget);
        CurrentPantryShelvingWidget = NewShelvingWidget;
        CurrentPantryShelvingWidgetSlotCount = 0;
        
        return NewShelvingWidget;
    }
    
    // Return the current shelving widget
    return CurrentPantryShelvingWidget.Get();
}

bool UPUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget(UPUIngredientSlot* IngredientSlot)
{
    if (!IngredientSlot)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget - IngredientSlot is null"));
        return false;
    }
    
    if (!CurrentPantryShelvingWidget.IsValid())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget - CurrentPantryShelvingWidget is not valid"));
        return false;
    }
    
    // Find the HorizontalBox inside the shelving widget
    // First, try to get it by name
    UWidget* FoundWidget = CurrentPantryShelvingWidget->GetWidgetFromName(ShelvingHorizontalBoxName);
    if (!FoundWidget)
    {
        // If not found by name, try common names
        FoundWidget = CurrentPantryShelvingWidget->GetWidgetFromName(TEXT("HorizontalBox"));
        if (!FoundWidget)
        {
            FoundWidget = CurrentPantryShelvingWidget->GetWidgetFromName(TEXT("SlotContainer"));
        }
    }
    
    // Try to cast to HorizontalBox
    if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(FoundWidget))
    {
        HorizontalBox->AddChild(IngredientSlot);
        CurrentPantryShelvingWidgetSlotCount++;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget - Added slot to HorizontalBox (Slot count: %d/3)"), 
        //    CurrentPantryShelvingWidgetSlotCount);
        return true;
    }
    
    // Try casting to any panel widget
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(FoundWidget))
    {
        PanelWidget->AddChild(IngredientSlot);
        CurrentPantryShelvingWidgetSlotCount++;
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget - Added slot to panel widget (Slot count: %d/3)"), 
        //    CurrentPantryShelvingWidgetSlotCount);
        return true;
    }
    
    // If still not found, log error with helpful message
    //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::AddSlotToCurrentPantryShelvingWidget - Could not find HorizontalBox or panel widget in WBP_Shelving!"));
    //UE_LOG(LogTemp,Error, TEXT("   Searched for widget named: '%s', 'HorizontalBox', 'SlotContainer'"), *ShelvingHorizontalBoxName.ToString());
    //UE_LOG(LogTemp,Error, TEXT("   Please ensure WBP_Shelving contains a HorizontalBox (or other panel widget) with one of these names."));
    return false;
}

void UPUDishCustomizationWidget::SetPreppedIngredientContainer(UPanelWidget* Container)
{
    PreppedIngredientContainer = Container;
    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPreppedIngredientContainer - Prepped ingredient container set"));
    
    // Create prepped slots from dish data if we have ingredients (prepped area stays empty until player adds from pantry - dish starts cleared)
    if (CurrentDishData.IngredientInstances.Num() > 0)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPreppedIngredientContainer - Creating prepped slots for %d existing ingredients"), 
        //    CurrentDishData.IngredientInstances.Num());
        
        for (const FIngredientInstance& IngredientInstance : CurrentDishData.IngredientInstances)
        {
            if (IngredientInstance.IngredientData.IngredientTag.IsValid() && IngredientInstance.Quantity > 0)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::SetPreppedIngredientContainer - Creating prepped slot for: %s (ID: %d)"),
                //    *IngredientInstance.IngredientData.DisplayName.ToString(), IngredientInstance.InstanceID);
                CreateOrUpdatePreppedSlot(IngredientInstance);
            }
        }
    }
}

void UPUDishCustomizationWidget::CreateOrUpdatePreppedSlot(const FIngredientInstance& IngredientInstance)
{
    if (!PU_ShouldSpawnDishPantryLikeDynamicWidgets(GetWorld(), this))
    {
        return;
    }

    // Only create prepped slots if we have a container
    if (!PreppedIngredientContainer.IsValid())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - No prepped container set! Cannot create prepped slot for: %s"), 
        //    *IngredientInstance.IngredientData.DisplayName.ToString());
        //UE_LOG(LogTemp,Warning, TEXT("⚠️   This ingredient will be added to prepped area when container is set via SetPreppedIngredientContainer"));
        return;
    }

    // Allow creating prepped slots even without preparations (ingredients in prep area should appear in prepped area)

    // Use ingredient instance ID as the key so each unique instance gets its own prepped slot
    const int32 InstanceID = IngredientInstance.InstanceID;
    if (InstanceID == 0)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - IngredientInstance has InstanceID = 0 for %s; prepped slot may not be tracked correctly."),
        //    *IngredientInstance.IngredientData.DisplayName.ToString());
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - InstanceID: %d"), InstanceID);

    // Check if a prepped slot already exists for this specific ingredient instance
    UPUIngredientSlot* ExistingSlot = PreppedSlotMap.FindRef(InstanceID);

    if (ExistingSlot && ExistingSlot->IsValidLowLevel())
    {
        // Update existing slot with new preparation data
        //UE_LOG(LogTemp,Display, TEXT("🔄 PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - Updating existing prepped slot"));
        ExistingSlot->SetIngredientInstance(IngredientInstance);
        ExistingSlot->UpdateDisplay();
    }
    else
    {
        // Create new prepped slot
        //UE_LOG(LogTemp,Display, TEXT("✨ PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - Creating new prepped slot"));

        // Get the slot class
        TSubclassOf<UPUIngredientSlot> SlotClass;
        if (IngredientSlotClass)
        {
            SlotClass = IngredientSlotClass;
        }
        else
        {
            SlotClass = UPUIngredientSlot::StaticClass();
        }

        // Create the slot widget
        UPUIngredientSlot* PreppedSlot = CreateWidget<UPUIngredientSlot>(this, SlotClass);
        if (PreppedSlot)
        {
            // Set the dish widget reference
            PreppedSlot->SetDishCustomizationWidget(this);

            // Set the location to Prepped
            PreppedSlot->SetLocation(EPUIngredientSlotLocation::Prepped);

            // Set the ingredient instance
            PreppedSlot->SetIngredientInstance(IngredientInstance);

            // Set the preparation data table if available
            if (CustomizationComponent && CustomizationComponent->PreparationDataTable)
            {
                PreppedSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
            }

            // Update the display
            PreppedSlot->UpdateDisplay();

            // Add to container
            PreppedIngredientContainer->AddChild(PreppedSlot);

            // Store in arrays and map
            CreatedPreppedSlots.Add(PreppedSlot);
            PreppedSlotMap.Add(InstanceID, PreppedSlot);

            TArray<FGameplayTag> PrepTags;
            IngredientInstance.Preparations.GetGameplayTagArray(PrepTags);
            //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - Created prepped slot for %s with %d preparations"),
            //    *IngredientInstance.IngredientData.DisplayName.ToString(), PrepTags.Num());
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ PUDishCustomizationWidget::CreateOrUpdatePreppedSlot - Failed to create prepped slot widget"));
        }
    }
}

void UPUDishCustomizationWidget::RemovePreppedSlot(const FIngredientInstance& IngredientInstance)
{
    // Use ingredient instance ID as the key (one prepped slot per unique instance)
    const int32 InstanceID = IngredientInstance.InstanceID;
    
    // Find and remove the slot
    UPUIngredientSlot* SlotToRemove = PreppedSlotMap.FindRef(InstanceID);
    if (SlotToRemove && SlotToRemove->IsValidLowLevel())
    {
        //UE_LOG(LogTemp,Display, TEXT("🗑️ PUDishCustomizationWidget::RemovePreppedSlot - Removing prepped slot for InstanceID: %d"), InstanceID);
        
        // Remove from container
        if (SlotToRemove->GetParent())
        {
            SlotToRemove->RemoveFromParent();
        }
        
        // Remove from arrays and map
        CreatedPreppedSlots.Remove(SlotToRemove);
        PreppedSlotMap.Remove(InstanceID);
        
        //UE_LOG(LogTemp,Display, TEXT("✅ PUDishCustomizationWidget::RemovePreppedSlot - Removed prepped slot"));
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("ℹ️ PUDishCustomizationWidget::RemovePreppedSlot - No prepped slot found for InstanceID: %d"), InstanceID);
    }
}

void UPUDishCustomizationWidget::SetupPrepSlotNavigation()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPrepSlotNavigation - Setting up navigation for prep slots"));
    
    // Set up navigation grid for prep slots (for controller support)
    // Slots are arranged in a wrap container: 2 rows of 6 slots each (12 total slots)
    const int32 SlotsPerRow = 6;
    
    TArray<UPUIngredientSlot*> PrepSlots;
    for (UPUIngredientSlot* PrepSlot : CreatedIngredientSlots)
    {
        if (PrepSlot && PrepSlot->IsPlanningGatherPlateSlot())
        {
            // Ensure ALL prep slots are focusable, including empty ones
            PrepSlot->SetIsFocusable(true);
            PrepSlots.Add(PrepSlot);
            
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPrepSlotNavigation - Added prep slot: %s (Empty: %s)"), 
                *PrepSlot->GetName(), PrepSlot->IsEmpty() ? TEXT("YES") : TEXT("NO"));
        }
    }
    
    if (PrepSlots.Num() == 0)
    {
        return;
    }
    
    // Set up navigation for each slot
    for (int32 i = 0; i < PrepSlots.Num(); ++i)
    {
        UPUIngredientSlot* CurrentSlot = PrepSlots[i];
        if (!CurrentSlot)
        {
            continue;
        }
        
        int32 Row = i / SlotsPerRow;
        int32 Col = i % SlotsPerRow;
        
        // Calculate neighbor indices
        int32 UpIndex = (Row > 0) ? (i - SlotsPerRow) : INDEX_NONE;
        int32 DownIndex = ((Row + 1) * SlotsPerRow <= PrepSlots.Num()) ? (i + SlotsPerRow) : INDEX_NONE;
        
        // Left navigation: if at start of row, wrap to last slot of previous row
        int32 LeftIndex = INDEX_NONE;
        if (Col > 0)
        {
            // Not at start of row, go to previous slot in same row
            LeftIndex = i - 1;
        }
        else
        {
            // At start of row, wrap to last slot of previous row (if it exists)
            if (Row > 0)
            {
                int32 PrevRowLastIndex = (Row * SlotsPerRow) - 1;
                if (PrevRowLastIndex >= 0 && PrevRowLastIndex < PrepSlots.Num())
                {
                    LeftIndex = PrevRowLastIndex;
                }
            }
        }
        
        // Right navigation: if at end of row, wrap to first slot of next row
        int32 RightIndex = INDEX_NONE;
        if (Col < SlotsPerRow - 1)
        {
            // Not at end of row, go to next slot in same row
            RightIndex = (i + 1 < PrepSlots.Num()) ? (i + 1) : INDEX_NONE;
        }
        else
        {
            // At end of row, wrap to first slot of next row (if it exists)
            int32 NextRowFirstIndex = (Row + 1) * SlotsPerRow;
            if (NextRowFirstIndex < PrepSlots.Num())
            {
                RightIndex = NextRowFirstIndex;
            }
        }
        
        // Get neighbor slots
        UPUIngredientSlot* UpSlot = (UpIndex != INDEX_NONE && UpIndex >= 0 && UpIndex < PrepSlots.Num()) ? PrepSlots[UpIndex] : nullptr;
        UPUIngredientSlot* DownSlot = (DownIndex != INDEX_NONE && DownIndex >= 0 && DownIndex < PrepSlots.Num()) ? PrepSlots[DownIndex] : nullptr;
        UPUIngredientSlot* LeftSlot = (LeftIndex != INDEX_NONE && LeftIndex >= 0 && LeftIndex < PrepSlots.Num()) ? PrepSlots[LeftIndex] : nullptr;
        UPUIngredientSlot* RightSlot = (RightIndex != INDEX_NONE && RightIndex >= 0 && RightIndex < PrepSlots.Num()) ? PrepSlots[RightIndex] : nullptr;
        
        // Set up navigation
        CurrentSlot->SetupNavigation(UpSlot, DownSlot, LeftSlot, RightSlot);
        
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPrepSlotNavigation - Slot %d (%s): Up=%s, Down=%s, Left=%s, Right=%s"), 
            i, *CurrentSlot->GetName(),
            UpSlot ? *UpSlot->GetName() : TEXT("NULL"),
            DownSlot ? *DownSlot->GetName() : TEXT("NULL"),
            LeftSlot ? *LeftSlot->GetName() : TEXT("NULL"),
            RightSlot ? *RightSlot->GetName() : TEXT("NULL"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPrepSlotNavigation - Navigation setup complete for %d prep slots"), PrepSlots.Num());
}

void UPUDishCustomizationWidget::SetupPantrySlotNavigation()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPantrySlotNavigation - Setting up navigation for pantry slots"));
    
    // Set up navigation grid for pantry slots (for controller support)
    // Pantry slots are typically in a horizontal or wrap container
    // For now, we'll set up simple linear navigation (left/right, with wrap)
    const int32 SlotsPerRow = 3;
    
    TArray<UPUIngredientSlot*> PantrySlots;
    for (UPUIngredientSlot* PantrySlot : CreatedPantrySlots)
    {
        if (PantrySlot && PantrySlot->GetLocation() == EPUIngredientSlotLocation::Pantry)
        {
            PantrySlot->SetIsFocusable(!PantrySlot->IsPantryShelfPaddingCell());
            PantrySlots.Add(PantrySlot);
            
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPantrySlotNavigation - Added pantry slot: %s"), 
                *PantrySlot->GetName());
        }
    }
    
    if (PantrySlots.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetupPantrySlotNavigation - No pantry slots found"));
        return;
    }
    
    // Set up navigation for each slot (simple linear navigation with wrap)
    for (int32 i = 0; i < PantrySlots.Num(); ++i)
    {
        UPUIngredientSlot* CurrentSlot = PantrySlots[i];
        if (!CurrentSlot)
        {
            continue;
        }
        
        int32 Row = i / SlotsPerRow;
        int32 Col = i % SlotsPerRow;
        
        // Calculate neighbor indices
        int32 UpIndex = (Row > 0) ? (i - SlotsPerRow) : INDEX_NONE;
        int32 DownIndex = ((Row + 1) * SlotsPerRow <= PantrySlots.Num()) ? (i + SlotsPerRow) : INDEX_NONE;
        
        // Left navigation: if at start of row, wrap to last slot of previous row
        int32 LeftIndex = INDEX_NONE;
        if (Col > 0)
        {
            // Not at start of row, go to previous slot in same row
            LeftIndex = i - 1;
        }
        else
        {
            // At start of row, wrap to last slot of previous row (if it exists)
            if (Row > 0)
            {
                int32 PrevRowLastIndex = (Row * SlotsPerRow) - 1;
                if (PrevRowLastIndex >= 0 && PrevRowLastIndex < PantrySlots.Num())
                {
                    LeftIndex = PrevRowLastIndex;
                }
            }
        }
        
        // Right navigation: if at end of row, wrap to first slot of next row
        int32 RightIndex = INDEX_NONE;
        if (Col < SlotsPerRow - 1)
        {
            // Not at end of row, go to next slot in same row
            RightIndex = (i + 1 < PantrySlots.Num()) ? (i + 1) : INDEX_NONE;
        }
        else
        {
            // At end of row, wrap to first slot of next row (if it exists)
            int32 NextRowFirstIndex = (Row + 1) * SlotsPerRow;
            if (NextRowFirstIndex < PantrySlots.Num())
            {
                RightIndex = NextRowFirstIndex;
            }
        }
        
        // Get neighbor slots
        UPUIngredientSlot* UpSlot = (UpIndex != INDEX_NONE && UpIndex >= 0 && UpIndex < PantrySlots.Num()) ? PantrySlots[UpIndex] : nullptr;
        UPUIngredientSlot* DownSlot = (DownIndex != INDEX_NONE && DownIndex >= 0 && DownIndex < PantrySlots.Num()) ? PantrySlots[DownIndex] : nullptr;
        UPUIngredientSlot* LeftSlot = (LeftIndex != INDEX_NONE && LeftIndex >= 0 && LeftIndex < PantrySlots.Num()) ? PantrySlots[LeftIndex] : nullptr;
        UPUIngredientSlot* RightSlot = (RightIndex != INDEX_NONE && RightIndex >= 0 && RightIndex < PantrySlots.Num()) ? PantrySlots[RightIndex] : nullptr;
        
        // Set up navigation
        CurrentSlot->SetupNavigation(UpSlot, DownSlot, LeftSlot, RightSlot);
        
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPantrySlotNavigation - Slot %d (%s): Up=%s, Down=%s, Left=%s, Right=%s"), 
            i, *CurrentSlot->GetName(),
            UpSlot ? *UpSlot->GetName() : TEXT("NULL"),
            DownSlot ? *DownSlot->GetName() : TEXT("NULL"),
            LeftSlot ? *LeftSlot->GetName() : TEXT("NULL"),
            RightSlot ? *RightSlot->GetName() : TEXT("NULL"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPantrySlotNavigation - Navigation setup complete for %d pantry slots"), PantrySlots.Num());
}

void UPUDishCustomizationWidget::SetupPreppedPantrySlotNavigation()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPreppedPantrySlotNavigation - Setting up navigation for prepped pantry picker slots"));

    if (PreppedPantryContainer.IsValid())
    {
        for (UWidget* Ancestor = PreppedPantryContainer.Get(); Ancestor; Ancestor = Ancestor->GetParent())
        {
            if (UScrollBox* ScrollBox = Cast<UScrollBox>(Ancestor))
            {
                ScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
                break;
            }
        }
    }

    const int32 SlotsPerRow = 3;

    TArray<UPUIngredientSlot*> PantrySlots;
    for (UPUIngredientSlot* PantrySlot : CreatedPreppedPantrySlots)
    {
        if (PantrySlot && PantrySlot->GetLocation() == EPUIngredientSlotLocation::Prepped)
        {
            PantrySlot->SetIsFocusable(!PantrySlot->IsPantryShelfPaddingCell());
            PantrySlots.Add(PantrySlot);
        }
    }

    if (PantrySlots.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetupPreppedPantrySlotNavigation - No prepped pantry slots found"));
        return;
    }

    for (int32 i = 0; i < PantrySlots.Num(); ++i)
    {
        UPUIngredientSlot* CurrentSlot = PantrySlots[i];
        if (!CurrentSlot)
        {
            continue;
        }

        int32 Row = i / SlotsPerRow;
        int32 Col = i % SlotsPerRow;

        int32 UpIndex = (Row > 0) ? (i - SlotsPerRow) : INDEX_NONE;
        int32 DownIndex = ((Row + 1) * SlotsPerRow <= PantrySlots.Num()) ? (i + SlotsPerRow) : INDEX_NONE;

        int32 LeftIndex = INDEX_NONE;
        if (Col > 0)
        {
            LeftIndex = i - 1;
        }
        else if (Row > 0)
        {
            int32 PrevRowLastIndex = (Row * SlotsPerRow) - 1;
            if (PrevRowLastIndex >= 0 && PrevRowLastIndex < PantrySlots.Num())
            {
                LeftIndex = PrevRowLastIndex;
            }
        }

        int32 RightIndex = INDEX_NONE;
        if (Col < SlotsPerRow - 1)
        {
            RightIndex = (i + 1 < PantrySlots.Num()) ? (i + 1) : INDEX_NONE;
        }
        else
        {
            int32 NextRowFirstIndex = (Row + 1) * SlotsPerRow;
            if (NextRowFirstIndex < PantrySlots.Num())
            {
                RightIndex = NextRowFirstIndex;
            }
        }

        UPUIngredientSlot* UpSlot = (UpIndex != INDEX_NONE && UpIndex >= 0 && UpIndex < PantrySlots.Num()) ? PantrySlots[UpIndex] : nullptr;
        UPUIngredientSlot* DownSlot = (DownIndex != INDEX_NONE && DownIndex >= 0 && DownIndex < PantrySlots.Num()) ? PantrySlots[DownIndex] : nullptr;
        UPUIngredientSlot* LeftSlot = (LeftIndex != INDEX_NONE && LeftIndex >= 0 && LeftIndex < PantrySlots.Num()) ? PantrySlots[LeftIndex] : nullptr;
        UPUIngredientSlot* RightSlot = (RightIndex != INDEX_NONE && RightIndex >= 0 && RightIndex < PantrySlots.Num()) ? PantrySlots[RightIndex] : nullptr;

        CurrentSlot->SetupNavigation(UpSlot, DownSlot, LeftSlot, RightSlot);
    }

    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupPreppedPantrySlotNavigation - Navigation setup complete for %d slots"), PantrySlots.Num());
}

bool UPUDishCustomizationWidget::IsDialogueVisible() const
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (ACharacter* PC = UGameplayStatics::GetPlayerCharacter(World, 0))
    {
        if (AProjectUmeowmiCharacter* Char = Cast<AProjectUmeowmiCharacter>(PC))
        {
            if (UPUDialogueBox* DialogueBox = Char->GetDialogueBox())
            {
                return DialogueBox->GetVisibility() == ESlateVisibility::Visible;
            }
        }
    }
    return false;
}

void UPUDishCustomizationWidget::SetInitialFocusForPantry()
{
    if (IsDialogueVisible())
    {
        return; // Dialogue has priority - don't steal focus
    }
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - Setting initial focus for pantry"));
    
    // Ensure this widget can receive focus first
    SetIsFocusable(true);
    
    // Find the first pantry slot and set focus to it
    for (UPUIngredientSlot* PantrySlot : CreatedPantrySlots)
    {
        if (PantrySlot && PantrySlot->GetLocation() == EPUIngredientSlotLocation::Pantry)
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - Found pantry slot: %s (Focusable: %s)"), 
                *PantrySlot->GetName(), PantrySlot->IsFocusable() ? TEXT("YES") : TEXT("NO"));
            
            // Ensure slot is focusable
            PantrySlot->SetIsFocusable(true);
            
            // Use a delayed timer to set focus after the widget is fully visible
            FTimerHandle FocusTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(FocusTimerHandle, [WeakSlot = TWeakObjectPtr<UPUIngredientSlot>(PantrySlot), this]()
            {
                if (WeakSlot.IsValid() && !IsDialogueVisible())
                {
                    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - Setting focus to pantry slot: %s"), 
                        *WeakSlot->GetName());
                    
                    // Set focus using both methods for robustness
                    WeakSlot->SetIsFocusable(true);
                    WeakSlot->SetKeyboardFocus();
                    FSlateApplication::Get().SetUserFocus(0, WeakSlot->TakeWidget());
                    
                    // Manually trigger visual feedback
                    WeakSlot->ShowFocusVisuals();
                    ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                    
                    // Retry if focus wasn't set
                    if (!WeakSlot->HasKeyboardFocus())
                    {
                        FTimerHandle RetryTimerHandle;
                        GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, [WeakSlot, this]()
                        {
                            if (WeakSlot.IsValid())
                            {
                                WeakSlot->SetKeyboardFocus();
                                WeakSlot->ShowFocusVisuals();
                                ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - Retry: Focus set to %s (HasFocus: %s)"), 
                                    *WeakSlot->GetName(), WeakSlot->HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
                            }
                        }, 0.1f, false);
                    }
                }
            }, 0.2f, false);
            
            return;
        }
    }

    for (UPUIngredientSlot* PickSlot : CreatedPreppedPantrySlots)
    {
        if (PickSlot && PickSlot->IsPreppedPantryPickerSlot() && PickSlot->GetIngredientInstance().IngredientData.IngredientTag.IsValid())
        {
            PickSlot->SetIsFocusable(true);

            FTimerHandle FocusTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(FocusTimerHandle, [WeakSlot = TWeakObjectPtr<UPUIngredientSlot>(PickSlot), this]()
            {
                if (WeakSlot.IsValid() && !IsDialogueVisible())
                {
                    WeakSlot->SetIsFocusable(true);
                    WeakSlot->SetKeyboardFocus();
                    FSlateApplication::Get().SetUserFocus(0, WeakSlot->TakeWidget());
                    WeakSlot->ShowFocusVisuals();
                    ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());

                    if (!WeakSlot->HasKeyboardFocus())
                    {
                        FTimerHandle RetryTimerHandle;
                        GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, [WeakSlot, this]()
                        {
                            if (WeakSlot.IsValid())
                            {
                                WeakSlot->SetKeyboardFocus();
                                WeakSlot->ShowFocusVisuals();
                                ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - Retry: Focus set to %s (HasFocus: %s)"),
                                    *WeakSlot->GetName(), WeakSlot->HasKeyboardFocus() ? TEXT("YES") : TEXT("NO"));
                            }
                        }, 0.1f, false);
                    }
                }
            }, 0.2f, false);

            return;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPantry - No pantry slots found"));
}

void UPUDishCustomizationWidget::SetupCookingSlotNavigation()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupCookingSlotNavigation - Setting up navigation for cooking stage slots"));
    
    // Enable scroll-into-view when focus changes so slots come into view when navigating in the scrollbox
    auto EnableScrollWhenFocusChangesForContainer = [this](UPanelWidget* Container)
    {
        if (!Container) return;
        for (UWidget* Ancestor = Container; Ancestor; Ancestor = Ancestor->GetParent())
        {
            if (UScrollBox* ScrollBox = Cast<UScrollBox>(Ancestor))
            {
                ScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupCookingSlotNavigation - Enabled ScrollWhenFocusChanges on ScrollBox: %s"), *ScrollBox->GetName());
                break;
            }
        }
    };
    
    // Cooking stage slots: same as prep stage - either from CreateSlots (CreatedIngredientSlots with Prepped/ActiveIngredientArea)
    // or from CreateOrUpdatePreppedSlot (CreatedPreppedSlots)
    TArray<UPUIngredientSlot*> CookingSlots;
    for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
    {
        if (IngredientSlot &&
            (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::Prepped ||
             (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::ActiveIngredientArea &&
              StageType == EDishCustomizationStageType::Cooking)))
        {
            IngredientSlot->SetIsFocusable(true);
            CookingSlots.Add(IngredientSlot);
        }
    }
    if (CookingSlots.Num() == 0)
    {
        for (UPUIngredientSlot* IngredientSlot : CreatedPreppedSlots)
        {
            if (IngredientSlot)
            {
                IngredientSlot->SetIsFocusable(true);
                CookingSlots.Add(IngredientSlot);
            }
        }
    }
    
    if (CookingSlots.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetupCookingSlotNavigation - No cooking stage slots found"));
        return;
    }
    
    // Enable scroll-into-view when navigating: find ScrollBox ancestor of the slot container
    if (PreppedIngredientContainer.IsValid())
    {
        EnableScrollWhenFocusChangesForContainer(PreppedIngredientContainer.Get());
    }
    else if (CookingSlots.Num() > 0 && CookingSlots[0])
    {
        EnableScrollWhenFocusChangesForContainer(CookingSlots[0]->GetParent());
    }
    
    // Linear navigation for slots in scrollbox: Up = previous, Down = next
    for (int32 i = 0; i < CookingSlots.Num(); ++i)
    {
        UPUIngredientSlot* CurrentSlot = CookingSlots[i];
        if (!CurrentSlot) continue;
        
        UPUIngredientSlot* UpSlot = (i > 0) ? CookingSlots[i - 1] : nullptr;
        UPUIngredientSlot* DownSlot = (i < CookingSlots.Num() - 1) ? CookingSlots[i + 1] : nullptr;
        
        CurrentSlot->SetupNavigation(UpSlot, DownSlot, nullptr, nullptr);
        
        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupCookingSlotNavigation - Slot %d (%s): Up=%s, Down=%s"), 
            i, *CurrentSlot->GetName(),
            UpSlot ? *UpSlot->GetName() : TEXT("NULL"),
            DownSlot ? *DownSlot->GetName() : TEXT("NULL"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetupCookingSlotNavigation - Navigation setup complete for %d cooking slots"), CookingSlots.Num());
}

void UPUDishCustomizationWidget::SetInitialFocusForCookingStage()
{
    if (IsDialogueVisible())
    {
        return; // Dialogue has priority - don't steal focus
    }
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - Setting initial focus for cooking stage"));
    
    SetIsFocusable(true);
    
    // Find first cooking slot - check CreatedIngredientSlots (Prepped/ActiveIngredientArea) first, then CreatedPreppedSlots
    UPUIngredientSlot* FirstSlot = nullptr;
    for (UPUIngredientSlot* IngredientSlot : CreatedIngredientSlots)
    {
        if (IngredientSlot &&
            (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::Prepped ||
             (IngredientSlot->GetLocation() == EPUIngredientSlotLocation::ActiveIngredientArea &&
              StageType == EDishCustomizationStageType::Cooking)))
        {
            FirstSlot = IngredientSlot;
            break;
        }
    }
    if (!FirstSlot && CreatedPreppedSlots.Num() > 0)
    {
        FirstSlot = CreatedPreppedSlots[0];
    }
    
    if (!FirstSlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - No cooking stage slots found"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - Found cooking slot: %s (Focusable: %s)"), 
        *FirstSlot->GetName(), FirstSlot->IsFocusable() ? TEXT("YES") : TEXT("NO"));
    
    FirstSlot->SetIsFocusable(true);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialFocusTimerHandle);
        TWeakObjectPtr<UPUIngredientSlot> WeakSlot = FirstSlot;
        
        World->GetTimerManager().SetTimer(InitialFocusTimerHandle, [WeakSlot, this]()
        {
            if (WeakSlot.IsValid() && WeakSlot->IsValidLowLevel() && !IsDialogueVisible())
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - Delayed focus set to slot: %s"), *WeakSlot->GetName());
                
                WeakSlot->SetIsFocusable(true);
                WeakSlot->SetKeyboardFocus();
                
                if (APlayerController* PC = GetOwningPlayer())
                {
                    if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
                    {
                        FSlateApplication::Get().SetUserFocus(LocalPlayer->GetControllerId(), WeakSlot->TakeWidget(), EFocusCause::SetDirectly);
                        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - Set user focus via Slate"));
                    }
                }
                
                WeakSlot->ShowFocusVisuals();
                ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                
                if (!WeakSlot->HasKeyboardFocus() && GetWorld())
                {
                    FTimerHandle RetryTimer;
                    GetWorld()->GetTimerManager().SetTimer(RetryTimer, [WeakSlot, this]()
                    {
                        if (WeakSlot.IsValid())
                        {
                            WeakSlot->SetIsFocusable(true);
                            WeakSlot->SetKeyboardFocus();
                            WeakSlot->ShowFocusVisuals();
                            ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForCookingStage - Retry: Focus set to slot: %s"), *WeakSlot->GetName());
                        }
                    }, 0.2f, false);
                }
            }
                }, 0.3f, false); // Slightly longer than prep (0.15f) to allow cooking stage entrance animations
    }
    else
    {
        FirstSlot->SetKeyboardFocus();
        ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
    }
}

void UPUDishCustomizationWidget::SetInitialFocusForPrepStage()
{
    if (IsDialogueVisible())
    {
        return; // Dialogue has priority - don't steal focus
    }
    UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Setting initial focus for prep stage"));
    
    // Ensure this widget can receive focus first
    SetIsFocusable(true);
    
    // Find the first prep slot (including empty ones) and set focus to it
    for (UPUIngredientSlot* PrepSlot : CreatedIngredientSlots)
    {
        if (PrepSlot && PrepSlot->IsPlanningGatherPlateSlot())
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Found prep slot: %s (Empty: %s, Focusable: %s)"), 
                *PrepSlot->GetName(), 
                PrepSlot->IsEmpty() ? TEXT("YES") : TEXT("NO"),
                PrepSlot->IsFocusable() ? TEXT("YES") : TEXT("NO"));
            
            // Ensure the slot is focusable (should already be set in NativeConstruct, but double-check)
            PrepSlot->SetIsFocusable(true);
            
            // Use a small delay to ensure the widget is fully constructed and visible
            // This is necessary because SetKeyboardFocus might fail if called too early
            if (UWorld* World = GetWorld())
            {
                // Clear any existing timer
                World->GetTimerManager().ClearTimer(InitialFocusTimerHandle);
                
                // Store weak pointer to avoid issues if slot is destroyed
                TWeakObjectPtr<UPUIngredientSlot> WeakSlot = PrepSlot;
                
                World->GetTimerManager().SetTimer(InitialFocusTimerHandle, [WeakSlot, this]()
                {
                    if (WeakSlot.IsValid() && WeakSlot->IsValidLowLevel() && !IsDialogueVisible())
                    {
                        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Delayed focus set to slot: %s"), *WeakSlot->GetName());
                        
                        // Ensure the slot is still focusable
                        WeakSlot->SetIsFocusable(true);
                        
                        // Set keyboard focus - this should trigger NativeOnAddedToFocusPath which shows the outline
                        WeakSlot->SetKeyboardFocus();
                        
                        // Also try setting user focus (for gamepad)
                        APlayerController* PC = GetOwningPlayer();
                        if (PC)
                        {
                            if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
                            {
                                FSlateApplication::Get().SetUserFocus(LocalPlayer->GetControllerId(), WeakSlot->TakeWidget(), EFocusCause::SetDirectly);
                                UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Also set user focus via Slate"));
                            }
                        }
                        
                        // Manually trigger focus visuals to ensure outline is shown
                        // This is a backup in case NativeOnAddedToFocusPath doesn't fire immediately
                        WeakSlot->ShowFocusVisuals();
                        ReassertVirtualCursorAfterSlotFocus(this, PC);
                        
                        // Verify focus was set
                        if (WeakSlot->HasKeyboardFocus())
                        {
                            UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Focus successfully set! Outline should be visible now."));
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Focus was NOT set (widget may not be focusable or visible)"));
                            // Try one more time after another small delay
                            if (UWorld* RetryWorld = GetWorld())
                            {
                                FTimerHandle RetryTimer;
                                RetryWorld->GetTimerManager().SetTimer(RetryTimer, [WeakSlot, this]()
                                {
                                    if (WeakSlot.IsValid())
                                    {
                                        WeakSlot->SetIsFocusable(true);
                                        WeakSlot->SetKeyboardFocus();
                                        ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
                                        UE_LOG(LogTemp, Log, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - Retry: Focus set to slot: %s"), *WeakSlot->GetName());
                                    }
                                }, 0.2f, false);
                            }
                        }
                    }
                }, 0.15f, false); // 0.15 second delay
            }
            else
            {
                // Fallback: try immediately if no world available
                UE_LOG(LogTemp, Warning, TEXT("🎮 UPUDishCustomizationWidget::SetInitialFocusForPrepStage - No world available, setting focus immediately"));
                PrepSlot->SetKeyboardFocus();
                ReassertVirtualCursorAfterSlotFocus(this, GetOwningPlayer());
            }
            
            break; // Only focus the first slot
        }
    }
}
