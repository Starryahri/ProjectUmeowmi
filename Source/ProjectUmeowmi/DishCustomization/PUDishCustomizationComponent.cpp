#include "PUDishCustomizationComponent.h"
#include "Math/Box.h"
#include "PUDishPreviewComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Engine/EngineBaseTypes.h"
#include "EngineUtils.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "../UI/PUScorecardWidget.h"
#include "../UI/PUVirtualCursorUserWidget.h"
#include "../UI/PUPlatingWidget.h"
#include "../UI/PUIngredientSlot.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PUIngredientMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "Layout/WidgetPath.h"
#include "Input/Events.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    // Enables logging of every ingredient tag when dish data is updated.
    constexpr bool bPU_LogDishDataIngredientTags = false;

    // Enables logging for ingredient drag (click detection, trace hits, position updates).
    constexpr bool bPU_LogIngredientDrag = true;

    // Enables logging for movement restoration when exiting customization (to debug "can look but not move").
    constexpr bool bPU_LogMovementRestore = true;

    /** Right-stick virtual cursor: StartCustomization, HandleControllerMouse (throttled), deferred sync. Set false after debugging. */
    constexpr bool bPU_LogVirtualCursor = true;

    /** Synthetic Interact → Slate LMB down/up over virtual cursor (ingredient slots). Set false after debugging. */
    constexpr bool bPU_LogVirtualCursorClick = true;

    void LogMovementState(APlayerController* PC, const TCHAR* Context)
    {
        if (!bPU_LogMovementRestore || !PC) return;
        bool bIgnoreMove = PC->IsMoveInputIgnored();
        bool bIgnoreLook = PC->IsLookInputIgnored();
        EMovementMode MoveMode = MOVE_None;
        if (APawn* Pawn = PC->GetPawn())
        {
            if (ACharacter* C = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* M = C->GetCharacterMovement())
                    MoveMode = M->MovementMode;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] %s - IgnoreMove=%d IgnoreLook=%d MovementMode=%d"), Context, bIgnoreMove, bIgnoreLook, (int32)MoveMode);
    }

    /**
     * Match FSceneViewport::SetMouse: norm = pixel / GetSizeXY(), local = norm * CachedGeometry.GetLocalSize(), then LocalToAbsolute.
     * ViewportToVirtualDesktopPixel uses norm * SizeX instead of GetLocalSize — when those differ (editor viewport, DPI, mid-resize), hover/clamp no longer match the real scene viewport widget.
     */
    bool VirtualViewportPixelsToSlateCursorAbsolute(FSceneViewport* SceneViewport, const FVector2D& VirtualViewportPixels, int32 VSX, int32 VSY, FVector2D& OutAbsolute)
    {
        if (!SceneViewport || VSX <= 0 || VSY <= 0)
        {
            return false;
        }
        const FVector2D Norm(
            VirtualViewportPixels.X / static_cast<float>(VSX),
            VirtualViewportPixels.Y / static_cast<float>(VSY));
        const FGeometry& Geo = SceneViewport->GetCachedGeometry();
        const FVector2D LocalSize = Geo.GetLocalSize();
        if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
        {
            return false;
        }
        const FVector2D LocalInViewport = Norm * LocalSize;
        OutAbsolute = Geo.LocalToAbsolute(LocalInViewport);
        return true;
    }

    /** ProcessMouseButtonDownEvent re-broadcasts OnApplicationMousePreInputButtonDownListener; guard OnPreInputMouseButtonDown against re-entering HandleMouseClick. */
    struct FPUScopedSyntheticSlateMouseDispatch
    {
        bool& bFlag;
        explicit FPUScopedSyntheticSlateMouseDispatch(bool& InFlag) : bFlag(InFlag) { bFlag = true; }
        ~FPUScopedSyntheticSlateMouseDispatch() { bFlag = false; }
    };

    /** Match FAnalogCursor: move Slate's pointer + ProcessMouseMoveEvent so UMG hover (e.g. ingredient slots) tracks the virtual position. */
    void ApplyVirtualCursorSlateHover(const FVector2D& VirtualViewportPixels, APlayerController* PC, int32 VSX, int32 VSY)
    {
        if (!PC || !FSlateApplication::IsInitialized() || VSX <= 0 || VSY <= 0)
        {
            return;
        }
        ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
        if (!LocalPlayer || !LocalPlayer->ViewportClient)
        {
            return;
        }
        FSceneViewport* SceneViewport = LocalPlayer->ViewportClient->GetGameViewport();
        if (!SceneViewport)
        {
            return;
        }
        FVector2D NewAbs;
        if (!VirtualViewportPixelsToSlateCursorAbsolute(SceneViewport, VirtualViewportPixels, VSX, VSY, NewAbs))
        {
            return;
        }

        FSlateApplication& SlateApp = FSlateApplication::Get();
        TSharedPtr<FSlateUser> SlateUser = SlateApp.GetUser(LocalPlayer->GetControllerId());
        if (!SlateUser.IsValid())
        {
            SlateUser = SlateApp.GetCursorUser();
        }
        if (!SlateUser.IsValid())
        {
            return;
        }

        const FVector2D OldAbs = SlateUser->GetCursorPosition();
        const FVector2D NewAbsRounded = NewAbs.RoundToVector();
        SlateUser->SetCursorPosition(static_cast<int32>(NewAbsRounded.X), static_cast<int32>(NewAbsRounded.Y));
        const FVector2D UpdatedAbs = SlateUser->GetCursorPosition();

        const bool bIsPrimaryUser = FSlateApplication::CursorUserIndex == SlateUser->GetUserIndex();
        const FPointerEvent MouseEvent(
            SlateUser->GetUserIndex(),
            FSlateApplication::CursorPointerIndex,
            UpdatedAbs,
            OldAbs,
            bIsPrimaryUser ? SlateApp.GetPressedMouseButtons() : FTouchKeySet::EmptySet,
            EKeys::Invalid,
            0.f,
            bIsPrimaryUser ? SlateApp.GetModifierKeys() : FModifierKeysState());
        SlateApp.ProcessMouseMoveEvent(MouseEvent);
    }

    /** True if the hit path includes UMG (SObjectWidget) — synthetic click should go to Slate, not world trace only. */
    bool WidgetPathContainsSObjectWidget(const FWidgetPath& Path)
    {
        if (!Path.IsValid())
        {
            return false;
        }
        for (int32 i = Path.Widgets.Num() - 1; i >= 0; --i)
        {
            const FString TypeStr = Path.Widgets[i].Widget->GetTypeAsString();
            if (TypeStr.Contains(TEXT("SObjectWidget")))
            {
                return true;
            }
        }
        return false;
    }
}

UPUDishCustomizationComponent::UPUDishCustomizationComponent()
{
    PrimaryComponentTick.bCanEverTick = true; // Virtual cursor sync; drag updates while dragging
    QuantityIncreaseBindingHandle = 0;
    QuantityDecreaseBindingHandle = 0;
}

void UPUDishCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPUDishCustomizationComponent::BeginDestroy()
{
#if WITH_EDITOR
    // Unregister Slate delegate before destruction to prevent shutdown crash.
    // EndCustomization() may not be called when editor closes or level unloads.
    if (PreInputMouseDownHandle.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().Remove(PreInputMouseDownHandle);
        PreInputMouseDownHandle.Reset();
    }
#endif
    Super::BeginDestroy();
}

bool UPUDishCustomizationComponent::ShouldSuppressHardwareMouseCursor() const
{
    return bVirtualCursorInitialized && VirtualCursorWidgetClass != nullptr;
}

void UPUDishCustomizationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // PIE / selected viewport / window resize: extents can change without right-stick input — re-clamp and sync.
    // Also re-stomp OS cursor each frame: HUD / dialogue / focus paths often call UsePlatformCursorForCursorUser(true).
    if (bVirtualCursorInitialized && CurrentCharacter && VirtualCursorWidgetClass)
    {
        if (APlayerController* VPC = Cast<APlayerController>(CurrentCharacter->GetController()))
        {
            ApplyVirtualCursorHardwareCursorLock(VPC);
            int32 nw = 0;
            int32 nh = 0;
            if (TryGetVirtualCursorViewportPixelExtents(VPC, nw, nh))
            {
                if (nw != CachedVirtualCursorViewportExtentsX || nh != CachedVirtualCursorViewportExtentsY)
                {
                    ApplyVirtualCursorVisual(VPC);
                }
            }
        }
    }

    // Update mouse dragging if active
    if (bIsDragging)
    {
        UpdateMouseDrag();
    }
}

void UPUDishCustomizationComponent::StartCustomization(AProjectUmeowmiCharacter* Character)
{
    //UE_LOG(LogTemp,Display, TEXT("🚀 UPUDishCustomizationComponent::StartCustomization - STARTING CUSTOMIZATION"));
    
    if (!Character)
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Character"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Character valid: %s"), *Character->GetName());
    CurrentCharacter = Character;
    if (UWorld* NotifyWorld = GetWorld())
    {
        if (UPUProjectUmeowmiGameInstance* GI = NotifyWorld->GetGameInstance<UPUProjectUmeowmiGameInstance>())
        {
            GI->NotifyPlayerQuestObjectiveOverlayVisibility();
        }
    }
    if (AProjectUmeowmiCharacter* PUChar = Cast<AProjectUmeowmiCharacter>(Character))
    {
        PUChar->SanitizeScoringStackOrphansInViewport();
    }
    bWasMouseDown = false;  // Reset for clean state when entering customization
    bVirtualClickConsumedBySlateUI = false;
    bLastVirtualCursorDesktopValid = false;
    CachedVirtualCursorViewportExtentsX = 0;
    CachedVirtualCursorViewportExtentsY = 0;

    ResetCustomizationPipelineProgress();

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Player Controller"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Player Controller valid: %s"), *PlayerController->GetName());

    // Set input mode first to ensure the input system is ready
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(nullptr);
    // With an on-screen pointer, Slate/UI capture must not pop the OS cursor (see ApplyVirtualCursorHardwareCursorLock + next-frame reschedule on click).
    InputMode.SetHideCursorDuringCapture(VirtualCursorWidgetClass != nullptr);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);

    // Project DefaultInput often uses CapturePermanently_IncludingInitialMouseDown — the viewport won't track a controller-moved cursor until the user clicks. NoCapture fixes virtual cursor (right stick → SetMouseLocation).
    SavedViewportMouseCaptureModeForCustomization = UGameplayStatics::GetViewportMouseCaptureMode(PlayerController);
    bHasSavedViewportMouseCaptureForCustomization = true;
    UGameplayStatics::SetViewportMouseCaptureMode(PlayerController, EMouseCaptureMode::NoCapture);

    // Enable click/over for world traces at virtual screen position (UMG widget draws the pointer; OS cursor optional).
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;

    // Virtual cursor: viewport position is owned only by the right stick + this component (never GetMousePosition for movement).
    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    TryGetVirtualCursorViewportPixelExtents(PlayerController, ViewportSizeX, ViewportSizeY);
    if (ViewportSizeX > 0 && ViewportSizeY > 0)
    {
        VirtualCursorViewport.X = static_cast<float>(ViewportSizeX) * 0.5f;
        VirtualCursorViewport.Y = static_cast<float>(ViewportSizeY) * 0.5f;
        bVirtualCursorInitialized = true;

        if (VirtualCursorWidgetClass)
        {
            if (IsValid(VirtualCursorWidgetInstance))
            {
                VirtualCursorWidgetInstance->RemoveFromParent();
                VirtualCursorWidgetInstance = nullptr;
            }
            VirtualCursorWidgetInstance = CreateWidget<UUserWidget>(PlayerController, VirtualCursorWidgetClass);
            if (VirtualCursorWidgetInstance)
            {
                VirtualCursorWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
                const int32 CursorZ = FMath::Max(VirtualCursorZOrder, PUDishVirtualCursorViewportZOrder);
                VirtualCursorWidgetInstance->AddToViewport(CursorZ);
            }
            PlayerController->SetShowMouseCursor(false);
            PlayerController->CurrentMouseCursor = EMouseCursor::None;
        }
        else
        {
            PlayerController->SetShowMouseCursor(true);
            PlayerController->CurrentMouseCursor = EMouseCursor::Default;
        }

        ApplyVirtualCursorVisual(PlayerController);
        if (VirtualCursorWidgetClass)
        {
            FInputModeGameAndUI InputModeAfterCursor;
            InputModeAfterCursor.SetWidgetToFocus(nullptr);
            InputModeAfterCursor.SetHideCursorDuringCapture(true);
            InputModeAfterCursor.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeAfterCursor);
        }
        if (bPU_LogVirtualCursor)
        {
            UE_LOG(LogTemp, Log, TEXT("[VirtualCursor] StartCustomization: viewport %dx%d, initial (%.1f, %.1f) widget=%s"),
                ViewportSizeX, ViewportSizeY, VirtualCursorViewport.X, VirtualCursorViewport.Y,
                VirtualCursorWidgetClass ? *VirtualCursorWidgetClass->GetName() : TEXT("(none)"));
        }
    }
    else if (bPU_LogVirtualCursor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursor] StartCustomization: viewport size invalid (%d x %d), virtual cursor not initialized"), ViewportSizeX, ViewportSizeY);
    }

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Input mode set, viewport size: %dx%d"), ViewportSizeX, ViewportSizeY);

    // Layer customization IMC above DefaultMappingContext (priority 0) — do not remove default so journal can stack (e.g. Default 0, Dish 1, Journal 2).
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
        if (CustomizationMappingContext)
        {
            Subsystem->AddMappingContext(CustomizationMappingContext, CustomizationMappingContextPriority);
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Subsystem"));
    }

    // Setup input handling for exit action and controller mouse
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Found Enhanced Input Component"));
        
        // First unbind any existing bindings to ensure clean state
        if (ControllerMouseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing controller mouse binding"));
        }
        
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing exit action binding"));
        }

        // Now set up new bindings
        if (ExitCustomizationAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding exit action: %s"), *ExitCustomizationAction->GetName());
            ExitActionBindingHandle = EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput).GetHandle();
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Exit action bound with handle: %d"), ExitActionBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ExitCustomizationAction set"));
        }

        if (ControllerMouseAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding controller mouse action: %s"), *ControllerMouseAction->GetName());
            // Bind to both Ongoing and Triggered events to ensure we catch all input
            ControllerMouseBindingHandle = EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Ongoing, this, &UPUDishCustomizationComponent::HandleControllerMouse).GetHandle();
            EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleControllerMouse);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Controller mouse action bound with handle: %d"), ControllerMouseBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ControllerMouseAction set"));
        }

        if (MouseClickAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding mouse click action: %s"), *MouseClickAction->GetName());
            MouseClickBindingHandle = EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Started, this, &UPUDishCustomizationComponent::HandleMouseClick).GetHandle();
            EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Completed, this, &UPUDishCustomizationComponent::HandleMouseRelease);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Mouse click action bound with handle: %d"), MouseClickBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No MouseClickAction set"));
        }

#if WITH_EDITOR
        // Slate pre-input listener: fires BEFORE widgets consume the click (bypasses widget blocking)
        PreInputMouseDownHandle = FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().AddUObject(this, &UPUDishCustomizationComponent::OnPreInputMouseButtonDown);
#endif // WITH_EDITOR

        // Bind stage navigation actions
        if (NextStageAction)
        {
            NextStageBindingHandle = EnhancedInputComponent->BindAction(NextStageAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleNextStage).GetHandle();
        }

        if (PreviousStageAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("🔙 Binding PreviousStageAction: %s"), *PreviousStageAction->GetName());
            PreviousStageBindingHandle = EnhancedInputComponent->BindAction(PreviousStageAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandlePreviousStage).GetHandle();
            UE_LOG(LogTemp, Warning, TEXT("🔙 PreviousStageBindingHandle: %d"), PreviousStageBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🔙 PreviousStageAction is NULL!"));
        }

        if (QuantityIncreaseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(QuantityIncreaseBindingHandle);
            QuantityIncreaseBindingHandle = EnhancedInputComponent->BindAction(QuantityIncreaseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleQuantityIncrease).GetHandle();
        }
        if (QuantityDecreaseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(QuantityDecreaseBindingHandle);
            QuantityDecreaseBindingHandle = EnhancedInputComponent->BindAction(QuantityDecreaseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleQuantityDecrease).GetHandle();
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Component"));
    }

    // Create and show the customization widget
    //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to create widget"));
    //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - CustomizationWidgetClass: %s"), 
    //    CustomizationWidgetClass ? *CustomizationWidgetClass->GetName() : TEXT("NULL"));
    
    if (CustomizationWidgetClass)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Creating widget with class: %s"), *CustomizationWidgetClass->GetName());
        
        UWorld* World = GetWorld();
        if (!World)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No World available for widget creation"));
            return;
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - World valid: %s"), *World->GetName());
        
        CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
        if (CustomizationWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget created successfully: %s"), *CustomizationWidget->GetName());
            
            // Try to cast to our custom widget class and set up the connection
            if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is PUDishCustomizationWidget, connecting to component"));
                DishWidget->SetCustomizationComponent(this);
                
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Component connection completed"));
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is not a PUDishCustomizationWidget, manual connection may be needed"));
                //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidget->GetClass()->GetName());
            }
            
            // Pass the initial dish data to the widget during creation
            // The widget can access this data in its construction script or BeginPlay
            //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to add widget to viewport"));
            
            // Add to viewport with a lower Z-Order so it doesn't override the recipe book widget
            CustomizationWidget->AddToViewport(PUDishCustomizationViewportZOrder);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget added to viewport successfully with Z-Order -100"));
            
            // Check if widget is visible
            if (CustomizationWidget->IsVisible())
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is visible"));
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is NOT visible"));
            }
            
            // Broadcast the initial dish data now that the widget is created and subscribed
            if (CurrentDishData.DishTag.IsValid())
            {
                //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Broadcasting initial dish data to newly created widget"));
                //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Dish data: %s"), *CurrentDishData.DisplayName.ToString());
                BroadcastInitialDishData(CurrentDishData);
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No valid dish data to broadcast"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to create Customization UI Widget"));
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidgetClass->GetName());
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No CustomizationWidgetClass set"));
    }

    // Spring-arm zoom-framing on enter is disabled — customization is UI-first; character camera stays as-is.

    // Re-apply capture after layout; re-sync virtual cursor once FSceneViewport CachedGeometry has ticked (no SetFocusToGameViewport — keeps Game+UI widget focus).
    if (UWorld* WorldForCapture = GetWorld())
    {
        WorldForCapture->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UPUDishCustomizationComponent::OnCustomizationViewportDeferredSetup));
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎉 UPUDishCustomizationComponent::StartCustomization - CUSTOMIZATION STARTED SUCCESSFULLY"));
}

void UPUDishCustomizationComponent::EndCustomization()
{
    if (bInEndCustomization)
    {
        return;
    }

    UWorld* World = GetWorld();
    APlayerController* WorldPC = World ? World->GetFirstPlayerController() : nullptr;

    if (bPU_LogMovementRestore)
        UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] EndCustomization ENTRY - Component=%s CurrentCharacter=%s WorldPC=%s"), *GetName(), CurrentCharacter ? *CurrentCharacter->GetName() : TEXT("NULL"), WorldPC ? *WorldPC->GetName() : TEXT("NULL"));

    if (!CurrentCharacter)
    {
        bVirtualCursorInitialized = false;
        if (bPU_LogMovementRestore)
            UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] EndCustomization EARLY RETURN (no CurrentCharacter) - restoring via WorldPC"));
        // Still restore movement so player can move (e.g. component ref mismatch or already cleared)
        if (WorldPC)
        {
            if (bHasSavedViewportMouseCaptureForCustomization)
            {
                UGameplayStatics::SetViewportMouseCaptureMode(WorldPC, SavedViewportMouseCaptureModeForCustomization);
                bHasSavedViewportMouseCaptureForCustomization = false;
            }
            LogMovementState(WorldPC, TEXT("EARLY before restore"));
            WorldPC->ResetIgnoreMoveInput();
            WorldPC->ResetIgnoreLookInput();
            // Do not call SetAllUserFocusToGameViewport here — FindPathToWidget can recurse until stack overflow during teardown.
            if (APawn* Pawn = WorldPC->GetPawn())
            {
                if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
                {
                    if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                    {
                        MovementComp->SetMovementMode(MOVE_Walking);
                    }
                }
            }
            LogMovementState(WorldPC, TEXT("EARLY after restore"));
        }
        return;
    }

    bInEndCustomization = true;
    struct FScopedEndCustomizationGuard
    {
        bool& Flag;
        explicit FScopedEndCustomizationGuard(bool& InFlag) : Flag(InFlag) {}
        ~FScopedEndCustomizationGuard() { Flag = false; }
    } EndCustomizationGuard(bInEndCustomization);

    // End plating stage if we're in plating mode
    if (bPlatingMode)
    {
        EndPlatingStage();
    }

    bWasMouseDown = false;  // Reset for clean state when exiting

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (PlayerController)
    {
        // Re-enable physical mouse input
        PlayerController->bEnableMouseOverEvents = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->CurrentMouseCursor = EMouseCursor::Default;

#if WITH_EDITOR
        // Unregister Slate pre-input listener (must remove before component becomes invalid)
        if (PreInputMouseDownHandle.IsValid())
        {
            FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().Remove(PreInputMouseDownHandle);
            PreInputMouseDownHandle.Reset();
        }
#else
        PreInputMouseDownHandle.Reset();
#endif // WITH_EDITOR

        // Unbind the controller mouse action first (use Cast so we still restore move/look if this fails)
        if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
        {
            if (ControllerMouseAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
                //UE_LOG(LogTemp,Log, TEXT("Unbound controller mouse action"));
            }
            if (MouseClickAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(MouseClickBindingHandle);
            }
            if (ExitCustomizationAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
                //UE_LOG(LogTemp,Log, TEXT("Unbound exit action"));
            }

            if (NextStageAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(NextStageBindingHandle);
            }

            if (PreviousStageAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(PreviousStageBindingHandle);
            }
            if (QuantityIncreaseAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(QuantityIncreaseBindingHandle);
            }
            if (QuantityDecreaseAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(QuantityDecreaseBindingHandle);
            }
        }

        // Remove only the customization layer; DefaultMappingContext was never removed.
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (CustomizationMappingContext)
            {
                Subsystem->RemoveMappingContext(CustomizationMappingContext);
            }
        }

        // Re-enable movement and look
        if (bPU_LogMovementRestore)
            LogMovementState(PlayerController, TEXT("MAIN before restore"));
        PlayerController->ResetIgnoreMoveInput();
        PlayerController->ResetIgnoreLookInput();
        PlayerController->bShowMouseCursor = true;
        RestoreSlatePlatformCursorForUser();

        // Restore character movement (dialogue box or other systems may have called DisableMovement)
        if (APawn* Pawn = PlayerController->GetPawn())
        {
            if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                {
                    MovementComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
        if (bPU_LogMovementRestore)
        {
            LogMovementState(PlayerController, TEXT("MAIN after restore"));
            if (CurrentCharacter && CurrentCharacter->GetDefaultMappingContext())
            {
                UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] DefaultMappingContext still active (layered): %s"), *CurrentCharacter->GetDefaultMappingContext()->GetName());
            }
        }

        // Set input mode back to game and UI (mouse visible, no specific widget focus)
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(nullptr);
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(InputMode);

        // Optional viewport focus skipped — SetAllUserFocusToGameViewport walks Slate tree (FindPathToWidget) and has overflowed stacks here.

        if (bHasSavedViewportMouseCaptureForCustomization)
        {
            UGameplayStatics::SetViewportMouseCaptureMode(PlayerController, SavedViewportMouseCaptureModeForCustomization);
            bHasSavedViewportMouseCaptureForCustomization = false;
        }
    }

    // Clear stuck synthetic LMB / pointer capture before removing UMG — otherwise Slate can keep LMB pressed or capture on destroyed widgets and the next session has no slot hover/clicks.
    if (CurrentCharacter)
    {
        if (APlayerController* PCFlush = Cast<APlayerController>(CurrentCharacter->GetController()))
        {
            FlushSlateVirtualCursorPointerState(PCFlush);
        }
    }

    if (IsValid(VirtualCursorWidgetInstance))
    {
        VirtualCursorWidgetInstance->RemoveFromParent();
        VirtualCursorWidgetInstance = nullptr;
    }

    // Clean up the customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
        //UE_LOG(LogTemp,Log, TEXT("Customization UI Widget Removed"));
    }

    // Clean up the cooking stage widget
    if (CookingStageWidget)
    {
        CookingStageWidget->RemoveFromParent();
        CookingStageWidget = nullptr;
        //UE_LOG(LogTemp,Log, TEXT("Cooking Stage Widget Removed"));
    }

    ClearAll3DIngredientMeshes();
    if (World)
    {
        if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
        {
            GI->ClearCurrentDishTag();
        }
    }

    bVirtualCursorInitialized = false;
    bVirtualClickConsumedBySlateUI = false;
    bLastVirtualCursorDesktopValid = false;
    CachedVirtualCursorViewportExtentsX = 0;
    CachedVirtualCursorViewportExtentsY = 0;
    ActiveCustomizationPipelineIndex = INDEX_NONE;
    CurrentCharacter = nullptr;

    if (World)
    {
        if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
        {
            GI->NotifyPlayerQuestObjectiveOverlayVisibility();
        }
    }

    // Always re-enable movement on the world's player controller so keyboard/joystick move works no matter what.
    if (WorldPC)
    {
        if (bPU_LogMovementRestore)
            LogMovementState(WorldPC, TEXT("WorldPC before restore"));
        WorldPC->ResetIgnoreMoveInput();
        WorldPC->ResetIgnoreLookInput();

        // Restore character movement (belt-and-suspenders in case PlayerController path was skipped)
        if (APawn* Pawn = WorldPC->GetPawn())
        {
            if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                {
                    MovementComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
        if (bPU_LogMovementRestore)
            LogMovementState(WorldPC, TEXT("WorldPC after restore"));
    }

    if (World)
    {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UPUDishCustomizationComponent::BroadcastOnCustomizationEndedNextTick));
    }
    else
    {
        BroadcastOnCustomizationEndedNextTick();
    }
}

void UPUDishCustomizationComponent::HandleExitInput()
{
    //UE_LOG(LogTemp,Log, TEXT("Exit input received"));
    EndCustomization();
}

void UPUDishCustomizationComponent::HandleControllerMouse(const FInputActionValue& Value)
{
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse called"));
    
    if (!CurrentCharacter)
    {
        //UE_LOG(LogTemp,Warning, TEXT("HandleControllerMouse - No current character"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("HandleControllerMouse - No player controller"));
        return;
    }

    // Get the input value (should be a Vector2D for the right stick)
    FVector2D StickInput = Value.Get<FVector2D>();
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Raw stick input: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);
    
    // Apply deadzone
    if (FMath::Abs(StickInput.X) < ControllerMouseDeadzone)
    {
        StickInput.X = 0.0f;
    }
    if (FMath::Abs(StickInput.Y) < ControllerMouseDeadzone)
    {
        StickInput.Y = 0.0f;
    }
    if (StickInput.IsNearlyZero(1.e-4f))
    {
        return;
    }
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - After deadzone: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    if (!TryGetVirtualCursorViewportPixelExtents(PlayerController, ViewportSizeX, ViewportSizeY))
    {
        return;
    }

    if (!bVirtualCursorInitialized)
    {
        VirtualCursorViewport.X = static_cast<float>(ViewportSizeX) * 0.5f;
        VirtualCursorViewport.Y = static_cast<float>(ViewportSizeY) * 0.5f;
        bVirtualCursorInitialized = true;
    }

    const float DeltaX = StickInput.X * ControllerMouseSensitivity;
    const float DeltaY = StickInput.Y * ControllerMouseSensitivity;
    VirtualCursorViewport.X += DeltaX;
    VirtualCursorViewport.Y += DeltaY;
    VirtualCursorViewport.X = FMath::Clamp(VirtualCursorViewport.X, 0.0f, static_cast<float>(ViewportSizeX));
    VirtualCursorViewport.Y = FMath::Clamp(VirtualCursorViewport.Y, 0.0f, static_cast<float>(ViewportSizeY));

    if (bPU_LogVirtualCursor)
    {
        static float sLastVirtualCursorLogTime = -1000.f;
        const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        if (Now - sLastVirtualCursorLogTime >= 0.12f)
        {
            sLastVirtualCursorLogTime = Now;
            UE_LOG(LogTemp, Log, TEXT("[VirtualCursor] stick=(%.3f,%.3f) delta=(%.1f,%.1f) -> virtual=(%.1f,%.1f) viewport=%dx%d init=%d"),
                StickInput.X, StickInput.Y, DeltaX, DeltaY, VirtualCursorViewport.X, VirtualCursorViewport.Y, ViewportSizeX, ViewportSizeY, bVirtualCursorInitialized ? 1 : 0);
        }
    }

    ApplyVirtualCursorVisual(PlayerController);
}

FVector2D UPUDishCustomizationComponent::GetVirtualCursorScreenPosition(APlayerController* PC) const
{
    if (bVirtualCursorInitialized)
    {
        return FVector2D(VirtualCursorViewport.X, VirtualCursorViewport.Y);
    }
    float X = 0.f;
    float Y = 0.f;
    if (PC && PC->GetMousePosition(X, Y))
    {
        return FVector2D(X, Y);
    }
    return FVector2D::ZeroVector;
}

bool UPUDishCustomizationComponent::TryGetVirtualCursorViewportPixelExtents(APlayerController* PC, int32& OutW, int32& OutH) const
{
    if (!PC)
    {
        return false;
    }
    if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
    {
        if (LocalPlayer->ViewportClient)
        {
            if (FSceneViewport* SceneViewport = LocalPlayer->ViewportClient->GetGameViewport())
            {
                const FIntPoint Size = SceneViewport->GetSizeXY();
                OutW = Size.X;
                OutH = Size.Y;
                if (OutW > 0 && OutH > 0)
                {
                    return true;
                }
            }
        }
    }
    PC->GetViewportSize(OutW, OutH);
    return OutW > 0 && OutH > 0;
}

bool UPUDishCustomizationComponent::TryComputeVirtualCursorDesktopAbsolute(APlayerController* PC, FVector2D& OutDesktopAbs) const
{
    if (!PC || !bVirtualCursorInitialized)
    {
        return false;
    }
    int32 VSX = 0;
    int32 VSY = 0;
    if (!TryGetVirtualCursorViewportPixelExtents(PC, VSX, VSY))
    {
        return false;
    }
    ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    if (!LocalPlayer || !LocalPlayer->ViewportClient)
    {
        return false;
    }
    FSceneViewport* SceneViewport = LocalPlayer->ViewportClient->GetGameViewport();
    if (!SceneViewport)
    {
        return false;
    }
    if (!VirtualViewportPixelsToSlateCursorAbsolute(SceneViewport, FVector2D(VirtualCursorViewport.X, VirtualCursorViewport.Y), VSX, VSY, OutDesktopAbs))
    {
        return false;
    }
    if (!FMath::IsFinite(OutDesktopAbs.X) || !FMath::IsFinite(OutDesktopAbs.Y))
    {
        return false;
    }
    return true;
}

void UPUDishCustomizationComponent::ApplyVirtualCursorVisual(APlayerController* PC)
{
    if (!PC || !bVirtualCursorInitialized)
    {
        return;
    }
    int32 VSX = 0;
    int32 VSY = 0;
    if (!TryGetVirtualCursorViewportPixelExtents(PC, VSX, VSY))
    {
        return;
    }
    VirtualCursorViewport.X = FMath::Clamp(VirtualCursorViewport.X, 0.0f, static_cast<float>(VSX));
    VirtualCursorViewport.Y = FMath::Clamp(VirtualCursorViewport.Y, 0.0f, static_cast<float>(VSY));

    if (bPU_LogVirtualCursor)
    {
        static float sLastSyncLogTime = -1000.f;
        const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        if (Now - sLastSyncLogTime >= 0.12f)
        {
            sLastSyncLogTime = Now;
            UE_LOG(LogTemp, Log, TEXT("[VirtualCursor] ApplyVisual: viewport pos (%.1f, %.1f) size %dx%d widget=%s"),
                VirtualCursorViewport.X, VirtualCursorViewport.Y, VSX, VSY,
                IsValid(VirtualCursorWidgetInstance) ? TEXT("yes") : TEXT("no"));
        }
    }

    if (IsValid(VirtualCursorWidgetInstance))
    {
        // true: position is in viewport pixels (same as GetViewportSize / SceneViewport size); subsystem divides by viewport scale for Slate layout.
        VirtualCursorWidgetInstance->SetPositionInViewport(
            FVector2D(VirtualCursorViewport.X - VirtualCursorHotspotOffset.X, VirtualCursorViewport.Y - VirtualCursorHotspotOffset.Y),
            true);
    }

    ApplyVirtualCursorSlateHover(FVector2D(VirtualCursorViewport.X, VirtualCursorViewport.Y), PC, VSX, VSY);

    FVector2D DesktopAbs;
    if (TryComputeVirtualCursorDesktopAbsolute(PC, DesktopAbs))
    {
        LastVirtualCursorDesktopAbs = DesktopAbs;
        bLastVirtualCursorDesktopValid = true;
    }

    CachedVirtualCursorViewportExtentsX = VSX;
    CachedVirtualCursorViewportExtentsY = VSY;

    ApplyVirtualCursorHardwareCursorLock(PC);
}

void UPUDishCustomizationComponent::ReassertVirtualCursorAfterUMGFocus(APlayerController* PC)
{
    if (!PC || !ShouldSuppressHardwareMouseCursor() || !bVirtualCursorInitialized)
    {
        return;
    }
    ApplyVirtualCursorHardwareCursorLock(PC);
    ScheduleVirtualCursorHardwareCursorLockNextFrame(PC);
    ApplyVirtualCursorVisual(PC);
}

void UPUDishCustomizationComponent::FlushSlateVirtualCursorPointerState(APlayerController* PC)
{
    if (!PC || !FSlateApplication::IsInitialized())
    {
        return;
    }
    ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    if (!LocalPlayer)
    {
        return;
    }

    FSlateApplication& SlateApp = FSlateApplication::Get();
    const int32 UserIndex = LocalPlayer->GetControllerId();
    SlateApp.ReleaseAllPointerCapture(UserIndex);

    TSharedPtr<FSlateUser> SlateUser = SlateApp.GetUser(UserIndex);
    if (!SlateUser.IsValid())
    {
        SlateUser = SlateApp.GetCursorUser();
    }
    if (!SlateUser.IsValid())
    {
        return;
    }

    const bool bPhysicalLMB = PC->IsInputKeyDown(EKeys::LeftMouseButton);
    if (bPhysicalLMB)
    {
        return;
    }

    const TSet<FKey>& PressedKeys = SlateApp.GetPressedMouseButtons();
    if (!PressedKeys.Contains(EKeys::LeftMouseButton))
    {
        return;
    }

    FVector2D AbsPos;
    if (!TryComputeVirtualCursorDesktopAbsolute(PC, AbsPos))
    {
        AbsPos = SlateUser->GetCursorPosition();
    }

    SlateUser->SetCursorPosition(static_cast<int32>(AbsPos.X), static_cast<int32>(AbsPos.Y));
    const FVector2D OldAbs = bLastVirtualCursorDesktopValid ? LastVirtualCursorDesktopAbs : AbsPos;
    const bool bIsPrimaryUser = FSlateApplication::CursorUserIndex == SlateUser->GetUserIndex();
    const FPointerEvent MouseEvent(
        SlateUser->GetUserIndex(),
        FSlateApplication::CursorPointerIndex,
        AbsPos,
        OldAbs,
        bIsPrimaryUser ? SlateApp.GetPressedMouseButtons() : FTouchKeySet::EmptySet,
        EKeys::LeftMouseButton,
        0.f,
        bIsPrimaryUser ? SlateApp.GetModifierKeys() : FModifierKeysState());
    SlateApp.ProcessMouseButtonUpEvent(MouseEvent);
    bVirtualClickConsumedBySlateUI = false;
}

void UPUDishCustomizationComponent::ApplyVirtualCursorHardwareCursorLock(APlayerController* PC) const
{
    if (!PC || !VirtualCursorWidgetClass)
    {
        return;
    }
    PC->SetShowMouseCursor(false);
    PC->CurrentMouseCursor = EMouseCursor::None;
    // Slate re-enables the real OS cursor after a few platform mouse-move events (see FSlateApplication::OnMouseMove).
    // FFauxSlateCursor keeps the Windows pointer hidden while we drive position with SetCursorPosition / virtual UMG pointer.
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UsePlatformCursorForCursorUser(false);
    }
}

void UPUDishCustomizationComponent::RestoreSlatePlatformCursorForUser()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UsePlatformCursorForCursorUser(true);
    }
}

void UPUDishCustomizationComponent::ScheduleVirtualCursorHardwareCursorLockNextFrame(APlayerController* PC)
{
    if (!PC || !ShouldSuppressHardwareMouseCursor())
    {
        return;
    }
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    TWeakObjectPtr<UPUDishCustomizationComponent> WeakThis(this);
    TWeakObjectPtr<APlayerController> WeakPC(PC);
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, WeakPC]()
    {
        UPUDishCustomizationComponent* Comp = WeakThis.Get();
        APlayerController* P = WeakPC.Get();
        if (!Comp || !P || !Comp->ShouldSuppressHardwareMouseCursor())
        {
            return;
        }
        Comp->ApplyVirtualCursorHardwareCursorLock(P);
    }));
}

void UPUDishCustomizationComponent::NotifyVirtualCursorInteractVisual(bool bPressed)
{
    if (!IsValid(VirtualCursorWidgetInstance))
    {
        return;
    }
    if (UPUVirtualCursorUserWidget* CursorWidget = Cast<UPUVirtualCursorUserWidget>(VirtualCursorWidgetInstance))
    {
        CursorWidget->OnVirtualCursorInteractVisual(bPressed);
    }
}

void UPUDishCustomizationComponent::BroadcastOnCustomizationEndedNextTick()
{
    OnCustomizationEnded.Broadcast();
}

void UPUDishCustomizationComponent::OnCustomizationViewportDeferredSetup()
{
    if (!IsCustomizing() || !CurrentCharacter)
    {
        return;
    }
    APlayerController* PC = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PC)
    {
        return;
    }
    UGameplayStatics::SetViewportMouseCaptureMode(PC, EMouseCaptureMode::NoCapture);
    if (bVirtualCursorInitialized)
    {
        FlushSlateVirtualCursorPointerState(PC);
        ApplyVirtualCursorVisual(PC);
        if (bPU_LogVirtualCursor)
        {
            UE_LOG(LogTemp, Log, TEXT("[VirtualCursor] OnCustomizationViewportDeferredSetup: re-applied visual, virtual=(%.1f,%.1f)"), VirtualCursorViewport.X, VirtualCursorViewport.Y);
        }
    }
    else if (bPU_LogVirtualCursor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursor] OnCustomizationViewportDeferredSetup: bVirtualCursorInitialized was false, skipped sync"));
    }
}

void UPUDishCustomizationComponent::OnPreInputMouseButtonDown(const FPointerEvent& MouseEvent)
{
    // Fires BEFORE Slate widgets consume the click - bypasses widget blocking
    if (bInsideSyntheticSlateMouseDispatch)
    {
        return;
    }
    // UI-only customization: do not forward to world-ingredient click/drag (Enhanced Input + Slate handle UMG).
    (void)MouseEvent;
}

void UPUDishCustomizationComponent::HandleMouseClick(const FInputActionValue& Value)
{
    if (bPU_LogIngredientDrag)
    {
        UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick called"));
    }

    if (!CurrentCharacter || bIsDragging)
    {
        if (bPU_LogIngredientDrag) UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick early out: Character=%s Dragging=%s"), CurrentCharacter ? TEXT("ok") : TEXT("null"), bIsDragging ? TEXT("yes") : TEXT("no"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        if (bPU_LogIngredientDrag) UE_LOG(LogTemp, Warning, TEXT("[DRAG] HandleMouseClick - No player controller"));
        return;
    }

    bVirtualClickConsumedBySlateUI = false;

    NotifyVirtualCursorInteractVisual(true);
    ApplyVirtualCursorHardwareCursorLock(PlayerController);

    const FVector2D ViewportCursor = GetVirtualCursorScreenPosition(PlayerController);
    const bool bPhysicalLMB = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
    if (bPU_LogVirtualCursorClick)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] HandleMouseClick START: viewport=(%.1f,%.1f) virtualInit=%d physicalLMB_down=%d"),
            ViewportCursor.X, ViewportCursor.Y, bVirtualCursorInitialized ? 1 : 0, bPhysicalLMB ? 1 : 0);
    }

    // Interact / MouseClickAction without physical LMB: send a synthetic left click through Slate so UMG matches mouse (ingredient slots, buttons).
    // Real LMB still uses normal Slate routing; we skip this branch so we do not double-fire ProcessMouseButtonDownEvent.
    if (bVirtualCursorInitialized && FSlateApplication::IsInitialized() && !bPhysicalLMB)
    {
        if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            FSlateApplication& SlateApp = FSlateApplication::Get();
            TSharedPtr<FSlateUser> SlateUser = SlateApp.GetUser(LocalPlayer->GetControllerId());
            if (!SlateUser.IsValid())
            {
                SlateUser = SlateApp.GetCursorUser();
            }
            if (SlateUser.IsValid())
            {
                // Do not use SlateUser->GetCursorPosition() here: with OS cursor hidden it can be invalid (e.g. INT_MIN), breaking LocateWindowUnderMouse.
                FVector2D AbsPos;
                if (!TryComputeVirtualCursorDesktopAbsolute(PlayerController, AbsPos))
                {
                    if (bPU_LogVirtualCursorClick)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] NO synthetic click: TryComputeVirtualCursorDesktopAbsolute failed"));
                    }
                }
                else
                {
                const FWidgetPath Path = SlateApp.LocateWindowUnderMouse(AbsPos, SlateApp.GetInteractiveTopLevelWindows(), false, SlateUser->GetUserIndex());
                const bool bHasUMG = Path.IsValid() && WidgetPathContainsSObjectWidget(Path);
                FString LeafType = TEXT("(no path)");
                if (Path.IsValid() && Path.Widgets.Num() > 0)
                {
                    LeafType = Path.GetLastWidget()->GetTypeAsString();
                }
                if (bPU_LogVirtualCursorClick)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] Slate probe: desktopAbs=(%.1f,%.1f) pathValid=%d pathWidgets=%d hasSObjectWidget=%d leafType=%s"),
                        AbsPos.X, AbsPos.Y, Path.IsValid() ? 1 : 0, Path.IsValid() ? Path.Widgets.Num() : 0, bHasUMG ? 1 : 0, *LeafType);
                }
                if (Path.IsValid() && bHasUMG)
                {
                    SlateUser->SetCursorPosition(static_cast<int32>(AbsPos.X), static_cast<int32>(AbsPos.Y));
                    const FVector2D OldAbs = bLastVirtualCursorDesktopValid ? LastVirtualCursorDesktopAbs : AbsPos;
                    const bool bIsPrimaryUser = FSlateApplication::CursorUserIndex == SlateUser->GetUserIndex();
                    const FPointerEvent MouseEvent(
                        SlateUser->GetUserIndex(),
                        FSlateApplication::CursorPointerIndex,
                        AbsPos,
                        OldAbs,
                        bIsPrimaryUser ? SlateApp.GetPressedMouseButtons() : FTouchKeySet::EmptySet,
                        EKeys::LeftMouseButton,
                        0.f,
                        bIsPrimaryUser ? SlateApp.GetModifierKeys() : FModifierKeysState());
                    TSharedPtr<FGenericWindow> GenWindow;
                    {
                        FPUScopedSyntheticSlateMouseDispatch GuardSyntheticDispatch(bInsideSyntheticSlateMouseDispatch);
                        SlateApp.ProcessMouseButtonDownEvent(GenWindow, MouseEvent);
                    }
                    ApplyVirtualCursorHardwareCursorLock(PlayerController);
                    ScheduleVirtualCursorHardwareCursorLockNextFrame(PlayerController);
                    bVirtualClickConsumedBySlateUI = true;
                    if (bPU_LogVirtualCursorClick)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] SYNTHETIC LMB DOWN sent to Slate (ProcessMouseButtonDownEvent). Expect slot NativeOnMouseButtonDown; hover may flicker while button is 'held'."));
                    }
                    return;
                }
                if (bPU_LogVirtualCursorClick)
                {
                    if (!Path.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] NO synthetic click: LocateWindowUnderMouse returned invalid path (SlateAbs mismatch with UI?)"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] NO synthetic click: path has no SObjectWidget — falling through to 3D trace (leaf=%s)"), *LeafType);
                    }
                }
                }
            }
            else if (bPU_LogVirtualCursorClick)
            {
                UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] NO synthetic click: no FSlateUser for local player / cursor user"));
            }
        }
        else if (bPU_LogVirtualCursorClick)
        {
            UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] NO synthetic click: no ULocalPlayer"));
        }
    }
    else if (bPU_LogVirtualCursorClick && bVirtualCursorInitialized && FSlateApplication::IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] Synthetic UMG click SKIPPED because physical LeftMouseButton is DOWN — if you use gamepad only, check IMC: Interact must NOT also press LMB."));
    }

    // No world trace / ingredient mesh drag — plating uses UI slots only.
    ScheduleVirtualCursorHardwareCursorLockNextFrame(PlayerController);
}

void UPUDishCustomizationComponent::HandleNextStage()
{
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            // Call the Blueprint event so animations can play first
            DishWidget->OnControllerNextStage();
        }
    }
}

void UPUDishCustomizationComponent::HandlePreviousStage()
{
    UE_LOG(LogTemp, Warning, TEXT("🔙 HandlePreviousStage called - CustomizationWidget: %s"), 
        CustomizationWidget ? *CustomizationWidget->GetName() : TEXT("NULL"));
    
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            UE_LOG(LogTemp, Warning, TEXT("🔙 Calling OnControllerPreviousStage on widget: %s"), *DishWidget->GetName());
            // Call the Blueprint event so animations can play first
            DishWidget->OnControllerPreviousStage();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🔙 Failed to cast CustomizationWidget to UPUDishCustomizationWidget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🔙 CustomizationWidget is NULL!"));
    }
}

void UPUDishCustomizationComponent::HandleQuantityIncrease(const FInputActionValue& Value)
{
    (void)Value;
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            DishWidget->TryApplyQuantityInputFromEnhancedInput(1);
        }
    }
}

void UPUDishCustomizationComponent::HandleQuantityDecrease(const FInputActionValue& Value)
{
    (void)Value;
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            DishWidget->TryApplyQuantityInputFromEnhancedInput(-1);
        }
    }
}

void UPUDishCustomizationComponent::HandleMouseRelease(const FInputActionValue& Value)
{
    (void)Value;
    if (bVirtualCursorInitialized && CurrentCharacter)
    {
        if (APlayerController* VPC = Cast<APlayerController>(CurrentCharacter->GetController()))
        {
            NotifyVirtualCursorInteractVisual(false);
            ApplyVirtualCursorHardwareCursorLock(VPC);
        }
    }
    if (bPU_LogVirtualCursorClick)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] HandleMouseRelease: consumedBySlate=%d virtualInit=%d dragging3D=%d"),
            bVirtualClickConsumedBySlateUI ? 1 : 0, bVirtualCursorInitialized ? 1 : 0, bIsDragging ? 1 : 0);
    }

    if (bVirtualCursorInitialized && bVirtualClickConsumedBySlateUI && FSlateApplication::IsInitialized() && CurrentCharacter)
    {
        bVirtualClickConsumedBySlateUI = false;
        if (APlayerController* PC = Cast<APlayerController>(CurrentCharacter->GetController()))
        {
            if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
            {
                FSlateApplication& SlateApp = FSlateApplication::Get();
                TSharedPtr<FSlateUser> SlateUser = SlateApp.GetUser(LocalPlayer->GetControllerId());
                if (!SlateUser.IsValid())
                {
                    SlateUser = SlateApp.GetCursorUser();
                }
                if (SlateUser.IsValid())
                {
                    FVector2D AbsPos;
                    if (TryComputeVirtualCursorDesktopAbsolute(PC, AbsPos))
                    {
                        SlateUser->SetCursorPosition(static_cast<int32>(AbsPos.X), static_cast<int32>(AbsPos.Y));
                        const FVector2D OldAbs = bLastVirtualCursorDesktopValid ? LastVirtualCursorDesktopAbs : AbsPos;
                        const bool bIsPrimaryUser = FSlateApplication::CursorUserIndex == SlateUser->GetUserIndex();
                        const FPointerEvent MouseEvent(
                            SlateUser->GetUserIndex(),
                            FSlateApplication::CursorPointerIndex,
                            AbsPos,
                            OldAbs,
                            bIsPrimaryUser ? SlateApp.GetPressedMouseButtons() : FTouchKeySet::EmptySet,
                            EKeys::LeftMouseButton,
                            0.f,
                            bIsPrimaryUser ? SlateApp.GetModifierKeys() : FModifierKeysState());
                        SlateApp.ProcessMouseButtonUpEvent(MouseEvent);
                        ApplyVirtualCursorHardwareCursorLock(PC);
                        ScheduleVirtualCursorHardwareCursorLockNextFrame(PC);
                        if (bPU_LogVirtualCursorClick)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] SYNTHETIC LMB UP sent (ProcessMouseButtonUpEvent) desktopAbs=(%.1f,%.1f). Hover should restore on next stick move."),
                                AbsPos.X, AbsPos.Y);
                        }
                    }
                    else if (bPU_LogVirtualCursorClick)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] Release: TryComputeVirtualCursorDesktopAbsolute failed; synthetic LMB up skipped"));
                    }
                }
                else if (bPU_LogVirtualCursorClick)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] Release: expected synthetic up but SlateUser invalid"));
                }
            }
        }
        if (!bIsDragging)
        {
            if (APlayerController* PCSchedule = Cast<APlayerController>(CurrentCharacter->GetController()))
            {
                ScheduleVirtualCursorHardwareCursorLockNextFrame(PCSchedule);
            }
            return;
        }
    }
    else if (bPU_LogVirtualCursorClick && bVirtualCursorInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VirtualCursorClick] Release: not sending synthetic LMB up (consumedBySlate was false or slate not init) — 3D drag release or missed paired down"));
    }

    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        FString IngredientName = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetName() : TEXT("INVALID");
        FVector FinalPosition = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetActorLocation() : FVector::ZeroVector;
        
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] Stopping drag for %s at final position (%.2f,%.2f,%.2f)"), 
        //    *IngredientName, FinalPosition.X, FinalPosition.Y, FinalPosition.Z);
        
        // Call the ingredient's release function
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
            
            FVector PositionAfterRelease = CurrentlyDraggedIngredient->GetActorLocation();
            //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] After OnMouseRelease - %s at position (%.2f,%.2f,%.2f)"), 
            //    *CurrentlyDraggedIngredient->GetName(), 
            //    PositionAfterRelease.X, PositionAfterRelease.Y, PositionAfterRelease.Z);
        }
        
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
    }

    if (ShouldSuppressHardwareMouseCursor() && CurrentCharacter)
    {
        if (APlayerController* VPC = Cast<APlayerController>(CurrentCharacter->GetController()))
        {
            ScheduleVirtualCursorHardwareCursorLockNextFrame(VPC);
        }
    }
}

void UPUDishCustomizationComponent::StartDraggingIngredient(APUIngredientMesh* Ingredient)
{
    if (bPU_LogIngredientDrag && Ingredient)
    {
        UE_LOG(LogTemp, Log, TEXT("[DRAG] StartDraggingIngredient (no-op UI-only) — %s"), *Ingredient->GetName());
    }
}

void UPUDishCustomizationComponent::UpdateMouseDrag()
{
    if (!bIsDragging || !CurrentCharacter)
    {
        return;
    }
    
    // Check if the ingredient is still valid (not destroyed)
    if (!IsValid(CurrentlyDraggedIngredient))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [DRAG] UpdateMouseDrag - Ingredient is no longer valid, stopping drag"));
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        return;
    }

    // Check if mouse button is still held down
    bool bLeftMouseDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
    if (!bLeftMouseDown)
    {
        // Mouse button released, stop dragging
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] UpdateMouseDrag - Mouse button released, stopping drag for %s"), 
        //    *CurrentlyDraggedIngredient->GetName());
        HandleMouseRelease(FInputActionValue());
        return;
    }

    const FVector2D ScreenPos = GetVirtualCursorScreenPosition(PlayerController);
    FHitResult HitResult;
    if (!PlayerController->GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, true, HitResult))
    {
        return;
    }

    float SurfaceHeight = 0.0f;
    if (!GetPlateSurfaceHeight(SurfaceHeight))
    {
        SurfaceHeight = CurrentlyDraggedIngredient->GetActorLocation().Z;
    }

    // Project hit location onto dish surface (use X,Y from hit, Z from surface)
    FVector MouseWorldPosition(HitResult.Location.X, HitResult.Location.Y, SurfaceHeight);
    FVector NewPosition = MouseWorldPosition + DragOffset;

    AActor* OwnerActor = GetOwner();
    FVector RefPoint = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
    if (!NewPosition.ContainsNaN() && FVector::Dist(NewPosition, RefPoint) < 5000.0f)
    {
        CurrentlyDraggedIngredient->UpdatePosition(NewPosition);
    }
}

bool UPUDishCustomizationComponent::HasActiveCustomizationPipeline() const
{
    return CurrentDishData.HasCustomizationPipeline();
}

int32 UPUDishCustomizationComponent::GetCustomizationPipelineStageCount() const
{
    return CurrentDishData.CustomizationStages.Num();
}

bool UPUDishCustomizationComponent::TryGetActivePipelineStage(FPUDishCustomizationStageDescriptor& OutStage) const
{
    if (!CurrentDishData.HasCustomizationPipeline() || !CurrentDishData.CustomizationStages.IsValidIndex(ActiveCustomizationPipelineIndex))
    {
        return false;
    }
    OutStage = CurrentDishData.CustomizationStages[ActiveCustomizationPipelineIndex];
    return true;
}

bool UPUDishCustomizationComponent::TryGetPipelineStageByIndex(int32 Index, FPUDishCustomizationStageDescriptor& OutStage) const
{
    if (!CurrentDishData.CustomizationStages.IsValidIndex(Index))
    {
        return false;
    }
    OutStage = CurrentDishData.CustomizationStages[Index];
    return true;
}

void UPUDishCustomizationComponent::ResetCustomizationPipelineProgress()
{
    ActiveCustomizationPipelineIndex = CurrentDishData.HasCustomizationPipeline() ? 0 : INDEX_NONE;
}

bool UPUDishCustomizationComponent::AdvanceCustomizationPipeline()
{
    if (!CurrentDishData.HasCustomizationPipeline())
    {
        return false;
    }
    if (ActiveCustomizationPipelineIndex == INDEX_NONE)
    {
        ActiveCustomizationPipelineIndex = 0;
    }
    const int32 Next = ActiveCustomizationPipelineIndex + 1;
    if (!CurrentDishData.CustomizationStages.IsValidIndex(Next))
    {
        return false;
    }
    ActiveCustomizationPipelineIndex = Next;
    return true;
}

void UPUDishCustomizationComponent::SetActiveCustomizationPipelineIndex(int32 Index)
{
    if (!CurrentDishData.HasCustomizationPipeline() || !CurrentDishData.CustomizationStages.IsValidIndex(Index))
    {
        return;
    }
    ActiveCustomizationPipelineIndex = Index;
}

void UPUDishCustomizationComponent::UpdateCurrentDishData(const FPUDishBase& NewDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Updating dish data with %d ingredients"), 
    //    NewDishData.IngredientInstances.Num());
    
    CurrentDishData = NewDishData;

    if (!CurrentDishData.HasCustomizationPipeline())
    {
        ActiveCustomizationPipelineIndex = INDEX_NONE;
    }
    else if (ActiveCustomizationPipelineIndex != INDEX_NONE && !CurrentDishData.CustomizationStages.IsValidIndex(ActiveCustomizationPipelineIndex))
    {
        ActiveCustomizationPipelineIndex = FMath::Clamp(ActiveCustomizationPipelineIndex, 0, CurrentDishData.CustomizationStages.Num() - 1);
    }

    // Log the ingredients for debugging
    if (bPU_LogDishDataIngredientTags)
    {
        for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
        {
            const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
            //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Ingredient %d: %s (Qty: %d)"),
            //    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
        }
    }
}

void UPUDishCustomizationComponent::SyncDishDataFromUI(const FPUDishBase& DishDataFromUI)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Syncing dish data from UI with %d ingredients"), 
    //    DishDataFromUI.IngredientInstances.Num());
    
    // Update the current dish data with the data from the UI
    UpdateCurrentDishData(DishDataFromUI);
    
    // Broadcast the updated dish data to all subscribers
    OnDishDataUpdated.Broadcast(DishDataFromUI);
    
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Dish data synced and broadcasted successfully"));
}

// This function is no longer needed - we'll use a different approach
void UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget(UUserWidget* Widget)
{
    // Removed to avoid crashes - using alternative data passing methods
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget - Function disabled, using alternative approach"));
}

void UPUDishCustomizationComponent::SetWidgetComponentReference(UPUDishCustomizationWidget* Widget)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Setting widget component reference"));
    
    if (Widget)
    {
        // Set the component reference on the widget
        Widget->SetCustomizationComponent(this);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Widget component reference set successfully"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetWidgetComponentReference - Widget is null"));
    }
}

void UPUDishCustomizationComponent::SetActiveCustomizationWidget(UPUDishCustomizationWidget* ActiveWidget)
{
    if (ActiveWidget)
    {
        // Update the CustomizationWidget reference to track the currently active widget
        // This ensures EndCustomization() can properly clean up the active widget
        CustomizationWidget = ActiveWidget;
        //UE_LOG(LogTemp,Display, TEXT("🔄 UPUDishCustomizationComponent::SetActiveCustomizationWidget - Updated active widget to: %s"), 
        //    *ActiveWidget->GetName());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetActiveCustomizationWidget - Active widget is null"));
    }
}

void UPUDishCustomizationComponent::SetInitialDishData(const FPUDishBase& InitialDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Setting initial dish data: %s with %d ingredients"), 
    //    *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());

    EnsureDishIngredientsInPantry(InitialDishData);
    
    // Set the initial dish data
    UpdateCurrentDishData(InitialDishData);

    // Notify Game Instance so journal recipe section shows this dish when opened during customization
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        GI->SetCurrentDishTag(InitialDishData.DishTag);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Initial dish data set successfully"));
}

// New event-driven data passing methods
void UPUDishCustomizationComponent::BroadcastDishDataUpdate(const FPUDishBase& NewDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::BroadcastDishDataUpdate - Broadcasting dish data update with %d ingredients"), 
    //    NewDishData.IngredientInstances.Num());
    
    // Update internal data
    UpdateCurrentDishData(NewDishData);
    
    // Broadcast the update to all subscribers
    OnDishDataUpdated.Broadcast(NewDishData);
}

void UPUDishCustomizationComponent::BroadcastInitialDishData(const FPUDishBase& InitialDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::BroadcastInitialDishData - Broadcasting initial dish data: %s"), 
    //    *InitialDishData.DisplayName.ToString());

    EnsureDishIngredientsInPantry(InitialDishData);
    
    CurrentDishData = InitialDishData;
    OnInitialDishDataReceived.Broadcast(InitialDishData);
}

void UPUDishCustomizationComponent::EnsureDishIngredientsInPantry(const FPUDishBase& Dish)
{
    UWorld* World = GetWorld();
    UPUProjectUmeowmiGameInstance* GI = World ? World->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
    if (!GI) return;

    TArray<FGameplayTag> TagsToUnlock;
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        FGameplayTag Tag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (Tag.IsValid() && !GI->IsIngredientUnlocked(Tag))
        {
            TagsToUnlock.AddUnique(Tag);
        }
    }
    if (TagsToUnlock.Num() > 0)
    {
        GI->UnlockIngredients(TagsToUnlock, true); // silent: add to pantry without popup
    }
}

void UPUDishCustomizationComponent::StartPlanningMode()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Initialize planning data with current dish
    CurrentPlanningData.TargetDish = CurrentDishData;
    CurrentPlanningData.SelectedIngredients.Empty();
    CurrentPlanningData.bPlanningCompleted = false;
    
    // Populate SelectedIngredients from existing dish ingredients
    // Extract unique ingredients from IngredientInstances (planning mode uses ingredients without quantities)
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
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Added existing ingredient to SelectedIngredients: %s"), 
            //    *InstanceTag.ToString());
        }
    }
    
    // Add all unique ingredients to SelectedIngredients
    UniqueIngredients.GenerateValueArray(CurrentPlanningData.SelectedIngredients);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Populated %d existing ingredients into SelectedIngredients"), 
    //    CurrentPlanningData.SelectedIngredients.Num());
    
    // Notify the widget to start planning mode
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            DishWidget->StartPlanningMode();
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Planning mode started for dish: %s"), 
    //    *CurrentPlanningData.TargetDish.DisplayName.ToString());
}

void UPUDishCustomizationComponent::TransitionToCookingStage(const FPUDishBase& DishData)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Transitioning to cooking stage"));
    
    // Store the dish data
    CurrentDishData = DishData;
    
    // Save reference to dish widget BEFORE removing it (needed for ingredient slots)
    UPUDishCustomizationWidget* SavedDishWidget = nullptr;
    if (CustomizationWidget)
    {
        SavedDishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget);
    }
    
    // Remove the current customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }
    
    // Create and show the cooking stage widget (now uses PUDishCustomizationWidget or subclass)
    if (CurrentCharacter && CurrentCharacter->GetWorld())
    {
        // Create cooking stage widget (should be a subclass of PUDishCustomizationWidget)
        if (UPUDishCustomizationWidget* CookingWidget = CreateWidget<UPUDishCustomizationWidget>(CurrentCharacter->GetWorld(), CookingStageWidgetClass))
        {
            // Store reference to cooking stage widget
            CookingStageWidget = CookingWidget;
            
            // Add to viewport first
            CookingWidget->AddToViewport(PUDishCustomizationViewportZOrder);
            
            // Get the cooking station location (this component's owner location)
            FVector CookingStationLocation = GetOwner()->GetActorLocation();
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking station location: %s"), 
            //    *CookingStationLocation.ToString());
            
            // Set the dish customization component reference
            CookingWidget->SetCustomizationComponent(this);
            
            // Broadcast initial dish data to the new widget
            BroadcastInitialDishData(DishData);
            
            // Set as active widget so stage navigation (X/B) and input routing work correctly
            SetActiveCustomizationWidget(CookingWidget);
            
            // Note: The new cooking stage widget should handle initialization in its Blueprint
            // or override OnInitialDishDataReceived to set up ingredient slots, etc.
            // The old InitializeCookingStage method is no longer used - initialization happens
            // through the standard dish data flow
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking stage widget created and added to viewport"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToCookingStage - Failed to create cooking stage widget"));
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Planning completed with %d selected ingredients"), 
    //    DishData.IngredientInstances.Num());
}

void UPUDishCustomizationComponent::SetDataTables(UDataTable* DishTable, UDataTable* IngredientTable, UDataTable* PreparationTable)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetDataTables - Setting data table references"));
    
    IngredientDataTable = IngredientTable;
    PreparationDataTable = PreparationTable;
}

TArray<FPUIngredientBase> UPUDishCustomizationComponent::GetIngredientData() const
{
    TArray<FPUIngredientBase> IngredientData;
    
    if (IngredientDataTable)
    {
        // Get the game instance to check unlocked ingredients
        UWorld* World = GetWorld();
        UPUProjectUmeowmiGameInstance* GameInstance = World ? Cast<UPUProjectUmeowmiGameInstance>(World->GetGameInstance()) : nullptr;
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Getting ingredient data from table: %s"), *IngredientDataTable->GetName());
        
        TArray<FName> RowNames = IngredientDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUIngredientBase* Ingredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredientData")))
            {
                // Only include ingredients that are unlocked (if game instance is available)
                // If no game instance, include all ingredients (for backwards compatibility or editor use)
                if (GameInstance)
                {
                    if (GameInstance->IsIngredientUnlocked(Ingredient->IngredientTag))
                    {
                        IngredientData.Add(*Ingredient);
                    }
                }
                else
                {
                    // No game instance available, include all ingredients
                    // This can happen in editor or before game instance is initialized
                    IngredientData.Add(*Ingredient);
                }
            }
        }
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Retrieved %d unlocked ingredients (out of %d total)"), 
        //    IngredientData.Num(), RowNames.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishCustomizationComponent::GetIngredientData - No ingredient data table available"));
    }
    
    return IngredientData;
}

TArray<FPUPreparationBase> UPUDishCustomizationComponent::GetPreparationData() const
{
    TArray<FPUPreparationBase> PreparationData;
    
    if (PreparationDataTable)
    {
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Getting preparation data from table: %s"), *PreparationDataTable->GetName());
        
        TArray<FName> RowNames = PreparationDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUPreparationBase* Preparation = PreparationDataTable->FindRow<FPUPreparationBase>(RowName, TEXT("GetPreparationData")))
            {
                PreparationData.Add(*Preparation);
            }
        }
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Retrieved %d preparations"), PreparationData.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishCustomizationComponent::GetPreparationData - No preparation data table available"));
    }
    
    return PreparationData;
}

// Plating-specific functions
void UPUDishCustomizationComponent::SpawnIngredientIn3D(const FGameplayTag& IngredientTag, const FVector& WorldPosition)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - START - Ingredient %s at position (%.2f,%.2f,%.2f)"), 
    //    *IngredientTag.ToString(), WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!CanSpawnIngredientsIn3D())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Cannot spawn (not in cooking or plating stage)"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - In plating mode, checking %d ingredient instances"), 
    //    CurrentDishData.IngredientInstances.Num());

    // Find the ingredient in the current dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Checking instance %d: %s vs %s"), 
        //    i, *Instance.IngredientData.IngredientTag.ToString(), *IngredientTag.ToString());
        
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(Instance.InstanceID))
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Cannot place ingredient %s - quantity limit reached"), 
                //    *IngredientTag.ToString());
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed (quantity limit)"));
                return;
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Found matching ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(Instance.InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(Instance.InstanceID);
            
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3D - Set plating for instance %d"), Instance.InstanceID);
            
            // Broadcast the updated dish data
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Success"));
            return;
        }
    }

    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Ingredient %s not found in current dish"), *IngredientTag.ToString());
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed"));
}

void UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID(int32 InstanceID, const FVector& WorldPosition)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - START - InstanceID %d at position (%.2f,%.2f,%.2f)"), 
    //    InstanceID, WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!CanSpawnIngredientsIn3D())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Cannot spawn (not in cooking or plating stage)"));
        return;
    }

    // Find the specific ingredient instance by InstanceID
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Found instance %d: %s"), 
            //    InstanceID, *Instance.IngredientData.DisplayName.ToString());
            
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(InstanceID))
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Cannot place ingredient %s (InstanceID: %d) - quantity limit reached"), 
                //    *Instance.IngredientData.DisplayName.ToString(), InstanceID);
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed (quantity limit)"));
                return;
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Can place ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(InstanceID);
            
            // Update the ingredient slot's quantity display (NOT buttons - we use slots in plating mode)
            UpdateIngredientSlotQuantity(InstanceID);
            
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Set plating for instance %d"), InstanceID);
            
            // Broadcast the updated dish data
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Success"));
            return;
        }
    }

    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - InstanceID %d not found in current dish"), InstanceID);
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed"));
}

void UPUDishCustomizationComponent::SetPlatingMode(bool bInPlatingMode)
{
    bPlatingMode = bInPlatingMode;
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SetPlatingMode - Plating mode set to: %s"), 
    //    bPlatingMode ? TEXT("TRUE") : TEXT("FALSE"));
}

bool UPUDishCustomizationComponent::IsPlatingMode() const
{
    return bPlatingMode;
}

bool UPUDishCustomizationComponent::CanSpawnIngredientsIn3D() const
{
    // Allow plating/cooking UI to record world-space plating coordinates (no mesh actors spawned).
    if (bPlatingMode)
    {
        return true;
    }
    // Check if we're in cooking stage (active widget has Cooking stage type)
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        return DishWidget->GetStageType() == EDishCustomizationStageType::Cooking;
    }
    return false;
}

FVector UPUDishCustomizationComponent::GetSpawnPositionAboveStation() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return FVector::ZeroVector;
    }

    constexpr float SpawnHeightOffset = 40.f;
    // Use DishContainer mesh component so we spawn directly above the bowl, not the station center (which may be over platform/collision)
    TArray<UStaticMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp && MeshComp->GetName().Contains(TEXT("DishContainer"), ESearchCase::IgnoreCase))
        {
            FBoxSphereBounds DishBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
            // Spawn above center of dish container; use dish bounds for X,Y,Z so we're not over platform/rim
            float SurfaceHeight = DishBounds.Origin.Z + DishBounds.BoxExtent.Z;  // Top of bowl
            float SpawnHeight = SurfaceHeight + SpawnHeightOffset;
            return FVector(DishBounds.Origin.X, DishBounds.Origin.Y, SpawnHeight);
        }
    }

    // Fallback: use station bounds
    FBox StationBoundsBox = OwnerActor->GetComponentsBoundingBox();
    FVector Center = StationBoundsBox.GetCenter();
    FVector Extent = StationBoundsBox.GetExtent();
    float SurfaceHeight = Center.Z + Extent.Z;
    return FVector(Center.X, Center.Y, SurfaceHeight + SpawnHeightOffset);
}

bool UPUDishCustomizationComponent::GetPlateSurfaceHeight(float& OutSurfaceHeight) const
{
    FVector DummyCenter;
    return GetPlateSurfaceInfo(OutSurfaceHeight, DummyCenter);
}

bool UPUDishCustomizationComponent::GetPlateSurfaceInfo(float& OutSurfaceHeight, FVector& OutSurfaceCenter) const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    TArray<UStaticMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp && MeshComp->GetName().Contains(TEXT("DishContainer"), ESearchCase::IgnoreCase))
        {
            FBoxSphereBounds DishBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
            OutSurfaceHeight = DishBounds.Origin.Z + DishBounds.BoxExtent.Z;  // Top of bowl/plate
            OutSurfaceCenter = FVector(DishBounds.Origin.X, DishBounds.Origin.Y, OutSurfaceHeight);
            return true;
        }
    }

    return false;
}

void UPUDishCustomizationComponent::TransitionToPlatingStage(const FPUDishBase& DishData)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Transitioning to plating stage"));
    
    // Set plating mode
    SetPlatingMode(true);
    
    // Reset placement tracking for new plating session
    ResetPlatingPlacements();
    
    // Update the current dish data
    CurrentDishData = DishData;

    // Preload cap and material instances for procedural mesh ingredients (same pattern as prepped UI)
    // Ensures they're in memory before spawning - fixes materials not loading on game restart
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        const FPUIngredientBase& Ing = Instance.IngredientData;
        if (Ing.CapMaterialInstance.IsValid() || !Ing.CapMaterialInstance.ToSoftObjectPath().IsNull())
        {
            Ing.CapMaterialInstance.LoadSynchronous();
        }
        if (Ing.MaterialInstance.IsValid() || !Ing.MaterialInstance.ToSoftObjectPath().IsNull())
        {
            Ing.MaterialInstance.LoadSynchronous();
        }
    }
    
    // Switch to plating widget class if available
    if (PlatingWidgetClass)
    {
        // Store the original widget class before switching
        OriginalWidgetClass = CustomizationWidgetClass;
        CustomizationWidgetClass = PlatingWidgetClass;
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Stored original widget class and switched to plating widget class"));
        
        // Remove the current widget if it exists
        if (CustomizationWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Removing current widget"));
            CustomizationWidget->RemoveFromParent();
            CustomizationWidget = nullptr;
        }
        
        // Create the new plating widget
        UWorld* World = GetWorld();
        if (World)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating new plating widget"));
            CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
            
            if (CustomizationWidget)
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget created successfully"));
                
                // Try to cast to dish customization widget and set up the connection
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is UPUDishCustomizationWidget, connecting to component"));
                    DishWidget->SetCustomizationComponent(this);
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is not a UPUDishCustomizationWidget"));
                }
                
                // Add the widget to viewport
                CustomizationWidget->AddToViewport(PUDishCustomizationViewportZOrder);
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget added to viewport"));
                
                // Create plating ingredient buttons
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating plating ingredient slots"));
                    DishWidget->CreatePlatingIngredientSlots();
                }
            }
            else
            {
                //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - Failed to create plating widget"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - No world available"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - No plating widget class set"));
    }
    
    // Broadcast the dish data to the new widget
    BroadcastInitialDishData(DishData);

    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating stage transition complete"));
}

void UPUDishCustomizationComponent::CaptureScorecardSnapshotFromPlatingStation()
{
	if (AProjectUmeowmiCharacter* Pawn = CurrentCharacter)
	{
		if (Pawn->bEnableDishCaptureForScorecard)
		{
			Pawn->CaptureDishSnapshotFromPlatingStation(this);
		}
	}
}

void UPUDishCustomizationComponent::EndPlatingStage()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Ending plating stage"));

    // Capture transforms from live ingredient meshes BEFORE any cleanup (widget removal, mesh destruction)
    CapturePlatingTransformsFromMeshes();

    // Scorecard dish photo: live station shot (show-only dish + ingredients) before head preview / mesh cleanup.
    CaptureScorecardSnapshotFromPlatingStation();

    // Set plating mode to false
    SetPlatingMode(false);

    // Remove the plating widget from viewport
    if (CustomizationWidget)
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Removing plating widget from viewport"));
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }

    // Restore the original widget class
    if (OriginalWidgetClass)
    {
        CustomizationWidgetClass = OriginalWidgetClass;
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Restored original widget class"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::EndPlatingStage - No original widget class stored"));
    }

    // Clear the original widget class reference
    OriginalWidgetClass = nullptr;

    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Plating stage ended"));
}

// Plating placement limits implementation
bool UPUDishCustomizationComponent::CanPlaceIngredient(int32 InstanceID) const
{
    // Find the ingredient instance
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            bool bCanPlace = PlacedQuantity < Instance.Quantity;
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d: Placed %d/%d, Can place: %s"), 
            //    InstanceID, PlacedQuantity, Instance.Quantity, bCanPlace ? TEXT("Yes") : TEXT("No"));
            
            return bCanPlace;
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d not found"), InstanceID);
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantity(int32 InstanceID) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            int32 Remaining = Instance.Quantity - PlacedQuantity;
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d: %d remaining"), 
            //    InstanceID, Remaining);
            
            return Remaining;
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d not found"), InstanceID);
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantity(int32 InstanceID) const
{
    if (const int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        return *PlacedQuantity;
    }
    return 0;
}

void UPUDishCustomizationComponent::PlaceIngredient(int32 InstanceID)
{
    int32 CurrentPlaced = GetPlacedQuantity(InstanceID);
    PlacedIngredientQuantities.Add(InstanceID, CurrentPlaced + 1);
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::PlaceIngredient - Instance %d: Placed %d"), 
    //    InstanceID, CurrentPlaced + 1);
}

void UPUDishCustomizationComponent::RemoveIngredient(int32 InstanceID)
{
    if (int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        if (*PlacedQuantity > 0)
        {
            (*PlacedQuantity)--;
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RemoveIngredient - Instance %d: Now placed %d"), 
            //    InstanceID, *PlacedQuantity);
        }
    }
}

void UPUDishCustomizationComponent::ResetPlatingPlacements()
{
    PlacedIngredientQuantities.Empty();
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlatingPlacements - Reset all placement tracking"));
}

// Blueprint-callable plating limits
bool UPUDishCustomizationComponent::CanPlaceIngredientByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return CanPlaceIngredient(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredientByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetRemainingQuantity(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetPlacedQuantity(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetPlacedQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

void UPUDishCustomizationComponent::UpdateIngredientSlotQuantity(int32 InstanceID)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Updating slot quantity for InstanceID: %d"), InstanceID);
    
    // Find the ingredient slot and update its quantity
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        // Find the slot with matching InstanceID
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID == InstanceID)
            {
                // Decrease the slot's remaining quantity
                IngredientSlot->DecreaseQuantity();
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Decreased quantity for slot (InstanceID: %d)"), InstanceID);
                return;
            }
        }
        
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No slot found for InstanceID: %d"), InstanceID);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No customization widget found"));
    }
}

void UPUDishCustomizationComponent::ResetPlating()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Resetting all plating"));
    
    // Clear all placed ingredient quantities
    PlacedIngredientQuantities.Empty();
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Cleared placed ingredient quantities"));
    
    // Reset all ingredient slot quantities (for plating stage)
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Found %d ingredient slots to reset"), 
        //    IngredientSlots.Num());
        
        // Reset each slot's quantity from the dish data
        // In plating mode, slots are created with ActiveIngredientArea location (not Plating)
        // So we reset all slots that have ingredients, regardless of location
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID != 0)
            {
                IngredientSlot->ResetQuantityFromDishData();
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Reset slot for InstanceID: %d (Location: %d)"), 
                //    IngredientSlot->GetIngredientInstance().InstanceID, (int32)IngredientSlot->GetLocation());
            }
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::ResetPlating - No customization widget found"));
    }
    
    // Clear all 3D ingredient meshes
    ClearAll3DIngredientMeshes();
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Plating reset complete"));
}

void UPUDishCustomizationComponent::CapturePlatingTransformsFromMeshes()
{
    UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - %d meshes, %d liquids to capture"), SpawnedIngredientMeshes.Num(), SpawnedLiquidComponents.Num());

    // Capture dish surface center for consistent ingredient placement when copying to preview
    float SurfaceHeight;
    if (GetPlateSurfaceInfo(SurfaceHeight, CurrentDishData.PlatingDishCenter))
    {
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - dish center (%.1f, %.1f, %.1f)"), CurrentDishData.PlatingDishCenter.X, CurrentDishData.PlatingDishCenter.Y, CurrentDishData.PlatingDishCenter.Z);
    }
    else
    {
        CurrentDishData.PlatingDishCenter = FVector::ZeroVector;
    }

    CurrentDishData.PlatingEntries.Empty();
    int32 Captured = 0;

    for (const TWeakObjectPtr<APUIngredientMesh>& WeakMesh : SpawnedIngredientMeshes)
    {
        APUIngredientMesh* IngredientMesh = WeakMesh.Get();
        if (!IngredientMesh)
        {
            continue;
        }

        const int32 InstanceID = IngredientMesh->GetPlatingInstanceID();
        if (InstanceID < 0)
        {
            UE_LOG(LogDishPreview, Warning, TEXT("CapturePlatingTransformsFromMeshes - mesh has invalid InstanceID %d, skipping"), InstanceID);
            continue;
        }

        const FVector WorldPos = IngredientMesh->GetActorLocation();
        const FRotator WorldRot = IngredientMesh->GetActorRotation();
        FVector WorldScale = IngredientMesh->GetActorScale3D();
        if (WorldScale.SizeSquared() < KINDA_SMALL_NUMBER)
        {
            WorldScale = FVector::OneVector;
        }

        CurrentDishData.SetIngredientPlating(InstanceID, WorldPos, WorldRot, WorldScale);
        FPUPlatingEntry Entry;
        Entry.InstanceID = InstanceID;
        Entry.Position = WorldPos;
        Entry.Rotation = WorldRot;
        Entry.Scale = WorldScale;
        Entry.bIsLiquid = false;
        // For chopped/minced ingredients, capture each piece's world transform so preview shows broken-apart layout
        if (IngredientMesh->IsChopped())
        {
            Entry.ChoppedPieceTransforms = IngredientMesh->GetChoppedPieceWorldTransforms();
        }
        CurrentDishData.PlatingEntries.Add(Entry);
        Captured++;
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - InstanceID %d at (%.1f, %.1f, %.1f)"), InstanceID, WorldPos.X, WorldPos.Y, WorldPos.Z);
    }

    for (const TPair<int32, TObjectPtr<UNiagaraComponent>>& Pair : SpawnedLiquidComponents)
    {
        const int32 InstanceID = Pair.Key;
        UNiagaraComponent* NiagaraComp = Pair.Value.Get();
        if (!NiagaraComp || !IsValid(NiagaraComp))
        {
            continue;
        }

        const FVector WorldPos = NiagaraComp->GetComponentLocation();
        CurrentDishData.SetIngredientPlating(InstanceID, WorldPos, FRotator::ZeroRotator, FVector::OneVector);
        FPUPlatingEntry Entry;
        Entry.InstanceID = InstanceID;
        Entry.Position = WorldPos;
        Entry.Rotation = FRotator::ZeroRotator;
        Entry.Scale = FVector::OneVector;
        Entry.bIsLiquid = true;
        CurrentDishData.PlatingEntries.Add(Entry);
        Captured++;
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - liquid InstanceID %d at (%.1f, %.1f, %.1f)"), InstanceID, WorldPos.X, WorldPos.Y, WorldPos.Z);
    }

    if (Captured == 0)
    {
        for (const FIngredientInstance& Inst : CurrentDishData.IngredientInstances)
        {
            if (!Inst.bIsPlated)
            {
                continue;
            }
            FPUPlatingEntry Entry;
            Entry.InstanceID = Inst.InstanceID;
            Entry.Position = Inst.PlatingPosition;
            Entry.Rotation = Inst.PlatingRotation;
            Entry.Scale = Inst.PlatingScale;
            Entry.bIsLiquid = Inst.IngredientData.bIsLiquid;
            CurrentDishData.PlatingEntries.Add(Entry);
            Captured++;
        }
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - filled %d entries from plated instances (UI-only / no world meshes)"), Captured);
    }

    UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - captured %d transforms"), Captured);
}

void UPUDishCustomizationComponent::GatherDishSnapshotPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
    OutPrimitives.Reset();
    AActor* DishStation = GetOwner();
    if (!IsValid(DishStation))
    {
        return;
    }

    TArray<UStaticMeshComponent*> AllMeshComponents;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents);

    UStaticMeshComponent* DishContainer = nullptr;
    for (UStaticMeshComponent* MeshComp : AllMeshComponents)
    {
        if (!IsValid(MeshComp))
        {
            continue;
        }
        if (MeshComp->GetName().Contains(TEXT("DishContainer"), ESearchCase::IgnoreCase))
        {
            DishContainer = MeshComp;
            break;
        }
    }

    if (IsValid(DishContainer))
    {
        OutPrimitives.Add(DishContainer);
        TArray<USceneComponent*> Descendants;
        DishContainer->GetChildrenComponents(true, Descendants);
        for (USceneComponent* Child : Descendants)
        {
            if (!IsValid(Child))
            {
                continue;
            }
            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Child))
            {
                if (IsValid(Prim) && Prim->IsVisible())
                {
                    OutPrimitives.AddUnique(Prim);
                }
            }
        }
    }

    for (const TWeakObjectPtr<APUIngredientMesh>& WeakActor : SpawnedIngredientMeshes)
    {
        APUIngredientMesh* IngActor = WeakActor.Get();
        if (!IngActor)
        {
            continue;
        }
        // Same rule as CapturePlatingTransformsFromMeshes — invalid IDs correlate with broken/chopped state that can confuse component walks
        if (IngActor->GetPlatingInstanceID() < 0)
        {
            continue;
        }
        TArray<UPrimitiveComponent*> IngPrims;
        IngActor->GatherSnapshotPrimitiveComponents(IngPrims);
        for (UPrimitiveComponent* Prim : IngPrims)
        {
            if (IsValid(Prim) && Prim->IsVisible())
            {
                OutPrimitives.AddUnique(Prim);
            }
        }
    }

    for (const TPair<int32, TObjectPtr<UNiagaraComponent>>& Pair : SpawnedLiquidComponents)
    {
        UNiagaraComponent* NiagaraComp = Pair.Value.Get();
        if (NiagaraComp && IsValid(NiagaraComp) && NiagaraComp->IsVisible())
        {
            OutPrimitives.AddUnique(NiagaraComp);
        }
    }
}

void UPUDishCustomizationComponent::ClearAll3DIngredientMeshes()
{
    // Stop any active dragging before clearing
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        bIsDragging = false;
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
        }
        CurrentlyDraggedIngredient = nullptr;
    }
    
    // Destroy all tracked ingredient meshes (Weak.Get skips meshes already destroyed out-of-band, e.g. fall-through Tick)
    for (const TWeakObjectPtr<APUIngredientMesh>& WeakMesh : SpawnedIngredientMeshes)
    {
        if (APUIngredientMesh* IngredientMesh = WeakMesh.Get())
        {
            IngredientMesh->Destroy();
        }
    }
    SpawnedIngredientMeshes.Empty();
    
    // Destroy all tracked liquid Niagara components
    for (const TPair<int32, TObjectPtr<UNiagaraComponent>>& Pair : SpawnedLiquidComponents)
    {
        if (UNiagaraComponent* NiagaraComp = Pair.Value.Get())
        {
            if (IsValid(NiagaraComp))
            {
                NiagaraComp->Deactivate();
                NiagaraComp->DestroyComponent();
            }
        }
    }
    SpawnedLiquidComponents.Empty();
}
